#ifndef _item_h_
#define _item_h_
#include "utility.h"

#define ITEM_MAX_FRAMES 16

typedef enum _ITEM_TYPES
{
	ITEM_TYPE_NOITEM,
	ITEM_TYPE_SHOTGUN,
	ITEM_TYPE_KNIFE,
	ITEM_TYPE_APPLE,
	ITEM_TYPE_CAKE,
	ITEM_TYPE_STRENGTH_POTION,
	ITEM_TYPE_SPEED_POTION,
	ITEM_TYPE_JUMP_POTION,
	ITEM_TYPE_SWORD,
	ITEM_TYPE_HEALTH_POITION,
	ITEM_TYPE_SNIPER,
	ITEM_TYPE_50MONEY,
	ITEM_TYPE_100MONEY,
	ITEM_TYPE_500MONEY,
	ITEM_TYPE_CHEESEBURGER,
	ITEM_TYPE_GOLDEN_AXE,
	ITEM_TYPE_UNBOUND_WAND,
	ITEM_TYPE_FIRE_WAND,
	ITEM_TYPE_ICE_WAND,
	ITEM_TYPE_AIR_WAND,
	ITEM_TYPE_LIGHTNING_WAND,
	ITEM_TYPE_GOLDEN_SHOTGUN,
	ITEM_TYPE_LASER_RIFLE,
	ITEM_TYPE_ROCKET_LAUNCHER,
	ITEM_TYPE_FIRE_POTION,
	ITEM_TYPE_MINIGUN,
	ITEM_TYPE_MEGA_POTION,
	ITEM_TYPE_ULTRA_MEGA_POTION,
	ITEM_TYPE_AWP,
	ITEM_TYPE_FLAMETHROWER,
	ITEM_TYPE_POISIONUS_STRENGTH_POTION,
	ITEM_TYPE_POISIONUS_SPEED_POTION,
	ITEM_TYPE_POISIONUS_JUMP_POTION,
	ITEM_TYPE_BEASTCHURGER,
	ITEM_TYPE_ULTRA_SWORD,
	ITEM_TYPE_HEAVY_HAMMER,
	ITEM_TYPE_FISSION_GUN,
	ITEM_TYPE_KEY_RED,
	ITEM_TYPE_KEY_GREEN,
	ITEM_TYPE_KEY_BLUE,
	ITEM_TYPE_1UP_JUICE,
} ITEM_TYPES;

typedef struct _WOBJ WOBJ;
typedef struct _ITEM ITEM;
typedef void(*ITEM_FUNC)(ITEM *item, WOBJ *player);
typedef void(*ITEM_DRAW_FUNC)(ITEM *item, WOBJ *player, int camx, int camy);

typedef struct _ITEM_TYPE
{
	CNM_RECT frames[ITEM_MAX_FRAMES];
	ITEM_FUNC update, on_pickup, on_drop, on_use;
	ITEM_DRAW_FUNC draw;
	int use_on_held;
	int wobj_dropped_type;
} ITEM_TYPE;

typedef struct _ITEM
{
	int type;
	int anim_frame, times_used, life_timer, use_timer, durability;
	int custom_timer;
	int custom_ints[2];
	float custom_floats[2];
	WOBJ *melee_obj, *objs[8];
	int in_use, last_in_use;
} ITEM;

extern ITEM_TYPE item_types[];

void Item_Init(void);
void Item_Reset(void);
ITEM *Item_GetCurrentItem(void);
const ITEM_TYPE *Item_GetItemType(int type_id);

void Item_TryPickupAndDrop(WOBJ *player);
void Item_Pickup(WOBJ *player, WOBJ *dropped_item);
void Item_PickupByType(WOBJ *player, int itemtype);
void Item_Drop(WOBJ *player);
void Item_Update(WOBJ *player);
void Item_Draw(WOBJ *player, int camx, int camy);
void Item_TryUse(WOBJ *player);
void Item_Use(WOBJ *player);
void Item_DestroyCurrentItem(WOBJ *player);

#endif
