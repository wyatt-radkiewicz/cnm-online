#include "renderer.h"
#include "supervirus.h"

#define T 0x00
#define B 0xf7
#define W 0x04

unsigned char _supervirus_bitmap[] = {
	T, T, B, W, W, W, W, W, W, W, W, W, B, T,
	T, B, W, W, W, W, W, W, B, B, W, W, B, T,
	T, B, W, W, W, W, W, B, B, B, B, W, W, B,
	B, W, W, W, W, B, W, B, B, B, B, B, W, B,
	B, W, B, W, W, B, W, B, B, B, B, B, W, B,
	B, W, B, W, B, B, W, B, B, B, B, B, W, B,
	B, W, B, B, B, B, W, B, B, B, B, B, W, B,
	B, B, B, B, B, B, W, W, B, B, B, W, W, B,
	B, W, B, B, B, W, W, W, W, B, W, W, W, B,
	B, W, B, B, W, W, W, W, W, B, W, W, W, B,
	B, W, W, W, W, W, W, W, W, B, W, W, W, B,
	B, W, B, W, W, W, W, W, W, B, B, W, W, B,
	B, W, B, B, W, W, W, W, W, B, B, W, B, T,
	B, W, W, B, B, W, W, B, B, B, W, W, B, T,
	T, B, W, W, B, B, B, B, B, W, W, B, T, T,
	T, T, B, W, W, B, B, B, B, W, W, B, T, T,
	T, T, B, W, W, W, B, B, W, W, B, T, T, T,
	T, T, T, B, B, B, W, W, B, B, B, T, T, T,
	T, T, T, B, B, B, W, W, B, B, B, T, T, T,
	T, T, T, B, B, B, W, W, B, B, B, T, T, T,
	T, T, T, B, W, B, B, B, B, B, B, T, T, T,
	T, T, B, W, W, B, B, B, B, W, B, T, T, T,
	T, T, B, W, W, B, B, B, W, B, B, T, T, T,
	T, T, B, B, B, B, B, B, B, W, B, T, T, T,
	T, T, B, B, B, B, B, B, B, B, B, T, T, T,
	T, T, B, B, B, B, B, B, B, B, B, T, T, T,
	T, T, T, B, B, B, B, B, B, B, B, T, T, T,
	T, T, T, T, T, T, B, B, B, T, T, T, T, T,
};

static int num = 0;
static int _x[32], _y[32];

void supervirus_render(int x, int y) {
	if (num < 32) {
		_x[num] = x;
		_y[num] = y;
		num++;
	}
}
void supervirus_render_all(void) {
	int i, j, k;

	for (k = 0; k < num; k++) {
		for (j = 0; j < 28 * 3; j++)
		{
			for (i = 0; i < 14 * 3; i++)
			{
				Renderer_PlotPixel(_x[k] + i, _y[k] + j, _supervirus_bitmap[(j / 3) * 14 + (i / 3)]);
			}
		}
	}

	num = 0;
}
