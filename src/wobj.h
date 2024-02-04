#ifndef _wobj_h_
#define _wobj_h_
#include <stdlib.h>
#include <stdint.h>
#include "utility.h"
#include "obj_grid.h"
#include "interaction.h"

#define WOBJ_NET_SIZE (offsetof(WOBJ, internal))

typedef struct _SPAWNER SPAWNER;

typedef enum _WOBJ_TYPES
{
	WOBJ_NULL,
	WOBJ_TELEPORT,
	WOBJ_SLIME,
	WOBJ_PLAYER,
	WOBJ_SHOTGUN_ITEM,
	WOBJ_PLAYER_PELLET,
	WOBJ_SMALL_TUNES_TRIGGER,
	WOBJ_BIG_TUNES_TRIGGER,
	WOBJ_PLAYER_SPAWN,
	WOBJ_ENDING_TEXT_SPAWNER,
	WOBJ_SCMOVING_PLATFORM,
	WOBJ_BRICK_WALL_BREAKABLE,
	WOBJ_BACKGROUND_SWITCHER,
	WOBJ_KNIFE_ATTACK,
	WOBJ_KNIFE_ITEM,
	WOBJ_APPLE_ITEM,
	WOBJ_DROPPED_ITEM,
	WOBJ_SWORD_ATTACK,
	WOBJ_GOLDEN_AXE_ATTACK,
	WOBJ_ICE_RUNE,
	WOBJ_AIR_RUNE,
	WOBJ_FIRE_RUNE,
	WOBJ_LIGHTNING_RUNE,
	WOBJ_FIRE,
	WOBJ_ICE_ATTACK,
	WOBJ_CLOUD_PLATFORM,
	WOBJ_LIGHTNING_ATTACK_PARTICLE,
	WOBJ_VEL_PLAYER_PELLET,
	WOBJ_PLAYER_LASER,
	WOBJ_ROCKET,
	WOBJ_ROCKET_EXPLOSION,
	WOBJ_PLAYER_MINIGUN_PELLET,
	WOBJ_UPGRADE_SHOES,
	WOBJ_UPGRADE_WINGS,
	WOBJ_UPGRADE_CRYSTAL_WINGS,
	WOBJ_FLYING_SLIME,
	WOBJ_HEAVY,
	WOBJ_HEAVY_BLAST,
	WOBJ_DRAGON,
	WOBJ_FIREBALL,
	WOBJ_BOZO_PIN,
	WOBJ_BOZO,
	WOBJ_SILVER_SLIME,
	LAVA_MONSTER,
	TT_MINION_SMALL,
	TT_MINION_BIG,
	SLIME_WALKER,
	MEGA_FISH,
	LAVA_DRAGON_HEAD,
	LAVA_DRAGON_BODY,
	LAVA_DRAGON_BLOB,
	TT_CHASE_TRIGGER,
	TT_NORMAL_TRIGGER,
	TT_BOSS_WAYPOINT,
	TT_BOSS,
	EATER_BUG,
	WOBJ_FLAMETHROWER_FLAME,
	SPIDER_WALKER,
	SPIDER_WALKER_WEB,
	WOBJ_SPIKE_TRAP,
	ROTATING_FIRE_COLUNM_PIECE,
	MOVING_FIRE_VERTICAL,
	MOVING_FIRE_HORIZONTAL,
	WOBJ_SUPER_DRAGON,
	WOBJ_SUPER_DRAGON_LANDING_ZONE,
	DEEPHOUSE_BOOT_BLAST,
	ULTRA_SWORD_BOOMERANG,
	HEAVY_HAMMER_SWING,
	PLAYER_HEAVY_BLAST,
	BOZO_LASER_MINION,
	BOZO_LASER_LOCKON,
	BOZO_LASER_PART,
	BOZO_MKII,
	WOBJ_CHECKPOINT,
	SPIKE_GUY,
	SPIKE_GUY_SPIKE,
	BANDIT_GUY,
	HORIZONTAL_PUSH_ZONE,
	VERTICAL_PUSH_ZONE,
	VERTICAL_WIND_ZONE,
	SMALL_HORIZONTAL_PUSH_ZONE,
	FISSION_ENERGY_BOLT,
	WOBJ_MOVING_PLATFORM_VERITCAL,
	WOBJ_DISAPEARING_PLATFORM,
	WOBJ_KAMAKAZI_SLIME,
	WOBJ_KAMAKAZI_SLIME_EXPLOSION,
	WOBJ_SPRING_BOARD,
	WOBJ_HORZ_BG_SWITCH,
	WOBJ_VERT_BG_SWITCH,
	WOBJ_VERYBIG_TUNES_TRIGGER,
	WOBJ_JUMPTHROUGH,
	WOBJ_BIG_JUMPTHROUGH,
	WOBJ_BREAKABLE_PLATFORM,
	WOBJ_SKIN_WALL_BREAKABLE,
	WOBJ_LOCKED_BLOCK_RED,
	WOBJ_LOCKED_BLOCK_GREEN,
	WOBJ_LOCKED_BLOCK_BLUE,
	ROCK_GUY_MEDIUM,
	ROCK_GUY_SMALL1,
	ROCK_GUY_SMALL2,
	ROCK_GUY_SLIDER,
	ROCK_GUY_SMASHER,
	ROCK_GUY_SPEAR,
	WOBJ_WATER_SPLASH_EFFECT,
	WOBJ_HEALTH_SET_TRIGGER,
	WOBJ_VORTEX,
	WOBJ_VORTEX_UPGRADE_TRIGGER,
	WOBJ_CUSTOMIZEABLE_MOVEABLE_PLATFORM,
	WOBJ_DIALOGE_BOX_TRIGGER,
	WOBJ_GRAPHICS_CHANGE_TRIGGER,
	WOBJ_TELE_PARTICLES,
	WOBJ_DUST_PARTICLE,
	WOBJ_BLOOD_PARTICLE,
	WOBJ_GIB_PARTICLE,
	WOBJ_MAXPOWER_RUNE,
	WOBJ_BOSS_BAR_INFO,
	WOBJ_BGSPEED_X,
	WOBJ_BGSPEED_Y,
	WOBJ_BGTRANS,
	WOBJ_TELETRIGGER1,
	WOBJ_TELEAREA1,
	WOBJ_SFX_POINT,
	WOBJ_WOLF,
	WOBJ_SUPERVIRUS,
	WOBJ_LUA0,
	WOBJ_LUA1,
	WOBJ_LUA2,
	WOBJ_LUA3,
	WOBJ_LUA4,
	WOBJ_LUA5,
	WOBJ_LUA6,
	WOBJ_LUA7,
	WOBJ_LUA8,
	WOBJ_LUA9,
	WOBJ_LUA10,
	WOBJ_LUA11,
	WOBJ_LUA12,
	WOBJ_LUA13,
	WOBJ_LUA14,
	WOBJ_LUA15,
	WOBJ_NOUPGRADE_TRIGGER,
	WOBJ_FINISH_TRIGGER,
	WOBJ_SHOTGUN_PEL,
	WOBJ_GRAV_TRIGGER,
	WOBJ_HEAVY_SHEILD_BOX,
	WOBJ_ENEMY_ROCKET,
	WOBJ_BOZO_WAYPOINT,
	WOBJ_BOZO_FIREBALL,
	WOBJ_SKIN_UNLOCK,
	WOBJ_COOLPLAT,
	WOBJ_TELEAREA2,
	WOBJ_ITEM_BREAK_PART,
	WOBJ_PLAYER_STOMP_DUST,
	WOBJ_INVIS_BLOCK,
	WOBJ_PET_UNLOCK,
	WOBJ_PLAYER_PET,
	WOBJ_KEY_REMOVER,
	WOBJ_PARTICLE,
	WOBJ_MAX
} WOBJ_TYPES;

#define WOBJ_MAX_UNOWNED_WOBJS 2048

#define WOBJ_MAX_COLLISIONS 12
#define WOBJ_IS_PLAYER		(1 << 0)
#define WOBJ_IS_HOSTILE		(1 << 1)
#define WOBJ_IS_PLAYER_WEAPON (1 << 2)
#define WOBJ_IS_ITEM		(1 << 3)
#define	WOBJ_HFLIP			(1 << 4)
#define WOBJ_VFLIP			(1 << 5)
#define WOBJ_INVULN			(1 << 6)
#define WOBJ_LIGHT_SMALL	(1 << 7)
#define WOBJ_LIGHT_BIG		(1 << 8)
#define WOBJ_IS_SOLID		(1 << 9)
#define WOBJ_IS_MOVESTAND	(1 << 10)
#define WOBJ_IS_BREAKABLE	(1 << 11)
#define WOBJ_HAS_TELEPORTED	(1 << 12)
#define WOBJ_IS_GROUNDED	(1 << 13)
#define WOBJ_IS_JUMPTHROUGH (1 << 16)
#define WOBJ_DAMAGE_INDICATE (1 << 17)
#define WOBJ_DONT_DRAW		(1 << 18)
#define WOBJ_PLAYER_IS_RESPAWNING (1 << 19)
#define WOBJ_IS_PLAYER_BULLET (1 << 20)
#define WOBJ_HAS_PLAYER_FINISHED (1 << 21)
#define WOBJ_SKIP_JUMPTHROUGH (1 << 22)
#define WOBJ_BEING_ICED (1 << 23)
#define WOBJ_WAS_SPLASH (1 << 24)
#define WOBJ_WAS_WATER (1 << 25)

#define Wobj_IsGrouneded(w) ((w)->flags & WOBJ_IS_GROUNDED)
#define Wobj_IsGrounded(w) Wobj_IsGrouneded(w)
#define Wobj_DamageLighting(w, l) (((w)->flags & WOBJ_DAMAGE_INDICATE) ? RENDERER_WHITE : (l))

#define WOBJDATA_STRUCT_DEF(name) struct name \
	{ \
		float x, y; \
		float vel_x, vel_y; \
		float custom_floats[2]; \
		int custom_ints[2]; \
		float speed, jump; \
		float strength, health; \
		int money; \
		int anim_frame; \
		int type; \
		int item; \
		int flags; \
		int last_sound_played; \
		int last_sound_uuid; \
		unsigned char link_node; \
		int link_uuid; \
		CNM_BOX hitbox; \
		unsigned char node_id; \
		int uuid; \
	};


WOBJDATA_STRUCT_DEF(wobjdata)

typedef struct _WOBJ WOBJ;
typedef struct _WOBJ_TYPE
{
	void (*create)(WOBJ *wobj);
	void (*update)(WOBJ *wobj);
	void (*draw)(WOBJ *wobj, int camx, int camy);
	INTERACTION_TAKE_DAMAGE hurt;
	CNM_RECT frames[64];
	float strength_reward;
	int money_reward;
	int do_interp;
	int respawnable;
	int score_reward;
} WOBJ_TYPE;
typedef struct _WOBJ
{
	WOBJDATA_STRUCT_DEF()
	struct
	{
		int owned;
		OBJGRID_OBJECT obj;
		struct _WOBJ *next, *last;
	} internal;

	void *local_data;
	void (*on_destroy)(WOBJ *);

	SPAWNER *parent_spawner;
	int dropped_death_item;
	int interpolate;
	int interp_frame;
	float interp_x, interp_y;
	float smooth_x, smooth_y;

	struct wobjdata history[NETGAME_MAX_HISTORY];
	int history_frames[NETGAME_MAX_HISTORY];
	uint16_t nodes_sent_to[NETGAME_MAX_NODES];
} WOBJ;

typedef struct _WOBJ_GRID_ITER
{
	WOBJ *wobj;
	OBJGRID_ITER iter;
} WOBJ_GRID_ITER;

extern WOBJ_TYPE wobj_types[WOBJ_MAX];

void Wobj_Init(void);
void Wobj_Quit(void);
void Wobj_SetNodeId(int node);
int Wobj_GetNodeId(void);
WOBJ *Wobj_CreateOwned(int type, float x, float y, int ci, float cf);
WOBJ *Wobj_CreateUnowned(int type, float x, float y, int frame, int flags, int ci, float cf, int node_id, int uuid, int run_create);
void Wobj_RecreateUnwoned(WOBJ *wobj, float tx, float ty);
void Wobj_DestroyWobj(WOBJ *wobj);
void Wobj_DestroyUnownedWobjs(void);
void Wobj_DestroyOwnedWobjs(void);
void Wobj_DestroyOwnedObjectsFromLastFrame(void);
void Wobj_UpdateOwnedWobjs(void);

void WobjDbg_CheckMemLeaks(void);

void Wobj_RecordWobjHistory(int newframe);
void Wobj_MarkForBeingSent(WOBJ *wobj, int to_node, int frame);
int Wobj_HasHistoryForNode(WOBJ *wobj, int node, int frame);
int Wobj_GetDeltaForWobj(WOBJ *output, WOBJ *input, int node, int last_frame, int current_frame);
void Wobj_GetDeltaForWobjData(struct wobjdata *output, const struct wobjdata *curr, const struct wobjdata *last);
void Wobj_UncompressDelta(struct wobjdata *output, struct wobjdata *delta, struct wobjdata *last);

void Wobj_GetCollisionsWithFlags(WOBJ *subject, WOBJ *collisions[WOBJ_MAX_COLLISIONS], int flags);
void Wobj_GetCollisionsWithType(WOBJ *subject, WOBJ *collisions[WOBJ_MAX_COLLISIONS], int type);
void Wobj_GetCollision(WOBJ *subject, WOBJ *collisions[WOBJ_MAX_COLLISIONS]);
void Wobj_DrawWobjs(int camx, int camy);
void Wobj_ResolveBlocksCollision(WOBJ *obj);
void wobj_move_and_hit_blocks(WOBJ *obj);
void WobjPhysics_BeginUpdate(WOBJ *wobj);
void WobjPhysics_EndUpdate(WOBJ *wobj);
void WobjPhysics_ApplyWindForces(WOBJ *wobj);
void Wobj_ResolveObjectsCollision(WOBJ *obj, int set_velx, int set_vely);
WOBJ *Wobj_GetOwnedWobjFromUUID(int uuid);
WOBJ *Wobj_GetAnyWOBJFromUUIDAndNode(int node, int uuid);
int Wobj_TryTeleportWobj(WOBJ *wobj, int only_telearea2);
WOBJ *Wobj_GetWobjColliding(WOBJ *wobj, int flags);
WOBJ *Wobj_GetWobjCollidingWithType(WOBJ *wobj, int type);
int Wobj_IsCollidingWithBlocks(WOBJ *wobj, float offset_x, float offset_y);
int Wobj_IsCollidingWithBlocksOrObjects(WOBJ *wobj, float offset_x, float offset_y);
void Wobj_IterateOverOwned(WOBJ **iter);
void Wobj_IterateOverDebugUnowned(WOBJ **iter);
void Wobj_InitGridIteratorOwned(WOBJ_GRID_ITER *iter, int gx, int gy);
void Wobj_GridIteratorIterate(WOBJ_GRID_ITER *iter);
void Wobj_UpdateWobjMovedPosition(WOBJ *wobj);
void Wobj_InitGridIteratorUnowned(WOBJ_GRID_ITER *iter, int gx, int gy);
void Wobj_CalculateLightForGridChunk(int gx, int gy);
void Wobj_CalculateLightForScreen(int camx, int camy);
void Wobj_UpdateGridPos(WOBJ *wobj);
void Wobj_DoEnemyCry(WOBJ *wobj, int cry_sound);
float Wobj_GetGroundAngle(const WOBJ *wobj);
void WobjCalculate_InterpolatedPos(WOBJ *wobj, float *px, float *py);
int Wobj_InWater(WOBJ *wobj, int with_splash, int with_water);
int Wobj_GetWaterBlockID(WOBJ *wobj, int with_splash, int with_water);
//void Wobj_OnDestroyLocalData(WOBJ *wobj);

void WobjGeneric_Draw(WOBJ *obj, int camx, int camy);
void WobjGenericAttack_Update(WOBJ *wobj);

void BreakPart_CreateParts(float x, float y, float yspd_start, int srcx, int srcy, int w, int h);
void Wobj_Particle_Spawn(float x, float y, CNM_RECT src, float vx, float vy, float grav, float bounce, int collide, int make_splash, int lifetime, int bounce_snd, int slowly_fade);
void Wobj_Particle_Splash_Spawn(float x, float y, int block, float vx, float vy);
void Create_Splash_Particles(float x, float y, int block, float ang, float spd, int n, int nmultiplayer);
void Wobj_DoSplashes(WOBJ *wobj);

void Wobj_NormalWobjs_ZoneAllocLocalDataPools(void);

#endif
