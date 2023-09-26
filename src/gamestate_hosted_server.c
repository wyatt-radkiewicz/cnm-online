#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "console.h"
#include "renderer.h"
#include "serial.h"
#include "game_console.h"
#include "input.h"
#include "gui.h"
#include "command.h"
#include "game.h"
#include "net.h"
#include "server.h"
#include "audio.h"
#include "net_file.h"
#include "world.h"

static GUI_FRAME *pause_menu;

static void GoBackToPlaying(GUI_ELEMENT *elem, int index)
{
	Gui_Losefocus();
}
static void GoBackToMainMenu(GUI_ELEMENT *elem, int index)
{
	Game_SwitchState(GAME_STATE_MAINMENU);
}
static void Respawn(GUI_ELEMENT *elem, int index)
{
	Command_Execute("kill");
	Gui_Losefocus();
	pause_menu->active_index = 1;
}

void GameState_HostedServer_Init(void)
{
	GUI_FRAME_PROPS props;

	Gui_Reset();
	props.top = 96;
	props.line_count = 10;
	props.align[0] = GUI_ALIGN_CENTER;
	props.bounds[0] = RENDERER_WIDTH / 2;
	props.bounds[1] = props.bounds[0];
	pause_menu = Gui_CreateFrame(4, 10, NULL, &props, 0);
	Gui_InitHeaderElement(pause_menu, 0, "=== MENU ===");
	Gui_InitButtonElement(pause_menu, 1, GoBackToPlaying, "CONTINUE PLAYING", NULL, CNM_FALSE);
	Gui_InitButtonElement(pause_menu, 2, Respawn, "RESPAWN", NULL, CNM_FALSE);
	Gui_InitButtonElement(pause_menu, 3, GoBackToMainMenu, "SHUT DOWN SERVER", NULL, CNM_FALSE);
	Gui_SetRoot(pause_menu);

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

	Gui_DestroyFrame(pause_menu);
	Gui_Reset();
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
	Gui_Update();
	GameConsole_Update();
	World_Update(WORLD_MODE_HOSTED_SERVER);
	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING))
		Gui_Focus();
}
void GameState_HostedServer_Draw(void)
{
	World_Draw(WORLD_MODE_HOSTED_SERVER);
	Gui_Draw();
	GameConsole_Draw();
	Renderer_Update();
}