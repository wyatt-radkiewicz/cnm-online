#ifndef _server_h_
#define _server_h_
#include "net.h"
#include "netgame.h"

void Server_Create(void);
void Server_Destroy(void);
void Server_Update(NET_PACKET *packet);
void Server_Tick(void);
void Server_SendChatMessage(int node, const char *message);

#endif