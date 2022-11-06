#ifndef _filesystem_h_
#define _filesystem_h_
#include "utility.h"

#define FILESYSTEM_MAX_LEVELS 48
#define FILESYSTEM_MAX_MIDIS 48
#define FILESYSTEM_MAX_WAVS 64
#define FILESYSTEM_MAX_LENGTH 64

typedef struct _FILESYSTEM_REGISTERED_FILE
{
	int used;
	unsigned int size;
	unsigned int checksum;
	char name[FILESYSTEM_MAX_LENGTH];
} FILESYSTEM_REGISTERED_FILE;

void FileSystem_SearchForLevels(int clear_level_list);
const char *FileSystem_GetLevel(int level);
const CNM_RECT *FileSystem_GetLevelPreview(int level);
const char *FileSystem_GetLevelName(int level);
int FileSystem_GetLevelDifficulty(int level);
int FileSystem_NumLevels(void);
void FileSystem_ResetLevelOrderBuffer(void);
void FileSystem_AddLevelToLevelOrder(const char *levelname);
int FileSystem_GetLevelFromLevelOrder(int level);

void FileSystem_Init(void);
void FileSystem_RegisterGfx(const char *bmp_file);
void FileSystem_RegisterAudioCfg(const char *cnma_file);
void FileSystem_RegisterBlocks(const char *cnmb_file);
void FileSystem_RegisterSpawners(const char *cnms_file);
void FileSystem_RegisterMusic(const char *mid_file, int id);
void FileSystem_RegisterSound(const char *wav_file, int id);
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredGfxFile(void);
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredAudioCfgFile(void);
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredBlocksFile(void);
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredSpawnersFile(void);
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredMusicFile(int id);
const FILESYSTEM_REGISTERED_FILE *FileSystem_GetRegisteredSoundFile(int id);
int FileSystem_DoesFileExist(const char *file_name, unsigned int checksum);

#endif