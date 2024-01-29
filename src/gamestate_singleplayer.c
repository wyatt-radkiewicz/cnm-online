#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "console.h"
#include "renderer.h"
#include "serial.h"
#include "game_console.h"
#include "input.h"
#include "command.h"
#include "game.h"
#include "audio.h"
#include "world.h"
#include "pausemenu.h"
#include "fadeout.h"
#include "savedata.h"
#include "player.h"
#include "mem.h"

static void GoBackToPlaying(void)
{
	pause_menu_unfocus();
}
static void GoBackToMainMenu(void)
{
	Player_SaveData(Game_GetVar(GAME_VAR_PLAYER)->data.pointer, g_saves + g_current_save);
	if (!Game_GetVar(GAME_VAR_NOSAVE)->data.integer) save_game(g_current_save, g_saves + g_current_save);
	Game_SwitchState(GAME_STATE_MAINMENU);
}
static void Respawn(void)
{
	Command_Execute("kill", CNM_FALSE);
	pause_menu_unfocus();
}

void GameState_Singleplayer_Init(void)
{
	arena_push_zone("SPLAY");
	pause_menu_init();
	pause_menu_setcallback(PAUSE_MENU_RESPAWN, Respawn);
	pause_menu_setcallback(PAUSE_MENU_CONTINUE, GoBackToPlaying);
	pause_menu_setcallback(PAUSE_MENU_EXIT, GoBackToMainMenu);
	World_Start(WORLD_MODE_SINGLEPLAYER);
}
void GameState_Singleplayer_Quit(void)
{
	World_Stop();
	arena_pop_zone("SPLAY");
}
void GameState_Singleplayer_Update(void)
{
	Input_Update();
	pause_menu_update();
	GameConsole_Update();
	if (!pause_menu_isfocused()) World_Update(WORLD_MODE_SINGLEPLAYER);
	Fadeout_StepFade();
	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING) && g_can_pause) pause_menu_focus();
}
void GameState_Singleplayer_Draw(void)
{
	World_Draw(WORLD_MODE_SINGLEPLAYER);
	pause_menu_draw();
	Fadeout_ApplyFade();
	GameConsole_Draw();
	Renderer_Update();
}
