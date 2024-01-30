#include <string.h>
#include <stdlib.h>
#include "utility.h"
#include "console.h"
#include "petdefs.h"
#include "game.h"
#include "mem.h"

int g_num_petdefs;
PetDef *g_petdefs;

static int getint(const char **str) {
	char intbuf[32] = { '\0' };
	for (int i = 0; i < 31 && **str && ((**str >= '0' && **str <= '9') || **str == '-'); i++, (*str)++) {
		intbuf[i] = **str;
		intbuf[i+1] = '\0';
	}
	(*str)++; // skip the space
	return atoi(intbuf);
}

static float getfloat(const char **str) {
	char floatbuf[32] = { '\0' };
	for (int i = 0; i < 31 && **str && ((**str >= '0' && **str <= '9') || **str == '-' || **str == '.'); i++, (*str)++) {
		floatbuf[i] = **str;
		floatbuf[i+1] = '\0';
	}
	(*str)++; // skip the space
	return atof(floatbuf);
}

void petdef_sys_init(void) {
	g_petdefs = arena_global_alloc(sizeof(*g_petdefs) * MAX_PETDEFS);
}
PetDef petdef_from_line(const char *line) {
	PetDef pet = (PetDef) {
		.name = { '\0' },
		.ai_type = PETAI_NULL,
	};

	// skip first "
	line++;
	for (int i = 0; i < sizeof(pet.name) - 1 && *line && *line != '"'; i++, line++) {
		pet.name[i] = *line;
		if (*line == '_') pet.name[i] = ' ';
		pet.name[i+1] = '\0';
	}
	if (*line == '\0') {
		Console_Print("Malformed pet name!!!!");
		return pet;
	}
	line++; // skip second " 
	line++; // skip a space or tab

	// now get the base x and y
	pet.basex = getint(&line);
	pet.basey = getint(&line);
	pet.iconx = getint(&line);
	pet.icony = getint(&line);
	pet.idle_snd = getint(&line);
	
	// now get the ai type
	if (*line == 'f') pet.ai_type = PETAI_FLY;
	else if (*line == 'w') pet.ai_type = PETAI_WALK;
	else if (*line == 'b') pet.ai_type = PETAI_BOUNCE;
	else {
		Console_Print("Malformed pet ai type!!!!");
		return pet;
	}
	line++;
	line++; // skip a space or tab
	
	// now we switch it up based on the ai type
	if (pet.ai_type == PETAI_FLY) {
		pet.ai_data.fly.fly_frames = getint(&line);
	} else if (pet.ai_type == PETAI_WALK) {
		pet.ai_data.walk.idle_frames = getint(&line);
		pet.ai_data.walk.walk_frames = getint(&line);
		pet.ai_data.walk.fall_frames = getint(&line);
	} else if (pet.ai_type == PETAI_BOUNCE) {
		pet.ai_data.bounce.idle_frames = getint(&line);
		pet.ai_data.bounce.bounce_frames = getint(&line);
		pet.ai_data.bounce.bounce_idly = getint(&line);
		pet.ai_data.bounce.jump_height = getfloat(&line);
	}

	return pet;
}
