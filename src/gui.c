#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "input.h"
#include "gui.h"
#include "renderer.h"
#include "audio.h"
#include "game.h"

static void Gui_ButtonUpdate(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing);
static void Gui_NumberUpdate(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing);
static void Gui_StringUpdate(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing);
static void Gui_SetUpdate(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing);
static void Gui_FloatUpdate(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing);

static void Gui_GenericDraw(int index, GUI_ELEMENT *elem, int y, int *left);
static void Gui_StringDraw(int index, GUI_ELEMENT *elem, int y);
static void Gui_SetDraw(int index, GUI_ELEMENT *elem, int y);
static void Gui_BitmapDraw(int index, GUI_ELEMENT *elem, int y);
static void Gui_BitmapButtonDraw(int index, GUI_ELEMENT *elem, int y);

static GUI_FRAME *gui_root;
static GUI_FRAME *gui_frame;
static int gui_focused;
static int gui_initialized = CNM_FALSE;
static int gui_state_updated;

static int Gui_GetAlignmentPos(int width, int x_anchor, int alignment);
static int Gui_GetBoundedYValue(int y);
static void Gui_DrawBitmapGeneric(int index, int y, const CNM_RECT *src, int x_offset, int side, int trans, int light);

void Gui_Init(void)
{
	gui_root = NULL;
	gui_frame = NULL;
	gui_focused = CNM_FALSE;
	gui_initialized = CNM_TRUE;
	gui_state_updated = CNM_FALSE;
}
void Gui_Reset(void)
{
	if (!gui_initialized)
		return;

	Gui_Losefocus();
	gui_frame = NULL;
	gui_root = NULL;
	gui_state_updated = CNM_TRUE;
}
void Gui_Update(void)
{
	int end_editing, bugged_move_index, started_editing, did_move;
	int i;
	
	if (!gui_initialized || !gui_focused || gui_frame == NULL)
		return;
	gui_state_updated = CNM_FALSE;
	started_editing = CNM_FALSE;
	end_editing = CNM_FALSE;
	bugged_move_index = -1;
	
	if (!gui_frame->elements[gui_frame->active_index].hoverable)
	{
		for (i = gui_frame->active_index; i < gui_frame->num_elements; i++)
		{
			if (gui_frame->elements[i].hoverable)
			{
				bugged_move_index = i;
				end_editing = CNM_TRUE;
				break;
			}
		}
		for (i = gui_frame->active_index; i > -1 && bugged_move_index == -1; i--)
		{
			if (gui_frame->elements[i].hoverable)
			{
				bugged_move_index = i;
				end_editing = CNM_TRUE;
				break;
			}
		}
	}

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_GUI))
	{
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		if (gui_frame->editing)
			end_editing = CNM_TRUE;
		else
			Gui_SwitchBack();
	}
	if (Input_GetButtonPressedRepeated(INPUT_DOWN, INPUT_STATE_GUI) && !gui_frame->editing)
	{
		for (i = gui_frame->active_index + 1; i < gui_frame->num_elements; i++)
		{
			if (gui_frame->elements[i].hoverable)
			{
				Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
				gui_frame->active_index = i;
				break;
			}
		}
		if (gui_frame->active_index - gui_frame->cam_index >= gui_frame->props.line_count)
		{
			gui_frame->cam_index = gui_frame->active_index - gui_frame->props.line_count + 1;
		}
	}
	if (Input_GetButtonPressedRepeated(INPUT_UP, INPUT_STATE_GUI) && !gui_frame->editing)
	{
		did_move = CNM_FALSE;
		for (i = gui_frame->active_index - 1; i > -1; i--)
		{
			if (gui_frame->elements[i].hoverable)
			{
				Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
				gui_frame->active_index = i;
				did_move = CNM_TRUE;
				break;
			}
		}

		if (did_move)
		{
			did_move = CNM_FALSE;
			for (i = gui_frame->active_index - 1; i > -1; i--)
			{
				if (gui_frame->elements[i].hoverable)
				{
					did_move = CNM_TRUE;
					break;
				}
			}
		}

		if (!did_move && gui_frame->active_index < gui_frame->props.line_count)
		{
			gui_frame->cam_index = 0;
		}
		else if (gui_frame->active_index < gui_frame->cam_index)
		{
			gui_frame->cam_index = gui_frame->active_index;
		}
	}
	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_GUI) && gui_frame->elements[gui_frame->active_index].editable &&
		!gui_frame->editing && !end_editing)
	{
		Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		gui_frame->editing = CNM_TRUE;
		started_editing = CNM_TRUE;
	}

	for (i = 0; i < gui_frame->num_elements && !gui_state_updated; i++)
	{
		if (gui_frame->elements[i].update != NULL)
			gui_frame->elements[i].update(i, gui_frame->elements + i, i == gui_frame->active_index, started_editing, end_editing);
	}
	if (gui_state_updated)
	{
		gui_state_updated = CNM_FALSE;
		return;
	}
	if (end_editing)
		gui_frame->editing = CNM_FALSE;
	if (bugged_move_index != -1)
		gui_frame->active_index = bugged_move_index;
}
void Gui_Draw(void)
{
	CNM_RECT src;
	int i, left, y, arrow_pos;
	if (!gui_initialized || gui_frame == NULL || !gui_focused)
		return;

	arrow_pos = (gui_frame->props.bounds[1] - gui_frame->props.bounds[0]) / 2 - 8 + gui_frame->props.bounds[0];
	if (gui_frame->cam_index != 0)
	{
		/* Draw arrow going up */
		y = gui_frame->props.top - 14;
		Util_SetRect(&src, 360, 1288, 16, 8);
		Renderer_DrawBitmap(arrow_pos, y, &src, 0, RENDERER_LIGHT);
	}
	for (i = 0, y = gui_frame->props.top - gui_frame->cam_index * 8; i < gui_frame->num_elements; i++, y += 8)
	{
		if (i - gui_frame->cam_index >= gui_frame->props.line_count)
			break;

		if (!gui_frame->elements[i].active)
			Renderer_SetFont(384, 1264, 8, 8);
		if (i >= gui_frame->cam_index)
			Gui_GenericDraw(i, gui_frame->elements + i, y, &left);
		if (gui_frame->elements[i].draw != NULL)
			gui_frame->elements[i].draw(i, gui_frame->elements + i, y);
		if (!gui_frame->elements[i].active)
			Renderer_SetFont(384, 448, 8, 8);

		if (i == gui_frame->active_index && gui_frame->elements[i].type && i >= gui_frame->cam_index)
		{
			/* Draw selecting pointer */
			if (gui_frame->editing)
			{
				Util_SetRect(&src, 352, (Game_GetFrame() / 3) % 6 * 8 + 1264, 8, 8);
				Renderer_DrawBitmap(left - 10, y, &src, 0, RENDERER_LIGHT);
			}
			else
			{
				Util_SetRect(&src, 376, (Game_GetFrame() / 3) % 6 * 8 + 1264, 8, 8);
				Renderer_DrawBitmap(left - 14, y, &src, 0, RENDERER_LIGHT);
			}
		}
	}
	if (i != gui_frame->num_elements)
	{
		/* Draw arrow going down */
		y = (gui_frame->props.top + gui_frame->props.line_count * 8) + 6;
		Util_SetRect(&src, 360, 1280, 16, 8);
		Renderer_DrawBitmap(arrow_pos, y, &src, 0, RENDERER_LIGHT);
	}
}
void Gui_SetRoot(GUI_FRAME *frame)
{
	if (!gui_initialized)
		return;

	frame->active_index = 0;
	frame->cam_index = 0;
	frame->editing = CNM_FALSE;
	gui_root = frame;
	gui_frame = gui_root;
}
GUI_FRAME *Gui_GetRoot(void)
{
	if (!gui_initialized)
		return NULL;

	return gui_root;
}
GUI_FRAME *Gui_GetCurrentFrame(void)
{
	if (!gui_initialized)
		return NULL;

	return gui_frame;
}
void Gui_Focus(void)
{
	if (!gui_initialized)
		return;

	if (!gui_focused)
	{
		Input_PushState(INPUT_STATE_GUI);
		gui_focused = CNM_TRUE;
	}
}
void Gui_Losefocus(void)
{
	if (!gui_initialized)
		return;

	if (gui_focused)
	{
		Input_PopState();
		gui_focused = CNM_FALSE;
		gui_state_updated = CNM_TRUE;
	}
}
int Gui_IsFocused(void)
{
	if (!gui_initialized)
		return CNM_FALSE;

	return gui_focused;
}
void Gui_SwitchBack(void)
{
	if (!gui_initialized)
		return;
	if (gui_frame->parent != NULL || gui_focused)
	{
		if (gui_frame->elements[gui_frame->active_index].update && gui_frame->editing)
		{
			gui_frame->elements[gui_frame->active_index].update(
				gui_frame->active_index, gui_frame->elements + gui_frame->active_index, CNM_TRUE, CNM_FALSE, CNM_TRUE);
			gui_frame->editing = CNM_FALSE;
		}
		gui_state_updated = CNM_TRUE;
	}

	if (gui_frame->escape_callback != NULL)
		gui_frame->escape_callback(gui_frame);

	if (gui_frame->parent != NULL)
		gui_frame = gui_frame->parent;
	else if (gui_focused)
		Gui_Losefocus();
}

GUI_FRAME *Gui_CreateFrame(int num_elements, int reserve, GUI_FRAME *parent, const GUI_FRAME_PROPS *props, int custom_hint)
{
	GUI_FRAME *frame;
	int i;
	if (!gui_initialized)
		return NULL;

	frame = malloc(sizeof(GUI_FRAME) + sizeof(GUI_ELEMENT) * reserve);
	frame->elements = (GUI_ELEMENT *)(frame + 1);
	frame->num_elements = num_elements;
	frame->active_index = 0;
	frame->cam_index = 0;
	frame->editing = CNM_FALSE;
	frame->custom_hint = custom_hint;
	frame->escape_callback = NULL;
	memset(frame->elements, 0, sizeof(GUI_ELEMENT) * reserve);
	for (i = 0; i < num_elements; i++)
	{
		frame->elements[i].frame = frame;
	}
	frame->parent = parent;
	memcpy(&frame->props, props, sizeof(GUI_FRAME_PROPS));
	return frame;
}
void Gui_DestroyFrame(GUI_FRAME *frame)
{
	if (!gui_initialized || frame == NULL)
		return;
	int i;
	for (i = 0; i < frame->num_elements; i++)
	{
		if (frame->elements[i].type == GUI_ELEMENT_SET)
		{
			for (int j = 0; j < frame->elements[i].props.set_size; j++)
			{
				free(frame->elements[i].props.set[j]);
			}
			free(frame->elements[i].props.set);
		}
	}
	free(frame);
}
static GUI_ELEMENT *Gui_InitGenericElement(GUI_FRAME *frame, int index, int type, GUI_INTERACT_CALLBACK callback,
										   GUI_ELEMENT_UPDATE update, GUI_ELEMENT_DRAW draw, const char *name, int hoverable, int editable)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return NULL;

	elem = frame->elements + index;
	memset(elem, 0, sizeof(GUI_ELEMENT));
	strncpy(elem->name, name, UTIL_MAX_TEXT_WIDTH - 1);
	elem->frame = frame;
	elem->type = type;
	elem->callback = callback;
	elem->custom_hint = 0;
	elem->update = update;
	elem->draw = draw;
	elem->hoverable = hoverable;
	elem->editable = editable;
	elem->active = CNM_TRUE;
	return elem;
}
void Gui_InitHeaderElement(GUI_FRAME *frame, int index, const char *name)
{
	Gui_InitGenericElement(frame, index, GUI_ELEMENT_HEADER, NULL, NULL, NULL, name, CNM_FALSE, CNM_FALSE);
}
void Gui_InitButtonElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name, GUI_FRAME *btn_link, int toggleable)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return;

	elem = Gui_InitGenericElement(frame, index, GUI_ELEMENT_BUTTON, callback, Gui_ButtonUpdate, NULL, name, CNM_TRUE, CNM_FALSE);
	elem->callback = callback;
	elem->props.toggleable = toggleable;
	elem->props.btn_link = btn_link;
}
void Gui_InitNumberElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name, int min, int max, int initial)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return;

	elem = Gui_InitGenericElement(frame, index, GUI_ELEMENT_NUMBER, callback, Gui_NumberUpdate, Gui_StringDraw, name, CNM_TRUE, CNM_TRUE);
	elem->props.min = min;
	elem->props.max = max;
	memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
	if (initial < min)
		initial = min;
	if (initial > max)
		initial = max;
	sprintf(elem->props.string, "%d", initial);
}
void Gui_InitStringElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name, int max)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return;

	elem = Gui_InitGenericElement(frame, index, GUI_ELEMENT_STRING, callback, Gui_StringUpdate, Gui_StringDraw, name, CNM_TRUE, CNM_TRUE);
	memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
	if (max > UTIL_MAX_TEXT_WIDTH)
		max = UTIL_MAX_TEXT_WIDTH;
	elem->props.max = max;
	elem->props.allow_spaces = CNM_TRUE;
}
void Gui_InitSetElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return;

	elem = Gui_InitGenericElement(frame, index, GUI_ELEMENT_SET, callback, Gui_SetUpdate, Gui_SetDraw, name, CNM_TRUE, CNM_FALSE);
	elem->props.set_index = 0;
	elem->props.set_size = 0;
	elem->props.set_capacity = 10;
	elem->props.set = malloc(elem->props.set_capacity * sizeof(void *));
	memset(elem->props.set, 0, elem->props.set_capacity * sizeof(void *));
}
void Gui_InitBitmapElement(GUI_FRAME *frame, int index, int offset, int bitmapx, int bitmapy, int w, int h)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return;

	elem = Gui_InitGenericElement(frame, index, GUI_ELEMENT_BITMAP, NULL, NULL, Gui_BitmapDraw, "", CNM_FALSE, CNM_FALSE);
	elem->props.bitmap_offset = offset;
	elem->props.bitmap.x = bitmapx;
	elem->props.bitmap.y = bitmapy;
	elem->props.bitmap.w = w;
	elem->props.bitmap.h = h;
	elem->props.bitmap_trans = 0;
	elem->props.bitmap_light = RENDERER_LIGHT;
}
void Gui_InitPaddingElement(GUI_FRAME *frame, int index)
{
	if (!gui_initialized)
		return;
	Gui_InitGenericElement(frame, index, GUI_ELEMENT_NULL, NULL, NULL, NULL, "", CNM_TRUE, CNM_FALSE);
}
void Gui_InitNullElement(GUI_FRAME *frame, int index)
{
	if (!gui_initialized)
		return;
	Gui_InitGenericElement(frame, index, GUI_ELEMENT_NULL, NULL, NULL, NULL, "", CNM_FALSE, CNM_FALSE);
}
void Gui_InitBitmapButtonElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name, GUI_FRAME *btn_link,
								 int toggleable, int offset, int bitmapx, int bitmapy, int w, int h)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return;

	elem = Gui_InitGenericElement(frame, index, GUI_ELEMENT_BITMAP_BUTTON, callback, Gui_ButtonUpdate, Gui_BitmapButtonDraw, name, CNM_TRUE, CNM_FALSE);
	elem->callback = callback;
	elem->props.toggleable = toggleable;
	elem->props.btn_link = btn_link;
	elem->props.bitmap_offset = offset;
	elem->props.bitmap.x = bitmapx;
	elem->props.bitmap.y = bitmapy;
	elem->props.bitmap.w = w;
	elem->props.bitmap.h = h;
	elem->props.bitmap_trans = 0;
	elem->props.bitmap_light = RENDERER_LIGHT;
}
void Gui_InitFloatElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name, float min, float max, float initial)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return;

	elem = Gui_InitGenericElement(frame, index, GUI_ELEMENT_FLOAT, callback, Gui_FloatUpdate, Gui_StringDraw, name, CNM_TRUE, CNM_TRUE);
	elem->props.fmin = min;
	elem->props.fmax = max;
	memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
	if (initial < min)
		initial = min;
	if (initial > max)
		initial = max;
	sprintf(elem->props.string, "%.2f", initial);
}
void Gui_AddItemToSet(GUI_FRAME *frame, int index, const char *item_name)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return;

	elem = frame->elements + index;
	if (elem->props.set_size < elem->props.set_capacity)
	{
		elem->props.set[elem->props.set_size] = malloc(strlen(item_name) + 1);
		strcpy(elem->props.set[elem->props.set_size++], item_name);
		if (elem->props.set_size == elem->props.set_capacity)
		{
			int old_capacity = elem->props.set_capacity;
			elem->props.set_capacity += 10;
			char **new_set = malloc(elem->props.set_capacity * sizeof(void *));
			memcpy(new_set, elem->props.set, old_capacity * sizeof(void *));
			free(elem->props.set);
			elem->props.set = new_set;
		}
	}
}
int Gui_GetNumberElementInt(GUI_FRAME *frame, int index)
{
	GUI_ELEMENT *elem;
	int num, len;
	if (!gui_initialized)
		return 0;

	elem = frame->elements + index;
	num = atoi(elem->props.string);
	len = strlen(elem->props.string);
	if (len > 0)
	{
		if (elem->props.string[len - 1] == 's')
		{
			num *= 30;
			sprintf(elem->props.string, "%d", num);
		}
		else if (elem->props.string[len - 1] == 'm')
		{
			num *= 30 * 60;
			sprintf(elem->props.string, "%d", num);
		}
	}

	char *plus = strchr(elem->props.string, '+');
	char *multiply = strchr(elem->props.string, '*');
	char *divide = strchr(elem->props.string, '/');
	char *subtract = strrchr(elem->props.string, '-');
	if (plus != NULL)
	{
		num += atoi(plus + 1);
		sprintf(elem->props.string, "%d", num);
	}
	else if (multiply != NULL)
	{
		num *= atoi(multiply + 1);
		sprintf(elem->props.string, "%d", num);
	}
	else if (divide != NULL)
	{
		num /= atoi(divide + 1);
		sprintf(elem->props.string, "%d", num);
	}
	else if (subtract != NULL && plus == NULL && subtract != elem->props.string)
	{
		num -= atoi(subtract + 1);
		sprintf(elem->props.string, "%d", num);
	}

	return num;
}
void Gui_SetNumberElementInt(GUI_FRAME *frame, int index, int new_value)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return;

	elem = frame->elements + index;
	if (new_value < elem->props.min)
		new_value = elem->props.min;
	if (new_value > elem->props.max)
		new_value = elem->props.max;
	memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
	sprintf(elem->props.string, "%d", new_value);
	if (elem->callback != NULL)
		elem->callback(elem, index);
}
float Gui_GetFloatElementFloat(GUI_FRAME *frame, int index)
{
	GUI_ELEMENT *elem;
	float num;
	if (!gui_initialized)
		return 0;

	elem = frame->elements + index;
	num = (float)atof(elem->props.string);
	char *plus = strchr(elem->props.string, '+');
	char *multiply = strchr(elem->props.string, '*');
	char *divide = strchr(elem->props.string, '/');
	char *subtract = strrchr(elem->props.string, '-');
	if (plus != NULL)
	{
		num += (float)atof(plus+1);
		sprintf(elem->props.string, "%.2f", num);
	}
	else if (multiply != NULL)
	{
		num *= (float)atof(multiply + 1);
		sprintf(elem->props.string, "%.2f", num);
	}
	else if (divide != NULL)
	{
		num /= (float)atof(divide + 1);
		sprintf(elem->props.string, "%.2f", num);
	}
	else if (subtract != NULL && plus == NULL && subtract != elem->props.string)
	{
		num -= (float)atof(subtract + 1);
		sprintf(elem->props.string, "%.2f", num);
	}
	return num;
}
void Gui_SetFloatElementFloat(GUI_FRAME *frame, int index, float new_value)
{
	GUI_ELEMENT *elem;
	if (!gui_initialized)
		return;

	elem = frame->elements + index;
	if (new_value < elem->props.fmin)
		new_value = elem->props.fmin;
	if (new_value > elem->props.fmax)
		new_value = elem->props.fmax;
	memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
	sprintf(elem->props.string, "%.2f", new_value);
	if (elem->callback != NULL)
		elem->callback(elem, index);
}

static void Gui_ButtonUpdate(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing)
{
	if (active && !lost_editing && Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_GUI))
	{
		Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());

		if (elem->props.toggleable)
		{	
			elem->active = !elem->active;
		}
		if (elem->callback != NULL)
		{
			elem->callback(elem, index);
		}
		if (!elem->props.toggleable && elem->props.btn_link != NULL)
		{	
			gui_frame = elem->props.btn_link;
			gui_state_updated = CNM_TRUE;
		}
	}
}
static void Gui_NumberUpdate(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing)
{
	char print_buf[3];
	char c;
	int num;

	if (active && !lost_editing && gui_frame->editing)
	{
		if (started_editing && strcmp(elem->props.string, "0") == 0)
			memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);

		c = Input_GetCharPressed(INPUT_STATE_GUI);
		if (Input_GetButtonPressedRepeated(INPUT_BACKSPACE, INPUT_STATE_GUI) && strlen(elem->props.string) > 0)
		{
			elem->props.string[strlen(elem->props.string) - 1] = '\0';
			if (strcmp(elem->props.string, "-") == 0)
				elem->props.string[strlen(elem->props.string) - 1] = '\0';
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		}
		if (((c >= '0' && c <= '9') || c == '-' || c == 's' || c == 'm' || c == '*' || c == '/' || c == '+') && strlen(elem->props.string) < UTIL_MAX_TEXT_WIDTH)
		{
			print_buf[0] = Input_GetCharPressed(INPUT_STATE_GUI);
			print_buf[1] = '\0';
			strcat(elem->props.string, print_buf);
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		}
		num = (Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_GUI) || Input_GetButtonPressedRepeated(INPUT_UP, INPUT_STATE_GUI)) -
			(Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_GUI) || Input_GetButtonPressedRepeated(INPUT_DOWN, INPUT_STATE_GUI));
		if (num != 0)
		{
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			num += Gui_GetNumberElementInt(gui_frame, gui_frame->active_index);
			memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
			sprintf(elem->props.string, "%d", num);
		}
	}

	if (active && gui_frame->editing && lost_editing)
	{
		num = Gui_GetNumberElementInt(gui_frame, index);
		if (num < elem->props.min)
			num = elem->props.min;
		if (num > elem->props.max)
			num = elem->props.max;
		memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
		sprintf(elem->props.string, "%d", num);
		if (elem->callback != NULL)
			elem->callback(elem, index);
	}
}
static void Gui_FloatUpdate(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing)
{
	char print_buf[3];
	char c;
	int inc;
	float num;

	if (active && !lost_editing && gui_frame->editing)
	{
		if (started_editing && (float)atoi(elem->props.string) == atof(elem->props.string))
			sprintf(elem->props.string, "%d", atoi(elem->props.string));

		if (started_editing && atof(elem->props.string) == 0.0)
			memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);

		c = Input_GetCharPressed(INPUT_STATE_GUI);
		if (Input_GetButtonPressedRepeated(INPUT_BACKSPACE, INPUT_STATE_GUI) && strlen(elem->props.string) > 0)
		{
			elem->props.string[strlen(elem->props.string) - 1] = '\0';
			if (strcmp(elem->props.string, "-") == 0)
				elem->props.string[strlen(elem->props.string) - 1] = '\0';
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		}
		if (((c >= '0' && c <= '9') || c == '-' || c == '.' || c == '+' || c == '/' || c == '*') && strlen(elem->props.string) < UTIL_MAX_TEXT_WIDTH)
		{
			print_buf[0] = Input_GetCharPressed(INPUT_STATE_GUI);
			print_buf[1] = '\0';
			strcat(elem->props.string, print_buf);
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		}
		inc = (Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_GUI) || Input_GetButtonPressedRepeated(INPUT_UP, INPUT_STATE_GUI)) -
			(Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_GUI) || Input_GetButtonPressedRepeated(INPUT_DOWN, INPUT_STATE_GUI));
		if (inc != 0)
		{
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			num = (float)inc + Gui_GetFloatElementFloat(gui_frame, gui_frame->active_index);
			memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
			sprintf(elem->props.string, "%.2f", num);
		}
	}

	if (active && gui_frame->editing && lost_editing)
	{
		num = Gui_GetFloatElementFloat(gui_frame, index);
		if (num < elem->props.fmin)
			num = elem->props.fmin;
		if (num > elem->props.fmax)
			num = elem->props.fmax;
		memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
		sprintf(elem->props.string, "%.2f", num);
		if (elem->callback != NULL)
			elem->callback(elem, index);
	}
}
static void Gui_StringUpdate(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing)
{
	char talkbuf[3];

	if (gui_frame->editing && active && !lost_editing && elem->props.max)
	{
		if (Input_GetButtonPressedRepeated(INPUT_BACKSPACE, INPUT_STATE_GUI) && strlen(elem->props.string) > 0)
		{
			elem->props.string[strlen(elem->props.string) - 1] = '\0';
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		}
		if (Util_IsSafePrintable(Input_GetCharPressed(INPUT_STATE_GUI)) && strlen(elem->props.string) < (size_t)elem->props.max)
		{
			if (elem->props.allow_spaces || Input_GetCharPressed(INPUT_STATE_GUI) != ' ')
			{
				talkbuf[0] = Input_GetCharPressed(INPUT_STATE_GUI);
				talkbuf[1] = '\0';
				strcat(elem->props.string, talkbuf);
				Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			}
		}
	}

	if (lost_editing || ((Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_GUI) && !started_editing)) && active)
	{
		if (elem->callback != NULL)
			elem->callback(elem, index);
		Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
	}
}
static void Gui_SetUpdate(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing)
{
	int move_right = Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_GUI) || Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_GUI);
	int move_left = Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_GUI);

	if (active && !lost_editing && elem->props.set_size)
	{
		if (gui_frame->editing)
		{
			move_right |= Input_GetButtonPressedRepeated(INPUT_UP, INPUT_STATE_GUI);
			move_left |= Input_GetButtonPressedRepeated(INPUT_DOWN, INPUT_STATE_GUI);
		}

		if (move_left && !move_right)
		{
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			elem->props.set_index--;
			if (elem->props.set_index < 0)
				elem->props.set_index = elem->props.set_size - 1;
			if (elem->callback != NULL)
				elem->callback(elem, index);
		}
		else if (!move_left && move_right)
		{
			Audio_PlaySound(44, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			elem->props.set_index++;
			if (elem->props.set_index >= elem->props.set_size)
				elem->props.set_index = 0;
			if (elem->callback != NULL)
				elem->callback(elem, index);
		}
	}
}

static int Gui_GetAlignmentPos(int width, int x_anchor, int alignment)
{
	switch (alignment)
	{
		case GUI_ALIGN_CENTER:
		return x_anchor - width / 2;
		case GUI_ALIGN_RIGHT:
		return x_anchor - width;
		default:
		return x_anchor;
	}
}
static int Gui_GetBoundedYValue(int y)
{
	if (y > gui_frame->props.top + gui_frame->props.line_count * 8)
	{
		y -= y - (gui_frame->props.top + gui_frame->props.line_count * 8);
	}
	else if (y < gui_frame->props.top)
	{
		y = gui_frame->props.top;
	}
	return y;
}
static void Gui_DrawBitmapGeneric(int index, int y, const CNM_RECT *src, int x_offset, int side, int trans, int light)
{
	CNM_RECT rect;
	int start, end;

	if (gui_frame->cam_index >= index + src->h / 8)
		return;
	start = gui_frame->cam_index - index;
	end = (index + src->h / 8) - (gui_frame->cam_index + gui_frame->props.line_count);
	rect.x = src->x;
	if (start > 0)
		rect.y = src->y + start * 8;
	else
		rect.y = src->y;
	rect.w = src->w;
	if (start > 0 && end > 0)
		rect.h = src->h - (start + end) * 8;
	else if (start > 0)
		rect.h = src->h - start * 8;
	else if (end > 0)
		rect.h = src->h - end * 8;
	else
		rect.h = src->h;

	Renderer_DrawBitmap
	(
		x_offset + Gui_GetAlignmentPos(src->w, gui_frame->props.bounds[side], gui_frame->props.align[side]),
		Gui_GetBoundedYValue(y),
		&rect,
		trans,
		light
	);
}

static void Gui_GenericDraw(int index, GUI_ELEMENT *elem, int y, int *left)
{
	*left = Gui_GetAlignmentPos((int)strlen(elem->name) * 8, gui_frame->props.bounds[0], gui_frame->props.align[0]);
	Renderer_DrawText(*left, y, 0, RENDERER_LIGHT, elem->name);
}
static void Gui_StringDraw(int index, GUI_ELEMENT *elem, int y)
{
	if (y != Gui_GetBoundedYValue(y))
		return;

	Renderer_DrawText
	(
		Gui_GetAlignmentPos((int)strlen(elem->props.string) * 8, gui_frame->props.bounds[1], gui_frame->props.align[1]),
		y,
		0,
		RENDERER_LIGHT,
		elem->props.string
	);
}
static void Gui_SetDraw(int index, GUI_ELEMENT *elem, int y)
{
	if (y != Gui_GetBoundedYValue(y) || elem->props.set_index >= elem->props.set_size || elem->props.set == NULL)
		return;

	const char *str = elem->props.set[elem->props.set_index];
	Renderer_DrawText
	(
		Gui_GetAlignmentPos((int)strlen(str) * 8, gui_frame->props.bounds[1], gui_frame->props.align[1]),
		y,
		0,
		RENDERER_LIGHT,
		str
	);
}
static void Gui_BitmapDraw(int index, GUI_ELEMENT *elem, int y)
{
	Gui_DrawBitmapGeneric
	(
		index,
		y,
		&elem->props.bitmap,
		elem->props.bitmap_offset,
		0,
		elem->props.bitmap_trans,
		elem->props.bitmap_light
	);
}
static void Gui_BitmapButtonDraw(int index, GUI_ELEMENT *elem, int y)
{
	Gui_DrawBitmapGeneric
	(
		index,
		y,
		&elem->props.bitmap,
		-elem->props.bitmap_offset,
		1,
		elem->props.bitmap_trans,
		elem->props.bitmap_light
	);
}