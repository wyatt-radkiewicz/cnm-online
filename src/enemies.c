#include <string.h>
#include <math.h>
#include "interaction.h"
#include "renderer.h"
#include "wobj.h"
#include "input.h"
#include "blocks.h"
#include "game.h"
#include "console.h"
#include "teleport_infos.h"
#include "netgame.h"
#include "player.h"
#include "audio.h"
#include "player_spawns.h"
#include "ending_text.h"
#include "background.h"
#include "item.h"
#include "utility.h"
#include "supervirus.h"

void WobjSlime_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 2.0f;
	wobj->strength = 0.8333333f;
	wobj->speed = 3.0f;
	wobj->hitbox.x = 1.0f;
	wobj->hitbox.y = 14.0f;
	wobj->hitbox.w = 29.0f;
	wobj->hitbox.h = 18.0f;
	wobj->money = 10;
	wobj->custom_ints[0] = Util_RandInt(10, 30 * 5);
	wobj->custom_floats[0] = (float)(Util_RandInt(0, 1) * 2 - 1);
	wobj->custom_ints[1] = Util_RandInt(30, 120);
}
void WobjSlime_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);
	WobjPhysics_BeginUpdate(wobj);

	wobj->custom_ints[0]--;
	if (wobj->custom_ints[0] <= 0)
	{
		wobj->custom_floats[0] *= -1.0f;
		wobj->custom_ints[0] = Util_RandInt(45, 30 * 5);
	}

	if (wobj->custom_ints[1]-- < 0)
	{
		Interaction_PlaySound(wobj, 18);
		wobj->custom_ints[1] = Util_RandInt(30, 120);
	}

	if (wobj->type == WOBJ_SILVER_SLIME)
	{
		if (!Wobj_IsCollidingWithBlocksOrObjects(wobj, wobj->custom_floats[0] * 8.0f, 16.0f))
			wobj->custom_floats[0] *= -1.0f;
	}

	wobj->vel_x = wobj->custom_floats[0];
	if (Wobj_IsCollidingWithBlocksOrObjects(wobj, wobj->custom_floats[0] * 2.0f, 0.0f))
		wobj->custom_floats[0] *= -1.0f;
	WobjPhysics_ApplyWindForces(wobj);

	wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
	wobj->anim_frame = Game_GetFrame() / 10 % 2;
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
	WobjPhysics_EndUpdate(wobj);
}
void WobjSlime_Draw(WOBJ *wobj, int camx, int camy)
{
	wobj->anim_frame = Game_GetFrame() / 10 % 2;
	WobjGeneric_Draw(wobj, camx, camy);
}

void WobjFlyingSlime_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	wobj->custom_ints[0] = 0;
	wobj->custom_ints[1] = 3;
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 1.6666667f;
	wobj->anim_frame = 0;
	wobj->health = 6.0f;
	wobj->speed = 3.0f;
}
void WobjFlyingSlime_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);
	wobj->x += wobj->speed;
	wobj->y -= wobj->speed / 2.0f;

	wobj->anim_frame = (Game_GetFrame() / 15) % 2;
	if (wobj->custom_ints[0]++ >= 30 * 2)
	{
		wobj->speed *= -1.0f;
		wobj->custom_ints[0] = 0;
	}

	if (Game_GetFrame() % 15 == 0 && Util_RandInt(0, 100) < 50)
		Interaction_PlaySound(wobj, 18);

	if (Game_GetFrame() % 30 * 2 == 0 && wobj->custom_ints[1] > 0)
	{
		wobj->custom_ints[1]--;
		Interaction_CreateWobj(WOBJ_SLIME, wobj->x, wobj->y, 0, 0.0f);
	}
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}

void WobjHeavySheildBox_Create(WOBJ *wobj) {
	Util_SetBox(&wobj->hitbox, 0.0, 0.0, 32.0f, 40.0f);
	wobj->flags = WOBJ_IS_BREAKABLE;
}
void WobjHeavySheildBox_Draw(WOBJ *wobj, int camx, int camy) {
#ifdef DEBUG
	//CNM_RECT r;
	//Util_SetRect(&r, wobj->x - camx + wobj->hitbox.x, wobj->y - camy + wobj->hitbox.y, wobj->hitbox.w, wobj->hitbox.h);
	//Renderer_DrawRect(&r, 15, 3, RENDERER_LIGHT);
	//Renderer_DrawText(54, 54, 0, RENDERER_LIGHT, "%f", wobj->health);
#endif
}
void WobjHeavy_Destroy(WOBJ *wobj) {
	WOBJ *sh = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
	if (sh) {
		Interaction_DestroyWobj(sh);
	}
}
void WobjHeavy_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 8.0f, 0.0f, 48.0f, 64.0f);
	wobj->flags = WOBJ_IS_HOSTILE;
	WOBJ *s = Interaction_CreateWobj(WOBJ_HEAVY_SHEILD_BOX, wobj->x, wobj->y, 0, 0.0f);
	wobj->link_node = s->node_id;
	wobj->link_uuid = s->uuid;
	wobj->strength = 4.0f;
	wobj->health = 7.0f;
	wobj->anim_frame = 0;
	if (wobj->custom_floats[0] < 0.0f)
		wobj->flags |= WOBJ_HFLIP;
	wobj->custom_ints[0] = 0;
	wobj->custom_ints[1] = 0;
	wobj->money = 0;
	wobj->on_destroy = WobjHeavy_Destroy;
}
#define HEAVY_SPD 2.0f
void WobjHeavy_Update(WOBJ *wobj)
{
	CNM_BOX tempbox;
	//memcpy(&tempbox, &wobj->hitbox, sizeof(tempbox));
	//Util_SetBox(&wobj->hitbox, 16.0f, 0.0f, 16.0f, 64.0f);
	Wobj_DoEnemyCry(wobj, 45);
	Util_SetBox(&wobj->hitbox, 8.0f, 0.0f, 48.0f, 64.0f);
	WobjPhysics_BeginUpdate(wobj);
	//memcpy(&wobj->hitbox, &tempbox, sizeof(tempbox));
	WOBJ *sh = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
	if (sh) {
		sh->health = 1000000.0f;
		Wobj_UpdateGridPos(sh);
	}
	WOBJ *plr = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
	if (Game_GetFrame() % 45 == 0 || wobj->custom_ints[0] == 1) {
		if (plr->x + 16.0f > wobj->x + 32.0f) wobj->flags &= ~WOBJ_HFLIP;
		else wobj->flags |= WOBJ_HFLIP;
	}
	wobj->strength = 4.0f;
	wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
	int is_close = Interaction_GetDistanceToWobj(wobj, plr) <= 96.0f;
	if (wobj->custom_ints[0] == 0) {
		if (Game_GetFrame() % 30 == 0) {
			wobj->custom_ints[1] = Util_RandInt(0, 1);
			if (is_close) {
				wobj->custom_ints[1] = 0;
			}
		}
		wobj->money++;
		if (wobj->money > 85 || (wobj->money > 25 && is_close)) {
			wobj->custom_ints[0] = 1;
			wobj->money = 0;
		}
		wobj->anim_frame = 2 + ((Game_GetFrame() / 5) & 1);
		if (wobj->custom_ints[1] == 0) {
			if (wobj->flags & WOBJ_HFLIP) wobj->vel_x = -HEAVY_SPD;
			else wobj->vel_x = HEAVY_SPD;
			if (!Wobj_IsCollidingWithBlocksOrObjects(wobj, 48.0f, 16.0f)) {
				wobj->custom_ints[1] = 3;
			}
			if (!Wobj_IsCollidingWithBlocksOrObjects(wobj, -48.0f, 16.0f)) {
				wobj->custom_ints[1] = 2;
				//wobj->x += (float)(((wobj->flags & WOBJ_HFLIP) != 0) * 2 - 1) * 2.0f * HEAVY_SPD;
			}
		} else if (wobj->custom_ints[1] == 1) {
			wobj->vel_x = 0.0f;
		} else if (wobj->custom_ints[1] == 2) {
			wobj->vel_x = HEAVY_SPD;
		} else {
			wobj->vel_x = -HEAVY_SPD;
		}
		wobj->strength = 4.0f;
		if (sh) {
			sh->y = wobj->y + 20.0f;
			Util_SetBox(&sh->hitbox, 0.0, 0.0, 32.0f, 40.0f);
			if (wobj->flags & WOBJ_HFLIP) {
				sh->x = wobj->x - 20.0f;
			} else {
				sh->x = wobj->x + wobj->hitbox.x + wobj->hitbox.w - (32.0f - 20.0f);
			}
		}
	} else if (wobj->custom_ints[0] == 1) {
		if (sh) {
			Util_SetBox(&sh->hitbox, 0.0, 0.0, 0.0f, 0.0f);
		}
		wobj->anim_frame = 0;
		if (wobj->money > 15) {
			wobj->anim_frame = 1;
			wobj->strength = 20.0f;
			if (wobj->flags & WOBJ_HFLIP)
				Util_SetBox(&wobj->hitbox, -16.0f, 0.0f, 48.0f, 64.0f);
			else
				Util_SetBox(&wobj->hitbox, 24.0f, 0.0f, 48.0f, 64.0f);
		}
		wobj->vel_x = 0.0f;
		if (wobj->money++ > 20) {
			WOBJ *blast = Interaction_CreateWobj(WOBJ_HEAVY_BLAST, wobj->x + 32.0f, wobj->y + 32.0f, 0, 0.0f);
			blast->speed = 7.0f;
			if (wobj->flags & WOBJ_HFLIP)
				blast->speed *= -1.0f;
			blast->flags = (blast->flags & ~WOBJ_HFLIP) | (wobj->flags & WOBJ_HFLIP);
			wobj->custom_ints[0] = 0;
			wobj->custom_ints[1] = 0;
			wobj->money = 0;
		}
	}
	
	//WobjPhysics_ApplyWindForces(wobj);
	memcpy(&tempbox, &wobj->hitbox, sizeof(tempbox));
	Util_SetBox(&wobj->hitbox, 8.0f, 0.0f, 48.0f, 64.0f);
	WobjPhysics_EndUpdate(wobj);
	memcpy(&wobj->hitbox, &tempbox, sizeof(tempbox));
}

void WobjHeavyBlast_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 24.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 7.0f;
	wobj->anim_frame = 0;
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->custom_ints[0] = 0;
	wobj->strength = 2.6666667f;
	wobj->health = 1000000.0f;
}
void WobjHeavyBlast_Update(WOBJ *wobj)
{
	wobj->x += wobj->speed;
	if (Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 0.0f) || Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID) || wobj->custom_ints[0]++ >= 30*5)
		Interaction_DestroyWobj(wobj);
}

void WobjDragon_Create(WOBJ *wobj)
{
	wobj->anim_frame = 0;
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 128.0f;
	wobj->hitbox.h = 128.0f;
	wobj->strength = 5.0f;
	wobj->health = 80.0f;
}
void WobjDragon_Update(WOBJ *wobj)
{
	float cx = wobj->x + 64.0f;
	float cy = wobj->y + 64.0f;

	Wobj_DoEnemyCry(wobj, 45);
	wobj->anim_frame = (Game_GetFrame() / 15) % 2;
	if (wobj->custom_ints[0]) wobj->anim_frame += 2;

	if (Game_GetFrame() % 100 == 0)
		Interaction_PlaySound(wobj, 27);

	if (Game_GetFrame() % 30 == 0)
	{
		WOBJ *closest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (closest_player->x > wobj->x + 64.0f) wobj->flags |= WOBJ_HFLIP;
		else wobj->flags &= ~WOBJ_HFLIP;
		if (Interaction_GetDistanceToWobj(wobj, closest_player) < (float)RENDERER_WIDTH)
		{
			float ang = atan2f(closest_player->y - cy, closest_player->x - cx);
			WOBJ *fireball = Interaction_CreateWobj(WOBJ_FIREBALL, cx, cy, 0, ang);
		}
	}
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}

void WobjFireball_Create(WOBJ *wobj)
{
	wobj->custom_ints[0] = 30*5;
	wobj->speed = 6.5f;
	wobj->strength = 3.333333333f;
	wobj->health = 10000.0f;
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	Interaction_PlaySound(wobj, 16);
}
void WobjFireball_Update(WOBJ *wobj)
{
	wobj->x += cosf(wobj->custom_floats[0]) * wobj->speed;
	wobj->y += sinf(wobj->custom_floats[0]) * wobj->speed;
	if (wobj->custom_ints[0]-- <= 0 || Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 0.0f))
		Interaction_DestroyWobj(wobj);
}
void WobjFireball_Draw(WOBJ *wobj, int camx, int camy)
{
	wobj->anim_frame = Game_GetFrame() % 2;
	WobjGeneric_Draw(wobj, camx, camy);
}

void WobjBozoPin_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 8.0f;
	wobj->hitbox.y = 8.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 48.0f;
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 12.0f;
	wobj->strength = 3.0f;
	wobj->custom_ints[0] = 0;
	wobj->speed = 4.0f;
	if (wobj->custom_floats[0] == 0.0f) wobj->speed = 0.0f;
	wobj->item = wobj->speed == 0.0f;
	wobj->custom_floats[1] = 0.0f;
}
void WobjBozoPin_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);
	WobjPhysics_BeginUpdate(wobj);
	WOBJ *closest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
	if (Interaction_GetDistanceToWobj(wobj, closest_player) < (float)320.0f && !wobj->item)
	{
		wobj->custom_ints[0]++;
		float tang = atan2f(closest_player->y - wobj->y, closest_player->x - wobj->x);
		if (wobj->custom_ints[0] > 30*4) wobj->speed += (0.5f - wobj->speed) * 0.5f;
		if (wobj->custom_ints[0] > 30*6) {
			wobj->x += Util_RandFloat() * 2.0f - 1.0f;
			wobj->y += Util_RandFloat() * 2.0f - 1.0f;
			if (wobj->custom_ints[0] > 30*7) {
				if (tang > wobj->custom_floats[1]) wobj->custom_floats[1] += 7.0f / 180.0f * CNM_PI;
				else wobj->custom_floats[1] -= 7.0f / 180.0f * CNM_PI;
				wobj->speed = 8.0f;
			}
			if (wobj->custom_ints[0] > 30*9) {
				wobj->speed = 4.0f;
				wobj->custom_ints[0] = 0;
			}
		}
		if (wobj->custom_ints[0] < 30*7) {
			wobj->custom_floats[1] = tang;
		}
		wobj->vel_x = cosf(wobj->custom_floats[1]) * wobj->speed;
		wobj->vel_y = sinf(wobj->custom_floats[1]) * wobj->speed;
	}
	WobjPhysics_EndUpdate(wobj);

	if (Game_GetFrame() % 25 == 0)
		Interaction_PlaySound(wobj, 17);
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}
void WobjBozoPin_Draw(WOBJ *wobj, int camx, int camy)
{
	wobj->anim_frame = (Game_GetFrame() / 3) % 8;
	WobjGeneric_Draw(wobj, camx, camy);
}

#define BOZO_WAYPOINTS_MAX 32
static float bzx[BOZO_WAYPOINTS_MAX], bzy[BOZO_WAYPOINTS_MAX];
static int bznum;
void Enemies_Reset(void) {
	bznum = 0;
}
void WobjBozoWaypoint_Create(WOBJ *wobj) {
	if (bznum >= BOZO_WAYPOINTS_MAX) return;
	bzx[bznum] = wobj->x;
	bzy[bznum++] = wobj->y - (64.0f - 16.0f);
}
void WobjBozo_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 64.0f;
	wobj->hitbox.h = 128.0f;
	wobj->custom_ints[1] = CNM_FALSE;
	wobj->custom_ints[0] = 0;
	wobj->anim_frame = 0;
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 100.0f;
	if (wobj->custom_floats[0] > 0.001f) wobj->health = wobj->custom_floats[0];
	wobj->strength = 16.66667f;
}
void WobjBozo_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);
	wobj->anim_frame = (wobj->health < 50.0f) ? 1 : 0;
	if (bznum != 0) {
		float tx = bzx[wobj->custom_ints[0]];
		float ty = bzy[wobj->custom_ints[0]];
		if (wobj->custom_ints[1] == 0) {
			if (wobj->x < tx) wobj->x += 4.0f;
			if (wobj->x > tx) wobj->x -= 4.0f;
			if (wobj->y < ty) wobj->y += 4.0f;
			if (wobj->y > ty) wobj->y -= 4.0f;
			if (fabsf(wobj->x - tx) < 8.0f && fabsf(wobj->y - ty) < 8.0f) {
				wobj->custom_ints[1] = Util_RandInt(1, 3); // modes 1-2
				wobj->money = 0;
			}
		} else if (wobj->custom_ints[1] == 1) {
			if (wobj->money++ >= 30*8) {
				wobj->custom_ints[0] = Util_RandInt(0, bznum);
				wobj->custom_ints[1] = 0;
			}
			if (wobj->money % (30*3) == 0) {
				WOBJ *bozo_pin = Interaction_CreateWobj(WOBJ_BOZO_PIN, wobj->x + 32.0f, wobj->y + 64.0f, 0, 5.5f);
			}
		} else if (wobj->custom_ints[1] == 2) {
			if (wobj->money++ >= 30*8) {
				wobj->custom_ints[0] = Util_RandInt(0, bznum);
				wobj->custom_ints[1] = 0;
			}
			if (wobj->money % 2 == 0) {
				if ((wobj->money / 2) % 16 < 11) {
					float ang = ((float)wobj->money * 4.0f) / 180.0f * CNM_PI;
					WOBJ *fireball = Interaction_CreateWobj(WOBJ_FIREBALL, wobj->x + 32.0f, wobj->y + 64.0f, 0, ang);
					fireball = Interaction_CreateWobj(WOBJ_FIREBALL, wobj->x + 32.0f, wobj->y + 64.0f, 0, ang+(0.5*CNM_PI));
					fireball = Interaction_CreateWobj(WOBJ_FIREBALL, wobj->x + 32.0f, wobj->y + 64.0f, 0, ang+(1.5*CNM_PI));
					fireball = Interaction_CreateWobj(WOBJ_FIREBALL, wobj->x + 32.0f, wobj->y + 64.0f, 0, ang+(1.0*CNM_PI));
				}
			}
		}
		return;
	}
	

	if (Game_GetFrame() % 30 * 5 == 0)
	{
		WOBJ *closest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		wobj->custom_ints[1] = (Interaction_GetDistanceToWobj(wobj, closest_player) < (float)RENDERER_WIDTH);
	}

	if (Game_GetFrame() % 25 == 0 && Util_RandInt(0, 100) > 75)
		Interaction_PlaySound(wobj, 19);

	int timer = (wobj->health < 20.0f) ? (60) : (30*8);
	if (Game_GetFrame() % timer == 0)
	{
		if (wobj->custom_ints[1])
		{
			WOBJ *bozo_pin = Interaction_CreateWobj(WOBJ_BOZO_PIN, wobj->x + 32.0f, wobj->y + 64.0f, 0, 5.5f);
		}
	}
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}
void WobjBozoFireball_Create(WOBJ *wobj) {
	wobj->custom_ints[0] = 30*10;
	wobj->speed = 6.5f;
	wobj->strength = 3.333333333f;
	wobj->health = 10000.0f;
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	Interaction_PlaySound(wobj, 16);
}
void WobjBozoFireball_Update(WOBJ *wobj) {
	wobj->x += cosf(wobj->custom_floats[0]) * wobj->speed;
	wobj->y += sinf(wobj->custom_floats[0]) * wobj->speed;
	if (wobj->custom_ints[0]-- <= 0)
		Interaction_DestroyWobj(wobj);
}

void WobjSliverSlime_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 4.0f;
	wobj->strength = 1.5f;
	wobj->speed = 5.0f;
	wobj->hitbox.x = 1.0f;
	wobj->hitbox.y = 14.0f;
	wobj->hitbox.w = 29.0f;
	wobj->hitbox.h = 18.0f;
	wobj->money = 100;
	wobj->custom_ints[0] = Util_RandInt(10, 30 * 5);
	wobj->custom_floats[0] = (float)(Util_RandInt(0, 1) * 2 - 1) * wobj->speed;
}

// TODO: Update lava monster
enum LAVAMONSTER_STATES {
	LMS_IDLE,
	LMS_CHASING,
	LMS_DROP,
	LMS_FLYING,
	LMS_SHOOTING,
	LMS_SWOOP,
};
typedef struct _LAVAMONSTER_LOCAL_DATA {
	int state;
	int state_timer;
} LAVAMONSTER_LOCAL_DATA;
void WobjLavaMonster_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 16.0f;
	wobj->strength = 6.0f;
	wobj->jump = 10.0f;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 48.0f;
	wobj->hitbox.h = 48.0f;
	wobj->vel_x = 0.0f;
	wobj->vel_y = 0.0f;
	if (wobj->custom_ints[0])
		wobj->flags |= WOBJ_HFLIP;
}
void WobjLavaMonster_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);
	WobjPhysics_BeginUpdate(wobj);
	if (Wobj_IsGrouneded(wobj))
		wobj->vel_y = 0.0f;
	if (Game_GetFrame() % 30 * 3 == 0 && Wobj_IsGrouneded(wobj))
		wobj->vel_y -= wobj->jump;
	wobj->vel_y += 0.5f;
	WobjPhysics_EndUpdate(wobj);

	if (Game_GetFrame() % 29 == 0)
		Interaction_PlaySound(wobj, 20);

	if (Game_GetFrame() % 30 * 2 == 0)
	{
		WOBJ *fireball = Interaction_CreateWobj
		(
			WOBJ_FIREBALL, wobj->x, wobj->y, 0,
			(wobj->custom_ints[0]) ? CNM_2RAD(180.0f) : 0.0f
		);
		fireball->speed = 7.0f;

		fireball = Interaction_CreateWobj
		(
			WOBJ_FIREBALL, wobj->x, wobj->y, 0,
			(wobj->custom_ints[0]) ? CNM_2RAD(172.0f) : CNM_2RAD(8.0f)
		);
		fireball->speed = 7.0f;
	}
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}

void WobjTTMinionSmall_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	wobj->anim_frame = 0;
	wobj->health = 1.0f;
	wobj->strength = 15.0f;
	wobj->local_data = NULL;
	wobj->custom_ints[1] = 0;
}
void WobjTTMinionSmall_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);
	if (Game_GetFrame() % 29 == 0)
	{
		WOBJ *closest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Interaction_GetDistanceToWobj(wobj, closest_player) <= 140.0f)
		{
			if (wobj->local_data == NULL)
			{
				if (wobj->x > closest_player->x + 96.0f)
					wobj->x = closest_player->x + 96.0f;
				if (wobj->x < closest_player->x - 96.0f)
					wobj->x = closest_player->x - 96.0f;
			}
			wobj->local_data = closest_player;
		}
		else
		{
			wobj->local_data = NULL;
			wobj->custom_ints[1] = 0;
		}
	}

	if (wobj->local_data != NULL)
	{
		if (wobj->custom_ints[1]++ == 15)
			Interaction_PlaySound(wobj, 21);

		WOBJ *cp = wobj->local_data;
		wobj->y = cp->y;
		if ((Game_GetFrame() / 60) % 2 == 0)
		{
			if (wobj->x > cp->x - 64.0f)
				wobj->x -= 3.0f;
			if (wobj->x < cp->x + 64.0f)
				wobj->x += 3.0f;
		}
		else
		{
			if (wobj->x < cp->x - 8.0f)
				wobj->x += 7.0f;
			if (wobj->x > cp->x + 8.0f)
				wobj->x -= 7.0f;
		}
	}
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}

void WobjTTMinionBig_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 64.0f;
	wobj->anim_frame = 0;
	wobj->health = 10.0f;
	wobj->strength = 3.0f;
	wobj->local_data = NULL;
	wobj->custom_floats[0] = CNM_2RAD(1.0f);
	wobj->custom_floats[1] = 0.0f;
}
void WobjTTMinionBig_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);
	if (Game_GetFrame() % 28 == 0 && wobj->local_data == NULL)
	{
		WOBJ *closest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Interaction_GetDistanceToWobj(wobj, closest_player) <= 140.0f)
			wobj->local_data = closest_player;
		else
			wobj->local_data = NULL;
	}

	if (wobj->local_data != NULL)
	{
		WOBJ *cp = wobj->local_data;
		if ((Game_GetFrame() / 60) % 2 == 0)
		{
			wobj->x = cp->x + cosf(wobj->custom_floats[0]) * 96.0f;
			wobj->y = cp->y + sinf(wobj->custom_floats[0]) * 96.0f;
			wobj->custom_floats[0] += wobj->custom_floats[1];
			wobj->custom_floats[1] += CNM_2RAD(0.5f);

			int scream_speed = 15;
			scream_speed -= (int)(wobj->custom_floats[1] * 10.0f);
			if (scream_speed < 1)
				Interaction_PlaySound(wobj, 21);
			else if (Game_GetFrame() % 15 == 0)
				Interaction_PlaySound(wobj, 21);
		}

		if (Game_GetFrame() % 30 == 0)
		{
			WOBJ *fireball = Interaction_CreateWobj
			(
				WOBJ_FIREBALL,
				wobj->x,
				wobj->y,
				0,
				atan2f(cp->y - wobj->y, cp->x - wobj->x)
			);
		}
	}
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}

void WobjWolf_Create(WOBJ *wobj) {
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 7.5f;
	wobj->strength = 0.83333f;
	wobj->speed = 6.0f;
	Util_SetBox(&wobj->hitbox, 8.0f, 16.0f, 48.0f, 16.0f);
	wobj->money = 100;
	wobj->custom_ints[0] = Util_RandInt(10, 30 * 3);
	wobj->custom_ints[1] = 0;
	wobj->custom_floats[0] = (float)(Util_RandInt(0, 1) * 2 - 1) * wobj->speed;
}
void WobjSlimeWalker_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 18.0f;
	wobj->strength = 1.5f;
	wobj->speed = 3.0f;
	wobj->hitbox.x = 8.0f;
	wobj->hitbox.y = 16.0f;
	wobj->hitbox.w = 48.0f;
	wobj->hitbox.h = 48.0f;
	wobj->money = 100;
	wobj->item = 0;
	wobj->custom_ints[0] = Util_RandInt(10, 30 * 3);
	wobj->custom_ints[1] = 0;
	wobj->custom_floats[0] = (float)(Util_RandInt(0, 1) * 2 - 1) * wobj->speed;
}
void WobjSlimeWalker_Update(WOBJ *wobj)
{
	if (wobj->item == 0) {
		WOBJ *closest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Interaction_GetDistanceToWobj(wobj, closest_player) <= (float)256.0f) {
			wobj->item = 1;
		}
		Wobj_DoEnemyCry(wobj, 45);
		WobjPhysics_BeginUpdate(wobj);
		wobj->vel_y += 0.5f;
		WobjPhysics_EndUpdate(wobj);
		return;
	}

	Wobj_DoEnemyCry(wobj, 45);
	WobjPhysics_BeginUpdate(wobj);
	if (wobj->custom_ints[1] == 0)
	{
		wobj->custom_ints[0]--;
		if (wobj->custom_ints[0] <= 0)
		{
			WOBJ *closest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
			wobj->custom_floats[0] = (closest_player->x > wobj->x) ? wobj->speed : -wobj->speed;
			wobj->custom_ints[0] = Util_RandInt(10, 28);
		}
	}
	if (wobj->custom_ints[1] == 1 && Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 3.0f) && wobj->vel_y >= 0.0f)
	{
		wobj->custom_ints[1] = 0;
		//wobj->custom_floats[0] = (wobj->custom_floats[0] >= 0.0f) ? wobj->speed : -wobj->speed;
		WOBJ *closest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		wobj->custom_floats[0] = (closest_player->x > wobj->x) ? wobj->speed : -wobj->speed;
		Interaction_PlaySound(wobj, 22);
	}
	if (Game_GetFrame() % 45 == rand() % 45)
	{
		WOBJ *closest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Interaction_GetDistanceToWobj(wobj, closest_player) <= (float)256.0f && wobj->custom_ints[1] == 0 && Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 3.0f))
		{
			wobj->custom_ints[1] = 1;
			float time_jump = 20.0f;
			if (wobj->type == WOBJ_WOLF) time_jump = 28.5f;
			float think_x = closest_player->x + closest_player->vel_x * time_jump;
			float spd = (think_x - wobj->x) / time_jump;
			wobj->custom_floats[0] = spd;
			wobj->vel_y = -10.0f;
			Interaction_PlaySound(wobj, 23);
		}
	}

	if (Game_GetFrame() % 12 == 0 && wobj->type == SLIME_WALKER)
		Interaction_PlaySound(wobj, 24);

	if (Game_GetFrame() % 1000 == (rand() % 1000) && wobj->type == WOBJ_WOLF)
		Interaction_PlaySound(wobj, 50);

	if (wobj->custom_floats[0] < 0.0f)
		wobj->flags |= WOBJ_HFLIP;
	else
		wobj->flags &= ~WOBJ_HFLIP;

	wobj->vel_x = wobj->custom_floats[0];
	//wobj->x += wobj->custom_floats[0];
	//wobj->y += wobj->vel_y;
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
	WobjPhysics_EndUpdate(wobj);
	//Wobj_ResolveBlocksCollision(wobj);
	float grav = Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
	if (wobj->type == SLIME_WALKER)
		grav *= 2.0f;
	if (Wobj_IsCollidingWithBlocksOrObjects(wobj, ((wobj->custom_floats[0] >= 0.0f) ? 1.0f : -1.0f), 0.0f))
		wobj->custom_floats[0] *= -1.0f;
	if (Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 2.0f) && wobj->custom_ints[1] == 0)
		wobj->vel_y = 0.0f;
	else
		wobj->vel_y += grav;
}
void WobjSlimeWalker_Draw(WOBJ *wobj, int camx, int camy)
{
	if (wobj->type == SLIME_WALKER)
		wobj->anim_frame = (Game_GetFrame() / 12) % 4;
	else
		wobj->anim_frame = (Game_GetFrame() / 4) % 4;
	WobjGeneric_Draw(wobj, camx, camy);
}

void WobjMegaFish_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 3.0f;
	wobj->strength = 1.33333333f;
	wobj->speed = wobj->custom_floats[0];
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	wobj->custom_floats[1] = (float)Util_RandInt(0, 360);
}
void WobjMegaFish_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);
	WobjPhysics_BeginUpdate(wobj);
	wobj->vel_x = cosf(CNM_2RAD(wobj->custom_floats[1])) * wobj->speed;
	wobj->vel_y = sinf(CNM_2RAD(wobj->custom_floats[1])) * wobj->speed;

	if (wobj->speed >= 0.1f)
		wobj->speed -= 0.1f;
	if (Game_GetFrame() % 50 == 0)
	{
		wobj->speed = wobj->custom_floats[0];
		wobj->custom_floats[1] = (float)Util_RandInt(0, 360);
		Interaction_PlaySound(wobj, 26);
	}

	WobjPhysics_EndUpdate(wobj);
	if (Wobj_IsCollidingWithBlocksOrObjects(wobj, wobj->vel_x * 2.0f, wobj->vel_y * 2.0f) || wobj->y < (float)wobj->custom_ints[0])
		wobj->custom_floats[1] -= 180.0f;
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}

// Lava Dragon Code
#define LAVADRAGON_MAX_LENGTH 32
#define LAVADRAGON_HISTORY (30*10)
#define LAVADRAGON_STATE_IDLE 0
#define LAVADRAGON_STATE_FLYING 1
#define LAVADRAGON_STATE_UNUSED 2
#define LAVADRAGON_STATE_ATTACKING 3
typedef struct _LAVADRAGON_DATA
{
	int length;
	WOBJ *links[LAVADRAGON_MAX_LENGTH];
	int state;
	int state_timer;
	int has_target;
	float tx_spd, ty_spd;
	float x_history[LAVADRAGON_HISTORY];
	float y_history[LAVADRAGON_HISTORY];
	int history_index;
} LAVADRAGON_DATA;

void WobjLavaDragonHead_Death(WOBJ *wobj)
{
	LAVADRAGON_DATA *data = wobj->local_data;

	for (int i = 0; i < data->length; i++)
	{
		if (data->links[i] != NULL)
			Interaction_DestroyWobj(data->links[i]);
		data->links[i] = NULL;
	}

	free(data);
}
void WobjLavaDragonHead_Create(WOBJ *wobj)
{
	if (wobj->internal.owned) {
		LAVADRAGON_DATA *data = malloc(sizeof(LAVADRAGON_DATA));
		wobj->local_data = data;
		data->tx_spd = 0.0f;
		data->ty_spd = 0.0f;
		data->length = wobj->custom_ints[0];
		memset(data->links, 0, sizeof(data->links));
		for (int i = 0; i < data->length; i++)
		{
			float lx = wobj->x - i * 48;

			switch (data->length - i)
			{
			case 1: data->links[i] = Interaction_CreateWobj(LAVA_DRAGON_BODY, lx, wobj->y, 3, 0.0f); break;
			case 2: data->links[i] = Interaction_CreateWobj(LAVA_DRAGON_BODY, lx, wobj->y, 2, 0.0f); break;
			case 3: data->links[i] = Interaction_CreateWobj(LAVA_DRAGON_BODY, lx, wobj->y, 1, 0.0f); break;
			default: data->links[i] = Interaction_CreateWobj(LAVA_DRAGON_BODY, lx, wobj->y, 0, 0.0f); break;
			}
		}
		for (int i = 0; i < LAVADRAGON_HISTORY; i++)
		{
			data->x_history[i] = wobj->x;
			data->y_history[i] = wobj->y;
		}
		data->history_index = 0;
		data->state = LAVADRAGON_STATE_IDLE;
		data->state_timer = 0;
		data->has_target = CNM_FALSE;
		wobj->on_destroy = WobjLavaDragonHead_Death;
	}
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->anim_frame = 0;
	wobj->link_node = 0;
	wobj->link_uuid = 0;
	wobj->speed = 0.0f;
	wobj->strength = 1.66666667f;
	wobj->health = wobj->custom_floats[0];
	if (wobj->health == 0.0f)
		wobj->health = 30.0f;
	Util_SetBox(&wobj->hitbox, 1.0f, 1.0f, 62.0f, 62.0f);
}
void WobjLavaDragonHead_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);

	LAVADRAGON_DATA *data = wobj->local_data;
	WOBJ *target = NULL;
	
	if (Game_GetFrame() % 15 == 0 && Util_RandInt(0, 100) > 70)
		Interaction_PlaySound(wobj, 27);

	if (!data->has_target)
	{
		data->state = LAVADRAGON_STATE_IDLE;

		WOBJ *nearest = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Interaction_GetDistanceToWobj(wobj, nearest) < 128.0f)
		{
			data->has_target = CNM_TRUE;
			wobj->link_node = nearest->node_id;
			wobj->link_uuid = nearest->uuid;
		}
	}
	else
	{
		target = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
		if (target == NULL)
		{
			data->has_target = CNM_FALSE;
			data->state = LAVADRAGON_STATE_IDLE;
			wobj->link_node = 0;
			wobj->link_uuid = 0;
		}
	}

	if (data->state == LAVADRAGON_STATE_IDLE)
	{
		data->state_timer = 0;
		if (data->has_target)
			data->state = LAVADRAGON_STATE_FLYING;
	}
	else if (data->state == LAVADRAGON_STATE_FLYING && target != NULL)
	{
		if (Game_GetFrame() % 30 == 0)
		{
			float player_ang = atan2f(target->y - wobj->y, target->x - wobj->x);
			data->tx_spd = cosf(player_ang) * 12.0f;
			data->ty_spd = sinf(player_ang) * 12.0f;
		}
		if (wobj->vel_x > data->tx_spd)
			wobj->vel_x -= 0.1f;
		if (wobj->vel_x < data->tx_spd)
			wobj->vel_x += 0.1f;
		if (wobj->vel_y > data->ty_spd)
			wobj->vel_y -= 0.1f;
		if (wobj->vel_y < data->ty_spd)
			wobj->vel_y += 0.1f;

		if (wobj->x < target->x)
			wobj->vel_x += 0.15f;
		if (wobj->x > target->x)
			wobj->vel_x -= 0.15f;
		if (wobj->y < target->y)
			wobj->vel_y += 0.15f;
		if (wobj->y > target->y)
			wobj->vel_y -= 0.15f;

		data->state_timer++;
		if (data->state_timer > 30*10)
		{
			data->state_timer = 0;
			data->state = LAVADRAGON_STATE_ATTACKING;
		}

		if (Game_GetFrame() % 45 == 0)
		{
			WOBJ *blob = Interaction_CreateWobj(LAVA_DRAGON_BLOB, wobj->x, wobj->y, 0, atan2f(target->y - wobj->y, target->x - wobj->x));
			blob->strength = 50.0f;
			blob->speed = 6.0f;
		}
	}
	else if (data->state == LAVADRAGON_STATE_ATTACKING)
	{
		if (Interaction_GetDistanceToWobj(wobj, target) <= 48.0f)
		{
			data->state = LAVADRAGON_STATE_FLYING;
		}

		if (wobj->x < target->x)
		{
			wobj->vel_x += 0.4f;
			if (wobj->vel_x < -1.0f)
				wobj->vel_x += 0.4f;
		}
		else if (wobj->x > target->x)
		{
			wobj->vel_x -= 0.4f;
			if (wobj->vel_x > 1.0f)
				wobj->vel_x -= 0.4f;
		}
		if (wobj->y < target->y)
		{
			wobj->vel_y += 0.4f;
			if (wobj->vel_y < -1.0f)
				wobj->vel_y += 0.4f;
		}
		else if (wobj->y > target->y)
		{
			wobj->vel_y -= 0.4f;
			if (wobj->vel_y > 1.0f)
				wobj->vel_y -= 0.4f;
		}
	}

	if (data->state == LAVADRAGON_STATE_UNUSED || data->state == LAVADRAGON_STATE_FLYING || data->state == LAVADRAGON_STATE_ATTACKING)
	{
		const float max_speed = 320.0f / 15.0f;
		if (wobj->vel_x < -max_speed)
			wobj->vel_x = -max_speed;
		if (wobj->vel_x > max_speed)
			wobj->vel_x = max_speed;
		if (wobj->vel_y > max_speed)
			wobj->vel_y = max_speed;
		if (wobj->vel_y < -max_speed)
			wobj->vel_y = -max_speed;

		wobj->x += wobj->vel_x;
		wobj->y += wobj->vel_y;

		if (data->state == LAVADRAGON_STATE_UNUSED)
		{
			if (data->state_timer++ > 50)
			{
				data->state_timer = 0;
				data->state = LAVADRAGON_STATE_FLYING;
			}
		}
	}

	if (--data->history_index < 0)
		data->history_index = LAVADRAGON_HISTORY - 1;
	data->x_history[data->history_index] = wobj->x;
	data->y_history[data->history_index] = wobj->y;

	for (int i = 0; i < data->length; i++)
	{
		WOBJ *link = data->links[i];
		if (link == NULL)
			continue;

		link->x = data->x_history[(data->history_index + (i+1) * 5) % LAVADRAGON_HISTORY];
		link->y = data->y_history[(data->history_index + (i+1) * 5) % LAVADRAGON_HISTORY];

		if (link->anim_frame == 3)
		{
			link->x += 24.0f;
			link->y += 24.0f;
		}
		else if (link->anim_frame == 2)
		{
			link->x += 16.0f;
			link->y += 16.0f;
		}

		Wobj_UpdateGridPos(link);
	}

	// Change Frame Based On Angle
	float move_angle = atan2f(-wobj->vel_y, wobj->vel_x);
	int show_ang = (int)(move_angle / CNM_PI * 180.0f) % 360;
	if (show_ang < 0)
		show_ang += 360;
	if (show_ang < 270 && show_ang >= 90)
		wobj->flags |= WOBJ_HFLIP;
	else
		wobj->flags &= ~WOBJ_HFLIP;
	if (show_ang < 360 && show_ang >= 324)
		wobj->anim_frame = 3;
	if (show_ang < 324 && show_ang >= 288)
		wobj->anim_frame = 4;
	if (show_ang < 36 && show_ang >= 0)
		wobj->anim_frame = 0;
	if (show_ang < 72 && show_ang >= 36)
		wobj->anim_frame = 1;
	if (show_ang < 90 && show_ang >= 72)
		wobj->anim_frame = 2;

	if (show_ang < 270 && show_ang >= 234)
		wobj->anim_frame = 4;
	if (show_ang < 234 && show_ang >= 198)
		wobj->anim_frame = 3;
	if (show_ang < 198 && show_ang >= 162)
		wobj->anim_frame = 0;
	if (show_ang < 162 && show_ang >= 126)
		wobj->anim_frame = 1;
	if (show_ang < 126 && show_ang >= 90)
		wobj->anim_frame = 2;

	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}
void WobjLavaDragonBody_Create(WOBJ *wobj)
{
	wobj->anim_frame = wobj->custom_ints[0];
	switch (wobj->anim_frame)
	{
	case 0: Util_SetBox(&wobj->hitbox, 1.0f, 1.0f, 62.0f, 62.0f); break;
	case 1: Util_SetBox(&wobj->hitbox, 8.0f, 8.0f, 48.0f, 48.0f); break;
	case 2: Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f); break;
	case 3: Util_SetBox(&wobj->hitbox, 8.0f, 8.0f, 16.0f, 16.0f); break;
	}
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 100000.0f;
	wobj->strength = 1.666666667f;
}

#define TTBOSS_MAX_WAYPOINTS 128
#define TTBOSS_STATE_SEARCHING 0
#define TTBOSS_STATE_ATTACKING 1
static float ttboss_x[TTBOSS_MAX_WAYPOINTS];
static float ttboss_y[TTBOSS_MAX_WAYPOINTS];
static int ttboss_num_waypoints = 0;
static int ttboss_state = TTBOSS_STATE_SEARCHING;

#define TTBOSS_PROVOKED custom_ints[1]

void TTBoss_ResetOnLevelLoad(void)
{
	memset(ttboss_x, 0, sizeof(ttboss_x));
	memset(ttboss_y, 0, sizeof(ttboss_y));
	ttboss_num_waypoints = 0;
	ttboss_state = TTBOSS_STATE_SEARCHING;
}
void TTBoss_CalmDown(void)
{
	ttboss_state = TTBOSS_STATE_SEARCHING;
}
void WobjTTBossTriggerGeneric_Create(WOBJ *wobj)
{
	wobj->anim_frame = 1;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
	wobj->custom_ints[1] = 0;
}
void WobjTTBossTriggerNormal_Update(WOBJ *wobj)
{
	wobj->custom_ints[1]--;
	if (wobj->custom_ints[1] <= 0)
		wobj->anim_frame = 1;
	else
		wobj->anim_frame = 2;
}
void WobjTTBossWaypoint_Create(WOBJ *wobj)
{
	wobj->anim_frame = 1;
	ttboss_x[wobj->custom_ints[0]] = wobj->x;
	ttboss_y[wobj->custom_ints[0]] = wobj->y;
	if (wobj->custom_ints[0] >= ttboss_num_waypoints)
		ttboss_num_waypoints = wobj->custom_ints[0] + 1;
}
void WobjTTBoss_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 16.67f;
	wobj->health = 75.0f;
	wobj->speed = wobj->custom_floats[0];
	wobj->custom_floats[0] = 0.0f;
	wobj->custom_ints[0] = 0;
	wobj->TTBOSS_PROVOKED = CNM_FALSE;
	wobj->link_node = 0;
	wobj->link_uuid = 0;
	wobj->custom_floats[1] = 134.0f;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}
void WobjTTBoss_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);

	if (!wobj->TTBOSS_PROVOKED)
	{
		WOBJ *nearest = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Interaction_GetDistanceToWobj(wobj, nearest) < 128.0f)
		{
			wobj->TTBOSS_PROVOKED = CNM_TRUE;
			wobj->link_node = nearest->node_id;
			wobj->link_uuid = nearest->uuid;
		}
	}
	else
	{
		if (Game_GetFrame() % 30 * 3 == 0)
		{
			WOBJ *nearest = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
			wobj->link_node = nearest->node_id;
			wobj->link_uuid = nearest->uuid;
		}

		WOBJ *target = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
		if (target == NULL)
			return;

		if (ttboss_state == TTBOSS_STATE_SEARCHING)
		{
			if (ttboss_x[wobj->custom_ints[0]] < wobj->x)
				wobj->x -= wobj->speed;
			if (ttboss_x[wobj->custom_ints[0]] > wobj->x)
				wobj->x += wobj->speed;
			if (ttboss_y[wobj->custom_ints[0]] < wobj->y)
				wobj->y -= wobj->speed;
			if (ttboss_y[wobj->custom_ints[0]] > wobj->y)
				wobj->y += wobj->speed;

			int scream_rate = (int)(Interaction_GetDistanceToWobj(target, wobj) / 160.0f * 45.0f) - 20;
			if (scream_rate > 45)
				scream_rate = 45;

			if (scream_rate < 1)
				Interaction_PlaySound(wobj, 21);
			else if (Game_GetFrame() % scream_rate == 0)
				Interaction_PlaySound(wobj, 21);

			wobj->custom_floats[0] = 0.0f;
			if (Wobj_GetWobjCollidingWithType(wobj, TT_CHASE_TRIGGER) != NULL)
				ttboss_state = TTBOSS_STATE_ATTACKING;

			if (fabsf(ttboss_x[wobj->custom_ints[0]] - wobj->x) < wobj->speed * 1.5f &&
				fabsf(ttboss_y[wobj->custom_ints[0]] - wobj->y) < wobj->speed * 1.5f)
			{
				if (wobj->custom_ints[0] <= 0)
				{
					wobj->custom_ints[0] = 0;
					wobj->custom_floats[1] = 134.0f;
				}
				if (wobj->custom_ints[0] + 1 >= ttboss_num_waypoints)
				{
					wobj->custom_ints[0] = ttboss_num_waypoints - 1;
					wobj->custom_floats[1] = -134.0f;
				}

				if (wobj->custom_floats[1] >= 0.0f)
					wobj->custom_ints[0]++;
				else
					wobj->custom_ints[0]--;
			}
		}
		else
		{
			if (wobj->custom_floats[0]++ >= 30.0f*15.0f ||
				sqrtf((target->x-wobj->x)*(target->x-wobj->x)+(target->y-wobj->y)*(target->y-wobj->y)) > 
					(float)RENDERER_WIDTH * 2.5f)
				ttboss_state = TTBOSS_STATE_SEARCHING;

			Interaction_PlaySound(wobj, 21);

			if (target->x < wobj->x)
				wobj->x -= wobj->speed * 1.25f;
			if (target->x > wobj->x)
				wobj->x += wobj->speed * 1.25f;
			if (target->y < wobj->y)
				wobj->y -= wobj->speed * 1.25f;
			if (target->y > wobj->y)
				wobj->y += wobj->speed * 1.25f;
		}
	}

	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}

void WobjEaterBug_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 1.6666667f;
	wobj->health = 2.0f;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 96.0f);
	wobj->custom_ints[0] = 0;
	wobj->speed = wobj->custom_floats[0];
	wobj->custom_floats[0] = wobj->y;
	wobj->custom_floats[1] = wobj->y + 92.0f;
}
void WobjEaterBug_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);

	if (wobj->custom_ints[0] == 0)
	{
		WOBJ *nearest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (fabsf(nearest_player->x - (wobj->x + 16.0f)) < 32.0f && wobj->y <= wobj->custom_floats[0])
			wobj->custom_ints[0] = 1;
		if (wobj->y > wobj->custom_floats[0])
			wobj->y -= wobj->speed;
		else
			wobj->y = wobj->custom_floats[0];
	}
	else
	{
		if (wobj->y < wobj->custom_floats[1])
		{
			wobj->y += wobj->speed;
			Interaction_PlaySound(wobj, 29);
		}
		else
		{
			wobj->y = wobj->custom_floats[1] + 0.5f;
			if (wobj->custom_ints[0]++ > 30 * 1)
			{
				wobj->custom_ints[0] = 0;
			}
		}
	}

	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}

// Spider Walker
void WobjSpiderWalker_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 2.0f, 32.0f, 12.0f);
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->anim_frame = 0;
	wobj->health = 2.8f;
	wobj->strength = 0.66666667f;
	wobj->speed = wobj->custom_floats[0];
	wobj->custom_floats[0] = 1.0f;
	wobj->custom_ints[1] = Util_RandInt(30, 30*3);
	wobj->custom_ints[0] = 0;
}
void WobjSpiderWalker_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);
	if (Game_GetFrame() % 5 == 0 && wobj->custom_ints[0] < 0)
	{
		WOBJ *np = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Game_GetFrame() % (5 * 60) == 0 || fabsf(np->x - (wobj->x + 16.0f)) < 128.0f)
		{
			wobj->custom_ints[0] = (int)((float)(16 + 45) * 1.5f);
			Interaction_CreateWobj(SPIDER_WALKER_WEB, wobj->x, wobj->y + 16.0f, 0, 0.0f);
			Interaction_PlaySound(wobj, 31);
		}
	}

	if (wobj->custom_ints[0]-- > 45)
		return;
	if (wobj->custom_ints[0] == 44)
		Interaction_PlaySound(wobj, 29);

	if (wobj->custom_ints[1]-- <= 0 || Wobj_IsCollidingWithBlocksOrObjects(wobj, wobj->custom_floats[0] * 3.0f, 0.0f) ||
		!Wobj_IsCollidingWithBlocksOrObjects(wobj, wobj->custom_floats[0] * wobj->hitbox.w, -5.0f))
	{
		wobj->custom_ints[1] = Util_RandInt(30, 30*3);
		wobj->custom_floats[0] *= -1.0f;
		wobj->flags ^= WOBJ_HFLIP;
	}

	wobj->x += wobj->custom_floats[0] * wobj->speed;

	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}
void WobjSpiderWalkerWeb_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 2.0f, 32.0f, 32.0f);
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->anim_frame = 0;
	wobj->strength = 1.1667f;
	wobj->custom_ints[1] = 0;
}
void WobjSpiderWalkerWeb_Update(WOBJ *wobj)
{
	wobj->y += 0.75f;
	if (wobj->custom_ints[1]++ > 30 * 10 || Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, -12.0f))
		Interaction_DestroyWobj(wobj);
}

void WobjSpikeTrap_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->anim_frame = 0;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
	wobj->strength = 0.0f;
	wobj->custom_ints[0] = 0;
}
void WobjSpikeTrap_Update(WOBJ *wobj)
{
	if ((Game_GetFrame() - 1) % 2 == 0)
	{
		WOBJ *nearest_player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		float dist = Interaction_GetDistanceToWobj(nearest_player, wobj);
		if (wobj->custom_ints[0] <= 0)
		{
			if (dist < 64.0f)
				wobj->anim_frame = 0;
			else
				wobj->anim_frame = 3;
		}

		if (wobj->anim_frame == 0)
		{
			if (fabsf(nearest_player->x - (wobj->x + 16.0f)) < 32.0f)
				wobj->custom_ints[0] = 45;
		}

		if (wobj->custom_ints[0] > 0)
		{
			wobj->custom_ints[0]--;
			if (wobj->custom_ints[0] >= 43 || wobj->custom_ints[0] <= 2)
				wobj->anim_frame = 1;
			else
			{
				wobj->anim_frame = 2;
				wobj->strength = 100.0f;
				if (wobj->custom_ints[0] == 42)
					Interaction_PlaySound(wobj, 34);
			}
		}
		else
		{
			wobj->strength = 0.0f;
		}
	}
}

void WobjRotatingFireColunmPiece_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 25.0f;
	wobj->speed = 0.0;
	wobj->custom_floats[0] = CNM_2RAD(wobj->custom_floats[0]);
	//wobj->custom_ints[0] = wobj->custom_ints[0];
	wobj->custom_ints[1] = (int)wobj->y;
	wobj->vel_x = wobj->x - (float)wobj->custom_ints[0]; // This is how far away it is from the center
	//if (wobj->x < wobj->custom_ints[0]) wobj->speed = CNM_PI;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}
void WobjRotatingFireColunmPiece_Update(WOBJ *wobj)
{
	wobj->speed += wobj->custom_floats[0];
	wobj->x = (float)wobj->custom_ints[0] + (cosf(wobj->speed) * wobj->vel_x);
	wobj->y = (float)wobj->custom_ints[1] - (sinf(wobj->speed) * wobj->vel_x);
}
void WobjRotatingFireColunmPiece_Draw(WOBJ *wobj, int camx, int camy)
{
	wobj->anim_frame = Game_GetFrame() % 2 + 1;
	WobjGeneric_Draw(wobj, camx, camy);
}

void WobjMovingFireVertical_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 25.0f;

	wobj->item = wobj->custom_ints[0] >> 16;
	wobj->custom_ints[0] = (wobj->custom_ints[0] << 16) >> 16;
	if (wobj->item == 0 || wobj->item == -1) {
		wobj->item = (10 << 12) | 70;
	}

	wobj->money = wobj->custom_floats[0] < 0;
	wobj->speed = fabsf(wobj->custom_floats[0]);
	wobj->custom_floats[0] = wobj->y;
	wobj->custom_floats[1] = wobj->y + (float)wobj->custom_ints[0];
	wobj->health = wobj->speed / 10.0f;
	wobj->jump = fabsf((wobj->speed * wobj->speed) / (2.0f * wobj->health));
	if (wobj->custom_floats[1] < wobj->custom_floats[0]) {
		float tmp = wobj->custom_floats[1];
		wobj->custom_floats[1] = wobj->custom_floats[0];
		wobj->custom_floats[0] = tmp;
		wobj->speed *= -1.f;
		wobj->health *= -1.0f;
	}
	wobj->vel_y = 0.0f;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}
void WobjMovingFireVertical_Update(WOBJ *wobj)
{
	wobj->y += wobj->vel_y;

	if (wobj->y < wobj->custom_floats[0] + wobj->jump) {
		if (wobj->speed > 0.0f) wobj->vel_y += wobj->health;
		else wobj->vel_y -= wobj->health;

		if (wobj->money) {
			if ((wobj->speed > 0.0f && wobj->vel_y < 0.0f) ||
				(wobj->speed < 0.0f && wobj->vel_y > 0.0f))
				Interaction_DestroyWobj(wobj);
		}
	} else if (wobj->y > wobj->custom_floats[1] - wobj->jump) {
		if (wobj->speed > 0.0f) wobj->vel_y -= wobj->health;
		else wobj->vel_y += wobj->health;

		if (wobj->money) {
			if ((wobj->speed > 0.0f && wobj->vel_y < 0.0f) ||
				(wobj->speed < 0.0f && wobj->vel_y > 0.0f))
				Interaction_DestroyWobj(wobj);
		}
	}
}
void WobjMovingFireHorizontal_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 25.0f;

	wobj->item = wobj->custom_ints[0] >> 16;
	wobj->custom_ints[0] = (wobj->custom_ints[0] << 16) >> 16;
	if (wobj->item == 0 || wobj->item == -1) {
		wobj->item = (10 << 12) | 70;
	}

	wobj->money = wobj->custom_floats[0] < 0;
	wobj->speed = fabsf(wobj->custom_floats[0]);
	wobj->custom_floats[0] = wobj->x;
	wobj->custom_floats[1] = wobj->x + (float)wobj->custom_ints[0];
	wobj->health = wobj->speed / 10.0f;
	wobj->jump = fabsf((wobj->speed * wobj->speed) / (2.0f * wobj->health));
	if (wobj->custom_floats[1] < wobj->custom_floats[0]) {
		float tmp = wobj->custom_floats[1];
		wobj->custom_floats[1] = wobj->custom_floats[0];
		wobj->custom_floats[0] = tmp;
		wobj->speed *= -1.f;
		wobj->health *= -1.0f;
	}
	wobj->vel_x = 0.0f;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}
void WobjMovingFireHorizontal_Update(WOBJ *wobj)
{
	wobj->x += wobj->vel_x;
	if (wobj->x < wobj->custom_floats[0] + wobj->jump) {
		if (wobj->speed > 0.0f) wobj->vel_x += wobj->health;
		else wobj->vel_x -= wobj->health;

		if (wobj->money) {
			if ((wobj->speed > 0.0f && wobj->vel_x < 0.0f) ||
				(wobj->speed < 0.0f && wobj->vel_x > 0.0f))
				Interaction_DestroyWobj(wobj);
		}
	} else if (wobj->x > wobj->custom_floats[1] - wobj->jump) {
		if (wobj->speed > 0.0f) wobj->vel_x -= wobj->health;
		else wobj->vel_x += wobj->health;

		if (wobj->money) {
			if ((wobj->speed > 0.0f && wobj->vel_x < 0.0f) ||
				(wobj->speed < 0.0f && wobj->vel_x > 0.0f))
				Interaction_DestroyWobj(wobj);
		}
	}
}
void WobjMovingFire_Draw(WOBJ *wobj, int camx, int camy) {
	Renderer_DrawBitmap2
	(
		(int)wobj->x - camx,
		(int)wobj->y - camy,
		&(CNM_RECT){
			.x = (((wobj->item >> 12) & 0xf) * 32) + ((Game_GetFrame() & 1) * 32),
			.y = (wobj->item & 0xfff) * 32,
			.w = 32,
			.h = 32,
		},
		0,
		Blocks_GetCalculatedBlockLight(((int)wobj->x + 16) / BLOCK_SIZE, ((int)wobj->y + 16) / BLOCK_SIZE),
		0,
		0
	);

	if (Game_GetVar(GAME_VAR_SHOW_COLLISION_BOXES)->data.integer)
	{
		CNM_RECT r;
		Util_SetRect(&r, (int)(wobj->x + wobj->hitbox.x) - camx, (int)(wobj->y + wobj->hitbox.y) - camy,
					 (int)wobj->hitbox.w, (int)wobj->hitbox.h);
		Renderer_DrawRect(
			&r,
			Renderer_MakeColor(255, 0, 255),
			2,
			RENDERER_LIGHT
		);
	}
}

// TODO: Update super dragon
#define MAX_SUPER_DRAGONS 32
typedef struct _SUPER_DRAGON_DATA
{
	int state;
	int state_timer;
	float land_x;
	float land_y;
	float origin_x;
	float origin_y;
	float shooting_dir;
	float angle;
} SUPER_DRAGON_DATA;

SUPER_DRAGON_DATA super_dragons[MAX_SUPER_DRAGONS];

#define SUPER_DRAGON_STATE_IDLE 4
#define SUPER_DRAGON_STATE_FLYING 0
#define SUPER_DRAGON_STATE_LANDING 1
#define SUPER_DRAGON_STATE_SHOOTING 2
#define SUPER_DRAGON_STATE_TAKINGOFF 3

void WobjSuperDragonBoss_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->anim_frame = 0;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 128.0f, 128.0f);
	wobj->strength = 1.666666667f;
	wobj->health = 150.0f;

	super_dragons[wobj->custom_ints[0]].state = SUPER_DRAGON_STATE_IDLE;
	super_dragons[wobj->custom_ints[0]].state_timer = 0;
	wobj->local_data = super_dragons + wobj->custom_ints[0];
	super_dragons[wobj->custom_ints[0]].origin_x = wobj->x;
	super_dragons[wobj->custom_ints[0]].origin_y = wobj->y;
	super_dragons[wobj->custom_ints[0]].shooting_dir = 1.0f;
}
void WobjSuperDragonBoss_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);

	SUPER_DRAGON_DATA *data = wobj->local_data;

	if (Game_GetFrame() % 25 == 0 && Util_RandInt(0, 100) > 69)
		Interaction_PlaySound(wobj, Util_RandInt(0, 99) / 50 + 27);

	if (data->state == SUPER_DRAGON_STATE_IDLE)
	{
		if (Game_GetFrame() % 60 == 0)
		{
			WOBJ *np = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
			if (Interaction_GetDistanceToWobj(np, wobj) < 320.0f)
			{
				data->state = SUPER_DRAGON_STATE_FLYING;
			}
		}
	}
	if (data->state == SUPER_DRAGON_STATE_LANDING)
	{
		const float spd = 4.5f;
		if (wobj->x > data->land_x)
			wobj->x -= spd;
		if (wobj->x < data->land_x)
			wobj->x += spd;
		if (wobj->y > data->land_y)
			wobj->y -= spd;
		if (wobj->y < data->land_y)
			wobj->y += spd;

		if (fabsf(wobj->x - data->land_x) < spd * 1.5f && fabsf(wobj->y - data->land_y) < spd * 1.5f)
		{
			data->state = SUPER_DRAGON_STATE_SHOOTING;
			data->state_timer = 0;

			WOBJ *np = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
			if (np->x > wobj->x)
			{
				data->shooting_dir = 1.0f;
				wobj->flags &= ~WOBJ_HFLIP;
			}
			else
			{
				data->shooting_dir = -1.0f;
				wobj->flags |= WOBJ_HFLIP;
			}
		}
	}
	else if (data->state == SUPER_DRAGON_STATE_SHOOTING)
	{
		if (Game_GetFrame() % 6 == 0)
		{
			WOBJ *np = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
			float shoot_ang = atan2f(np->y - wobj->y, np->x - wobj->x);
			Interaction_CreateWobj
			(
				WOBJ_FIREBALL,
				wobj->x + 64.0f,
				wobj->y + (float)Util_RandInt(0, 128),
				0,
				(np->x + 16.0f > wobj->x + 64.0f ? 0.0f : CNM_PI)
			);
			if (Game_GetFrame() % 6*3 == 0) {
				Interaction_CreateWobj
				(
					WOBJ_FIREBALL,
					wobj->x + 64.0f,
					wobj->y + (float)Util_RandInt(0, 128),
					0,
					shoot_ang
				);
			}
		}

		if (data->state_timer++ > 30 * 10)
		{
			data->state = SUPER_DRAGON_STATE_TAKINGOFF;
			data->state_timer = 0;
			wobj->flags &= ~WOBJ_HFLIP;
		}
	}
	else if (data->state == SUPER_DRAGON_STATE_FLYING)
	{
		float t = (float)data->state_timer / 40.0f;
		wobj->x = data->origin_x + (cosf(t)) * 250.0f;
		wobj->y = data->origin_y + (sinf(2.0f*t) / 2.0f) * 125.0f;

		if ((Game_GetFrame() + 3) % 30 == 0)
		{
			WOBJ *np = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
			float shoot_ang = atan2f(np->y - wobj->y, np->x - wobj->x);
			Interaction_CreateWobj
			(
				WOBJ_FIREBALL,wobj->x + 64.0f,wobj->y + 64.0f,0,
				shoot_ang
			);
			Interaction_CreateWobj
			(
				WOBJ_FIREBALL, wobj->x + 64.0f, wobj->y + 64.0f, 0,
				shoot_ang - CNM_2RAD(20.0f)
			);
			Interaction_CreateWobj
			(
				WOBJ_FIREBALL, wobj->x + 64.0f, wobj->y + 64.0f, 0,
				shoot_ang + CNM_2RAD(20.0f)
			);
		}

		if (data->state_timer++ >= 30 * 20)
		{
			data->state = SUPER_DRAGON_STATE_LANDING;
			data->state_timer = 0;
		}
	}
	else if (data->state == SUPER_DRAGON_STATE_TAKINGOFF)
	{
		const float spd = 8.0f;
		if (wobj->x > data->origin_x + 250.0f)
			wobj->x -= spd;
		if (wobj->x < data->origin_x + 250.0f)
			wobj->x += spd;
		if (wobj->y > data->origin_y)
			wobj->y -= spd;
		if (wobj->y < data->origin_y)
			wobj->y += spd;

		if (fabsf(wobj->x - (data->origin_x + 250.0f)) < spd * 1.5f && fabsf(wobj->y - data->origin_y) < spd * 1.5f)
		{
			data->state = SUPER_DRAGON_STATE_FLYING;
			data->state_timer = 0;
		}
	}

	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}
void WobjSuperDragonLandingZone_Create(WOBJ *wobj)
{
	super_dragons[wobj->custom_ints[0]].land_x = wobj->x;
	super_dragons[wobj->custom_ints[0]].land_y = wobj->y;
}

void WobjBozoLaserMinion_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 15.0f;
	wobj->strength = 0.8333333333f;
	wobj->custom_ints[0] = 0;
	Util_SetBox(&wobj->hitbox, 7.0f, 8.0f, 16.0f, 64.0f-8.0f);
}
void WobjBozoLaserMinion_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);

	// Finding Closest Player
	if (wobj->custom_ints[0] == 0 && Game_GetFrame() % 31 == 0)
	{
		WOBJ *n = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Interaction_GetDistanceToWobj(wobj, n) <= 192.0f)
		{
			wobj->custom_ints[0] = 1;
			wobj->link_node = n->node_id;
			wobj->link_uuid = n->uuid;
		}
	}

	// Attacking the player
	if (wobj->custom_ints[0] == 1)
	{
		// Searching for player
		WOBJ *player = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
		if (player == NULL || (Game_GetFrame() % 31 == 0 && Interaction_GetDistanceToWobj(player, wobj) >= 1024.0f))
		{
			wobj->custom_ints[0] = 0;
			return;
		}

		// Physics/Movement AI
		WobjPhysics_BeginUpdate(wobj);

		if (player->x > wobj->x && wobj->vel_x < wobj->custom_floats[0])
			wobj->vel_x += 0.5f;
		if (player->x < wobj->x && wobj->vel_x > -wobj->custom_floats[0])
			wobj->vel_x -= 0.5f;
		if (wobj->custom_floats[0] == 0.0f)
			wobj->vel_x = 0.0f;
		if (!Wobj_IsGrouneded(wobj))
			wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
		else
			wobj->vel_y = 0.0f;
		if (player->y < wobj->y - 64.0f && Wobj_IsGrouneded(wobj))
			wobj->vel_y = -14.0f;
		if (((Wobj_IsCollidingWithBlocksOrObjects(wobj, 10.0f, 0.0f) && wobj->vel_x > 0.0f) ||
			(Wobj_IsCollidingWithBlocksOrObjects(wobj, -10.0f, 0.0f) && wobj->vel_x < 0.0f)) &&
			player->y <= wobj->y)
			wobj->vel_y = -5.0f - Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;

		WobjPhysics_EndUpdate(wobj);

		// Laser Code
		wobj->custom_ints[1]--;
		if (wobj->custom_ints[1] < 0)
		{
			WOBJ *lo = Interaction_CreateWobj(BOZO_LASER_LOCKON, player->x, player->y, 0, 0.0f);
			lo->link_node = wobj->link_node;
			lo->link_uuid = wobj->link_uuid;
			wobj->custom_ints[1] = 120;
		}
		if (wobj->custom_ints[1] == 120 - (15*4))
		{
			float lx = wobj->x, ly = wobj->y;
			float ang = atan2f((player->y + 16.0f) - (wobj->y + 16.0f), (player->x + 16.0f) - (wobj->x + 16.0f));
			for (int i = 0; i < 100; i++)
			{
				CNM_BOX b;
				Util_SetBox(&b, lx, ly, 16.0f, 16.0f);
				BLOCK is_solid = Blocks_IsCollidingWithSolid(&b, 0);
				if (is_solid)
					break;
				lx += cosf(ang) * 12.0f;
				ly += sinf(ang) * 12.0f;
				Interaction_CreateWobj(BOZO_LASER_PART, lx, ly, 0, 0.0f);
			}
		}
	}

	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}
void WobjBozoLaserLockon_Create(WOBJ *wobj)
{
	wobj->anim_frame = 0;
	wobj->custom_ints[0] = 0;
	Interaction_PlaySound(wobj, 30);
}
void WobjBozoLaserLockon_Update(WOBJ *wobj)
{
	WOBJ *player = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
	if (player != NULL)
	{
		wobj->x = player->x;
		wobj->y = player->y;
	}

	int frame = wobj->custom_ints[0] / 15;
	if (frame > 3)
		frame = 3;
	wobj->anim_frame = frame;

	if (wobj->custom_ints[0]++ > 15*4)
	{
		Interaction_DestroyWobj(wobj);
	}
}
void WobjBozoLaserPart_Create(WOBJ *wobj)
{
	wobj->strength = 1.0f;
	wobj->custom_ints[0] = 0;
	wobj->flags = WOBJ_IS_HOSTILE;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 16.0f, 16.0f);
}
void WobjBozoLaserPart_Update(WOBJ *wobj)
{
	if (wobj->custom_ints[0]++ > 20)
		Interaction_DestroyWobj(wobj);
}

#define BZM2_SPD 4.0f
void WobjBozoMk2_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 8.0f, 8.0f, 32.0f, 56.0f);
	wobj->anim_frame = 0;
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 20.0f;
	wobj->strength = 1.666667f;
	wobj->custom_ints[0] = 0; // State
	wobj->money = 0; // State timer
}
void WobjBozoMk2_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);
	WOBJ *np = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
	if (np->x > wobj->x + 24.0f) wobj->flags &= ~WOBJ_HFLIP;
	else wobj->flags |= WOBJ_HFLIP;
	WobjPhysics_BeginUpdate(wobj);
	if (wobj->custom_ints[0] == 0) { // Mk2 is in idle
		wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
		wobj->anim_frame = 0;
		if (Interaction_GetDistanceToWobj(wobj, np) <= 320.0f) {
			wobj->custom_ints[0] = 1; // Set into walk state
			wobj->money = 0;
		}
	} else if (wobj->custom_ints[0] == 1) { // Walking state
		wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
		wobj->anim_frame = (wobj->money / 5) % 3;
		if (wobj->x + 24.0f > np->x) wobj->vel_x = -BZM2_SPD;
		else wobj->vel_x = BZM2_SPD;
		if (wobj->money++ > 90) {
			wobj->money = 12;
			wobj->custom_ints[0] = 2; // Set into jumping state
			float time_jump = (float)wobj->money;
			float think_x = np->x + np->vel_x * time_jump;
			float spd = (think_x - wobj->x) / time_jump;
			if (spd > 13.0f) spd = 13.0f;
			if (spd < -13.0f) spd = -13.0f;
			wobj->vel_x = spd;
			wobj->vel_y = -10.0f;
			wobj->custom_floats[1] = wobj->vel_x;
		}
	} else if (wobj->custom_ints[0] == 2) { // Jumping state
		if (Wobj_IsCollidingWithBlocksOrObjects(wobj, wobj->custom_floats[1], 0.0f))
			wobj->custom_floats[1] *= -1.0f;
		wobj->vel_x = wobj->custom_floats[1];
		wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
		if (wobj->vel_y > 0.0f && Wobj_IsGrouneded(wobj) && wobj->money-- <= 6) {
			wobj->vel_x = 0.0f;
			wobj->custom_ints[0] = 3; // Floating upwards state
			wobj->money = 60;
		}
	} else if (wobj->custom_ints[0] == 3) { // Floating upwards state
		wobj->anim_frame = 3;
		wobj->vel_x = 0.0f;
		if (wobj->money > 20) {
			wobj->vel_y = -1.0f;
		} else {
			wobj->vel_y = 0.0f;
		}
		if (wobj->money-- <= 0) {
			wobj->custom_ints[0] = 4; // Firing state
			wobj->money = 0;
		}
	} else if (wobj->custom_ints[0] == 4) { // Firing state
		wobj->anim_frame = 4;
		wobj->vel_y = 0.0f;
		wobj->vel_x = 0.0f;
		wobj->money++;
		float ang = atan2f((np->y + 16.0f) - (wobj->y + 20.0f), (np->x + 16.0f) - (wobj->x + 10.0f));
		if (wobj->money % 40 == 0) {
			WOBJ *rocket = Interaction_CreateWobj(WOBJ_ENEMY_ROCKET, wobj->x + 10.0f, wobj->y + 20.0f, 8, ang);
		}
		if (wobj->money % 10 == 0) {
			WOBJ *fireball = Interaction_CreateWobj(WOBJ_FIREBALL, wobj->x + 10.0f, wobj->y + 20.0f, 0, ang + (Util_RandFloat() * 2.0f - 1.0f) * 0.5f);
		}
		if (wobj->money > 50*3) {
			wobj->custom_ints[0] = 1; // Walking state
			wobj->money = 0;
		}
	}
	
	WobjPhysics_ApplyWindForces(wobj);
	WobjPhysics_EndUpdate(wobj);
}
void WobjEnemyRocket_Create(WOBJ *wobj) {
	wobj->hitbox.x = 13.0f;
	wobj->hitbox.y = 13.0f;
	wobj->hitbox.w = 20.0f;
	wobj->hitbox.h = 7.0f;
	wobj->vel_x = (float)wobj->custom_ints[0] * cosf(wobj->custom_floats[0]);
	wobj->vel_y = (float)wobj->custom_ints[0] * sinf(wobj->custom_floats[0]);
	wobj->flags |= wobj->vel_x < 0.0f ? WOBJ_HFLIP : 0;
	wobj->custom_ints[1] = 0;
	Interaction_PlaySound(wobj, 7);
}
void WobjEnemyRocket_Update(WOBJ *wobj) {
	wobj->x += wobj->vel_x;
	wobj->y += wobj->vel_y;
	if (wobj->custom_ints[1]++ >= 30*4)
		Interaction_DestroyWobj(wobj);
	if (Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 0.0f) ||
		Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID) ||
		Wobj_GetWobjColliding(wobj, WOBJ_IS_PLAYER))
	{
		WOBJ *explosion = Interaction_CreateWobj(WOBJ_KAMAKAZI_SLIME_EXPLOSION, wobj->x - 96.0f, wobj->y - 96.0f, 0, 0.0f);
		Interaction_DestroyWobj(wobj);
	}
}

void WobjSpikeGuy_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 8.0f;
	wobj->strength = 0.666666667f;
	wobj->custom_ints[0] = 0;
	Util_SetBox(&wobj->hitbox, 4.0f, 4.0f, 24.0f, 28.0f);
}
void WobjSpikeGuy_Update(WOBJ *wobj)
{
	Wobj_DoEnemyCry(wobj, 45);

	// Finding Closest Player
	if (wobj->custom_ints[0] == 0 && Game_GetFrame() % 31 == 0)
	{
		WOBJ *n = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Interaction_GetDistanceToWobj(wobj, n) <= 192.0f)
		{
			wobj->custom_ints[0] = 1;
			wobj->link_node = n->node_id;
			wobj->link_uuid = n->uuid;
		}
	}

	// Animating
	if ((Game_GetFrame() / 10) % 2 == 0)
		wobj->flags &= ~WOBJ_HFLIP;
	else
		wobj->flags |= WOBJ_HFLIP;

	// Attacking the player
	if (wobj->custom_ints[0] == 1)
	{
		// Searching for player
		WOBJ *player = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
		if (player == NULL || (Game_GetFrame() % 31 == 0 && Interaction_GetDistanceToWobj(player, wobj) >= 1024.0f))
		{
			wobj->custom_ints[0] = 0;
			return;
		}

		// Physics/Movement AI
		WobjPhysics_BeginUpdate(wobj);

		if (player->x > wobj->x && wobj->vel_x < 5.25f)
			wobj->vel_x += 0.5f;
		if (player->x < wobj->x && wobj->vel_x > -5.25f)
			wobj->vel_x -= 0.5f;
		if (!Wobj_IsGrouneded(wobj))
			wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
		else
			wobj->vel_y = 0.0f;
		if (player->y < wobj->y - 64.0f && Wobj_IsGrouneded(wobj))
			wobj->vel_y = -6.0f;
		if (((Wobj_IsCollidingWithBlocksOrObjects(wobj, 10.0f, 0.0f) && wobj->vel_x > 0.0f) ||
			 (Wobj_IsCollidingWithBlocksOrObjects(wobj, -10.0f, 0.0f) && wobj->vel_x < 0.0f)) &&
			player->y <= wobj->y)
			wobj->vel_y = -3.0f - Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;

		if (wobj->custom_ints[1] > 120 - 10)
		{
			wobj->vel_y = 0.0f;
			wobj->vel_x = 0.0f;
		}
		if (wobj->custom_ints[1] == 120 - 11) {
			if (rand() % 100 > 65) {
				for (int i = 0; i < 8; i++)
					Interaction_CreateWobj(SPIKE_GUY_SPIKE, wobj->x, wobj->y, i, 0.0f);
			}
			else {
				Interaction_CreateWobj(SPIKE_GUY_SPIKE, wobj->x, wobj->y, 0, 0.0f);
				Interaction_CreateWobj(SPIKE_GUY_SPIKE, wobj->x, wobj->y, 2, 0.0f);
				Interaction_CreateWobj(SPIKE_GUY_SPIKE, wobj->x, wobj->y, 4, 0.0f);
				Interaction_CreateWobj(SPIKE_GUY_SPIKE, wobj->x, wobj->y, 6, 0.0f);
			}
		}

		WobjPhysics_EndUpdate(wobj);

		// Spike Attack Code
		wobj->custom_ints[1]--;
		if (wobj->custom_ints[1] < 0)
		{
			wobj->custom_ints[1] = 120;
			Interaction_PlaySound(wobj, 2);
		}
	}

	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}
void WobjSpikeGuySpike_Create(WOBJ *wobj)
{
	const float spd = 10.0f;//5.0f;

	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 2.1666667f*1.5f;
	switch (wobj->custom_ints[0])
	{
	case 0: wobj->vel_x = spd; wobj->vel_y = 0.0f; break;
	case 1: wobj->vel_x = spd; wobj->vel_y = -spd; break;
	case 2: wobj->vel_x = 0.0f; wobj->vel_y = -spd; break;
	case 3: wobj->vel_x = -spd; wobj->vel_y = -spd; break;
	case 4: wobj->vel_x = -spd; wobj->vel_y = 0.0f; break;
	case 5: wobj->vel_x = -spd; wobj->vel_y = spd; break;
	case 6: wobj->vel_x = 0.0f; wobj->vel_y = spd; break;
	case 7: wobj->vel_x = spd; wobj->vel_y = spd; break;
	}
	wobj->anim_frame = wobj->custom_ints[0];
	wobj->custom_ints[1] = 0;
	Util_SetBox(&wobj->hitbox, 4.0f, 4.0f, 28.0f, 28.0f);
}
void WobjSpikeGuySpike_Update(WOBJ *wobj)
{
	wobj->x += wobj->vel_x;
	wobj->y += wobj->vel_y;
	Util_SetBox(&wobj->hitbox, 15.0f, 15.0f, 2.0f, 2.0f);
	if (wobj->custom_ints[1]++ > 30 || Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 0.0f))
		Interaction_DestroyWobj(wobj);
	Util_SetBox(&wobj->hitbox, 4.0f, 4.0f, 28.0f, 28.0f);
}

void WobjBanditGuy_Create(WOBJ *wobj)
{
	wobj->custom_ints[0] = 0;
	wobj->speed = wobj->custom_floats[0];
	wobj->strength = 0.5f;
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 0.5f;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}
void WobjBanditGuy_Update(WOBJ *wobj)
{
	//Wobj_DoEnemyCry(wobj, 45);

	if (wobj->custom_ints[0] == 0)
	{
		WOBJ *np = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Interaction_GetDistanceToWobj(wobj, np) <= (float)(RENDERER_WIDTH / 2))
		{
			wobj->custom_ints[0] = 1;
			wobj->link_node = np->node_id;
			wobj->link_uuid = np->uuid;
			wobj->custom_floats[0] = 1.0f;
			if (wobj->x > np->x)
				wobj->custom_floats[0] = -1.0f;
		}
	}

	if (wobj->custom_ints[0] == 1)
	{
		WOBJ *player = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
		wobj->y = player->y;
		wobj->x += wobj->speed * wobj->custom_floats[0];

		if (wobj->custom_floats[0] > 0.0f && wobj->x > player->x + (float)RENDERER_WIDTH / 1.5f)
			Interaction_DestroyWobj(wobj);
		if (wobj->custom_floats[0] < 0.0f && wobj->x < player->x - (float)RENDERER_WIDTH / 1.5f)
			Interaction_DestroyWobj(wobj);

		WOBJ *item = Wobj_GetWobjCollidingWithType(wobj, WOBJ_DROPPED_ITEM);
		if (item != NULL)
			Interaction_DestroyWobj(item);
		item = Wobj_GetWobjCollidingWithType(wobj, WOBJ_KNIFE_ITEM);
		if (item != NULL)
			Interaction_DestroyWobj(item);
		item = Wobj_GetWobjCollidingWithType(wobj, WOBJ_SHOTGUN_ITEM);
		if (item != NULL)
			Interaction_DestroyWobj(item);
		item = Wobj_GetWobjCollidingWithType(wobj, WOBJ_APPLE_ITEM);
		if (item != NULL)
			Interaction_DestroyWobj(item);
	}

	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}

void WobjKamakaziSlime_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->custom_floats[0] = (float)(Util_RandInt(0, 1) * 2 - 1);
	wobj->custom_ints[0] = 0;
	wobj->health = 5.0f;
	wobj->strength = 1.333333f;
	wobj->custom_ints[1] = Util_RandInt(30, 90);
	Util_SetBox(&wobj->hitbox, 0.0f, 16.0f, 32.0f, 16.0f);
}
void WobjKamakaziSlime_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
	Wobj_DoEnemyCry(wobj, 45);
	wobj->custom_ints[0]--;

	if ((Game_GetFrame() + 1) % 4 == 0 && wobj->custom_ints[0] < 0)
	{
		WOBJ *np = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (Interaction_GetDistanceToWobj(np, wobj) < 128.0f)
		{
			wobj->custom_ints[0] = 30*3;
			wobj->custom_floats[0] *= 7.0f;
			wobj->link_node = np->node_id;
			wobj->link_uuid = np->uuid;
			Interaction_PlaySound(wobj, 30);
		}
	}

	WobjPhysics_BeginUpdate(wobj);

	if (wobj->custom_ints[0] < 0)
	{
		if (wobj->custom_ints[1] <= 0)
		{
			wobj->custom_ints[1] = Util_RandInt(30, 90);
			wobj->custom_floats[0] *= -1.0f;
		}

		if (Wobj_IsCollidingWithBlocksOrObjects(wobj, wobj->custom_floats[0] * 3.0f, 0.0f))
		{
			wobj->custom_floats[0] *= -1.0f;
		}
	}
	else
	{
		WOBJ *p = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
		if (p != NULL)
		{
			if (p->x < wobj->x)
				wobj->custom_floats[0] = -7.0f;
			else
				wobj->custom_floats[0] = 7.0f;
		}
	}

	wobj->vel_x = wobj->custom_floats[0];
	if (!Wobj_IsGrouneded(wobj))
		wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
	else
		wobj->vel_y = 0.0f;
	WobjPhysics_ApplyWindForces(wobj);
	WobjPhysics_EndUpdate(wobj);

	if (wobj->custom_ints[0] == 0)
	{
		Interaction_CreateWobj(WOBJ_KAMAKAZI_SLIME_EXPLOSION, wobj->x - 96.0f, wobj->y - 96.0f, 0, 0.0f);
		Interaction_DestroyWobj(wobj);
	}
}
void WobjKamakaziSlime_Draw(WOBJ *wobj, int camx, int camy)
{
	int gframe = Game_GetFrame() / 2;
	if (wobj->custom_ints[0] < 0)
		gframe = Game_GetFrame() / 12;
	gframe %= 2;
	wobj->anim_frame = gframe;
	WobjGeneric_Draw(wobj, camx, camy);
}
void WobjKamakaziSlimeExplosion_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 1.6667f;
	wobj->custom_ints[0] = 20;
	Util_SetBox(&wobj->hitbox, 48.0f, 48.0f, 96.0f, 96.0f);
	Interaction_PlaySound(wobj, 6);
}
void WobjKamakaziSlimeExplosion_Update(WOBJ *wobj)
{
	if (wobj->custom_ints[0]-- < 0)
		Interaction_DestroyWobj(wobj);
}

void WobjRockGuySpear_Create(WOBJ *wobj)
{
	while (wobj->custom_floats[0] < 0.0f)
		wobj->custom_floats[0] += 2.0f*CNM_PI;
	while (wobj->custom_floats[0] > 2.0f * CNM_PI)
		wobj->custom_floats[0] -= 2.0f * CNM_PI;
	float d = CNM_2DEG(wobj->custom_floats[0]) - 22.5f;
	if (d > 360.0f)
		d -= 360.0f;
	if (d < 0.0f)
		d += 360.0f;
	wobj->anim_frame = (int)(d / 45.0f);
	wobj->custom_floats[1] = 10.0f;
	wobj->custom_ints[0] = 60;
	wobj->strength = 3.33333333f;
	wobj->flags = WOBJ_IS_HOSTILE;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
	Interaction_PlaySound(wobj, 40);
}
void WobjRockGuySpear_Update(WOBJ *wobj)
{
	wobj->x += cosf(wobj->custom_floats[0]) * wobj->custom_floats[1];
	wobj->y += sinf(wobj->custom_floats[0]) * wobj->custom_floats[1];
	wobj->custom_floats[1] *= 0.995f;
	wobj->y += wobj->vel_y;
	if (wobj->custom_ints[0] < 25)
		wobj->vel_y += 0.05f;
	Util_SetBox(&wobj->hitbox, 15.0f, 15.0f, 2.0f, 2.0f);
	if (wobj->custom_ints[0]-- < 0 || Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 0.0f))
		Interaction_DestroyWobj(wobj);
	Util_SetBox(&wobj->hitbox, 4.0f, 4.0f, 28.0f, 28.0f);
}

#define ROCK_GUY_MEDIUM_SLEEPING 0
#define ROCK_GUY_MEDIUM_SHOOTING 1
#define ROCK_GUY_MEDIUM_MAXHP 6.0f

#define ROCK_GUY_MEDIUM_TRACKING_PLAYER (1 << 15)
void WobjRockGuyMedium_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 1.333333333f;
	wobj->health = ROCK_GUY_MEDIUM_MAXHP;
	wobj->custom_ints[0] = ROCK_GUY_MEDIUM_SLEEPING;
	wobj->custom_ints[1] = Util_RandInt(0, 30);
	wobj->link_node = 0;
	wobj->anim_frame = 0;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 64.0f);
}
void WobjRockGuyMedium_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
	Wobj_DoEnemyCry(wobj, 45);
	WobjPhysics_BeginUpdate(wobj);

	WOBJ *player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
	if (player != NULL)
	{
		if (wobj->custom_ints[0] == ROCK_GUY_MEDIUM_SLEEPING)
		{
			if (Interaction_GetDistanceToWobj(player, wobj) < 64.0f || wobj->health < ROCK_GUY_MEDIUM_MAXHP)
				wobj->custom_ints[0] = ROCK_GUY_MEDIUM_SHOOTING;
			wobj->anim_frame = 0;
		}
		if (wobj->custom_ints[0] == ROCK_GUY_MEDIUM_SHOOTING)
		{
			if ((Game_GetFrame() + wobj->custom_ints[1]) % 30 == 0)
				Interaction_CreateWobj(ROCK_GUY_SPEAR, wobj->x, wobj->y, 0, atan2f(player->y - wobj->y, player->x - wobj->x));
			wobj->anim_frame = 1;

			static const float spd = 3.75f, acc = 0.5f;
			if (wobj->x > player->x && wobj->vel_x > -spd)
				wobj->vel_x += -acc;
			if (wobj->x < player->x && wobj->vel_x < spd)
				wobj->vel_x += acc;

			if (Wobj_IsGrouneded(wobj))
			{
				wobj->vel_y = -7.0f;
				Interaction_PlaySound(wobj, 36);
			}
			else
				wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;

			if (Interaction_GetDistanceToWobj(player, wobj) > (float)RENDERER_WIDTH * 4.0f)
			{
				wobj->custom_ints[0] = ROCK_GUY_MEDIUM_SLEEPING;
				wobj->vel_x = 0.0f;
				wobj->vel_y = 0.0f;
			}
		}
	}

	WobjPhysics_ApplyWindForces(wobj);
	WobjPhysics_EndUpdate(wobj);
}

static void WobjRockGuySmall_AI(WOBJ *wobj, float spd, float jmp)
{
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
	Wobj_DoEnemyCry(wobj, 45);
	WobjPhysics_BeginUpdate(wobj);

	WOBJ *player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
	if (player != NULL)
	{
		static const float acc = 0.5f;
		if (wobj->custom_ints[0] && wobj->vel_x > -spd)
			wobj->vel_x += -acc;
		else if (wobj->vel_x < spd)
			wobj->vel_x += acc;

		if (Wobj_IsGrouneded(wobj))
		{
			wobj->vel_y = jmp + (Util_RandFloat() - 0.25f);
			Interaction_PlaySound(wobj, 33);
		}
		else
			wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
	}

	WobjPhysics_ApplyWindForces(wobj);
	WobjPhysics_EndUpdate(wobj);
}

void WobjRockGuySmall1_Death(WOBJ *wobj)
{
	Interaction_CreateWobj(ROCK_GUY_SMALL2, wobj->x, wobj->y, 0, 0.0f);
	Interaction_CreateWobj(ROCK_GUY_SMALL2, wobj->x, wobj->y, 1, 0.0f);
}
void WobjRockGuySmall1_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 0.4f;
	wobj->health = 2.0f;
	wobj->anim_frame = 0;
	wobj->custom_ints[0] = 0;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 22.0f, 18.0f);
	if (wobj->internal.owned)
		Interaction_PlaySound(wobj, 37);
	wobj->on_destroy = WobjRockGuySmall1_Death;
	wobj->custom_floats[0] = Util_RandFloat() * 1.5f + 3.25f;
}
void WobjRockGuySmall1_Update(WOBJ *wobj)
{
	WOBJ *player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
	if (player != NULL)
	{
		if (wobj->x < player->x)
			wobj->custom_ints[0] = 0;
		if (wobj->x > player->x)
			wobj->custom_ints[0] = 1;
	}
	WobjRockGuySmall_AI(wobj, wobj->custom_floats[0], -6.0f);
}

void WobjRockGuySmall2_Create(WOBJ *wobj)
{
	wobj->custom_ints[1] = 0;
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 0.3333333f;
	wobj->health = 0.7f;
	wobj->anim_frame = 0;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 13.0f, 14.0f);
	if (wobj->internal.owned)
		Interaction_PlaySound(wobj, 38);
	wobj->custom_floats[0] = Util_RandFloat() * 1.5f + 4.0f;
}
void WobjRockGuySmall2_Update(WOBJ *wobj)
{
	if (Wobj_IsCollidingWithBlocksOrObjects(wobj, 5.0f, 0.0f))
		wobj->custom_ints[0] = 1;
	if (Wobj_IsCollidingWithBlocksOrObjects(wobj, -5.0f, 0.0f))
		wobj->custom_ints[0] = 0;

	if (wobj->custom_ints[1]++ > 45)
	{
		WOBJ *player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
		if (player != NULL)
		{
			if (wobj->x < player->x)
				wobj->custom_ints[0] = 0;
			if (wobj->x > player->x)
				wobj->custom_ints[0] = 1;
		}
	}

	WobjRockGuySmall_AI(wobj, wobj->custom_floats[0], -5.0f);
}

#define RGSLIDER_MAXHP 16.0f
void WobjRockGuySlider_Death(WOBJ *wobj)
{
	for (int i = 0; i < 10; i++)
		Interaction_CreateWobj(ROCK_GUY_SMALL1, wobj->x + (float)(Util_RandInt(0, 64) - 32), wobj->y - (float)Util_RandInt(0, 48), 0, 0.0f);
}
void WobjRockGuySlider_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = RGSLIDER_MAXHP;
	wobj->strength = 2.833333333f;
	wobj->custom_floats[0] = (float)(Util_RandInt(0, 100) / 60) * 2.0f + 1.0f;
	wobj->anim_frame = 0;
	Util_SetBox(&wobj->hitbox, 8.0f, 8.0f, 64.0f-16.0f, 24.0f);
	wobj->custom_ints[0] = 0;
	wobj->on_destroy = WobjRockGuySlider_Death;
}
void WobjRockGuySlider_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
	Wobj_DoEnemyCry(wobj, 45);
	WobjPhysics_BeginUpdate(wobj);

	if (Wobj_IsCollidingWithBlocksOrObjects(wobj, 5.0f, 0.0f))
		wobj->custom_floats[0] = -1.0f;
	if (Wobj_IsCollidingWithBlocksOrObjects(wobj, -5.0f, 0.0f))
		wobj->custom_floats[0] = 1.0f;

	if (fabsf(wobj->vel_x) < 15.0f)
		wobj->vel_x += wobj->custom_floats[0] * 0.1f;

	if (wobj->custom_ints[0] == 0 && wobj->health < ((RGSLIDER_MAXHP / 3.0f) * 2.0f))
		goto break_apart;
	if (wobj->custom_ints[0] == 1 && wobj->health < ((RGSLIDER_MAXHP / 3.0f) * 1.0f))
		goto break_apart;

	WobjPhysics_ApplyWindForces(wobj);
	WobjPhysics_EndUpdate(wobj);
	return;
break_apart:
	Interaction_PlaySound(wobj, 32);
	wobj->anim_frame++;
	if (wobj->anim_frame > 2)
		wobj->anim_frame = 2;
	wobj->custom_ints[0]++;
	for (int i = 0; i < 5; i++)
		Interaction_CreateWobj(ROCK_GUY_SMALL1, wobj->x + (float)(Util_RandInt(0, 64) - 32), wobj->y - (float)Util_RandInt(0, 48), 0, 0.0f);
}

#define RGSMASHER_IDLE 0
#define RGSMASHER_RUNNING 1
#define RGSMASHER_SMASHING 2
void WobjRockGuySmasher_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->health = 40.0f;
	wobj->strength = 3.33333333f;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 96.0f);
	wobj->anim_frame = 0;
	wobj->custom_ints[0] = RGSMASHER_IDLE;
	wobj->custom_ints[1] = 0;

	wobj->money = (int)(wobj->health);
}
void WobjRockGuySmasher_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
	Wobj_DoEnemyCry(wobj, 45);
	WobjPhysics_BeginUpdate(wobj);

	WOBJ *player = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);
	if (wobj->custom_ints[0] == RGSMASHER_IDLE)
	{
		wobj->vel_x = 0.0f;
		Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 96.0f);

		if (Game_GetFrame() % 15 == 0)
			wobj->money = (int)(wobj->health);

		if ((Interaction_GetDistanceToWobj(player, wobj) < 128.0f && wobj->custom_ints[1] > 30) ||
			(wobj->money > (int)wobj->health))
		{
			if (player->x > wobj->x)
			{
				wobj->custom_floats[1] = 1.0f;
				wobj->flags &= ~WOBJ_HFLIP;
			}
			else
			{
				wobj->custom_floats[1] = -1.0f;
				wobj->flags |= WOBJ_HFLIP;
			}
			wobj->custom_ints[0] = RGSMASHER_RUNNING;
			wobj->custom_ints[1] = 0;
			return;
		}
		wobj->custom_ints[1]++;
		wobj->anim_frame = 0;
	}
	if (wobj->custom_ints[0] == RGSMASHER_RUNNING)
	{
		Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 48.0f, 96.0f);
		if (fabsf(wobj->vel_x) < 14.0f)
			wobj->vel_x += wobj->custom_floats[1] * 0.15f;

		int on_wrong_side = CNM_FALSE;
		if (wobj->custom_floats[1] > 0.0f && wobj->x < player->x)
			on_wrong_side = CNM_TRUE;
		if (wobj->custom_floats[1] < 0.0f && wobj->x > player->x)
   			on_wrong_side = CNM_TRUE;

		if ((Interaction_GetDistanceToWobj(player, wobj) < 128.0f && wobj->custom_ints[1] > 120) ||
			Wobj_IsCollidingWithBlocksOrObjects(wobj, wobj->custom_floats[1] * 4.0f, -1.0f) ||
			(wobj->custom_ints[1] > 30*10) ||
			(on_wrong_side && wobj->custom_ints[1] > 30*2))
		{
			wobj->custom_ints[0] = RGSMASHER_SMASHING;
			wobj->custom_ints[1] = 0;
			wobj->anim_frame = 2;
			wobj->y += 32.0f;
			if (~wobj->flags & WOBJ_HFLIP) wobj->x -= 64.0f;
			Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 96.0f, 64.0f);
			return;
		}
		wobj->custom_ints[1]++;
		wobj->anim_frame = 1;
	}
	if (wobj->custom_ints[0] == RGSMASHER_SMASHING)
	{
		static const float dec = 0.4f;
		if (/*fabsf(wobj->vel_x) <= dec && */wobj->custom_ints[1] > 40)
		{
			wobj->custom_ints[1] = 0;
			wobj->custom_ints[0] = RGSMASHER_IDLE;
			if (wobj->hitbox.h == 32.0f)
				wobj->y -= 64.0f;
			else
				wobj->y -= 32.0f;
			wobj->anim_frame = 0;
			Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 96.0f);
			return;
		}

		if (wobj->custom_ints[1] >= 10 || fabsf(wobj->vel_x) <= 3.0f)
		{
			if (wobj->anim_frame == 2)
			{
				wobj->y += 32.0f;
				Interaction_PlaySound(wobj, 39);
				if (Util_RandInt(0, 100) > 50)
				{
					Interaction_CreateWobj(ROCK_GUY_SMALL1, wobj->x + (float)(Util_RandInt(0, 64) - 32), wobj->y - (float)Util_RandInt(0, 48), 0, 0.0f);
				}
			}
			wobj->anim_frame = 3;
			Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 96.0f, 32.0f);
		}

		if (wobj->vel_x > dec)
			wobj->vel_x -= dec;
		if (wobj->vel_x < -dec)
			wobj->vel_x += dec;
		if (fabsf(wobj->vel_x) <= dec)
			wobj->vel_x = 0.0f;

		wobj->custom_ints[1]++;
	}

	if (!Wobj_IsGrouneded(wobj))
		wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;

	WobjPhysics_EndUpdate(wobj);
}

void WobjSupervirus_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_HOSTILE;
	wobj->strength = 1.0f;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 14.0f * 3.0f, 28.0f * 3.0f);
	wobj->health = 10000.0f;
}
void WobjSupervirus_Update(WOBJ *wobj)
{
	WOBJ *p = (WOBJ *)Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	float spd = 1.0f;
	if (fabsf(wobj->x - p->x) > 1000.0f || fabsf(wobj->y - p->y) > 1000.0f) spd = 32.0f;
	if (fabsf(wobj->x - p->x) > 500.0f || fabsf(wobj->y - p->y) > 500.0f) spd = 16.0f;
	if (fabsf(wobj->x - p->x) > 250.0f || fabsf(wobj->y - p->y) > 250.0f) spd = 8.0f;
	if (rand() % 30 == Game_GetFrame() % 30)
	{
		if (wobj->x < p->x) wobj->vel_x = spd;
		if (wobj->y < p->y) wobj->vel_y = spd;
		if (wobj->x > p->x) wobj->vel_x = -spd;
		if (wobj->y > p->y) wobj->vel_y = -spd;
	}
	wobj->x += wobj->vel_x;
	wobj->y += wobj->vel_y;
	int i;
	for (i = 0; i < 32; i++)
	{
		WOBJ *other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_SUPERVIRUS);
		if (other != NULL)
		{
			CNM_BOX me, them;
			int ix, iy;

			me.x = wobj->x;
			me.y = wobj->y;
			me.w = 14.0f * 3.0f;
			me.h = 28.0f * 3.0f;
			them.x = other->x;
			them.y = other->y;
			them.w = 14.0f * 3.0f;
			them.h = 28.0f * 3.0f;
			Util_ResolveAABBCollision(&me, &them, &ix, &iy);
			wobj->x = me.x;
			wobj->y = me.y;
		}
	}
	Wobj_DoEnemyCry(wobj, 21);
}
void WobjSupervirus_Draw(WOBJ *wobj, int camx, int camy)
{
	supervirus_render((int)wobj->x - camx, (int)wobj->y - camy);
}
