#include <string.h>
#include <math.h>
#include "renderer.h"
#include "item.h"
#include "wobj.h"
#include "game.h"
#include "interaction.h"
#include "input.h"
#include "player.h"
#include "audio.h"
#include "utility.h"
#include "savedata.h"

//static void NULL(ITEM *item, WOBJ *player, int camx, int camy);
static void ItemGeneric_MusleFlash(ITEM *item, WOBJ* player);

static void ItemShotgun_OnUse(ITEM *shotgun, WOBJ *player);
static void ItemShotgun_OnDrop(ITEM *shotgun, WOBJ *player);

static void ItemGoldenShotgun_OnUse(ITEM *shotgun, WOBJ *player);

static void ItemUnboundWand_Update(ITEM *wand, WOBJ *player);

static void ItemSniper_OnUse(ITEM *shotgun, WOBJ *player);
static void ItemAWP_OnUse(ITEM *awp, WOBJ *player);
static void ItemFissionGun_OnUse(ITEM *gun, WOBJ *player);

static void ItemFlameThrower_OnUse(ITEM *flamethrower, WOBJ *player);
static void ItemFlameThrower_OnUpdate(ITEM *flamethrower, WOBJ *player);

static void ItemLaserRifle_Update(ITEM *laser, WOBJ *player);
static void ItemLaserRifle_OnUse(ITEM *laser, WOBJ *player);

static void ItemRocketLauncher_OnUse(ITEM *rpg, WOBJ *player);

static void ItemMinigun_OnUse(ITEM *minigun, WOBJ *player);
static void ItemMinigun_OnPickup(ITEM *minigun, WOBJ *player);
static void ItemMinigun_OnUpdate(ITEM *minigun, WOBJ *player);
static void ItemMinigun_OnDrop(ITEM *minigun, WOBJ *player);

static void ItemGenericMelee_Update(ITEM *melee, WOBJ *player);
static void ItemGenericMelee_OnPickup(ITEM *melee, WOBJ *player);
static void ItemGenericMelee_OnDrop(ITEM *melee, WOBJ *player);

static void ItemHeavyHammer_OnUse(ITEM *hammer, WOBJ *player);
static void ItemHeavyHammer_OnUpdate(ITEM *hammer, WOBJ *player);

static void ItemUltraSword_OnUse(ITEM *sword, WOBJ *player);
static void ItemUltraSword_OnUpdate(ITEM *sword, WOBJ *player);

static void ItemGenericConsumeable_OnUse(ITEM *item, WOBJ *player);

static void ItemFireWand_OnUse(ITEM *item, WOBJ *player);
static void ItemIceWand_OnUse(ITEM *item, WOBJ *player);
static void ItemAirWand_OnUse(ITEM *item, WOBJ *player);
static void ItemLightningWand_OnUse(ITEM *item, WOBJ *player);

static void ItemKeyGeneric_Update(ITEM *key, WOBJ *player);

ITEM_TYPE item_types[] =
{
	{ // No item
		{0, 0, 0, 0},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		NULL, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_NULL, // What object it is when dropped
		0.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // Shotgun Item
		{32, 352, 32, 32},
		ItemGeneric_MusleFlash, // Update
		NULL, // On Pickup
		ItemShotgun_OnDrop, // On Drop
		ItemShotgun_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		75.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // Knife Item
		{64, 352, 32, 32},
		ItemGenericMelee_Update, // Update
		ItemGenericMelee_OnPickup, // On Pickup
		ItemGenericMelee_OnDrop, // On Drop
		NULL, // On Use
		NULL, // Draw
		CNM_TRUE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		50.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // Apple Item
		{96, 352, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // Cake Item
		{128, 352, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // Strength Item
		{160, 352, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // Speed Item
		{192, 352, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // Jump Item
		{224, 352, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // Sword Item
		{0, 384, 32, 32},
		ItemGenericMelee_Update,
		ItemGenericMelee_OnPickup,
		ItemGenericMelee_OnDrop,
		NULL, // On Use
		NULL, // Draw
		CNM_TRUE, // Activate on "use" held
		WOBJ_DROPPED_ITEM, // Generic dropped item object
		50.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // Healh Potion Item
		{32, 384, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // Sniper Rifle Item
		{64, 384, 32, 32},
		ItemGeneric_MusleFlash, // Update
		NULL, // On Pickup
		ItemShotgun_OnDrop, // On Drop
		ItemSniper_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		80.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 50 Money Item
		{96, 384, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 100 Money Item
		{128, 384, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 500 Money Item
		{160, 384, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // Cheeseburger Item
		{192, 384, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // Golden Axe Item
		{224, 384, 32, 32},
		ItemGenericMelee_Update,
		ItemGenericMelee_OnPickup,
		ItemGenericMelee_OnDrop,
		NULL, // On Use
		NULL, // Draw
		CNM_TRUE, // Activate on "use" held
		WOBJ_DROPPED_ITEM, // Generic dropped item object
		50.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // Unbound Wand Item
		{0, 416, 32, 32},
		ItemUnboundWand_Update,
		NULL,
		NULL,
		NULL, // On Use
		NULL, // Draw
		CNM_FALSE, // Activate on "use" held
		WOBJ_DROPPED_ITEM, // Generic dropped item object
		50.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 17: Fire Wand Item
		{32, 416, 32, 32},
		NULL,
		NULL,
		NULL,
		ItemFireWand_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Activate on "use" held
		WOBJ_DROPPED_ITEM, // Generic dropped item object
		100.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 18: Ice Wand Item
		{64, 416, 32, 32},
		NULL,
		NULL,
		NULL,
		ItemIceWand_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Activate on "use" held
		WOBJ_DROPPED_ITEM, // Generic dropped item object
		100.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 19: Air Wand Item
		{96, 416, 32, 32},
		NULL,
		NULL,
		NULL,
		ItemAirWand_OnUse, // On Use
		NULL, // Draw
		CNM_TRUE, // Activate on "use" held
		WOBJ_DROPPED_ITEM, // Generic dropped item object
		0.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 20: Lightning Wand Item
		{128, 416, 32, 32},
		NULL,
		NULL,
		NULL,
		ItemLightningWand_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Activate on "use" held
		WOBJ_DROPPED_ITEM, // Generic dropped item object
		100.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 21: Golden Shotgun Item
		{160, 416, 32, 32},
		ItemGeneric_MusleFlash, // Update
		NULL, // On Pickup
		ItemShotgun_OnDrop, // On Drop
		ItemShotgun_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Activate on "use" held
		WOBJ_DROPPED_ITEM, // Generic dropped item object
		75.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 22: Laser Rifle Item
		{192, 416, 32, 32},
		ItemLaserRifle_Update, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemLaserRifle_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Activate on "use" held
		WOBJ_DROPPED_ITEM, // Generic dropped item object
		0.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 23: Rocket Launcher Item
		{224, 416, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemRocketLauncher_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Activate on "use" held
		WOBJ_DROPPED_ITEM, // Generic dropped item object
		15.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 24: Fire Potion
		{0, 448, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 25: Minigun Item
		{32, 448, 32, 32},
		ItemMinigun_OnUpdate, // Update
		ItemMinigun_OnPickup, // On Pickup
		ItemMinigun_OnDrop, // On Drop
		ItemMinigun_OnUse, // On Use
		NULL, // Draw
		CNM_TRUE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		100.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 26: Mega Potion
		{64, 448, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 27: Ultra Mega Potion
		{96, 448, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 28: AWP
		{128, 448, 32, 32},
		ItemGeneric_MusleFlash, // Update
		NULL, // On Pickup
		ItemShotgun_OnDrop, // On Drop
		ItemSniper_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		50.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 29: Flamethrower
		{160, 448, 32, 32},
		ItemFlameThrower_OnUpdate, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemFlameThrower_OnUse, // On Use
		NULL, // Draw
		CNM_TRUE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		100.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 30: Poisonious Strength Potion
		{192, 448, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 31: Poisonious Speed Potion
		{224, 448, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 32: Poisonious Jump Potion
		{0, 480, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 33: Beastchurger Consumable
		{32, 480, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 34: Ultra Sword
		{64, 480, 32, 32},
		ItemUltraSword_OnUpdate, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemUltraSword_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		50.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 34: Heavy Hammer
		{96, 480, 32, 32},
		ItemHeavyHammer_OnUpdate, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemHeavyHammer_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		75.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 34: Fission Gun
		{128, 480, 32, 32},
		ItemGeneric_MusleFlash, // Update
		NULL, // On Pickup
		ItemShotgun_OnDrop, // On Drop
		ItemFissionGun_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		30.0f, // Item durability
		CNM_FALSE, // Draw Infront
	},
	{ // 35: Red Key
		{160, 480, 32, 32},
		ItemKeyGeneric_Update, // Update
		NULL, // On Pickup
		NULL, // On Drop
		NULL, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 36: Green Key
		{192, 480, 32, 32},
		ItemKeyGeneric_Update, // Update
		NULL, // On Pickup
		NULL, // On Drop
		NULL, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 37: Blue Key
		{224, 480, 32, 32},
		ItemKeyGeneric_Update, // Update
		NULL, // On Pickup
		NULL, // On Drop
		NULL, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 38: 1-Up Orange Juice Box 
		{128, 736, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	},
	{ // 39: Fix it up Wrench 
		{160, 2048, 32, 32},
		NULL, // Update
		NULL, // On Pickup
		NULL, // On Drop
		ItemGenericConsumeable_OnUse, // On Use
		NULL, // Draw
		CNM_FALSE, // Can be used when "use" is held
		WOBJ_DROPPED_ITEM, // What object it is when dropped
		0.0f, // Item durability
		CNM_TRUE, // Draw Infront
	}
};

ITEM item_current;
static int _pickup_cooldown;
static int _ghost_picked_up;

void Item_Init(void)
{
	memset(&item_current, 0, sizeof(item_current));
	_pickup_cooldown = 0;
	_ghost_picked_up = CNM_FALSE;
}
void Item_Reset(void) {
	memset(&item_current, 0, sizeof(item_current));
}
ITEM *Item_GetCurrentItem(void)
{
	return &item_current;
}
const ITEM_TYPE *Item_GetItemType(int type_id)
{
	if (type_id > -1 && type_id < sizeof(item_types) / sizeof(*item_types))
		return item_types + type_id;
	else
		return NULL;
}

void Item_NullifyGhostPickup(void) {
	_ghost_picked_up = CNM_FALSE;
}
void Item_TryPickupAndDrop(WOBJ *player)
{
	WOBJ *other_item = Wobj_GetWobjColliding(player, WOBJ_IS_ITEM);
	PLAYER_LOCAL_DATA *plr_local = player->local_data;
	if (_pickup_cooldown > 0) --_pickup_cooldown;
	int pressed_drop = Input_GetButtonPressed(INPUT_DROP, INPUT_STATE_PLAYING) && !plr_local->lock_controls;
	if (other_item != NULL &&
		pressed_drop &&
		player->item != ITEM_TYPE_NOITEM &&
		!item_types[Item_GetCurrentItem()->type].draw_infront) {
		Player_SwapOffhand(player);
		if (player->item != ITEM_TYPE_NOITEM) Item_Drop(player);
		else _ghost_picked_up = CNM_TRUE;
		Item_Pickup(player, other_item);
	}
	else if (other_item != NULL && pressed_drop && player->item == ITEM_TYPE_NOITEM)
	{
		_ghost_picked_up = CNM_FALSE;
		Item_Pickup(player, other_item);
	}
	else if (pressed_drop && player->item != ITEM_TYPE_NOITEM)
	{
		_ghost_picked_up = CNM_FALSE;
		Item_Drop(player);
	}
}
void Item_Pickup(WOBJ *player, WOBJ *dropped_item)
{
	if (player->item == ITEM_TYPE_NOITEM && _pickup_cooldown <= 0)
	{
		_pickup_cooldown = 10;
		memset(&item_current, 0, sizeof(item_current));
		player->item = dropped_item->item;
		item_current.hide_timer = ITEM_HIDE_TIMER * 2;
		item_current.type = dropped_item->item;
		item_current.durability = dropped_item->custom_floats[0];
		if (item_types[dropped_item->item].on_pickup != NULL)
			item_types[dropped_item->item].on_pickup(&item_current, player);
		Interaction_DestroyWobjInstant(dropped_item);
	}
}
void Item_PickupByType(WOBJ *player, int itemtype, float durability)
{
	if (player->item == ITEM_TYPE_NOITEM)
	{
		memset(&item_current, 0, sizeof(item_current));
		player->item = itemtype;
		item_current.hide_timer = ITEM_HIDE_TIMER;
		item_current.type = itemtype;
		item_current.durability = durability;
		if (durability < 0.0f) item_current.durability = item_types[itemtype].max_durability;
		if (item_types[itemtype].on_pickup != NULL)
			item_types[itemtype].on_pickup(&item_current, player);
	}
}
void Item_Drop(WOBJ *player)
{
	if (player->item != ITEM_TYPE_NOITEM)
	{
		_ghost_picked_up = CNM_FALSE;
		if (item_types[player->item].on_drop != NULL)
			item_types[player->item].on_drop(&item_current, player);
		WOBJ *dropped = Interaction_CreateWobj(item_types[player->item].wobj_dropped_type,
							   player->x, player->y, player->item, 0.0f);
		dropped->custom_floats[0] = item_current.durability;
		player->item = ITEM_TYPE_NOITEM;
		item_current.type = ITEM_TYPE_NOITEM;
	}
}
void Item_DestroyCurrentItem(WOBJ *player)
{
	if (player->item != ITEM_TYPE_NOITEM)
	{
		if (item_types[player->item].on_drop != NULL)
			item_types[player->item].on_drop(&item_current, player);
		player->item = ITEM_TYPE_NOITEM;
		item_current.type = ITEM_TYPE_NOITEM;
	}
}
void Item_Update(WOBJ *player)
{
	if (player->item != ITEM_TYPE_NOITEM)
	{
		if (item_types[player->item].update != NULL)
			item_types[player->item].update(&item_current, player);
		if (item_current.durability <= 0.0f && item_types[player->item].max_durability > 0.1f) {
			Interaction_PlaySound(player, 56);
			BreakPart_CreateParts(player->x, player->y, -5.0f, item_types[item_current.type].frames[0].x, item_types[item_current.type].frames[0].y, 2, 2);
			Item_DestroyCurrentItem(player);
		}
		item_current.life_timer++;
		item_current.use_timer--;
		item_current.custom_timer--;
		if (item_current.hide_timer > 0) item_current.hide_timer--;

		item_current.last_in_use = item_current.in_use;
		item_current.in_use = CNM_FALSE;
	}
}
void Item_Draw(WOBJ *player, int camx, int camy)
{
	if (player->item != ITEM_TYPE_NOITEM)
	{
		if (item_types[player->item].draw != NULL)
			item_types[player->item].draw(&item_current, player, camx, camy);
	}
}
void Item_TryUse(WOBJ *player)
{
	PLAYER_LOCAL_DATA *plr_local = player->local_data;
	if (player->item != ITEM_TYPE_NOITEM)
	{
		if (item_types[player->item].use_on_held && Input_GetButton(INPUT_FIRE, INPUT_STATE_PLAYING) && !plr_local->lock_controls && plr_local->finish_timer == 0)
			Item_Use(player);
		else if (!item_types[player->item].use_on_held && 
				 Input_GetButtonPressed(INPUT_FIRE, INPUT_STATE_PLAYING) && !plr_local->lock_controls && plr_local->finish_timer == 0)
			Item_Use(player);
	}
}
void Item_Use(WOBJ *player)
{
	PLAYER_LOCAL_DATA *plr_local = player->local_data;
	if (plr_local->lock_controls || plr_local->finish_timer > 0) return;
	if (player->item != ITEM_TYPE_NOITEM && item_current.use_timer <= 0)
	{
		if (item_types[player->item].on_use != NULL)
			item_types[player->item].on_use(&item_current, player);
		item_current.in_use = CNM_TRUE;
	}
}

/*static void NULL(ITEM *item, WOBJ *player, int camx, int camy)
{
	Renderer_DrawBitmap
	(
		(int)player->x - camx,
		(int)player->y - camy,
		&item_types[item->type].frames[item->anim_frame],
		0,
		RENDERER_LIGHT
	);
}*/
static void ItemGeneric_MusleFlash(ITEM *item, WOBJ *player)
{
	if (item->custom_timer >= 0)
		player->flags |= WOBJ_LIGHT_SMALL;
	else
		player->flags &= ~WOBJ_LIGHT_SMALL;
}

static void ItemShotgun_OnUse(ITEM *shotgun, WOBJ *player)
{
	//WOBJ *w = Interaction_CreateWobj(WOBJ_PLAYER_PELLET,
	//								 player->x, player->y, 0,
	//								 (player->flags & WOBJ_HFLIP) ? -1.0f : 1.0f);
	//w->strength = player->strength + 0.02f;
	//Interaction_PlaySound(player, 0);
	//shotgun->use_timer = 10;
	//shotgun->custom_timer = 15;
	//Player_PlayShootAnim(player);

	shotgun->hide_timer = ITEM_HIDE_TIMER;
	int numbullets = 8 - (shotgun->custom_timer < 0 ? 0 : shotgun->custom_timer / 5);
	if (numbullets < 3) numbullets = 3;
	float spread = (float)shotgun->custom_timer / 2.0f;
	float dir = (player->flags & WOBJ_HFLIP) ? -1.0f : 1.0f;
	if (spread < 0.0f) spread = 0.0f;
	spread += 1.0f;
	for (int i = 0; i < numbullets; i++) {
		WOBJ *pel = Interaction_CreateWobj(WOBJ_SHOTGUN_PEL, player->x + 16.0f, player->y + 12.0f, 0, 0.0f);
		pel->strength = player->strength + 0.01f;
		if (shotgun->type == ITEM_TYPE_GOLDEN_SHOTGUN) pel->strength += 0.1f;
		pel->y += spread * 2.0f - (spread / 2.0f);
		pel->vel_x = player->vel_x + dir * 20.0 * (Util_RandFloat() * 0.25 + 0.75);
		pel->vel_y = player->vel_y + (Util_RandFloat() * 2.0f - 1.0f) * spread;
	}
	Player_PlayShootAnim(player);
	Interaction_PlaySound(player, 0);
	shotgun->custom_timer = 20+10;
	shotgun->use_timer = 10;

	shotgun->durability -= 1.0f;
}
static void ItemShotgun_OnDrop(ITEM *shotgun, WOBJ *player)
{
	player->flags &= ~WOBJ_LIGHT_SMALL;
}

static void ItemSniper_OnUse(ITEM *shotgun, WOBJ *player)
{
	//WOBJ *w = Interaction_CreateWobj(WOBJ_PLAYER_PELLET,
	//								 player->x, player->y, 0,
	//								 (player->flags & WOBJ_HFLIP) ? -1.0f : 1.0f);
	//w->speed = 16.0f;
	//w->strength = player->strength + 0.5f;
	//Interaction_PlaySound(player, 11);
	//shotgun->use_timer = 15;
	//shotgun->custom_timer = 10;

	//if (player->flags & WOBJ_HFLIP)
	//	player->vel_x += 3.0f;
	//else
	//	player->vel_x -= 3.0f;

	shotgun->hide_timer = ITEM_HIDE_TIMER;
	float dmg_mul = 1.0f - CNM_MAX((float)shotgun->custom_timer + 8.0f, 0.0f) / 20.0f; 
	float spread = (float)shotgun->custom_timer / 2.25f;
	float dir = (player->flags & WOBJ_HFLIP) ? -1.0f : 1.0f;
	if (spread < 0.0f) spread = 0.0f;
	float sprd_rnd = (Util_RandFloat() * 2.0f - 1.0f);
	for (int i = 0; i < (shotgun->type == ITEM_TYPE_AWP ? 5 : 1); i++) {
		WOBJ *pel = Interaction_CreateWobj(WOBJ_SHOTGUN_PEL, player->x + 16.0f, player->y + 13.0f, 0, 0.0f);
		pel->strength = player->strength + (1.0f * dmg_mul);
		pel->anim_frame = 1;
		if (shotgun->type == ITEM_TYPE_AWP) {
			pel->x -= 11.0f;
			pel->x += (float)i * 5.0f;
			pel->custom_floats[1] = 0.35f;
		} else {
			pel->custom_floats[1] = 0.8f;
		}
		pel->y += spread * 2.0f - (spread / 2.0f);
		pel->vel_x = player->vel_x + dir * 20.0;
		pel->vel_y = player->vel_y + sprd_rnd * spread;
		pel->speed = 0.01f;
		if (player->flags & WOBJ_HFLIP) pel->flags |= WOBJ_HFLIP;
	}
	Player_PlayShootAnim(player);
	Interaction_PlaySound(player, 11);
	shotgun->custom_timer = 37;
	shotgun->use_timer = 25;
	shotgun->durability -= 1.0f;

	float knockback = (shotgun->type == ITEM_TYPE_AWP) ? 5.0f : 3.0f;
	if (player->flags & WOBJ_HFLIP)
		player->vel_x += knockback;
	else
		player->vel_x -= knockback;
}

static void ItemAWP_OnUse(ITEM *awp, WOBJ *player)
{
	awp->hide_timer = ITEM_HIDE_TIMER;
	float dir = (player->flags & WOBJ_HFLIP) ? -1.0f : 1.0f;
	for (int i = 0; i < 5; i++)
	{
		WOBJ *w = Interaction_CreateWobj(WOBJ_PLAYER_PELLET,
										 player->x + dir * i * 32.0f, player->y, 0,
										 (player->flags & WOBJ_HFLIP) ? -1.0f : 1.0f);
		w->speed = 20.0f;
		w->strength = player->strength + 0.5f;
	}
	Interaction_PlaySound(player, 11);
	awp->use_timer = 15;
	awp->custom_timer = 10;
	awp->durability -= 1.0f;

	if (player->flags & WOBJ_HFLIP)
		player->vel_x += 4.5f;
	else
		player->vel_x -= 4.5f;
}

static void ItemGenericMelee_Update(ITEM *melee, WOBJ *player)
{
	WOBJ *obj = melee->melee_obj;
	float strength_mul[2] = {0.0f, 0.0f};
	float range = 0.0f;

	switch (melee->type)
	{
	case ITEM_TYPE_KNIFE:
		strength_mul[0] = 0.85f;
		strength_mul[1] = 0.5f;
		range = 28.0f;
		break;
	case ITEM_TYPE_SWORD:
		strength_mul[0] = 1.75f;
		strength_mul[1] = 1.0f;
		range = 32.0f;
		break;
	case ITEM_TYPE_GOLDEN_AXE:
		strength_mul[0] = 5.0f;
		strength_mul[1] = 3.75f;
		range = 40.0f;
		break;
	}

	if (melee->custom_timer > 0) return;
	//melee->hide_timer = ITEM_HIDE_TIMER;
	if (melee->custom_timer == 0) {
		melee->custom_floats[0] = -1.0f;
	}
	if (melee->in_use && !melee->custom_ints[0]) {
		if (melee->custom_floats[0] <= 0.0f) {
			melee->custom_floats[0] = 0.5f * CNM_PI;
		}
		melee->custom_floats[0] += 4.0f / 180.0f * CNM_PI;
		if (melee->custom_floats[0] >= 0.9f * CNM_PI) {
			melee->custom_timer = 20;
			obj->x = player->x;
			obj->y = player->y;
			obj->hitbox.w = 0.0f;
			obj->hitbox.h = 0.0f;
			obj->anim_frame = 1;
			Wobj_UpdateGridPos(obj);
			return;
		}
	}
	if (melee->last_in_use && !melee->in_use && !melee->custom_ints[0]) {
		melee->custom_ints[0] = 1;
		Interaction_PlaySound(player, 2);
		player->vel_x *= 0.6f;
		player->vel_y *= 0.6f;
		melee->custom_floats[1] = (melee->custom_floats[0] - (0.5f * CNM_PI)) / 0.9f * 0.8f + 0.5f;
		melee->custom_floats[0] = 0.5f * CNM_PI;
	}

	if (melee->custom_ints[0]) {
		if (melee->custom_floats[0] >= 45.0f) {
			obj->strength = player->strength + strength_mul[0] * melee->custom_floats[1];
		} else {
			obj->strength = player->strength + strength_mul[1] * melee->custom_floats[1];
		}
		if (melee->custom_floats[0] <= 0.0f) {
			melee->custom_ints[0] = 0;
		}
		melee->custom_floats[0] -= 18.0f / 180.0f * CNM_PI;
		obj->hitbox.w = 32.0f;
		obj->hitbox.h = 32.0f;
	} else {
		obj->hitbox.w = 0.0f;
		obj->hitbox.h = 0.0f;
	}

	if (melee->custom_floats[0] <= 0.0f) {
		obj->x = player->x;
		obj->y = player->y;
		obj->anim_frame = 1;
	} else {
		if (player->flags & WOBJ_HFLIP)
			obj->x = player->x - cosf(melee->custom_floats[0]) * range;
		else
			obj->x = player->x + cosf(melee->custom_floats[0]) * range;
		obj->flags = (obj->flags & ~WOBJ_HFLIP) | (player->flags & WOBJ_HFLIP);
		obj->y = player->y - sinf(melee->custom_floats[0]) * range * (2.0f / 3.0f);
		obj->anim_frame = 0;
	}
	Wobj_UpdateGridPos(obj);
}
static void ItemGenericMelee_OnPickup(ITEM *melee, WOBJ *player)
{
	int wobj_type = WOBJ_NULL;

	switch (melee->type)
	{
	case ITEM_TYPE_KNIFE: wobj_type = WOBJ_KNIFE_ATTACK; break;
	case ITEM_TYPE_SWORD: wobj_type = WOBJ_SWORD_ATTACK; break;
	case ITEM_TYPE_GOLDEN_AXE: wobj_type = WOBJ_GOLDEN_AXE_ATTACK; break;
	}

	melee->melee_obj = Interaction_CreateWobj(wobj_type, player->x, player->y, 0, 0.0f);
	melee->melee_obj->strength = player->strength;
}
static void ItemGenericMelee_OnDrop(ITEM *melee, WOBJ *player)
{
	Interaction_DestroyWobj(melee->melee_obj);
}

static void ItemUltraSword_OnUpdate(ITEM *sword, WOBJ *player)
{
	if (sword->use_timer > 0 && (sword->use_timer - 1) % 5 == 0)
	{
		sword->hide_timer = ITEM_HIDE_TIMER;
		if (sword->custom_ints[0] > 3)
			return;
		WOBJ *b = Interaction_CreateWobj(ULTRA_SWORD_BOOMERANG, player->x, player->y, player->flags & WOBJ_HFLIP, 0.0f);
		switch (sword->custom_ints[0]++)
		{
		case 1: b->anim_frame = 1; break;
		case 2: b->anim_frame = 2; break;
		case 3: b->anim_frame = 3; break;
		default: b->anim_frame = 0; break;
		}
		b->flags |= WOBJ_IS_PLAYER_WEAPON | WOBJ_IS_PLAYER_BULLET;
		b->strength = player->strength + 0.14f;
		b->link_node = player->node_id;
		b->link_uuid = player->uuid;
		b->vel_x = player->vel_x / 3.0f;
		b->vel_y = player->vel_y;
	}
}
static void ItemUltraSword_OnUse(ITEM *sword, WOBJ *player)
{
	PLAYER_LOCAL_DATA *data = player->local_data;
	if (data->uswords_have >= 4)
	{
		sword->use_timer = 5*4+3;
		data->uswords_have = 0;
		sword->custom_ints[0] = 0;
	}
}

static void ItemGenericConsumeable_OnUse(ITEM *item, WOBJ *player)
{
	switch (item->type)
	{
	case ITEM_TYPE_APPLE:
	case ITEM_TYPE_CAKE:
	case ITEM_TYPE_CHEESEBURGER:
	case ITEM_TYPE_BEASTCHURGER:
		Interaction_PlaySound(player, 3); break;
	case ITEM_TYPE_HEALTH_POITION:
	case ITEM_TYPE_SPEED_POTION:
	case ITEM_TYPE_JUMP_POTION:
	case ITEM_TYPE_STRENGTH_POTION:
	case ITEM_TYPE_FIRE_POTION:
	case ITEM_TYPE_POISIONUS_JUMP_POTION:
	case ITEM_TYPE_POISIONUS_SPEED_POTION:
	case ITEM_TYPE_POISIONUS_STRENGTH_POTION:
	case ITEM_TYPE_MEGA_POTION:
	case ITEM_TYPE_ULTRA_MEGA_POTION:
		Interaction_PlaySound(player, 4); break;
	case ITEM_TYPE_50MONEY:
	case ITEM_TYPE_100MONEY:
	case ITEM_TYPE_500MONEY:
		Interaction_PlaySound(player, 5); break;
	case ITEM_TYPE_1UP_JUICE:
		Interaction_PlaySound(player, 55); break;
	case ITEM_TYPE_WRENCH:
		Interaction_PlaySound(player, 57); break;
	}

	PLAYER_LOCAL_DATA *pld;
	switch (item->type)
	{
	case ITEM_TYPE_APPLE: player->health += 25.0f; break;
	case ITEM_TYPE_CAKE: player->health += 60.0f; break;
	case ITEM_TYPE_STRENGTH_POTION: player->strength += 0.01f; break;
	case ITEM_TYPE_SPEED_POTION: player->speed += 0.5f; break;
	case ITEM_TYPE_JUMP_POTION: player->jump += 0.5f; break;
	case ITEM_TYPE_POISIONUS_STRENGTH_POTION: player->strength -= 1.0f; break;
	case ITEM_TYPE_POISIONUS_SPEED_POTION: player->speed -= 1.0f; break;
	case ITEM_TYPE_POISIONUS_JUMP_POTION: player->jump -= 0.5f; break;
	case ITEM_TYPE_HEALTH_POITION: player->health += 50.0f; break;
	case ITEM_TYPE_50MONEY: player->money += 50; break;
	case ITEM_TYPE_100MONEY: player->money += 100; break;
	case ITEM_TYPE_500MONEY: player->money += 500; break;
	case ITEM_TYPE_CHEESEBURGER: player->health += 35.0f; break;
	case ITEM_TYPE_BEASTCHURGER:
		pld = player->local_data;
		if (pld->beastchurger_timer < 0)
		{
			player->strength *= 25.0f;
			pld->beastchurger_music = Audio_GetCurrentPlayingMusic();
			Audio_PlayMusic(6, CNM_TRUE);
		}
		pld->beastchurger_timer = 30 * 20;
		break;
	case ITEM_TYPE_FIRE_POTION:
		((PLAYER_LOCAL_DATA *)player->local_data)->fire_resistance = 30 * 15;
		break;
	case ITEM_TYPE_MEGA_POTION: player->health += 1000.0f; break;
	case ITEM_TYPE_ULTRA_MEGA_POTION: player->health += 10000.0f; break;
	case ITEM_TYPE_1UP_JUICE: g_saves[g_current_save].lives++; break;
	case ITEM_TYPE_WRENCH:
		((PLAYER_LOCAL_DATA *)player->local_data)->offhand_durability =
			item_types[((PLAYER_LOCAL_DATA *)player->local_data)->offhand_item].max_durability;
		break;
	}

	//player->item = ITEM_TYPE_NOITEM;
	//player->item = ITEM_TYPE_NOITEM;
	Item_DestroyCurrentItem(player);
	if (_ghost_picked_up) Player_SwapOffhand(player);
}

static void ItemUnboundWand_Update(ITEM *wand, WOBJ *player)
{
	if (Wobj_GetWobjCollidingWithType(player, WOBJ_ICE_RUNE))
	{
		wand->type = ITEM_TYPE_ICE_WAND;
		goto got_wand;
	}
	if (Wobj_GetWobjCollidingWithType(player, WOBJ_AIR_RUNE))
	{
		wand->type = ITEM_TYPE_AIR_WAND;
		goto got_wand;
	}
	if (Wobj_GetWobjCollidingWithType(player, WOBJ_FIRE_RUNE))
	{
		wand->type = ITEM_TYPE_FIRE_WAND;
		goto got_wand;
	}
	if (Wobj_GetWobjCollidingWithType(player, WOBJ_LIGHTNING_RUNE))
	{
		wand->type = ITEM_TYPE_LIGHTNING_WAND;
		goto got_wand;
	}

	return;
got_wand:
	player->item = wand->type;
	if (item_types[wand->type].on_pickup != NULL)
		item_types[wand->type].on_pickup(wand, player);
}

static void ItemFireWand_OnUse(ITEM *item, WOBJ *player)
{
	WOBJ *fire = Interaction_CreateWobj(WOBJ_FIRE, player->x, player->y - 24.0f, 0, 0.0f);
	fire->flags |= WOBJ_IS_PLAYER_WEAPON;
	fire->speed = (player->flags & WOBJ_HFLIP) ? -6.0f : 6.0f;
	fire->strength = player->strength + 0.05f;
	item->use_timer = 20;
	Interaction_PlaySound(player, 9);
	item->durability -= 1.0f;
	item->hide_timer = ITEM_HIDE_TIMER;
}
static void ItemIceWand_OnUse(ITEM *item, WOBJ *player)
{
	WOBJ *ice = Interaction_CreateWobj(WOBJ_ICE_ATTACK, player->x, player->y - 24.0f, 0, 0.0f);
	ice->flags |= WOBJ_IS_PLAYER_WEAPON;
	ice->speed = (player->flags & WOBJ_HFLIP) ? -8.0f : 8.0f;
	ice->strength = player->strength + 0.01f;
	item->use_timer = 25;
	Interaction_PlaySound(player, 9);
	item->durability -= 1.0f;
	item->hide_timer = ITEM_HIDE_TIMER;
}
static void ItemAirWand_OnUse(ITEM *item, WOBJ *player)
{
	//player->y -= 3.0f;
	WOBJ *cloud = Interaction_CreateWobj
	(
		WOBJ_CLOUD_PLATFORM,
		player->x - 8.0f,
		player->y + player->hitbox.x + player->hitbox.h - 16.0f - 4.0f,
		0,
		0.0f
	);
	item->hide_timer = ITEM_HIDE_TIMER;
	if (!item->last_in_use)
		Interaction_PlaySound(player, 9);
	//item->use_timer = 1;
}
static void ItemLightningWand_OnUse(ITEM *item, WOBJ *player)
{
	const float speed = 18.0f;
	const float x_speeds[4] = 
	{
		speed, speed*0.707f, 0.0f, -speed*0.707f
	};
	const float y_speeds[4] =
	{
		0.0f, -speed*0.707f, -speed, -speed*0.707f
	};
	item->durability -= 1.0f;

	for (int i = 0; i < 8; i++)
	{
		WOBJ *p;
		p = Interaction_CreateWobj(WOBJ_LIGHTNING_ATTACK_PARTICLE, player->x, player->y, 0, 0.0f);
		p->flags |= WOBJ_IS_PLAYER_WEAPON;
		p->vel_x = x_speeds[i % 4] * ((i / 4) * 2 - 1);
		p->vel_y = y_speeds[i % 4] * ((i / 4) * 2 - 1);
		p->strength = player->strength + 0.2f;
	}

	Interaction_PlaySound(player, 9);
	item->use_timer = 30;
	item->hide_timer = ITEM_HIDE_TIMER;
}
static void ItemGoldenShotgun_OnUse(ITEM *shotgun, WOBJ *player)
{
	const float y[5] = {-1.8f, -0.8f, 0.0f, 0.8f, 1.8f};

	for (int i = 0; i < 5; i++)
	{
		WOBJ *w = Interaction_CreateWobj(WOBJ_VEL_PLAYER_PELLET,
										 player->x, player->y, 0,
										 (player->flags & WOBJ_HFLIP) ? -1.0f : 1.0f);
		w->flags |= WOBJ_IS_PLAYER_WEAPON;
		w->strength = (player->strength) + 0.1f;
		w->vel_x = (player->flags & WOBJ_HFLIP) ? -9.0f : 9.0f;
		w->vel_y = y[i];
	}
	shotgun->durability -= 1.0f;
	
	Interaction_PlaySound(player, 0);
	shotgun->use_timer = 10;
	shotgun->custom_timer = 10;
	Player_PlayShootAnim(player);
	shotgun->hide_timer = ITEM_HIDE_TIMER;
}

static void ItemLaserRifle_Update(ITEM *laser, WOBJ *player)
{
	if (laser->use_timer <= 1)
	{
		laser->melee_obj = NULL;
	}
	else
	{
		laser->melee_obj->x = player->x;
		laser->melee_obj->y = player->y;
		laser->melee_obj->custom_ints[0] = ((player->flags & WOBJ_HFLIP) ? -1 : 1);
	}
}
static void ItemLaserRifle_OnUse(ITEM *laser, WOBJ *player)
{
	WOBJ *l = Interaction_CreateWobj(WOBJ_PLAYER_LASER, player->x, player->y, ((player->flags & WOBJ_HFLIP) ? -1 : 1), 0.0f);
	l->strength = player->strength + 100.0f;
	l->flags |= WOBJ_IS_PLAYER_WEAPON;
	laser->use_timer = 30;
	laser->melee_obj = l;
	laser->hide_timer = ITEM_HIDE_TIMER;
}

static void ItemRocketLauncher_OnUse(ITEM *rpg, WOBJ *player)
{
	WOBJ *rocket = Interaction_CreateWobj(WOBJ_ROCKET, player->x, player->y, player->flags & WOBJ_HFLIP, 0.0f);
	rocket->strength = player->strength + 0.5f;// + 1000.0f;
	rocket->flags |= WOBJ_IS_PLAYER_WEAPON;
	rpg->use_timer = 13;
	Player_PlayShootAnim(player);
	rpg->durability -= 1.0f;
	rpg->hide_timer = ITEM_HIDE_TIMER;
}

static void ItemMinigun_OnUse(ITEM *minigun, WOBJ *player)
{
	int i;
	float d, x;
	d = 1.0f;
	x = 0.0f;
	if (player->flags & WOBJ_HFLIP)
	{
		d *= -1.0f;
		x = -64.0f+32.0f;
	}
	for (i = 0; i < 4; i++) {
		if (minigun->objs[i] == NULL) {
			WOBJ *p = Interaction_CreateWobj(WOBJ_PLAYER_MINIGUN_PELLET, player->x + x, player->y, 0, 0.0f);
			p->flags |= WOBJ_IS_PLAYER_WEAPON;
			if (player->flags & WOBJ_HFLIP)
				p->flags |= WOBJ_HFLIP;
			p->strength = player->strength + 0.25f;
			x += 63.0f*d;
			minigun->objs[i] = p;
		}
	}

	if (!Audio_IsSoundPlaying(10))
	{
		Interaction_PlaySound(player, 10);
	}
}
static void ItemMinigun_OnPickup(ITEM *minigun, WOBJ *player)
{
	memset(minigun->objs, 0, sizeof(minigun->objs));
}
static void ItemMinigun_OnUpdate(ITEM *minigun, WOBJ *player)
{
	int i;
	float x, d;
	d = 1.0f;
	x = 0.0f;
	if (player->flags & WOBJ_HFLIP) {
		d *= -1.0f;
		x = -64.0f + 32.0f;
	}
	for (i = 0; i < 4; i++)
	{
		if (minigun->objs[i] != NULL) {
			minigun->hide_timer = ITEM_HIDE_TIMER;
			minigun->objs[i]->x = player->x + x;
			minigun->objs[i]->y = player->y;
			if (player->flags & WOBJ_HFLIP)
				minigun->objs[i]->flags |= WOBJ_HFLIP;
			else
				minigun->objs[i]->flags &= ~WOBJ_HFLIP;
		}
		x += 63.0f * d;

		if (minigun->objs[i] != NULL && !minigun->in_use)
		{
			Interaction_DestroyWobj(minigun->objs[i]);
			minigun->objs[i] = NULL;
		}
	}

	if (minigun->in_use) {
		minigun->durability -= 0.1f;
	}
}
static void ItemMinigun_OnDrop(ITEM *minigun, WOBJ *player)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		if (minigun->objs[i] != NULL)
		{
			Interaction_DestroyWobj(minigun->objs[i]);
			minigun->objs[i] = NULL;
		}
	}
}

static void ItemFlameThrower_OnUpdate(ITEM *flamethrower, WOBJ *player)
{
	if (Game_GetFrame() % 10 == 0 && flamethrower->custom_ints[0] > 0 && flamethrower->use_timer <= 0)
	{
		flamethrower->custom_ints[0]--;
	}
}
static void ItemFlameThrower_OnUse(ITEM *flamethrower, WOBJ *player)
{
	float ang = (Util_RandFloat() - 0.5f);
	if (player->flags & WOBJ_HFLIP)
		ang += CNM_PI;
	Interaction_PlaySound(player, 16);
	WOBJ *flame = Interaction_CreateWobj(WOBJ_FLAMETHROWER_FLAME, player->x, player->y, 0, ang);
	flame->flags |= WOBJ_IS_PLAYER_WEAPON;
	flame->strength = player->strength + 0.01f;
	flame->speed += fabsf(player->vel_x);
	flamethrower->use_timer = 3;
	flamethrower->custom_ints[0] += 1;
	flamethrower->durability -= 0.5f;
	flamethrower->hide_timer = ITEM_HIDE_TIMER;
	if (flamethrower->custom_ints[0] > 10)
	{
		flamethrower->use_timer = 45;
		flamethrower->custom_ints[0] = 0;
	}
}

static void ItemHeavyHammer_OnUse(ITEM *hammer, WOBJ *player)
{
	hammer->melee_obj = Interaction_CreateWobj(HEAVY_HAMMER_SWING, player->x, player->y, player->flags & WOBJ_HFLIP, 0.0f);
	hammer->melee_obj->link_node = player->node_id;
	hammer->melee_obj->link_uuid = player->uuid;
	hammer->melee_obj->strength = player->strength + 0.4f;
	hammer->use_timer = 14;
	hammer->custom_ints[0] = hammer->melee_obj->node_id;
	hammer->custom_ints[1] = hammer->melee_obj->uuid;
	hammer->hide_timer = ITEM_HIDE_TIMER;
	Player_PlayMeleeAnim(player);
}
static void ItemHeavyHammer_OnUpdate(ITEM *hammer, WOBJ *player)
{
	static const float x_offsets[6] = {-4.0f, 0.0f, 4.0f, 8.0f, 14.0f, 32.0f};
	static const float y_offsets[6] = {-24.0f, -28.0f, -20.0f, -16.0f, -10.0f, 0.0f};

	if (hammer->use_timer > 0)
	{
		WOBJ *hobj = Wobj_GetAnyWOBJFromUUIDAndNode
		(
			hammer->custom_ints[0], hammer->custom_ints[1]
		);
		if (hobj != NULL)
		{
			hobj->x = player->x + x_offsets[hobj->anim_frame] * hobj->custom_floats[0];
			hobj->y = player->y + y_offsets[hobj->anim_frame];
		}
	}
}

static void ItemFissionGun_OnUse(ITEM *gun, WOBJ *player)
{
	WOBJ *bolt = Interaction_CreateWobj(FISSION_ENERGY_BOLT, player->x, player->y, ((player->flags & WOBJ_HFLIP) ? 2 : 0), 0.0f);
	bolt->strength = player->strength + 0.01f;
	bolt->custom_ints[1] = 0;
	gun->use_timer = 20;
	gun->custom_timer = 20;
	gun->durability -= 1.5f;
	gun->hide_timer = ITEM_HIDE_TIMER;
}

static void ItemKeyGeneric_Update(ITEM *key, WOBJ *player)
{
	WOBJ *colliding_block = NULL;
	CNM_BOX old;

	memcpy(&old, &player->hitbox, sizeof(CNM_BOX));
	player->hitbox.x -= 8.0f;
	player->hitbox.y -= 8.0f;
	player->hitbox.w += 16.0f;
	player->hitbox.h += 16.0f;

	switch (key->type)
	{
	case ITEM_TYPE_KEY_RED:
		colliding_block = Wobj_GetWobjCollidingWithType(player, WOBJ_LOCKED_BLOCK_RED);
		break;
	case ITEM_TYPE_KEY_GREEN:
		colliding_block = Wobj_GetWobjCollidingWithType(player, WOBJ_LOCKED_BLOCK_GREEN);
		break;
	case ITEM_TYPE_KEY_BLUE:
		colliding_block = Wobj_GetWobjCollidingWithType(player, WOBJ_LOCKED_BLOCK_BLUE);
		break;
	}

	memcpy(&player->hitbox, &old, sizeof(CNM_BOX));

	if (colliding_block != NULL)
	{
		Interaction_DestroyWobj(colliding_block);
		Interaction_PlaySound(player, 35);
		if (colliding_block->custom_ints[0]) {
			Item_DestroyCurrentItem(player);
			if (_ghost_picked_up) Player_SwapOffhand(player);
		}
	}
}
