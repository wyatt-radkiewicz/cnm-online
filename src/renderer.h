#ifndef _renderer_h_
#define _renderer_h_
#include "utility.h"

//#define RENDERER_WIDTH 320
//#define RENDERER_HEIGHT 240
#define RENDERER_MAX_WIDTH 424
#define RENDERER_MAX_HEIGHT 240
#define RENDERER_TRANS 0
#define RENDERER_LIGHT 3
#define RENDERER_LEVELS 8
#define RENDERER_BLACK (RENDERER_LEVELS)
#define RENDERER_WHITE (RENDERER_LEVELS + 1)

typedef enum rcol {
	RCOL_WHITE = 4,
	RCOL_YELLOW = 231,
	RCOL_PINK = 251,
	RCOL_RED = 176,
	RCOL_BLUE = 200,
	RCOL_BLACK = 247,
	RCOL_LIGHT_BLUE = 196,
	RCOL_PAUSE_COLOR = 240,
} rcol_t;

extern int RENDERER_WIDTH;
extern int RENDERER_HEIGHT;

void Renderer_Init(int start_fullscreen, int hi_res, int widescreen);
void Renderer_Quit(void);
void Renderer_SetFullscreen(int fullscreen);
//void Renderer_StartDrawing(void);
void Renderer_Update(void);
void Renderer_SetHiResMode(int hi_res);
void Renderer_WidescreenMode(int widescreen);
void Renderer_RestartWindow(void);
void Renderer_SetScreenModeFull(int fullscreen, int hi_res, int widescreen);

void Renderer_LoadBitmap(const char *gfx_file);
void Renderer_BuildTables(void);
void Renderer_LoadTables(void);
//int Renderer_MakeColor(int r, int g, int b);
int Renderer_GetBitmapHeight(void);
int Renderer_GetBitmapPixel(int x, int y);

void Renderer_SaveToEffectsBuffer(void);
void Renderer_DrawVertRippleEffect(const CNM_RECT *rect, float period, float amp, float spd);
void Renderer_DrawHorzRippleEffect(const CNM_RECT *rect, float period, float amp, float spd);
void Renderer_DrawStetchedSpan(int x, int y, int w, int srcx, int srcy, int srcw, int trans);

void Renderer_Clear(int color);
void Renderer_DrawEmptyRect(const CNM_RECT *rect, int color, int trans, int light);
//#ifndef CNM_NO_X86ASSEMBLY
//void __cdecl Renderer_DrawRect(const CNM_RECT *rect, int color, int trans, int light);
//void __stdcall Renderer_DrawBitmap(int x, int y, const CNM_RECT *src, int trans, int light);
//#else
void render_draw_additive_light(int x, int y, CNM_RECT src);
void Renderer_DrawRect(const CNM_RECT *rect, int color, int trans, int light);
void Renderer_DrawBitmap(int x, int y, const CNM_RECT *src, int trans, int light);
//#endif
void Renderer_DrawBitmap2(int x, int y, const CNM_RECT *src, int trans, int light, int hflip, int vflip);
void Renderer_DrawColorMask(int x, int y, const CNM_RECT *src, int trans, int color, int hflip, int vflip);
//void Renderer_DrawBitmapScaled(const CNM_RECT *dest, const CNM_RECT *src, int trans, int light);
//void Renderer_SetPixel(int x, int y, int color, int trans, int light);
int Renderer_GetPixel(int x, int y);
void Renderer_PlotPixel(int x, int y, int color);
void Renderer_PlotPixel2(int x, int y, int color, int trans, int light);

void Renderer_DrawText(int x, int y, int trans, int light, const char *format, ...);
void Renderer_SetFont(int base_x, int base_y, int font_w, int font_h);

#endif
