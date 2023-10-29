#include <stdio.h>
#include <string.h>
#include "input.h"
#include "console.h"
#include "game_console.h"
#include "renderer.h"
#include "utility.h"
#include "command.h"
#include "interaction.h"
#include "client.h"
#include "server.h"
#include "audio.h"
#include "game.h"

#define GAMECONSOLE_STATE_BACK 0x0000
#define GAMECONSOLE_STATE_ROLLUP 0x1001
#define GAMECONSOLE_STATE_FRONT 0x2000
#define GAMECONSOLE_STATE_TYPING 0x0001
#define GAMECONSOLE_STATE_ROLLBACK 0x1002

static char gameconsole_lines[GAMECONSOLE_HISTORY][UTIL_MAX_TEXT_WIDTH + 1];
static char gameconsole_back_buffer[UTIL_MAX_TEXT_WIDTH + 1];
static char gameconsole_buffer[GAMECONSOLE_HISTORY * UTIL_MAX_TEXT_WIDTH];
static int gameconsole_y = 0;
static int gameconsole_state = GAMECONSOLE_STATE_BACK;
static int gameconsole_fadetime = 0;
static int gameconsole_didtype = CNM_FALSE;
static int gameconsole_initialized = CNM_FALSE;

static char *GameConsole_GetNextLine(void);

void GameConsole_Init(void)
{
	memset(gameconsole_lines, 0, sizeof(gameconsole_lines));
	memset(gameconsole_buffer, 0, sizeof(gameconsole_buffer));
	memset(gameconsole_back_buffer, 0, sizeof(gameconsole_back_buffer));
	Console_SetCallback(GameConsole_PrintCallback);
	gameconsole_initialized = CNM_TRUE;
}
void GameConsole_PrintCallback(const char *str)//const char *format, va_list args)
{
	int n, num_lines, i;
	if (!gameconsole_initialized)
		return;

	n = strlen(str);//Util_StringPrintF(gameconsole_buffer, sizeof(gameconsole_buffer), format, args);
	memset(gameconsole_buffer, 0, sizeof(gameconsole_buffer));
	strncpy(gameconsole_buffer, str, sizeof(gameconsole_buffer) - 1);
	num_lines = (n % UTIL_MAX_TEXT_WIDTH == 0) ? (n / UTIL_MAX_TEXT_WIDTH) : (n / UTIL_MAX_TEXT_WIDTH + 1);
	for (i = 0; i < num_lines; i++)
		memcpy(GameConsole_GetNextLine(), gameconsole_buffer + (i * UTIL_MAX_TEXT_WIDTH), UTIL_MAX_TEXT_WIDTH);

	if (gameconsole_state == GAMECONSOLE_STATE_BACK && num_lines >= 1)
	{
		memset(gameconsole_back_buffer, 0, sizeof(gameconsole_back_buffer));
		memcpy(gameconsole_back_buffer, gameconsole_buffer + ((num_lines - 1) * UTIL_MAX_TEXT_WIDTH), UTIL_MAX_TEXT_WIDTH);
		gameconsole_fadetime = 90;
	}
}
void GameConsole_HandleInput(int input, char c)
{
	if (!gameconsole_initialized)
		return;

	switch (input)
	{
		case GAMECONSOLE_INPUT_TOGGLE:
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		if (gameconsole_state == GAMECONSOLE_STATE_BACK ||
			gameconsole_state == GAMECONSOLE_STATE_ROLLBACK)
		{
			gameconsole_state = GAMECONSOLE_STATE_ROLLUP;
			memset(gameconsole_back_buffer, 0, sizeof(gameconsole_back_buffer));
			Input_PushState(INPUT_STATE_CONSOLE);
			return;
		}
		if (gameconsole_state != GAMECONSOLE_STATE_TYPING)
		{
			gameconsole_state = GAMECONSOLE_STATE_ROLLBACK;
			Input_PopState();
			return;
		}
		break;
		case GAMECONSOLE_INPUT_TALK:
		if (gameconsole_state == GAMECONSOLE_STATE_BACK)
		{
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			gameconsole_state = GAMECONSOLE_STATE_TYPING;
			Input_PushState(INPUT_STATE_CONSOLE);
			memset(gameconsole_back_buffer, 0, sizeof(gameconsole_back_buffer));
		}
		break;
		case GAMECONSOLE_INPUT_ESC:
		memset(gameconsole_back_buffer, 0, sizeof(gameconsole_back_buffer));
		if (gameconsole_state == GAMECONSOLE_STATE_TYPING)
		{
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			Input_PopState();
			gameconsole_state = GAMECONSOLE_STATE_BACK;
			gameconsole_fadetime = 15;
		}
		if (gameconsole_state == GAMECONSOLE_STATE_FRONT)
		{
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			Input_PopState();
			memset(gameconsole_back_buffer, 0, sizeof(gameconsole_back_buffer));
			gameconsole_state = GAMECONSOLE_STATE_ROLLBACK;
		}
		break;
		case GAMECONSOLE_INPUT_ENTER:
		if (gameconsole_state == GAMECONSOLE_STATE_TYPING ||
			gameconsole_state == GAMECONSOLE_STATE_FRONT)
		{
			if (gameconsole_state == GAMECONSOLE_STATE_TYPING)
			{
				Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
				if (Interaction_GetMode() == INTERACTION_MODE_CLIENT)
					Client_SendChatMessage(gameconsole_back_buffer);
				if (Interaction_GetMode() == INTERACTION_MODE_HOSTED_SERVER)
					Server_SendChatMessage(0, gameconsole_back_buffer);
				if (Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER)
					Console_Print(gameconsole_back_buffer);
			}
			else
			{
				Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
				Console_Print(gameconsole_back_buffer);
			}
		}
		if (gameconsole_state == GAMECONSOLE_STATE_TYPING)
		{
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			Input_PopState();
			gameconsole_state = GAMECONSOLE_STATE_BACK;
		}
		if (gameconsole_state == GAMECONSOLE_STATE_FRONT)
		{
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			Command_Execute(gameconsole_back_buffer);
			gameconsole_didtype = CNM_TRUE;
			memset(gameconsole_back_buffer, 0, sizeof(gameconsole_back_buffer));
		}
		break;
		case GAMECONSOLE_INPUT_CHAR:
		if (gameconsole_state == GAMECONSOLE_STATE_TYPING ||
			gameconsole_state == GAMECONSOLE_STATE_FRONT)
		{
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			if (strlen(gameconsole_back_buffer) + 1 < UTIL_MAX_TEXT_WIDTH) {
				char *tmp = gameconsole_back_buffer;
				for (; *tmp; tmp++) {}
				*(tmp++) = c;
				*tmp = '\0';
			}
				//strcat(gameconsole_back_buffer, &c);
		}
		break;
		case GAMECONSOLE_INPUT_BACKSPACE:
		if (gameconsole_state == GAMECONSOLE_STATE_TYPING ||
			gameconsole_state == GAMECONSOLE_STATE_FRONT)
		{
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			if (strlen(gameconsole_back_buffer))
				gameconsole_back_buffer[strlen(gameconsole_back_buffer) - 1] = '\0';
		}
		break;
	}
}
void GameConsole_Update(void)
{
	if (!gameconsole_initialized)
		return;

	if (Input_GetButtonPressed(INPUT_CONSOLE, INPUT_STATE_CONSOLE))
		GameConsole_HandleInput(GAMECONSOLE_INPUT_TOGGLE, '\0');
	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_CONSOLE))
		GameConsole_HandleInput(GAMECONSOLE_INPUT_ENTER, '\0');
	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_CONSOLE))
		GameConsole_HandleInput(GAMECONSOLE_INPUT_ESC, '\0');
	if (Input_GetButtonPressedRepeated(INPUT_BACKSPACE, INPUT_STATE_CONSOLE))
		GameConsole_HandleInput(GAMECONSOLE_INPUT_BACKSPACE, '\0');
	if (Input_GetButtonPressed(INPUT_TALK, INPUT_STATE_PLAYING) && !GameConsole_IsOpen() && Game_TopState() != GAME_STATE_BLOCKSEDIT && Game_TopState() != GAME_STATE_MAINMENU)
		GameConsole_HandleInput(GAMECONSOLE_INPUT_TALK, '\0');
	else if (Util_IsSafePrintable(Input_GetCharPressed(INPUT_STATE_CONSOLE)))
		GameConsole_HandleInput(GAMECONSOLE_INPUT_CHAR, Input_GetCharPressed(INPUT_STATE_CONSOLE));

	if (gameconsole_state == GAMECONSOLE_STATE_BACK)
	{
		if (gameconsole_fadetime)
			gameconsole_fadetime--;
	}
	else
	{
		gameconsole_fadetime = 90;
	}
	if (gameconsole_state == GAMECONSOLE_STATE_BACK ||
		gameconsole_state == GAMECONSOLE_STATE_TYPING)
		gameconsole_y = 0;
	if (gameconsole_state == GAMECONSOLE_STATE_FRONT)
		gameconsole_y = 112;
	if (gameconsole_state == GAMECONSOLE_STATE_ROLLUP)
	{
		gameconsole_y += 8;
		if (gameconsole_y >= 112)
			gameconsole_state = GAMECONSOLE_STATE_FRONT;
	}
	if (gameconsole_state == GAMECONSOLE_STATE_ROLLBACK)
	{
		gameconsole_y -= 8;
		if (gameconsole_y <= 0)
		{
			gameconsole_state = GAMECONSOLE_STATE_BACK;
			memcpy(gameconsole_back_buffer, gameconsole_lines[0], sizeof(gameconsole_back_buffer));
			if (!gameconsole_didtype)
				gameconsole_fadetime = 15;
			gameconsole_didtype = CNM_FALSE;
		}
	}
}
void GameConsole_Draw(void)
{
	int l, y, c, f;
	if (!gameconsole_initialized)
		return;

	Renderer_SetFont(0, 704, 8, 8);
	y = ((GAMECONSOLE_HISTORY + 1) * -8) + gameconsole_y;
	for (l = GAMECONSOLE_HISTORY - 1; l > -1; l--)
	{
		y += 8;
		for (c = 0; c < UTIL_MAX_TEXT_WIDTH; c++)
		{
			if (gameconsole_lines[l][c] != '\0')
				Renderer_DrawText(c * 8, y, 3, RENDERER_LIGHT, "%c", gameconsole_lines[l][c]);
			else
				Renderer_DrawText(c * 8, y, 3, RENDERER_LIGHT, " ");
		}
	}
	f = 3;
	if (gameconsole_fadetime < 15)
		f++;
	if (gameconsole_fadetime < 10)
		f++;
	if (gameconsole_fadetime < 5)
		f++;
	if (gameconsole_fadetime <= 0)
		f++;
	for (c = 0; c < UTIL_MAX_TEXT_WIDTH; c++)
	{
		if (gameconsole_back_buffer[c] != '\0')
			Renderer_DrawText(c * 8, gameconsole_y, f, RENDERER_LIGHT, "%c", gameconsole_back_buffer[c]);
		else
			Renderer_DrawText(c * 8, gameconsole_y, f, RENDERER_LIGHT, " ");
	}
	Renderer_SetFont(384, 448, 8, 8);
}
int GameConsole_IsOpen(void)
{
	if (!gameconsole_initialized)
		return CNM_FALSE;

	return gameconsole_state == GAMECONSOLE_STATE_TYPING ||
		gameconsole_state == GAMECONSOLE_STATE_ROLLUP ||
		gameconsole_state == GAMECONSOLE_STATE_FRONT;
}

static char *GameConsole_GetNextLine(void)
{
	memmove(gameconsole_lines[1], gameconsole_lines[0], (GAMECONSOLE_HISTORY - 1) * (UTIL_MAX_TEXT_WIDTH + 1));
	memset(gameconsole_lines[0], 0, UTIL_MAX_TEXT_WIDTH + 1);
	return gameconsole_lines[0];
}
