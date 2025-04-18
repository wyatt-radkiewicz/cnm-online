#include <string.h>
#include <math.h>
#include "interaction.h"
#include "wobj.h"
#include "net.h"
#include "netgame.h"
#include "player.h"
#include "server.h"
#include "client.h"
#include "audio.h"
#include "blocks.h"
#include "game.h"
#include "renderer.h"
#include "ending_text.h"
#include "fadeout.h"
#include "mem.h"
#include "filesystem.h"

#define NETGAME_MAX_WOBJ_UPDATES 32

typedef struct _WOBJ_HURT_ENTRY
{
	int used;
	int node;
	int uuid;
	float original_hp;
	float hp_taken_away;
} WOBJ_HURT_ENTRY;

static int interaction_mode = INTERACTION_MODE_SINGLEPLAYER;
static WOBJ *interaction_player = NULL;
static WOBJ **destroyed_wobjs;
static WOBJ_HURT_ENTRY *hurt_wobjs;
static int destroyed_wobjs_allocater = 0;
static int audio_uuid = -1;
static int _next_level_timer = -1;
static WOBJ *victims[WOBJ_MAX_COLLISIONS];

#define HURT_MAP_SZ 32

typedef struct hurt {
	int node, uuid;
	float damage, x, y;
	int psl;
} hurt_t;

static hurt_t *hurt_map;
static int hurt_map_len;

static int Interaction_IsUnownedWobjInDestoryed(WOBJ *wobj);
static void Interaction_AddDestroyedWobjToList(WOBJ *wobj);
static int Interaction_GetHurtIndex(WOBJ *wobj);

void Interaction_Init(void)
{
	destroyed_wobjs = arena_global_alloc(sizeof(*destroyed_wobjs) * NETGAME_MAX_WOBJ_UPDATES);
	hurt_wobjs = arena_global_alloc(sizeof(*hurt_wobjs) * NETGAME_MAX_WOBJ_UPDATES);
	hurt_map = arena_global_alloc(sizeof(*hurt_map) * HURT_MAP_SZ);

	//_next_level_timer = -1;
	memset(destroyed_wobjs, 0, sizeof(*destroyed_wobjs) * NETGAME_MAX_WOBJ_UPDATES);
	memset(hurt_wobjs, 0, sizeof(*hurt_wobjs) * NETGAME_MAX_WOBJ_UPDATES);
	audio_uuid = 0;
}
void Interaction_SetMode(int mode)
{
	for (int i = 0; i < HURT_MAP_SZ; i++) {
		hurt_map[i].psl = 0;
		hurt_map[i].node = -1;
	}
	hurt_map_len = 0;
	interaction_mode = mode;
}
void Interaction_SetClientPlayerWObj(WOBJ *player)
{
	interaction_player = player;
}
void Interaction_FinishLevel(int ending_text_line) {
	netgame_playerfinish_t finish;
	NET_PACKET *p;
	switch (interaction_mode) {
	case INTERACTION_MODE_SINGLEPLAYER:
		strcpy(Game_GetVar(GAME_VAR_LEVEL)->data.string, "levels/");
		strcat(Game_GetVar(GAME_VAR_LEVEL)->data.string, EndingText_GetLine(ending_text_line));
		_next_level_timer = 40;
		if (Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) Fadeout_FadeToWhite(30, 20, 30);
		else Fadeout_FadeToBlack(30, 20, 30);
		break;
	case INTERACTION_MODE_CLIENT:
		finish.node = Client_GetNode()->id;
		finish.ending_text_line = ending_text_line;
		p = Net_CreatePacket(
			NET_PLAYER_FINISHED, CNM_TRUE, &NetGame_GetNode(0)->addr, sizeof(finish), &finish);
		Net_Send(p);
		break;
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_DEDICATED_SERVER:
		Server_PlayerFinish(0, ending_text_line);
		break;
	}
}
void Interaction_Tick(void) {
	if ((Game_GetFrame() + 5) % 10 == 0) {
		int x = -1000, y = -1000;
		for (int i = 0; i < HURT_MAP_SZ; i++) {
			hurt_t *hurt = &hurt_map[i];
			if (hurt->node < 0) continue;
			int lastx = x, lasty = y;
			x = hurt->x / 9;
			y = hurt->y / 11;
			if (abs(x - lastx) <= 3) {
				y = lasty - 1;
				x = lastx;
			}
			add_hitmarker(ceilf(hurt->damage), x * 9, y * 11);
			hurt->node = -1;
			hurt->psl = 0;
		}
		hurt_map_len = 0;
	}

	if (_next_level_timer > -1) _next_level_timer--;
	if (_next_level_timer == 0 && interaction_mode == INTERACTION_MODE_SINGLEPLAYER) {
		if (Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
			Game_SwitchState(GAME_STATE_MAINMENU);
		} else if (strcmp(Game_GetVar(GAME_VAR_LEVEL)->data.string, "levels/credits") == 0) {
			Game_SwitchState(GAME_STATE_CREDITS);
		} else {
			Game_PopState();
			Game_PushState(GAME_STATE_SINGLEPLAYER);
		}
	}
}
WOBJ *Interaction_GetVictim(WOBJ *inflictor, int flags)
{
	int i;
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	WOBJ *victim;
	if (inflictor == NULL)
		return NULL;

	switch (interaction_mode)
	{
	case INTERACTION_MODE_SINGLEPLAYER:
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		victim = NULL;
		Wobj_GetCollisionsWithFlags(inflictor, collisions, flags);
		memset(victims, 0, sizeof(victims));
		for (i = 0; i < WOBJ_MAX_COLLISIONS && collisions[i] != NULL; i++)
		{
			if (collisions[i]->flags & flags) {
				victim = collisions[i];
				victims[i] = collisions[i];
			}
		}
		return victim;
	}

	return NULL;
}
static void hurt_map_inflict(WOBJ *victim, float dmg) {
	// Add to the hurt map
	if (hurt_map_len >= HURT_MAP_SZ) return;
	int hurt_id = victim->uuid % HURT_MAP_SZ;
	if (victim->type != WOBJ_PLAYER) dmg *= 10.0f;
	hurt_t hurt = (hurt_t){
		.psl = 0,
		.node = victim->node_id,
		.uuid = victim->uuid,
		.damage = dmg,
		.x = victim->x + victim->hitbox.x + victim->hitbox.w / 2.0f,
		.y = victim->y + victim->hitbox.y - 4.0f,
	};
	while (hurt_map[hurt_id].node >= 0) {
		if (hurt_map[hurt_id].uuid == hurt.uuid && hurt_map[hurt_id].node == hurt.node) {
			hurt_map[hurt_id].damage += dmg;
			hurt_map[hurt_id].x = hurt.x;
			hurt_map[hurt_id].y = hurt.y;
			return;
		}

		if (hurt.psl > hurt_map[hurt_id].psl) {
			const hurt_t tmp = hurt_map[hurt_id];
			hurt_map[hurt_id] = hurt;
			hurt = tmp;
		}

		hurt_id = (hurt_id + 1) % HURT_MAP_SZ;
		hurt.psl++;
	}
	hurt_map_len++;
	hurt_map[hurt_id] = hurt;
}
float get_dmg_mul(int inflictor, int victim) {
	if ((inflictor & WOBJ_WATER_TYPE) && (victim & WOBJ_FIRE_TYPE)) {
		return 3.0f;
	}
	if ((inflictor & WOBJ_FIRE_TYPE) && (victim & WOBJ_VOID_TYPE)) {
		return 2.0f;
	}
	if ((inflictor & WOBJ_VOID_TYPE) && (victim & WOBJ_EARTH_TYPE)) {
		return 3.0f;
	}
	if ((inflictor & WOBJ_EARTH_TYPE) && (victim & WOBJ_GOOP_TYPE)) {
		return 2.0f;
	}
	if ((inflictor & WOBJ_GOOP_TYPE) && (victim & WOBJ_WATER_TYPE)) {
		return 2.0f;
	}
	if ((inflictor & WOBJ_GOOP_TYPE) && (victim & WOBJ_FIRE_TYPE)) {
		return 2.0f;
	}

	if ((victim & WOBJ_WATER_TYPE) && (inflictor & WOBJ_FIRE_TYPE)) {
		return 0.1f;
	}
	if ((victim & WOBJ_FIRE_TYPE) && (inflictor & WOBJ_VOID_TYPE)) {
		return 0.5f;
	}
	if ((victim & WOBJ_FIRE_TYPE) && (inflictor & WOBJ_FIRE_TYPE)) {
		return 0.1f;
	}
	if ((victim & WOBJ_VOID_TYPE) && (inflictor & WOBJ_EARTH_TYPE)) {
		return 0.25f;
	}
	if ((victim & WOBJ_EARTH_TYPE) && (inflictor & WOBJ_GOOP_TYPE)) {
		return 0.5f;
	}
	if ((victim & WOBJ_GOOP_TYPE) && (inflictor & WOBJ_WATER_TYPE)) {
		return 0.5f;
	}
	if ((victim & WOBJ_GOOP_TYPE) && (inflictor & WOBJ_FIRE_TYPE)) {
		return 0.5f;
	}
	if ((inflictor & WOBJ_GOOP_TYPE) && (victim & WOBJ_GOOP_TYPE)) {
		return 0.1f;
	}

	return 1.0f;
}
void Interaction_DamageWobj(WOBJ *inflictor, WOBJ *victim)
{
	int is_player;
	if (inflictor == NULL || victim == NULL)
		return;
	is_player = inflictor->flags & WOBJ_IS_PLAYER_WEAPON || inflictor->flags & WOBJ_IS_PLAYER;

	float dmg = inflictor->strength * get_dmg_mul(inflictor->flags, victim->flags);

	switch (interaction_mode)
	{
	case INTERACTION_MODE_SINGLEPLAYER:
		if (wobj_types[victim->type].hurt != NULL && !(victim->flags & WOBJ_INVULN))
		{
			PLAYER_LOCAL_DATA *pld = NULL;
			if (is_player) {
				pld = (PLAYER_LOCAL_DATA *)interaction_player->local_data;
			}

			if (pld) {
				pld->power_level += dmg * PLAYER_POWER_LEVEL_MUL;
			}

			victim->flags |= WOBJ_DAMAGE_INDICATE;
			victim->health -= dmg;
			//if (is_player)
			//	interaction_player->strength += wobj_types[victim->type].strength_reward;
			wobj_types[victim->type].hurt(victim, inflictor);
			hurt_map_inflict(victim, dmg);
			if (victim->health < 0.0f)
			{
				if (is_player)
				{
					interaction_player->money += wobj_types[victim->type].money_reward;
					interaction_player->strength += wobj_types[victim->type].strength_reward;
					if (victim->flags & WOBJ_AWARD_SCORE) {
						int score = wobj_types[victim->type].score_reward + 100;
						pld->score += score;
						add_scoremarker(score);
					}
					pld->special_level += wobj_types[victim->type].score_reward / 10 + 10;
				}
				Wobj_DestroyWobj(victim);
				create_enemy_corpse(victim);
			}
		}
		break;
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		if (wobj_types[victim->type].hurt != NULL && !(victim->flags & WOBJ_INVULN))
		{
			PLAYER_LOCAL_DATA *pld = NULL;
			if (is_player && interaction_player == Game_GetVar(GAME_VAR_PLAYER)->data.pointer) {
				pld = (PLAYER_LOCAL_DATA *)interaction_player->local_data;
			}
			/*victim->health -= inflictor->strength;
			if (!victim->internal.owned)
			{
				int hurt_index = Interaction_GetHurtIndex(victim);
				if (hurt_index != -1)
				{
					hurt_wobjs[hurt_index].hp_taken_away += inflictor->strength;
				}
			}*/

			victim->flags |= WOBJ_DAMAGE_INDICATE;
			hurt_map_inflict(victim, dmg);
			if (!victim->internal.owned)
				NetGame_DamageUnownedWobj(victim, dmg);
			else
				victim->health -= dmg;

			if (pld) {
				pld->power_level += dmg * PLAYER_POWER_LEVEL_MUL;
			}

			//if (is_player && !Interaction_IsUnownedWobjInDestoryed(victim))
			//	interaction_player->strength += wobj_types[victim->type].strength_reward;
			if (NetGame_GetClientWobjHealth(victim) < 0.0f)//victim->health < 0.0f)
			{
				if (is_player && !Interaction_IsUnownedWobjInDestoryed(victim))
				{
					interaction_player->money += wobj_types[victim->type].money_reward;
					interaction_player->strength += wobj_types[victim->type].strength_reward;
					if (pld && victim->flags & WOBJ_AWARD_SCORE) {
						int score = wobj_types[victim->type].score_reward + 100;
						pld->score += score;
						add_scoremarker(score);
					}
				}
				Interaction_DestroyWobj(victim);
				create_enemy_corpse(victim);
			}
		}
		if (victim->type == WOBJ_PLAYER) {
			victim->flags |= WOBJ_DAMAGE_INDICATE;
			if (!victim->internal.owned) {
				hurt_map_inflict(victim, dmg * 5.0f);
				NetGame_DamageUnownedWobj(victim, dmg * 5.0f);
				// This damage multiplication is done because players have the highest health in the game...
				// For instance, most bosses have 80 HP...
				// Laser minion has 15 hp and lava monster has 16 hp (but wider)
			}
		}
		break;
	}
}
void Interaction_DamageFoundWobjs(WOBJ *inflictor) {
	int i;

	switch (interaction_mode)
	{
	case INTERACTION_MODE_SINGLEPLAYER:
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		for (i = 0; i < WOBJ_MAX_COLLISIONS && victims[i] != NULL; i++)
		{
			Interaction_DamageWobj(inflictor, victims[i]);
		}
	}

	memset(victims, 0, sizeof(victims));
}
void Interaction_DamageOtherPlayers(WOBJ *inflictor) {
	int i;
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	if (inflictor == NULL || interaction_mode == INTERACTION_MODE_SINGLEPLAYER)
		return;
	if (!Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer)
		return;

	switch (interaction_mode)
	{
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		Wobj_GetCollisionsWithFlags(inflictor, collisions, WOBJ_IS_PLAYER);
		for (i = 0; i < WOBJ_MAX_COLLISIONS && collisions[i] != NULL; i++)
		{
			if (collisions[i] != Game_GetVar(GAME_VAR_PLAYER)->data.pointer) {
				Interaction_DamageWobj(inflictor, collisions[i]);
				return;
			}
		}
	}
}
WOBJ *Interaction_GetPlayerAsVictim(WOBJ *inflictor) {
	int i;
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	if (inflictor == NULL || interaction_mode == INTERACTION_MODE_SINGLEPLAYER)
		return NULL;
	if (!Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer)
		return NULL;

	switch (interaction_mode)
	{
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		Wobj_GetCollisionsWithFlags(inflictor, collisions, WOBJ_IS_PLAYER);
		for (i = 0; i < WOBJ_MAX_COLLISIONS && collisions[i] != NULL; i++)
		{
			if (collisions[i] != Game_GetVar(GAME_VAR_PLAYER)->data.pointer)
				return collisions[i];
		}
	}
	return NULL;
}
int Interaction_PlayerRecieveDamage(void)
{
	int i, taken_damage = CNM_FALSE;
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	if (interaction_player == NULL)
		return CNM_FALSE;

	switch (interaction_mode)
	{
	case INTERACTION_MODE_SINGLEPLAYER:
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		Wobj_GetCollisionsWithFlags(interaction_player, collisions, WOBJ_IS_HOSTILE);
		for (i = 0; i < WOBJ_MAX_COLLISIONS && collisions[i] != NULL; i++)
		{
			WOBJ *inflictor = collisions[i];
			float dmg = inflictor->strength * get_dmg_mul(inflictor->flags, interaction_player->flags);
			Player_RecieveDamage(interaction_player, inflictor, dmg);
			taken_damage = CNM_TRUE;
		}
	}

	return taken_damage;
}
int Interaction_WobjReceiveBlockDamage(WOBJ *wobj)
{
	CNM_BOX b;
	BLOCK_PROPS *props;

	if (wobj == NULL || wobj->flags & WOBJ_FIRE_TYPE)
		return CNM_FALSE;

	if (wobj->internal.owned)
	{
		Util_SetBox(&b, wobj->x + wobj->hitbox.x, wobj->y + wobj->hitbox.y, wobj->hitbox.w, wobj->hitbox.h);
		props = Blocks_IsCollidingWithDamage(&b);
		if (props == NULL) return CNM_FALSE;
		if (props->dmg_type != BLOCK_DMG_TYPE_NONE)
		{
			float dmg = (float)props->dmg / 30.0f;
			if (wobj == Game_GetVar(GAME_VAR_PLAYER)->data.pointer) {
				if ((wobj->custom_ints[1] & PLAYER_FLAG_SPECIAL)
					&& dmg < 99.0f) {
					PLAYER_LOCAL_DATA *data = wobj->local_data;
					data->power_level += dmg * PLAYER_POWER_LEVEL_MUL;
					dmg = 0;
				}
			}
			wobj->health -= dmg;
			if (props->dmg_type == BLOCK_DMG_TYPE_QUICKSAND)
			{
				if (wobj->vel_y > -2.5f) wobj->vel_y = 0.5f;
				if (wobj->vel_x > 1.0f) wobj->vel_x = 1.0f;
				if (wobj->vel_x < -1.0f) wobj->vel_x = -1.0f;
			}
			return CNM_TRUE;
		}
	}

	return CNM_FALSE;
}
void Interaction_DestroyWobj(WOBJ *wobj)
{
	switch (interaction_mode)
	{
	case INTERACTION_MODE_SINGLEPLAYER:
		Wobj_DestroyWobj(wobj);
		break;
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		if (wobj->internal.owned)
		{
			Wobj_DestroyWobj(wobj);
		}
		else if (!Interaction_IsUnownedWobjInDestoryed(wobj))
		{
			CLIENT_WOBJ_UPDATE_REQUEST req;
			if (interaction_mode == INTERACTION_MODE_CLIENT)
				req.node = Client_GetNode()->id;
			else
				req.node = 0;
			req.mode = CLIENT_WOBJ_UPDATE_DESTROY;
			req.obj_node = wobj->node_id;
			req.obj_uuid = wobj->uuid;
			NET_PACKET *p;
			if (interaction_mode == INTERACTION_MODE_CLIENT)
				p = Net_CreatePacket(NET_CLIENT_OBJECT_UPDATE_REQUEST, CNM_TRUE, &NetGame_GetNode(0)->addr, sizeof(req), &req);
			else
				p = Net_CreatePacket(NET_CLIENT_OBJECT_UPDATE_REQUEST, CNM_TRUE, &NetGame_GetNode(wobj->node_id)->addr, sizeof(req), &req);
			p->hdr.size = serialize_wobj_update_packet((uint8_t *)p->data, &req);
			Net_Send(p);
			if (interaction_mode == INTERACTION_MODE_CLIENT)
				Interaction_AddDestroyedWobjToList(wobj);
		} 
		break;
	}
}
void Interaction_DestroyWobjInstant(WOBJ *wobj) {
	if (interaction_mode != INTERACTION_MODE_SINGLEPLAYER) {
		int unowned = !wobj->internal.owned;
		if (unowned && netgame_should_create_unowned(wobj->node_id, wobj->uuid)) netgame_add_to_destroy_ringbuf(wobj->node_id, wobj->uuid);
		Interaction_DestroyWobj(wobj);
		if (unowned) Wobj_DestroyWobj(wobj);
	} else {
		Wobj_DestroyWobj(wobj);
	}
}
WOBJ *Interaction_CreateWobj(int type, float x, float y, int ci, float cf)
{
	switch (interaction_mode)
	{
	case INTERACTION_MODE_SINGLEPLAYER:
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		return Wobj_CreateOwned(type, x, y, ci, cf);
	}

	return NULL;
}

static int Interaction_IsUnownedWobjInDestoryed(WOBJ *wobj)
{
	for (int i = 0; i < NETGAME_MAX_WOBJ_UPDATES; i++)
	{
		if (destroyed_wobjs[i] == wobj)
			return CNM_TRUE;
	}
	return CNM_FALSE;
}
static void Interaction_AddDestroyedWobjToList(WOBJ *wobj)
{
	destroyed_wobjs_allocater += 1;
	if (destroyed_wobjs_allocater >= NETGAME_MAX_WOBJ_UPDATES)
		destroyed_wobjs_allocater = 0;
	destroyed_wobjs[destroyed_wobjs_allocater] = wobj;
}
void Interaction_SendWobjHurtPackets(int to_node)
{
	for (int i = 0; i < NETGAME_MAX_WOBJ_UPDATES; i++)
	{
		if (hurt_wobjs[i].used == CNM_FALSE || (hurt_wobjs[i].node != to_node && hurt_wobjs[i].node != -1))
			continue;

		CLIENT_WOBJ_UPDATE_REQUEST req;
		if (interaction_mode == INTERACTION_MODE_CLIENT)
			req.node = Client_GetNode()->id;
		else
			req.node = 0;
		req.mode = CLIENT_WOBJ_UPDATE_HEALTH;
		req.obj_node = hurt_wobjs[i].node;
		req.obj_uuid = hurt_wobjs[i].uuid;
		req.hp_taken_away = hurt_wobjs[i].hp_taken_away;
		NET_PACKET *p = Net_CreatePacket(NET_CLIENT_OBJECT_UPDATE_REQUEST, 0, &NetGame_GetNode(req.obj_node)->addr, sizeof(req), &req);
		p->hdr.size = serialize_wobj_update_packet((uint8_t *)p->data, &req);
		Net_Send(p);

		hurt_wobjs[i].used = CNM_FALSE;
		hurt_wobjs[i].node = 0;
		hurt_wobjs[i].uuid = 0;
		hurt_wobjs[i].hp_taken_away = 0.0f;
		hurt_wobjs[i].original_hp = 0.0f;
	}
}
static int Interaction_GetHurtIndex(WOBJ *wobj)
{
	int i, free_id = -1;
	for (i = 0; i < NETGAME_MAX_WOBJ_UPDATES; i++)
	{
		if (hurt_wobjs[i].used == CNM_FALSE)
		{
			free_id = i;
			continue;
		}
		if (hurt_wobjs[i].node == wobj->node_id && hurt_wobjs[i].uuid == wobj->uuid)
			return i;
	}
	if (free_id != -1)
	{
		hurt_wobjs[free_id].used = CNM_TRUE;
		hurt_wobjs[free_id].node = wobj->node_id;
		hurt_wobjs[free_id].uuid = wobj->uuid;
		hurt_wobjs[free_id].original_hp = wobj->health;
		return free_id;
	}
	else
	{
		return -1;
	}
}
void Interaction_ApplyHurtPacketsToWobj(WOBJ *wobj)
{
	int i;
	for (i = 0; i < NETGAME_MAX_WOBJ_UPDATES; i++)
	{
		if (hurt_wobjs[i].used == CNM_FALSE)
			continue;
		if (hurt_wobjs[i].node == wobj->node_id && hurt_wobjs[i].uuid == wobj->uuid)
		{
			wobj->health = hurt_wobjs[i].original_hp - hurt_wobjs[i].hp_taken_away;
		}
	}
}
void Interaction_ClearDestroyedWobjsBuffer(void)
{
	for (int i = 0; i < NETGAME_MAX_WOBJ_UPDATES; i++)
	{
		destroyed_wobjs[i] = NULL;
	}
}
int Interaction_GetMode(void)
{
	return interaction_mode;
}
void Interaction_PlaySound(WOBJ *wobj, int sound_id)
{
	Audio_PlaySound(sound_id, CNM_FALSE, (int)wobj->x, (int)wobj->y);
	wobj->last_sound_played = sound_id;
	wobj->last_sound_uuid = ++audio_uuid;
}
void Interaction_ResetAudioUUID(void)
{
	audio_uuid = -1;
}
WOBJ *Interaction_GetNearestPlayerToPoint(float px, float py)
{
	if (interaction_mode == INTERACTION_MODE_CLIENT)
	{
		return NULL;
	}
	else if (interaction_mode == INTERACTION_MODE_HOSTED_SERVER)
	{
		WOBJ *closest_wobj = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
		float dx = closest_wobj->x - px, dy = closest_wobj->y - py;
		float closest_dist = sqrtf(dx*dx + dy*dy), current_dist;
		NETGAME_NODE *node = NULL;
		NetGame_Iterate(&node);
		while (node != NULL)
		{
			if (node->id != NETGAME_SERVER_NODE && node->client_player != NULL)
			{
				dx = node->client_player->x - px;
				dy = node->client_player->y - py;
				current_dist = sqrtf(dx * dx + dy * dy);
				if (current_dist < closest_dist)
				{
					closest_wobj = node->client_player;
					closest_dist = current_dist;
				}
			}
			NetGame_Iterate(&node);
		}

		return closest_wobj;
	}
	else if (interaction_mode == INTERACTION_MODE_SINGLEPLAYER)
	{
		return Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	}

	return NULL;
}
float Interaction_GetDistanceToWobj(WOBJ *a, WOBJ *b)
{
	if (a != NULL && b != NULL)
	{
		float dx = a->x - b->x, dy = a->y - b->y;
		return sqrtf(dx*dx + dy*dy);
	}
	else
	{
		return INFINITY;
	}
}
void Interaction_ForceWobjPosition(WOBJ *wobj, float x, float y)
{
	switch (interaction_mode)
	{
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		if (!wobj->internal.owned)
		{
			NetGame_ForceUnownedWobjsPosition(wobj, x, y);
			break;
		}
	case INTERACTION_MODE_SINGLEPLAYER:
		wobj->x = x;
		wobj->y = y;
		wobj->vel_x = 0.0f;
		wobj->vel_y = 0.0f;
		Wobj_UpdateGridPos(wobj);
		break;
	}
}
void Interaction_SetWobjFlag(WOBJ *wobj, int flags) {
	switch (interaction_mode)
	{
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		if (!wobj->internal.owned)
		{
			NetGame_ForceUnownedWobjsFlags(wobj, flags, -1);
			break;
		}
	case INTERACTION_MODE_SINGLEPLAYER:
		wobj->flags |= flags;
		break;
	}
}
void Interaction_ClearWobjFlag(WOBJ *wobj, int flags) {
	switch (interaction_mode)
	{
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		if (!wobj->internal.owned)
		{
			NetGame_ForceUnownedWobjsFlags(wobj, 0, ~flags);
			break;
		}
	case INTERACTION_MODE_SINGLEPLAYER:
		wobj->flags &= ~flags;
		break;
	}
}
void Interaction_GameOver(void) {
	switch (interaction_mode)
	{
	case INTERACTION_MODE_HOSTED_SERVER:
	case INTERACTION_MODE_CLIENT:
		break;
	case INTERACTION_MODE_SINGLEPLAYER:
		// Delete save file
		//new_save(g_saves + g_current_save);
		g_saves[g_current_save].lives = 3;
		g_saves[g_current_save].offhand = 0;
		g_saves[g_current_save].item = 0;
		g_saves[g_current_save].upgrade_state = 0;
		g_saves[g_current_save].hp = 100;
		g_saves[g_current_save].strength = 5;
		save_game(g_current_save, g_saves + g_current_save);
		Game_SwitchState(GAME_STATE_MAINMENU);
		break;
	}
}
