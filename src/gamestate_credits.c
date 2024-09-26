#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "console.h"
#include "renderer.h"
#include "blocks.h"
#include "serial.h"
#include "game_console.h"
#include "input.h"
//#_include "gui.h"
#include "command.h"
#include "wobj.h"
#include "spawners.h"
#include "game.h"
#include "net.h"
#include "client.h"
#include "titlebg.h"
#include "mem.h"
#include "world.h"
#include "fadeout.h"
#include "background.h"
#include "audio.h"
#include "ending_text.h"

static int timer1 = 0, timer2 = 0, timer3 = 0;
static int imgid = 0, imgstate = 0;
static float imgx = 0, imgw = 0;

// 4:3 pos x 16:9 pos x rotations
static int pos_x[] = {
	28, 90,
	10, 40,
	45, 140,
	28, 90,
	60, 160,
};
static int pos_y[] = {
	64, 16, 64, 8, 80,
};

static int msg_pos_x[] = {
	16, 40,
	8, 20,
	24, 40,
	16, 60,
	32, 80,
};
static int msg_pos_y[] = {
	16, 210, 16, 180, 32,
};

static const char *msgs[22] = {
	"Wyatt (eklipsed) Radkiewicz",
	"Summer (natwap) Radkiewicz",
	"Tyler (TyJayT) Toelander",
	"Jalen (Mr. Jalen Toes) Toelander",
	"Huh?",
	"LET ME OUT",
	"GET ME OUT OF HERE",
	"We had a lot of fun making this game",
	"We are all ayana",
	"Wow",
	"My favorite enemy is the small rock dudes",
	"(yes that is there name)",
	"I didnt' know what to put here. Hmmmm",
	"- Special Thanks -",
	"Game Maker Studio 1.0",
	"Jacob (Peaches)",
	"Yuseph (Mr. Mister Man)",
	"Chris (Shizuka Gokou)",
	"And you",
	"Thanks for playing!",
};

void GameState_Credits_Init(void) {
	arena_push_zone("CREDITS");
	Serial_LoadLevelGfx("levels/credits");
	Audio_StopMusic();

	Renderer_SetFont(0, 1584, 8, 8);

	EndingText_SetLine(0, "      - CNM ONLINE - ");
	EndingText_SetLine(1, "\"the complete experience\"");
	EndingText_SetLine(2, "     PRODUCTION STAFF");

	EndingText_ResetYValue();
	EndingText_Start(0, 3);

	timer1 = 12*30;
	timer2 = 0;
	timer3 = 0;
	imgx = 0;
	imgw = 0;
	imgid = 0;
	imgstate = 0;
}
void GameState_Credits_Quit(void) {
	titlebg_cleanup();
	arena_pop_zone("CREDITS");
}
void update_boxes(void) {
	if (timer1-- > 0) return;

	if (imgstate == 0) {
		if (imgid % 2 == 0) {
			imgw += 256.0f/30.0f;
		} else {
			imgw += 256.0f/30.0f;
			imgx = 256 - (int)imgw;
		}
		if (timer2 == 0) {
			imgx = 0;
			imgw = 0;
			timer3 = 0;
		}

		if (timer2++ >= 30) {
			imgstate = 1;
			timer2 = 0;
			imgw = 256;
		}
	} else if (imgstate == 1) {
		int time = 60;
		if (imgid == 19) time = 120;
		if (timer2++ >= time) {
			imgstate = 2;
			timer2 = 0;
		}
	} else if (imgstate == 2) {
		int time = 30;
		if (imgid == 19) time = 60;

		if (imgid % 2 == 0) {
			imgw -= 256.0f/(float)time;
			imgx = 256 - (int)imgw;
		} else {
			imgw -= 256.0f/(float)time;
		}

		if (timer2++ >= time) {
			imgstate = 3;
			timer2 = 0;
			imgw = 0;
		}
	} else if (imgstate == 3) {
		if (timer2++ >= 30) {
			imgstate = 0;
			timer2 = 0;
			imgid++;
			if (imgid >= 20) {
				imgstate = 4;
			}
		}
	} else if (imgstate == 4) {
		if (timer2++ >= 30) {
			Game_SwitchState(GAME_STATE_MAINMENU);
		}
	}
}
void GameState_Credits_Update(void) {
	Input_Update();
	GameConsole_Update();
	
	if (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING)) {
		timer1 -= 3;
		update_boxes();
		update_boxes();
		update_boxes();
		EndingText_Draw();
		EndingText_Draw();
		EndingText_Draw();
	}

	if (timer1 < 12*30 - 60 && Audio_GetCurrentPlayingMusic() != 28) {
		Audio_PlayMusic(28, 0);
	}

	update_boxes();
}
void GameState_Credits_Draw(void) {
	CNM_RECT r;
	
	Renderer_Clear(RCOL_BLACK);

	int x = pos_x[imgid % 5 * 2];
	if (Game_GetVar(GAME_VAR_WIDESCREEN)->data.integer) {
		x = pos_x[imgid % 5 * 2 + 1];
	}

	r.x = imgid % 2 * 256 + imgx;
	r.y = imgid / 2 * 144;
	r.w = imgw;
	r.h = 144;

	r.w -= 20;
	r.h -= 20;

	float time = 210;
	if (imgid == 19) time += 180.0f;
	int addx = (float)timer3 / time * 20;
	int addy = (float)timer3++ / time * 20;
	if (imgid % 2 == 0) {
		r.x += addx; r.y += addy;
	} else {
		r.x += 20 - addx;
		r.y += 20 - addy;
	}

	if (imgid < 20) {
		Renderer_DrawBitmap(x + imgx + 10, pos_y[imgid % 5] + 10, &r, 0, RENDERER_LIGHT);
	}

	if (timer1 <= 0 && imgid < 20) {
		int trans = 7;

		if (imgstate == 0) {
			trans = 7 - timer2 / 5;
			if (trans < 0) trans = 0;
		} else if (imgstate == 1) {
			trans = 0;
		} else if (imgstate == 2) {
			trans = timer2 / 5;
			if (trans > 7) trans = 7;
		}

		int mx = msg_pos_x[imgid % 5 * 2];
		if (Game_GetVar(GAME_VAR_WIDESCREEN)->data.integer) {
			mx = msg_pos_x[imgid % 5 * 2 + 1];
		}

		Renderer_SetFont(0, 1584, 8, 8);
		Renderer_DrawText(mx, msg_pos_y[imgid % 5], trans, RENDERER_LIGHT, msgs[imgid]);
	}

	EndingText_Draw();
	Fadeout_StepFade();
	Fadeout_ApplyFade();
	GameConsole_Draw();
	Renderer_Update();
}

