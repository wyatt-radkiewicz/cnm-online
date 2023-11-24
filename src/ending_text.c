#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "ending_text.h"
#include "renderer.h"
#include "utility.h"
#include "input.h"
#include "audio.h"

char ending_lines[ENDING_TEXT_MAX_LINES][ENDING_TEXT_MAX_WIDTH + 1];
static int ending_y = 1000000;
static int ending_line_start = 0;
static int ending_line_end = 0;
static int ending_has_started = CNM_FALSE;

static int dialoge_line_start = 0;
static int dialoge_line_end = 0;
static int dialoge_current_line = 0;
static int dialoge_active = CNM_FALSE;
static int dialoge_cooldown = 0;
static int dialoge_active_fadeout = 0;

static int draw_to_line = 0;
static int draw_to_char = 0;
static int done_drawing = CNM_FALSE;
static int dialoge_push_cooldown = 0;
static int dialoge_automatic_end_timer = 0;

void EndingText_ResetYValue(void)
{
	dialoge_cooldown = 0;
	ending_y = 1000000;
	ending_has_started = CNM_FALSE;
	ending_line_start = 0;
	ending_line_end = 0;
	dialoge_line_start = 0;
	dialoge_line_end = 0;
	dialoge_current_line = 0;
	dialoge_active = CNM_FALSE;
	dialoge_automatic_end_timer = 0;
	dialoge_active_fadeout = 7;
}
void EndingText_SetLine(int index, const char *line)
{
	strncpy(ending_lines[index], line, ENDING_TEXT_MAX_WIDTH);
	ending_lines[index][ENDING_TEXT_MAX_WIDTH] = '\0';
}
const char *EndingText_GetLine(int index)
{
	return ending_lines[index];
}
void EndingText_Start(int start_line, int end_line)
{
	if (ending_has_started && ending_line_start == start_line && ending_line_end == end_line)
		return;
	ending_y = 0;
	ending_line_start = start_line;
	ending_line_end = end_line;
	ending_has_started = CNM_TRUE;
}
void EndingText_Draw(void)
{
	ending_y += 1;

	Renderer_SetFont(256, 192, 8, 8);

	int num_lines = (ending_line_end - ending_line_start);
	if (-ending_y + (num_lines * 8 + 16) + RENDERER_HEIGHT > 0)
	{
		int y = RENDERER_HEIGHT - ending_y;
		for (int i = ending_line_start; i < ending_line_end; i++)
		{
			Renderer_DrawText(32, y + i * 8, 0, RENDERER_LIGHT, ending_lines[i]);
		}
	}
	else
	{
		ending_has_started = CNM_FALSE;
	}
}
void EndingText_ClearAllLines(void)
{
	memset(ending_lines, 0, sizeof(ending_lines));
}

void Dialoge_Start(int start_line, int end_line)
{
	if (dialoge_active && dialoge_line_start == start_line && dialoge_line_end == end_line)
		return;
	if (dialoge_line_start == start_line && dialoge_line_end == end_line && dialoge_cooldown > 0)
		return;
	dialoge_line_start = start_line;
	dialoge_line_end = end_line;
	dialoge_current_line = start_line;
	dialoge_active = CNM_TRUE;
	draw_to_line = 0;
	draw_to_char = 0;
	done_drawing = CNM_FALSE;
	dialoge_push_cooldown = 0;
}
void Dialoge_Update(void)
{
	if (!dialoge_active) {
		dialoge_cooldown--;
		return;
	}
	dialoge_cooldown = 45;
	dialoge_push_cooldown--;
	if (done_drawing) dialoge_automatic_end_timer++;
	else dialoge_automatic_end_timer = 0;
	if (dialoge_automatic_end_timer > 30 && dialoge_push_cooldown < 0)
	{
		dialoge_push_cooldown = 8;
		if (done_drawing)
		{
			dialoge_current_line += DIALOGE_PAGE_LINES;
			draw_to_line = 0;
			draw_to_char = 0;
			done_drawing = CNM_FALSE;
		}
		else
			draw_to_line = DIALOGE_PAGE_LINES;
	}

	if (draw_to_line < DIALOGE_PAGE_LINES && dialoge_current_line + draw_to_line <= dialoge_line_end && !done_drawing)
	{
		//if (Game_GetFrame() % 2 == 0)
		//{
			//if (Game_GetFrame() % 4 == 0)
				//Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			draw_to_char++;
			if (ending_lines[dialoge_current_line + draw_to_line][draw_to_char] == '\0' ||
				draw_to_char >= ENDING_TEXT_MAX_WIDTH)
			{
				draw_to_line++;
				draw_to_char = 0;
				Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			}
		//}
	}
	if (draw_to_line >= DIALOGE_PAGE_LINES || dialoge_current_line + draw_to_line > dialoge_line_end || ending_lines[dialoge_current_line + draw_to_line][0] == '\0')
		done_drawing = CNM_TRUE;
	if (dialoge_current_line > dialoge_line_end)
		dialoge_active = CNM_FALSE;
}
int Dialoge_IsActive(void) {
	return dialoge_active;
}
void Dialoge_Draw(void)
{
	CNM_RECT r;

	if (dialoge_active) {
		if (dialoge_active_fadeout > 0) dialoge_active_fadeout--;
	} else {
		if (dialoge_active_fadeout < 7 && Game_GetFrame() % 2 == 0) dialoge_active_fadeout++;
	}

	if (dialoge_active_fadeout >= 7)
		return;
	
	// Draw dialoge background
	for (int y = 0; y < DIALOGE_PAGE_LINES + 3; y++)
	{
		switch (y)
		{
		case 0: Util_SetRect(&r, 384, 584, 32, 8); break; // Dialoge Bottom
		case DIALOGE_PAGE_LINES + 2: Util_SetRect(&r, 416, 576, 32, 8); break; // Dialoge Top
		default: Util_SetRect(&r, 416, 584, 32, 8); break; // Dialoge Middle
		}

		for (int x = 0; x < RENDERER_WIDTH; x += 32)
			Renderer_DrawBitmap(x, RENDERER_HEIGHT - (y+1)*8, &r, CNM_MIN(7, dialoge_active_fadeout + 3), RENDERER_LIGHT);
	}

	// Draw the dialoge
	Renderer_SetFont(256, 192, 8, 8);
	char line_buf[ENDING_TEXT_MAX_WIDTH * 2];

	for (int i = 0; i <= draw_to_line; i++)
	{
		if (dialoge_current_line + i > dialoge_line_end || i >= DIALOGE_PAGE_LINES)
			break;
		strcpy(line_buf, ending_lines[dialoge_current_line + i]);
		if (i == draw_to_line)
			line_buf[draw_to_char + 1] = '\0';
		Renderer_DrawText(8, (RENDERER_HEIGHT - (DIALOGE_PAGE_LINES + 2) * 8) + i*8 + 4, dialoge_active_fadeout, RENDERER_LIGHT, line_buf);
	}
	//Renderer_DrawText(8, (RENDERER_HEIGHT - 16), 0, RENDERER_LIGHT, "PRESS ENTER TO ESCAPE DIALOGE BOX");
}
void Dialoge_End(void)
{
	dialoge_active = CNM_FALSE;
}
