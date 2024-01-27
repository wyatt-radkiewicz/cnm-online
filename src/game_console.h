#ifndef _game_console_h_
#define _game_console_h_
#include <stdarg.h>
#include "renderer.h"
#include "utility.h"

#define GAMECONSOLE_HISTORY 16

#define GAMECONSOLE_INPUT_NONE 0
#define GAMECONSOLE_INPUT_TOGGLE 1
#define GAMECONSOLE_INPUT_BACKSPACE 3
#define GAMECONSOLE_INPUT_CHAR 4
#define GAMECONSOLE_INPUT_ENTER 5
#define GAMECONSOLE_INPUT_TALK 6
#define GAMECONSOLE_INPUT_ESC 7

void GameConsole_Init(void);
void GameConsole_PrintCallback(const char *str);
void GameConsole_HandleInput(int input, char c);
void GameConsole_Update(void);
void GameConsole_Draw(void);
int GameConsole_IsOpen(void);

#endif
