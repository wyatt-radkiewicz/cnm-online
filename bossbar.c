#include <string.h>
#include "renderer.h"
#include "bossbar.h"
#include "utility.h"
#include "wobj.h"
#include "game.h"

#define MAX_BARS 12
#define MAX_CONCURRENT_BARS 4
#define TIMEOUT_DURATION 60

static float maxhps[MAX_BARS];
static char bossnames[MAX_BARS][UTIL_MAX_TEXT_WIDTH];
static unsigned int bossnodes[MAX_BARS];
static unsigned int bossuuids[MAX_BARS];
static float hps[MAX_BARS];
static int timeouts[MAX_BARS];

static int BossBar_GetFreeBar(unsigned int wobj_node, unsigned int wobj_uuid);

void BossBar_Init(void)
{
	memset(bossnodes, 0, sizeof(bossnodes));
	memset(bossuuids, 0, sizeof(bossuuids));
	memset(bossnames, 0, sizeof(bossnames));
	memset(maxhps, 0, sizeof(maxhps));
	memset(hps, 0, sizeof(hps));
	memset(timeouts, 0, sizeof(timeouts));
}
void BossBar_Update(void)
{
	int i;
	WOBJ *w;

	if (Game_GetFrame() % 8 != 0) return;
	// Get the hp of all the guys
	for (i = 0; i < MAX_BARS; i++) {
		if (timeouts[i] <= 0) continue;
		w = Wobj_GetAnyWOBJFromUUIDAndNode(bossnodes[i], bossuuids[i]);

		if (w != NULL) {
			timeouts[i] = TIMEOUT_DURATION;
			hps[i] = w->health;
		}
		else {
			timeouts[i] = 0;
		}
		timeouts[i]--;
	}
}
void BossBar_Draw(void)
{
	int i, j, width, x, y, pixels;
	CNM_RECT src;

	for (i = 0, j = 0; i < MAX_BARS; i++) {
		if (timeouts[i] > 0) {
			width = strlen(bossnames[i]) * 8;
			x = RENDERER_WIDTH / 2 - width / 2;
			y = 4 + j * (10+10+4);

			Renderer_DrawText(x, y, 0, RENDERER_LIGHT, bossnames[i]);
			x = RENDERER_WIDTH / 2 - 128;
			y += 10;
			if (maxhps[0] == 0.0f)
				pixels = 0;
			else
				pixels = (int)((hps[i] / maxhps[i]) * 256.0f);
			if (pixels < 0) pixels = 0;
			if (pixels > 256) pixels = 256;
			Util_SetRect(&src, 256, 4072, 256 - pixels, 8);
			Renderer_DrawBitmap(x, y, &src, 2, RENDERER_LIGHT);
			Util_SetRect(&src, 256 + (256 - pixels), 4064, pixels, 8);
			Renderer_DrawBitmap(x + (256 - pixels), y, &src, 2, RENDERER_LIGHT);
			Util_SetRect(&src, 256, 4080, 256, 10);
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
		if (id & 0x800000) {
			id &= ~0x800000;
			hps[id] = maxhp;
			strcpy(bossnames[id], bossname);
			maxhps[id] = maxhp;
			bossnodes[id] = wobj_node;
			bossuuids[id] = wobj_uuid;
		}
		timeouts[id] = TIMEOUT_DURATION;
	}
}

static int BossBar_GetFreeBar(unsigned int wobj_node, unsigned int wobj_uuid) {
	int i;

	for (i = 0; i < MAX_BARS; i++) {
		if (bossnodes[i] == wobj_node && bossuuids[i] == wobj_uuid && timeouts[i] > 0) {
			return i;
		}
	}
	for (i = 0; i < MAX_BARS; i++) {
		if (timeouts[i] <= 0)
		{
			return i | 0x800000;
		}
	}
	return -1;
}