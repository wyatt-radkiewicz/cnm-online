#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "serial.h"
#include "console.h"
#include "game.h"
#include "renderer.h"
#include "audio.h"
#include "filesystem.h"
#include "player.h"
#include "gamelua.h"

void Serial_LoadAudioCfg(const char *cnma_file)
{
	FILE *fp = fopen(cnma_file, "r");
	int maxpower_i;

	// Reset maxpowerinfos for normal player
	for (maxpower_i = 0; maxpower_i < 32; maxpower_i++) {
		maxpowerinfos[maxpower_i].spd = 1.0f;
		maxpowerinfos[maxpower_i].jmp = 1.0f;
		maxpowerinfos[maxpower_i].grav = 1.0f;
		maxpowerinfos[maxpower_i].strength = 1.0f;
		maxpowerinfos[maxpower_i].hpcost = 0.0f;
		maxpowerinfos[maxpower_i].ability = 0;
	}

	if (fp != NULL)
	{
		float findex;
		int index;
		char lualine[GAMELUA_LINE_SIZE];
		char index_buf[64];
		int mode = 0;
		char filename[1024];
		char levelpath[96];
		FileSystem_RegisterAudioCfg(cnma_file);
		Audio_AddMusicEntry(18, "2drend.dll");
		while (fscanf(fp, "%s %s", index_buf, filename) == 2)
		{
			index = atoi(index_buf);
			if (strcmp(index_buf, "MODE") == 0)
			{
				if (strcmp(filename, "MUSIC") == 0)
					mode = 0;
				else if (strcmp(filename, "SOUNDS") == 0)
					mode = 1;
				else if (strcmp(filename, "LEVELSELECT_ORDER") == 0)
				{
					mode = 2;
					FileSystem_ResetLevelOrderBuffer();
				}
				else if (strcmp(filename, "MUSIC_VOLUME_OVERRIDE") == 0)
					mode = 3;
				else if (strncmp(filename, "MAXPOWER", strlen("MAXPOWER")) == 0) {
					index = atoi(filename+strlen("MAXPOWER"));
					mode = 4 + index;
				}
				else if (strcmp(filename, "LUA_AUTORUN") == 0) {
					mode = 0x8000;
					AutorunLua_ClearPrgm();
					while (1) {
						Util_GetLine(lualine, sizeof(lualine), fp);
						if (strcmp(lualine, "__ENDLUA__") == 0)
							break;
						AutorunLua_AddLine(lualine);
					}
					AutorunLua_Run();
					continue;
				}
			}
			else
			{
				if (mode == 0)
				{
					Audio_AddMusicEntry(index, filename);
					FileSystem_RegisterMusic(filename, index);
				}
				else if (mode == 1)
				{
					Audio_AddSoundEntry(index, filename);
					FileSystem_RegisterSound(filename, index);
				}
				else if (mode == 2)
				{
					strcpy(levelpath, "levels/");
					strcat(levelpath, index_buf);
					FileSystem_AddLevelToLevelOrder(levelpath);
				}
				else if (mode == 3)
				{
					Console_Print("Audio_AutoBalanceMusic is depricated, don't use this anymore!");
					Audio_AutoBalanceMusic(index, atoi(filename));
				}
				else if (mode >= 4 && mode < 0x8000) {
					// index_buf, filename
					findex = (float)atof(filename);
					index = atoi(filename);
					if (strcmp(index_buf, "spd") == 0) maxpowerinfos[mode - 4].spd = findex;
					if (strcmp(index_buf, "jmp") == 0) maxpowerinfos[mode - 4].jmp = findex;
					if (strcmp(index_buf, "grav") == 0) maxpowerinfos[mode - 4].grav = findex;
					if (strcmp(index_buf, "strength") == 0) maxpowerinfos[mode - 4].strength = findex;
					if (strcmp(index_buf, "hpcost") == 0) maxpowerinfos[mode - 4].hpcost = findex;
					if (strcmp(index_buf, "ability") == 0) maxpowerinfos[mode - 4].ability = index;
				}
			}
		}
		fclose(fp);
	}
	else
	{
		Console_Print("Could not load the audio configuration %s!", cnma_file);
	}
}
void Serial_SaveConfig(void)
{
	FILE *fp = fopen("config.txt", "w");
	if (fp != NULL)
	{
		fprintf(fp, "%s\n%s\n%d\n%d\n%d\n%d\n%s",
				Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string,
				Game_GetVar(GAME_VAR_PLAYER_NAME)->data.string,
				Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer,
				Game_GetVar(GAME_VAR_HIRESMODE)->data.integer,
				Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer,
				(int)(Audio_GetGlobalVolume()*100.0f),
				Game_GetVar(GAME_VAR_MASTER_SERVER_ADDR)->data.string);
		fclose(fp);
	}
	else
	{
		Console_Print("Cannot save config to config.txt!");
	}
}
void Serial_LoadConfig(void)
{
	FILE *fp = fopen("config.txt", "r");
	if (fp != NULL)
	{
		fscanf(fp, "%s\n%s\n%d\n%d\n%d\n%d\n%s",
			   Game_GetVar(GAME_VAR_CURRENT_CONNECTING_IP)->data.string,
			   Game_GetVar(GAME_VAR_PLAYER_NAME)->data.string,
			   &Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer,
			   &Game_GetVar(GAME_VAR_HIRESMODE)->data.integer,
			   &Game_GetVar(GAME_VAR_PLAYER_SKIN)->data.integer,
			   &Game_GetVar(GAME_VAR_INITIALIZED_AUDIO_VOLUME)->data.integer,
			   Game_GetVar(GAME_VAR_MASTER_SERVER_ADDR)->data.string);
		//Renderer_SetFullscreen(Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer);
		//Renderer_SetHiResMode(Game_GetVar(GAME_VAR_HIRESMODE)->data.integer);
		Renderer_SetScreenModeFull(Game_GetVar(GAME_VAR_FULLSCREEN)->data.integer,
								   Game_GetVar(GAME_VAR_HIRESMODE)->data.integer);
		//Game_GetVar(GAME_VAR_INITIALIZED_AUDIO_VOLUME)->data.decimal = (float)Game_GetVar(GAME_VAR_INITIALIZED_AUDIO_VOLUME)->data.integer / 100.0f;
		fclose(fp);
	}
	else
	{
		Console_Print("Cannot load config.txt!");
	}
}