#ifndef _player_h_
#define _player_h_
#include <stdbool.h>

#include "wobj.h"
#include "savedata.h"

#define PLAYER_ANIM_STANDING 0
#define PLAYER_ANIM_WALKING 1
#define PLAYER_ANIM_JUMP 2
#define PLAYER_ANIM_SHOOT 3
#define PLAYER_ANIM_JUMP_START 3
#define PLAYER_ANIM_MELEE 4
#define PLAYER_ANIM_JUMP_END 4
#define PLAYER_ANIM_HURT 5
#define PLAYER_ANIM_SLIDE 6
#define PLAYER_TURN_WALK 7
#define PLAYER_TURN_AIR 8
#define PLAYER_ANIM_MAX 9

#define PLAYER_SLIDING_MAX_SPD (final_speed * 2.5f)

#define PLAYER_HP_NORMAL_MAX 200
#define PLAYER_HP_MAX 1000
#define PLAYER_FINISH_TIMER (30*10)

#define PLAYER_UPGRADE_NONE 0
#define PLAYER_UPGRADE_SHOES 1
#define PLAYER_UPGRADE_WINGS 2
#define PLAYER_UPGRADE_CRYSTAL_WINGS 3
#define PLAYER_UPGRADE_VORTEX 4
#define PLAYER_UPGRADE_MAXPOWER 5

#define PLAYER_FLAG_SHOWN_UPGRADE_STATE 0x7
#define PLAYER_FLAG_BEASTCHURGER (1 << 3)
#define PLAYER_FLAG_USED_DOUBLE_JUMP (1 << 4)
#define PLAYER_FLAG_STOMPING (1 << 5)
#define PLAYER_FLAG_SLIDE_AFTERIMAGE (1 << 6)
#define PLAYER_FLAG_SPECIAL (1 << 7)
#define PLAYER_MAX_VORTEXES 3

#define PLAYER_MAX_SKINS 11

#define PLAYER_POWER_LEVEL_CAP 10000
#define PLAYER_POWER_LEVEL_MUL 100.0f
#define PLAYER_SPECIAL_LEVEL_CAP 5000

typedef struct _PLAYER_MAXPOWER_INFO
{
	float spd, jmp, grav, strength;
	float hpcost;
	int ability;
} PLAYER_MAXPOWER_INFO;

extern PLAYER_MAXPOWER_INFO maxpowerinfos[32];

typedef struct player_platform_info {
	bool active;
	int node;
	int uuid;
	float last_velx;
	float last_vely;
	float relx;
} player_platform_info_t;

typedef enum hmtype {
	HM_RED,
	HM_GREEN,
	HM_WHITE,
} hmtype_t;

typedef struct hitmarker {
	bool active;
	hmtype_t type;
	float x, y, vy;
	int digits[5], ndigits;

	int state;
	int timer;
} hitmarker_t;

typedef struct _PLAYER_LOCAL_DATA
{
	int death_limbo_counter;
	int ability3_hasjumped;
	int ability3_timer;
	int mpoverride;
	int pvp_damage_indicator_timer;

	int currskin;
	int curranim;
	int currframe;
	int animspd;
	int animtimer;
	int animforce_cooldown;
	int animforce_cooldown_timer;

	int death_cam_timer;
	int in_teleport;
	int upgrade_state;
	int fire_resistance;
	int low_beast_flag;
	int uswords_have;
	int beastchurger_music;
	int checkpoint;
	int jumped;
	int last_in_water, in_water;
	int vortexed_mode;
	float vortex_speed;
	float vortex_x, vortex_y;
	float vortex_dir;
	float vortex_ang;
	int vortex_cooldown;
	int vortex_usage_timer;
	int vortex_death;

	int last_used_vortex_node;
	int last_used_vortex_uuid;

	int created_vortexes_node[PLAYER_MAX_VORTEXES];
	int created_vortexes_uuid[PLAYER_MAX_VORTEXES];
	int created_vortexes_id;
	unsigned int score;
	int offhand_item;
	//float offhand_durability;
	int finish_timer;
	float grav, grav_add;
	int is_sliding;
	int sliding_crit_timer;
	int sliding_jump_timer;
	int slide_jump_cooldown;
	float control_mul;
	int sliding_cap_landing_speed;
	int final_time_forscore;
	int lock_controls;
	//float flap_spdadd, flap_accadd;
	int isflying;
	int numflaps, isdiving, saved_diving_vel;
	//float upgradehp;
	int next_level_line;
	float jump_init_yspd;
	int level_end_rank, level_end_score, level_end_time_score;
	int level_end_unlockable, level_end_found_secret, level_end_norank;
	int last_touched_skin_unlock, last_touched_pet_unlock;
	int num_deaths;

	int jump_input_buffer, has_cut_jump, is_grounded_buffer;
	int slide_input_buffer;
	float stored_slide_speed;
	int slide_super_jump_timer;

	int been_jumping_timer;
	int skip_jumpthrough_timer;
	int last_water_block;
	int in_splash, last_in_splash;
	int touching_non_cloud_ground;
	int air_time;
	//int slope_jumped;

	float last_hp;
	float plat_velx_add;

	int power_level;
	int special_level;
	int wings_counter;

	WOBJ *pet;

	player_platform_info_t platinfo;
	//int game_over_active//, game_over_timer;
	//float item_durability;
	//float stored_yvel;
} PLAYER_LOCAL_DATA;

void player_sys_init(void);
void WobjPlayer_Create(WOBJ *wobj);
void WobjPlayer_Update(WOBJ *wobj);
void WobjPlayer_Draw(WOBJ *wobj, int camx, int camy);
void Player_RecieveDamage(WOBJ *player, WOBJ *inflictor, float damage_taken);
void Player_PlayShootAnim(WOBJ *player);
void Player_PlayMeleeAnim(WOBJ *player);
void Player_OnRecievePVPDamage(WOBJ *player);
void Player_ResetHUD(void);
void Player_DrawHUD(WOBJ *player);
void Player_SaveData(WOBJ *player, savedata_t *data);
void Player_LoadFromSave(WOBJ *player, const savedata_t *data);
void Player_SetSkinInstant(WOBJ *player, int skinid);
void Player_SwapOffhand(WOBJ *player);
void Player_TryTitlePopup(void);
void Player_ChangePet(WOBJ *player, int petid);
void player_launch_from_platinfo(WOBJ *wobj, int dovelx);
void add_hitmarker(int dmg, float x, float y);
void add_scoremarker(int score);

#endif
