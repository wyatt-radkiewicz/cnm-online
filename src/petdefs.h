#ifndef _petdefs_h_
#define _petdefs_h_

#define MAX_PETDEFS 24

typedef enum PetAIType {
	PETAI_NULL,
	PETAI_FLY,
	PETAI_WALK,
	PETAI_BOUNCE,
} PetAIType;

typedef struct PetAIFly {
	int fly_frames;
} PetAIFly;

typedef struct PetAIWalk {
	int idle_frames, walk_frames, fall_frames;
} PetAIWalk;

typedef struct PetAIBounce {
	int idle_frames, bounce_frames, bounce_idly;
	float jump_height;
} PetAIBounce;

typedef union PetAIData {
	PetAIFly fly;
	PetAIWalk walk;
	PetAIBounce bounce;
} PetAIData;

typedef struct PetDef {
	char name[48];
	int basex, basey, iconx, icony;
	int idle_snd;
	int ai_type;
	PetAIData ai_data;
} PetDef;

extern int g_num_petdefs;
extern PetDef *g_petdefs;

void petdef_sys_init(void);
PetDef petdef_from_line(const char *line);

#endif

