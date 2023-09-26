#ifndef _client_h_
#define _client_h_
#include "netgame.h"
#include "net.h"

NETGAME_NODE *Client_GetNode(void);
void Client_Create(NET_ADDR addr);
void Client_Destory(void);
void Client_Update(NET_PACKET *packet);
void Client_OnLevelStart(void);
void Client_Tick(void);
void Client_SendChatMessage(const char *message);

#endif