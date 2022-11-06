#include <string.h>
#include "game.h"
#include "utility.h"
#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

static DEDICATED_GAME_INFO gi;

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
			strncpy(gi.serv_name, argv[i], sizeof(gi.lvl));
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

	Game_Init(&gi);
	Game_Quit();
	return 0;
}