#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "filesystem.h"
#include "serial.h"
#include "console.h"
#include "xmacro_impl.h"
#include "mem.h"
//#include "blocks.h"

// due to new.h's making fopen deprecated
#undef fopen
xmacro_as_str(_FILE_LIFETIME_XMACROS, file_lifetime)
xmacro_as_str(_FILE_RESULT_XMACROS, file_result)

#ifndef REFACTOR

typedef struct levelinf {
	char level[FILESYSTEM_MAX_LENGTH];
	char name[UTIL_MAX_TEXT_WIDTH - 1];
	CNM_RECT preview;
	int diff, par, type;
} levelinf_t;

//static char levels[FILESYSTEM_MAX_LEVELS][FILESYSTEM_MAX_LENGTH];
//static char level_names[FILESYSTEM_MAX_LEVELS][UTIL_MAX_TEXT_WIDTH + 1];
//CNM_RECT level_previews[FILESYSTEM_MAX_LEVELS];
static levelinf_t *levels;
static int *level_order;
//static int level_diffs[FILESYSTEM_MAX_LEVELS];
//static int level_pars[FILESYSTEM_MAX_LEVELS];
//static int level_types[FILESYSTEM_MAX_LEVELS];
static int num_levels;
static int num_title_levels;

static FILESYSTEM_REGISTERED_FILE *bmp_file, *cnma_file, *cnmb_file, *cnms_file;
static FILESYSTEM_REGISTERED_FILE *mid_files, *wav_files;

const char *FileSystem_GetLevel(int level)
{
	return levels[level].level;
}
const CNM_RECT *FileSystem_GetLevelPreview(int level)
{
	return &levels[level].preview;
}
const char *FileSystem_GetLevelName(int level)
{
	return levels[level].name;
}
int FileSystem_GetLevelParScore(int level) {
	return levels[level].par;
}
void FileSystem_SetLevelParScore(int level, int par) {
	levels[level].par = par;
}
int FileSystem_GetLevelDifficulty(int level)
{
	if (levels[level].diff > 9)
		return 9;
	return levels[level].diff;
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
int FileSystem_AddLevelToLevelOrder(const char *levelname)
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
			return -1;
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
			return lvl_index;
	}

	// Next move everything in front of it forward 1
	memmove(level_order + 1, level_order, ordered_index * sizeof(int));
	level_order[0] = lvl_index;
	return lvl_index;
}
int FileSystem_GetLevelFromLevelOrder(int level)
{
	return level_order[level];
}
int Filesystem_GetLevelIdFromName(const char *levelname) {
	int id = 0;
	for (int i = 0; i < FILESYSTEM_MAX_LEVELS; i++) {
		if (strcmp(levelname, levels[i].name) == 0) {
			id = i;
		}
	}
	return id;
}
int Filesystem_GetLevelIdFromFileName(const char *level) {
	int id = -1;
	if (strlen(level) == 0) return id;
	for (int i = 0; i < FILESYSTEM_MAX_LEVELS; i++) {
		if (strcmp(level, levels[i].level) == 0) {
			id = i;
		}
	}
	return id;
}
int Filesystem_LevelIDToLevelOrderNum(int id) {
	for (int i = 0; i < num_levels; i++) {
		if (level_order[i] == id) return i;
	}
	return -1;
}

void FileSystem_Init(void)
{
	level_order = arena_global_alloc(sizeof(*level_order) * FILESYSTEM_MAX_LEVELS);
	levels = arena_global_alloc(sizeof(*levels) * FILESYSTEM_MAX_LEVELS);

	bmp_file = arena_global_alloc(sizeof(*bmp_file));
	memset(bmp_file, 0, sizeof(*bmp_file));
	cnma_file = arena_global_alloc(sizeof(*cnma_file));
	memset(cnma_file, 0, sizeof(*cnma_file));
	cnmb_file = arena_global_alloc(sizeof(*cnmb_file));
	memset(cnmb_file, 0, sizeof(*cnmb_file));
	cnms_file = arena_global_alloc(sizeof(*cnms_file));
	memset(cnms_file, 0, sizeof(*cnms_file));
	mid_files = arena_global_alloc(sizeof(*mid_files) * FILESYSTEM_MAX_MIDIS);
	memset(mid_files, 0, sizeof(*mid_files) * FILESYSTEM_MAX_MIDIS);
	wav_files = arena_global_alloc(sizeof(*wav_files) * FILESYSTEM_MAX_WAVS);
	memset(wav_files, 0, sizeof(*wav_files) * FILESYSTEM_MAX_WAVS);
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
	//unsigned char *byte_buffer;
	if (fp != NULL)
	{
		rf->used = CNM_TRUE;
		fseek(fp, 0, SEEK_END);
		rf->size = (unsigned int)ftell(fp);
		rewind(fp);
		if (rf->size > 1024*1024*2) rf->size = 1024*1024*2;
		strcpy(rf->name, file_name);
		//byte_buffer = arena_alloc(rf->size);
		//fread(byte_buffer, rf->size, 1, fp);

		rf->checksum = 0;
		for (int i = 0; i < rf->size; i++) {
			char byte;
			fread(&byte, 1, 1, fp);
			rf->checksum += byte;
			//rf->checksum += byte_buffer[i];
		}

		//arena_popfree(byte_buffer);
		fclose(fp);
	}
}
void FileSystem_RegisterGfx(const char *file_name) { FileSystem_RegisterFile(file_name, bmp_file); }
void FileSystem_RegisterAudioCfg(const char *file_name)
{
	FileSystem_RegisterFile(file_name, cnma_file);
	for (int i = 0; i < FILESYSTEM_MAX_MIDIS; i++)
	{
		memset(mid_files + i, 0, sizeof(FILESYSTEM_REGISTERED_FILE));
	}
	for (int i = 0; i < FILESYSTEM_MAX_WAVS; i++)
	{
		memset(wav_files + i, 0, sizeof(FILESYSTEM_REGISTERED_FILE));
	}
}
void FileSystem_RegisterBlocks(const char *file_name) { FileSystem_RegisterFile(file_name, cnmb_file); }
void FileSystem_RegisterSpawners(const char *file_name) { FileSystem_RegisterFile(file_name, cnms_file); }
void FileSystem_RegisterMusic(const char *file_name, int id)
{
	FileSystem_RegisterFile(file_name, mid_files + id);
}
void FileSystem_RegisterSound(const char *file_name, int id)
{
	FileSystem_RegisterFile(file_name, wav_files + id);
}
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredGfxFile(void) { return bmp_file; }
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredAudioCfgFile(void) { return cnma_file; }
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredBlocksFile(void) { return cnmb_file; }
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredSpawnersFile(void) { return cnms_file; }
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredMusicFile(int id) { return mid_files + id; }
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredSoundFile(int id) { return wav_files + id; }

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

int Filesystem_GetNumTitleLevels(void) {
	return num_title_levels;
}
int Filesystem_IsLevelSecret(int level) {
	return levels[level].type == LEVEL_TYPE_SECRET;
}
int Filesystem_GetLevelType(int level) {
	return levels[level].type;
}

#if defined(__unix__) || defined(__APPLE__) || defined(__MINGW32__)

#include <dirent.h>
#include <unistd.h>

int Filesystem_FileExist(const char *filepath) {
	return access(filepath, F_OK) == 0;
}
void FileSystem_SearchForLevels(int clear_level_list)
{
    num_levels = 0;
	num_title_levels = 0;
    if (clear_level_list)
    {
        for (int i = 0; i < FILESYSTEM_MAX_LEVELS; i++)
        {
            levels[i].name[0] = '\0';
            levels[i].level[0] = '\0';
            memset(&levels[i].preview, 0, sizeof(CNM_RECT));
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
		if (strncmp(dir->d_name, "_title", 6) == 0) {
			num_title_levels++;
			continue;
		} else if (dir->d_name[0] == '_') {
			continue;
		}
                char cnms_level_name_path[FILESYSTEM_MAX_LENGTH + 1];
                int l;
                strcpy(levels[num_levels].level, "levels/");
                strcat(levels[num_levels].level, dir->d_name);
               
				int level_type;
                Serial_LoadBlocksLevelPreview(levels[num_levels].level, &levels[num_levels].preview, &levels[num_levels].diff, &level_type);
				levels[num_levels].type = level_type;
                
                *strrchr(levels[num_levels++].level, '.') = '\0';
                
                l = num_levels - 1;
                strcpy(cnms_level_name_path, levels[l].level);
                strcat(cnms_level_name_path, ".cnms");
                Serial_LoadSpawnersLevelName(cnms_level_name_path, levels[l].name);
                if (strcmp(levels[l].name, "") == 0)
                    strcpy(levels[l].name, levels[l].level);
            }
        }
        closedir(d);
    }
}

#elif defined(_WIN32)

#include <windows.h>
#include <io.h>
//#define F_OK 0
//#define access _access

int Filesystem_FileExist(const char *filepath) {
	return _access(filepath, 0) == 0;
}

void FileSystem_SearchForLevels(int clear_level_list)
{
	num_levels = 0;
	num_title_levels = 0;
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
			if (strncmp(find_data.cFileName, "_title", 6) == 0) {
				num_title_levels++;
				continue;
			}
			char cnms_level_name_path[FILESYSTEM_MAX_LENGTH + 1];
			int l;
			strcpy(levels[num_levels], "levels/");
			strcat(levels[num_levels], find_data.cFileName);

			int level_type;
            Serial_LoadBlocksLevelPreview(levels[num_levels], &level_previews[num_levels], &level_diffs[num_levels], &level_type);
			level_types[num_levels] = level_type;
			
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

#else

void FileSystem_SearchForLevels(int clear_level_list)
{
	Console_Print("No filesystem functions, loading hardcoded levels");
	
	num_levels = 0;
	num_title_levels = 0;
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
		"tt.cnmb",
		"_title0.cnmb",
	};
	for (int i = 0; i < sizeof(hardcoded_levels)/sizeof(const char *); i++) {
		if (strncmp(hardcoded_levels[i], "_title", 6) == 0) {
			num_title_levels++;
			continue;
		}
        char cnms_level_name_path[FILESYSTEM_MAX_LENGTH + 1];
        int l;
        strcpy(levels[num_levels], "levels/");
        strcat(levels[num_levels], hardcoded_levels[i]);
        
		int level_type;
        Serial_LoadBlocksLevelPreview(levels[num_levels], &level_previews[num_levels], &level_diffs[num_levels], &level_type);
		level_types[num_levels] = level_type;
        
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

#endif
