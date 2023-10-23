#include <string.h>
#include <stdio.h>
#include <math.h>
#include "console.h"
#include "renderer.h"
#include "blocks.h"
#include "serial.h"
#include "game_console.h"
#include "spawners.h"
#include "input.h"
#include "game.h"
#include "gui.h"
#include "utility.h"
#include "command.h"
#include "wobj.h"
#include "audio.h"
#include "filesystem.h"
#include "master_server.h"
#include "fadeout.h"
#include "background.h"

#define SB_START 4

extern int skin_bases[10][2];
//static CNM_RECT clouds;
static CNM_RECT title_card;
static CNM_RECT player;
static CNM_RECT wings[4];
//static int cloudsx[2];
static float pos[2];
static float vel[2];
static int controlling;
static int ai_xdir;
static int ai_ydir;
static int ai_timer;
static int ai_same_speed;
static int editor_cheat = CNM_FALSE;

static float _camx = 0.0f, _camy = 0.0f, _camx_coarse = 0.0f, _camy_coarse = 0.0f, _camx_spd = 0.0f, _camy_spd = 0.0f;
static float *_camx_list, *_camy_list;
static int _cam_list_len, _next_cam_target;
#define MAX_TITLE_CARDS 4
static int _title_card_ticker, _title_card_y[MAX_TITLE_CARDS];

static const char *mission_difs[] =
{
	"*_________ - TUTORIAL",
	"**________ - REALLY EASY",
	"***_______ - EASY",
	"****______ - NORMAL",
	"*****_____ - KINDA HARD",
	"******____ - HARD",
	"*******___ - ULTRA",
	"********__ - EXTREME",
	"*********_ - DEATH!!!",
	"********** - ULTRA DEATH!"
};
static GUI_FRAME *options, *root, *startnewgame, *joingame_ip, *hostgame, *playersetup, *serverbrowser;

#define NUM_SNOWFLAKES 512
typedef struct _XMAS_SNOWFLAKE
{
	short x, y;
	char xdir, alive;
} XMAS_SNOWFLAKE;
static unsigned char xmas_static_snow[RENDERER_HEIGHT][RENDERER_WIDTH];
static unsigned char xmas_static_snow_colors[RENDERER_HEIGHT][RENDERER_WIDTH];
static unsigned char xmas_obstacles[RENDERER_HEIGHT][RENDERER_WIDTH];
static XMAS_SNOWFLAKE xmas_snowflakes[NUM_SNOWFLAKES];
static int next_snowflake;

static void PopulateGuiLevelSelector(GUI_ELEMENT *elem, int index);

static void CyaManButton(GUI_ELEMENT *elem, int index)
{
	Game_Stop();
}
static void FullscreenGuiFunc(GUI_ELEMENT *elem, int index)
{
	Renderer_SetFullscreen(elem->props.set_index);
	Serial_SaveConfig();
}
static void HiresGuiFunc(GUI_ELEMENT *elem, int index)
{
	Renderer_SetHiResMode(elem->props.set_index);
	Serial_SaveConfig();
}
static void ChangeMainMenuSpecial(GUI_ELEMENT *elem, int index)
{
	editor_cheat = (strcmp(elem->props.set[elem->props.set_index], "ENABLED") == 0);

	if (editor_cheat)
		Gui_GetRoot()->num_elements = 13;
	else
		Gui_GetRoot()->num_elements = 7;
}
static void BlockEditorButton(GUI_ELEMENT *elem, int index)
{
	Command_Execute("start_blockedit");
}
static void LightEditorButton(GUI_ELEMENT *elem, int index)
{
	Command_Execute("start_lightedit");
}
static void BlockPropsEditorButton(GUI_ELEMENT *elem, int index)
{
	Game_SwitchState(GAME_STATE_BLOCKPROPSEDIT);
}
static void ObjectEditorButton(GUI_ELEMENT *elem, int index)
{
	Game_SwitchState(GAME_STATE_OBJEDIT);
}
static void EndingTextEditorButton(GUI_ELEMENT *elem, int index)
{
	Game_SwitchState(GAME_STATE_ENDTEXT_EDITOR);
}
static void BackgroundEditorButton(GUI_ELEMENT *elem, int index)
{
	Game_SwitchState(GAME_STATE_BGEDIT);
}
static void StartNewGame(GUI_ELEMENT *elem, int index)
{
	int id = FileSystem_GetLevelFromLevelOrder(startnewgame->elements[9].props.set_index);
	strcpy(Game_GetVar(GAME_VAR_LEVEL)->data.string, FileSystem_GetLevel(id));
	Game_GetVar(GAME_VAR_PAR_SCORE)->data.integer = FileSystem_GetLevelParScore(id);
	Game_SwitchState(GAME_STATE_SINGLEPLAYER);
}
static void JoinGameIpCallback(GUI_ELEMENT *elem, int index)
{
	char buffer[64] = {'\0'};
	strcpy(buffer, "connect ");
	strcat(buffer, elem->frame->elements[index - 1].props.string);
	Console_Print(buffer);
	Command_Execute(buffer);
}
static void HostGameCallback(GUI_ELEMENT *elem, int index)
{
	strcpy(Game_GetVar(GAME_VAR_LEVEL)->data.string, FileSystem_GetLevel(FileSystem_GetLevelFromLevelOrder(hostgame->elements[9].props.set_index)));
	Game_SwitchState(GAME_STATE_HOSTED_SERVER);
}
static void ChangePlayerName(GUI_ELEMENT *elem, int index)
{
	strcpy(Game_GetVar(GAME_VAR_PLAYER_NAME)->data.string, elem->props.string);
	Serial_SaveConfig();
}
static void IpAddrCallback(GUI_ELEMENT *elem, int index)
{
	strcpy(Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string, elem->props.string);
	Serial_SaveConfig();
}
static void PlayerSkinCallback(GUI_ELEMENT *elem, int index)
{
	GUI_ELEMENT *bitmap = elem->frame->elements + index + 1;
	int skinno = Gui_GetNumberElementInt(elem->frame, index);
	Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer = skinno;
	memcpy(&bitmap->props.bitmap, &wobj_types[WOBJ_PLAYER].frames[skinno], sizeof(CNM_RECT));
	memcpy(&bitmap->props.bitmap, skin_bases[skinno], sizeof(int)*2);
	bitmap->props.bitmap.w = 32;
	bitmap->props.bitmap.h = 32;
	Serial_SaveConfig();
}
static void PopulateGuiLevelSelector(GUI_ELEMENT *elem, int index)
{
	int i = 0;
	for (i = 0; i < FileSystem_NumLevels(); i++)
	{
		Gui_AddItemToSet(elem->frame, index, FileSystem_GetLevelName(FileSystem_GetLevelFromLevelOrder(i)));
	}
}
static void HostgameSwitchLevel(GUI_ELEMENT *elem, int index)
{
	memcpy(&hostgame->elements[1].props.bitmap, FileSystem_GetLevelPreview(FileSystem_GetLevelFromLevelOrder(elem->props.set_index)), sizeof(CNM_RECT));
}
static void SingleplayerSwitchLevel(GUI_ELEMENT *elem, int index)
{
	char mission_num[16];
	sprintf(mission_num, "MISSION: %d", elem->props.set_index + 1);
	memcpy(&startnewgame->elements[1].props.bitmap, FileSystem_GetLevelPreview(FileSystem_GetLevelFromLevelOrder(elem->props.set_index)), sizeof(CNM_RECT));
	strcpy(startnewgame->elements[10].name, mission_num);
	char mission_dif[64];
	sprintf(mission_dif, "DIFFICULTY %s", mission_difs[FileSystem_GetLevelDifficulty(FileSystem_GetLevelFromLevelOrder(elem->props.set_index))]);
	strcpy(startnewgame->elements[11].name, mission_dif);
}
static void ChangeGameVolume(GUI_ELEMENT *elem, int index)
{
	char cmd[100];
	sprintf(cmd, "volume %d", Gui_GetNumberElementInt(elem->frame, index));
	Command_Execute(cmd);
	Serial_SaveConfig();
	//Audio_SetGlobalVolume();
}
static void NoDownloadButtonCallback(GUI_ELEMENT *elem, int index) {
	Game_GetVar(GAME_VAR_NODOWNLOAD)->data.integer = elem->props.set_index;//options->elements[4].props.set_index;
}
static void HostgameChangePvpMode(GUI_ELEMENT *elem, int index) {
	Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer = elem->props.set_index;
}
static void HostGameChangeServerName(GUI_ELEMENT *elem, int index) {
	char *sname = Game_GetVar(GAME_VAR_SERVER_NAME)->data.string;
	if (strlen(elem->props.string) < 3)
		strcpy(sname, "NAME TOO SMALL");
	else
		strcpy(sname, elem->props.string);
}
static void HostGameChangeAdvertisingMode(GUI_ELEMENT *elem, int index) {
	Game_GetVar(GAME_VAR_ADVERTISE_SERVER)->data.integer = elem->props.set_index;
}
static void MasterServerAddrCallback(GUI_ELEMENT *elem, int index) {
	strcpy(Game_GetVar(GAME_VAR_MASTER_SERVER_ADDR)->data.string, elem->props.string);
	Serial_SaveConfig();
}

MSPAGE_DATA msdata;
static int refresh_cooldown;

static void ServerBrowserLoadPage(int pageid)
{
	msdata.page = pageid;
	memset(msdata.servers, 0, sizeof(msdata.servers));
	refresh_cooldown = 30 * 3;
	int i;

	for (i = 0; i < MSPAGE_SIZE; i++)
	{
		msdata.servers[i].num_players = -1;
		strcpy(serverbrowser->elements[SB_START + i * 4 + 0].props.string, "NAME:");
		strcpy(serverbrowser->elements[SB_START + i * 4 + 1].props.string, "PLAYERS:");
		strcpy(serverbrowser->elements[SB_START + i * 4 + 2].props.string, "MAP:");
		serverbrowser->elements[SB_START + i * 4 + 0].active = CNM_FALSE;
		serverbrowser->elements[SB_START + i * 4 + 1].active = CNM_FALSE;
		serverbrowser->elements[SB_START + i * 4 + 2].active = CNM_FALSE;
		serverbrowser->elements[SB_START + i * 4 + 3].active = CNM_FALSE;
	}
	serverbrowser->elements[MSPAGE_SIZE * 4 + SB_START + 1].active = CNM_FALSE;
	serverbrowser->elements[MSPAGE_SIZE * 4 + SB_START + 2].active = CNM_FALSE;

	MSPAGE_REQUEST req;
	NET_ADDR msaddr;
	req.page = pageid;
	req.sort = 0;
	msaddr = Net_GetIpFromString(Game_GetVar(GAME_VAR_MASTER_SERVER_ADDR)->data.string);
	msaddr.port = Net_HostToNetU16(NET_MSERVER_PORT);
	NET_PACKET *packet = Net_CreatePacket(NET_MASTER_SERVER_PAGE_REQUEST, 0, &msaddr, sizeof(req), &req);
	Net_Send(packet);
}
static void ServerBrowserButtonCallback(GUI_ELEMENT *elem, int index)
{
	msdata.num_pages = 0;
	ServerBrowserLoadPage(0);
	sprintf(serverbrowser->elements[1].name, "PAGE: %d/%d", 0, 0);
}
static void ServerBrowserRefreshCallback(GUI_ELEMENT *elem, int index)
{
	if (refresh_cooldown < 0) {
		ServerBrowserLoadPage(msdata.page);
	}
}
static void NextPageCallback(GUI_ELEMENT *elem, int index)
{
	if (!elem->active) return;
	if (refresh_cooldown < 0 && msdata.page + 1 < msdata.num_pages)
	{
		ServerBrowserLoadPage(msdata.page+1);
		serverbrowser->active_index = 2;
		serverbrowser->cam_index = 0;
	}
}
static void LastPageCallback(GUI_ELEMENT *elem, int index)
{
	if (!elem->active) return;
	if (refresh_cooldown < 0 && msdata.page > 0)
	{
		ServerBrowserLoadPage(msdata.page-1);
		serverbrowser->active_index = 2;
		serverbrowser->cam_index = 0;
	}
}
static void JoinGameBrowserCallback(GUI_ELEMENT *elem, int index)
{
	if (!elem->active) return;
	
	char buffer[64] = {'\0'};
	strcpy(buffer, "connect ");
	strcat(buffer, Net_GetStringFromIp(&msdata.servers[elem->custom_hint].addr));
	Console_Print(buffer);
	Command_Execute(buffer);
}

static void MainMenu_OnPacket(NET_PACKET *packet);

void GameState_MainMenu_Init(void)
{
	GUI_FRAME_PROPS props;

	FileSystem_SearchForLevels(CNM_TRUE);
	Fadeout_Setup();
	Fadeout_FadeFromWhite(10, 25);

	//Util_SetRect(&title_card, 320, 832, 192, 32);
	Util_SetRect(&title_card, 0, 7104, 304, 96);
	//Util_SetRect(&clouds, 0, 832, 320, 240);
	//Util_SetRect(&clouds, 0, 7200, 256, 320);
	//cloudsx[0] = 0;
	//cloudsx[1] = 320;
	pos[0] = 120.0f;
	pos[1] = 96.0f;
	vel[0] = 10.0f;
	vel[1] = -2.0f;
	Util_SetRect(&player, 0, 288, 32, 32);
	Util_SetRect(wings + 0, 256, 3088, 48, 48);
	Util_SetRect(wings + 1, 256+48, 3088, 48, 48);
	Util_SetRect(wings + 2, 0,  7056, 48, 48);
	Util_SetRect(wings + 3, 48, 7056, 48, 48);
	controlling = 0;
	Util_RandSetSeed(0);
	ai_xdir = Util_RandInt(0, 1) * 2 - 1;
	ai_ydir = Util_RandInt(0, 1) * 2 - 1;
	ai_timer = Util_RandInt(5, 60);
	ai_same_speed = 0;

	/* Set up the GUI */
	props.top = RENDERER_HEIGHT - 8*6 - 16;
	props.line_count = 6;
	props.align[0] = GUI_ALIGN_CENTER;
	props.bounds[0] = RENDERER_WIDTH / 2;
	props.bounds[1] = props.bounds[0];
	root = Gui_CreateFrame(7, 20, NULL, &props, 0);
	if (editor_cheat)
		root->num_elements = 13;
	props.align[0] = GUI_ALIGN_LEFT;
	props.bounds[0] = RENDERER_WIDTH / 16;
	props.align[1] = GUI_ALIGN_RIGHT;
	props.bounds[1] = RENDERER_WIDTH / 16 * 15;
	options = Gui_CreateFrame(6, 64, root, &props, 0);
	props.top = RENDERER_HEIGHT - 8 * 6 - 96;
	props.line_count = 16;
	startnewgame = Gui_CreateFrame(13, 20, root, &props, 0);
	joingame_ip = Gui_CreateFrame(3, 20, root, &props, 0);
	hostgame = Gui_CreateFrame(14, 20, root, &props, 0);
	playersetup = Gui_CreateFrame(3, 20, root, &props, 0);
	serverbrowser = Gui_CreateFrame(SB_START+MSPAGE_SIZE*4+3, 100, root, &props, 0);
	Gui_InitSetElement(options, 0, FullscreenGuiFunc, "FULLSCREEN");
	Gui_AddItemToSet(options, 0, "NO");
	Gui_AddItemToSet(options, 0, "YES");
	options->elements[0].props.set_index = Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer;
	//Gui_InitStringElement(options, 1, NULL, "PLAYER NAME: ", 15);
	//Gui_InitFloatElement(options, 2, NULL, "NUMBER FIELD TEST: ", -1.5f, 3.14f, 0);
	Gui_InitSetElement(options, 3, ChangeMainMenuSpecial, "LEVEL EDIT CHEAT: ");
	Gui_AddItemToSet(options, 3, "DISABLED");
	Gui_AddItemToSet(options, 3, "ENABLED");
	Gui_InitSetElement(options, 1, HiresGuiFunc, "HIRES MODE");
	Gui_AddItemToSet(options, 1, "NO");
	Gui_AddItemToSet(options, 1, "YES");
	Gui_InitNumberElement(options, 2, ChangeGameVolume, "MASTER VOLUME: ", 0, 100, (int)(Audio_GetGlobalVolume() * 100.0f));
	options->elements[0].props.set_index = Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer;
	options->elements[1].props.set_index = Game_GetVar(GAME_VAR_HIRESMODE)->data.integer;
	options->elements[3].props.set_index = editor_cheat;
	Gui_InitSetElement(options, 4, NoDownloadButtonCallback, "DOWNLOAD SERVER FILES: ");
	Gui_AddItemToSet(options, 4, "YES");
	Gui_AddItemToSet(options, 4, "NO");
	options->elements[4].props.set_index = Game_GetVar(GAME_VAR_NODOWNLOAD)->data.integer;
	Gui_InitStringElement(options, 5, MasterServerAddrCallback, "MASTER SERVER ADDRESS: ", 18);
	strcpy(options->elements[5].props.string, Game_GetVar(GAME_VAR_MASTER_SERVER_ADDR)->data.string);

	Gui_InitHeaderElement(joingame_ip, 0, "JOIN A GAME (SPECIFY IP ADDRESS)");
	Gui_InitStringElement(joingame_ip, 1, IpAddrCallback, "IP ADDRESS: ", 18);
	strcpy(joingame_ip->elements[1].props.string, Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string);
	Gui_InitButtonElement(joingame_ip, 2, JoinGameIpCallback, "CONNECT TO THIS ADDRESS!!!", NULL, CNM_FALSE);

	Gui_InitHeaderElement(hostgame, 0, "HOST A MULTIPLAYER GAME");
	Gui_InitBitmapElement(hostgame, 1, 64, 0, 1376, 96, 64);
	memcpy(&hostgame->elements[1].props.bitmap, FileSystem_GetLevelPreview(FileSystem_GetLevelFromLevelOrder(0)), sizeof(CNM_RECT));
	Gui_InitNullElement(hostgame, 2); // 1
	Gui_InitNullElement(hostgame, 3); // 2
	Gui_InitNullElement(hostgame, 4); // 3
	Gui_InitNullElement(hostgame, 5); // 4
	Gui_InitNullElement(hostgame, 6); // 5
	Gui_InitNullElement(hostgame, 7); // 6
	Gui_InitNullElement(hostgame, 8); // 7
	Gui_InitSetElement(hostgame, 9, HostgameSwitchLevel, "WORLD MAP: ");
	PopulateGuiLevelSelector(&hostgame->elements[9], 9);
	Gui_InitSetElement(hostgame, 10, HostgameChangePvpMode, "ENABLE PVP: ");
	Gui_AddItemToSet(hostgame, 10, "NO");
	Gui_AddItemToSet(hostgame, 10, "YES");
	hostgame->elements[10].props.set_index = Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer;
	Gui_InitStringElement(hostgame, 11, HostGameChangeServerName, "SERVER NAME: ", 31);
	strcpy(hostgame->elements[11].props.string, Game_GetVar(GAME_VAR_SERVER_NAME)->data.string);
	Gui_InitSetElement(hostgame, 12, HostGameChangeAdvertisingMode, "ADVERTISE SERVER: ");
	Gui_AddItemToSet(hostgame, 12, "NO");
	Gui_AddItemToSet(hostgame, 12, "YES");
	hostgame->elements[12].props.set_index = Game_GetVar(GAME_VAR_ADVERTISE_SERVER)->data.integer;
	Gui_InitButtonElement(hostgame, 13, HostGameCallback, "HOST GAME!!!", NULL, CNM_FALSE);

	Gui_InitHeaderElement(startnewgame, 0, "START A SINGLEPLAYER GAME");
	Gui_InitBitmapElement(startnewgame, 1, 64, 0, 1376, 96, 64);
	memcpy(&startnewgame->elements[1].props.bitmap, FileSystem_GetLevelPreview(FileSystem_GetLevelFromLevelOrder(0)), sizeof(CNM_RECT));
	Gui_InitNullElement(startnewgame, 2); // 1
	Gui_InitNullElement(startnewgame, 3); // 2
	Gui_InitNullElement(startnewgame, 4); // 3
	Gui_InitNullElement(startnewgame, 5); // 4
	Gui_InitNullElement(startnewgame, 6); // 5
	Gui_InitNullElement(startnewgame, 7); // 6
	Gui_InitNullElement(startnewgame, 8); // 7
	Gui_InitSetElement(startnewgame, 9, SingleplayerSwitchLevel, "WORLD MAP: ");
	PopulateGuiLevelSelector(&startnewgame->elements[9], 9);
	Gui_InitHeaderElement(startnewgame, 10, "MISSION: 1");
	char mission_dif[64];
	sprintf(mission_dif, "DIFFICULTY %s", mission_difs[FileSystem_GetLevelDifficulty(FileSystem_GetLevelFromLevelOrder(0))]);
	Gui_InitHeaderElement(startnewgame, 11, mission_dif);
	Gui_InitButtonElement(startnewgame, 12, StartNewGame, "*START!!!*", NULL, CNM_FALSE);

	Gui_InitStringElement(playersetup, 0, ChangePlayerName, "PLAYER NAME: ", 15);
	strcpy(playersetup->elements[0].props.string, Game_GetVar(GAME_VAR_PLAYER_NAME)->data.string);
	playersetup->elements[0].props.allow_spaces = CNM_FALSE;
	Gui_InitNumberElement(playersetup, 1, PlayerSkinCallback, "PLAYER SKIN: ", 0, 9, 0);
	Gui_InitBitmapElement(playersetup, 2, 64, 0, 0, 32, 32);
	int skinno = Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer;
	Gui_SetNumberElementInt(playersetup, 1, skinno);
	memcpy(&playersetup->elements[2].props.bitmap, &wobj_types[WOBJ_PLAYER].frames[skinno], sizeof(CNM_RECT));
	memcpy(&playersetup->elements[2].props.bitmap, skin_bases[skinno], sizeof(CNM_RECT) / 2);
	playersetup->elements[2].props.bitmap.w = 32;
	playersetup->elements[2].props.bitmap.h = 32;

	refresh_cooldown = 0;
	Gui_InitHeaderElement(serverbrowser, 0, "-- SERVER BROWSER --");
	Gui_InitHeaderElement(serverbrowser, 1, "PAGE: 0/0");
	Gui_InitButtonElement(serverbrowser, 2, ServerBrowserRefreshCallback, "<REFRESH PAGE>", NULL, CNM_FALSE);
	Gui_InitNullElement(serverbrowser, 3);
	int i, x;
	x = 0;
	for (i = SB_START; i < MSPAGE_SIZE*4+ SB_START; i += 4)
	{
		Gui_InitHeaderElement(serverbrowser, i, "NAME: ");
		serverbrowser->elements[i].active = CNM_FALSE;
		Gui_InitHeaderElement(serverbrowser, i+1, "PLAYERS: ");
		serverbrowser->elements[i+1].active = CNM_FALSE;
		Gui_InitHeaderElement(serverbrowser, i+2, "MAP: ");
		serverbrowser->elements[i+2].active = CNM_FALSE;
		Gui_InitButtonElement(serverbrowser, i+3, JoinGameBrowserCallback, "-- JOIN SERVER --", NULL, CNM_FALSE);
		serverbrowser->elements[i+3].custom_hint = x;
		serverbrowser->elements[i+3].active = CNM_FALSE;
		x++;
	}
	Gui_InitNullElement(serverbrowser, i++);
	Gui_InitButtonElement(serverbrowser, i++, NextPageCallback, "NEXT PAGE", NULL, CNM_FALSE);
	Gui_InitButtonElement(serverbrowser, i, LastPageCallback, "LAST PAGE", NULL, CNM_FALSE);
	serverbrowser->elements[i - 1].active = CNM_FALSE;
	serverbrowser->elements[i].active = CNM_FALSE;

	Gui_SetRoot(root);
	Gui_InitButtonElement(root, 0, NULL, "START NEW GAME", startnewgame, CNM_FALSE);
	Gui_InitButtonElement(root, 1, NULL, "HOST NEW GAME", hostgame, CNM_FALSE);
	Gui_InitButtonElement(root, 2, NULL, "JOIN GAME (SPECIFY IP)", joingame_ip, CNM_FALSE);
	Gui_InitButtonElement(root, 3, ServerBrowserButtonCallback, "SERVER BROWSER", serverbrowser, CNM_FALSE);
	Gui_InitButtonElement(root, 4, NULL, "OPTIONS", options, CNM_FALSE);
	Gui_InitButtonElement(root, 5, NULL, "PLAYER SETUP", playersetup, CNM_FALSE);
	Gui_InitButtonElement(root, 6, CyaManButton, "SEE YA MAN...", NULL, CNM_FALSE);
	Gui_InitButtonElement(root, 7, BlockEditorButton, "BLOCK EDITOR", NULL, CNM_FALSE);
	Gui_InitButtonElement(root, 8, BlockPropsEditorButton, "BLOCK PROPERTIES EDITOR", NULL, CNM_FALSE);
	Gui_InitButtonElement(root, 9, ObjectEditorButton, "OBJECT PLACEMENT EDITOR", NULL, CNM_FALSE);
	Gui_InitButtonElement(root, 10, LightEditorButton, "LIGHT EDITOR", NULL, CNM_FALSE);
	Gui_InitButtonElement(root, 11, EndingTextEditorButton, "ENDING TEXT EDITOR", NULL, CNM_FALSE);
	Gui_InitButtonElement(root, 12, BackgroundEditorButton, "BACKGROUND EDITOR", NULL, CNM_FALSE);
	Gui_Focus();

	Net_AddPollingFunc(MainMenu_OnPacket);

	Audio_PlayMusic(2, CNM_FALSE);
	
	int num_title_levels = Filesystem_GetNumTitleLevels();
	if (!num_title_levels) {
		Console_Print("No title background levels!");
		Game_PopState();
		Game_Stop();
		return;
	}
	int title_level = Util_RandInt(0, num_title_levels);
	char pathbuf[32];

	sprintf(pathbuf, "titlelvls/%d.cnmb", title_level);
	Serial_LoadBlocks(pathbuf);
	sprintf(pathbuf, "titlelvls/%d.cnms", title_level);
	Serial_LoadSpawners(pathbuf);

	_cam_list_len = 0;
	_next_cam_target = 1;
	SPAWNER *spawner = NULL;
	spawner = Spawners_Iterate(spawner);
	while (spawner) {
		if (spawner->wobj_type == TT_BOSS_WAYPOINT) _cam_list_len++;
		spawner = Spawners_Iterate(spawner);
	}
	if (!_cam_list_len) _cam_list_len = 2;
	_camx_list = malloc(sizeof(*_camx_list) * _cam_list_len);
	_camy_list = malloc(sizeof(*_camy_list) * _cam_list_len);
	spawner = Spawners_Iterate(NULL);
	while (spawner) {
		if (spawner->wobj_type == TT_BOSS_WAYPOINT)	{
			//Console_Print("asdf");
			_camx_list[spawner->custom_int] = spawner->x;
			_camy_list[spawner->custom_int] = spawner->y;
		}
		spawner = Spawners_Iterate(spawner);
	}
	int start_idx = Util_RandInt(0, _cam_list_len - 2);
	_camx = _camx_list[start_idx];
	_camy = _camy_list[start_idx];
	_next_cam_target = start_idx + 1;
	_camx_coarse = _camx;
	_camy_coarse = _camy;
	_camx_spd = 0.0f;
	_camy_spd = 0.0f;

	_title_card_ticker = 0;
	for (int i = 0; i < MAX_TITLE_CARDS; i++) {
		_title_card_y[i] = -title_card.h;
	}

	// XMAS mode stuff
	memset(xmas_static_snow, 0, sizeof(xmas_static_snow));
	memset(xmas_snowflakes, 0, sizeof(xmas_snowflakes));
	memset(xmas_obstacles, 0, sizeof(xmas_obstacles));
	next_snowflake = 0;

	int y;
	for (i = 0; i < NUM_SNOWFLAKES; i++) {
		xmas_snowflakes[next_snowflake].alive = 1;
		xmas_snowflakes[next_snowflake].x = rand() % RENDERER_WIDTH;
		xmas_snowflakes[next_snowflake].y = rand() % (RENDERER_HEIGHT - 5);
		xmas_snowflakes[next_snowflake].xdir = rand() % 3 - 1;
		next_snowflake = (next_snowflake + 1) % NUM_SNOWFLAKES;
	}

	for (y = 0; y < RENDERER_HEIGHT; y++)
	{
		for (x = 0; x < RENDERER_WIDTH; x++)
		{
			i = (255 - (rand() % 32));
			xmas_static_snow_colors[y][x] = Renderer_MakeColor(i, i, i);
		}
	}
}
void GameState_MainMenu_Quit(void)
{
	free(_camx_list);
	free(_camy_list);

	Net_RemovePollingFunc(MainMenu_OnPacket);

	Gui_Reset();
	Gui_DestroyFrame(options);
	Gui_DestroyFrame(root);
	Gui_DestroyFrame(startnewgame);
	Gui_DestroyFrame(joingame_ip);
	Gui_DestroyFrame(hostgame);
	Gui_DestroyFrame(playersetup);
}

void SetSnow(XMAS_SNOWFLAKE *sf, int x, int y) {
	sf->alive = 0;
	if (sf->x < 0) sf->x = 0;
	if (sf->x > RENDERER_WIDTH - 1) sf->x = RENDERER_WIDTH - 1;
	if (x >= 0 && x < RENDERER_WIDTH && y >= 0 && y < RENDERER_HEIGHT)
		xmas_static_snow[y][x] = 1;

	sf->alive = 1;
	sf->x = rand() % RENDERER_WIDTH;
	sf->y = 0;
	sf->xdir = rand() % 3 - 1;
	next_snowflake = (next_snowflake + 1) % NUM_SNOWFLAKES;
}
void MoveSnow(int oldx, int oldy, int newx, int newy) {
	if (oldx >= 0 && oldx < RENDERER_WIDTH && oldy >= 0 && oldy < RENDERER_HEIGHT)
		xmas_static_snow[oldy][oldx] = 0;
	if (newx >= 0 && newx < RENDERER_WIDTH && newy >= 0 && newy < RENDERER_HEIGHT)
		xmas_static_snow[newy][newx] = 1;
}
int GetSnow(int x, int y)
{
	if (x >= 0 && x < RENDERER_WIDTH && y >= 0 && y < RENDERER_HEIGHT)
		return xmas_static_snow[y][x] || xmas_obstacles[y][x];
	else
		return 1;
}
static void MainMenu_OnPacket(NET_PACKET *packet)
{
	MSPAGE_DATA *dat;

	if (packet->hdr.type == NET_MASTER_SERVER_PAGE_DATA)
	{
		int displayed_page;
		dat = (MSPAGE_DATA *)packet->data;
		memcpy(&msdata, dat, sizeof(msdata));
		displayed_page = dat->page;
		if (dat->num_pages) displayed_page++;
		sprintf(serverbrowser->elements[1].name, "PAGE: %d/%d", displayed_page, dat->num_pages);

		if (msdata.page + 1 >= msdata.num_pages)
			serverbrowser->elements[MSPAGE_SIZE * 4 + SB_START + 1].active = CNM_FALSE;
		if (msdata.page <= 0)
			serverbrowser->elements[MSPAGE_SIZE * 4 + SB_START + 2].active = CNM_FALSE;

		int i;
		for (i = 0; i < MSPAGE_SIZE; i++) 
		{
			if (dat->servers[i].addr.host == Net_GetIpFromString("127.0.0.1").host)
				msdata.servers[i].addr.host = packet->hdr.addr.host;

			if (dat->servers[i].num_players == -1) {
				strcpy(serverbrowser->elements[SB_START+i*4+0].name, "NAME:");
				strcpy(serverbrowser->elements[SB_START+i*4+1].name, "PLAYERS:");
				strcpy(serverbrowser->elements[SB_START+i*4+2].name, "MAP:");
				serverbrowser->elements[SB_START + i * 4 + 0].active = CNM_FALSE;
				serverbrowser->elements[SB_START + i * 4 + 1].active = CNM_FALSE;
				serverbrowser->elements[SB_START + i * 4 + 2].active = CNM_FALSE;
				serverbrowser->elements[SB_START + i * 4 + 3].active = CNM_FALSE;
			}
			else {
				sprintf(serverbrowser->elements[SB_START + i * 4 + 0].name, "NAME:    %s", dat->servers[i].name);
				sprintf(serverbrowser->elements[SB_START + i * 4 + 1].name, "PLAYERS: %d", dat->servers[i].num_players);
				sprintf(serverbrowser->elements[SB_START + i * 4 + 2].name, "MAP:     %s", dat->servers[i].level);
				serverbrowser->elements[SB_START + i * 4 + 0].active = CNM_TRUE;
				serverbrowser->elements[SB_START + i * 4 + 1].active = CNM_TRUE;
				serverbrowser->elements[SB_START + i * 4 + 2].active = CNM_TRUE;
				serverbrowser->elements[SB_START + i * 4 + 3].active = CNM_TRUE;
			}
		}
	}
}
void GameState_MainMenu_Update(void)
{
	int last_ydir = ai_ydir;

	Net_PollPackets(64);
	Net_Update();
	Input_Update();
	GameConsole_Update();
	Gui_Update();

	refresh_cooldown--;
	if (refresh_cooldown < 0) {
		strcpy(serverbrowser->elements[2].name, "<REFRESH PAGE>");
		if (msdata.page + 1 < msdata.num_pages)
			serverbrowser->elements[MSPAGE_SIZE*4+SB_START+1].active = CNM_TRUE;
		if (msdata.page > 0)
			serverbrowser->elements[MSPAGE_SIZE*4+SB_START+2].active = CNM_TRUE;
	}
	else {
		strcpy(serverbrowser->elements[2].name, "REFRESHING...");
		serverbrowser->elements[MSPAGE_SIZE * 4 + SB_START + 1].active = CNM_FALSE;
		serverbrowser->elements[MSPAGE_SIZE * 4 + SB_START + 2].active = CNM_FALSE;
	}

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING))
		Gui_Focus();
	//cloudsx[0] -= 1;
	//cloudsx[1] -= 1;
	//if (cloudsx[0] <= 0 - 256)
	//{
	//	cloudsx[0] = 0;
	//	cloudsx[1] = 256;
	//}

	if (Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING))
	{
		controlling = 45;
		if (vel[1] >= -14.0f)
			vel[1] -= 1.0f;
	}
	if (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING))
	{
		controlling = 45;
		if (vel[1] <= 0.0f)
			vel[1] += 1.0f;
	}
	if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING))
	{
		controlling = 45;
		if (vel[0] <= 12.0f)
			vel[0] += 2.0f;
	}
	if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING))
	{
		controlling = 45;
		if (vel[0] >= -12.0f)
			vel[0] -= 2.0f;
	}

	controlling--;
	ai_timer--;

	if (controlling <= 0)
	{
		if (ai_timer <= 0)
		{
			ai_timer = Util_RandInt(5, 30);
			if (pos[0] < 140.0f)
				ai_xdir = 1;
			if (pos[0] > 180.0f)
				ai_xdir = -1;

			if (ai_same_speed >= 60)
			{
				if (ai_ydir)
					ai_ydir = 0;
				else
					ai_ydir = Util_RandInt(0, 1) * 2 - 1;
			}
			else
			{
				if (pos[1] < 100.0f)
					ai_ydir = 1;
				else if (pos[1] > 140.0f)
					ai_ydir = -1;
				else
					ai_ydir = 0;
			}
		}

		if (ai_ydir <= -9.0f || ai_ydir >= 1.0f)
			ai_same_speed++;
		else
			ai_same_speed = 0;

		if (ai_xdir == 1 && vel[0] <= 12.0f)
			vel[0] += 2.0f;
		if (ai_xdir == -1 && vel[0] >= -12.0f)
			vel[0] -= 2.0f;
		if (ai_ydir == 1 && vel[0] <= 0.0f)
			vel[1] += 1.0f;
		if (ai_ydir == -1 && vel[0] >= -10.0f)
			vel[1] -= 1.0f;
	}

	vel[0] = vel[0] * 0.9f;
	vel[1] += 0.3f;
	pos[0] += vel[0];
	pos[1] += vel[1];
	if (vel[1] >= 16.0f && controlling)
		vel[1] = 16.0f;
	else if (vel[1] >= 10.0f && !controlling)
		vel[1] = 10.0f;
	if (pos[1] >= 280.0f)
		pos[1] = -33.0f;
	if (pos[1] <= -64.0f)
		pos[1] = 260.0f;
	if (pos[0] >= 360.0f)
		pos[0] = -33.0f;
	if (pos[0] <= -64.0f)
		pos[0] = 350.0f;

	// Camera shit
	if (fabsf(_camx_coarse - _camx_list[_next_cam_target]) < 2.5f &&
		fabsf(_camy_coarse - _camy_list[_next_cam_target]) < 2.5f) {
		_next_cam_target++;
		if (_next_cam_target == _cam_list_len) {
			_camx_coarse = _camx_list[0];
			_camy_coarse = _camy_list[0];
			_camx = _camx_coarse;
			_next_cam_target = 1;
		}
	}
	const float cam_speed = 1.5f, cam_accel = 0.02f;
	if (_camx_coarse < _camx_list[_next_cam_target] - cam_speed * 0.75f) _camx_coarse += cam_speed;
	if (_camx_coarse > _camx_list[_next_cam_target] + cam_speed * 0.75f) _camx_coarse -= cam_speed;
	if (_camy_coarse < _camy_list[_next_cam_target] - cam_speed * 0.75f) _camy_coarse += cam_speed;
	if (_camy_coarse > _camy_list[_next_cam_target] + cam_speed * 0.75f) _camy_coarse -= cam_speed;
	const float deccel_distx = (-1.0f * _camx_spd * _camx_spd)/(-2.0f * cam_accel);
	const float deccel_disty = (-1.0f * _camy_spd * _camy_spd)/(-2.0f * cam_accel);
	if (_camx < _camx_coarse && _camx_spd < cam_speed) _camx_spd += cam_accel;
	if (_camx > _camx_coarse && _camx_spd > -cam_speed) _camx_spd -= cam_accel;
	if (_camy < _camy_coarse && _camy_spd < cam_speed) _camy_spd += cam_accel;
	if (_camy > _camy_coarse && _camy_spd > -cam_speed) _camy_spd -= cam_accel;
	_camx += _camx_spd;
	_camy += _camy_spd;

	if (_camx > _camx_coarse - deccel_distx && _camx < _camx_coarse &&  _camx_spd > 0.0f) _camx_spd -= cam_accel * 2.0f;
	if (_camx < _camx_coarse + deccel_distx && _camx > _camx_coarse &&  _camx_spd < 0.0f) _camx_spd += cam_accel * 2.0f;
	if (_camy > _camy_coarse - deccel_disty && _camy < _camy_coarse &&  _camy_spd > 0.0f) _camy_spd -= cam_accel * 2.0f;
	if (_camy < _camy_coarse + deccel_disty && _camy > _camy_coarse &&  _camy_spd < 0.0f) _camy_spd += cam_accel * 2.0f;

	// XMAS MODE!
	if (!Game_GetVar(GAME_VAR_XMAS_MODE)->data.integer) return;

	int i, x, y, grav;
	grav = 1;
	if (Input_GetButton(INPUT_DROP, INPUT_STATE_PLAYING))
		grav = 12;
	XMAS_SNOWFLAKE *sf;
	for (x = 0; x < grav; x++) {
		for (i = 0; i < NUM_SNOWFLAKES; i++)
		{
			sf = &xmas_snowflakes[i];
			if (!sf->alive) continue;
			if (Game_GetFrame() % 4 == 0)
				sf->x += sf->xdir;
			if (Game_GetFrame() % 20 == 0)
				sf->xdir = rand() % 3 - 1;
			if (sf->x < 0)
			{
				sf->x = 0;
				sf->xdir = 1;
			}
			if (sf->x >= RENDERER_WIDTH)
			{
				sf->x = RENDERER_WIDTH - 1;
				sf->xdir = -1;
			}

			sf->y += 1;
			if (GetSnow(sf->x, sf->y + 1))
				SetSnow(sf, sf->x, sf->y);
			sf->y += 1;
			if (GetSnow(sf->x, sf->y + 1))
				SetSnow(sf, sf->x, sf->y);
		}
	}
	for (i = 0; i < grav*2; i++) {
		for (y = RENDERER_HEIGHT; y >= 0; y--)
		{
			for (x = 0; x < RENDERER_WIDTH; x++)
			{
				if (!xmas_static_snow[y][x]) continue;

				if (!GetSnow(x, y + 1))
				{
					MoveSnow(x, y, x, y + 1); continue;
				}
				if (!GetSnow(x - 1, y + 1))
				{
					MoveSnow(x, y, x - 1, y + 1); continue;
				}
				if (!GetSnow(x + 1, y + 1))
				{
					MoveSnow(x, y, x + 1, y + 1); continue;
				}
			}
		}
	}
}
static const int bar_colors[] = {
	195, 196, 197, 198, 199, 200, 201, 202, 203, 204,
	205,
	204, 203, 202, 201, 200, 199, 198, 197, 196, 195
};
static const int bar_trans[] = {
	7, 6, 5, 4, 3, 2, 1, 0,
	1, 2, 3, 4, 5, 6, 7
};
static float _colors_pos = 0.0f, _trans_pos = 0.0f;
void GameState_MainMenu_Draw(void)
{
	//Renderer_StartDrawing();
	if (!Game_GetVar(GAME_VAR_XMAS_MODE)->data.integer)
	{
		Background_SetVisibleLayers(0, BACKGROUND_MAX_LAYERS);
		for (int i = 0; i < BACKGROUND_MAX_LAYERS; i++) {
			Background_Draw(i, (int)_camx, (int)_camy);
		}
		//Renderer_Clear(Renderer_MakeColor(128, 128, 255));
		//Renderer_DrawBitmap(cloudsx[0], 0, &clouds, 0, RENDERER_LIGHT);
		//Renderer_DrawBitmap(cloudsx[1], 0, &clouds, 0, RENDERER_LIGHT);
		//Renderer_DrawBitmap(cloudsx[0], 0, &clouds, 0, RENDERER_LIGHT);
		//Renderer_DrawBitmap(cloudsx[0]+256, 0, &clouds, 0, RENDERER_LIGHT);
		//Renderer_DrawBitmap(cloudsx[0]+512, 0, &clouds, 0, RENDERER_LIGHT);
	}
	else
	{
		int x, y, z;
		Renderer_Clear(Renderer_MakeColor(0, 0, 0));
		Renderer_DrawBitmap(160 - 96, 30, &title_card, 0, RENDERER_LIGHT);
		Gui_Draw();
		for (y = 0; y < RENDERER_HEIGHT; y++) {
			for (x = 0; x < RENDERER_WIDTH; x++) {
				xmas_obstacles[y][x] = (Renderer_GetPixel(x, y) != Renderer_MakeColor(0, 0, 0));
			}
		}

		Renderer_Clear(Renderer_MakeColor(0, 0, 0));

		for (y = 0; y < RENDERER_HEIGHT; y++) {
			for (x = 0; x < RENDERER_WIDTH; x++) {
				z = (255 - (x % 32));
				if (xmas_static_snow[y][x]) Renderer_PlotPixel(x, y, xmas_static_snow_colors[y][x]);
			}
		}

		for (x = 0; x < NUM_SNOWFLAKES; x++) {
			if (xmas_snowflakes[x].alive)
				Renderer_PlotPixel(xmas_snowflakes[x].x, xmas_snowflakes[x].y, Renderer_MakeColor(255, 255, 255));
		}
	}

	Blocks_DrawBlocks(BLOCKS_BG, (int)_camx, (int)(_camy - 0.5f));
	Blocks_DrawBlocks(BLOCKS_FG, (int)_camx, (int)(_camy - 0.5f));

	Util_SetRect(&player, 40 + ((Game_GetFrame() % 2) * 40), 4688, 40, 40);
	int hflip = vel[0] < 0.0f;
	if (vel[1] > 3.0f) {
		Renderer_DrawBitmap2((int)pos[0] - 8, (int)pos[1] - 8, wings + 2 + (Game_GetFrame() / 3 % 2), 2, RENDERER_LIGHT, hflip, 0);
	} else {
		Renderer_DrawBitmap2((int)pos[0] - 8, (int)pos[1] - 8, wings + (Game_GetFrame() / 10 % 2), 2, RENDERER_LIGHT, hflip, 0);
	}
	Renderer_DrawBitmap2((int)pos[0], (int)pos[1], &player, 0, RENDERER_LIGHT, hflip, 0.0f);

	const int frame = Game_GetFrame() / 2;
	float color_spd = sinf((float)frame / 50.0f) / 10.0f;
	float trans_spd = sinf((float)frame / 30.0f) / 50.0f;
	const float color_spd_bar = sinf((float)frame / 40.0f) * 60.0f;
	const float trans_spd_bar = sinf((float)frame / 20.0f) * 30.0f;
	_colors_pos += cosf((float)frame / 50.0f) / 10.0f;
	_trans_pos += cosf((float)frame / 30.0f) / 10.0f;
	float cpos = _colors_pos, tpos = _trans_pos;
	for (int i = 0; i < RENDERER_HEIGHT; i++) {
		cpos += color_spd;
		//color_spd += cosf((float)frame / 40.0f + (float)i / color_spd_bar) / 30.0f;
		tpos += trans_spd;
		//trans_spd += cosf((float)frame / 20.0f + (float)i / trans_spd_bar) / 30.0f;
		int cidx = abs((int)cpos) % (sizeof(bar_colors) / sizeof(*bar_colors));
		int tidx = abs((int)tpos) % (sizeof(bar_trans) / sizeof(*bar_trans));
		CNM_RECT r;
		Util_SetRect(&r, 0, i, RENDERER_WIDTH, 1);
		Renderer_DrawRect(&r, bar_colors[cidx], bar_trans[tidx], RENDERER_LIGHT);
	}

	_title_card_ticker++;
	for (int i = MAX_TITLE_CARDS - 1; i > -1; i--) {
		if (_title_card_ticker > 30 + (MAX_TITLE_CARDS - i - 1) * 3) {
			int ty = 36 + i * 2;
			_title_card_y[i] += (ty - _title_card_y[i]) * 0.6f;
		}
	}
	for (int i = MAX_TITLE_CARDS - 1; i > 0; i--) {
		Renderer_DrawBitmap(160 - (304 / 2), _title_card_y[i], &title_card, 6, RENDERER_LIGHT);
	}
	Renderer_DrawBitmap(160 - (304 / 2), _title_card_y[0], &title_card, 0, RENDERER_LIGHT);
	Gui_Draw();

	Fadeout_StepFade();
	Fadeout_ApplyFade();
	GameConsole_Draw();
	Renderer_Update();
}
