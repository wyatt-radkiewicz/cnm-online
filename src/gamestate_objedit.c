#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include "console.h"
#include "renderer.h"
#include "blocks.h"
#include "serial.h"
#include "game_console.h"
#include "input.h"
#include "gui.h"
#include "command.h"
#include "wobj.h"
#include "spawners.h"
#include "game.h"
#include "teleport_infos.h"
#include "player_spawns.h"

#define NUM_ITEMS (sizeof(objedit_item_names) / sizeof(const char *))
#define NUM_INTS (sizeof(objedit_custom_ints) / sizeof(const char *))
#define NUM_FLOATS (sizeof(objedit_custom_floats) / sizeof(const char *))

static const char *objedit_custom_ints[] =
{
	"CUSTOM INT", // WOBJ_NULL
	"CUSTOM INT", // WOBJ_TELEPORT
	"CUSTOM INT", // WOBJ_SLIME
	"CUSTOM INT", // WOBJ_PLAYER
	"CUSTOM INT", // WOBJ_SHOTGUN_ITEM
	"CUSTOM INT", // WOBJ_PLAYER_PELLET
	"MUSIC ID", // WOBJ_SMALL_TUNES_TRIGGER
	"MUSIC ID", // WOBJ_BIG_TUNES_TRIGGER
	"SPAWN LOCATION ID", // WOBJ_PLAYER_SPAWN
	"STARTING LINE", // WOBJ_ENDING_TEXT_SPAWNER
	"NUM FRAMES IN DIR", // WOBJ_SCMOVING_PLATFORM
	"CUSTOM INT", // WOBJ_BRICK_WALL_BREAKABLE
	"STARTING BG LAYER", // WOBJ_BACKGROUND_SWITCHER
	"CUSTOM INT", // WOBJ_KNIFE_ATTACK
	"CUSTOM INT", // WOBJ_KNIFE_ITEM
	"CUSTOM INT", // WOBJ_APPLE_ITEM
	"ITEM ID", // WOBJ_DROPPED_ITEM
	"CUSTOM INT", // WOBJ_SWORD_ATTACK,
	"CUSTOM INT", // WOBJ_GOLDEN_AXE_ATTACK,
	"CUSTOM INT", // WOBJ_ICE_RUNE,
	"CUSTOM INT", // WOBJ_AIR_RUNE,
	"CUSTOM INT", // WOBJ_FIRE_RUNE,
	"CUSTOM INT", // WOBJ_LIGHTNING_RUNE,
	"CUSTOM INT", // WOBJ_FIRE,
	"CUSTOM INT", // WOBJ_ICE_ATTACK,
	"CUSTOM INT", // WOBJ_CLOUD_PLATFORM,
	"CUSTOM INT", // WOBJ_LIGHTNING_ATTACK_PARTICLE
	"CUSTOM INT", // WOBJ_VEL_PLAYER_PELLET,
	"CUSTOM INT", // WOBJ_PLAYER_LASER,
	"CUSTOM INT", // WOBJ_ROCKET,
	"CUSTOM INT", // WOBJ_ROCKET_EXPLOSION,
	"CUSTOM INT", // WOBJ_PLAYER_MINIGUN_PELLET,
	"CUSTOM INT", // WOBJ_UPGRADE_SHOES,
	"CUSTOM INT", // WOBJ_UPGRADE_WINGS,
	"CUSTOM INT", // WOBJ_UPGRADE_CRYSTAL_WINGS,
	"CUSTOM INT", // WOBJ_FLYING_SLIME,
	"CUSTOM INT", // WOBJ_HEAVY,
	"CUSTOM INT", // WOBJ_HEAVY_BLAST,
	"USE SPACE SPRITE", // WOBJ_DRAGON,
	"CUSTOM INT", // WOBJ_FIREBALL,
	"CUSTOM INT", // WOBJ_BOZO_PIN,
	"CUSTOM INT", // WOBJ_BOZO,
	"CUSTOM INT", // WOBJ_SILVER_SLIME,
	"FACE LEFT FLAG", // LAVA_MONSTER,
	"CUSTOM INT", // TT_MINION_SMALL,
	"CUSTOM INT", // TT_MINION_BIG,
	"CUSTOM INT", // SLIME_WALKER,
	"WATER (MIN Y VALUE)", // MEGA_FISH,
	"LENGTH (32 MAX)", // LAVA_DRAGON_HEAD,
	"CUSTOM INT", // LAVA_DRAGON_BODY,
	"CUSTOM INT", // LAVA_DRAGON_BLOB,
	"CUSTOM INT", // TT_CHASE_TRIGGER,
	"CUSTOM INT", // TT_NORMAL_TRIGGER,
	"BOSS WAYPOINT ID", // TT_BOSS_WAYPOINT,
	"CUSTOM INT", // TT_BOSS,
	"CUSTOM INT", // EATER_BUG,
	"CUSTOM INT", // WOBJ_FLAMETHROWER_FLAME,
	"CUSTOM INT", // SPIDER_WALKER,
	"CUSTOM INT", // SPIDER_WALKER_WEB,
	"CUSTOM INT", // WOBJ_SPIKE_TRAP,
	"ROT. X ORIGIN", // ROTATING_FIRE_COLUNM_PIECE,
	"DISTANCE", // MOVING_FIRE_VERTICAL,
	"DISTANCE", // MOVING_FIRE_HORIZONTAL,
	"WAYPOINT ID (16 MAX)", // WOBJ_SUPER_DRAGON,
	"CUSTOM INT", // WOBJ_SUPER_DRAGON_LANDING_ZONE,
	"CUSTOM INT", // DEEPHOUSE_BOOT_BLAST,
	"CUSTOM INT", // ULTRA_SWORD_BOOMERANG,
	"CUSTOM INT", // HEAVY_HAMMER_SWING,
	"CUSTOM INT", // PLAYER_HEAVY_BLAST,
	"CUSTOM INT", // BOZO_LASER_MINION,
	"CUSTOM INT", // BOZO_LASER_LOCKON,
	"CUSTOM INT", // BOZO_LASER_PART,
	"CUSTOM INT", // BOZO_MKII,
	"SPAWN LOC ID (START AT 256)", // WOBJ_CHECKPOINT,
	"CUSTOM INT", // SPIKE_GUY,
	"CUSTOM INT", // SPIKE_GUY_SPIKE,
	"CUSTOM INT", // BANDIT_GUY,
	"CUSTOM INT", // HORIZONTAL_PUSH_ZONE,
	"CUSTOM INT", // VERTICAL_PUSH_ZONE,
	"CUSTOM INT", // VERTICAL_WIND_ZONE,
	"CUSTOM INT", // SMALL_HORIZONTAL_PUSH_ZONE,
	"CUSTOM INT",  // FISSION_ENERGY_BOLT,
	"NUM FRAMES IN DIR",  // WOBJ_MOVING_PLATFORM_VERITCAL,
	"NUM FRAMES ALIVE",  // WOBJ_DISAPEARING_PLATFORM,
	"CUSTOM INT", // WOBJ_KAMAKAZI_SLIME,
	"CUSTOM INT", // WOBJ_KAMAKAZI_SLIME_EXPLOSION,
	"CUSTOM INT", // WOBJ_SPRING_BOARD,
	"STARTING BG LAYER", // WOBJ_HORZ_BG_SWITCH,
	"STARTING BG LAYER", // WOBJ_VERT_BG_SWITCH,
	"MUSIC ID", // WOBJ_VERYBIG_TUNES_TRIGGER,
	"CUSTOM INT", // WOBJ_JUMPTHROUGH,
	"CUSTOM INT", // WOBJ_BIG_JUMPTHROUGH,
	"GRACE PERIOD", // WOBJ_BREAKABLE_PLATFORM
	"WALL SKIN ID", // WOBJ_SKIN_WALL_BREAKABLE
	"DESTROY KEY FLAG", // WOBJ_LOCKED_BLOCK_RED
	"DESTROY KEY FLAG", // WOBJ_LOCKED_BLOCK_GREEN
	"DESTROY KEY FLAG", // WOBJ_LOCKED_BLOCK_BLUE
	"CUSTOM INT", // ROCK_GUY_MEDIUM
	"CUSTOM INT", // ROCK_GUY_SMALL1
	"FLIP OBJECT FLAG", // ROCK_GUY_SMALL2
	"CUSTOM INT", // ROCK_GUY_SLIDER
	"CUSTOM INT", // ROCK_GUY_SMASHER
	"CUSTOM INT", // ROCK_GUY_SPEAR
	"CUSTOM INT", // WOBJ_WATER_SPLASH_EFFECT
	"CUSTOM INT", // WOBJ_HEALTH_SET_TRIGGER
	"SUCK ENEMIES IN FLAG", // WOBJ_VORTEX
	"CUSTOM INT", // WOBJ_VORTEX_UPGRADE_TRIGGER
	"PACKED DATA FORMAT", // WOBJ_CUSTOMIZEABLE_MOVEABLE_PLATFORM
	"STARTING LINE", // WOBJ_DIALOGE_BOX_TRIGGER
	"GRAPHICS FILE (TEXT LINE)", // WOBJ_GRAPHICS_CHANGE_TRIGGER
	"CUSTOM INT", // WOBJ_TELE_PARTICLES
	"CUSTOM INT", // WOBJ_DUST_PARTICLE
	"CUSTOM INT", // WOBJ_BLOOD_PARTICLE
	"CUSTOM INT", // WOBJ_GIB_PARTICLE
	"SKIN OVERRIDE", // WOBJ_MAXPOWER_RUNE
	"BOSS NAME (END TEXT LINE)", // WOBJ_BOSS_BAR_INFO
	"BACKGROUND ID", // WOBJ_BGSPEED_X
	"BACKGROUND ID", // WOBJ_BGSPEED_Y
	"BACKGROUND ID", // WOBJ_BGTRANS
	"LOGIC LINK ID", // WOBJ_TELETRIGGER1
	"CUSTOM INT", // WOBJ_TELEAREA1
	"SOUND EFFECT ID", // WOBJ_SFX_POINT
	"CUSTOM INT", // WOBJ_WOLF
	"CUSTOM INT", // WOBJ_SUPERVIRUS
	"CUSTOM INT", // WOBJ_LUA0
	"CUSTOM INT", // WOBJ_LUA1
	"CUSTOM INT", // WOBJ_LUA2
	"CUSTOM INT", // WOBJ_LUA3
	"CUSTOM INT", // WOBJ_LUA4
	"CUSTOM INT", // WOBJ_LUA5
	"CUSTOM INT", // WOBJ_LUA6
	"CUSTOM INT", // WOBJ_LUA7
	"CUSTOM INT", // WOBJ_LUA8
	"CUSTOM INT", // WOBJ_LUA9
	"CUSTOM INT", // WOBJ_LUA10
	"CUSTOM INT", // WOBJ_LUA11
	"CUSTOM INT", // WOBJ_LUA12
	"CUSTOM INT", // WOBJ_LUA13
	"CUSTOM INT", // WOBJ_LUA14
	"CUSTOM INT", // WOBJ_LUA15
	"CUSTOM INT", // WOBJ_NOUPGRADE_TRIGGER

};
static const char * objedit_custom_floats[] =
{
	"CUSTOM FLOAT",          // WOBJ_NULL
	"CUSTOM FLOAT",			 // WOBJ_TELEPORT
	"CUSTOM FLOAT",			 // WOBJ_SLIME
	"CUSTOM FLOAT",			 // WOBJ_PLAYER
	"CUSTOM FLOAT",			 // WOBJ_SHOTGUN_ITEM
	"CUSTOM FLOAT",			 // WOBJ_PLAYER_PELLET
	"CUSTOM FLOAT",			 // WOBJ_SMALL_TUNES_TRIGGER
	"CUSTOM FLOAT",			 // WOBJ_BIG_TUNES_TRIGGER
	"CUSTOM FLOAT",			 // WOBJ_PLAYER_SPAWN
	"ENDING LINE PLUS ONE",	 // WOBJ_ENDING_TEXT_SPAWNER
	"SPEED", 		 // WOBJ_SCMOVING_PLATFORM
	"HEALTH", 		 // WOBJ_BRICK_WALL_BREAKABLE
	"ENDING BG LAYER", 		 // WOBJ_BACKGROUND_SWITCHER
	"CUSTOM FLOAT", 		 // WOBJ_KNIFE_ATTACK
	"CUSTOM FLOAT", 		 // WOBJ_KNIFE_ITEM
	"CUSTOM FLOAT", 		 // WOBJ_APPLE_ITEM
	"CUSTOM FLOAT", 		 // WOBJ_DROPPED_ITEM
	"CUSTOM FLOAT", // WOBJ_SWORD_ATTACK,
	"CUSTOM FLOAT", // WOBJ_GOLDEN_AXE_ATTACK,
	"CUSTOM FLOAT", // WOBJ_ICE_RUNE,
	"CUSTOM FLOAT", // WOBJ_AIR_RUNE,
	"CUSTOM FLOAT", // WOBJ_FIRE_RUNE,
	"CUSTOM FLOAT", // WOBJ_LIGHTNING_RUNE,
	"CUSTOM FLOAT", // WOBJ_FIRE,
	"CUSTOM FLOAT", // WOBJ_ICE_ATTACK,
	"CUSTOM FLOAT", // WOBJ_CLOUD_PLATFORM,
	"CUSTOM FLOAT", // WOBJ_LIGHTNING_ATTACK_PARTICLE
	"CUSTOM FLOAT", // WOBJ_VEL_PLAYER_PELLET,
	"CUSTOM FLOAT", // WOBJ_PLAYER_LASER,
	"CUSTOM FLOAT", // WOBJ_ROCKET,
	"CUSTOM FLOAT", // WOBJ_ROCKET_EXPLOSION,
	"CUSTOM FLOAT", // WOBJ_PLAYER_MINIGUN_PELLET,
	"CUSTOM FLOAT", // WOBJ_UPGRADE_SHOES,
	"CUSTOM FLOAT", // WOBJ_UPGRADE_WINGS,
	"CUSTOM FLOAT", // WOBJ_UPGRADE_CRYSTAL_WINGS,
	"CUSTOM FLOAT", // WOBJ_FLYING_SLIME,
	"SPEED", // WOBJ_HEAVY,
	"CUSTOM FLOAT", // WOBJ_HEAVY_BLAST,
	"CUSTOM FLOAT", // WOBJ_DRAGON,
	"CUSTOM FLOAT", // WOBJ_FIREBALL,
	"FLYING SPEED", // WOBJ_BOZO_PIN,
	"CUSTOM FLOAT", // WOBJ_BOZO,
	"CUSTOM FLOAT", // WOBJ_SILVER_SLIME,
	"CUSTOM FLOAT", // LAVA_MONSTER,
	"CUSTOM FLOAT", // TT_MINION_SMALL,
	"CUSTOM FLOAT", // TT_MINION_BIG,
	"CUSTOM FLOAT", // SLIME_WALKER,
	"SWIMMING SPEED", // MEGA_FISH,
	"HEALTH", // LAVA_DRAGON_HEAD,
	"CUSTOM FLOAT", // LAVA_DRAGON_BODY,
	"CUSTOM FLOAT", // LAVA_DRAGON_BLOB,
	"CUSTOM FLOAT", // TT_CHASE_TRIGGER,
	"CUSTOM FLOAT", // TT_NORMAL_TRIGGER,
	"CUSTOM FLOAT", // TT_BOSS_WAYPOINT,
	"SPEED", // TT_BOSS,
	"SPEED", // EATER_BUG,
	"CUSTOM FLOAT", // WOBJ_FLAMETHROWER_FLAME,
	"SPEED", // SPIDER_WALKER,
	"CUSTOM FLOAT", // SPIDER_WALKER_WEB,
	"CUSTOM FLOAT", // WOBJ_SPIKE_TRAP,
	"ROTATING SPEED", // ROTATING_FIRE_COLUNM_PIECE,
	"SPEED", // MOVING_FIRE_VERTICAL,
	"SPEED", // MOVING_FIRE_HORIZONTAL,
	"CUSTOM FLOAT", // WOBJ_SUPER_DRAGON,
	"CUSTOM FLOAT", // WOBJ_SUPER_DRAGON_LANDING_ZONE,
	"CUSTOM FLOAT", // DEEPHOUSE_BOOT_BLAST,
	"CUSTOM FLOAT", // ULTRA_SWORD_BOOMERANG,
	"CUSTOM FLOAT", // HEAVY_HAMMER_SWING,
	"CUSTOM FLOAT", // PLAYER_HEAVY_BLAST,
	"SPEED", // BOZO_LASER_MINION,
	"CUSTOM FLOAT", // BOZO_LASER_LOCKON,
	"CUSTOM FLOAT", // BOZO_LASER_PART,
	"CUSTOM FLOAT", // BOZO_MKII,
	"CUSTOM FLOAT", // WOBJ_CHECKPOINT,
	"CUSTOM FLOAT", // SPIKE_GUY,
	"CUSTOM FLOAT", // SPIKE_GUY_SPIKE,
	"SPEED", // BANDIT_GUY,
	"SPEED", // HORIZONTAL_PUSH_ZONE,
	"SPEED", // VERTICAL_PUSH_ZONE,
	"SPEED", // VERTICAL_WIND_ZONE,
	"SPEED", // SMALL_HORIZONTAL_PUSH_ZONE,
	"CUSTOM FLOAT",  // FISSION_ENERGY_BOLT,
	"SPEED",  // WOBJ_MOVING_PLATFORM_VERITCAL,
	"FRAMES DEAD",  // WOBJ_DISAPEARING_PLATFORM,
	"CUSTOM FLOAT", // WOBJ_KAMAKAZI_SLIME,
	"CUSTOM FLOAT", // WOBJ_KAMAKAZI_SLIME_EXPLOSION,
	"JUMP VELOCITY", // WOBJ_SPRING_BOARD,
	"ENDING BG LAYER", // WOBJ_HORZ_BG_SWITCH,
	"ENDING BG LAYER", // WOBJ_VERT_BG_SWITCH,
	"CUSTOM FLOAT", // WOBJ_VERYBIG_TUNES_TRIGGER,
	"CUSTOM FLOAT", // WOBJ_JUMPTHROUGH,
	"CUSTOM FLOAT", // WOBJ_BIG_JUMPTHROUGH,
	"CUSTOM FLOAT", // WOBJ_BREAKABLE_PLATFORM
	"HEALTH", // WOBJ_SKIN_WALL_BREAKABLE
	"CUSTOM FLOAT", // WOBJ_LOCKED_BLOCK_RED
	"CUSTOM FLOAT", // WOBJ_LOCKED_BLOCK_GREEN
	"CUSTOM FLOAT", // WOBJ_LOCKED_BLOCK_BLUE
	"CUSTOM FLOAT", // ROCK_GUY_MEDIUM
	"CUSTOM FLOAT", // ROCK_GUY_SMALL1
	"CUSTOM FLOAT", // ROCK_GUY_SMALL2
	"CUSTOM FLOAT", // ROCK_GUY_SLIDER
	"CUSTOM FLOAT", // ROCK_GUY_SMASHER
	"CUSTOM FLOAT", // ROCK_GUY_SPEAR
	"CUSTOM FLOAT", // WOBJ_WATER_SPLASH_EFFECT
	"TARGET HEALTH", // WOBJ_HEALTH_SET_TRIGGER
	"CUSTOM FLOAT", // WOBJ_VORTEX
	"CUSTOM FLOAT", // WOBJ_VORTEX_UPGRADE_TRIGGER
	"FRAMES IN DIR", // WOBJ_CUSTOMIZEABLE_MOVEABLE_PLATFORM
	"ENDING LINE", // WOBJ_DIALOGE_BOX_TRIGGER
	"CUSTOM FLOAT", // WOBJ_GRAPHICS_CHANGE_TRIGGER
	"CUSTOM FLOAT", // WOBJ_TELE_PARTICLES
	"CUSTOM FLOAT", // WOBJ_DUST_PARTICLE
	"CUSTOM FLOAT", // WOBJ_BLOOD_PARTICLE
	"CUSTOM FLOAT", // WOBJ_GIB_PARTICLE
	"CUSTOM FLOAT", // WOBJ_MAXPOWER_RUNE
	"CUSTOM FLOAT", // WOBJ_BOSS_BAR_INFO
	"X SPEED", // WOBJ_BGSPEED_X
	"Y SPEED", // WOBJ_BGSPEED_Y
	"TRANSPARENCY (0-7)", // WOBJ_BGTRANS
	"DELAY (SECONDS)", // WOBJ_TELETRIGGER1
	"LOGIC LINK ID", // WOBJ_TELEAREA1
	"CUSTOM FLOAT", // WOBJ_SFX_POINT
	"CUSTOM FLOAT", // WOBJ_WOLF
	"CUSTOM FLOAT", // WOBJ_SUPERVIRUS
	"CUSTOM FLOAT", // WOBJ_LUA0
	"CUSTOM FLOAT", // WOBJ_LUA1
	"CUSTOM FLOAT", // WOBJ_LUA2
	"CUSTOM FLOAT", // WOBJ_LUA3
	"CUSTOM FLOAT", // WOBJ_LUA4
	"CUSTOM FLOAT", // WOBJ_LUA5
	"CUSTOM FLOAT", // WOBJ_LUA6
	"CUSTOM FLOAT", // WOBJ_LUA7
	"CUSTOM FLOAT", // WOBJ_LUA8
	"CUSTOM FLOAT", // WOBJ_LUA9
	"CUSTOM FLOAT", // WOBJ_LUA10
	"CUSTOM FLOAT", // WOBJ_LUA11
	"CUSTOM FLOAT", // WOBJ_LUA12
	"CUSTOM FLOAT", // WOBJ_LUA13
	"CUSTOM FLOAT", // WOBJ_LUA14
	"CUSTOM FLOAT", // WOBJ_LUA15
	"CUSTOM FLOAT", // WOBJ_NOUPGRADE_TRIGGER
};

static const char * objedit_item_names[] =
{
	"NOITEM",
	"SHOTGUN",
	"KNIFE",
	"APPLE",
	"CAKE",
	"STRENGTH_POTION",
	"SPEED_POTION",
	"JUMP_POTION",
	"SWORD",
	"HEALTH_POITION",
	"SNIPER",
	"50MONEY",
	"100MONEY",
	"500MONEY",
	"CHEESEBURGER",
	"GOLDEN_AXE",
	"UNBOUND_WAND",
	"FIRE_WAND",
	"ICE_WAND",
	"AIR_WAND",
	"LIGHTNING_WAND",
	"GOLDEN_SHOTGUN",
	"LASER_RIFLE",
	"ROCKET_LAUNCHER",
	"FIRE_POTION",
	"MINIGUN",
	"MEGA_POTION",
	"ULTRA_MEGA_POTION",
	"AWP",
	"FLAMETHROWER",
	"POISIONUS_STRENGTH_POTION",
	"POISIONUS_SPEED_POTION",
	"POISIONUS_JUMP_POTION",
	"BEASTCHURGER",
	"ULTRA_SWORD",
	"HEAVY_HAMMER",
	"FISSION_GUN",
	"KEY_RED",
	"KEY_GREEN",
	"KEY_BLUE"
};

#define NUM_OBJS (sizeof(objedit_allowed_wobjs) / sizeof(int))

static int objedit_allowed_wobjs[] =
{
	// Triggers/Yellow Marked Editor only objects
	WOBJ_TELEPORT,
	WOBJ_TELEAREA1,
	WOBJ_SMALL_TUNES_TRIGGER,
	WOBJ_BIG_TUNES_TRIGGER,
	WOBJ_VERYBIG_TUNES_TRIGGER,
	WOBJ_PLAYER_SPAWN,
	WOBJ_ENDING_TEXT_SPAWNER,
	WOBJ_DIALOGE_BOX_TRIGGER,
	WOBJ_BACKGROUND_SWITCHER,
	WOBJ_HORZ_BG_SWITCH,
	WOBJ_VERT_BG_SWITCH,
	WOBJ_DROPPED_ITEM,
	TT_CHASE_TRIGGER,
	TT_NORMAL_TRIGGER,
	TT_BOSS_WAYPOINT,
	ROTATING_FIRE_COLUNM_PIECE,
	MOVING_FIRE_VERTICAL,
	MOVING_FIRE_HORIZONTAL,
	HORIZONTAL_PUSH_ZONE,
	VERTICAL_PUSH_ZONE,
	VERTICAL_WIND_ZONE,
	SMALL_HORIZONTAL_PUSH_ZONE,
	WOBJ_SUPER_DRAGON_LANDING_ZONE,
	WOBJ_JUMPTHROUGH,
	WOBJ_BIG_JUMPTHROUGH,
	WOBJ_HEALTH_SET_TRIGGER,
	WOBJ_GRAPHICS_CHANGE_TRIGGER,
	WOBJ_BOSS_BAR_INFO,
	WOBJ_BGSPEED_X,
	WOBJ_BGSPEED_Y,
	WOBJ_BGTRANS,
	WOBJ_TELETRIGGER1,
	WOBJ_SFX_POINT,
	// Enemies
	WOBJ_SLIME,
	WOBJ_FLYING_SLIME,
	WOBJ_HEAVY,
	WOBJ_DRAGON,
	WOBJ_BOZO_PIN,
	WOBJ_BOZO,
	WOBJ_SILVER_SLIME,
	LAVA_MONSTER,
	TT_MINION_SMALL,
	TT_MINION_BIG,
	SLIME_WALKER,
	MEGA_FISH,
	LAVA_DRAGON_HEAD,
	TT_BOSS,
	EATER_BUG,
	SPIDER_WALKER,
	WOBJ_SPIKE_TRAP,
	WOBJ_SUPER_DRAGON,
	BOZO_LASER_MINION,
	BOZO_MKII,
	SPIKE_GUY,
	BANDIT_GUY,
	WOBJ_KAMAKAZI_SLIME,
	ROCK_GUY_MEDIUM,
	ROCK_GUY_SMALL1,
	ROCK_GUY_SMALL2,
	ROCK_GUY_SLIDER,
	ROCK_GUY_SMASHER,
	WOBJ_WOLF,
	// Misc. (Checkpoints, Upgrades, etc)
	WOBJ_BRICK_WALL_BREAKABLE,
	WOBJ_SCMOVING_PLATFORM,
	WOBJ_MOVING_PLATFORM_VERITCAL,
	WOBJ_DISAPEARING_PLATFORM,
	WOBJ_SPRING_BOARD,
	WOBJ_SKIN_WALL_BREAKABLE,
	WOBJ_BREAKABLE_PLATFORM,
	WOBJ_CUSTOMIZEABLE_MOVEABLE_PLATFORM,
	WOBJ_LOCKED_BLOCK_RED,
	WOBJ_LOCKED_BLOCK_GREEN,
	WOBJ_LOCKED_BLOCK_BLUE,
	WOBJ_VORTEX,
	WOBJ_ICE_RUNE,
	WOBJ_AIR_RUNE,
	WOBJ_FIRE_RUNE,
	WOBJ_LIGHTNING_RUNE,
	WOBJ_MAXPOWER_RUNE,
	WOBJ_UPGRADE_SHOES,
	WOBJ_UPGRADE_WINGS,
	WOBJ_VORTEX_UPGRADE_TRIGGER,
	WOBJ_UPGRADE_CRYSTAL_WINGS,
	WOBJ_NOUPGRADE_TRIGGER,
	WOBJ_CHECKPOINT,
	// Lua Objects
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
};

static CNM_RECT cross_rect;
static int current_id;
static int current_creation_id;
static float camx, camy;
static GUI_FRAME *frame, *sframe, *lframe, *lbframe, *eframe, *item_selecter;
static int tele_state;
static float tele_x, tele_y;
static int editing_stats;
static SPAWNER *current_spawner;
static int last_duration;
static int last_max;
static int last_ci;
static float last_cf;
static int last_spawn_mode;

static void ClearSpawnersButton(GUI_ELEMENT *elem, int index)
{
	Spawners_UnloadSpawners();
}
static void SaveSpawnersElementString(GUI_ELEMENT *elem, int index)
{
	static int saving = CNM_FALSE;
	char buffer[128] = {'\0'};
	if (!saving)
	{
		saving = CNM_TRUE;
		strcpy(buffer, "save_spawners ");
		strcat(buffer, elem->props.string);
		Command_Execute(buffer);
		Gui_SwitchBack();
		saving = CNM_FALSE;
	}
}
static void LoadSpawnersElementString(GUI_ELEMENT *elem, int index)
{
	static int loading = CNM_FALSE;
	char buffer[128] = {'\0'};
	if (!loading)
	{
		loading = CNM_TRUE;
		strcpy(buffer, "load_spawners ");
		strcat(buffer, elem->props.string);
		Command_Execute(buffer);
		strcpy(sframe->elements[0].props.string, elem->props.string);
		memset(elem->props.string, 0, UTIL_MAX_TEXT_WIDTH + 1);
		Gui_SwitchBack();
		loading = CNM_FALSE;
	}
}
static void LoadBlocksElementString(GUI_ELEMENT *elem, int index)
{
	static int loading = CNM_FALSE;
	char buffer[128] = {'\0'};
	if (!loading)
	{
		loading = CNM_TRUE;
		strcpy(buffer, "load_blocks ");
		strcat(buffer, elem->props.string);
		Command_Execute(buffer);
		Gui_SwitchBack();
		loading = CNM_FALSE;
	}
}
static void GoBackToMainMenu(GUI_ELEMENT *elem, int index)
{
	Game_SwitchState(GAME_STATE_MAINMENU);
}
static void SpawnTeleportersButton(GUI_ELEMENT *elem, int index)
{
	if (elem->active && !editing_stats)
	{
		tele_state = 1;
		frame->num_elements = 9;
	}
	else if (!editing_stats)
	{
		tele_state = 0;
		frame->num_elements = 7;
	}
	else if (editing_stats)
	{
		elem->active = CNM_FALSE;
	}
}
static void EditSpawnElementX(GUI_ELEMENT *elem, int index)
{
	current_spawner->x = (float)Gui_GetNumberElementInt(elem->frame, index);
	Spawners_MoveSpawner(current_spawner, current_spawner->x, current_spawner->y);

	if (current_spawner->wobj_type == WOBJ_PLAYER_SPAWN || current_spawner->wobj_type == WOBJ_CHECKPOINT)
	{
		PlayerSpawns_SetSpawnLoc(current_spawner->custom_int, current_spawner->x, current_spawner->y);
	}
}
static void EditSpawnElementY(GUI_ELEMENT *elem, int index)
{
	current_spawner->y = (float)Gui_GetNumberElementInt(elem->frame, index);
	Spawners_MoveSpawner(current_spawner, current_spawner->x, current_spawner->y);

	if (current_spawner->wobj_type == WOBJ_PLAYER_SPAWN || current_spawner->wobj_type == WOBJ_CHECKPOINT)
	{
		PlayerSpawns_SetSpawnLoc(current_spawner->custom_int, current_spawner->x, current_spawner->y);
	}
}
static void EditSpawnElementType(GUI_ELEMENT *elem, int index)
{
	current_spawner->wobj_type = Gui_GetNumberElementInt(elem->frame, index);
	sprintf(elem->frame->elements[0].name, "==== EDITING OBJECT TYPE %d ====", current_spawner->wobj_type);
}
static void EditSpawnElementDuration(GUI_ELEMENT *elem, int index)
{
	current_spawner->duration = Gui_GetNumberElementInt(elem->frame, index);
	last_duration = Gui_GetNumberElementInt(elem->frame, index);
}
static void EditSpawnElementMax(GUI_ELEMENT *elem, int index)
{
	current_spawner->max = Gui_GetNumberElementInt(elem->frame, index);
	last_max = Gui_GetNumberElementInt(elem->frame, index);
}
static void EditSpawnElementCI(GUI_ELEMENT *elem, int index)
{
	if (current_spawner->wobj_type == WOBJ_PLAYER_SPAWN || current_spawner->wobj_type == WOBJ_CHECKPOINT)
		PlayerSpawn_ClearSpawnId(current_spawner->custom_int);
	current_spawner->custom_int = Gui_GetNumberElementInt(elem->frame, index);
	last_ci = Gui_GetNumberElementInt(elem->frame, index);

	if (current_spawner->wobj_type == WOBJ_TELEPORT)
	{
		Gui_SetNumberElementInt(elem->frame, index + 2, (int)TeleportInfos_GetTeleport(current_spawner->custom_int)->x);
		Gui_SetNumberElementInt(elem->frame, index + 3, (int)TeleportInfos_GetTeleport(current_spawner->custom_int)->y);
		strcpy(elem->frame->elements[index + 4].props.string, TeleportInfos_GetTeleport(current_spawner->custom_int)->name);
		Gui_SetNumberElementInt(elem->frame, index + 5, TeleportInfos_GetTeleport(current_spawner->custom_int)->cost);
	}

	if (current_spawner->wobj_type == WOBJ_PLAYER_SPAWN || current_spawner->wobj_type == WOBJ_CHECKPOINT)
		PlayerSpawns_SetSpawnLoc(current_spawner->custom_int, current_spawner->x, current_spawner->y);

	if (current_spawner->wobj_type == WOBJ_CUSTOMIZEABLE_MOVEABLE_PLATFORM)
	{
		int converted;
		float fconverted, fpart;

		Gui_SetNumberElementInt(elem->frame, 9, current_spawner->custom_int & 0xf);
		Gui_SetNumberElementInt(elem->frame, 10, current_spawner->custom_int >> 4 & 0xfff);

		converted = current_spawner->custom_int >> 16 & 0xff;
		fconverted = (float)((int)(converted >> 4 & 0xf) - 8);
		fpart = (1.0f / 16.0f) * (float)(converted & 0xf);
		fconverted += fconverted < 0 ? -fpart : fpart;
		Gui_SetFloatElementFloat(elem->frame, 11, fconverted);

		converted = current_spawner->custom_int >> 24 & 0xff;
		fconverted = (float)((int)(converted >> 4 & 0xf) - 8);
		fpart = (1.0f / 16.0f) * (float)(converted & 0xf);
		fconverted += fconverted < 0 ? -fpart : fpart;
		Gui_SetFloatElementFloat(elem->frame, 12, fconverted);
	}
}
static void EditSpawnElementCF(GUI_ELEMENT *elem, int index)
{
	current_spawner->custom_float = Gui_GetFloatElementFloat(elem->frame, index);
	last_cf = Gui_GetFloatElementFloat(elem->frame, index);
}
static void EditTeleportElementX(GUI_ELEMENT *elem, int index)
{
	TeleportInfos_GetTeleport(current_spawner->custom_int)->x = (float)Gui_GetNumberElementInt(elem->frame, index);
}
static void EditTeleportElementY(GUI_ELEMENT *elem, int index)
{
	TeleportInfos_GetTeleport(current_spawner->custom_int)->y = (float)Gui_GetNumberElementInt(elem->frame, index);
}
static void EditTeleportElementName(GUI_ELEMENT *elem, int index)
{
	strcpy(TeleportInfos_GetTeleport(current_spawner->custom_int)->name, elem->props.string);
}
static void EditTeleportElementCost(GUI_ELEMENT *elem, int index)
{
	TeleportInfos_GetTeleport(current_spawner->custom_int)->cost = Gui_GetNumberElementInt(elem->frame, index);
}
static void EditTeleportGotoTeleport(GUI_ELEMENT *elem, int index)
{
	editing_stats = CNM_FALSE;
	//tele_state = 0;
	Gui_Reset();
	Gui_SetRoot(frame);
	Gui_Losefocus();
	camx = (float)((int)TeleportInfos_GetTeleport(current_spawner->custom_int)->x - RENDERER_WIDTH / 2);
	camy = (float)((int)TeleportInfos_GetTeleport(current_spawner->custom_int)->y - RENDERER_HEIGHT / 2);
}
static void EditTeleportStartNewTeleLoc(GUI_ELEMENT *elem, int index)
{
	Gui_Losefocus();
	tele_state = 3;
	editing_stats = CNM_FALSE;
}
static void ChoseAItemID(GUI_ELEMENT *elem, int index)
{
	static int chose = CNM_FALSE;
	if (!chose)
	{
		chose = CNM_TRUE;
		if (current_spawner->wobj_type == WOBJ_DROPPED_ITEM)
		{
			current_spawner->custom_int = index - 1;
			Gui_SetNumberElementInt(eframe, 7, current_spawner->custom_int);
		}
		else
		{
			//current_spawner->dropped_item = index - 1;
			SPAWNER_SET_DROPPED_ITEM(current_spawner, index - 1);
			//Gui_SetNumberElementInt(eframe, 8, current_spawner->dropped_item);
			Gui_SetNumberElementInt(eframe, 9, SPAWNER_GET_DROPPED_ITEM(current_spawner));
		}
		Gui_SwitchBack();
		chose = CNM_FALSE;
	}
}
static void CopySpawnElement(GUI_ELEMENT *elem, int index)
{
	static int copying = CNM_FALSE;
	if (!copying)
	{
		copying = CNM_TRUE;
		current_id = current_spawner->wobj_type;
		last_duration = current_spawner->duration;
		last_max = current_spawner->max;
		last_ci = current_spawner->custom_int;
		last_cf = current_spawner->custom_float;
		last_spawn_mode = SPAWNER_GET_SPAWN_MODE(current_spawner);
		Gui_SwitchBack();
		copying = CNM_FALSE;
	}
}
static void EditSpawnElementSpawnMode(GUI_ELEMENT *elem, int index)
{
	SPAWNER_SET_SPAWN_MODE(current_spawner, elem->props.set_index);
	last_spawn_mode = elem->props.set_index;
}
static void EditCMPElementUpdateCustomInt(GUI_ELEMENT *elem, int index)
{
	Gui_SetNumberElementInt(elem->frame, 7, current_spawner->custom_int);
}
static int cmp_element_safety = CNM_FALSE;
static void EditCMPElementBitmapX(GUI_ELEMENT *elem, int index)
{
	if (!cmp_element_safety)
	{
		cmp_element_safety = CNM_TRUE;
		current_spawner->custom_int = (current_spawner->custom_int & ~0xf) | (Gui_GetNumberElementInt(elem->frame, index) & 0xf);
		//Gui_SetNumberElementInt(elem->frame, index, current_spawner->custom_int & 0xf);
		EditCMPElementUpdateCustomInt(elem, index);
		cmp_element_safety = CNM_FALSE;
	}
}
static void EditCMPElementBitmapY(GUI_ELEMENT *elem, int index)
{
	if (!cmp_element_safety)
	{
		cmp_element_safety = CNM_TRUE;
		current_spawner->custom_int = (current_spawner->custom_int & ~0xfff0) | (Gui_GetNumberElementInt(elem->frame, index) << 4 & 0xfff0);
		//Gui_SetNumberElementInt(elem->frame, index, current_spawner->custom_int >> 4 & 0xfff);
		EditCMPElementUpdateCustomInt(elem, index);
		cmp_element_safety = CNM_FALSE;
	}
}
static void EditCMPElementSpeedX(GUI_ELEMENT *elem, int index)
{
	int converted;
	float fconverted;
	if (!cmp_element_safety)
	{
		cmp_element_safety = CNM_TRUE;
		fconverted = Gui_GetFloatElementFloat(elem->frame, index);
		if (fconverted != roundf(fconverted))
			fconverted -= 1.0f;
		converted = ((int)fconverted + 32 & 0x3f) << 2;
		converted |= (int)(fconverted * 4.0f) & 0x3;
		current_spawner->custom_int = (current_spawner->custom_int & ~0x00ff0000) |
			(converted << 16);

		/*converted = current_spawner->custom_int >> 16 & 0xff;
		fconverted = (float)((converted >> 6 & 0x3f) - 32);
		fconverted += 1.0f / (float)(converted & 0x3);
		Gui_SetFloatElementFloat(elem->frame, index, fconverted);*/
		EditCMPElementUpdateCustomInt(elem, index);
		cmp_element_safety = CNM_FALSE;
	}
}
static void EditCMPElementSpeedY(GUI_ELEMENT *elem, int index)
{
	int converted;
	float fconverted;
	if (!cmp_element_safety)
	{
		cmp_element_safety = CNM_TRUE;
		fconverted = Gui_GetFloatElementFloat(elem->frame, index);
		if (fconverted != roundf(fconverted))
			fconverted -= 1.0f;
		converted = ((int)fconverted + 32 & 0x3f) << 2;
		converted |= (int)(fconverted * 4.0f) & 0x3;
		current_spawner->custom_int = (current_spawner->custom_int & ~0xff000000) |
			(converted << 24);

		/*converted = current_spawner->custom_int >> 24 & 0xff;
		fconverted = (float)((converted >> 6 & 0x3f) - 32);
		fconverted += 1.0f / (float)(converted & 0x3);
		Gui_SetFloatElementFloat(elem->frame, index, fconverted);*/
		EditCMPElementUpdateCustomInt(elem, index);
		cmp_element_safety = CNM_FALSE;
	}
}

void GameState_ObjEdit_Init(void)
{
	GUI_FRAME_PROPS props;
	Gui_Reset();
	editing_stats = CNM_FALSE;
	current_creation_id = 0;
	current_id = objedit_allowed_wobjs[current_creation_id];
	last_duration = 0;
	last_max = 0;
	last_ci = 0;
	last_cf = 0.0f;
	last_spawn_mode = SPAWNER_MODE_MULTI_AND_SINGLE_PLAYER;
	props.top = 96;
	props.align[0] = GUI_ALIGN_LEFT;
	props.bounds[0] = 16;
	props.line_count = 10;
	props.align[1] = GUI_ALIGN_RIGHT;
	props.bounds[1] = RENDERER_WIDTH - 16;
	frame = Gui_CreateFrame(7, 10, NULL, &props, 0);
	sframe = Gui_CreateFrame(1, 1, frame, &props, 0);
	lframe = Gui_CreateFrame(1, 1, frame, &props, 0);
	eframe = Gui_CreateFrame(6, 30, NULL, &props, 0);
	item_selecter = Gui_CreateFrame(NUM_ITEMS + 1, NUM_ITEMS + 5, eframe, &props, 0);
	Gui_InitStringElement(lframe, 0, LoadSpawnersElementString, "LOAD FILE: ", 23);
	Gui_InitStringElement(sframe, 0, SaveSpawnersElementString, "SAVE FILE: ", 23);
	lbframe = Gui_CreateFrame(1, 1, frame, &props, 0);
	Gui_InitStringElement(lbframe, 0, LoadBlocksElementString, "LOAD FILE: ", 23);
	Gui_InitHeaderElement(frame, 0, "===== BLOCKS EDITOR =====");
	Gui_InitButtonElement(frame, 1, NULL, "LOAD SPAWNERS", lframe, CNM_FALSE);
	Gui_InitButtonElement(frame, 2, NULL, "LOAD BLOCKS", lbframe, CNM_FALSE);
	Gui_InitButtonElement(frame, 3, NULL, "SAVE SPAWNERS", sframe, CNM_FALSE);
	Gui_InitButtonElement(frame, 4, ClearSpawnersButton, "CLEAR SPAWNERS", NULL, CNM_FALSE);
	Gui_InitButtonElement(frame, 5, SpawnTeleportersButton, "SPAWN TELEPORTERS", NULL, CNM_TRUE);
	frame->elements[5].active = CNM_FALSE;
	Gui_InitButtonElement(frame, 6, GoBackToMainMenu, "GO BACK TO MAIN MENU", NULL, CNM_FALSE);
	Gui_InitStringElement(frame, 7, NULL, "TELEPORT NAME: ", 15);
	Gui_InitNumberElement(frame, 8, NULL, "TELEPORT COST: ", -1000000, 1000000000, 0);

	Gui_InitHeaderElement(item_selecter, 0, "=== SELECT A ITEM ID ===");
	for (int i = 0; i < NUM_ITEMS; i++)
	{
		Gui_InitButtonElement(item_selecter, i + 1, ChoseAItemID, objedit_item_names[i], NULL, 0);
	}

	Gui_SetRoot(frame);
	Gui_Focus();
}
void GameState_ObjEdit_Quit(void)
{
	Gui_Reset();
	Gui_DestroyFrame(frame);
	Gui_DestroyFrame(sframe);
	Gui_DestroyFrame(lframe);
	Gui_DestroyFrame(lbframe);
	Gui_DestroyFrame(eframe);
	Gui_DestroyFrame(item_selecter);
}
void GameState_ObjEdit_Update(void)
{
	TELEPORT_INFO *tele_info;
	SPAWNER *s;
	char name_buf[UTIL_MAX_TEXT_WIDTH + 1];
	float spd = 4.0f, cx, cy;

	Input_Update();
	GameConsole_Update();
	Gui_Update();

	if (Input_GetButtonPressed(INPUT_ESCAPE, INPUT_STATE_PLAYING) && !editing_stats && tele_state != 3)
		Gui_Focus();
	int pw = Input_GetButtonPressedRepeated(INPUT_W, INPUT_STATE_PLAYING);
	int ps = Input_GetButtonPressedRepeated(INPUT_S, INPUT_STATE_PLAYING);
	if ((ps || pw))
	{
		int is_valid = CNM_FALSE;
		for (int i = 0; i < NUM_OBJS; i++)
			if (objedit_allowed_wobjs[i] == current_id)
				is_valid = CNM_TRUE;
		if (is_valid)
		{
			if (pw && current_creation_id + 1 < NUM_OBJS)
				current_creation_id++;
			if (ps && current_creation_id - 1 > -1)
				current_creation_id--;
			current_id = objedit_allowed_wobjs[current_creation_id];
		}
		else
		{
			current_creation_id = 0;
			current_id = objedit_allowed_wobjs[current_creation_id];
		}

		last_ci = 0;
		last_cf = 0.0f;
	}
	//if (Input_GetButtonPressedRepeated(INPUT_W, INPUT_STATE_PLAYING) && !tele_state && current_id + 1 < sizeof(wobj_types) / sizeof(WOBJ_TYPE))
	//	current_id++;
	//if (Input_GetButtonPressedRepeated(INPUT_S, INPUT_STATE_PLAYING) && !tele_state && current_id > 1)
	//	current_id--;

	if (tele_state) {
		if (current_creation_id < 0) current_creation_id = 0;
		if (current_creation_id > 1) current_creation_id = 1;
		current_id = objedit_allowed_wobjs[current_creation_id];
	}
	if (Input_GetButton(INPUT_D, INPUT_STATE_PLAYING))
		spd = 64.0f;
	if (Input_GetButton(INPUT_A, INPUT_STATE_PLAYING))
		spd = 32.0f;
	if (Input_GetButton(INPUT_RIGHT, INPUT_STATE_PLAYING))
		camx += spd;
	if (Input_GetButton(INPUT_LEFT, INPUT_STATE_PLAYING))
		camx -= spd;
	if (Input_GetButton(INPUT_DOWN, INPUT_STATE_PLAYING))
		camy += spd;
	if (Input_GetButton(INPUT_UP, INPUT_STATE_PLAYING))
		camy -= spd;

	cx = (float)((int)camx + RENDERER_WIDTH / 2);
	cy = (float)((int)camy + RENDERER_HEIGHT / 2);
	if (!tele_state && !editing_stats)
	{
		if (Input_GetButtonPressed(INPUT_DROP, INPUT_STATE_PLAYING) && current_id != WOBJ_TELEPORT &&
			current_id != WOBJ_TELEAREA1)
		{
			s = Spawners_CreateSpawner(cx, cy, current_id, last_duration, last_max, -1);
			s->custom_int = last_ci;
			s->custom_float = last_cf;
			SPAWNER_SET_SPAWN_MODE(s, last_spawn_mode);
			
			if (s->wobj_type == WOBJ_PLAYER_SPAWN)
			{
				s->custom_int = PlayerSpawn_GetUnusedSpawnId(PLAYER_SPAWN_TYPE_NORMAL_MODES);
				PlayerSpawns_SetSpawnLoc(s->custom_int, s->x, s->y);
			}
			if (s->wobj_type == WOBJ_CHECKPOINT)
			{
				s->custom_int = PlayerSpawn_GetUnusedSpawnId(PLAYER_SPAWN_TYPE_CHECKPOINTS);
				PlayerSpawns_SetSpawnLoc(s->custom_int, s->x, s->y);
			}

			s = NULL;
		}
	}
	else if (!editing_stats)
	{
		if (Input_GetButtonPressed(INPUT_DROP, INPUT_STATE_PLAYING))
		{
			if (tele_state == 1)
			{
				tele_x = cx;
				tele_y = cy;
				tele_state = 2;
			}
			else if (tele_state == 2)
			{
				tele_info = TeleportInfos_AllocTeleportInfo();
				tele_info->cost = Gui_GetNumberElementInt(frame, 8);
				strcpy(tele_info->name, frame->elements[7].props.string);
				tele_info->x = cx;
				tele_info->y = cy;
				s = Spawners_CreateSpawner
				(
					tele_x,
					tele_y,
					current_id,
					0,
					0,
					-1
				);
				s->custom_int = tele_info->index;
				tele_state = 1;
			}
		}
	}

	if (tele_state == 3 && Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING))
	{
		Gui_Focus();
		editing_stats = CNM_TRUE;
		eframe->active_index = 13;
		TeleportInfos_GetTeleport(current_spawner->custom_int)->x = cx;
		TeleportInfos_GetTeleport(current_spawner->custom_int)->y = cy;
		camx = (float)((int)current_spawner->x - RENDERER_WIDTH / 2 + 16);
		camy = (float)((int)current_spawner->y - RENDERER_HEIGHT / 2 + 16);
		tele_state = frame->elements[5].active ? 1 : 0;
	}

	s = Spawners_GetSpawnerWithinBox(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
	if (Input_GetButtonPressed(INPUT_BACKSPACE, INPUT_STATE_PLAYING) && !editing_stats && s != NULL && tele_state != 3)
	{
		if (s->wobj_type == WOBJ_TELEPORT)
			TeleportInfos_FreeTeleportInfo(s->custom_int);
		if (s->wobj_type == WOBJ_PLAYER_SPAWN || s->wobj_type == WOBJ_CHECKPOINT)
			PlayerSpawn_ClearSpawnId(s->custom_int);

		Spawners_DestroySpawner(s);
	}
	s = Spawners_GetSpawnerWithinBox(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);
	if (Input_GetButtonPressed(INPUT_ENTER, INPUT_STATE_PLAYING) && !editing_stats && s != NULL && tele_state != 3)
	{
		char custom_int_name[UTIL_MAX_TEXT_WIDTH];
		char custom_float_name[UTIL_MAX_TEXT_WIDTH];

		if (s->wobj_type < NUM_INTS)
			strcpy(custom_int_name, objedit_custom_ints[s->wobj_type]);
		else
			strcpy(custom_int_name, "CUSTOM INT");
		if (s->wobj_type < NUM_FLOATS)
			strcpy(custom_float_name, objedit_custom_floats[s->wobj_type]);
		else
			strcpy(custom_float_name, "CUSTOM FLOAT");
		strcat(custom_int_name, ": ");
		strcat(custom_float_name, ": ");

		editing_stats = CNM_TRUE;
		current_spawner = s;
		Gui_Reset();
		sprintf(name_buf, "==== EDITING OBJECT TYPE %d ====", s->wobj_type);
		Gui_InitHeaderElement(eframe, 0, name_buf);
		Gui_InitNumberElement(eframe, 1, EditSpawnElementX, "X: ", -10000000, 1000000, (int)s->x);
		Gui_InitNumberElement(eframe, 2, EditSpawnElementY, "Y: ", -10000000, 1000000, (int)s->y);
		
		if (s->wobj_type != WOBJ_TELEAREA1 && s->wobj_type != WOBJ_TELEPORT && s->wobj_type != WOBJ_PLAYER_SPAWN && s->wobj_type != WOBJ_CHECKPOINT)
			Gui_InitButtonElement(eframe, 3, CopySpawnElement, "COPY VALUES", NULL, CNM_FALSE);
		else
			Gui_InitNullElement(eframe, 3);

		Gui_InitNumberElement(eframe, 4, EditSpawnElementDuration, "SPAWN TIMER: ", 0, 1000000, (int)s->duration);
		Gui_InitNumberElement(eframe, 5, EditSpawnElementMax, "MAX SPAWNS: ", 0, 1000000, (int)s->max);
		Gui_InitSetElement(eframe, 6, EditSpawnElementSpawnMode, "SPAWN MODE: ");
		eframe->elements[6].props.set_index = SPAWNER_GET_SPAWN_MODE(s);
		Gui_AddItemToSet(eframe, 6, "SINGLE AND MULTI-PLAYER");
		Gui_AddItemToSet(eframe, 6, "SINGLEPLAYER ONLY");
		Gui_AddItemToSet(eframe, 6, "MULTIPLAYER ONLY");
		Gui_AddItemToSet(eframe, 6, "BASED ON PLAYER COUNT");
		Gui_InitNumberElement(eframe, 7, EditSpawnElementCI, custom_int_name, INT_MIN, INT_MAX, (int)s->custom_int);
		Gui_InitFloatElement(eframe, 8, EditSpawnElementCF, custom_float_name, -10000000.0f, 1000000.0f, s->custom_float);

		eframe->num_elements = 9;
		if (s->wobj_type == WOBJ_TELEPORT ||
			s->wobj_type == WOBJ_TELEAREA1)
		{
			eframe->num_elements = 15;
			Gui_InitNumberElement(eframe, 9, EditTeleportElementX, "TELEPORT X: ", -10000000, 1000000, (int)TeleportInfos_GetTeleport(current_spawner->custom_int)->x);
			Gui_InitNumberElement(eframe, 10, EditTeleportElementY, "TELEPORT Y: ", -10000000, 1000000, (int)TeleportInfos_GetTeleport(current_spawner->custom_int)->y);
			Gui_InitStringElement(eframe, 11, EditTeleportElementName, "TELEPORT NAME: ", 30);
			memcpy(eframe->elements[11].props.string, TeleportInfos_GetTeleport(current_spawner->custom_int)->name, 30);
			Gui_InitNumberElement(eframe, 12, EditTeleportElementCost, "TELEPORT COST: ", 0, 1000000, TeleportInfos_GetTeleport(current_spawner->custom_int)->cost);
			Gui_InitButtonElement(eframe, 13, EditTeleportGotoTeleport, "GO TO TELE LOCATION", NULL, CNM_FALSE);
			Gui_InitButtonElement(eframe, 14, EditTeleportStartNewTeleLoc, "SET TELE LOCATION", NULL, CNM_FALSE);
		}
		else if (s->wobj_type == WOBJ_DROPPED_ITEM)
		{
			eframe->num_elements = 10;
			Gui_InitButtonElement(eframe, 9, NULL, "SEARCH FOR ITEM", item_selecter, CNM_FALSE);
		}
		else if (s->wobj_type == WOBJ_CUSTOMIZEABLE_MOVEABLE_PLATFORM)
		{
			eframe->num_elements = 13;
			int converted;
			float fconverted, fpart;
			converted = current_spawner->custom_int & 0xf;
			Gui_InitNumberElement(eframe, 9, EditCMPElementBitmapX, "BITMAP X (32 WIDTH): ", 0, 15, converted);
			converted = current_spawner->custom_int >> 4 & 0xfff;
			Gui_InitNumberElement(eframe, 10, EditCMPElementBitmapY, "BITMAP Y (32 WIDTH): ", 0, 4095, converted);
			converted = current_spawner->custom_int >> 16 & 0xff;
			fconverted = (float)((int)(converted >> 4 & 0xf) - 8);
			fpart = (1.0f / 16.0f) * (float)(converted & 0xf);
			fconverted += fconverted < 0 ? -fpart : fpart;
			Gui_InitFloatElement(eframe, 11, EditCMPElementSpeedX, "SPEED X: ", -32.0f, 31.75f, fconverted);
			converted = current_spawner->custom_int >> 24 & 0xff;
			fconverted = (float)((int)(converted >> 4 & 0xf) - 8);
			fpart = (1.0f / 16.0f) * (float)(converted & 0xf);
			fconverted += fconverted < 0 ? -fpart : fpart;
			Gui_InitFloatElement(eframe, 12, EditCMPElementSpeedY, "SPEED Y: ", -32.0f, 31.75f, fconverted);
		}
		else
		{
			eframe->num_elements = 11;
			//Gui_InitNumberElement(eframe, 8, NULL, "DROPPED ITEM", 0, 100, s->dropped_item);
			Gui_InitNumberElement(eframe, 9, NULL, "DROPPED ITEM", 0, 100, SPAWNER_GET_DROPPED_ITEM(s));
			Gui_InitButtonElement(eframe, 10, NULL, "SEARCH FOR DROPPED ITEM", item_selecter, CNM_FALSE);
		}

		Gui_SetRoot(eframe);
		Gui_Focus();
	}

	if (editing_stats && !Gui_IsFocused())
	{
		editing_stats = CNM_FALSE;
		Gui_Reset();
		Gui_SetRoot(frame);
		Gui_Losefocus();
	}
}
void GameState_ObjEdit_Draw(void)
{
	float cx, cy;
	Renderer_Clear(Renderer_MakeColor(128, 128, 255));
	Blocks_DrawBlocks(BLOCKS_BG, (int)camx, (int)camy);
	Blocks_DrawBlocks(BLOCKS_FG, (int)camx, (int)camy);
	Spawners_DrawSpawners((int)camx, (int)camy);

	Util_SetRect(&cross_rect, 160-1, 120-4, 2, 8);
	Renderer_DrawRect(&cross_rect, Renderer_MakeColor(255, 0, 0), 4, RENDERER_LIGHT);
	Util_SetRect(&cross_rect, 160 - 4, 120 - 1, 8, 2);
	Renderer_DrawRect(&cross_rect, Renderer_MakeColor(255, 0, 0), 4, RENDERER_LIGHT);

	CNM_RECT r;
	int x, y;
	for (x = (int)camx / (int)OBJGRID_SIZE - 1; x < ((int)camx + RENDERER_WIDTH) / (int)OBJGRID_SIZE + 1; x++)
	{
		for (y = (int)camy / (int)OBJGRID_SIZE - 1; y < ((int)camy + RENDERER_WIDTH) / (int)OBJGRID_SIZE + 1; y++)
		{
			if (x < 0 || y < 0 || x > 128 || y > 64)
				continue;
			Util_SetRect
			(
				&r,
				x * (int)OBJGRID_SIZE - (int)camx,
				y * (int)OBJGRID_SIZE - (int)camy,
				(int)OBJGRID_SIZE,
				(int)OBJGRID_SIZE
			);
			Renderer_DrawEmptyRect(&r, Renderer_MakeColor(0, 0, 255), 0, RENDERER_LIGHT);

			// Grid position helpers
			if (Game_GetVar(GAME_VAR_SHOW_GRIDPOS)->data.integer)
			{
				Renderer_DrawText
				(
					x * (int)OBJGRID_SIZE - (int)camx,
					y * (int)OBJGRID_SIZE - (int)camy,
					0,
					RENDERER_LIGHT,
					"(%d, %d)",
					x, y
				);
			}
		}
	}

	Renderer_DrawBitmap(RENDERER_WIDTH / 2, RENDERER_HEIGHT / 2, wobj_types[current_id].frames, 4, RENDERER_LIGHT);

	cx = (float)((int)camx + RENDERER_WIDTH / 2);
	cy = (float)((int)camy + RENDERER_HEIGHT / 2);
	Renderer_DrawText(8, 0, 0, RENDERER_LIGHT, "X: %d Y: %d CX: %d CY: %d", (int)cx, (int)cy, (int)(cx / OBJGRID_SIZE), (int)(cy / OBJGRID_SIZE));
	Renderer_DrawText(8, 8, 0, RENDERER_LIGHT, "CURRENT WOBJ_ID: %d", current_id);
	if (tele_state == 2 || tele_state == 3)
		Renderer_DrawText(8, 16, 0, RENDERER_LIGHT, "PLACE TELEPORT LOCATION");
	Gui_Draw();
	GameConsole_Draw();
	Renderer_Update();
}
