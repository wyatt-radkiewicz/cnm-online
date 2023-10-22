#include <stdlib.h>
#include <string.h>
#include "net.h"
#include "utility.h"
#include "console.h"
#include "game.h"
#include "pool.h"


static int net_initialized = CNM_FALSE;
#define NET_MAX_POLLING_FUNCS 8
#define NET_MAX_FAKEDPINGERS 2048

typedef struct _NET_SAFE
{
	NET_PACKET packet;
	int times_sent;
	struct _NET_SAFE *next, *last;
} NET_SAFE;
typedef struct fakedpinger {
	int frames_left;
	NET_PACKET *packet;
} fakedpinger_t;
static NET_SAFE *net_head;
static int net_fake_loss;
static fakedpinger_t _pingers[NET_MAX_FAKEDPINGERS];
static int _pingframes, _total_pingframes, _num_pingers;
static uint32_t net_avg_incoming[30];
static uint32_t net_avg_outgoing[30];
static int net_id;
static NET_POLL_FUNC polling_funcs[NET_MAX_POLLING_FUNCS];
//static POOL *net_pool;

#ifndef __EMSCRIPTEN__
#include <SDL_net.h>


static UDPsocket net_socket;
static UDPpacket *net_packet;

static NET_PACKET *Net_Recv2(int get_packet);
static void Net_Send2(const NET_PACKET *packet, int is_new, int do_send, int delay);
static void Net_DestroySafe(NET_SAFE *s);

void Net_Init(void)
{
	int i, base_port;
	net_id = 0;
	net_initialized = CNM_TRUE;
	memset(polling_funcs, 0, sizeof(polling_funcs));
	if (SDLNet_Init() == -1)
	{
		Console_Print("Could not initialize SDLNet!");
		net_initialized = CNM_FALSE;
		return;
	}
	base_port = NET_DEFUALT_PORT;
	if (Game_GetDedicatedGameInfo()->master_server)
		base_port = NET_MSERVER_PORT;
	for (i = 0; i < 128; i++)
	{
		net_socket = SDLNet_UDP_Open(base_port + i);
		if (net_socket != NULL)
		{
			i = -1;
			break;
		}
	}
	if (i != -1)
	{
		Console_Print("Could not bind a port for the game!!!");
		net_initialized = CNM_FALSE;
		return;
	}
	net_packet = SDLNet_AllocPacket(sizeof(NET_PACKET));
	if (net_packet == NULL)
	{
		Console_Print("There was an error allocating a packet!");
		Console_Print(SDLNet_GetError());
	}

	net_head = NULL;
	net_fake_loss = 0;
	_pingframes = 0;
	_total_pingframes = 0;
	_num_pingers = 0;
	memset(net_avg_incoming, 0, sizeof(net_avg_incoming));
	memset(net_avg_outgoing, 0, sizeof(net_avg_outgoing));
	//net_pool = Pool_Create(sizeof(NET_SAFE));
}
void Net_Quit(void)
{
	//Pool_Destroy(net_pool);
	for (int i = 0; i < _num_pingers; i++) {
		Net_DestroyPacket(_pingers[i].packet);
	}
	NET_SAFE *s = net_head, *n;
	while (s) {
		n = s->next;
		Net_DestroySafe(s);
		s = n;
	}
	net_head = NULL;
	SDLNet_FreePacket(net_packet);
	SDLNet_UDP_Close(net_socket);
	net_socket = NULL;
	SDLNet_Quit();
	net_initialized = CNM_FALSE;
}

void Net_FakeLoss(int percent) {
	net_fake_loss = percent;
}
void Net_FakeSenderPing(int ms) {
	if (ms == 0) {
		_pingframes = 0;
	} else {
		_pingframes = ms/33+1;
	}
}

NetU32 Net_HostToNetU32(NetU32 host)
{
	NetU32 i;
	SDLNet_Write32(host, &i);
	return i;
}
NetU16 Net_HostToNetU16(NetU16 host)
{
	NetU16 i;
	SDLNet_Write16(host, &i);
	return i;
}
NetU32 Net_NetToHostU32(NetU32 net)
{
	return SDLNet_Read32(&net);
}
NetU16 Net_NetToHostU16(NetU16 net)
{
	return SDLNet_Read16(&net);
}
NET_ADDR Net_GetIpFromString(const char *string)
{
	NET_ADDR netip;
	IPaddress addr;
	char str_buffer[UTIL_MAX_TEXT_WIDTH];
	if (strchr(string, ':') == NULL)
	{
		SDLNet_ResolveHost(&addr, string, NET_DEFUALT_PORT);
	}
	else
	{
		memcpy(str_buffer, string, strchr(string, ':') - string);
		SDLNet_ResolveHost(&addr, str_buffer, atoi(strchr(string, ':') + 1));
	}
	netip.host = addr.host;
	netip.port = addr.port;
	return netip;
}
char ipbuf[128];
const char *Net_GetStringFromIp(NET_ADDR *addr)
{
	IPaddress a;
	a.host = addr->host;
	a.port = addr->port;
	strcpy(ipbuf, SDLNet_ResolveIP(&a));
	return ipbuf;
}
void Net_Send(NET_PACKET *packet)
{
	if (net_fake_loss == 0) {
		Net_Send2(packet, CNM_TRUE, CNM_TRUE, CNM_TRUE);
	}
	else if (rand() % 101 >= net_fake_loss) {
		Net_Send2(packet, CNM_TRUE, CNM_TRUE, CNM_TRUE);
	} else {
		Net_Send2(packet, CNM_TRUE, CNM_FALSE, CNM_TRUE);
	}
	Net_DestroyPacket(packet);
}
static void Net_Send2(const NET_PACKET *packet, int is_new, int do_send, int delay)
{
	IPaddress addr;
	NET_SAFE *s;
	if (_pingframes == 0 || !delay) {
		memcpy(net_packet->data, packet, sizeof(NET_PACKET));
		addr.host = packet->hdr.addr.host;
		addr.port = packet->hdr.addr.port;
		net_packet->address = addr;
		net_packet->len = sizeof(NET_PACKET_HEADER) + packet->hdr.size;
		if (do_send) {
			SDLNet_UDP_Send(net_socket, -1, net_packet);
		}
		net_avg_outgoing[Game_GetFrame() % 30] += packet->hdr.size;
	} else if (_num_pingers < NET_MAX_FAKEDPINGERS) {
		fakedpinger_t *pinger = _pingers + _num_pingers++;
		pinger->frames_left = _pingframes - _total_pingframes;
		_total_pingframes += pinger->frames_left;
		pinger->packet = malloc(sizeof(NET_PACKET));
		memcpy(pinger->packet, packet, sizeof(*packet));
	}

	if (packet->hdr.safe && is_new)
	{
		s = malloc(sizeof(NET_SAFE));
		s->times_sent = -5;
		s->last = NULL;
		s->next = net_head;
		if (net_head != NULL)
			net_head->last = s;
		net_head = s;
		memcpy(&s->packet, packet, sizeof(NET_PACKET));
		//Console_Print("Sending Safe id: %d", packet->hdr.id);
	}
}
NET_PACKET *Net_Recv(void) {
	return Net_Recv2(1);
}
NET_PACKET *Net_Recv2(int get_packet)
{
	NET_SAFE *s;
	NET_PACKET *packet, *ack;
	if (SDLNet_UDP_Recv(net_socket, net_packet) == 0)
		return NULL;
	packet = malloc(sizeof(NET_PACKET));
	memcpy(packet, net_packet->data, net_packet->len);
	packet->hdr.addr.host = net_packet->address.host;
	packet->hdr.addr.port = net_packet->address.port;

	if (!get_packet) {
		free(packet);
		return NULL;
	}

	if (packet->hdr.safe)
	{
		if (packet->hdr.type != NET_ACK)
		{
			// Send an ack back
			ack = Net_CreatePacket(NET_ACK, 1, &packet->hdr.addr, 0, NULL);
			ack->hdr.id = packet->hdr.id;
			//Console_Print("Sending Ack for %d", ack->hdr.id);
			Net_Send2(ack, CNM_FALSE, CNM_TRUE, CNM_TRUE);
			Net_DestroyPacket(ack);
		}
		else
		{
			// Clear the ack from our net thing
			s = net_head;
			while (s != NULL)
			{
				if (s->packet.hdr.id == packet->hdr.id)
				{
					//Console_Print("Got Ack for %d", packet->hdr.id);
					Net_DestroySafe(s);
					return packet;
				}
				s = s->next;
			}
		}
	}

	return packet;
}
void Net_Update(void)
{
	if (_pingframes) {
		if (_total_pingframes > 0) _total_pingframes--;
		if (_num_pingers) {
			_pingers[0].frames_left--;
			int i = 0;
			while (_pingers[i].frames_left <= 0 && i < _num_pingers) {
				Net_Send2(_pingers[i].packet, CNM_FALSE, CNM_TRUE, CNM_FALSE);
				Net_DestroyPacket(_pingers[i].packet);
				i++;
			}
			memmove(_pingers, _pingers + i, (_num_pingers - i) * sizeof(*_pingers));
			_num_pingers -= i;
		}
	}

	NET_SAFE *s, *n;
	s = net_head;
	while (s != NULL)
	{
		n = s->next;
		s->times_sent++;
		if (s->times_sent % 10 == 0)
			Net_Send2(&s->packet, CNM_FALSE, CNM_TRUE, CNM_TRUE);
		if (s->times_sent >= 600)
			Net_DestroySafe(s);
		s = n;
	}
}
static void Net_DestroySafe(NET_SAFE *s)
{
	if (s->last != NULL)
		s->last->next = s->next;
	else
		net_head = s->next;
	if (s->next != NULL)
		s->next->last = s->last;
	free(s);
}
void Net_PollPackets(int max_num)
{
	int i, j, use_packet;
	NET_PACKET *packet;

	net_avg_incoming[Game_GetFrame() % 30] = 0;
	use_packet = 0;
	if (net_fake_loss == 0)
		use_packet = 1;
	else if (rand() % 101 >= net_fake_loss)
		use_packet = 1;

	for (i = 0; i < max_num; i++)
	{
		packet = Net_Recv2(use_packet);
		if (packet != NULL)
		{
			net_avg_incoming[Game_GetFrame() % 30] += packet->hdr.size;
			for (j = 0; j < NET_MAX_POLLING_FUNCS; j++)
			{
				if (polling_funcs[j] != NULL)
					polling_funcs[j](packet);
			}
			Net_DestroyPacket(packet);
		}
		packet = NetTcp_Recv();
		if (packet != NULL)
		{
			for (j = 0; j < NET_MAX_POLLING_FUNCS; j++)
			{
				if (polling_funcs[j] != NULL)
					polling_funcs[j](packet);
			}
			Net_DestroyPacket(packet);
		}
	}
}
void Net_AddPollingFunc(NET_POLL_FUNC func)
{
	int i;
	for (i = 0; i < NET_MAX_POLLING_FUNCS; i++)
	{
		if (polling_funcs[i] == NULL)
		{
			polling_funcs[i] = func;
			return;
		}
	}
}
void Net_RemovePollingFunc(NET_POLL_FUNC func)
{
	int i;
	for (i = 0; i < NET_MAX_POLLING_FUNCS; i++)
	{
		if (polling_funcs[i] == func)
		{
			polling_funcs[i] = NULL;
			return;
		}
	}
}
int Net_GetAvgUDPIncomingBandwidth(void) {
	int avg = 0, i;

	for (i = 0; i < 30; i++) {
		avg += net_avg_incoming[i];
	}
	return avg / 30;
}
int Net_GetAvgUDPOutgoingBandwidth(void) {
	int avg = 0, i;

	for (i = 0; i < 30; i++)
	{
		avg += net_avg_outgoing[i];
	}
	return avg / 30;
}
void Net_InitAvgUDPOutgoingBandwidth(void) {
	net_avg_outgoing[Game_GetFrame() % 30] = 0;
}

static TCPsocket tcp_socket = NULL;
static TCPsocket tcp_client_socket = NULL;
static int tcp_is_server = CNM_FALSE;
static SDLNet_SocketSet tcp_socket_set = NULL;

void NetTcp_OpenServerPort(void)
{
	// Open up the server port on NET_DOWNLOADING_PORT
	IPaddress ip;
	SDLNet_ResolveHost(&ip, NULL, NET_DOWNLOADING_PORT);
	tcp_socket = SDLNet_TCP_Open(&ip);

	if (tcp_socket != NULL)
	{
		// Allocate a socket set and add a socket to it
		tcp_socket_set = SDLNet_AllocSocketSet(2);
		if (SDLNet_TCP_AddSocket(tcp_socket_set, tcp_socket) == -1)
		{
			Console_Print("There was an error making the TCP socket set!");
			Console_Print("SDLNet_GetError: %s", SDLNet_GetError());
		}
	}
	else
	{
		// We couldn't create the socket for the server
		Console_Print("Couldn't create the server TCP socket!");
	}

	// Initialize the server state
	tcp_is_server = CNM_TRUE;
}
void NetTcp_ResetServerPort(void)
{
	// Reset the server state and the client socket
	Console_Print("TCP Client has disconnected.");
	SDLNet_TCP_DelSocket(tcp_socket_set, tcp_client_socket);
	SDLNet_TCP_Close(tcp_client_socket);
	tcp_client_socket = NULL;
}
int NetTcp_OpenClientPort(NET_ADDR server)
{
	// Initialize the SDLNet IP address from the NET_ADDR server address
	IPaddress ip;
	ip.host = server.host;
	ip.port = Net_HostToNetU16(NET_DOWNLOADING_PORT);

	// Initialize the client state
	tcp_is_server = CNM_FALSE;

	// Open up the TCP socket to the server
	tcp_socket = SDLNet_TCP_Open(&ip);
	if (tcp_socket != NULL)
	{ // The TCP connection and socket was created successfully

		// Here we are allocating the socket set
		tcp_socket_set = SDLNet_AllocSocketSet(2);
		if (SDLNet_TCP_AddSocket(tcp_socket_set, tcp_socket) == -1)
		{
			Console_Print("There was an error making the TCP socket set!");
			Console_Print("SDLNet_GetError: %s", SDLNet_GetError());
			return CNM_FALSE;
		}
		
		return CNM_TRUE;
	}
	else
	{ // We couldn't connect
		Console_Print("Couldn't connect to the TCP server!");
		return CNM_FALSE;
	}
}
void NetTcp_ClosePort(void)
{
	// Delete the socket global to every state
	if (tcp_socket != NULL)
	{
		if (tcp_socket_set != NULL)
			SDLNet_TCP_DelSocket(tcp_socket_set, tcp_socket);
		SDLNet_TCP_Close(tcp_socket);
	}

	if (tcp_is_server && tcp_client_socket != NULL)
	{ // Delete the client socket if we are the server
		SDLNet_TCP_DelSocket(tcp_socket_set, tcp_client_socket);
		SDLNet_TCP_Close(tcp_client_socket);
	}

	if (tcp_socket_set != NULL)
		SDLNet_FreeSocketSet(tcp_socket_set);

	// Reset variables to known states
	tcp_socket = NULL;
	tcp_client_socket = NULL;
	tcp_socket_set = NULL;
}
int NetTcp_HasConnection(void)
{
	if (tcp_is_server)
		return tcp_client_socket != NULL;
	else
		return tcp_socket != NULL;
}
static NET_PACKET *NetTcp_ServerRecv(void)
{
	NET_TCP_PACKET tcp;
	int num_rdy;

	// First check to see if we even have the socket open
	if (tcp_socket == NULL)
		return NULL;

	// Next run a function to find if we have any new things on our sockets
	num_rdy = SDLNet_CheckSockets(tcp_socket_set, 0);
	if (num_rdy == 0)
		return NULL;

	if (SDLNet_SocketReady(tcp_socket) && tcp_client_socket == NULL) // Make sure we don't have a client already connected
	{ // We got a new connnection!
		tcp_client_socket = SDLNet_TCP_Accept(tcp_socket);
		if (tcp_client_socket != NULL)
			SDLNet_TCP_AddSocket(tcp_socket_set, tcp_client_socket);
		num_rdy--;
	}

	// If we have no clients and we are the server, just return
	if (tcp_is_server && tcp_client_socket == NULL)
		return NULL;

	if (num_rdy && SDLNet_SocketReady(tcp_client_socket))
	{
		// Our client has something for us maybe?
		if (SDLNet_TCP_Recv(tcp_client_socket, &tcp, sizeof(tcp)) > 0)
		{ // Out client does have something for us after all
			return Net_CreatePacket(NET_TCP_PACKET_TYPE, 1, NULL, sizeof(tcp), &tcp);
		}
		else
		{ // Our client has stopped the connection
			NetTcp_ResetServerPort();
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}
static NET_PACKET *NetTcp_ClientRecv(void)
{
	NET_TCP_PACKET tcp;

	// Check if we even have our socket open
	if (tcp_socket == NULL)
		return NULL;

	// Run function to check if sockets are ready and then see if they are ready
	SDLNet_CheckSockets(tcp_socket_set, 0);
	if (SDLNet_SocketReady(tcp_socket))
	{ // We have some data from the server
		if (SDLNet_TCP_Recv(tcp_socket, &tcp, sizeof(tcp)) > 0)
		{
			return Net_CreatePacket(NET_TCP_PACKET_TYPE, 1, NULL, sizeof(tcp), &tcp);
		}
		else
		{ // The server closed the connection
			NetTcp_ClosePort();
			Console_Print("The TCP server has shutdown");
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}
NET_PACKET *NetTcp_Recv(void)
{
	if (tcp_is_server)
		return NetTcp_ServerRecv();
	else
		return NetTcp_ClientRecv();
}
void NetTcp_Send(const NET_PACKET *p)
{
	TCPsocket recipent;

	// Get what socket to send to.
	recipent = tcp_is_server ? tcp_client_socket : tcp_socket;

	// If there is no recipent then just bail
	if (recipent == NULL)
		return;

	// Send the data over
	if (SDLNet_TCP_Send(recipent, p->data, sizeof(NET_TCP_PACKET)) < sizeof(NET_TCP_PACKET))
	{ // The connection was terminated
		if (tcp_is_server)
			NetTcp_ResetServerPort();
		else
			NetTcp_ClosePort();
	}
}
#else

void Net_Init(void) {
	
}
void Net_Quit(void) {
	
}

void Net_FakeLoss(int percent) {
	
}

NetU32 Net_HostToNetU32(NetU32 host) {
	return host;
}
NetU16 Net_HostToNetU16(NetU16 host) {
	return host;
}
NetU32 Net_NetToHostU32(NetU32 net) {
	return net;
}
NetU16 Net_NetToHostU16(NetU16 net) {
	return net;
}
NET_ADDR Net_GetIpFromString(const char *string) {
	return (NET_ADDR){
		.host = 0,
		.port = 0,
	};
}
const char *Net_GetStringFromIp(NET_ADDR *addr) {
	return "127.0.0.1";
}
void Net_Send(NET_PACKET *packet) {
	
}
NET_PACKET *Net_Recv(void) {
	return NULL;
}
void Net_Update(void) {
	
}
void Net_PollPackets(int max_num) {
	
}
void Net_AddPollingFunc(NET_POLL_FUNC func) {
	
}
void Net_RemovePollingFunc(NET_POLL_FUNC func) {
	
}
int Net_GetAvgUDPIncomingBandwidth(void) {
	return 1;
}
int Net_GetAvgUDPOutgoingBandwidth(void) {
	return 1;
}
void Net_InitAvgUDPOutgoingBandwidth(void) {
	
}

void NetTcp_OpenServerPort(void) {
	
}
void NetTcp_ResetServerPort(void) {
	
}
int NetTcp_OpenClientPort(NET_ADDR server) {
	return 0;
}
void NetTcp_ClosePort(void) {
	
}
int NetTcp_HasConnection(void) {
	return 0;
}
NET_PACKET *NetTcp_Recv(void) {
	return NULL;
}
void NetTcp_Send(const NET_PACKET *p) {
	
}
#endif

NET_PACKET *Net_CreatePacket(int type, int safe, const NET_ADDR *addr, int size, const void *data)
{
	NET_PACKET *packet = malloc(sizeof(NET_PACKET));
	packet->hdr.type = type;
	packet->hdr.safe = safe;
	if (size > NET_DATA_SIZE)
		size = NET_DATA_SIZE;
	packet->hdr.size = size;
	if (size && data != NULL)
	{
		memcpy(packet->data, data, size);
	}
	packet->hdr.id = net_id++;
	if (addr != NULL)
		memcpy(&packet->hdr.addr, addr, sizeof(NET_ADDR));
	return packet;
}
void Net_DestroyPacket(NET_PACKET *packet)
{
	free(packet);
}
