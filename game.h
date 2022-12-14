#ifndef _game_h_
#define _game_h_
#include "utility.h"
#include "net.h"

#define GAME_VAR_STRING 0
#define GAME_VAR_INT 1
#define GAME_VAR_FLOAT 2
#define GAME_VAR_POINTER 3
typedef struct _DEDICATED_GAME_INFO
{
	char lvl[256];
	int dedicated;
	int enable_pvp;
	int xmasmode;
	int master_server;
	int advertise;
	int enable_cheats;
	char serv_name[32];
} DEDICATED_GAME_INFO;
typedef struct _GAME_VAR
{
	union
	{
		char string[UTIL_MAX_TEXT_WIDTH + 1];
		int integer;
		float decimal;
		void *pointer;
	} data;
	int data_type;
} GAME_VAR;
typedef enum _GAME_VARS
{
	GAME_VAR_CAMERA_X,
	GAME_VAR_CAMERA_Y,
	GAME_VAR_LEVEL,
	GAME_VAR_PLAYER,
	GAME_VAR_GRAVITY,
	GAME_VAR_SHOW_COLLISION_BOXES,
	GAME_VAR_SHOW_OBJGRID,
	GAME_VAR_SHOW_GRIDPOS,
	GAME_VAR_CURRENT_CONNECTING_IP,
	GAME_VAR_PLAYER_NAME,
	GAME_VAR_FULLSCREEN,
	GAME_VAR_HIRESMODE,
	GAME_VAR_CL_INTERP,
	GAME_VAR_PLAYER_SKIN,
	GAME_VAR_SHOWPOS,
	GAME_VAR_CL_SMOOTHING,
	GAME_VAR_CL_TIME_BETWEEN_UPDATES,
	GAME_VAR_CL_SHOW_NODEUUIDS,
	GAME_VAR_NOCLIP,
	GAME_VAR_INITIALIZED_AUDIO_VOLUME,
	GAME_VAR_NODOWNLOAD,
	GAME_VAR_ENABLE_SERVER_PVP,
	GAME_VAR_SV_CHEATS,
	GAME_VAR_XMAS_MODE,
	GAME_VAR_SUPERVIRUS,
	GAME_VAR_SERVER_NAME,
	GAME_VAR_ADVERTISE_SERVER,
	GAME_VAR_FANCY_LEVEL_NAME,
	GAME_VAR_MASTER_SERVER_ADDR,
	GAME_VAR_SHOW_BANDWIDTH,
	GAME_VAR_MAX
} GAME_VARS;

typedef void(*GAME_STATE_INIT)(void);
typedef void(*GAME_STATE_QUIT)(void);
typedef void(*GAME_STATE_UPDATE)(void);
typedef void(*GAME_STATE_DRAW)(void);

typedef enum _GAME_STATES
{
	GAME_STATE_NULL,
	GAME_STATE_BASE,
	GAME_STATE_BLOCKSEDIT,
	GAME_STATE_MAINMENU,
	GAME_STATE_BLOCKPROPSEDIT,
	GAME_STATE_OBJEDIT,
	GAME_STATE_SINGLEPLAYER,
	GAME_STATE_CLIENT,
	GAME_STATE_HOSTED_SERVER,
	GAME_STATE_DEDICATED_SERVER,
	GAME_STATE_CLIENT_CONNECTING,
	GAME_STATE_LIGHT_EDITOR,
	GAME_STATE_ENDTEXT_EDITOR,
	GAME_STATE_CLIENT_DOWNLOADING,
	GAME_STATE_BGEDIT,
	GAME_STATE_BITMAP_BENCH,
	GAME_STATE_MINIGAME1_TEST,
	GAME_STATE_MASTER_SERVER
} GAME_STATES;

void GameState_Base_Init(void);
void GameState_Base_Quit(void);
void GameState_BlocksEdit_Init(void);
void GameState_BlocksEdit_Quit(void);
void GameState_BlocksEdit_Update(void);
void GameState_BlocksEdit_Draw(void);
void GameState_MainMenu_Init(void);
void GameState_MainMenu_Quit(void);
void GameState_MainMenu_Update(void);
void GameState_MainMenu_Draw(void);
void GameState_BlockPropsEdit_Init(void);
void GameState_BlockPropsEdit_Quit(void);
void GameState_BlockPropsEdit_Update(void);
void GameState_BlockPropsEdit_Draw(void);
void GameState_ObjEdit_Init(void);
void GameState_ObjEdit_Quit(void);
void GameState_ObjEdit_Update(void);
void GameState_ObjEdit_Draw(void);
void GameState_Singleplayer_Init(void);
void GameState_Singleplayer_Quit(void);
void GameState_Singleplayer_Update(void);
void GameState_Singleplayer_Draw(void);
void GameState_Client_Init(void);
void GameState_Client_Quit(void);
void GameState_Client_Update(void);
void GameState_Client_Draw(void);
void GameState_HostedServer_Init(void);
void GameState_HostedServer_Quit(void);
void GameState_HostedServer_Update(void);
void GameState_HostedServer_Draw(void);
void GameState_DedicatedServer_Init(void);
void GameState_DedicatedServer_Quit(void);
void GameState_DedicatedServer_Update(void);
void GameState_DedicatedServer_Draw(void);
void GameState_ClientConnecting_Init(void);
void GameState_ClientConnecting_Quit(void);
void GameState_ClientConnecting_Update(void);
void GameState_ClientConnecting_Draw(void);
void GameState_LightEditor_Init(void);
void GameState_LightEditor_Quit(void);
void GameState_LightEditor_Update(void);
void GameState_LightEditor_Draw(void);
void GameState_EndTextEditor_Init(void);
void GameState_EndTextEditor_Quit(void);
void GameState_EndTextEditor_Update(void);
void GameState_EndTextEditor_Draw(void);
void GameState_ClientDownloading_Init(void);
void GameState_ClientDownloading_Quit(void);
void GameState_ClientDownloading_Update(void);
void GameState_ClientDownloading_Draw(void);
void GameState_BgEdit_Init(void);
void GameState_BgEdit_Quit(void);
void GameState_BgEdit_Update(void);
void GameState_BgEdit_Draw(void);
void GameState_BitmapBench_Init(void);
void GameState_BitmapBench_Quit(void);
void GameState_BitmapBench_Update(void);
void GameState_BitmapBench_Draw(void);
void GameState_MasterServer_Init(void);
void GameState_MasterServer_Quit(void);
void GameState_MasterServer_Update(void);

void Game_Init(const DEDICATED_GAME_INFO *gi);
void Game_Quit(void);
void Game_Stop(void);
int Game_DedicatedMode(void);
int Game_GetFrame(void);

void Game_SwitchState(int state_id);
void Game_PushState(int state_id);
int Game_PopState(void);
int Game_TopState(void);

const DEDICATED_GAME_INFO *Game_GetDedicatedGameInfo(void);
GAME_VAR *Game_GetVar(int index);

#endif