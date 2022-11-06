#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "background.h"
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
void Background_DrawLayer(int layer, int camx, int camy)
{
	BACKGROUND_LAYER *l = &bglayers[layer];
	float pos[2];
	float cam_scrolled[2];
	int final_pos[2];
	int bitmaps[2];
	int size[2];
	int padded_size[2];
	if (l->clear_color == 0)
	{
		if (l->bitmap.w == 0 || l->bitmap.h == 0)
			return;

		padded_size[0] = l->bitmap.w + l->spacing[0];
		padded_size[1] = l->bitmap.h + l->spacing[1];

		l->pos[0] += l->speed[0];
		l->pos[1] += l->speed[1];
		cam_scrolled[0] = (l->scroll[0] != 0.0f) ? ((float)camx / l->scroll[0]) : 0.0f;
		cam_scrolled[1] = (l->scroll[1] != 0.0f) ? ((float)camy / l->scroll[1]) : 0.0f;
		pos[0] = (l->pos[0] + l->origin[0]) - cam_scrolled[0];
		pos[1] = (l->pos[1] + l->origin[1]) - cam_scrolled[1];
		bitmaps[0] = (int)ceilf((float)RENDERER_WIDTH / (float)padded_size[0]);
		bitmaps[1] = (int)ceilf((float)RENDERER_HEIGHT / (float)padded_size[1]);
		size[0] = bitmaps[0] * padded_size[0];
		size[1] = bitmaps[1] * padded_size[1];

		if (!l->repeat[0])
		{
			final_pos[0] = (int)pos[0];
			bitmaps[0] = 1;
		}
		else
		{
			final_pos[0] = PositiveModulo((int)pos[0] + size[0], size[0]) - size[0];
		}
		if (!l->repeat[1])
		{
			final_pos[1] = (int)pos[1];
			bitmaps[1] = 1;
		}
		else if (l->repeat[1] == BACKGROUND_REPEAT_DOWN)
		{
			if (pos[1] < 0.0f)
				final_pos[1] = PositiveModulo((int)pos[1] + size[1], size[1]) - size[1];
			else
				final_pos[1] = (int)pos[1];
		}
		else if (l->repeat[1] == BACKGROUND_REPEAT_UP)
		{
			if (pos[1] > (float)RENDERER_HEIGHT)
				final_pos[1] = PositiveModulo((int)pos[1] + size[1], size[1]) - size[1];
			else
				final_pos[1] = (int)pos[1] - size[1] * 2 + padded_size[1];
		}
		else if (l->repeat[1] == BACKGROUND_REPEAT_BOTH)
		{
			final_pos[1] = PositiveModulo((int)pos[1] + size[1], size[1]) - size[1];
		}

		for (int x = 0; x < (l->repeat[0] ? (bitmaps[0] * 2) : 1); x++)
		{
			for (int y = 0; y < (l->repeat[1] ? (bitmaps[1] * 2) : 1); y++)
			{
				Renderer_DrawBitmap
				(
					final_pos[0] + (x * padded_size[0]),
					final_pos[1] + (y * padded_size[1]),
					&l->bitmap,
					l->transparency,
					RENDERER_LIGHT
				);
			}
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