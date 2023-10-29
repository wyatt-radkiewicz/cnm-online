#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "console.h"
#include "renderer.h"
#include "blocks.h"
#include "serial.h"
#include "game_console.h"
#include "input.h"
#include "gui.h"
#include "command.h"
#include "wobj.h"
#include "spawners.h"
#include "game.h"
#include "net.h"
#include "client.h"
#include "titlebg.h"

void GameState_ClientConnecting_Init(void)
{
	NET_ADDR serverip = Net_GetIpFromString(Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string);
	Client_Create(serverip);
}
void GameState_ClientConnecting_Quit(void)
{
	titlebg_cleanup();
}
void GameState_ClientConnecting_Update(void)
{
	Net_PollPackets(64);
	Net_Update();
	Input_Update();
	titlebg_update();
	GameConsole_Update();

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING))
		Game_SwitchState(GAME_STATE_MAINMENU);
}
void GameState_ClientConnecting_Draw(void)
{
	CNM_RECT r;
	//Util_SetRect(&r, 0, 832, 320, 240);
	//Renderer_DrawBitmap(0, 0, &r, 0, RENDERER_LIGHT);
	//Util_SetRect(&r, 320, 832, 192, 32);
	//Renderer_DrawBitmap(160 - 96, 30, &r, 0, RENDERER_LIGHT);
	titlebg_draw(NULL);
	Util_SetRect(&r, 400, 7136, 112, 48);
	int x = RENDERER_WIDTH / 2 - r.w + 12, y = RENDERER_HEIGHT / 2 + 16 + 12+(12*2);
	Renderer_DrawBitmap2(RENDERER_WIDTH / 2, RENDERER_HEIGHT / 2 + 16, &r, 2, RENDERER_LIGHT, CNM_FALSE, CNM_FALSE);
	Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - r.w, RENDERER_HEIGHT / 2 + 16, &r, 2, RENDERER_LIGHT, CNM_TRUE, CNM_FALSE);
	Renderer_DrawBitmap2(RENDERER_WIDTH / 2, RENDERER_HEIGHT / 2 + 16 + r.h, &r, 2, RENDERER_LIGHT, CNM_FALSE, CNM_TRUE);
	Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - r.w, RENDERER_HEIGHT / 2 + 16 + r.h, &r, 2, RENDERER_LIGHT, CNM_TRUE, CNM_TRUE);
	if ((Game_GetFrame() / 10) % 3 == 0)
		Renderer_DrawText(x, y, 0, RENDERER_LIGHT, "CONNECTING TO: %s...", Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string);
	if ((Game_GetFrame() / 10) % 3 == 1)
		Renderer_DrawText(x, y, 0, RENDERER_LIGHT, "CONNECTING TO: %s..", Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string);
	if ((Game_GetFrame() / 10) % 3 == 2)
		Renderer_DrawText(x, y, 0, RENDERER_LIGHT, "CONNECTING TO: %s.", Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string);
	Renderer_DrawText(RENDERER_WIDTH / 2 - (8*21) / 2, RENDERER_HEIGHT / 2 + 16 + 12, 0, RENDERER_LIGHT, "PRESS <ESC> TO CANCEL");
	GameConsole_Draw();
	Renderer_Update();
}
