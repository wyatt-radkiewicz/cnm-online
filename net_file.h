#ifndef _net_file_h_
#define _net_file_h_
#include "filesystem.h"
#include "utility.h"
#include "net.h"

typedef enum _NETFILE_FILE_TYPES
{
	NETFILE_TYPE_BITMAP,
	NETFILE_TYPE_AUDIO_CFG,
	NETFILE_TYPE_BLOCKS,
	NETFILE_TYPE_SPAWNERS,
	NETFILE_TYPE_MUSIC,
	NETFILE_TYPE_SOUNDS = NETFILE_TYPE_MUSIC + FILESYSTEM_MAX_MIDIS,
	NETFILE_TYPE_MAX = NETFILE_TYPE_SOUNDS + FILESYSTEM_MAX_WAVS
} NETFILE_FILE_TYPES;

#define NETFILE_CHUNK_SIZE 1000

int NetFile_AskForFiles(NET_ADDR addr);
const char *NetFile_GetCurrentDownloadingFileName(void);
int NetFile_GetCurrentDownloadingFilePercent(void);

void NetFile_AddServerNetCallback(void);
void NetFile_AddClientNetCallback(void);
void NetFile_RemoveNetCallbacks(void);

void NetFile_TickServer(void);

int NetFile_ClientHasToDownloadFiles(void);
int NetFile_HasStartedDownloading(void);

#endif