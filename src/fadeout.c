#include "fadeout.h"
#include "utility.h"
#include "renderer.h"

#define TYPE_WHITE 0
#define TYPE_BLACK 1
#define TYPE_DEATH 2

#define DEATH_ALL_BLACK 10

static int fade_type = TYPE_WHITE;
static int fade_counter, fade_in, fade_time, fade_out, fade_total;

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
void Fadeout_FadeDeath(int fadein, int fade, int fadeout)
{
	fade_type = TYPE_DEATH;
	fade_counter = 0;
	fade_in = fadein;
	fade_time = fade;
	fade_out = fadeout;
	fade_total = fadein + fade + fadeout + DEATH_ALL_BLACK;
}
void Fadeout_StepFade(void)
{
	if (fade_total)
	{
		if (fade_counter++ == fade_total)
			fade_total = 0;
	}
}
void Fadeout_ApplyFade(void)
{
	CNM_RECT r = {0, 0, RENDERER_WIDTH, RENDERER_HEIGHT};
	int lvl, fc = Renderer_MakeColor(0, 0, 0), offsx = 0, offsy = 0;
	if (!fade_total)
		return;
	switch (fade_type)
	{
	case TYPE_WHITE:
		fc = Renderer_MakeColor(255, 255, 255);
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
		Util_SetRect(&r, 128, 1888, 128, 64);
		if (fade_counter < 25)
		{
			offsx = Util_RandInt(-(25 - fade_counter), 25 - fade_counter);
			offsy = Util_RandInt(-(25 - fade_counter), 25 - fade_counter);
		}
		if (fade_counter < fade_in + fade_time + 1)
		{
			Renderer_DrawBitmap(RENDERER_WIDTH / 2 - 64 + offsx, RENDERER_HEIGHT / 2 - 32 + offsy, &r, 0, RENDERER_LIGHT);
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
	}
}