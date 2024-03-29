#ifndef _spawners_h_
#define _spawners_h_
#include "obj_grid.h"
#include "utility.h"

#define SPAWNER_DROPPED_ITEM_MASK 0x00ffffff
#define SPAWNER_SPAWN_MODE_MASK 0xff000000
#define SPAWNER_SPAWN_MODE_SHIFT 24
#define SPAWNER_GET_DROPPED_ITEM(s) ((s)->dropped_item & SPAWNER_DROPPED_ITEM_MASK)
#define SPAWNER_GET_SPAWN_MODE(s) (((s)->dropped_item & SPAWNER_SPAWN_MODE_MASK) >> SPAWNER_SPAWN_MODE_SHIFT)
#define SPAWNER_SET_DROPPED_ITEM(s, item) ((s)->dropped_item = (s)->dropped_item & ~SPAWNER_DROPPED_ITEM_MASK | (item))
#define SPAWNER_SET_SPAWN_MODE(s, mode) ((s)->dropped_item = (((s)->dropped_item & ~SPAWNER_SPAWN_MODE_MASK) | ((mode) << SPAWNER_SPAWN_MODE_SHIFT)))

#define SPAWNER_MODE_MULTI_AND_SINGLE_PLAYER 0
#define SPAWNER_MODE_SINGLEPLAYER_ONLY (1 << 4)
#define SPAWNER_MODE_MULTIPLAYER_ONLY (2 << 4)
#define SPAWNER_MODE_PLAYER_COUNTED (3 << 4)
#define SPAWNER_MODE_NOSPAWN 4

#define SPAWNER_SINGLEPLAYER 0
#define SPAWNER_MULTIPLAYER 1

#define SPAWNER_SERIALIZATION_SIZE 76

#define MAX_SPAWNER_GROUPS 32
#define MAX_SPAWNERS_PER_GROUP 16

typedef struct _SPAWNER
{
	float x, y;
	int wobj_type;
	int duration;
	int max;
	int timer;
	int custom_int;
	float custom_float;
	int curr_wobjs;
	int has_respawned;
	unsigned int dropped_item;
	char spawner_group;

	struct
	{
		struct _SPAWNER *next, *last;
	} alloc;
	OBJGRID_OBJECT grid_object;
} SPAWNER;

void Spawners_Init(void);
void Spawners_Quit(void);
void Spawners_UnloadSpawners(void);
SPAWNER *Spawners_CreateSpawner(float x, float y, int wobj_type, int duration, int max, char spawner_group);
SPAWNER *Spawners_GetSpawnerWithinBox(float x, float y, float w, float h);
int Spawners_GetSpawnersWithinBox(SPAWNER **buf, int buflen, float x, float y, float w, float h);
void Spawners_MoveSpawner(SPAWNER *spawner, float newx, float newy);
void Spawners_DestroySpawner(SPAWNER *spawner);
void Spawners_DrawSpawners(int camx, int camy);
SPAWNER *Spawners_Iterate(SPAWNER *iter);
void Spawners_CreateWobjsFromSpawners(int gridx, int gridy);
void Spawners_CreateAllWobjsFromSpawners(void);

void Spawners_SetGlobalMode(int mode);
void Spawners_MultiModeArbitraryTick(void);

#endif
