#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "wobj.h"
//#include "pool.h"
#include "obj_grid.h"
#include "renderer.h"
#include "blocks.h"
#include "teleport_infos.h"
#include "game.h"
#include "netgame.h"
#include "spawners.h"

#define GET_WOBJ(iter) ((WOBJ *)((unsigned char *)(iter) - offsetof(WOBJ, internal.obj)))
#define WOBJ_SEARCH_MAP_SIZE (1 << 11)

//static POOL *wobj_pool;
static OBJGRID *owned_grid;
static OBJGRID *unowned_grid;
static int owned_uuid;
static int wobj_node_id;
static WOBJ *deleted_owned[4096];
static int deleted_size;
static int major_reset;

static WOBJ *owned;
static WOBJ unowned[WOBJ_MAX_UNOWNED_WOBJS];
static int unowned_size;

static WOBJ *search_map[WOBJ_SEARCH_MAP_SIZE];

static void Wobj_FreeOwnedWobjAndRemove(WOBJ *wobj);

static WOBJ *WobjSearch_FindEntry(int node, int uuid);
static void WobjSearch_DestoryEntry(int node, int uuid);
static void WobjSearch_Reset(void);
static void WobjSearch_DestroyUnownedEntries(void);

void Wobj_Init(void)
{
	memset(unowned, 0, sizeof(unowned));
	memset(deleted_owned, 0, sizeof(deleted_owned));
	deleted_size = 0;
	owned = NULL;
	owned_uuid = 0;
	wobj_node_id = 0;
	unowned_size = 0;
	major_reset = CNM_FALSE;
	owned_grid = ObjGrid_Create(128, 128);
	unowned_grid = ObjGrid_Create(128, 64);

	WobjSearch_Reset();
	//wobj_pool = Pool_Create(sizeof(WOBJ));
}
void Wobj_Quit(void)
{
	owned = NULL;
	ObjGrid_Destroy(owned_grid);
	ObjGrid_Destroy(unowned_grid);
	owned_grid = NULL;
	unowned_grid = NULL;
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

	WOBJ *wobj = malloc(sizeof(WOBJ));

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

	if (wobj_types[type].create != NULL)
		wobj_types[type].create(wobj);
	return wobj;
}
WOBJ *Wobj_CreateUnowned(int type, float x, float y, int frame, int flags, int ci, float cf, int node_id, int uuid)
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
	if (wobj_types[type].create != NULL)
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
	if (wobj->dropped_death_item != 0 && !major_reset)
		Wobj_CreateOwned(WOBJ_DROPPED_ITEM, wobj->x, wobj->y, wobj->dropped_death_item, 0.0f);

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
	free(wobj);
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
void Wobj_UpdateOwnedWobjs(void)
{
	WOBJ *rover, *next;
	/* Update only owned objects */
	rover = owned;
	while (rover != NULL)
	{
		next = rover->internal.next;
		if (wobj_types[rover->type].update != NULL)
		{
			wobj_types[rover->type].update(rover);
			ObjGrid_MoveObject(owned_grid, &rover->internal.obj, rover->x, rover->y);
			if (Game_GetFrame() % 3 == 0)
				rover->flags &= ~WOBJ_DAMAGE_INDICATE;
		}
		rover = next;
	}
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
	if (wobj->internal.owned)
		ObjGrid_MoveObject(owned_grid, &wobj->internal.obj, wobj->x, wobj->y);
	else
		ObjGrid_MoveObject(unowned_grid, &wobj->internal.obj, wobj->x, wobj->y);
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
					b.x = other->x + other->hitbox.x;
					b.y = other->y + other->hitbox.y;
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
void Wobj_GetCollision(WOBJ *subject, WOBJ *collisions[WOBJ_MAX_COLLISIONS])
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
					if (other == subject)
					{
						ObjGrid_AdvanceIter(&iter);
						continue;
					}
					b.x = other->x + other->hitbox.x;
					b.y = other->y + other->hitbox.y;
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
void Wobj_DrawWobjs(int camx, int camy)
{
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

					if (!other->internal.owned && other->interpolate && Game_GetVar(GAME_VAR_CL_INTERP)->data.integer)
					{
						int interp_node = (Interaction_GetMode() == INTERACTION_MODE_CLIENT) ? 0 : other->node_id;
						int interp_frame = other->interp_frame++;
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

						other->x = oldx;
						other->y = oldy;
					}
					else
					{
						if (wobj_types[other->type].draw != NULL && ~other->flags & WOBJ_DONT_DRAW)
							wobj_types[other->type].draw(other, camx, camy);
					}

					// Debug helpers with non-drawer things
					if (wobj_types[other->type].draw == NULL) {
						CNM_RECT r;
						if (Game_GetVar(GAME_VAR_SHOW_COLLISION_BOXES)->data.integer)
						{
							Util_SetRect(&r, (int)(other->x + other->hitbox.x) - camx, (int)(other->y + other->hitbox.y) - camy,
										 (int)other->hitbox.w, (int)other->hitbox.h);
							Renderer_DrawRect(&r, Renderer_MakeColor(255, 0, 255), 2, RENDERER_LIGHT);
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

					ObjGrid_AdvanceIter(&iter);
				}
			}
		}
	}
}
void Wobj_ResolveBlocksCollision(WOBJ *obj)
{
	int x, y;
	CNM_BOX h;
	h.x = obj->x + obj->hitbox.x;
	h.y = obj->y + obj->hitbox.y;
	h.w = obj->hitbox.w;
	h.h = obj->hitbox.h;
	Blocks_ResolveCollision(&h, &x, &y);
	obj->x = h.x - obj->hitbox.x;
	obj->y = h.y - obj->hitbox.y;
	if (x)
		obj->vel_x = 0.0f;
	if (y)
		obj->vel_y = 0.0f;
}

static int WobjPhysics_IsGrounded(WOBJ *wobj)
{
	WOBJ *other, *jump_through;

	wobj->y += 1.0f;
	other = Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID);
	if ((jump_through = Wobj_GetWobjColliding(wobj, WOBJ_IS_JUMPTHROUGH)) != NULL)
	{
		if ((wobj->y + wobj->hitbox.y + wobj->hitbox.h) > (jump_through->y + jump_through->hitbox.y + 1) ||
			wobj->vel_y < 0.0f)
			jump_through = NULL;
	}
	wobj->y -= 1.0f;
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 1.0f) || other != NULL || jump_through != NULL)
		return CNM_TRUE;
	else
		return CNM_FALSE;
}
void WobjPhysics_BeginUpdate(WOBJ *wobj)
{
	wobj->flags &= ~WOBJ_IS_GROUNDED;
	wobj->flags |= WobjPhysics_IsGrounded(wobj) ? WOBJ_IS_GROUNDED : 0;
}
void WobjPhysics_EndUpdate(WOBJ *wobj)
{
	wobj->x += wobj->vel_x;
	wobj->y += wobj->vel_y;

	Wobj_ResolveObjectsCollision(wobj);
	Wobj_ResolveBlocksCollision(wobj);

	// Stick to ground with slopes
	if (Wobj_IsGrouneded(wobj) &&
		!WobjPhysics_IsGrounded(wobj) &&
		wobj->vel_y >= 0.0f)
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

	// Stick to ground with moving platforms
	wobj->y += 1.0f;
	WOBJ *plat = Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID);
	if (plat != NULL)
	{
		if (plat->flags & WOBJ_IS_MOVESTAND)
		{
			wobj->x += plat->vel_x;
			if (plat->vel_y > 0.0f)
				wobj->y += plat->vel_y;
		}
	}
	wobj->y -= 1.0f;
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
void Wobj_ResolveObjectsCollision(WOBJ *obj)
{
	int i, num_indexes = 0;
	WOBJ_COLINFO_HELPER infos[WOBJ_MAX_COLLISIONS];
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	WOBJ *collider;
	int x, y;
	CNM_BOX h, other_h;
	Wobj_GetCollision(obj, collisions);
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
			Util_SetBox
			(
				&other_h,
				collider->x + collider->hitbox.x,
				collider->y + collider->hitbox.y,
				collider->hitbox.w,
				collider->hitbox.h
			);
			Util_ResolveAABBCollision(&h, &other_h, &x, &y);
			obj->x = h.x - obj->hitbox.x;
			obj->y = h.y - obj->hitbox.y;
			if (x)
				obj->vel_x = 0.0f;
			if (y)
				obj->vel_y = 0.0f;
		}
		if (collider->flags & WOBJ_IS_JUMPTHROUGH)
		{
			if (obj->vel_y > 0.0f && (obj->y + obj->hitbox.y + obj->hitbox.h / 2.0f) < (collider->y + collider->hitbox.y))
			{
				obj->y = (collider->y + collider->hitbox.y) - (obj->hitbox.h + obj->hitbox.y);
				obj->vel_y = 0.0f;
			}
		}
	}
}
WOBJ *Wobj_GetOwnedWobjFromUUID(int uuid)
{
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
	return WobjSearch_FindEntry(node, uuid);
	/*if (node == wobj_node_id)
		return Wobj_GetOwnedWobjFromUUID(uuid);
	else
		return Wobj_GetUnownedWobjFromUUID(node, uuid);*/
}
int Wobj_TryTeleportWobj(WOBJ *wobj)
{
	int i = 0, tryed = CNM_FALSE;
	TELEPORT_INFO *info;
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	Wobj_GetCollision(wobj, collisions);
	while (collisions[i] != NULL && i < WOBJ_MAX_COLLISIONS)
	{
		if (collisions[i]->type == WOBJ_TELEPORT || 
			(collisions[i]->type == WOBJ_TELEAREA1 && collisions[i]->custom_ints[1]))
		{
			tryed = CNM_TRUE;
			info = TeleportInfos_GetTeleport(collisions[i]->custom_ints[0]);
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
	Wobj_GetCollision(wobj, collisions);
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
	Wobj_GetCollision(wobj, collisions);
	for (i = 0; i < WOBJ_MAX_COLLISIONS && collisions[i] != NULL; i++)
	{
		if (collisions[i]->type == type)
			return collisions[i];
	}
	return NULL;
}
int Wobj_IsCollidingWithBlocks(WOBJ *wobj, float offset_x, float offset_y)
{
	CNM_BOX b;
	Util_SetBox(&b, wobj->x + offset_x + wobj->hitbox.x, wobj->y + offset_y + wobj->hitbox.y, wobj->hitbox.w, wobj->hitbox.h);
	return Blocks_IsCollidingWithSolid(&b);
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
			ox = (int)iters[g].wobj->x / BLOCK_SIZE;
			oy = (int)iters[g].wobj->y / BLOCK_SIZE;
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

void WobjGeneric_Draw(WOBJ *obj, int camx, int camy)
{
	CNM_RECT r;
	float obj_center_x, obj_center_y;
	obj_center_x = obj->x + (float)(wobj_types[obj->type].frames[obj->anim_frame].w / 2);
	obj_center_y = obj->y + (float)(wobj_types[obj->type].frames[obj->anim_frame].h / 2);
	Renderer_DrawBitmap2
	(
		(int)obj->x - camx,
		(int)obj->y - camy,
		&wobj_types[obj->type].frames[obj->anim_frame],
		0,
		Wobj_DamageLighting(obj, Blocks_GetCalculatedBlockLight((int)obj_center_x / BLOCK_SIZE, (int)obj_center_y / BLOCK_SIZE)),
		obj->flags & WOBJ_HFLIP,
		obj->flags & WOBJ_VFLIP
	);
	//obj->flags &= WOBJ_DAMAGE_INDICATE;
	if (Game_GetFrame() % 3 == 0)
		obj->flags &= ~WOBJ_DAMAGE_INDICATE;

	if (Game_GetVar(GAME_VAR_SHOW_COLLISION_BOXES)->data.integer)
	{
		Util_SetRect(&r, (int)(obj->x + obj->hitbox.x) - camx, (int)(obj->y + obj->hitbox.y) - camy,
					 (int)obj->hitbox.w, (int)obj->hitbox.h);
		Renderer_DrawRect(&r, Renderer_MakeColor(255, 0, 255), 2, RENDERER_LIGHT);
	}

	if (Game_GetVar(GAME_VAR_SHOW_GRIDPOS)->data.integer)
	{
		Renderer_DrawText
		(
			(int)obj->x - camx, (int)obj->y - camy, 0, RENDERER_LIGHT,
			"(%d, %d)",
			(int)(obj->x / OBJGRID_SIZE), (int)(obj->y / OBJGRID_SIZE)
		);
	}

	if (Game_GetVar(GAME_VAR_SHOWPOS)->data.integer)
	{
		Renderer_DrawText
		(
			(int)obj->x - camx, (int)obj->y - camy + 8, 0, RENDERER_LIGHT,
			"(%d, %d)",
			(int)(obj->x), (int)(obj->y)
		);
	}

	if (Game_GetVar(GAME_VAR_CL_SHOW_NODEUUIDS)->data.integer)
	{
		Renderer_DrawText
		(
			(int)obj->x - camx, (int)obj->y - camy + 16, 0, RENDERER_LIGHT,
			"NODE: %d, UUID: %d",
			obj->node_id, obj->uuid
		);
	}
}
void WobjGenericAttack_Update(WOBJ *wobj)
{
	//Interaction_DamageWobj(wobj, Interaction_GetVictim(wobj, WOBJ_IS_HOSTILE | WOBJ_IS_BREAKABLE));
	if (wobj->hitbox.w == 0.0f || wobj->hitbox.h == 0.0f) return;
	Interaction_GetVictim(wobj, WOBJ_IS_HOSTILE | WOBJ_IS_BREAKABLE);
	Interaction_DamageFoundWobjs(wobj);
	Interaction_DamageOtherPlayers(wobj);
}

static int WobjSearch_GetCalculatedHash(int node, int uuid)
{
	return ((node * 120340) + (uuid ^ 134)) % WOBJ_SEARCH_MAP_SIZE;
}

static WOBJ *WobjSearch_FindEntry(int node, int uuid)
{
	int hash = WobjSearch_GetCalculatedHash(node, uuid);
	if (search_map[hash] != NULL && search_map[hash]->node_id == node && search_map[hash]->uuid == uuid)
	{
		return search_map[hash];
	}
	else
	{
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
	}

	return NULL;
}
static void WobjSearch_DestoryEntry(int node, int uuid)
{
	int hash = WobjSearch_GetCalculatedHash(node, uuid);
	if (search_map[hash] != NULL && search_map[hash]->node_id == node && search_map[hash]->uuid == uuid)
		search_map[hash] = NULL;
}
static void WobjSearch_Reset(void)
{
	memset(search_map, 0, sizeof(search_map));
}
static void WobjSearch_DestroyUnownedEntries(void)
{
	for (int i = 0; i < WOBJ_SEARCH_MAP_SIZE; i++)
	{
		if (search_map[i] != NULL && search_map[i]->node_id != wobj_node_id)
			search_map[i] = NULL;
	}
}