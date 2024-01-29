#include <string.h>
#include "renderer.h"
#include "bossbar.h"
#include "utility.h"
#include "wobj.h"
#include "game.h"
#include "mem.h"

#define MAX_BARS 12
#define MAX_CONCURRENT_BARS 4
#define TIMEOUT_DURATION 60

typedef struct bossbar {
	float maxhp, hp;
	int timeout;
	unsigned int node, uuid;
	char name[UTIL_MAX_TEXT_WIDTH];
} bossbar_t;

static bossbar_t *bars;

static int BossBar_GetFreeBar(unsigned int wobj_node, unsigned int wobj_uuid);

void BossBar_Init(void)
{
	bars = arena_alloc(sizeof(*bars) * MAX_BARS);
	memset(bars, 0, sizeof(*bars) * MAX_BARS);
}
void BossBar_Update(void)
{
	int i;
	WOBJ *w;

	if (Game_GetFrame() % 8 != 0) return;
	// Get the hp of all the guys
	for (i = 0; i < MAX_BARS; i++) {
		bossbar_t *bar = &bars[i];

		if (bar->timeout <= 0) continue;
		w = Wobj_GetAnyWOBJFromUUIDAndNode(bar->node, bar->uuid);

		if (w != NULL) {
			bar->timeout = TIMEOUT_DURATION;
			bar->hp = w->health;
		}
		else {
			bar->timeout = 0;
		}
		bar->timeout--;
	}
}
void BossBar_Draw(void)
{
	int i, j, width, x, y, pixels;
	CNM_RECT src;

	for (i = 0, j = 0; i < MAX_BARS; i++) {
		bossbar_t *bar = &bars[i];

		if (bar->timeout > 0) {
			width = strlen(bar->name) * 8;
			x = RENDERER_WIDTH / 2 - width / 2;
			y = 4 + j * (10+10+4);

			Renderer_DrawText(x, y, 0, RENDERER_LIGHT, bar->name);
			x = RENDERER_WIDTH / 2 - 128;
			y += 10;
			if (bar->maxhp == 0.0f)
				pixels = 0;
			else
				pixels = (int)((bar->hp / bar->maxhp) * 256.0f);
			if (pixels < 0) pixels = 0;
			if (pixels > 256) pixels = 256;
			Util_SetRect(&src, 256, 1672, 256 - pixels, 8);
			Renderer_DrawBitmap(x, y, &src, 2, RENDERER_LIGHT);
			Util_SetRect(&src, 256 + (256 - pixels), 1664, pixels, 8);
			Renderer_DrawBitmap(x + (256 - pixels), y, &src, 2, RENDERER_LIGHT);
			Util_SetRect(&src, 256, 1680, 256, 10);
			Renderer_DrawBitmap(x, y-1, &src, 0, RENDERER_LIGHT);

			if (j++ >= MAX_CONCURRENT_BARS) return;
		}
	}
}
void BossBar_RegisterBar(unsigned int wobj_node, unsigned int wobj_uuid, float maxhp, const char *bossname)
{
	int id;

	id = BossBar_GetFreeBar(wobj_node, wobj_uuid);
	if (id > -1)
	{
		bossbar_t *bar = &bars[id];
		if (id & 0x800000) {
			id &= ~0x800000;
			bar = &bars[id];

			bar->hp = maxhp;
			strcpy(bar->name, bossname);
			bar->maxhp = maxhp;
			bar->node = wobj_node;
			bar->uuid = wobj_uuid;
		}
		bar->timeout = TIMEOUT_DURATION;
	}
}

static int BossBar_GetFreeBar(unsigned int wobj_node, unsigned int wobj_uuid) {
	int i;

	for (i = 0; i < MAX_BARS; i++) {
		if (bars[i].node == wobj_node && bars[i].uuid == wobj_uuid && bars[i].timeout > 0) {
			return i;
		}
	}
	for (i = 0; i < MAX_BARS; i++) {
		if (bars[i].timeout <= 0)
		{
			return i | 0x800000;
		}
	}
	return -1;
}
