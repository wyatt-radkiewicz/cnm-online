#ifndef _savedata_h_
#define _savedata_h_

typedef struct savedata {
	//int started;
	int lives, hp, strength;
	char level[32];
	int item, offhand;
	float itemhp, offhandhp;
	int upgrade_state;
	float upgradehp;
} savedata_t;

#define SAVE_SLOTS 10
#define SAVE_DIR "saves/"

extern int g_current_save;
extern savedata_t g_saves[SAVE_SLOTS];

void new_save(savedata_t *data);
void save_game(int slot, const savedata_t *data);
void load_game(int slot, savedata_t *data);

#endif
