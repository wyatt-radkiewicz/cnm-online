#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "spawners.h"
#include "console.h"
#include "renderer.h"
#include "wobj.h"
#include "teleport_infos.h"
#include "netgame.h"
#include "mem.h"

#define GET_SPAWNER(i) ((SPAWNER *)((unsigned char *)(i) - offsetof(SPAWNER, grid_object)))

static OBJGRID *spawners_grid;
static SPAWNER *spawners_head;
static SPAWNER *(*spawner_groups)[MAX_SPAWNERS_PER_GROUP];
//static POOL *spawners_pool;
static dynpool_t _pool;
static int spawners_mode;
static int spawners_last_num_players, spawners_num_players, spawners_count_change;

void Spawners_Init(void)
{
	//spawners_pool = Pool_Create(sizeof(SPAWNER));
	_pool = dynpool_init(512, sizeof(SPAWNER), arena_global_alloc);
	spawners_head = NULL;
	spawners_grid = ObjGrid_Create(256, 256);
	spawners_mode = SPAWNER_SINGLEPLAYER;
	spawner_groups = arena_global_alloc(sizeof(*spawner_groups) * MAX_SPAWNER_GROUPS);
	memset(spawner_groups, 0, sizeof(*spawner_groups) * MAX_SPAWNER_GROUPS);

	spawners_last_num_players = 0;
	spawners_num_players = 0;
	spawners_count_change = CNM_FALSE;
}
void Spawners_Quit(void)
{
	Spawners_UnloadSpawners();
	ObjGrid_Destroy(spawners_grid);
	dynpool_deinit(_pool);
	//Pool_Destroy(spawners_pool);
}
void Spawners_UnloadSpawners(void)
{
	SPAWNER *spawner = spawners_head;
	SPAWNER *next;
	ObjGrid_Clear(spawners_grid);
	while (spawner != NULL)
	{
		next = spawner->alloc.next;
		dynpool_free(_pool, spawner);
		//Pool_Free(spawners_pool, spawner);
		spawner = next;
	}
	spawners_head = NULL;
	memset(spawner_groups, 0, sizeof(*spawner_groups) * MAX_SPAWNER_GROUPS);

	dynpool_fast_clear(_pool);
	assert(dynpool_empty(_pool));
}
SPAWNER *Spawners_CreateSpawner(float x, float y, int wobj_type, int duration, int max, char spawner_group)
{
	SPAWNER *spawner = dynpool_alloc(_pool);
	memset(spawner, 0, sizeof(SPAWNER));
	spawner->alloc.next = spawners_head;
	spawner->alloc.last = NULL;
	if (spawner->alloc.next != NULL)
		spawner->alloc.next->alloc.last = spawner;
	spawners_head = spawner;
	spawner->x = x;
	spawner->y = y;
	spawner->duration = duration;
	spawner->max = max;
	spawner->timer = 0;
	spawner->wobj_type = wobj_type;
	spawner->curr_wobjs = 0;
	spawner->has_respawned = CNM_FALSE;
	spawner->grid_object.x = x + (float)wobj_types[wobj_type].frames[0].w / 2.0f;
	spawner->grid_object.y = y + (float)wobj_types[wobj_type].frames[0].h / 2.0f;
	assert(spawner_group >= -1 && spawner_group < MAX_SPAWNER_GROUPS);
	spawner->spawner_group = spawner_group;
	if (spawner->spawner_group > -1) {
		assert(spawner->spawner_group < MAX_SPAWNER_GROUPS);
		for (int i = 0; i < MAX_SPAWNERS_PER_GROUP; i++) {
			if (!spawner_groups[spawner_group][i]) {
				//Console_Print("Adding spawner to group %d at id %d", spawner_group, i);
				//Console_Print("Spawner type: %d", spawner->wobj_type);
				spawner_groups[spawner_group][i] = spawner;
				break;
			}
		}
	}
	ObjGrid_AddObject(spawners_grid, &spawner->grid_object);
	return spawner;
}
SPAWNER *Spawners_GetSpawnerWithinBox(float x, float y, float w, float h)
{
	OBJGRID_ITER iter;
	SPAWNER *spawner;
	CNM_BOX a, b;
	int cx, cy;
	for (cx = (int)(x / OBJGRID_SIZE) - 1; cx <= (int)((x + w) / OBJGRID_SIZE) + 1; cx++)
	{
		for (cy = (int)(y / OBJGRID_SIZE) - 1; cy <= (int)((y + h) / OBJGRID_SIZE) + 1; cy++)
		{
			ObjGrid_MakeIter(spawners_grid, cx, cy, &iter);
			while (iter != NULL)
			{
				spawner = GET_SPAWNER(iter);
				if (spawner->wobj_type)
				{
					Util_SetBox(&a, x, y, w, h);
					b.x = spawner->x;
					b.y = spawner->y;
					b.w = (float)wobj_types[spawner->wobj_type].frames[0].w;
					b.h = (float)wobj_types[spawner->wobj_type].frames[0].h;
					if (Util_AABBCollision(&a, &b))
					{
						return spawner;
					}
				}
				ObjGrid_AdvanceIter(&iter);
			}
		}
	}

	return NULL;
}
int Spawners_GetSpawnersWithinBox(SPAWNER **buf, int buflen, float x, float y, float w, float h) {
	OBJGRID_ITER iter;
	SPAWNER *spawner;
	CNM_BOX a, b;
	int cx, cy, len = 0;
	for (cx = (int)(x / OBJGRID_SIZE) - 1; cx <= (int)((x + w) / OBJGRID_SIZE) + 1; cx++)
	{
		for (cy = (int)(y / OBJGRID_SIZE) - 1; cy <= (int)((y + h) / OBJGRID_SIZE) + 1; cy++)
		{
			ObjGrid_MakeIter(spawners_grid, cx, cy, &iter);
			while (iter != NULL)
			{
				spawner = GET_SPAWNER(iter);
				if (spawner->wobj_type)
				{
					Util_SetBox(&a, x, y, w, h);
					b.x = spawner->x;
					b.y = spawner->y;
					b.w = (float)wobj_types[spawner->wobj_type].frames[0].w;
					b.h = (float)wobj_types[spawner->wobj_type].frames[0].h;
					if (Util_AABBCollision(&a, &b))
					{
						buf[len++] = spawner;
						if (len >= buflen) return len;
					}
				}
				ObjGrid_AdvanceIter(&iter);
			}
		}
	}

	return len;
}
void Spawners_MoveSpawner(SPAWNER *spawner, float newx, float newy)
{
	ObjGrid_MoveObject(spawners_grid, &spawner->grid_object, newx, newy);
	spawner->x = newx;
	spawner->y = newy;
}
void Spawners_DestroySpawner(SPAWNER *spawner)
{
	if (spawner->alloc.last != NULL)
		spawner->alloc.last->alloc.next = spawner->alloc.next;
	else
		spawners_head = spawner->alloc.next;
	if (spawner->alloc.next != NULL)
		spawner->alloc.next->alloc.last = spawner->alloc.last;
	if (spawner->spawner_group != -1) {
		for (int i = 0; i < MAX_SPAWNERS_PER_GROUP; i++) {
			if (spawner_groups[spawner->spawner_group][i] == spawner) {
				spawner_groups[spawner->spawner_group][i] = NULL;
				break;
			}
		}
	}
	ObjGrid_RemoveObject(spawners_grid, &spawner->grid_object);
	//Pool_Free(spawners_pool, spawner);
	dynpool_free(_pool, spawner);
}
void Spawners_DrawSpawners(int camx, int camy)
{
	SPAWNER *spawner;
	OBJGRID_ITER iter;
	int x, y;
	for (x = camx / (int)OBJGRID_SIZE - 1; x < (camx + RENDERER_WIDTH) / (int)OBJGRID_SIZE + 1; x++)
	{
		for (y = camy / (int)OBJGRID_SIZE - 1; y < (camy + RENDERER_WIDTH) / (int)OBJGRID_SIZE + 1; y++)
		{
			ObjGrid_MakeIter(spawners_grid, x, y, &iter);
			while (iter != NULL)
			{
				spawner = GET_SPAWNER(iter);
				if (spawner->wobj_type != WOBJ_NULL && spawner->wobj_type != WOBJ_DROPPED_ITEM)
				{
					Renderer_DrawBitmap
					(
						(int)spawner->x - camx,
						(int)spawner->y - camy,
						wobj_types[spawner->wobj_type].frames,
						0,
						RENDERER_LIGHT
					);
				}
				if (spawner->wobj_type == WOBJ_DROPPED_ITEM)
				{
					CNM_RECT dir;
					Util_SetRect(&dir, 352, 2080, 32, 32);
					Renderer_DrawBitmap
					(
						(int)spawner->x - camx,
						(int)spawner->y - camy,
						&dir,
						0,
						RENDERER_LIGHT
					);
					Renderer_DrawBitmap
					(
						(int)spawner->x - camx,
						(int)spawner->y - camy,
						&wobj_types[spawner->wobj_type].frames[spawner->custom_int],
						0,
						RENDERER_LIGHT
					);
				}
				if (spawner->wobj_type == WOBJ_TELEPORT)
				{
					Renderer_DrawText
					(
						(int)spawner->x - camx,
						(int)spawner->y - 12 - camy,
						0,
						RENDERER_LIGHT,
						"%s: $%d",
						TeleportInfos_GetTeleport(spawner->custom_int)->name,
						TeleportInfos_GetTeleport(spawner->custom_int)->cost
					);
				}
				if (spawner->max > 0)
				{
					char respawn_text[UTIL_MAX_TEXT_WIDTH];
					//strcpy(respawn_text, "R");
					if (spawner->duration <= 0)
						sprintf(respawn_text, "R-I");
					else if (spawner->duration < (120 * 30))
						sprintf(respawn_text, "%1.1fs", (float)spawner->duration / 30.0f);
					else
						sprintf(respawn_text, "%1.1fm", (float)spawner->duration / (30.0f * 60.0f));

					Renderer_DrawText
					(
						(int)spawner->x - camx,
						(int)spawner->y - 4 - camy,
						0,
						RENDERER_LIGHT,
						respawn_text
					);
				}
				ObjGrid_AdvanceIter(&iter);
			}
		}
	}
}
SPAWNER *Spawners_Iterate(SPAWNER *iter)
{
	if (iter == NULL)
	{
		return spawners_head;
	}
	else
	{
		return iter->alloc.next;
	}
}
static int spawner_spawn(SPAWNER *spawner, int spawn_grouped) {
	if (spawner->spawner_group > -1 &&
		spawner_groups[spawner->spawner_group][0] != spawner &&
		!spawn_grouped) return CNM_FALSE;
	int create_count = 1;
	if (spawners_mode == SPAWNER_MULTIPLAYER && SPAWNER_GET_SPAWN_MODE(spawner) == SPAWNER_MODE_PLAYER_COUNTED)
	{
		create_count = spawners_num_players;
		if (spawn_grouped) goto skip_checks;
		if (spawner->curr_wobjs + create_count > spawner->max*create_count && spawner->max != 0)
			return CNM_FALSE;
	}
	else
	{
		if (spawn_grouped) goto skip_checks;
		if (spawner->curr_wobjs >= spawner->max && spawner->max != 0)
			return CNM_FALSE;
	}
	spawner->timer--;
	if (SPAWNER_GET_SPAWN_MODE(spawner) == SPAWNER_MODE_NOSPAWN)
		return CNM_FALSE;
	if (SPAWNER_GET_SPAWN_MODE(spawner) == SPAWNER_MODE_MULTIPLAYER_ONLY &&
		spawners_mode == SPAWNER_SINGLEPLAYER)
		return CNM_FALSE;
	if (SPAWNER_GET_SPAWN_MODE(spawner) == SPAWNER_MODE_SINGLEPLAYER_ONLY &&
		spawners_mode == SPAWNER_MULTIPLAYER)
		return CNM_FALSE;
	if (spawner->timer > 0)
		return CNM_FALSE;
	if (!wobj_types[spawner->wobj_type].respawnable && spawner->has_respawned)
		return CNM_FALSE;
	if (spawner->max == 0 && spawner->has_respawned)
		return CNM_FALSE;

	/* Spawn a thing */
skip_checks:
	for (int i = 0; i < create_count; i++)
	{
		//if (spawner->wobj_type == WOBJ_CUSTOMIZEABLE_MOVEABLE_PLATFORM) Console_Print("%u -> ci", spawner->custom_int);
		WOBJ *created = Wobj_CreateOwned(spawner->wobj_type, spawner->x, spawner->y, spawner->custom_int, spawner->custom_float);
		created->parent_spawner = spawner;
		//created->dropped_death_item = spawner->dropped_item;
		created->dropped_death_item = SPAWNER_GET_DROPPED_ITEM(spawner);
		spawner->curr_wobjs++;
		if (wobj_types[spawner->wobj_type].respawnable)
			spawner->timer = spawner->duration;
		created->flags |= spawner->has_respawned ? 0 : WOBJ_AWARD_SCORE;
		spawner->has_respawned = CNM_TRUE;
	}
	return CNM_TRUE;
}
void Spawners_CreateWobjsFromSpawners(int gridx, int gridy)
{
	OBJGRID_ITER iter;
	SPAWNER *spawner;
	WOBJ *created;
	int create_count;
	ObjGrid_MakeIter(spawners_grid, gridx, gridy, &iter);
	while (iter != NULL)
	{
		spawner = GET_SPAWNER(iter);
		if (spawner_spawn(spawner, CNM_FALSE) && spawner->spawner_group != -1) {
			// follow groups
			for (int i = 1; i < MAX_SPAWNERS_PER_GROUP; i++) {
				SPAWNER *const grouped = spawner_groups[spawner->spawner_group][i];
				//if (grouped == spawner) Console_Print("imposible spawn?");
				if (grouped) {
					spawner_spawn(grouped, CNM_TRUE);
					//Console_Print("initiated group spawn");
				}
			}
		}
		ObjGrid_AdvanceIter(&iter);
	}
}
void Spawners_CreateAllWobjsFromSpawners(void)
{
	int x, y;
	for (x = 0; x < 256; x++)
	{
		for (y = 0; y < 256; y++)
		{
			Spawners_CreateWobjsFromSpawners(x, y);
		}
	}
}

static void Spawners_UpdateSpawnerBasedOnNewPlayerCount(SPAWNER *spawner)
{
	if (spawners_num_players > spawners_last_num_players)
	{
		if (!wobj_types[spawner->wobj_type].respawnable && spawner->has_respawned)
			return;
		if (spawner->max == 0 && spawner->has_respawned)
			return;

		WOBJ *created;
		int create_count = (spawner->curr_wobjs / spawners_num_players) * spawners_num_players;
		if (spawner->curr_wobjs % spawners_num_players != 0)
			create_count += spawners_num_players;
		create_count -= spawner->curr_wobjs;
		for (int i = 0; (i < create_count) && (spawner->curr_wobjs < spawner->max * spawners_num_players); i++)
		{
			created = Wobj_CreateOwned(spawner->wobj_type, spawner->x, spawner->y, spawner->custom_int, spawner->custom_float);
			created->parent_spawner = spawner;
			//created->dropped_death_item = spawner->dropped_item;
			created->dropped_death_item = SPAWNER_GET_DROPPED_ITEM(spawner);
			spawner->curr_wobjs++;
			if (wobj_types[spawner->wobj_type].respawnable)
				spawner->timer = spawner->duration;
			spawner->has_respawned = CNM_TRUE;
		}
	}
}
static void Spawners_UpdatePlayerCountBasedSpawnersWithNewCount(void)
{
	int x, y;
	for (x = 0; x < 128; x++)
	{
		for (y = 0; y < 128; y++)
		{
			OBJGRID_ITER iter;
			SPAWNER *spawner;
			ObjGrid_MakeIter(spawners_grid, x, y, &iter);
			while (iter != NULL)
			{
				spawner = GET_SPAWNER(iter);
				if (spawners_mode == SPAWNER_MULTIPLAYER && SPAWNER_GET_SPAWN_MODE(spawner) == SPAWNER_MODE_PLAYER_COUNTED)
				{
					Spawners_UpdateSpawnerBasedOnNewPlayerCount(spawner);
				}
				ObjGrid_AdvanceIter(&iter);
			}
		}
	}
}

void Spawners_SetGlobalMode(int mode)
{
	spawners_mode = mode;
}
void Spawners_MultiModeArbitraryTick(void)
{
	if (spawners_mode != SPAWNER_MULTIPLAYER)
		return;

	spawners_last_num_players = spawners_num_players;
	spawners_num_players = NetGame_GetNumActiveNodes();
	if (spawners_num_players != spawners_last_num_players)
	{
		spawners_count_change = CNM_TRUE;
		Spawners_UpdatePlayerCountBasedSpawnersWithNewCount();
	}
	else
	{
		spawners_count_change = CNM_FALSE;
	}
}
