#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <SDL.h>
#include <SDL_surface.h>
#include <SDL_video.h>
#include "renderer.h"
#include "console.h"
#include "game.h"
#include "mem.h"

#define RENDERER_LEVELS_LIGHT (RENDERER_LEVELS + 2)
static SDL_Window *renderer_win;
static SDL_Surface *renderer_scr;
static SDL_Surface *renderer_gfx;
static SDL_Surface *renderer_hires_temp;
static SDL_Surface *renderer_effects_buf;
static unsigned char (*renderer_trans)[256][RENDERER_LEVELS]; /* Source color, Destination Color, Transparency level */
static unsigned char (*renderer_light)[RENDERER_LEVELS_LIGHT]; /* Color, Light level */
static unsigned char *renderer_additive;
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
static void rgb_to_hsv(int ri, int gi, int bi, float *h, float *s, float *v);
static int get_nearest_color(int r, int g, int b);

static void Renderer_SetPixel(int x, int y, int color, int trans, int light);

#define MAX_SVBIN 512
static int supervirus_binary_pos[MAX_SVBIN][3];

void Renderer_Init(int start_fullscreen, int hi_res, int widescreen)
{
	RENDERER_WIDTH = widescreen ? 424 : 320;
	RENDERER_HEIGHT = 240;

	renderer_trans = arena_global_alloc(sizeof(*renderer_trans) * 256);
	renderer_light = arena_global_alloc(sizeof(*renderer_light) * 256);
	renderer_additive = arena_global_alloc(sizeof(*renderer_additive) * 256);
	
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

static void rgb_to_hsv(int ri, int gi, int bi, float *h, float *s, float *v) {
	float r = (float)ri / 255.0f, g = (float)gi / 255.0f, b = (float)bi / 255.0f;
	int valuei = CNM_MAX(ri, CNM_MAX(gi, bi));
	float value = CNM_MAX(r, CNM_MAX(g, b));
	float xmin = CNM_MIN(r, CNM_MIN(g, b));
	float chroma = value - xmin;
	float light = (value + xmin) / 2.0f;
	float hue = 0.0f;
	if (chroma <= 0.001f) {
		hue = 0.0f;
	} else if (valuei == ri) {
		hue = 60.0f * (fmodf((g-b)/chroma, 6.0f));
	} else if (valuei == gi) {
		hue = 60.0f * ((b - r)/chroma + 2.0f);
	} else if (valuei == bi) {
		hue = 60.0f * ((r - g)/chroma + 4.0f);
	}
	float sat = 0.0f;
	//value
	if (value != 0.0f) sat = chroma / value;
	//lightness
	if (light != 0.0f && light != 1.0f) sat = (value - light)/CNM_MIN(light, 1.0f - light);

	*h = hue;
	*s = sat * 100.0f;
	*v = value * 100.0f;
}

static float hsv_dist(float h1, float s1, float v1, float h2, float s2, float v2) {
	float sv_dist = fabsf(s1 - s2) + fabsf(v1 - v2);
	float h_dist = fabsf(h1 - h2);
	if (h1 < h2) {
		float new_dist = fabsf((h1 + 360.0f) - h2);
		if (new_dist < h_dist) h_dist = new_dist;
	}
	if (h2 < h1) {
		float new_dist = fabsf(h1 - (h2 + 360.0f));
		if (new_dist < h_dist) h_dist = new_dist;
	}
	return h_dist * 0.8f + sv_dist * 1.2f;
}

static int get_nearest_color_hsv(int r, int g, int b) {
	int closest_index = 1;
	float closest_dist = FLT_MAX;
	float h, s, v;
	rgb_to_hsv(r, g, b, &h, &s, &v);
	for (int i = 1; i < 0x100; i++)
	{
		if (i >= renderer_scr->format->palette->ncolors)
			continue;

		SDL_Color cur_color = renderer_scr->format->palette->colors[i];
		float curh, curs, curv;
		rgb_to_hsv(cur_color.r, cur_color.g, cur_color.b, &curh, &curs, &curv);
		float dist = hsv_dist(h, s, v, curh, curs, curv);
		if (dist < closest_dist)
		{
			closest_dist = dist;
			closest_index = i;
		}
	}
	return closest_index;
}

static int rgb_dist(int r1, int g1, int b1, int r2, int g2, int b2) {
	int dist_r = r2 - r1;
	int dist_g = g2 - g1;
	int dist_b = b2 - b1;
	return (dist_r * dist_r) + (dist_g * dist_g) + (dist_b * dist_b);
}
static int rgb_dist_naive(int r1, int g1, int b1, int r2, int g2, int b2) {
	int dist_r = abs(r2 - r1);
	int dist_g = abs(g2 - g1);
	int dist_b = abs(b2 - b1);
	return dist_r + dist_g + dist_b;
}

static int get_nearest_color(int r, int g, int b) {
	int closest_index = 1, closest_dist = INT_MAX;
	for (int i = 1; i < 0x100; i++)
	{
		if (i >= renderer_scr->format->palette->ncolors)
			continue;

		SDL_Color cur_color = renderer_scr->format->palette->colors[i];
		int dist = rgb_dist(r, g, b, cur_color.r, cur_color.g, cur_color.b);
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
			rs = (((float)dc.r / 255.0f) - r) / (float)(RENDERER_LEVELS);
			gs = (((float)dc.g / 255.0f) - g) / (float)(RENDERER_LEVELS);
			bs = (((float)dc.b / 255.0f) - b) / (float)(RENDERER_LEVELS);
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

	renderer_additive[0] = RENDERER_LIGHT;
	for (s = 1; s < 256; s++) {
		sc = renderer_scr->format->palette->colors[s];
		int max = CNM_MAX(CNM_MAX(sc.r, sc.g), sc.b);
		int light = RENDERER_LIGHT - (max / 64);
		//int trans = 8 - (max / 32);
		//if (trans > 7) trans = 7;
		if (light < 0) light = RENDERER_WHITE;
		renderer_additive[s] = light;
		//renderer_additive[s] = trans;
	}

	/* Build the light tables */
	renderer_light[0][0] = 0; renderer_light[0][1] = 0; renderer_light[0][2] = 0; renderer_light[0][3] = 0;
	renderer_light[0][4] = 0; renderer_light[0][5] = 0; renderer_light[0][6] = 0; renderer_light[0][7] = 0;
	for (s = 1; s < 256; s++)
	{
		sc = renderer_scr->format->palette->colors[s];
		//renderer_light[s][0] = get_nearest_color(255, 255, 255);
		//const int lightadd[] = {50, 130, 0};
		const int lightadd[] = {30, 60, 120};
		for (l = RENDERER_LIGHT - 1; l > -1; l--)
		{
			int _r = sc.r + lightadd[RENDERER_LIGHT - l - 1];
			int _g = sc.g + lightadd[RENDERER_LIGHT - l - 1];
			int _b = sc.b + lightadd[RENDERER_LIGHT - l - 1];
			if (_r < _g && _r < _b) _r += 16;
			else if (_g < _r && _g < _b) _g += 16;
			else if (_b < _g && _b < _r) _b += 16;
			if (_r > 255) _r = 255;
			if (_g > 255) _g = 255;
			if (_b > 255) _b = 255;
			renderer_light[s][l] = get_nearest_color(_r, _g, _b);
		}
		renderer_light[s][RENDERER_LIGHT] = s;
		const int darksub[] = {10, 30, 60, 120};
		for (l = RENDERER_LIGHT + 1; l < RENDERER_LEVELS_LIGHT; l++)
		{
			int _r = sc.r - darksub[l - RENDERER_LIGHT - 1];
			int _g = sc.g - darksub[l - RENDERER_LIGHT - 1];
			int _b = sc.b - darksub[l - RENDERER_LIGHT - 1];
			if (_r > _g && _r > _b) _r -= 16;
			else if (_g > _r && _g > _b) _g -= 16;
			else if (_b > _g && _b > _r) _b -= 16;
			if (_r < 0) _r = 0;
			if (_g < 0) _g = 0;
			if (_b < 0) _b = 0;
			renderer_light[s][l] = get_nearest_color(_r, _g, _b);
		}
		renderer_light[s][RENDERER_BLACK] = get_nearest_color(0, 0, 0);
		renderer_light[s][RENDERER_WHITE] = get_nearest_color(255, 255, 255);
	}
}
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
void render_draw_additive_light(int _x, int _y, CNM_RECT src) {
	if (!renderer_initialized)
		return;

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
	
	const uint8_t *restrict src_pixel = ((unsigned char *)renderer_gfx->pixels) + (src.y * renderer_gfx->w + src.x);
	uint8_t *restrict dest_pixel = ((unsigned char *)renderer_scr->pixels) + (_y * RENDERER_WIDTH + _x);
	const int src_width = renderer_gfx->w - src.w;
	const int gfx_width = RENDERER_WIDTH - src.w;

	for (int y = 0; y < src.h; y++) {
		for (int x = 0; x < src.w; x++) {
			*dest_pixel = renderer_light[*dest_pixel][renderer_additive[*src_pixel]];
			dest_pixel++;
			src_pixel++;
		}
		src_pixel += src_width;
		dest_pixel += gfx_width;
	}
}
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
#if 0
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
	//if (_src->x % 8 != 0 && _src->w > RENDERER_MAX_WIDTH / 2) {
		//Console_Print("warning: slow normal draw because of unaligned source x");
	//}
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
			if (vflip) src.y += src.h - (RENDERER_HEIGHT - _y);
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
void Renderer_DrawColorMask(int _x, int _y, const CNM_RECT *_src, int trans,
		int _color, int hflip, int vflip) {
	int x, y, tx, ty;
	if (!renderer_initialized) return;
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
			if (vflip) src.y += src.h - (RENDERER_HEIGHT - _y);
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
			if (!src_color) continue;
			const uint8_t color = renderer_trans[_color][*dst_color][trans];
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
