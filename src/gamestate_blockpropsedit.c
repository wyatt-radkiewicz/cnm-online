#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "console.h"
#include "renderer.h"
#include "blocks.h"
#include "serial.h"
#include "game_console.h"
#include "input.h"
#include "gui.h"
#include "command.h"
#include "game.h"

#define MAX_BLOCKPROPS 4096
#define HEADER_ELEMENTS 5
#define PREVIEW_ELEMENTS 4
#define EDIT_HEADER_ELEMENTS 14

#define FIND_W (RENDERER_WIDTH / BLOCK_SIZE)
#define FIND_H (RENDERER_HEIGHT / BLOCK_SIZE)

static GUI_FRAME *previews, *lframe, *sframe;
static GUI_FRAME **block_props = NULL;
static GUI_FRAME **block_coll_hitbox = NULL;
static int initing_previews = CNM_FALSE;
static int finding = CNM_FALSE;
static int editing_hitbox = CNM_FALSE;
static int editing_heightmap = CNM_FALSE;
static int find_x = 0;
static int find_y = 0;
static int finding_frame = 0;
static int finding_block = 0;
static int find_camx = 0;
static int find_camy = 0;

static int heightmap_x = 0;
static int heightmap_block_id = 0;

static void InitializeBlockPropPreviews(void);

static void SaveBlocksElementString(GUI_ELEMENT *elem, int index)
{
	static int saving = CNM_FALSE;
	char buffer[128] = {'\0'};
	if (!saving)
	{
		saving = CNM_TRUE;
		strcpy(buffer, "save_blocks ");
		strcat(buffer, elem->props.string);
		Command_Execute(buffer, CNM_FALSE);
		Gui_SwitchBack();
		saving = CNM_FALSE;
	}
}
static void LoadBlocksElementString(GUI_ELEMENT *elem, int index)
{
	static int loading = CNM_FALSE;
	char buffer[128] = {'\0'};
	if (!loading)
	{
		loading = CNM_TRUE;
		strcpy(buffer, "load_blocks ");
		strcat(buffer, elem->props.string);
		Command_Execute(buffer, CNM_FALSE);
		strcpy(sframe->elements[0].props.string, elem->props.string);
		memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
		Gui_SwitchBack();
		InitializeBlockPropPreviews();
		loading = CNM_FALSE;
	}
}
static void GoBackToMainMenu(GUI_ELEMENT *elem, int index)
{
	Game_SwitchState(GAME_STATE_MAINMENU);
}

static void UpdateBlockPropsEditableFrames(GUI_FRAME *frame);

static void ChangeBlockTransparency(GUI_ELEMENT *elem, int index)
{
	if (!initing_previews)
		Blocks_GetBlockProp(Gui_GetCurrentFrame()->custom_hint)->transparency = Gui_GetNumberElementInt(Gui_GetCurrentFrame(), index);
}
static void ChangeBlockDamageType(GUI_ELEMENT *elem, int index)
{
	if (!initing_previews)
		Blocks_GetBlockProp(Gui_GetCurrentFrame()->custom_hint)->dmg_type = elem->props.set_index;
}
static void ChangeBlockDamage(GUI_ELEMENT *elem, int index)
{
	if (!initing_previews)
		Blocks_GetBlockProp(Gui_GetCurrentFrame()->custom_hint)->dmg = Gui_GetNumberElementInt(Gui_GetCurrentFrame(), index);
}
static void ChangeBlockAnimSpeed(GUI_ELEMENT *elem, int index)
{
	if (!initing_previews)
		Blocks_GetBlockProp(Gui_GetCurrentFrame()->custom_hint)->anim_speed = Gui_GetNumberElementInt(Gui_GetCurrentFrame(), index);
}
static void ChangeBlockFrameCount(GUI_ELEMENT *elem, int index)
{
	BLOCK_PROPS *bp;

	if (!initing_previews)
	{
		bp = Blocks_GetBlockProp(Gui_GetCurrentFrame()->custom_hint);

		int old_num_frames = bp->num_frames;
		bp->num_frames = Gui_GetNumberElementInt(Gui_GetCurrentFrame(), index);
		if (bp->num_frames > old_num_frames)
		{
			memset(bp->frames_x + old_num_frames, 0, (bp->num_frames - old_num_frames) * sizeof(int));
			memset(bp->frames_y + old_num_frames, 0, (bp->num_frames - old_num_frames) * sizeof(int));
		}
		UpdateBlockPropsEditableFrames(Gui_GetCurrentFrame());
	}
}
static void ChangeBlockFrameX(GUI_ELEMENT *elem, int index)
{
	if (!initing_previews)
	{
		Blocks_GetBlockProp(Gui_GetCurrentFrame()->custom_hint)->frames_x[elem->custom_hint] = Gui_GetNumberElementInt(Gui_GetCurrentFrame(), index);
		Gui_GetCurrentFrame()->elements[index - 1].props.bitmap.x = Gui_GetNumberElementInt(Gui_GetCurrentFrame(), index) * BLOCK_SIZE;
	}
}
static void ChangeBlockFrameY(GUI_ELEMENT *elem, int index)
{
	if (!initing_previews)
	{
		Blocks_GetBlockProp(Gui_GetCurrentFrame()->custom_hint)->frames_y[elem->custom_hint] = Gui_GetNumberElementInt(Gui_GetCurrentFrame(), index);
		Gui_GetCurrentFrame()->elements[index - 2].props.bitmap.y = Gui_GetNumberElementInt(Gui_GetCurrentFrame(), index) * BLOCK_SIZE;
	}
}
static void StartFindBitmap(GUI_ELEMENT *elem, int index)
{
	finding = CNM_TRUE;
	find_x = Gui_GetNumberElementInt(Gui_GetCurrentFrame(), index - 2);
	find_y = Gui_GetNumberElementInt(Gui_GetCurrentFrame(), index - 1);
	finding_frame = elem->custom_hint;
	finding_block = Gui_GetCurrentFrame()->custom_hint;
}
static void ChangeBlockIsSolid(GUI_ELEMENT *elem, int index)
{
	if (!initing_previews)
	{
		if (elem->active)
			Blocks_GetBlockProp(elem->frame->custom_hint)->flags |= BLOCK_FLAG_SOLID;
		else
			Blocks_GetBlockProp(elem->frame->custom_hint)->flags &= ~BLOCK_FLAG_SOLID;
	}
}
static void ChangeBlockCollType(GUI_ELEMENT *elem, int index)
{
	if (!initing_previews)
	{
		BLOCK_PROPS *bp = Blocks_GetBlockProp(elem->frame->custom_hint);
		bp->coll_type = elem->props.set_index;

		if (bp->coll_type == BLOCKS_COLL_HEIGHT)
		{
			memset(bp->coll_data.heightmap, 0, sizeof(bp->coll_data.heightmap));
		}
		else if (bp->coll_type == BLOCKS_COLL_BOX || bp->coll_type == BLOCKS_COLL_JT)
		{
			Util_SetRect(&bp->coll_data.hitbox, 0, 0, BLOCK_SIZE, BLOCK_SIZE);
		}

		elem->frame->elements[index + 1].props.btn_link = (bp->coll_type == BLOCKS_COLL_HEIGHT ? NULL : block_coll_hitbox[elem->frame->custom_hint]);
	}
}
static void BlockChangeHitboxSize(GUI_ELEMENT *elem, int index)
{
	if (!initing_previews)
	{
		Blocks_GetBlockProp(elem->frame->custom_hint)->coll_data.hitbox.x = Gui_GetNumberElementInt(elem->frame, 1);
		Blocks_GetBlockProp(elem->frame->custom_hint)->coll_data.hitbox.y = Gui_GetNumberElementInt(elem->frame, 2);
		Blocks_GetBlockProp(elem->frame->custom_hint)->coll_data.hitbox.w = Gui_GetNumberElementInt(elem->frame, 3);
		Blocks_GetBlockProp(elem->frame->custom_hint)->coll_data.hitbox.h = Gui_GetNumberElementInt(elem->frame, 4);
	}
}
static void SetHitboxEditingMode(GUI_ELEMENT *elem, int index)
{
	if (!initing_previews)
	{
		BLOCK_PROPS *bp = Blocks_GetBlockProp(elem->frame->custom_hint);

		if (bp->coll_type == BLOCKS_COLL_BOX || bp->coll_type == BLOCKS_COLL_JT)
		{
			editing_hitbox = CNM_TRUE;
		}
		else
		{
			heightmap_x = 0;
			heightmap_block_id = elem->frame->custom_hint;
			editing_heightmap = CNM_TRUE;
		}
	}
}
static void BlockHitboxEscapeCallback(GUI_FRAME *frame)
{
	editing_hitbox = CNM_FALSE;
	editing_heightmap = CNM_FALSE;
}

static void UpdateBlockPropsEditableFrames(GUI_FRAME *frame)
{
	BLOCK_PROPS *props;
	int i, index;

	props = Blocks_GetBlockProp(frame->custom_hint);
	frame->num_elements = EDIT_HEADER_ELEMENTS + props->num_frames * 4;
	for (i = 0, index = EDIT_HEADER_ELEMENTS; i < props->num_frames; i++)
	{
		Gui_InitBitmapElement(frame, index++, 0, props->frames_x[i] * BLOCK_SIZE, props->frames_y[i] * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);
		Gui_InitNumberElement(frame, index, ChangeBlockFrameX, "     FRAME X: ", 0, 15, props->frames_x[i]);
		frame->elements[index++].custom_hint = i;
		Gui_InitNumberElement(frame, index, ChangeBlockFrameY, "     FRAME Y: ", 0, 1000000, props->frames_y[i]);
		frame->elements[index++].custom_hint = i;
		Gui_InitButtonElement(frame, index, StartFindBitmap, "     FIND BITMAP", NULL, CNM_FALSE);
		frame->elements[index++].custom_hint = i;
	}
}

static void InitializeBlockPropPreviews(void)
{
	GUI_FRAME_PROPS props;
	BLOCK_PROPS *bp;
	char header_name[UTIL_MAX_TEXT_WIDTH + 1] = {'\0'};
	char preview_name[UTIL_MAX_TEXT_WIDTH + 1] = {'\0'};
	int i = 0, index;
	initing_previews = CNM_TRUE;
	previews->num_elements = Blocks_GetBlockPropsCount() * PREVIEW_ELEMENTS + HEADER_ELEMENTS;
	free(block_props);
	block_props = malloc(sizeof(GUI_FRAME *) * Blocks_GetBlockPropsCount());
	block_coll_hitbox = malloc(sizeof(GUI_FRAME *) * Blocks_GetBlockPropsCount());
	props.top = 32;
	props.line_count = 24;
	props.align[0] = GUI_ALIGN_LEFT;
	props.align[1] = GUI_ALIGN_RIGHT;
	props.bounds[0] = 32;
	props.bounds[1] = RENDERER_WIDTH - 32;
	for (i = 0; i < Blocks_GetBlockPropsCount(); i++)
	{
		bp = Blocks_GetBlockProp(i);

		sprintf(header_name, "=== BLOCK PROPS FOR ID %d ===", i);
		block_props[i] = Gui_CreateFrame(EDIT_HEADER_ELEMENTS, EDIT_HEADER_ELEMENTS + BLOCKS_MAX_FRAMES * 4, previews, &props, i);
		
		block_coll_hitbox[i] = Gui_CreateFrame(5, 10, block_props[i], &props, i);
		Gui_InitHeaderElement(block_coll_hitbox[i], 0, "=== BLOCK HITBOX ===");
		Gui_InitNumberElement(block_coll_hitbox[i], 1, BlockChangeHitboxSize, "HITBOX X: ", 0, BLOCK_SIZE - 1, bp->coll_data.hitbox.x);
		Gui_InitNumberElement(block_coll_hitbox[i], 2, BlockChangeHitboxSize, "HITBOX Y: ", 0, BLOCK_SIZE - 1, bp->coll_data.hitbox.y);
		Gui_InitNumberElement(block_coll_hitbox[i], 3, BlockChangeHitboxSize, "HITBOX W: ", 0, BLOCK_SIZE, bp->coll_data.hitbox.w);
		Gui_InitNumberElement(block_coll_hitbox[i], 4, BlockChangeHitboxSize, "HITBOX H: ", 0, BLOCK_SIZE, bp->coll_data.hitbox.h);
		block_coll_hitbox[i]->escape_callback = BlockHitboxEscapeCallback;

		Gui_InitHeaderElement(block_props[i], 0, header_name);
		Gui_InitBitmapElement(block_props[i], 1, 112, 0, 0, 0, 0);
		Gui_InitNullElement(block_props[i], 2);
		Gui_InitNullElement(block_props[i], 3);
		Gui_InitNullElement(block_props[i], 4);
		Gui_InitNullElement(block_props[i], 5);
		Gui_InitNumberElement(block_props[i], 6, ChangeBlockTransparency, "TRANSPARENCY: ", 0, 7, bp->transparency);
		Gui_InitSetElement(block_props[i], 7, ChangeBlockDamageType, "DAMAGE TYPE: ");
		Gui_AddItemToSet(block_props[i], 7, "NONE");
		Gui_AddItemToSet(block_props[i], 7, "LAVA");
		Gui_AddItemToSet(block_props[i], 7, "SPIKES");
		Gui_AddItemToSet(block_props[i], 7, "QUICKSAND");
		block_props[i]->elements[7].props.set_index = bp->dmg_type;
		Gui_InitNumberElement(block_props[i], 8, ChangeBlockDamage, "DAMAGE: ", -100000000, 100000000, bp->dmg);
		Gui_InitButtonElement(block_props[i], 9, ChangeBlockIsSolid, "SOLID", NULL, CNM_TRUE);
		block_props[i]->elements[9].active = bp->flags & BLOCK_FLAG_SOLID;
		Gui_InitSetElement(block_props[i], 10, ChangeBlockCollType, "COLLISION TYPE: ");
		Gui_AddItemToSet(block_props[i], 10, "HITBOX");
		Gui_AddItemToSet(block_props[i], 10, "HEIGHTMAP");
		Gui_AddItemToSet(block_props[i], 10, "JMPT");
		block_props[i]->elements[10].props.set_index = bp->coll_type;
		Gui_InitButtonElement(block_props[i], 11, SetHitboxEditingMode, "COLLISION DATA", (bp->coll_type == BLOCKS_COLL_HEIGHT ? NULL : block_coll_hitbox[i]), CNM_FALSE);
		Gui_InitNumberElement(block_props[i], 12, ChangeBlockAnimSpeed, "ANIMATION SPEED: ", 1, 30*60*60, bp->anim_speed);
		Gui_InitNumberElement(block_props[i], 13, ChangeBlockFrameCount, "NUMBER OF FRAMES: ", 1, BLOCKS_MAX_FRAMES, bp->num_frames);
		UpdateBlockPropsEditableFrames(block_props[i]);
	}

	for (i = 0; i < previews->num_elements - HEADER_ELEMENTS; i++)
	{
		index = i + HEADER_ELEMENTS;
		if (i % PREVIEW_ELEMENTS == 0)
		{
			sprintf(preview_name, "[%d]", i / PREVIEW_ELEMENTS);
			Gui_InitBitmapButtonElement
			(
				previews,
				index,
				NULL,
				preview_name,
				block_props[i / PREVIEW_ELEMENTS],
				CNM_FALSE,
				0,
				0,
				0,
				0,
				0
			);
		}
		else
		{
			Gui_InitNullElement(previews, index);
		}
	}

	initing_previews = CNM_FALSE;
}

void GameState_BlockPropsEdit_Init(void)
{
	GUI_FRAME_PROPS props;

	Gui_Reset();
	initing_previews = CNM_FALSE;
	finding = CNM_FALSE;
	editing_hitbox = CNM_FALSE;
	props.top = 32;
	props.line_count = 24;
	props.align[0] = GUI_ALIGN_LEFT;
	props.align[1] = GUI_ALIGN_RIGHT;
	props.bounds[0] = 32;
	props.bounds[1] = RENDERER_WIDTH - 32;
	previews = Gui_CreateFrame(HEADER_ELEMENTS + PREVIEW_ELEMENTS * MAX_BLOCKPROPS, PREVIEW_ELEMENTS * MAX_BLOCKPROPS, NULL, &props, -1);
	props.top = 96;
	block_props = NULL;
	sframe = Gui_CreateFrame(1, 1, previews, &props, -1);
	lframe = Gui_CreateFrame(1, 1, previews, &props, -1);
	
	Gui_InitStringElement(sframe, 0, SaveBlocksElementString, "SAVE PROPS: ", 23);
	Gui_InitStringElement(lframe, 0, LoadBlocksElementString, "LOAD PROPS: ", 23);

	Gui_InitHeaderElement(previews, 0, "===== BLOCK PROPERTIES EDITOR =====");
	Gui_InitButtonElement(previews, 1, NULL, "LOAD BLOCK PROPS", lframe, CNM_FALSE);
	Gui_InitButtonElement(previews, 2, NULL, "SAVE BLOCK PROPS", sframe, CNM_FALSE);
	Gui_InitButtonElement(previews, 3, GoBackToMainMenu, "GO BACK TO MAIN MENU", NULL, CNM_FALSE);
	Gui_InitHeaderElement(previews, 4, "===== BLOCK PROPERTIES =====");

	previews->num_elements = 5;

	Gui_SetRoot(previews);
	Gui_Focus();
	InitializeBlockPropPreviews();
}
void GameState_BlockPropsEdit_Quit(void)
{
	int i;
	Gui_Reset();
	Gui_DestroyFrame(previews);
	Gui_DestroyFrame(sframe);
	Gui_DestroyFrame(lframe);
	for (i = 0; i < Blocks_GetBlockPropsCount(); i++)
	{
		Gui_DestroyFrame(block_coll_hitbox[i]);
		Gui_DestroyFrame(block_props[i]);
	}
	free(block_props);
	free(block_coll_hitbox);
}
void GameState_BlockPropsEdit_Update(void)
{
	int i, index;
	CNM_RECT prev_src;
	Input_Update();
	GameConsole_Update();
	Gui_Update();
	if (!Gui_IsFocused() && !finding && !editing_heightmap)
		Gui_Focus();
	if (Gui_IsFocused() && (finding || editing_heightmap))
		Gui_Losefocus();

	if (Gui_GetCurrentFrame()->custom_hint == -1 && !finding)
	{
		for (i = previews->cam_index; i < previews->cam_index + 24; i++)
		{
			if (previews->elements[i].type == GUI_ELEMENT_BITMAP_BUTTON)
			{
				index = (i - HEADER_ELEMENTS) / PREVIEW_ELEMENTS;
				Blocks_GetBlockDrawRect(index, &prev_src);
				memcpy(&previews->elements[i].props.bitmap, &prev_src, sizeof(CNM_RECT));
				previews->elements[i].props.bitmap_trans = Blocks_GetBlockProp(index)->transparency;
			}
		}
	}
	else if (!finding)
	{
		Blocks_GetBlockDrawRect(Gui_GetCurrentFrame()->custom_hint, &prev_src);
		memcpy(&Gui_GetCurrentFrame()->elements[1].props.bitmap, &prev_src, sizeof(CNM_RECT));
		Gui_GetCurrentFrame()->elements[1].props.bitmap_trans = Blocks_GetBlockProp(Gui_GetCurrentFrame()->custom_hint)->transparency;
	}

	if (finding)
	{
		if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING))
		{
			finding = CNM_FALSE;
			Gui_SetNumberElementInt(Gui_GetCurrentFrame(), EDIT_HEADER_ELEMENTS + finding_frame * 4 + 1, find_x);
			Gui_SetNumberElementInt(Gui_GetCurrentFrame(), EDIT_HEADER_ELEMENTS + finding_frame * 4 + 2, find_y);
		}

		int find_max_y = Renderer_GetBitmapHeight() / BLOCK_SIZE;
		if (Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_PLAYING) && find_x < 15)
			find_x++;
		if (Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_PLAYING) && find_x > 0)
			find_x--;
		if (Input_GetButtonPressedRepeated(INPUT_DOWN, INPUT_STATE_PLAYING) && find_y < find_max_y - 1)
			find_y++;
		if (Input_GetButtonPressedRepeated(INPUT_UP, INPUT_STATE_PLAYING) && find_y > 0)
			find_y--;

		if (find_x + 1 >= find_camx + FIND_W)
			find_camx = find_x + 2 - FIND_W;
		if (find_x <= find_camx)
			find_camx = find_x - 1;
		if (find_y + 1 >= find_camy + FIND_H)
			find_camy = find_y + 2 - FIND_H;
		if (find_y <= find_camy)
			find_camy = find_y - 1;
		if (find_camx < 0)
			find_camx = 0;
		if (find_camy < 0)
			find_camy = 0;
		if (find_camx > 16 - FIND_W)
			find_camx = 16 - FIND_W;
		if (find_camy > find_max_y - FIND_H)
			find_camy = find_max_y - FIND_H;

		if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING))
		{
			finding = CNM_FALSE;
		}
	}

	if (editing_heightmap)
	{
		BLOCK_PROPS *bp = Blocks_GetBlockProp(heightmap_block_id);
		int *current = bp->coll_data.heightmap + heightmap_x;

		if (Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_PLAYING) && heightmap_x < BLOCK_SIZE - 1)
			heightmap_x++;
		if (Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_PLAYING) && heightmap_x > 0)
			heightmap_x--;

		if (Input_GetButtonPressedRepeated(INPUT_UP, INPUT_STATE_PLAYING) && *current < BLOCK_SIZE)
			(*current)++;
		if (Input_GetButtonPressedRepeated(INPUT_DOWN, INPUT_STATE_PLAYING) && *current > 0)
			(*current)--;

		if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING))
		{
			editing_heightmap = CNM_FALSE;
		}
	}
}
void GameState_BlockPropsEdit_Draw(void)
{
	CNM_RECT r;
	Renderer_Clear(Renderer_MakeColor(128, 128, 255));
	Gui_Draw();

	if (finding)
	{
		Util_SetRect(&r, find_camx * BLOCK_SIZE, find_camy * BLOCK_SIZE, RENDERER_WIDTH, RENDERER_HEIGHT);
		Renderer_DrawBitmap(0, 0, &r, 0, RENDERER_LIGHT);
		Util_SetRect(&r, 416, 1248, 32, 32);
		Renderer_DrawBitmap((find_x - find_camx) * BLOCK_SIZE, (find_y - find_camy) * BLOCK_SIZE, &r, 0, RENDERER_LIGHT);
		Renderer_DrawText(128, 240 - 12, 0, RENDERER_LIGHT, "FIND A BITMAP");
	}

	if (editing_hitbox)
	{
		int block = Gui_GetCurrentFrame()->custom_hint;
		r.x = Blocks_GetBlockProp(block)->frames_x[0] * BLOCK_SIZE;
		r.y = Blocks_GetBlockProp(block)->frames_y[0] * BLOCK_SIZE;
		r.w = BLOCK_SIZE;
		r.h = BLOCK_SIZE;

		Renderer_DrawBitmap(128, 128, &r, 0, RENDERER_LIGHT);

		r.x = 128 + Blocks_GetBlockProp(block)->coll_data.hitbox.x;
		r.y = 128 + Blocks_GetBlockProp(block)->coll_data.hitbox.y;
		r.w = Blocks_GetBlockProp(block)->coll_data.hitbox.w;
		r.h = Blocks_GetBlockProp(block)->coll_data.hitbox.h;
		Renderer_DrawRect(&r, Renderer_MakeColor(255, 0, 255), 4, RENDERER_LIGHT);
	}

	if (editing_heightmap)
	{
		BLOCK_PROPS *bp = Blocks_GetBlockProp(heightmap_block_id);
		
		r.x = bp->frames_x[0] * BLOCK_SIZE;
		r.y = bp->frames_y[0] * BLOCK_SIZE;
		//r.w = BLOCK_SIZE;
		//r.h = BLOCK_SIZE;
		//Renderer_DrawBitmap(128, 128, &r, 0, RENDERER_LIGHT);
		for (int x = 0; x < BLOCK_SIZE; x++)
		{
			for (int y = 0; y < BLOCK_SIZE; y++)
			{
				Renderer_PlotPixel(100 + (x * 2 + 0), 100 + (y * 2 + 0), Renderer_GetBitmapPixel(r.x + x, r.y + y));
				Renderer_PlotPixel(100 + (x * 2 + 1), 100 + (y * 2 + 0), Renderer_GetBitmapPixel(r.x + x, r.y + y));
				Renderer_PlotPixel(100 + (x * 2 + 0), 100 + (y * 2 + 1), Renderer_GetBitmapPixel(r.x + x, r.y + y));
				Renderer_PlotPixel(100 + (x * 2 + 1), 100 + (y * 2 + 1), Renderer_GetBitmapPixel(r.x + x, r.y + y));
			}
		}

		for (int x = 0; x < BLOCK_SIZE; x++)
		{
			//Util_SetRect(&r, 352 + (x == heightmap_x ? 1 : 0), 1248, 1, 1);
			int heightmap_edit_color = Renderer_GetBitmapPixel(352 + (x == heightmap_x ? 1 : 0), 1248);
			if (bp->coll_data.heightmap[x] == 0)
				continue;
			for (int y = bp->coll_data.heightmap[x]; y > 0; y--)
			{
				r.x = 100 + x * 2;
				r.y = 100 + (BLOCK_SIZE - y) * 2;
				r.w = 2;
				r.h = 2;
				Renderer_DrawRect
				(
					&r, heightmap_edit_color, 2, RENDERER_LIGHT
				);
				/*Renderer_DrawBitmap
				(
					128 + (x ),
					128 + (BLOCK_SIZE - y),
					&r,
					2,
					RENDERER_LIGHT
				);*/
			}
		}
	}
	
	GameConsole_Draw();
	Renderer_Update();
}
