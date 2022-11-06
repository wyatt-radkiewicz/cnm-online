#ifndef _gui_h_
#define _gui_h_
#include "utility.h"

typedef struct _GUI_ELEMENT GUI_ELEMENT;
typedef struct _GUI_FRAME GUI_FRAME;
typedef void(*GUI_INTERACT_CALLBACK)(GUI_ELEMENT *element, int index);
typedef void(*GUI_ELEMENT_UPDATE)(int index, GUI_ELEMENT *elem, int active, int started_editing, int lost_editing);
typedef void(*GUI_ELEMENT_DRAW)(int index, GUI_ELEMENT *elem, int y);
typedef void(*GUI_FRAME_ESCAPE)(GUI_FRAME *frame);

typedef enum _GUI_ELEMENT_TYPE
{
	GUI_ELEMENT_NULL,
	GUI_ELEMENT_BUTTON,
	GUI_ELEMENT_NUMBER,
	GUI_ELEMENT_STRING,
	GUI_ELEMENT_SET,
	GUI_ELEMENT_HEADER,
	GUI_ELEMENT_BITMAP,
	GUI_ELEMENT_BITMAP_BUTTON,
	GUI_ELEMENT_FLOAT,
	GUI_ELEMENT_TYPE_MAX
} GUI_ELEMENT_TYPE;
typedef struct _GUI_ELEMENT
{
	struct
	{
		char string[UTIL_MAX_TEXT_WIDTH + 1];
		char **set;
		int set_index;
		int set_size;
		int set_capacity;
		int min;
		int max;
		int allow_spaces;
		int toggleable;
		GUI_FRAME *btn_link;
		CNM_RECT bitmap;
		int bitmap_offset;
		int bitmap_trans, bitmap_light;
		float fmin;
		float fmax;
	} props;
	
	int custom_hint;
	int hoverable;
	int editable;
	GUI_ELEMENT_UPDATE update;
	GUI_ELEMENT_DRAW draw;
	GUI_INTERACT_CALLBACK callback;
	char name[UTIL_MAX_TEXT_WIDTH + 1];
	int type;
	int active;
	GUI_FRAME *frame;
} GUI_ELEMENT;

#define GUI_ALIGN_LEFT 0
#define GUI_ALIGN_RIGHT 1
#define GUI_ALIGN_CENTER 2
typedef struct _GUI_FRAME_PROPS
{
	int top, line_count;
	int bounds[2], align[2];
} GUI_FRAME_PROPS;
typedef struct _GUI_FRAME
{
	GUI_FRAME *parent;
	GUI_ELEMENT *elements;
	int num_elements;
	int active_index;
	int cam_index;
	int editing;

	int custom_hint;
	GUI_FRAME_ESCAPE escape_callback;

	GUI_FRAME_PROPS props;
} GUI_FRAME;

void Gui_Init(void);
void Gui_Reset(void);
void Gui_Update(void);
void Gui_Draw(void);
void Gui_SetRoot(GUI_FRAME *frame);
GUI_FRAME *Gui_GetRoot(void);
GUI_FRAME *Gui_GetCurrentFrame(void);
void Gui_Focus(void);
void Gui_Losefocus(void);
int Gui_IsFocused(void);
void Gui_SwitchBack(void);

GUI_FRAME *Gui_CreateFrame(int num_elements, int reserve, GUI_FRAME *parent, const GUI_FRAME_PROPS *props, int custom_hint);
void Gui_DestroyFrame(GUI_FRAME *frame);
void Gui_InitHeaderElement(GUI_FRAME *frame, int index, const char *name);
void Gui_InitButtonElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name, GUI_FRAME *btn_link, int toggleable);
void Gui_InitNumberElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name, int min, int max, int initial);
void Gui_InitStringElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name, int max);
void Gui_InitSetElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name);
void Gui_InitBitmapElement(GUI_FRAME *frame, int index, int offset, int bitmapx, int bitmapy, int w, int h);
void Gui_InitPaddingElement(GUI_FRAME *frame, int index);
void Gui_InitNullElement(GUI_FRAME *frame, int index);
void Gui_InitBitmapButtonElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name, GUI_FRAME *btn_link,
								 int toggleable, int offset, int bitmapx, int bitmapy, int w, int h);
void Gui_InitFloatElement(GUI_FRAME *frame, int index, GUI_INTERACT_CALLBACK callback, const char *name, float min, float max, float initial);
void Gui_AddItemToSet(GUI_FRAME *frame, int index, const char *item_name);
int Gui_GetNumberElementInt(GUI_FRAME *frame, int index);
void Gui_SetNumberElementInt(GUI_FRAME *frame, int index, int new_value);
float Gui_GetFloatElementFloat(GUI_FRAME *frame, int index);
void Gui_SetFloatElementFloat(GUI_FRAME *frame, int index, float new_value);

#endif