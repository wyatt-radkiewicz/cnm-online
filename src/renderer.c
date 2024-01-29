#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <SDL.h>
#include <SDL_surface.h>
#include <SDL_video.h>
#include "renderer.h"
#include "console.h"
#include "game.h"

#define RENDERER_LEVELS 8

static SDL_Window *renderer_win;
static SDL_Surface *renderer_scr;
static SDL_Surface *renderer_gfx;
static SDL_Surface *renderer_hires_temp;
static SDL_Surface *renderer_effects_buf;
static unsigned char renderer_trans[256][256][RENDERER_LEVELS]; /* Source color, Destination Color, Transparency level */
static unsigned char renderer_light[256][RENDERER_LEVELS]; /* Color, Light level */
static CNM_RECT renderer_font;
static int renderer_fullscreen;
static int renderer_initialized = CNM_FALSE;
static int renderer_hires_mode;
static int renderer_widescreen;

int RENDERER_WIDTH;
int RENDERER_HEIGHT;

//extern unsigned char *_renderer_asm_scr_pixels;
//extern unsigned char *_renderer_asm_gfx_pixels;

static int Abs(int i);
//static void Renderer_BuildConvTable(void);
static void Renderer_UpdateWindowFromSettings(void);
static int get_nearest_color(int r, int g, int b);

static void Renderer_SetPixel(int x, int y, int color, int trans, int light);

#define MAX_SVBIN 512
static int supervirus_binary_pos[MAX_SVBIN][3];

void Renderer_Init(int start_fullscreen, int hi_res, int widescreen)
{
	RENDERER_WIDTH = widescreen ? 424 : 320;
	RENDERER_HEIGHT = 240;
	
	renderer_fullscreen = start_fullscreen;
	renderer_hires_mode = hi_res;
	renderer_widescreen = widescreen;
	renderer_hires_temp = NULL;
	Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer = start_fullscreen;
	Game_GetVar(GAME_VAR_HIRESMODE)->data.integer = hi_res;
	renderer_win = NULL;/*SDL_CreateWindow
	(
		"Cnm Online Prototype Version 0000.00.0001 Beta",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		RENDERER_WIDTH,
		RENDERER_HEIGHT,
		SDL_WINDOW_SHOWN | (renderer_fullscreen ? SDL_WINDOW_FULLSCREEN : 0)
	);*/
	Renderer_UpdateWindowFromSettings();
	renderer_scr = NULL;
	renderer_gfx = NULL;
	memset(supervirus_binary_pos, 0, sizeof(supervirus_binary_pos));
	int i;
	for (i = 0; i < MAX_SVBIN; i++)
	{
		supervirus_binary_pos[i][1] = rand() % RENDERER_HEIGHT;
		supervirus_binary_pos[i][0] = rand() % RENDERER_WIDTH;
		supervirus_binary_pos[i][2] = rand() % 2;
	}
	renderer_initialized = CNM_TRUE;
}
void Renderer_Quit(void)
{
	SDL_FreeSurface(renderer_effects_buf);
	SDL_FreeSurface(renderer_hires_temp);
	SDL_FreeSurface(renderer_gfx);
	SDL_FreeSurface(renderer_scr);
	SDL_DestroyWindow(renderer_win);
	renderer_initialized = CNM_FALSE;
}
void Renderer_SetFullscreen(int fullscreen)
{
	int old = renderer_fullscreen;
	if (!renderer_initialized)
		return;

	renderer_fullscreen = fullscreen;
	if (old != renderer_fullscreen)
		Renderer_UpdateWindowFromSettings();
}
/*void Renderer_StartDrawing(void)
{
	SDL_LockSurface(renderer_gfx);
	SDL_LockSurface(renderer_scr);
}*/

#include "supervirus.h"
void Renderer_Update(void)
{
	if (!renderer_initialized)
		return;

	if (Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer) {
		SDL_Color colors[256];
		int i, j;
		for (i = 0; i < 256; i++)
		{
			j = (i % 2) * 255;
			colors[i] = (SDL_Color) {j, 0, 0, 255};
		}
		//SDL_SetPaletteColors(renderer_gfx->format->palette, colors, 0, 0x100);
		//SDL_SetPaletteColors(renderer_scr->format->palette, colors, 0, 0x100);
		SDL_SetPaletteColors(renderer_scr->format->palette, colors, 0, 0x100);

		for (i = 0; i < MAX_SVBIN; i++) {
			Renderer_DrawText(supervirus_binary_pos[i][0], supervirus_binary_pos[i][1], 0, RENDERER_LIGHT,
							  supervirus_binary_pos[i][2] ? "0" : "1");

			supervirus_binary_pos[i][1] += 6;
			if (supervirus_binary_pos[i][1] > RENDERER_HEIGHT) {
				supervirus_binary_pos[i][1] = rand() % 32 - 32;
				supervirus_binary_pos[i][0] = rand() % RENDERER_WIDTH;
				supervirus_binary_pos[i][2] = rand() % 2;
			}
		}

		supervirus_render_all();

		Renderer_DrawText(50 + rand() % 5, 50 + rand() % 5, 0, RENDERER_LIGHT, "THE SUPERVARUS EXECUTES!!!!");
		Renderer_DrawText(80 + rand() % 5, 70 + rand() % 5, 0, RENDERER_LIGHT, "YOULL NEVER MAKE IT OUT ALIVE!!!!");
		Renderer_DrawText(70 + rand() % 5, 80 + rand() % 5, 0, RENDERER_LIGHT, "HAHAHHAHAHAHHAHAHHA!!!!");
		Renderer_DrawText(80 + rand() % 5, 100 + rand() % 5, 0, RENDERER_LIGHT, "WELCOME TO MY WORRRLRLRLLDLDLLDLD!!!!");
	}

	//SDL_LockSurface(SDL_GetWindowSurface(renderer_win));
	if (!renderer_hires_mode)
		SDL_BlitSurface(renderer_scr, NULL, SDL_GetWindowSurface(renderer_win), NULL);
	else {
		SDL_Surface *winsurf = SDL_GetWindowSurface(renderer_win);
		SDL_FillRect(winsurf, NULL, SDL_MapRGB(winsurf->format, 10, 27, 71));
		SDL_BlitSurface(renderer_scr, NULL, renderer_hires_temp, NULL);
		SDL_Rect r = {0, 0, RENDERER_WIDTH * 3, RENDERER_HEIGHT * 3};
		if (renderer_fullscreen) {
			SDL_DisplayMode dm;
			SDL_GetCurrentDisplayMode(0, &dm);
			if (!renderer_widescreen) {
				r.h = dm.h;
				r.w = dm.h / 3 * 4;
				r.x = (dm.w - r.w) / 2;
				r.y = 0;
			} else {
				if ((float)r.w / (float)r.h < 16.0f/9.0f) {
					r.w = dm.w;
					r.h = dm.w / 16 * 9;
					r.y = (dm.h - r.h) / 2;
					r.x = 0;
				} else {
					r.h = dm.h;
					r.w = dm.h / 9 * 16;
					r.x = (dm.w - r.w) / 2;
					r.y = 0;
				}
			}
		}
		SDL_BlitScaled(renderer_hires_temp, NULL, winsurf, &r);
		//SDL_Surface *temp_scr = SDL_CreateRGBSurface(0, RENDERER_WIDTH * 2, RENDERER_HEIGHT * 2, 32, 0, 0, 0, 0);
		//SDL_Surface *wsurf = SDL_GetWindowSurface(renderer_win);
		//Uint32 colors[256];
		//{
		//	SDL_Color *unconvcols = renderer_scr->format->palette->colors;
		//	for (int i = 0; i < 256; i++)
		//	{
		//		colors[i] = SDL_MapRGBA(
		//			wsurf->format,
		//			unconvcols[i].r,
		//			unconvcols[i].g,
		//			unconvcols[i].b,
		//			0xff
		//		);
		//	}
		//}
		//Uint32 *dst = (Uint32 *)wsurf->pixels;
		//Uint8 *src = (Uint8 *)renderer_scr->pixels;
		//int x = 0, y = 0;

//#define REPEAT8(X) X X X X X X X X
//#define REPEAT16(X) REPEAT8(X) REPEAT8(X)
//#define REPEAT32(X) REPEAT16(X) REPEAT16(X)
//#define REPEAT64(X) REPEAT32(X) REPEAT32(X)
//#define REPEATWIDTH(X) REPEAT64(X) REPEAT64(X) REPEAT64(X) REPEAT64(X) REPEAT64(X)
		//for (; y < RENDERER_HEIGHT;)
		//{
		//	REPEATWIDTH(
		//		*dst = colors[*src];
		//		dst++;
		//		*dst = colors[*src];
		//		src++; dst++;
		//	)
		//	src -= RENDERER_WIDTH;
		//	REPEATWIDTH(
		//		*dst = colors[*src];
		//		dst++;
		//		*dst = colors[*src];
		//		src++; dst++;
		//	)
		//	y++;
		//}
		//SDL_BlitSurface(renderer_hires_temp, NULL, SDL_GetWindowSurface(renderer_win), NULL);
		/*SDL_PixelFormat *screen_format = renderer_scr->format;
		Uint32 *pixels = (Uint32 *)(renderer_hires_temp->pixels);
		int x, y;
		for (y = 0; y < RENDERER_HEIGHT * 2; y++)
		{
			for (x = 0; x < RENDERER_WIDTH * 2; x++)
			{
				Uint32 *color_bytes = pixels + (y * RENDERER_WIDTH * 2 + x);
				unsigned char index = ((unsigned char *)renderer_scr->pixels)[(y / 2) * RENDERER_WIDTH + (x / 2)];
				*color_bytes = SDL_MapRGBA(
					renderer_hires_temp->format,
					screen_format->palette->colors[index].r,
					screen_format->palette->colors[index].g,
					screen_format->palette->colors[index].b,
					0xff
				);
			}
		}
		SDL_BlitSurface(renderer_hires_temp, NULL, SDL_GetWindowSurface(renderer_win), NULL);*/
		//SDL_FreeSurface(temp_scr);
	}
	//SDL_UnlockSurface(SDL_GetWindowSurface(renderer_win));
	//SDL_UnlockSurface(renderer_scr);
	//SDL_UnlockSurface(renderer_gfx);
	SDL_UpdateWindowSurface(renderer_win);
}
void Renderer_SetHiResMode(int hi_res)
{
	int old_mode = renderer_hires_mode;
	if (!renderer_initialized || old_mode == hi_res)
		return;
	renderer_hires_mode = hi_res;
	Renderer_UpdateWindowFromSettings();
}
void Renderer_WidescreenMode(int widescreen)
{
	if (!renderer_initialized || renderer_widescreen == widescreen)
		return;
	renderer_widescreen = widescreen;
	Renderer_UpdateWindowFromSettings();
}
void Renderer_RestartWindow(void)
{
	if (!renderer_initialized)
		return;
	Renderer_UpdateWindowFromSettings();
}
void Renderer_SetScreenModeFull(int fullscreen, int hi_res, int widescreen)
{
	int old_mode[2] = {renderer_fullscreen, renderer_hires_mode};
	if (!renderer_initialized || (old_mode[1] == hi_res && old_mode[0] == fullscreen && renderer_widescreen == widescreen))
		return;
	renderer_hires_mode = hi_res;
	renderer_fullscreen = fullscreen;
	renderer_widescreen = widescreen;
	Renderer_UpdateWindowFromSettings();
}
static void Renderer_UpdateWindowFromSettings(void)
{
	Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer = renderer_fullscreen;
	Game_GetVar(GAME_VAR_HIRESMODE)->data.integer = renderer_hires_mode;
	Game_GetVar(GAME_VAR_WIDESCREEN)->data.integer = renderer_widescreen;

	RENDERER_WIDTH = renderer_widescreen ? 424 : 320;
	RENDERER_HEIGHT = 240;

	char windowname[128];
	strcpy(windowname, "Cnm Online ");
	strcat(windowname, CNM_VERSION_STRING);
	int width = RENDERER_WIDTH;
	int height = RENDERER_HEIGHT;
	if (renderer_hires_mode)
	{
		width *= 3;
		height *= 3;
	}
	if (renderer_fullscreen) {
		// non 4 by 3 aspect ratio assume width is more than height
		SDL_DisplayMode dm;
		SDL_GetCurrentDisplayMode(0, &dm);
		height = dm.h;
		width = dm.w;
	}
	//if (renderer_hires_temp != NULL) SDL_FreeSurface(renderer_hires_temp);
	if (renderer_win != NULL) SDL_DestroyWindow(renderer_win);
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
	renderer_win = SDL_CreateWindow
	(
		windowname,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		SDL_WINDOW_SHOWN | (renderer_fullscreen ? SDL_WINDOW_FULLSCREEN : 0)
	);
	if (renderer_gfx) {
		if (renderer_scr) SDL_FreeSurface(renderer_scr);
		if (renderer_effects_buf) SDL_FreeSurface(renderer_effects_buf);
		renderer_scr = SDL_CreateRGBSurfaceWithFormat(0, RENDERER_WIDTH, RENDERER_HEIGHT, 8, SDL_PIXELFORMAT_INDEX8);
		SDL_SetPaletteColors(renderer_scr->format->palette, renderer_gfx->format->palette->colors, 0, 0x100);
		renderer_effects_buf = SDL_CreateRGBSurfaceWithFormat(0, RENDERER_WIDTH, RENDERER_HEIGHT, 8, SDL_PIXELFORMAT_INDEX8);
		SDL_SetPaletteColors(renderer_effects_buf->format->palette, renderer_gfx->format->palette->colors, 0, 0x100);
	}
	if (!renderer_hires_mode)
		SDL_ConvertSurfaceFormat(SDL_GetWindowSurface(renderer_win), SDL_PIXELFORMAT_INDEX8, 0);
	else
	{
		SDL_FreeSurface(renderer_hires_temp);
		renderer_hires_temp = SDL_CreateRGBSurfaceWithFormat(0, RENDERER_WIDTH, RENDERER_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
		SDL_ConvertSurfaceFormat(SDL_GetWindowSurface(renderer_win), SDL_PIXELFORMAT_RGBA32, 0);
	}
}

void Renderer_LoadBitmap(const char *gfx_file)
{
	if (!renderer_initialized)
		return;

	renderer_gfx = SDL_LoadBMP(gfx_file);
	if (renderer_gfx == NULL)
	{
		Console_Print("Cannot load the graphics file: \"%s\"!", gfx_file);
		return;
	}
	if (renderer_gfx->format->BitsPerPixel != 8 || renderer_gfx->format->format != SDL_PIXELFORMAT_INDEX8)
	{
		Console_Print("The graphics file is not 8bpp: \"%s\"!", gfx_file);
		SDL_FreeSurface(renderer_gfx);
		return;
	}
	if (renderer_scr) SDL_FreeSurface(renderer_scr);
	if (renderer_effects_buf) SDL_FreeSurface(renderer_effects_buf);
	renderer_scr = SDL_CreateRGBSurfaceWithFormat(0, RENDERER_WIDTH, RENDERER_HEIGHT, 8, SDL_PIXELFORMAT_INDEX8);
	SDL_SetPaletteColors(renderer_scr->format->palette, renderer_gfx->format->palette->colors, 0, 0x100);
	renderer_effects_buf = SDL_CreateRGBSurfaceWithFormat(0, RENDERER_WIDTH, RENDERER_HEIGHT, 8, SDL_PIXELFORMAT_INDEX8);
	SDL_SetPaletteColors(renderer_effects_buf->format->palette, renderer_gfx->format->palette->colors, 0, 0x100);
	//_renderer_asm_gfx_pixels = renderer_gfx->pixels;
	//_renderer_asm_scr_pixels = renderer_scr->pixels;
}
void Renderer_SaveToEffectsBuffer(void) {
	SDL_BlitSurface(renderer_scr, NULL, renderer_effects_buf, NULL);
}
void Renderer_DrawVertRippleEffect(const CNM_RECT *rect, float period, float amp, float spd) {
	CNM_RECT r = (CNM_RECT) {
		.w = RENDERER_WIDTH, .h = RENDERER_HEIGHT,
		.x = 0, .y = 0,
	};
	if (rect != NULL) r = *rect;
	if (r.x > RENDERER_WIDTH || r.x + r.w < 0 ||
		r.y > RENDERER_HEIGHT || r.y + r.h < 0) return;
	if (r.x + r.w > RENDERER_WIDTH) r.w = RENDERER_WIDTH - r.x;
	if (r.y + r.h > RENDERER_HEIGHT) r.h = RENDERER_HEIGHT - r.y;
	if (r.x < 0) {
		r.w += r.x;
		r.x = 0;
	}
	if (r.y < 0) {
		r.h += r.y;
		r.y = 0;
	}

	for (int y = r.y; y < r.y + r.h; y++) {
		int ys = y + (int)(sinf((y + ((float)Game_GetFrame() * spd)) * CNM_PI / period) * amp);
		if (ys < 0) ys = 0;
		if (ys > RENDERER_HEIGHT - 1) ys = RENDERER_HEIGHT - 1;

		const unsigned char *srcptr = ((unsigned char *)renderer_effects_buf->pixels) + (r.x + ys * RENDERER_WIDTH);
		unsigned char *dstptr = ((unsigned char *)renderer_scr->pixels) + (r.x + y * RENDERER_WIDTH);

		for (int x = 0; x < r.w; x++) {
			*(dstptr++) = *(srcptr++);
		}
	}
}
void Renderer_DrawHorzRippleEffect(const CNM_RECT *rect, float period, float amp, float spd) {
	CNM_RECT r = (CNM_RECT) {
		.w = RENDERER_WIDTH, .h = RENDERER_HEIGHT,
		.x = 0, .y = 0,
	};
	if (rect != NULL) r = *rect;
	if (r.x > RENDERER_WIDTH || r.x + r.w < 0 ||
		r.y > RENDERER_HEIGHT || r.y + r.h < 0) return;
	if (r.x + r.w > RENDERER_WIDTH) r.w = RENDERER_WIDTH - r.x;
	if (r.y + r.h > RENDERER_HEIGHT) r.h = RENDERER_HEIGHT - r.y;
	if (r.x < 0) {
		r.w += r.x;
		r.x = 0;
	}
	if (r.y < 0) {
		r.h += r.y;
		r.y = 0;
	}

	for (int y = r.y; y < r.y + r.h; y++) {
		int xs = r.x + (int)(sinf((y + ((float)Game_GetFrame() * spd)) * CNM_PI / period) * amp);
		int startx = 0;
		if (xs < 0) {
			startx -= xs;
			xs = 0;
		}

		const unsigned char *srcptr = ((unsigned char *)renderer_effects_buf->pixels) + (xs + y * RENDERER_WIDTH);
		unsigned char *dstptr = ((unsigned char *)renderer_scr->pixels) + (r.x + y * RENDERER_WIDTH);
		
		for (int x = startx; x < r.w; x++) {
			if (xs + x < 0 || xs + x >= RENDERER_WIDTH) continue;
			*(dstptr++) = *(srcptr++);
		}
	}
}
void Renderer_DrawStetchedSpan(int x, int y, int w, int srcx, int srcy, int srcw, int trans) {
	float src_xstep = (float)srcw / (float)w;
	if (y < 0 || y >= RENDERER_HEIGHT || x + w < 0) return;
	if (x < 0) {
		srcx -= (int)((float)x * src_xstep);
		w += x;
		x = 0;
	}

	float srcx_real = srcx;
	const unsigned char *srcptr_base = ((unsigned char *)renderer_gfx->pixels) + (srcy * renderer_gfx->w);
	unsigned char *dstptr = ((unsigned char *)renderer_scr->pixels) + (x + y * RENDERER_WIDTH);

	if (trans == 0) {
		while (w > 0 && x < RENDERER_WIDTH) {
			unsigned char src = *(srcptr_base + (int)srcx_real);
			if (src) *dstptr = src;
			srcx_real += src_xstep;
			dstptr++;
			x++;
			w--;
		}
	} else {
		while (w > 0 && x < RENDERER_WIDTH) {
			unsigned char dst = *dstptr;
			unsigned char src = *(srcptr_base + (int)srcx_real);
			if (src) *dstptr = renderer_trans[src][dst][trans];
			dstptr++;
			srcx_real += src_xstep;
			x++;
			w--;
		}
	}
}

static int get_nearest_color(int r, int g, int b) {
	int closest_index = 1, closest_dist = INT_MAX;
	for (int i = 1; i < 0x100; i++)
	{
		if (i >= renderer_scr->format->palette->ncolors)
			continue;

		SDL_Color cur_color = renderer_scr->format->palette->colors[i];
		int dist = abs(r - cur_color.r) + abs(g - cur_color.g) + abs(b - cur_color.b);
		if (dist < closest_dist)
		{
			closest_dist = dist;
			closest_index = i;
		}
	}
	return closest_index;
}

void Renderer_BuildTables(void)
{
	int s, d, l;
	SDL_Color sc, dc;
	float r, g, b, rs, gs, bs;
	if (!renderer_initialized)
		return;

	//Renderer_BuildConvTable();
	/* Build the transparency tables */
	for (s = 0; s < 256; s++)
	{
		for (d = 0; d < 256; d++)
		{
			sc = renderer_scr->format->palette->colors[s];
			dc = renderer_scr->format->palette->colors[d];
			r = (float)sc.r / 255.0f; g = (float)sc.g / 255.0f; b = (float)sc.b / 255.0f;
			rs = (((float)dc.r / 255.0f) - r) / (float)(RENDERER_LEVELS + 1);
			gs = (((float)dc.g / 255.0f) - g) / (float)(RENDERER_LEVELS + 1);
			bs = (((float)dc.b / 255.0f) - b) / (float)(RENDERER_LEVELS + 1);
			renderer_trans[s][d][0] = s;
			if (!s)
				renderer_trans[s][d][0] = d;
			for (l = 1; l < RENDERER_LEVELS - 1; l++)
			{
				r += rs; g += gs; b += bs;
				renderer_trans[s][d][l] = get_nearest_color((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f));
				if (!s)
					renderer_trans[s][d][l] = d;
			}
			renderer_trans[s][d][RENDERER_LEVELS - 1] = d;
		}
	}

	/* Build the light tables */
	renderer_light[0][0] = 0; renderer_light[0][1] = 0; renderer_light[0][2] = 0; renderer_light[0][3] = 0;
	renderer_light[0][4] = 0; renderer_light[0][5] = 0; renderer_light[0][6] = 0; renderer_light[0][7] = 0;
	for (s = 1; s < 256; s++)
	{
		sc = renderer_scr->format->palette->colors[s];
		r = (float)sc.r / 255.0f; g = (float)sc.g / 255.0f; b = (float)sc.b / 255.0f;
		rs = (1.0f - r) / (float)(RENDERER_LIGHT);
		gs = (1.0f - g) / (float)(RENDERER_LIGHT);
		bs = (1.0f - b) / (float)(RENDERER_LIGHT);
		renderer_light[s][0] = get_nearest_color(255, 255, 255);
		for (l = RENDERER_LIGHT - 1; l > 0; l--)
		{
			r += rs; g += gs; b += bs;
			renderer_light[s][l] = get_nearest_color((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f));
		}
		renderer_light[s][RENDERER_LIGHT] = s;
		r = (float)sc.r / 255.0f; g = (float)sc.g / 255.0f; b = (float)sc.b / 255.0f;
		rs = (0.0f - r) / (float)(RENDERER_LEVELS - RENDERER_LIGHT - 1);
		gs = (0.0f - g) / (float)(RENDERER_LEVELS - RENDERER_LIGHT - 1);
		bs = (0.0f - b) / (float)(RENDERER_LEVELS - RENDERER_LIGHT - 1);
		for (l = RENDERER_LIGHT + 1; l < RENDERER_LEVELS - 1; l++)
		{
			r += rs; g += gs; b += bs;
			renderer_light[s][l] = get_nearest_color((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f));
		}
		renderer_light[s][RENDERER_LEVELS - 1] = get_nearest_color(0, 0, 0);
	}
}
//static void Renderer_BuildConvTable(void)
//{
//	int r, g, b;
//	int closest_index, closest_match, i, cur_match;
//	SDL_Color cur_color;
//	if (!renderer_initialized)
//		return;
//
//	for (r = 0; r < (1 << RENDERER_DEPTH); r++)
//	{
//		for (g = 0; g < (1 << RENDERER_DEPTH); g++)
//		{
//			for (b = 0; b < (1 << RENDERER_DEPTH); b++)
//			{
//				closest_index = 1;
//				closest_match = 0x100*3;
//				for (i = 1; i < 0x100; i++)
//				{
//					if (i >= renderer_scr->format->palette->ncolors)
//						continue;
//
//					cur_color = renderer_scr->format->palette->colors[i];
//					cur_match =
//						Abs((r << (8 - RENDERER_DEPTH)) - cur_color.r) +
//						Abs((g << (8 - RENDERER_DEPTH)) - cur_color.g) +
//						Abs((b << (8 - RENDERER_DEPTH)) - cur_color.b);
//					if (cur_match < closest_match)
//					{
//						closest_match = cur_match;
//						closest_index = i;
//					}
//				}
//				renderer_conv[r][g][b] = closest_index;
//			}
//		}
//	}
//}
//int Renderer_MakeColor(int r, int g, int b)
//{
//	if (!renderer_initialized)
//		return 0;
//
//	return renderer_conv[r >> (8 - RENDERER_DEPTH)][g >> (8 - RENDERER_DEPTH)][b >> (8 - RENDERER_DEPTH)];
//}
int Renderer_GetBitmapHeight(void)
{
	if (renderer_initialized && renderer_gfx != NULL)
		return renderer_gfx->h;
	else
		return 0;
}
int Renderer_GetBitmapPixel(int x, int y)
{
	if (renderer_initialized && renderer_gfx != NULL)
		return ((unsigned char *)renderer_gfx->pixels)[y * 512 + x];
	else
		return 0;
}

void Renderer_Clear(int color)
{
	CNM_RECT rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = RENDERER_WIDTH;
	rect.h = RENDERER_HEIGHT;
	Renderer_DrawRect(&rect, color, 0, RENDERER_LIGHT);
}
//#ifndef CNM_NO_X86ASSEMBLY
//void __declspec(naked) __cdecl Renderer_DrawRect(const CNM_RECT *rect, int color, int trans, int light)
//{
//	__asm
//	{
//		push ebp // Create stack frame
//		mov ebp,esp
//		push ebx
//		push edi
//		push esi
//
//		mov eax, DWORD PTR[renderer_initialized]
//		cmp eax,0
//		je drawrect_return
//		cmp DWORD PTR [ebp+12],0 // Make sure we dont render transparent rectangles
//		je drawrect_return
//
//		mov edx,DWORD PTR [ebp+8] // Get the rectangle pointer
//		mov ecx,[edx]
//		add ecx,[edx+8] // ECX now has the right bound of the rectangle
//		cmp ecx,RENDERER_WIDTH
//		jle dont_clip_right
//		mov ecx,RENDERER_WIDTH
//dont_clip_right:
//		mov eax,[edx]
//		cmp ecx,eax
//		jle drawrect_return // Return if the rect is not on screen
//		cmp eax,0
//		jge dont_clip_left
//		mov eax,0
//dont_clip_left:
//		sub ecx,eax
//		jle drawrect_return // Return if the rect is not on screen
//// By this point ECX has the width and EAX has the x position
//
//		mov ebx,[edx+4]
//		add ebx,[edx+12] // EAX now has the right bound of the rectangle
//		cmp ebx,RENDERER_HEIGHT
//		jle dont_clip_bottom
//		mov ebx,RENDERER_HEIGHT
//dont_clip_bottom:
//		mov esi,[edx+4]
//		cmp ebx,esi
//		jle drawrect_return // Return if the rect is not on screen
//		cmp esi,0
//		jge dont_clip_top
//		mov esi,0
//dont_clip_top:
//		sub ebx,esi
//		jle drawrect_return // Return if the rect is not on screen
//// By this point EBX has the height and ESI has the y position
//
//		mov [esp-4],ecx // Width
//		mov [esp-8],ebx // Height
//		mov edi,ecx
//		sub edi,RENDERER_WIDTH
//		neg edi
//		mov [esp-12], edi // Counter width
//
//// Now we calculate the destination pointer
//		mov edi,renderer_scr
//		mov edi,[edi]renderer_scr.pixels
//		mov ebx,esi
//		imul ebx,ebx,RENDERER_WIDTH
//		add edi,eax
//		add edi,ebx // EDI now has the desination
//
//		mov eax, DWORD PTR[ebp + 12] // Get color byte
//		lea esi, [renderer_light + eax*8]
//		add esi, DWORD PTR[ebp + 20]
//		movzx eax, BYTE PTR[esi] // Now we have the lighted color
//		shl eax, 8+3
//		or al, BYTE PTR[ebp + 16] // EBX has pre-shifted index for the transparency
//drawrect_loop:
//		movzx ebx, BYTE PTR[edi]
//		shl ebx, 3
//		or ebx, eax
//		movzx ebx, BYTE PTR[renderer_trans + ebx]
//		mov BYTE PTR[edi],bl
//
//		inc edi
//		loop drawrect_loop
//		mov ecx,[esp-8]
//		dec ecx
//		je drawrect_return
//		mov [esp-8],ecx // Go down the veritcal loop counter
//		mov ecx,[esp-4] // Reset horizontal loop counter
//		add edi,[esp-12]
//		jmp drawrect_loop
//
//drawrect_return: // Return from the function
//		pop esi
//		pop edi
//		pop ebx
//		pop ebp
//		ret
//	}
//
//	/*int x, y;
//	if (!renderer_initialized)
//		return;
//
//	for (y = rect->y; y < rect->y + rect->h; y++)
//	{
//		for (x = rect->x; x < rect->x + rect->w; x++)
//		{
//			if (x > -1 && x < RENDERER_WIDTH && y > -1 && y < RENDERER_HEIGHT)
//				Renderer_SetPixel(x, y, color, trans, light);
//		}
//	}*/
//}
//#else
void Renderer_DrawRect(const CNM_RECT *_rect, int color, int trans, int light) {
	int x, y, last_color;
	CNM_RECT rect;
	if (!renderer_initialized)
		return;

	if (!color) return;
	memcpy(&rect, _rect, sizeof(CNM_RECT));
	if (rect.x < 0) {
		rect.w += rect.x;
		rect.x = 0;
		if (rect.w <= 0) return;
	}
	if (rect.x >= RENDERER_WIDTH) return;
	if (rect.x + rect.w >= RENDERER_WIDTH) rect.w = RENDERER_WIDTH - rect.x;
	if (rect.y < 0) {
		rect.h += rect.y;
		rect.y = 0;
		if (rect.h <= 0) return;
	}
	if (rect.y >= RENDERER_HEIGHT) return;
	if (rect.y + rect.h >= RENDERER_HEIGHT) rect.h = RENDERER_HEIGHT - rect.y;

	if (trans == 0 && light == RENDERER_LIGHT) {
		for (y = rect.y; y < rect.y + rect.h; y++) {
			memset(&((unsigned char *)renderer_scr->pixels)[rect.x + y * RENDERER_WIDTH], color, rect.w);
		}
		return;
	}

	for (y = rect.y; y < rect.y + rect.h; y++)
	{
		for (x = rect.x; x < rect.x + rect.w; x++)
		{
			last_color = ((unsigned char *)renderer_scr->pixels)[x + y * RENDERER_WIDTH];
			//if (color)
			((unsigned char *)renderer_scr->pixels)[x + y * RENDERER_WIDTH] = renderer_trans[renderer_light[color][light]][last_color][trans];
		}
	}
}
//#endif
void Renderer_DrawEmptyRect(const CNM_RECT *rect, int color, int trans, int light)
{
	CNM_RECT r;
	Util_SetRect(&r, rect->x, rect->y, rect->w, 1);
	Renderer_DrawRect(&r, color, trans, light);
	Util_SetRect(&r, rect->x, rect->y + rect->h, rect->w, 1);
	Renderer_DrawRect(&r, color, trans, light);
	Util_SetRect(&r, rect->x, rect->y, 1, rect->h);
	Renderer_DrawRect(&r, color, trans, light);
	Util_SetRect(&r, rect->x + rect->w, rect->y, 1, rect->h);
	Renderer_DrawRect(&r, color, trans, light);
}


//#ifndef CNM_NO_X86ASSEMBLY
//#define FAST_DRAWBITMAP
//#ifdef FAST_DRAWBITMAP
//typedef long long mm0_int64;
//mm0_int64 __declspec(align(16)) xmm_zero_register[2] = {0, 0};
//void __declspec(naked) __stdcall Renderer_DrawBitmap(int _x, int _y, const CNM_RECT *_src, int trans, int light)
//#else
//void __stdcall Renderer_DrawBitmap(int _x, int _y, const CNM_RECT *_src, int trans, int light)
//#endif
//{
//#ifdef FAST_DRAWBITMAP
//#define ARG_X 8
//#define ARG_Y 12
//#define ARG_SRC 16
//#define ARG_TRANS 20
//#define ARG_LIGHT 24
//
//#define LOCAL_DX 4
//#define LOCAL_SX 8
//#define LOCAL_W 12
//#define LOCAL_CW 16
//
////#define LOCAL_DY 20
////#define LOCAL_SY 24
//#define LOCAL_H 20
//	__asm
//	{
//		push ebp
//		mov ebp,esp
//		push ebx
//		push esi
//		push edi
//		
//// If there is no renderer, then don't render
//		mov eax, DWORD PTR[renderer_initialized]
//		cmp eax, 0
//		je drawbitmap_return1
//
//// EAX = dest x, EBX = src x, ECX = width
//		mov eax, [ebp + ARG_X]
//		mov ecx,[ebp+ARG_SRC]
//		mov ebx,[ecx]_src.x
//		mov ecx,[ecx]_src.w
//		add ecx,eax
//		cmp ecx,RENDERER_WIDTH
//		jle dont_clip_right
//		mov ecx,RENDERER_WIDTH
//dont_clip_right:
//		cmp ecx,eax
//		jle end
//		cmp eax,0
//		jge dont_clip_left
//		sub ebx,eax
//		mov eax,0
//dont_clip_left:
//		sub ecx,eax
//		jle end
//		mov [esp-LOCAL_DX],eax
//		mov [esp-LOCAL_SX],ebx
//		mov [esp-LOCAL_W],ecx
//		sub ecx,RENDERER_WIDTH
//		neg ecx
//		mov [esp-LOCAL_CW],ecx
//
//// EAX = dest y, EBX = src y, ECX = height
//		mov eax, [ebp + ARG_Y]
//		mov ecx, [ebp + ARG_SRC]
//		mov ebx, [ecx]_src.y
//		mov ecx, [ecx]_src.h
//		add ecx, eax
//		cmp ecx, RENDERER_HEIGHT
//		jle dont_clip_bottom
//		mov ecx, RENDERER_HEIGHT
//dont_clip_bottom:
//		cmp ecx, eax
//		jle end
//		cmp eax, 0
//		jge dont_clip_top
//		sub ebx, eax
//		mov eax, 0
//dont_clip_top:
//		sub ecx, eax
//		jle end
//		mov[esp - LOCAL_H], ecx
//
//// Set up the source bitmap pointer and the screen destination pointer
//		mov esi,renderer_gfx
//		mov esi,[esi]renderer_gfx.pixels
//		imul ebx,ebx,512
//		add esi,ebx
//		add esi,[esp - LOCAL_SX]
//
//		mov edi,renderer_scr
//		mov edi,[edi]renderer_scr.pixels
//		imul eax,eax,RENDERER_WIDTH
//		add edi,eax
//		add edi,[esp - LOCAL_DX]
//
//		movzx ebx, BYTE PTR[ebp + ARG_TRANS]
//		movzx ebp, BYTE PTR[ebp + ARG_LIGHT]
//		mov ecx, [esp - LOCAL_W]
//		cmp ebp,RENDERER_LIGHT
//		jne slow_path
//		cmp ebx,0
//		je fast_path
//slow_path:
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor edx, edx
//drawing_loop:
//		movzx eax, BYTE PTR[esi]
//		inc esi
//		movzx eax, BYTE PTR[renderer_light + eax*8 + ebp]
//		shl eax,8
//		movzx edx, BYTE PTR[edi]
//		or eax,edx
//		movzx edx, BYTE PTR[renderer_trans + eax*8 + ebx]
//		mov BYTE PTR[edi],dl
//
//		inc edi
//		dec ecx
//		jne drawing_loop
//		add esi, [esp - LOCAL_CW]
//		add esi, 512 - RENDERER_WIDTH
//		add edi, [esp - LOCAL_CW]
//		mov ecx,[esp -  LOCAL_H]
//		dec ecx
//		mov[esp - LOCAL_H], ecx
//		mov ecx, [esp - LOCAL_W]
//		jne drawing_loop
//
//end:
//		pop edi
//		pop esi
//		pop ebx
//		pop ebp
//		ret 20
//
//// This is the special case where the graphics are normal brightness and transparency
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//		xor eax, eax
//fast_path:
//		mov ebp,0xff
//		xor eax, eax
//		xor ebx, ebx
//		xor edx, edx
//
//loop_start:
//		cmp ecx, 16
//		jl drawing_loop3
//drawing_loop2:
//		movups xmm0, XMMWORD PTR[esi]
//		movups xmm3, XMMWORD PTR[edi]
//		movups xmm2,xmm0
//		movups xmm1, xmm_zero_register
//		pcmpeqb xmm0, xmm1
//		pand xmm0, xmm3
//		por xmm0, xmm2
//		movups XMMWORD PTR[edi], xmm0
//		add edi,16
//		add esi,16
//
//		sub ecx,16
//		cmp ecx, 16
//		jge drawing_loop2
//		cmp ecx, 0
//drawing_loop3:
//		je loop_complete
//		movzx eax, BYTE PTR[esi]
//		cmp eax,0
//		cmove eax, DWORD PTR[edi]
//		inc esi
//		mov BYTE PTR[edi], al
//		inc edi
//		dec ecx
//		jmp drawing_loop3
//loop_complete:
//
//		add esi, [esp - LOCAL_CW]
//		add esi, 512 - RENDERER_WIDTH
//		add edi, [esp - LOCAL_CW]
//		mov ecx, [esp - LOCAL_H]
//		dec ecx
//		mov[esp - LOCAL_H], ecx
//		mov ecx, [esp - LOCAL_W]
//		jne loop_start
//
//drawbitmap_return1:
//		pop edi
//		pop esi
//		pop ebx
//		pop ebp
//		ret 20
//
//	}
//#undef ARG_X
//#undef ARG_Y
//#undef ARG_SRC
//#undef ARG_TRANS
//#undef ARG_LIGHT
//
//#else
//	return; // This macro option has been now moved to CNM_NO_X86ASSEMBLY look below...
//#endif
//}
//#else

inline static uint64_t passthru(uint64_t src, uint64_t dst) {
    union bytes {
        uint64_t i;
        uint8_t b[8];
    };
    const union bytes srcu = (union bytes){ .i = src },
		dstu = (union bytes){ .i = dst };

    return ((union bytes){
        .b[0] = srcu.b[0] ? srcu.b[0] : dstu.b[0],
        .b[1] = srcu.b[1] ? srcu.b[1] : dstu.b[1],
        .b[2] = srcu.b[2] ? srcu.b[2] : dstu.b[2],
        .b[3] = srcu.b[3] ? srcu.b[3] : dstu.b[3],
        .b[4] = srcu.b[4] ? srcu.b[4] : dstu.b[4],
        .b[5] = srcu.b[5] ? srcu.b[5] : dstu.b[5],
        .b[6] = srcu.b[6] ? srcu.b[6] : dstu.b[6],
        .b[7] = srcu.b[7] ? srcu.b[7] : dstu.b[7],
    }).i;
}

void Renderer_DrawBitmap(int _x, int _y, const CNM_RECT *_src, int trans, int light) {
	CNM_RECT src;
	if (!renderer_initialized)
		return;

	src = *_src;
	//memcpy(&src, _src, sizeof(CNM_RECT));
	if (_y < 0) {
		if (_y + src.h < 0) {
			return;
		} else {
			src.y -= _y;
			src.h += _y;
			_y = 0;
		}
	}
	if (_y + src.h > RENDERER_HEIGHT) {
		if (_y >= RENDERER_HEIGHT) {
			return;
		} else {
			src.h = RENDERER_HEIGHT - _y;
		}
	}
	if (_x < 0) {
		if (_x + src.w < 0) {
			return;
		} else {
			src.x -= _x;
			src.w += _x;
			_x = 0;
		}
	}
	if (_x + src.w > RENDERER_WIDTH) {
		if (_x >= RENDERER_WIDTH) {
			return;
		} else {
			src.w = RENDERER_WIDTH - _x;
		}
	}

	if (src.y + src.h > renderer_gfx->h) {
#ifdef DEBUG
		Console_Print("Don't use out of bounds gfx DUMBASS!");
#endif
		return;
	}

	// Only go for 32 and above because otherwise its not worth it for speed
#if 1
	if (light == RENDERER_LIGHT && trans == 0 && src.x % 8 == 0 && src.w >= 32) {
 		const uint64_t *restrict src_pixel = (uint64_t *)(((uint8_t *)renderer_gfx->pixels) + (src.y * renderer_gfx->w + src.x));
 		const int grid_x = _x / 8;
 		const int grid_srcw = (src.w - 1) / 8 + 1;
 		uint64_t *restrict dest_pixel = (uint64_t *)(((uint8_t *)renderer_scr->pixels) + (_y * RENDERER_WIDTH + grid_x*8));
 		const int grid_destw = ((_x + src.w - 1) / 8 + 1) - grid_x;

 		const int dest_mod = (_x - grid_x*8) * 8;
 		const uint64_t start_set_mask = ((uint64_t)-1) << dest_mod;
		const int end_set_mask_shift = (64 - ((_x + src.w) % 8)*8);
 		const uint64_t end_set_mask = ((uint64_t)-1) >> (end_set_mask_shift == 64 ? 0 : end_set_mask_shift);
 		const bool dirty_end = end_set_mask != (uint64_t)-1;
 		const bool clean_start = _x == grid_x*8;

 		const int src_pitch = renderer_gfx->w / sizeof(uint64_t) - grid_srcw + (!clean_start && !dirty_end);
 		const int gfx_pitch = RENDERER_WIDTH / sizeof(uint64_t) - grid_destw;

 		for (int y = 0; y < src.h; y++) {
 			uint64_t srcload = rol64(*src_pixel, dest_mod);
 			*dest_pixel = passthru((srcload & start_set_mask) | (*dest_pixel & ~start_set_mask), *dest_pixel);

 			for (int x = 1+dirty_end; x < grid_destw; x++) {
 				dest_pixel++;
 				src_pixel++;
 				srcload &= ~start_set_mask;
 				srcload |= *src_pixel << dest_mod;
 				*dest_pixel = passthru(srcload, *dest_pixel);
 				srcload = *src_pixel >> (64 - dest_mod);
 			}

 			if (dirty_end) {
 				dest_pixel++;
 				src_pixel++;
 				if (clean_start) {
					srcload = *src_pixel;
				} else if (src.w % 8 != 0) {
					// We still have a little left probably to load...
					src_pixel++;
					srcload |= *src_pixel << dest_mod;
				}
 				*dest_pixel = passthru((srcload & end_set_mask) | (*dest_pixel & ~end_set_mask), *dest_pixel);
 			}

 			src_pixel += src_pitch+clean_start;
 			dest_pixel += gfx_pitch+1; // Make up for not consuming last dest pixel
 		}
 		return;
 	}
#endif

#ifdef DEBUG
	if (_src->x % 8 != 0 && _src->w > RENDERER_MAX_WIDTH / 2) {
		Console_Print("warning: slow normal draw because of unaligned source x");
	}
#endif

	const uint8_t *restrict src_pixel = ((unsigned char *)renderer_gfx->pixels) + (src.y * renderer_gfx->w + src.x);
	uint8_t *restrict dest_pixel = ((unsigned char *)renderer_scr->pixels) + (_y * RENDERER_WIDTH + _x);
	const int src_width = renderer_gfx->w - src.w;
	const int gfx_width = RENDERER_WIDTH - src.w;

	for (int y = 0; y < src.h; y++) {
		for (int x = 0; x < src.w; x++) {
			*dest_pixel = renderer_trans[renderer_light[*src_pixel][light]][*dest_pixel][trans];
			dest_pixel++;
			src_pixel++;
		}
		src_pixel += src_width;
		dest_pixel += gfx_width;
	}
}
//#endif
void Renderer_DrawBitmap2(int _x, int _y, const CNM_RECT *_src, int trans, int light, int hflip, int vflip)
{
	int x, y, tx, ty;
	if (!renderer_initialized)
		return;
	if (!hflip && !vflip) {
		Renderer_DrawBitmap(_x, _y, _src, trans, light);
		return;
	}

	CNM_RECT src = *_src;

	if (_y < 0) {
		if (_y + src.h < 0) {
			return;
		} else {
			if (!vflip) src.y -= _y;
			src.h += _y;
			_y = 0;
		}
	}
	if (_y + src.h > RENDERER_HEIGHT) {
		if (_y >= RENDERER_HEIGHT) {
			return;
		} else {
			if (vflip) src.y += src.h - (RENDERER_WIDTH - _y);
			src.h = RENDERER_HEIGHT - _y;
		}
	}
	if (_x < 0) {
		if (_x + src.w < 0) {
			return;
		} else {
			if (!hflip) src.x -= _x;
			src.w += _x;
			_x = 0;
		}
	}
	if (_x + src.w > RENDERER_WIDTH) {
		if (_x >= RENDERER_WIDTH) {
			return;
		} else {
			if (hflip) src.x += src.w - (RENDERER_WIDTH - _x);
			src.w = RENDERER_WIDTH - _x;
		}
	}

	int tx_start, ty_start, tx_step, ty_step;
	tx_start = (hflip) ? (src.x + src.w - 1) : (src.x);
	ty_start = (vflip) ? (src.y + src.h - 1) : (src.y);
	tx_step = hflip ? -1 : 1;
	ty_step = vflip ? -1 : 1;

	for (y = _y, ty = ty_start; y < _y + src.h; y++, ty += ty_step)
	{
		uint8_t *dst_color = &((unsigned char *)renderer_scr->pixels)[_x + y * RENDERER_WIDTH];
		const uint8_t *src_color = &((unsigned char *)renderer_gfx->pixels)[ty * renderer_gfx->w + tx_start];
		for (x = _x, tx = tx_start; x < _x + src.w; x++, tx += tx_step, dst_color++, src_color += tx_step)
		{
			const uint8_t color = renderer_trans[renderer_light[*src_color][light]][*dst_color][trans];
			*dst_color = *src_color ? color : *dst_color;
		}
	}
}
/*void Renderer_DrawBitmapScaled(const CNM_RECT *dest, const CNM_RECT *src, int trans, int light)
{
	int x, y;
	float tx, txs, ty, tys;
	if (!renderer_initialized)
		return;

	ty = (float)src->y;
	tys = (float)src->h / (float)dest->h;
	for (y = dest->y; y < dest->y + dest->h; y++, ty += tys)
	{
		tx = (float)src->x;
		txs = (float)src->w / (float)dest->w;
		for (x = dest->x; x < dest->x + dest->w; x++, tx += txs)
		{
			if ((int)tx > -1 && (int)tx < renderer_gfx->w && (int)ty > -1 && (int)ty < renderer_gfx->h)
				Renderer_SetPixel(x, y, ((unsigned char *)renderer_gfx->pixels)[(int)ty * renderer_gfx->w + (int)tx], trans, light);
			else
				Renderer_SetPixel(x, y, Renderer_MakeColor(255, 0, 255), 0, RENDERER_LIGHT);
		}
	}
}*/
static inline void Renderer_SetPixel(int x, int y, int color, int trans, int light)
{
	int last_color;
	//if (x > -1 && x < RENDERER_WIDTH && y > -1 && y < RENDERER_HEIGHT)
	//{
		last_color = ((unsigned char *)renderer_scr->pixels)[x + y * RENDERER_WIDTH];
		if (color)
			((unsigned char *)renderer_scr->pixels)[x + y * RENDERER_WIDTH] = renderer_trans[renderer_light[color][light]][last_color][trans];
	//}
}
int Renderer_GetPixel(int x, int y)
{
	if (!renderer_initialized)
		return 0;

	if (x > -1 && x < RENDERER_WIDTH && y > -1 && y < RENDERER_HEIGHT)
		return (int)((unsigned char *)renderer_scr->pixels)[x + y * RENDERER_WIDTH];
	else
		return 0;
}
void Renderer_PlotPixel(int x, int y, int color)
{
	if (!renderer_initialized)
		return;

	if (x > -1 && x < RENDERER_WIDTH && y > -1 && y < RENDERER_HEIGHT && color != 0)
		((unsigned char *)renderer_scr->pixels)[x + y * RENDERER_WIDTH] = color;
}
void Renderer_PlotPixel2(int x, int y, int color, int trans, int light) {
	if (!renderer_initialized)
		return;
	
	if (x > -1 && x < RENDERER_WIDTH && y > -1 && y < RENDERER_HEIGHT && color != 0) {
		unsigned char *pxl = &(((unsigned char *)renderer_scr->pixels)[x + y * RENDERER_WIDTH]);
		*pxl = renderer_trans[renderer_light[color][light]][*pxl][trans];
	}
}

void Renderer_DrawText(int x, int y, int trans, int light, const char *format, ...)
{
	CNM_RECT src;
	char buffer[RENDERER_MAX_WIDTH + 1] = {'\0'};
	int c, i;
	va_list args;
	if (!renderer_initialized)
		return;

	va_start(args, format);
	Util_StringPrintF(buffer, RENDERER_WIDTH + 1, format, args);
	va_end(args);
	for (c = 0; c < (int)strlen(buffer); c++)
	{
		i = (int)buffer[c];
		src.w = renderer_font.w;
		src.h = renderer_font.h;
		src.x = (i % 16 * renderer_font.w) + renderer_font.x;
		src.y = (i / 16 * renderer_font.h) + renderer_font.y;
		Renderer_DrawBitmap
		(
			c * renderer_font.w + x,
			y,
			&src,
			trans,
			light
		);
	}
}
void Renderer_SetFont(int base_x, int base_y, int font_w, int font_h)
{
	if (!renderer_initialized)
		return;

	renderer_font.x = base_x;
	renderer_font.y = base_y;
	renderer_font.w = font_w;
	renderer_font.h = font_h;
}

static int Abs(int i)
{
	return (i < 0) ? -i : i;
}
