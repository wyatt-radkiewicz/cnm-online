#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "pausemenu.h"
#include "interaction.h"
#include "renderer.h"
#include "console.h"
#include "game.h"
#include "input.h"
#include "audio.h"
#include "fadeout.h"
#include "savedata.h"

static int gui_timer;
static int side_blob_x;
static int side_xstart;
static int left_disp;
static int options_num;
static int is_focused;
static int exit_timer, is_exiting;

static pause_menu_func_t funcs[3];

// Singleplayer, Server, Client, Level Select
static const char *option_names[][4] = {
	{ "RESPAWN", "RESPAWN", "RESPAWN", "RESPAWN" },
	{ "CONTINUE", "CONTINUE", "CONTINUE", "CONTINUE" },
	{ "EXIT", "STOP SERVER", "LEAVE", "EXIT" },
};
static int help_text_lines[][4] = {
	{ 4, 4, 4, 4, }, { 2, 2, 2, 2, }, { 3, 4, 3, 2 },
};
static const char *help_text[][4][4] = {
	{
		{
			"DIE AND",
			"RESPAWN",
			"AT YOUT LAST",
			"CHECKPOINT!"
		},
		{
			"DIE AND",
			"RESPAWN",
			"AT YOUT LAST",
			"CHECKPOINT!"
		},
		{
			"DIE AND",
			"RESPAWN",
			"AT YOUT LAST",
			"CHECKPOINT!"
		},
		{
			"DIE AND",
			"RESPAWN",
			"AT YOUT LAST",
			"CHECKPOINT!"
		},
	},
	{
		{
			"CONTINUE",
			"THE GAME",
			"",
			""
		},
		{
			"CONTINUE",
			"THE GAME",
			"",
			""
		},
		{
			"CONTINUE",
			"THE GAME",
			"",
			""
		},
		{
			"CONTINUE",
			"THE GAME",
			"",
			""
		},
	},
	{
		{
			"RETURN TO",
			"MAIN MENU",
			"AND SAVE",
			"",
		},
		{
			"STOP THE",
			"SERVER AND",
			"KICK EVERY-",
			"ONE OFF",
		},
		{
			"LEAVE THE",
			"SERVER",
			"(WONT SAVE)",
			"",
		},
		{
			"RETURN TO",
			"MAIN MENU",
			"",
			"",
		},
	},
};

int g_can_pause;

void pause_menu_init(void) {
	is_focused = CNM_FALSE;
	gui_timer = 100;
	side_xstart = -192;
	left_disp = 0;
	side_blob_x = RENDERER_WIDTH + 8;
	options_num = 1;
	exit_timer = 0;
	is_exiting = CNM_FALSE;
	g_can_pause = CNM_TRUE;
	memset(funcs, 0, sizeof(funcs));
}
void pause_menu_focus(void) {
	is_focused = CNM_TRUE;
	options_num = 1;
	Input_PushState(INPUT_STATE_GUI);
	gui_timer = 0;
}
void pause_menu_unfocus(void) {
	is_focused = CNM_FALSE;
	Input_PopState();
	gui_timer = 0;
}
void pause_menu_update(void) {
	if (gui_timer < 256) gui_timer++;
	if (!is_focused) return;
	if (is_exiting) {
		if (++exit_timer >= 30 && funcs[options_num]) {
			pause_menu_unfocus();
			funcs[options_num]();
		}
		return;
	}
	if (Input_GetButtonPressedRepeated(INPUT_DOWN, INPUT_STATE_GUI) && options_num + 1 < 3) {
		left_disp = 32;
		side_blob_x = RENDERER_WIDTH;
		options_num++;
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
	}
	int minopt = (
			(g_saves[g_current_save].lives > 1 ||
			 Interaction_GetMode() != INTERACTION_MODE_SINGLEPLAYER ||
			 Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer)
			? 0 : 1);
	if (Input_GetButtonPressedRepeated(INPUT_UP, INPUT_STATE_GUI) && options_num > minopt) {
		left_disp = -32;
		side_blob_x = RENDERER_WIDTH;
		options_num--;
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
	}
	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_GUI)) {
		pause_menu_unfocus();
	}
	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_GUI) && funcs[options_num]) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		if (options_num != PAUSE_MENU_EXIT) {
			funcs[options_num]();
		} else {
			is_exiting = CNM_TRUE;
			exit_timer = 0;
			Fadeout_FadeToWhite(20, 30, 0);
		}
	}
}
int pause_menu_isfocused(void) {
	return is_focused;
}
void pause_menu_draw(void) {
	CNM_RECT r;

	if (!is_focused && gui_timer > 15) return;
	const int min = 4;
	int trans = is_focused ? 7 - (gui_timer / 5) : min + (gui_timer / 2);
	if (trans < min) trans = min;
	if (trans > 7) trans = 7;
	Util_SetRect(&r, 0, 0, RENDERER_WIDTH, RENDERER_HEIGHT);
	Renderer_DrawRect(&r, RCOL_PAUSE_COLOR, trans, RENDERER_LIGHT);

	int text_mode;
	switch (Interaction_GetMode()) {
	case INTERACTION_MODE_SINGLEPLAYER:
		text_mode = 0;
		break;
	case INTERACTION_MODE_CLIENT:
		text_mode = 2;
		break;
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_DEDICATED_SERVER:
		text_mode = 1;
		break;
	}
	if (Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) text_mode = 3;

	const int start = 160;
	int idx = options_num - 5;
	//Console_Print("%d", options_num);
	for (int i = -1; i < start/r.h+2; i++) {
		Util_SetRect(&r, 384, 1536, 128, 32);
		Renderer_DrawBitmap(-r.w + side_xstart + i*32 + left_disp, RENDERER_HEIGHT-start + i*32 + left_disp, &r, 2, RENDERER_LIGHT);
		if (idx >= 0 && idx < sizeof(option_names)/sizeof(*option_names)) {
			int center = strlen(option_names[idx][text_mode]) * 8 / 2;
			if (idx != options_num) Renderer_SetFont(384, 576, 8, 8);
			else Renderer_SetFont(256, 192, 8, 8);
			if (!(g_saves[g_current_save].lives <= 1 && idx == 0) || Interaction_GetMode() != INTERACTION_MODE_SINGLEPLAYER) Renderer_DrawText(-r.w + side_xstart + i*32 + left_disp + (r.w / 2 - center), RENDERER_HEIGHT - start + i*32 + left_disp + 8, 0, RENDERER_LIGHT, option_names[idx][text_mode]);
			int w = r.w;
			if (idx == options_num) {
				Util_SetRect(&r, 376-24, 576 + 8*(Game_GetFrame() / 2 % 6), 8, 8);
				Renderer_DrawBitmap(-w + side_xstart + i*32 + left_disp + (w / 2 - center) - 12, RENDERER_HEIGHT - start + i*32 + left_disp + 8, &r, 0, RENDERER_LIGHT);
				Util_SetRect(&r, 376-24, 576 + 8*((Game_GetFrame() / 2 + 2) % 6), 8, 8);
				Renderer_DrawBitmap2(-w + side_xstart + i*32 + left_disp + (w / 2 + center) , RENDERER_HEIGHT - start + i*32 + left_disp + 8, &r, 0, RENDERER_LIGHT, CNM_TRUE, CNM_FALSE);
			}
			Util_SetRect(&r, 360, 576 + 8*((Game_GetFrame() / 2 + 4) % 6), 16, 8);
			int down_arrow_y = RENDERER_HEIGHT - start + i*32 + left_disp + 16;
			int down_arrow_trans = ((RENDERER_HEIGHT - 16 - 8) - down_arrow_y) / 2;
			if (down_arrow_trans < 4) down_arrow_trans = 4;
			if (down_arrow_trans > 7) down_arrow_trans = 7;
			if (idx == sizeof(option_names)/sizeof(*option_names) - 1) down_arrow_trans = 7;
			Renderer_DrawBitmap2(-w + side_xstart + i*32 + left_disp + (w / 2) - 8, down_arrow_y+3+(int)sinf((float)Game_GetFrame() / 4.0f)*3, &r, down_arrow_trans, RENDERER_LIGHT, 0, 0);
		}
		idx++;
	}

	Util_SetRect(&r, 400, 1792, 112, 48);
	Renderer_SetFont(256, 192, 8, 8);
	Renderer_DrawBitmap2(side_blob_x, RENDERER_HEIGHT-48, &r, 2, RENDERER_LIGHT, CNM_TRUE, CNM_FALSE);
	int text_off = 12;
	if (help_text_lines[options_num][text_mode] == 3) text_off += 6;
	if (help_text_lines[options_num][text_mode] == 2) text_off += 6;
	for (int i = 0; i < 4; i++) {
		int align_right = (12 - strlen(help_text[options_num][text_mode][i])) * 8;
		Renderer_DrawText(side_blob_x + 8 + align_right, RENDERER_HEIGHT-48+text_off+i*8, 0, RENDERER_LIGHT, help_text[options_num][text_mode][i]);
	}

	int target = is_focused ? 0 : -192;
	side_xstart += (target - side_xstart) * 0.25f;
	target = is_focused ? RENDERER_WIDTH - r.w : RENDERER_WIDTH + 8;
	side_blob_x += (target - side_blob_x) * 0.25f;
	left_disp += (0 - left_disp) * 0.25f;
}
void pause_menu_setcallback(enum pause_menu_funcs ty, pause_menu_func_t func) {
	funcs[ty] = func;
}
