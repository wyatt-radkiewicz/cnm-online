#ifndef _player_h_
#define _player_h_
#include "wobj.h"

#define PLAYER_ANIM_STANDING 0
#define PLAYER_ANIM_WALKING 1
#define PLAYER_ANIM_JUMP 2
#define PLAYER_ANIM_SHOOT 3
#define PLAYER_ANIM_JUMP_START 3
#define PLAYER_ANIM_MELEE 4
#define PLAYER_ANIM_JUMP_END 4
#define PLAYER_ANIM_HURT 5
#define PLAYER_ANIM_MAX 6

#define PLAYER_HP_NORMAL_MAX 200
#define PLAYER_HP_MAX 1000

#define PLAYER_UPGRADE_NONE 0
#define PLAYER_UPGRADE_SHOES 1
#define PLAYER_UPGRADE_WINGS 2
#define PLAYER_UPGRADE_CRYSTAL_WINGS 3
#define PLAYER_UPGRADE_VORTEX 4
#define PLAYER_UPGRADE_MAXPOWER 5

#define PLAYER_FLAG_SHOWN_UPGRADE_STATE 0xF
#define PLAYER_FLAG_USED_DOUBLE_JUMP (1 << 4)
#define PLAYER_MAX_VORTEXES 3

typedef struct _PLAYER_MAXPOWER_INFO
{
	float spd, jmp, grav, strength;
	float hpcost;
	int ability;
} PLAYER_MAXPOWER_INFO;

extern PLAYER_MAXPOWER_INFO maxpowerinfos[32];

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
	int beastchurger_timer;
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
} PLAYER_LOCAL_DATA;

void WobjPlayer_Create(WOBJ *wobj);
void WobjPlayer_Update(WOBJ *wobj);
void WobjPlayer_Draw(WOBJ *wobj, int camx, int camy);
void Player_RecieveDamage(WOBJ *player, WOBJ *inflictor, float damage_taken);
void Player_PlayShootAnim(WOBJ *player);
void Player_PlayMeleeAnim(WOBJ *player);
void Player_OnRecievePVPDamage(WOBJ *player);
void Player_ResetHUD(void);
void Player_DrawHUD(WOBJ *player);

#endif