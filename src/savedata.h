#ifndef _savedata_h_
#define _savedata_h_

	//
	//Here we go again, motherfucker
	//Yeah
	//Come on down and see the idiot right here
	//Too fucked to beg and not afraid to care
	//What's the matter with calamity anyway?
	//Right? Get the fuck outta my face
	//Understand I can't feel anything
	//It isn't like I wanna sift through the decay
	//I feel like a wound like I got a fucking gun against my head
	//You live when I'm dead
	//One more time, motherfucker
	//Everybody hates me now, so fuck it
	//Blood's on my face and my hands
	//And I don't know why I'm not afraid to cry
	//But that's none of your business
	//Whose life is it? Get it? See it? Feel it? Eat it?
	//Spin it around so I can spit in its face
	//I wanna leave without a trace
	//'Cause I don't wanna die in this place
	//People equal shit
	//People equal shit
	//People equal shit
	//People equal shit
	//What you gonna do?
	//(PEOPLE EQUAL SHIT) 'Cause I'm not afraid of you
	//(PEOPLE EQUAL SHIT) I'm everything you'll never be
	//(PEOPLE EQUAL SHIT) Yeah
	//

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
extern savedata_t *g_saves;
extern struct globalsave g_globalsave;

void savedata_sys_init(void);

void new_save(savedata_t *data);
void save_game(int slot, const savedata_t *data);
void load_game(int slot, savedata_t *data);

void globalsave_clear(struct globalsave *gs);
int globalsave_visit_level(struct globalsave *gs, const char *level);
int globalsave_visit_skin(struct globalsave *gs, int skinid);
int globalsave_visit_pet(struct globalsave *gs, int petid);
void globalsave_load(struct globalsave *gs);
void globalsave_save(const struct globalsave *gs);
void globalsave_save_override(const struct globalsave *gs);
int globalsave_get_num_secrets(const struct globalsave *gs);
int globalsave_get_num_levels(const struct globalsave *gs);
int globalsave_get_num_skins(const struct globalsave *gs);
int globalsave_get_num_pets(const struct globalsave *gs);
int globalsave_find_skin(struct globalsave *gs, int skinid);
int globalsave_find_pet(struct globalsave *gs, int petid);

#endif
