#ifndef _input_h_
#define _input_h_

typedef void(*INPUT_QUITCALLBACK)(void);

typedef enum _INPUT_BUTTONS
{
	INPUT_UP,
	INPUT_DOWN,
	INPUT_LEFT,
	INPUT_RIGHT,
	INPUT_W,
	INPUT_S,
	INPUT_A,
	INPUT_D,
	INPUT_TALK,
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
	INPUT_STATE_GUI,
	INPUT_STATE_CONSOLE
} INPUT_STATES;

void Input_Init(void);
void Input_Quit(void);
void Input_Update(void);
void Input_SetQuitCallback(INPUT_QUITCALLBACK callback);
int Input_GetButton(int button, int desired_state);
int Input_GetButtonPressed(int button, int desired_state);
int Input_GetButtonReleased(int button, int desired_state);
int Input_GetButtonPressedRepeated(int button, int desired_state);
char Input_GetCharPressed(int desired_state);

int Input_PushState(int state);
void Input_PopState(void);
int Input_TopState(void);

#endif