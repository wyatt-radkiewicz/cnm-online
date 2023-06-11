#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "filesystem.h"
#include "serial.h"
#include "console.h"

static char levels[FILESYSTEM_MAX_LEVELS][FILESYSTEM_MAX_LENGTH];
static char level_names[FILESYSTEM_MAX_LEVELS][UTIL_MAX_TEXT_WIDTH + 1];
CNM_RECT level_previews[FILESYSTEM_MAX_LEVELS];
static int level_order[FILESYSTEM_MAX_LEVELS];
static int level_diffs[FILESYSTEM_MAX_LEVELS];
static int num_levels;

static FILESYSTEM_REGISTERED_FILE bmp_file, cnma_file, cnmb_file, cnms_file;
static FILESYSTEM_REGISTERED_FILE mid_file[FILESYSTEM_MAX_MIDIS], wav_file[FILESYSTEM_MAX_WAVS];

const char *FileSystem_GetLevel(int level)
{
	return levels[level];
}
const CNM_RECT *FileSystem_GetLevelPreview(int level)
{
	return level_previews + level;
}
const char *FileSystem_GetLevelName(int level)
{
	return level_names[level];
}
int FileSystem_GetLevelDifficulty(int level)
{
	if (level_diffs[level] > 9)
		return 9;
	return level_diffs[level];
}
int FileSystem_NumLevels(void)
{
	return num_levels;
}
void FileSystem_ResetLevelOrderBuffer(void)
{
	for (int i = 0; i < FILESYSTEM_MAX_LEVELS; i++)
		level_order[i] = i;
}
void FileSystem_AddLevelToLevelOrder(const char *levelname)
{
	// First search up the index of the level name
	int lvl_index = -1;
	{
		for (int i = 0; i < FileSystem_NumLevels(); i++)
		{
			if (strcmp(FileSystem_GetLevel(i), levelname) == 0)
			{
				lvl_index = i;
				break;
			}
		}
		if (lvl_index == -1)
			return;
	}

	// Find the index of the old level order in the level order array
	int ordered_index = -1;
	{
		for (int i = 0; i < FileSystem_NumLevels(); i++)
		{
			if (level_order[i] == lvl_index)
			{
				ordered_index = i;
				break;
			}
		}
		if (ordered_index == -1)
			return;
	}

	// Next move everything in front of it forward 1
	memmove(level_order + 1, level_order, ordered_index * sizeof(int));
	level_order[0] = lvl_index;
}
int FileSystem_GetLevelFromLevelOrder(int level)
{
	return level_order[level];
}

void FileSystem_Init(void)
{
	memset(&bmp_file, 0, sizeof(bmp_file));
	memset(&cnma_file, 0, sizeof(cnma_file));
	memset(&cnmb_file, 0, sizeof(cnmb_file));
	memset(&cnms_file, 0, sizeof(cnms_file));
	memset(mid_file, 0, sizeof(mid_file));
	memset(wav_file, 0, sizeof(wav_file));
	/*for (int i = 0; i < FILESYSTEM_MAX_MIDIS; i++)
	{
		memset(mid_file + i, 0, sizeof(mid_file[0]));
	}
	for (int i = 0; i < FILESYSTEM_MAX_WAVS; i++)
	{
		memset(wav_file + i, 0, sizeof(wav_file[0]));
	}*/
}
static void FileSystem_RegisterFile(const char *file_name, FILESYSTEM_REGISTERED_FILE *rf)
{
	FILE *fp = fopen(file_name, "rb");
	unsigned char *byte_buffer;
	if (fp != NULL)
	{
		rf->used = CNM_TRUE;
		fseek(fp, 0, SEEK_END);
		rf->size = (unsigned int)ftell(fp);
		rewind(fp);
		strcpy(rf->name, file_name);
		byte_buffer = malloc(rf->size);
		fread(byte_buffer, rf->size, 1, fp);

		rf->checksum = 0;
		for (int i = 0; i < rf->size; i++) rf->checksum += byte_buffer[i];

		free(byte_buffer);
		fclose(fp);
	}
}
void FileSystem_RegisterGfx(const char *file_name) { FileSystem_RegisterFile(file_name, &bmp_file); }
void FileSystem_RegisterAudioCfg(const char *file_name)
{
	FileSystem_RegisterFile(file_name, &cnma_file);
	for (int i = 0; i < FILESYSTEM_MAX_MIDIS; i++)
	{
		memset(mid_file + i, 0, sizeof(FILESYSTEM_REGISTERED_FILE));
	}
	for (int i = 0; i < FILESYSTEM_MAX_WAVS; i++)
	{
		memset(wav_file + i, 0, sizeof(FILESYSTEM_REGISTERED_FILE));
	}
}
void FileSystem_RegisterBlocks(const char *file_name) { FileSystem_RegisterFile(file_name, &cnmb_file); }
void FileSystem_RegisterSpawners(const char *file_name) { FileSystem_RegisterFile(file_name, &cnms_file); }
void FileSystem_RegisterMusic(const char *file_name, int id)
{
	FileSystem_RegisterFile(file_name, mid_file + id);
}
void FileSystem_RegisterSound(const char *file_name, int id)
{
	FileSystem_RegisterFile(file_name, wav_file + id);
}
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredGfxFile(void) { return &bmp_file; }
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredAudioCfgFile(void) { return &cnma_file; }
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredBlocksFile(void) { return &cnmb_file; }
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredSpawnersFile(void) { return &cnms_file; }
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredMusicFile(int id) { return mid_file + id; }
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredSoundFile(int id) { return wav_file + id; }

int FileSystem_DoesFileExist(const char *file_name, unsigned int checksum)
{
	FILE *fp = fopen(file_name, "rb");
	unsigned char curr_byte;
	unsigned int file_checksum = 0;
	if (fp != NULL)
	{
		while (fread(&curr_byte, 1, 1, fp) == 1)
		{
			file_checksum += curr_byte;
		}
		fclose(fp);
		return file_checksum == checksum;
	}
	else
	{
		return CNM_FALSE;
	}
}

#ifdef _WIN32

#include <windows.h>
void FileSystem_SearchForLevels(int clear_level_list)
{
	num_levels = 0;
	if (clear_level_list)
	{
		for (int i = 0; i < FILESYSTEM_MAX_LEVELS; i++)
		{
			level_names[i][0] = '\0';
			levels[i][0] = '\0';
			memset(level_previews + i, 0, sizeof(CNM_RECT));
		}
	}
	
	HANDLE find_handle;
	WIN32_FIND_DATAA find_data;

	find_handle = FindFirstFileA(".\\levels\\*", &find_data);
	do
	{
		if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(strrchr(find_data.cFileName, '.') != NULL) &&
			(strcmp(strrchr(find_data.cFileName, '.'), ".cnmb") == 0))
		{
			char cnms_level_name_path[FILESYSTEM_MAX_LENGTH + 1];
			int l;
			strcpy(levels[num_levels], "levels/");
			strcat(levels[num_levels], find_data.cFileName);

			Serial_LoadBlocksLevelPreview(levels[num_levels], &level_previews[num_levels], &level_diffs[num_levels]);
			
			*strrchr(levels[num_levels++], '.') = '\0';
			
			l = num_levels - 1;
			strcpy(cnms_level_name_path, levels[l]);
			strcat(cnms_level_name_path, ".cnms");
			Serial_LoadSpawnersLevelName(cnms_level_name_path, level_names[l]);
			if (strcmp(level_names[l], "") == 0)
				strcpy(level_names[l], levels[l]);
		}
	} while (FindNextFileA(find_handle, &find_data) != 0);
}
#elif defined(__APPLE__)

#include <dirent.h>

void FileSystem_SearchForLevels(int clear_level_list)
{
    num_levels = 0;
    if (clear_level_list)
    {
        for (int i = 0; i < FILESYSTEM_MAX_LEVELS; i++)
        {
            level_names[i][0] = '\0';
            levels[i][0] = '\0';
            memset(level_previews + i, 0, sizeof(CNM_RECT));
        }
    }
    
    DIR *d;
    struct dirent *dir;
    const char *period;
    
    d = opendir("./levels");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            period = strchr(dir->d_name, '.');
            if (period != NULL && strcmp(period, ".cnmb") == 0) {
                char cnms_level_name_path[FILESYSTEM_MAX_LENGTH + 1];
                int l;
                strcpy(levels[num_levels], "levels/");
                strcat(levels[num_levels], dir->d_name);
                
                Serial_LoadBlocksLevelPreview(levels[num_levels], &level_previews[num_levels], &level_diffs[num_levels]);
                
                *strrchr(levels[num_levels++], '.') = '\0';
                
                l = num_levels - 1;
                strcpy(cnms_level_name_path, levels[l]);
                strcat(cnms_level_name_path, ".cnms");
                Serial_LoadSpawnersLevelName(cnms_level_name_path, level_names[l]);
                if (strcmp(level_names[l], "") == 0)
                    strcpy(level_names[l], levels[l]);
            }
        }
        closedir(d);
    }
}

#else

void FileSystem_SearchForLevels(int clear_level_list)
{
	Console_Print("No filesystem functions, loading hardcoded levels");
	
	num_levels = 0;
	if (clear_level_list)
	{
		for (int i = 0; i < FILESYSTEM_MAX_LEVELS; i++)
		{
			level_names[i][0] = '\0';
			levels[i][0] = '\0';
			memset(level_previews + i, 0, sizeof(CNM_RECT));
		}
	}

	const char *hardcoded_levels[] = {
		"AC.cnmb",
		"CASIMOE.cnmb",
		"deephaus.cnmb",
		"ei.cnmb",
		"kumo.cnmb",
		"lavaland.cnmb",
		"legacy_city.cnmb",
		"ocean.cnmb",
		"sc.cnmb",
		"themine.cnmb",
		"train.cnmb",
		"tt.cnmb"
	};
	for (int i = 0; i < sizeof(hardcoded_levels)/sizeof(const char *); i++) {
		char cnms_level_name_path[FILESYSTEM_MAX_LENGTH + 1];
		int l;
		strcpy(levels[num_levels], hardcoded_levels[i]);

		Serial_LoadBlocksLevelPreview(levels[num_levels], &level_previews[num_levels], &level_diffs[num_levels]);

		*strrchr(levels[num_levels++], '.') = '\0';

		l = num_levels - 1;
		strcpy(cnms_level_name_path, levels[l]);
		strcat(cnms_level_name_path, ".cnms");
		Serial_LoadSpawnersLevelName(cnms_level_name_path, level_names[l]);
		if (strcmp(level_names[l], "") == 0)
			strcpy(level_names[l], levels[l]);
	}
}

#endif