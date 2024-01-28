// REMOVED GAME LUA SUPPORT
/*#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "game.h"
#include "wobj.h"
#include "gamelua.h"
#include "console.h"
#include "enemies.h"
#include "input.h"
#include "item.h"

static int luastring_len;
static int luastring_cap;
static char *luastring;

static lua_State *autorun_L;

#define XFUNCS X(SetWobjTypeStrengthReward)\
X(SetWobjTypeStrengthReward)\
X(SetWobjTypeMoneyReward)\
X(SetWobjTypeFrame)\
X(SetWobjTypeDoInterp)\
X(SetWobjTypeRespawnable)\
X(ConsolePrint)\
X(NormalID)\
X(GetX)\
X(GetY)\
X(GetVelX)\
X(GetVelY)\
X(GetFloat)\
X(GetInt)\
X(GetSpeed)\
X(GetJump)\
X(GetStrength)\
X(GetHealth)\
X(GetMoney)\
X(GetAnimFrame)\
X(GetItem)\
X(GetFlag)\
X(SetX)\
X(SetY)\
X(SetVelX)\
X(SetVelY)\
X(SetFloat)\
X(SetInt)\
X(SetSpeed)\
X(SetJump)\
X(SetStrength)\
X(SetHealth)\
X(SetMoney)\
X(SetAnimFrame)\
X(SetItem)\
X(SetFlag)\
X(SetHitbox)\
X(DoEnemyCry)\
X(PlaySound)\
X(IsCollidingWithBlocks)\
X(ApplyWindForces)\
X(TryTeleport)\
X(ResolveBlocksCollision)\
X(GetFrame)\
X(DestroyWobj)\
X(GetClosestPlayer)\
X(DistanceToWobj)\
X(CreateWobj)\
X(RandInt)\
X(RandFloat)\
X(PhysicsBegin)\
X(PhysicsEnd)\
X(ApplyGravity)\
X(SlimeAI)\
X(HeavyAI)\
X(SpiderAI)\
X(DragonAI)\
X(FireballAI)\
X(BozoPinAI)\
X(SpikeGuyAI)\
X(SlimeAICreate)\
X(HeavyAICreate)\
X(SpiderAICreate)\
X(DragonAICreate)\
X(FireballAICreate)\
X(BozoPinAICreate)\
X(SpikeGuyAICreate)\
X(IsGrounded)\
X(GetLinkNode)\
X(GetLinkUUID)\
X(SetLinkNode)\
X(SetLinkUUID)\
X(GetNode)\
X(GetUUID)\
X(IsCollidingWithType)\
X(IsCollidingWithFlags)\
X(GetLinkWobj)\
X(Exists)\
X(GetButton)\
X(GetButtonPressed)\
X(ApplyPlayerWeaponDamage)

#define X(I) static int gamelua_##I(lua_State *L);
XFUNCS
#undef X
#define X(I) lua_register(autorun_L, XSTRINGIZE(I), gamelua_##I);
static void GameLua_RegisterFuncs(void) {
	XFUNCS
}
#undef X

static void GameLua_CreateGlobalInt(const char *name, int value) {
	lua_pushinteger(autorun_L, value);
	lua_setglobal(autorun_L, name);
}
static void GameLua_RegisterVariables(void) {
	GameLua_CreateGlobalInt("wobj_slime", WOBJ_SLIME);
	GameLua_CreateGlobalInt("wobj_player", WOBJ_PLAYER);
	GameLua_CreateGlobalInt("wobj_dropped_item", WOBJ_DROPPED_ITEM);
	GameLua_CreateGlobalInt("wobj_crystal_wings", WOBJ_UPGRADE_CRYSTAL_WINGS);
	GameLua_CreateGlobalInt("wobj_heavy", WOBJ_HEAVY);
	GameLua_CreateGlobalInt("wobj_heavy_blast", WOBJ_HEAVY_BLAST);
	GameLua_CreateGlobalInt("wobj_dragon", WOBJ_DRAGON);
	GameLua_CreateGlobalInt("wobj_fireball", WOBJ_FIREBALL);
	GameLua_CreateGlobalInt("wobj_bozopin", WOBJ_BOZO_PIN);
	GameLua_CreateGlobalInt("wobj_bozo", WOBJ_BOZO);
	GameLua_CreateGlobalInt("wobj_silver_slime", WOBJ_SILVER_SLIME);
	GameLua_CreateGlobalInt("wobj_lava_monster", LAVA_MONSTER);
	GameLua_CreateGlobalInt("wobj_ttminion_small", TT_MINION_SMALL);
	GameLua_CreateGlobalInt("wobj_ttminion_big", TT_MINION_BIG);
	GameLua_CreateGlobalInt("wobj_slime_walker", SLIME_WALKER);
	GameLua_CreateGlobalInt("wobj_megafish", MEGA_FISH);
	GameLua_CreateGlobalInt("wobj_lava_dragon", LAVA_DRAGON_HEAD);
	GameLua_CreateGlobalInt("wobj_eater_bug", EATER_BUG);
	GameLua_CreateGlobalInt("wobj_spider_walker", SPIDER_WALKER);
	GameLua_CreateGlobalInt("wobj_spike_guy", SPIKE_GUY);
	GameLua_CreateGlobalInt("wobj_bandit_guy", BANDIT_GUY);
	GameLua_CreateGlobalInt("wobj_kamakazi_slime", WOBJ_KAMAKAZI_SLIME);
	GameLua_CreateGlobalInt("wobj_rockguy_medium", ROCK_GUY_MEDIUM);
	GameLua_CreateGlobalInt("wobj_rockguy_small", ROCK_GUY_SMALL1);
	GameLua_CreateGlobalInt("wobj_rockguy_slider", ROCK_GUY_SLIDER);
	GameLua_CreateGlobalInt("wobj_rockguy_smasher", ROCK_GUY_SMASHER);
	GameLua_CreateGlobalInt("wobj_vortex", WOBJ_VORTEX);
	GameLua_CreateGlobalInt("wobj_wolf", WOBJ_WOLF);
	GameLua_CreateGlobalInt("wobj_supervirus", WOBJ_SUPERVIRUS);
	GameLua_CreateGlobalInt("wobj_lua0", WOBJ_LUA0);
	GameLua_CreateGlobalInt("wobj_lua1", WOBJ_LUA1);
	GameLua_CreateGlobalInt("wobj_lua2", WOBJ_LUA2);
	GameLua_CreateGlobalInt("wobj_lua3", WOBJ_LUA3);
	GameLua_CreateGlobalInt("wobj_lua4", WOBJ_LUA4);
	GameLua_CreateGlobalInt("wobj_lua5", WOBJ_LUA5);
	GameLua_CreateGlobalInt("wobj_lua6", WOBJ_LUA6);
	GameLua_CreateGlobalInt("wobj_lua7", WOBJ_LUA7);
	GameLua_CreateGlobalInt("wobj_lua8", WOBJ_LUA8);
	GameLua_CreateGlobalInt("wobj_lua9", WOBJ_LUA9);
	GameLua_CreateGlobalInt("wobj_lua10", WOBJ_LUA10);
	GameLua_CreateGlobalInt("wobj_lua11", WOBJ_LUA11);
	GameLua_CreateGlobalInt("wobj_lua12", WOBJ_LUA12);
	GameLua_CreateGlobalInt("wobj_lua13", WOBJ_LUA13);
	GameLua_CreateGlobalInt("wobj_lua14", WOBJ_LUA14);
	GameLua_CreateGlobalInt("wobj_lua15", WOBJ_LUA15);
	GameLua_CreateGlobalInt("flag_player_weapon", WOBJ_IS_PLAYER_WEAPON);
	GameLua_CreateGlobalInt("flag_player", WOBJ_IS_PLAYER);
	GameLua_CreateGlobalInt("flag_hostile", WOBJ_IS_HOSTILE);
	GameLua_CreateGlobalInt("flag_flip_x", WOBJ_HFLIP);
	GameLua_CreateGlobalInt("flag_flip_y", WOBJ_VFLIP);
	GameLua_CreateGlobalInt("flag_invuln", WOBJ_INVULN);
	GameLua_CreateGlobalInt("flag_light_small", WOBJ_LIGHT_SMALL);
	GameLua_CreateGlobalInt("flag_light_big", WOBJ_LIGHT_BIG);
	GameLua_CreateGlobalInt("flag_solid", WOBJ_IS_SOLID);
	GameLua_CreateGlobalInt("flag_movestand", WOBJ_IS_MOVESTAND);
	GameLua_CreateGlobalInt("flag_breakable", WOBJ_IS_BREAKABLE);
	GameLua_CreateGlobalInt("flag_grounded", WOBJ_IS_GROUNDED);
	GameLua_CreateGlobalInt("flag_dont_draw", WOBJ_DONT_DRAW);
	GameLua_CreateGlobalInt("btn_left", INPUT_LEFT);
	GameLua_CreateGlobalInt("btn_right", INPUT_RIGHT);
	GameLua_CreateGlobalInt("btn_up", INPUT_UP);
	GameLua_CreateGlobalInt("btn_down", INPUT_DOWN);
	GameLua_CreateGlobalInt("btn_a", INPUT_A);
	GameLua_CreateGlobalInt("btn_d", INPUT_D);
	GameLua_CreateGlobalInt("btn_w", INPUT_W);
	GameLua_CreateGlobalInt("btn_s", INPUT_S);
	GameLua_CreateGlobalInt("btn_drop", INPUT_DROP);
	GameLua_CreateGlobalInt("btn_enter", INPUT_ENTER);
	GameLua_CreateGlobalInt("item_none", ITEM_TYPE_NOITEM);
	GameLua_CreateGlobalInt("item_shotgun", ITEM_TYPE_SHOTGUN);
	GameLua_CreateGlobalInt("item_knife", ITEM_TYPE_KNIFE);
	GameLua_CreateGlobalInt("item_apple", ITEM_TYPE_APPLE);
	GameLua_CreateGlobalInt("item_cake", ITEM_TYPE_CAKE);
	GameLua_CreateGlobalInt("item_strength_pot", ITEM_TYPE_STRENGTH_POTION);
	GameLua_CreateGlobalInt("item_speed_pot", ITEM_TYPE_SPEED_POTION);
	GameLua_CreateGlobalInt("item_jump_pot", ITEM_TYPE_JUMP_POTION);
	GameLua_CreateGlobalInt("item_sword", ITEM_TYPE_SWORD);
	GameLua_CreateGlobalInt("item_health_pot", ITEM_TYPE_HEALTH_POITION);
	GameLua_CreateGlobalInt("item_sniper", ITEM_TYPE_SNIPER);
	GameLua_CreateGlobalInt("item_50money", ITEM_TYPE_50MONEY);
	GameLua_CreateGlobalInt("item_100money", ITEM_TYPE_100MONEY);
	GameLua_CreateGlobalInt("item_500money", ITEM_TYPE_500MONEY);
	GameLua_CreateGlobalInt("item_cheeseburger", ITEM_TYPE_CHEESEBURGER);
	GameLua_CreateGlobalInt("item_golden_axe", ITEM_TYPE_GOLDEN_AXE);
	GameLua_CreateGlobalInt("item_unbound_wand", ITEM_TYPE_UNBOUND_WAND);
	GameLua_CreateGlobalInt("item_fire_wand", ITEM_TYPE_FIRE_WAND);
	GameLua_CreateGlobalInt("item_ice_wand", ITEM_TYPE_ICE_WAND);
	GameLua_CreateGlobalInt("item_air_wand", ITEM_TYPE_AIR_WAND);
	GameLua_CreateGlobalInt("item_lightning_wand", ITEM_TYPE_LIGHTNING_WAND);
	GameLua_CreateGlobalInt("item_golden_shotgun", ITEM_TYPE_GOLDEN_SHOTGUN);
	GameLua_CreateGlobalInt("item_laser_rifle", ITEM_TYPE_LASER_RIFLE);
	GameLua_CreateGlobalInt("item_rocket_launcher", ITEM_TYPE_ROCKET_LAUNCHER);
	GameLua_CreateGlobalInt("item_fire_pot", ITEM_TYPE_FIRE_POTION);
	GameLua_CreateGlobalInt("item_minigun", ITEM_TYPE_MINIGUN);
	GameLua_CreateGlobalInt("item_mega_pot", ITEM_TYPE_MEGA_POTION);
	GameLua_CreateGlobalInt("item_ultra_pot", ITEM_TYPE_ULTRA_MEGA_POTION);
	GameLua_CreateGlobalInt("item_awp", ITEM_TYPE_AWP);
	GameLua_CreateGlobalInt("item_flamethrower", ITEM_TYPE_FLAMETHROWER);
	GameLua_CreateGlobalInt("item_poision_strength_pot", ITEM_TYPE_POISIONUS_STRENGTH_POTION);
	GameLua_CreateGlobalInt("item_poision_speed_pot", ITEM_TYPE_POISIONUS_SPEED_POTION);
	GameLua_CreateGlobalInt("item_poision_jump_pot", ITEM_TYPE_POISIONUS_JUMP_POTION);
	GameLua_CreateGlobalInt("item_beastchurger", ITEM_TYPE_BEASTCHURGER);
	GameLua_CreateGlobalInt("item_ultra_sword", ITEM_TYPE_ULTRA_SWORD);
	GameLua_CreateGlobalInt("item_heavy_hammer", ITEM_TYPE_HEAVY_HAMMER);
	GameLua_CreateGlobalInt("item_fission_gun", ITEM_TYPE_FISSION_GUN);
	GameLua_CreateGlobalInt("item_red_key", ITEM_TYPE_KEY_RED);
	GameLua_CreateGlobalInt("item_green_key", ITEM_TYPE_KEY_GREEN);
	GameLua_CreateGlobalInt("item_blue_key", ITEM_TYPE_KEY_BLUE);
}

int CheckLua(lua_State *L, int r) {
	if (r != LUA_OK) {
		Console_Print(lua_tostring(L, -1));
		return 0;
	}
	return 1;
}

void AutorunLua_ClearPrgm(void)
{
	free(luastring);
	luastring_cap = 0;
	luastring_len = 0;
	luastring = NULL;

	if (autorun_L != NULL) {
		lua_close(autorun_L);
		autorun_L = NULL;
	}
}
void AutorunLua_AddLine(const char *_line)
{
	char line[GAMELUA_LINE_SIZE+2];
	strncpy(line, _line, GAMELUA_LINE_SIZE);
	line[GAMELUA_LINE_SIZE] = '\0';
	strcat(line, "\n");

	luastring_len += strlen(line);
	if (luastring_cap < luastring_len) {
		luastring_cap = luastring_len + GAMELUA_LINE_SIZE * 15;
		if (luastring != NULL) {
			luastring = realloc(luastring, luastring_cap);
		}
		else {
			luastring = malloc(GAMELUA_LINE_SIZE * 35);
			luastring[0] = '\0';
		}
	}

	strcat(luastring, line);
}
void AutorunLua_Run(void)
{
	autorun_L = luaL_newstate();
	luaL_openlibs(autorun_L);
	GameLua_RegisterFuncs();
	GameLua_RegisterVariables();
	CheckLua(autorun_L, luaL_dostring(autorun_L, luastring));
}

int NormalID_To_LuaID(int normalid) {
	if (normalid >= WOBJ_LUA0 && normalid <= WOBJ_LUA15)
		return normalid - WOBJ_LUA0;
	else
		return WOBJ_LUA0;
}
int LuaID_To_NormalID(int luaid) {
	if (luaid >= 0 && luaid <= 15)
		return luaid + WOBJ_LUA0;
	else
		return WOBJ_LUA0;
}

void WobjLua_Create(WOBJ *wobj)
{
	char funcname[64];
	sprintf(funcname, "LUA_CREATE%d", NormalID_To_LuaID(wobj->type));
	if (lua_getglobal(autorun_L, funcname) && lua_isfunction(autorun_L, -1)) {
		lua_pushlightuserdata(autorun_L, wobj);
		CheckLua(autorun_L, lua_pcall(autorun_L, 1, 0, 0));
	}
	else {
		lua_pop(autorun_L, 1);
	}
}
void WobjLua_Update(WOBJ *wobj)
{
	char funcname[64];
	sprintf(funcname, "LUA_UPDATE%d", NormalID_To_LuaID(wobj->type));
	if (lua_getglobal(autorun_L, funcname) && lua_isfunction(autorun_L, -1))
	{
		lua_pushlightuserdata(autorun_L, wobj);
		CheckLua(autorun_L, lua_pcall(autorun_L, 1, 0, 0));
	}
	else {
		lua_pop(autorun_L, 1);
	}
}
void WobjLua_Draw(WOBJ *wobj, int camx, int camy)
{
	WobjGeneric_Draw(wobj, camx, camy);
}

static int gamelua_SetWobjTypeStrengthReward(lua_State *L) {
	float strength = (float)lua_tonumber(L, 2);
	int type = (int)lua_tonumber(L, 1);
	wobj_types[type].strength_reward = strength;
	return 0;
}
static int gamelua_SetWobjTypeMoneyReward(lua_State *L)
{
	int type = (int)lua_tonumber(L, 1);
	int money = (int)lua_tonumber(L, 2);
	wobj_types[type].money_reward = money;
	return 0;
}
static int gamelua_SetWobjTypeFrame(lua_State *L)
{
	int type = (int)lua_tonumber(L, 1);
	int frame = (int)lua_tonumber(L, 2);
	int x = (int)lua_tonumber(L, 3);
	int y = (int)lua_tonumber(L, 4);
	int w = (int)lua_tonumber(L, 5);
	int h = (int)lua_tonumber(L, 6);
	Util_SetRect(&wobj_types[type].frames[frame], x, y, w, h);
	return 0;
}
static int gamelua_SetWobjTypeDoInterp(lua_State *L)
{
	int type = (int)lua_tonumber(L, 1);
	int dointerp = (int)lua_tonumber(L, 2);
	wobj_types[type].do_interp = dointerp;
	return 0;
}
static int gamelua_SetWobjTypeRespawnable(lua_State *L)
{
	int type = (int)lua_tonumber(L, 1);
	int respawnable = (int)lua_tonumber(L, 2);
	wobj_types[type].respawnable = respawnable;
	return 0;
}
static int gamelua_ConsolePrint(lua_State *L) {
	const char *str = lua_tostring(L, 1);
	Console_Print("%s", str);
	return 0;
}
static int gamelua_NormalID(lua_State *L)
{
	int idx = (int)lua_tonumber(L, 1);
	lua_pushinteger(L, LuaID_To_NormalID(idx));
	return 1;
}

static int gamelua_GetX(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (float)w->x);
	return 1;
}
static int gamelua_GetY(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (float)w->y);
	return 1;
}
static int gamelua_GetVelX(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (float)w->vel_x);
	return 1;
}
static int gamelua_GetVelY(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (float)w->vel_y);
	return 1;
}
static int gamelua_GetFloat(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	int idx = (int)lua_tonumber(L, 2);
	lua_pushnumber(L, (float)w->custom_floats[idx]);
	return 1;
}
static int gamelua_GetInt(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	int idx = (int)lua_tonumber(L, 2);
	lua_pushnumber(L, (int)w->custom_ints[idx]);
	return 1;
}
static int gamelua_GetSpeed(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (float)w->speed);
	return 1;
}
static int gamelua_GetJump(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (float)w->jump);
	return 1;
}
static int gamelua_GetStrength(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (float)w->strength);
	return 1;
}
static int gamelua_GetHealth(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (float)w->health);
	return 1;
}
static int gamelua_GetMoney(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (int)w->money);
	return 1;
}
static int gamelua_GetAnimFrame(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (int)w->anim_frame);
	return 1;
}
static int gamelua_GetItem(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, 0);
		return 1;
	}
	lua_pushnumber(L, (int)w->item);
	return 1;
}
static int gamelua_GetFlag(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1))
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	int idx = (int)lua_tonumber(L, 2);
	lua_pushboolean(L, (int)w->flags & idx);
	return 1;
}
static int gamelua_SetX(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	float i = (float)lua_tonumber(L, 2);
	w->x = i;
	if (Interaction_GetMode() != INTERACTION_MODE_SINGLEPLAYER &&
		!w->internal.owned) {
		Interaction_ForceWobjPosition(w, w->x, w->y);
	}
	return 0;
}
static int gamelua_SetY(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	float i = (float)lua_tonumber(L, 2);
	w->y = i;
	if (Interaction_GetMode() != INTERACTION_MODE_SINGLEPLAYER &&
		!w->internal.owned)
	{
		Interaction_ForceWobjPosition(w, w->x, w->y);
	}
	return 0;
}
static int gamelua_SetVelX(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	float i = (float)lua_tonumber(L, 2);
	w->vel_x = i;
	return 0;
}
static int gamelua_SetVelY(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	float i = (float)lua_tonumber(L, 2);
	w->vel_y = i;
	return 0;
}
static int gamelua_SetFloat(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	int idx = (int)lua_tonumber(L, 2);
	float i = (float)lua_tonumber(L, 3);
	w->custom_floats[idx] = i;
	return 0;
}
static int gamelua_SetInt(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	int idx = (int)lua_tonumber(L, 2);
	int i = (int)lua_tonumber(L, 3);
	w->custom_ints[idx] = i;
	return 0;
}
static int gamelua_SetSpeed(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	float i = (float)lua_tonumber(L, 2);
	w->speed = i;
	return 0;
}
static int gamelua_SetJump(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	float i = (float)lua_tonumber(L, 2);
	w->jump = i;
	return 0;
}
static int gamelua_SetStrength(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	float i = (float)lua_tonumber(L, 2);
	w->strength = i;
	return 0;
}
static int gamelua_SetHealth(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	float i = (float)lua_tonumber(L, 2);
	w->health = i;
	return 0;
}
static int gamelua_SetMoney(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	int i = (int)lua_tonumber(L, 2);
	w->money = i;
	return 0;
}
static int gamelua_SetAnimFrame(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	int i = (int)lua_tonumber(L, 2);
	w->anim_frame = i;
	return 0;
}
static int gamelua_SetItem(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	int i = (int)lua_tonumber(L, 2);
	w->item = i;
	return 0;
}
static int gamelua_SetFlag(lua_State *L)
{
	WOBJ *w = (WOBJ *)lua_touserdata(L, 1);
	if (w == NULL || lua_isnil(L, 1)) return 0;
	int idx = (int)lua_tonumber(L, 2);
	int val = lua_toboolean(L, 3);
	if (val)
		w->flags |= idx;
	else
		w->flags &= ~idx;
	return 0;
}
static int gamelua_SetHitbox(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	float x = (float)lua_tonumber(L, 2);
	float y = (float)lua_tonumber(L, 3);
	float w = (float)lua_tonumber(L, 4);
	float h = (float)lua_tonumber(L, 5);
	Util_SetBox(&wobj->hitbox, x, y, w, h);
	return 0;
}
static int gamelua_DoEnemyCry(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	Wobj_DoEnemyCry(wobj, (int)lua_tonumber(L, 2));
	return 0;
}
static int gamelua_PlaySound(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	Interaction_PlaySound(wobj, (int)lua_tonumber(L, 2));
	return 0;
}
static int gamelua_IsCollidingWithBlocks(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	float xoff = (float)lua_tonumber(L, 2);
	float yoff = (float)lua_tonumber(L, 3);
	lua_pushboolean(L, Wobj_IsCollidingWithBlocks(wobj, xoff, yoff));
	return 1;
}
static int gamelua_ApplyWindForces(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjPhysics_ApplyWindForces(wobj);
	return 0;
}
static int gamelua_TryTeleport(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	Wobj_TryTeleportWobj(wobj);
	return 0;
}
static int gamelua_ResolveBlocksCollision(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	Wobj_ResolveBlocksCollision(wobj);
	return 0;
}
static int gamelua_GetFrame(lua_State *L)
{
	lua_pushnumber(L, (lua_Number)Game_GetFrame());
	return 1;
}
static int gamelua_DestroyWobj(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	Interaction_DestroyWobj(wobj);
	return 0;
}
static int gamelua_GetClosestPlayer(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		lua_pushlightuserdata(L, NULL);
		return 1;
	}
	lua_pushlightuserdata(L, Interaction_GetNearestPlayerToPoint(wobj->x, wobj->y));
	return 1;
}
static int gamelua_DistanceToWobj(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	WOBJ *wobj2 = (WOBJ *)lua_touserdata(L, 2);
	if (wobj == NULL || wobj2 == NULL || lua_isnil(L, 1) || lua_isnil(L, 2))
	{
		lua_pushnumber(L, INFINITY);
		return 1;
	}
	lua_pushnumber(L, (lua_Number)Interaction_GetDistanceToWobj(wobj, wobj2));
	return 1;
}
static int gamelua_CreateWobj(lua_State *L)
{
	WOBJ *w;
	int type = (int)lua_tonumber(L, 1);
	float x = (float)lua_tonumber(L, 2);
	float y = (float)lua_tonumber(L, 3);
	int ci = (int)lua_tonumber(L, 4);
	float cf = (float)lua_tonumber(L, 5);
	w = Interaction_CreateWobj(type, x, y, ci, cf);
	lua_pushlightuserdata(L, w);
	return 1;
}
static int gamelua_RandInt(lua_State *L)
{
	int max = (int)lua_tonumber(L, 1);
	lua_pushnumber(L, (lua_Number)(rand() % max));
	return 1;
}
static int gamelua_RandFloat(lua_State *L)
{
	lua_pushnumber(L, (lua_Number)Util_RandFloat());
	return 1;
}
static int gamelua_PhysicsBegin(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjPhysics_BeginUpdate(wobj);
	return 0;
}
static int gamelua_PhysicsEnd(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjPhysics_EndUpdate(wobj);
	return 0;
}
static int gamelua_ApplyGravity(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	wobj->vel_y += Game_GetVar(GAME_VAR_GRAVITY)->data.decimal;
	return 0;
}
static int gamelua_SlimeAI(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjSlime_Update(wobj);
	return 0;
}
static int gamelua_HeavyAI(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjHeavy_Update(wobj);
	return 0;
}
static int gamelua_SpiderAI(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjSpiderWalker_Update(wobj);
	return 0;
}
static int gamelua_DragonAI(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjDragon_Update(wobj);
	return 0;
}
static int gamelua_FireballAI(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjFireball_Update(wobj);
	return 0;
}
static int gamelua_BozoPinAI(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjBozoPin_Update(wobj);
	return 0;
}
static int gamelua_SpikeGuyAI(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjSpikeGuy_Update(wobj);
	return 0;
}
static int gamelua_SlimeAICreate(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjSlime_Create(wobj);
	return 0;
}
static int gamelua_HeavyAICreate(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjHeavy_Create(wobj);
	return 0;
}
static int gamelua_SpiderAICreate(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjSpiderWalker_Create(wobj);
	return 0;
}
static int gamelua_DragonAICreate(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjDragon_Create(wobj);
	return 0;
}
static int gamelua_FireballAICreate(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjFireball_Create(wobj);
	return 0;
}
static int gamelua_BozoPinAICreate(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjBozoPin_Create(wobj);
	return 0;
}
static int gamelua_SpikeGuyAICreate(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	WobjSpikeGuy_Create(wobj);
	return 0;
}
static int gamelua_IsGrounded(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) {
		lua_pushboolean(L, 0);
		return 1;
	}
	lua_pushboolean(L, Wobj_IsGrouneded(wobj));
	return 1;
}
static int gamelua_GetLinkNode(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	lua_pushnumber(L, wobj->link_node);
	return 1;
}
static int gamelua_GetLinkUUID(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	lua_pushnumber(L, wobj->link_uuid);
	return 1;
}
static int gamelua_SetLinkNode(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	int link_node = (int)lua_tonumber(L, 2);
	wobj->link_node = link_node;
	return 0;
}
static int gamelua_SetLinkUUID(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1)) return 0;
	int link_uuid = (int)lua_tonumber(L, 2);
	wobj->link_uuid = link_uuid;
	return 0;
}
static int gamelua_GetNode(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	lua_pushnumber(L, wobj->node_id);
	return 1;
}
static int gamelua_GetUUID(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		lua_pushnumber(L, -1);
		return 1;
	}
	lua_pushnumber(L, wobj->uuid);
	return 1;
}
static int gamelua_IsCollidingWithType(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		lua_pushlightuserdata(L, NULL);
		return 1;
	}
	int type = (int)lua_tonumber(L, 2);
	float offx = (float)lua_tonumber(L, 3);
	float offy = (float)lua_tonumber(L, 4);
	float offhx = (float)lua_tonumber(L, 5);
	float offhy = (float)lua_tonumber(L, 6);
	float ox = wobj->x, oy = wobj->y;
	CNM_BOX oldh;

	memcpy(&oldh, &wobj->hitbox, sizeof(oldh));
	wobj->hitbox.x += offhx;
	wobj->hitbox.y += offhy;
	wobj->hitbox.w -= offhx * 2.0f;
	wobj->hitbox.h -= offhy * 2.0f;
	if (wobj->hitbox.w < 0.0f) wobj->hitbox.w = 0.0f;
	if (wobj->hitbox.h < 0.0f) wobj->hitbox.h = 0.0f;
	wobj->x = ox + offx;
	wobj->y = oy + offy;
	WOBJ *a = Wobj_GetWobjCollidingWithType(wobj, type);
	wobj->x = ox;
	wobj->y = oy;
	memcpy(&wobj->hitbox, &oldh, sizeof(oldh));

	if (a != NULL) lua_pushlightuserdata(L, a);
	else lua_pushlightuserdata(L, NULL);
	return 1;
}
static int gamelua_IsCollidingWithFlags(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		lua_pushlightuserdata(L, NULL);
		return 1;
	}
	int flags = (int)lua_tonumber(L, 2);
	float offx = (float)lua_tonumber(L, 3);
	float offy = (float)lua_tonumber(L, 4);
	float offhx = (float)lua_tonumber(L, 5);
	float offhy = (float)lua_tonumber(L, 6);
	float ox = wobj->x, oy = wobj->y;
	CNM_BOX oldh;

	memcpy(&oldh, &wobj->hitbox, sizeof(oldh));
	wobj->hitbox.x += offhx;
	wobj->hitbox.y += offhy;
	wobj->hitbox.w -= offhx*2.0f;
	wobj->hitbox.h -= offhy*2.0f;
	if (wobj->hitbox.w < 0.0f) wobj->hitbox.w = 0.0f;
	if (wobj->hitbox.h < 0.0f) wobj->hitbox.h = 0.0f;
	wobj->x = ox + offx;
	wobj->y = oy + offy;
	WOBJ *a = Wobj_GetWobjColliding(wobj, flags);
	wobj->x = ox;
	wobj->y = oy;
	memcpy(&wobj->hitbox, &oldh, sizeof(oldh));

	if (a != NULL) lua_pushlightuserdata(L, a);
	else lua_pushlightuserdata(L, NULL);
	return 1;
}
static int gamelua_GetLinkWobj(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		lua_pushlightuserdata(L, NULL);
		return 1;
	}
	lua_pushlightuserdata(L, Wobj_GetAnyWOBJFromUUIDAndNode(wobj->link_node, wobj->link_uuid));
	return 1;
}
static int gamelua_Exists(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	lua_pushboolean(L, 1);
	return 1;
}
static int gamelua_GetButton(lua_State *L)
{
	int btn = (int)lua_tonumber(L, 1);
	lua_pushboolean(L, Input_GetButton(btn, INPUT_STATE_PLAYING));
	return 1;
}
static int gamelua_GetButtonPressed(lua_State *L)
{
	int btn = (int)lua_tonumber(L, 1);
	lua_pushboolean(L, Input_GetButtonPressed(btn, INPUT_STATE_PLAYING));
	return 1;
}
static int gamelua_ApplyPlayerWeaponDamage(lua_State *L)
{
	WOBJ *wobj = (WOBJ *)lua_touserdata(L, 1);
	if (wobj == NULL || lua_isnil(L, 1))
	{
		return 0;
	}
	WobjGenericAttack_Update(wobj);
	return 0;
}*/