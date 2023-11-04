#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "renderer.h"
#include "spawners.h"
#include "wobj.h"
#include "input.h"
#include "blocks.h"
#include "game.h"
#include "console.h"
#include "teleport_infos.h"
#include "interaction.h"
#include "player.h"
#include "audio.h"
#include "item.h"
#include "enemies.h"
#include "bossbar.h"
#include "ending_text.h"
#include "logic_links.h"
#include "gamelua.h"

static void WobjGeneric_Hurt(WOBJ *victim, WOBJ *inflictor)
{

}

static void WobjTeleport_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 8.0f;
	wobj->hitbox.y = 6.0f;
	wobj->hitbox.w = 24.0f;
	wobj->hitbox.h = 26.0f;
}
static void WobjTeleport_Animate(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	wobj->anim_frame = Game_GetFrame() % 2;
}

static void WobjTeleport_Draw(WOBJ *wobj, int camx, int camy)
{
	wobj->anim_frame = Game_GetFrame() % 2;
	if (strcmp(TeleportInfos_GetTeleport(wobj->custom_ints[0])->name, "__") == 0)
		return;
	WobjGeneric_Draw(wobj, camx, camy);
	if (TeleportInfos_GetTeleport(wobj->custom_ints[0])->cost > 0)
	{
		if (strcmp(TeleportInfos_GetTeleport(wobj->custom_ints[0])->name, ""))
		{
			int len = strlen(TeleportInfos_GetTeleport(wobj->custom_ints[0])->name);
			Renderer_DrawText
			(
				(int)wobj->x - camx - (len*8) / 2,
				(int)wobj->y - 12 - camy,
				0,
				RENDERER_LIGHT,
				"%s: $%d",
				TeleportInfos_GetTeleport(wobj->custom_ints[0])->name,
				TeleportInfos_GetTeleport(wobj->custom_ints[0])->cost
			);
		}
		else
		{
			Renderer_DrawText
			(
				(int)wobj->x - camx,
				(int)wobj->y - 12 - camy,
				0,
				RENDERER_LIGHT,
				"$%d",
				TeleportInfos_GetTeleport(wobj->custom_ints[0])->cost
			);
		}
	}
	else
	{
		Renderer_DrawText
		(
			(int)wobj->x - camx,
			(int)wobj->y - 12 - camy,
			0,
			RENDERER_LIGHT,
			TeleportInfos_GetTeleport(wobj->custom_ints[0])->name
		);
	}
}

static void WobjShotgunItem_Create(WOBJ *wobj)
{
	wobj->anim_frame = 0;
	wobj->item = 1;
	wobj->flags = WOBJ_IS_ITEM;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
}

static void Wobj_ShotgunPel_Create(WOBJ *wobj) {
	wobj->strength = 1.0f;
	wobj->anim_frame = 0;
	wobj->custom_ints[0] = 0;
	wobj->flags = WOBJ_IS_PLAYER_WEAPON | WOBJ_IS_PLAYER_BULLET;
	Util_SetBox(&wobj->hitbox, 2.0f, 2.0f, 4.0f, 4.0f);
	wobj->speed = 0.3f;
	wobj->local_data = (void *)1234;
	wobj->custom_ints[0] = 0;
	wobj->custom_floats[1] = 1.0f;
}
static void Wobj_ShotgunPel_Update(WOBJ *wobj) {
	wobj->custom_ints[0]++;
	wobj->x += wobj->vel_x;
	wobj->y += wobj->vel_y;
	if (wobj->vel_x > 0.0f) wobj->vel_x -= 0.1f;
	else wobj->vel_x += 0.1f;
	wobj->vel_y += wobj->speed;
	WobjGenericAttack_Update(wobj);
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f) || wobj->custom_ints[0]++ >= (30*5) || Wobj_GetWobjColliding(wobj, WOBJ_IS_BREAKABLE))
	{
		if (wobj->anim_frame == 1 && wobj->local_data != NULL && wobj->custom_ints[0] < 10) {
			WOBJ *plr = (WOBJ *)Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
			float k = (10.0f - (float)wobj->custom_ints[0]) * wobj->custom_floats[1];
			if (plr->x > wobj->x) plr->vel_x += k;
			else plr->vel_x -= k;
		}
		Interaction_DestroyWobj(wobj);
	}
}

static void WobjPlayerPellet_Create(WOBJ *wobj)
{
	wobj->strength = 1.0f;
	wobj->speed = 10.0f;
	wobj->anim_frame = 0;
	wobj->custom_ints[0] = 0;
	wobj->flags = WOBJ_IS_PLAYER_WEAPON | WOBJ_IS_PLAYER_BULLET;
	wobj->hitbox.x = 8.0f;
	wobj->hitbox.y = 8.0f;
	wobj->hitbox.w = 16.0f;
	wobj->hitbox.h = 16.0f;
}
static void WobjPlayerPellet_Update(WOBJ *wobj)
{
	wobj->x += wobj->custom_floats[0] * wobj->speed;
	wobj->custom_ints[0]++;
	WobjGenericAttack_Update(wobj);
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f) || wobj->custom_ints[0] >= (30*5) || Wobj_GetWobjColliding(wobj, WOBJ_IS_BREAKABLE))
	{
		Interaction_DestroyWobj(wobj);
	}
}

static void WobjVelPlayerPellet_Create(WOBJ *wobj)
{
	wobj->strength = 1.0f;
	wobj->vel_x = 10.0f;
	wobj->vel_y = 1.0f;
	wobj->anim_frame = 0;
	wobj->flags = WOBJ_IS_PLAYER_WEAPON | WOBJ_IS_PLAYER_BULLET;
	wobj->hitbox.x = 8.0f;
	wobj->hitbox.y = 8.0f;
	wobj->hitbox.w = 16.0f;
	wobj->hitbox.h = 16.0f;
}
static void WobjVelPlayerPellet_Update(WOBJ *wobj)
{
	wobj->x += wobj->vel_x;
	wobj->y += wobj->vel_y;
	wobj->custom_ints[0]++;
	WobjGenericAttack_Update(wobj);
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f) || wobj->custom_ints[0] >= (30 * 5) || Wobj_GetWobjColliding(wobj, WOBJ_IS_BREAKABLE))
	{
		Interaction_DestroyWobj(wobj);
	}
}

static void WobjSmallTunesTrigger_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
}

static void WobjBigTunesTrigger_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 64.0f;
	wobj->hitbox.h = 64.0f;
}

static void WobjEndingTextTrigger_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	wobj->custom_ints[1] = (int)wobj->custom_floats[0];
}

static void WobjScMovingPlatform_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_MOVESTAND | WOBJ_IS_SOLID;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	if (wobj->custom_ints[0] < 0) {
		wobj->x += wobj->custom_ints[0];
		wobj->custom_ints[0] *= -1;
		wobj->custom_floats[0] *= -1.0f;
	}
	wobj->custom_ints[1] = wobj->custom_ints[0];
	wobj->vel_x = wobj->custom_floats[0];
	wobj->vel_y = 0.0f;

	if (wobj->vel_x < 0.0) {
		wobj->x += wobj->custom_ints[1] * -wobj->vel_x;
	}
}
static void WobjScMovingPlatform_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	wobj->x += wobj->vel_x;
	wobj->custom_ints[1]--;
	if (wobj->custom_ints[1] <= 0)
	{
		wobj->custom_ints[1] = wobj->custom_ints[0];
		wobj->vel_x *= -1.0f;
	}
}

static void WobjMovingPlatformVertical_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_MOVESTAND | WOBJ_IS_SOLID;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	if (wobj->custom_ints[0] < 0) {
		wobj->y += wobj->custom_ints[0];
		wobj->custom_ints[0] *= -1;
		wobj->custom_floats[0] *= -1.0f;
	}
	wobj->custom_ints[1] = wobj->custom_ints[0];
	wobj->vel_y = wobj->custom_floats[0];
	wobj->vel_x = 0.0f;

	if (wobj->vel_y < 0.0) {
		wobj->y += wobj->custom_ints[1] * -wobj->vel_y;
	}
}
static void WobjMovingPlatformVertical_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	wobj->y += wobj->vel_y;
	wobj->custom_ints[1]--;
	if (wobj->custom_ints[1] <= 0)
	{
		wobj->custom_ints[1] = wobj->custom_ints[0];
		wobj->vel_y *= -1.0f;
	}
}

static void WobjBrickWallBreakable_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_BREAKABLE | WOBJ_IS_SOLID;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	wobj->health = wobj->custom_floats[0];
}

static void WobjBackgroundSwitch_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	wobj->custom_ints[1] = (int)wobj->custom_floats[0];
}

static void WobjKnifeAttack_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_PLAYER_WEAPON;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 0.0f;
	wobj->hitbox.h = 0.0f;
	wobj->anim_frame = 1;
}

static void WobjKnifeItem_Create(WOBJ *wobj)
{
	wobj->anim_frame = 0;
	wobj->item = ITEM_TYPE_KNIFE;
	wobj->flags = WOBJ_IS_ITEM;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
}

static void WobjAppleItem_Create(WOBJ *wobj)
{
	wobj->anim_frame = 0;
	wobj->item = ITEM_TYPE_APPLE;
	wobj->flags = WOBJ_IS_ITEM;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
}

static void WobjDroppedItem_Create(WOBJ *wobj)
{
	wobj->item = wobj->custom_ints[0];
	wobj->anim_frame = wobj->item;
	wobj->flags = WOBJ_IS_ITEM;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;

	// This is poisonous potion stuff
	if (wobj->item == ITEM_TYPE_STRENGTH_POTION && Util_RandInt(0, 1000) >= 999)
		wobj->item = ITEM_TYPE_POISIONUS_STRENGTH_POTION;
	if (wobj->item == ITEM_TYPE_SPEED_POTION && Util_RandInt(0, 1000) >= 999)
		wobj->item = ITEM_TYPE_POISIONUS_SPEED_POTION;
	if (wobj->item == ITEM_TYPE_JUMP_POTION && Util_RandInt(0, 1000) >= 999)
		wobj->item = ITEM_TYPE_POISIONUS_JUMP_POTION;

	// Setting the durability
	wobj->custom_floats[0] = item_types[wobj->item].max_durability;
}
static void WobjDroppedItem_Update(WOBJ *wobj) {
	Wobj_TryTeleportWobj(wobj, CNM_FALSE);
}
static void WobjDroppedItem_Draw(WOBJ *wobj, int camx, int camy) {
	CNM_RECT r;
	Renderer_DrawBitmap2
	(
		(int)wobj->x - camx,
		(int)wobj->y - camy,
		&wobj_types[WOBJ_DROPPED_ITEM].frames[wobj->item],
		0,
		Blocks_GetCalculatedBlockLight((int)(wobj->x + 16.0f) / BLOCK_SIZE, (int)(wobj->y + 16.0f) / BLOCK_SIZE),
		wobj->flags & WOBJ_HFLIP,
		wobj->flags & WOBJ_VFLIP
	);
	if (wobj->item == 40) {
		Renderer_DrawBitmap
		(
			(int)wobj->x - camx,
			(int)wobj->y - camy,
			&(CNM_RECT) {
				.x = 480, .y = 7520 + ((Game_GetFrame() / 2) % 8) * 32, .w = 32, .h = 32
			},
			2,
			RENDERER_LIGHT
		);
	}
}

static void WobjIceRune_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 64.0f;
	wobj->hitbox.h = 64.0f;
}
static void WobjLightningRune_Draw(WOBJ *wobj, int camx, int camy)
{
	wobj->anim_frame = Game_GetFrame() & 1;
	WobjGeneric_Draw(wobj, camx, camy);
}

static void WobjFire_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 16.0f, 16.0f, 32.0f, 32.0f);
	wobj->anim_frame = 0;
	wobj->custom_ints[0] = 0;
	wobj->link_node = 0;
	wobj->link_uuid = 0;
	wobj->custom_ints[1] = 0;
	wobj->strength = 5.0f;
	wobj->flags = WOBJ_LIGHT_BIG;
}
static void WobjFire_Update(WOBJ *wobj)
{
	wobj->custom_ints[1]++;

	if (wobj->custom_ints[0] == 0)
	{
		wobj->x += wobj->speed;
		WOBJ *other = Wobj_GetWobjColliding(wobj, WOBJ_IS_HOSTILE);
		if (other == NULL) other = Interaction_GetPlayerAsVictim(wobj);
		if (other != NULL)
		{
			wobj->link_node = other->node_id;
			wobj->link_uuid = other->uuid;
			wobj->custom_ints[0] = 1;
			wobj->custom_ints[1] = 0;
		}

		if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f) || wobj->custom_ints[1] > 30*5)
			Interaction_DestroyWobj(wobj);
	}
	else if (wobj->custom_ints[0] == 1)
	{
		WOBJ *other = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
		if (other != NULL)
		{
			if (other->type == WOBJ_PLAYER && other->flags & WOBJ_PLAYER_IS_RESPAWNING) {
				Interaction_DestroyWobj(wobj);
				return;
			}

			//WobjGenericAttack_Update(wobj);
			Interaction_DamageWobj(wobj, other);
			wobj->x = other->x;
			wobj->y = other->y - 24.0f;
		}
		else
		{
			Interaction_DestroyWobj(wobj);
		}

		if (wobj->custom_ints[1] >= 30*20)
			Interaction_DestroyWobj(wobj);
	}
}

static void WobjIceAttack_Create(WOBJ *wobj)
{
	wobj->anim_frame = 0;
	wobj->hitbox.x = 0;
	wobj->hitbox.y = 0;
	wobj->hitbox.w = 46.0f;
	wobj->hitbox.h = 44.0f;
	wobj->custom_ints[0] = 0;
	wobj->custom_ints[1] = 0;
	wobj->strength = 10.0f;
}
static void WobjIceAttack_Update(WOBJ *wobj)
{
	wobj->custom_ints[1]++;

	if (wobj->custom_ints[0] == 0)
	{
		WobjPhysics_BeginUpdate(wobj);
		wobj->vel_x = wobj->speed;
		if (!Wobj_IsGrouneded(wobj))
			wobj->vel_y += 0.8f;
		else
			wobj->vel_y = 0.0f;
		if (Wobj_IsCollidingWithBlocks(wobj, wobj->vel_x, -fabsf(wobj->speed)))
		{
			wobj->speed *= -1.0f;
			wobj->custom_ints[1] += 30*2;
		}
		WobjPhysics_EndUpdate(wobj);

		WOBJ *other = Wobj_GetWobjColliding(wobj, WOBJ_IS_HOSTILE);
		if (other == NULL) other = Interaction_GetPlayerAsVictim(wobj);
		if (other != NULL)
		{
			wobj->link_node = other->node_id;
			wobj->link_uuid = other->uuid;
			wobj->custom_ints[1] = -30*5;
			wobj->custom_ints[0] = 1;
		}
	}
	else if (wobj->custom_ints[0] == 1)
	{
		WOBJ *other = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
		if (other != NULL)
		{
			if (other->type == WOBJ_PLAYER && other->flags & WOBJ_PLAYER_IS_RESPAWNING) {
				Interaction_DestroyWobj(wobj);
				return;
			}

			if (Game_GetFrame() % 4 == 0)
				Interaction_DamageWobj(wobj, other);
			//other->x = wobj->x;
			//other->y = wobj->y - 8.0f;
			//other->vel_x = 0.0f;
			//other->vel_y = 0.0f;
			Interaction_ForceWobjPosition(other, wobj->x, wobj->y - 8.0f);
		}
		else
		{
			Interaction_DestroyWobj(wobj);
		}
	}

	if (wobj->custom_ints[1] >= 30*10)
		Interaction_DestroyWobj(wobj);
}

static void WobjCloudPlatform_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 8.0f, 16.0f, 48.0f, 8.0f);
	wobj->flags = WOBJ_IS_SOLID;
	wobj->custom_ints[0] = 7;
}
static void WobjCloudPlatform_Update(WOBJ *wobj)
{
	if (wobj->custom_ints[0]-- <= 0)
		Interaction_DestroyWobj(wobj);
}

static void WobjLightningAttackParticle_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_LIGHT_SMALL | WOBJ_LIGHT_BIG;
	wobj->custom_ints[0] = 10;
	wobj->strength = 10000.0f;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 64.0f;
	wobj->hitbox.h = 64.0f;
}
static void WobjLightningAttackParticle_Update(WOBJ *wobj)
{
	if (wobj->custom_ints[0]-- <= 0)
		Interaction_DestroyWobj(wobj);

	wobj->x += wobj->vel_x;
	wobj->y += wobj->vel_y;

	WobjGenericAttack_Update(wobj);
	//WOBJ *other = Wobj_GetWobjColliding(wobj, WOBJ_IS_HOSTILE);
	//if (other != NULL)
}


static void WobjPlayerLaser_Create(WOBJ *wobj)
{
	wobj->hitbox.x = (wobj->custom_ints[0] == -1) ? -128.0f : 0.0f;
	wobj->hitbox.y = -48.0f;
	wobj->hitbox.w = 128.0f;
	wobj->hitbox.h = 96.0f;
	wobj->custom_ints[1] = 0;
	Interaction_PlaySound(wobj, 8);
}
static void WobjPlayerLaser_Update(WOBJ *wobj)
{
	wobj->hitbox.x = (wobj->custom_ints[0] == -1) ? -128.0f : 0.0f;

	WobjGenericAttack_Update(wobj);
	wobj->custom_ints[1]++;
	if (wobj->custom_ints[1] >= 30)
		Interaction_DestroyWobj(wobj);
}
static void WobjPlayerLaser_Draw(WOBJ *wobj, int camx, int camy)
{
	CNM_RECT r;

	float start = wobj->y, end = wobj->y - 48.0f + ((float)wobj->custom_ints[1] / 30.0f * 96.0f);
	const int size = 16;

	for (int i = 0; i < size; i++)
	{
		Util_SetRect(&r, 320, 128, 32, 32);
		Renderer_DrawBitmap
		(
			(int)wobj->x + ((i * (128 / size)) * wobj->custom_ints[0]) - camx,
			(int)(((float)i / (float)size) * (end - start) + start) - camy,
			&r,
			0,
			RENDERER_LIGHT
		);
	}
}

static void WobjRocket_Create(WOBJ *wobj)
{
	wobj->flags |= wobj->custom_ints[0] ? WOBJ_HFLIP : 0;
	wobj->hitbox.x = 13.0f;
	wobj->hitbox.y = 13.0f;
	wobj->hitbox.w = 20.0f;
	wobj->hitbox.h = 7.0f;
	wobj->speed = wobj->custom_ints[0] ? -10.0f : 10.0f;
	wobj->vel_y = (float)Util_RandInt(-5, 5) / 3.5f;
	wobj->custom_ints[1] = 0;
	Interaction_PlaySound(wobj, 7);
}
static void WobjRocket_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	wobj->x += wobj->speed;
	wobj->y += wobj->vel_y;
	if (Game_GetFrame() % 15 == 0)
		wobj->vel_y = (float)Util_RandInt(-5, 5) / 3.5f;
	if (wobj->custom_ints[1]++ >= 30*3)
		Interaction_DestroyWobj(wobj);
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f) ||
		Wobj_GetWobjColliding(wobj, WOBJ_IS_HOSTILE))
	{
		WOBJ *explosion = Interaction_CreateWobj(WOBJ_ROCKET_EXPLOSION, wobj->x - 96.0f, wobj->y - 96.0f, 0, 0.0f);
		if (wobj->flags & WOBJ_HFLIP)
			explosion->flags |= WOBJ_HFLIP;
		if (wobj->flags & WOBJ_IS_PLAYER_WEAPON)
			explosion->flags |= WOBJ_IS_PLAYER_WEAPON;
		explosion->strength = wobj->strength;
		Interaction_DestroyWobj(wobj);
	}
}

static void WobjRocketExplosion_Create(WOBJ *wobj)
{
	wobj->custom_ints[0] = 20;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 192.0f;
	wobj->hitbox.h = 192.0f;
	wobj->x += 64.0f;
	wobj->y += 64.0f;
	Interaction_PlaySound(wobj, 6);
	wobj->x -= 64.0f;
	wobj->y -= 64.0f;
	wobj->flags = WOBJ_LIGHT_BIG;
}
static void WobjRocketExplosion_Update(WOBJ *wobj)
{
	WobjGenericAttack_Update(wobj);
	if (wobj->custom_ints[0]-- < 0)
		Interaction_DestroyWobj(wobj);
}

static void WobjPlayerMinigunPellet_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 64.0f;
	wobj->hitbox.h = 32.0f;
	wobj->custom_ints[0] = 0;
	wobj->strength = 100.0f;
}
static void WobjPlayerMinigunPellet_Update(WOBJ *wobj)
{
	WobjGenericAttack_Update(wobj);
}
static void WobjPlayerMinigunPellet_Draw(WOBJ *wobj, int camx, int camy) {
	wobj->anim_frame = Game_GetFrame() % 3;
	WobjGeneric_Draw(wobj, camx, camy);
}

static void WobjUpgradeShoes_Create(WOBJ *wobj)
{
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
}

static void WobjFlameThrowerFlame_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 4.0f, 4.0f, 24.0f, 24.0f);
	wobj->custom_ints[0] = 30*3 + 15;
	wobj->vel_y = 0.15f;
	wobj->flags = WOBJ_LIGHT_BIG; 
	wobj->speed = 3.0f;
}
static void WobjFlameThrowerFlame_Update(WOBJ *wobj)
{
	if (Game_GetFrame() % 2 == 0)
		wobj->custom_floats[0] += (Util_RandFloat() - 0.5f) / 2.0f;

	wobj->vel_y -= 0.01f;

	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 8.0f))
		wobj->vel_y -= 0.05f;
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, -8.0f))
		wobj->vel_y += 0.05f;
	wobj->x += cosf(wobj->custom_floats[0]) * wobj->speed;
	wobj->y += sinf(wobj->custom_floats[0]) * wobj->speed + wobj->vel_y;

	WobjGenericAttack_Update(wobj);

	if (wobj->custom_ints[0] < 30)
		wobj->speed *= 0.92f;

	if (wobj->custom_ints[0]-- <= 0)
		Interaction_DestroyWobj(wobj);
}

static void WobjUltraSwordBoomerang_Create(WOBJ *wobj)
{
	Interaction_PlaySound(wobj, 2);
	// x = cos(t);
	// y = sin(2*t) / 2;
	if (wobj->custom_ints[0])
		wobj->speed = -1.0f;
	else
		wobj->speed = 1.0f;
	wobj->custom_floats[0] = wobj->x;
	wobj->custom_floats[1] = wobj->y;
	wobj->anim_frame = wobj->custom_ints[0];
	wobj->custom_ints[0] = 0;
	wobj->custom_ints[1] = 0;
	wobj->vel_x  = 0.0f; wobj->vel_y = 0.0f;
	wobj->money = 0; // Used as timer
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}
static void WobjUltraSwordBoomerang_Update(WOBJ *wobj)
{
	WobjGenericAttack_Update(wobj);
	WOBJ *player = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
	if (wobj->money++ >= 30 * 8 || player == NULL)
	{
		Interaction_DestroyWobj(wobj);
		if (player != NULL)
			((PLAYER_LOCAL_DATA *)player->local_data)->uswords_have++;
		return;
	}
	if (wobj->money >= 20)
	{
		if (Interaction_GetDistanceToWobj(player, wobj) < 32.0f || Interaction_GetDistanceToWobj(player, wobj) > 500.0f)
		{
			Interaction_DestroyWobj(wobj);
			if (player != NULL)
				((PLAYER_LOCAL_DATA *)player->local_data)->uswords_have++;
		}
	}
	if (wobj->custom_ints[1] == 0)
	{
		const float t = CNM_2RAD(-90.0f + ((float)wobj->custom_ints[0] * 4.0f));
		wobj->custom_ints[0]++;
		wobj->custom_floats[0] += wobj->vel_x;
		wobj->custom_floats[1] += wobj->vel_y;
		wobj->x = wobj->custom_floats[0] + (cosf(t)) * 180.0f * wobj->speed;
		wobj->y = wobj->custom_floats[1] + (sinf(2.0f * t) / 2.0f) * 60.0f;
		if (wobj->speed > 0.0f)
		{
			if (wobj->x + 16.0f < wobj->custom_floats[0])
				wobj->custom_ints[1] = 1;
			if (player->x - 16.0f > wobj->x)
				wobj->custom_ints[1] = 1;
		}
		else
		{
			if (wobj->x - 16.0f > wobj->custom_floats[0])
				wobj->custom_ints[1] = 1;
			if (player->x + 16.0f < wobj->x)
				wobj->custom_ints[1] = 1;
		}

		if (Interaction_GetDistanceToWobj(player, wobj) > 196.0f)
			wobj->custom_ints[1] = 1;
	}
	else
	{
		const float spd = 14.0f;
		if (wobj->x > player->x)
			wobj->x -= spd;
		if (wobj->x < player->x)
			wobj->x += spd;
		if (wobj->y > player->y)
			wobj->y -= spd;
		if (wobj->y < player->y)
			wobj->y += spd;
	}
}

static void WobjHeavyHammerSwing_Create(WOBJ *wobj)
{
	wobj->anim_frame = 0;
	wobj->custom_ints[0] = 0; // timer
	wobj->custom_ints[1] = CNM_TRUE; // Apply ground jump?
	wobj->custom_floats[0] = wobj->custom_ints[0] ? -1.0f : 1.0f;
	wobj->flags = WOBJ_IS_PLAYER_WEAPON;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 0.0f, 0.0f);
}
static void WobjHeavyHammerSwing_Update(WOBJ *wobj)
{
	//static const float x_offsets[3] = { -4.0f, 8.0f, 32.0f };
	//static const float y_offsets[3] = { -24.0f, -20.0f, 0.0f };
	WOBJ *player = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);

	if (player != NULL)
	{
		wobj->custom_floats[0] = (player->flags & WOBJ_HFLIP) ? -1.0f : 1.0f;
		wobj->flags &= ~WOBJ_HFLIP;
		wobj->flags |= (player->flags & WOBJ_HFLIP);

		int frame = wobj->custom_ints[0] / 2;
		if (frame > 5)
			frame = 5;
		wobj->anim_frame = frame;
		//wobj->x = player->x + x_offsets[frame] * wobj->custom_floats[0];
		//wobj->y = player->y + y_offsets[frame];

		if (wobj->anim_frame == 5 || wobj->anim_frame == 4)
		{
			Util_SetBox(&wobj->hitbox, -32.0f, 0.0f, 92.0f, 32.0f);
			WOBJ *victim = Interaction_GetVictim(wobj, WOBJ_IS_HOSTILE | WOBJ_IS_BREAKABLE);
			//Interaction_DamageWobj(wobj, victim);
			Interaction_DamageFoundWobjs(wobj);
			Interaction_DamageOtherPlayers(wobj);
			//WobjGenericAttack_Update(wobj);
			//if (wobj->custom_ints[0] >= 8 && wobj->custom_ints[0] <= 12)
			//{
				if (wobj->custom_ints[0] == 10)
					Interaction_PlaySound(wobj, 15);

				if (victim != NULL && !Wobj_IsGrouneded(player) && wobj->custom_ints[1])// && !((PLAYER_LOCAL_DATA *)player->local_data)->has_hammer_jumped)
				{
					//((PLAYER_LOCAL_DATA *)player->local_data)->has_hammer_jumped = CNM_TRUE;
					wobj->custom_ints[1] = CNM_FALSE;
					if (player != NULL)
					{
						if (player->flags & WOBJ_HFLIP)
							player->vel_x += 10.0f;
						else
							player->vel_x -= 10.0f;
						player->vel_y = -10.0f;
						player->y -= 1.0f;
					}
				}
			//}
		}
	}

	if (wobj->custom_ints[0]++ > 12)
	{
		WOBJ *blast = Interaction_CreateWobj(PLAYER_HEAVY_BLAST, wobj->x, wobj->y - 1.0f, 0, wobj->custom_floats[0]);
		blast->speed = 9.0f;
		blast->strength = wobj->strength - 0.3f;
		if (player != NULL)
		{
			if (Wobj_IsGrouneded(player) && wobj->custom_ints[1])
			{
				if (player->flags & WOBJ_HFLIP)
					player->vel_x += 10.0f;
				else
					player->vel_x -= 10.0f;
				player->vel_y = -10.0f;
				player->y -= 1.0f;
			}
		}
		Interaction_DestroyWobj(wobj);
		if (player->item) {
			Item_GetCurrentItem()->durability -= 1.0f;
		}
		//((PLAYER_LOCAL_DATA *)player->local_data)->has_hammer_jumped = CNM_FALSE;
	}
}
static void WobjCheckpoint_Create(WOBJ *wobj)
{
	wobj->anim_frame = 0;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}

static void WobjHorizontalPushZone_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 128.0f, 128.0f);
}
static void WobjVerticalPushZone_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 64.0f, 64.0f);
}
static void WobjSmallHorizontalPushZone_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}

static void WobjFissionEnergyBolt_Create(WOBJ *wobj)
{
	const float spd = 8.0f;
	switch (wobj->custom_ints[0])
	{
	case 0: wobj->vel_x = spd; wobj->vel_y = 0.0f; break;
	case 1: wobj->vel_x = 0.0f; wobj->vel_y = -spd; break;
	case 2: wobj->vel_x = -spd; wobj->vel_y = 0.0f; break;
	case 3: wobj->vel_x = 0.0f; wobj->vel_y = spd; break;
	}
	wobj->custom_ints[0] = 0;
	wobj->flags = WOBJ_IS_PLAYER_WEAPON;
	wobj->money = 0; // Has hit enemy flag
	Util_SetBox(&wobj->hitbox, 2.0f, 2.0f, 28.0f, 28.0f);
	Interaction_PlaySound(wobj, 14);
}
static void WobjFissionEnergyBolt_Update(WOBJ *wobj)
{
	if (wobj->custom_ints[1] > 3 || wobj->custom_ints[0]++ > 30*4)
	{
		Interaction_DestroyWobj(wobj);
		return;
	}

	wobj->x += wobj->vel_x;
	wobj->y += wobj->vel_y;

	const int penalty = 15;
	if (wobj->vel_x != 0.0f)
	{
		if (Wobj_IsCollidingWithBlocks(wobj, 8.0f, 0.0f))
		{
			wobj->vel_x = -fabsf(wobj->vel_x);
			wobj->custom_ints[0] += penalty;
		}
		if (Wobj_IsCollidingWithBlocks(wobj, -8.0f, 0.0f))
		{
			wobj->vel_x = fabsf(wobj->vel_x);
			wobj->custom_ints[0] += penalty;
		}
	}
	if (wobj->vel_y != 0.0f)
	{
		if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 8.0f))
		{
			wobj->vel_y = -fabsf(wobj->vel_y);
			wobj->custom_ints[0] += penalty;
		}
		if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, -8.0f))
		{
			wobj->vel_y = fabsf(wobj->vel_y);
			wobj->custom_ints[0] += penalty;
		}
	}
	WobjGenericAttack_Update(wobj);

	WOBJ *nme = Wobj_GetWobjColliding(wobj, WOBJ_IS_HOSTILE);
	if (nme != NULL && !wobj->money)
	{
		if (wobj->custom_ints[1] > 0 && wobj->link_node == nme->node_id && wobj->link_uuid == nme->uuid)
			return;

		for (int i = 0; i < 4; i++)
		{
			WOBJ *bolt = Interaction_CreateWobj(FISSION_ENERGY_BOLT, wobj->x, wobj->y, i, 0.0f);
			bolt->strength = wobj->strength;
			bolt->custom_ints[1] = wobj->custom_ints[1] + 1;
			bolt->link_node = nme->node_id;
			bolt->link_uuid = nme->uuid;
		}

		wobj->money = 1;
	}
}

static void WobjDissapearingPlatform_Create(WOBJ *wobj)
{
	wobj->custom_ints[1] = (int)wobj->custom_floats[0];
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
	if (wobj->custom_ints[0] >= 0)
	{
		wobj->flags = WOBJ_IS_SOLID;							// state
		wobj->money = wobj->custom_ints[0];		// state timer
	}
	else
	{
		wobj->custom_ints[0] *= -1;
		wobj->flags = 0;							// state
		wobj->money = wobj->custom_ints[1];		// state timer
	}
}
static void WobjDissapearingPlatform_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	if (wobj->money-- < 0)
	{
		wobj->flags ^= WOBJ_IS_SOLID;
		if (wobj->flags & WOBJ_IS_SOLID)
			wobj->money = wobj->custom_ints[0];
		else
			wobj->money = wobj->custom_ints[1];
	}

	if (wobj->flags & WOBJ_IS_SOLID)
	{
		wobj->anim_frame = 0;
		if (wobj->money < 5 || wobj->money > wobj->custom_ints[0] - 10)
			wobj->anim_frame = 1;
	}
	else
		wobj->anim_frame = 2;
}

static void WobjCoolPlatform_Create(WOBJ *wobj) {
	int packed = wobj->custom_ints[0];
	wobj->custom_ints[0] = packed & 0xff; // Time off
	wobj->custom_ints[1] = wobj->custom_ints[0] + (packed >> 8 & 0xff); // Time on
	wobj->item = wobj->custom_ints[1] + (packed >> 16 & 0xff); // Time off (again)
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
	wobj->flags = WOBJ_IS_SOLID;
	wobj->money = 0;
}
static void WobjCoolPlatform_Update(WOBJ *wobj) {
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	wobj->money = Game_GetFrame() % (wobj->item);
	int solid = wobj->money >= wobj->custom_ints[0] && wobj->money < wobj->custom_ints[1];
	if (!solid) wobj->flags &= ~WOBJ_IS_SOLID;
	else wobj->flags |= WOBJ_IS_SOLID;
	wobj->anim_frame = 2;
	if (solid) {
		if (wobj->money < wobj->custom_ints[0] + 5) wobj->anim_frame = 1;
		else if (wobj->money > wobj->custom_ints[1] - 10) wobj->anim_frame = 1;
		else wobj->anim_frame = 0;
	}
}

static void WobjSpringBoard_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 16.0f, 32.0f, 16.0f);
	wobj->custom_ints[0] = 0;
}
static void WobjSpringBoard_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	if (wobj->custom_ints[0]-- <= 0)
		wobj->anim_frame = 0;
	else
		wobj->anim_frame = 1;
}

static void WobjHorizontalBackgroundSwitch_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 128.0f, 32.0f);
	wobj->custom_ints[1] = (int)wobj->custom_floats[0];
}
static void WobjVerticalBackgroundSwitch_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 96.0f);
	wobj->custom_ints[1] = (int)wobj->custom_floats[0];
}
static void WobjVeryBigTunesTrigger_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 96.0f, 96.0f);
}

static void WobjJumpthrough_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_JUMPTHROUGH;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}
static void WobjBigJumpthrough_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_JUMPTHROUGH;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 96.0f, 32.0f);
}

static void WobjBreakablePlatform_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
	wobj->custom_ints[1] = wobj->custom_ints[0];
	wobj->custom_ints[0] = 0;
	wobj->flags = WOBJ_IS_SOLID;
	wobj->anim_frame = 0;
}
static void WobjBreakablePlatform_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	WOBJ *np = Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y);

	if (np != NULL && wobj->custom_ints[0] == 0)
	{
		CNM_BOX a, b;
		Util_SetBox(&a, wobj->x, wobj->y - 2.0f, 32.0f, 32.0f);
		Util_SetBox(&b, np->x + np->hitbox.x, np->y + np->hitbox.y, np->hitbox.w, np->hitbox.h);
		if (Util_AABBCollision(&a, &b) && (np->y + np->hitbox.y + np->hitbox.h) < (wobj->y + 2.0f))
			wobj->custom_ints[0] = 1;
	}

	if (wobj->custom_ints[0])
	{
		if (wobj->custom_ints[0] == wobj->custom_ints[1] - 10)
			Interaction_PlaySound(wobj, 32);
		if (wobj->custom_ints[1] - 10 < 1 && wobj->custom_ints[0] == 1)
			Interaction_PlaySound(wobj, 32);
		if (wobj->custom_ints[0] > wobj->custom_ints[1] + 1)
			Interaction_DestroyWobj(wobj);
		wobj->custom_ints[0]++;
		wobj->anim_frame = 0;
		if (wobj->custom_ints[1] - wobj->custom_ints[0] < 8)
			wobj->anim_frame = 2;
	}
}

static void WobjSkinBreakableWall_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_BREAKABLE | WOBJ_IS_SOLID;
	wobj->hitbox.x = 0.0f;
	wobj->hitbox.y = 0.0f;
	wobj->hitbox.w = 32.0f;
	wobj->hitbox.h = 32.0f;
	wobj->health = wobj->custom_floats[0];
	wobj->anim_frame = wobj->custom_ints[0] + 1;
}

static void WobjLockedBlockGeneric_Create(WOBJ *wobj)
{
	wobj->anim_frame = 0;
	wobj->flags = WOBJ_IS_SOLID;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}

static void WobjWaterSplashEffect_Create(WOBJ *wobj)
{
	Interaction_PlaySound(wobj, 41);
	wobj->custom_ints[0] = 0;
	wobj->anim_frame = 0;
}
static void WobjWaterSplashEffect_Update(WOBJ *wobj)
{
	wobj->anim_frame = (wobj->custom_ints[0]++) / 4;
	if (wobj->anim_frame > 3)
	{
		wobj->anim_frame = 3;
		Interaction_DestroyWobj(wobj);
	}
}
static void WobjWaterSplashEffect_Draw(WOBJ *wobj, int camx, int camy)
{
	int offset = 16;
	if (wobj->anim_frame >= 2)
		offset = 0;
	Renderer_DrawBitmap((int)wobj->x - camx, (int)wobj->y - camy + offset, wobj_types[WOBJ_WATER_SPLASH_EFFECT].frames + wobj->anim_frame, 0, RENDERER_LIGHT);
}

static void WobjHealthSetTrigger_Create(WOBJ *wobj)
{
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 64.0f, 96.0f);
}

#define VORTEX_CHECK_HOTBOX Util_SetBox(&wobj->hitbox, 48.0f, 48.0f, 32.0f, 32.0f);
#define VORTEX_NORMAL_HITBOX Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 96.0f, 96.0f)

static void WobjVortex_Create(WOBJ *wobj)
{
	//Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 96.0f, 96.0f);
	wobj->custom_ints[1] = CNM_FALSE; // This is a like "cannot use flag"
	VORTEX_CHECK_HOTBOX;
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f))
		wobj->custom_ints[1] = CNM_TRUE;
	VORTEX_NORMAL_HITBOX;
	//wobj->custom_ints[1] = 0;
}
static void WobjVortex_Update(WOBJ *wobj)
{
	//if (--wobj->custom_ints[1] == 0)
	//	Interaction_DestroyWobj(wobj);
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	VORTEX_CHECK_HOTBOX;
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f))
		Interaction_DestroyWobj(wobj);
	VORTEX_NORMAL_HITBOX;

	if (wobj->custom_ints[0])
	{
		// Attrack enemies in
		int gx, gy;
		for (int x = -1; x < 3; x++)
		{
			for (int y = -1; y < 3; y++)
			{
				gx = (int)(wobj->x / OBJGRID_SIZE) + x;
				gy = (int)(wobj->y / OBJGRID_SIZE) + y;
				WOBJ_GRID_ITER iter[2];
				Wobj_InitGridIteratorOwned(iter + 0, gx, gy);
				Wobj_InitGridIteratorUnowned(iter + 1, gx, gy);
				for (int i = 0; i < 2; i++)
				{
					int started = 0;
					while (iter[i].wobj != NULL)
					{
						if (started)
							Wobj_GridIteratorIterate(iter + i);
						started++;
						if (iter[i].wobj == NULL || started > 16)
							break;
						if (!(iter[i].wobj->flags & WOBJ_IS_HOSTILE))
							continue;

						float change_speed = 4.0f;
						float change_x = (iter[i].wobj->vel_x < 0.0f) ? (iter[i].wobj->vel_x + change_speed) : change_speed;
						float change_y = (iter[i].wobj->vel_y < 0.0f) ? (iter[i].wobj->vel_y + change_speed) : change_speed;

						if (iter[i].wobj->x > wobj->x + 32.0f)
							change_x = (iter[i].wobj->vel_x > 0.0f) ? (-iter[i].wobj->vel_x - change_speed) : -change_speed;
						if (iter[i].wobj->y > wobj->y + 32.0f)
							change_y = (iter[i].wobj->vel_y > 0.0f) ? (-iter[i].wobj->vel_y - change_speed) : -change_speed;
						
						Interaction_ForceWobjPosition(iter[i].wobj, iter[i].wobj->x + change_x, iter[i].wobj->y + change_y);
					}
				}
			}
		}
	}
}

#define CMPF_STOP_ON_TURN 1
#define CMPF_HOP_ON 2
#define CMPF_FIRST_GO 4
#define CMPF_WAS_ON 8
#define CMPF_DESPAWN 16

static void WobjCustomizableMovingPlatform_Create(WOBJ *wobj)
{
	wobj->flags = WOBJ_IS_SOLID | WOBJ_IS_MOVESTAND;
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);

	int converted;
	float fconverted, fpart;
	//Console_Print("%d %d %d %d %d %d", CMPF_INT_BITS, CMPF_FRAC_BITS, CMPF_INT_MASK, CMPF_FRAC_MASK, CMPF_INT_MID, CMPF_NUM_FRACS);
	converted = (int)wobj->custom_ints[0] >> 16 & 0xff;
	fconverted = (float)((int)(converted >> 4 & 0xf) - 8);
	fpart = (1.0f / 16.0f) * (float)(converted & 0xf);
	fconverted += fconverted < 0 ? -fpart : fpart;
	wobj->vel_x = fconverted;
	converted = (int)wobj->custom_ints[0] >> 24 & 0xff;
	fconverted = (float)((int)(converted >> 4 & 0xf) - 8);
	fpart = (1.0f / 16.0f) * (float)(converted & 0xf);
	fconverted += fconverted < 0 ? -fpart : fpart;
	wobj->vel_y = fconverted;
	//Console_Print("%f %f %u", wobj->vel_x, wobj->vel_y, wobj->custom_ints[0]);
	const float fractional_comp = wobj->custom_floats[0] - floorf(wobj->custom_floats[0]);

	if (wobj->custom_floats[0] < 0.0f)
	{
		wobj->money |= CMPF_STOP_ON_TURN; // Stop on turn flag and only start moving once player touched this
		wobj->custom_ints[1] = (int)floorf(wobj->custom_floats[0]) * -1;

		// If timer has a fraciontal component, then dont let player hop back on and go back up, else go back up
		if (fractional_comp < 0.1f)
			wobj->money |= CMPF_HOP_ON; // Let player hop back on
	}
	else
		wobj->custom_ints[1] = (int)floorf(wobj->custom_floats[0]);
	if (fractional_comp > 0.1f && fractional_comp < 0.3f)
		wobj->money |= CMPF_DESPAWN;

	wobj->money |= CMPF_FIRST_GO;
	if (wobj->money & CMPF_STOP_ON_TURN)
		wobj->item = 0;
	else
		wobj->item = wobj->custom_ints[1];
	wobj->custom_floats[0] = wobj->vel_x;
	wobj->custom_floats[1] = wobj->vel_y;
}
static void WobjCustomizableMovingPlatform_Update(WOBJ *wobj)
{
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	if (wobj->item > 0)
	{
		wobj->x += wobj->vel_x;
		wobj->y += wobj->vel_y;
		wobj->money &= ~CMPF_FIRST_GO;
		wobj->money |= CMPF_WAS_ON;
		wobj->item--;
	}
	else
	{
		if ((wobj->money & CMPF_DESPAWN) && (~wobj->money & CMPF_FIRST_GO))
			Interaction_DestroyWobj(wobj);
		if (~wobj->money & CMPF_FIRST_GO) {
			if (wobj->vel_x != 0.0f) {
				wobj->custom_floats[0] = -wobj->vel_x;
				wobj->vel_x = 0.0f;
			}
			if (wobj->vel_y != 0.0f) {
				wobj->custom_floats[1] = -wobj->vel_y;
				wobj->vel_y = 0.0f;
			}
		}
		if (!(wobj->money & CMPF_STOP_ON_TURN) && !(wobj->money & CMPF_HOP_ON))
		{
			wobj->item = wobj->custom_ints[1];
			wobj->vel_x = wobj->custom_floats[0];
			wobj->vel_y = wobj->custom_floats[1];
			return;
		}
		if (wobj->money & CMPF_STOP_ON_TURN)
		{
			// Look for player
			Util_SetBox(&wobj->hitbox, 0.0f, -4.0f, 32.0f, 32.0f);
			if (Wobj_GetWobjCollidingWithType(wobj, WOBJ_PLAYER))
			{
				if ((wobj->money & CMPF_HOP_ON || wobj->money & CMPF_FIRST_GO) &&
					!(wobj->money & CMPF_WAS_ON))
				{
					wobj->item = wobj->custom_ints[1];
					wobj->vel_x = wobj->custom_floats[0];
					wobj->vel_y = wobj->custom_floats[1];
				}
			}
			else
			{
				wobj->money &= ~CMPF_WAS_ON;
			}
			Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
		}
	}
}
static void WobjCustomizableMovingPlatform_Draw(WOBJ *wobj, int camx, int camy)
{
	CNM_RECT r;
	r.x = (wobj->custom_ints[0] & 0xf) * 32;
	r.y = (wobj->custom_ints[0] >> 4 & 0xfff) * 32;
	r.w = 32;
	r.h = 32;
	Renderer_DrawBitmap
	(
		(int)wobj->x - camx,
		(int)wobj->y - camy,
		&r,
		0,
		RENDERER_LIGHT
	);
}
static void WobjTeleParticles_Create(WOBJ *wobj)
{
	if (!wobj->custom_ints[0])
		return;
	for (int i = 0; i < 10; i++)
		Interaction_CreateWobj(WOBJ_DUST_PARTICLE, wobj->x + 10.0f, wobj->y - 10.0f, 0, Util_RandFloat() * CNM_PI);
}
static void WobjTeleParticles_Update(WOBJ *wobj)
{
	if (wobj->custom_ints[0]++ > 24)
		Interaction_DestroyWobj(wobj);
	wobj->anim_frame = wobj->custom_ints[0] / 3;
}
static void WobjTeleParticles_Draw(WOBJ *wobj, int camx, int camy)
{
	int x, y;
	x = (int)wobj->x - camx;
	y = (int)wobj->y - camy;
	Renderer_DrawBitmap2(x, y, wobj_types[WOBJ_TELE_PARTICLES].frames + wobj->anim_frame, 0, RENDERER_LIGHT, 0, 0);
	Renderer_DrawBitmap2(x+16, y, wobj_types[WOBJ_TELE_PARTICLES].frames + wobj->anim_frame, 0, RENDERER_LIGHT, 1, 0);
	Renderer_DrawBitmap2(x, y+16, wobj_types[WOBJ_TELE_PARTICLES].frames + wobj->anim_frame, 0, RENDERER_LIGHT, 0, 1);
	Renderer_DrawBitmap2(x+16, y+16, wobj_types[WOBJ_TELE_PARTICLES].frames + wobj->anim_frame, 0, RENDERER_LIGHT, 1, 1);
}
static void WobjDustParticle_Create(WOBJ *wobj)
{
	wobj->custom_floats[1] = ((sinf(wobj->custom_floats[0]) *-1.0f) + 1.0f / 2.0f) * 7.0f + 0.4f;
	wobj->anim_frame = 0;
}
static void WobjDustParticle_Update(WOBJ *wobj)
{
	if (wobj->custom_ints[0]++ > 10*4)
		Interaction_DestroyWobj(wobj);
	wobj->anim_frame = wobj->custom_ints[0] / 10;
	wobj->x += cosf(wobj->custom_floats[0]) * wobj->custom_floats[1];
	wobj->y += sinf(wobj->custom_floats[0]) * wobj->custom_floats[1];
	if (wobj->custom_floats[1] > 0.05f)
		wobj->custom_floats[1] -= 0.08f;
	if (cosf(wobj->custom_floats[0]) < 0.0f)
		wobj->custom_floats[0] -= 5.0f / 180.0f * CNM_PI;
	if (cosf(wobj->custom_floats[1]) > 0.0f)
		wobj->custom_floats[0] += 5.0f / 180.0f * CNM_PI;
}

static void WobjBloodParticle_Create(WOBJ *wobj)
{
	wobj->anim_frame = Util_RandInt(0, 3);
	wobj->vel_x = cosf(wobj->custom_floats[0]) * (float)wobj->custom_ints[0];
	wobj->vel_y = -sinf(wobj->custom_floats[0]) * (float)wobj->custom_ints[0];
	Util_SetBox(&wobj->hitbox, 4.0f, 4.0f, 8.0f, 8.0f);
}
static void WobjBloodParticle_Update(WOBJ *wobj)
{
	wobj->vel_y += 0.3f;
	wobj->x += wobj->vel_x;
	wobj->y += wobj->vel_y;
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f))
	{
		wobj->vel_x = 0.0f;
		wobj->vel_y = 0.0f;
		if (!wobj->custom_ints[1])
		{
			Interaction_PlaySound(wobj, 48);
			wobj->custom_ints[1] = CNM_TRUE;
		}
	}
	if (wobj->custom_ints[0]++ > 70)
		Interaction_DestroyWobj(wobj);
}

static void WobjGibParticle_Create(WOBJ *wobj)
{
	wobj->anim_frame = Util_RandInt(0, 3);
	wobj->vel_x = cosf(wobj->custom_floats[0]) * (float)wobj->custom_ints[0];
	wobj->vel_y = -sinf(wobj->custom_floats[0]) * (float)wobj->custom_ints[0];
	Util_SetBox(&wobj->hitbox, 4.0f, 4.0f, 8.0f, 8.0f);
}
static void WobjGibParticle_Update(WOBJ *wobj)
{
	wobj->vel_y += 0.4f;
	wobj->x += wobj->vel_x;
	wobj->y += wobj->vel_y;
	if (wobj->custom_ints[0]++ > 70)
		Interaction_DestroyWobj(wobj);
	if (Wobj_IsCollidingWithBlocks(wobj, wobj->vel_x, 0.0f))
	{
		wobj->vel_x *= -1.0f;
		Interaction_PlaySound(wobj, 48);
	}
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, wobj->vel_y))
	{
		wobj->vel_y *= -1.0f;
		Interaction_PlaySound(wobj, 48);
	}
}

static void WobjBossBarInfo_Create(WOBJ *wobj)
{
	wobj->custom_ints[1] = 0;
	Util_SetBox(&wobj->hitbox, -8.0f, 8.0f, 48.0f, 48.0f);

	WOBJ *w;
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	int i;

	w = NULL;
	Wobj_GetCollisionsWithFlags(wobj, collisions, WOBJ_IS_HOSTILE);
	for (i = 0; i < WOBJ_MAX_COLLISIONS; i++)
	{
		if (collisions[i] != NULL && wobj_types[collisions[i]->type].hurt != NULL)
			w = collisions[i];
	}

	if (w != NULL)
	{
		wobj->link_node = w->node_id;
		wobj->link_uuid = w->uuid;
		wobj->custom_floats[0] = w->health;
		wobj->custom_ints[1] = 100;
	}
}
static void WobjBossBarInfo_Update(WOBJ *wobj)
{
	WOBJ *w;
	WOBJ *collisions[WOBJ_MAX_COLLISIONS];
	int i;

	if (wobj->custom_ints[1]++ == 0) {
		//w = Interaction_GetVictim(wobj, WOBJ_IS_HOSTILE);
		w = NULL;
		Wobj_GetCollisionsWithFlags(wobj, collisions, WOBJ_IS_HOSTILE);
		for (i = 0; i < WOBJ_MAX_COLLISIONS; i++) {
			if (collisions[i] != NULL && wobj_types[collisions[i]->type].hurt != NULL)
				w = collisions[i];
		}

		if (w == NULL) {
			Wobj_GetCollisionsWithType(wobj, collisions, LAVA_DRAGON_HEAD);
			for (i = 0; i < WOBJ_MAX_COLLISIONS; i++) if (collisions[i] != NULL) w = collisions[i];
		}

		if (w != NULL) {
			wobj->link_node = w->node_id;
			wobj->link_uuid = w->uuid;
			wobj->custom_floats[0] = w->health;
		}
		else
			Interaction_DestroyWobj(wobj);
	}
	if (wobj->custom_ints[1] % 9 == 0) {
		w = Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid);
		if (w == NULL)
			Interaction_DestroyWobj(wobj);
		else
		{
			wobj->x = w->x;
			wobj->y = w->y;
		}
	}
}
static void WobjBossBarInfo_Draw(WOBJ *wobj, int camx, int camy)
{
	BossBar_RegisterBar(wobj->link_node, wobj->link_uuid, wobj->custom_floats[0], EndingText_GetLine(wobj->custom_ints[0]));
}
static void Wobj_Trigger32x32_Create(WOBJ *wobj) {
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}
static void Wobj_TeleTrigger1_Create(WOBJ *wobj) {
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 96.0f);
	wobj->custom_ints[1] = -1;
}
static void Wobj_TeleTrigger1_Update(WOBJ *wobj) {
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	if (Wobj_GetWobjCollidingWithType(wobj, WOBJ_PLAYER) != NULL && wobj->custom_ints[1] == -1) {
		wobj->custom_ints[1] = (int)(wobj->custom_floats[0] * 30.0f);
	}

	if (wobj->custom_ints[1] == -1) return;
	if (wobj->custom_ints[1]-- <= 0) {
		if (LogicLinks_GetWobjFromLink(wobj->custom_ints[0]) != NULL) {
			LogicLinks_GetWobjFromLink(wobj->custom_ints[0])->custom_ints[1] = 1;
		}
		Interaction_DestroyWobj(wobj);
	}
}
static void Wobj_TeleArea1_Create(WOBJ *wobj) {
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 128.0f, 128.0f);
	if (wobj->internal.owned)
		LogicLinks_AddWobjToLink(wobj, (int)wobj->custom_floats[0]);
}
static void Wobj_TeleArea1_Update(WOBJ *wobj) {
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
	return;
}

static void Wobj_TeleArea2_Create(WOBJ *wobj) {
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 128.0f, 128.0f);
	if (wobj->custom_ints[0] & 0x10000) wobj->custom_ints[1] = 1;
	if (wobj->internal.owned)
		LogicLinks_AddWobjToLink(wobj, (int)wobj->custom_floats[0]);
}
static void Wobj_TeleArea2_Update(WOBJ *wobj) {
	return;
}
static void Wobj_TryTeleportArea2(WOBJ *wobj) {
	Wobj_TryTeleportWobj(wobj, CNM_TRUE);
}

static void Wobj_SfxPoint_Draw(WOBJ *wobj, int camx, int camy) {
	if (!Audio_IsSoundPlaying(wobj->custom_ints[0]))
		Audio_PlaySoundscape(wobj->custom_ints[0], (int)wobj->x, (int)wobj->y);
}
static void WobjSkinUnlock_Create(WOBJ *wobj) {
	Util_SetBox(&wobj->hitbox, 0.0f, 0.0f, 32.0f, 32.0f);
}
static void WobjSkinUnlock_Draw(WOBJ *wobj, int camx, int camy) {
	if (Game_GetFrame() % 2 == 0) wobj->anim_frame = (wobj->anim_frame + 1) % 8;
	Renderer_DrawBitmap2
	(
		(int)wobj->x - camx,
		(int)wobj->y + (sinf((float)Game_GetFrame() / 10.0f + wobj->x + wobj->y) * 5.0f) - camy,
		&wobj_types[WOBJ_SKIN_UNLOCK].frames[wobj->custom_ints[0]],
		0,
		Blocks_GetCalculatedBlockLight(wobj->x / BLOCK_SIZE, wobj->y / BLOCK_SIZE),
		wobj->flags & WOBJ_HFLIP,
		wobj->flags & WOBJ_VFLIP
	);
	Renderer_DrawBitmap
	(
		(int)wobj->x - camx,
		(int)wobj->y - camy,
		&(CNM_RECT) {
			.x = 480, .y = 7520 + wobj->anim_frame * 32, .w = 32, .h = 32
		},
		2,
		RENDERER_LIGHT
	);
}

#define LUAOBJ_DEF {\
	NULL,\
	NULL,\
	NULL,\
	WobjGeneric_Hurt,\
	{\
		{448, 1344, 32, 32},\
	},\
	0.0f,\
	10,\
	CNM_TRUE,\
	CNM_TRUE\
	},
WOBJ_TYPE wobj_types[WOBJ_MAX] =
{
	{ /* 0: Null Object */
		NULL, NULL, NULL, NULL,
		{
			{0, 0, 0, 0}
		},
		0.0f,
		0,
		0,
		CNM_FALSE
	},
	{ /* 1: Teleport Object */
		WobjTeleport_Create, WobjTeleport_Animate, WobjTeleport_Draw, NULL,
		{
			{96, 32, 32, 32},
			{32, 160, 32, 32}
		},
		0.0f,
		0,
		CNM_FALSE,
		CNM_FALSE
	},
	{ /* 2: Slime Object */
		WobjSlime_Create, WobjSlime_Update, WobjSlime_Draw, WobjGeneric_Hurt,
		{
			{32, 256, 32, 32},
			{32, 256 + 32, 32, 32}
		},
		0.00f,
		10,
		CNM_TRUE,
		CNM_TRUE,
	},
	{ /* 3: Player Object */
		WobjPlayer_Create, WobjPlayer_Update, WobjPlayer_Draw, NULL,
		{
			{0, 288, 32, 32},			// 0
			{256, 800, 32, 32},			// 1
			{0, 768, 32, 32},			// 2
			{32, 768, 32, 32},			// 3
			{64, 768, 32, 32},			// 4
			{96, 768, 32, 32},			// 5
			{128, 768, 32, 32},			// 6
			{192, 704, 32, 32},			// 7
			{224, 768, 32, 32},			// 8
			{256, 768, 32, 32},			// 9
			{0, 800, 32, 32},			// 10
			{32, 800, 32, 32},			// 11
			{64, 800, 32, 32},			// 12
			{96, 800, 32, 32},			// 13
			{128, 800, 32, 32},			// 14
			{160, 800, 32, 32},			// 15
			{192, 800, 32, 32},			// 16
			{224, 800, 32, 32},			// 17
			{480, 768, 32, 32}			// 18
		},
		0.0f,
		0,
		CNM_TRUE,
		CNM_FALSE
	},
	{ /* 4: Shotgun Item Object */
		WobjShotgunItem_Create, NULL, WobjGeneric_Draw, NULL,
		{
			{32, 352, 32, 32}
		},
		0.0f,
		0,
		CNM_FALSE,
		CNM_FALSE
	},
	{ /* 5: Player Shotgun Pellet Object */
		WobjPlayerPellet_Create, WobjPlayerPellet_Update, WobjGeneric_Draw, NULL,
		{
			{0, 224, 32, 32}
		},
		0.0f,
		0,
		CNM_TRUE,
		CNM_FALSE
	},
	{ /* 6: Small Tunes Object */
		WobjSmallTunesTrigger_Create, Wobj_TryTeleportArea2, NULL, NULL,
		{
			{416, 1216, 32, 32}
		},
		0.0f,
		0,
		CNM_FALSE,
		CNM_FALSE
	},
	{ /* 7: Big Tunes Object */
		WobjBigTunesTrigger_Create, Wobj_TryTeleportArea2, NULL, NULL,
		{
			{448, 1216, 64, 64}
		},
		0.0f,
		0,
		CNM_FALSE,
		CNM_FALSE
	},
	{ /* 8: Player Spawn Object */
		NULL, NULL, NULL, NULL,
		{
			{384, 1216, 32, 32}
		},
		0.0f,
		0,
		CNM_FALSE,
		CNM_FALSE
	},
	{ /* 9: Ending Text Trigger */
		WobjEndingTextTrigger_Create, Wobj_TryTeleportArea2, NULL, NULL,
		{
			{352, 1216, 32, 32}
		},
		0.0f,
		0,
		CNM_FALSE,
		CNM_FALSE
	},
	{ /* 10: Super City Moving Platform */
		WobjScMovingPlatform_Create, WobjScMovingPlatform_Update, WobjGeneric_Draw, NULL,
		{
			{288, 160, 32, 32}
		},
		0.0f,
		0,
		CNM_TRUE,
		CNM_FALSE
	},
	{ /* 11: Generic Breakable Brick Wall */
		WobjBrickWallBreakable_Create, Wobj_TryTeleportArea2, WobjGeneric_Draw, WobjGeneric_Hurt,
		{
			{256, 160, 32, 32}
		},
		0.0f,
		0,
		CNM_FALSE,
		CNM_TRUE // can respawn
	},
	{ /* 12: Background Switch Trigger */
		WobjBackgroundSwitch_Create, Wobj_TryTeleportArea2, NULL, NULL,
		{
			{352, 1312, 32, 32}
		},
		0.0f,
		0,
		CNM_FALSE,
		CNM_FALSE
	},
	{ /* 13: Knife attack object */
		WobjKnifeAttack_Create,				// On Create
		WobjGenericAttack_Update,								// On Update
		WobjGeneric_Draw,					// On Draw
		NULL,								// On Hurt
		{									// Animation Frames
			{32, 320, 32, 32},
			{0, 0, 0, 0}
		},
		0.0f,								// Strength Reward
		0,									// Money Reward
		CNM_TRUE,							// Does Network Interpolation
		CNM_FALSE							// Can Respawn
	},
	{ /* 14: Knife Item Object */
		WobjKnifeItem_Create, NULL, WobjGeneric_Draw, NULL,
		{
			{64, 352, 32, 32}
		},
		0.0f,
		0,
		CNM_FALSE,
		CNM_FALSE
	},
	{ /* 15: Apple Item Object */
		WobjAppleItem_Create, // On Create
		NULL,				  // On Update
		WobjGeneric_Draw,	  // On Draw
		NULL,				  // On Hurt
		{					  // Animation Frames
			{96, 352, 32, 32}
		},
		0.0f,				  // Strength Reward
		0,					  // Money Reward
		CNM_FALSE,			  // Does Network Interpolation
		CNM_TRUE			  // Can Respawn
	},
	{ /* 16: Dropped Item Object */
		WobjDroppedItem_Create, // On Create
		WobjDroppedItem_Update,	// On Update
		WobjDroppedItem_Draw,	  // On Draw
		NULL,				  // On Hurt
		{					  // Animation Frames
			{320, 1216, 32, 32}, // Null Item
			{32, 352, 32, 32},  // Shotgun item
			{64, 352, 32, 32},  // Knife Item
			{96, 352, 32, 32},  // Apple Item
			{128, 352, 32, 32}, // Cake Item
			{160, 352, 32, 32}, // Strength Potion
			{192, 352, 32, 32}, // Speed Potion
			{224, 352, 32, 32}, // Jump Potion
			{0, 384, 32, 32},	// Sword Item
			{32, 384, 32, 32}, 	// Health Potion
			{64, 384, 32, 32}, 	// Sniper Rifle
			{96, 384, 32, 32}, 	// 50 Dollars
			{128, 384, 32, 32},	// 100 Dollars
			{160, 384, 32, 32},	// 500 Dollars
			{192, 384, 32, 32},	// Cheeseburger
			{224, 384, 32, 32},	// Golden Axe
			{0, 416, 32, 32},	// Unbound Wand
			{32, 416, 32, 32}, 	// Fire Wand
			{64, 416, 32, 32}, 	// Ice Wand
			{96, 416, 32, 32}, 	// Air Wand
			{128, 416, 32, 32},	// Lightning Wand
			{160, 416, 32, 32},	// Golden Shotgun
			{192, 416, 32, 32},	// Laser Rifle
			{224, 416, 32, 32},	// Rocket Launcher
			{0, 448, 32, 32},	// Fire Resistance Potion
			{32, 448, 32, 32}, 	// Minigun
			{64, 448, 32, 32}, 	// Mega Health Potion
			{96, 448, 32, 32}, 	// Ultra Mega Health Potion
			{128, 448, 32, 32},	// AWP Item
			{160, 448, 32, 32},	// Flamethrower
			{192, 448, 32, 32},	// Poisionus Strength Potion
			{224, 448, 32, 32},	// Poisionus Speed Potion
			{0, 480, 32, 32},	// Poisionus Jump Potion
			{32, 480, 32, 32}, 	// Unused
			{64, 480, 32, 32}, 	// Unused
			{96, 480, 32, 32}, 	// Unused
			{128, 480, 32, 32},	// Unused
			{160, 480, 32, 32},	// Unused
			{192, 480, 32, 32},	// Unused
			{224, 480, 32, 32},	// Unused
			{128, 736, 32, 32}, // 1-Up box
			{160, 2048, 32, 32}, // Wrench
		},
		0.0f,				  // Strength Reward
		0,					  // Money Reward
		CNM_FALSE,			  // Does Network Interpolation
		CNM_TRUE			  // Can Respawn
	},
	{ // 17: Sword Attack Object
		WobjKnifeAttack_Create,
		WobjGenericAttack_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{64, 320, 32, 32},
			{0, 0, 0, 0} // Null Frame
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 18: Golden Axe Attack Object
		WobjKnifeAttack_Create,
		WobjGenericAttack_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{224, 384, 32, 32},
			{0, 0, 0, 0} // Null Frame
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 19: Ice Rune Object
		WobjIceRune_Create,
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{263, 361, 46, 44}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 20: Air Rune Object
		WobjIceRune_Create,
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{448, 32, 64, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 21: Fire Rune Object
		WobjIceRune_Create,
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{256, 64, 64, 64}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 22: Lightning Rune Object
		WobjIceRune_Create,
		Wobj_TryTeleportArea2, // Update
		WobjLightningRune_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 64, 64, 64},
			{320+64, 64, 64, 64}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 23: Fire Object
		WobjFire_Create, // Create
		WobjFire_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{256, 64, 64, 64}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 24: Ice Attack Object
		WobjIceAttack_Create, // Create
		WobjIceAttack_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{263, 361, 46, 44}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 25: Cloud Platform Object
		WobjCloudPlatform_Create, // Create
		WobjCloudPlatform_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{448, 32, 64, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 26: Lightning Attack Particle Object
		WobjLightningAttackParticle_Create, // Create
		WobjLightningAttackParticle_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 64, 64, 64},
			{320 + 64, 64, 64, 64}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 27: Velocity Based Player Pellet Object
		WobjVelPlayerPellet_Create, // Create
		WobjVelPlayerPellet_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{0, 224, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 28: Player Laser Object
		WobjPlayerLaser_Create, // Create
		WobjPlayerLaser_Update, // Update
		WobjPlayerLaser_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 128, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 29: Rocket Object
		WobjRocket_Create, // Create
		WobjRocket_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{96, 320, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 30: Rocket Explosion Object
		WobjRocketExplosion_Create, // Create
		WobjRocketExplosion_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 896, 192, 192}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 31: Player Minigun Pellet Object
		WobjPlayerMinigunPellet_Create, // Create
		WobjPlayerMinigunPellet_Update, // Update
		WobjPlayerMinigunPellet_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{0, 4064, 64, 32},
			{64, 4064, 64, 32},
			{128, 4064, 64, 32},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 32: Shoes Upgrade Trigger Object
		WobjUpgradeShoes_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{256, 128, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 33: Wings Upgrade Trigger Object
		WobjUpgradeShoes_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 352, 36, 38}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 34: Crystal Wings Upgrade Trigger Object
		WobjUpgradeShoes_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 352, 48, 48}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 35: Flying Slime Object
		WobjFlyingSlime_Create, // Create
		WobjFlyingSlime_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{448, 320, 32, 32},
			{448+32, 320, 32, 32}
		},
		0.00f, // Strength reward
		20, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		100, // Score reward
	},
	{ // 36: Heavy Object
		WobjHeavy_Create, // Create
		WobjHeavy_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{128, 224, 64, 64},
			{128, 288, 64, 64},
			{160, 2432, 64, 64},
			{160, 2432+64, 64, 64},
		},
		0.0f, // Strength reward
		100, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		100, // Score reward
	},
	{ // 37: Heavy Blast Object
		WobjHeavyBlast_Create, // Create
		WobjHeavyBlast_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{32, 224, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 38: Dragon Object
		WobjDragon_Create, // Create
		WobjDragon_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{192, 224, 128, 128},
			{192 + 128, 224, 128, 128},
			{256, 640, 128, 128},
			{256 + 128, 640, 128, 128}
		},
		0.0f, // Strength reward
		1000, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		200, // Score reward
	},
	{ // 39: Fireball Object
		WobjFireball_Create, // Create
		WobjFireball_Update, // Update
		WobjFireball_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{96, 256, 32, 32},
			{96, 256+32, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 40: Bozo Pin Object
		WobjBozoPin_Create, // Create
		WobjBozoPin_Update, // Update
		WobjBozoPin_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{320 + (48 * 0), 1088 + (64 * 0), 48, 64},
			{320 + (48 * 1), 1088 + (64 * 0), 48, 64},
			{320 + (48 * 2), 1088 + (64 * 0), 48, 64},
			{320 + (48 * 3), 1088 + (64 * 0), 48, 64},
			{320 + (48 * 0), 1088 + (64 * 1), 48, 64},
			{320 + (48 * 1), 1088 + (64 * 1), 48, 64},
			{320 + (48 * 2), 1088 + (64 * 1), 48, 64},
			{320 + (48 * 3), 1088 + (64 * 1), 48, 64}
		},
		0.0f, // Strength reward
		750, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		150, // Score reward
	},
	{ // 41: Bozo Object
		WobjBozo_Create, // Create
		WobjBozo_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{448, 64, 64, 128},
			{448, 64+128, 64, 128}
		},
		0.0f, // Strength reward
		10000, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		500, // Score reward
	},
	{ // 42: Sliver Slime Object
		WobjSliverSlime_Create, // Create
		WobjSlime_Update, // Update
		WobjSlime_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{64, 256, 32, 32},
			{64, 256 + 32, 32, 32}
		},
		0.0f, // Strength reward
		100, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 43: Lava Monster Object
		WobjLavaMonster_Create, // Create
		WobjLavaMonster_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{416, 400, 48, 48},
			{416 + 48, 400, 48, 48}
		},
		0.0f, // Strength reward
		5000, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		50, // Score reward
	},
	{ // 44: TT Minion Small Object
		WobjTTMinionSmall_Create, // Create
		WobjTTMinionSmall_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{256, 416, 32, 32}
		},
		0.0f, // Strength reward
		5000, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 45: TT Minion Big Object
		WobjTTMinionBig_Create, // Create
		WobjTTMinionBig_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{288, 416, 32, 64}
		},
		0.0f, // Strength reward
		10000, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		100, // Score reward
	},
	{ // 46: Slime Walker Object
		WobjSlimeWalker_Create, // Create
		WobjSlimeWalker_Update, // Update
		WobjSlimeWalker_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{384, 1376, 64, 64},
			{384+64, 1376, 64, 64},
			{384, 1376+64, 64, 64},
			{384+64, 1376+64, 64, 64}
		},
		0.0f, // Strength reward
		5000, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		100, // Score reward
	},
	{ // 47: Mega Fish Object
		WobjMegaFish_Create, // Create
		WobjMegaFish_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{256, 448, 32, 32}
		},
		0.0f, // Strength reward
		50, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 48: Lava Dragon Head Object
		WobjLavaDragonHead_Create, // Create
		WobjLavaDragonHead_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{0, 2368, 64, 64},
			{64, 2368, 64, 64},
			{128, 2368, 64, 64},
			{192, 2368, 64, 64},
			{256, 2368, 64, 64}
		},
		0.01f, // Strength reward
		12500, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		4900, // Score reward
	},
	{ // 49: Lava Dragon Body Piece Object
		WobjLavaDragonBody_Create, // Create
		NULL, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 2368, 64, 64},
			{384, 2368, 64, 64},
			{448, 2368, 32, 32},
			{480, 2368, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 50: Lava Dragon Lava Blob/Spit Object
		WobjFireball_Create, // Create
		WobjFireball_Update, // Update
		WobjFireball_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{448, 2400, 32, 32},
			{480, 2400, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 51: Talking Teddy Chase Trigger Object
		WobjTTBossTriggerGeneric_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 1248, 32, 32},
			{256, 480, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 52: Talking Teddy Normal Trigger Object
		WobjTTBossTriggerGeneric_Create, // Create
		WobjTTBossTriggerNormal_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 1280, 32, 32},
			{224, 640, 32, 32},
			{0, 0, 0, 0}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 53: Talking Teddy Waypoint Object
		WobjTTBossWaypoint_Create, // Create
		NULL, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 1312, 32, 32},
			{0, 0, 0, 0}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 54: Talking Teddy Boss Object
		WobjTTBoss_Create, // Create
		WobjTTBoss_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{384, 416, 32, 32}
		},
		0.25f, // Strength reward
		500000, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 55: Eater Bug Object
		WobjEaterBug_Create, // Create
		WobjEaterBug_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{352, 1408, 32, 96}
		},
		0.0f, // Strength reward
		100, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 56: Player's Flamethrower Flame Object
		WobjFlameThrowerFlame_Create, // Create
		WobjFlameThrowerFlame_Update, // Update
		WobjFireball_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{96, 256, 32, 32},
			{96, 256 + 32, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 57: Spider Walker Object
		WobjSpiderWalker_Create, // Create
		WobjSpiderWalker_Update, // Update
		WobjFireball_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{288, 1472, 32, 32},
			{320, 1472, 32, 32}
		},
		0.0f, // Strength reward
		500, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 58: Spider Walker Web Object
		WobjSpiderWalkerWeb_Create, // Create
		WobjSpiderWalkerWeb_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 2048, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 59: Spike Trap Object
		WobjSpikeTrap_Create, // Create
		WobjSpikeTrap_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{224, 1536, 32, 32},
			{256, 1536, 32, 32},
			{288, 1536, 32, 32},
			{0, 0, 0, 0}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 60: Rotating Fire Column Piece Object
		WobjRotatingFireColunmPiece_Create, // Create
		WobjRotatingFireColunmPiece_Update, // Update
		WobjRotatingFireColunmPiece_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{416, 2048, 32, 32},
			{96, 256, 32, 32},
			{96, 256 + 32, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 61: Moving Fire Vertical Object
		WobjMovingFireVertical_Create, // Create
		WobjMovingFireVertical_Update, // Update
		WobjRotatingFireColunmPiece_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{480, 2048, 32, 32},
			{96, 256, 32, 32},
			{96, 256 + 32, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 62: Moving Fire Horizontal Object
		WobjMovingFireHorizontal_Create, // Create
		WobjMovingFireHorizontal_Update, // Update
		WobjRotatingFireColunmPiece_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{448, 2048, 32, 32},
			{96, 256, 32, 32},
			{96, 256 + 32, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 63: Super Dragon Boss Object
		WobjSuperDragonBoss_Create, // Create
		WobjSuperDragonBoss_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{384, 1664, 128, 128}
		},
		0.01f, // Strength reward
		10000, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		900, // Score reward
	},
	{ // 64: Super Dragon Boss Landing Zone Object
		WobjSuperDragonLandingZone_Create, // Create
		NULL, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 2080, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 65: Deephouse Boots Blast Object
		WobjPlayerPellet_Create, // Create
		WobjPlayerPellet_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{416, 2080, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 66: Ultra Sword Boomerang Object
		WobjUltraSwordBoomerang_Create, // Create
		WobjUltraSwordBoomerang_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 2432, 32, 32},
			{416, 2432, 32, 32},
			{448, 2432, 32, 32},
			{384, 2464, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 67: Player's Heavy Hammer Swing Attack Object
		WobjHeavyHammerSwing_Create, // Create
		WobjHeavyHammerSwing_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{0, 2512, 32, 32},
			{64, 2512, 32, 32},
			{96, 2512, 32, 32},
			{0, 2512+32, 32, 32},
			{32, 2512+32, 32, 32},
			{64, 2512+32, 32, 32},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 68: Player Heavy Blast Object
		WobjPlayerPellet_Create, // Create
		WobjPlayerPellet_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{416, 2080, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 69: Bozo Laser Minion Object
		WobjBozoLaserMinion_Create, // Create
		WobjBozoLaserMinion_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{384, 2528, 32, 64}
		},
		0.0f, // Strength reward
		20000, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		300, // Score reward
	},
	{ // 70: Bozo Laser Lock-On Target Object
		WobjBozoLaserLockon_Create, // Create
		WobjBozoLaserLockon_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{416, 2528, 32, 32},
			{448, 2528, 32, 32},
			{416, 2560, 32, 32},
			{448, 2560, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 71: Bozo Laser Part Object
		WobjBozoLaserPart_Create, // Create
		WobjBozoLaserPart_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{328, 136, 16, 16}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 72: Bozo Mk. II Object
		WobjBozoMk2_Create, // Create
		WobjBozoMk2_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{224, 2432, 48, 64},
			{224+48, 2432, 48, 64},
			{224+48*2, 2432, 48, 64},
			{224, 2432+64, 48, 64},
			{224+48, 2432+64, 48, 64},
		},
		0.05f, // Strength reward
		100000, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		4900, // Score reward
	},
	{ // 73: Checkpoint Object
		WobjCheckpoint_Create, // Create
		NULL, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 2592, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 74: Spike Guy Object
		WobjSpikeGuy_Create, // Create
		WobjSpikeGuy_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{384, 2208, 32, 32}
		},
		0.0f, // Strength reward
		100, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		100, // Score reward
	},
	{ // 75: Spike Guy Spike Attack Object
		WobjSpikeGuySpike_Create, // Create
		WobjSpikeGuySpike_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384 + (32 * 0), 1984+ (32 * 0), 32, 32},
			{384 + (32 * 1), 1984+ (32 * 0), 32, 32},
			{384 + (32 * 2), 1984+ (32 * 0), 32, 32},
			{384 + (32 * 3), 1984+ (32 * 0), 32, 32},
			{384 + (32 * 0), 1984+ (32 * 1), 32, 32},
			{384 + (32 * 1), 1984+ (32 * 1), 32, 32},
			{384 + (32 * 2), 1984+ (32 * 1), 32, 32},
			{384 + (32 * 3), 1984+ (32 * 1), 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 76: Bandit Guy Object
		WobjBanditGuy_Create, // Create
		WobjBanditGuy_Update, // Update
		WobjFireball_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{416, 2208, 32, 32},
			{448, 2208, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 77: Horizontal Push Zone Object
		WobjHorizontalPushZone_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 1856, 128, 128}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 78: Vertical Push Zone Object
		WobjVerticalPushZone_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 1792, 64, 64}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 79: Vertical Wind Zone Object
		WobjVerticalPushZone_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384+64, 1792, 64, 64}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 80: Small Horizontal Push Zone Object
		WobjSmallHorizontalPushZone_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{416, 2592, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 81: Fission Gun Energy Bolt Object
		WobjFissionEnergyBolt_Create, // Create
		WobjFissionEnergyBolt_Update, // Update
		WobjFireball_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{416, 2492, 32, 32},
			{448, 2492, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 82: Moving Platform Vertical Object
		WobjMovingPlatformVertical_Create, // Create
		WobjMovingPlatformVertical_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{288, 160, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 83: Dissapearing Platform Object
		WobjDissapearingPlatform_Create, // Create
		WobjDissapearingPlatform_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{352, 2112, 32, 32},
			{352, 2112+32, 32, 32},
			{0, 0, 0, 0}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 84: Kamakazi Slime Object
		WobjKamakaziSlime_Create, // Create
		WobjKamakaziSlime_Update, // Update
		WobjKamakaziSlime_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{448, 2080, 32, 32},
			{480, 2080, 32, 32},
		},
		0.0f, // Strength reward
		200, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 85: Kamakazi Slime Explosion Object
		WobjKamakaziSlimeExplosion_Create, // Create
		WobjKamakaziSlimeExplosion_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 896, 192, 192}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 86: Spring Board Object
		WobjSpringBoard_Create, // Create
		WobjSpringBoard_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{448, 0, 32, 32},
			{480, 0, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 87: Horizontal Background Switch Object
		WobjHorizontalBackgroundSwitch_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 2816, 128, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 88: Vertical Background Switch Object
		WobjVerticalBackgroundSwitch_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 2848, 32, 96}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 89: Very Big Tunes Trigger Object
		WobjVeryBigTunesTrigger_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{416, 2848, 96, 96}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 90: Small Jumpthough Platform (Invisible) Object
		WobjJumpthrough_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 2944, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 91: Big Jumpthough Platform (Invisible) Object
		WobjBigJumpthrough_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{416, 2944, 96, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 92: Breakable/Crumbling Platform Object
		WobjBreakablePlatform_Create, // Create
		WobjBreakablePlatform_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{256, 1184, 32, 32},
			{256+32, 1184, 32, 32},
			{256, 1184+32, 32, 32},
			{256+32, 1184+32, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 93: Breakable Wall With Skin Selection Object
		WobjSkinBreakableWall_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{288, 1312, 32, 32},
			{256, 1248, 32, 32},
			{288, 1248, 32, 32},
			{256, 1280, 32, 32},
			{288, 1280, 32, 32},
			{256, 1312, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 94: Locked Red Block Object
		WobjLockedBlockGeneric_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{288, 1120, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 95: Locked Green Block Object
		WobjLockedBlockGeneric_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{288, 1152, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 96: Locked Blue Block Object
		WobjLockedBlockGeneric_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{288, 1184, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 97: Medium Rock Guy Enemy Object
		WobjRockGuyMedium_Create, // Create
		WobjRockGuyMedium_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{384, 3040, 32, 64},
			{416, 3040, 32, 64}
		},
		0.0f, // Strength reward
		500, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		100, // Score reward
	},
	{ // 98: Small Rock Guy (1) Enemy Object
		WobjRockGuySmall1_Create, // Create
		WobjRockGuySmall1_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{390, 3117, 22, 19}
		},
		0.0f, // Strength reward
		10, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 99: Small Rock Guy (2) Enemy Object
		WobjRockGuySmall2_Create, // Create
		WobjRockGuySmall2_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{425, 3122, 14, 14}
		},
		0.0f, // Strength reward
		10, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 100: Slider Rock Guy Enemy Object
		WobjRockGuySlider_Create, // Create
		WobjRockGuySlider_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{384, 3136, 64, 32},
			{384, 3168, 64, 32},
			{448, 3168, 64, 32}
		},
		0.0f, // Strength reward
		750, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		200, // Score reward
	},
	{ // 101: Smasher Rock Guy Enemy Object
		WobjRockGuySmasher_Create, // Create
		WobjRockGuySmasher_Update, // Update
		WobjGeneric_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{384, 3392, 32, 96},
			{416, 3392, 64, 96},
			{384, 3488, 96, 64},
			{384, 3552, 96, 32},
		},
		0.0f, // Strength reward
		1500, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE, // Can respawn?
		400, // Score reward
	},
	{ // 102: Spear from a medium Rock Guy Enemy Object
		WobjRockGuySpear_Create, // Create
		WobjRockGuySpear_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{448 + (32 * 1), 3040 + (32 * 3), 32, 32},
			{448 + (32 * 1), 3040 + (32 * 2), 32, 32},
			{448 + (32 * 1), 3040 + (32 * 1), 32, 32},
			{448 + (32 * 1), 3040 + (32 * 0), 32, 32},

			{448 + (32 * 0), 3040 + (32 * 3), 32, 32},
			{448 + (32 * 0), 3040 + (32 * 2), 32, 32},
			{448 + (32 * 0), 3040 + (32 * 1), 32, 32},
			{448 + (32 * 0), 3040 + (32 * 0), 32, 32},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 103: Water Splash Effect Object
		WobjWaterSplashEffect_Create, // Create
		WobjWaterSplashEffect_Update, // Update
		WobjWaterSplashEffect_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{288, 1088+16, 32, 16},
			{256, 1088+16, 32, 16},
			{256, 1088+32, 32, 32},
			{256, 1088+64, 32, 32},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 104: Health Set Trigger Object
		WobjHealthSetTrigger_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{448, 3968, 64, 96}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 105: Vortex Object
		WobjVortex_Create, // Create
		WobjVortex_Update, // Update
		WobjFireball_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{256, 3968, 96, 96},
			{256+96, 3968, 96, 96},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 106: Vortex Machine Upgrade Trigger Object
		WobjUpgradeShoes_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{96, 224, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 107: Customizeable Moving Platform Object
		WobjCustomizableMovingPlatform_Create, // Create
		WobjCustomizableMovingPlatform_Update, // Update
		WobjCustomizableMovingPlatform_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{320, 1344, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 108: Dialoge Box Trigger Object
		WobjEndingTextTrigger_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{288, 1344, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 109: Graphics Change Trigger Object
		WobjSmallTunesTrigger_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{256, 1344, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 110: Teleport Particles From a Player Object
		WobjTeleParticles_Create, // Create
		WobjTeleParticles_Update, // Update
		WobjTeleParticles_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{224, 1088, 16, 16},
			{224+16, 1088, 16, 16},
			{224, 1088+16, 16, 16},
			{224+16, 1088+16, 16, 16},
			{224+32, 1088, 16, 16},
			{224+48, 1088, 16, 16},
			{224 + 64, 1088, 16, 16},
			{224 + 80, 1088, 16, 16},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 111: Dust Particle Object
		WobjDustParticle_Create, // Create
		WobjDustParticle_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{192, 1088, 16, 16},
			{192+16, 1088, 16, 16},
			{192, 1088+16, 16, 16},
			{192+16, 1088+16, 16, 16}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 112: Blood Particle Object
		WobjBloodParticle_Create, // Create
		WobjBloodParticle_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{-128+ 256, 64+ 1888, 16, 16},
			{-128+ 256 + 16, 64+ 1888, 16, 16},
			{-128+ 256, 64+ 1888 + 16, 16, 16},
			{-128+ 256 + 16, 64+ 1888 + 16, 16, 16}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 113: Gib Particle Object
		WobjGibParticle_Create, // Create
		WobjGibParticle_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{-128+ 288, 64+ 1888, 16, 16},
			{-128+ 288 + 16, 64+ 1888, 16, 16},
			{-128+ 288, 64+ 1888 + 16, 16, 16},
			{-128+ 288 + 16, 64+ 1888 + 16, 16, 16}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 114: MAX POWER Rune Object
		WobjIceRune_Create,
		Wobj_TryTeleportArea2, // Update
		WobjLightningRune_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{192, 1120, 48, 48},
			{192, 1120+48, 48, 48}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 115: Boss Bar Info Object
		WobjBossBarInfo_Create,
		WobjBossBarInfo_Update, // Update
		WobjBossBarInfo_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 1344, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
			//WOBJ_BGSPEED_X,
			//WOBJ_BGSPEED_Y,
			//WOBJ_BGTRANS,
			//WOBJ_TELETRIGGER1,
			//WOBJ_TELEAREA1,
	{ // 116: Background Speed X Change Trigger Object
		Wobj_Trigger32x32_Create,
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{480, 3392, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 117: Background Speed Y Change Trigger Object
		Wobj_Trigger32x32_Create,
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{480, 3392+32, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 118: Background Transparency Change Trigger Object
		Wobj_Trigger32x32_Create,
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{480, 3392+64, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 119: Teleport Area Trigger Object
		Wobj_TeleTrigger1_Create,
		Wobj_TeleTrigger1_Update, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{480, 3392+96, 32, 96}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 120: Teleport Area Object
		Wobj_TeleArea1_Create,
		Wobj_TeleArea1_Update, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 4224, 128, 128}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 121: Sound Effect Point Object
		NULL,
		Wobj_TryTeleportArea2, // Update
		Wobj_SfxPoint_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{480, 3360, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 122: Wolf Enemy Object
		WobjWolf_Create, // Create
		WobjSlimeWalker_Update, // Update
		WobjSlimeWalker_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{352,      4480,      64, 32},
			{352 + 64, 4480,      64, 32},
			{352,      4480 + 32, 64, 32},
			{352 + 64, 4480 + 32, 64, 32}
		},
		0.0f, // Strength reward
		500, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	{ // 123: Supervirus Object
		WobjSupervirus_Create, // Create
		WobjSupervirus_Update, // Update
		WobjSupervirus_Draw, // Draw
		WobjGeneric_Hurt, // Hurt callback
		{ // Animation Frames
			{0,      0,      0, 0}
		},
		100.0f, // Strength reward
		1000000000, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_TRUE // Can respawn?
	},
	LUAOBJ_DEF // 124: Custom Lua (Type 0) Object
	LUAOBJ_DEF // 125: Custom Lua (Type 1) Object
	LUAOBJ_DEF // 126: Custom Lua (Type 2) Object
	LUAOBJ_DEF // 127: Custom Lua (Type 3) Object
	LUAOBJ_DEF // 128: Custom Lua (Type 4) Object
	LUAOBJ_DEF // 129: Custom Lua (Type 5) Object
	LUAOBJ_DEF // 130: Custom Lua (Type 6) Object
	LUAOBJ_DEF // 131: Custom Lua (Type 7) Object
	LUAOBJ_DEF // 132: Custom Lua (Type 8) Object
	LUAOBJ_DEF // 133: Custom Lua (Type 9) Object
	LUAOBJ_DEF // 134: Custom Lua (Type 10) Object
	LUAOBJ_DEF // 135: Custom Lua (Type 11) Object
	LUAOBJ_DEF // 136: Custom Lua (Type 12) Object
	LUAOBJ_DEF // 137: Custom Lua (Type 13) Object
	LUAOBJ_DEF // 138: Custom Lua (Type 14) Object
	LUAOBJ_DEF // 139: Custom Lua (Type 15) Object
	{ // 140: No Upgrades Trigger Object
		Wobj_Trigger32x32_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{480, 3936, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 141: Finishing Trigger Object
		Wobj_Trigger32x32_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{352, 2992, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 142: Shotgun Pelletes Object
		Wobj_ShotgunPel_Create, // Create
		Wobj_ShotgunPel_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{288, 32, 6, 6},
			{304, 32, 9, 4}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 143: Gravity Trigger Object
		Wobj_Trigger32x32_Create, // Create
		Wobj_TryTeleportArea2, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{480, 1344, 32, 32},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 144: WOBJ_HEAVY_SHEILD_BOX
		WobjHeavySheildBox_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjHeavySheildBox_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{0, 0, 32, 32},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 145: Enemy Rockets
		WobjEnemyRocket_Create, // Create
		WobjEnemyRocket_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{96, 320, 32, 32},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 146: Bozo Waypoint
		WobjBozoWaypoint_Create, // Create
		NULL, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{384, 2528, 32, 32},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 147: Bozo Fireball
		WobjBozoFireball_Create, // Create
		WobjBozoFireball_Update, // Update
		WobjFireball_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{96, 256, 32, 32},
			{96, 256+32, 32, 32}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_TRUE, // Does network interpolation?
		CNM_FALSE // Can respawn?
	},
	{ // 148: Skin Unlock
		WobjSkinUnlock_Create, // Create
		Wobj_TryTeleportArea2, // Update
		WobjSkinUnlock_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{128, 1824, 32, 32},
			{256, 1824, 32, 32},
			{256, 1920-32, 32, 32},
			{256, 3392, 32, 32},
			{128, 1984, 32, 32},
			{0, 1280, 32, 32},
			{128, 1280, 32, 32},
			{0, 768, 32, 32},
			{128, 768, 32, 32},
			{5, 4616, 32, 32},
			{0, 7776, 32, 32},
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE, // Can respawn?
		0, // Score reward
	},
	{ // 149: Cool Dissaparing Platform Object
		WobjCoolPlatform_Create, // Create
		WobjCoolPlatform_Update, // Update
		WobjGeneric_Draw, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{352, 2112, 32, 32},
			{352, 2112+32, 32, 32},
			{0, 0, 0, 0}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE, // Can respawn?
		0, // Score reward
	},
	{ // 150: Teleport Area 2 Object
		Wobj_TeleArea2_Create,
		Wobj_TeleArea2_Update, // Update
		NULL, // Draw
		NULL, // Hurt callback
		{ // Animation Frames
			{480, 3392+96, 32, 96}
		},
		0.0f, // Strength reward
		0, // Money reward
		CNM_FALSE, // Does network interpolation?
		CNM_FALSE, // Can respawn?
		0, // Score reward
	},
};
