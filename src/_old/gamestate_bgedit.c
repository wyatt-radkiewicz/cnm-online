#include <string.h>
#include <stdio.h>
#include "utility.h"
#include "game.h"
#include "renderer.h"
#include "background.h"
#include "gui.h"
#include "serial.h"
#include "input.h"
#include "command.h"
#include "game_console.h"
#include "blocks.h"

#define LAYER_BITMAP_ELEMENTS 21

#define FINDING_STATE_POS 0
#define FINDING_STATE_SIZE 1
#define FINDING_STATE_EDIT 2

static int finding_state = 0;
static int finding_x = 0;
static int finding_y = 0;
static int finding_layer = 0;
static int is_finding = CNM_FALSE;
static int finding_cam_x = 0;
static int finding_cam_y = 0;
static int finding_origin_x = 0;
static int finding_origin_y = 0;


static int is_previewing;
static int show_blocks;
static char loaded_file[64];
static GUI_FRAME *root, *load_frame, *save_frame;
static GUI_FRAME *layer_frames[BACKGROUND_MAX_LAYERS];
static int *camx, *camy;
static int loading = CNM_FALSE;

static void ReloadGui(void);

static void LoadCallback(GUI_ELEMENT *elem, int index)
{
	char command_buf[UTIL_MAX_TEXT_WIDTH + 1];
	if (!loading)
	{
		loading = CNM_TRUE;
		strcpy(loaded_file, elem->props.string);
		sprintf(command_buf, "load_blocks %s", loaded_file);
		Command_Execute(command_buf, CNM_FALSE);
		Gui_SwitchBack();
		strcpy(save_frame->elements[0].props.string, elem->props.string);
		ReloadGui();
		loading = CNM_FALSE;
	}
}
static void SaveCallback(GUI_ELEMENT *elem, int index)
{
	char command_buf[UTIL_MAX_TEXT_WIDTH + 1];
	static int saving = CNM_FALSE;
	if (!saving)
	{
		saving = CNM_TRUE;
		sprintf(command_buf, "save_blocks %s", elem->props.string);
		Command_Execute(command_buf, CNM_FALSE);
		Gui_SwitchBack();
		saving = CNM_FALSE;
	}
}
static void BackToMainMenu(GUI_ELEMENT *elem, int index)
{
	Game_SwitchState(GAME_STATE_MAINMENU);
}
static void PreviewToggleCallback(GUI_ELEMENT *elem, int index)
{
	is_previewing = CNM_TRUE;
	Gui_Losefocus();
}
static void ShowBlocksCallback(GUI_ELEMENT *elem, int index)
{
	show_blocks = elem->active;
}
static void OriginXCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->origin[0] = Gui_GetFloatElementFloat(elem->frame, index);
}
static void OriginYCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->origin[1] = Gui_GetFloatElementFloat(elem->frame, index);
}
static void ScrollXCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->scroll[0] = Gui_GetFloatElementFloat(elem->frame, index);
}
static void ScrollYCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->scroll[1] = Gui_GetFloatElementFloat(elem->frame, index);
}
static void SpeedXCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->speed[0] = Gui_GetFloatElementFloat(elem->frame, index);
}
static void SpeedYCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->speed[1] = Gui_GetFloatElementFloat(elem->frame, index);
}
static void HRepeatCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->repeat[0] = elem->props.set_index;
}
static void VRepeatCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->repeat[1] = elem->props.set_index;
}
static void SpacingXCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->spacing[0] = Gui_GetNumberElementInt(elem->frame, index);
}
static void SpacingYCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->spacing[1] = Gui_GetNumberElementInt(elem->frame, index);
}
static void ClearColorCallback(GUI_ELEMENT *elem, int index)
{
	if (loading)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->clear_color = Gui_GetNumberElementInt(elem->frame, index);
}
static void BitmapChangeCallback(GUI_ELEMENT *elem, int index)
{
	if (loading || is_finding)
		return;
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->bitmap.x = Gui_GetNumberElementInt(elem->frame, LAYER_BITMAP_ELEMENTS + 0);
	l->bitmap.y = Gui_GetNumberElementInt(elem->frame, LAYER_BITMAP_ELEMENTS + 1);
	l->bitmap.w = Gui_GetNumberElementInt(elem->frame, LAYER_BITMAP_ELEMENTS + 2);
	l->bitmap.h = Gui_GetNumberElementInt(elem->frame, LAYER_BITMAP_ELEMENTS + 3);
	GUI_ELEMENT *bitmap_elem = elem->frame->elements + LAYER_BITMAP_ELEMENTS + 6;
	memcpy(&bitmap_elem->props.bitmap, &l->bitmap, sizeof(CNM_RECT));
}
static void ReloadGui(void)
{
	for (int i = 0; i < BACKGROUND_MAX_LAYERS; i++)
	{
		BACKGROUND_LAYER *layer = Background_GetLayer(i);
		Gui_SetFloatElementFloat(layer_frames[i], 2, layer->origin[0]);
		Gui_SetFloatElementFloat(layer_frames[i], 3, layer->origin[1]);
		Gui_SetFloatElementFloat(layer_frames[i], 5, layer->scroll[0]);
		Gui_SetFloatElementFloat(layer_frames[i], 6, layer->scroll[1]);
		Gui_SetFloatElementFloat(layer_frames[i], 8, layer->speed[0]);
		Gui_SetFloatElementFloat(layer_frames[i], 9, layer->speed[1]);
		Gui_SetNumberElementInt(layer_frames[i], 11, layer->spacing[0]);
		Gui_SetNumberElementInt(layer_frames[i], 12, layer->spacing[1]);
		layer_frames[i]->elements[14].props.set_index = layer->repeat[0];
		layer_frames[i]->elements[15].props.set_index = layer->repeat[1];
		Gui_SetNumberElementInt(layer_frames[i], 17, layer->clear_color);
		layer_frames[i]->elements[18].props.set_index = layer->high;
		Gui_SetNumberElementInt(layer_frames[i], 19, layer->transparency);
		Gui_SetNumberElementInt(layer_frames[i], LAYER_BITMAP_ELEMENTS + 0, layer->bitmap.x);
		Gui_SetNumberElementInt(layer_frames[i], LAYER_BITMAP_ELEMENTS + 1, layer->bitmap.y);
		Gui_SetNumberElementInt(layer_frames[i], LAYER_BITMAP_ELEMENTS + 2, layer->bitmap.w);
		Gui_SetNumberElementInt(layer_frames[i], LAYER_BITMAP_ELEMENTS + 3, layer->bitmap.h);
		memcpy(&layer_frames[i]->elements[LAYER_BITMAP_ELEMENTS + 6].props.bitmap, &layer->bitmap, sizeof(CNM_RECT));
	}
}
static void BackgroundVisibleLayersCallback(GUI_ELEMENT *elem, int index)
{
	Background_SetVisibleLayers
	(
		Gui_GetNumberElementInt(root, 8),
		Gui_GetNumberElementInt(root, 9)
	);
}
static void FindBitmapCallback(GUI_ELEMENT *elem, int index)
{
	BACKGROUND_LAYER *l;

	finding_layer = Gui_GetCurrentFrame()->custom_hint;
	l = Background_GetLayer(finding_layer);
	is_finding = CNM_TRUE;
	finding_state = FINDING_STATE_POS;
	finding_x = l->bitmap.x;
	finding_y = l->bitmap.y;
	finding_origin_x = finding_x;
	finding_origin_y = finding_y;
	finding_cam_x = finding_x;
	finding_cam_y = finding_y;
	Gui_Losefocus();
}
static void HighPriorityCallback(GUI_ELEMENT *elem, int index) {
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->high = elem->props.set_index;
}
static void TransparencyCallback(GUI_ELEMENT *elem, int index) {
	BACKGROUND_LAYER *l = Background_GetLayer(elem->frame->custom_hint);
	l->transparency = Gui_GetNumberElementInt(elem->frame, index);
}

void GameState_BgEdit_Init(void)
{
	GUI_FRAME_PROPS props;

	show_blocks = CNM_TRUE;
	is_previewing = CNM_FALSE;
	is_finding = CNM_FALSE;
	camx = &Game_GetVar(GAME_VAR_CAMERA_X)->data.integer;
	camy = &Game_GetVar(GAME_VAR_CAMERA_Y)->data.integer;
	*camx = 0;
	*camy = 0;

	Background_SetVisibleLayers(0, BACKGROUND_MAX_LAYERS - 1);

	Gui_Reset();
	
	props.top = 16;
	props.line_count = 25;
	props.align[0] = GUI_ALIGN_LEFT;
	props.align[1] = GUI_ALIGN_RIGHT;
	props.bounds[0] = 16;
	props.bounds[1] = RENDERER_WIDTH - 16;
	
	root = Gui_CreateFrame(10 + BACKGROUND_MAX_LAYERS, 11 + BACKGROUND_MAX_LAYERS + 5, NULL, &props, 0);
	save_frame = Gui_CreateFrame(1, 1, root, &props, 0);
	load_frame = Gui_CreateFrame(1, 1, root, &props, 0);
	
	Gui_InitHeaderElement(root, 0, "=== BACKGROUND EDITOR ===");
	Gui_InitButtonElement(root, 1, NULL, "LOAD BACKGROUND", load_frame, CNM_FALSE);
	Gui_InitButtonElement(root, 2, NULL, "SAVE BACKGROUND", save_frame, CNM_FALSE);
	Gui_InitButtonElement(root, 3, PreviewToggleCallback, "PREVIEW BACKGROUND", NULL, CNM_FALSE);
	Gui_InitButtonElement(root, 4, ShowBlocksCallback, "SHOW BLOCKS", NULL, CNM_TRUE);
	Gui_InitButtonElement(root, 5, BackToMainMenu, "GO BACK TO MAIN MENU", NULL, CNM_FALSE);
	Gui_InitNullElement(root, 7);
	Gui_InitNumberElement(root, 8, BackgroundVisibleLayersCallback, "VISIBLE LAYERS START: ", 0, BACKGROUND_MAX_LAYERS - 1, 0);
	Gui_InitNumberElement(root, 9, BackgroundVisibleLayersCallback, "VISIBLE LAYERS END: ", 0, BACKGROUND_MAX_LAYERS - 1, BACKGROUND_MAX_LAYERS - 1);
	Gui_InitNullElement(root, 10);

	char layer_text[64];
	for (int i = 0; i < BACKGROUND_MAX_LAYERS; i++)
	{
		BACKGROUND_LAYER *layer = Background_GetLayer(i);

		layer_frames[i] = Gui_CreateFrame(22 + 16, 64, root, &props, i);
		sprintf(layer_text, "<BG LAYER %d>", i);
		Gui_InitButtonElement(root, 10 + i, NULL, layer_text, layer_frames[i], CNM_FALSE);

		sprintf(layer_text, "=== BACKGROUND LAYER %d ===", i);
		Gui_InitHeaderElement(layer_frames[i], 0, layer_text);
		Gui_InitNullElement(layer_frames[i], 1);
		
		Gui_InitFloatElement(layer_frames[i], 2, OriginXCallback, "ORIGIN X: ", -10000000.0f, 100000000.0f, layer->origin[0]);
		Gui_InitFloatElement(layer_frames[i], 3, OriginYCallback, "ORIGIN Y: ", -10000000.0f, 100000000.0f, layer->origin[1]);
		Gui_InitNullElement(layer_frames[i], 4);

		Gui_InitFloatElement(layer_frames[i], 5, ScrollXCallback, "SCROLL X: ", -10000000.0f, 100000000.0f, layer->scroll[0]);
		Gui_InitFloatElement(layer_frames[i], 6, ScrollYCallback, "SCROLL Y: ", -10000000.0f, 100000000.0f, layer->scroll[1]);
		Gui_InitNullElement(layer_frames[i], 7);

		Gui_InitFloatElement(layer_frames[i], 8, SpeedXCallback, "SPEED X: ", -10000000.0f, 100000000.0f, layer->speed[0]);
		Gui_InitFloatElement(layer_frames[i], 9, SpeedYCallback, "SPEED Y: ", -10000000.0f, 100000000.0f, layer->speed[1]);
		Gui_InitNullElement(layer_frames[i], 10);

		Gui_InitNumberElement(layer_frames[i], 11, SpacingXCallback, "SPACING X: ", 0, 100000000, layer->spacing[0]);
		Gui_InitNumberElement(layer_frames[i], 12, SpacingYCallback, "SPACING Y: ", 0, 100000000, layer->spacing[1]);
		Gui_InitNullElement(layer_frames[i], 13);

		Gui_InitSetElement(layer_frames[i], 14, HRepeatCallback, "H. REPEAT METHOD: ");
		Gui_AddItemToSet(layer_frames[i], 14, "NO REPEAT");
		Gui_AddItemToSet(layer_frames[i], 14, "FULL REPEAT");
		layer_frames[i]->elements[14].props.set_index = layer->repeat[0];

		Gui_InitSetElement(layer_frames[i], 15, VRepeatCallback, "V. REPEAT METHOD: ");
		Gui_AddItemToSet(layer_frames[i], 15, "NO REPEAT");
		Gui_AddItemToSet(layer_frames[i], 15, "REPEAT DOWN");
		Gui_AddItemToSet(layer_frames[i], 15, "REPEAT UP");
		Gui_AddItemToSet(layer_frames[i], 15, "FULL REPEAT");
		layer_frames[i]->elements[15].props.set_index = layer->repeat[1];
		Gui_InitNullElement(layer_frames[i], 16);

		Gui_InitNumberElement(layer_frames[i], 17, ClearColorCallback, "CLEAR COLOR: ", 0, 255, layer->clear_color);
		Gui_InitSetElement(layer_frames[i], 18, HighPriorityCallback, "HIGH PRIORITY: ");
		Gui_AddItemToSet(layer_frames[i], 18, "NO");
		Gui_AddItemToSet(layer_frames[i], 18, "YES");
		layer_frames[i]->elements[18].props.set_index = layer->high;
		Gui_InitNumberElement(layer_frames[i], 19, TransparencyCallback, "TRANSPARENCY: ", 0, 7, layer->transparency);
		Gui_InitNullElement(layer_frames[i], 20);

		Gui_InitNumberElement(layer_frames[i], LAYER_BITMAP_ELEMENTS + 0, BitmapChangeCallback, "BITMAP X: ", 0, 512, layer->bitmap.x);
		Gui_InitNumberElement(layer_frames[i], LAYER_BITMAP_ELEMENTS + 1, BitmapChangeCallback, "BITMAP Y: ", 0, 10000000, layer->bitmap.y);
		Gui_InitNumberElement(layer_frames[i], LAYER_BITMAP_ELEMENTS + 2, BitmapChangeCallback, "BITMAP W: ", 0, 512, layer->bitmap.w);
		Gui_InitNumberElement(layer_frames[i], LAYER_BITMAP_ELEMENTS + 3, BitmapChangeCallback, "BITMAP H: ", 0, 10000000, layer->bitmap.h);
		Gui_InitButtonElement(layer_frames[i], LAYER_BITMAP_ELEMENTS + 4, FindBitmapCallback, "FIND BITMAP", NULL, CNM_FALSE);
		Gui_InitNullElement(layer_frames[i], LAYER_BITMAP_ELEMENTS + 5);

		Gui_InitBitmapElement(layer_frames[i], LAYER_BITMAP_ELEMENTS + 6, 0, layer->bitmap.x, layer->bitmap.y, layer->bitmap.w, layer->bitmap.h);
		for (int j = 0; j < 16; j++)
			Gui_InitPaddingElement(layer_frames[i], LAYER_BITMAP_ELEMENTS + 7 + j);
	}

	Gui_InitStringElement(save_frame, 0, SaveCallback, "SAVE TO: ", 23);
	Gui_InitStringElement(load_frame, 0, LoadCallback, "LOAD FROM: ", 23);

	memset(loaded_file, 0, sizeof(loaded_file));

	Gui_SetRoot(root);
	Gui_Focus();
}
void GameState_BgEdit_Quit(void)
{
	Gui_Reset();
	Gui_DestroyFrame(root);
}
void GameState_BgEdit_Update(void)
{
	Input_Update();
	GameConsole_Update();
	Gui_Update();

	if (!is_previewing && !is_finding)
	{
		if (!Gui_IsFocused())
			Gui_Focus();
	}
	else
	{
		if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING) && !Gui_IsFocused())
		{
			is_previewing = CNM_FALSE;
			is_finding = CNM_FALSE;
			Gui_Focus();
		}
	}

	if (is_previewing)
	{
		int cam_speed = 4;
		if (Input_GetButton(INPUT_S, INPUT_STATE_PLAYING))
			cam_speed = 2;
		if (Input_GetButton(INPUT_W, INPUT_STATE_PLAYING))
			cam_speed *= 3;
		if (Input_GetButton(INPUT_D, INPUT_STATE_PLAYING))
			cam_speed *= 2;
		if (Input_GetButton(INPUT_A, INPUT_STATE_PLAYING))
			cam_speed *= 2;
		if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING))
			*camx += cam_speed;
		if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING))
			*camx -= cam_speed;
		if (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING))
			*camy += cam_speed;
		if (Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING))
			*camy -= cam_speed;
	}
	else if (is_finding)
	{
		int cam_speed = 1;
		if (Input_GetButton(INPUT_S, INPUT_STATE_PLAYING) || Input_GetButton(INPUT_W, INPUT_STATE_PLAYING) ||
			Input_GetButton(INPUT_D, INPUT_STATE_PLAYING) || Input_GetButton(INPUT_A, INPUT_STATE_PLAYING))
			cam_speed = 16;
		if (Input_GetButton(INPUT_W, INPUT_STATE_PLAYING))
			cam_speed *= 2;
		if (Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_PLAYING))
			finding_x += cam_speed;
		if (Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_PLAYING))
			finding_x -= cam_speed;
		if (Input_GetButtonPressedRepeated(INPUT_DOWN, INPUT_STATE_PLAYING))
			finding_y += cam_speed;
		if (Input_GetButtonPressedRepeated(INPUT_UP, INPUT_STATE_PLAYING))
			finding_y -= cam_speed;
		
		if (Input_GetButtonPressed(INPUT_DROP, INPUT_STATE_PLAYING))
		{
			finding_x = (finding_x / 32) * 32;
			finding_y = (finding_y / 32) * 32;
		}

		if (finding_x < 0)
			finding_x = 0;
		if (finding_x > 511)
			finding_x = 511;
		if (finding_y < 0)
			finding_y = 0;
		if (finding_y >= Renderer_GetBitmapHeight())
			finding_y = Renderer_GetBitmapHeight() - 1;

		if (finding_x > finding_cam_x + RENDERER_WIDTH - 48)
			finding_cam_x = finding_x - (RENDERER_WIDTH - 48);
		if (finding_x < finding_cam_x + 48)
			finding_cam_x = finding_x - 48;
		if (finding_y > finding_cam_y + RENDERER_HEIGHT - 48)
			finding_cam_y = finding_y - (RENDERER_HEIGHT - 48);
		if (finding_y < finding_cam_y + 48)
			finding_cam_y = finding_y - 48;
		if (finding_cam_x < 0)
			finding_cam_x = 0;
		if (finding_cam_y < 0)
			finding_cam_y = 0;
		if (finding_cam_x > 512 - RENDERER_WIDTH)
			finding_cam_x = 512 - RENDERER_WIDTH;
		if (finding_cam_y > Renderer_GetBitmapHeight() - RENDERER_HEIGHT)
			finding_cam_y = Renderer_GetBitmapHeight() - RENDERER_HEIGHT;

		if (finding_state == FINDING_STATE_POS)
		{
			finding_origin_x = finding_x;
			finding_origin_y = finding_y;
		}

		if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING))
		{
			if (finding_state == FINDING_STATE_POS)
			{
				finding_state = FINDING_STATE_SIZE;
			}
			else if (finding_state == FINDING_STATE_SIZE)
			{
				BACKGROUND_LAYER *flayer = Background_GetLayer(finding_layer);
				flayer->bitmap.x = finding_origin_x;
				flayer->bitmap.y = finding_origin_y;
				flayer->bitmap.w = finding_x - finding_origin_x;
				flayer->bitmap.h = finding_y - finding_origin_y;
				Gui_Focus();
				Gui_SetNumberElementInt(Gui_GetCurrentFrame(), LAYER_BITMAP_ELEMENTS + 0, flayer->bitmap.x);
				Gui_SetNumberElementInt(Gui_GetCurrentFrame(), LAYER_BITMAP_ELEMENTS + 1, flayer->bitmap.y);
				Gui_SetNumberElementInt(Gui_GetCurrentFrame(), LAYER_BITMAP_ELEMENTS + 2, flayer->bitmap.w);
				Gui_SetNumberElementInt(Gui_GetCurrentFrame(), LAYER_BITMAP_ELEMENTS + 3, flayer->bitmap.h);
				GUI_ELEMENT *bitmap_elem = Gui_GetCurrentFrame()->elements + LAYER_BITMAP_ELEMENTS + 6;
				memcpy(&bitmap_elem->props.bitmap, &flayer->bitmap, sizeof(CNM_RECT));
				is_finding = CNM_FALSE;
			}
		}
	}
}
void GameState_BgEdit_Draw(void)
{
	CNM_RECT r;

	if (is_previewing)
	{
		Background_Draw(0, *camx, *camy);

		if (show_blocks)
		{
			Blocks_DrawBlocks(BLOCKS_BG, *camx, *camy);
			Blocks_DrawBlocks(BLOCKS_FG, *camx, *camy);
		}

		Background_Draw(1, *camx, *camy);
	}
	else if (is_finding)
	{
		Renderer_Clear(RCOL_LIGHT_BLUE);
		Util_SetRect(&r, finding_cam_x, finding_cam_y, RENDERER_WIDTH, RENDERER_HEIGHT);
		Renderer_DrawBitmap(0, 0, &r, 0, RENDERER_LIGHT);
		if (finding_state == FINDING_STATE_SIZE)
		{
			Util_SetRect
			(
				&r,
				finding_origin_x - finding_cam_x,
				finding_origin_y - finding_cam_y,
				finding_x - finding_origin_x,
				finding_y - finding_origin_y
			);
			Renderer_DrawRect(&r, RCOL_YELLOW, 5, RENDERER_LIGHT);
		}
		Util_SetRect(&r, 376, 1248, 8, 8);
		Renderer_DrawBitmap
		(
			finding_x - finding_cam_x - 3,
			finding_y - finding_cam_y - 3,
			&r, 0, RENDERER_LIGHT
		);
		Renderer_DrawText(0, RENDERER_HEIGHT - 16, 0, RENDERER_LIGHT, "POSITION: (%d, %d)", finding_x, finding_y);
		if (finding_state == FINDING_STATE_SIZE)
			Renderer_DrawText(0, RENDERER_HEIGHT - 8, 0, RENDERER_LIGHT, "SIZE: (%d, %d)", finding_x - finding_origin_x, finding_y - finding_origin_y);
	}
	else
	{
		Renderer_Clear(RCOL_LIGHT_BLUE);
	}

	Gui_Draw();
	GameConsole_Draw();
	Renderer_Update();
}
