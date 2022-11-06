#ifndef _net_h_
#define _net_h_

#define NET_DEFUALT_PORT (unsigned short)('C' | 'O' << 8)
#define NET_MSERVER_PORT (unsigned short)('M' | 'S' << 8)
#define NET_DOWNLOADING_PORT (unsigned short)('C' | 'D' << 8)
#define NET_MAX_SAFE_PACKETS 1024
#define NET_DATA_SIZE 4096
#define NET_IP_NULL 0xffffffff

typedef struct _NET_PACKET NET_PACKET;
typedef void(*NET_POLL_FUNC)(NET_PACKET *packet);

typedef unsigned short	NetU16;
typedef unsigned int	NetU32;

typedef enum _NET_PACKET_TYPE
{
	NET_NONE = 0x00,
	NET_ACK,
	NET_CONNECTION_REQUEST_OLD,
	NET_DISCONNECT,
	NET_CONNECTION_ACCEPT,
	NET_NEW_CONNECTION,
	NET_DISCONNECT_BROADCAST,
	NET_CLIENT_SENT_CAMERA_POSITION,
	NET_SERVER_OWNED_OBJECTS,
	NET_CLIENT_OWNED_OBJECTS,
	NET_CLIENT_OBJECT_UPDATE_REQUEST,
	NET_CHAT_MESSAGE_BROADCAST,
	NET_TCP_PACKET_TYPE,
	NET_DAMAGED_OBJECTS,
	//NET_OBJECT_UPDATE_LIST,
	NET_SERVER_ISOKAY_PING,
	NET_MASTER_SERVER_SERVER_PING,
	NET_MASTER_SERVER_PAGE_REQUEST,
	NET_MASTER_SERVER_PAGE_DATA,
	NET_CONNECTION_REQUEST,
	NET_CONNECTION_DENIED,
	NET_PACKET_TYPE_MAX
} NET_PACKET_TYPE;

typedef enum _NET_TCP_PACKET_TYPES
{
	NET_TCP_FILE_INFOS = 13,
	NET_TCP_FILE_REQUESTS = 1,
	NET_TCP_FILE_CHUNK,
	NET_TCP_FILE_SWITCH,
	NET_TCP_PACKET_TYPE_MAX
} NET_TCP_PACKET_TYPES;

typedef struct _NET_ADDR
{
	NetU32 host;
	NetU16 port;
} NET_ADDR;

typedef struct _NET_PACKET_HEADER
{
	unsigned char type : 7;
	unsigned char safe : 1;
	NET_ADDR addr;
	unsigned short size;
	unsigned int id;
} NET_PACKET_HEADER;

typedef struct _NET_PACKET
{
	NET_PACKET_HEADER hdr;
	unsigned char data[NET_DATA_SIZE];
} NET_PACKET;

typedef struct _NET_TCP_PACKET
{
	int type;
	unsigned char data[(NET_DATA_SIZE - sizeof(int)) / 4];
} NET_TCP_PACKET;

void Net_Init(void);
void Net_Quit(void);

void Net_FakeLoss(int percent);

NetU32 Net_HostToNetU32(NetU32 host);
NetU16 Net_HostToNetU16(NetU16 host);
NetU32 Net_NetToHostU32(NetU32 net);
NetU16 Net_NetToHostU16(NetU16 net);
NET_ADDR Net_GetIpFromString(const char *string);
const char *Net_GetStringFromIp(NET_ADDR *addr);
NET_PACKET *Net_CreatePacket(int type, int safe, const NET_ADDR *addr, int size, const void *data);
void Net_DestroyPacket(NET_PACKET *packet);
void Net_Send(NET_PACKET *packet);
NET_PACKET *Net_Recv(void);
void Net_Update(void);
void Net_PollPackets(int max_num);
void Net_AddPollingFunc(NET_POLL_FUNC func);
void Net_RemovePollingFunc(NET_POLL_FUNC func);
int Net_GetAvgUDPIncomingBandwidth(void);
int Net_GetAvgUDPOutgoingBandwidth(void);
void Net_InitAvgUDPOutgoingBandwidth(void);

void NetTcp_OpenServerPort(void);
void NetTcp_ResetServerPort(void);
int NetTcp_OpenClientPort(NET_ADDR server);
void NetTcp_ClosePort(void);
int NetTcp_HasConnection(void);
NET_PACKET *NetTcp_Recv(void);
void NetTcp_Send(const NET_PACKET *p);

#endif