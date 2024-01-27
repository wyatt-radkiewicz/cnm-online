#include <string.h>
#include "console.h"
#include "renderer.h"
#include "blocks.h"
#include "serial.h"
#include "game_console.h"
#include "input.h"
#include "gui.h"
#include "command.h"
#include "game.h"

static CNM_RECT cross_rect;
static int light = RENDERER_LIGHT;
static float cx = 0, cy = 0;
static GUI_FRAME *frame, *sframe, *lframe;

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
		loading = CNM_FALSE;
	}
}
static void GoBackToMainMenu(GUI_ELEMENT *elem, int index)
{
	Game_SwitchState(GAME_STATE_MAINMENU);
}

void GameState_LightEditor_Init(void)
{
	GUI_FRAME_PROPS props;
	Console_Print("TESTING 123");
	Console_Print("WELCOME TO CNM ONLINE!");
	Console_Print("help: to help do some cool things around the place!");

	Gui_Reset();
	props.top = 96;
	props.align[0] = GUI_ALIGN_CENTER;
	props.bounds[0] = 160;
	props.line_count = 6;
	frame = Gui_CreateFrame(4, 10, NULL, &props, 0);
	props.top = 96;
	props.align[0] = GUI_ALIGN_LEFT;
	props.bounds[0] = 16;
	props.line_count = 6;
	props.align[1] = GUI_ALIGN_LEFT;
	props.bounds[1] = props.bounds[0] + (11 * 8);
	sframe = Gui_CreateFrame(1, 1, frame, &props, 0);
	lframe = Gui_CreateFrame(1, 1, frame, &props, 0);
	Gui_InitStringElement(lframe, 0, LoadBlocksElementString, "LOAD FILE: ", 23);
	Gui_InitStringElement(sframe, 0, SaveBlocksElementString, "SAVE FILE: ", 23);
	Gui_InitHeaderElement(frame, 0, "===== LIGHT EDITOR =====");
	Gui_InitButtonElement(frame, 1, NULL, "LOAD LIGHT", lframe, CNM_FALSE);
	Gui_InitButtonElement(frame, 2, NULL, "SAVE LIGHT", sframe, CNM_FALSE);
	Gui_InitButtonElement(frame, 3, GoBackToMainMenu, "GO BACK TO MAIN MENU", NULL, CNM_FALSE);
	Gui_SetRoot(frame);
	Gui_Focus();
}
void GameState_LightEditor_Quit(void)
{
	Gui_Reset();
	Gui_DestroyFrame(frame);
	Gui_DestroyFrame(sframe);
	Gui_DestroyFrame(lframe);
}
void GameState_LightEditor_Update(void)
{
	Input_Update();
	GameConsole_Update();
	Gui_Update();
	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING))
		Gui_Focus();
	float spd = 8.0f;
	if (Input_GetButtonPressedRepeated(INPUT_A, INPUT_STATE_PLAYING) && light > 0)
		light -= 1;
	if (Input_GetButtonPressedRepeated(INPUT_D, INPUT_STATE_PLAYING) && light < 7)
		light += 1;
	if (Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING))
		cy -= spd;
	if (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING))
		cy += spd;
	if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING))
		cx += spd;
	if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING))
		cx -= spd;
	if (Input_GetButton(INPUT_W, INPUT_STATE_PLAYING))
		Blocks_SetBlockAmbientLight((int)((float)(cx + 160.0f) / (float)BLOCK_SIZE), (int)((float)(cy + 120.0f) / (float)BLOCK_SIZE), light);
	if (Input_GetButton(INPUT_S, INPUT_STATE_PLAYING))
		Blocks_SetBlockAmbientLight((int)((float)(cx + 160.0f) / (float)BLOCK_SIZE), (int)((float)(cy + 120.0f) / (float)BLOCK_SIZE), RENDERER_LIGHT);
}
void GameState_LightEditor_Draw(void)
{
	Renderer_Clear(RCOL_LIGHT_BLUE);
	Blocks_DrawBlocks(BLOCKS_BG, (int)cx, (int)cy);
	Blocks_DrawBlocks(BLOCKS_FG, (int)cx, (int)cy);
	if (light == RENDERER_LIGHT)
		Renderer_DrawText(8, 16, 0, RENDERER_LIGHT, "CURRENT LIGHT LEVEL: NORMAL");
	else
		Renderer_DrawText(8, 16, 0, RENDERER_LIGHT, "CURRENT LIGHT LEVEL: %d", RENDERER_LIGHT - light);
	Renderer_DrawText(8, 0, 0, RENDERER_LIGHT, "X: %d Y: %d BX: %d BY: %d ",
					  (int)(cx + 160.0f), (int)(cy + 120.0f), (int)(cx + 160.0f) / BLOCK_SIZE, (int)(cy + 120.0f) / BLOCK_SIZE);
	cross_rect.x = 160 - 1;
	cross_rect.y = 120 - 4;
	cross_rect.w = 2;
	cross_rect.h = 8;
	Renderer_DrawRect(&cross_rect, RCOL_RED, 4, RENDERER_LIGHT);
	cross_rect.x = 160 - 4;
	cross_rect.y = 120 - 1;
	cross_rect.w = 8;
	cross_rect.h = 2;
	Renderer_DrawRect(&cross_rect, RCOL_RED, 4, RENDERER_LIGHT);
	Gui_Draw();
	GameConsole_Draw();
	Renderer_Update();
}
