#ifndef _filesystem_h_
#define _filesystem_h_
#include <stdio.h>
#include "utility.h"
#include "new.h"
#include "str.h"
#include "types.h"
#include "result.h"

#include "xmacro_start.h"
#define _FILE_LIFETIME_XMACROS \
	ENUM(FileLifetimeGlobal) \
	ENUM(FileLifetimeLevel)
xmacro_enum(_FILE_LIFETIME_XMACROS, file_lifetime)
#define _FILE_RESULT_XMACROS \
	ENUM(FileResultOk) \
	ENUM(FileResultNotFound) \
	ENUM(FileResultNotAllowed) \
	ENUM(FileResultReachedEOF)
xmacro_enum(_FILE_RESULT_XMACROS, file_result)
#define _FILE_ACCESS_XMACROS \
	ENUM(FileAccessR) \
	ENUM(FileAccessW) \
	ENUM(FileAccessRW)
xmacro_enum(_FILE_ACCESS_XMACROS, file_access)
#include "xmacro_end.h"

typedef struct file_t {
	FILE *_fp;
} file_t;
make_result_type(file_open, file_result_t, file_t)
make_result_type(file_tell, file_result_t, usize)
make_result_type(file_size, file_result_t, usize)

file_open_result_t file_open(str_t game_path, file_lifetime_t lifetime, file_access_t access);
file_result_t file_close(file_t *file);
file_result_t file_read(file_t *file, usize size, void *buf);
file_result_t file_write(file_t *file, usize size, const void *buf);
file_tell_result_t file_tell(const file_t *file);
file_result_t file_seek(file_t *file, usize offset);
file_size_result_t file_size(const file_t *file);

// Release build is "stable" debug build is not
#ifndef REFACTOR
#define FILESYSTEM_MAX_LEVELS 48
#define FILESYSTEM_MAX_MIDIS 48
#define FILESYSTEM_MAX_WAVS 96
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
void FileSystem_SetLevelParScore(int level, int par);
int FileSystem_GetLevelParScore(int level);
int FileSystem_GetLevelDifficulty(int level);
int FileSystem_NumLevels(void);
void FileSystem_ResetLevelOrderBuffer(void);
int FileSystem_AddLevelToLevelOrder(const char *levelname);
int FileSystem_GetLevelFromLevelOrder(int level);
int Filesystem_GetLevelIdFromName(const char *levelname);
int Filesystem_GetLevelIdFromFileName(const char *level);
int Filesystem_GetNumTitleLevels(void);
int Filesystem_IsLevelSecret(int level);
int Filesystem_GetLevelType(int level);
int Filesystem_LevelIDToLevelOrderNum(int id);

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

#endif
