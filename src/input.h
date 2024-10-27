#ifndef _input_h_
#define _input_h_

#include <SDL.h>

typedef void(*INPUT_QUITCALLBACK)(void);

typedef enum _INPUT_BUTTONS
{
	INPUT_UP,
	INPUT_DOWN,
	INPUT_LEFT,
	INPUT_RIGHT,
	INPUT_FIRE,
	INPUT_TALK,
	INPUT_MENUUP,
	INPUT_MENUDOWN,
	INPUT_PAUSE,
	INPUT_ESCAPE,
	INPUT_ENTER,
	INPUT_DROP,
	INPUT_CONSOLE,
	INPUT_BACKSPACE,
	INPUT_BUTTONS_MAX
} INPUT_BUTTONS;
typedef enum _INPUT_STATES
{
	INPUT_STATE_NOINPUT,
	INPUT_STATE_PLAYING,
	INPUT_STATE_LOCK_PLAYER,
	INPUT_STATE_GUI,
	INPUT_STATE_CONSOLE,
} INPUT_STATES;

typedef struct inputbind {
	SDL_Scancode sc;
	SDL_GameControllerButton btn;
	struct {
		SDL_GameControllerAxis axis;
		int dir;
	} axis;
} inputbind_t;

void input_reset_binds(void);
void Input_Init(void);
void Input_Quit(void);
void Input_Update(void);
void Input_SetQuitCallback(INPUT_QUITCALLBACK callback);
int Input_GetButton(int button, int desired_state);
int Input_GetButtonPressed(int button, int desired_state);
int Input_GetButtonReleased(int button, int desired_state);
int Input_GetButtonPressedRepeated(int button, int desired_state);
char Input_GetCharPressed(int desired_state);
void input_setbind(INPUT_BUTTONS btn, inputbind_t bind);
inputbind_t input_getbind(INPUT_BUTTONS btn);
void input_start_textmode(void);
void input_end_textmode(void);

int Input_PushState(int state);
void Input_PopState(void);
int Input_TopState(void);

#endif
