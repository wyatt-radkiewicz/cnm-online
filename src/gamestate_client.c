#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "console.h"
#include "renderer.h"
//#include "blocks.h"
#include "serial.h"
#include "game_console.h"
#include "input.h"
//#_include "gui.h"
#include "command.h"
//#include "wobj.h"
//#include "spawners.h"
#include "game.h"
#include "net.h"
#include "netgame.h"
#include "client.h"
#include "audio.h"
//#include "player_spawns.h"
//#include "ending_text.h"
//#include "background.h"
#include "world.h"
#include "pausemenu.h"
#include "fadeout.h"
#include "mem.h"

static int *camx, *camy;
static WOBJ *player;

static void GoBackToPlaying(void)
{
	pause_menu_unfocus();
}
static void GoBackToMainMenu(void)
{
	Game_SwitchState(GAME_STATE_MAINMENU);
}
static void Respawn()
{
	Command_Execute("kill", CNM_FALSE);
	pause_menu_unfocus();
}

void GameState_Client_Init(void)
{
	arena_push_zone("CLIENT");
	pause_menu_init();
	pause_menu_setcallback(PAUSE_MENU_RESPAWN, Respawn);
	pause_menu_setcallback(PAUSE_MENU_CONTINUE, GoBackToPlaying);
	pause_menu_setcallback(PAUSE_MENU_EXIT, GoBackToMainMenu);
	World_Start(WORLD_MODE_CLIENT);
	Client_OnLevelStart();
}
void GameState_Client_Quit(void)
{
	Client_Destory();
	World_Stop();
	arena_pop_zone("CLIENT");
}
void GameState_Client_Update(void)
{
	Net_InitAvgUDPOutgoingBandwidth();
	NetGame_Update();
	Client_Tick();
	Net_PollPackets(64);
	Net_Update();
	Input_Update();
	//Gui_Update();
	pause_menu_update();
	Fadeout_StepFade();
	GameConsole_Update();
	World_Update(WORLD_MODE_CLIENT);
	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING)) pause_menu_focus();
}
void GameState_Client_Draw(void)
{
	World_Draw(WORLD_MODE_CLIENT);
	//Gui_Draw();
	pause_menu_draw();
	Fadeout_ApplyFade();
	GameConsole_Draw();
	Renderer_Update();
}
