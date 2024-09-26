#include <string.h>
#include <math.h>
#include "player_spawns.h"
#include "utility.h"
#include "mem.h"
#include "game.h"

static int spawning_mode = PLAYER_SPAWN_TYPE_NORMAL_MODES;
static float (*spawns_x)[PLAYER_SPAWNS_MAX];
static float (*spawns_y)[PLAYER_SPAWNS_MAX];

void PlayerSpawns_Init(void)
{
	spawning_mode = PLAYER_SPAWN_TYPE_NORMAL_MODES;
	spawns_x = arena_global_alloc(sizeof(*spawns_x) * PLAYER_SPAWN_TYPE_MAX);
	memset(spawns_x, 0, sizeof(*spawns_x) * PLAYER_SPAWN_TYPE_MAX);
	spawns_y = arena_global_alloc(sizeof(*spawns_y) * PLAYER_SPAWN_TYPE_MAX);
	memset(spawns_y, 0, sizeof(*spawns_y) * PLAYER_SPAWN_TYPE_MAX);
	for (int i = 0; i < PLAYER_SPAWN_TYPE_MAX * PLAYER_SPAWNS_MAX; i++)
	{
		spawns_x[i / PLAYER_SPAWNS_MAX][i % PLAYER_SPAWNS_MAX] = INFINITY;
		spawns_y[i / PLAYER_SPAWNS_MAX][i % PLAYER_SPAWNS_MAX] = INFINITY;
	}
}
void PlayerSpawns_SetMode(int mode)
{
	spawning_mode = PLAYER_SPAWN_TYPE_NORMAL_MODES;
}
void PlayerSpawns_SetSpawnLoc(int index, float x, float y)
{
	spawns_x[index / PLAYER_SPAWNS_MAX][index % PLAYER_SPAWNS_MAX] = x;
	spawns_y[index / PLAYER_SPAWNS_MAX][index % PLAYER_SPAWNS_MAX] = y;
}
void PlayerSpawn_SetWobjLoc(float *coords)
{
	int index = Game_GetVar(GAME_VAR_SPECIAL_ENTRANCE)->data.integer ? 1 : 0;
	//int index = Util_RandInt(0, PLAYER_SPAWNS_MAX - 1);
	//int i = 0;
	//while (spawns_x[spawning_mode][index] == INFINITY)
	//{
	//	index = Util_RandInt(0, PLAYER_SPAWNS_MAX - 1);
	//	if (i++ > 255)
	//		break;
	//}
	//if (i > 255)
	//{
	//	index = 0;
	//	for (i = 0; i < 256; i++)
	//	{
	//		if (spawns_x[spawning_mode][i] != INFINITY)
	//		{
	//			index = i;
	//			break;
	//		}
	//	}
	//}
	coords[0] = spawns_x[spawning_mode][index];
	coords[1] = spawns_y[spawning_mode][index];
}
float PlayerSpawn_GetSpawnLocX(int index)
{
	return spawns_x[index / PLAYER_SPAWNS_MAX][index % PLAYER_SPAWNS_MAX];
}
float PlayerSpawn_GetSpawnLocY(int index)
{
	return spawns_y[index / PLAYER_SPAWNS_MAX][index % PLAYER_SPAWNS_MAX];
}
int PlayerSpawn_GetUnusedSpawnId(int mode)
{
	int i;
	for (i = 0; i < PLAYER_SPAWNS_MAX; i++)
	{
		if (spawns_x[mode][i] == INFINITY)
		{
			return mode * PLAYER_SPAWNS_MAX + i;
		}
	}
	return 0;
}
void PlayerSpawn_ClearSpawnId(int index)
{
	spawns_x[index / PLAYER_SPAWNS_MAX][index % PLAYER_SPAWNS_MAX] = INFINITY;
	spawns_y[index / PLAYER_SPAWNS_MAX][index % PLAYER_SPAWNS_MAX] = INFINITY;
}
void PlayerSpawn_ClearAllSpawns(void) {
	int i;
	for (i = 0; i < PLAYER_SPAWNS_MAX*PLAYER_SPAWN_TYPE_MAX; i++) {
		PlayerSpawn_ClearSpawnId(i);
	}
}
