#include <string.h>
#include <math.h>
#include <stdio.h>
#include "renderer.h"
#include "utility.h"
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
#include "savedata.h"
#include "pausemenu.h"
#include "serial.h"
#include "petdefs.h"
#include "mem.h"

PLAYER_MAXPOWER_INFO maxpowerinfos[32] = {0};

#define FLAPACCEL 8.0f 
#define JUMP_TIMER 6

static const int anim_lengths[PLAYER_MAX_SKINS][PLAYER_ANIM_MAX] = {
	{ // skin 0
		1, // standing
		4, // walking
		1, // jumping
		1, // shooting
		1, // melee
		1, // hurt
		1, // slide
		0, // turn walk
		0, // turn air
	},
	{ // skin 1
		6, // standing
		8, // walking
		6, // jumping
		0, // jumping1
		2, // jumping2
		1, // hurt
		2, // slide
		1, // turn walk
		1, // turn air
	},
	{ // skin 2
		1, // standing
		4, // walking
		1, // jumping
		1, // shooting
		1, // melee
		1, // hurt
		1, // slide
		0, // turn walk
		0, // turn air
	},
	{ // skin 3
		1, // standing
		4, // walking
		1, // jumping
		1, // shooting
		1, // melee
		1, // hurt
		1, // slide
		0, // turn walk
		0, // turn air
	},
	{ // skin 4
		6, // standing
		8, // walking
		6, // jumping
		0, // jumping1
		2, // jumping2
		1, // hurt
		2, // slide
		1, // turn walk
		1, // turn air
	},
	{ // skin 5
		1, // standing
		4, // walking
		1, // jumping
		1, // shooting
		1, // melee
		1, // hurt
		1, // slide
		0, // turn walk
		0, // turn air
	},
	{ // skin 6
		1, // standing
		4, // walking
		1, // jumping
		1, // shooting
		1, // melee
		1, // hurt
		1, // slide
		0, // turn walk
		0, // turn air
	},
	{ // skin 7
		1, // standing
		4, // walking
		1, // jumping
		1, // shooting
		1, // melee
		1, // hurt
		1, // slide
		0, // turn walk
		0, // turn air
	},
	{ // skin 8
		6, // standing
		8, // walking
		6, // jumping
		0, // jumping1
		2, // jumping2
		1, // hurt
		2, // slide
		1, // turn walk
		1, // turn air
	},
	{ // skin 9
		6, // standing
		9, // walking
		10, // jumping
		2, // jumping1
		2, // jumping2
		1, // hurt
		2, // slide
		0, // turn walk
		0, // turn air
	},
	{ // skin 10
		6, // standing
		8, // walking
		6, // jumping
		0, // jumping1
		2, // jumping2
		1, // hurt
		2, // slide
		1, // turn walk
		1, // turn air
	},
};

static const int anim_offsets[PLAYER_ANIM_MAX][6][2] = 
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
	},
	{ // slide
		{64, 32}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	},
};
static const int skin_bases[PLAYER_MAX_SKINS][2] =
{
	{0, 704},
	{0, 1904},
	{0, 832},
	{0, 1232},
	{0, 1584},
	{0, 640},
	{128, 640},
	{384, 640},
	{0, 1712},
	{0, 1328},
	{0, 1456},
};
static const int skin_srcbase[PLAYER_MAX_SKINS] = {
	0,
	1904,
	0, 0,
	1584,
	0, 0, 0,
	1712,
	1328,
	1456,
};
const int complex_skins[PLAYER_MAX_SKINS] = {
	CNM_FALSE,
	CNM_TRUE,
	CNM_FALSE,
	CNM_FALSE,
	CNM_TRUE,
	CNM_FALSE,
	CNM_FALSE,
	CNM_FALSE,
	CNM_TRUE,
	CNM_TRUE,
	CNM_TRUE,
};

#define TURN_ANIM_TIME 3

// For when the player dies and respawns
static float _hud_player_y = 0.0f, _hud_player_yvel = 0.0f;
static int level_end_unlockable_y = 0, unlockable_show = CNM_FALSE;
static int level_end_rank_y = 0;
static int skin_unlock_y = 0, skin_unlock_timer = 0;
static int pet_unlock_y = 0, pet_unlock_timer = 0;
static int titlepopup_y = 0, titlepopup_timer = 0;

#define MAX_USED_DIALOGS 16
static unsigned char *_used_dialogs_node;
static int *_used_dialogs_uuid;
static int _ud_idx;

void player_sys_init(void) {
	_used_dialogs_node = arena_global_alloc(sizeof(*_used_dialogs_node) * MAX_USED_DIALOGS);
	_used_dialogs_uuid = arena_global_alloc(sizeof(*_used_dialogs_uuid) * MAX_USED_DIALOGS);
}

static void clear_used_dialogs(void) {
	for (int i = 0; i < MAX_USED_DIALOGS; i++) {
		_used_dialogs_node[i] = 0;
		_used_dialogs_uuid[i] = -1;
	}
	_ud_idx = 0;
}
static int has_used_dialog(unsigned char node, int uuid) {
	for (int i = 0; i < MAX_USED_DIALOGS; i++) {
		if (_used_dialogs_node[i] == node && _used_dialogs_uuid[i] == uuid) return CNM_TRUE;
	}
	return CNM_FALSE;
}

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

void Player_SetSkinInstant(WOBJ *wobj, int skinid) {
	PLAYER_LOCAL_DATA *local_data = wobj->local_data;
	if (skinid < 0 || skinid > PLAYER_MAX_SKINS) skinid = 0;
	local_data->currskin = skinid;
	local_data->curranim = PLAYER_ANIM_STANDING;
	local_data->currframe = 0;
	local_data->animspd = 0;
	//if (wobj->internal.owned)
	//{
		//if (!complex_skins[local_data->currskin])
			//anim_lengths = anim_lengths1;
		//else
			//anim_lengths = anim_lengths2;
	//}

}
void Player_SwapOffhand(WOBJ *wobj) {
	PLAYER_LOCAL_DATA *local_data = wobj->local_data;
	int curritem = Item_GetCurrentItem()->type;
	float currdur = Item_GetCurrentItem()->durability;
	int offhand = local_data->offhand_item;
	Item_DestroyCurrentItem(wobj);
	//wobj->item = offhand;
	Item_PickupByType(wobj, offhand, local_data->offhand_durability);
	Audio_PlaySound(51, CNM_FALSE, wobj->x, wobj->y);
	local_data->offhand_item = curritem;
	local_data->offhand_durability = currdur;
	Item_NullifyGhostPickup();
}

static int player_is_on_spring(WOBJ *wobj) {
	wobj->y += 8.0f;
	WOBJ *other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_SPRING_BOARD);
	wobj->y -= 8.0f;
	if (other) {
		return CNM_TRUE;//wobj->y + wobj->hitbox.y + wobj->hitbox.h < other->y + 16.0f;
	} else {
		return CNM_FALSE;
	}
}

void WobjPlayer_Create(WOBJ *wobj)
{
	PLAYER_LOCAL_DATA *local_data;
	wobj->local_data = arena_alloc(sizeof(PLAYER_LOCAL_DATA));
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
	clear_used_dialogs();

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
	wobj->hitbox.y = 3.0f;
	wobj->hitbox.w = 17.0f;
	wobj->hitbox.h = 29.0f;
	wobj->custom_floats[1] = 0.0f;
	wobj->custom_ints[1] = 0; // Player Flags
	local_data->uswords_have = 4;
	local_data->death_limbo_counter = 0;

	Player_SetSkinInstant(wobj, wobj->custom_ints[0]);

	local_data->control_mul = 1.0f;
	local_data->upgrade_state = PLAYER_UPGRADE_NONE;
	local_data->fire_resistance = 0;
	local_data->grav = Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
	local_data->grav_add = 0.0f;
	local_data->sliding_cap_landing_speed = CNM_FALSE;
	local_data->slide_jump_cooldown = 0;
	local_data->offhand_item = 0;
	local_data->lock_controls = CNM_FALSE;
	//local_data->flap_accadd = 0.0f;
	//local_data->flap_spdadd = 0.0f;
	local_data->isflying = CNM_FALSE;
	local_data->numflaps = 0;
	local_data->isdiving = CNM_FALSE;
	local_data->saved_diving_vel = 0.0f;
	local_data->upgradehp = 100.0f;
	local_data->jump_init_yspd = 0.0f;
	local_data->level_end_unlockable = -1;
	local_data->level_end_found_secret = CNM_FALSE;
	local_data->level_end_rank = 0;
	local_data->level_end_score = 0;
	local_data->level_end_time_score = 0;
	local_data->level_end_norank = CNM_FALSE;
	unlockable_show = CNM_FALSE;
	level_end_unlockable_y = -128;
	level_end_rank_y = RENDERER_HEIGHT;

	skin_unlock_y = -128;
	skin_unlock_timer = 0;
	pet_unlock_y = -128;
	pet_unlock_timer = 0;
	titlepopup_y = -128;
	titlepopup_timer = 0;
	local_data->last_touched_skin_unlock = -1;
	local_data->last_touched_pet_unlock = -1;
	local_data->num_deaths = 0;
	//local_data->item_durability = 100.0f;
	g_can_pause = CNM_TRUE;

	local_data->jump_input_buffer = 0;
	local_data->has_cut_jump = CNM_FALSE;
	local_data->is_grounded_buffer = 0;
	local_data->slide_input_buffer = 0;
	local_data->stored_slide_speed = 0.0f;
	local_data->slide_super_jump_timer = 0;

	//local_data->stored_plat_velx = 0.0f;
	local_data->been_jumping_timer = 0;
	local_data->skip_jumpthrough_timer = 0;
	local_data->in_splash = 0;
	local_data->last_in_splash = 0;
	//local_data->slope_jumped = CNM_FALSE;

	local_data->platinfo.active = false;

	PlayerSpawn_SetWobjLoc(&wobj->x);

	if (Game_GetVar(GAME_VAR_PLAYER_PET)->data.integer != -1 && wobj->internal.owned) {
		local_data->pet = Interaction_CreateWobj(WOBJ_PLAYER_PET, wobj->x, wobj->y, Game_GetVar(GAME_VAR_PLAYER_PET)->data.integer, 0.0f);
		local_data->pet->link_node = wobj->node_id;
		local_data->pet->link_uuid = wobj->uuid;
	} else {
		local_data->pet = NULL;
	}
}

void player_launch_from_platinfo(WOBJ *wobj) {
	PLAYER_LOCAL_DATA *local_data = wobj->local_data;
	if (!local_data->platinfo.active) return;
	if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 1.0f)) return;
	wobj->vel_x += local_data->platinfo.last_velx;
	wobj->vel_y += local_data->platinfo.last_vely;
	local_data->platinfo.active = false;
}

void WobjPlayer_Update(WOBJ *wobj)
{
	PLAYER_LOCAL_DATA *local_data = wobj->local_data;

	// Finishing and noclipping
	if (wobj->flags & WOBJ_HAS_PLAYER_FINISHED) {
		local_data->finish_timer++;
		if ((!local_data->level_end_norank && local_data->finish_timer == PLAYER_FINISH_TIMER + 60) ||
			(local_data->level_end_norank && local_data->finish_timer == PLAYER_FINISH_TIMER)) {
			Interaction_FinishLevel(local_data->next_level_line);
		}
		//if (local_data->finish_timer > PLAYER_FINISH_TIMER) {
		//	//local_data->currframe = 0;
		//	//wobj->anim_frame = 0;
		//}
	}
	if (Game_GetVar(GAME_VAR_NOCLIP)->data.integer)
	{
		float spd = wobj->speed * 2.0f;
		if (Input_GetButton(INPUT_DROP, INPUT_STATE_PLAYING) && local_data->finish_timer <= 0)
			spd *= 3.0f;
		if (Input_GetButton(INPUT_FIRE, INPUT_STATE_PLAYING) && local_data->finish_timer <= 0)
			spd *= 3.0f;
		if (local_data->finish_timer == 0 || local_data->finish_timer > PLAYER_FINISH_TIMER) {
			if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING))
				wobj->x += spd;
			if (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING))
				wobj->y += spd;
			if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING))
				wobj->x -= spd;
			if (Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING))
				wobj->y -= spd;
		}
		return;
	}
	if (local_data->finish_timer == 0 || local_data->finish_timer > PLAYER_FINISH_TIMER || local_data->level_end_norank) {
		local_data->lock_controls = CNM_FALSE;
	} else {
		local_data->lock_controls = CNM_TRUE;
	}
	if (local_data->finish_timer == 1 && (wobj->flags & WOBJ_HAS_PLAYER_FINISHED) && !local_data->level_end_norank) {
		int par = Game_GetVar(GAME_VAR_PAR_SCORE)->data.integer;
		local_data->level_end_score = local_data->score;
		local_data->level_end_time_score = 20 - (local_data->final_time_forscore / (30 * 60));
		if (local_data->level_end_time_score < 0) local_data->level_end_time_score = 0;
		local_data->level_end_time_score *= 100 * 12;
		local_data->level_end_rank = (int)((float)(local_data->level_end_score + local_data->level_end_time_score) / (float)par * 4.0);
		if (local_data->level_end_rank > 4) local_data->level_end_rank = 4;
		if (local_data->level_end_rank < 0) local_data->level_end_rank = 0;
		if ((!Game_GetVar(GAME_VAR_NOSAVE)->data.integer || Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) &&
			Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER && !Game_GetVar(GAME_VAR_FORCE_NOSAVE)->data.integer) {
			// Save rank and time
			int id = -1;
			for (int i = 0; i < globalsave_get_num_levels(&g_globalsave); i++) {
				if (strcmp(g_globalsave.levels_found[i], Game_GetVar(GAME_VAR_LEVEL)->data.string) == 0) {
					id = i;
					break;
				}
			}
			if (id != -1) {
				if (local_data->level_end_rank > g_globalsave.best_ranks[id]) {
					g_globalsave.best_ranks[id] = local_data->level_end_rank;
				}
				int secs = local_data->final_time_forscore / 30;
				if (secs < g_globalsave.best_times[id]) {
					g_globalsave.best_times[id] = secs;
				}
				Console_Print("Saving new best times and ranks...");
				globalsave_save_override(&g_globalsave);
			}
		}
	}
	// Give the player lives
	if ((wobj->flags & WOBJ_HAS_PLAYER_FINISHED) &&
		!Game_GetVar(GAME_VAR_NOSAVE)->data.integer &&
		!Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer &&
		!local_data->level_end_norank) {
		const static int rank_lives[] = { 0, 0, 1, 5, 10 };
		for (int i = 0; i < rank_lives[local_data->level_end_rank]; i++) {
			if (local_data->finish_timer == PLAYER_FINISH_TIMER / 4 + 30 + i * 15) {
				Interaction_PlaySound(wobj, 55);
				g_saves[g_current_save].lives++;
			}
		}
	}
	
	local_data->pvp_damage_indicator_timer--;
	if (local_data->death_cam_timer-- == 0)
	{
		if (g_saves[g_current_save].lives > 0) {
			Camera_SetForced(CNM_FALSE);
			Camera_TeleportPos((int)wobj->x + 16, (int)wobj->y + 16);
			wobj->flags &= ~WOBJ_INVULN;
		} else {
			Interaction_GameOver();
			return;
		}
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
		if (local_data->death_limbo_counter == 30) {
			_hud_player_y = -320.0f;
			_hud_player_yvel = 0.0f;
		}
		return;
	}
	wobj->flags &= ~WOBJ_PLAYER_IS_RESPAWNING;
	if (local_data->death_limbo_counter == 0) {
		Interaction_CreateWobj(WOBJ_TELE_PARTICLES, wobj->x, wobj->y, 0, 0.0f);
		wobj->flags &= ~WOBJ_DONT_DRAW;
		g_can_pause = CNM_TRUE;
	}

	float accel = 1.3f;
	float dec = 1.0f;
	WOBJ *other;
	float final_speed;
	float final_grav;
	float final_jmp;
	//float final_strength;
	//int apply_plat_velx = CNM_FALSE;//!Wobj_IsGrounded(wobj);

	final_speed = wobj->speed;
	final_jmp = wobj->jump;
	final_grav = local_data->grav + local_data->grav_add;
	//local_data->stored_plat_velx = 0.0f;

	if (local_data->upgrade_state == PLAYER_UPGRADE_MAXPOWER)
	{
		final_speed *= maxpowerinfos[local_data->mpoverride].spd;
		final_jmp *= maxpowerinfos[local_data->mpoverride].jmp;
		final_grav *= maxpowerinfos[local_data->mpoverride].grav;
	}
	if (local_data->is_sliding) {
		accel = 0.1f;
		dec = 0.2f;
	}

	// Air drag due to wings
	// if (local_data->upgrade_state == PLAYER_UPGRADE_WINGS) {
	// 	if (!Wobj_IsGrouneded(wobj)) {
	// 		if (fabsf(wobj->vel_x) > wobj->speed * 0.6f && wobj->vel_y < 4.0f) {
	// 				float decfactor = dec / 2.0f;
	// 				if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING) && wobj->vel_x > 0.0f) decfactor += accel;
	// 				if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING) && wobj->vel_x < 0.0f) decfactor += accel;
	// 				if (wobj->vel_x > 0.0f) wobj->vel_x -= decfactor;
	// 				if (wobj->vel_x < 0.0f) wobj->vel_x += decfactor;
	// 				//local_data->flap_spdadd -= dec / 2.0f;
	// 		} else if (fabsf(wobj->vel_x) < wobj->speed * 2.0f && wobj->vel_y > 4.0f) {
	// 			if (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING)) {
	// 				// if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING)) wobj->vel_x += 0.05f;
	// 				// if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING)) wobj->vel_x -= 0.05f;
	// 				local_data->flap_spdadd += 0.05f + 0.1f;
	// 				wobj->vel_y *= 0.75f;
	// 			}
	// 		}
	// 	}
	// }
	if (Wobj_IsGrouneded(wobj)) {
		local_data->isflying = CNM_FALSE;
		local_data->numflaps = 0;
	}
	if ((local_data->upgrade_state == PLAYER_UPGRADE_WINGS ||
		local_data->upgrade_state == PLAYER_UPGRADE_CRYSTAL_WINGS) && !Wobj_IsGrouneded(wobj)) {
		//if (local_data->isdiving) local_data->slide_jump_cooldown = 3;
		if (wobj->vel_y > 1.0f && wobj->vel_y < 9.0f && local_data->isflying) {
			final_speed += wobj->vel_y / 3.0f;
			if (!Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING)) wobj->vel_y *= 0.85f;
		}
		if (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING) && 
			!Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING) &&
			wobj->vel_y > 1.0f) {
			if (Game_GetFrame() % 30 == 0) Interaction_PlaySound(wobj, 62);
			local_data->isdiving = CNM_TRUE;
			wobj->vel_y += 0.2f;
			if (fabsf(wobj->vel_x) < wobj->speed * 3.0f) {
				const float spdadd = local_data->upgrade_state == PLAYER_UPGRADE_CRYSTAL_WINGS ? 0.5f : 0.2f;
				if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING)) wobj->vel_x += spdadd;
				if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING)) wobj->vel_x -= spdadd;
			}
			local_data->saved_diving_vel = local_data->upgrade_state == PLAYER_UPGRADE_CRYSTAL_WINGS ? wobj->vel_y * 1.25f : wobj->vel_y / 2.0f;
		}
	}
	if (Wobj_IsGrouneded(wobj) && local_data->isdiving && local_data->upgrade_state != PLAYER_UPGRADE_CRYSTAL_WINGS) {
		if (wobj->vel_x > wobj->speed * 1.5f) wobj->vel_x = wobj->speed * 1.5f;
		if (wobj->vel_x < wobj->speed * -1.5f) wobj->vel_x = wobj->speed * -1.5f;
	}
	if (Wobj_IsGrouneded(wobj)) {
		local_data->isdiving = CNM_FALSE;
		local_data->saved_diving_vel = 0.0f;
	}
	//final_speed -= local_data->flap_spdadd;
	//local_data->flap_spdadd -= 0.05f;
	//if (Wobj_IsGrouneded(wobj)) local_data->flap_spdadd = 0.0f;
	//if (local_data->flap_spdadd < 0.0f) local_data->flap_spdadd = 0.0f;
	//local_data->flap_accadd -= 0.01f;
	//if (local_data->flap_accadd < 0.0f) local_data->flap_accadd = 0.0f;

	if (local_data->animforce_cooldown_timer-- <= 0)
		local_data->animforce_cooldown = CNM_FALSE;

	int touching_ice = CNM_FALSE, is_turning = CNM_FALSE;
	float ice_friction = 1.0f;

	const int playing_jump_start = local_data->curranim == PLAYER_ANIM_JUMP && local_data->currframe < anim_lengths[local_data->currskin][PLAYER_ANIM_JUMP] - 2;
	const int playing_jump_anim = local_data->curranim == PLAYER_ANIM_JUMP || local_data->curranim == PLAYER_TURN_AIR || local_data->curranim == PLAYER_ANIM_JUMP_END;

	// Water things
	if (!local_data->vortexed_mode)
	{
		for (float icep = 0.1f; icep <= 0.9f; icep = (icep < 0.3f ? 0.5f : (icep < 0.8f ? 0.9f : 100.0f))) {
			int icex = (int)(wobj->hitbox.w * icep + wobj->hitbox.x);
			BLOCK_PROPS *ice_props = Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_BG, ((int)wobj->x + icex) / BLOCK_SIZE, (int)(wobj->y + wobj->hitbox.y + wobj->hitbox.h + 3.0f) / BLOCK_SIZE));
			if (ice_props->dmg_type != BLOCK_DMG_TYPE_ICE)
				ice_props = Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_FG, ((int)wobj->x + icex) / BLOCK_SIZE, (int)(wobj->y + wobj->hitbox.y + wobj->hitbox.h + 3.0f) / BLOCK_SIZE));
			if (ice_props->dmg_type == BLOCK_DMG_TYPE_ICE) {
				//local_data->control_mul = (float)ice_props->dmg / 100.0f;
				ice_friction = (float)ice_props->dmg / 100.0f;
				local_data->control_mul = ice_friction / 10.0f;
				dec *= ice_friction;
				accel *= ice_friction;
				touching_ice = CNM_TRUE;
				//Console_Print("%f", (float)ice_props->dmg / 100.0f);
			}
		}
		for (int icex = 0; icex <= 32; icex += 16) {
			BLOCK_PROPS *ice_props = Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_BG, ((int)wobj->x + icex) / BLOCK_SIZE, (int)(wobj->y + wobj->hitbox.y + wobj->hitbox.h + 3.0f) / BLOCK_SIZE));
			if (ice_props->dmg_type != BLOCK_DMG_TYPE_ICE)
				ice_props = Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_FG, ((int)wobj->x + icex) / BLOCK_SIZE, (int)(wobj->y + wobj->hitbox.y + wobj->hitbox.h + 3.0f) / BLOCK_SIZE));
			if (ice_props->dmg_type == BLOCK_DMG_TYPE_ICE) {
				//local_data->control_mul = (float)ice_props->dmg / 100.0f;
				ice_friction = (float)ice_props->dmg / 100.0f;
				touching_ice = CNM_TRUE;
				//Console_Print("%f", (float)ice_props->dmg / 100.0f);
			}
		}
		local_data->last_in_water = local_data->in_water;
		local_data->last_in_splash = local_data->in_splash;
		local_data->in_water = Wobj_InWater(wobj, 0, 1);
		local_data->in_splash = Wobj_InWater(wobj, 1, 0);
		if (local_data->in_water || local_data->in_splash) {
			local_data->last_water_block = Wobj_GetWaterBlockID(wobj, 1, 1);
		}
		//BLOCK_PROPS *water_props = Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_BG, (int)(wobj->x + 16.0f) / BLOCK_SIZE, (int)(wobj->y + 16.0f) / BLOCK_SIZE));
		//local_data->in_water = water_props->dmg_type == BLOCK_DMG_TYPE_LAVA;
		//if (!local_data->in_water)
		//{
		//	water_props = Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_FG, (int)(wobj->x + 16.0f) / BLOCK_SIZE, (int)(wobj->y + 16.0f) / BLOCK_SIZE));
		//	local_data->in_water = water_props->dmg_type == BLOCK_DMG_TYPE_LAVA;
		//}

		if (local_data->in_water)
		{
			accel /= 2.0f;
			dec /= 2.0f;
		}
		if (~wobj->flags & WOBJ_IS_GROUNDED) {
			accel /= 2.0f;
			dec /= 5.0f;
			if (wobj->vel_y < -1.0f) {
				final_speed *= 0.8f;
				accel *= 2.5f;
			}
			if (wobj->vel_y > -wobj->jump / 2.0f) {
				if (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING) && !local_data->lock_controls) {
					//local_data->grav_add += 0.1f;
				}
			}
		} else {
			local_data->grav_add = 0.0f;
			if (wobj->vel_y >= -1.0f) {
				if (!touching_ice) local_data->control_mul = 1.0f;
				if (local_data->sliding_cap_landing_speed && local_data->been_jumping_timer < 45) {
					if (wobj->vel_x > final_speed) wobj->vel_x -= dec / 6.0f;
					if (wobj->vel_x < -final_speed) wobj->vel_x += dec / 6.0f;
					if (wobj->vel_x <= final_speed && wobj->vel_x >= -final_speed) {
						local_data->sliding_cap_landing_speed = CNM_FALSE;
					}
				} else if (local_data->been_jumping_timer >= 45) {
					local_data->sliding_cap_landing_speed = CNM_FALSE;
				}
			}
		}
		if (local_data->in_water != local_data->last_in_water ||
			local_data->in_splash != local_data->last_in_splash) {
			float vx = wobj->vel_x, vy = wobj->vel_y;
			if (local_data->platinfo.active) {
				vx += local_data->platinfo.last_velx * 2.0f;
				vy += local_data->platinfo.last_vely * 2.0f;
			}
			float ang = atan2f(-vy, -vx);
			if ((local_data->in_water != local_data->last_in_water && !local_data->in_water)
				|| (local_data->in_splash != local_data->last_in_splash && !local_data->in_splash)) ang += CNM_PI;
			if (local_data->in_water && local_data->last_in_splash && !local_data->in_splash) ang -= CNM_PI;
			float pspd = sqrtf(vx*vx + vy*vy) * 0.5f;
			Create_Splash_Particles(
				wobj->x + 16.0f,
				wobj->y + 20.0f,
				local_data->last_water_block, 
				ang,
				pspd,
				20,
				5
			);
			Create_Splash_Particles(
				wobj->x + 16.0f + sinf(ang) * 5.0f,
				wobj->y + 20.0f + cosf(ang) * 5.0f,
				local_data->last_water_block, 
				ang,
				pspd,
				20,
				5
			);
			Create_Splash_Particles(
				wobj->x + 16.0f - sinf(ang) * 5.0f,
				wobj->y + 20.0f - cosf(ang) * 5.0f,
				local_data->last_water_block, 
				ang,
				pspd,
				20,
				5
			);
		}

		float cmul = local_data->control_mul;
		if (local_data->been_jumping_timer < 9999) local_data->been_jumping_timer++;
		if (cmul < 0.0f) cmul = 0.0f;
		if (local_data->control_mul < 1.0f) local_data->control_mul += 0.01f;
		if (local_data->control_mul > 1.0f) local_data->control_mul -= 0.01f;
		if (fabsf(local_data->control_mul - 1.0f) < 0.08f) local_data->control_mul = 1.0f;
		//Console_Print("%f", cmul);
		//local_data->control_mul += local_data->flap_accadd;

		if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING) && wobj->vel_x < final_speed && !local_data->lock_controls && !Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING))
		{
			//if (wobj->vel_x < final_speed) {
			if (!local_data->is_sliding || (wobj->vel_x < 0.0f)) {
				if (anim_lengths[local_data->currskin][PLAYER_TURN_AIR] && (~wobj->flags & WOBJ_IS_GROUNDED) && !playing_jump_start && (wobj->flags & WOBJ_HFLIP)) {
					local_data->curranim = PLAYER_TURN_AIR;
					local_data->animspd = 5;
					local_data->animtimer = 0;
				}
				if (wobj->vel_x < 0.0 && ~wobj->flags & WOBJ_IS_GROUNDED) {
					if (anim_lengths[local_data->currskin][PLAYER_TURN_AIR] && !playing_jump_start) {
						local_data->curranim = PLAYER_TURN_AIR;
						local_data->animspd = 5;
						local_data->animtimer = 0;
					}
					wobj->vel_x += accel * 2.0f * cmul;
				} else if (wobj->vel_x < 0.0) {
					if (anim_lengths[local_data->currskin][PLAYER_TURN_WALK] && !playing_jump_anim) {
						local_data->curranim = PLAYER_TURN_WALK;
						local_data->animspd = 5;
						local_data->animtimer = 0;
					}
					wobj->vel_x += accel * 3.0f * cmul;
				} else {
					wobj->vel_x += accel * cmul;
				}
			}
			//}
			//if (Input_GetButtonReleased(INPUT_LEFT, INPUT_STATE_PLAYING) ||
			//	Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING))
			//	wobj->vel_x = final_speed;

			//if (wobj->vel_x <= accel && wobj->vel_x >= -accel)
			//	wobj->vel_x = final_speed;
			//else
			//	wobj->vel_x += accel;

			//if (wobj->vel_x < 0.0f)
			//	wobj->vel_x += accel * 2.0f;
			wobj->flags &= ~WOBJ_HFLIP;
		}
		if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING) && wobj->vel_x > -final_speed && !local_data->lock_controls && !Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING))
		{
			//if (wobj->vel_x > -final_speed) {
			if (!local_data->is_sliding || (wobj->vel_x > 0.0f)) {
				if (anim_lengths[local_data->currskin][PLAYER_TURN_AIR] && (~wobj->flags & WOBJ_IS_GROUNDED) && !playing_jump_start && (~wobj->flags & WOBJ_HFLIP)) {
					local_data->curranim = PLAYER_TURN_AIR;
					local_data->animspd = 5;
					local_data->animtimer = 0;
				}
				if (wobj->vel_x > 0.0 && ~wobj->flags & WOBJ_IS_GROUNDED) {
					if (anim_lengths[local_data->currskin][PLAYER_TURN_AIR] && !playing_jump_start) {
						local_data->curranim = PLAYER_TURN_AIR;
						local_data->animspd = 5;
						local_data->animtimer = 0;
					}
					wobj->vel_x -= accel * 2.0f * cmul;
				} else if (wobj->vel_x > 0.0) {
					if (anim_lengths[local_data->currskin][PLAYER_TURN_WALK] && !playing_jump_anim) {
						local_data->curranim = PLAYER_TURN_WALK;
						local_data->animspd = 5;
						local_data->animtimer = 0;
					}
					wobj->vel_x -= accel * 3.0f * cmul;
				} else {
					wobj->vel_x -= accel * cmul;
				}
			}
			//}
			//if (Input_GetButtonReleased(INPUT_RIGHT, INPUT_STATE_PLAYING) ||
			//	Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING))
			//	wobj->vel_x = -final_speed;

			//if (wobj->vel_x <= accel && wobj->vel_x >= -accel)
			//	wobj->vel_x = -final_speed;
			//else
			//	wobj->vel_x -= accel;

			//if (wobj->vel_x > 0.0f)
			//	wobj->vel_x -= accel * 2.0f;
			wobj->flags |= WOBJ_HFLIP;
		}
		
		is_turning = (local_data->curranim == PLAYER_TURN_AIR || local_data->curranim == PLAYER_TURN_WALK) && local_data->animtimer < TURN_ANIM_TIME;

		if (complex_skins[local_data->currskin] && (local_data->curranim == PLAYER_ANIM_JUMP || local_data->curranim == PLAYER_TURN_AIR) && Wobj_IsGrouneded(wobj))
		{
			Interaction_PlaySound(wobj, 60);
			local_data->curranim = PLAYER_ANIM_JUMP_END;
			local_data->currframe = 0;
			local_data->animspd = 4;
			local_data->animtimer = 0;
		} else if (local_data->curranim == PLAYER_ANIM_JUMP && Wobj_IsGrouneded(wobj)) {
			Interaction_PlaySound(wobj, 60);
		}

		if ((Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING) || Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING)) &&
			!local_data->animforce_cooldown && Wobj_IsGrouneded(wobj) && !local_data->lock_controls && fabsf(wobj->vel_x) > 0.01f)
		{
			if (local_data->curranim != PLAYER_ANIM_JUMP_END && !is_turning)
			{
				local_data->curranim = PLAYER_ANIM_WALKING;
				local_data->animspd = 8 - (int)fabsf(wobj->vel_x);
				if (local_data->animspd < 0)
					local_data->animspd = 0;
			}
		}
	
		if (!Wobj_IsGrouneded(wobj)) local_data->sliding_crit_timer = 8;
		if (local_data->sliding_crit_timer > 0) local_data->sliding_crit_timer--;
		if (local_data->sliding_jump_timer > 0) local_data->sliding_jump_timer--;
		if (local_data->slide_input_buffer > 0) local_data->slide_input_buffer--;
		if (local_data->slide_super_jump_timer > 0) local_data->slide_super_jump_timer--;
		if (local_data->skip_jumpthrough_timer > 0) local_data->skip_jumpthrough_timer--;
		if (Wobj_IsGrouneded(wobj)) local_data->slide_jump_cooldown--;
		if (Input_GetButtonPressed(INPUT_DOWN, INPUT_STATE_PLAYING)) local_data->slide_input_buffer = 5;
		if (Wobj_IsGrouneded(wobj) && (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING) || local_data->slide_input_buffer > 0) && !local_data->is_sliding && local_data->slide_jump_cooldown <= 0 && !local_data->lock_controls) {
			if ((wobj->vel_x > 2.0f || wobj->vel_x < -2.0f) || local_data->sliding_crit_timer > 0) {
				const float fast_spd = 15.0f;
				wobj->vel_x *= 1.25f;
				if (local_data->sliding_crit_timer > 0 && (Input_GetButtonPressed(INPUT_DOWN, INPUT_STATE_PLAYING) || local_data->slide_input_buffer > 0)) {
					if (wobj->flags & WOBJ_HFLIP && wobj->vel_x > -final_speed) wobj->vel_x = -final_speed;
					else if (~wobj->flags & WOBJ_HFLIP && wobj->vel_x < final_speed) wobj->vel_x = final_speed;  
					wobj->vel_x *= 1.5f;
					Interaction_PlaySound(wobj, 63);
				} else {
					wobj->vel_x = CNM_CLAMP(wobj->vel_x, -PLAYER_SLIDING_MAX_SPD, PLAYER_SLIDING_MAX_SPD);
					Interaction_PlaySound(wobj, 59);
				}
				if (fabsf(wobj->vel_x) > fast_spd) {
					if (wobj->vel_x > fast_spd) wobj->vel_x = fast_spd;
					else wobj->vel_x = -fast_spd;
				}
				local_data->slide_super_jump_timer = 0;
				local_data->slide_input_buffer = 0;
				local_data->is_sliding = CNM_TRUE;
				local_data->sliding_jump_timer = 30;
				wobj->custom_ints[1] |= PLAYER_FLAG_SLIDE_AFTERIMAGE;
				wobj->hitbox.y = 17.0f;
				wobj->hitbox.h = 15.0f;
				local_data->stored_slide_speed = fabsf(wobj->vel_x);
			}
		}
		if (Wobj_IsGrounded(wobj) && local_data->skip_jumpthrough_timer <= 0 && local_data->slide_jump_cooldown <= 0 && Input_GetButtonPressed(INPUT_DOWN, INPUT_STATE_PLAYING) && !local_data->is_sliding && fabsf(wobj->vel_x) < 0.05f) {
			local_data->skip_jumpthrough_timer = 8;
			wobj->vel_y = 2.0f;
			const float oy = wobj->y;
			wobj->y += 10.0f;
			WOBJ *plat = Wobj_GetWobjColliding(wobj, WOBJ_IS_JUMPTHROUGH);
			wobj->y = oy;
			if (plat) {
				//local_data->stored_plat_velx = plat->vel_x;
				if (plat->vel_y > 0.0f) wobj->vel_y += plat->vel_y;
				else if (plat->vel_y < -4.0f) wobj->vel_y -= 2.0f;
			}
		}
		if (local_data->skip_jumpthrough_timer <= 0) wobj->flags &= ~WOBJ_SKIP_JUMPTHROUGH;
		else wobj->flags |= WOBJ_SKIP_JUMPTHROUGH;
		if (Wobj_IsGrouneded(wobj) && !local_data->is_sliding) {
			wobj->hitbox.y = 3.0f;
			wobj->hitbox.h = 29.0f;
			wobj->custom_ints[1] &= ~PLAYER_FLAG_SLIDE_AFTERIMAGE;
			//Util_SetBox(&wobj->hitbox, 8.0f, 0.0f, 48.0f, 64.0f);
		}
		if (local_data->is_sliding) {
			if (wobj->vel_x > dec) {
				wobj->vel_x -= dec;
			} else if (wobj->vel_x < -dec) {
				wobj->vel_x += dec;
			} else {
				wobj->vel_x = 0.0f;
			}
			float absvelx = wobj->vel_x < 0.0f ? -wobj->vel_x : wobj->vel_x;
			if (absvelx < 1.5f || !Wobj_IsGrouneded(wobj)) {
				local_data->is_sliding = CNM_FALSE;
				local_data->slide_super_jump_timer = 3;
			}
		}
	
		if ((Input_GetButtonReleased(INPUT_RIGHT, INPUT_STATE_PLAYING) ||
			Input_GetButtonReleased(INPUT_LEFT, INPUT_STATE_PLAYING)) &&
			(!Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING) &&
			 !Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING)) && !local_data->is_sliding) {
			//wobj->vel_x = 0.0f;
			if (wobj->vel_x > dec) {
				wobj->vel_x -= dec * cmul;
			} else if (wobj->vel_x < -dec) {
				wobj->vel_x += dec * cmul;
			} else {
				wobj->vel_x = 0.0f;
			}
		}

		//Console_Print("%d", local_data->is_sliding);

		if (!(Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING) ||
			  Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING)) && !local_data->is_sliding)
		{
			if (wobj->vel_x > dec)
				wobj->vel_x -= dec * cmul;
			else if (wobj->vel_x < -dec)
				wobj->vel_x += dec * cmul;
			else
				wobj->vel_x = 0.0f;
			if (!local_data->animforce_cooldown && Wobj_IsGrouneded(wobj) &&
				local_data->curranim != PLAYER_ANIM_JUMP_END && !is_turning)
			{
				local_data->curranim = PLAYER_ANIM_STANDING;
				local_data->animspd = 3;
			}
		}
		//Console_Print("%d", Wobj_IsGrounded(wobj));

		if (local_data->upgrade_state == PLAYER_UPGRADE_NONE) {
			if (!Wobj_IsGrounded(wobj) && Input_GetButtonPressed(INPUT_DOWN, INPUT_STATE_PLAYING) && !(wobj->custom_ints[1] & PLAYER_FLAG_STOMPING)) {
				wobj->custom_ints[1] |= PLAYER_FLAG_STOMPING;
				if (local_data->slide_jump_cooldown < 5) local_data->slide_jump_cooldown = 5;
				else local_data->slide_jump_cooldown = 7;
				Interaction_PlaySound(wobj, 64);
			}
		}
		if (Wobj_IsGrounded(wobj)) {
			if (wobj->custom_ints[1] & PLAYER_FLAG_STOMPING) {
				Interaction_CreateWobj(WOBJ_PLAYER_STOMP_DUST, wobj->x - 16.0f, wobj->y, 0, 0.0f);
				Interaction_CreateWobj(WOBJ_PLAYER_STOMP_DUST, wobj->x + 16.0f, wobj->y, 1, 0.0f);
			}
			wobj->custom_ints[1] &= ~PLAYER_FLAG_STOMPING;
		}
		if ((wobj->custom_ints[1] & PLAYER_FLAG_STOMPING) && wobj->vel_y < 12.0f) {
			wobj->vel_y += 4.0f;
		}

		if (local_data->upgrade_state == PLAYER_UPGRADE_SHOES)
		{
			if (!Wobj_IsGrouneded(wobj))
			{
				if (!(wobj->custom_ints[1] & PLAYER_FLAG_USED_DOUBLE_JUMP) && Input_GetButtonPressed(INPUT_DOWN, INPUT_STATE_PLAYING) && !local_data->lock_controls)
				{
					wobj->vel_y = 10.0f;
					for (int i = 0; i < 32 && !Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 32.0f) && !player_is_on_spring(wobj); i++)
						wobj->y += 24.0f;
					for (int i = 0; i < 32 && !Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 1.0f) && !player_is_on_spring(wobj); i++)
						wobj->y += 1.0f;

					local_data->slide_jump_cooldown = 5;

					WOBJ *w = Interaction_CreateWobj(DEEPHOUSE_BOOT_BLAST,
													 wobj->x + 16.0f, wobj->y + 1.0f, 0, 1.0f);
					w->speed = 9.0f;
					w->strength = wobj->strength + 0.1f;
					w = Interaction_CreateWobj(DEEPHOUSE_BOOT_BLAST,
													 wobj->x + 16.0f, wobj->y + 1.0f, 0, -1.0f);
					w->flags |= WOBJ_HFLIP;
					w->speed = 9.0f;
					w->strength = wobj->strength + 0.1f;

					local_data->been_jumping_timer = 10;
					wobj->vel_y = 15.0f;
					wobj->custom_ints[1] |= PLAYER_FLAG_USED_DOUBLE_JUMP;
					wobj->flags |= WOBJ_IS_GROUNDED; // Set grounded flag because we're on the ground lol
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
				) || local_data->upgrade_state == PLAYER_UPGRADE_CRYSTAL_WINGS)
		{
			if (!Wobj_IsGrouneded(wobj) && Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING) && !local_data->lock_controls &&
				(local_data->upgrade_state == PLAYER_UPGRADE_CRYSTAL_WINGS || local_data->numflaps < 2))
			{
				local_data->isflying = CNM_TRUE;
				if (!local_data->isdiving || wobj->vel_y < 3.0f) {
					local_data->numflaps++;
					if (wobj->vel_y > 2.0f) wobj->vel_y = 2.0f;
					wobj->vel_y -= FLAPACCEL;
					Interaction_PlaySound(wobj, 61);
					if (local_data->upgrade_state == PLAYER_UPGRADE_CRYSTAL_WINGS) wobj->vel_y -= FLAPACCEL;
					local_data->isdiving = CNM_FALSE;
					//local_data->flap_spdadd = wobj->speed * (1.0f / 5.0f);
					//local_data->flap_accadd = 0.1f;
					//wobj->vel_x *= 0.6f;
					//local_data->saved_diving_vel = 0.0f;
					if (local_data->upgrade_state == PLAYER_UPGRADE_WINGS) {
						local_data->control_mul = 0.2f;
						if (wobj->vel_x > wobj->speed / 2.0f) wobj->vel_x = wobj->speed / 2.0f;
						else if (wobj->vel_x < -wobj->speed / 2.0f) wobj->vel_x = -wobj->speed / 2.0f;
						else if (wobj->vel_x > 0.75f) wobj->vel_x = wobj->speed / 2.5f;
						else if (wobj->vel_x < -0.75f) wobj->vel_x = -wobj->speed / 2.5f;
					} else {
						local_data->control_mul = 2.0f;
						wobj->vel_x *= 0.75f;
					}
					if (wobj->vel_y < -FLAPACCEL) wobj->vel_y = -FLAPACCEL;
				}
			}
			if (!Wobj_IsGrouneded(wobj) && Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING) && !local_data->lock_controls) {
				if (local_data->isdiving && wobj->vel_y > -local_data->saved_diving_vel) {
					if (local_data->upgrade_state == PLAYER_UPGRADE_CRYSTAL_WINGS) wobj->vel_x *= 1.0f;
					else wobj->vel_x *= 0.7f;
					wobj->vel_y -= 1.0f;
					if (wobj->vel_y <= -local_data->saved_diving_vel) {
						local_data->isdiving = CNM_FALSE;
					}
				}
			}
		}
		//else if (local_data->upgrade_state == PLAYER_UPGRADE_CRYSTAL_WINGS)
		//{
		//	if (!Wobj_IsGrouneded(wobj) && Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING) && !local_data->lock_controls)
		//	{
		//		if (fabsf(wobj->vel_x) < 15.0f)
		//			wobj->vel_x *= 1.1f;

		//		if (wobj->vel_y > -15.0f)
		//			wobj->vel_y -= final_grav * 2.0f;
		//	}
		//}
		else if (local_data->upgrade_state == PLAYER_UPGRADE_VORTEX)
		{
			if (!Wobj_IsGrouneded(wobj) && Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING) && !local_data->lock_controls)
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
		if (local_data->jump_input_buffer > 0) local_data->jump_input_buffer--;
		if (local_data->is_grounded_buffer > 0) local_data->is_grounded_buffer--;
		if (Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING) && !Wobj_IsGrouneded(wobj)) {
			local_data->jump_input_buffer = 3;
		}
		if (local_data->been_jumping_timer > 5 && wobj->vel_y > 0.0f && Wobj_IsCollidingWithBlocks(wobj, 0.0f, wobj->vel_y)) {
			// get vel x once landed
			//Console_Print("asdf %d", Game_GetFrame());
			
			const float oldy = wobj->y;
			for (int i = 0; i < 32 && !Wobj_IsCollidingWithBlocksOrObjects(wobj, 0.0f, 1.0f) && !player_is_on_spring(wobj); i++)
				wobj->y += 1.0f;
			
			const float ang = Wobj_GetGroundAngle(wobj);
			wobj->vel_x += (wobj->vel_y * -sinf(ang)) * 0.85f;
			wobj->vel_y = (wobj->vel_y * cosf(ang));
			local_data->been_jumping_timer = 0;
			wobj->y = oldy;
			wobj->flags |= WOBJ_IS_GROUNDED;
			if (fabsf(wobj->vel_x) >= final_speed + 1.0f) {
				local_data->sliding_cap_landing_speed = CNM_TRUE;
			}
		}
		if (Wobj_IsGrouneded(wobj)) {
			local_data->is_grounded_buffer = 6;
			local_data->been_jumping_timer = 0;
		}
		//Console_Print("%d", local_data->been_jumping_timer);
		//

		{
			if (Wobj_IsGrounded(wobj)) {
				const float oy = wobj->y;
				wobj->y += 10.0f;
				WOBJ *plat = Wobj_GetWobjColliding(wobj, WOBJ_IS_SOLID);
				if (!plat && !(wobj->flags & WOBJ_SKIP_JUMPTHROUGH)) plat = Wobj_GetWobjColliding(wobj, WOBJ_IS_JUMPTHROUGH);
				wobj->y = oy;
				if (plat) {
					//local_data->stored_plat_velx = plat->vel_x;
				}
			}
		}

		//Console_Print("%d", Wobj_IsGrounded(wobj));

		if (local_data->is_grounded_buffer > 0 || (local_data->upgrade_state == PLAYER_UPGRADE_NONE && local_data->in_water))//Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f) || other != NULL)
		{
			//local_data->has_hammer_jumped = CNM_FALSE;
			if (Wobj_IsGrouneded(wobj))
			{
				local_data->jumped = 0;
				//wobj->vel_y = 0.0f;
			}
			if ((local_data->jump_input_buffer > 0 || Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING)) && !local_data->lock_controls)
			{
				local_data->been_jumping_timer = 0;
				local_data->has_cut_jump = CNM_FALSE;
				local_data->jump_input_buffer = 0;
				local_data->is_grounded_buffer = 0;
				float jmp_speed = final_jmp;
				if (local_data->in_water)
					jmp_speed /= 1.5f;
				local_data->jump_init_yspd = 0.0f;
				if (local_data->platinfo.active)
				{
					jmp_speed -= local_data->platinfo.last_vely;
					local_data->jump_init_yspd = local_data->platinfo.last_vely;
					float vy = wobj->vel_y;
					player_launch_from_platinfo(wobj);
					wobj->vel_y = vy;
					//wobj->vel_x += plat->vel_x;
					// Look further down for vel_x change
					//apply_plat_velx = CNM_TRUE;
				}
				if (local_data->in_water) {
					if (wobj->custom_ints[1] & PLAYER_FLAG_STOMPING) {
						Interaction_CreateWobj(WOBJ_PLAYER_STOMP_DUST, wobj->x - 16.0f, wobj->y, 0, 0.0f);
						Interaction_CreateWobj(WOBJ_PLAYER_STOMP_DUST, wobj->x + 16.0f, wobj->y, 1, 0.0f);
					}
					wobj->custom_ints[1] &= ~PLAYER_FLAG_STOMPING;
				}
				const float ang = Wobj_GetGroundAngle(wobj);
				//wobj->vel_y = -jmp_speed * cosf(ang);
				//wobj->vel_x += -jmp_speed * sinf(ang);
				wobj->vel_y = (-jmp_speed * cosf(ang)) + (-wobj->vel_x * sinf(ang));
				wobj->vel_x = (-jmp_speed * sinf(ang)) + (wobj->vel_x * cosf(ang));
				if (fabsf(wobj->vel_x) > final_speed && !(ang <= 0.1f || ang >= 359.9f)) {
					local_data->sliding_cap_landing_speed = CNM_TRUE;
				}
				local_data->jumped = JUMP_TIMER;
				local_data->animtimer = 0;
				Interaction_PlaySound(wobj, 58);
				if (local_data->slide_super_jump_timer > 0 ||
					(local_data->is_sliding && fabsf(wobj->vel_x) < 3.0f)) {
					local_data->is_sliding = CNM_FALSE;
					local_data->control_mul = 0.75f;
					wobj->vel_x *= 0.0f;
					if (wobj->vel_y < -11.7) wobj->vel_y *= 1.1f;
					else wobj->vel_y = -11.7;
				} else if (local_data->is_sliding) {
					local_data->is_sliding = CNM_FALSE;
					if (local_data->sliding_jump_timer > 20) {
						local_data->slide_jump_cooldown = 10;
						local_data->control_mul = 0.0f;
						wobj->vel_x *= 1.1f;
						wobj->vel_y = -6.4f;
					} else if (local_data->sliding_jump_timer > 10) {
						local_data->slide_jump_cooldown = 5;
						local_data->control_mul = 0.25f;
						wobj->vel_x *= 1.0f;
						wobj->vel_y = -8.5f;
					} else {
						local_data->slide_jump_cooldown = 0;
						local_data->control_mul = 0.5f;
					}
					if (local_data->sliding_jump_timer > 15) local_data->sliding_cap_landing_speed = CNM_TRUE;
					//if (wobj->vel_x > -2.5f && wobj->vel_x < 2.5f) {
					//	wobj->vel_y *= 1.25f;
					//	wobj->vel_x *= 0.1f;
					//	local_data->control_mul = 0.5f;
					//} else if (local_data->sliding_jump_timer > 0) {
					//	wobj->vel_x *= 1.1f;
					//	wobj->vel_y *= 0.5f;
					//	local_data->control_mul = -0.5f;
					//	local_data->sliding_cap_landing_speed = CNM_TRUE;
					//} else {
					//	wobj->vel_x *= 0.9f;
					//	wobj->vel_y *= 0.9f;
					//	local_data->control_mul = 0.0f;
					//	//if (wobj->vel_x < -final_speed) wobj->vel_x = -final_speed;
					//	//else if (wobj->vel_x > final_speed) wobj->vel_x = final_speed;
					//}
				}
			}
		}

		// Max power double jumping
		if (!Wobj_IsGrouneded(wobj))
		{
			if (Input_GetButtonPressed(INPUT_UP, INPUT_STATE_PLAYING) &&
				local_data->upgrade_state == PLAYER_UPGRADE_MAXPOWER &&
				maxpowerinfos[local_data->mpoverride].ability == 1 &&
				!(wobj->custom_ints[1] & PLAYER_FLAG_USED_DOUBLE_JUMP) && !local_data->lock_controls)
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
		if (local_data->jumped > 0 && !local_data->has_cut_jump && !Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING) && wobj->vel_y < (final_jmp / -2.0f && !local_data->lock_controls))
		{
			//wobj->vel_y = (final_jmp - local_data->jump_init_yspd) / -2.0f;
			local_data->has_cut_jump = CNM_TRUE;
			float time = ((float)(JUMP_TIMER) - (float)(local_data->jumped));
			if (time < 0.0f) time = 0.0f;
			const float virt_y = -0.25f * time*time - final_jmp * time;
			float delta_s = (-72.0f+(local_data->jump_init_yspd)) - virt_y;
			if (local_data->jumped > 3) {
				delta_s = (-40.0f+(local_data->jump_init_yspd)) - virt_y;
				//Console_Print("small");
			}// else Console_Print("medium");
			const float newvel = -sqrtf(-delta_s);
			if (delta_s > 0.0f) {
				wobj->vel_y = (final_jmp - local_data->jump_init_yspd) / -2.0f;
			} else {
				wobj->vel_y = newvel + local_data->jump_init_yspd;
			}
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
	if (other != NULL && !has_used_dialog(other->node_id, other->uuid)) {
		EndingText_Start(other->custom_ints[0] & 0x00ffffff, other->custom_ints[1]);
		if (other->custom_ints[0] & 0xff000000) {
			_used_dialogs_node[_ud_idx] = other->node_id;
			_used_dialogs_uuid[_ud_idx] = other->uuid;
			_ud_idx = (_ud_idx + 1) % MAX_USED_DIALOGS;
		}
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_DIALOGE_BOX_TRIGGER);
	if (other != NULL && !has_used_dialog(other->node_id, other->uuid)) {
		Dialoge_Start(other->custom_ints[0] & 0x00ffffff, other->custom_ints[1]);
		if (other->custom_ints[0] & 0xff000000) {
			_used_dialogs_node[_ud_idx] = other->node_id;
			_used_dialogs_uuid[_ud_idx] = other->uuid;
			_ud_idx = (_ud_idx + 1) % MAX_USED_DIALOGS;
		}
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_GRAPHICS_CHANGE_TRIGGER);
	if (other != NULL)
	{
		char gct_buff[256];
		const char *gct_file_path = EndingText_GetLine(other->custom_ints[0]);
		if (strcmp(FileSystem_GetRegisteredGfxFile()->name, gct_file_path) != 0)
		{
			sprintf(gct_buff, "load_gfx_file %s", gct_file_path);
			Command_Execute(gct_buff, CNM_FALSE);
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
	if (other != NULL && local_data->finish_timer <= 0) {
		local_data->upgrade_state = PLAYER_UPGRADE_SHOES;
		local_data->upgradehp = 100.0f;
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_UPGRADE_WINGS);
	if (other != NULL && local_data->finish_timer <= 0) {
		local_data->upgrade_state = PLAYER_UPGRADE_WINGS;
		local_data->upgradehp = 100.0f;
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_UPGRADE_CRYSTAL_WINGS);
	if (other != NULL && local_data->finish_timer <= 0) {
		local_data->upgrade_state = PLAYER_UPGRADE_CRYSTAL_WINGS;
		local_data->upgradehp = 50.0f;
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_NOUPGRADE_TRIGGER);
	if (other != NULL && local_data->finish_timer <= 0)
		local_data->upgrade_state = PLAYER_UPGRADE_NONE;
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_MAXPOWER_RUNE);
	if (other != NULL && local_data->finish_timer <= 0)
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
	if (other != NULL && local_data->finish_timer <= 0) {
		local_data->upgrade_state = PLAYER_UPGRADE_VORTEX;
		local_data->upgradehp = 100.0f;
	}

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
		wobj->custom_ints[1] &= ~PLAYER_FLAG_STOMPING;
		other->custom_ints[0] = 15;
		Interaction_PlaySound(wobj, 33);
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_HEALTH_SET_TRIGGER);
	if (other != NULL && local_data->finish_timer <= 0)
		wobj->health = other->custom_floats[0];
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_GRAV_TRIGGER);
	if (other != NULL)
		local_data->grav = other->custom_floats[0];
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_SKIN_UNLOCK);
	//Console_Print("%d", local_data->last_touched_skin_unlock);
	if (other != NULL && !(wobj->flags & WOBJ_HAS_PLAYER_FINISHED) && Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER) {
		if (local_data->last_touched_skin_unlock != other->custom_ints[0]) {
			local_data->last_touched_skin_unlock = other->custom_ints[0];
			skin_unlock_y = -128;
			skin_unlock_timer = 30*5;
		}
		if (!Game_GetVar(GAME_VAR_NOSAVE)->data.integer && !Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
			// save to globalsave
			globalsave_visit_skin(&g_globalsave, other->custom_ints[0]);
			globalsave_save(&g_globalsave);
		}
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_PET_UNLOCK);
	//Console_Print("%d", local_data->last_touched_skin_unlock);
	if (other != NULL && !(wobj->flags & WOBJ_HAS_PLAYER_FINISHED) && Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER) {
		if (local_data->last_touched_pet_unlock != other->custom_ints[0]) {
			local_data->last_touched_pet_unlock = other->custom_ints[0];
			pet_unlock_y = -128;
			pet_unlock_timer = 30*5;
		}
		if (!Game_GetVar(GAME_VAR_NOSAVE)->data.integer && !Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
			// save to globalsave
			globalsave_visit_pet(&g_globalsave, other->custom_ints[0]);
			globalsave_save(&g_globalsave);
		}
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_FINISH_TRIGGER);
	if (other != NULL && !(wobj->flags & WOBJ_HAS_PLAYER_FINISHED)) {
		Audio_PlayMusic(26, CNM_FALSE);
		if (!Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer && Filesystem_GetLevelType(Filesystem_GetLevelIdFromFileName(Game_GetVar(GAME_VAR_LEVEL)->data.string)) == LEVEL_TYPE_NORANK) {
			local_data->level_end_norank = CNM_TRUE;
		} else {
			local_data->level_end_norank = CNM_FALSE;
		}
		local_data->final_time_forscore = Game_GetVar(GAME_VAR_LEVEL_TIMER)->data.integer;
		g_can_pause = CNM_FALSE;
		//Console_Print(EndingText_GetLine(other->custom_ints[0]));
		//strcpy(Game_GetVar(GAME_VAR_LEVEL)->data.string, "levels/");
		//strcat(Game_GetVar(GAME_VAR_LEVEL)->data.string, EndingText_GetLine(other->custom_ints[0]));
		local_data->next_level_line = other->custom_ints[0] & 0xff;
		local_data->finish_timer = 0;
		local_data->level_end_unlockable = (int)other->custom_floats[0] > 0 ? (int)other->custom_floats[0] - 1 : -1;
		char lvlbuf[48];
		//sprintf(lvlbuf, "levels/%s", EndingText_GetLine(local_data->next_level_line));
		//local_data->level_end_found_secret = Filesystem_IsLevelSecret(Filesystem_GetLevelIdFromFileName(lvlbuf));
		local_data->level_end_found_secret = other->custom_ints[0] & 0xf00;
		if (local_data->level_end_unlockable > -1 || local_data->level_end_found_secret) {
			if (local_data->level_end_unlockable > -1 && !Game_GetVar(GAME_VAR_NOSAVE)->data.integer && !Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
				sprintf(lvlbuf, "levels/%s", EndingText_GetLine(local_data->level_end_unlockable));
				globalsave_visit_level(&g_globalsave, lvlbuf);
			}
			level_end_unlockable_y = -64;
			unlockable_show = CNM_TRUE;
		} else {
			level_end_unlockable_y = -64;
			unlockable_show = CNM_FALSE;
		}
		level_end_rank_y = RENDERER_HEIGHT;
		wobj->flags |= WOBJ_HAS_PLAYER_FINISHED;
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_BGSPEED_X);
	if (other != NULL)
		Background_GetLayer(other->custom_ints[0])->speed[0] = other->custom_floats[0];
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_BGSPEED_Y);
	if (other != NULL)
		Background_GetLayer(other->custom_ints[0])->speed[1] = other->custom_floats[0];
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_KEY_REMOVER);
	if (other != NULL) {
		if (other->custom_ints[0] & 0x1) {
			if (local_data->offhand_item == ITEM_TYPE_KEY_RED) {
				local_data->offhand_item = ITEM_TYPE_NOITEM;
			}
			if (Item_GetCurrentItem()->type == ITEM_TYPE_KEY_RED) {
				Item_DestroyCurrentItem(wobj);
			}
		}
		if (other->custom_ints[0] & 0x2) {
			if (local_data->offhand_item == ITEM_TYPE_KEY_GREEN) {
				local_data->offhand_item = ITEM_TYPE_NOITEM;
			}
			if (Item_GetCurrentItem()->type == ITEM_TYPE_KEY_GREEN) {
				Item_DestroyCurrentItem(wobj);
			}
		}
		if (other->custom_ints[0] & 0x4) {
			if (local_data->offhand_item == ITEM_TYPE_KEY_BLUE) {
				local_data->offhand_item = ITEM_TYPE_NOITEM;
			}
			if (Item_GetCurrentItem()->type == ITEM_TYPE_KEY_BLUE) {
				Item_DestroyCurrentItem(wobj);
			}
		}
	}
	other = Wobj_GetWobjCollidingWithType(wobj, WOBJ_BGTRANS);
	if (other != NULL)
		Background_GetLayer(other->custom_ints[0])->transparency = (int)other->custom_floats[0];
	if (!local_data->vortexed_mode)
		WobjPhysics_ApplyWindForces(wobj);

	if (wobj->item != ITEM_TYPE_ULTRA_SWORD)
		local_data->uswords_have = 4;

	if (--local_data->beastchurger_timer == 0)
	{
		wobj->strength /= 25.0f;
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
			wobj->custom_ints[1] &= ~(PLAYER_FLAG_STOMPING | PLAYER_FLAG_SLIDE_AFTERIMAGE);
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
			&& !Wobj_IsCollidingWithBlocks(wobj, 0.0f, 0.0f) && !local_data->lock_controls)
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

	if (local_data->vortex_death && local_data->finish_timer <= 0)
	{
		wobj->health -= 100.0f;
	}

	if (!local_data->vortexed_mode)
	{
		float old_tele_pos[2] = {wobj->x, wobj->y}, new_tele_pos[2];
		if (Wobj_TryTeleportWobj(wobj, CNM_FALSE))
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
	if (wobj->health <= 0.0f && local_data->finish_timer <= 0)
	{
		Interaction_PlaySound(wobj, 47);
		local_data->in_water = CNM_FALSE;
		local_data->vortexed_mode = CNM_FALSE;
		local_data->vortex_death = CNM_FALSE;
		local_data->num_deaths++;
		wobj->custom_ints[1] &= ~PLAYER_FLAG_STOMPING;
		wobj->custom_ints[1] &= ~PLAYER_FLAG_SLIDE_AFTERIMAGE;
		wobj->health = 100.0f;
		wobj->speed = 5.0f;
		wobj->jump = 10.0f;
		local_data->platinfo.active = CNM_FALSE;
		clear_used_dialogs();
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
		//Camera_TeleportPos((int)wobj->x + 16, (int)wobj->y + 16);
		if (Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER && !Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
			g_saves[g_current_save].lives--;
		}
		if (g_saves[g_current_save].lives > 0 || Interaction_GetMode() != INTERACTION_MODE_SINGLEPLAYER || Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
			local_data->death_cam_timer = 45+20+2;
			local_data->death_limbo_counter = 20+20+45+20;
			Fadeout_FadeDeath(45, 20, 20);
		} else {
			local_data->death_cam_timer = 45+30*4+20+2;
			local_data->death_limbo_counter = 45+30*4+20+20;
			Audio_PlayMusic(27, CNM_FALSE);
			Fadeout_FadeGameOver(45, 30*4, 20, 20);
		}
		wobj->flags |= WOBJ_INVULN;
		_hud_player_y = 0.0f;
		_hud_player_yvel = -5.0f;
		wobj->vel_x = 0.0f;
		wobj->vel_y = 0.0f;
		local_data->upgradehp -= 15.0f;
		if (Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER) {
			g_can_pause = CNM_FALSE;
		}
		//local_data->item_durability -= 10.0f;
		wobj->flags |= WOBJ_PLAYER_IS_RESPAWNING;
		{
			//WOBJ *tempitem;
			//int itemtype = Item_GetCurrentItem()->type;
			Item_GetCurrentItem()->durability -= 7.5f;
			if (Item_GetCurrentItem()->durability <= 0.0f &&
				item_types[Item_GetCurrentItem()->type].max_durability > 0.1f) {
				BreakPart_CreateParts(wobj->x, wobj->y, -5.0f, item_types[Item_GetCurrentItem()->type].frames[0].x, item_types[Item_GetCurrentItem()->type].frames[0].y, 2, 2);
				Item_DestroyCurrentItem(wobj);
			}
			//float durability = Item_GetCurrentItem()->durability;
			//Item_DestroyCurrentItem(wobj);
			//if (durability > 0.0f) Item_PickupByType(wobj, itemtype, durability);
			//tempitem = Wobj_CreateOwned(WOBJ_DROPPED_ITEM, wobj->x, wobj->y, itemtype, 0.0f);
			//Item_Pickup(wobj, tempitem);
		}
	}

	if (isnan(wobj->health)) wobj->health = 100.0f;

	if (!local_data->vortexed_mode)
		WobjPhysics_EndUpdate(wobj);

	// Search for player platforms (platinfo)
	{
		if (!local_data->platinfo.active) goto search_platinfos;
		WOBJ *plat = Wobj_GetAnyWOBJFromUUIDAndNode(local_data->platinfo.node, local_data->platinfo.uuid);
		if (!plat) {
			player_launch_from_platinfo(wobj);
			goto search_platinfos;
		}
		float px, py;
		WobjCalculate_InterpolatedPos(plat, &px, &py);

		local_data->platinfo.last_velx = plat->vel_x;
		local_data->platinfo.last_vely = plat->vel_y;
		local_data->platinfo.relx += wobj->vel_x;

		wobj->x = px + local_data->platinfo.relx;
		wobj->y = py + plat->hitbox.y - wobj->hitbox.h - wobj->hitbox.y;
		wobj->vel_y = 0.0f;
		wobj->flags |= WOBJ_IS_GROUNDED;

		if (Blocks_IsCollidingWithSolid(&(CNM_BOX){
			.x = wobj->x + wobj->hitbox.x + wobj->hitbox.w / 2.0f,
			.y = wobj->y + wobj->hitbox.y + wobj->hitbox.h + 1.5f,
			.w = 1.0f,
			.h = 1.0f,
		}, CNM_TRUE)) {
			local_data->platinfo.active = CNM_FALSE;
			goto search_platinfos;
		}

		float plry = wobj->y;
		wobj->y += 2.0f;
		int not_on_platform = !Wobj_GetWobjColliding(wobj, WOBJ_IS_MOVESTAND);
		wobj->y = plry;
		if ((wobj->x + wobj->hitbox.x > px + plat->hitbox.x + plat->hitbox.w ||
			wobj->x + wobj->hitbox.x + wobj->hitbox.w < px + plat->hitbox.x) && not_on_platform) {
			player_launch_from_platinfo(wobj);
			goto search_platinfos;
		}

		//Wobj_ResolveObjectsCollision(wobj, 0, 0);
		Wobj_ResolveBlocksCollision(wobj);
	}
search_platinfos:
	{
		//if (local_data->platinfo.active) goto plat_velx_application;
		if (!Wobj_IsGrounded(wobj)) goto plat_velx_application;
		if (Wobj_IsCollidingWithBlocks(wobj, 0.0f, 3.0f)) goto plat_velx_application;
		float plry = wobj->y;
		wobj->y += 2.0f;
		WOBJ *plat = Wobj_GetWobjColliding(wobj, WOBJ_IS_MOVESTAND);
		wobj->y = plry;
		if (!plat) goto plat_velx_application;
		if (wobj->vel_y < plat->vel_y) goto plat_velx_application;

		if (local_data->platinfo.active && local_data->platinfo.node == plat->node_id && local_data->platinfo.uuid == plat->uuid) {
			goto plat_velx_application;
		}
		local_data->platinfo.active = true;
		local_data->platinfo.node = plat->node_id;
		local_data->platinfo.uuid = plat->uuid;
		float px, py;
		WobjCalculate_InterpolatedPos(plat, &px, &py);
		local_data->platinfo.relx = wobj->x - px;
	}
plat_velx_application:
	//if (local_data->platinfo.active) {
	//	Console_Print("true %d %d", local_data->platinfo.node, local_data->platinfo.uuid);
	//} else {
	//	Console_Print("false n/a n/a");
	//}
	//Console_Print("%f %f", wobj->vel_x, wobj->vel_y);
	//Console_Print("%d", Wobj_IsGrounded(wobj));

	// HPCOST from MAXPOWER upgrade
	if (local_data->upgrade_state == PLAYER_UPGRADE_MAXPOWER && local_data->finish_timer <= 0) {
		wobj->health -= maxpowerinfos[local_data->mpoverride].hpcost / 30.0f;
	}

	if (!local_data->lock_controls && local_data->finish_timer <= 0) {
		Item_TryPickupAndDrop(wobj);
	}
	Item_Update(wobj);
	Item_TryUse(wobj);

	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING) && !local_data->lock_controls) {
		Player_SwapOffhand(wobj);
	}

	int recved_lava_damage = CNM_FALSE;
	float old_hp = wobj->health;
	int recved_normal_damage = 0;
	// ability 3
	if (!local_data->ability3_hasjumped) {
		local_data->ability3_timer--;
	}
	if (local_data->ability3_timer <= 0 && local_data->finish_timer <= 0 && !Game_GetVar(GAME_VAR_GOD)->data.integer) {
		if (local_data->fire_resistance-- <= 0)
			recved_lava_damage = Interaction_WobjReceiveBlockDamage(wobj);
		if (wobj->health >= old_hp)
			recved_lava_damage = CNM_FALSE;
		recved_normal_damage = Interaction_PlayerRecieveDamage();
	}
	else {
		//if (local_data->currskin != 9)
			//local_data->curranim = PLAYER_ANIM_SHOOT;
	}
	if (wobj->health < old_hp)
	{
		wobj->flags |= WOBJ_DAMAGE_INDICATE;
		local_data->upgradehp -= 0.5f;
		if (wobj->item) {
			Item_GetCurrentItem()->durability -= 0.1f;
		}
		//local_data->item_durability -= 0.33f;
		local_data->animforce_cooldown_timer = 4;
		local_data->animforce_cooldown = CNM_TRUE;
		local_data->curranim = PLAYER_ANIM_HURT;
	}
	if (local_data->upgradehp <= 0.0f) {
		switch (local_data->upgrade_state) {
		case PLAYER_UPGRADE_WINGS:
			BreakPart_CreateParts(wobj->x - 8.0f, wobj->y - 8.0f, -6.0f, 416, 1040, 3, 3);
			break;
		case PLAYER_UPGRADE_CRYSTAL_WINGS:
			BreakPart_CreateParts(wobj->x - 8.0f, wobj->y - 8.0f, -6.0f, 320, 1040, 3, 3);
			break;
		case PLAYER_UPGRADE_VORTEX:
			BreakPart_CreateParts(wobj->x, wobj->y, -4.0f, 160, 32, 2, 2);
			break;
		case PLAYER_UPGRADE_SHOES:
			BreakPart_CreateParts(wobj->x, wobj->y, -4.0f, 400, 0, 2, 1);
			break;
		}
		local_data->upgrade_state = PLAYER_UPGRADE_NONE;
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

	if (local_data->is_sliding) {
		local_data->curranim = PLAYER_ANIM_SLIDE;
		local_data->animspd = 2;
		if (wobj->vel_x > 4.0f || wobj->vel_x < -4.0f) local_data->animspd = 1;
		if (wobj->vel_x > 6.0f || wobj->vel_x < -6.0f) local_data->animspd = 0;
	}
	if (!Wobj_IsGrouneded(wobj) && !local_data->animforce_cooldown && !is_turning)
	{
		if (local_data->curranim != PLAYER_ANIM_JUMP)
		{
			if (complex_skins[local_data->currskin])
			{
				if (wobj->vel_y < -1.0f && local_data->curranim != PLAYER_TURN_AIR)
					local_data->currframe = 0;
				else
					local_data->currframe = anim_lengths[local_data->currskin][PLAYER_ANIM_JUMP] - 2;
			}
		}
		local_data->curranim = PLAYER_ANIM_JUMP;
		local_data->animspd = local_data->currskin == 9 ? 3 : 4;
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
		af |= (pr.x / 2)&0x0ff;
		af |= ((pr.y / 2) << 8)&0x1fff00;
		if (Item_GetCurrentItem()->hide_timer > 0 || item_types[Item_GetCurrentItem()->type].draw_infront) {
			//af |= 0x400000;
			if (item_types[Item_GetCurrentItem()->type].draw_infront) {
				af |= ((3 << 21) & 0x600000);
			} else {
				if (Item_GetCurrentItem()->hide_timer >= 8) {
					af |= ((3 << 21) & 0x600000);
				} else if (Item_GetCurrentItem()->hide_timer > 0) {
					af |= (((Item_GetCurrentItem()->hide_timer >> 1) << 21) & 0x600000);
				} else {
					af |= ((0 << 21) & 0x600000);
				}
			}
		}
		af |= (local_data->currskin << 23) & 0x07800000;
		af |= (local_data->curranim << 27) & 0x78000000;
		if (complex_skins[local_data->currskin]) af |= 0x80000000;
		wobj->anim_frame = *((int *)(&af));
	}

	// Cap player health, money, and strength
	if (wobj->health > PLAYER_HP_MAX)
		wobj->health = PLAYER_HP_MAX;
	if (wobj->strength > 9.99f)
		wobj->strength = 9.99f;
	// if (wobj->money > 99999)
	// 	wobj->money = 99999;
}
int get_player_packed_skin(int anim_frame) {
	return ((anim_frame & 0x07800000) >> 23);
}
int get_player_packed_anim(int anim_frame) {
	return ((anim_frame & 0x78000000) >> 27);
}
int get_player_packed_draw_item(int anim_frame) {
	int af = ((anim_frame & 0x600000) >> 21);
	switch (af) {
	case 0: return 7;
	case 1: return 4;
	case 2: return 2;
	case 3: return 0;
	default: return 0;
	}
}
void WobjPlayer_Draw(WOBJ *wobj, int camx, int camy)
{
	CNM_RECT r;
	//CNM_RECT r;
	int skin = get_player_packed_skin(wobj->anim_frame);
	int anim = get_player_packed_anim(wobj->anim_frame);
	if ((wobj->custom_ints[1] & PLAYER_FLAG_SHOWN_UPGRADE_STATE) == PLAYER_UPGRADE_VORTEX)
	{
		Util_SetRect(&r, 128, 32, 32, 32);
		//Renderer_DrawBitmap((int)wobj->x - camx, (int)wobj->y - camy, &r, 0, RENDERER_LIGHT);
		Renderer_DrawBitmap2
		(
			(int)wobj->x - camx,
			(int)wobj->y - camy + (anim == PLAYER_ANIM_SLIDE ? 16 : 0),
			&r,
			0,
			RENDERER_LIGHT,
			wobj->flags & WOBJ_HFLIP,
			CNM_FALSE
		);
	}
	if ((wobj->custom_ints[1] & PLAYER_FLAG_SHOWN_UPGRADE_STATE) == PLAYER_UPGRADE_SHOES) {
		//Util_SetRect(&r, 256, 128, 32, 32);
		//Renderer_DrawBitmap((int)wobj->x - camx, (int)wobj->y - camy, &r, 0, RENDERER_LIGHT);
	}
	//wobj->anim_frame = custom ints[0];
	//WobjGeneric_Draw(wobj, camx, camy);
	//Util_SetRect(&r, (wobj->item % 8) * 32, 352 + (wobj->item / 8) * 32, 32, 32);
	//if (wobj->item)
	//	Renderer_DrawBitmap((int)wobj->x - camx, (int)wobj->y - camy, &r, 0, RENDERER_LIGHT);
	if ((wobj->custom_ints[1] & PLAYER_FLAG_SHOWN_UPGRADE_STATE) == PLAYER_UPGRADE_WINGS ||
		(wobj->custom_ints[1] & PLAYER_FLAG_SHOWN_UPGRADE_STATE) == PLAYER_UPGRADE_CRYSTAL_WINGS)
	{
		int iscryst = (wobj->custom_ints[1] & PLAYER_FLAG_SHOWN_UPGRADE_STATE) == PLAYER_UPGRADE_CRYSTAL_WINGS;
		const CNM_RECT frames[] = {
			{ 416,    1040, 48, 48 },	
			{ 416+48, 1040, 48, 48 },	
			{ 416,    1040+48, 48, 48 },	
			{ 416+48, 1040+48, 48, 48 },
			{ 224, 1040, 48, 48 },
			{ 224+48, 1040, 48, 48 },
		}, framesc[] = {
			{ 320,    1040, 48, 48 },	
			{ 320+48, 1040, 48, 48 },	
			{ 320,    1040+48, 48, 48 },	
			{ 320+48, 1040+48, 48, 48 },	
			{ 224,  1040+48, 48, 48 },
			{ 224+48, 1040+48, 48, 48 },
		};
		int frame = 0;
		if (wobj->vel_y > 0.0f || Wobj_IsGrouneded(wobj)) {
			int spd = 1;
			if (wobj->vel_y < 8.0f) spd = 2;
			if (wobj->vel_y < 6.0f) spd = 3;
			if (wobj->vel_y < 4.0f) spd = 4;
			if (wobj->vel_y < 2.0f) spd = 5;
			if (wobj->vel_y < 0.1f) spd = 13;
			int isdiving = wobj->vel_y > 4.0f ? 4 : 0;
			frame = (Game_GetFrame() / spd) % 2 + isdiving;
		} else if (wobj->vel_y > -FLAPACCEL / 4.0f * 1.0f) frame = 2;
		else if (wobj->vel_y > -FLAPACCEL / 4.0f * 2.0f) frame = 3;
		else if (wobj->vel_y > -FLAPACCEL / 4.0f * 3.0f) frame = 2;
		else if (wobj->vel_y > -FLAPACCEL / 4.0f * 4.0f) frame = 1;
		int flipdist = wobj->flags & WOBJ_HFLIP ? 7 : 10;
		Renderer_DrawBitmap2(
			(int)wobj->x - flipdist - camx,
			(int)wobj->y - 14 - camy - (skin == 10 ? 9 : 0) + (anim == PLAYER_ANIM_SLIDE ? 16 : 0),
			(iscryst ? framesc : frames) + frame,
			iscryst ? 1 : 0, RENDERER_LIGHT,
			wobj->flags & WOBJ_HFLIP,
			CNM_FALSE);
	}

	DrawPlayerChar(wobj, camx, camy);
	Renderer_DrawBitmap2
	(
		(int)wobj->x - camx,
		(int)wobj->y - camy - (skin == 10 ? 5 : 3) + (anim == PLAYER_ANIM_SLIDE ? 8 : 0),
		&item_types[wobj->item].frames[0],
		get_player_packed_draw_item(wobj->anim_frame),
		Blocks_GetCalculatedBlockLight((int)(wobj->x + 16.0f) / BLOCK_SIZE, (int)(wobj->y + 16.0f) / BLOCK_SIZE),
		wobj->flags & WOBJ_HFLIP,
		CNM_FALSE
	);

	const char *name = NULL;
	if (Interaction_GetMode() == INTERACTION_MODE_CLIENT ||
		Interaction_GetMode() == INTERACTION_MODE_HOSTED_SERVER)
		name = NetGame_GetNode(wobj->node_id)->name;
	else if (Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER)
		name = Game_GetVar(GAME_VAR_PLAYER_NAME)->data.string;
	Renderer_DrawText((int)wobj->x - camx - (strlen(name) * 4) + 16, (int)ceilf(wobj->y) - camy - 12, 0, RENDERER_LIGHT, name);

	switch (wobj->custom_ints[1] & PLAYER_FLAG_SHOWN_UPGRADE_STATE)
	{
	case PLAYER_UPGRADE_MAXPOWER:
		if ((Game_GetFrame() / 3) % 2 == 0)
			Util_SetRect(&r, 192, 480, 48, 48);
		else
			Util_SetRect(&r, 192, 480+48, 48, 48);
		Renderer_DrawBitmap((int)wobj->x - camx - 8, (int)wobj->y - camy - 8, &r, 2, RENDERER_LIGHT);
		break;
	// case PLAYER_UPGRADE_CRYSTAL_WINGS:
	// 	if ((Game_GetFrame() / 2) % 2 == 0)
	// 		Util_SetRect(&r, 384 - 2, 352, 48, 48);
	// 	else
	// 		Util_SetRect(&r, 384+64, 352, 48, 48);
	// 	Renderer_DrawBitmap((int)wobj->x - camx, (int)wobj->y - camy, &r, 4, RENDERER_LIGHT);
	// 	break;
	}
}

void Player_RecieveDamage(WOBJ *player, WOBJ *inflictor, float damage_taken)
{
	if (!Game_GetVar(GAME_VAR_GOD)->data.integer) player->health -= damage_taken;
}

static void StepPlayerAnimation(WOBJ *wobj)
{
	PLAYER_LOCAL_DATA *ld = (PLAYER_LOCAL_DATA *)wobj->local_data;

	static int goodanim_godir = 1;

	if (ld->animspd == 0) ld->animspd = 1;

	if (ld->currframe >= anim_lengths[ld->currskin][ld->curranim])  ld->currframe = anim_lengths[ld->currskin][ld->curranim]-1;
	ld->animtimer++;
	if (!complex_skins[ld->currskin] ||
		(ld->curranim == PLAYER_ANIM_WALKING ||
		 ld->curranim == PLAYER_ANIM_HURT))
	{
		if (ld->animtimer % ld->animspd == 0)
			ld->currframe = (ld->currframe + 1) % anim_lengths[ld->currskin][ld->curranim];
	}
	else if (ld->curranim == PLAYER_ANIM_STANDING)
	{
		if (ld->animtimer % ld->animspd == 0)
		{
			ld->currframe += goodanim_godir;
			if (ld->currframe >= anim_lengths[ld->currskin][ld->curranim]) {
				goodanim_godir = -1;
				ld->currframe = anim_lengths[ld->currskin][ld->curranim] - 2;
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
			if (ld->currframe >= anim_lengths[ld->currskin][ld->curranim])
				ld->currframe = anim_lengths[ld->currskin][ld->curranim]-2;
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
			ld->currframe = (ld->currframe + 1) % anim_lengths[ld->currskin][ld->curranim];
	}
	wobj->anim_frame = ld->currframe;
}
static void PlayerAnimGetRect(CNM_RECT *r, int skin, int anim, int frame)
{
	if (!complex_skins[skin])
	{
		r->x = skin_bases[skin][0] + anim_offsets[anim][frame][0];
		r->y = skin_bases[skin][1] + anim_offsets[anim][frame][1];
		r->w = 32;
		r->h = 32;
	}
	else
	{
		int base = 0;
		switch (anim) {
		case PLAYER_TURN_AIR:
			base += anim_lengths[skin][7];
		case PLAYER_TURN_WALK:
			base += anim_lengths[skin][6];
		case PLAYER_ANIM_SLIDE:
			base += anim_lengths[skin][5];
		case PLAYER_ANIM_HURT:
			base += anim_lengths[skin][4];
		case PLAYER_ANIM_JUMP_END:
			base += anim_lengths[skin][3] + anim_lengths[skin][2];
		case PLAYER_ANIM_JUMP:
			base += anim_lengths[skin][1];
		case PLAYER_ANIM_WALKING:
			base += anim_lengths[skin][0];
		default:
			break;
		}
		//if (anim == PLAYER_ANIM_WALKING) base = anim_lengths[skin][0];
		//if (anim == PLAYER_ANIM_JUMP) base = anim_lengths[skin][0]+ anim_lengths[skin][1];
		//if (anim == PLAYER_ANIM_JUMP_END)
		//	base = anim_lengths[skin][0]+anim_lengths[skin][1] +
		//	anim_lengths[skin][2] + anim_lengths[skin][3];
		//if (anim == PLAYER_ANIM_HURT)
		//	base = anim_lengths[skin][0]+anim_lengths[skin][1] +
		//	anim_lengths[skin][2] + anim_lengths[skin][3]+anim_lengths[skin][4];
		//if (anim == PLAYER_TURN_WALK || anim == PLAYER_TURN_AIR || anim == PLAYER_ANIM_SLIDE) {
		//	base = anim_lengths[skin][0]+anim_lengths[skin][1] +
		//	anim_lengths[skin][2] + anim_lengths[skin][3]+anim_lengths[skin][4]+anim_lengths[skin][5]+anim_lengths[skin][6];
		//	if (anim == PLAYER_TURN_AIR) base += anim_lengths[skin][7];
		//	if (anim == PLAYER_ANIM_SLIDE)
		//}
		int dx = (base+frame)%12;
		int dy = (base+frame)/12;
		r->x = dx*40;
		r->y = skin_srcbase[skin]+dy*40;
		r->w = 40;
		r->h = 40;
		//if (anim == PLAYER_ANIM_SLIDE) {
			//r->x = 40 + frame * 40;
			//r->y = skin_srcbase[skin]+120;//4728;
		//}
	}
}
CNM_RECT get_player_src_rect(int anim_frame, int *skin9offsetx, int *skin9offsety) {
	CNM_RECT pr;

	unsigned int af = *((unsigned int *)(&anim_frame));
	pr.w = 32;
	pr.h = 32;
	pr.x = ((af&0xff)) * 2;
	pr.y = ((af&0x1fff00) >> 8) * 2;
	
	if (skin9offsetx) *skin9offsetx = 0;
	if (skin9offsety) *skin9offsety = 0;
	if (af & 0x80000000) {
		
		//if (pr.y >= 4000) {
			if (skin9offsetx) *skin9offsetx = -5;
			if (skin9offsety) *skin9offsety = -8;
			pr.w = 40;
			pr.h = 40;
		//}
	}
	return pr;
}
static void DrawPlayerChar(WOBJ *wobj, int camx, int camy)
{
	//PLAYER_LOCAL_DATA *ld = (PLAYER_LOCAL_DATA *)wobj->local_data;
	CNM_RECT r, pr;
	float wobj_center_x, wobj_center_y;
	wobj_center_x = wobj->x + 16.0f; //wobj->x + (float)(wobj_types[wobj->type].frames[wobj->anim_frame].w / 2);
	wobj_center_y = wobj->y + 16.0f; //wobj->y + (float)(wobj_types[wobj->type].frames[wobj->anim_frame].h / 2);
	
	int skin9offsetx, skin9offsety;
	pr = get_player_src_rect(wobj->anim_frame, &skin9offsetx, &skin9offsety);
	if (wobj->custom_ints[1] & (PLAYER_FLAG_STOMPING | PLAYER_FLAG_SLIDE_AFTERIMAGE)) {
		if (wobj->custom_ints[1] & PLAYER_FLAG_STOMPING) {
			Renderer_DrawBitmap2
			(
				(int)(wobj->x - wobj->vel_x) - camx + skin9offsetx,
				(int)(wobj->y - wobj->vel_y) - camy + skin9offsety,
				&pr,
				4,
				Wobj_DamageLighting(wobj, Blocks_GetCalculatedBlockLight((int)wobj_center_x / BLOCK_SIZE, (int)wobj_center_y / BLOCK_SIZE)),
				wobj->flags & WOBJ_HFLIP,
				wobj->flags & WOBJ_VFLIP
			);
		}
		if (!(wobj->custom_ints[1] & PLAYER_FLAG_SLIDE_AFTERIMAGE) || fabsf(wobj->vel_x) > wobj->speed * (2.0f / 3.0f) || (~wobj->flags & WOBJ_IS_GROUNDED)) {
			Renderer_DrawBitmap2
			(
				(int)(wobj->x - wobj->vel_x * 0.5f) - camx + skin9offsetx,
				(int)(wobj->y - wobj->vel_y * 0.5f) - camy + skin9offsety,
				&pr,
				(wobj->custom_ints[1] & PLAYER_FLAG_STOMPING) ? 4 : 3,
				Wobj_DamageLighting(wobj, Blocks_GetCalculatedBlockLight((int)wobj_center_x / BLOCK_SIZE, (int)wobj_center_y / BLOCK_SIZE)),
				wobj->flags & WOBJ_HFLIP,
				wobj->flags & WOBJ_VFLIP
			);
		}
	}
	Renderer_DrawBitmap2
	(
		(int)wobj->x - camx + skin9offsetx,
		(int)ceilf(wobj->y) - camy + skin9offsety,
		&pr,
		0,
		Wobj_DamageLighting(wobj, Blocks_GetCalculatedBlockLight((int)wobj_center_x / BLOCK_SIZE, (int)wobj_center_y / BLOCK_SIZE)),
		wobj->flags & WOBJ_HFLIP,
		wobj->flags & WOBJ_VFLIP
	);
	//wobj->flags &= WOBJ_DAMAGE_INDICATE;

	if (wobj->flags & WOBJ_HAS_PLAYER_FINISHED) {
		Util_SetRect(&r, 192, 0, 32, 32);
		Renderer_DrawBitmap((int)wobj->x - camx + skin9offsetx, (int)wobj->y - camy + skin9offsety - 32, &r, 0, RENDERER_LIGHT);
	}
	
	if (Game_GetFrame() % 3 == 0)
		wobj->flags &= ~WOBJ_DAMAGE_INDICATE;

	if (Game_GetVar(GAME_VAR_SHOW_COLLISION_BOXES)->data.integer)
	{
		Util_SetRect(&r, (int)(wobj->x + wobj->hitbox.x) - camx, (int)(wobj->y + wobj->hitbox.y) - camy,
					 (int)wobj->hitbox.w, (int)wobj->hitbox.h);
		Renderer_DrawRect(&r, RCOL_PINK, 2, RENDERER_LIGHT);
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

	if (Game_GetVar(GAME_VAR_SHOWPOS)->data.integer || Game_GetVar(GAME_VAR_CL_POS)->data.integer)
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

static void DrawUpgradeHPBar(int x, int y, int w) {
	CNM_RECT r;

	int fine = (Game_GetFrame() / 3), coarse = (Game_GetFrame() / 10), ccoarse = (Game_GetFrame() / 5);
	int offsets[4] = {
		0, 11, 22, 11
	};
	int xoff = 0;
	for (; w > 0; w -= 32) {
		int ww = w;
		if (ww > 32) ww = 32;
		Util_SetRect(&r, 64 + (fine % 32), 1200 + offsets[coarse % 4] + (ccoarse % 5), ww, 5);
		Renderer_DrawBitmap(x+xoff-ww, y, &r, 0, RENDERER_LIGHT);
		xoff -= 32;
	}
}
static void DrawDurabilityBar(int x, int y, int w) {
	CNM_RECT r;

	int fine = (Game_GetFrame() / 3), coarse = (Game_GetFrame() / 10), ccoarse = (Game_GetFrame() / 5);
	int offsets[4] = {
		0, 11, 22, 11
	};
	int xoff = 0;
	for (; w > 0; w -= 32) {
		int ww = w;
		if (ww > 32) ww = 32;
		Util_SetRect(&r, 64 + (fine % 32), 1168 + offsets[coarse % 4] + (ccoarse % 5), ww, 5);
		Renderer_DrawBitmap(x+xoff-ww, y, &r, 0, RENDERER_LIGHT);
		xoff -= 32;
	}
}
static void DrawHealthBar(int x, int y, int w) {
	CNM_RECT r;

	int fine = (Game_GetFrame() / 3), coarse = (Game_GetFrame() / 10), ccoarse = (Game_GetFrame() / 5);
	int offsets[4] = {
		0, 11, 22, 11
	};
	int xoff = 0;
	for (; w > 0; w -= 32) {
		int ww = w;
		if (ww > 32) ww = 32;
		Util_SetRect(&r, 416 + (fine % 32), 1264 + offsets[coarse % 4] + (ccoarse % 5), ww, 5);
		Renderer_DrawBitmap(x+xoff, y, &r, 0, RENDERER_LIGHT);
		xoff += 32;
	}
}
static void DrawHealthOutline(int x, int y, int w)
{
	CNM_RECT r;

	int xoff = 0;
	for (; w > 0; w -= 32)
	{
		int ww = w;
		Util_SetRect(&r, 448, 1255, 32, 9);
		if (ww > 32) ww = 32;
		else {
			Util_SetRect(&r, 480+(32-ww), 1255, ww, 9);
		}
		Renderer_DrawBitmap(x + xoff, y, &r, 0, RENDERER_LIGHT);
		xoff += ww;
	}
	Util_SetRect(&r, 480, 1269 + (Game_GetFrame() % 3)*9, 9, 9);
	Renderer_DrawBitmap(x + xoff - 1, y, &r, 0, RENDERER_LIGHT);
}
static void DrawHealthBG(int x, int y, int w)
{
	CNM_RECT r;

	int xoff = 0;
	for (; w > 0; w -= 32)
	{
		int ww = w;
		if (ww > 32) ww = 32;
		Util_SetRect(&r, 480, 1264, ww, 5);
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

	Renderer_PlotPixel2(x, y, RCOL_WHITE, t, 0);
	Renderer_PlotPixel2(x-1, y, RCOL_WHITE, j, 0);
	Renderer_PlotPixel2(x+1, y, RCOL_WHITE, j, 0);
	Renderer_PlotPixel2(x, y-1, RCOL_WHITE, j, 0);
	Renderer_PlotPixel2(x, y+1, RCOL_WHITE, j, 0);
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

void Player_TryTitlePopup(void) {
	for (int i = 0; i < 64; i++) {
		if (strcmp(g_globalsave.levels_found[i], Game_GetVar(GAME_VAR_LEVEL)->data.string) == 0) return;
	}
	int ordernum = Filesystem_LevelIDToLevelOrderNum(Filesystem_GetLevelIdFromFileName(Game_GetVar(GAME_VAR_LEVEL)->data.string));
	if (ordernum == -1) return;
	char checkbuf[32], lvlpath[32];
	sprintf(lvlpath, "levels/_title%d.cnms", ordernum);
	strcpy(checkbuf, "");
	Serial_LoadSpawnersLevelName(lvlpath, checkbuf);
	if (strlen(checkbuf) == 0) return;
	//Console_Print("found it");
	titlepopup_y = -128;
	titlepopup_timer = 30*5;
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
	PLAYER_LOCAL_DATA *local_data = (PLAYER_LOCAL_DATA *)player->local_data;

	Renderer_SetFont(256, 192, 8, 8);

	char temp_hud[64];
	Util_SetRect(&r, 320, 1232, 96, 6);
	Renderer_DrawBitmap(0, RENDERER_HEIGHT - 64, &r, 2, RENDERER_LIGHT);
	Util_SetRect(&r, 416, 1249, 81, 5);
	Renderer_DrawBitmap(2, RENDERER_HEIGHT - 64 + 8, &r, 2, RENDERER_LIGHT);

	if (curr_hpbreak) {
		Util_SetRect(&r, 480, 1264, 32, 5);
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
			Util_SetRect(&r, 416, 1238, 93, 9);
			Renderer_DrawBitmap(0, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
			hp_break_anim = 0.0f;
		}
		else {
			Util_SetRect(&r, 416, 1238, 74, 9);
			Renderer_DrawBitmap(0, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
			Util_SetRect(&r, 416, 1255, 19, 9);
			Renderer_DrawBitmap(74, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
			DrawHealthBG(83, RENDERER_HEIGHT - 64 + 6+2, (int)hp_break_anim+3);
			DrawHealthOutline(87, RENDERER_HEIGHT - 64 + 6, (int)hp_break_anim);
		}
		hp_break_anim -= 10.0f;
		//hp_break_anim -= 0.01f;
	}
	else {
		hp_break_anim = 0;
		Util_SetRect(&r, 416, 1238, 74, 9);
		Renderer_DrawBitmap(0, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
		Util_SetRect(&r, 416, 1255, 32, 9);
		Renderer_DrawBitmap(74, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
		Util_SetRect(&r, 448, 1255, 32, 9);
		Renderer_DrawBitmap(106, RENDERER_HEIGHT - 64 + 6, &r, 0, RENDERER_LIGHT);
		if (!curr_hpbreak2) {
			Util_SetRect(&r, 503, 1255, 9, 9);
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

	Util_SetRect(&r, 224, 1264, 96, 32);
	Renderer_DrawBitmap(RENDERER_WIDTH - 96, 0, &r, 2, RENDERER_LIGHT);
	// Upgrade Health
	if (local_data->upgradehp > 0.0f && local_data->upgrade_state != PLAYER_UPGRADE_NONE) {
		DrawUpgradeHPBar(RENDERER_WIDTH - 96 + 91, 5+16, (int)(81.0f * (local_data->upgradehp / 100.0f)));
	}
	// Item Durability
	int visible_width = 81;
	if (!player->item) visible_width = 0;
	if (player->item && item_types[player->item].max_durability <= 0.1f) {
		visible_width = 81;
	}
	if (player->item && item_types[player->item].max_durability > 0.1f) {
		visible_width = (int)(Item_GetCurrentItem()->durability / item_types[player->item].max_durability * 81.0f); 
	}

	//Console_Print("%f",Item_GetCurrentItem()->durability);
	DrawDurabilityBar(RENDERER_WIDTH - 96 + 91, 6, visible_width);
	Util_SetRect(&r, 160, 960, 96, 16);
	Renderer_DrawBitmap(RENDERER_WIDTH - 96, 16, &r, 2, RENDERER_LIGHT);
	Util_SetRect(&r, 64, 960, 96, 16);
	Renderer_DrawBitmap(RENDERER_WIDTH - 96, 0, &r, 2, RENDERER_LIGHT);


	Util_SetRect(&r, 320, 1247, 96, 64-6-9);
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

	//Util_SetRect(&r, 0, 4288, 64, 32);
	//Renderer_DrawBitmap2(RENDERER_WIDTH - 64, RENDERER_HEIGHT - 32, &r, 2, RENDERER_LIGHT, 1, 1);
	Util_SetRect(&r, 320, 960, 64, 16);
	Renderer_DrawBitmap(RENDERER_WIDTH - 64, RENDERER_HEIGHT - 16, &r, 2, RENDERER_LIGHT);
	Util_SetRect(&r, 256, 960, 64, 16);
	Renderer_DrawBitmap(RENDERER_WIDTH - 64, RENDERER_HEIGHT - 32, &r, 2, RENDERER_LIGHT);

	int bx = RENDERER_WIDTH - 64, by = RENDERER_HEIGHT - 32;
	int skinoffx, skinoffy, anim = local_data->curranim, fr = local_data->currframe;
	_hud_player_y += _hud_player_yvel;
	_hud_player_yvel += 0.3f;
	if (local_data->death_limbo_counter > 30) {
		anim = PLAYER_ANIM_HURT;
		fr = 0;
	} else if (local_data->death_limbo_counter > 0) {
		if (_hud_player_y >= -0.01f && _hud_player_y <= 0.01f) {
			anim = PLAYER_ANIM_STANDING;
		}
	} else if (_hud_player_y >= 0.0f) {
		_hud_player_y = 0.0f;
		_hud_player_yvel = 0.0f;
	}
	if (_hud_player_y < -1.0f && local_data->death_limbo_counter <= 30) {
		anim = PLAYER_ANIM_JUMP;
		fr = 0;
	}
	PlayerAnimGetRect(&r, local_data->currskin, anim, fr); // Get the source rect
	get_player_src_rect(player->anim_frame, &skinoffx, &skinoffy); // Get draw offsets
	skinoffx += 34-5;
	skinoffy += 11-4 + (int)_hud_player_y;
	Renderer_DrawBitmap2(bx+skinoffx, by+skinoffy, &r, 0, RENDERER_LIGHT, 1, 0);
	Util_SetRect(&r, 0, 960, 64, 16);
	Renderer_DrawBitmap(RENDERER_WIDTH - 64, RENDERER_HEIGHT - 16, &r, 0, RENDERER_LIGHT);
	if (Interaction_GetMode() == INTERACTION_MODE_SINGLEPLAYER && !Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
		sprintf(temp_hud, "%d", g_saves[g_current_save].lives);
		Renderer_DrawText(RENDERER_WIDTH - 64+6, RENDERER_HEIGHT - 32+18, 0, RENDERER_LIGHT, temp_hud);
	} else {
		Util_SetRect(&r, 256, 200, 8, 8);
		Renderer_DrawBitmap(RENDERER_WIDTH - 64+6+2, RENDERER_HEIGHT - 32+18, &r, 0, RENDERER_LIGHT);
	}
	//sprintf(temp_hud, "%d%%%%", (int)ceilf(player->speed * 20.0f));
	//Renderer_DrawText(bx+8, by+10, 0, RENDERER_LIGHT, "SP");
	//Renderer_DrawText(bx+62 - (strlen(temp_hud)-1) * 8, by+10, 0, RENDERER_LIGHT, temp_hud);
	//sprintf(temp_hud, "%d%%%%", (int)ceilf(player->jump * 10.0f));
	//Renderer_DrawText(bx+8, by+22, 0, RENDERER_LIGHT, "JP");
	//Renderer_DrawText(bx+62 - (strlen(temp_hud) - 1) * 8, by+22, 0, RENDERER_LIGHT, temp_hud);

	// Draw score hud
	Util_SetRect(&r, 128, 1264, 96, 32);
	Renderer_DrawBitmap(0, 0, &r, 2, RENDERER_LIGHT);
	sprintf(temp_hud, "%08d", local_data->score);
	Renderer_DrawText(29, 5, 0, RENDERER_LIGHT, temp_hud);
	if (local_data->offhand_item) {
		Renderer_DrawBitmap(-2, -4, &item_types[local_data->offhand_item].frames[0], 0, RENDERER_LIGHT);
	}
	
	StepAndDrawParticles();

	// draw finish hud
	if ((player->flags & WOBJ_HAS_PLAYER_FINISHED) && local_data->finish_timer < PLAYER_FINISH_TIMER + 3 && !local_data->level_end_norank) {
		int trans = 7 - local_data->finish_timer;
		if (local_data->finish_timer > PLAYER_FINISH_TIMER - 7) trans = local_data->finish_timer - (PLAYER_FINISH_TIMER - 7);
		int trans2 = trans - 2;
		if (trans2 < 0) trans2 = 0;
		if (trans2 > 7) trans2 = 7;
		if (trans < 2) trans = 2;
		if (trans > 7) trans = 7;
		bx = RENDERER_WIDTH / 2 - 64, by = level_end_rank_y;
		const CNM_RECT rank_rects[] = {
			{ 128, 256, 32, 32 },
			{ 128, 256+32, 32, 32 },
			{ 128+32, 256, 32, 32 },
			{ 128+32, 256+32, 32, 32 },
			{ 128+64, 256, 32, 32 },
		};
		//Util_SetRect(&r, 128, 2432, 32, 32);
		if (local_data->finish_timer < PLAYER_FINISH_TIMER / 4) {
			//r.y += Util_RandInt(0, 4) * 32;
			r = rank_rects[Util_RandInt(0, 4)];
		} else {
			//r.y += local_data->level_end_rank * 32;
			r = rank_rects[local_data->level_end_rank];
		}
		Renderer_DrawBitmap(bx + 80, by + 32, &r, trans2, RENDERER_LIGHT);
		Util_SetRect(&r, 384, 896, 128, 80);
		Renderer_DrawBitmap(bx, by, &r, trans, RENDERER_LIGHT);
		Renderer_DrawText(bx + 4, by + 8, trans2, RENDERER_LIGHT, "SCORE: %d", local_data->level_end_score);
		Renderer_DrawText(bx + 4, by + 16, trans2, RENDERER_LIGHT, "TIME: %d", local_data->level_end_time_score);
		Renderer_DrawText(bx + 4, by + 24, trans2, RENDERER_LIGHT, "TOTAL: %d", local_data->level_end_score + local_data->level_end_time_score);
		Renderer_DrawText(bx + 4, by + 32, trans2, RENDERER_LIGHT, "A-RANK: %d", Game_GetVar(GAME_VAR_PAR_SCORE)->data.integer);

		int target = local_data->finish_timer > PLAYER_FINISH_TIMER - 3 ? RENDERER_HEIGHT : RENDERER_HEIGHT / 2 - 40;
		level_end_rank_y += (target - level_end_rank_y) * 0.25f;
	}

	// draw unlocked level thing
	if (unlockable_show) {
		Util_SetRect(&r, 384, 1792, 128, 48);
		Renderer_DrawBitmap2(RENDERER_WIDTH / 2, level_end_unlockable_y, &r, 2, RENDERER_LIGHT, CNM_FALSE, CNM_TRUE);
		Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - r.w, level_end_unlockable_y, &r, 2, RENDERER_LIGHT, CNM_TRUE, CNM_TRUE);
		int unlock_start = 0, secret_start = local_data->level_end_found_secret && local_data->level_end_unlockable > -1 ? 2 : 1;
		if (local_data->level_end_found_secret) {
			Renderer_DrawText(RENDERER_WIDTH / 2 - (8*23) / 2, level_end_unlockable_y + 4+(12*(0+secret_start)), 0, RENDERER_LIGHT, "FOUND A SECRET EXIT!!!!");
		}
		if (local_data->level_end_unlockable > -1) {
			Renderer_DrawText(RENDERER_WIDTH / 2 - (8*22) / 2, level_end_unlockable_y + 4+(12*(0+unlock_start)), 0, RENDERER_LIGHT, "FOUND UNLOCKABLE EXIT!");
			if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer || Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
				Renderer_DrawText(RENDERER_WIDTH / 2 - (8*32) / 2, level_end_unlockable_y + 4+(12*(1+unlock_start)), 0, RENDERER_LIGHT, "NOT UNLOCKED DUE TO LEVEL SELECT");
			} else {
				Renderer_DrawText(RENDERER_WIDTH / 2 - (8*28) / 2, level_end_unlockable_y + 4+(12*(1+unlock_start)), 0, RENDERER_LIGHT, "FIND IT IN THE LEVEL SELECT!");
			}
		}
		int target = local_data->finish_timer > PLAYER_FINISH_TIMER / 3 * 2 ? -64 : 0;
		level_end_unlockable_y += (target - level_end_unlockable_y) * 0.25f;
	}

	if (titlepopup_timer > 0) {
		Util_SetRect(&r, 400-16, 1792, 128, 48);
		Renderer_DrawBitmap2(RENDERER_WIDTH / 2, titlepopup_y, &r, 2, RENDERER_LIGHT, CNM_FALSE, CNM_TRUE);
		Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - r.w, titlepopup_y, &r, 2, RENDERER_LIGHT, CNM_TRUE, CNM_TRUE);
		Renderer_DrawText(RENDERER_WIDTH / 2 - (8*18) / 2, titlepopup_y + 4+(12*0), 0, RENDERER_LIGHT, "UNLOCKED TITLE BG!");
		if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer || Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
			Renderer_DrawText(RENDERER_WIDTH / 2 - (8*32) / 2, titlepopup_y + 4+(12*1), 0, RENDERER_LIGHT, "NOT UNLOCKED DUE TO LEVEL SELECT");
		} else {
			Renderer_DrawText(RENDERER_WIDTH / 2 - (8*25) / 2, titlepopup_y + 4+(12*1), 0, RENDERER_LIGHT, "FIND IT IN THE MAIN MENU!");
		}
		int target = titlepopup_timer < 10 ? -80 : 0;
		titlepopup_y += (target - titlepopup_y) * 0.25f;
		titlepopup_timer--;
	}

	if (skin_unlock_timer > 0) {
		Util_SetRect(&r, 400-16, 1792, 128, 48);
		Renderer_DrawBitmap2(RENDERER_WIDTH / 2, skin_unlock_y, &r, 2, RENDERER_LIGHT, CNM_FALSE, CNM_TRUE);
		Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - r.w, skin_unlock_y, &r, 2, RENDERER_LIGHT, CNM_TRUE, CNM_TRUE);
		Renderer_DrawText(RENDERER_WIDTH / 2 - (8*13) / 2, skin_unlock_y + 4+(12*0), 0, RENDERER_LIGHT, "FOUND SKIN!!!");
		if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer || Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
			Renderer_DrawText(RENDERER_WIDTH / 2 - (8*32) / 2, skin_unlock_y + 4+(12*1), 0, RENDERER_LIGHT, "NOT UNLOCKED DUE TO LEVEL SELECT");
		} else {
			Renderer_DrawText(RENDERER_WIDTH / 2 - (8*28) / 2, skin_unlock_y + 4+(12*1), 0, RENDERER_LIGHT, "FIND IT IN THE PLAYER SETUP!");
		}
		int target = skin_unlock_timer < 10 ? -80 : 0;
		skin_unlock_y += (target - skin_unlock_y) * 0.25f;
		skin_unlock_timer--;
	}

	if (pet_unlock_timer > 0) {
		Util_SetRect(&r, 400-16, 1792, 128, 48);
		Renderer_DrawBitmap2(RENDERER_WIDTH / 2, pet_unlock_y, &r, 2, RENDERER_LIGHT, CNM_FALSE, CNM_TRUE);
		Renderer_DrawBitmap2(RENDERER_WIDTH / 2 - r.w, pet_unlock_y, &r, 2, RENDERER_LIGHT, CNM_TRUE, CNM_TRUE);
		Renderer_DrawText(RENDERER_WIDTH / 2 - (8*13) / 2, pet_unlock_y + 4+(12*0), 0, RENDERER_LIGHT, "FOUND PET!!!!");
		if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer || Game_GetVar(GAME_VAR_LEVEL_SELECT_MODE)->data.integer) {
			Renderer_DrawText(RENDERER_WIDTH / 2 - (8*32) / 2, pet_unlock_y + 4+(12*1), 0, RENDERER_LIGHT, "NOT UNLOCKED DUE TO LEVEL SELECT");
		} else {
			Renderer_DrawText(RENDERER_WIDTH / 2 - (8*28) / 2, pet_unlock_y + 4+(12*1), 0, RENDERER_LIGHT, "FIND IT IN THE PLAYER SETUP!");
		}
		int target = pet_unlock_timer < 10 ? -80 : 0;
		pet_unlock_y += (target - pet_unlock_y) * 0.25f;
		pet_unlock_timer--;
	}
}
void Player_SaveData(WOBJ *player, savedata_t *data) {
	PLAYER_LOCAL_DATA *local_data = player->local_data;
	data->hp = player->health;
	data->strength = (int)(player->strength * 100.0f);
	//strcpy(data->level, Game_GetVar(GAME_VAR_LEVEL)->data.string);
	data->item = player->item;
	data->offhand = local_data->offhand_item;
	data->itemhp = Item_GetCurrentItem()->durability;
	data->offhandhp = local_data->offhand_durability;
	data->upgrade_state = local_data->upgrade_state;
	data->upgradehp = local_data->upgradehp;
}
void Player_LoadFromSave(WOBJ *player, const savedata_t *data) {
	PLAYER_LOCAL_DATA *local_data = player->local_data;
	local_data->offhand_item = data->offhand;
	local_data->offhand_durability = data->offhandhp;
	local_data->upgradehp = data->upgradehp;
	local_data->upgrade_state = data->upgrade_state;
	Item_PickupByType(player, data->item, data->itemhp);
	player->health = data->hp;
	player->strength = (float)data->strength / 100.0f;
	//strcpy();
}
void Player_ChangePet(WOBJ *wobj, int petid) {
	PLAYER_LOCAL_DATA *local_data = wobj->local_data;
	if (local_data->pet) Interaction_DestroyWobj(local_data->pet);
	if (petid != -1) {
		local_data->pet = Interaction_CreateWobj(WOBJ_PLAYER_PET, wobj->x, wobj->y, petid, 0.0f);
		local_data->pet->link_node = wobj->node_id;
		local_data->pet->link_uuid = wobj->uuid;
	} else {
		local_data->pet = NULL;
	}
}
