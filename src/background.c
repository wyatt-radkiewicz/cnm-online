#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "background.h"
#include "game.h"
#include "renderer.h"

static BACKGROUND_LAYER bglayers[BACKGROUND_MAX_LAYERS];
static int bg_start = 0;
static int bg_end = BACKGROUND_MAX_LAYERS - 1;

void Background_ResetBackgrounds(void)
{
	memset(bglayers, 0, sizeof(bglayers));
	bg_start = 0;
	bg_end = BACKGROUND_MAX_LAYERS - 1;
}
BACKGROUND_LAYER *Background_GetLayer(int layer)
{
	return &bglayers[layer];
}
//int RoundAwayFromZero(float f)
//{
//	return (f < 0.0f) ? (int)floorf(f) : (int)ceilf(f);
//}
int PositiveModulo(int x, int m)
{
	if (m == 0)
		return 0;
	int mod = x % m;
	if (mod < 0)
		mod += m;
	return mod;
}
typedef void(*draw_callback_t)(const float pos[0]);
static void draw_repeated(
	int width,
	int height,
	int camx,
	int camy,
	const float pos[2],
	const int padding[2],
	int repeat_horiz,
	int repeat_vert,
	draw_callback_t drawer) {

	float cam_scrolled[2];
	int final_pos[2];
	int bitmaps[2];
	int size[2];
	int padded_size[2];
	padded_size[0] = width + padding[0];
	padded_size[1] = height + padding[1];

	bitmaps[0] = (int)ceilf((float)RENDERER_WIDTH / (float)padded_size[0]);
	bitmaps[1] = (int)ceilf((float)RENDERER_HEIGHT / (float)padded_size[1]);
	size[0] = bitmaps[0] * padded_size[0];
	size[1] = bitmaps[1] * padded_size[1];

	if (!repeat_horiz)
	{
		final_pos[0] = (int)pos[0];
		bitmaps[0] = 1;
	}
	else
	{
		final_pos[0] = PositiveModulo((int)pos[0] + size[0], size[0]) - size[0];
	}
	if (!repeat_vert)
	{
		final_pos[1] = (int)pos[1];
		bitmaps[1] = 1;
	}
	else if (repeat_vert == BACKGROUND_REPEAT_DOWN)
	{
		if (pos[1] < 0.0f)
			final_pos[1] = PositiveModulo((int)pos[1] + size[1], size[1]) - size[1];
		else
			final_pos[1] = (int)pos[1];
	}
	else if (repeat_vert == BACKGROUND_REPEAT_UP)
	{
		if (pos[1] > (float)RENDERER_HEIGHT)
			final_pos[1] = PositiveModulo((int)pos[1] + size[1], size[1]) - size[1];
		else
			final_pos[1] = (int)pos[1] - size[1] * 2 + padded_size[1];
	}
	else if (repeat_vert == BACKGROUND_REPEAT_BOTH)
	{
		final_pos[1] = PositiveModulo((int)pos[1] + size[1], size[1]) - size[1];
	}

	for (int x = 0; x < (repeat_horiz ? (bitmaps[0] * 2) : 1); x++)
	{
		for (int y = 0; y < (repeat_vert ? (bitmaps[1] * 2) : 1); y++)
		{
			float sending_pos[2] = {
				final_pos[0] + (x * padded_size[0]),
				final_pos[1] + (y * padded_size[1]),
			};
			drawer(sending_pos);
			//Renderer_DrawBitmap
			//(
			//	final_pos[0] + (x * padded_size[0]),
			//	final_pos[1] + (y * padded_size[1]),
			//	src,
			//	trans,
			//	RENDERER_LIGHT
			//);
		}
	}
}
static int draw_bg_layer_id;
static void draw_bg_layer(const float pos[2]) {
	BACKGROUND_LAYER *l = &bglayers[draw_bg_layer_id];
	Renderer_DrawBitmap
	(
		pos[0], pos[1],
		&l->bitmap,
		l->transparency,
		RENDERER_LIGHT
	);
}
static float width3d, yidx_3d, move_pixels;
static void draw_bg_3d(const float pos[2]) {
	BACKGROUND_LAYER *l = &bglayers[draw_bg_layer_id];
	
	if ((int)move_pixels == 0) {
		Renderer_DrawStetchedSpan(
			pos[0],
			pos[1],
			width3d,
			l->bitmap.x,
			l->bitmap.y + yidx_3d,
			l->bitmap.w,
			l->transparency
		);
	} else if ((int)move_pixels < 0) {
		int dstsize = (int)(-move_pixels * (width3d / (float)l->bitmap.w));
		Renderer_DrawStetchedSpan(
			pos[0],
			pos[1],
			width3d - dstsize,
			l->bitmap.x - (int)move_pixels,
			l->bitmap.y + yidx_3d,
			l->bitmap.w + (int)move_pixels,
			l->transparency
		);
		Renderer_DrawStetchedSpan(
			pos[0] + (width3d - dstsize),
			pos[1],
			dstsize,
			l->bitmap.x,
			l->bitmap.y + yidx_3d,
			-(int)move_pixels,
			l->transparency
		);
	} else if ((int)move_pixels > 0) {
		int dstsize = (int)(move_pixels * (width3d / (float)l->bitmap.w));
		Renderer_DrawStetchedSpan(
			pos[0],
			pos[1],
			dstsize,
			l->bitmap.x + l->bitmap.w - (int)move_pixels,
			l->bitmap.y + yidx_3d,
			(int)move_pixels,
			l->transparency
		);
		Renderer_DrawStetchedSpan(
			pos[0] + dstsize,
			pos[1],
			width3d - dstsize,
			l->bitmap.x,
			l->bitmap.y + yidx_3d,
			l->bitmap.w - (int)move_pixels,
			l->transparency
		);
	}
}
#include "console.h"
void Background_DrawLayer(int layer, int camx, int camy)
{
	BACKGROUND_LAYER *l = &bglayers[layer];
	float pos[2];
	float cam_scrolled[2];
	if (l->clear_color == 0)
	{
		if (l->bitmap.w == 0 || l->bitmap.h == 0)
			return;

		draw_bg_layer_id = layer;
		cam_scrolled[0] = (l->scroll[0] != 0.0f) ? ((float)camx / l->scroll[0]) : 0.0f;
		cam_scrolled[1] = (l->scroll[1] != 0.0f) ? ((float)camy / l->scroll[1]) : 0.0f;
		if (l->top3d == 0 || l->bottom3d == 0 || l->height3d == 0) {
			l->pos[0] += l->speed[0];
			l->pos[1] += l->speed[1];
			pos[0] = (l->pos[0] + l->origin[0]) - cam_scrolled[0];
			pos[1] = (l->pos[1] + l->origin[1]) - cam_scrolled[1];
			draw_repeated(
				l->bitmap.w, l->bitmap.h,
				camx, camy,
				pos,
				l->spacing,
				l->repeat[0],
				l->repeat[1],
				draw_bg_layer
			);

			return;
		}

		// 3d stuff
		float slope3d = (float)(l->bottom3d - l->top3d) / (float)l->height3d;
		width3d = (float)l->top3d;
		int old_width3d = (int)width3d;
		int spacing[2] = { 0, 0 };
		float scrollspd = (float)l->spacing[1] / 100.0f;
		if (l->scroll[0] != 0.0f) {
			move_pixels = -camx / l->scroll[0];
			move_pixels = (int)move_pixels % l->bitmap.w;
			move_pixels = (int)(move_pixels + (float)Game_GetFrame() * l->speed[0]) % l->bitmap.w;
		} else {
			move_pixels = (int)((float)Game_GetFrame() * l->speed[0]) % l->bitmap.w;
		}
		yidx_3d = PositiveModulo((int)((float)Game_GetFrame() * l->speed[1]), l->bitmap.h);
		float yidx_spd = CNM_MAX(l->top3d, l->bottom3d) / width3d;

		for (int y = 0; y < l->height3d; y++) {
			pos[0] = l->origin[0] + (float)RENDERER_WIDTH / 2.0f;// - cam_scrolled[0];
			pos[1] = (l->origin[1] + y - 0.5f) - cam_scrolled[1];
			draw_repeated(
				(int)width3d, 1, camx, camy,
				pos, spacing, CNM_TRUE, 0, draw_bg_3d
			);
			old_width3d = (int)width3d;
			width3d += slope3d;
			yidx_spd = CNM_MAX(l->top3d, l->bottom3d) / width3d;
			yidx_3d += yidx_spd;
			if (yidx_3d > l->bitmap.h) yidx_3d = 0.0f;
			if ((int)width3d != old_width3d) scrollspd *= (float)l->spacing[0] / 100.0f;
		}
	}
	else
	{
		Renderer_Clear(l->clear_color);
	}
}
void Background_Draw(int layer, int camx, int camy)
{
	for (int i = bg_start; i <= bg_end; i++)
	{
		if (i > -1 && i < BACKGROUND_MAX_LAYERS && bglayers[i].high == layer)
			Background_DrawLayer(i, camx, camy);
	}
}
void Background_SetVisibleLayers(int id_start, int id_end)
{
	bg_start = id_start;
	bg_end = id_end;
}
