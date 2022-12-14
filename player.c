#include <string.h>
#include <math.h>
#include <stdio.h>
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
#include "enemies.h"
#include "command.h"
#include "filesystem.h"
#include "camera.h"
#include "fadeout.h"

extern PLAYER_MAXPOWER_INFO maxpowerinfos[32] = {0};

static int *anim_lengths;

static int anim_lengths1[PLAYER_ANIM_MAX] = 
{
	1, // standing
	4, // walking
	1, // jumping
	1, // shooting
	1, // melee
	1, // hurt
};
static int anim_lengths2[PLAYER_ANIM_MAX] =
{
	6, // standing
	9, // walking
	10, // jumping
	2, // jumping1
	2, // jumping2
	1, // hurt
};
static int anim_offsets[PLAYER_ANIM_MAX][6][2] = 
{
	{ // standing
		{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	},
	{ // walking
		{32, 0}, {64, 0}, {96, 0}, {96, 32}, {0, 0}, {0, 0},
	},
	{ // jumping
		{32, 32}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	},
	{ // shooting
		{64, 32}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	},
	{ // melee
		{96, 32}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	},
	{ // hurt
		{0, 32}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	}
};
extern int skin_bases[10][2] =
{
	{128, 1824},
	{256, 1824},
	{256, 1920-32},
	{256, 3392},
	{128, 1984},
	{0, 1280},
	{128, 1280},
	{0, 768},
	{128, 768},
	{5, 4616}
};

static void PlayerAnimGetRect(CNM_RECT *r, int skin, int anim, int frame);
static void StepPlayerAnimation(WOBJ *player);
static void DrawPlayerChar(WOBJ *wobj, int camx, int camy);

static void GetVortexPos(WOBJ *player, float ang_around_vortex, float *x, float *y)
{
	PLAYER_LOCAL_DATA *local_data = player->local_data;
	*x = local_data->vortex_x + cosf(ang_around_vortex) * 32.0f;
	*y = local_data->vortex_y + sinf(ang_around_vortex) * 32.0f;
}

static void Normalize(float *x, float *y)
{
	float dist = sqrtf((*x)*(*x) + (*y)*(*y));
	*x /= dist;
	*y /= dist;
}

void WobjPlayer_Create(WOBJ *wobj)
{
	PLAYER_LOCAL_DATA *local_data;
	wobj->local_data = malloc(sizeof(PLAYER_LOCAL_DATA));
	memset(wobj->local_data, 0, sizeof(PLAYER_LOCAL_DATA));
	local_data = wobj->local_data;
	local_data->beastchurger_music = 0;
	local_data->checkpoint = -1;
	local_data->jumped = 0;
	local_data->last_in_water = CNM_FALSE;
	local_data->in_water = CNM_FALSE;
	local_data->vortexed_mode = CNM_FALSE;
	local_data->created_vortexes_id = 0;
	local_data->in_teleport = -32;
	local_data->death_cam_timer = -1;
	memset(local_data->created_vortexes_node, 0xff, sizeof(int)*PLAYER_MAX_VORTEXES);
	memset(local_data->created_vortexes_uuid, 0xff, sizeof(int)*PLAYER_MAX_VORTEXES);

	wobj->flags = WOBJ_IS_PLAYER;
	wobj->health = 100.0f;
	wobj->item = 0;
	wobj->money = 100;
	wobj->vel_x = 0.0f;
	wobj->strength = 0.05f;
	wobj->vel_y = 0.0f;
	wobj->speed = 5.0f;
	wobj->jump = 10.0f;
	wobj->hitbox.x = 6.0f;
	wobj->hitbox.y = 2.0f;
	wobj->hitbox.w = 17.0f;
	wobj->hitbox.h = 29.0f;
	wobj->custom_floats[1] = 0.0f;
	wobj->custom_ints[1] = 0; // Player Flags
	local_data->uswords_have = 4;
	local_data->death_limbo_counter = 0;

	local_data->currskin = wobj->custom_ints[0];
	local_data->curranim = PLAYER_ANIM_STANDING;
	local_data->currframe = 0;
	local_data->animspd = 0;
	if (wobj->internal.owned)
	{
		if (local_data->currskin != 9)
			anim_lengths = anim_lengths1;
		else
			anim_lengths = anim_lengths2;
	}

	local_data->upgrade_state = PLAYER_UPGRADE_NONE;
	local_data->fire_resistance = 0;

	PlayerSpawn_SetWobjLoc(&wobj->x);
}
void WobjPlayer_Update(WOBJ *wobj)
{
	PLAYER_LOCAL_DATA *local_data = wobj->local_data;

	if (Game_GetVar(GAME_VAR_NOCLIP)->data.integer)
	{
		float spd = wobj->speed * 2.0f;
		if (Input_GetButton(INPUT_DROP, INPUT_STATE_PLAYING))
			spd *= 3.0f;
		if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING))
			wobj->x += spd;
		if (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING))
			wobj->y += spd;
		if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING))
			wobj->x -= spd;
		if (Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING))
			wobj->y -= spd;
		return;
	}
	
	local_data->pvp_damage_indicator_timer--;
	if (local_data->death_cam_timer-- == 0)
	{
		Camera_SetForced(CNM_FALSE);
		Camera_TeleportPos((int)wobj->x + 16, (int)wobj->y + 16);
		wobj->flags &= ~WOBJ_INVULN;
	}
	local_data->in_teleport--;
	if (local_data->in_teleport == 0)
	{
		Camera_SetForced(CNM_FALSE);
		Camera_TeleportPos((int)wobj->x + 16, (int)wobj->y + 16);
		Interaction_CreateWobj(WOBJ_TELE_PARTICLES, wobj->x, wobj->y, 0, 0.0f);
	}
	if (local_data->in_teleport > -8) {
		wobj->vel_x = 0.0f;
		wobj->vel_y = 0.0f;
		return;
	}
	local_data->death_limbo_counter--;
	if (local_data->death_limbo_counter > 0) {
		wobj->flags |= WOBJ_DONT_DRAW;
		wobj->flags |= WOBJ_PLAYER_IS_RESPAWNING;
		wobj->health = 100.0f;
		return;
	}
	wobj->flags &= ~WOBJ_PLAYER_IS_RESPAWNING;
	if (local_data->death_limbo_counter == 0) {
		Interaction_CreateWobj(WOBJ_TELE_PARTICLES, wobj->x, wobj->y, 0, 0.0f);
		wobj->flags &= ~WOBJ_DONT_DRAW;
	}

	float accel = 0.4f;
	float dec = 0.2f;
	WOBJ *other;
	float final_speed;
	float final_grav;
	float final_jmp;
	//float final_strength;

	final_speed = wobj->speed;
	final_jmp = wobj->jump;
	final_grav = Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
	if (local_data->upgrade_state == PLAYER_UPGRADE_MAXPOWER)
	{
		final_speed *= maxpowerinfos[local_data->mpoverride].spd;
		final_jmp *= maxpowerinfos[local_data->mpoverride].jmp;
		final_grav *= maxpowerinfos[local_data->mpoverride].grav;
	}

	if (local_data->animforce_cooldown_timer-- <= 0)
		local_data->animforce_cooldown = CNM_FALSE;

	// Water things
	if (!local_data->vortexed_mode)
	{
		local_data->last_in_water = local_data->in_water;
		BLOCK_PROPS *water_props = Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_BG, (int)(wobj->x + 16.0f) / BLOCK_SIZE, (int)(wobj->y + 16.0f) / BLOCK_SIZE));
		local_data->in_water = water_props->dmg_type == BLOCK_DMG_TYPE_LAVA;
		if (!local_data->in_water)
		{
			water_props = Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_FG, (int)(wobj->x + 16.0f) / BLOCK_SIZE, (int)(wobj->y + 16.0f) / BLOCK_SIZE));
			local_data->in_water = water_props->dmg_type == BLOCK_DMG_TYPE_LAVA;
		}

		if (local_data->in_water)
		{
			accel /= 2.0f;
			dec /= 2.0f;
		}
		if (local_data->in_water != local_data->last_in_water)
			Interaction_CreateWobj(WOBJ_WATER_SPLASH_EFFECT, wobj->x, wobj->y - 16.0f, 0, 0.0f);

		if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING) && wobj->vel_x < final_speed)
		{
			if (Input_GetButtonReleased(INPUT_LEFT, INPUT_STATE_PLAYING) ||
				Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING))
				wobj->vel_x = final_speed;

			if (wobj->vel_x <= accel && wobj->vel_x >= -accel)
				wobj->vel_x = final_speed;
			else
				wobj->vel_x += accel;

			if (wobj->vel_x < 0.0f)
				wobj->vel_x += accel * 2.0f;
			wobj->flags &= ~WOBJ_HFLIP;
		}
		if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING) && wobj->vel_x > -final_speed)
		{
			if (Input_GetButtonReleased(INPUT_RIGHT, INPUT_STATE_PLAYING) ||
				Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING))
				wobj->vel_x = -final_speed;

			if (wobj->vel_x <= accel && wobj->vel_x >= -accel)
				wobj->vel_x = -final_speed;
			else
				wobj->vel_x -= accel;

			if (wobj->vel_x > 0.0f)
				wobj->vel_x -= accel * 2.0f;
			wobj->flags |= WOBJ_HFLIP;
		}

		if (local_data->currskin == 9 && local_data->curranim == PLAYER_ANIM_JUMP && Wobj_IsGrouneded(wobj))
		{
			local_data->curranim = PLAYER_ANIM_JUMP_END;
			local_data->currframe = 0;
			local_data->animspd = 3;
			local_data->animtimer = 0;
		}

		if ((Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING) || Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING)) &&
			!local_data->animforce_cooldown && Wobj_IsGrouneded(wobj))
		{
			if (local_data->curranim != PLAYER_ANIM_JUMP_END)
			{
				local_data->curranim = PLAYER_ANIM_WALKING;
				local_data->animspd = 8 - (int)fabsf(wobj->vel_x);
				if (local_data->animspd < 0)
					local_data->animspd = 0;
			}
		}
	
		if ((Input_GetButtonReleased(INPUT_RIGHT, INPUT_STATE_PLAYING) ||
			Input_GetButtonReleased(INPUT_LEFT, INPUT_STATE_PLAYING)) &&
			(!Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING) &&
			 !Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING)))
			wobj->vel_x = 0.0f;

		if (!(Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING) ||
			  Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING)))
		{
			if (wobj->vel_x > dec)
				wobj->vel_x -= dec;
			else if (wobj->vel_x < -dec)
				wobj->vel_x += dec;
			else
				wobj->vel_x = 0.0f;
			if (!local_data->animforce_cooldown && Wobj_IsGrouneded(wobj)&&
				local_data->curranim != PLAYER_ANIM_JUMP_END)
			{
				local_data->curranim = PLAYER_ANIM_STANDING;
				local_data->animspd = 3;
			}
		}

		if (local_data->upgrade_state == PLAYER_UPGRADE_SHOES)
		{
			if (!Wobj_IsGrouneded(wobj))
			{
				if (!(wobj->custom_ints[1] & PLAYER_FLAG_USED_DOUBLE_JUMP) && Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING))
				{
					for (int i = 0; i < 32 && !Wobj_IsCollidingWithBlocks(wobj, 0.0f, 32.0f); i++)
						wobj->y += 24.0f;
					for (int i = 0; i < 32 && !Wobj_IsCollidingWithBlocks(wobj, 0.0f, 4.0f); i++)
						wobj->y += 1.0f;

					WOBJ *w = Interaction_CreateWobj(DEEPHOUSE_BOOT_BLAST,
													 wobj->x + 16.0f, wobj->y, 0, 1.0f);
					w->speed = 9.0f;
					w->strength = wobj->strength + 0.1f;
					w = Interaction_CreateWobj(DEEPHOUSE_BOOT_BLAST,
													 wobj->x + 16.0f, wobj->y, 0, -1.0f);
					w->speed = 9.0f;
					w->strength = wobj->strength + 0.1f;

					wobj->vel_y = 10.0f;
					wobj->custom_ints[1] |= PLAYER_FLAG_USED_DOUBLE_JUMP;
					Interaction_PlaySound(wobj, 25);
				}
			}
			else
			{
				wobj->custom_ints[1] &= ~PLAYER_FLAG_USED_DOUBLE_JUMP;
			}
		}
		else if (local_data->upgrade_state == PLAYER_UPGRADE_WINGS || (
				local_data->upgrade_state == PLAYER_UPGRADE_MAXPOWER &&
				maxpowerinfos[local_data->mpoverride].ability == 2
				))
		{
			if (!Wobj_IsGrouneded(wobj) && Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING))
			{
				if (wobj->vel_y > -3.0f)
					wobj->vel_y = -3.0f;
			}
		}
		else if (local_data->upgrade_state == PLAYER_UPGRADE_CRYSTAL_WINGS)
		{
			if (!Wobj_IsGrouneded(wobj) && Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING))
			{
				if (fabsf(wobj->vel_x) < 15.0f)
					wobj->vel_x *= 1.1f;

				if (wobj->vel_y > -15.0f)
					wobj->vel_y -= final_grav * 2.0f;
			}
		}
		else if (local_data->upgrade_state == PLAYER_UPGRADE_VORTEX)
		{
			if (!Wobj_IsGrouneded(wobj) && Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING))
			{
				int do_attract = CNM_FALSE;
				WOBJ *old_vortex = Wobj_GetAnyWOBJFromUUIDAndNode
				(
					local_data->created_vortexes_node[local_data->created_vortexes_id],
					local_data->created_vortexes_uuid[local_data->created_vortexes_id]
				);
				if (old_vortex != NULL)
					Interaction_DestroyWobj(old_vortex);

				float vx = wobj->x;
				float vy = wobj->y;
				static const float vdd = 32.0f;
				if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING))
					vx += vdd;
				else if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING))
					vx -= vdd + 96.0f;
				else
				{
					vy -= vdd;
					do_attract = CNM_TRUE;
				}

				WOBJ *new_vortex = Interaction_CreateWobj(WOBJ_VORTEX, vx, vy, do_attract, 0.0f);
				local_data->created_vortexes_node[local_data->created_vortexes_id] = new_vortex->node_id;
				local_data->created_vortexes_uuid[local_data->created_vortexes_id] = new_vortex->uuid;
				//new_vortex->custom_ints[1] = 30*10;
				//if (vx != 0.0f)
				//	new_vortex->custom_ints[0] = CNM_FALSE;
				local_data->created_vortexes_id = (local_data->created_vortexes_id + 1) % PLAYER_MAX_VORTEXES;
			}
		}

		if (wobj->vel_y >= 15.5f)
			wobj->vel_y = 15.5f;
	}
	if (!local_data->vortexed_mode)
		WobjPhysics_BeginUpdate(wobj);

	//float stored_movestand_yvel = 0.0f;

	//wobj->y += 1.0f;
	//other = Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID);
	local_data->jumped--;
	if (!local_data->vortexed_mode)
	{
		if (Wobj_IsGrouneded(wobj) || (local_data->upgrade_state == PLAYER_UPGRADE_NONE && local_data->in_water))//Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f) || other != NULL)
		{
			//local_data->has_hammer_jumped = CNM_FALSE;
			if (Wobj_IsGrouneded(wobj))
			{
				local_data->jumped = 0;
				wobj->vel_y = 0.0f;
			}
			if (Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING))
			{
				float jmp_speed = final_jmp;
				if (local_data->in_water)
					jmp_speed /= 1.5f;
				wobj->vel_y = -jmp_speed;
				local_data->jumped = 5;
				local_data->animtimer = 0;
			}
		}
		// Max power double jumping
		if (!Wobj_IsGrouneded(wobj))
		{
			if (Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING) &&
				local_data->upgrade_state == PLAYER_UPGRADE_MAXPOWER &&
				maxpowerinfos[local_data->mpoverride].ability == 1 &&
				!(wobj->custom_ints[1] & PLAYER_FLAG_USED_DOUBLE_JUMP))
			{
				float jmp_speed = final_jmp / 1.5f;
				if (local_data->in_water)
					jmp_speed /= 1.5f;
				wobj->vel_y = -jmp_speed;
				local_data->jumped = 0;
				wobj->custom_ints[1] |= PLAYER_FLAG_USED_DOUBLE_JUMP;
			}

			if (Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING) &&
				local_data->upgrade_state == PLAYER_UPGRADE_MAXPOWER &&
				maxpowerinfos[local_data->mpoverride].ability == 3 &&
				!local_data->ability3_hasjumped)
			{
				local_data->ability3_hasjumped = 1;
				local_data->ability3_timer = 1;
			}

			if (local_data->in_water)
				wobj->vel_y += final_grav / 2.0f;
			else
				wobj->vel_y += final_grav;
		}
		if (local_data->ability3_hasjumped) {
			local_data->ability3_timer++;
		}
		if (Wobj_IsGrouneded(wobj))
		{
			wobj->custom_ints[1] &= ~PLAYER_FLAG_USED_DOUBLE_JUMP;
			if (local_data->ability3_hasjumped) {
				local_data->ability3_hasjumped = 0;
				//local_data->ability3_timer = 30;
			}
		}
		if (local_data->jumped > 0 && !Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING) && wobj->vel_y < (final_jmp / -2.0f))
		{
			wobj->vel_y = final_jmp / -2.0f;
		}
	}
	//wobj->y -= 1.0f;

	other = Wobj_GetWobjColliding(wobj, WOBJ_IS_HOSTILE);
	if (other != NULL && local_data->upgrade_state == PLAYER_UPGRADE_MAXPOWER && maxpowerinfos[local_data->mpoverride].ability == 4) {
		wobj->vel_y = -final_jmp;
	}

	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_SMALL_TUNES_TRIGGER);
	if (other == NULL)
		other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_BIG_TUNES_TRIGGER);
	if (other == NULL)
		other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_VERYBIG_TUNES_TRIGGER);
	if (other != NULL)
	{
		if (local_data->beastchurger_timer > 0)
			local_data->beastchurger_music = other->custom_ints[0];
		else
			Audio_PlayMusic(other->custom_ints[0], CNM_TRUE);
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_ENDING_TEXT_SPAWNER);
	if (other != NULL)
		EndingText_Start(other->custom_ints[0], other->custom_ints[1]);
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_DIALOGE_BOX_TRIGGER);
	if (other != NULL)
		Dialoge_Start(other->custom_ints[0], other->custom_ints[1]);
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_GRAPHICS_CHANGE_TRIGGER);
	if (other != NULL)
	{
		char gct_buff[256];
		const char *gct_file_path = EndingText_GetLine(other->custom_ints[0]);
		if (strcmp(FileSystem_GetRegisteredGfxFile()->name, gct_file_path) != 0)
		{
			sprintf(gct_buff, "load_gfx_file %s", gct_file_path);
			Command_Execute(gct_buff);
		}
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_BACKGROUND_SWITCHER);
	if (other == NULL)
		other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_HORZ_BG_SWITCH);
	if (other == NULL)
		other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_VERT_BG_SWITCH);
	if (other != NULL)
		Background_SetVisibleLayers(other->custom_ints[0], other->custom_ints[1]);
	int old_upgrade_state = local_data->upgrade_state;
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_UPGRADE_SHOES);
	if (other != NULL)
		local_data->upgrade_state = PLAYER_UPGRADE_SHOES;
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_UPGRADE_WINGS);
	if (other != NULL)
		local_data->upgrade_state = PLAYER_UPGRADE_WINGS;
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_UPGRADE_CRYSTAL_WINGS);
	if (other != NULL)
		local_data->upgrade_state = PLAYER_UPGRADE_CRYSTAL_WINGS;
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_NOUPGRADE_TRIGGER);
	if (other != NULL)
		local_data->upgrade_state = PLAYER_UPGRADE_NONE;
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_MAXPOWER_RUNE);
	if (other != NULL)
	{
		if (other->custom_ints[0] != 0)
			local_data->mpoverride = other->custom_ints[0] - 1;
		else
			local_data->mpoverride = wobj->custom_ints[0];
		local_data->upgrade_state = PLAYER_UPGRADE_MAXPOWER;
		if (old_upgrade_state != PLAYER_UPGRADE_MAXPOWER) {
			wobj->strength *= maxpowerinfos[local_data->mpoverride].strength;
		}
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_VORTEX_UPGRADE_TRIGGER);
	if (other != NULL)
		local_data->upgrade_state = PLAYER_UPGRADE_VORTEX;

	if (local_data->upgrade_state != PLAYER_UPGRADE_MAXPOWER && old_upgrade_state == PLAYER_UPGRADE_MAXPOWER) {
		wobj->strength /= maxpowerinfos[local_data->mpoverride].strength;
	}

	other = Wobj_GetWobjCollidingWithType(wobj, TT_NORMAL_TRIGGER);
	if (other != NULL && other->custom_ints[1] <= 0)
	{
		other->custom_ints[1] = 30*5;
		TTBoss_CalmDown();
	}
	other = Wobj_GetWobjCollidingWithType(wobj, SPIDER_WALKER_WEB);
	if (other != NULL && !local_data->vortexed_mode)
	{
		wobj->vel_x = 0.0f;
		wobj->vel_y = 0.0f;
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_CHECKPOINT);
	if (other != NULL)
	{
		if (other->custom_ints[0] > local_data->checkpoint)
		{
			local_data->checkpoint = other->custom_ints[0];
			Interaction_PlaySound(wobj, 12);
		}
	}
	other = Wobj_GetWobjCollidingWithType(wobj, BANDIT_GUY);
	if (other != NULL)
	{
		Item_Drop(wobj);
		Interaction_PlaySound(wobj, 13);
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_SPRING_BOARD);
	if (other != NULL && wobj->y < other->y && wobj->vel_y > 0.0f && !local_data->vortexed_mode)
	{
		wobj->vel_y = -other->custom_floats[0];
		other->custom_ints[0] = 15;
		Interaction_PlaySound(wobj, 33);
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_HEALTH_SET_TRIGGER);
	if (other != NULL)
		wobj->health = other->custom_floats[0];
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_BGSPEED_X);
	if (other != NULL)
		Background_GetLayer(other->custom_ints[0])->speed[0] = other->custom_floats[0];
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_BGSPEED_Y);
	if (other != NULL)
		Background_GetLayer(other->custom_ints[0])->speed[1] = other->custom_floats[0];
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_BGTRANS);
	if (other != NULL)
		Background_GetLayer(other->custom_ints[0])->transparency = (int)other->custom_floats[0];
	if (!local_data->vortexed_mode)
		WobjPhysics_ApplyWindForces(wobj);

	if (wobj->item != ITEM_TYPE_ULTRA_SWORD)
		local_data->uswords_have = 4;

	if (--local_data->beastchurger_timer == 0)
	{
		wobj->strength /= 4.0f;
		Audio_PlayMusic(local_data->beastchurger_music, CNM_TRUE);
	}

	wobj->custom_ints[1] &= ~PLAYER_FLAG_SHOWN_UPGRADE_STATE;
	wobj->custom_ints[1] |= local_data->upgrade_state & PLAYER_FLAG_SHOWN_UPGRADE_STATE;

	local_data->vortex_cooldown--;
	if (local_data->vortex_cooldown <= 0 && !local_data->vortexed_mode)
	{
		local_data->last_used_vortex_node = -1;
		local_data->last_used_vortex_uuid = -1;
	}

	if (!local_data->vortexed_mode)
	{
#define DIST(x, y) (sqrtf((x)*(x)+(y)*(y)))

		other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_VORTEX);
		int in_vortex = CNM_FALSE;
		if (other != NULL)
		{
			float vx = other->x + 32.0f, vy = other->y + 32.0f;
			if (DIST(vx-wobj->x, vy-wobj->y) < 32.0f)// + 16.0f)
				in_vortex = CNM_TRUE;
		}
		if (other != NULL && in_vortex && other->custom_ints[1] == CNM_FALSE &&
			!(local_data->vortex_cooldown > 0 && local_data->last_used_vortex_node == other->node_id &&
			  local_data->last_used_vortex_uuid == other->uuid))
		{
			local_data->vortex_usage_timer = 30*10;
			local_data->vortexed_mode = CNM_TRUE;
			local_data->vortex_x = other->x + 32.0f;
			local_data->vortex_y = other->y + 32.0f;
			local_data->last_used_vortex_node = other->node_id;
			local_data->last_used_vortex_uuid = other->uuid;
			//float wobj_midx = wobj->x + 16.0f, wobj_midy = wobj->y + 16.0f;
			
			// We've entered into a vortex
			float player_ang = atan2f(wobj->y - local_data->vortex_y, wobj->x - local_data->vortex_x);
			
			// Calculate the direction to spin in for the vortex
			float calcx, calcy;
			float nvx = wobj->vel_x, nvy = wobj->vel_y;
			Normalize(&nvx, &nvy);
			GetVortexPos(wobj, player_ang - CNM_2RAD(50.0f), &calcx, &calcy);
			float nextx = wobj->x + (nvx * 30.0f), nexty = wobj->y + (nvy * 30.0f);

			float d1 = DIST(nextx-calcx, nexty-calcy);
			GetVortexPos(wobj, player_ang + CNM_2RAD(50.0f), &calcx, &calcy);
			float d2 = DIST(nextx-calcx, nexty-calcy);

			if (d1 > d2)
				local_data->vortex_dir = 1.0f;
			else
				local_data->vortex_dir = -1.0f;

			// Calculate the speed we entered into the vortex with
			float player_speed = DIST(wobj->vel_x, wobj->vel_y);
			GetVortexPos(wobj, player_ang + (local_data->vortex_dir * CNM_2RAD(50.0f)), &calcx, &calcy);
			//float sx, sy;
			//GetVortexPos(wobj, player_ang, &sx, &sy);
			//float sx2, sy2;
			float sx_ang = player_ang + (CNM_2RAD(90.0f) * local_data->vortex_dir);
			//sx2 = cosf(sx_ang);
			//sy2 = sinf(sx_ang);
			//float dx = fabsf(sx-calcx), dy = fabsf(sy-calcy);
			//Normalize(&dx, &dy);
			float dx = cosf(sx_ang), dy = sinf(sx_ang);
			float dot = nvx*dx + nvy*dy;
			if (dot < 0.0f)
				dot = 0.0f;

			local_data->vortex_speed = (player_speed * (dot + 0.1f)) * local_data->vortex_dir;
			local_data->vortex_speed = CNM_2RAD(local_data->vortex_speed * 1.25f);
			//local_data->vortex_speed = -0.01f;
			local_data->vortex_ang = player_ang;
		}
	}
	else
	{
		if (local_data->vortex_usage_timer-- == 45)
			Interaction_PlaySound(wobj, 42);
		else if (local_data->vortex_usage_timer <= 0)
			local_data->vortex_death = CNM_TRUE;

		local_data->vortex_ang += local_data->vortex_speed;
		if (fabsf(local_data->vortex_speed) < CNM_2RAD(45.0f))
			local_data->vortex_speed += CNM_2RAD(local_data->vortex_dir) / 16.0f;
		GetVortexPos(wobj, local_data->vortex_ang, &wobj->x, &wobj->y);

		if ((Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING) || local_data->vortex_death)
			&& !Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f))
		{
			float new_ang = local_data->vortex_ang + (CNM_2RAD(90.0f) * local_data->vortex_dir);
			float new_spd = fabsf(CNM_2DEG(local_data->vortex_speed)) * 0.8f;
			if (new_spd > 40.0f)
				new_spd = 40.0f;
			wobj->vel_x = cosf(new_ang) * new_spd;
			wobj->vel_y = sinf(new_ang) * new_spd;
			local_data->vortexed_mode = CNM_FALSE;
			local_data->vortex_cooldown = 22;
		}
	}

	if (local_data->vortex_death)
	{
		wobj->health -= 100.0f;
	}

	if (!local_data->vortexed_mode)
	{
		float old_tele_pos[2] = {wobj->x, wobj->y}, new_tele_pos[2];
		if (Wobj_TryTeleportWobj(wobj))
		{
			local_data->in_teleport = 8;
			Camera_SetForced(CNM_TRUE);
			//Camera_TeleportPos((int)wobj->x + 16, (int)wobj->y + 16);
			Fadeout_FadeToWhite(8, 3, 8);
			Interaction_CreateWobj(WOBJ_TELE_PARTICLES, old_tele_pos[0], old_tele_pos[1], 1, 0.0f);
			new_tele_pos[0] = wobj->x;
			new_tele_pos[1] = wobj->y;
			wobj->x = old_tele_pos[0];
			wobj->y = old_tele_pos[1];
			Interaction_PlaySound(wobj, 46);
			wobj->x = new_tele_pos[0];
			wobj->y = new_tele_pos[1];
		}
	}
	if (wobj->health <= 0.0f)
	{
		Interaction_PlaySound(wobj, 47);
		local_data->in_water = CNM_FALSE;
		local_data->vortexed_mode = CNM_FALSE;
		local_data->vortex_death = CNM_FALSE;
		wobj->health = 100.0f;
		for (int k = 0; k < 5; k++)
		{
			for (int ll = 0; ll < 5; ll++)
				Interaction_CreateWobj(WOBJ_BLOOD_PARTICLE, wobj->x, wobj->y, Util_RandInt(3, 9), (Util_RandFloat() * (CNM_PI / 2)) + CNM_PI / 4);
			Interaction_CreateWobj(WOBJ_GIB_PARTICLE, wobj->x, wobj->y, Util_RandInt(3, 9), Util_RandFloat() * CNM_PI);
		}
		PlayerSpawn_SetWobjLoc(&wobj->x);
		if (local_data->checkpoint >= 0)
		{
			wobj->x = PlayerSpawn_GetSpawnLocX(local_data->checkpoint);
			wobj->y = PlayerSpawn_GetSpawnLocY(local_data->checkpoint);
		}
		Camera_SetForced(CNM_TRUE);
		local_data->death_cam_timer = 45+20+2;
		//Camera_TeleportPos((int)wobj->x + 16, (int)wobj->y + 16);
		Fadeout_FadeDeath(45, 20, 20);
		wobj->flags |= WOBJ_INVULN;
		local_data->death_limbo_counter = 20+20+45+20;
		wobj->vel_x = 0.0f;
		wobj->vel_y = 0.0f;
		wobj->flags |= WOBJ_PLAYER_IS_RESPAWNING;
		{
			//WOBJ *tempitem;
			int itemtype = Item_GetCurrentItem()->type;
			Item_DestroyCurrentItem(wobj);
			Item_PickupByType(wobj, itemtype);
			//tempitem = Wobj_CreateOwned(WOBJ_DROPPED_ITEM, wobj->x, wobj->y, itemtype, 0.0f);
			//Item_Pickup(wobj, tempitem);
		}
	}

	if (isnan(wobj->health)) wobj->health = 100.0f;

	if (!local_data->vortexed_mode)
		WobjPhysics_EndUpdate(wobj);
	//wobj->y += stored_movestand_yvel;

	// HPCOST from MAXPOWER upgrade
	if (local_data->upgrade_state == PLAYER_UPGRADE_MAXPOWER) {
		wobj->health -= maxpowerinfos[local_data->mpoverride].hpcost / 30.0f;
	}

	Item_TryPickupAndDrop(wobj);
	Item_Update(wobj);
	Item_TryUse(wobj);

	int recved_lava_damage = CNM_FALSE;
	float old_hp = wobj->health;
	int recved_normal_damage = 0;
	// ability 3
	if (!local_data->ability3_hasjumped) {
		local_data->ability3_timer--;
	}
	if (local_data->ability3_timer <= 0) {
		if (local_data->fire_resistance-- <= 0)
			recved_lava_damage = Interaction_WobjReceiveBlockDamage(wobj);
		if (wobj->health >= old_hp)
			recved_lava_damage = CNM_FALSE;
		recved_normal_damage = Interaction_PlayerRecieveDamage();
	}
	else {
		if (local_data->currskin != 9)
			local_data->curranim = PLAYER_ANIM_SHOOT;
	}
	if (wobj->health < old_hp)
	{
		wobj->flags |= WOBJ_DAMAGE_INDICATE;
		local_data->animforce_cooldown_timer = 4;
		local_data->animforce_cooldown = CNM_TRUE;
		local_data->curranim = PLAYER_ANIM_HURT;
	}

	if ((recved_lava_damage || recved_normal_damage) && !Audio_IsSoundPlaying(1))
		Interaction_PlaySound(wobj, 1);
	
	// PVP damage indictaion
	//local_data->pvp_damage_indicator_timer--; // <- MOVED UP BEFORE DEATH CODE
	if (local_data->pvp_damage_indicator_timer > 0) {
		if (local_data->pvp_damage_indicator_timer % 2 == 0)
			wobj->flags &= ~WOBJ_DAMAGE_INDICATE;
		else
			wobj->flags |= WOBJ_DAMAGE_INDICATE;
	}
	
	if (!Wobj_IsGrouneded(wobj) && !local_data->animforce_cooldown)
	{
		if (local_data->curranim != PLAYER_ANIM_JUMP)
		{
			if (local_data->currskin == 9)
			{
				if (wobj->vel_y < -1.0f)
					local_data->currframe = 0;
				else
					local_data->currframe = 7;
			}
		}
		local_data->curranim = PLAYER_ANIM_JUMP;
		local_data->animspd = 3;
	}
	StepPlayerAnimation(wobj);

	{
		CNM_RECT pr;
		unsigned int af = 0;
		PlayerAnimGetRect(&pr, local_data->currskin, local_data->curranim, local_data->currframe);
		if (local_data->death_limbo_counter > 0) {
			PlayerAnimGetRect(&pr, local_data->currskin, 0, 0);
		}
		// Smush this into a 32bit integer
		af |= pr.x&0xffff;
		af |= (pr.y&0x7fff) << 16;
		if (local_data->currskin == 9) af |= 0x80000000;
		wobj->anim_frame = *((int *)(&af));
	}

	// Cap player health, money, and strength
	if (wobj->health > PLAYER_HP_MAX)
		wobj->health = PLAYER_HP_MAX;
	if (wobj->strength > 9.99f)
		wobj->strength = 9.99f;
	if (wobj->money > 99999)
		wobj->money = 99999;
}
void WobjPlayer_Draw(WOBJ *wobj, int camx, int camy)
{
	CNM_RECT r;
	//CNM_RECT r;
	if ((wobj->custom_ints[1] & PLAYER_FLAG_SHOWN_UPGRADE_STATE) == PLAYER_UPGRADE_VORTEX)
	{
		Util_SetRect(&r, 64, 224, 32, 32);
		//Renderer_DrawBitmap((int)wobj->x - camx, (int)wobj->y - camy, &r, 0, RENDERER_LIGHT);
		Renderer_DrawBitmap2
		(
			(int)wobj->x - camx,
			(int)wobj->y - camy,
			&r,
			0,
			RENDERER_LIGHT,
			wobj->flags & WOBJ_HFLIP,
			CNM_FALSE
		);
	}
	//wobj->anim_frame = custom ints[0];
	//WobjGeneric_Draw(wobj, camx, camy);
	//Util_SetRect(&r, (wobj->item % 8) * 32, 352 + (wobj->item / 8) * 32, 32, 32);
	//if (wobj->item)
	//	Renderer_DrawBitmap((int)wobj->x - camx, (int)wobj->y - camy, &r, 0, RENDERER_LIGHT);
	DrawPlayerChar(wobj, camx, camy);
	Renderer_DrawBitmap2
	(
		(int)wobj->x - camx,
		(int)wobj->y - camy,
		&item_types[wobj->item].frames[0],
		0,
		RENDERER_LIGHT,
		wobj->flags & WOBJ_HFLIP,
		CNM_FALSE
	);

	const char *name = NULL;
	if (Interaction_GetMode() == INTERACTION_MODE_CLIENT ||
		Interaction_GetMode() == INTERACTION_MODE_HOSTED_SERVER)
		name = NetGame_GetNode(wobj->node_id)->name;
	else if (Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER)
		name = Game_GetVar(GAME_VAR_PLAYER_NAME)->data.string;
	Renderer_DrawText((int)wobj->x - camx - (strlen(name) * 4) + 16, (int)wobj->y - camy - 12, 0, RENDERER_LIGHT, name);

	switch (wobj->custom_ints[1] & PLAYER_FLAG_SHOWN_UPGRADE_STATE)
	{
	case PLAYER_UPGRADE_SHOES:
		Util_SetRect(&r, 256, 128, 32, 32);
		Renderer_DrawBitmap((int)wobj->x - camx, (int)wobj->y - camy, &r, 0, RENDERER_LIGHT);
		break;
	case PLAYER_UPGRADE_MAXPOWER:
		if ((Game_GetFrame() / 3) % 2 == 0)
			Util_SetRect(&r, 192, 1120, 48, 48);
		else
			Util_SetRect(&r, 192, 1120+48, 48, 48);
		Renderer_DrawBitmap((int)wobj->x - camx - 8, (int)wobj->y - camy - 8, &r, 0, RENDERER_LIGHT);
		break;
	case PLAYER_UPGRADE_WINGS:
		if ((Game_GetFrame() / 4) % 2 == 0)
			Util_SetRect(&r, 320, 352, 36, 38);
		else
			Util_SetRect(&r, 356, 352, 28, 38);
		Renderer_DrawBitmap((int)wobj->x - camx, (int)wobj->y - camy, &r, 0, RENDERER_LIGHT);
		break;
	case PLAYER_UPGRADE_CRYSTAL_WINGS:
		if ((Game_GetFrame() / 2) % 2 == 0)
			Util_SetRect(&r, 384 - 2, 352, 48, 48);
		else
			Util_SetRect(&r, 384+64, 352, 48, 48);
		Renderer_DrawBitmap((int)wobj->x - camx, (int)wobj->y - camy, &r, 4, RENDERER_LIGHT);
		break;
	}
}

void Player_RecieveDamage(WOBJ *player, WOBJ *inflictor, float damage_taken)
{
	player->health -= damage_taken;
}

static void StepPlayerAnimation(WOBJ *wobj)
{
	PLAYER_LOCAL_DATA *ld = (PLAYER_LOCAL_DATA *)wobj->local_data;

	static int goodanim_godir = 1;

	if (ld->animspd == 0) ld->animspd = 1;

	if (ld->currframe >= anim_lengths[ld->curranim])  ld->currframe = anim_lengths[ld->curranim]-1;
	ld->animtimer++;
	if (ld->currskin != 9 ||
		(ld->curranim == PLAYER_ANIM_WALKING ||
		 ld->curranim == PLAYER_ANIM_HURT))
	{
		if (ld->animtimer % ld->animspd == 0)
			ld->currframe = (ld->currframe + 1) % anim_lengths[ld->curranim];
	}
	else if (ld->curranim == PLAYER_ANIM_STANDING)
	{
		if (ld->animtimer % ld->animspd == 0)
		{
			ld->currframe += goodanim_godir;
			if (ld->currframe >= anim_lengths[ld->curranim]) {
				goodanim_godir = -1;
				ld->currframe = anim_lengths[ld->curranim] - 2;
			}
			if (ld->currframe < 0) {
				goodanim_godir = 1;
				ld->currframe = 1;
			}
		}
	}
	else if (ld->curranim == PLAYER_ANIM_JUMP)
	{
		if (ld->animtimer % ld->animspd == 0) {
			ld->currframe++;
			if (ld->currframe >= anim_lengths[ld->curranim])
				ld->currframe = anim_lengths[ld->curranim]-2;
		}
	}
	else if (ld->curranim == PLAYER_ANIM_JUMP_END)
	{
		ld->currframe = (ld->animtimer / 2 + 1) % 2;
		if (ld->animtimer / 2 > 2)
		{
			ld->currframe = 0;
			ld->curranim = 0;
		}
	}
	else
	{
		if (ld->animtimer % ld->animspd == 0)
			ld->currframe = (ld->currframe + 1) % anim_lengths[ld->curranim];
	}
	wobj->anim_frame = ld->currframe;
}
static void PlayerAnimGetRect(CNM_RECT *r, int skin, int anim, int frame)
{
	if (skin != 9)
	{
		r->x = skin_bases[skin][0] + anim_offsets[anim][frame][0];
		r->y = skin_bases[skin][1] + anim_offsets[anim][frame][1];
		r->w = 32;
		r->h = 32;
	}
	else
	{
		int base = 0;
		if (anim == PLAYER_ANIM_WALKING) base = anim_lengths2[0];
		if (anim == PLAYER_ANIM_JUMP) base = anim_lengths2[0]+ anim_lengths2[1]+2;
		if (anim == PLAYER_ANIM_JUMP_END)
			base = anim_lengths2[0]+anim_lengths2[1] +
			anim_lengths2[2] + anim_lengths2[3];
		if (anim == PLAYER_ANIM_HURT)
			base = 29;
		int dx = (base+frame)%12;
		int dy = (base+frame)/12;
		r->x = dx*40;
		r->y = 4608+dy*40;
		r->w = 40;
		r->h = 40;
	}
}
static void DrawPlayerChar(WOBJ *wobj, int camx, int camy)
{
	//PLAYER_LOCAL_DATA *ld = (PLAYER_LOCAL_DATA *)wobj->local_data;
	CNM_RECT r, pr;
	float wobj_center_x, wobj_center_y;
	wobj_center_x = wobj->x + 16.0f; //wobj->x + (float)(wobj_types[wobj->type].frames[wobj->anim_frame].w / 2);
	wobj_center_y = wobj->y + 16.0f; //wobj->y + (float)(wobj_types[wobj->type].frames[wobj->anim_frame].h / 2);
	
	
		unsigned int af = *((unsigned int *)(&wobj->anim_frame));
		pr.w = 32;
		pr.h = 32;
		pr.x = (af&0xffff);
		pr.y = ((af >> 16)&0x7fff);
	
		int skin9offsetx = 0;
		int skin9offsety = 0;
	if (af & 0x80000000) {
		
		//if (pr.y >= 4000) {
			skin9offsetx = 5;
			skin9offsety = 8;
			pr.w = 40;
			pr.h = 40;
		//}
	}

	Renderer_DrawBitmap2
	(
		(int)wobj->x - camx - skin9offsetx,
		(int)wobj->y - camy - skin9offsety,
		&pr,
		0,
		Wobj_DamageLighting(wobj, Blocks_GetCalculatedBlockLight((int)wobj_center_x / BLOCK_SIZE, (int)wobj_center_y / BLOCK_SIZE)),
		wobj->flags & WOBJ_HFLIP,
		wobj->flags & WOBJ_VFLIP
	);
	//wobj->flags &= WOBJ_DAMAGE_INDICATE;

	if (Game_GetFrame() % 3 == 0)
		wobj->flags &= ~WOBJ_DAMAGE_INDICATE;

	if (Game_GetVar(GAME_VAR_SHOW_COLLISION_BOXES)->data.integer)
	{
		Util_SetRect(&r, (int)(wobj->x + wobj->hitbox.x) - camx, (int)(wobj->y + wobj->hitbox.y) - camy,
					 (int)wobj->hitbox.w, (int)wobj->hitbox.h);
		Renderer_DrawRect(&r, Renderer_MakeColor(255, 0, 255), 2, RENDERER_LIGHT);
	}

	if (Game_GetVar(GAME_VAR_SHOW_GRIDPOS)->data.integer)
	{
		Renderer_DrawText
		(
			(int)wobj->x - camx, (int)wobj->y - camy, 0, RENDERER_LIGHT,
			"(%d, %d)",
			(int)(wobj->x / OBJGRID_SIZE), (int)(wobj->y / OBJGRID_SIZE)
		);
	}

	if (Game_GetVar(GAME_VAR_SHOWPOS)->data.integer)
	{
		Renderer_DrawText
		(
			(int)wobj->x - camx, (int)wobj->y - camy + 8, 0, RENDERER_LIGHT,
			"(%d, %d)",
			(int)(wobj->x), (int)(wobj->y)
		);
	}

	if (Game_GetVar(GAME_VAR_CL_SHOW_NODEUUIDS)->data.integer)
	{
		Renderer_DrawText
		(
			(int)wobj->x - camx, (int)wobj->y - camy + 16, 0, RENDERER_LIGHT,
			"NODE: %d, UUID: %d",
			wobj->node_id, wobj->uuid
		);
	}
}

void Player_PlayShootAnim(WOBJ *player)
{
	//PLAYER_LOCAL_DATA *ld = (PLAYER_LOCAL_DATA *)player->local_data;
	//ld->animforce_cooldown_timer = 7;
	//ld->animforce_cooldown = CNM_TRUE;
	//ld->curranim = PLAYER_ANIM_SHOOT;
}
void Player_PlayMeleeAnim(WOBJ *player)
{
	PLAYER_LOCAL_DATA *ld = (PLAYER_LOCAL_DATA *)player->local_data;
	ld->animforce_cooldown_timer = 20;
	ld->animforce_cooldown = CNM_TRUE;
	ld->curranim = PLAYER_ANIM_MELEE;
}

void Player_OnRecievePVPDamage(WOBJ *player) {
	PLAYER_LOCAL_DATA *local_data = (PLAYER_LOCAL_DATA *)player->local_data;

	player->flags |= WOBJ_DAMAGE_INDICATE;
	local_data->animforce_cooldown_timer = 4;
	local_data->animforce_cooldown = CNM_TRUE;
	local_data->curranim = PLAYER_ANIM_HURT;
	local_data->pvp_damage_indicator_timer = 10;
	if (!Audio_IsSoundPlaying(1)) Interaction_PlaySound(player, 1);
}

static void DrawHealthBar(int x, int y, int w) {
	CNM_RECT r;

	int fine = (Game_GetFrame() / 3), coarse = (Game_GetFrame() / 10), ccoarse = (Game_GetFrame() / 5);
	int offsets[4] = {
		0, 11, 22, 11
	};
	int xoff = 0;
	for (w; w > 0; w -= 32) {
		int ww = w;
		if (ww > 32) ww = 32;
		Util_SetRect(&r, 288 + (fine % 32), 4256 + offsets[coarse % 4] + (ccoarse % 5), ww, 5);
		Renderer_DrawBitmap(x+xoff, y, &r, 0, RENDERER_LIGHT);
		xoff += 32;
	}
}
static void DrawHealthOutline(int x, int y, int w)
{
	CNM_RECT r;

	int xoff = 0;
	for (w; w > 0; w -= 32)
	{
		int ww = w;
		Util_SetRect(&r, 320, 4247, 32, 9);
		if (ww > 32) ww = 32;
		else {
			Util_SetRect(&r, 352+(32-ww), 4247, ww, 9);
		}
		Renderer_DrawBitmap(x + xoff, y, &r, 0, RENDERER_LIGHT);
		xoff += ww;
	}
	Util_SetRect(&r, 352, 4261 + (Game_GetFrame() % 3)*9, 9, 9);
	Renderer_DrawBitmap(x + xoff - 1, y, &r, 0, RENDERER_LIGHT);
}
static void DrawHealthBG(int x, int y, int w)
{
	CNM_RECT r;

	int xoff = 0;
	for (w; w > 0; w -= 32)
	{
		int ww = w;
		if (ww > 32) ww = 32;
		Util_SetRect(&r, 352, 4256, ww, 5);
		Renderer_DrawBitmap(x + xoff, y, &r, 2, RENDERER_LIGHT);
		xoff += ww;
	}
}
static void DrawParticle(int x, int y, int dt) {
	int t, j;

	t = 1;
	j = 4;
	if (dt < 7) {
		t = 3;
		j = 6;
	}
	if (dt < 4)
	{
		t = 4;
		j = 7;
	}

	Renderer_PlotPixel2(x, y, Renderer_MakeColor(255, 255, 255), t, 0);
	Renderer_PlotPixel2(x-1, y, Renderer_MakeColor(255, 255, 255), j, 0);
	Renderer_PlotPixel2(x+1, y, Renderer_MakeColor(255, 255, 255), j, 0);
	Renderer_PlotPixel2(x, y-1, Renderer_MakeColor(255, 255, 255), j, 0);
	Renderer_PlotPixel2(x, y+1, Renderer_MakeColor(255, 255, 255), j, 0);
}

#define MAXP 128

static float hp_curr = 0.0f;
static int last_hpbreak = 0, curr_hpbreak = 0;
static int last_hpbreak2 = 0, curr_hpbreak2 = 0;
static float hp_break_anim;
static float hp_x[MAXP], hp_y[MAXP], hp_vx[MAXP], hp_vy[MAXP];
static int hp[MAXP];
static int next_hp = 0;

static void CreateParticle(int x, int y, float vx, float vy) {
	hp_x[next_hp] = (float)x;
	hp_y[next_hp] = (float)y;
	hp_vx[next_hp] = vx;
	hp_vy[next_hp] = vy;
	hp[next_hp] = rand() % 20 + 15;
	next_hp = (next_hp + 1) % MAXP;
}

static void StepAndDrawParticles(void) {
	int i;

	for (i = 0; i < MAXP; i++) {
		if (hp[i]-- <= 0) continue;
		hp_x[i] += hp_vx[i];
		hp_y[i] += hp_vy[i];
		hp_vy[i] += 0.1f;
		DrawParticle((int)hp_x[i], (int)hp_y[i], hp[i]);
	}
}

void Player_ResetHUD(void) {
	hp_curr = 0.0f;
	last_hpbreak = 0;
	curr_hpbreak = 0;
	last_hpbreak2 = 0;
	curr_hpbreak2 = 0;
	hp_break_anim = 0.0f;
	memset(hp, 0, sizeof(hp));
	next_hp = 0;
}
void Player_DrawHUD(WOBJ *player) {
	CNM_RECT r;

	char temp_hud[64];
	Util_SetRect(&r, 192, 4224, 96, 6);
	Renderer_DrawBitmap(0, RENDERER_HEIGHT - 64, &r, 2, RENDERER_LIGHT);
	Util_SetRect(&r, 288, 4241, 81, 5);
	Renderer_DrawBitmap(2, RENDERER_HEIGHT - 64 + 8, &r, 2, RENDERER_LIGHT);

	if (curr_hpbreak) {
		Util_SetRect(&r, 352, 4256, 32, 5);
		Renderer_DrawBitmap(83, RENDERER_HEIGHT - 64 + 8, &r, 2, RENDERER_LIGHT);
		Renderer_DrawBitmap(83+32, RENDERER_HEIGHT - 64 + 8, &r, 2, RENDERER_LIGHT);
	}

	int bar_len, i, x, y;

	float target_hp = player->health, tween = 0.5f;
	if (target_hp < 0.0f) target_hp = 0.0f;
	if (player->flags & WOBJ_PLAYER_IS_RESPAWNING) {
		target_hp = 0.0f;
		tween = 0.2f;
	}
	hp_curr -= (hp_curr - target_hp) * tween;

	bar_len = (int)((hp_curr / PLAYER_HP_NORMAL_MAX) * 81.0f);
	if (hp_curr > 500) {
		bar_len = (int)(202.5f + ((hp_curr - 500) / 500) * 81.0f);
	}
	DrawHealthBar(2, RENDERER_HEIGHT - 64 + 8, bar_len);

	last_hpbreak = curr_hpbreak;
	curr_hpbreak = hp_curr > PLAYER_HP_NORMAL_MAX;
	last_hpbreak2 = curr_hpbreak2;
	curr_hpbreak2 = bar_len > 136+8;

	if (curr_hpbreak != last_hpbreak) {

		x = 83;
		y = RENDERER_HEIGHT - 64 + 10;
		if (!curr_hpbreak) {
			x = 106+32;
			hp_break_anim = 60.0f;
		}
		for (i = 0; i < 128; i++)
		{
			CreateParticle(x, y,
						   (float)(rand() % 30 - 15) / 5.0f,
						   (float)(rand() % 30 - 15) / 5.0f);
		}
	}

	if (!curr_hpbreak) {
		if (hp_break_anim < 2.0f) {
			Util_SetRect(&r, 288, 4230, 93, 9);
			Renderer_DrawBitmap(0, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
			hp_break_anim = 0.0f;
		}
		else {
			Util_SetRect(&r, 288, 4230, 74, 9);
			Renderer_DrawBitmap(0, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
			Util_SetRect(&r, 288, 4247, 19, 9);
			Renderer_DrawBitmap(74, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
			DrawHealthBG(83, RENDERER_HEIGHT - 64 + 6+2, (int)hp_break_anim+3);
			DrawHealthOutline(87, RENDERER_HEIGHT - 64 + 6, (int)hp_break_anim);
		}
		hp_break_anim -= 10.0f;
		//hp_break_anim -= 0.01f;
	}
	else {
		hp_break_anim = 0;
		Util_SetRect(&r, 288, 4230, 74, 9);
		Renderer_DrawBitmap(0, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
		Util_SetRect(&r, 288, 4247, 32, 9);
		Renderer_DrawBitmap(74, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
		Util_SetRect(&r, 320, 4247, 32, 9);
		Renderer_DrawBitmap(106, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
		if (!curr_hpbreak2) {
			Util_SetRect(&r, 375, 4247, 9, 9);
			Renderer_DrawBitmap(106 + 32, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
		}
		else {
			DrawHealthOutline(106+32, RENDERER_HEIGHT - 64 + 6, bar_len-135);

			for (i = 0; i < 4; i++) {
				CreateParticle(106 + 30 + (bar_len - 135), RENDERER_HEIGHT - 64 + 7 + rand() % 7, 1.0f,
							   (float)(rand() % 15 - 10) / 5.0f);
			}
		}
	}
	
	Util_SetRect(&r, 192, 4224+6+9, 96, 64-6-9);
	Renderer_DrawBitmap(0, RENDERER_HEIGHT - 64 + 6 + 9, &r, 2, RENDERER_LIGHT);
	
	sprintf(temp_hud, "%d", (int)ceilf(player->health));
	Renderer_DrawText(4, RENDERER_HEIGHT - 45, 0, RENDERER_LIGHT, "HP");
	Renderer_DrawText(90 - strlen(temp_hud) * 8, RENDERER_HEIGHT - 45, 0, RENDERER_LIGHT, temp_hud);
	sprintf(temp_hud, "%d", (int)ceilf(player->strength * 100.0f));
	Renderer_DrawText(39, RENDERER_HEIGHT - 34, 0, RENDERER_LIGHT, "STR");
	Renderer_DrawText(91 - strlen(temp_hud) * 8, RENDERER_HEIGHT - 34, 0, RENDERER_LIGHT, temp_hud);
	
	switch (player->custom_ints[1] & PLAYER_FLAG_SHOWN_UPGRADE_STATE) {
	case PLAYER_UPGRADE_NONE:
		Renderer_DrawText(39, RENDERER_HEIGHT - 22, 0, RENDERER_LIGHT, "NORMAL");
		break;
	case PLAYER_UPGRADE_SHOES:
		Renderer_DrawText(39, RENDERER_HEIGHT - 22, 0, RENDERER_LIGHT, "BOOTS");
		break;
	case PLAYER_UPGRADE_WINGS:
		Renderer_DrawText(39, RENDERER_HEIGHT - 22, 0, RENDERER_LIGHT, "WINGS");
		break;
	case PLAYER_UPGRADE_CRYSTAL_WINGS:
		Renderer_DrawText(39, RENDERER_HEIGHT - 22, 0, RENDERER_LIGHT, "CWINGS");
		break;
	case PLAYER_UPGRADE_VORTEX:
		Renderer_DrawText(39, RENDERER_HEIGHT - 22, 0, RENDERER_LIGHT, "VORTEX");
		break;
	case PLAYER_UPGRADE_MAXPOWER:
		Renderer_DrawText(39, RENDERER_HEIGHT - 22, 0, RENDERER_LIGHT, "MAX POW");
		break;
	}
	
	sprintf(temp_hud, "%d", player->money);
	Renderer_DrawText(33, RENDERER_HEIGHT - 11, 0, RENDERER_LIGHT, "$$"); 
	Renderer_DrawText(91 - strlen(temp_hud) * 8, RENDERER_HEIGHT - 11, 0, RENDERER_LIGHT, temp_hud);
	if (player->item)
	{
		Renderer_DrawBitmap(3, RENDERER_HEIGHT - 35, &item_types[player->item].frames[0], 0, RENDERER_LIGHT);
	}

	Util_SetRect(&r, 0, 4288, 64, 32);
	Renderer_DrawBitmap2(RENDERER_WIDTH - 64, RENDERER_HEIGHT - 32, &r, 2, RENDERER_LIGHT, 1, 1);

	int bx = RENDERER_WIDTH - 64, by = RENDERER_HEIGHT - 32;
	sprintf(temp_hud, "%d%%%%", (int)ceilf(player->speed * 20.0f));
	Renderer_DrawText(bx+8, by+10, 0, RENDERER_LIGHT, "SP");
	Renderer_DrawText(bx+62 - (strlen(temp_hud)-1) * 8, by+10, 0, RENDERER_LIGHT, temp_hud);
	sprintf(temp_hud, "%d%%%%", (int)ceilf(player->jump * 10.0f));
	Renderer_DrawText(bx+8, by+22, 0, RENDERER_LIGHT, "JP");
	Renderer_DrawText(bx+62 - (strlen(temp_hud) - 1) * 8, by+22, 0, RENDERER_LIGHT, temp_hud);

	StepAndDrawParticles();
}