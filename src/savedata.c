#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "console.h"
#include "utility.h"
#include "filesystem.h"
#include "lparse.h"
#include "savedata.h"
#include "game.h"
#include "mem.h"

int g_current_save;
savedata_t *g_saves;
struct globalsave g_globalsave;

void new_save(savedata_t *data) {
	data->hp = 100;
	memset(data->level, 0, sizeof(data->level));
	data->lives = 3;
	data->strength = 5;
	data->item = 0;
	data->itemhp = 100.0f;
	data->offhand = 0;
	data->offhandhp = 100.0f;
	data->upgrade_state = 0;
	data->upgradehp = 100.0f;
}

void savedata_sys_init(void) {
	g_saves = arena_global_alloc(sizeof(*g_saves) * (SAVE_SLOTS+1));
}

static void save_data(LParse *lp, const savedata_t *data);
static void load_data(LParse *lp, savedata_t *data);
void save_game(int slot, const savedata_t *data) {
	if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer) return;
	assert(slot > -1 && slot < SAVE_SLOTS);
	char filename[32] = { '\0' };
	sprintf(filename, SAVE_DIR"save%d.lps", slot);
	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		Console_Print("Can't open the save file: %s! ERROR", filename);
		return;
	}
	LParse *lp = lparse_open_from_file_inplace(global_lparse, fp, lparse_write);
	save_data(lp, data);
	lparse_close_inplace(lp);
	//fclose(fp);
}
void load_game(int slot, savedata_t *data) {
	assert(slot > -1 && slot < SAVE_SLOTS);
	char filename[32] = { '\0' };
	sprintf(filename, SAVE_DIR"save%d.lps", slot);
	FILE *fp = fopen(filename, "rb");
	if (!fp || Game_GetVar(GAME_VAR_NOSAVE)->data.integer) {
		if (fp) fclose(fp);
		//Console_Print("Can't open the save file: %s! ERROR", filename);
		//data->level[0] = '\0';
		new_save(data);
		return;
	}
	LParse *lp = lparse_open_from_file_inplace(global_lparse, fp, lparse_read);
	load_data(lp, data);
	lparse_close_inplace(lp);
	//fclose(fp);
}

static void save_data(LParse *lp, const savedata_t *data) {
	//LParseEntry *level = lparse_make_entry(lp, "LEVEL", lparse_i32, 1);
	//lparse_set_data(lp, level, 0, 1, & (int32_t) { data->level });
	LParseEntry *level = lparse_make_entry(lp, "LEVEL", lparse_u8, sizeof(data->level));
	lparse_set_data(lp, level, 0, sizeof(data->level), data->level);
	LParseEntry *lives = lparse_make_entry(lp, "LIVES", lparse_i32, 1);
	lparse_set_data(lp, lives, 0, 1, & (int32_t) { data->lives });
	LParseEntry *hp = lparse_make_entry(lp, "HP", lparse_i32, 1);
	lparse_set_data(lp, hp, 0, 1, & (int32_t) { data->hp });
	LParseEntry *strength = lparse_make_entry(lp, "STR", lparse_i32, 1);
	lparse_set_data(lp, strength, 0, 1, & (int32_t) { data->strength });
	LParseEntry *item = lparse_make_entry(lp, "ITEM", lparse_i32, 1);
	lparse_set_data(lp, item, 0, 1, & (int32_t) { data->item });
	LParseEntry *offhand = lparse_make_entry(lp, "OFFHAND", lparse_i32, 1);
	lparse_set_data(lp, offhand, 0, 1, & (int32_t) { data->offhand });
	LParseEntry *upgrade_state = lparse_make_entry(lp, "UPSTATE", lparse_i32, 1);
	lparse_set_data(lp, upgrade_state, 0, 1, & (int32_t) { data->upgrade_state });
	LParseEntry *itemhp = lparse_make_entry(lp, "ITEMHP", lparse_float, 1);
	lparse_set_data(lp, itemhp, 0, 1, & data->itemhp);
	LParseEntry *offhandhp = lparse_make_entry(lp, "OFFHANDHP", lparse_float, 1);
	lparse_set_data(lp, offhandhp, 0, 1, & data->offhandhp);
	LParseEntry *upgradehp = lparse_make_entry(lp, "UPHP", lparse_float, 1);
	lparse_set_data(lp, upgradehp, 0, 1, & data->upgradehp);
	//LParseEntry *speed = lparse_make_entry(lp, "SPD", lparse_i32, 1);
	//lparse_set_data(lp, speed, 0, 1, & (int32_t) { data->speed });
	//LParseEntry *jump = lparse_make_entry(lp, "JMP", lparse_i32, 1);
	//lparse_set_data(lp, jump, 0, 1, & (int32_t) { data->jump });
}

static void load_data(LParse *lp, savedata_t *data) {
	LParseEntry *entry;
	int32_t int_data;
	float float_data;

#define LOAD_VAR(lparse_name, member_name, intermediate) \
	entry = lparse_get_entry(lp, #lparse_name); \
	if (!entry) { \
		Console_Print("Corrupted save data, or old save data format!"); \
		return; \
	} else { \
		lparse_get_data(lp, entry, 0, 1, &intermediate); \
		data->member_name = intermediate; \
	}

	entry = lparse_get_entry(lp, "LEVEL");
	if (!entry) {
		Console_Print("Corrupted save data, or old save data format!");
		return;
	} else {
		lparse_get_data(lp, entry, 0, sizeof(data->level), data->level);
		lparse_get_data(lp, entry, 0, 1, &int_data);
	}
	//LOAD_INT(LEVEL, level)
	LOAD_VAR(LIVES, lives, int_data)
	LOAD_VAR(HP, hp, int_data)
	LOAD_VAR(STR, strength, int_data)
	LOAD_VAR(ITEM, item, int_data)
	LOAD_VAR(OFFHAND, offhand, int_data)
	LOAD_VAR(UPSTATE, upgrade_state, int_data)
	LOAD_VAR(ITEMHP, itemhp, float_data)
	LOAD_VAR(OFFHANDHP, offhandhp, float_data)
	LOAD_VAR(UPHP, upgradehp, float_data)
	//LOAD_INT(SPD, speed)
	//LOAD_INT(JMP, jump)
}

void globalsave_clear(struct globalsave *gs) {
	if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer) return;
	memset(gs->levels_found, 0, sizeof(gs->levels_found));
	//memset(gs->skins_found, 0, sizeof(gs->skins_found));
	for (int i = 0; i < sizeof(gs->skins_found) / sizeof(gs->skins_found[0]); i++) {
		gs->skins_found[i] = -1;
	}
	for (int i = 0; i < sizeof(gs->pets_found) / sizeof(gs->pets_found[0]); i++) {
		gs->pets_found[i] = -1;
	}
	for (int i = 0; i < sizeof(gs->best_times) / sizeof(gs->best_times[0]); i++) {
		gs->best_times[i] = 99*60+59;
		gs->best_ranks[i] = 0;
	}
	gs->skins_found[0] = 10;
	gs->saves_created = 0;
	gs->titlebg = 0;
	Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer = 10;
	Game_GetVar(GAME_VAR_PLAYER_PET)->data.integer = -1;
}
int globalsave_visit_level(struct globalsave *gs, const char *level) {
	if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer) return CNM_TRUE;
	for (int i = 0; i < sizeof(gs->levels_found) / sizeof(gs->levels_found[0]); i++) {
		if (strcmp(gs->levels_found[i], level) == 0) return CNM_FALSE;
	}

	for (int i = 0; i < sizeof(gs->levels_found) / sizeof(gs->levels_found[0]); i++) {
		if (strlen(gs->levels_found[i]) == 0) {
			strcpy(gs->levels_found[i], level);
			return CNM_TRUE;
		}
	}
	return CNM_FALSE;
}
int globalsave_visit_skin(struct globalsave *gs, int skinid) {
	if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer) return CNM_TRUE;
	for (int i = 0; i < sizeof(gs->skins_found) / sizeof(gs->skins_found[0]); i++) {
		if (gs->skins_found[i] == skinid) return CNM_FALSE;
	}

	for (int i = 0; i < sizeof(gs->skins_found) / sizeof(gs->skins_found[0]); i++) {
		if (gs->skins_found[i] == -1) {
			gs->skins_found[i] = skinid;
			return CNM_TRUE;
		}
	}
	return CNM_FALSE;
}
int globalsave_find_skin(struct globalsave *gs, int skinid) {
	for (int i = 0; i < sizeof(gs->skins_found) / sizeof(gs->skins_found[0]); i++) {
		if (gs->skins_found[i] == skinid) return i;
	}
	return -1;
}
void globalsave_load(struct globalsave *gs) {
	globalsave_clear(gs);
	FILE *fp = fopen(SAVE_DIR"gameinfo.lps", "rb");
	if (!fp || Game_GetVar(GAME_VAR_NOSAVE)->data.integer) {
		if (fp) fclose(fp);
		return;
	}
	LParse *lp = lparse_open_from_file_inplace(global_lparse, fp, lparse_read);
	if (!lp) {
		fclose(fp);
		return;
	}

	LParseEntry *entry;
	entry = lparse_get_entry(lp, "PLRSKIN");
	if (entry) lparse_get_data(lp, entry, 0, 1, &Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer);
	entry = lparse_get_entry(lp, "PETSKIN");
	if (entry) lparse_get_data(lp, entry, 0, 1, &Game_GetVar(GAME_VAR_PLAYER_PET)->data.integer);
	entry = lparse_get_entry(lp, "PETSFOUND");
	if (entry) lparse_get_data(lp, entry, 0, sizeof(gs->pets_found) / sizeof(gs->pets_found[0]), &gs->pets_found);
	entry = lparse_get_entry(lp, "NUMSAVES");
	if (entry) lparse_get_data(lp, entry, 0, 1, &gs->saves_created);
	entry = lparse_get_entry(lp, "LVLSFOUND");
	if (entry) lparse_get_data(lp, entry, 0, sizeof(gs->levels_found), &gs->levels_found);
	entry = lparse_get_entry(lp, "SKINSFOUND");
	if (entry) lparse_get_data(lp, entry, 0, sizeof(gs->skins_found) / sizeof(gs->skins_found[0]), &gs->skins_found);
	entry = lparse_get_entry(lp, "TITLEBG");
	if (entry) lparse_get_data(lp, entry, 0, 1, &gs->titlebg);
	entry = lparse_get_entry(lp, "RANKS");
	if (entry) lparse_get_data(lp, entry, 0, sizeof(gs->best_ranks) / sizeof(gs->best_ranks[0]), &gs->best_ranks);
	entry = lparse_get_entry(lp, "TIMES");
	if (entry) lparse_get_data(lp, entry, 0, sizeof(gs->best_times) / sizeof(gs->best_times[0]), &gs->best_times);

	lparse_close_inplace(lp);
	//fclose(fp);
}
static void _globalsave_save(const struct globalsave *gs) {
	FILE *fp = fopen(SAVE_DIR"gameinfo.lps", "wb");
	if (!fp) {
		Console_Print("Can't open the game info file: "SAVE_DIR"gameinfo.lps""! ERROR");
		return;
	}
	LParse *lp = lparse_open_from_file_inplace(global_lparse, fp, lparse_write);
	if (!lp) {
		fclose(fp);
		return;
	}

	LParseEntry *entry;
	entry = lparse_make_entry(lp, "PLRSKIN", lparse_i32, 1);
	lparse_set_data(lp, entry, 0, 1, &Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer);
	entry = lparse_make_entry(lp, "PETSKIN", lparse_i32, 1);
	lparse_set_data(lp, entry, 0, 1, &Game_GetVar(GAME_VAR_PLAYER_PET)->data.integer);
	entry = lparse_make_entry(lp, "PETSFOUND", lparse_i32, sizeof(gs->pets_found) / sizeof(gs->pets_found[0]));
	lparse_set_data(lp, entry, 0, sizeof(gs->pets_found) / sizeof(gs->pets_found[0]), &gs->pets_found);
	entry = lparse_make_entry(lp, "NUMSAVES", lparse_i32, 1);
	lparse_set_data(lp, entry, 0, 1, &gs->saves_created);
	entry = lparse_make_entry(lp, "LVLSFOUND", lparse_u8, sizeof(gs->levels_found));
	lparse_set_data(lp, entry, 0, sizeof(gs->levels_found), &gs->levels_found);
	entry = lparse_make_entry(lp, "SKINSFOUND", lparse_i32, sizeof(gs->skins_found) / sizeof(gs->skins_found[0]));
	lparse_set_data(lp, entry, 0, sizeof(gs->skins_found) / sizeof(gs->skins_found[0]), &gs->skins_found);
	entry = lparse_make_entry(lp, "TITLEBG", lparse_i32, 1);
	lparse_set_data(lp, entry, 0, 1, &gs->titlebg);
	entry = lparse_make_entry(lp, "RANKS", lparse_i32, sizeof(gs->best_ranks) / sizeof(gs->best_ranks[0]));
	lparse_set_data(lp, entry, 0, sizeof(gs->best_ranks) / sizeof(gs->best_ranks[0]), &gs->best_ranks);
	entry = lparse_make_entry(lp, "TIMES", lparse_i32, sizeof(gs->best_times) / sizeof(gs->best_times[0]));
	lparse_set_data(lp, entry, 0, sizeof(gs->best_times) / sizeof(gs->best_times[0]), &gs->best_times);

	lparse_close_inplace(lp);
	//fclose(fp);
}
void globalsave_save(const struct globalsave *gs) {
	if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer) return;
	_globalsave_save(gs);
}
void globalsave_save_override(const struct globalsave *gs) {
	_globalsave_save(gs);
}
int globalsave_get_num_secrets(const struct globalsave *gs) {
	int num = 0;
	for (int i = 0; i < sizeof(gs->levels_found) / sizeof(gs->levels_found[0]); i++) {
		if (strlen(gs->levels_found[i]) != 0 &&
			Filesystem_IsLevelSecret(Filesystem_GetLevelIdFromFileName(gs->levels_found[i]))) {
			num++;
		}
	}
	return num;
}
int globalsave_get_num_levels(const struct globalsave *gs) {
	int num = 0;
	for (int i = 0; i < sizeof(gs->levels_found) / sizeof(gs->levels_found[0]); i++) {
		if (strlen(gs->levels_found[i]) != 0) num++;
	}
	return num;
}
int globalsave_get_num_skins(const struct globalsave *gs) {
	int num = 0;
	for (int i = 0; i < sizeof(gs->skins_found) / sizeof(gs->skins_found[0]); i++) {
		if (gs->skins_found[i] != -1) ++num;
	}
	return num;
}
int globalsave_visit_pet(struct globalsave *gs, int petid) {
	if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer) return CNM_TRUE;
	for (int i = 0; i < sizeof(gs->pets_found) / sizeof(gs->pets_found[0]); i++) {
		if (gs->pets_found[i] == petid) return CNM_FALSE;
	}

	for (int i = 0; i < sizeof(gs->pets_found) / sizeof(gs->pets_found[0]); i++) {
		if (gs->pets_found[i] == -1) {
			gs->pets_found[i] = petid;
			return CNM_TRUE;
		}
	}
	return CNM_FALSE;
}
int globalsave_get_num_pets(const struct globalsave *gs) {
	int num = 0;
	for (int i = 0; i < sizeof(gs->pets_found) / sizeof(gs->pets_found[0]); i++) {
		if (gs->pets_found[i] != -1) ++num;
	}
	return num;
}
int globalsave_find_pet(struct globalsave *gs, int petid) {
	if (petid == -1) return -1;
	for (int i = 0; i < sizeof(gs->pets_found) / sizeof(gs->pets_found[0]); i++) {
		if (gs->pets_found[i] == petid) return i;
	}
	return -1;
}

