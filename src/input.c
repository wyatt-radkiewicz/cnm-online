#include <string.h>
#include <SDL.h>
#include <SDL_scancode.h>
#include "game.h"
#include "input.h"
#include "console.h"
#include "game_console.h"

#define INPUT_MAX_STATES 6

static int input_last_buttons[INPUT_BUTTONS_MAX];
static int input_buttons[INPUT_BUTTONS_MAX];
static int input_repeated_buttons[INPUT_BUTTONS_MAX];
static int input_controller_repeat[INPUT_BUTTONS_MAX];
static INPUT_QUITCALLBACK input_callback = NULL;
static int input_state_stack[INPUT_MAX_STATES];
static int input_state_top;
static char input_last_char;
static int input_initialized = CNM_FALSE;
static int mouse_pressed;
static inputbind_t binds[INPUT_BUTTONS_MAX];
static int textmode;

extern SDL_Window *renderer_win;

static SDL_GameController *input_pad;

static int input_state_stack2[INPUT_MAX_STATES];
static int input_state_top2;
static int input_state_changed;

static char Input_ScancodeToChar(SDL_Scancode c, int shift, int caps);
static void Input_ApplyStateChanges(void);

void input_reset_binds(void) {
	binds[INPUT_UP] = (inputbind_t){
		.sc = SDL_SCANCODE_UP,
		.btn = SDL_CONTROLLER_BUTTON_A,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
	binds[INPUT_DOWN] = (inputbind_t){
		.sc = SDL_SCANCODE_DOWN,
		.btn = SDL_CONTROLLER_BUTTON_X,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
	binds[INPUT_MENUUP] = (inputbind_t){
		.sc = SDL_SCANCODE_UP,
		.btn = SDL_CONTROLLER_BUTTON_INVALID,
		.axis = { .axis = SDL_CONTROLLER_AXIS_LEFTY, .dir = -1 },
	};
	binds[INPUT_MENUDOWN] = (inputbind_t){
		.sc = SDL_SCANCODE_DOWN,
		.btn = SDL_CONTROLLER_BUTTON_INVALID,
		.axis = { .axis = SDL_CONTROLLER_AXIS_LEFTY, .dir = 1 },
	};
	binds[INPUT_LEFT] = (inputbind_t){
		.sc = SDL_SCANCODE_LEFT,
		.btn = SDL_CONTROLLER_BUTTON_INVALID,
		.axis = { .axis = SDL_CONTROLLER_AXIS_LEFTX, .dir = -1 },
	};
	binds[INPUT_RIGHT] = (inputbind_t){
		.sc = SDL_SCANCODE_RIGHT,
		.btn = SDL_CONTROLLER_BUTTON_INVALID,
		.axis = { .axis = SDL_CONTROLLER_AXIS_LEFTX, .dir = 1 },
	};
	binds[INPUT_FIRE] = (inputbind_t){
		.sc = SDL_SCANCODE_Z,
		.btn = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
		.axis = { .axis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT, .dir = 1 },
	};
	binds[INPUT_TALK] = (inputbind_t){
		.sc = SDL_SCANCODE_T,
		.btn = SDL_CONTROLLER_BUTTON_INVALID,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
	binds[INPUT_PAUSE] = (inputbind_t){
		.sc = SDL_SCANCODE_BACKSPACE,
		.btn = SDL_CONTROLLER_BUTTON_START,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
	binds[INPUT_ESCAPE] = (inputbind_t){
		.sc = SDL_SCANCODE_ESCAPE,
		.btn = SDL_CONTROLLER_BUTTON_X,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
	binds[INPUT_ENTER] = (inputbind_t){
		.sc = SDL_SCANCODE_RETURN,
		.btn = SDL_CONTROLLER_BUTTON_A,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
	binds[INPUT_SWAP] = (inputbind_t){
		.sc = SDL_SCANCODE_C,
		.btn = SDL_CONTROLLER_BUTTON_B,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
	binds[INPUT_DROP] = (inputbind_t){
		.sc = SDL_SCANCODE_V,
		.btn = SDL_CONTROLLER_BUTTON_Y,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
	binds[INPUT_CONSOLE] = (inputbind_t){
		.sc = SDL_SCANCODE_GRAVE,
		.btn = SDL_CONTROLLER_BUTTON_INVALID,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
	binds[INPUT_BACKSPACE] = (inputbind_t){
		.sc = SDL_SCANCODE_BACKSPACE,
		.btn = SDL_CONTROLLER_BUTTON_INVALID,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
	binds[INPUT_RESTART] = (inputbind_t){
		.sc = SDL_SCANCODE_R,
		.btn = SDL_CONTROLLER_BUTTON_INVALID,
		.axis = { .axis = SDL_CONTROLLER_AXIS_INVALID, .dir = 0 },
	};
}

void Input_Init(void)
{
	memset(input_last_buttons, 0, sizeof(input_last_buttons));
	memset(input_buttons, 0, sizeof(input_buttons));
	memset(input_repeated_buttons, 0, sizeof(input_repeated_buttons));
	input_state_top = 0;
	input_state_stack[input_state_top] = INPUT_STATE_NOINPUT;
	input_last_char = '\0';
	input_initialized = CNM_TRUE;
	input_state_top2 = 0;
	input_state_changed = CNM_FALSE;
	input_pad = NULL;

	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if (SDL_IsGameController(i))
		{
			input_pad = SDL_GameControllerOpen(i);
			break;
		}
		else
		{
			Console_Print("Couldn't connect the joystick!");
		}
	}
}
void Input_Quit(void)
{
	if (input_pad != NULL) SDL_GameControllerClose(input_pad);
}
void Input_Update(void)
{
	SDL_Event event;
	const unsigned char *keys;
	if (!input_initialized)
		return;

	Input_ApplyStateChanges();

	input_last_char = '\0';
	memset(input_repeated_buttons, 0, sizeof(input_repeated_buttons));
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_MOUSEBUTTONDOWN:
			mouse_pressed = CNM_TRUE;
			break;
		case SDL_MOUSEBUTTONUP:
			mouse_pressed = CNM_FALSE;
			break;
		case SDL_QUIT:
			if (input_callback != NULL)
				input_callback();
			break;
		case SDL_MOUSEMOTION:
			SDL_ShowCursor(SDL_ENABLE);
			break;
		case SDL_KEYDOWN:
			if (renderer_win
				&& (SDL_GetWindowFlags(renderer_win) & SDL_WINDOW_INPUT_FOCUS)) {
				SDL_ShowCursor(SDL_DISABLE);
			}

			if (event.key.keysym.scancode == SDL_SCANCODE_F4 && event.key.keysym.mod & KMOD_LALT && input_callback != NULL)
				input_callback();
			input_last_char = Input_ScancodeToChar(event.key.keysym.scancode,
				event.key.keysym.mod & KMOD_SHIFT,
				event.key.keysym.mod & KMOD_CAPS);

			for (int i = 0; i < INPUT_BUTTONS_MAX; i++) {
				if (binds[i].sc == event.key.keysym.scancode) {
					input_repeated_buttons[i] = CNM_TRUE;
				}
			}
			break;
		case SDL_CONTROLLERDEVICEADDED:
			if (SDL_IsGameController(event.cdevice.which))
			{
				if (input_pad != NULL) SDL_GameControllerClose(input_pad);
				input_pad = SDL_GameControllerOpen(event.cdevice.which);
			}
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			if (SDL_IsGameController(event.cdevice.which) && input_pad != NULL)
			{
				SDL_GameControllerClose(input_pad);
				input_pad = NULL;
			}
			break;
		default: break;
		}
	}

	keys = SDL_GetKeyboardState(NULL);
	memcpy(input_last_buttons, input_buttons, sizeof(input_buttons));

	for (int i = 0; i < INPUT_BUTTONS_MAX; i++) {
		input_buttons[i] = keys[binds[i].sc];
		if (binds[i].sc == SDL_SCANCODE_BACKSPACE
			&& textmode) input_buttons[i] = 0;

		if (!input_pad) continue;

		int cbtn = 0;

		input_controller_repeat[i]++;

		if (binds[i].axis.axis != SDL_CONTROLLER_AXIS_INVALID) {
			int activation = 12000;
			int x = (int)SDL_GameControllerGetAxis(input_pad, binds[i].axis.axis);

			if (binds[i].axis.dir > 0) {
				cbtn |= x > activation;
			} else {
				cbtn |= x < -activation;
			}
		}

		if (binds[i].btn != SDL_CONTROLLER_BUTTON_INVALID) {
			cbtn |= (int)SDL_GameControllerGetButton(input_pad, binds[i].btn);
		}

		if (!cbtn) {
			input_controller_repeat[i] = 0;
		}

		if (cbtn) {
			input_buttons[i] = CNM_TRUE;
			if (input_buttons[i] && !input_last_buttons[i]) {
				input_repeated_buttons[i] = CNM_TRUE;
				input_controller_repeat[i] = 0;
			}
			if (input_controller_repeat[i] >= 25
				&& input_controller_repeat[i] % 5 == 0) {
				input_repeated_buttons[i] = CNM_TRUE;
			}
		}
	}
}
void Input_SetQuitCallback(INPUT_QUITCALLBACK callback)
{
	input_callback = callback;
}
int Input_GetButton(int button, int desired_state)
{
	if (!input_initialized)
		return CNM_FALSE;

	if (Input_TopState() <= desired_state)
		return input_buttons[button];
	else
		return CNM_FALSE;
}
int Input_GetButtonPressed(int button, int desired_state)
{
	if (!input_initialized)
		return CNM_FALSE;

	if (Input_TopState() <= desired_state)
		return !input_last_buttons[button] && input_buttons[button];
	else
		return CNM_FALSE;
}
int Input_GetButtonReleased(int button, int desired_state)
{
	if (!input_initialized)
		return CNM_FALSE;

	if (Input_TopState() <= desired_state)
		return input_last_buttons[button] && !input_buttons[button];
	else
		return CNM_FALSE;
}
int Input_GetButtonPressedRepeated(int button, int desired_state)
{
	if (!input_initialized)
		return CNM_FALSE;

	if (Input_TopState() <= desired_state)
		return input_repeated_buttons[button];
	else
		return CNM_FALSE;
}
char Input_GetCharPressed(int desired_state)
{
	if (!input_initialized)
		return '\0';

	if (Input_TopState() <= desired_state)
		return input_last_char;
	else
		return '\0';
}

static void Input_ApplyStateChanges(void)
{
	memcpy(input_state_stack, input_state_stack2, sizeof(input_state_stack));
	input_state_top = input_state_top2;
	input_state_changed = CNM_FALSE;
}

int Input_PushState(int state)
{
	if (!input_initialized)
		return CNM_TRUE;

	if (!input_state_changed)
	{
		memcpy(input_state_stack2, input_state_stack, sizeof(input_state_stack2));
		input_state_top2 = input_state_top;
	}
	if (input_state_stack2[input_state_top2] <= state && input_state_top2 + 1 < INPUT_MAX_STATES)
	{
		input_state_stack2[++input_state_top2] = state;
		input_state_changed = CNM_TRUE;
		return CNM_TRUE;
	}

	return CNM_FALSE;
}
void Input_PopState(void)
{
	if (!input_initialized)
		return;

	if (!input_state_changed)
	{
		memcpy(input_state_stack2, input_state_stack, sizeof(input_state_stack2));
		input_state_top2 = input_state_top;
	}
	if (input_state_top2 > 0)
	{
		input_state_top2--;
		input_state_changed = CNM_TRUE;
	}
}
int Input_TopState(void)
{
	if (!input_initialized)
		return INPUT_STATE_NOINPUT;

	return input_state_stack[input_state_top];
}
void input_setbind(INPUT_BUTTONS btn, inputbind_t bind) {
	binds[btn] = bind;
	input_last_buttons[btn] = 0;
	input_buttons[btn] = 0;
	input_repeated_buttons[btn] = 0;
}
inputbind_t input_getbind(INPUT_BUTTONS btn) {
	return binds[btn];
}
void input_start_textmode(void) {
	textmode = 1;
}
void input_end_textmode(void) {
	textmode = 0;
}

static char Input_ScancodeToChar(SDL_Scancode c, int shift, int caps)
{
	if (!shift && !caps)
	{
		switch (c)
		{
			case SDL_SCANCODE_SPACE:	return ' ';
			case SDL_SCANCODE_APOSTROPHE:	return '\'';
			case SDL_SCANCODE_COMMA:	return ',';
			case SDL_SCANCODE_MINUS:	return '-';
			case SDL_SCANCODE_PERIOD:	return '.';
			case SDL_SCANCODE_SLASH:	return '/';
			case SDL_SCANCODE_0:	return '0';
			case SDL_SCANCODE_1:	return '1';
			case SDL_SCANCODE_2:	return '2';
			case SDL_SCANCODE_3:	return '3';
			case SDL_SCANCODE_4:	return '4';
			case SDL_SCANCODE_5:	return '5';
			case SDL_SCANCODE_6:	return '6';
			case SDL_SCANCODE_7:	return '7';
			case SDL_SCANCODE_8:	return '8';
			case SDL_SCANCODE_9:	return '9';
			case SDL_SCANCODE_SEMICOLON:	return ';';
			case SDL_SCANCODE_EQUALS:	return '=';
			case SDL_SCANCODE_LEFTBRACKET:	return '[';
			case SDL_SCANCODE_RIGHTBRACKET:	return ']';
			case SDL_SCANCODE_BACKSLASH:	return '\\';
			case SDL_SCANCODE_GRAVE:	return '`';
			case SDL_SCANCODE_A:	return 'a';
			case SDL_SCANCODE_B:	return 'b';
			case SDL_SCANCODE_C:	return 'c';
			case SDL_SCANCODE_D:	return 'd';
			case SDL_SCANCODE_E:	return 'e';
			case SDL_SCANCODE_F:	return 'f';
			case SDL_SCANCODE_G:	return 'g';
			case SDL_SCANCODE_H:	return 'h';
			case SDL_SCANCODE_I:	return 'i';
			case SDL_SCANCODE_J:	return 'j';
			case SDL_SCANCODE_K:	return 'k';
			case SDL_SCANCODE_L:	return 'l';
			case SDL_SCANCODE_M:	return 'm';
			case SDL_SCANCODE_N:	return 'n';
			case SDL_SCANCODE_O:	return 'o';
			case SDL_SCANCODE_P:	return 'p';
			case SDL_SCANCODE_Q:	return 'q';
			case SDL_SCANCODE_R:	return 'r';
			case SDL_SCANCODE_S:	return 's';
			case SDL_SCANCODE_T:	return 't';
			case SDL_SCANCODE_U:	return 'u';
			case SDL_SCANCODE_V:	return 'v';
			case SDL_SCANCODE_W:	return 'w';
			case SDL_SCANCODE_X:	return 'x';
			case SDL_SCANCODE_Y:	return 'y';
			case SDL_SCANCODE_Z:	return 'z';
			default: break;
		}
	}
	else
	{
		switch (c)
		{
			case SDL_SCANCODE_SPACE:	return ' ';
			case SDL_SCANCODE_A:	return 'A';
			case SDL_SCANCODE_B:	return 'B';
			case SDL_SCANCODE_C:	return 'C';
			case SDL_SCANCODE_D:	return 'D';
			case SDL_SCANCODE_E:	return 'E';
			case SDL_SCANCODE_F:	return 'F';
			case SDL_SCANCODE_G:	return 'G';
			case SDL_SCANCODE_H:	return 'H';
			case SDL_SCANCODE_I:	return 'I';
			case SDL_SCANCODE_J:	return 'J';
			case SDL_SCANCODE_K:	return 'K';
			case SDL_SCANCODE_L:	return 'L';
			case SDL_SCANCODE_M:	return 'M';
			case SDL_SCANCODE_N:	return 'N';
			case SDL_SCANCODE_O:	return 'O';
			case SDL_SCANCODE_P:	return 'P';
			case SDL_SCANCODE_Q:	return 'Q';
			case SDL_SCANCODE_R:	return 'R';
			case SDL_SCANCODE_S:	return 'S';
			case SDL_SCANCODE_T:	return 'T';
			case SDL_SCANCODE_U:	return 'U';
			case SDL_SCANCODE_V:	return 'V';
			case SDL_SCANCODE_W:	return 'W';
			case SDL_SCANCODE_X:	return 'X';
			case SDL_SCANCODE_Y:	return 'Y';
			case SDL_SCANCODE_Z:	return 'Z';
			default: break;
		}

		if (!shift)
		{
			switch (c)
			{
				case SDL_SCANCODE_APOSTROPHE:	return '\'';
				case SDL_SCANCODE_COMMA:	return ',';
				case SDL_SCANCODE_MINUS:	return '-';
				case SDL_SCANCODE_PERIOD:	return '.';
				case SDL_SCANCODE_SLASH:	return '/';
				case SDL_SCANCODE_0:	return '0';
				case SDL_SCANCODE_1:	return '1';
				case SDL_SCANCODE_2:	return '2';
				case SDL_SCANCODE_3:	return '3';
				case SDL_SCANCODE_4:	return '4';
				case SDL_SCANCODE_5:	return '5';
				case SDL_SCANCODE_6:	return '6';
				case SDL_SCANCODE_7:	return '7';
				case SDL_SCANCODE_8:	return '8';
				case SDL_SCANCODE_9:	return '9';
				case SDL_SCANCODE_SEMICOLON:	return ';';
				case SDL_SCANCODE_EQUALS:	return '=';
				case SDL_SCANCODE_LEFTBRACKET:	return '[';
				case SDL_SCANCODE_RIGHTBRACKET:	return ']';
				case SDL_SCANCODE_BACKSLASH:	return '\\';
				case SDL_SCANCODE_GRAVE:	return '`';
				default: break;
			}
		}
		else
		{
			switch (c)
			{
				case SDL_SCANCODE_1:		return '!';
				case SDL_SCANCODE_APOSTROPHE:	return '\"';
				case SDL_SCANCODE_3:	return '#';
				case SDL_SCANCODE_4:	return '$';
				case SDL_SCANCODE_5:	return '%';
				case SDL_SCANCODE_7:	return '&';
				case SDL_SCANCODE_9:	return '(';
				case SDL_SCANCODE_0:	return ')';
				case SDL_SCANCODE_8:	return '*';
				case SDL_SCANCODE_EQUALS:	return '+';
				case SDL_SCANCODE_SEMICOLON:	return ':';
				case SDL_SCANCODE_COMMA:	return '<';
				case SDL_SCANCODE_PERIOD:	return '>';
				case SDL_SCANCODE_SLASH:	return '?';
				case SDL_SCANCODE_2:	return '@';
				case SDL_SCANCODE_6:	return '^';
				case SDL_SCANCODE_MINUS:	return '_';
				case SDL_SCANCODE_LEFTBRACKET:	return '{';
				case SDL_SCANCODE_RIGHTBRACKET:	return '}';
				case SDL_SCANCODE_BACKSLASH:	return '|';
				case SDL_SCANCODE_GRAVE:	return '~';
				default: break;
			}
		}
	}

	return '\0';
}
