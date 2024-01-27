#ifndef _player_spawns_h_
#define _player_spawns_h_
#include "netgame.h"

#define PLAYER_SPAWNS_MAX 48
#define OLD_PLAYER_SPAWNS_MAX 128
#define PLAYER_SPAWN_TYPE_NORMAL_MODES			0
#define PLAYER_SPAWN_TYPE_SINGLEPLAYER_ONLY		1
#define PLAYER_SPAWN_TYPE_CHECKPOINTS			2
#define PLAYER_SPAWN_TYPE_MAX					3

void PlayerSpawns_Init(void);
void PlayerSpawns_SetMode(int mode);
void PlayerSpawns_SetSpawnLoc(int index, float x, float y);
void PlayerSpawn_SetWobjLoc(float *coords);
float PlayerSpawn_GetSpawnLocX(int index);
float PlayerSpawn_GetSpawnLocY(int index);
int PlayerSpawn_GetUnusedSpawnId(int mode);
void PlayerSpawn_ClearSpawnId(int index);
void PlayerSpawn_ClearAllSpawns(void);

#endif
