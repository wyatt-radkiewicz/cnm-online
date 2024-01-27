#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "netgame.h"
#include "net_file.h"
#include "game.h"
#include "renderer.h"
#include "serial.h"
#include "console.h"

static int is_server_connected;

static int current_file_id;
static char current_file_name[256];
static int current_file_percent;
static int state = 0;
static int sub_state = 0;
static int needs_files = CNM_TRUE, started_files = CNM_FALSE;
static int client_use_server_bitmap = 0;
static int client_use_server_audiocnma = 0;

static FILESYSTEM_REGISTERED_FILE downloading_files[NETFILE_TYPE_MAX];
static int needed_files[NETFILE_TYPE_MAX];
static int perfect_files[NETFILE_TYPE_MAX];
//static int use_server[NETFILE_TYPE_MAX];

static void NetFile_ClientCallback(NET_PACKET *p);
static void NetFile_ServerCallback(NET_PACKET *p);

static void NetFile_ClientEnd(void);
static const FILESYSTEM_REGISTERED_FILE *NetFile_GetLocalRegisteredFileFromID(int id);
static int NetFile_GetNextFileId(int id);

int NetFile_AskForFiles(NET_ADDR addr)
{
	is_server_connected = CNM_FALSE;
	current_file_id = -1;
	current_file_name[0] = '\0';
	current_file_percent = 0;
	client_use_server_bitmap = 0;
	client_use_server_audiocnma = 0;
	state = 0;
	sub_state = 0;
	needs_files = CNM_TRUE;
	started_files = CNM_FALSE;
	memset(downloading_files, 0, sizeof(downloading_files));
	memset(needed_files, 0, sizeof(needed_files));
	memset(perfect_files, 0, sizeof(perfect_files));
	return NetTcp_OpenClientPort(addr);
}
const char *NetFile_GetCurrentDownloadingFileName(void)
{
	return current_file_name;
}
int NetFile_GetCurrentDownloadingFilePercent(void)
{
	return current_file_percent;
}

void NetFile_AddServerNetCallback(void)
{
	Net_AddPollingFunc(NetFile_ServerCallback);
	NetTcp_OpenServerPort();
	state = 0;
	is_server_connected = CNM_FALSE;
}
void NetFile_AddClientNetCallback(void)
{
	Net_AddPollingFunc(NetFile_ClientCallback);
}
void NetFile_RemoveNetCallbacks(void)
{
	Net_RemovePollingFunc(NetFile_ServerCallback);
	Net_RemovePollingFunc(NetFile_ClientCallback);
	NetTcp_ClosePort();
}

static void NetFile_ClientEnd(void)
{
	int needed_audio_file = CNM_FALSE;
	if (needed_files[NETFILE_TYPE_BITMAP])
	{
		//Renderer_LoadBitmap(downloading_files[NETFILE_TYPE_BITMAP].name);
		//Renderer_BuildTables();
		//Renderer_RestartWindow();
		//strcpy(downloading_files[NETFILE_TYPE_BITMAP].name, "gfx_server.bmp");
		Renderer_LoadBitmap(downloading_files[NETFILE_TYPE_BITMAP].name);
		Renderer_BuildTables();
		FileSystem_RegisterGfx(downloading_files[NETFILE_TYPE_BITMAP].name);
	}
	if (client_use_server_bitmap) {
		Renderer_LoadBitmap("gfx_server.bmp");
		Renderer_BuildTables();
		FileSystem_RegisterGfx("gfx_server.bmp");
	}
	for (int i = NETFILE_TYPE_MUSIC; i < NETFILE_TYPE_MAX; i++)
		needed_audio_file |= needed_files[i];
	if (needed_files[NETFILE_TYPE_AUDIO_CFG] || needed_audio_file)
	{
		Serial_LoadAudioCfg(downloading_files[NETFILE_TYPE_AUDIO_CFG].name);
	}
	if (client_use_server_audiocnma) {
		Serial_LoadAudioCfg("audio_server.cnma");
	}
	Game_SwitchState(GAME_STATE_CLIENT_CONNECTING);
}

static void NetFile_ClientCallback(NET_PACKET *packet)
{
	NET_TCP_PACKET *tcp, sendtcp;
	FILESYSTEM_REGISTERED_FILE *f;
	FILE *fp = NULL;
	tcp = (void *)packet->data;
	//char server_alt[FILESYSTEM_MAX_LENGTH+16];

	switch (state)
	{
	case 0:
		if (tcp->type == NET_TCP_FILE_INFOS)
		{
			memcpy(downloading_files + sub_state, tcp->data, sizeof(FILESYSTEM_REGISTERED_FILE));
			f = downloading_files + sub_state;
			//if (!f->used && !f->size && !strcmp(f->name, "") && !f->checksum && sub_state == 0)
			//	break; // IDK why this happens but it sometimes does
			needed_files[sub_state] = (f->used ? !FileSystem_DoesFileExist(f->name, f->checksum) : 0);
			if (needed_files[sub_state] && sub_state == NETFILE_TYPE_BITMAP) {
				needed_files[sub_state] = (f->used ? !FileSystem_DoesFileExist("gfx_server.bmp", f->checksum) : 0);
				if (!needed_files[sub_state])
					client_use_server_bitmap = 1;
			}
			if (needed_files[sub_state] && sub_state == NETFILE_TYPE_AUDIO_CFG) {
				needed_files[sub_state] = (f->used ? !FileSystem_DoesFileExist("audio_server.cnma", f->checksum) : 0);
				if (!needed_files[sub_state])
					client_use_server_audiocnma = 1;
			}
			sub_state++;

			if (sub_state >= NETFILE_TYPE_MAX)
			{
				if (memcmp(needed_files, perfect_files, sizeof(needed_files)) == 0)
				{
					NetFile_ClientEnd();
				}
				else
				{
					sendtcp.type = NET_TCP_FILE_REQUESTS;
					memcpy(sendtcp.data, needed_files, sizeof(needed_files));
					NET_PACKET *p = Net_CreatePacket(NET_TCP_PACKET_TYPE, 1, NULL, sizeof(sendtcp), &sendtcp);
					NetTcp_Send(p);
					state = 1;
				}
			}
			break;
		}
		else if (tcp->type == NET_TCP_FILE_SWITCH)
		{
			state = 1;
		}
	case 1:
		if (tcp->type == NET_TCP_FILE_SWITCH)
		{
			memcpy(&current_file_id, tcp->data, sizeof(int));
			sub_state = 0;
			if (current_file_id == -1)
			{
				if (!started_files)
					needs_files = CNM_FALSE;
				started_files = CNM_TRUE;
				NetFile_ClientEnd();
			}
			else
			{
				if (current_file_id == NETFILE_TYPE_BITMAP)
					strcpy(downloading_files[NETFILE_TYPE_BITMAP].name, "gfx_server.bmp");
				if (current_file_id == NETFILE_TYPE_AUDIO_CFG)
					strcpy(downloading_files[NETFILE_TYPE_AUDIO_CFG].name, "audio_server.cnma");
				strcpy(current_file_name, downloading_files[current_file_id].name);
				fp = fopen(downloading_files[current_file_id].name, "wb");
				if (fp != NULL)
					fclose(fp);
			}

			started_files = CNM_TRUE;
		}
		if (tcp->type == NET_TCP_FILE_CHUNK)
		{
			if (current_file_id == NETFILE_TYPE_BITMAP)
				strcpy(downloading_files[NETFILE_TYPE_BITMAP].name, "gfx_server.bmp");
			if (current_file_id == NETFILE_TYPE_AUDIO_CFG)
				strcpy(downloading_files[NETFILE_TYPE_AUDIO_CFG].name, "audio_server.cnma");
			fp = fopen(downloading_files[current_file_id].name, "ab");
			if (fp != NULL)
			{
				fwrite(tcp->data + sizeof(int), *((int *)tcp->data), 1, fp);
				fclose(fp);
			}
			sub_state++;
			current_file_percent = (int)((float)(sub_state * NETFILE_CHUNK_SIZE) / (float)(downloading_files[current_file_id].size) * 100.0f);
			started_files = CNM_TRUE;
		}
		break;
	}
}

void NetFile_TickServer(void)
{
	NET_TCP_PACKET tcp;
	FILE *fp;
	const FILESYSTEM_REGISTERED_FILE *curr_file;

	if (NetTcp_HasConnection() != is_server_connected)
	{
		is_server_connected = NetTcp_HasConnection();
		state = is_server_connected ? 1 : 0;
		if (is_server_connected)
			Console_Print("A new player is downloading files...");
	}

	switch (state)
	{
	case 1:
		for (int i = 0; i < NETFILE_TYPE_MAX; i++)
		{
			tcp.type = NET_TCP_FILE_INFOS;
			memcpy(tcp.data, NetFile_GetLocalRegisteredFileFromID(i), sizeof(FILESYSTEM_REGISTERED_FILE));
			NET_PACKET *p = Net_CreatePacket(NET_TCP_PACKET_TYPE, 1, NULL, sizeof(tcp), &tcp);
			NetTcp_Send(p);
		}
		state++;
		break;
	case 3:
		if (sub_state == -1)
		{
			tcp.type = NET_TCP_FILE_SWITCH;
			current_file_id = NetFile_GetNextFileId(current_file_id);
			memcpy(tcp.data, &current_file_id, sizeof(int));
			NetTcp_Send(Net_CreatePacket(NET_TCP_PACKET_TYPE, 1, NULL, sizeof(tcp), &tcp));
			if (current_file_id == -1)
				state = 0;
			sub_state = 0;
			break;
		}
		else
		{
			curr_file = NetFile_GetLocalRegisteredFileFromID(current_file_id);
			fp = fopen(curr_file->name, "rb");
			if (fp != NULL)
			{
				if ((unsigned int)sub_state * (unsigned int)NETFILE_CHUNK_SIZE >= curr_file->size)
				{
					sub_state = -1;
				}
				else
				{
					tcp.type = NET_TCP_FILE_CHUNK;
					fseek(fp, sub_state * NETFILE_CHUNK_SIZE, SEEK_SET);
					*((int *)tcp.data) = (int)fread(tcp.data + sizeof(int), 1, NETFILE_CHUNK_SIZE, fp);
					NetTcp_Send(Net_CreatePacket(NET_TCP_PACKET_TYPE, 1, NULL, sizeof(tcp), &tcp));
					sub_state++;
				}
				fclose(fp);
			}
			else
			{
				NetTcp_ResetServerPort();
			}
			break;
		}
	}
}
static void NetFile_ServerCallback(NET_PACKET *packet)
{
	NET_TCP_PACKET *tcp;
	tcp = (void *)packet->data;
	switch (state)
	{
	case 2:
		memcpy(needed_files, tcp->data, sizeof(needed_files));
		current_file_id = -1;
		sub_state = -1;
		state++;
		break;
	}
}

static const FILESYSTEM_REGISTERED_FILE *NetFile_GetLocalRegisteredFileFromID(int id)
{
	const FILESYSTEM_REGISTERED_FILE *registered_files[NETFILE_TYPE_MAX];/* = {
		FileSystem_GetRegisteredGfxFile(),
		FileSystem_GetRegisteredAudioCfgFile(),
		FileSystem_GetRegisteredBlocksFile(),
		FileSystem_GetRegisteredSpawnersFile()
	};*/
	registered_files[0] = FileSystem_GetRegisteredGfxFile();
	registered_files[1] = FileSystem_GetRegisteredAudioCfgFile();
	registered_files[2] = FileSystem_GetRegisteredBlocksFile();
	registered_files[3] = FileSystem_GetRegisteredSpawnersFile();
	for (int i = 0; i < FILESYSTEM_MAX_MIDIS; i++)
		registered_files[i + NETFILE_TYPE_MUSIC] = FileSystem_GetRegisteredMusicFile(i);
	for (int i = 0; i < FILESYSTEM_MAX_WAVS; i++)
		registered_files[i + NETFILE_TYPE_SOUNDS] = FileSystem_GetRegisteredSoundFile(i);
	return registered_files[id];
}
static int NetFile_GetNextFileId(int id)
{
	for (int i = id + 1; i < NETFILE_TYPE_MAX; i++)
	{
		if (needed_files[i])
		{
			return i;
		}
	}

	return -1;
}

int NetFile_ClientHasToDownloadFiles(void)
{
	return needs_files;
}
int NetFile_HasStartedDownloading(void)
{
	return started_files;
}