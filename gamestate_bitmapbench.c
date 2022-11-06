#include <string.h>
#include <stdio.h>
#include <time.h>
#include "console.h"
#include "renderer.h"
#include "blocks.h"
#include "serial.h"
#include "game_console.h"
#include "input.h"
#include "game.h"
#include "gui.h"
#include "utility.h"
#include "command.h"
#include "wobj.h"
#include "audio.h"
#include "filesystem.h"

#define MAXBTIME 30

static int bench_ticks = 0;
static double bench_time = 0.0;

void GameState_BitmapBench_Init(void)
{
	bench_ticks = 0;
	bench_time = 0.0;
}
void GameState_BitmapBench_Quit(void)
{

}
void GameState_BitmapBench_Update(void)
{
	Input_Update();
	GameConsole_Update();

	bench_ticks++;
	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING))
		Game_SwitchState(GAME_STATE_MAINMENU);
}
void GameState_BitmapBench_Draw(void)
{
	CNM_RECT r;
	Util_SetRect(&r, 0, 832, 320, 240);

	if (bench_ticks < MAXBTIME)
	{
		int i;
		clock_t start = clock(), diff;
		for (i = 0; i < 1000* MAXBTIME; i++)
		{
			Renderer_DrawBitmap(0, 0, &r, 0, RENDERER_LIGHT);
		}
		diff = clock() - start;
		double mm = (double)diff * 10000.0 / (double)CLOCKS_PER_SEC;
		bench_time += mm;
		Renderer_DrawText(32, 32, 0, RENDERER_LIGHT, "Bench... Percent Complete: %d", (int)(((float)bench_ticks / (float)MAXBTIME) * 100.0f));
		bench_ticks = 100000;
	}
	else
	{
		Renderer_DrawBitmap(0, 0, &r, 0, RENDERER_LIGHT);
		float score = (float)bench_time / 10.0f;
		Renderer_DrawText(32, 32, 0, RENDERER_LIGHT, "Bench Complete! Time (ms): %d", (int)score);
		Renderer_DrawText(32, 32+32, 0, RENDERER_LIGHT, "Efficency Percent: %d", (int)(1000.0f / score * 100.0f));
	}
	Renderer_DrawText(32, 32+16, 0, RENDERER_LIGHT, "Press enter to abort.");
	GameConsole_Draw();
	Renderer_Update();
}