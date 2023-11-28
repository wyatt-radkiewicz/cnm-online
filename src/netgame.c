#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "netgame.h"
#include "renderer.h"
#include "game.h"
#include "console.h"
#include "audio.h"
#include "client.h"
#include "player.h"
#include "packet.h"
#include "item.h"

#define DAMAGES_HASH_INC 4

static NETGAME_NODE nodes[NETGAME_MAX_NODES];
static NETGAME_DAMAGE_ENTRY *damages[NETGAME_MAX_OBJECTS];
static int damages_size[NETGAME_MAX_OBJECTS];
static int damages_packet_num = 0;

static int num_forced_changes = 0;
static CLIENT_WOBJ_UPDATE_REQUEST forced_changes[NETGAME_MAX_MISCUPDATES];

static WOBJ server_history[NETGAME_MAX_HISTORY][NETGAME_MAX_OBJECTS];
static unsigned int sendframe;

static int numwobj_bytes, numwobj_bytes2;
static int numwobjs_for_bytes, numwobjs_for_bytes2;

#define DRBUF_SZ 16
static int _drbuf_node[DRBUF_SZ];
static int _drbuf_uuid[DRBUF_SZ];
static int _drbuf_idx;

void NetGame_Init(void)
{
	int i;
	memset(nodes, 0, sizeof(nodes));
	for (i = 0; i < NETGAME_MAX_OBJECTS; i++)
	{
		damages_size[i] = DAMAGES_HASH_INC;
		damages[i] = malloc(damages_size[i] * sizeof(NETGAME_DAMAGE_ENTRY));
		memset(damages[i], 0, sizeof(*damages[i]));
	}
	for (i = 0; i < NETGAME_MAX_NODES; i++)
	{
		nodes[i].active = CNM_FALSE;
		nodes[i].id = i;
		nodes[i].current_audio_uuid = 0;
		nodes[i].nodes_first_update = CNM_TRUE;
	}
	numwobj_bytes = 0;
	numwobjs_for_bytes = 0;
	num_forced_changes = 0;

	for (int i = 0; i < DRBUF_SZ; i++) {
		_drbuf_node[i] = -1;
		_drbuf_uuid[i] = -1;
	}
	_drbuf_idx = 0;
	memset(forced_changes, 0, sizeof(forced_changes));
}
void NetGame_Quit(void)
{
	int i;
	for (i = 0; i < NETGAME_MAX_OBJECTS; i++)
		free(damages[i]);
}
int NetGame_GetNumActiveNodes(void)
{
	NETGAME_NODE *iter = NULL;
	int num = 0;
	NetGame_Iterate(&iter);
	while (iter != NULL)
	{
		num++;
		NetGame_Iterate(&iter);
	}
	return num;
}
NETGAME_NODE *NetGame_GetNode(int node_id)
{
	return nodes + node_id;
}
NETGAME_NODE *NetGame_GetFreeNode(void)
{
	int i;
	for (i = 0; i < NETGAME_MAX_NODES; i++)
	{
		if (!nodes[i].active)
			return nodes + i;
	}
	return NULL;
}
NETGAME_NODE *NetGame_FindNodeFromAddr(NET_ADDR addr)
{
	int i;
	for (i = 0; i < NETGAME_MAX_NODES; i++)
	{
		if (nodes[i].addr.host == addr.host && nodes[i].addr.port == addr.port && nodes[i].active)
			return nodes + i;
	}
	return NULL;
}
void NetGame_Iterate(NETGAME_NODE **iter)
{
	int i;
	for (i = (*iter != NULL ? (*iter)->id + 1 : 0); i < NETGAME_MAX_NODES; i++)
	{
		if (nodes[i].active)
		{
			*iter = nodes + i;
			return;
		}	
	}
	*iter = NULL;
}
void NetGame_GenInterpCanidates(int from_node)
{
	const float pad = 48.0f;
	int gx, gy, i, j;
	int camx = Game_GetVar(GAME_VAR_CAMERA_X)->data.integer, camy = Game_GetVar(GAME_VAR_CAMERA_Y)->data.integer;
	WOBJ_GRID_ITER iter;

	memmove(nodes[from_node].interp_frames_history + 1, nodes[from_node].interp_frames_history, sizeof(int) * 7);
	nodes[from_node].interp_frames_history[0] = nodes[from_node].cur_interp_frames;
	nodes[from_node].cur_interp_frames = 0;
	float avg = 0.0f;
	for (i = 0; i < 8; i++)
	{
		avg += (float)nodes[from_node].interp_frames_history[i];
	}
	nodes[from_node].avgframes_between_updates = avg / 8.0f;
	for (i = 0; i < NETGAME_MAX_INTERP_UNOWNEDS; i++)
	{
		nodes[from_node].interps[i].node = -1;
	}

	nodes[from_node].interps_size = 0;
	for (gx = (int)floorf((float)camx / OBJGRID_SIZE) - 1; gx < (int)ceilf(((float)(camx + RENDERER_MAX_WIDTH)) / OBJGRID_SIZE); gx++)
	{
		for (gy = (int)floorf((float)camy / OBJGRID_SIZE) - 1; gy < (int)ceilf(((float)(camy + RENDERER_MAX_HEIGHT)) / OBJGRID_SIZE); gy++)
		{
			Wobj_InitGridIteratorUnowned(&iter, gx, gy);
			while (iter.wobj != NULL)
			{
				if ((int)iter.wobj->x + (int)iter.wobj->hitbox.w > (camx - pad) && (int)iter.wobj->x < (camx + RENDERER_MAX_WIDTH + pad) &&
					(int)iter.wobj->y + (int)iter.wobj->hitbox.h > (camy - pad) && (int)iter.wobj->y < (camy + RENDERER_MAX_HEIGHT + pad) &&
					nodes[from_node].interps_size < NETGAME_MAX_INTERP_UNOWNEDS &&
					wobj_types[iter.wobj->type].do_interp)
				{
					for (i = 0; i < NETGAME_MAX_INTERP_UNOWNEDS; i++) {
						j = (iter.wobj->uuid + i) % NETGAME_MAX_INTERP_UNOWNEDS;
						if (nodes[from_node].interps[j].node == -1)
						{
							nodes[from_node].interps[j].uuid = iter.wobj->uuid;
							nodes[from_node].interps[j].node = iter.wobj->node_id;
							nodes[from_node].interps[j].x = iter.wobj->x;
							nodes[from_node].interps[j].y = iter.wobj->y;
							nodes[from_node].interps[j].sx = iter.wobj->smooth_x;
							nodes[from_node].interps[j].sy = iter.wobj->smooth_y;
							break;
						}
					}

					nodes[from_node].interps_size++;
				}
				Wobj_GridIteratorIterate(&iter);
			}
		}
	}
}
void NetGame_TryToApplyInterpToUnownedWobj(int from_node, WOBJ *wobj)
{
	int i, j;
	for (i = 0; i < NETGAME_MAX_INTERP_UNOWNEDS; i++)
	{
		j = (wobj->uuid + i) % NETGAME_MAX_INTERP_UNOWNEDS;
		if (nodes[from_node].interps[j].node == wobj->node_id && nodes[from_node].interps[j].uuid == wobj->uuid &&
			nodes[from_node].interps[j].node != -1)
		{
			wobj->interpolate = CNM_TRUE;
			wobj->interp_x = nodes[from_node].interps[j].x;
			wobj->interp_y = nodes[from_node].interps[j].y;
			wobj->smooth_x = nodes[from_node].interps[j].sx;
			wobj->smooth_y = nodes[from_node].interps[j].sy;
			return;
		}
	}
}
void NetGame_PrintChatMessage(const NET_CHAT_MESSAGE *msg)
{
	char buf[UTIL_MAX_TEXT_WIDTH * 3 + 1] = {'\0'};
	sprintf(buf, "%s: %s", nodes[msg->source_node].name, msg->messagebuf);
	Console_Print(buf);
}
void NetGame_Update(void)
{
	for (int i = 0; i < NETGAME_MAX_NODES; i++)
	{
		nodes[i].cur_interp_frames++;
	}
}
void NetGame_AttemptWobjAudioPlayback(WOBJ *wobj)
{
	if (nodes[wobj->node_id].nodes_first_update)
	{
		nodes[wobj->node_id].current_audio_uuid = wobj->last_sound_uuid + 1;
		return;
	}
	if (nodes[wobj->node_id].current_audio_uuid <= wobj->last_sound_uuid)
	{
		Audio_PlaySound(wobj->last_sound_played, CNM_TRUE, (int)wobj->x, (int)wobj->y);
		nodes[wobj->node_id].current_audio_uuid = wobj->last_sound_uuid + 1;
	}
}

#define DAMAGE_HASH(w) (((unsigned short)(w->node_id + w->uuid) ^ (unsigned short)(w->node_id >> 4)) % NETGAME_MAX_OBJECTS)

static NETGAME_DAMAGE_ENTRY *find_damage(WOBJ *wobj, int s)
{
	unsigned short hash = DAMAGE_HASH(wobj);
	int i, free_loc;

	// Find the wobj
	free_loc = -1; // Index of a free hash bucket
	for (i = 0; i < damages_size[hash]; i++)
	{
		if (damages[hash][i].damage_dealt != 0.0f && damages[hash][i].obj_node == wobj->node_id && damages[hash][i].obj_uuid == wobj->uuid)
			goto wobj_found;
		else if (damages[hash][i].damage_dealt == 0.0f)
			free_loc = i;
	}

	if (!s) // If this isn't a garenteed safe search, then just die
		return NULL;

	// Create a new hash entry using floc (if it isnt -1)
	if (free_loc == -1)
	{
		// Expand the hash bucket
		free_loc = damages_size[hash];
		damages_size[hash] += DAMAGES_HASH_INC;
		damages[hash] = realloc(damages[hash], damages_size[hash] * sizeof(NETGAME_DAMAGE_ENTRY));
	}

	damages[hash][free_loc].obj_node = wobj->node_id;
	damages[hash][free_loc].obj_uuid = wobj->uuid;
	i = free_loc;

wobj_found:
	return &damages[hash][i];
}
void NetGame_DamageUnownedWobj(WOBJ *wobj, float damage)
{
	find_damage(wobj, CNM_TRUE)->damage_dealt += damage;
}
float NetGame_GetClientWobjHealth(WOBJ *w)
{
	NETGAME_DAMAGE_ENTRY *e = find_damage(w, CNM_FALSE);
	return w->health - (e ? e->damage_dealt : 0.0f);
}
static void clear_all_damages(void) {
	int i;
	for (i = 0; i < NETGAME_MAX_OBJECTS; i++)
	{
		memset(damages[i], 0, sizeof(NETGAME_DAMAGE_ENTRY) * damages_size[i]);
	}
}
static void send_damages(int n, int m) //	Yes this was made by ShadowRealm9, I just like this way of formatting better,
//									although this is still a comprimise. I have a much better idea of formmating in my
//									head, but this will do for now because much later down the line, maybe even when the
//									game has been released I am going to start a new engine rewrite (a very portable and
//									optimized rewrite) that will include better netcode, better coding style, and this
//									code by then will probably be open-sourced (the only problem about this old version
//									then being open-sourced is the fact that all the bugs will show up, and this codebase
//									no doubt has ACE exploits and other things written all over it). Rant over (I'm a
//									little sleepy right now and want to play CS:S instead but got to get this damage
//									bug fixed where you deal less damage in multiplayer. Probably because of dropped
//									packets). P.S: This whole codebase is a goliath compared to anything else I made and
//									it is a dirty monster with old code everywhere, and now conflicting styles. (I
//									wonder if most big projects end up this way, atleast one-man-team ones)
{
////
	int i, j;

	NET_PACKET *p = Net_CreatePacket(NET_DAMAGED_OBJECTS, CNM_TRUE, &NetGame_GetNode(m)->addr, 0, NULL);
	NETGAME_DAMAGE_PACKET d;
	d.num_entries = 0;
	NETGAME_DAMAGE_ENTRY o[NETGAME_MAX_OBJECTS];

	for (i = 0; i < NETGAME_MAX_OBJECTS; i++)
	{
		for (j = 0; j < damages_size[i]; j++)
			if (damages[i][j].damage_dealt != 0.0f && d.num_entries < NETGAME_MAX_OBJECTS &&
				(damages[i][j].obj_node == n || n == -1))
				memcpy(&o[d.num_entries++], &damages[i][j], sizeof(NETGAME_DAMAGE_ENTRY));
	}
	if (d.num_entries)
	{
		d.packet_number = damages_packet_num++;
		//p->hdr.size = d->num_entries * sizeof(NETGAME_DAMAGE_ENTRY) + sizeof(NETGAME_DAMAGE_PACKET);
		int head = 0;
		serialize_damage_packet(p->data, d.packet_number, d.num_entries, &head);
		for (i = 0; i < d.num_entries; i++) {
			serialize_damage_entry(p->data, o + i, &head);
		}
		p->hdr.size = packet_bytecount(head);
		Net_Send(p);
	}
}
void NetGame_ClientSendDamages(void)
{
	send_damages(-1, 0);
	clear_all_damages();
}
void NetGame_ServerSendDamages(void)
{
	NETGAME_NODE *iter = NULL;
	NetGame_Iterate(&iter);

	while (iter != NULL)
	{
		if (iter->id != 0)
			send_damages(iter->id, iter->id);
		NetGame_Iterate(&iter);
	}
	clear_all_damages();
}
void NetGame_GetClientDamages(uint8_t *buf, int n)
{
	int i;
	NETGAME_DAMAGE_PACKET *p = parse_damage_packet(buf);

	if (p->packet_number > nodes[n].damage_packet_num)
		nodes[n].damage_packet_num = p->packet_number;
	else
		return;

	for (i = 0; i < p->num_entries; i++)
	{
		NETGAME_DAMAGE_ENTRY *d = parse_damage_entry(buf, &p->head);
		if (d->obj_node == 0)
		{
			// Apply the damages
			WOBJ *w = Wobj_GetOwnedWobjFromUUID(d->obj_uuid);
			if (w != NULL)
			{
				if (w == Game_GetVar(GAME_VAR_PLAYER)->data.pointer &&
					Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer) {
					Player_OnRecievePVPDamage(w);
				}
				w->flags |= WOBJ_DAMAGE_INDICATE;
				w->health -= d->damage_dealt;
			}
		}
		else
		{
			// Create a damage request
			WOBJ *w = Wobj_GetAnyWOBJFromUUIDAndNode(d->obj_node, d->obj_uuid);
			if (w != NULL)
				NetGame_DamageUnownedWobj(w, d->damage_dealt);
		}
	}
}
void NetGame_GetServerDamages(uint8_t *buf)
{
	int i;
	NETGAME_DAMAGE_PACKET *p = parse_damage_packet(buf);

	if (p->packet_number > nodes[0].damage_packet_num)
		nodes[0].damage_packet_num = p->packet_number;
	else
		return;

	for (i = 0; i < p->num_entries; i++)
	{
		NETGAME_DAMAGE_ENTRY *d = parse_damage_entry(buf, &p->head);
		if (d->obj_node == Client_GetNode()->id)
		{
			// Apply the damages
			WOBJ *w = Wobj_GetOwnedWobjFromUUID(d->obj_uuid);
			if (w != NULL)
			{
				if (w == Game_GetVar(GAME_VAR_PLAYER)->data.pointer &&
					Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer) {
					Player_OnRecievePVPDamage(w);
				}
				w->health -= d->damage_dealt;
			}
		}
	}
}

static CLIENT_WOBJ_UPDATE_REQUEST *NetGame_GetFreeForcedChange(WOBJ *wobj)
{
	int i;
	for (i = 0; i < num_forced_changes; i++)
	{
		// If found return
		if (forced_changes[i].obj_node == wobj->node_id &&
			forced_changes[i].obj_uuid == wobj->uuid)
			return forced_changes + i;
	}

	if (i < NETGAME_MAX_MISCUPDATES)
		return forced_changes + (num_forced_changes++);
	else
		return NULL;
}
static void NetGame_InitForcedUpdateRequest(CLIENT_WOBJ_UPDATE_REQUEST *req, WOBJ *wobj, int mode)
{
	req->obj_node = wobj->node_id;
	req->obj_uuid = wobj->uuid;
	req->flag_set_mask = 0;
	req->flag_clear_mask = -1;
	req->mode = mode;
}

void NetGame_ForceUnownedWobjsFlags(WOBJ *wobj, int set_flag_mask, int clear_flag_mask) {
	// Find the node we need
	CLIENT_WOBJ_UPDATE_REQUEST *req = NetGame_GetFreeForcedChange(wobj);
	if (req != NULL)
	{
		NetGame_InitForcedUpdateRequest(req, wobj, CLIENT_WOBJ_UPDATE_FLAGS);
		req->flag_set_mask = set_flag_mask;
		req->flag_clear_mask = clear_flag_mask;
	}
}
void NetGame_ForceUnownedWobjsPosition(WOBJ *wobj, float x, float y)
{
	// Find the node we need
	CLIENT_WOBJ_UPDATE_REQUEST *req = NetGame_GetFreeForcedChange(wobj);
	if (req != NULL)
	{
		NetGame_InitForcedUpdateRequest(req, wobj, CLIENT_WOBJ_UPDATE_LOCATION);
		req->posx = x;
		req->posy = y;
	}
}
void NetGame_SendMiscWobjForcedChanges(int node)
{
	for (int i = 0; i < num_forced_changes; i++)
	{
		forced_changes[i].node = node;
		NET_PACKET *p = Net_CreatePacket(NET_CLIENT_OBJECT_UPDATE_REQUEST, CNM_FALSE, &NetGame_GetNode(forced_changes[i].obj_node)->addr, sizeof(CLIENT_WOBJ_UPDATE_REQUEST), forced_changes + i);
		p->hdr.size = serialize_wobj_update_packet((uint8_t *)p->data, forced_changes + i);
		Net_Send(p);
	}
	memset(forced_changes, 0, sizeof(forced_changes));
	num_forced_changes = 0;
}

unsigned int netgame_get_sendframe(void) {
	return sendframe;
}
void netgame_inc_sendframe(void) {
	sendframe++;
}
void netgame_reset_sendframe(void) {
	sendframe = 1;
}
void NetGame_ClearHistoryWobjsForNode(int node, int frame) {
	NETGAME_NODE *n = &nodes[node];
	int local_frame;

	local_frame = frame % NETGAME_MAX_HISTORY;
	memset(n->wobj_history + local_frame, 0, sizeof(n->wobj_history[0]));
	n->history_frames[local_frame] = frame;
}
struct wobjdata *NetGame_SetHistoryWobjFromNode(int node, int frame, const struct wobjdata *data) {
	NETGAME_NODE *n = &nodes[node];
	int local_frame, i;
	struct wobjdata *curr;

	local_frame = frame % NETGAME_MAX_HISTORY;
	if (local_frame < 0) local_frame += NETGAME_MAX_HISTORY;
	if (n->history_frames[local_frame] != frame) return NULL;
	for (i = 0; i < MAX_HISTORY_SIZE; i++) {
		curr = &n->wobj_history[local_frame][(i + data->uuid) % MAX_HISTORY_SIZE];
		if (curr->type == WOBJ_NULL) {
			memcpy(curr, data, sizeof(struct wobjdata));
			return curr;
		}
	}
	return NULL;
}
struct wobjdata *NetGame_GetHistoryWobjFromNode(int node, int frame, int wobj_node, int uuid) {
	NETGAME_NODE *n = &nodes[node];
	int local_frame, i;
	struct wobjdata *curr;

	local_frame = frame % NETGAME_MAX_HISTORY;
	if (local_frame < 0) local_frame += NETGAME_MAX_HISTORY;
	if (n->history_frames[local_frame] != frame) return NULL;
	for (i = 0; i < MAX_HISTORY_SIZE; i++)
	{
		curr = &n->wobj_history[local_frame][(i + uuid) % MAX_HISTORY_SIZE];
		if (curr->node_id == wobj_node && curr->uuid == uuid)
		{
			return curr;
		}
	}
	return NULL;
}
void netgame_newbytes_per_wobj(void) {
	numwobjs_for_bytes = 0;
	numwobjs_for_bytes2 = 0;
	numwobj_bytes = 0;
	numwobj_bytes2 = 0;
}
float netgame_avgbytes_per_wobj(void) {
	return numwobjs_for_bytes ? ((float)numwobj_bytes / (float)numwobjs_for_bytes) : 0.0f;
}
float netgame_avgbytes_per_server_unowned_wobj_from_client(void) {
	return numwobjs_for_bytes2 ? ((float)numwobj_bytes2 / (float)numwobjs_for_bytes2) : 0.0f;
}
NET_RECV_OBJECTS_BUFFER *NetGame_RecvBuf(void) {
	static NET_RECV_OBJECTS_BUFFER buf;
	return &buf;
}

// Removeing/adding
#define RECV_DESTROY 0
#define RECV_KEEP 1
void NetGame_RemoveAndAddNewUnownedServerWobjs(int _node) {
	NET_RECV_OBJECTS_BUFFER *rbuf = NetGame_RecvBuf();
	NETGAME_NODE *node = &nodes[_node];
	int i, j;

	memset(node->recv_states, RECV_DESTROY, sizeof(node->recv_states));
	memset(rbuf->final_wobjs, 0, sizeof(rbuf->final_wobjs));
	for (j = 0; j < rbuf->num_objects; j++)
	{
		for (i = 0; i < node->num_client_wobjs; i++)
		{
			if (node->client_wobjs[i]->uuid == rbuf->wobjs[j].uuid) {
				rbuf->final_wobjs[j] = node->client_wobjs[i];
				node->recv_states[i] = RECV_KEEP;
			}
		}
	}

	// Remove all wobjs that aren't here anymore
	for (i = 0; i < node->num_client_wobjs; i++) {
		if (node->recv_states[i] == RECV_DESTROY) {
			// Remove that wobj
			if (node->client_wobjs[i] != NULL)
				Wobj_DestroyWobj(node->client_wobjs[i]);
			node->client_wobjs[i] = NULL;
		}
	}

	// Add wobjs and update/keep wobjs
	node->num_client_wobjs = 0;
	for (j = 0; j < rbuf->num_objects; j++) {
		node->client_wobjs[node->num_client_wobjs] = rbuf->final_wobjs[j];
		if (node->client_wobjs[node->num_client_wobjs] == NULL) {
			node->client_wobjs[node->num_client_wobjs] =
				Wobj_CreateUnowned(rbuf->wobjs[j].type, rbuf->wobjs[j].x, rbuf->wobjs[j].y, rbuf->wobjs[j].anim_frame,
								   rbuf->wobjs[j].flags, 0, 0.0f, _node, rbuf->wobjs[j].uuid);
		}
		else {
			Wobj_RecreateUnwoned(node->client_wobjs[node->num_client_wobjs], rbuf->wobjs[j].x, rbuf->wobjs[j].y);
		}
		memcpy(node->client_wobjs[node->num_client_wobjs], &rbuf->wobjs[j], WOBJ_NET_SIZE);
		node->client_wobjs[node->num_client_wobjs]->interpolate = CNM_FALSE;
		if (node->client_wobjs[node->num_client_wobjs]->type == WOBJ_PLAYER)
			node->client_player = node->client_wobjs[node->num_client_wobjs];
		Wobj_UpdateGridPos(node->client_wobjs[node->num_client_wobjs]);
		NetGame_TryToApplyInterpToUnownedWobj(node->id, node->client_wobjs[node->num_client_wobjs]);
		//Interaction_ApplyHurtPacketsToWobj(node->client_wobjs[current_wobj]);
		NetGame_AttemptWobjAudioPlayback(node->client_wobjs[node->num_client_wobjs]);
		node->num_client_wobjs++;
	}
}

void NetGame_ConvertClientItemsToServer(int _node, int for_disconnect) {
	int i;
	NETGAME_NODE *node = &nodes[_node];
	WOBJ *wobj;

	for (i = 0; i < node->num_client_wobjs; i++) {
		wobj = node->client_wobjs[i];
		if (wobj->type == WOBJ_DROPPED_ITEM) {
			WOBJ *new_item = Wobj_CreateOwned(WOBJ_DROPPED_ITEM, wobj->x, wobj->y, wobj->custom_ints[0], 0.0f);
			new_item->custom_floats[0] = wobj->custom_floats[0];
			if (!for_disconnect) {
				CLIENT_WOBJ_UPDATE_REQUEST req;
				req.node = 0;
				req.mode = CLIENT_WOBJ_UPDATE_DESTROY;
				req.obj_node = wobj->node_id;
				req.obj_uuid = wobj->uuid;
				NET_PACKET *p =
					Net_CreatePacket(NET_CLIENT_OBJECT_UPDATE_REQUEST, CNM_TRUE, &NetGame_GetNode(wobj->node_id)->addr, sizeof(req), &req);
				p->hdr.size = serialize_wobj_update_packet((uint8_t *)p->data, &req);
				Net_Send(p);
			}
		}
		if (wobj->type == WOBJ_PLAYER && for_disconnect && wobj->item != ITEM_TYPE_NOITEM) {
			Wobj_CreateOwned(WOBJ_DROPPED_ITEM, wobj->x, wobj->y, wobj->item, 0.0f);
		}
	}
}

int NetGame_ConvertOwnedBufferToOwnedPacket(int apply_delta, const NET_OWNED_OBJECTS_BUFFER *buf, NET_OWNED_OBJECTS *obj,
											int currnode, uint32_t currframe, int numobjs, int start, int to_node)
{
	int i, num_deltas, can_delta;
	int head;
	uint8_t *wobjbuf;
	WOBJ temp;

	obj->num_objects = (uint16_t)numobjs;
	obj->node = (uint16_t)currnode;
	obj->frame = currframe;
	obj->last_frame = nodes[to_node].frame;
	obj->delta_frame = 0;
	if (apply_delta) {
		obj->delta_frame = currframe - nodes[to_node].last_frame;
	}
	wobjbuf = (uint8_t *)obj + sizeof(NET_OWNED_OBJECTS);

	head = 0;
	num_deltas = 0;
	for (i = start; i < start+numobjs; i++) {
		if (currnode != 0) can_delta = apply_delta && buf->wobjs[i]->internal.owned;
		else can_delta = apply_delta;
		if (can_delta) {
			if (Wobj_GetDeltaForWobj(&temp, buf->wobjs[i], to_node, nodes[to_node].last_frame, currframe)) {
				serialize_wobj_packet(1, &temp, wobjbuf, &head);
				Wobj_MarkForBeingSent(buf->wobjs[i], to_node, currframe);
				num_deltas++;
				continue;
			}
		}
		serialize_wobj_packet(0, buf->wobjs[i], wobjbuf, &head);
		Wobj_MarkForBeingSent(buf->wobjs[i], to_node, currframe);
	}

	if (num_deltas == 0) {
		obj->delta_frame = 0;
	}

	return packet_bytecount(head) + sizeof(NET_OWNED_OBJECTS);
}

// Net Packing for Packets things
void serialize_wobj_packet(int delta, const WOBJ *w, uint8_t *buf, int *head) {
	packet_write_bit(buf, head, delta);
	packet_write_float(buf, head, w->x);
	packet_write_float(buf, head, w->y);
	packet_write_float(buf, head, w->vel_x);
	packet_write_float(buf, head, w->vel_y);
	packet_write_float(buf, head, w->custom_floats[0]);
	packet_write_float(buf, head, w->custom_floats[1]);
	packet_write_s32(buf, head, w->custom_ints[0]);
	packet_write_s32(buf, head, w->custom_ints[1]);
	packet_write_float(buf, head, w->speed);
	packet_write_float(buf, head, w->jump);
	packet_write_float(buf, head, w->strength);
	packet_write_float(buf, head, w->health);
	packet_write_s32(buf, head, w->money);
	packet_write_s32(buf, head, w->anim_frame);
	packet_write_s16(buf, head, w->type);
	packet_write_s16(buf, head, w->item);
	packet_write_s32(buf, head, w->flags);
	packet_write_s16(buf, head, w->last_sound_played);
	packet_write_s32(buf, head, w->last_sound_uuid);
	packet_write_s16(buf, head, w->link_node);
	packet_write_s32(buf, head, w->link_uuid);
	packet_write_float(buf, head, w->hitbox.x);
	packet_write_float(buf, head, w->hitbox.y);
	packet_write_float(buf, head, w->hitbox.w);
	packet_write_float(buf, head, w->hitbox.h);
	packet_write_s16(buf, head, w->node_id);
	packet_write_s32(buf, head, w->uuid);
}
int parse_wobj_packet(WOBJ *w, const uint8_t *buf, int *head) {
	int delta = packet_read_bit(buf, head);
	int head_start = *head;
	w->x = packet_read_float(buf, head);
	w->y = packet_read_float(buf, head);
	w->vel_x = packet_read_float(buf, head);
	w->vel_y = packet_read_float(buf, head);
	w->custom_floats[0] = packet_read_float(buf, head);
	w->custom_floats[1] = packet_read_float(buf, head);
	w->custom_ints[0] = packet_read_s32(buf, head);
	w->custom_ints[1] = packet_read_s32(buf, head);
	w->speed = packet_read_float(buf, head);
	w->jump = packet_read_float(buf, head);
	w->strength = packet_read_float(buf, head);
	w->health = packet_read_float(buf, head);
	w->money = packet_read_s32(buf, head);
	w->anim_frame = packet_read_s32(buf, head);
	w->type = packet_read_s16(buf, head);
	w->item = packet_read_s16(buf, head);
	w->flags = packet_read_s32(buf, head);
	w->last_sound_played = packet_read_s16(buf, head);
	w->last_sound_uuid = packet_read_s32(buf, head);
	w->link_node = packet_read_s16(buf, head);
	w->link_uuid = packet_read_s32(buf, head);
	w->hitbox.x = packet_read_float(buf, head);
	w->hitbox.y = packet_read_float(buf, head);
	w->hitbox.w = packet_read_float(buf, head);
	w->hitbox.h = packet_read_float(buf, head);
	w->node_id = packet_read_s16(buf, head);
	w->uuid = packet_read_s32(buf, head);
	numwobj_bytes += packet_bytecount(*head - head_start);
	numwobjs_for_bytes++;
	if (w->node_id != 0) {
		numwobj_bytes2 += packet_bytecount(*head - head_start);
		numwobjs_for_bytes2++;
	}
	return delta;
}

int serialize_wobj_update_packet(uint8_t *buf, CLIENT_WOBJ_UPDATE_REQUEST *req)
{
	int head = 0;
	packet_write_bits(buf, &head, req->mode, 2);
	if (req->mode == CLINET_WOBJ_UPDATE_NOTHING) return packet_bytecount(head);
	packet_write_u8(buf, &head, req->node);
	packet_write_u8(buf, &head, req->obj_node);
	packet_write_u32(buf, &head, req->obj_uuid);
	if (req->mode == CLIENT_WOBJ_UPDATE_DESTROY) return packet_bytecount(head);
	if (req->mode == CLIENT_WOBJ_UPDATE_HEALTH) {
		packet_write_float(buf, &head, req->hp_taken_away);
	}
	else if (req->mode == CLIENT_WOBJ_UPDATE_LOCATION) {
		packet_write_float(buf, &head, req->posx);
		packet_write_float(buf, &head, req->posy);
	}
	return packet_bytecount(head);
}
CLIENT_WOBJ_UPDATE_REQUEST *parse_wobj_update_packet(const uint8_t *buf)
{
	static CLIENT_WOBJ_UPDATE_REQUEST req;
	int head = 0;

	req.mode = (int)packet_read_bits(buf, &head, 2);
	if (req.mode == CLINET_WOBJ_UPDATE_NOTHING) return &req;
	req.node = packet_read_u8(buf, &head);
	req.obj_node = packet_read_u8(buf, &head);
	req.obj_uuid = packet_read_u32(buf, &head);
	if (req.mode == CLIENT_WOBJ_UPDATE_DESTROY) return &req;
	if (req.mode == CLIENT_WOBJ_UPDATE_HEALTH)
	{
		req.hp_taken_away = packet_read_float(buf, &head);
	}
	else if (req.mode == CLIENT_WOBJ_UPDATE_LOCATION)
	{
		req.posx = packet_read_float(buf, &head);
		req.posy = packet_read_float(buf, &head);
	}

	return &req;
}

void serialize_damage_packet(uint8_t *buf, uint32_t packet_num, uint32_t numentries, int *head) {
	packet_write_u32(buf, head, packet_num);
	packet_write_u8(buf, head, (uint8_t)numentries);
}
void serialize_damage_entry(uint8_t *buf, NETGAME_DAMAGE_ENTRY *e, int *head) {
	packet_write_u8(buf, head, e->obj_node);
	packet_write_u32(buf, head, e->obj_uuid);
	packet_write_float(buf, head, e->damage_dealt);
}
NETGAME_DAMAGE_PACKET *parse_damage_packet(const uint8_t *buf) {
	static NETGAME_DAMAGE_PACKET p;

	p.head = 0;
	p.buf = buf;
	p.packet_number = packet_read_u32(buf, &p.head);
	p.num_entries = packet_read_u8(buf, &p.head);
	return &p;
}
NETGAME_DAMAGE_ENTRY *parse_damage_entry(const uint8_t *buf, int *head) {
	static NETGAME_DAMAGE_ENTRY e;

	e.obj_node = packet_read_u8(buf, head);
	e.obj_uuid = packet_read_u32(buf, head);
	e.damage_dealt = packet_read_float(buf, head);
	return &e;
}

void netgame_add_to_destroy_ringbuf(int node, int uuid) {
	_drbuf_node[_drbuf_idx] = node;
	_drbuf_uuid[_drbuf_idx] = uuid;
	_drbuf_idx = (_drbuf_idx + 1) % DRBUF_SZ;
}
int netgame_should_create_unowned(int node, int uuid) {
	for (int i = 0; i < DRBUF_SZ; i++) {
		if (_drbuf_node[i] == node && _drbuf_uuid[i] == uuid) return CNM_FALSE;
	}
	return CNM_TRUE;
}
