#ifndef _savedata_h_
#define _savedata_h_

typedef struct savedata {
	int level, lives, hp, strength, speed, jump;
} savedata_t;

#define SAVE_SLOTS 10
#define SAVE_DIR "saves/"

extern savedata_t g_current_save;

void save_game(int slot, const savedata_t *data);
void load_game(int slot, savedata_t *data);

#endif
