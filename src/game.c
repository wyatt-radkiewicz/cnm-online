#include <string.h>
#include <time.h>
#include <SDL.h>
#include "blocks.h"
#include "command.h"
#include "console.h"
#include "game.h"
#include "game_console.h"
#include "input.h"
#include "serial.h"
#include "renderer.h"
#include "utility.h"
#include "wobj.h"
#include "spawners.h"
#include "teleport_infos.h"
#include "net.h"
#include "netgame.h"
#include "audio.h"
#include "player_spawns.h"
#include "filesystem.h"
#include "interaction.h"
#include "background.h"
//#_include "gamelua.h"
#include "savedata.h"
#include "petdefs.h"
#include "player.h"
#include "ending_text.h"
#include "supervirus.h"
#include "mem.h"

#define GAME_MAX_STATES 8

typedef struct _GAME_STATE_TYPE
{
	GAME_STATE_INIT init;
	GAME_STATE_QUIT quit;
	GAME_STATE_UPDATE update;
	GAME_STATE_DRAW draw;
} GAME_STATE_TYPE;
static GAME_STATE_TYPE game_states[] =
{
	{NULL, NULL, NULL, NULL},
	{GameState_Base_Init, GameState_Base_Quit, NULL, NULL},
#ifdef _OLD
	{GameState_BlocksEdit_Init, GameState_BlocksEdit_Quit, GameState_BlocksEdit_Update, GameState_BlocksEdit_Draw},
#endif
	{GameState_MainMenu_Init, GameState_MainMenu_Quit, GameState_MainMenu_Update, GameState_MainMenu_Draw},
#ifdef _OLD
	{GameState_BlockPropsEdit_Init, GameState_BlockPropsEdit_Quit, GameState_BlockPropsEdit_Update, GameState_BlockPropsEdit_Draw},
	{GameState_ObjEdit_Init, GameState_ObjEdit_Quit, GameState_ObjEdit_Update, GameState_ObjEdit_Draw},
#endif
	{GameState_Singleplayer_Init, GameState_Singleplayer_Quit, GameState_Singleplayer_Update, GameState_Singleplayer_Draw},
	{GameState_Client_Init, GameState_Client_Quit, GameState_Client_Update, GameState_Client_Draw},
	{GameState_HostedServer_Init, GameState_HostedServer_Quit, GameState_HostedServer_Update, GameState_HostedServer_Draw},
	{GameState_DedicatedServer_Init, GameState_DedicatedServer_Quit, GameState_DedicatedServer_Update, GameState_DedicatedServer_Draw},
	{GameState_ClientConnecting_Init, GameState_ClientConnecting_Quit, GameState_ClientConnecting_Update, GameState_ClientConnecting_Draw},
#ifdef _OLD
	{GameState_LightEditor_Init, GameState_LightEditor_Quit, GameState_LightEditor_Update, GameState_LightEditor_Draw},
	{GameState_EndTextEditor_Init, GameState_EndTextEditor_Quit, GameState_EndTextEditor_Update, GameState_EndTextEditor_Draw},
	{GameState_ClientDownloading_Init, GameState_ClientDownloading_Quit, GameState_ClientDownloading_Update, GameState_ClientDownloading_Draw},
	{GameState_BgEdit_Init, GameState_BgEdit_Quit, GameState_BgEdit_Update, GameState_BgEdit_Draw},
	{GameState_ClientDownloading_Init, GameState_ClientDownloading_Quit, GameState_ClientDownloading_Update, GameState_ClientDownloading_Draw},
#endif
	{GameState_BitmapBench_Init, GameState_BitmapBench_Quit, GameState_BitmapBench_Update, GameState_BitmapBench_Draw},
	{GameState_MasterServer_Init, GameState_MasterServer_Quit, GameState_MasterServer_Update, NULL}
};
static GAME_VAR game_vars[GAME_VAR_MAX];

static int game_running;
static int game_dedicated = CNM_FALSE;
static int game_state_stack[GAME_MAX_STATES];
static int game_state_top;
static int game_frame;
static DEDICATED_GAME_INFO dgame_info;

static unsigned int game_state_changes[GAME_MAX_STATES];
static int game_state_changes_size;
static int game_last_state_top;

static void Game_ApplyStateChanges(void);
static void Game_InitGameVars(void);
static void AttachToConsole(void);

void Game_Init(const DEDICATED_GAME_INFO *gi)
{
	Uint32 start, end;

	game_running = CNM_TRUE;
	game_state_changes_size = 0;
	game_dedicated = gi->dedicated;
	memcpy(&dgame_info, gi, sizeof(dgame_info));
	if (dgame_info.dedicated || dgame_info.master_server)
		AttachToConsole();
	game_state_top = -1;
	game_last_state_top = -1;
	game_frame = 0;
	
	memset(game_vars, 0, sizeof(game_vars));
	Game_PushState(GAME_STATE_BASE);
	while (game_running)
	{
		start = SDL_GetTicks();
		Game_ApplyStateChanges();
		if (game_states[Game_TopState()].update != NULL && game_state_top != -1)
			game_states[Game_TopState()].update();
		if (game_states[Game_TopState()].draw != NULL && game_state_top != -1)
			game_states[Game_TopState()].draw();
		game_frame++;
		end = SDL_GetTicks();
		if (end - start < 33)
			SDL_Delay(33 - (end - start));
	}
}
void Game_Quit(void)
{
	while (game_state_top >= 0)
	{
		if (game_states[game_state_top].quit != NULL)
			game_states[game_state_top].quit();
		game_state_top--;
	}
}
void Game_Stop(void)
{
	game_running = CNM_FALSE;
}
int Game_DedicatedMode(void)
{
	return game_dedicated;
}
int Game_GetFrame(void)
{
	return game_frame;
}
void Game_ResetFrame(void) {
	game_frame = 0;
}

static void Game_ApplyStateChanges(void)
{
	int i, state_id, calls_size = 0;
	void *calls[GAME_MAX_STATES];
	int call_types[GAME_MAX_STATES];
	if (game_state_changes_size == 0)
		return;

	for (i = 0; i < game_state_changes_size; i++)
	{
		state_id = game_state_changes[i];

		if (state_id)
		{
			/* Push state */
			game_state_stack[++game_last_state_top] = state_id;
			if (game_states[game_state_stack[game_last_state_top]].init != NULL)
			{
				calls[calls_size] = game_states[game_state_stack[game_last_state_top]].init;
				call_types[calls_size++] = 1;
			}
		}
		else
		{
			/* Pop state */
			if (game_states[game_state_stack[game_last_state_top]].quit != NULL)
			{
				calls[calls_size] = game_states[game_state_stack[game_last_state_top]].quit;
				call_types[calls_size++] = 0;
			}
			game_last_state_top--;
		}
	}
	game_last_state_top = game_state_top;
	game_state_changes_size = 0;

	for (i = 0; i < calls_size; i++)
	{
		if (call_types[i])
			((GAME_STATE_INIT)calls[i])();
		else
			((GAME_STATE_QUIT)calls[i])();
	}
}
void Game_SwitchState(int state_id)
{
	Game_PopState();
	Game_PushState(state_id);
}
void Game_PushState(int state_id)
{
	if (state_id <= GAME_STATE_NULL)
		return;

	if ((game_state_top + 1 < GAME_MAX_STATES && game_state_stack[game_state_top] != state_id) || game_state_top == -1)
	{
		game_state_changes[game_state_changes_size++] = state_id;
		game_state_top++;
	}
}
int Game_PopState(void)
{
	if (game_state_top > -1)
	{
		game_state_changes[game_state_changes_size++] = 0;
		game_state_top--;
	}
	return game_state_top != -1;
}
int Game_TopState(void)
{
	return game_state_stack[game_state_top];
}
const DEDICATED_GAME_INFO *Game_GetDedicatedGameInfo(void) {
	return &dgame_info;
}
GAME_VAR *Game_GetVar(int index)
{
	return game_vars + index;
}

static void Game_InitGameVars(void)
{
	game_vars[GAME_VAR_SHOW_COLLISION_BOXES].data.integer = 0;
	game_vars[GAME_VAR_SHOW_OBJGRID].data.integer = 0;
	game_vars[GAME_VAR_SHOW_GRIDPOS].data.integer = 0;
	game_vars[GAME_VAR_PLAYER_NAME].data.string[0] = '\0';
	strcpy(game_vars[GAME_VAR_SERVER_NAME].data.string, dgame_info.serv_name);
	game_vars[GAME_VAR_CL_INTERP].data.integer = CNM_TRUE;
	game_vars[GAME_VAR_PLAYER_SKIN].data.integer = 0;
	game_vars[GAME_VAR_SHOWPOS].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_CL_SMOOTHING].data.decimal = 1.0f;
	game_vars[GAME_VAR_CL_TIME_BETWEEN_UPDATES].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_CL_SHOW_NODEUUIDS].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_NOCLIP].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_NODOWNLOAD].data.integer = CNM_TRUE;
	game_vars[GAME_VAR_ENABLE_SERVER_PVP].data.integer = 0;
#ifdef DEBUG
	game_vars[GAME_VAR_SV_CHEATS].data.integer = CNM_TRUE;
#else
	game_vars[GAME_VAR_SV_CHEATS].data.integer = dgame_info.enable_cheats;
#endif
	game_vars[GAME_VAR_SUPERVIRUS].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_SHOW_BANDWIDTH].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_ADVERTISE_SERVER].data.integer = dgame_info.advertise;
	game_vars[GAME_VAR_FANCY_LEVEL_NAME].data.string[0] = '\0';
	game_vars[GAME_VAR_LEVEL_TIMER].data.integer = 0;
	game_vars[GAME_VAR_SAVE_SLOT].data.integer = 0;
	game_vars[GAME_VAR_LEVEL_SELECT_MODE].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_NOSAVE].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_FORCE_NOSAVE].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_CL_POS].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_GOD].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_WIDESCREEN].data.integer = CNM_FALSE;
	game_vars[GAME_VAR_MEM_STATUS].data.integer = CNM_FALSE;

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	game_vars[GAME_VAR_XMAS_MODE].data.integer = CNM_FALSE;
	if (tm->tm_mon == 11 || dgame_info.xmasmode) {
		// Its december!
		game_vars[GAME_VAR_XMAS_MODE].data.integer = CNM_TRUE;
	}
}

void GameState_Base_Init(void)
{
	arena_init("_BASE_STATE");
	Console_Init("cnm_log.txt");
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0)
	{
		Console_Print("Could not init SDL2!!!!");
		game_running = CNM_FALSE;
		return;
	}

	Game_InitGameVars();
	savedata_sys_init();
	FileSystem_Init();
	//Blocks_SetWorldSize(512, 256);
	//Blocks_GenBlockProps(1024);
	EndingText_Init();
	TeleportInfos_Init();
	Spawners_Init();
	Wobj_Init();
	PlayerSpawns_Init();
	Net_Init();
	NetGame_Init();
	Interaction_Init();
	//Console_Print("WELCOME TO CNM ONLINE!");
	Console_Print("WELCOME TO CNM ONLINE "CNM_VERSION_STRING"!");
	Serial_LoadConfig();
	petdef_sys_init();
	player_sys_init();
	supervirus_sys_init();
	if (!dgame_info.dedicated && !dgame_info.master_server)
	{
		Renderer_Init(Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer,
					  Game_GetVar(GAME_VAR_HIRESMODE)->data.integer,
					  Game_GetVar(GAME_VAR_WIDESCREEN)->data.integer);
		Renderer_LoadBitmap("gfx.bmp");
		Renderer_BuildTables();
		Renderer_SetFont(384, 448, 8, 8);
		
		Input_Init();
		Input_PushState(INPUT_STATE_PLAYING);
		Input_SetQuitCallback(Game_Stop);
		GameConsole_Init();
		
		Audio_Init();
		Audio_SetGlobalVolume((float)Game_GetVar(GAME_VAR_INITIALIZED_AUDIO_VOLUME)->data.integer / 100.0f);
	}
	Background_ResetBackgrounds();
	//Gui_Init();
	FileSystem_RegisterGfx("gfx.bmp");
	FileSystem_SearchForLevels(CNM_FALSE);
	Serial_LoadAudioCfg("audio.cnma");
	g_current_save = 0;
	globalsave_clear(&g_globalsave);
	globalsave_load(&g_globalsave);
	for (int i = 0; i < SAVE_SLOTS; i++) {
		new_save(g_saves + i);
		load_game(i, g_saves + i);
	}
	
	strcpy(game_vars[GAME_VAR_LEVEL].data.string, dgame_info.lvl);
	game_vars[GAME_VAR_ENABLE_SERVER_PVP].data.integer = dgame_info.enable_pvp;
	if (dgame_info.master_server) {
		Game_PushState(GAME_STATE_MASTER_SERVER);
		return;
	}
	if (!dgame_info.dedicated) {
		if (dgame_info.warp) {
			Game_GetVar(GAME_VAR_NOSAVE)->data.integer = CNM_TRUE;
			Game_GetVar(GAME_VAR_FORCE_NOSAVE)->data.integer = CNM_TRUE;
			if (Filesystem_GetLevelIdFromFileName(dgame_info.lvl) != -1) {
				int id = Filesystem_GetLevelIdFromFileName(dgame_info.lvl);
				strcpy(Game_GetVar(GAME_VAR_LEVEL)->data.string, FileSystem_GetLevel(id));
				Game_GetVar(GAME_VAR_PAR_SCORE)->data.integer = FileSystem_GetLevelParScore(id);
				Game_PushState(GAME_STATE_SINGLEPLAYER);
			} else {
				Game_PushState(GAME_STATE_MAINMENU);
			}
		} else {
			Game_PushState(GAME_STATE_MAINMENU);
		}
	}
	else {
		strcpy(game_vars[GAME_VAR_PLAYER_NAME].data.string, "DEDICATEDSERVER");
		game_vars[GAME_VAR_PLAYER_SKIN].data.integer = 0;
		Game_PushState(GAME_STATE_HOSTED_SERVER);
	}
}
void GameState_Base_Quit(void)
{
	//AutorunLua_ClearPrgm();
	NetGame_Quit();
	NetTcp_ClosePort();
	Net_Quit();
	Wobj_Quit();
	Spawners_Quit();
	Blocks_Unload();
	//Gui_Reset();
	Audio_Quit();
	Input_Quit();
	Renderer_Quit();
	SDL_Quit();
	Console_Quit();
	arena_deinit();
}


#ifdef _WIN32

#include <Windows.h>
#include <io.h>
#include <fcntl.h>

static void AttachToConsole(void)
{
	if (AttachConsole(ATTACH_PARENT_PROCESS) == 0)
		if (AllocConsole() == 0)
			return;

	HANDLE console_output = GetStdHandle(STD_OUTPUT_HANDLE);
	int system_output = _open_osfhandle((intptr_t)console_output, _O_TEXT);
	if (_isatty(system_output) == 0)
		return;
	FILE *coutput_handle = _fdopen(system_output, "w");
	
	HANDLE console_error = GetStdHandle(STD_ERROR_HANDLE);
	int system_error = _open_osfhandle((intptr_t)console_error, _O_TEXT);
	FILE *cerror_handle = _fdopen(system_error, "w");

	HANDLE console_input = GetStdHandle(STD_INPUT_HANDLE);
	int system_input = _open_osfhandle((intptr_t)console_input, _O_TEXT);
	FILE *cinput_handle = _fdopen(system_input, "r");
	
	clearerr(stdout);
	clearerr(stderr);
	clearerr(stdin);

	printf("Attached To Console!\n");
}

#else

static void AttachToConsole(void) {
	printf("Null: can't attach to new console window...\n");
}

#endif
