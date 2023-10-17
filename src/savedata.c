#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "console.h"
#include "lparse.h"
#include "savedata.h"

savedata_t g_current_save;

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
	LParseEntry *level = lparse_make_entry(lp, "LEVEL", lparse_i32, 1);
	lparse_set_data(lp, level, 0, 1, & (int32_t) { data->level });
	LParseEntry *lives = lparse_make_entry(lp, "LIVES", lparse_i32, 1);
	lparse_set_data(lp, lives, 0, 1, & (int32_t) { data->lives });
	LParseEntry *hp = lparse_make_entry(lp, "HP", lparse_i32, 1);
	lparse_set_data(lp, hp, 0, 1, & (int32_t) { data->hp });
	LParseEntry *strength = lparse_make_entry(lp, "STR", lparse_i32, 1);
	lparse_set_data(lp, strength, 0, 1, & (int32_t) { data->strength });
	LParseEntry *speed = lparse_make_entry(lp, "SPD", lparse_i32, 1);
	lparse_set_data(lp, speed, 0, 1, & (int32_t) { data->speed });
	LParseEntry *jump = lparse_make_entry(lp, "JMP", lparse_i32, 1);
	lparse_set_data(lp, jump, 0, 1, & (int32_t) { data->jump });
}

static void load_data(LParse *lp, savedata_t *data) {
	LParseEntry *entry;
	int32_t int_data;

#define LOAD_INT(lparse_name, member_name) \
	entry = lparse_get_entry(lp, #lparse_name); \
	if (!entry) { \
		Console_Print("Corrupted save data, or old save data format!"); \
		return; \
	} else { \
		lparse_get_data(lp, entry, 0, 1, &int_data); \
		data->member_name = int_data; \
	}

	LOAD_INT(LEVEL, level)
	LOAD_INT(LIVES, lives)
	LOAD_INT(HP, hp)
	LOAD_INT(STR, strength)
	LOAD_INT(SPD, speed)
	LOAD_INT(JMP, jump)
}

