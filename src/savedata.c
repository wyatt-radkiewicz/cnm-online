#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "console.h"
#include "lparse.h"
#include "savedata.h"

savedata_t g_current_save;

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
	if (!fp) {
		Console_Print("Can't open the save file: %s! ERROR", filename);
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

