#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "console.h"
#include "interaction.h"
#include "wobj.h"
#include "obj_grid.h"
#include "renderer.h"
#include "blocks.h"
#include "teleport_infos.h"
#include "game.h"
#include "netgame.h"
#include "spawners.h"
#include "player.h"
#include "mem.h"
#include "world.h"

#define GET_WOBJ(iter) ((WOBJ *)((unsigned char *)(iter) - offsetof(WOBJ, internal.obj)))
//#define WOBJ_SEARCH_MAP_SIZE (1 << 11)
#define SMAPSZ 64
#define DELOWNSZ 128
#define NOVERLAYERS 64

// Debug prints
#define DBGLRU 0

//static POOL *wobj_pool;
static OBJGRID *owned_grid;
static OBJGRID *unowned_grid;
static int owned_uuid;
static int wobj_node_id;
static WOBJ **deleted_owned;
static int deleted_size;
static int major_reset;

static dynpool_t _wobjpool;
static WOBJ *owned;
static WOBJ *unowned;
static int unowned_size;

static WOBJ **overlayer_draws;
static int noverlayer_draws;

//static WOBJ *search_map[WOBJ_SEARCH_MAP_SIZE];
typedef struct smaplru {
	struct smaplru *last, *next;
	WOBJ *wobj;
} smaplru_t;
typedef struct smapent {
	int psl;
	smaplru_t *lru;
} smapent_t;
static smapent_t *_smap_owned, *_smap_unowned;
static int _smap_owned_sz, _smap_unowned_sz;

static smaplru_t *_lru_owned_head, *_lru_owned_tail;
static smaplru_t *_lru_unowned_head, *_lru_unowned_tail;
static fixedpool_t *_lru_owned_pool;
static fixedpool_t *_lru_unowned_pool;

static void Wobj_FreeOwnedWobjAndRemove(WOBJ *wobj);

static WOBJ *WobjSearch_FindEntry(int node, int uuid);
static void WobjSearch_DestoryEntry(int node, int uuid);
static void WobjSearch_Reset(void);
static void WobjSearch_DestroyUnownedEntries(void);

#define LRU_POOL_SZ (sizeof(fixedpool_t) + (SMAPSZ + 1) * sizeof(smaplru_t))

void Wobj_Init(void)
{
	overlayer_draws = arena_global_alloc(sizeof(*overlayer_draws) * NOVERLAYERS);
	noverlayer_draws = 0;
	_lru_owned_pool = arena_global_alloc(LRU_POOL_SZ);
	_lru_unowned_pool = arena_global_alloc(LRU_POOL_SZ);
	_smap_owned = arena_global_alloc(sizeof(*_smap_owned) * SMAPSZ);
	_smap_unowned = arena_global_alloc(sizeof(*_smap_unowned) * SMAPSZ);
	unowned = arena_global_alloc(sizeof(*unowned) * WOBJ_MAX_UNOWNED_WOBJS);
	memset(unowned, 0, sizeof(*unowned) * WOBJ_MAX_UNOWNED_WOBJS);
	deleted_owned = arena_global_alloc(sizeof(*deleted_owned) * DELOWNSZ);
	memset(deleted_owned, 0, sizeof(*deleted_owned) * DELOWNSZ);
	deleted_size = 0;
	owned = NULL;
	owned_uuid = 0;
	wobj_node_id = 0;
	unowned_size = 0;
	major_reset = CNM_FALSE;
	owned_grid = ObjGrid_Create(256, 256);
	unowned_grid = ObjGrid_Create(256, 256);
	
	fixedpool_init(_lru_owned_pool, sizeof(smaplru_t), LRU_POOL_SZ);
	fixedpool_init(_lru_unowned_pool, sizeof(smaplru_t), LRU_POOL_SZ);
	WobjSearch_Reset();
	_wobjpool = dynpool_init(512, sizeof(WOBJ), arena_global_alloc);
	//wobj_pool = Pool_Create(sizeof(WOBJ));
}
void Wobj_Quit(void)
{
	owned = NULL;
	ObjGrid_Destroy(owned_grid);
	ObjGrid_Destroy(unowned_grid);
	owned_grid = NULL;
	unowned_grid = NULL;
	dynpool_deinit(_wobjpool);
	//Pool_Destroy(wobj_pool);
	//wobj_pool = NULL;
}
void Wobj_SetNodeId(int node)
{
	WobjSearch_Reset();
	wobj_node_id = node;
}
int Wobj_GetNodeId(void)
{
	return wobj_node_id;
}
WOBJ *Wobj_CreateOwned(int type, float x, float y, int ci, float cf)
{
	int i;

	if (major_reset)
		return NULL;

	WOBJ *wobj = dynpool_alloc(_wobjpool);

	memset(wobj, 0, sizeof(WOBJ));
	wobj->x = x;
	wobj->y = y;
	wobj->type = type;
	wobj->node_id = wobj_node_id;
	wobj->uuid = owned_uuid++;
	wobj->last_sound_played = 0;
	wobj->last_sound_uuid = -1;
	wobj->link_node = 0;
	wobj->link_uuid = 0;
	
	wobj->internal.owned = CNM_TRUE;
	wobj->internal.obj.x = x;
	wobj->internal.obj.y = y;
	wobj->internal.last = NULL;
	wobj->internal.next = owned;
	owned = wobj;
	if (wobj->internal.next != NULL)
		wobj->internal.next->internal.last = wobj;

	wobj->custom_ints[0] = ci;
	wobj->custom_floats[0] = cf;
	wobj->interpolate = CNM_FALSE;
	wobj->parent_spawner = NULL;
	ObjGrid_AddObject(owned_grid, &wobj->internal.obj);

	for (i = 0; i < NETGAME_MAX_HISTORY; i++) {
		wobj->history_frames[i] = -1;
		memcpy(wobj->history + i, (struct wobjdata *)wobj, sizeof(struct wobjdata));
	}

	//if (type == WOBJ_CUSTOMIZEABLE_MOVEABLE_PLATFORM)
	//	Console_Print("ci: %d", ci);
	if (wobj_types[type].create != NULL)
		wobj_types[type].create(wobj);
	return wobj;
}
WOBJ *Wobj_CreateUnowned(int type, float x, float y, int frame, int flags, int ci, float cf, int node_id, int uuid, int run_create)
{
	WOBJ *wobj = NULL;
	int i;
	
	if (unowned_size < WOBJ_MAX_UNOWNED_WOBJS)
		wobj = &unowned[unowned_size++];
	else
	{
		int i;
		for (i = 0; i < WOBJ_MAX_UNOWNED_WOBJS; i++)
		{
			if (unowned[i].type == WOBJ_NULL)
			{
				wobj = &unowned[i];
				break;
			}
		}
		if (i == WOBJ_MAX_UNOWNED_WOBJS)
			return NULL;
	}
	memset(wobj, 0, sizeof(WOBJ));
	wobj->type = type;
	wobj->x = x;
	wobj->y = y;
	wobj->anim_frame = frame;
	wobj->flags = flags;
	wobj->node_id = node_id;
	wobj->uuid = uuid;
	wobj->last_sound_played = 0;
	wobj->last_sound_uuid = -1;
	wobj->custom_ints[0] = ci;
	wobj->custom_floats[0] = cf;
	wobj->internal.last = NULL;
	wobj->internal.next = NULL;
	wobj->internal.owned = CNM_FALSE;
	wobj->internal.obj.x = x;
	wobj->internal.obj.y = y;
	wobj->parent_spawner = NULL;
	wobj->interpolate = CNM_FALSE;
	wobj->interp_frame = 0;
	wobj->smooth_x = x;
	wobj->smooth_y = y;

	for (i = 0; i < NETGAME_MAX_HISTORY; i++)
	{
		wobj->history_frames[i] = -1;
		memcpy(wobj->history + i, (struct wobjdata *)wobj, sizeof(struct wobjdata));
	}

	ObjGrid_AddObject(unowned_grid, &wobj->internal.obj);
	if (wobj_types[type].create != NULL && run_create)
		wobj_types[type].create(wobj);
	return wobj;
}
void Wobj_RecreateUnwoned(WOBJ *wobj, float tx, float ty) {
	wobj->interpolate = CNM_FALSE;
	wobj->interp_frame = 0;
	wobj->smooth_x = tx;
	wobj->smooth_y = ty;
}
void Wobj_DestroyWobj(WOBJ *wobj)
{
	if (wobj != NULL)
		WobjSearch_DestoryEntry(wobj->node_id, wobj->uuid);
	else
		return;

	if (wobj->internal.owned)
	{
		if (deleted_size >= DELOWNSZ) return;
		for (int i = 0; i < deleted_size; i++)
		{
			if (deleted_owned[i] == wobj)
				return;
		}
		deleted_owned[deleted_size++] = wobj;
	}
	else
	{
		wobj->type = 0;
		ObjGrid_RemoveObject(unowned_grid, &wobj->internal.obj);
		memset(wobj, 0, sizeof(WOBJ));
	}
}
static void Wobj_FreeOwnedWobjAndRemove(WOBJ *wobj)
{
	if (wobj->dropped_death_item != 0 && !major_reset) {
		float x = wobj->x, y = wobj->y;
		if (wobj->hitbox.w > 0.0f) x += wobj->hitbox.x + wobj->hitbox.w / 2.0f;
		if (wobj->hitbox.h > 0.0f) y += wobj->hitbox.y + wobj->hitbox.h / 2.0f;
		Wobj_CreateOwned(WOBJ_DROPPED_ITEM, x - 16.0f, y - 16.0f, wobj->dropped_death_item, 0.0f);
	}

	if (wobj->on_destroy != NULL)
		wobj->on_destroy(wobj);
	if (wobj->parent_spawner != NULL)
		wobj->parent_spawner->curr_wobjs--;
	if (wobj->internal.next != NULL)
		wobj->internal.next->internal.last = wobj->internal.last;
	if (wobj->internal.last != NULL)
		wobj->internal.last->internal.next = wobj->internal.next;
	else
		owned = wobj->internal.next;
	WobjSearch_DestoryEntry(wobj->node_id, wobj->uuid);
	ObjGrid_RemoveObject(owned_grid, &wobj->internal.obj);
	dynpool_free(_wobjpool, wobj);
}
void Wobj_DestroyOwnedObjectsFromLastFrame(void)
{
	int i;
	for (i = 0; i < deleted_size; i++)
	{
		Wobj_FreeOwnedWobjAndRemove(deleted_owned[i]);
	}
	deleted_size = 0;
}
void Wobj_DestroyUnownedWobjs(void)
{
	unowned_size = 0;
	WobjSearch_DestroyUnownedEntries();
	ObjGrid_FastClear(unowned_grid);
}
void Wobj_DestroyOwnedWobjs(void)
{
#if DBGLRU >= 1
	smaplru_t *lru = _lru_owned_head;
	while (lru) {
		smaplru_t *const next = lru->next;
		Console_Print("lru %d %d", lru->wobj->node_id, lru->wobj->uuid);
		lru = next;
	}
#endif

	WOBJ *wobj, *next;
	wobj = owned;
	major_reset = CNM_TRUE;
	while (wobj != NULL)
	{
		next = wobj->internal.next;
		Wobj_FreeOwnedWobjAndRemove(wobj);
		wobj = next;
	}
	owned = NULL;
	deleted_size = 0;
	major_reset = CNM_FALSE;
	WobjSearch_Reset();
}
void WobjDbg_CheckMemLeaks(void) {
	assert(dynpool_empty(_wobjpool));
}
static void update_owned_wobj(WOBJ *wobj) {
	if (wobj_types[wobj->type].update != NULL)
	{
		wobj_types[wobj->type].update(wobj);
		ObjGrid_MoveObject(owned_grid, &wobj->internal.obj, wobj->x, wobj->y);
		if (Game_GetFrame() % 3 == 0)
			wobj->flags &= ~WOBJ_DAMAGE_INDICATE;
	}
}
void Wobj_UpdateOwnedWobjs(void)
{
	WOBJ *rover, *next, *player = NULL;
	/* Update only owned objects */
	rover = owned;
	while (rover != NULL)
	{
		next = rover->internal.next;
		if (rover->type == WOBJ_PLAYER) player = rover;
		else update_owned_wobj(rover);
		rover = next;
	}

	if (player) update_owned_wobj(player);
}

void Wobj_RecordWobjHistory(int newframe) {
	WOBJ *rover, *next;
	int local_index, i, j;
	local_index = newframe % NETGAME_MAX_HISTORY;
	rover = owned;
	while (rover != NULL)
	{
		next = rover->internal.next;
		memcpy(rover->history + local_index, (struct wobjdata *)rover, sizeof(struct wobjdata));
		rover->history_frames[local_index] = newframe;
		for (i = 0; i < NETGAME_MAX_NODES; i++) {
			rover->nodes_sent_to[i] &= ~(1 << local_index);
		}
		rover = next;
	}
	if (wobj_node_id != 0) return;
	// Now record history for unowned objects
	for (j = 0; j < WOBJ_MAX_UNOWNED_WOBJS; j++) {
		if (unowned[j].type && unowned[j].internal.obj.chunk != NULL) {
			memcpy(unowned[j].history + local_index, (struct wobjdata *)&unowned[j], sizeof(struct wobjdata));
			unowned[j].history_frames[local_index] = newframe;
			for (i = 0; i < NETGAME_MAX_NODES; i++)
			{
				unowned[j].nodes_sent_to[i] &= ~(1 << local_index);
			}
		}
	}
}
void Wobj_MarkForBeingSent(WOBJ *wobj, int to_node, int frame) {
	int local_index;
	/* Record the histroy of only owned objects */
	local_index = frame % NETGAME_MAX_HISTORY;
	wobj->nodes_sent_to[to_node] |= (1 << local_index);
}
int Wobj_HasHistoryForNode(WOBJ *wobj, int node, int frame) {
	int local_index;
	/* Record the histroy of only owned objects */
	local_index = frame % NETGAME_MAX_HISTORY;
	if (wobj->history_frames[local_index] + NETGAME_MAX_HISTORY < frame) return 0; // This object hasn't been sent in a while
	if (~wobj->nodes_sent_to[node] & (1 << local_index)) return 0; // We didnt send it to that node
	return 1; // We have sent it to that node
}
int Wobj_GetDeltaForWobj(WOBJ *output, WOBJ *input, int node, int last_frame, int current_frame) {
	if (!Wobj_HasHistoryForNode(input, node, last_frame) ||
		last_frame == -1 || current_frame == -1) {
		memcpy(output, input, sizeof(WOBJ));
		return 0;
	}

	struct wobjdata *last, *curr;
	last = input->history + (last_frame % NETGAME_MAX_HISTORY);
	curr = input->history + (current_frame % NETGAME_MAX_HISTORY);
	Wobj_GetDeltaForWobjData((struct wobjdata *)output, curr, last);
	return 1;
}
void Wobj_GetDeltaForWobjData(struct wobjdata *output, const struct wobjdata *curr, const struct wobjdata *last) {
	output->x = curr->x - last->x;
	output->y = curr->y - last->y;
	output->vel_x = curr->vel_x - last->vel_x;
	output->vel_y = curr->vel_y - last->vel_y;
	output->custom_floats[0] = curr->custom_floats[0] - last->custom_floats[0];
	output->custom_floats[1] = curr->custom_floats[1] - last->custom_floats[1];
	output->custom_ints[0] = curr->custom_ints[0] - last->custom_ints[0];
	output->custom_ints[1] = curr->custom_ints[1] - last->custom_ints[1];
	output->speed = curr->speed - last->speed;
	output->jump = curr->jump - last->jump;
	output->strength = curr->strength - last->strength;
	output->health = curr->health - last->health;
	output->money = curr->money - last->money;
	output->anim_frame = curr->anim_frame - last->anim_frame;
	output->type = curr->type - last->type;
	output->item = curr->item - last->item;
	output->flags = curr->flags - last->flags;
	output->last_sound_played = curr->last_sound_played - last->last_sound_played;
	output->last_sound_uuid = curr->last_sound_uuid - last->last_sound_uuid;
	output->link_node = curr->link_node - last->link_node;
	output->link_uuid = curr->link_uuid - last->link_uuid;
	output->hitbox.x = curr->hitbox.x - last->hitbox.x;
	output->hitbox.y = curr->hitbox.y - last->hitbox.y;
	output->hitbox.w = curr->hitbox.w - last->hitbox.w;
	output->hitbox.h = curr->hitbox.h - last->hitbox.h;
	output->node_id = curr->node_id;
	output->uuid = curr->uuid;
}
void Wobj_UncompressDelta(struct wobjdata *output, struct wobjdata *delta, struct wobjdata *last) {
	output->x = last->x + delta->x;
	output->y = last->y + delta->y;
	output->vel_x = last->vel_x + delta->vel_x;
	output->vel_y = last->vel_y + delta->vel_y;
	output->custom_floats[0] = last->custom_floats[0] + delta->custom_floats[0];
	output->custom_floats[1] = last->custom_floats[1] + delta->custom_floats[1];
	output->custom_ints[0] = last->custom_ints[0] + delta->custom_ints[0];
	output->custom_ints[1] = last->custom_ints[1] + delta->custom_ints[1];
	output->speed = last->speed + delta->speed;
	output->jump = last->jump + delta->jump;
	output->strength = last->strength + delta->strength;
	output->health = last->health + delta->health;
	output->money = last->money + delta->money;
	output->anim_frame = last->anim_frame + delta->anim_frame;
	output->type = last->type + delta->type;
	output->item = last->item + delta->item;
	output->flags = last->flags + delta->flags;
	output->last_sound_played = last->last_sound_played + delta->last_sound_played;
	output->last_sound_uuid = last->last_sound_uuid + delta->last_sound_uuid;
	output->link_node = last->link_node + delta->link_node;
	output->link_uuid = last->link_uuid + delta->link_uuid;
	output->hitbox.x = last->hitbox.x + delta->hitbox.x;
	output->hitbox.y = last->hitbox.y + delta->hitbox.y;
	output->hitbox.w = last->hitbox.w + delta->hitbox.w;
	output->hitbox.h = last->hitbox.h + delta->hitbox.h;
	output->node_id = delta->node_id;
	output->uuid = delta->uuid;
}

void Wobj_UpdateGridPos(WOBJ *wobj)
{
	if (wobj->internal.owned) {
		ObjGrid_MoveObject(owned_grid, &wobj->internal.obj, wobj->x, wobj->y);
	} else {
		ObjGrid_MoveObject(unowned_grid, &wobj->internal.obj, wobj->x, wobj->y);
	}
}

static int WobjCollisionFlagCriteria(WOBJ *other, int flags) {
	return other->flags & flags;
}
static int WobjCollisionTypeCriteria(WOBJ *other, int type)
{
	return other->type == type;
}
static void Wobj_GetCollisionsGeneral(WOBJ *subject, WOBJ *collisions[WOBJ_MAX_COLLISIONS], int arg, int(*criteria)(WOBJ *, int))
{
	OBJGRID_ITER iter;
	CNM_BOX a, b;
	WOBJ *other;
	OBJGRID *grids[2] = {unowned_grid, owned_grid};
	int collisions_size = 0, i, x, y;

	memset(collisions, 0, sizeof(WOBJ *) * WOBJ_MAX_COLLISIONS);
	a.x = subject->x + subject->hitbox.x;
	a.y = subject->y + subject->hitbox.y;
	a.w = subject->hitbox.w;
	a.h = subject->hitbox.h;
	/* First start with unowned objects (because usually unowned objects are enemy objects) then the owned ones */
	for (x = (int)(a.x / OBJGRID_SIZE) - 1; x < (int)ceilf((a.x + a.w) / OBJGRID_SIZE) + 1; x++)
	{
		for (y = (int)(a.y / OBJGRID_SIZE) - 1; y < (int)ceilf((a.y + a.h) / OBJGRID_SIZE) + 1; y++)
		{
			for (i = 0; i < 2; i++)
			{
				ObjGrid_MakeIter(grids[i], x, y, &iter);
				while (iter != NULL)
				{
					other = GET_WOBJ(iter);
					if (other == subject || !criteria(other, arg))
					{
						ObjGrid_AdvanceIter(&iter);
						continue;
					}
					float ix, iy;
					WobjCalculate_InterpolatedPos(other, &ix, &iy);
					b.x = ix + other->hitbox.x;
					b.y = iy + other->hitbox.y;
					b.w = other->hitbox.w;
					b.h = other->hitbox.h;
					if (Util_AABBCollision(&a, &b))
					{
						if (collisions_size < WOBJ_MAX_COLLISIONS)
							collisions[collisions_size++] = other;
						else
							return;
					}
					ObjGrid_AdvanceIter(&iter);
				}
			}
		}
	}
}

void Wobj_GetCollisionsWithFlags(WOBJ *subject, WOBJ *collisions[WOBJ_MAX_COLLISIONS], int flags)
{
	Wobj_GetCollisionsGeneral(subject, collisions, flags, WobjCollisionFlagCriteria);	
}
void Wobj_GetCollisionsWithType(WOBJ *subject, WOBJ *collisions[WOBJ_MAX_COLLISIONS], int type) {
	Wobj_GetCollisionsGeneral(subject, collisions, type, WobjCollisionTypeCriteria);
}
// Calculates interpolated positions for moving platforms for better netplay
void WobjCalculate_InterpolatedPos(WOBJ *wobj, float *px, float *py) {
	if (!wobj->interpolate || ~wobj->flags & WOBJ_IS_MOVESTAND || ~wobj->flags & WOBJ_IS_SOLID || wobj->internal.owned) {
		*px = wobj->x; *py = wobj->y;
		return;
	}

	int interp_node = (Interaction_GetMode() == INTERACTION_MODE_CLIENT) ? 0 : wobj->node_id;
	*px = wobj->interp_x + ((wobj->x - wobj->interp_x) * ((float)wobj->interp_frame / NetGame_GetNode(interp_node)->avgframes_between_updates));
	*py = wobj->interp_y + ((wobj->y - wobj->interp_y) * ((float)wobj->interp_frame / NetGame_GetNode(interp_node)->avgframes_between_updates));
}
void Wobj_GetCollision(WOBJ *subject, WOBJ *collisions[WOBJ_MAX_COLLISIONS])
{
	OBJGRID_ITER iter;
	CNM_BOX a, b;
	WOBJ *other;
	OBJGRID *grids[2] = {unowned_grid, owned_grid};
	int collisions_size = 0, i, x, y;
	float interp_x, interp_y;

	memset(collisions, 0, sizeof(WOBJ *) * WOBJ_MAX_COLLISIONS);
	a.x = subject->x + subject->hitbox.x;
	a.y = subject->y + subject->hitbox.y;
	a.w = subject->hitbox.w;
	a.h = subject->hitbox.h;
	/* First start with unowned objects (because usually unowned objects are enemy objects) then the owned ones */
	for (x = (int)(a.x / OBJGRID_SIZE) - 1; x <= (int)ceilf((a.x + a.w) / OBJGRID_SIZE) + 1; x++)
	{
		for (y = (int)(a.y / OBJGRID_SIZE) - 1; y <= (int)ceilf((a.y + a.h) / OBJGRID_SIZE) + 1; y++)
		{
			for (i = 0; i < 2; i++)
			{
				ObjGrid_MakeIter(grids[i], x, y, &iter);
				while (iter != NULL)
				{
					other = GET_WOBJ(iter);
					if (other == subject)
					{
						ObjGrid_AdvanceIter(&iter);
						continue;
					}
					WobjCalculate_InterpolatedPos(other, &interp_x, &interp_y);
					b.x = interp_x + other->hitbox.x;
					b.y = interp_y + other->hitbox.y;
					b.w = other->hitbox.w;
					b.h = other->hitbox.h;
					if (Util_AABBCollision(&a, &b))
					{
						if (collisions_size < WOBJ_MAX_COLLISIONS)
							collisions[collisions_size++] = other;
						else
							return;
					}
					ObjGrid_AdvanceIter(&iter);
				}
			}
		}
	}
}
static void Wobj_DrawWobj(WOBJ *other, int camx, int camy) {
	if (!other->internal.owned && other->interpolate && Game_GetVar(GAME_VAR_CL_INTERP)->data.integer)
	{
		int interp_node = (Interaction_GetMode() == INTERACTION_MODE_CLIENT) ? 0 : other->node_id;
		int interp_frame = other->interp_frame;
		float interp_percent = (float)interp_frame / (float)(NetGame_GetNode(interp_node)->avgframes_between_updates);
		if (NetGame_GetNode(interp_node)->avgframes_between_updates == 0.0f)
			interp_percent = 1.0f;
		float oldx = other->x, oldy = other->y;
		float smoothing = Game_GetVar(GAME_VAR_CL_SMOOTHING)->data.decimal;

		float neededx = other->interp_x + ((oldx - other->interp_x) * interp_percent);
		float neededy = other->interp_y + ((oldy - other->interp_y) * interp_percent);

		other->smooth_x += (neededx - other->smooth_x) * smoothing;
		other->smooth_y += (neededy - other->smooth_y) * smoothing;

		if (Game_GetFrame() % (30*5) == 0)
		{
			other->smooth_x = neededx;
			other->smooth_y = neededy;
		}
		other->x = other->smooth_x;
		other->y = other->smooth_y;

		if (wobj_types[other->type].draw != NULL && ~other->flags & WOBJ_DONT_DRAW)
			wobj_types[other->type].draw(other, camx, camy);
		other->interp_frame++;

		other->x = oldx;
		other->y = oldy;
	}
	else
	{
		if (wobj_types[other->type].draw != NULL && ~other->flags & WOBJ_DONT_DRAW)
			wobj_types[other->type].draw(other, camx, camy);
	}

	// Debug helpers with non-drawer things
	CNM_RECT r;
	if (Game_GetVar(GAME_VAR_SHOW_COLLISION_BOXES)->data.integer)
	{
		Util_SetRect(&r, (int)(other->x + other->hitbox.x) - camx, (int)(other->y + other->hitbox.y) - camy,
					 (int)other->hitbox.w, (int)other->hitbox.h);
		Renderer_DrawRect(&r, RCOL_PINK, 2, RENDERER_LIGHT);
	}

	if (Game_GetVar(GAME_VAR_SHOW_GRIDPOS)->data.integer)
	{
		Renderer_DrawText
		(
			(int)other->x - camx, (int)other->y - camy, 0, RENDERER_LIGHT,
			"(%d, %d)",
			(int)(other->x / OBJGRID_SIZE), (int)(other->y / OBJGRID_SIZE)
		);
	}

	if (Game_GetVar(GAME_VAR_SHOWPOS)->data.integer)
	{
		Renderer_DrawText
		(
			(int)other->x - camx, (int)other->y - camy + 8, 0, RENDERER_LIGHT,
			"(%d, %d)",
			(int)(other->x), (int)(other->y)
		);
	}

	if (Game_GetVar(GAME_VAR_CL_SHOW_NODEUUIDS)->data.integer)
	{
		Renderer_DrawText
		(
			(int)other->x - camx, (int)other->y - camy + 16, 0, RENDERER_LIGHT,
			"NODE: %d, UUID: %d",
			other->node_id, other->uuid
		);
	}

	if (Game_GetVar(GAME_VAR_SHOW_COLLISION_BOXES)->data.integer)
	{
		float ix, iy;
		WobjCalculate_InterpolatedPos(other, &ix, &iy);
		Util_SetRect(&r, (int)(ix + other->hitbox.x) - camx, (int)(iy + other->hitbox.y) - camy,
					 (int)other->hitbox.w, (int)other->hitbox.h);
		Renderer_DrawRect(&r, RCOL_PINK, 2, RENDERER_LIGHT);
	}

	if (Game_GetVar(GAME_VAR_SHOWPOS)->data.integer)
	{
		Renderer_DrawText
		(
			(int)other->x - camx, (int)other->y - camy + 8, 0, RENDERER_LIGHT,
			"(%d, %d)",
			(int)(other->x), (int)(other->y)
		);
	}
}
void Wobj_DrawWobjs(int camx, int camy)
{
	clear_lens_flare();

	OBJGRID_ITER iter;
	WOBJ *other;
	OBJGRID *grids[2] = {unowned_grid, owned_grid};
	int gx, gy, g;
	for (gx = camx / (int)OBJGRID_SIZE - 1; gx < (camx + RENDERER_WIDTH) / (int)OBJGRID_SIZE + 1; gx++)
	{
		for (gy = camy / (int)OBJGRID_SIZE - 1; gy < (camy + RENDERER_HEIGHT) / (int)OBJGRID_SIZE + 1; gy++)
		{
			for (g = 0; g < 2; g++)
			{
				ObjGrid_MakeIter(grids[g], gx, gy, &iter);
				while (iter != NULL)
				{
					other = GET_WOBJ(iter);
					
					if (other->flags & WOBJ_OVERLAYER) {
						if (noverlayer_draws < NOVERLAYERS) {
							overlayer_draws[noverlayer_draws++] = other;
						}
					} else {
						Wobj_DrawWobj(other, camx, camy);
					}

					ObjGrid_AdvanceIter(&iter);
				}
			}
		}
	}
}
void Wobj_DrawWobjsOverlayer(int camx, int camy) {
	for (int i = 0; i < noverlayer_draws; i++) {
		Wobj_DrawWobj(overlayer_draws[i], camx, camy);
	}
	noverlayer_draws = 0;
}
wphys_flags_t Wobj_ResolveBlocksCollision(WOBJ *obj)
{
	int x, y;
	wphys_flags_t flags = 0;
	CNM_BOX h;
	h.x = obj->x + obj->hitbox.x;
	h.y = obj->y + obj->hitbox.y;
	h.w = obj->hitbox.w;
	h.h = obj->hitbox.h;
	Blocks_ResolveCollisionInstant(&h, &x, &y);
	obj->x = h.x - obj->hitbox.x;
	obj->y = h.y - obj->hitbox.y;
	if (x || y) {
		flags |= wphys_wall;
		if (x) {
			flags |= wphys_resolved_x;
			obj->vel_x = 0.0f;
		}
		if (y) {
			flags |= wphys_resolved_y;
			obj->vel_y = 0.0f;
		}
	}
	return flags;
}

static int WobjPhysics_IsGrounded(WOBJ *wobj)
{
	WOBJ *other, *jump_through;

	wobj->y += 1.0f;
	other = Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID);
	if ((jump_through = Wobj_GetWobjColliding(wobj, WOBJ_IS_JUMPTHROUGH)) != NULL)
	{
		//Console_Print("%f, %f, %f", wobj->vel_y, jump_through->vel_y, wobj->vel_y - jump_through->vel_y);
		if ((wobj->y + wobj->hitbox.y + wobj->hitbox.h) > (jump_through->y + jump_through->hitbox.y + 1) ||
			wobj->vel_y - jump_through->vel_y > 0.1f)
			jump_through = NULL;
		if (wobj->flags & WOBJ_SKIP_JUMPTHROUGH) {
			jump_through = NULL;
		}
	}
	wobj->y -= 1.0f;
	int touching_plat = 0;
	if (other) {
		float ycomp = other->internal.owned ? 0.0f : 0.0f;
		float ox, oy;
		WobjCalculate_InterpolatedPos(other, &ox, &oy);
		touching_plat = oy + other->hitbox.y + ycomp + 1.0f > wobj->y + wobj->hitbox.y + wobj->hitbox.h;
	}
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 1.0f) || touching_plat || jump_through != NULL)
		return CNM_TRUE;
	else
		return CNM_FALSE;
}
wphys_flags_t wobj_move_and_hit_blocks(WOBJ *wobj) {
	const float ground_ang = Wobj_IsGrounded(wobj) ? Wobj_GetGroundAngle(wobj) : 0.0f;
	const int ang_type = ceilf((ground_ang > CNM_PI ? CNM_PI - (ground_ang - CNM_PI) : ground_ang) / CNM_PI * 6.0f);
	const int doslow = (ground_ang < CNM_PI) == (wobj->vel_x > 0.0f);
	switch (ang_type) {
	case 1:
		wobj->vel_x *= doslow ? (2.0f / 3.0f) : (6.0f / 5.0f);
		if (!doslow && wobj->vel_y >= 0.0f && wobj->vel_y < fabsf(wobj->vel_x)) wobj->vel_y = fabsf(wobj->vel_x) / 2.0f;
		break;
	case 2:
		wobj->vel_x *= doslow ? (1.0f / 2.0f) : (1.0f);
		if (!doslow && wobj->vel_y >= 0.0f && wobj->vel_y < fabsf(wobj->vel_x)) wobj->vel_y = fabsf(wobj->vel_x);
		break;
	case 3:
		wobj->vel_x *= doslow ? (1.0f / 10.0f) : (3.0f / 4.0f);
		if (!doslow && wobj->vel_y >= 0.0f && wobj->vel_y < fabsf(wobj->vel_x)) wobj->vel_y = fabsf(wobj->vel_x) * 2.0f;
		break;
	default: break;
	}
	if (wobj->type == WOBJ_PLAYER) {
		//Console_Print("%d %d", ang_type, ground_ang);
	}
	struct bresolve_result result = bresolve_collision(wobj->x, wobj->y, wobj->vel_x, wobj->vel_y, wobj->hitbox, wobj->flags & WOBJ_SKIP_JUMPTHROUGH);
	wobj->x = result.x;
	wobj->y = result.y;

	wphys_flags_t flags = 0;
	if (result.resolved_x || result.resolved_y) {
		flags |= wphys_wall;
		if (result.resolved_x) flags |= wphys_resolved_x;
		if (result.resolved_y) flags |= wphys_resolved_y;
	}

	switch (ang_type) {
	case 1: result.vx *= doslow ? (3.0f / 2.0f) : (5.0f / 6.0f); break;
	case 2: result.vx *= doslow ? (2.0f) : (1.0f); break;
	case 3: result.vx *= doslow ? (10.0f) : (4.0f / 3.0f); break;
	default: break;
	}
	wobj->vel_x = result.vx;
	wobj->vel_y = result.vy;
	return flags;
}
static void stick_to_moving_platforms(WOBJ *wobj) {
	WOBJ *other;
	const float oy = wobj->y;
	wobj->y += 10.0f;
	other = Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID);
	if (other) {
		wobj->y = other->y + other->hitbox.y - wobj->hitbox.h - wobj->hitbox.y;
		wobj->vel_y = other->vel_y;
	} else {
		other = Wobj_GetWobjColliding(wobj, WOBJ_IS_JUMPTHROUGH);
		if (other && !(wobj->flags & WOBJ_SKIP_JUMPTHROUGH) && wobj->vel_y > other->vel_y - 1.0f) {
			wobj->y = other->y + other->hitbox.y - wobj->hitbox.h - wobj->hitbox.y;
			wobj->vel_y = other->vel_y;
		} else {
			wobj->y = oy;
		}
	}
}
void WobjPhysics_BeginUpdate(WOBJ *wobj)
{
	(void)wobj;
}

wphys_flags_t WobjPhysics_EndUpdate(WOBJ *wobj)
{
	wphys_flags_t flags = 0;
	flags |= wobj_move_and_hit_blocks(wobj);
	int set_velx = CNM_TRUE, set_vely = CNM_TRUE;
	if (wobj->type == WOBJ_PLAYER) {
		PLAYER_LOCAL_DATA *lc = wobj->local_data;
		if (lc->platinfo.active) {
			set_velx = CNM_FALSE;
			set_vely = CNM_FALSE;
		}
	}
	flags |= Wobj_ResolveObjectsCollision(wobj, set_velx, set_vely);
	if (flags & wphys_obj && Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f)) {
		flags |= Wobj_ResolveBlocksCollision(wobj);
	}

	// Stick to ground with slopes
	if ((Wobj_IsGrounded(wobj) || WobjPhysics_IsGrounded(wobj)) && wobj->vel_y >= 0.0f)
	{
		CNM_BOX h;
		h.x = wobj->x + wobj->hitbox.x;
		h.y = wobj->y + wobj->hitbox.y;
		h.w = wobj->hitbox.w;
		h.h = wobj->hitbox.h;
		Blocks_StickBoxToGround(&h);
		wobj->x = h.x - wobj->hitbox.x;
		wobj->y = h.y - wobj->hitbox.y;
	}

	if (wobj->type != WOBJ_PLAYER) {
		if (Wobj_IsGrounded(wobj)) {
			stick_to_moving_platforms(wobj);
		}

		// Stick to ground with moving platforms
		wobj->y += 1.0f;
		WOBJ *plat = Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID);
		if (plat != NULL)
		{
			if (plat->flags & WOBJ_IS_MOVESTAND) wobj->x += plat->vel_x;
		}
		if (plat == NULL && !(wobj->flags & WOBJ_SKIP_JUMPTHROUGH)) {
			plat = Wobj_GetWobjColliding(wobj, WOBJ_IS_JUMPTHROUGH);
			if (plat && (plat->flags & WOBJ_IS_MOVESTAND) && wobj->vel_y - plat->vel_y > -0.1f && (wobj->y + wobj->hitbox.y) < (plat->y + plat->hitbox.y + 8.0f)) {
				wobj->x += plat->vel_x;
			}
		}
		wobj->y -= 1.0f;
	}

	wobj->flags &= ~WOBJ_IS_GROUNDED;
	wobj->flags |= WobjPhysics_IsGrounded(wobj) ? WOBJ_IS_GROUNDED : 0;
	return flags;
}
void WobjPhysics_ApplyWindForces(WOBJ *wobj)
{
	WOBJ *other = Wobj_GetWobjCollidingWithType(wobj, HORIZONTAL_PUSH_ZONE);
	if (other == NULL)
		other = Wobj_GetWobjCollidingWithType(wobj, SMALL_HORIZONTAL_PUSH_ZONE);
	if (other != NULL)
	{
		wobj->x += other->custom_floats[0];
		if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f) ||
			Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID) != NULL)
		{
			wobj->x -= other->custom_floats[0];
		}
	}
	other = Wobj_GetWobjCollidingWithType(wobj, VERTICAL_PUSH_ZONE);
	if (other != NULL)
	{
		wobj->y += other->custom_floats[0];
		if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f) ||
			Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID) != NULL)
		{
			wobj->y -= other->custom_floats[0];
		}
		else if (wobj->vel_y > 0.0f)
			wobj->vel_y = 0.0f;
	}
	other = Wobj_GetWobjCollidingWithType(wobj, VERTICAL_WIND_ZONE);
	if (other != NULL)
	{
		wobj->vel_y += other->custom_floats[0];
		if (wobj->vel_y < -15.0f)
			wobj->vel_y = -15.0f;
		if (wobj->vel_y > 15.0f)
			wobj->vel_y = 15.0f;
	}
}

typedef struct _WOBJ_COLINFO_HELPER
{
	float dist;
	int index;
} WOBJ_COLINFO_HELPER;
int Wobj_ResolveObjectsCollisionSortFunc(const void *a, const void *b)
{
	float da = ((WOBJ_COLINFO_HELPER *)a)->dist;
	float db = ((WOBJ_COLINFO_HELPER *)b)->dist;
	if (da < db) return -1;
	if (da > db) return 1;
	return 0;
}
wphys_flags_t Wobj_ResolveObjectsCollision(WOBJ *obj, int set_velx, int set_vely)
{
	wphys_flags_t flags = 0;
	int i, num_indexes = 0;
	WOBJ_COLINFO_HELPER infos[WOBJ_MAX_COLLISIONS];
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	WOBJ *collider;
	int x, y;
	CNM_BOX h, other_h;
	float interp_x, interp_y;
	Wobj_GetCollisionsWithFlags(obj, collisions, WOBJ_IS_SOLID | WOBJ_IS_JUMPTHROUGH);
	for (i = 0; i < WOBJ_MAX_COLLISIONS; i++)
	{
		if (collisions[i] != NULL)
		{
			float dx, dy;
			dx = fabsf(obj->x - collisions[i]->x);
			dy = fabsf(obj->y - collisions[i]->y);
			infos[i].dist = sqrtf(dx * dx + dy * dy);
			infos[i].index = i;
			num_indexes++;
		}
		else
		{
			infos[i].dist = 1000000.0f;
		}
	}
	qsort(infos, WOBJ_MAX_COLLISIONS, sizeof(WOBJ_COLINFO_HELPER), Wobj_ResolveObjectsCollisionSortFunc);
	for (i = 0; i < num_indexes; i++)
	{
		collider = collisions[infos[i].index];
		if (collider->flags & WOBJ_IS_SOLID)
		{
			h.x = obj->x + obj->hitbox.x;
			h.y = obj->y + obj->hitbox.y;
			h.w = obj->hitbox.w;
			h.h = obj->hitbox.h;
			WobjCalculate_InterpolatedPos(collider, &interp_x, &interp_y);
			Util_SetBox
			(
				&other_h,
				interp_x + collider->hitbox.x,
				interp_y + collider->hitbox.y,
				collider->hitbox.w,
				collider->hitbox.h
			);
			Util_ResolveAABBCollision(&h, &other_h, &x, &y);
			obj->x = h.x - obj->hitbox.x;
			obj->y = h.y - obj->hitbox.y;
			if (x) flags |= wphys_obj | wphys_resolved_x;
			if (x && set_velx) {
				if ((obj->x + obj->hitbox.x + obj->hitbox.w / 2.0f < other_h.x + other_h.w / 2.0f && obj->vel_x >= collider->vel_x)
					|| (obj->x + obj->hitbox.x + obj->hitbox.w / 2.0f > other_h.x + other_h.w / 2.0f && obj->vel_x <= collider->vel_x))
					obj->vel_x = collider->vel_x;
			}
			if (y) flags |= wphys_obj | wphys_resolved_y;
			if (y && set_vely) {
				if ((obj->y + obj->hitbox.y + obj->hitbox.h / 2.0f < other_h.y + other_h.h / 2.0f && obj->vel_y >= collider->vel_y)
					|| (obj->y + obj->hitbox.y + obj->hitbox.h / 2.0f > other_h.y + other_h.h / 2.0f && obj->vel_y <= collider->vel_y))
					obj->vel_y = collider->vel_y;
			}
		}
		if ((collider->flags & WOBJ_IS_JUMPTHROUGH) && !(obj->flags & WOBJ_SKIP_JUMPTHROUGH))
		{
			if (obj->vel_y - collider->vel_y > 0.1f && (obj->y + obj->hitbox.y) < (collider->y + collider->hitbox.y))
			{
				obj->y = (collider->y + collider->hitbox.y) - (obj->hitbox.h + obj->hitbox.y);
				if (set_vely) obj->vel_y = collider->vel_y;
				//obj->vel_x = collider->vel_x;
			}
		}
	}
	return flags;
}
static void print_smap_data(void) {
	//Console_Print("smap print frame: %d", Game_GetFrame());
	Console_Print("owned size: %d", _smap_owned_sz);
	//for (int i = 0; i < SMAPSZ; i++) {
	//	if (_smap_owned[i].wobj) {
	//		Console_Print("psl: %d, node: %d, uuid: %d", _smap_owned[i].psl, _smap_owned[i].wobj->node_id, _smap_owned[i].wobj->uuid);
	//	} else {
	//		Console_Print("null");
	//	}
	//}
	//Console_Print("unowned size: %d", _smap_unowned_sz);
	//for (int i = 0; i < SMAPSZ; i++) {
	//	if (_smap_unowned[i].wobj) {
	//		Console_Print("psl: %d, node: %d, uuid: %d", _smap_unowned[i].psl, _smap_unowned[i].wobj->node_id, _smap_unowned[i].wobj->uuid);
	//	} else {
	//		Console_Print("null");
	//	}
	//}
}
WOBJ *Wobj_GetOwnedWobjFromUUID(int uuid)
{
	//print_smap_data();
	return WobjSearch_FindEntry(wobj_node_id, uuid);
	/*WOBJ *wobj = owned;
	while (wobj != NULL)
	{
		if (wobj->uuid == uuid)
			return wobj;
		wobj = wobj->internal.next;
	}
	return NULL;*/
}
WOBJ *Wobj_GetAnyWOBJFromUUIDAndNode(int node, int uuid)
{
	//print_smap_data();
	return WobjSearch_FindEntry(node, uuid);
	/*if (node == wobj_node_id)
		return Wobj_GetOwnedWobjFromUUID(uuid);
	else
		return Wobj_GetUnownedWobjFromUUID(node, uuid);*/
}
//void Wobj_OnDestroyLocalData(WOBJ *wobj) {
//	if (wobj->local_data) {
//		free(wobj->local_data);
//	}
//}
int Wobj_TryTeleportWobj(WOBJ *wobj, int only_telearea2)
{
	int i = 0, tryed = CNM_FALSE;
	TELEPORT_INFO *info;
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	Wobj_GetCollision(wobj, collisions);
	while (i < WOBJ_MAX_COLLISIONS && collisions[i] != NULL)
	{
		if ((collisions[i]->type == WOBJ_TELEPORT && wobj->type == WOBJ_PLAYER && !only_telearea2) || 
			(collisions[i]->type == WOBJ_TELEAREA1 && collisions[i]->custom_ints[1] && !only_telearea2) ||
			(collisions[i]->type == WOBJ_TELEAREA2 && collisions[i]->custom_ints[1]))
		{
			if (collisions[i]->type == WOBJ_TELEAREA2 && !(collisions[i]->custom_ints[0] & 0x20000) && wobj->type == WOBJ_PLAYER) {
				i++;
				continue;
			}

			tryed = CNM_TRUE;
			info = TeleportInfos_GetTeleport(collisions[i]->custom_ints[0] & 0xffff);
			if ((wobj->money >= info->cost || info->cost == 0) && !(wobj->flags & WOBJ_HAS_TELEPORTED))
			{
				wobj->money -= info->cost;
				wobj->x = info->x;
				wobj->y = info->y;
				wobj->flags |= WOBJ_HAS_TELEPORTED;
				return CNM_TRUE;
			}
		}
		i++;
	}
	if (!tryed)
		wobj->flags &= ~WOBJ_HAS_TELEPORTED;
	return CNM_FALSE;
}
WOBJ *Wobj_GetWobjColliding(WOBJ *wobj, int flags)
{
	int i;
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	Wobj_GetCollisionsWithFlags(wobj, collisions, flags);
	for (i = 0; i < WOBJ_MAX_COLLISIONS && collisions[i] != NULL; i++)
	{
		if (collisions[i]->flags & flags)
			return collisions[i];
	}
	return NULL;
}
WOBJ *Wobj_GetWobjCollidingWithType(WOBJ *wobj, int type)
{
	int i;
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	Wobj_GetCollisionsWithType(wobj, collisions, type);
	for (i = 0; i < WOBJ_MAX_COLLISIONS && collisions[i] != NULL; i++)
	{
		if (collisions[i]->type == type)
			return collisions[i];
	}
	return NULL;
}

static bool wobj_check_pt_in_chunk(
    const int32_t chunk_x,
    const int32_t chunk_y,
    const float x,
    const float y,
    const struct wobj_coll_query query
) {
    OBJGRID_ITER grid;
    for (int i = 0; i < 2; i++) {
        ObjGrid_MakeIter(
            ((OBJGRID *const [2]){ owned_grid, unowned_grid })[i],
            chunk_x,
            chunk_y,
            &grid
        );
        for (WOBJ *obj = GET_WOBJ(grid); grid; ObjGrid_AdvanceIter(&grid), obj = GET_WOBJ(grid)) {
            float interp_x, interp_y;
            WobjCalculate_InterpolatedPos(obj, &interp_x, &interp_y);
            
            if (x < interp_x + obj->hitbox.x ||
                x > interp_x + obj->hitbox.w + obj->hitbox.w ||
                y < interp_y + obj->hitbox.y ||
                y > interp_y + obj->hitbox.y + obj->hitbox.h) continue;
            if ((obj->flags & query.needs) != query.needs) continue;
            if ((obj->flags & query.any) == 0) continue;
            if (query.type != (typeof(query.type))-1 && query.type != obj->type) continue;
            if (obj == query.skip) continue;
            
            return true;
        }
    }
    
    return false;
}

bool wobj_check_pt(const float x, const float y, const struct wobj_coll_query query) {
    if (x < 0.0f || y < 0.0f || x >= OBJGRID_SIZE * 256 || y >= OBJGRID_SIZE * 256) {
        return false;
    }
    
    const int32_t base_x = (int32_t)(x / OBJGRID_SIZE), base_y = (int32_t)(y / OBJGRID_SIZE);
    for (int32_t chunk_y = base_y - 1; chunk_y <= base_y + 1; chunk_y++) {
        for (int32_t chunk_x = base_x - 1; chunk_x <= base_x + 1; chunk_x++) {
            if (wobj_check_pt_in_chunk(chunk_x, chunk_y, x, y, query)) {
                return true;
            }
        }
    }
    
    return false;
}
bool wobj_solid_pt(const WOBJ *const wobj, const float x, const float y) {
    const float rel_x = wobj->x + wobj->hitbox.x + wobj->hitbox.w / 2.0f + x,
        rel_y = wobj->y + wobj->hitbox.y + wobj->hitbox.h / 2.0f + y;
    const struct wobj_coll_query query = {
        .needs = 0,
        .any = WOBJ_IS_SOLID | WOBJ_IS_JUMPTHROUGH | WOBJ_IS_MOVESTAND,
        .type = (uint8_t)-1,
        .skip = wobj,
    };
    return world_check_pt(rel_x, rel_y, query);
}

int Wobj_IsCollidingWithBlocks(WOBJ *wobj, float offset_x, float offset_y)
{
	CNM_BOX b;
	Util_SetBox(&b, wobj->x + offset_x + wobj->hitbox.x, wobj->y + offset_y + wobj->hitbox.y, wobj->hitbox.w, wobj->hitbox.h);
	return Blocks_IsCollidingWithSolid(&b, wobj->vel_y >= -0.05f && !(wobj->flags & WOBJ_SKIP_JUMPTHROUGH));
}
int Wobj_IsCollidingWithBlocksOrObjects(WOBJ *wobj, float offset_x, float offset_y) {
	if (Wobj_IsCollidingWithBlocks(wobj, offset_x, offset_y)) return CNM_TRUE;
	WOBJ *other, *jump_through;
	wobj->y += offset_y;
	wobj->x += offset_x;
	other = Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID);
	if ((jump_through = Wobj_GetWobjColliding(wobj, WOBJ_IS_JUMPTHROUGH)) != NULL)
	{
		if (((wobj->y - offset_y) + wobj->hitbox.y + wobj->hitbox.h) > (jump_through->y + jump_through->hitbox.y + 1) ||
			wobj->vel_y < 0.0f)
			jump_through = NULL;
	}
	wobj->y -= offset_y;
	wobj->x -= offset_x;
	return other || jump_through;
}
void Wobj_IterateOverOwned(WOBJ **iter)
{
	if (*iter == NULL)
	{
		*iter = owned;
	}
	else
	{
		*iter = (*iter)->internal.next;
	}
}
void Wobj_IterateOverDebugUnowned(WOBJ **iter)
{
	int id = ((*iter) - unowned) + 1;
	if (*iter == NULL) id = 0;
	for (; id < unowned_size; id++) {
		if (unowned[id].type != WOBJ_NULL) {
			*iter = unowned + id;
			return;
		}
	}
	*iter = NULL;
}
void Wobj_InitGridIteratorOwned(WOBJ_GRID_ITER *iter, int gx, int gy)
{
	ObjGrid_MakeIter(owned_grid, gx, gy, &iter->iter);
	if (iter->iter != NULL)
		iter->wobj = GET_WOBJ(iter->iter);
	else
		iter->wobj = NULL;
}
void Wobj_GridIteratorIterate(WOBJ_GRID_ITER *iter)
{
	ObjGrid_AdvanceIter(&iter->iter);
	if (iter->iter != NULL)
		iter->wobj = GET_WOBJ(iter->iter);
	else
		iter->wobj = NULL;
}
void Wobj_UpdateWobjMovedPosition(WOBJ *wobj)
{
	ObjGrid_MoveObject(wobj->internal.owned ? owned_grid : unowned_grid, &wobj->internal.obj, wobj->x, wobj->y);
}
void Wobj_InitGridIteratorUnowned(WOBJ_GRID_ITER *iter, int gx, int gy)
{
	ObjGrid_MakeIter(unowned_grid, gx, gy, &iter->iter);
	if (iter->iter != NULL)
		iter->wobj = GET_WOBJ(iter->iter);
	else
		iter->wobj = NULL;
}
static void Wobj_CalculateLightForGridChunkClear(int gx, int gy)
{
	const int chunk_size_blocks = (int)OBJGRID_SIZE / BLOCK_SIZE;
	// First reset the direct light of the chunk
	int x, y;
	for (y = 0; y < chunk_size_blocks; y++)
		for (x = 0; x < chunk_size_blocks; x++)
			Blocks_SetBlockDirectLight(x + (gx * chunk_size_blocks), y + (gy * chunk_size_blocks), 0);
}
static void Wobj_CalculateLightForGridChunkAdd(int gx, int gy)
{
	WOBJ_GRID_ITER iters[2];
	Wobj_InitGridIteratorOwned(&iters[0], gx, gy);
	Wobj_InitGridIteratorUnowned(&iters[1], gx, gy);
	int x, y, ox, oy;
	const int light_tables[4][8][8] =
	{
		{
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0}
		},
		{
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 1, 0, 0, 0},
			{0, 0, 0, 1, 1, 1, 0, 0},
			{0, 0, 0, 0, 1, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0}
		},
		{
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 1, 0, 0, 0},
			{0, 0, 0, 1, 2, 1, 0, 0},
			{0, 0, 1, 2, 2, 2, 1, 0},
			{0, 0, 0, 1, 2, 1, 0, 0},
			{0, 0, 0, 0, 1, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0}
		},
		{
			{0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 1, 0, 0, 0},
			{0, 0, 0, 1, 2, 1, 0, 0},
			{0, 0, 1, 2, 3, 2, 1, 0},
			{0, 1, 2, 3, 3, 3, 2, 1},
			{0, 0, 1, 2, 3, 2, 1, 0},
			{0, 0, 0, 1, 2, 1, 0, 0},
			{0, 0, 0, 0, 1, 0, 0, 0}
		}
	};

	// Then loop through each iterator and light up the areas around them
	for (int g = 0; g < 2; g++)
	{
		while (iters[g].wobj != NULL)
		{
			ox = (int)(iters[g].wobj->x + iters[g].wobj->hitbox.x + iters[g].wobj->hitbox.w / 2.0f) / BLOCK_SIZE;
			oy = (int)(iters[g].wobj->y + iters[g].wobj->hitbox.y + iters[g].wobj->hitbox.h / 2.0f) / BLOCK_SIZE;
			int light_type = (((int)iters[g].wobj->flags & WOBJ_LIGHT_SMALL) ? 1 : 0) +
				(((int)iters[g].wobj->flags & WOBJ_LIGHT_BIG) ? 2 : 0);

			if (!light_type)
			{
				Wobj_GridIteratorIterate(&iters[g]);
				continue;
			}

			for (y = 0; y < 8; y++)
			{
				for (x = 0; x < 8; x++)
				{
					int new_light = light_tables[light_type][x][y], calculated_pos[2] = {x + ox - 4, y + oy - 4};
					new_light += Blocks_GetBlockDirectLight(calculated_pos[0], calculated_pos[1]);
					Blocks_SetBlockDirectLight(calculated_pos[0], calculated_pos[1], new_light);
				}
			}

			Wobj_GridIteratorIterate(&iters[g]);
		}
	}
}
void Wobj_CalculateLightForGridChunk(int gx, int gy)
{
	Wobj_CalculateLightForGridChunkClear(gx, gy);
	Wobj_CalculateLightForGridChunkAdd(gx, gy);
}
void Wobj_CalculateLightForScreen(int camx, int camy)
{
	int gx, gy;
	for (gx = (camx / (int)OBJGRID_SIZE) - 2; gx < ((camx + RENDERER_WIDTH) / (int)OBJGRID_SIZE) + 2; gx++)
		for (gy = (camy / (int)OBJGRID_SIZE) - 2; gy < ((camy + RENDERER_HEIGHT) / (int)OBJGRID_SIZE) + 2; gy++)
			Wobj_CalculateLightForGridChunkClear(gx, gy);
	for (gx = (camx / (int)OBJGRID_SIZE) - 1; gx < ((camx + RENDERER_WIDTH) / (int)OBJGRID_SIZE) + 1; gx++)
		for (gy = (camy / (int)OBJGRID_SIZE) - 1; gy < ((camy + RENDERER_HEIGHT) / (int)OBJGRID_SIZE) + 1; gy++)
			Wobj_CalculateLightForGridChunkAdd(gx, gy);
}
void Wobj_DoEnemyCry(WOBJ *wobj, int cry_sound)
{
	if (wobj->flags & WOBJ_DAMAGE_INDICATE)
		Interaction_PlaySound(wobj, cry_sound);
}
float Wobj_GetGroundAngle(const WOBJ *wobj) {
	//if (!Wobj_IsGrounded(wobj)) return 0.0f;
	const float ang1 = Blocks_GetAngle(wobj->x + wobj->hitbox.x + wobj->hitbox.w, wobj->y + wobj->hitbox.y + wobj->hitbox.h + 2.0f);
	const float ang2 = Blocks_GetAngle(wobj->x + wobj->hitbox.x + wobj->hitbox.w / 2.0f, wobj->y + wobj->hitbox.y + wobj->hitbox.h + 2.0f);
	const float ang3 = Blocks_GetAngle(wobj->x + wobj->hitbox.x, wobj->y + wobj->hitbox.y + wobj->hitbox.h + 2.0f);
	const float ang4 = Blocks_GetAngle(wobj->x + wobj->hitbox.x, wobj->y + wobj->hitbox.y + wobj->hitbox.h / 2.0f);
	if (ang1) return ang1;
	else if (ang2) return ang2;
	else if (ang3) return ang3;
	else return ang4;
}

void WobjGeneric_Draw(WOBJ *obj, int camx, int camy)
{
	CNM_RECT r;
	float obj_center_x, obj_center_y;
	obj_center_x = obj->x + (float)(wobj_types[obj->type].frames[obj->anim_frame].w / 2);
	obj_center_y = obj->y + (float)(wobj_types[obj->type].frames[obj->anim_frame].h / 2);
	Renderer_DrawBitmap2
	(
		(int)obj->x - camx,
		(int)ceilf(obj->y) - camy,
		&wobj_types[obj->type].frames[obj->anim_frame],
		0,
		Wobj_DamageLighting(obj, Blocks_GetCalculatedBlockLight((int)obj_center_x / BLOCK_SIZE, (int)obj_center_y / BLOCK_SIZE)),
		obj->flags & WOBJ_HFLIP,
		obj->flags & WOBJ_VFLIP
	);
	//obj->flags &= WOBJ_DAMAGE_INDICATE;
	if (Game_GetFrame() % 3 == 0)
		obj->flags &= ~WOBJ_DAMAGE_INDICATE;
}
void WobjGenericAttack_Update(WOBJ *wobj)
{
	//Interaction_DamageWobj(wobj, Interaction_GetVictim(wobj, WOBJ_IS_HOSTILE | WOBJ_IS_BREAKABLE));
	if (wobj->hitbox.w == 0.0f || wobj->hitbox.h == 0.0f) return;
	Interaction_GetVictim(wobj, WOBJ_IS_HOSTILE | WOBJ_IS_BREAKABLE);
	Interaction_DamageFoundWobjs(wobj);
	Interaction_DamageOtherPlayers(wobj);
}
int Wobj_InWater(WOBJ *wobj, int with_splash, int with_water) {
	return Wobj_GetWaterBlockID(wobj, with_splash, with_water) != -1;
}
static int get_water_blockid(WOBJ *wobj, int with_splashes, int with_water) {
	float cx = wobj->x + wobj->hitbox.x + wobj->hitbox.w / 2.0f;
	float cy = wobj->y + wobj->hitbox.y + wobj->hitbox.h / 2.0f;
	int in_water = CNM_FALSE;
	int bid = Blocks_GetBlock(BLOCKS_BG, (int)(cx) / BLOCK_SIZE, (int)(cy) / BLOCK_SIZE);
	BLOCK_PROPS *water_props = Blocks_GetBlockProp(bid);
	if (with_water) in_water |= water_props->dmg_type == BLOCK_DMG_TYPE_LAVA;
	if (with_splashes) in_water |= water_props->dmg_type == BLOCK_DMG_TYPE_SPLASH;
	if (!in_water)
	{
		bid = Blocks_GetBlock(BLOCKS_FG, (int)(cx) / BLOCK_SIZE, (int)(cy) / BLOCK_SIZE);
		water_props = Blocks_GetBlockProp(bid);
		if (with_water) in_water |= water_props->dmg_type == BLOCK_DMG_TYPE_LAVA;
		if (with_splashes) in_water |= water_props->dmg_type == BLOCK_DMG_TYPE_SPLASH;
	}
	return in_water ? bid : -1;
}
int Wobj_GetWaterBlockID(WOBJ *wobj, int with_splash, int with_water) {
	return get_water_blockid(wobj, with_splash, with_water);
}
void Wobj_DoSplashes(WOBJ *wobj) {
	if (Interaction_GetMode() != INTERACTION_MODE_SINGLEPLAYER && wobj->type != WOBJ_PLAYER) return;
	int inwater = get_water_blockid(wobj, 0, 1) != -1;
	int insplash = get_water_blockid(wobj, 1, 0) != -1;
	if ((!!(wobj->flags & WOBJ_WAS_WATER) != inwater)
		|| (!(wobj->flags & WOBJ_WAS_SPLASH) && insplash)) {
		int bid = (inwater || insplash) ? get_water_blockid(wobj, 1, 1) : -1;
		if (bid == -1) {
			const float ox = wobj->x, oy = wobj->y;
			wobj->x -= wobj->vel_x * 1.5f;
			wobj->y -= wobj->vel_y * 1.5f;
			bid = get_water_blockid(wobj, 1, 1);
			wobj->x = ox;
			wobj->y = oy;
		}
		float bullet_vel = 1.0f;
		if (wobj->flags & (WOBJ_IS_PLAYER_WEAPON | WOBJ_IS_PLAYER_BULLET)) {
			bullet_vel = 0.25f;
		}
		if (wobj->flags & (WOBJ_IS_MOVESTAND)) {
			bullet_vel = 1.25f;
		}
		float ang = atan2f(-wobj->vel_y, -wobj->vel_x);
		if (!inwater && !insplash) ang += CNM_PI;
		if (bid != -1) {
			Create_Splash_Particles(
				wobj->x + wobj->hitbox.x + wobj->hitbox.w / 2.0f,
				wobj->y + wobj->hitbox.y + wobj->hitbox.h / 2.0f,
				bid,
				ang,
				sqrtf(wobj->vel_x*wobj->vel_x +wobj->vel_y * wobj->vel_y)*bullet_vel,
				10,
				5,
				~wobj->flags & WOBJ_IS_SOLID
			);
			//if (bullet_vel > 0.5f && bullet_vel < 1.5f) Interaction_PlaySound(wobj, 65);
		}
	}
	if (inwater) wobj->flags |= WOBJ_WAS_WATER;
	else wobj->flags &= ~WOBJ_WAS_WATER;
	if (insplash) wobj->flags |= WOBJ_WAS_SPLASH;
	else wobj->flags &= ~WOBJ_WAS_SPLASH;
}

static uint32_t WobjSearch_GetCalculatedHash(int node, int uuid)
{
	return (uint32_t)((node * 120340) + (uuid ^ 134)) % SMAPSZ;
}
static WOBJ *_find_wobj(int node, int uuid) {
#if DBGLRU >= 1
	Console_Print("slow find on %d %d", node, uuid);
#endif
	if (node == wobj_node_id)
	{
		WOBJ *wobj = owned;
		while (wobj != NULL)
		{
			if (wobj->uuid == uuid)
				return wobj;
			wobj = wobj->internal.next;
		}
	}
	else
	{
		for (int i = 0; i < unowned_size; i++)
		{
			if (unowned[i].type && unowned[i].node_id == node && unowned[i].uuid == uuid)
				return unowned + i;
		}
	}

	return NULL;
}
static void remove_from_lru(smaplru_t *const lru) {
#if DBGLRU >= 2
	Console_Print("removing %d %d", lru->wobj->node_id, lru->wobj->uuid);
#endif
	if (lru->last) {
		lru->last->next = lru->next;
	} else {
		if (lru->wobj->node_id == wobj_node_id) _lru_owned_head = lru->next;
		else _lru_unowned_head = lru->next;
	}
	if (lru->next) {
		lru->next->last = lru->last;
	} else {
		if (lru->wobj->node_id == wobj_node_id) _lru_owned_tail = lru->last;
		else _lru_unowned_tail = lru->last;
	}
	lru->last = NULL;
	lru->next = NULL;
}
static void add_to_head_lru(smaplru_t *const lru) {
#if DBGLRU >= 2
	Console_Print("pushing to head %d %d", lru->wobj->node_id, lru->wobj->uuid);
#endif
	smaplru_t **head = lru->wobj->node_id == wobj_node_id ? &_lru_owned_head : &_lru_unowned_head;
	smaplru_t **tail = lru->wobj->node_id == wobj_node_id ? &_lru_owned_tail : &_lru_unowned_tail;

	if (*head) {
		(*head)->last = lru;
		lru->next = *head;
	} else {
		*tail = lru;
		lru->next = NULL;
	}
	*head = lru;
	lru->last = NULL;
}
static smaplru_t *alloc_linkless_lru(int node, int uuid) {
	WOBJ *wobj = _find_wobj(node, uuid);
	if (!wobj) return NULL;
	smaplru_t *lru = fixedpool_alloc(node == wobj_node_id ? _lru_owned_pool : _lru_unowned_pool);
	*lru = (smaplru_t){
		.wobj = wobj,
		.next = NULL,
		.last = NULL,
	};
	return lru;
}
static int pop_lru(int node) {
	if (node == wobj_node_id && _smap_owned_sz == SMAPSZ) {
		WobjSearch_DestoryEntry(_lru_owned_tail->wobj->node_id, _lru_owned_tail->wobj->uuid);
		return 1;
	}
	if (node != wobj_node_id && _smap_unowned_sz == SMAPSZ) {
		WobjSearch_DestoryEntry(_lru_unowned_tail->wobj->node_id, _lru_unowned_tail->wobj->uuid);
		return 1;
	}
	return 0;
}

#ifdef DEBUG
static int _hit_limits = 0;
#endif
static WOBJ *WobjSearch_FindEntry(const int node, const int uuid)
{
	smapent_t *const map = node == wobj_node_id ? _smap_owned : _smap_unowned;
	if (node == -1) return NULL;

	uint32_t i;
	smaplru_t *v = NULL;
	int vpsl;
	WOBJ *added_wobj = NULL;

search_start:
	i = WobjSearch_GetCalculatedHash(node, uuid);
	vpsl = 0;

	while (map[i].lru) {
		if (map[i].lru->wobj->uuid == uuid && map[i].lru->wobj->node_id == node) {
			remove_from_lru(map[i].lru);
			add_to_head_lru(map[i].lru);
#if DBGLRU >= 2
			Console_Print("Found %d %d", map[i].lru->wobj->node_id, map[i].lru->wobj->uuid);
#endif
			return map[i].lru->wobj;
		}

		if (vpsl > map[i].psl) {
			if (!v) {
				// remove if we're at limit
				if (pop_lru(node)) {
#ifdef DEBUG
					//Console_Print("Hit limit at %d %d", node, uuid);
					Console_Print("Hit search limit %d times", ++_hit_limits);
#endif
					goto search_start;
				}
				v = alloc_linkless_lru(node, uuid);
				if (!v) return NULL;
				add_to_head_lru(v);
				added_wobj = v->wobj;
			}

			smaplru_t *tmp = v;
			v = map[i].lru;
			map[i].lru = tmp;

			int tmp_psl = vpsl;
			vpsl = map[i].psl;
			map[i].psl = tmp_psl;
		}

		i = (i+1) % SMAPSZ;
		vpsl++;
	}
	
	if (!v) {
		if (node == wobj_node_id) assert(_smap_owned_sz < SMAPSZ);
		else assert(_smap_unowned_sz < SMAPSZ);
		v = alloc_linkless_lru(node, uuid);
		if (!v) return NULL;
		add_to_head_lru(v);
		added_wobj = v->wobj;
	}
#if DBGLRU >= 1
	Console_Print("Adding new search entry %d on %d %d", i, node, uuid);
#endif
	if (node == wobj_node_id) _smap_owned_sz++;
	else _smap_unowned_sz++;
	map[i].psl = vpsl;
	map[i].lru = v;
	return added_wobj;
}
static void WobjSearch_DestoryEntry(int node, int uuid)
{
	smapent_t *const map = node == wobj_node_id ? _smap_owned : _smap_unowned;
	uint32_t i = WobjSearch_GetCalculatedHash(node, uuid);
	int vpsl = 0;

	// Find the wobj first
	while (map[i].lru && vpsl <= map[i].psl) {
		if (map[i].lru->wobj->uuid == uuid && map[i].lru->wobj->node_id == node) {
			goto found;
		}

		i = (i+1) % SMAPSZ;
		vpsl++;
	}
	return;

found:
#if DBGLRU >= 1
	Console_Print("Removing search entry %d on %d %d", i, node, uuid);
#endif
	if (node == wobj_node_id) _smap_owned_sz--;
	else _smap_unowned_sz--;

	// Remove from lru cache
	remove_from_lru(map[i].lru);
	fixedpool_free(node == wobj_node_id ? _lru_owned_pool : _lru_unowned_pool,  map[i].lru);

	// Do backward shift deletion
	map[i].lru = NULL;
	map[i].psl = 0;
	uint32_t nexti = (i+1) % SMAPSZ;
	while (map[nexti].lru && map[nexti].psl > 0) {
		map[i].psl = map[nexti].psl - 1;
		map[i].lru = map[nexti].lru;
		map[nexti].psl = 0;
		map[nexti].lru = NULL;
		i = nexti;
		nexti = (i+1) % SMAPSZ;
	}


	//if (search_map[hash] != NULL && search_map[hash]->node_id == node && search_map[hash]->uuid == uuid)
	//	search_map[hash] = NULL;
}
static void WobjSearch_Reset(void)
{
	//memset(search_map, 0, sizeof(search_map));
	memset(_smap_owned, 0, sizeof(*_smap_owned) * SMAPSZ);
	_smap_owned_sz = 0;
	_lru_owned_head = NULL;
	_lru_owned_tail = NULL;
	fixedpool_fast_clear(_lru_owned_pool);
#ifdef DEBUG
	_hit_limits = 0;
#endif

	WobjSearch_DestroyUnownedEntries();
	//memset(_smap_unowned, 0, sizeof(_smap_unowned));
	//_smap_unowned_sz = 0;

}
static void WobjSearch_DestroyUnownedEntries(void)
{
	memset(_smap_unowned, 0, sizeof(*_smap_unowned) * SMAPSZ);
	_smap_unowned_sz = 0;
	_lru_unowned_head = NULL;
	_lru_unowned_tail = NULL;
	fixedpool_fast_clear(_lru_unowned_pool);
	//for (int i = 0; i < WOBJ_SEARCH_MAP_SIZE; i++)
	//{
	//	if (search_map[i] != NULL && search_map[i]->node_id != wobj_node_id)
	//		search_map[i] = NULL;
	//}
}
