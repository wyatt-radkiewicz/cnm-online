#include <string.h>
#include <stdio.h>
#include "console.h"
#include "renderer.h"
#include "serial.h"
#include "game_console.h"
#include "input.h"
#include "gui.h"
#include "command.h"
#include "game.h"
#include "ending_text.h"

static GUI_FRAME *frame, *sframe, *lframe;

static void SaveSpawnersElementString(GUI_ELEMENT *elem, int index)
{
	static int saving = CNM_FALSE;
	char buffer[128] = {'\0'};
	if (!saving)
	{
		saving = CNM_TRUE;
		strcpy(buffer, "save_spawners ");
		strcat(buffer, elem->props.string);
		Command_Execute(buffer, CNM_FALSE);
		Gui_SwitchBack();
		saving = CNM_FALSE;
	}
}
static void LoadSpawnersElementString(GUI_ELEMENT *elem, int index)
{
	static int loading = CNM_FALSE;
	char buffer[128] = {'\0'};
	if (!loading)
	{
		loading = CNM_TRUE;
		strcpy(buffer, "load_spawners ");
		strcat(buffer, elem->props.string);
		Command_Execute(buffer, CNM_FALSE);
		strcpy(sframe->elements[0].props.string, elem->props.string);
		memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
		Gui_SwitchBack();

		for (int i = 0; i < ENDING_TEXT_MAX_LINES; i++)
		{
			strcpy(frame->elements[i + 4].props.string, EndingText_GetLine(i));
		}

		loading = CNM_FALSE;
	}
}
static void GoBackToMainMenu(GUI_ELEMENT *elem, int index)
{
	Game_SwitchState(GAME_STATE_MAINMENU);
}
static void EndingTextLineEdit(GUI_ELEMENT *elem, int index)
{
	EndingText_SetLine(index - 4, elem->props.string);
}

void GameState_EndTextEditor_Init(void)
{
	GUI_FRAME_PROPS props;
	Gui_Reset();
	props.top = 64;
	props.align[0] = GUI_ALIGN_LEFT;
	props.bounds[0] = 16;
	props.line_count = 12;
	props.align[1] = GUI_ALIGN_RIGHT;
	props.bounds[1] = RENDERER_WIDTH - 16;
	frame = Gui_CreateFrame(4+ENDING_TEXT_MAX_LINES, 4 + ENDING_TEXT_MAX_LINES, NULL, &props, 0);
	sframe = Gui_CreateFrame(1, 1, frame, &props, 0);
	lframe = Gui_CreateFrame(1, 1, frame, &props, 0);
	Gui_InitStringElement(lframe, 0, LoadSpawnersElementString, "LOAD FILE: ", 23);
	Gui_InitStringElement(sframe, 0, SaveSpawnersElementString, "SAVE FILE: ", 23);
	Gui_InitHeaderElement(frame, 0, "===== ENDING TEXT EDITOR =====");
	Gui_InitButtonElement(frame, 1, NULL, "LOAD ENDING TEXT", lframe, CNM_FALSE);
	Gui_InitButtonElement(frame, 2, NULL, "SAVE ENDING TEXT", sframe, CNM_FALSE);
	Gui_InitButtonElement(frame, 3, GoBackToMainMenu, "GO BACK TO MAIN MENU", NULL, CNM_FALSE);
	for (int i = 0; i < ENDING_TEXT_MAX_LINES; i++)
	{
		char line_name_buf[8];
		sprintf(line_name_buf, "%d: ", i);
		EndingText_SetLine(i, "");
		Gui_InitStringElement(frame, i + 4, EndingTextLineEdit, line_name_buf, ENDING_TEXT_MAX_WIDTH);
	}

	Gui_SetRoot(frame);
	Gui_Focus();
}
void GameState_EndTextEditor_Quit(void)
{
	Gui_Reset();
	Gui_DestroyFrame(frame);
	Gui_DestroyFrame(lframe);
	Gui_DestroyFrame(sframe);
}
void GameState_EndTextEditor_Update(void)
{
	Input_Update();
	GameConsole_Update();
	Gui_Update();
	if (!Gui_IsFocused())
		Gui_Focus();
}
void GameState_EndTextEditor_Draw(void)
{
	Renderer_Clear(RCOL_LIGHT_BLUE);
	Gui_Draw();
	GameConsole_Draw();
	Renderer_Update();
}
