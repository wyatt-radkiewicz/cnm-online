#ifndef _pause_menu_h_
#define _pause_menu_h_

enum pause_menu_funcs {
	PAUSE_MENU_RESTART,
	PAUSE_MENU_RESPAWN,
	PAUSE_MENU_CONTINUE,
	PAUSE_MENU_EXIT,
};

typedef void(*pause_menu_func_t)(void);

extern int g_can_pause;

void pause_menu_init(void);
void pause_menu_focus(void);
void pause_menu_unfocus(void);
void pause_menu_update(void);
void pause_menu_draw(void);
int pause_menu_isfocused(void);
void pause_menu_setcallback(enum pause_menu_funcs ty, pause_menu_func_t func);

#endif
