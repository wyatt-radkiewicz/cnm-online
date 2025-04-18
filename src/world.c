#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "console.h"
#include "filesystem.h"
#include "renderer.h"
#include "blocks.h"
#include "serial.h"
#include "command.h"
#include "wobj.h"
#include "spawners.h"
#include "game.h"
#include "audio.h"
#include "player_spawns.h"
#include "ending_text.h"
#include "background.h"
#include "netgame.h"
#include "net.h"
#include "world.h"
#include "enemies.h"
#include "camera.h"
#include "fadeout.h"
#include "bossbar.h"
#include "teleport_infos.h"
#include "logic_links.h"
#include "player.h"
#include "item.h"
#include "mem.h"

#define TITLE_CARD_MAX_LINES 3

static WOBJ *player;

static char (*title_card_strings)[ENDING_TEXT_MAX_WIDTH + 1];
static int title_card_timer;
static int title_card_num_lines;

static int g_mode;

static void cleanup_spawner_related(void) {
	Wobj_DestroyOwnedWobjs();
	Wobj_DestroyUnownedWobjs();
	Spawners_UnloadSpawners();
	PlayerSpawn_ClearAllSpawns();
	TeleportInfos_FreeLegacyLevelInfo();
	EndingText_ClearAllLines();

	arena_pop_zone("WORLD_SPAWNER");
}

static void startup_spawner_related(void) {
	char buffer[64];

	arena_push_zone("WORLD_SPAWNER");

	// Initialize things to defualt values
	Wobj_NormalWobjs_ZoneAllocLocalDataPools();
	Enemies_ZoneAllocLocalDataPools();
	Enemies_Reset();
	Item_Reset();
	LogicLinks_ResetLinks();
	Background_SetVisibleLayers(0, BACKGROUND_MAX_LAYERS - 1);
	Spawners_UnloadSpawners();
	Wobj_DestroyOwnedObjectsFromLastFrame();
	Wobj_DestroyOwnedWobjs();
	Wobj_DestroyUnownedWobjs();
	EndingText_ResetYValue();
	Dialoge_End();
	TTBoss_ResetOnLevelLoad();
	//Fadeout_Setup();
	BossBar_Init();
	Player_ResetHUD();
	Game_ResetFrame();


	// Misc game global vars
	Game_GetVar(GAME_VAR_GRAVITY)->data.decimal = 0.5f;
	Game_GetVar(GAME_VAR_LEVEL_TIMER)->data.integer = 0;
	int lvlid = Filesystem_GetLevelIdFromFileName(Game_GetVar(GAME_VAR_LEVEL)->data.string);
	Game_GetVar(GAME_VAR_BRONZE_SCORE)->data.integer =
		FileSystem_GetLevelParScore(lvlid, 0);
	Game_GetVar(GAME_VAR_SILVER_SCORE)->data.integer =
		FileSystem_GetLevelParScore(lvlid, 1);
	Game_GetVar(GAME_VAR_GOLD_SCORE)->data.integer =
		FileSystem_GetLevelParScore(lvlid, 2);

	// Force unused save slot if in nosave g_mode
	if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer) g_current_save = SAVE_SLOTS;

	// Make sure the player is the first thing created as some other entities might depend on it for creation
	PlayerSpawns_SetMode(PLAYER_SPAWN_TYPE_NORMAL_MODES);
	player = Wobj_CreateOwned(WOBJ_PLAYER, 0.0f, 0.0f, Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer, 0.0f);
	Interaction_SetClientPlayerWObj(player);
	if (Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER) Player_TryTitlePopup();

	// Updating and getting world variables
	Game_GetVar(GAME_VAR_PLAYER)->data.pointer = player;

	// Initialize the spawner multi/single-player g_mode
	if (g_mode == WORLD_MODE_SINGLEPLAYER)
		Spawners_SetGlobalMode(SPAWNER_SINGLEPLAYER);
	else
		Spawners_SetGlobalMode(SPAWNER_MULTIPLAYER);

	// Loading in the actual spawners
	sprintf(buffer, "%s.cnms", Game_GetVar(GAME_VAR_LEVEL)->data.string);
	Serial_LoadSpawners(buffer);

	// Spawn all the objects in
	if (g_mode != WORLD_MODE_CLIENT)
	{
		Spawners_CreateAllWobjsFromSpawners();
	}

	// Load in the new player position
	PlayerSpawn_SetWobjLoc(&player->x);
	{
		PLAYER_LOCAL_DATA *local_data = player->local_data;
		if (local_data->pet) {
			local_data->pet->x = player->x;
			local_data->pet->y = player->y;
		}
	}
	Camera_Setup((int)player->x, (int)player->y); // Initializing camera values
	
	// Load player state
	if (Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER && !Game_GetVar(GAME_VAR_NOSAVE)->data.integer) {
		Player_LoadFromSave(player, g_saves + g_current_save);
		strcpy(g_saves[g_current_save].level, Game_GetVar(GAME_VAR_LEVEL)->data.string);
		save_game(g_current_save, g_saves + g_current_save);
		globalsave_visit_level(&g_globalsave, g_saves[g_current_save].level);
	} else {
		g_current_save = SAVE_SLOTS;
		new_save(g_saves + g_current_save);
		if (Game_GetVar(GAME_VAR_FORCE_NOSAVE)->data.integer) g_saves[g_current_save].lives = 50;
		Player_LoadFromSave(player, g_saves + g_current_save);
	}
}

void World_SoftRestart(void) {
	cleanup_spawner_related();
	startup_spawner_related();
}

void World_Start(int mode)
{
	arena_push_zone("WORLD");
	g_mode = mode;
	title_card_strings = arena_alloc(sizeof(*title_card_strings) * TITLE_CARD_MAX_LINES);
	char buffer[UTIL_MAX_TEXT_WIDTH * 2];

	// Initialize the interaction modes first
	switch (mode)
	{
	case WORLD_MODE_SINGLEPLAYER:
		Interaction_SetMode(INTERACTION_MODE_SINGLEPLAYER);
		Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer = 0; // Disable PVP
		Game_GetVar(GAME_VAR_SV_CHEATS)->data.integer = 0;
		break;
	case WORLD_MODE_HOSTED_SERVER: Interaction_SetMode(INTERACTION_MODE_HOSTED_SERVER); break;
	case WORLD_MODE_CLIENT: Interaction_SetMode(INTERACTION_MODE_CLIENT); break;
	}

	// Load in the graphics for the level
	Serial_LoadLevelGfx(Game_GetVar(GAME_VAR_LEVEL)->data.string);

	// Loading in the actual world
	sprintf(buffer, "%s.cnmb", Game_GetVar(GAME_VAR_LEVEL)->data.string);
	Serial_LoadBlocks(buffer);

	// Objects/other things
	startup_spawner_related();

	// Supervirus stuff
	if (Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer) {
		Interaction_CreateWobj(WOBJ_SUPERVIRUS, player->x - 160.0f, player->y - 160.0f, 0, 0.0f);
		Interaction_CreateWobj(WOBJ_SUPERVIRUS, player->x + 160.0f, player->y - 160.0f, 0, 0.0f);
		Interaction_CreateWobj(WOBJ_SUPERVIRUS, player->x - 160.0f, player->y + 160.0f, 0, 0.0f);
		Interaction_CreateWobj(WOBJ_SUPERVIRUS, player->x + 160.0f, player->y + 160.0f, 0, 0.0f);
	}

	// Title Card Stuff
	title_card_timer = 30*3;
	strcpy(title_card_strings[0], EndingText_GetLine(47));
	title_card_num_lines = 1;
	char *split = strchr(title_card_strings[0], '\\');
	while (split != NULL)
	{
		strcpy(title_card_strings[title_card_num_lines++], split + 1);
		*split = '\0';
		if (title_card_num_lines < TITLE_CARD_MAX_LINES)
			split = strchr(title_card_strings[title_card_num_lines], '\\');
		else
			split = NULL;
	}
	strcpy(Game_GetVar(GAME_VAR_FANCY_LEVEL_NAME)->data.string, title_card_strings[0]); // Setup the "Fancy Level Name" for server advertisement
}
void World_Stop(void)
{
	// Save player state
	if (Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER && !Game_GetVar(GAME_VAR_NOSAVE)->data.integer) Player_SaveData(player, g_saves + g_current_save);

	cleanup_spawner_related();
	// General cleanup
	//Wobj_DestroyOwnedWobjs();
	//Wobj_DestroyUnownedWobjs();
	//Spawners_UnloadSpawners();
	//PlayerSpawn_ClearAllSpawns();
	//TeleportInfos_FreeLegacyLevelInfo();
	//EndingText_ClearAllLines();
	WobjDbg_CheckMemLeaks();

	// Setting global variables to NULL values
	Game_GetVar(GAME_VAR_PLAYER)->data.pointer = NULL;
	//memset(Game_GetVar(GAME_VAR_LEVEL)->data.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
	arena_pop_zone("WORLD");
}
void World_Update(int mode)
{
	NETGAME_NODE *iter;
	int x, y, camx, camy;

	// Misc updates
	if (~player->flags & WOBJ_HAS_PLAYER_FINISHED) {
		Game_GetVar(GAME_VAR_LEVEL_TIMER)->data.integer++;
	}
	Audio_SetListenerOrigin((int)player->x, (int)player->y);
	Dialoge_Update();
	
	if (mode == WORLD_MODE_HOSTED_SERVER)
	{
		NetGame_GetNode(0)->x = (float)Camera_GetXPos();
		NetGame_GetNode(0)->y = (float)Camera_GetYPos();
	}

	// First destroy any owned objects that were deleted here instead of during updating
	Wobj_DestroyOwnedObjectsFromLastFrame();
	
	// Spawning in more things
	camx = Camera_GetXPos(); camy = Camera_GetYPos();
	switch (mode)
	{
	case WORLD_MODE_SINGLEPLAYER:
		for (x = camx / (int)OBJGRID_SIZE - 2; x < (camx + RENDERER_WIDTH) / (int)OBJGRID_SIZE + 2; x++)
			for (y = camy / (int)OBJGRID_SIZE - 2; y < (camy + RENDERER_HEIGHT) / (int)OBJGRID_SIZE + 2; y++)
				Spawners_CreateWobjsFromSpawners(x, y);
		break;
	case WORLD_MODE_HOSTED_SERVER:
		Spawners_MultiModeArbitraryTick();
		iter = NULL;
		NetGame_Iterate(&iter);
		while (iter != NULL)
		{
			for (x = (int)iter->x / (int)OBJGRID_SIZE - 2; x < ((int)iter->x + RENDERER_WIDTH) / (int)OBJGRID_SIZE + 2; x++)
				for (y = (int)iter->y / (int)OBJGRID_SIZE - 2; y < ((int)iter->y + RENDERER_HEIGHT) / (int)OBJGRID_SIZE + 2; y++)
					Spawners_CreateWobjsFromSpawners(x, y);

			NetGame_Iterate(&iter);
		}
		break;
	}
	
	// Dedicated server stuff
	if (Game_TopState() == GAME_STATE_HOSTED_SERVER &&
		Game_GetDedicatedGameInfo()->dedicated) {
		// Teleport the player out of bounds
		WOBJ *dedicated_player = (WOBJ *)Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
		dedicated_player->x = -1000.0f;
		dedicated_player->y = -1000.0f;
		Wobj_UpdateGridPos(dedicated_player);
	}

	// Actually updating the objects
	Wobj_UpdateOwnedWobjs();
	Interaction_Tick();

	// Camera and fadeout code, including bossbar code
	Camera_Update((int)player->x + 16, (int)player->y + 16);
	//Fadeout_StepFade();
	BossBar_Update();
}
void World_Draw(int mode)
{
	NETGAME_NODE *iter;
	int x, y, i, camx, camy;
	CNM_RECT r;

	camx = (int)Camera_GetXPos();
	camy = (int)Camera_GetYPos();

	if (Game_GetVar(GAME_VAR_DRAW_BG)->data.integer) {
		// Draw the background
		Background_Draw(0, camx, camy);
	}
	
	if (Game_GetVar(GAME_VAR_DRAW_FG)->data.integer) {
		// Draw low priority block layer
		Blocks_DrawBlocks(BLOCKS_BG, camx, camy);
	}

	if (Game_GetVar(GAME_VAR_DRAW_OBJ)->data.integer) {
		// Update block lighting with respect to object lights
		Wobj_CalculateLightForScreen(camx, camy);
		
		// Draw the wobjs
		Wobj_DrawWobjs(camx, camy);
	}

	if (Game_GetVar(GAME_VAR_DRAW_FG)->data.integer) {
		// Draw the high priority layer of blocks
		Blocks_DrawBlocks(BLOCKS_FG, camx, camy);
		Renderer_SaveToEffectsBuffer();
		Blocks_DrawBlocks(BLOCKS_DUMMY_EFFECTS, camx, camy);
		Renderer_SaveToEffectsBuffer();
		Blocks_DrawBlocks(BLOCKS_DUMMY_EFFECTS_EX, camx, camy);
	}

	if (Game_GetVar(GAME_VAR_DRAW_OBJ)->data.integer) {
		// Draw overlayer things
		Wobj_DrawWobjsOverlayer(camx, camy);
	}

	if (Game_GetVar(GAME_VAR_DRAW_BG)->data.integer) {
		// High priority background
		Background_Draw(1, camx, camy);
	}

	//Renderer_DrawHorzRippleEffect(&(CNM_RECT){ .x = 0, .y = 50, .w = 150, .h = 100}, 10.f, 2.f, 0.1f);

	if (Game_GetVar(GAME_VAR_DRAW_HUD)->data.integer) {
		// Player HUD
		Player_DrawHUD(player);
	}

	// Debug helpers
	//CNM_RECT dr = {RENDERER_WIDTH / 2 - 16, RENDERER_HEIGHT / 2 - 48, 32, 48};
	//Renderer_DrawRect(&dr, Renderer_MakeColor(255, 0, 0), 0, RENDERER_LIGHT);
	if (Game_GetVar(GAME_VAR_SHOW_OBJGRID)->data.integer)
	{
		for (x = camx / (int)OBJGRID_SIZE - 1; x < (camx + RENDERER_WIDTH) / (int)OBJGRID_SIZE + 1; x++)
		{
			for (y = camy / (int)OBJGRID_SIZE - 1; y < (camy + RENDERER_WIDTH) / (int)OBJGRID_SIZE + 1; y++)
			{
				if (x < 0 || y < 0 || x > 256 || y > 256)
					continue;
				Util_SetRect
				(
					&r,
					x * (int)OBJGRID_SIZE - camx,
					y * (int)OBJGRID_SIZE - camy,
					(int)OBJGRID_SIZE,
					(int)OBJGRID_SIZE
				);
				Renderer_DrawEmptyRect(&r, RCOL_BLUE, 0, RENDERER_LIGHT);

				// Grid position helpers
				if (Game_GetVar(GAME_VAR_SHOW_GRIDPOS)->data.integer)
				{
					Renderer_DrawText
					(
						x * (int)OBJGRID_SIZE - camx,
						y * (int)OBJGRID_SIZE - camy,
						0,
						RENDERER_LIGHT,
						"(%d, %d)",
						x, y
					);
				}
			}
		}
	}

	// Shows the camera's position
	if (Game_GetVar(GAME_VAR_SHOWPOS)->data.integer)
	{
		Renderer_DrawText(8, RENDERER_HEIGHT - 8, 0, RENDERER_LIGHT, "CAM POS (%d, %d)", camx, camy);
	}

	// Show the average incoming/outgoing bits per second
	if (Game_GetVar(GAME_VAR_SHOW_BANDWIDTH)->data.integer && mode != WORLD_MODE_SINGLEPLAYER)
	{
		Renderer_DrawText(8, RENDERER_HEIGHT - 8, 0, RENDERER_LIGHT, "AVG INCOMING (KILOBITS/SEC) %f", (float)Net_GetAvgUDPIncomingBandwidth() * 8.0f / 1024.0f);
		Renderer_DrawText(8, RENDERER_HEIGHT - 16, 0, RENDERER_LIGHT, "AVG OUTGOING (KILOBITS/SEC) %f", (float)Net_GetAvgUDPOutgoingBandwidth() * 8.0f / 1024.0f);
		Renderer_DrawText(8, RENDERER_HEIGHT - 24, 0, RENDERER_LIGHT, "AVG BYTES PER INCOMING WOBJ %f", netgame_avgbytes_per_wobj());
		if (mode == WORLD_MODE_CLIENT) {
			Renderer_DrawText(8, RENDERER_HEIGHT - 40, 0, RENDERER_LIGHT, "AVG BYTES INCOMING SERVER UNOWNED V");
			Renderer_DrawText(8, RENDERER_HEIGHT - 32, 0, RENDERER_LIGHT, "                              %f",netgame_avgbytes_per_server_unowned_wobj_from_client());
		}

		Renderer_DrawText(128, 8, 0, RENDERER_LIGHT, "local frame       %d", netgame_get_sendframe());
		if (mode == WORLD_MODE_CLIENT) {
			Renderer_DrawText(128, 16, 0, RENDERER_LIGHT, "remote frame      %d", NetGame_GetNode(0)->frame);
			Renderer_DrawText(128, 24, 0, RENDERER_LIGHT, "remote frame last %d", NetGame_GetNode(0)->last_frame);
		}
		if (mode == WORLD_MODE_HOSTED_SERVER)
		{
			Renderer_DrawText(128-32, 16, 0, RENDERER_LIGHT, "remote frame      %d", NetGame_GetNode(1)->frame);
			Renderer_DrawText(128-32, 24, 0, RENDERER_LIGHT, "remote frame last %d", NetGame_GetNode(1)->last_frame);
		}
	}

	// If in a netgame, this shows the average amount of frames between each update from the client(s) or server
	if (Game_GetVar(GAME_VAR_CL_TIME_BETWEEN_UPDATES)->data.integer && mode != WORLD_MODE_SINGLEPLAYER)
	{
		i = 0;
		iter = NULL;
		NetGame_Iterate(&iter);
		while (iter != NULL)
		{
			Renderer_DrawText(8, RENDERER_HEIGHT - 16 - ((i++) * 8), 0, RENDERER_LIGHT, "AVG. FRAMES BETWEEN UPDATES (%d): %f", iter->id, iter->avgframes_between_updates);
			NetGame_Iterate(&iter);
		}
	}

	if (Game_GetVar(GAME_VAR_DRAW_FG)->data.integer) {
		// Boss Bar
		BossBar_Draw();

		// Other MISC. Things to draw
		Dialoge_Draw();
		//Fadeout_ApplyFade();
		EndingText_Draw();
	}

	// Ending Text
	if (title_card_timer-- > 0)
	{
		int trans = 0;
		Renderer_SetFont(0, 544, 16, 16);
		if (title_card_timer < 8)
			trans = 7 - title_card_timer;
		for (int i = 0; i < title_card_num_lines; i++)
		{
			Renderer_DrawText
			(
				(RENDERER_WIDTH / 2) - (strlen(title_card_strings[i]) * 8),
				(128 - (title_card_num_lines / 2 * 16)) + (i * 16),
				trans,
				RENDERER_LIGHT,
				title_card_strings[i]
			);
		}
		Renderer_SetFont(256, 192, 8, 8);
	}
}

bool world_check_pt(const float x, const float y, const struct wobj_coll_query query) {
    return Blocks_IsCollidingWithSolid(&(const CNM_BOX){ .x = x, .y = y, .w = 1.0f, .h = 1.0f }, true) ||
        wobj_check_pt(x, y, query);
}
