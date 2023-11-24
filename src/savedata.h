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

struct globalsave {
	int saves_created;
	int titlebg;
	char levels_found[64][32];
	int skins_found[32];
	int pets_found[64];
	int best_ranks[64];
	int best_times[64];
};

#define SAVE_SLOTS 10
#define SAVE_DIR "saves/"

extern int g_current_save;
extern savedata_t g_saves[SAVE_SLOTS+1];
extern struct globalsave g_globalsave;

void new_save(savedata_t *data);
void save_game(int slot, const savedata_t *data);
void load_game(int slot, savedata_t *data);

void globalsave_clear(struct globalsave *gs);
int globalsave_visit_level(struct globalsave *gs, const char *level);
int globalsave_visit_skin(struct globalsave *gs, int skinid);
int globalsave_visit_pet(struct globalsave *gs, int petid);
void globalsave_load(struct globalsave *gs);
void globalsave_save(const struct globalsave *gs);
int globalsave_get_num_secrets(const struct globalsave *gs);
int globalsave_get_num_levels(const struct globalsave *gs);
int globalsave_get_num_skins(const struct globalsave *gs);
int globalsave_get_num_pets(const struct globalsave *gs);
int globalsave_find_skin(struct globalsave *gs, int skinid);
int globalsave_find_pet(struct globalsave *gs, int petid);

#endif
