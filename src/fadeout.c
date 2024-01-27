#include <math.h>
#include "fadeout.h"
#include "utility.h"
#include "renderer.h"
#include "audio.h"

#define TYPE_WHITE 0
#define TYPE_BLACK 1
#define TYPE_DEATH 2
#define TYPE_GAMEOVER 3

#define DEATH_ALL_BLACK 10

static int fade_type = TYPE_WHITE;
static int fade_counter, fade_in, fade_time, fade_out, fade_total;
static float gameover_y, gameover_yspd;

void Fadeout_Setup(void)
{
	fade_type = TYPE_WHITE;
	fade_counter = fade_total = 0;
}
void Fadeout_FadeToWhite(int fadein, int fade, int fadeout)
{
	fade_type = TYPE_WHITE;
	fade_counter = 0;
	fade_in = fadein;
	fade_time = fade;
	fade_out = fadeout;
	fade_total = fadein + fade + fadeout;
}
void Fadeout_FadeToBlack(int fadein, int fade, int fadeout)
{
	fade_type = TYPE_BLACK;
	fade_counter = 0;
	fade_in = fadein;
	fade_time = fade;
	fade_out = fadeout;
	fade_total = fadein + fade + fadeout;
}
void Fadeout_FadeFromBlack(int init, int fadefrom)
{
	fade_type = TYPE_BLACK;
	fade_counter = 0;
	fade_in = 0;
	fade_time = init;
	fade_out = fadefrom;
	fade_total = fade_in + init + fade_out;
}
void Fadeout_FadeFromWhite(int init, int fadefrom)
{
	fade_type = TYPE_WHITE;
	fade_counter = 0;
	fade_in = 0;
	fade_time = init;
	fade_out = fadefrom;
	fade_total = fade_in + init + fade_out;
}
void Fadeout_FadeDeath(int fadein, int fade, int fadeout)
{
	fade_type = TYPE_DEATH;
	fade_counter = 0;
	fade_in = fadein;
	fade_time = fade;
	fade_out = fadeout;
	fade_total = fadein + fade + fadeout + DEATH_ALL_BLACK;
}
void Fadeout_FadeGameOver(int fadein, int animtime, int fadeout, int fade) {
	fade_type = TYPE_GAMEOVER;
	gameover_y = -64;
	gameover_yspd = 5.0f;
	fade_counter = 0;
	fade_in = fadein;
	fade_time = animtime;
	fade_out = fadeout;
	fade_total = fadein + animtime + fadeout + fade + DEATH_ALL_BLACK;
}
void Fadeout_StepFade(void)
{
	if (fade_total)
	{
		if (fade_counter++ == fade_total)
			fade_total = 0;
	}

	if (fade_type == TYPE_GAMEOVER && fade_counter >= fade_in) {
		const float floor = (float)RENDERER_HEIGHT / 2.f - 32.f;
		if (gameover_y > floor && gameover_yspd > 0.0f) {
			Audio_PlaySound(48, 0, Audio_GetListenerX(), Audio_GetListenerY());
			gameover_yspd = -fabsf(gameover_yspd) * 0.6f;
			if (gameover_yspd > -1.0f) gameover_yspd = 0.0f;
			gameover_y = floor;
		}
		gameover_yspd += 0.5f;
		gameover_y += gameover_yspd;
	}
}
void Fadeout_ApplyFade(void)
{
	CNM_RECT r = {0, 0, RENDERER_WIDTH, RENDERER_HEIGHT};
	int lvl, fc = RCOL_BLACK, offsx = 0, offsy = 0;
	if (!fade_total)
		return;
	switch (fade_type)
	{
	case TYPE_WHITE:
		fc = RCOL_WHITE;
	case TYPE_BLACK:
		if (fade_counter < fade_in)
			lvl = 7 - (int)((float)fade_counter / (float)fade_in * 7.0f);
		else if (fade_counter > fade_in + fade_time)
			lvl = (int)((float)(fade_counter - fade_in - fade_time) / (float)fade_out * 7.0f);
		else
			lvl = 0;
		Renderer_DrawRect(&r, fc, lvl, RENDERER_LIGHT);
		break;
	case TYPE_DEATH:
		Util_SetRect(&r, 128, 832, 128, 64);
		if (fade_counter < 25)
		{
			offsx = Util_RandInt(-(25 - fade_counter), 25 - fade_counter);
			offsy = Util_RandInt(-(25 - fade_counter), 25 - fade_counter);
		}
		if (fade_counter < fade_in + fade_time + 1)
		{
			Util_SetRect(&r, 384, 896, 128, 32);
			Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - 64 + offsx, RENDERER_HEIGHT / 2 - 32 + offsy, &r, 3, RENDERER_LIGHT, CNM_FALSE, CNM_FALSE);
			Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - 64 + offsx, RENDERER_HEIGHT / 2 - 32 + offsy+32, &r, 3, RENDERER_LIGHT, CNM_FALSE, CNM_TRUE);
			Util_SetRect(&r, 128, 832, 96, 64);
			Renderer_DrawBitmap(RENDERER_WIDTH / 2 - 48 + offsx, RENDERER_HEIGHT / 2 - 32 + offsy, &r, 0, RENDERER_LIGHT);
		}
		Util_SetRect(&r, 0, 0, RENDERER_WIDTH, RENDERER_HEIGHT);
		if (fade_counter > fade_in)
		{
			if (fade_counter < fade_in + fade_time)
				lvl = 7 - (int)((float)(fade_counter - fade_in) / (float)fade_time * 7.0f);
			else if (fade_counter > fade_in + fade_time + DEATH_ALL_BLACK)
				lvl = (int)((float)(fade_counter - fade_in - fade_time - DEATH_ALL_BLACK) / (float)fade_out * 7.0f);
			else
				lvl = 0;
			Renderer_DrawRect(&r, fc, lvl, RENDERER_LIGHT);
		}
		break;
	case TYPE_GAMEOVER:
		if (fade_counter <= fade_in) lvl = (1.0f - ((float)fade_counter / (float)fade_in)) * 7.0f;
		else lvl = 0;
		fc = RCOL_BLACK;
		Util_SetRect(&r, 0, 0, RENDERER_WIDTH, RENDERER_HEIGHT);
		Renderer_DrawRect(&r, fc, lvl, RENDERER_LIGHT);
		Util_SetRect(&r, 400, 1408, 112, 48);
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 - r.w / 2, gameover_y, &r, 0, RENDERER_LIGHT);
		if (fade_counter >= fade_in + fade_time + fade_out) lvl = 0;
		else if (fade_counter >= fade_in + fade_time) lvl = (1.0f - ((float)(fade_counter - fade_in - fade_time) / (float)fade_out)) * 7.0f;
		else lvl = 7;
		fc = RCOL_WHITE;
		Util_SetRect(&r, 0, 0, RENDERER_WIDTH, RENDERER_HEIGHT);
		Renderer_DrawRect(&r, fc, lvl, RENDERER_LIGHT);
		break;
	}
}
