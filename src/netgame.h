#ifndef _netgame_h_
#define _netgame_h_
#include <stdint.h>
#include "utility.h"
#include "net.h"
#include "wobj.h"

#define NETGAME_SERVER_NODE 0
#define NETGAME_MAX_OBJECTS ((NET_DATA_SIZE / WOBJ_NET_SIZE) - 1)
#define NETGAME_TICK_RATE 3
#define NETGAME_MAX_INTERP_UNOWNEDS 128
#define NETGAME_MAX_MISCUPDATES 16

#define NETGAME_DENIED_ON_VERSION 0
#define NETGAME_DENIED_ON_PLAYER_COUNT 1
#define NETGAME_DENIED_ON_BAN 2
typedef struct _CONNECTION_REQUEST
{
	char version[UTIL_MAX_TEXT_WIDTH + 1];
	char player_name[UTIL_MAX_TEXT_WIDTH + 1];
	unsigned char has_supervirus;
} CONNECTION_REQUEST;

typedef struct _NEW_CONNECTION_INFO
{
	int node_id;
	int for_your_connection;
	int enable_pvp;
	int sv_cheats;
	NET_ADDR node_addr;
	char name[UTIL_MAX_TEXT_WIDTH + 1];
	char level[UTIL_MAX_TEXT_WIDTH + 1];
	unsigned char has_supervirus;
} NEW_CONNECTION_INFO;

typedef struct _NET_OWNED_OBJECTS_BUFFER
{
	int num_objects;
	WOBJ *wobjs[NETGAME_MAX_OBJECTS * 4];
} NET_OWNED_OBJECTS_BUFFER;

typedef struct _NET_RECV_OBJECTS_BUFFER
{
	int num_objects;
	struct wobjdata wobjs[NETGAME_MAX_OBJECTS * 4];
	WOBJ *final_wobjs[NETGAME_MAX_OBJECTS * 4];
} NET_RECV_OBJECTS_BUFFER;

typedef struct _NET_OWNED_OBJECTS
{
	uint16_t num_objects;
	uint16_t node;
	uint32_t frame;				// This is the frame that the objects were sent on
	uint32_t last_frame;		// This is the last frame that they got from me.
	uint8_t delta_frame;		// This is a delta to the current frame that the objects were sent on
								// that represents the frame that the objects were delta compressed to
} NET_OWNED_OBJECTS;

typedef struct _CLIENT_CAMERA_POSITION
{
	int node;
	float x, y;
} CLIENT_CAMERA_POSITION;

#define CLINET_WOBJ_UPDATE_NOTHING 0
#define CLIENT_WOBJ_UPDATE_HEALTH 2
#define CLIENT_WOBJ_UPDATE_DESTROY 1
#define CLIENT_WOBJ_UPDATE_LOCATION 3
#define CLIENT_WOBJ_UPDATE_FLAGS 4
typedef struct _CLIENT_WOBJ_UPDATE_REQUEST
{
	int node;
	int obj_uuid;
	int obj_node;
	int mode;

	float posx, posy;

	int flag_set_mask, flag_clear_mask;

	float hp_taken_away;
} CLIENT_WOBJ_UPDATE_REQUEST;

typedef struct _NET_CHAT_MESSAGE
{
	int source_node;
	char messagebuf[UTIL_MAX_TEXT_WIDTH * 2 + 1];
} NET_CHAT_MESSAGE;

typedef struct _NETGAME_INTERPER
{
	int uuid, node;
	float x, y;
	float sx, sy;
} NETGAME_INTERPER;

typedef struct _NETGAME_DAMAGE_PACKET
{
	int packet_number;
	int num_entries;
	const uint8_t *buf;
	int head;
} NETGAME_DAMAGE_PACKET;

typedef struct _NETGAME_DAMAGE_ENTRY
{
	int obj_node;
	int obj_uuid;
	float damage_dealt;
} NETGAME_DAMAGE_ENTRY;

typedef struct netgame_playerfinish {
	int node;
	int ending_text_line;
} netgame_playerfinish_t;

typedef struct netgame_changemap {
	char level_name[32];
} netgame_changemap_t;

#define MAX_HISTORY_SIZE (NETGAME_MAX_OBJECTS * 5)
typedef struct _NETGAME_NODE
{
	int active;
	int id;
	NET_ADDR addr;
	char name[UTIL_MAX_TEXT_WIDTH + 1];
	float x, y;
	WOBJ *client_wobjs[NETGAME_MAX_OBJECTS * 4];
	unsigned char recv_states[NETGAME_MAX_OBJECTS * 4];
	int num_client_wobjs;
	WOBJ *client_player;
	int timed_out_timer;
	NETGAME_INTERPER interps[NETGAME_MAX_INTERP_UNOWNEDS];
	int interps_size;
	float avgframes_between_updates;
	int cur_interp_frames;
	int interp_frames_history[8];
	int current_audio_uuid;
	int nodes_first_update;
	int damage_packet_num;

	struct wobjdata wobj_history[NETGAME_MAX_HISTORY][MAX_HISTORY_SIZE];
	int history_frames[NETGAME_MAX_HISTORY];
	int last_frame;  // This is the last frame that they got from me.
	int frame;       // This is the frame that they are on.
} NETGAME_NODE;

void NetGame_Init(void);
void NetGame_Quit(void);
int NetGame_GetNumActiveNodes(void);
NETGAME_NODE *NetGame_GetNode(int node_id);
NETGAME_NODE *NetGame_FindNodeFromAddr(NET_ADDR addr);
NETGAME_NODE *NetGame_GetFreeNode(void);
void NetGame_Iterate(NETGAME_NODE **iter);
void NetGame_GenInterpCanidates(int from_node);
void NetGame_TryToApplyInterpToUnownedWobj(int from_node, WOBJ *wobj);
void NetGame_PrintChatMessage(const NET_CHAT_MESSAGE *msg);
void NetGame_Update(void);
void NetGame_AttemptWobjAudioPlayback(WOBJ *wobj);

void NetGame_ForceUnownedWobjsPosition(WOBJ *wobj, float x, float y);
void NetGame_ForceUnownedWobjsFlags(WOBJ *wobj, int set_flag_mask, int clear_flag_mask);
void NetGame_SendMiscWobjForcedChanges(int node);

void NetGame_DamageUnownedWobj(WOBJ *wobj, float damage);
float NetGame_GetClientWobjHealth(WOBJ *wobj);
void NetGame_ClientSendDamages(void);
void NetGame_ServerSendDamages(void);
void NetGame_GetClientDamages(uint8_t *buf, int n);
void NetGame_GetServerDamages(uint8_t *buf);

unsigned int netgame_get_sendframe(void);
void netgame_inc_sendframe(void);
void netgame_reset_sendframe(void);
void NetGame_ClearHistoryWobjsForNode(int node, int frame);
struct wobjdata *NetGame_SetHistoryWobjFromNode(int node, int frame, const struct wobjdata *data);
struct wobjdata *NetGame_GetHistoryWobjFromNode(int node, int frame, int wobj_node, int uuid);
void netgame_newbytes_per_wobj(void);
float netgame_avgbytes_per_wobj(void);
float netgame_avgbytes_per_server_unowned_wobj_from_client(void);
NET_RECV_OBJECTS_BUFFER *NetGame_RecvBuf(void);
void NetGame_RemoveAndAddNewUnownedServerWobjs(int node);
void NetGame_ConvertClientItemsToServer(int node, int for_disconnect);

//int generate_wobj_delta(WOBJ *delta, WOBJ *in, int tonode, int curr_history_index);
int NetGame_ConvertOwnedBufferToOwnedPacket(int apply_delta, const NET_OWNED_OBJECTS_BUFFER *buf,
											NET_OWNED_OBJECTS *obj, int currnode, uint32_t currframe,
											int numobjs, int start, int to_node);
void serialize_wobj_packet(int delta, const WOBJ *w, uint8_t *buf, int *head);
int parse_wobj_packet(WOBJ *w, const uint8_t *buf, int *head);

int serialize_wobj_update_packet(uint8_t *buf, CLIENT_WOBJ_UPDATE_REQUEST *update_req);
CLIENT_WOBJ_UPDATE_REQUEST *parse_wobj_update_packet(const uint8_t *buf);

void serialize_damage_packet(uint8_t *buf, uint32_t packet_num, uint32_t numentries, int *head);
void serialize_damage_entry(uint8_t *buf, NETGAME_DAMAGE_ENTRY *e, int *head);
NETGAME_DAMAGE_PACKET *parse_damage_packet(const uint8_t *buf);
NETGAME_DAMAGE_ENTRY *parse_damage_entry(const uint8_t *buf, int *head);
void netgame_add_to_destroy_ringbuf(int node, int uuid);
int netgame_should_create_unowned(int node, int uuid);

#endif
