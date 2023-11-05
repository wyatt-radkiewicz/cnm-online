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

int g_current_save;
savedata_t g_saves[SAVE_SLOTS+1];
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
	LParse *lp = lparse_open_from_file(fp, lparse_write);
	save_data(lp, data);
	lparse_close(lp);
	fclose(fp);
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
	LParse *lp = lparse_open_from_file(fp, lparse_read);
	load_data(lp, data);
	lparse_close(lp);
	fclose(fp);
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
	gs->skins_found[0] = 10;
	gs->saves_created = 0;
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
	LParse *lp = lparse_open_from_file(fp, lparse_read);
	if (!lp) {
		fclose(fp);
		return;
	}

	LParseEntry *entry;
	entry = lparse_get_entry(lp, "NUMSAVES");
	if (entry) lparse_get_data(lp, entry, 0, 1, &gs->saves_created);
	entry = lparse_get_entry(lp, "LVLSFOUND");
	if (entry) lparse_get_data(lp, entry, 0, sizeof(gs->levels_found), &gs->levels_found);
	entry = lparse_get_entry(lp, "SKINSFOUND");
	if (entry) lparse_get_data(lp, entry, 0, sizeof(gs->skins_found) / sizeof(gs->skins_found[0]), &gs->skins_found);

	lparse_close(lp);
	fclose(fp);
}
void globalsave_save(const struct globalsave *gs) {
	if (Game_GetVar(GAME_VAR_NOSAVE)->data.integer) return;
	FILE *fp = fopen(SAVE_DIR"gameinfo.lps", "wb");
	if (!fp) {
		Console_Print("Can't open the game info file: "SAVE_DIR"gameinfo.lps""! ERROR");
		return;
	}
	LParse *lp = lparse_open_from_file(fp, lparse_write);
	if (!lp) {
		fclose(fp);
		return;
	}

	LParseEntry *entry;
	entry = lparse_make_entry(lp, "NUMSAVES", lparse_i32, 1);
	lparse_set_data(lp, entry, 0, 1, &gs->saves_created);
	entry = lparse_make_entry(lp, "LVLSFOUND", lparse_u8, sizeof(gs->levels_found));
	lparse_set_data(lp, entry, 0, sizeof(gs->levels_found), &gs->levels_found);
	entry = lparse_make_entry(lp, "SKINSFOUND", lparse_i32, sizeof(gs->skins_found) / sizeof(gs->skins_found[0]));
	lparse_set_data(lp, entry, 0, sizeof(gs->skins_found) / sizeof(gs->skins_found[0]), &gs->skins_found);

	lparse_close(lp);
	fclose(fp);
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

