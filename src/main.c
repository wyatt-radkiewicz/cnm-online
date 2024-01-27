#include <stdint.h>
#include <string.h>
#include "game.h"
#include "utility.h"
#include <SDL.h>

static DEDICATED_GAME_INFO gi;

static void run_tests(void);
static void Parse_Args(int argc, char **argv) {
	int i, mode;

	mode = 0;
	memset(&gi, 0, sizeof(gi));
	strcpy(gi.serv_name, "MY SERVER");
	strcpy(gi.lvl, "levels/AC");
	for (i = 0; i < argc; i++) {
		if (mode == 1) {
			strncpy(gi.lvl, argv[i], sizeof(gi.lvl));
			gi.lvl[sizeof(gi.lvl) - 1] = '\0';
			mode = 0;
		}
		if (mode == 2)
		{
			strncpy(gi.serv_name, argv[i], sizeof(gi.serv_name));
			gi.serv_name[sizeof(gi.serv_name) - 1] = '\0';
			if (strlen(gi.serv_name) < 3)
				strcpy(gi.serv_name, "NAME TOO SMALL");
			mode = 0;
		}
		
		if (strcmp(argv[i], "-dedicated") == 0) {
			gi.dedicated = CNM_TRUE;
		}
		if (strcmp(argv[i], "-level") == 0)
		{
			mode = 1;
		}
		if (strcmp(argv[i], "-server_name") == 0)
		{
			mode = 2;
		}
		if (strcmp(argv[i], "-pvp") == 0)
		{
			gi.enable_pvp = CNM_TRUE;
		}
		if (strcmp(argv[i], "-xmas") == 0)
		{
			gi.xmasmode = CNM_TRUE;
		}
		if (strcmp(argv[i], "-editorwarp") == 0)
		{
			gi.warp = CNM_TRUE;
		}
		if (strcmp(argv[i], "-advertise") == 0)
		{
			gi.advertise = CNM_TRUE;
		}
		if (strcmp(argv[i], "-master_server") == 0)
		{
			gi.master_server = CNM_TRUE;
		}
		if (strcmp(argv[i], "-sv_cheats") == 0)
		{
			gi.enable_cheats = CNM_TRUE;
		}
	}
}

int main(int argc, char **argv)
{
	Parse_Args(argc, argv);

#ifdef DEBUG
	run_tests();
#endif
	Game_Init(&gi);
	Game_Quit();
	return 0;
}

#ifdef DEBUG
#include "wobj.h"
#include "packet.h"
#include "netgame.h"
#include <stdint.h>
#include <assert.h>
uint32_t htonf(float Value);
float ntohf(uint32_t Value);
static void run_tests(void) {
	{
		printf("== htonf/ntohf tests ==");
		float test = 5.0;
		printf("hex represnetation of %f (le): %x\n", test, *(uint32_t *)(&test));
		uint32_t intconv = htonf(test);
		printf("hex represnetation of %f (be): %x\n", test, intconv);
		float conv = ntohf(intconv);
		printf("float of conv: %f == %f (%d)\n", test, conv, test == conv);
		assert(test == conv);
	}
	{
		printf("== float serialize tests ==");
		float t1 = 0.0, t2 = 0.5, t3 = 3.0, t4 = 100.0, t5 = 54321.1234;
	}
	{
		printf("== wobj serialize tests ==");
		WOBJ test;
		//for (int i = 0; i < sizeof(struct wobjdata); i++) {
		//	((unsigned char *)(&test))[i] = rand() & 0xff;
		//}
		unsigned char buffer[64];
		int bufptr = 0;
		serialize_wobj_packet(CNM_FALSE, &test, buffer, &bufptr);
		WOBJ test2;
		bufptr = 0;
		parse_wobj_packet(&test2, buffer, &bufptr);
#define CMPPROP(fmt, name) printf("cmp " #name ": " fmt " VS " #name ": " fmt "\n", test.name, test2.name)
		CMPPROP("%f", x); CMPPROP("%f", y);
		CMPPROP("%f", vel_x); CMPPROP("%f", vel_y);
		CMPPROP("%f", custom_floats[0]); CMPPROP("%f", custom_floats[1]);
		CMPPROP("%i", custom_ints[0]); CMPPROP("%i", custom_ints[1]);
		CMPPROP("%f", speed); CMPPROP("%f", jump);
		CMPPROP("%f", health); CMPPROP("%f", strength);
		CMPPROP("%i", money); CMPPROP("%i", anim_frame);
		CMPPROP("%i", type); CMPPROP("%i", last_sound_uuid);
		CMPPROP("%i", item); CMPPROP("%i", link_node);
		CMPPROP("%i", flags); CMPPROP("%i", link_uuid);
		CMPPROP("%i", last_sound_played); CMPPROP("%f", hitbox.x);
		CMPPROP("%f", hitbox.y); CMPPROP("%f", hitbox.h);
		CMPPROP("%f", hitbox.w); CMPPROP("%i", node_id);
		CMPPROP("%i", uuid);
		//int cmp = memcmp(&test, &test2, sizeof(struct wobjdata));
		//assert(cmp == 0);
	}
}
#endif
