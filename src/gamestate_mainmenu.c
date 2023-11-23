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
#include "savedata.h"
#include "titlebg.h"
#include "ending_text.h"
#include "player.h"
#include "petdefs.h"

#define SB_START 4

//extern int skin_bases[PLAYER_MAX_SKINS][2];
static const int skin_bases[PLAYER_MAX_SKINS][2] =
{
	{0, 96},
	{192, 256},
	{64, 96},
	{96, 96},
	{96, 160},
	{32, 128},
	{64, 128},
	{0, 128},
	{144, 160},
	{0, 160},
	{48, 160},
};
extern const int complex_skins[PLAYER_MAX_SKINS];
//static CNM_RECT clouds;
static int editor_cheat = CNM_FALSE;
//static int pressed_start;
static int gui_state, last_gui_state;

enum gui_state {
	GUI_PRESS_START,
	GUI_MAIN_STATE,
	GUI_PLAYER_SETUP,
	GUI_OPTIONS_STATE,
	GUI_JOIN_STATE,
	GUI_HOST_STATE,
	GUI_BROWSE_STATE,
	GUI_PLAY_STATE,
	GUI_QUIT_STATE,
	GUI_TITLEBG_STATE,
};


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

static int cleanup_bg;
static int playbit0_x, playbit1_x;

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
	Command_Execute("start_blockedit", CNM_FALSE);
}
static void LightEditorButton(GUI_ELEMENT *elem, int index)
{
	Command_Execute("start_lightedit", CNM_FALSE);
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
	Command_Execute(buffer, CNM_FALSE);
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
	Command_Execute(cmd, CNM_FALSE);
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
	cleanup_bg = CNM_FALSE;
	Command_Execute(buffer, CNM_FALSE);
}

static void MainMenu_OnPacket(NET_PACKET *packet);

static void scan_and_play_music(void) {
	SPAWNER *iter = Spawners_Iterate(NULL);
	while (iter) {
		if (iter->wobj_type == WOBJ_SMALL_TUNES_TRIGGER ||
			iter->wobj_type == WOBJ_VERYBIG_TUNES_TRIGGER ||
			iter->wobj_type == WOBJ_BIG_TUNES_TRIGGER) {
			Audio_PlayMusic(iter->custom_int, CNM_TRUE);	
			break;
		}
		iter = Spawners_Iterate(iter);
	}
}
static void swap_title_bg(const char *lvl) {
	char pathbuf[32];

	// Load in the graphics for the level
	Serial_LoadLevelGfx(lvl);
	sprintf(pathbuf, "%s.cnmb", lvl);
	Serial_LoadBlocks(pathbuf);
	Background_SetVisibleLayers(0, BACKGROUND_MAX_LAYERS);
	sprintf(pathbuf, "%s.cnms", lvl);
	Serial_LoadSpawners(pathbuf);
	scan_and_play_music();

	titlebg_cleanup();
	titlebg_init();
}

void GameState_MainMenu_Init(void)
{
	GUI_FRAME_PROPS props;

	FileSystem_SearchForLevels(CNM_TRUE);
	Fadeout_Setup();
	Fadeout_FadeFromWhite(10, 25);

	//Util_SetRect(&title_card, 320, 832, 192, 32);
	//Util_SetRect(&clouds, 0, 832, 320, 240);
	//Util_SetRect(&clouds, 0, 7200, 256, 320);
	//cloudsx[0] = 0;
	//cloudsx[1] = 320;
	Util_RandSetSeed(0);

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
	serverbrowser = Gui_CreateFrame(SB_START+MSPAGE_SIZE*4+3, 100, NULL, &props, 0);
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
	//Gui_InitNumberElement(playersetup, 1, PlayerSkinCallback, "PLAYER SKIN: ", 0, 9, 0);
	Gui_InitBitmapElement(playersetup, 2, 64, 0, 0, 32, 32);
	//int skinno = Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer;
	//Gui_SetNumberElementInt(playersetup, 1, skinno);
	//memcpy(&playersetup->elements[2].props.bitmap, &wobj_types[WOBJ_PLAYER].frames[skinno], sizeof(CNM_RECT));
	//memcpy(&playersetup->elements[2].props.bitmap, skin_bases[skinno], sizeof(CNM_RECT) / 2);
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
	//Gui_Focus();

	Net_AddPollingFunc(MainMenu_OnPacket);

	//Audio_PlayMusic(2, CNM_FALSE);
	
	int num_title_levels = Filesystem_GetNumTitleLevels();
	if (!num_title_levels) {
		Console_Print("No title background levels!");
		Game_PopState();
		Game_Stop();
		return;
	}
	//int title_level = Util_RandInt(0, num_title_levels);
	char pathbuf[32];
	sprintf(pathbuf, "levels/_title%d", g_globalsave.titlebg);
	swap_title_bg(pathbuf);
	cleanup_bg = CNM_TRUE;
	//Serial_LoadBlocks(pathbuf);
	//Background_SetVisibleLayers(0, BACKGROUND_MAX_LAYERS);
	//sprintf(pathbuf, "levels/_title%d.cnms", title_level);
	//Serial_LoadSpawners(pathbuf);
	//scan_and_play_music();

	gui_state = GUI_PRESS_START;
	last_gui_state = GUI_PRESS_START;

	//titlebg_init();
	//cleanup_bg = CNM_TRUE;
	playbit0_x = playbit1_x = -1000;
	globalsave_save(&g_globalsave);
	//pressed_start = CNM_FALSE;

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
	if (cleanup_bg) titlebg_cleanup();
	
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

	titlebg_update();



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
static int gui_timer;
static int side_blob_x;
static int side_xstart;
static int left_disp;
static int options_num;

static const char *option_names[] = {
	"PLAYER SETUP",
	"PLAY",
	"OPTIONS",
	"JOIN GAME",
	"HOST GAME",
	"BROWSE GAME",
	"TITLE BG",
	"SEE YA MAN",
};
static int help_text_lines[] = {
	4, 4, 4, 3, 3, 4, 3, 2,
};
static const char *help_text[][4] = {
	{
		"CUSTOMIZE",
		"YOUR PLAYER",
		"NAME AND",
		"SKIN!"
	},
	{
		"START A",
		"NEW GAME OR",
		"LOAD A PRE-",
		"VIOUS ONE",
	},
	{
		"EDIT GFX,",
		"AUDIO, AND",
		"OTHER MISC",
		"SETTINGS",
	},
	{
		"JOIN A",
		"MULTIPLAYER",
		"GAME BY IP",
		"",
	},
	{
		"HOST A GAME",
		"ON YOUR",
		"COMPUTER",
		"",
	},
	{
		"BROWSE THE",
		"MASTER",
		"SERVER FOR",
		"GAMES",
	},
	{
		"CUSTOMIZE",
		"THE TITLE",
		"BACKGROUND",
		"",
	},
	{
		"EXIT THE",
		"GAME",
		"",
		"",
	},
};

struct gui_text_box {
	char *buf;
	int buflen;
	int ps_text_cursor;
};

struct gui_text_box gui_text_box_init(char *buf, int len) {
	return (struct gui_text_box) {
		.buf = buf,
		.buflen = len,
		.ps_text_cursor = strlen(buf),
	};
}

void gui_text_box_draw(struct gui_text_box *text, int editing, int x, int y, int trans) {
	char *pname = text->buf;
	char name[64] = { 0 };
	strncpy(name, pname, text->ps_text_cursor);
	if ((Game_GetFrame() / 10) % 2 == 0 && editing) strcat(name, "_");
	else strcat(name, " ");
	strcat(name, pname + text->ps_text_cursor);
	Renderer_DrawText(x, y, trans, RENDERER_LIGHT, name);
}

void gui_text_box_update(struct gui_text_box *text, int editing) {
	char *pname = text->buf;
	int len = strlen(pname);
	if (!editing) {
		text->ps_text_cursor = len;
		return;
	}
	if (Input_GetButtonPressedRepeated(INPUT_BACKSPACE, INPUT_STATE_PLAYING) && text->ps_text_cursor > 0) {
		if (text->ps_text_cursor == len) {
			pname[len - 1] = '\0';
			text->ps_text_cursor--;
		} else {
			memmove(pname + text->ps_text_cursor - 1, pname + text->ps_text_cursor, len - text->ps_text_cursor + 1);
			text->ps_text_cursor--;
		}
	}
	if (Input_GetCharPressed(INPUT_STATE_PLAYING) && len < text->buflen - 1) {
		if (text->ps_text_cursor == len) {
			pname[len] = Input_GetCharPressed(INPUT_STATE_PLAYING);
			pname[len + 1] = '\0';
			text->ps_text_cursor++;
		} else {
			memmove(pname + text->ps_text_cursor + 1, pname + text->ps_text_cursor, len - text->ps_text_cursor + 1);
			pname[text->ps_text_cursor] = Input_GetCharPressed(INPUT_STATE_PLAYING);
			text->ps_text_cursor++;
		}
	}
	if (Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_PLAYING) && text->ps_text_cursor < len) {
		text->ps_text_cursor++;
	}
	if (Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_PLAYING) && text->ps_text_cursor > 0) {
		text->ps_text_cursor--;
	}
}

void draw_main_gui_bars(void) {
	CNM_RECT r;

	const int yheight = 160;
	int idx = options_num - 5;
	//Console_Print("%d", options_num);
	Util_SetRect(&r, 384, 0, 128, 32);
	for (int i = -1; i < yheight/r.h+2; i++) {
		Util_SetRect(&r, 384, 0, 128, 32);
		Renderer_DrawBitmap(-r.w + side_xstart + i*32 + left_disp, RENDERER_HEIGHT-yheight + i*32 + left_disp, &r, 2, RENDERER_LIGHT);
		if (idx >= 0 && idx < sizeof(option_names)/sizeof(*option_names)) {
			int center = strlen(option_names[idx]) * 8 / 2;
			if (idx != options_num) Renderer_SetFont(288, 544, 8, 8);
			else Renderer_SetFont(288, 416, 8, 8);
			Renderer_DrawText(-r.w + side_xstart + i*32 + left_disp + (r.w / 2 - center), RENDERER_HEIGHT - yheight + i*32 + left_disp + 8, 0, RENDERER_LIGHT, option_names[idx]);
			if (idx == options_num) {
				int w = r.w;
				Util_SetRect(&r, 312-24, 608 + 8*(Game_GetFrame() / 2 % 6), 8, 8);
				Renderer_DrawBitmap(-w + side_xstart + i*32 + left_disp + (w / 2 - center) - 12, RENDERER_HEIGHT - yheight + i*32 + left_disp + 8, &r, 0, RENDERER_LIGHT);
				Util_SetRect(&r, 312-24, 608 + 8*((Game_GetFrame() / 2 + 2) % 6), 8, 8);
				Renderer_DrawBitmap2(-w + side_xstart + i*32 + left_disp + (w / 2 + center) , RENDERER_HEIGHT - yheight + i*32 + left_disp + 8, &r, 0, RENDERER_LIGHT, CNM_TRUE, CNM_FALSE);
			}
		}
		idx++;
	}

	Util_SetRect(&r, 400, 32, 112, 48);
	Renderer_SetFont(288, 416, 8, 8);
	Renderer_DrawBitmap2(side_blob_x, RENDERER_HEIGHT-48, &r, 2, RENDERER_LIGHT, CNM_TRUE, CNM_FALSE);
	int text_off = 12;
	if (help_text_lines[options_num] == 3) text_off += 6;
	if (help_text_lines[options_num] == 2) text_off += 6;
	for (int i = 0; i < 4; i++) {
		int align_right = (12 - strlen(help_text[options_num][i])) * 8;
		Renderer_DrawText(side_blob_x + 8 + align_right, RENDERER_HEIGHT-48+text_off+i*8, 0, RENDERER_LIGHT, help_text[options_num][i]);
	}

	int target = gui_state == GUI_MAIN_STATE ? 0 : -192;
	side_xstart += (target - side_xstart) * 0.25f;
	target = gui_state == GUI_MAIN_STATE ? RENDERER_WIDTH - r.w : RENDERER_WIDTH + 8;
	side_blob_x += (target - side_blob_x) * 0.25f;
	left_disp += (0 - left_disp) * 0.25f;
}

static int ps_trans;
static int ps_selected;
static int ps_selected_pos;
static int ps_pos, ps_pos_target;
static int ps_pos_target_add;
static int ps2_pos, ps2_pos_target;
static int ps2_pos_target_add;
static struct gui_text_box ps_player_name, options_mserv;
static int host_game_level_idx;

static void DrawHealthBar(int x, int y, int w, int trans2) {
	CNM_RECT r;

	int fine = (Game_GetFrame() / 3), coarse = (Game_GetFrame() / 10), ccoarse = (Game_GetFrame() / 5);
	int offsets[4] = {
		0, 11, 22, 11
	};
	int xoff = 0;
	for (; w > 0; w -= 32) {
		int ww = w;
		if (ww > 32) ww = 32;
		Util_SetRect(&r, 288 + (fine % 32), 144 + offsets[coarse % 4] + (ccoarse % 5), ww, 5);
		Renderer_DrawBitmap(x+xoff, y, &r, trans2, RENDERER_LIGHT);
		xoff += 32;
	}
}

static float curr_audio = 0.0f;
static char joingame_buf[32];
static int return_rect, quit_rect;
static int return_rects[][2] = {
	{ 480, 480 + ( 32 * 0 ) },
	{ 480, 480 + ( 32 * 1 ) },
	{ 480, 480 + ( 32 * 2 ) },
	{ 480, 480 + ( 32 * 3 ) },
	{ 480, 480 + ( 32 * 4 ) },
	{ 480, 480 + ( 32 * 5 ) },
	{ 480, 480 + ( 32 * 6 ) },
	{ 480, 480 + ( 32 * 7 ) },
	//{ 6, 4616 }, { 0, 6368, }, { 288, 6368, }, { 192, 6368, }, { 224, 6368, },
	//{ 256, 3392, }, { 96, 448, }, { 160, 736, },
};
static int quit_rects[][2] = {
	{ 416, 480 + ( 32 * 0 ) },
	{ 416, 480 + ( 32 * 1 ) },
	{ 416, 480 + ( 32 * 2 ) },
	{ 416, 480 + ( 32 * 3 ) },
	{ 416, 480 + ( 32 * 4 ) },
	{ 416, 480 + ( 32 * 5 ) },
	{ 416, 480 + ( 32 * 6 ) },
	{ 416, 480 + ( 32 * 7 ) },
	{ 416, 480 + ( 32 * 8 ) },
	{ 448, 480 + ( 32 * 0 ) },
	{ 448, 480 + ( 32 * 1 ) },
	//{ 416, 2208 }, { 288, 1536, }, { 416, 1456, }, { 327, 1088, }, { 384, 416, },
	//{ 64, 256, }, { 256, 416, }, { 192, 736, },
	//{ 384, 2208, }, { 173, 2443, }, { 52, 6469, },
};
static int selected_skin, num_skins_cached;
static int selected_pet, num_pets_cached;
static const char *skin_names[] = {
	"\"OG\"",
	"...",
	"NATWAP",
	"HEAD",
	"PENCIL MAN",
	"FLAPPY",
	"ZOBO",
	"BLUE FELLA",
	"MOE",
	"RED GUY (BETA)",
	"RED GUY",
};

#define MAX_BGS 64
static char bgs[MAX_BGS][ENDING_TEXT_MAX_WIDTH + 1];
static int bgids[MAX_BGS];
static int num_bgs;

static void cache_titlebgs(void) {
	num_bgs = 1;
	bgids[0] = 0;
	ps_selected = 0;
	strcpy(bgs[0], "");
	Serial_LoadSpawnersLevelName("levels/_title0.cnms", bgs[0]);
	for (int i = 0; i < 64; i++) {
		if (strlen(g_globalsave.levels_found[i]) == 0) continue;

		bgids[num_bgs] = Filesystem_LevelIDToLevelOrderNum(Filesystem_GetLevelIdFromFileName(g_globalsave.levels_found[i]));
		if (bgids[num_bgs] == -1) continue;
		bgids[num_bgs]++;

		char level_path[32];
		sprintf(level_path, "levels/_title%d.cnms", bgids[num_bgs]);
		bgs[num_bgs][0] = '\0';
		Serial_LoadSpawnersLevelName(level_path, bgs[num_bgs]);
		if (strlen(bgs[num_bgs]) == 0) continue;

		if (bgids[num_bgs] == g_globalsave.titlebg) ps_selected = num_bgs;

		num_bgs++;
	}

	//Console_Print("%d", num_bgs);
}

void draw_player_setup(void) {
	CNM_RECT r, r2;
	static int quit_posx = RENDERER_WIDTH / 2 - 112 + 20 - 10;

	int trans = 7 - ps_trans / 2;
	if (trans < 2) trans = 2;
	if (ps_trans < 0) return;
	int trans2 = 7 - ps_trans / 2;
	if (trans2 < 0) trans2 = 0;

	int ystart = 16;
	int height = 0;
	if (last_gui_state == GUI_HOST_STATE || gui_state == GUI_HOST_STATE) {
		ystart = -48;
		height = 4;
	}
	if (last_gui_state == GUI_PLAYER_SETUP || gui_state == GUI_PLAYER_SETUP) {
		ystart = -32;
		height = 3;
	}
	Util_SetRect(&r, 400, 32, 112, 48);
	Util_SetRect(&r2, 400, 64, 112, 16);
	Renderer_DrawBitmap2(RENDERER_WIDTH / 2, RENDERER_HEIGHT / 2 + ystart, &r, trans, RENDERER_LIGHT, CNM_FALSE, CNM_FALSE);
	Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - r.w, RENDERER_HEIGHT / 2 + ystart, &r, trans, RENDERER_LIGHT, CNM_TRUE, CNM_FALSE);
	for (int i = 0; i < height; i++) {
		Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - r.w, RENDERER_HEIGHT / 2 + ystart + i*r2.h + r.h, &r2, trans, RENDERER_LIGHT, CNM_TRUE, CNM_FALSE);
		Renderer_DrawBitmap2(RENDERER_WIDTH / 2, RENDERER_HEIGHT / 2 + ystart + i*r2.h + r.h, &r2, trans, RENDERER_LIGHT, CNM_FALSE, CNM_FALSE);
	}
	Renderer_DrawBitmap2(RENDERER_WIDTH / 2, RENDERER_HEIGHT / 2 + ystart + height*r2.h + r.h, &r, trans, RENDERER_LIGHT, CNM_FALSE, CNM_TRUE);
	Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - r.w, RENDERER_HEIGHT / 2 + ystart + height*r2.h + r.h, &r, trans, RENDERER_LIGHT, CNM_TRUE, CNM_TRUE);
	gui_timer++;
	
	int target_selected = ps_selected * 12;
	if ((last_gui_state == GUI_HOST_STATE || gui_state == GUI_HOST_STATE) && ps_selected == 4) {
		target_selected = 10*12-1;
	}
	if ((last_gui_state == GUI_PLAYER_SETUP || gui_state == GUI_PLAYER_SETUP) && ps_selected == 2) {
		target_selected = 1*12+48;
	}
	ps_selected_pos += (target_selected - ps_selected_pos) * 0.75f;
	if (ps_pos_target_add > 0) ps_pos_target_add--;
	if (ps_pos_target_add < 0) ps_pos_target_add++;
	ps_pos += ((ps_pos_target + ps_pos_target_add) - ps_pos) * 0.33f;
	if (ps2_pos_target_add > 0) ps2_pos_target_add--;
	if (ps2_pos_target_add < 0) ps2_pos_target_add++;
	ps2_pos += ((ps2_pos_target + ps2_pos_target_add) - ps2_pos) * 0.33f;

	if (last_gui_state == GUI_PLAYER_SETUP || gui_state == GUI_PLAYER_SETUP) {
		Renderer_DrawText(RENDERER_WIDTH / 2 - 8*6, RENDERER_HEIGHT / 2 - 32 + 12, trans2, RENDERER_LIGHT, "PLAYER SETUP");
		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 - 32 + 12+12, trans2, RENDERER_LIGHT, "NAME: ");
		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 - 32 + 12+12+12, trans2, RENDERER_LIGHT, "SKIN: ");
		Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 12-16 - (8*strlen(skin_names[g_globalsave.skins_found[selected_skin]])), RENDERER_HEIGHT / 2 - 32 + 12+12+12, trans2, RENDERER_LIGHT, skin_names[g_globalsave.skins_found[selected_skin]]);

		Util_SetRect(&r2, 312, 608 + 8*(Game_GetFrame() / 2 % 6), 8, 8);
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 - r.w + 8, RENDERER_HEIGHT / 2 - 32 + 24 + ps_selected_pos, &r2, trans2, RENDERER_LIGHT);
		int ralign = r.w - 20 - 16 * 8;
		gui_text_box_draw(&ps_player_name, ps_selected == 0, RENDERER_WIDTH / 2 + ralign, RENDERER_HEIGHT / 2 - 32 + 24, trans2);

		//int *skin = &Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer;
		//Console_Print("%d %d", num_skins_cached, selected_skin);
		for (int i = -1; i <= num_skins_cached + 1; i++) {
			const int skinid = g_globalsave.skins_found[i];
			if (i < 0 || i >= num_skins_cached) {
				Util_SetRect(&r2, 416, 160, 32, 32);
			} else {
				memcpy(&r2, skin_bases[skinid], sizeof(CNM_RECT));
				if (complex_skins[skinid]) {
					r2.w = 40; r2.h = 40;
				} else {
					r2.w = 32; r2.h = 32;
				}
			}
			int pos = i*(32+16) - ps_pos;
			int t = trans2;
			if (pos < -(32+16)) t = trans2 + (-pos - (32+16)) / 3;
			if (pos > 32+16) t = trans2 + (pos - (32+16)) / 3;
			if (t > 7) t = 7;
			const int complex_offset = complex_skins[skinid] ? 4 : 0;
			Renderer_DrawBitmap(
				RENDERER_WIDTH / 2 - 16 + pos - complex_offset,
				RENDERER_HEIGHT / 2 + 16 + (i == selected_skin ? 0 : 4) - complex_offset,
				&r2,
				t,
				i == selected_skin ? RENDERER_LIGHT : RENDERER_LIGHT + 2
			);
		}

		{
			Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 - 32 + 12+12+12+48, trans2, RENDERER_LIGHT, "PET: ");
			const char *petname = selected_pet > 0 ? g_petdefs[g_globalsave.pets_found[selected_pet - 1]].name : "NO PET :(";
			Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 12-16 - (8*strlen(petname)), RENDERER_HEIGHT / 2 - 32 + 12+12+12+48, trans2, RENDERER_LIGHT, petname);
		}

		//Console_Print("%d", num_pets_cached);
		for (int i = -1; i <= num_pets_cached + 2; i++) {
			const int petid = i == 0 ? -1 : g_globalsave.pets_found[i - 1];
			if (i < 0 || i >= num_pets_cached + 1) {
				Util_SetRect(&r2, 416, 160, 32, 32);
			} else if (petid == -1) {
				Util_SetRect(&r2, 448, 224, 32, 32);
			} else {
				r2.x = g_petdefs[petid].iconx * 32;
				r2.y = g_petdefs[petid].icony * 32;
				r2.w = 32;
				r2.h = 32;
			}
			int pos = i*(32+16) - ps2_pos;
			int t = trans2;
			if (pos < -(32+16)) t = trans2 + (-pos - (32+16)) / 3;
			if (pos > 32+16) t = trans2 + (pos - (32+16)) / 3;
			if (t > 7) t = 7;
			Renderer_DrawBitmap(
				RENDERER_WIDTH / 2 - 16 + pos,
				RENDERER_HEIGHT / 2 + 16 + 48 + (i == selected_pet ? 0 : 4),
				&r2,
				t,
				i == selected_pet ? RENDERER_LIGHT : RENDERER_LIGHT + 2
			);
		}
	}

	if (last_gui_state == GUI_OPTIONS_STATE || gui_state == GUI_OPTIONS_STATE) {
		Renderer_DrawText(RENDERER_WIDTH / 2 - 8*3 - 4, RENDERER_HEIGHT / 2 + 16 + 12, trans2, RENDERER_LIGHT, "OPTIONS");
		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 + 16 + 12+12, trans2, RENDERER_LIGHT, "FULLSCREEN: ");
		Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 20 - 3*8, RENDERER_HEIGHT / 2 + 16 + 12+12, trans2, RENDERER_LIGHT,
			(Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer ? "YES" : " NO"));

		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 + 16 + 12+(12*2), trans2, RENDERER_LIGHT, "HI-RES MODE: ");
		Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 20 - 3*8, RENDERER_HEIGHT / 2 + 16 + 12+(12*2), trans2, RENDERER_LIGHT,
			(Game_GetVar(GAME_VAR_HIRESMODE)->data.integer ? "YES" : " NO"));

		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 + 16 + 12+(12*3), trans2, RENDERER_LIGHT, "VOLUME: ");
		Util_SetRect(&r2, 296, 180, 85, 9);
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 + r.w - 20 - r2.w, RENDERER_HEIGHT / 2 + 16 + 12+(12*3) - 1, &r2, trans2, RENDERER_LIGHT);
		curr_audio += (Audio_GetGlobalVolume() - curr_audio) * 0.5f;
		DrawHealthBar(RENDERER_WIDTH / 2 + r.w - 20 - r2.w + 2, RENDERER_HEIGHT / 2 + 16 + 12+(12*3) + 1, 81.0f * curr_audio, trans2);
		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 + 16 + 12+(12*4), trans2, RENDERER_LIGHT, "MS. IP: ");
		gui_text_box_draw(&options_mserv, ps_selected == 3, RENDERER_WIDTH / 2 + r.w - 20 - (14 * 8), RENDERER_HEIGHT / 2 + 16 + 12+(12*4), trans2);
		Util_SetRect(&r2, 312, 608 + 8*(Game_GetFrame() / 2 % 6), 8, 8);
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 - r.w + 8, RENDERER_HEIGHT / 2 + 16 + 24 + ps_selected_pos, &r2, trans2, RENDERER_LIGHT);
	}

	if (last_gui_state == GUI_TITLEBG_STATE || gui_state == GUI_TITLEBG_STATE) {
		Renderer_DrawText(RENDERER_WIDTH / 2 - (8*17)/2, RENDERER_HEIGHT / 2 + 16 + 12, trans2, RENDERER_LIGHT, "PICK A BACKGROUND");
		for (int i = -4; i < num_bgs + 4; i++) {
			const char *str = "????";
			if (i > -1 && i < num_bgs) str = bgs[i];
			if (i < 0 || i >= num_bgs || bgids[i] != g_globalsave.titlebg) Renderer_SetFont(288, 544, 8, 8);
			else Renderer_SetFont(288, 416, 8, 8);
			const int center = RENDERER_HEIGHT / 2 + 16 + (12 * 4 + 2);
			int y = center - ps_pos + (i * 12);
			int trans3 = trans2;
			if (y - center < 2) trans3 += -((y - center) + 2) / 4;
			if (y - center > 8) trans3 += ((y - center) - 8) / 4;
			if (trans3 > 7) trans3 = 7;
			Renderer_DrawText(RENDERER_WIDTH / 2 - (strlen(str)*4), y, trans3, RENDERER_LIGHT, str);
			if (ps_selected == i) {
				const int isblue = (i > -1 && i < num_bgs && bgids[i] == g_globalsave.titlebg) * 24;
				Util_SetRect(&r2, 312-isblue, 608 + 8*(Game_GetFrame() / 2 % 6), 8, 8);
				Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - (strlen(str)*4) - 12, y, &r2, trans2, RENDERER_LIGHT, 0, 0);
				Renderer_DrawBitmap2(RENDERER_WIDTH / 2 + (strlen(str)*4) + 4, y, &r2, trans2, RENDERER_LIGHT, 1, 0);
			}
		}
	}

	if (last_gui_state == GUI_JOIN_STATE || gui_state == GUI_JOIN_STATE) {
		Renderer_DrawText(RENDERER_WIDTH / 2 - 8*11-4, RENDERER_HEIGHT / 2 + 16 + 12, trans2, RENDERER_LIGHT, "JOIN GAME BY IP ADDRESS");
		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 + 16 + 12+(12*2), trans2, RENDERER_LIGHT, "IP: ");
		gui_text_box_draw(&options_mserv, ps_selected == 0, RENDERER_WIDTH / 2 + r.w - 20 - (14 * 8), RENDERER_HEIGHT / 2 + 16 + 12+(12*2), trans2);
		Util_SetRect(&r2, 312, 608 + 8*(Game_GetFrame() / 2 % 6), 8, 8);
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 - r.w + 8, RENDERER_HEIGHT / 2 + 16 + 12+(12*2) + ps_selected_pos, &r2, trans2, RENDERER_LIGHT);
	}

	if (last_gui_state == GUI_HOST_STATE || gui_state == GUI_HOST_STATE) {
		Renderer_DrawText(RENDERER_WIDTH / 2 - 8*11-4, RENDERER_HEIGHT / 2 - 48 + 12, trans2, RENDERER_LIGHT, "HOST A MULTIPLAYER GAME");
		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 - 48 + 12+12, trans2, RENDERER_LIGHT, "NAME: ");
		gui_text_box_draw(&options_mserv, ps_selected == 0, RENDERER_WIDTH / 2 + r.w - 20 - (17 * 8), RENDERER_HEIGHT / 2 - 48 + 12+(12*1), trans2);
		Util_SetRect(&r2, 312, 608 + 8*(Game_GetFrame() / 2 % 6), 8, 8);
		if (ps_selected >= 4) r2.x -= 24;
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 - r.w + 8, RENDERER_HEIGHT / 2 - 48 + 24 + ps_selected_pos, &r2, trans2, RENDERER_LIGHT);
		
		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 - 48 + 12+(12*2), trans2, RENDERER_LIGHT, "ENABLE PVP: ");
		Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 20 - 3*8, RENDERER_HEIGHT / 2 - 48 + 12+(12*2), trans2, RENDERER_LIGHT,
			(Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer ? "YES" : " NO"));

		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 - 48 + 12+(12*3), trans2, RENDERER_LIGHT, "ADVERTISE: ");
		Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 20 - 3*8, RENDERER_HEIGHT / 2 - 48 + 12+(12*3), trans2, RENDERER_LIGHT,
			(Game_GetVar(GAME_VAR_ADVERTISE_SERVER)->data.integer ? "YES" : " NO"));

		int num_lvls = globalsave_get_num_levels(&g_globalsave);
		const char *lvlname = num_lvls ? g_globalsave.levels_found[host_game_level_idx] : "";
		const char *fancyname = FileSystem_GetLevelName(Filesystem_GetLevelIdFromFileName(lvlname));
		//Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 - 48 + 12+(12*4), trans2, RENDERER_LIGHT, "[SELECT MAP]");
		Renderer_DrawText(RENDERER_WIDTH / 2 - (8*strlen(fancyname)) / 2, RENDERER_HEIGHT / 2 - 48 + 12+(12*4), trans2, RENDERER_LIGHT, fancyname);
		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 12+8, RENDERER_HEIGHT / 2 + 16 + 12+(12*5)+6, trans2, RENDERER_LIGHT, "[HOST SERVER]");

		//int *skin = &Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer;
		for (int i = -1; i <= num_lvls+1; i++) {
			if (i < 0 || i >= num_lvls) Util_SetRect(&r2, 416, 96, 96, 64);
			else memcpy(&r2, FileSystem_GetLevelPreview(Filesystem_GetLevelIdFromFileName(g_globalsave.levels_found[i])), sizeof(CNM_RECT));
			//r2.w = 96; r2.h = 64;
			int pos = i*(96+16) - ps_pos;
			int t = trans2;
			for (int slice = 0; slice < 96; slice += 8) {
				int flip = (i < 0 || i >= num_lvls) ? (Game_GetFrame() % 2) : CNM_FALSE;
				int final_pos = flip ? (pos + 96 - slice - 8) : (pos + slice);
				if (final_pos < -16) t = trans2 + (-final_pos - 16) / 4;
				if (final_pos > 96+16) t = trans2 + (final_pos - (96+16)) / 4;
				//if (i == host_game_level_idx) t = trans2;
				if (t > 7) t = 7;
				r2.w = 8;
				Renderer_DrawBitmap2(
					RENDERER_WIDTH / 2 - 48 + final_pos,
					RENDERER_HEIGHT / 2 + 24 + (i == host_game_level_idx ? 0 : 4),
					&r2,
					t,
					i == host_game_level_idx ? RENDERER_LIGHT : RENDERER_LIGHT + 2,
					flip,
					CNM_FALSE
				);
				r2.x += 8;
			}
		}
	}

	if (last_gui_state == GUI_QUIT_STATE || gui_state == GUI_QUIT_STATE) {
		const int y = RENDERER_HEIGHT / 2 + 16 + 12*2;
		Renderer_DrawText(RENDERER_WIDTH / 2 - (15*8)/2, RENDERER_HEIGHT / 2 + 16 + 12, trans2, RENDERER_LIGHT, "YOU WANNA QUIT?");
		Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 20, y, trans2, RENDERER_LIGHT, "RETURN");
		Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 20 - (8*6), y, trans2, RENDERER_LIGHT, "QUIT!?");

		int posx = RENDERER_WIDTH / 2 - r.w + 20 - 10;
		if (ps_selected == 1) posx = RENDERER_WIDTH / 2 + r.w - 20 - (8*6) - 10;
		quit_posx += (posx - quit_posx) * 0.75f;

		Util_SetRect(&r2, 312-24, 608 + 8*(Game_GetFrame() / 2 % 6), 8, 8);
		Renderer_DrawBitmap(quit_posx, y, &r2, trans2, RENDERER_LIGHT);
		Util_SetRect(&r2, 312-24, 608 + 8*((Game_GetFrame() / 2 + 2) % 6), 8, 8);
		Renderer_DrawBitmap2(quit_posx + (8*7) + 4, y, &r2, trans2, RENDERER_LIGHT, CNM_TRUE, CNM_FALSE);

		Util_SetRect(&r2, 240, 96, 32+8, 32+8);
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 - r.w + 20 + (8*3) - 20, y + 16, &r2, trans2, RENDERER_LIGHT);
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 + r.w - 20 - (8*3) - 20, y + 16, &r2, trans2, RENDERER_LIGHT);
		r2.w = 32; r2.h = 32;
		r2.x = return_rects[return_rect][0];
		r2.y = return_rects[return_rect][1];
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 - r.w + 20 + (8*3) - 20 + 4, y + 16 + 4, &r2, trans2, RENDERER_LIGHT);
		r2.x = quit_rects[quit_rect][0];
		r2.y = quit_rects[quit_rect][1];
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 + r.w - 20 - (8*3) - 20 + 4, y + 16 + 4, &r2, trans2, RENDERER_LIGHT);
	}
}

static int current_save_slot;
static int save_slot_basey;
static int completion_basey;
static int ss_trans;
static int delete_mode;
static int joining_timer, is_joining;
static int is_loading_save, loading_save_timer;
static int num_secrets_cached, num_found_cached;
static int saves_add_y, levels_add_y;
#define SAVE_SLOT_WIDTH 112
#define SAVE_SLOT_PADDING 16
#define SAVE_SLOT_EXTENT (SAVE_SLOT_WIDTH+SAVE_SLOT_PADDING)

static void draw_save_card(int slotx, int basey, int trans, int *out_light, int *out_basex) {
	CNM_RECT r, r2;

	Util_SetRect(&r, 456, 32, 56, 48);
	Util_SetRect(&r2, 456, 64, 56, 16);

	const int basex = RENDERER_WIDTH / 2 + slotx*SAVE_SLOT_EXTENT - ps_pos;
	int light = RENDERER_LIGHT;
	if (basex > RENDERER_WIDTH / 2 + 32 || basex < RENDERER_WIDTH / 2 - 32) light = RENDERER_LIGHT + 1;
	if (basex > RENDERER_WIDTH / 2 + 100 || basex < RENDERER_WIDTH / 2 - 100) light = RENDERER_LIGHT + 2;

	Renderer_DrawBitmap2(basex, basey, &r, trans, light, CNM_FALSE, CNM_FALSE);
	Renderer_DrawBitmap2(basex - 56, basey, &r, trans, light, CNM_TRUE, CNM_FALSE);
	for (int j = 0; j < 3; j++) {
		Renderer_DrawBitmap2(basex, basey + r.h + j*r2.h, &r2, trans, light, CNM_FALSE, CNM_FALSE);
		Renderer_DrawBitmap2(basex - 56, basey + r.h + j*r2.h, &r2, trans, light, CNM_TRUE, CNM_FALSE);
	}
	Renderer_DrawBitmap2(basex, basey + r.h + 3*r2.h, &r, trans, light, CNM_FALSE, CNM_TRUE);
	Renderer_DrawBitmap2(basex - 56, basey + r.h + 3*r2.h, &r, trans, light, CNM_TRUE, CNM_TRUE);

	if (out_light) *out_light = light;
	if (out_basex) *out_basex = basex;
}

static void draw_delete_bars(int basex, int basey, int trans2) {
	CNM_RECT r2;

	int time_val = (!delete_mode && gui_timer) ? gui_timer : 10 - gui_timer;
	int width = (float)SAVE_SLOT_WIDTH * ((float)time_val / 10.0f);
	if (delete_mode && gui_timer <= 0) width = SAVE_SLOT_WIDTH;
	int trans3 = trans2 + (7 - time_val);
	if (trans3 < 0) trans3 = 0;
	const int light = RENDERER_LIGHT;

	int x = basex - width / 2;
	int yoff = (int)(sinf((float)Game_GetFrame() / 20)*3.0f);
	Util_SetRect(&r2, 480, 160, 4, 16);
	Renderer_DrawBitmap2(x, basey - 8 - yoff, &r2, trans3, light, CNM_FALSE, CNM_FALSE);
	Renderer_DrawBitmap2(x, basey + 48*3 - 8 + yoff, &r2, trans3, light, CNM_FALSE, CNM_TRUE);
	CNM_RECT r3;
	Util_SetRect(&r3, 496, 160+((Game_GetFrame() / 2) % 7)*16, 16, 16);
	x += r2.w;
	int width_left = width - r2.w;
	for (; width_left >= 16; width_left -= 16) {
		Renderer_DrawBitmap(x, basey - 8 - yoff, &r3, trans3, light);
		Renderer_DrawBitmap2(x, basey + 48*3 - 8 + yoff, &r3, trans3, light, CNM_FALSE, CNM_TRUE);
		x += 16;
	}
	if (width_left > 0) {
		r3.w = width_left;
		Renderer_DrawBitmap(x, basey - 8 - yoff, &r3, trans3, light);
		Renderer_DrawBitmap2(x, basey + 48*3 - 8 + yoff, &r3, trans3, light, CNM_FALSE, CNM_TRUE);
	}
	x = basex + width / 2;
	Renderer_DrawBitmap2(x, basey - 8 - yoff, &r2, trans3, light, CNM_TRUE, CNM_FALSE);
	Renderer_DrawBitmap2(x, basey + 48*3 - 8 + yoff, &r2, trans3, light, CNM_TRUE, CNM_TRUE);
}

static const char *difmsgs[] =
{
	"BABY",
	"REALLY EASY",
	"EASY",
	"NORMAL",
	"KINDA HARD",
	"HARD",
	"ULTRA",
	"EXTREME",
	"DEATH!!!",
	"NIGHTMARE"
};

void draw_play_gui_nologic(void) {
	CNM_RECT r, r2;

	int trans = 7 - ss_trans / 2;
	if (trans < 2) trans = 2;
#define MOVEABLE(name, in, out) \
	if (gui_state == GUI_PLAY_STATE) name += (in - name) * 0.33f; \
	else name += (out - name) * 0.25f;
	MOVEABLE(save_slot_basey, 88, -128)
	MOVEABLE(completion_basey, 0, -50)
	MOVEABLE(playbit0_x, RENDERER_WIDTH - 144, RENDERER_WIDTH + 2)
	MOVEABLE(playbit1_x, 0, -112-2)
	if (ss_trans < 0) return;
	int trans2 = 7 - ss_trans / 2;
	if (trans2 < 0) trans2 = 0;

	const int lvlselect_mode = Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer;

	//Util_SetRect(&r, 368, 7376, 144, 144);
	//Renderer_DrawBitmap(playbit0_x, RENDERER_HEIGHT - r.h, &r, trans2, RENDERER_LIGHT);
	//Util_SetRect(&r, 256, 7328, 112, 192);
	//Renderer_DrawBitmap(playbit1_x, RENDERER_HEIGHT - r.h, &r, trans, RENDERER_LIGHT);

	Util_SetRect(&r, 400-16, 32, 128, 48);
	char text[16];
	Renderer_DrawBitmap2(RENDERER_WIDTH / 2, completion_basey, &r, trans, RENDERER_LIGHT, CNM_FALSE, CNM_TRUE);
	Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - r.w, completion_basey, &r, trans, RENDERER_LIGHT, CNM_TRUE, CNM_TRUE);
	Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 20, completion_basey + 4, trans2, RENDERER_LIGHT, "TOTAL COMPLETION %%:");
	sprintf(text, "%d%%", (int)((float)num_found_cached / (float)FileSystem_NumLevels() * 100.0f));
	Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 20 - (strlen(text)*8), completion_basey + 4, trans2, RENDERER_LIGHT, "%s", text);
	Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 20, completion_basey + 4 + (8*1), trans2, RENDERER_LIGHT, "LEVELS FOUND:");
	sprintf(text, "%d/%d", num_found_cached, FileSystem_NumLevels());
	Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 20 - (strlen(text)*8), completion_basey + 4 + (8*1), trans2, RENDERER_LIGHT, text);
	Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 20, completion_basey + 4 + (8*2), trans2, RENDERER_LIGHT, "NUM SAVES CREATED:");
	sprintf(text, "%d", g_globalsave.saves_created);
	Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 20 - (strlen(text)*8), completion_basey + 4 + (8*2), trans2, RENDERER_LIGHT, text);
	Renderer_DrawText(RENDERER_WIDTH / 2 - r.w + 20, completion_basey + 4 + (8*3), trans2, RENDERER_LIGHT, "SECRETS FOUND:");
	sprintf(text, "%d", num_secrets_cached);
	Renderer_DrawText(RENDERER_WIDTH / 2 + r.w - 20 - (strlen(text)*8), completion_basey + 4 + (8*3), trans2, RENDERER_LIGHT, text);

	const int gs_numlvls = globalsave_get_num_levels(&g_globalsave);

	// Draw the first card (if its on screen)
	if (current_save_slot < 2) {
		const int basey = save_slot_basey - (current_save_slot == -1 ? 6 + loading_save_timer : 0);
		int basex, light;
		if (gs_numlvls) draw_save_card(-1, basey, trans, &light, &basex);
		if ((delete_mode || gui_timer > 0) && current_save_slot == -1) {
			draw_delete_bars(basex, basey, trans2);
		}
		if (gs_numlvls) {
			if (!lvlselect_mode) strcpy(text, "[LEVEL SELECT]");
			else strcpy(text, "[SAVE GAMES]");
			Renderer_DrawText(basex - (8*strlen(text)) / 2, basey + (12*2), trans2, light, text);
			int id = Filesystem_GetLevelIdFromFileName(g_globalsave.levels_found[(Game_GetFrame() / 15) % gs_numlvls]);
			if (lvlselect_mode) id = -1;
			int flip = (id == -1) ? Game_GetFrame() % 2 == 0 : 0;
			if (id == -1) Util_SetRect(&r2, 416, 96, 96, 64);
			else memcpy(&r2, FileSystem_GetLevelPreview(id), sizeof(r2));
			Renderer_DrawBitmap2(basex - SAVE_SLOT_WIDTH / 2 + 8, basey + (12*3), &r2, trans2, light, flip, CNM_FALSE);
			if (lvlselect_mode) {
				Util_SetRect(&r2, 448, 192, 32, 32);
				Renderer_DrawBitmap2(
					basex - SAVE_SLOT_WIDTH / 2 + 8 + 48 - 16,
					basey + (12*3) + 32 - 16 + (int)(sinf((float)Game_GetFrame() / 10.0f)*4.0f),
					&r2,
					trans2, light,
					CNM_FALSE,
					CNM_FALSE
				);
			}
		}
	}

	// Draw the save games
	for (int i = -2; i <= 2; i++) {
		int slot = current_save_slot + i;
		if (slot < 0 || slot > SAVE_SLOTS) continue;
		const int basey = save_slot_basey + saves_add_y - (slot == current_save_slot ? 6 + loading_save_timer : 0);
		if (basey < -RENDERER_HEIGHT / 2 || basey > RENDERER_HEIGHT) continue;
		int basex, light;
		draw_save_card(slot, basey, trans, &light, &basex);

		char text[16];
		if (slot != SAVE_SLOTS) sprintf(text, "SAVE %d", slot + 1);
		else strcpy(text, "[DELETE]");
		Renderer_DrawText(basex - (8*strlen(text)) / 2, basey + 12, trans2, light, text);

		if ((delete_mode || gui_timer > 0) && slot == current_save_slot) {
			draw_delete_bars(basex, basey, trans2);
		}

		if (slot == SAVE_SLOTS) {
			Util_SetRect(&r2, 448, 160, 32, 32);
			Renderer_DrawBitmap2(
				basex - 16,
				basey + 88 - 16 + (int)(sinf((float)Game_GetFrame() / 10.0f)*4.0f),
				&r2,
				trans2, light,
				Game_GetFrame() % 2 == 0,
				CNM_FALSE
			);
			continue;
		}

		int id = Filesystem_GetLevelIdFromFileName(g_saves[slot].level);
		int flip = (id == -1) ? Game_GetFrame() % 2 == 0 : 0;
		if (id == -1) Util_SetRect(&r2, 416, 96, 96, 64);
		else memcpy(&r2, FileSystem_GetLevelPreview(id), sizeof(r2));
		const char *lvlname = id == -1 ? "NEW SAVE" : FileSystem_GetLevelName(id);
		Renderer_DrawText(basex - (8*strlen(lvlname)) / 2, basey + (12*2), trans2, light, lvlname);
		Renderer_DrawBitmap2(basex - SAVE_SLOT_WIDTH / 2 + 8, basey + (12*3), &r2, trans2, light, flip, CNM_FALSE);
		memcpy(&r2, skin_bases[Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer], sizeof(int[2]));
		int skinoff = complex_skins[Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer] ? -4 : 0;
		int skinsz = complex_skins[Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer] ? 40 : 32;
		r2.w = skinsz; r2.h = skinsz;
		Renderer_DrawBitmap2(basex - SAVE_SLOT_WIDTH / 2 + 12 + skinoff, basey + 104 + skinoff, &r2, trans2, light, CNM_TRUE, CNM_FALSE);
		sprintf(text, "X %d", g_saves[slot].lives);
		Renderer_DrawText(basex - SAVE_SLOT_WIDTH / 2 + 12 + 32 + 8, basey + 104 + 12, trans2, light, text);
	}

	// Draw the levels from the selection menu 
	for (int i = -2; i <= 2; i++) {
		int slot = current_save_slot + i;
		if (slot < 0 || slot >= gs_numlvls) continue;
		const int basey = save_slot_basey + levels_add_y - (slot == current_save_slot ? 6 + loading_save_timer : 0);
		if (basey < -RENDERER_HEIGHT / 2 || basey > RENDERER_HEIGHT) continue;
		int basex, light;
		draw_save_card(slot, basey, trans, &light, &basex);

		char text[32];
		int id = Filesystem_GetLevelIdFromFileName(g_globalsave.levels_found[slot]);
		if (id == -1) continue;
		const char *lvlname = FileSystem_GetLevelName(id);
		Renderer_DrawText(basex - (8*strlen(lvlname)) / 2, basey + 12, trans2, light, lvlname);
		memcpy(&r2, FileSystem_GetLevelPreview(id), sizeof(r2));
		Renderer_DrawBitmap(basex - SAVE_SLOT_WIDTH / 2 + 8, basey + (12*2), &r2, trans2, light);
		Renderer_DrawText(basex - (9*8) / 2, basey + 92 + (12*0), trans2, light, "- DIFF. -");
		const char *dif = difmsgs[FileSystem_GetLevelDifficulty(id)];
		Renderer_DrawText(basex - (8*strlen(dif))/2, basey + 92 + (12*1), trans2, light, dif);
		Renderer_DrawText(basex - (7*8) / 2, basey + 92 + (12*2), trans2, light, "- PAR -");
		sprintf(text, "%d", FileSystem_GetLevelParScore(id));
		Renderer_DrawText(basex - (8*strlen(text))/2, basey + 92 + (12*3), trans2, light, text);
	}

	if (ps_pos_target_add > 0) ps_pos_target_add--;
	if (ps_pos_target_add < 0) ps_pos_target_add++;
	ps_pos += ((ps_pos_target + ps_pos_target_add) - ps_pos) * 0.25f;

	if (lvlselect_mode) {
		saves_add_y += (-RENDERER_HEIGHT - saves_add_y) * 0.25f;
		levels_add_y += (0 - levels_add_y) * 0.25f;
	} else {
		levels_add_y += (-RENDERER_HEIGHT - levels_add_y) * 0.25f;
		saves_add_y += (0 - saves_add_y) * 0.25f;
	}
}

void draw_play_gui(void) {
	draw_main_gui_bars();
	draw_play_gui_nologic();

	ss_trans += 2;
	gui_timer--;

	if (is_loading_save) {
		loading_save_timer++;
		if (loading_save_timer == 30) {
			char cmdbuf[64];
			sprintf(cmdbuf, "localmap %s", g_saves[g_current_save].level);
			Command_Execute(cmdbuf, CNM_FALSE);
		}
		return;
	}

	int num_lvls = globalsave_get_num_levels(&g_globalsave);
	int *lvlselect = &Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer;
	int right = *lvlselect ? (num_lvls - 1) : SAVE_SLOTS;
	if (Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_PLAYING)) {
		if (current_save_slot < right) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			current_save_slot++;
		} else {
			ps_pos_target_add = 12;
		}
	}
	if (Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_PLAYING)) {
		if (current_save_slot > (num_lvls ? -1 : 0)) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			current_save_slot--;
		} else {
			ps_pos_target_add = -12;
		}
	}
	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING)) {
		if ((!delete_mode && (current_save_slot != SAVE_SLOTS && current_save_slot != -1)) ||
			*lvlselect && current_save_slot != -1) {
			Fadeout_FadeToWhite(20, 15, 20);
			is_loading_save = CNM_TRUE;
			loading_save_timer = 0;
			g_current_save = current_save_slot;
			if (*lvlselect) {
				Game_GetVar(GAME_VAR_NOSAVE)->data.integer = CNM_TRUE;
				g_current_save = SAVE_SLOTS;
				strcpy(g_saves[g_current_save].level, g_globalsave.levels_found[current_save_slot]);
			}
			if (Filesystem_GetLevelIdFromFileName(g_saves[g_current_save].level) == -1 && !(*lvlselect)) {
				strcpy(g_saves[g_current_save].level, FileSystem_GetLevel(FileSystem_GetLevelFromLevelOrder(0)));
				g_globalsave.saves_created++;
			}
		}
		if (!delete_mode && current_save_slot == -1) {
			if (!(*lvlselect)) {
				saves_add_y = 0;
				levels_add_y = RENDERER_HEIGHT / 2;
			} else {
				levels_add_y = 0;
				saves_add_y = RENDERER_HEIGHT / 2;
			}
			*lvlselect = !(*lvlselect);
		}
		if (current_save_slot == SAVE_SLOTS && !delete_mode && !(*lvlselect)) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			gui_timer = 10;
			delete_mode = CNM_TRUE;
		} else if (delete_mode && !(*lvlselect)) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			gui_timer = 10;
			delete_mode = CNM_FALSE;
			if (current_save_slot != SAVE_SLOTS && current_save_slot != -1) {
				new_save(g_saves + current_save_slot);
				save_game(current_save_slot, g_saves + current_save_slot);
				FileSystem_SearchForLevels(CNM_TRUE);
			}
		}
	}
	ps_pos_target = current_save_slot*SAVE_SLOT_EXTENT;

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING)) {
		if (delete_mode) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			gui_timer = 10;
			delete_mode = CNM_FALSE;
		} else {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			last_gui_state = gui_state;
			gui_state = GUI_MAIN_STATE;
			gui_timer = 0;
			if (Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
				current_save_slot = 0;
			}
			ss_trans = 7*2;
			side_blob_x = RENDERER_WIDTH;
			side_xstart = -192;
			titlebg_set_card_movement(36, 0, CNM_FALSE);
		}
	}
}

void draw_titlebg_gui(void) {
	draw_main_gui_bars();
	draw_player_setup();

	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		g_globalsave.titlebg = bgids[ps_selected];
		Fadeout_FadeToWhite(1, 5, 5);
		char levelname[32];
		sprintf(levelname, "levels/_title%d", g_globalsave.titlebg);
		swap_title_bg(levelname);
	}
	if (Input_GetButtonPressedRepeated(INPUT_DOWN, INPUT_STATE_PLAYING)) {
		if (ps_selected + 1 < num_bgs) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			ps_selected++;
			ps_pos_target += 8+4;
		} else {
			ps_pos_target_add = 8;
		}
	}
	if (Input_GetButtonPressedRepeated(INPUT_UP, INPUT_STATE_PLAYING)) {
		if (ps_selected > 0) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			ps_selected--;
			ps_pos_target -= 8+4;
		} else {
			ps_pos_target_add = -8;
		}
	}

	ps_trans += 2;

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		last_gui_state = gui_state;
		gui_state = GUI_MAIN_STATE;
		gui_timer = 0;
		side_blob_x = RENDERER_WIDTH;
		side_xstart = -192;
		ps_trans = 7*2;
		globalsave_save(&g_globalsave);
	}
}

void draw_main_gui(void) {
	CNM_RECT r;

	draw_player_setup();
	draw_play_gui_nologic();
	draw_main_gui_bars();

	if (Input_GetButtonPressedRepeated(INPUT_DOWN, INPUT_STATE_PLAYING) && options_num + 1 < sizeof(option_names)/sizeof(*option_names)) {
		left_disp = 32;
		side_blob_x = RENDERER_WIDTH;
		options_num++;
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
	}
	if (Input_GetButtonPressedRepeated(INPUT_UP, INPUT_STATE_PLAYING) && options_num > 0) {
		left_disp = -32;
		side_blob_x = RENDERER_WIDTH;
		options_num--;
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
	}
	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING)) {
		last_gui_state = gui_state;
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer = CNM_FALSE;
		switch (options_num) {
		case 0:
			gui_state = GUI_PLAYER_SETUP;
			ps_selected = 0;
			selected_skin = globalsave_find_skin(&g_globalsave, Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer);
			if (selected_skin == -1) selected_skin = 0;
			num_skins_cached = globalsave_get_num_skins(&g_globalsave);
			selected_pet = globalsave_find_pet(&g_globalsave, Game_GetVar(GAME_VAR_PLAYER_PET)->data.integer) + 1;
			num_pets_cached = globalsave_get_num_pets(&g_globalsave);
			ps_trans = 0;
			gui_timer = 0;
			ps_pos = selected_skin * (32+16);
			ps_pos_target = ps_pos;
			ps2_pos = selected_pet * (32+16);
			ps2_pos_target = ps2_pos;
			ps_player_name = gui_text_box_init(Game_GetVar(GAME_VAR_PLAYER_NAME)->data.string, 16);
			titlebg_set_card_movement(4, 0, CNM_FALSE);
			break;
		case 1:
			gui_state = GUI_PLAY_STATE;
			ps_selected = 0;
			ss_trans = 0;
			gui_timer = 0;
			delete_mode = CNM_FALSE;
			is_loading_save = CNM_FALSE;
			loading_save_timer = 0;
			saves_add_y = 0;
			levels_add_y = -RENDERER_HEIGHT+128;
			Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer = CNM_FALSE;
			//save_slot_basey = -128;
			num_secrets_cached = globalsave_get_num_secrets(&g_globalsave);
			num_found_cached = globalsave_get_num_levels(&g_globalsave);
			if (!num_found_cached && current_save_slot == -1) current_save_slot = 0;
			if (current_save_slot == SAVE_SLOTS) current_save_slot = SAVE_SLOTS - 1;
			ps_pos = current_save_slot * SAVE_SLOT_EXTENT;
			ps_pos_target = ps_pos;
			titlebg_set_card_movement(-96 - 10, 0, CNM_FALSE);
			break;
		case 2:
			gui_state = GUI_OPTIONS_STATE;
			ps_selected = 0;
			ps_trans = 0;
			gui_timer = 0;
			options_mserv = gui_text_box_init(Game_GetVar(GAME_VAR_MASTER_SERVER_ADDR)->data.string, 16);
			break;
		case 3:
			gui_state = GUI_JOIN_STATE;
			ps_selected = 0;
			ps_trans = 0;
			gui_timer = 0;
			strcpy(joingame_buf, Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string);
			options_mserv = gui_text_box_init(joingame_buf, 16);
			break;
		case 4:
			gui_state = GUI_HOST_STATE;
			ps_selected = 0;
			host_game_level_idx = 0;
			ps_pos = host_game_level_idx * (96+16);
			ps_pos_target = ps_pos;
			ps_trans = 0;
			gui_timer = 0;
			options_mserv = gui_text_box_init(Game_GetVar(GAME_VAR_SERVER_NAME)->data.string, 18);
			titlebg_set_card_movement(4, 0, CNM_FALSE);
			break;
		case 5:
			msdata.num_pages = 0;
			ServerBrowserLoadPage(0);
			sprintf(serverbrowser->elements[1].name, "PAGE: %d/%d", 0, 0);
			Gui_Focus();
			Gui_SetRoot(serverbrowser);
			gui_state = GUI_BROWSE_STATE;
			gui_timer = 0;
			titlebg_set_card_movement(4, 0, CNM_FALSE);
			break;
		case 6:
			cache_titlebgs();
			gui_state = GUI_TITLEBG_STATE;
			ps_trans = 0;
			gui_timer = 0;
			ps_pos = ps_selected * (8+8);
			ps_pos_target = ps_pos;
			break;
		case 7:
			return_rect = Util_RandInt(0, sizeof(return_rects) / sizeof(*return_rects));
			quit_rect = Util_RandInt(0, sizeof(quit_rects) / sizeof(*quit_rects));
			gui_state = GUI_QUIT_STATE;
			ps_selected = 0;
			ps_trans = 0;
			gui_timer = 0;
			break;
		default: break;
		}
		ps_selected_pos = ps_selected * 12;
	}
}

void draw_quit_gui(void) {
	draw_main_gui_bars();
	draw_player_setup();

	ps_trans += 2;

	if (Input_GetButtonPressed(INPUT_RIGHT, INPUT_STATE_PLAYING) && ps_selected == 0) ps_selected = 1;
	else if (Input_GetButtonPressed(INPUT_LEFT, INPUT_STATE_PLAYING) && ps_selected == 1) ps_selected = 0;

	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING) && ps_selected == 1) Game_Stop();

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING) ||
		(Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING) && ps_selected == 0)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		last_gui_state = gui_state;
		gui_state = GUI_MAIN_STATE;
		gui_timer = 0;
		side_blob_x = RENDERER_WIDTH;
		side_xstart = -192;
		ps_trans = 7*2;
	}
}

void draw_press_start(void) {
	CNM_RECT r;

	gui_timer++;
	if ((gui_timer / 15) % 2 == 0) {
		Util_SetRect(&r, 432-32, 80, 112, 16);
		Renderer_DrawBitmap(RENDERER_WIDTH / 2 - (r.w/2), RENDERER_HEIGHT / 2 + 96, &r, 0, RENDERER_LIGHT);
	}

	if (Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer && !Game_GetVar(GAME_VAR_FORCE_NOSAVE)->data.integer) {
		Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer = CNM_FALSE;
		Game_GetVar(GAME_VAR_NOSAVE)->data.integer = CNM_FALSE;
	}

	playbit1_x = -112-2;
	playbit0_x = RENDERER_WIDTH + 2;

	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		current_save_slot = 0;
		ps_trans = 0;
		joining_timer = 0;
		is_joining = CNM_FALSE;
		last_gui_state = gui_state;
		gui_state = GUI_MAIN_STATE;
		gui_timer = 0;
		ss_trans = 0;
		save_slot_basey = -256;
		options_num = 1;
		side_blob_x = RENDERER_WIDTH;
		side_xstart = -192;
	}
}

void draw_browser_game_gui(void) {
	draw_main_gui_bars();

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_GUI)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		last_gui_state = gui_state;
		gui_state = GUI_MAIN_STATE;
		gui_timer = 0;
		side_blob_x = RENDERER_WIDTH;
		side_xstart = -192;
		ps_trans = 0;
		titlebg_set_card_movement(36, 0, CNM_FALSE);
	}
}

void draw_join_game_gui(void) {
	draw_main_gui_bars();
	gui_text_box_update(&options_mserv, CNM_TRUE);
	draw_player_setup();

	ps_trans += 2;

	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		char buffer[64] = {'\0'};
		strcpy(buffer, "connect ");
		strcat(buffer, joingame_buf);
		Console_Print(buffer);
		cleanup_bg = CNM_FALSE;
		Command_Execute(buffer, CNM_FALSE);
	}

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		last_gui_state = gui_state;
		gui_state = GUI_MAIN_STATE;
		gui_timer = 0;
		side_blob_x = RENDERER_WIDTH;
		side_xstart = -192;
		ps_trans = 7*2;
	}
}

void draw_host_game_gui(void) {
	draw_main_gui_bars();
	gui_text_box_update(&options_mserv, ps_selected == 0 && !is_joining);
	draw_player_setup();
	ps_trans += 2;

	if (is_joining) {
		if (++joining_timer >= 30) {
			Game_SwitchState(GAME_STATE_HOSTED_SERVER);
		}
		return;
	}

	int num_lvls = globalsave_get_num_levels(&g_globalsave);
	if (Input_GetButtonPressed(INPUT_DOWN, INPUT_STATE_PLAYING) && ps_selected < (num_lvls ? 4 : 3)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		ps_selected++;
	}
	if (Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING) && ps_selected > 0) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		ps_selected--;
	}

	int change_button = Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING) ||
		Input_GetButtonPressed(INPUT_LEFT, INPUT_STATE_PLAYING) ||
		Input_GetButtonPressed(INPUT_RIGHT, INPUT_STATE_PLAYING);
	if (ps_selected == 1 && change_button) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		int *enable = &Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer;
		*enable = !(*enable);
	}
	if (ps_selected == 2 && change_button) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		int *enable = &Game_GetVar(GAME_VAR_ADVERTISE_SERVER)->data.integer;
		*enable = !(*enable);
	}
	if (ps_selected == 4 && Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		strcpy(Game_GetVar(GAME_VAR_LEVEL)->data.string, FileSystem_GetLevel(Filesystem_GetLevelIdFromFileName(g_globalsave.levels_found[host_game_level_idx])));
		is_joining = CNM_TRUE;
		joining_timer = 0;
		Fadeout_FadeToWhite(20, 10, 20);
		//Game_SwitchState(GAME_STATE_HOSTED_SERVER);
	}
	if (ps_selected == 3 && Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_PLAYING)) {
		if (host_game_level_idx + 1 < num_lvls) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			host_game_level_idx++;
			ps_pos_target += 96+16;
		} else {
			ps_pos_target_add = 8;
		}
	}
	if (ps_selected == 3 && Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_PLAYING)) {
		if (host_game_level_idx > 0) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			host_game_level_idx--;
			ps_pos_target -= 96+16;
		} else {
			ps_pos_target_add = -8;
		}
	}

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		titlebg_set_card_movement(36, 0, CNM_FALSE);
		last_gui_state = gui_state;
		gui_state = GUI_MAIN_STATE;
		gui_timer = 0;
		side_blob_x = RENDERER_WIDTH;
		side_xstart = -192;
		ps_trans = 7*2;
	}
}

void draw_player_setup_gui(void) {
	CNM_RECT r;

	draw_main_gui_bars();
	gui_text_box_update(&ps_player_name, ps_selected == 0);
	draw_player_setup();

	if (Input_GetButtonPressed(INPUT_DOWN, INPUT_STATE_PLAYING) && ps_selected < 2) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		ps_selected++;
	}
	if (Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING) && ps_selected > 0) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		ps_selected--;
	}
	//int *skin = &Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer;
	if (Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_PLAYING) && ps_selected == 1) {
		if (selected_skin + 1 < num_skins_cached) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			selected_skin++;
			ps_pos_target += 32+16;
		} else {
			ps_pos_target_add = 8;
		}
	}
	if (Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_PLAYING) && ps_selected == 1) {
		if (selected_skin > 0) {
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			selected_skin--;
			ps_pos_target -= 32+16;
		} else {
			ps_pos_target_add = -8;
		}
	}

	if (Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_PLAYING) && ps_selected == 2) {
		if (selected_pet + 1 < num_pets_cached + 1) {
			selected_pet++;
			int audioid = (selected_pet > 0) ? g_petdefs[g_globalsave.pets_found[selected_pet - 1]].idle_snd : -1;
			if (audioid == -1) audioid = 43;
			Audio_PlaySound(audioid, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			ps2_pos_target += 32+16;
		} else {
			ps2_pos_target_add = 8;
		}
	}
	if (Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_PLAYING) && ps_selected == 2) {
		if (selected_pet > 0) {
			selected_pet--;
			int audioid = (selected_pet > 0) ? g_petdefs[g_globalsave.pets_found[selected_pet - 1]].idle_snd : -1;
			if (audioid == -1) audioid = 43;
			Audio_PlaySound(audioid, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			ps2_pos_target -= 32+16;
		} else {
			ps2_pos_target_add = -8;
		}
	}

	ps_trans += 2;

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		titlebg_set_card_movement(36, 0, CNM_FALSE);
		last_gui_state = gui_state;
		gui_state = GUI_MAIN_STATE;
		gui_timer = 0;
		Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer = g_globalsave.skins_found[selected_skin];
		Game_GetVar(GAME_VAR_PLAYER_PET)->data.integer = selected_pet == 0 ? -1 : g_globalsave.pets_found[selected_pet - 1];
		side_blob_x = RENDERER_WIDTH;
		side_xstart = -192;
		ps_trans = 7*2;
		globalsave_save(&g_globalsave);
		Serial_SaveConfig();
	}
}

void draw_options_gui(void) {
	CNM_RECT r;

	draw_main_gui_bars();
	gui_text_box_update(&options_mserv, ps_selected == 3);
	draw_player_setup();

	if (Input_GetButtonPressed(INPUT_DOWN, INPUT_STATE_PLAYING) && ps_selected < 3) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		ps_selected++;
	}
	if (Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING) && ps_selected > 0) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		ps_selected--;
	}
	int change_button = Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING) ||
		Input_GetButtonPressed(INPUT_LEFT, INPUT_STATE_PLAYING) ||
		Input_GetButtonPressed(INPUT_RIGHT, INPUT_STATE_PLAYING);
	if (ps_selected == 0 && change_button) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		int *fullscreen = &Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer;
		*fullscreen = !(*fullscreen);
		Renderer_SetFullscreen(*fullscreen);
	}
	if (ps_selected == 1 && change_button) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		int *hires = &Game_GetVar(GAME_VAR_HIRESMODE)->data.integer;
		*hires = !(*hires);
		Renderer_SetHiResMode(*hires);
	}
	if (ps_selected == 2) {
		if (Input_GetButtonPressedRepeated(INPUT_RIGHT, INPUT_STATE_PLAYING) && Audio_GetGlobalVolume() < 1.0f) {
			Audio_SetGlobalVolume(Audio_GetGlobalVolume() + 0.02f);
			Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		}
		if (Input_GetButtonPressedRepeated(INPUT_LEFT, INPUT_STATE_PLAYING) && Audio_GetGlobalVolume() > 0.0f) {
			Audio_SetGlobalVolume(Audio_GetGlobalVolume() - 0.02f);
			if (Audio_GetGlobalVolume() < 0.05f) {
				Audio_SetGlobalVolume(0.0f);
			} else {
				Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
			}
		}
	}

	ps_trans += 2;

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING)) {
		Audio_PlaySound(43, CNM_FALSE, Audio_GetListenerX(), Audio_GetListenerY());
		last_gui_state = gui_state;
		gui_state = GUI_MAIN_STATE;
		gui_timer = 0;
		side_blob_x = RENDERER_WIDTH;
		side_xstart = -192;
		ps_trans = 7*2;
		Serial_SaveConfig();
	}
}

void draw_new_gui(void) {
	Renderer_SetFont(288, 416, 8, 8);
	ps_trans--;
	ss_trans--;
	switch (gui_state) {
	case GUI_PRESS_START: draw_press_start(); break;
	case GUI_PLAY_STATE: draw_play_gui(); break;
	case GUI_MAIN_STATE: draw_main_gui(); break;
	case GUI_PLAYER_SETUP: draw_player_setup_gui(); break;
	case GUI_OPTIONS_STATE: draw_options_gui(); break;
	case GUI_JOIN_STATE: draw_join_game_gui(); break;
	case GUI_HOST_STATE: draw_host_game_gui(); break;
	case GUI_BROWSE_STATE: draw_browser_game_gui(); break;
	case GUI_QUIT_STATE: draw_quit_gui(); break;
	case GUI_TITLEBG_STATE: draw_titlebg_gui(); break;
	}
}
void draw_play_gui_bg(void) {
	CNM_RECT r;
	int trans = 7 - ss_trans / 2;
	if (trans < 3) trans = 3;
	if (trans < 8) {
		Util_SetRect(&r, 368, 272, 144, 144);
		Renderer_DrawBitmap(playbit0_x, RENDERER_HEIGHT - r.h, &r, trans, RENDERER_LIGHT);
		Util_SetRect(&r, 256, 256, 112, 160);
		Renderer_DrawBitmap(playbit1_x, RENDERER_HEIGHT - r.h, &r, trans, RENDERER_LIGHT);
	}
}
void GameState_MainMenu_Draw(void)
{
	if (Game_GetVar(GAME_VAR_XMAS_MODE)->data.integer) {
		int x, y, z;
		Renderer_Clear(Renderer_MakeColor(0, 0, 0));
		for (y = 0; y < RENDERER_HEIGHT; y++) {
			for (x = 0; x < RENDERER_WIDTH; x++) {
				xmas_obstacles[y][x] = CNM_FALSE;//(Renderer_GetPixel(x, y) != Renderer_MakeColor(0, 0, 0));
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

	titlebg_draw(draw_play_gui_bg);

	draw_new_gui();

	Gui_Draw();

	Fadeout_StepFade();
	Fadeout_ApplyFade();
	GameConsole_Draw();
	Renderer_Update();
}
