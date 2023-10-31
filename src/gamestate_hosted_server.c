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
#include "net.h"
#include "server.h"
#include "audio.h"
#include "net_file.h"
#include "world.h"
#include "pausemenu.h"
#include "fadeout.h"

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

void GameState_HostedServer_Init(void)
{
	pause_menu_init();
	pause_menu_setcallback(PAUSE_MENU_RESPAWN, Respawn);
	pause_menu_setcallback(PAUSE_MENU_CONTINUE, GoBackToPlaying);
	pause_menu_setcallback(PAUSE_MENU_EXIT, GoBackToMainMenu);
	Server_Create();
	NetFile_AddServerNetCallback();
	World_Start(WORLD_MODE_HOSTED_SERVER);
}
void GameState_HostedServer_Quit(void)
{
	Console_Print("Shutting Down Server....");

	World_Stop();
	NetFile_RemoveNetCallbacks();
	Server_Destroy();
}
void GameState_HostedServer_Update(void)
{
	Net_InitAvgUDPOutgoingBandwidth();
	NetFile_TickServer();
	NetGame_Update();
	Server_Tick();
	Net_PollPackets(64);
	Net_Update();
	Input_Update();
	//Gui_Update();
	pause_menu_update();
	Fadeout_StepFade();
	GameConsole_Update();
	World_Update(WORLD_MODE_HOSTED_SERVER);
	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING)) pause_menu_focus();
}
void GameState_HostedServer_Draw(void)
{
	World_Draw(WORLD_MODE_HOSTED_SERVER);
	pause_menu_draw();
	Fadeout_ApplyFade();
	GameConsole_Draw();
	Renderer_Update();
}
