#include <stdlib.h>
#include <string.h>
#include "net.h"
#include "game.h"
#include "master_server.h"
#include "mem.h"

SERVER_INFO *server_infos;

static void MasterServer_GetServers(SERVER_INFO servers[MSPAGE_SIZE], int page);
static void MasterServer_OnPacket(NET_PACKET *packet);
static void MasterServer_RefreshServer(SERVER_INFO *server);
static int MasterServer_GetNumPages(void);

void GameState_MasterServer_Init(void)
{
	arena_push_zone("MSERV");
	server_infos = arena_alloc(sizeof(SERVER_INFO)*MAX_SERVERS);
	memset(server_infos, 0, sizeof(SERVER_INFO) * MAX_SERVERS);
	int i;
	for (i = 0; i < MAX_SERVERS; i++) {
		server_infos[i].num_players = -1;
		server_infos[i].timeout_period = -1;
	}
	Net_AddPollingFunc(MasterServer_OnPacket);
}
void GameState_MasterServer_Quit(void)
{
	Net_RemovePollingFunc(MasterServer_OnPacket);
	arena_pop_zone("MSERV");
}
static void MasterServer_OnPacket(NET_PACKET *packet)
{
	MSPAGE_REQUEST *req;
	SERVER_INFO *receved;
	SERVER_INFO temp;
	MSPAGE_DATA data;

	if (packet->hdr.type == NET_MASTER_SERVER_SERVER_PING)
	{
		receved = (SERVER_INFO *)packet->data;
		memcpy(&temp, receved, sizeof(SERVER_INFO));
		temp.addr.host = packet->hdr.addr.host;
		temp.addr.port = packet->hdr.addr.port;
		temp.timeout_period = MSTIMEOUT_PERIOD;
		MasterServer_RefreshServer(&temp);
	}
	if (packet->hdr.type == NET_MASTER_SERVER_PAGE_REQUEST)
	{
		req = (MSPAGE_REQUEST *)packet->data;
		data.page = req->page;
		data.num_pages = MasterServer_GetNumPages();
		if (data.page > data.num_pages - 1) data.page = data.num_pages - 1;
		if (data.page < 0) data.page = 0;
		MasterServer_GetServers(data.servers, data.page);
		NET_PACKET *p = Net_CreatePacket(NET_MASTER_SERVER_PAGE_DATA, 0, &packet->hdr.addr, sizeof(data), &data);
		Net_Send(p);
	}
}
void GameState_MasterServer_Update(void)
{
	int i;
	Net_PollPackets(64);
	Net_Update();

	for (i = 0; i < MAX_SERVERS; i++) {
		if (server_infos[i].timeout_period-- <= 0) {
			server_infos[i].num_players = -1;
		}
		if (server_infos[i].timeout_period < -100) {
			server_infos[i].timeout_period = 100;
		}
	}
}

static void MasterServer_GetServers(SERVER_INFO servers[MSPAGE_SIZE], int page)
{
	int i, j, p;

	for (i = 0; i < MSPAGE_SIZE; i++) {
		servers[i].num_players = -1;
	}
	if (page < 0 || page >= MasterServer_GetNumPages())
		return;
	for (i = 0, j = 0, p = 0; i < MAX_SERVERS; i++) {
		if (server_infos[i].num_players != -1) {
			if (p == page) memcpy(servers + j, server_infos + i, sizeof(SERVER_INFO));
			if (++j == MSPAGE_SIZE) {
				p++;
				j = 0;
				if (p > page) return;
			}
		}
	}
}
static int MasterServer_GetNumPages(void)
{
	int i, j;

	for (i = 0, j = 0; i < MAX_SERVERS; i++)
	{
		if (server_infos[i].num_players != -1)
		{
			j++;
		}
	}

	return (j % MSPAGE_SIZE) ? (j / MSPAGE_SIZE + 1) : (j / MSPAGE_SIZE);
}
static void MasterServer_RefreshServer(SERVER_INFO *server)
{
	int i;

	for (i = 0; i < MAX_SERVERS; i++)
	{
		if (server_infos[i].addr.host == server->addr.host && server_infos[i].addr.port == server->addr.port) {
			memcpy(server_infos + i, server, sizeof(SERVER_INFO));
			return;
		}
	}
	for (i = 0; i < MAX_SERVERS; i++)
	{
		if (server_infos[i].num_players == -1)
		{
			memcpy(server_infos + i, server, sizeof(SERVER_INFO));
			return;
		}
	}
}
