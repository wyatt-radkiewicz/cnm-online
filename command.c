#include <string.h>
#include <stdlib.h>
#include "command.h"
#include "game_console.h"
#include "console.h"
#include "blocks.h"
#include "serial.h"
#include "input.h"
#include "renderer.h"
#include "utility.h"
#include "game.h"
#include "spawners.h"
#include "net.h"
#include "server.h"
#include "client.h"
#include "wobj.h"
#include "netgame.h"
#include "audio.h"
#include "filesystem.h"
#include "player.h"
#include "item.h"
#include "ending_text.h"
#include "packet.h"

#define COMMAND_COUNT (sizeof(command_names) / sizeof(*command_names))

typedef void(*COMMAND_FUNC)(const char *args);
static void Command_SaveBlocks(const char *args);
static void Command_LoadBlocks(const char *args);
static void Command_SetBlock(const char *args);
static void Command_SetFullscreen(const char *args);
static void Command_Say(const char *args);
static void Command_Exit(const char *args);
static void Command_StartBlockEdit(const char *args);
static void Command_StartMainMenu(const char *args);
static void Command_ClearAllBlocks(const char *args);
static void Command_StartBPEdit(const char *args);
static void Command_SaveSpawners(const char *args);
static void Command_LoadSpawners(const char *args);
static void Command_SetLight(const char *args);
static void Command_DebugHitboxes(const char *args);
static void Command_DebugShowObjgrid(const char *args);
static void Command_DebugShowGridPoses(const char *args);
static void Command_Connect(const char *args);
static void Command_PrintConnectedPlayers(const char *args);
static void Command_PrintWobjNetSize(const char *args);
static void Command_HiRes(const char *args);
static void Command_ClInterp(const char *args);
static void Command_ShowPos(const char *args);
static void Command_ClSmoothing(const char *args);
static void Command_ClAvgFramesPerUpdate(const char *args);
static void Command_Tunes(const char *args);
static void Command_Kill(const char *args);
static void Command_ShowNodeUUIDS(const char *args);
static void Command_StartLightEdit(const char *args);
static void Command_Noclip(const char *args);
static void Command_GetChecksum(const char *args);
static void Command_LoadGfxFile(const char *args);
static void Command_LoadAudioFile(const char *args);
static void Command_IsTcpServerConnected(const char *args);
static void Command_SetStrength(const char *args);
static void Command_SetHealth(const char *args);
static void Command_Volume(const char *args);
static void Command_SetSpeed(const char *args);
static void Command_SetJump(const char *args);
static void Command_SetItem(const char *args);
static void Command_SetUpgrade(const char *args);
static void Command_StartDialoge(const char *args);
static void Command_DebugState(const char *args);
static void Command_SetMoney(const char *args);
static void Command_SvCheats(const char *args);
static void Command_ShowUUIDS(const char *args);
static void Command_SaveBCondenced(const char *args);
static void Command_ResizeBlocks(const char *args);
static void Command_GetBlocksSize(const char *args);
static void Command_WobjPlayerPacketTest(const char *args);
static void Command_ActivateSupervirus(const char *args);
static void Command_NetFakeLoss(const char *args);
static void Command_NetShowBandwidth(const char *args);
static const char *const command_names[] =
{
	"save_blocks",
	"load_blocks",
	"set_block",
	"full",
	"say",
	"exit",
	"start_blockedit",
	"start_mainmenu",
	"clear_all_blocks",
	"start_bp_edit",
	"save_spawners",
	"load_spawners",
	"set_light",
	"debug_hitboxes",
	"debug_showobjgrid",
	"debug_showgridposes",
	"connect",
	"print_connected_players",
	"print_wobj_net_size",
	"hires",
	"cl_interp",
	"showpos",
	"cl_smoothing",
	"cl_avg_frames_per_update",
	"tunes",
	"kill",
	"show_nodeuuids",
	"start_lightedit",
	"noclip",
	"get_checksum",
	"load_gfx_file",
	"load_audio_file",
	"tcpinfo",
	"set_strength",
	"set_health",
	"volume",
	"set_speed",
	"set_jump",
	"set_item",
	"set_upgrade",
	"start_dialoge",
	"debug_state",
	"set_money",
	"sv_cheats",
	"show_uuids",
	"saveb_condenced",
	"resize_blocks",
	"get_blocks_size",
	"wobj_player_packet_test",
	"DONT_LOOK",
	"net_fake_loss",
	"net_show_bandwidth"
};
static const COMMAND_FUNC command_funcs[] =
{
	Command_SaveBlocks,
	Command_LoadBlocks,
	Command_SetBlock,
	Command_SetFullscreen,
	Command_Say,
	Command_Exit,
	Command_StartBlockEdit,
	Command_StartMainMenu,
	Command_ClearAllBlocks,
	Command_StartBPEdit,
	Command_SaveSpawners,
	Command_LoadSpawners,
	Command_SetLight,
	Command_DebugHitboxes,
	Command_DebugShowObjgrid,
	Command_DebugShowGridPoses,
	Command_Connect,
	Command_PrintConnectedPlayers,
	Command_PrintWobjNetSize,
	Command_HiRes,
	Command_ClInterp,
	Command_ShowPos,
	Command_ClSmoothing,
	Command_ClAvgFramesPerUpdate,
	Command_Tunes,
	Command_Kill,
	Command_ShowNodeUUIDS,
	Command_StartLightEdit,
	Command_Noclip,
	Command_GetChecksum,
	Command_LoadGfxFile,
	Command_LoadAudioFile,
	Command_IsTcpServerConnected,
	Command_SetStrength,
	Command_SetHealth,
	Command_Volume,
	Command_SetSpeed,
	Command_SetJump,
	Command_SetItem,
	Command_SetUpgrade,
	Command_StartDialoge,
	Command_DebugState,
	Command_SetMoney,
	Command_SvCheats,
	Command_ShowUUIDS,
	Command_SaveBCondenced,
	Command_ResizeBlocks,
	Command_GetBlocksSize,
	Command_WobjPlayerPacketTest,
	Command_ActivateSupervirus,
	Command_NetFakeLoss,
	Command_NetShowBandwidth
};

static const char *Command_ExtractArg(const char *args, int arg);
static int Command_CanExecuteCheat(void);

void Command_Execute(const char *command)
{
	int i;
	char name[UTIL_MAX_TEXT_WIDTH + 1], args[UTIL_MAX_TEXT_WIDTH + 1];
	
	if (command == NULL || strlen(command) <= 1 || strlen(command) >= sizeof(name))
	{
		Console_Print("Unknown command!");
		return;
	}
	if (strchr(command, ' ') != NULL && strchr(command, ' ') - command >= sizeof(name))
	{
		Console_Print("Unknown command!");
		return;
	}

	for (i = 0; i < COMMAND_COUNT; i++)
	{
		memset(name, 0, sizeof(name));
		if (strchr(command, ' ') != NULL)
			memcpy(name, command, strchr(command, ' ') - command);
		else
			strcpy(name, command);
		memset(args, 0, sizeof(name));
		if (strchr(command, ' ') != NULL)
			strncpy(args, strchr(command, ' ') + 1, sizeof(args) - 1);
		if (strcmp(name, command_names[i]) == 0)
		{
			command_funcs[i](args);
			return;
		}
	}

	Console_Print("Unknown command!");
}
static const char *Command_ExtractArg(const char *args, int arg)
{
	int i, index = 0;
	if (!arg)
		return args;
	for (i = 0; i < (int)strlen(args); i++)
	{
		if (args[i] == ' ')
			index++;
		if (index == arg)
			return args + i + 1;
	}
	return args;
}
static int Command_CanExecuteCheat(void)
{
	if (Game_TopState() == GAME_STATE_SINGLEPLAYER)
		return 1;
	if (Game_GetVar(GAME_VAR_SV_CHEATS)->data.integer && (Game_TopState() == GAME_STATE_HOSTED_SERVER ||
														  Game_TopState() == GAME_STATE_DEDICATED_SERVER ||
														  Game_TopState() == GAME_STATE_CLIENT))
		return 1;
	return 0;
}

static void Command_SaveBlocks(const char *args)
{
	char pathbuf[128];

	if (Game_TopState() != GAME_STATE_BLOCKSEDIT &&
		Game_TopState() != GAME_STATE_BLOCKPROPSEDIT &&
		Game_TopState() != GAME_STATE_LIGHT_EDITOR &&
		Game_TopState() != GAME_STATE_BGEDIT)
		return;

	strcpy(pathbuf, "levels/");
	strcat(pathbuf, Command_ExtractArg(args, 0));
	Serial_SaveBlocks(pathbuf);
}
static void Command_LoadBlocks(const char *args)
{
	char pathbuf[128];

	if (Game_TopState() == GAME_STATE_BLOCKSEDIT ||
		Game_TopState() == GAME_STATE_BLOCKPROPSEDIT ||
		Game_TopState() == GAME_STATE_OBJEDIT ||
		Game_TopState() == GAME_STATE_LIGHT_EDITOR ||
		Game_TopState() == GAME_STATE_BGEDIT)
	{
		strcpy(pathbuf, "levels/");
		strcat(pathbuf, Command_ExtractArg(args, 0));
		Serial_LoadBlocks(pathbuf);
	}
}
static void Command_SetBlock(const char *args)
{
	if (Game_TopState() != GAME_STATE_BLOCKSEDIT)
		return;

	int x, y, b;
	x = atoi(Command_ExtractArg(args, 0));
	y = atoi(Command_ExtractArg(args, 1));
	b = atoi(Command_ExtractArg(args, 2));
	Blocks_SetBlock(BLOCKS_FG, x, y, b);
}
static void Command_SetFullscreen(const char *args)
{
	Renderer_SetFullscreen(atoi(Command_ExtractArg(args, 0)));
}
static void Command_Say(const char *args)
{
	Console_Print(Command_ExtractArg(args, 0));
}
static void Command_Exit(const char *args)
{
	Game_Stop();
}
static void Command_StartBlockEdit(const char *args)
{
	if (Game_TopState() == GAME_STATE_BLOCKSEDIT)
		return;

	Game_SwitchState(GAME_STATE_BLOCKSEDIT);
}
static void Command_StartMainMenu(const char *args)
{
	if (Game_TopState() == GAME_STATE_MAINMENU)
		return;

	Game_SwitchState(GAME_STATE_MAINMENU);
}
static void Command_ClearAllBlocks(const char *args)
{
	int x, y;
	if (Game_TopState() != GAME_STATE_BLOCKSEDIT)
		return;

	for (y = 0; y < Blocks_GetWorldHeight(); y++)
	{
		for (x = 0; x < Blocks_GetWorldWidth(); x++)
		{
			Blocks_SetBlock(BLOCKS_FG, x, y, 0);
			Blocks_SetBlock(BLOCKS_BG, x, y, 0);
		}
	}
}
static void Command_StartBPEdit(const char *args)
{
	if (Game_TopState() == GAME_STATE_BLOCKPROPSEDIT)
		return;

	Game_SwitchState(GAME_STATE_BLOCKPROPSEDIT);
}
static void Command_SaveSpawners(const char *args)
{
	char pathbuf[128];

	if (Game_TopState() != GAME_STATE_OBJEDIT &&
		Game_TopState() != GAME_STATE_ENDTEXT_EDITOR)
		return;

	strcpy(pathbuf, "levels/");
	strcat(pathbuf, Command_ExtractArg(args, 0));
	Serial_SaveSpawners(pathbuf);
}
static void Command_LoadSpawners(const char *args)
{
	char pathbuf[128];
	if (Game_TopState() != GAME_STATE_OBJEDIT &&
		Game_TopState() != GAME_STATE_ENDTEXT_EDITOR)
		return;

	strcpy(pathbuf, "levels/");
	strcat(pathbuf, Command_ExtractArg(args, 0));
	Serial_LoadSpawners(pathbuf);
}
static void Command_SetLight(const char *args)
{
	if (Game_TopState() != GAME_STATE_BLOCKSEDIT)
		return;

	int x, y, l;
	x = atoi(Command_ExtractArg(args, 0));
	y = atoi(Command_ExtractArg(args, 1));
	l = atoi(Command_ExtractArg(args, 2));
	Blocks_SetBlockAmbientLight(x, y, l);
}
static void Command_DebugHitboxes(const char *args)
{
	Game_GetVar(GAME_VAR_SHOW_COLLISION_BOXES)->data.integer = atoi(Command_ExtractArg(args, 0));
}
static void Command_DebugShowObjgrid(const char *args)
{
	Game_GetVar(GAME_VAR_SHOW_OBJGRID)->data.integer = atoi(Command_ExtractArg(args, 0));
}
static void Command_DebugShowGridPoses(const char *args)
{
	Game_GetVar(GAME_VAR_SHOW_GRIDPOS)->data.integer = atoi(Command_ExtractArg(args, 0));
}
static void Command_Connect(const char *args)
{
	strcpy(Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string, Command_ExtractArg(args, 0));
	if (!Game_GetVar(GAME_VAR_NODOWNLOAD)->data.integer)
	{
		Game_SwitchState(GAME_STATE_CLIENT_DOWNLOADING);
	}
	else
	{
		Game_SwitchState(GAME_STATE_CLIENT_CONNECTING);
	}
}
static void Command_PrintConnectedPlayers(const char *args)
{
	NETGAME_NODE *iter = NULL;
	NetGame_Iterate(&iter);
	while (iter != NULL)
	{
		Console_Print("Node %d: Name: %s", iter->id, iter->name);
		NetGame_Iterate(&iter);
	}
}
static void Command_PrintWobjNetSize(const char *args)
{
	Console_Print("%d bytes", WOBJ_NET_SIZE);
}
static void Command_HiRes(const char *args)
{
	Renderer_SetHiResMode(atoi(Command_ExtractArg(args, 0)));
}
static void Command_ClInterp(const char *args)
{
	Game_GetVar(GAME_VAR_CL_INTERP)->data.integer = atoi(Command_ExtractArg(args, 0));
}
static void Command_ShowPos(const char *args)
{
	Game_GetVar(GAME_VAR_SHOWPOS)->data.integer = atoi(Command_ExtractArg(args, 0));
}
static void Command_ClSmoothing(const char *args)
{
	Game_GetVar(GAME_VAR_CL_SMOOTHING)->data.decimal = (float)atof(Command_ExtractArg(args, 0));
}
static void Command_ClAvgFramesPerUpdate(const char *args)
{
	Game_GetVar(GAME_VAR_CL_TIME_BETWEEN_UPDATES)->data.integer = atoi(Command_ExtractArg(args, 0));
}
static void Command_Tunes(const char *args)
{
	Audio_PlayMusic(atoi(Command_ExtractArg(args, 0)), CNM_TRUE);
}
static void Command_Kill(const char *args)
{
	((WOBJ *)Game_GetVar(GAME_VAR_PLAYER)->data.pointer)->health = -100.0f;
}
static void Command_ShowNodeUUIDS(const char *args)
{
	Game_GetVar(GAME_VAR_CL_SHOW_NODEUUIDS)->data.integer = atoi(Command_ExtractArg(args, 0));
}
static void Command_StartLightEdit(const char *args)
{
	if (Game_TopState() == GAME_STATE_LIGHT_EDITOR)
		return;

	Game_SwitchState(GAME_STATE_LIGHT_EDITOR);
}
static void Command_Noclip(const char *args)
{
	if (Command_CanExecuteCheat())
		Game_GetVar(GAME_VAR_NOCLIP)->data.integer = !Game_GetVar(GAME_VAR_NOCLIP)->data.integer;
}
static void Command_GetChecksum(const char *args)
{
	const char *file = Command_ExtractArg(args, 0);
	int index = atoi(Command_ExtractArg(args, 1));
	if (strncmp(file, "bmp_file", strlen("bmp_file")) == 0)
		Console_Print("loaded bmp_file checksum: %d", FileSystem_GetRegisteredGfxFile()->checksum);
	if (strncmp(file, "cnma_file", strlen("cnma_file")) == 0)
		Console_Print("loaded cnma_file checksum: %d", FileSystem_GetRegisteredAudioCfgFile()->checksum);
	if (strncmp(file, "cnmb_file", strlen("cnmb_file")) == 0)
		Console_Print("loaded cnmb_file checksum: %d", FileSystem_GetRegisteredBlocksFile()->checksum);
	if (strncmp(file, "cnms_file", strlen("cnms_file")) == 0)
		Console_Print("loaded cnms_file checksum: %d", FileSystem_GetRegisteredSpawnersFile()->checksum);
	if (strncmp(file, "mid_file", strlen("mid_file")) == 0)
		Console_Print("loaded mid_file[%d] checksum: %d", index, FileSystem_GetRegisteredMusicFile(index)->checksum);
	if (strncmp(file, "wav_file", strlen("wav_file")) == 0)
		Console_Print("loaded wav_file[%d] checksum: %d", index, FileSystem_GetRegisteredSoundFile(index)->checksum);
}
static void Command_LoadGfxFile(const char *args)
{
	Renderer_LoadBitmap(Command_ExtractArg(args, 0));
	Renderer_BuildTables();
	//Renderer_RestartWindow();
	FileSystem_RegisterGfx(Command_ExtractArg(args, 0));
}
static void Command_LoadAudioFile(const char *args)
{
	Serial_LoadAudioCfg(Command_ExtractArg(args, 0));
}
static void Command_IsTcpServerConnected(const char *args)
{
	if (NetTcp_HasConnection())
		Console_Print("The TCP socket is connected");
	else
		Console_Print("The TCP socket has NO connection");
}
static void Command_SetStrength(const char *args)
{
	if (!Command_CanExecuteCheat())
		return;
	WOBJ *player = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	player->strength = (float)atoi(Command_ExtractArg(args, 0)) / 100.0f;
}
static void Command_SetHealth(const char *args)
{
	if (!Command_CanExecuteCheat())
		return;
	WOBJ *player = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	player->health = (float)atoi(Command_ExtractArg(args, 0));
}
static void Command_Volume(const char *args)
{
	Audio_SetGlobalVolume((float)atoi(Command_ExtractArg(args, 0)) / 100.0f);
	Console_Print("Set the new global audio volume to %f", (float)atoi(Command_ExtractArg(args, 0)) / 100.0f);
}
static void Command_SetSpeed(const char *args)
{
	if (!Command_CanExecuteCheat())
		return;
	WOBJ *player = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	player->speed = (float)atoi(Command_ExtractArg(args, 0));
}
static void Command_SetJump(const char *args)
{
	if (!Command_CanExecuteCheat())
		return;
	WOBJ *player = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	player->jump = (float)atoi(Command_ExtractArg(args, 0));
}
static void Command_SetItem(const char *args)
{
	if (!Command_CanExecuteCheat())
		return;
	WOBJ *player = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	Item_Drop(player);
	WOBJ *temp = Wobj_CreateOwned(WOBJ_DROPPED_ITEM, player->x, player->y, atoi(Command_ExtractArg(args, 0)), 0.0f);
	Item_Pickup(player, temp);
}
static void Command_SetUpgrade(const char *args)
{
	if (!Command_CanExecuteCheat())
		return;
	WOBJ *player = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	((PLAYER_LOCAL_DATA *)player->local_data)->upgrade_state = atoi(Command_ExtractArg(args, 0));
}
static void Command_StartDialoge(const char *args)
{
	Dialoge_Start(atoi(Command_ExtractArg(args, 0)), atoi(Command_ExtractArg(args, 1)));
}
static void Command_DebugState(const char *args)
{
	Game_SwitchState(atoi(Command_ExtractArg(args, 0)));
}
static void Command_SetMoney(const char *args)
{
	if (!Command_CanExecuteCheat())
		return;
	WOBJ *player = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	player->money = atoi(Command_ExtractArg(args, 0));
}
static void Command_SvCheats(const char *args)
{
	if (Game_TopState() == GAME_STATE_MAINMENU ||
		Game_TopState() == GAME_STATE_HOSTED_SERVER ||
		Game_TopState() == GAME_STATE_SINGLEPLAYER) {
		Game_GetVar(GAME_VAR_SV_CHEATS)->data.integer = atoi(Command_ExtractArg(args, 0));
	}
}
static void Command_ShowUUIDS(const char *args)
{
	Game_GetVar(GAME_VAR_CL_SHOW_NODEUUIDS)->data.integer = atoi(Command_ExtractArg(args, 0));
}
static void Command_SaveBCondenced(const char *args)
{
	char pathbuf[128];
	if (Game_TopState() != GAME_STATE_BLOCKSEDIT)
		return;

	strcpy(pathbuf, "levels/");
	strcat(pathbuf, Command_ExtractArg(args, 0));
	Serial_CondenceBlockFileAndSave(pathbuf);
}
static void Command_ResizeBlocks(const char *args)
{
	if (Game_TopState() != GAME_STATE_BLOCKSEDIT)
		return;
	Blocks_ResizeWorld(atoi(Command_ExtractArg(args, 0)), atoi(Command_ExtractArg(args, 1)));
}
static void Command_GetBlocksSize(const char *args)
{
	Console_Print("WORLD SIZE: (%d, %d)", Blocks_GetWorldWidth(), Blocks_GetWorldHeight());
}
static void Command_WobjPlayerPacketTest(const char *args)
{
	uint8_t bytes[128];
	int head, numbytes;
	
	head = 0;
	serialize_wobj_packet(0, Game_GetVar(GAME_VAR_PLAYER)->data.pointer, bytes, &head);
	numbytes = packet_bytecount(head);
	Console_Print("Normal Type: %d   Compressed: %d", WOBJ_NET_SIZE, numbytes);
	head = 0;
	parse_wobj_packet(Game_GetVar(GAME_VAR_PLAYER)->data.pointer, bytes, &head);
}
static void Command_ActivateSupervirus(const char *args)
{
	if (Game_TopState() != GAME_STATE_SINGLEPLAYER)
		return;
	WOBJ *p = (WOBJ *)Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer = 1;
	Interaction_CreateWobj(WOBJ_SUPERVIRUS, p->x - 160.0f, p->y - 160.0f, 0, 0.0f);
	Interaction_CreateWobj(WOBJ_SUPERVIRUS, p->x + 160.0f, p->y - 160.0f, 0, 0.0f);
	Interaction_CreateWobj(WOBJ_SUPERVIRUS, p->x - 160.0f, p->y + 160.0f, 0, 0.0f);
	Interaction_CreateWobj(WOBJ_SUPERVIRUS, p->x + 160.0f, p->y + 160.0f, 0, 0.0f);
}
static void Command_NetFakeLoss(const char *args)
{
	Net_FakeLoss(atoi(Command_ExtractArg(args, 0)));
}
static void Command_NetShowBandwidth(const char *args)
{
	Game_GetVar(GAME_VAR_SHOW_BANDWIDTH)->data.integer = atoi(Command_ExtractArg(args, 0));
}
