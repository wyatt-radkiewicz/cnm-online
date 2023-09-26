#ifndef _background_h_
#define _background_h_
#include "utility.h"

#define BACKGROUND_MAX_LAYERS 32

#define BACKGROUND_NO_REPEAT 0
#define BACKGROUND_REPEAT_DOWN 1
#define BACKGROUND_REPEAT_UP 2
#define BACKGROUND_REPEAT_BOTH 3

#define SERIAL_BACKGROUND_SIZE 68

typedef struct _BACKGROUND_LAYER
{
	float pos[2];

	float origin[2];
	unsigned int repeat[2];
	float scroll[2];
	float speed[2];
	int spacing[2];
	CNM_RECT bitmap;
	int clear_color;
	int high;
	int transparency;
} BACKGROUND_LAYER;

void Background_ResetBackgrounds(void);
BACKGROUND_LAYER *Background_GetLayer(int layer);
void Background_DrawLayer(int layer, int camx, int camy);
void Background_Draw(int layer, int camx, int camy);
void Background_SetVisibleLayers(int id_start, int id_end);

#endif