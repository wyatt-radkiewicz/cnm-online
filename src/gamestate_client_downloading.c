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
#include "client.h"
#include "net_file.h"
#include "utility.h"
#include "audio.h"

static int just_minigame = CNM_FALSE;
static int failure_tick = 0;
static int did_connect = 0;
static float player_y = 0.0f, player_vel = 0.0f;
static int num_bullets = 0;
#define MAX_BULLETS 3
static int bullets_y[MAX_BULLETS];
static int bullets_x[MAX_BULLETS];
static int bullets_active[MAX_BULLETS];
static int level;
static int num_to_next_level;
static int slime_timer;
static int slimes_killed;

#define MAX_SLIMES 15

typedef struct _MINIGAME_SLIME
{
	int x, y;
	int hp;
	int active;
} MINIGAME_SLIME;
static MINIGAME_SLIME slimes[MAX_SLIMES];
static int num_slimes;

static void RunMinigame(void);

void GameState_ClientDownloading_Init(void)
{
	just_minigame = CNM_FALSE;
	if (Game_TopState() == GAME_STATE_MINIGAME1_TEST)
		just_minigame = CNM_TRUE;

	if (!just_minigame)
	{
		NET_ADDR addr = Net_GetIpFromString(Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string);
		NetFile_AddClientNetCallback();
		did_connect = NetFile_AskForFiles(addr);
		failure_tick = 0;
	}

	// Minigame
	player_y = (float)(RENDERER_HEIGHT / 2 - 16);
	player_vel = 0.0f;
	num_bullets = 0;
	memset(bullets_active, 0, sizeof(bullets_active));
	level = 1;
	num_to_next_level = 3;
	num_slimes = 0;
	slime_timer = 0;
	slimes_killed = 0;
}

void GameState_ClientDownloading_Quit(void)
{
	if (!just_minigame)
	{
		NetFile_RemoveNetCallbacks();
	}
}

void GameState_ClientDownloading_Update(void)
{
	if (!just_minigame)
	{
		Net_PollPackets(32);
		Net_Update();
	}
	Input_Update();
	GameConsole_Update();

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING))
		Game_SwitchState(GAME_STATE_MAINMENU);

	RunMinigame();

	if (just_minigame)
		return;

	if (NetFile_ClientHasToDownloadFiles())
	{
		//failure_tick++;
		//if (failure_tick > 30*6)
		//	Game_SwitchState(GAME_STATE_MAINMENU);
	}
	else if (NetFile_HasStartedDownloading())
	{
		//Game_SwitchState(GAME_STATE_CLIENT_CONNECTING);
	}
}

static void DrawMinigame(void)
{
	CNM_RECT r;
	Util_SetRect(&r, 0, 288, 32, 32);
	Renderer_DrawBitmap(16, (int)player_y, &r, 0, RENDERER_LIGHT);
	Util_SetRect(&r, 32, 352, 32, 32);
	Renderer_DrawBitmap(16, (int)player_y, &r, 0, RENDERER_LIGHT);

	Util_SetRect(&r, 0, 224, 32, 32);
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (bullets_active[i])
		{
			Renderer_DrawBitmap(bullets_x[i], bullets_y[i], &r, 0, RENDERER_LIGHT);
		}
	}

	for (int i = 0; i < MAX_SLIMES; i++)
	{
		if (slimes[i].active)
		{
			Util_SetRect(&r, 32, 256 + ((Game_GetFrame() / 32 + slimes[i].y) % 2) * 32, 32, 32);
			Renderer_DrawBitmap(slimes[i].x, slimes[i].y, &r, 0, RENDERER_LIGHT);
		}
	}

	Renderer_DrawText(8, 8, 0, RENDERER_LIGHT, "LEVEL: %d", level);
}

void GameState_ClientDownloading_Draw(void)
{
	CNM_RECT r;
	Util_SetRect(&r, 0, 832, 320, 240);
	Renderer_DrawBitmap(0, 0, &r, 0, RENDERER_LIGHT);

	if (just_minigame)
	{
		DrawMinigame();
		Util_SetRect(&r, 320, 832, 192, 32);
		Renderer_DrawBitmap(160 - 96, 30, &r, 0, RENDERER_LIGHT);
		Renderer_DrawText(64 + 12, 128 + 32, 0, RENDERER_LIGHT, "PRESS <ESC> TO CANCEL");
		GameConsole_Draw();
		Renderer_Update();
		return;
	}

	if (did_connect)
	{
		//Renderer_DrawText(50, 100, 0, RENDERER_LIGHT, "Auto-Download feature does");
		//Renderer_DrawText(50, 100+12, 0, RENDERER_LIGHT, "not work currently!!!");
		//Renderer_DrawText(50, 100+24, 0, RENDERER_LIGHT, "try to download the servers files");
		//Renderer_DrawText(50, 100+36, 0, RENDERER_LIGHT, "somewhere else!!!");
		// Minigame
		DrawMinigame();

		// Download progress
		char downloading_buffer[3][UTIL_MAX_TEXT_WIDTH + 1];
		memset(downloading_buffer, 0, sizeof(downloading_buffer));
		sprintf(downloading_buffer[0], "Downloading %s",
				NetFile_GetCurrentDownloadingFileName());
		sprintf(downloading_buffer[1], "from %s",
				Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string);
		downloading_buffer[0][UTIL_MAX_TEXT_WIDTH] = '\0';
		sprintf(downloading_buffer[2], "%d%% complete", NetFile_GetCurrentDownloadingFilePercent());
		Renderer_DrawText(160 - (strlen(downloading_buffer[0]) * 4), 128 - 12, 0, RENDERER_LIGHT, downloading_buffer[0]);
		Renderer_DrawText(160 - (strlen(downloading_buffer[1]) * 4), 128, 0, RENDERER_LIGHT, downloading_buffer[1]);
		if ((Game_GetFrame() / 10) % 3 == 0)
			Renderer_DrawText(96, 128 + 12, 0, RENDERER_LIGHT, "%s...", downloading_buffer[2]);
		if ((Game_GetFrame() / 10) % 3 == 1)
			Renderer_DrawText(96, 128 + 12, 0, RENDERER_LIGHT, "%s..", downloading_buffer[2]);
		if ((Game_GetFrame() / 10) % 3 == 2)
			Renderer_DrawText(96, 128 + 12, 0, RENDERER_LIGHT, "%s.", downloading_buffer[2]);//*/
	}
	else
	{
		failure_tick++;
		if (failure_tick > 60)
			Game_SwitchState(GAME_STATE_MAINMENU);
#define WAITING_TEXT "Can't connect try again later."
		Renderer_DrawText(160 - (strlen(WAITING_TEXT) * 4), 128 - 12, 0, RENDERER_LIGHT, WAITING_TEXT);
#undef WAITING_TEXT
#define WAITING_TEXT "(maybe someone is connecting right now)"
		Renderer_DrawText(160 - (strlen(WAITING_TEXT) * 4), 128, 0, RENDERER_LIGHT, WAITING_TEXT);
	}
	Util_SetRect(&r, 320, 832, 192, 32);
	Renderer_DrawBitmap(160 - 96, 30, &r, 0, RENDERER_LIGHT);
	Renderer_DrawText(64 + 12, 128 + 32, 0, RENDERER_LIGHT, "PRESS <ESC> TO CANCEL");
	GameConsole_Draw();
	Renderer_Update();
}

static void RunMinigame(void) {
	// Minigame
	player_vel += 0.5f;
	player_y += player_vel;
	if (player_y > (float)(RENDERER_HEIGHT - 32))
		player_y = (float)(RENDERER_HEIGHT - 32);
	if (player_y > (float)(RENDERER_HEIGHT - 33))
	{
		if (Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING))
			player_vel = -14.5f;
	}

	if (num_bullets < MAX_BULLETS && Input_GetButtonPressed(INPUT_DOWN, INPUT_STATE_PLAYING))
	{
		for (int i = 0; i < MAX_BULLETS; i++)
		{
			if (!bullets_active[i])
			{
				bullets_x[i] = 32;
				bullets_y[i] = (int)player_y;
				num_bullets++;
				bullets_active[i] = CNM_TRUE;
				Audio_SetListenerOrigin(0, 0);
				Audio_PlaySound(0, CNM_FALSE, 0, 0);
				break;
			}
		}
	}

	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (bullets_active[i])
		{
			bullets_x[i] += 8;
			if (bullets_x[i] > RENDERER_WIDTH)
			{
				bullets_active[i] = CNM_FALSE;
				num_bullets--;
				continue;
			}

			for (int j = 0; j < MAX_SLIMES; j++)
			{
				if (slimes[j].active)
				{
					CNM_BOX b, s;
					b.x = (float)bullets_x[i];
					b.y = (float)bullets_y[i] + 8.0f;
					b.w = 24.0f;
					b.h = 16.0f;
					s.x = (float)slimes[j].x;
					s.y = (float)slimes[j].y + 16.0f;
					s.w = 32.0f;
					s.h = 16.0f;
					if (Util_AABBCollision(&b, &s))
					{
						slimes_killed++;

						slimes[j].active = CNM_FALSE;
						num_slimes--;
						bullets_active[i] = CNM_FALSE;
						num_bullets--;

						if (slimes_killed >= num_to_next_level)
						{
							num_to_next_level += 3;
							slimes_killed = 0;
							level++;
							memset(slimes, 0, sizeof(slimes));
							num_slimes = 0;
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < MAX_SLIMES; i++)
	{
		if (slimes[i].active)
		{
			slimes[i].x -= level / 2 + 1;
			if (slimes[i].x < 0)
			{
				memset(slimes, 0, sizeof(slimes));
				num_slimes = 0;
				player_y = (float)(RENDERER_HEIGHT / 2 - 16);
				player_vel = 0.0f;
				num_bullets = 0;
				memset(bullets_active, 0, sizeof(bullets_active));
				level = 1;
				num_to_next_level = 3;
				num_slimes = 0;
				slime_timer = 0;
				slimes_killed = 0;
			}
		}
	}

	slime_timer--;
	if (slime_timer < 0)
	{
		slime_timer = Util_RandInt(30 - CNM_MAX(15, level), 90 - CNM_MAX(level * 3, 60));

		for (int i = 0; i < MAX_SLIMES && num_slimes < MAX_SLIMES; i++)
		{
			if (!slimes[i].active)
			{
				slimes[i].x = RENDERER_WIDTH;
				slimes[i].y = Util_RandInt(32, RENDERER_HEIGHT - 8 - 32);
				num_slimes++;
				slimes[i].active = CNM_TRUE;
				break;
			}
		}
	}
}