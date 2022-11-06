#ifndef _renderer_h_
#define _renderer_h_
#include "utility.h"

#define RENDERER_WIDTH 320
#define RENDERER_HEIGHT 240
#define RENDERER_TRANS 0
#define RENDERER_LIGHT 3

void Renderer_Init(int start_fullscreen, int hi_res);
void Renderer_Quit(void);
void Renderer_SetFullscreen(int fullscreen);
//void Renderer_StartDrawing(void);
void Renderer_Update(void);
void Renderer_SetHiResMode(int hi_res);
void Renderer_RestartWindow(void);
void Renderer_SetScreenModeFull(int fullscreen, int hi_res);

void Renderer_LoadBitmap(const char *gfx_file);
void Renderer_BuildTables(void);
int Renderer_MakeColor(int r, int g, int b);
int Renderer_GetBitmapHeight(void);
int Renderer_GetBitmapPixel(int x, int y);

void Renderer_Clear(int color);
void Renderer_DrawEmptyRect(const CNM_RECT *rect, int color, int trans, int light);
#ifndef CNM_NO_X86ASSEMBLY
void __cdecl Renderer_DrawRect(const CNM_RECT *rect, int color, int trans, int light);
void __stdcall Renderer_DrawBitmap(int x, int y, const CNM_RECT *src, int trans, int light);
#else
void Renderer_DrawRect(const CNM_RECT *rect, int color, int trans, int light);
void Renderer_DrawBitmap(int x, int y, const CNM_RECT *src, int trans, int light);
#endif
void Renderer_DrawBitmap2(int x, int y, const CNM_RECT *src, int trans, int light, int hflip, int vflip);
//void Renderer_DrawBitmapScaled(const CNM_RECT *dest, const CNM_RECT *src, int trans, int light);
//void Renderer_SetPixel(int x, int y, int color, int trans, int light);
int Renderer_GetPixel(int x, int y);
void Renderer_PlotPixel(int x, int y, int color);
void Renderer_PlotPixel2(int x, int y, int color, int trans, int light);

void Renderer_DrawText(int x, int y, int trans, int light, const char *format, ...);
void Renderer_SetFont(int base_x, int base_y, int font_w, int font_h);

#endif