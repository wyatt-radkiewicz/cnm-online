#include <string.h>
#include "server.h"
#include "console.h"
#include "game.h"
#include "renderer.h"
#include "wobj.h"
#include "player.h"
#include "master_server.h"
#include "ending_text.h"
#include "fadeout.h"
#include "world.h"
#include "audio.h"

static int _players_finished[NETGAME_MAX_NODES];
static int _num_finished, _level_transition_timer;

static void Server_ResetFinishedPlayers(void);

void Server_Create(void)
{
	Server_ResetFinishedPlayers();
	Net_AddPollingFunc(Server_Update);
	NetGame_GetNode(0)->active = CNM_TRUE;
	strcpy(NetGame_GetNode(0)->name, Game_GetVar(GAME_VAR_PLAYER_NAME)->data.string);
	NetGame_GetNode(0)->addr = Net_GetIpFromString("127.0.0.1");
	Wobj_SetNodeId(0);
	netgame_reset_sendframe();
} 
void Server_Destroy(void)
{
	Net_RemovePollingFunc(Server_Update);
}
void Server_Update(NET_PACKET *packet)
{
	int i;
	NETGAME_NODE *new_node = NULL, *node_iter = NULL, *node_iter2 = NULL;
	NET_PACKET *p;
	if (packet->hdr.type == NET_CONNECTION_REQUEST &&
		NetGame_FindNodeFromAddr(packet->hdr.addr) == NULL)
	{
		// Authenticate version from request
		CONNECTION_REQUEST *con_req = (CONNECTION_REQUEST *)packet->data;
		if (strcmp(con_req->version, CNM_VERSION_STRING) != 0) {
			int denied_type;

			denied_type = NETGAME_DENIED_ON_VERSION;
			p = Net_CreatePacket(NET_CONNECTION_DENIED, 1, &packet->hdr.addr, sizeof(int), &denied_type);
			Net_Send(p);
			return;
		}

		if (con_req->has_supervirus && !Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer) {
			Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer = 1;
			Audio_PlayMusic(AUDIO_MAX_IDS - 1, CNM_TRUE);
		}

		// Now add the new player
		NEW_CONNECTION_INFO info;
		new_node = NetGame_GetFreeNode();
		new_node->timed_out_timer = 0;
		new_node->active = CNM_TRUE;
		new_node->addr = packet->hdr.addr;
		new_node->current_audio_uuid = 0;
		new_node->nodes_first_update = CNM_TRUE;
		new_node->damage_packet_num = -1;
		new_node->frame = -1;
		new_node->last_frame = -1;
		new_node->num_client_wobjs = 0;
		info.node_id = new_node->id;
		info.node_addr = new_node->addr;
		info.enable_pvp = Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer;
		info.sv_cheats = Game_GetVar(GAME_VAR_SV_CHEATS)->data.integer;
		info.has_supervirus = con_req->has_supervirus || Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer;
		strcpy(info.level, Game_GetVar(GAME_VAR_LEVEL)->data.string);
		strcpy(info.name, con_req->player_name);
		strcpy(new_node->name, con_req->player_name);
		Console_Print("Player %s connected with node id %d!", info.name, info.node_id);
		do
		{
			NetGame_Iterate(&node_iter);
			if (node_iter == NULL)
				break;
			if (node_iter->id == NETGAME_SERVER_NODE)
				continue;
			if (node_iter == new_node)
			{
				NEW_CONNECTION_INFO info2;
				info.for_your_connection = CNM_TRUE;
				p = Net_CreatePacket(NET_CONNECTION_ACCEPT, 1, &node_iter->addr, sizeof(info), &info);
				Net_Send(p);

				node_iter2 = NULL;
				NetGame_Iterate(&node_iter2);
				while (node_iter2 != NULL)
				{
					info2.for_your_connection = CNM_FALSE;
					strcpy(info2.name, node_iter2->name);
					info2.node_id = node_iter2->id;
					info2.enable_pvp = Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer;
					info2.sv_cheats = Game_GetVar(GAME_VAR_SV_CHEATS)->data.integer;
					info2.node_addr = node_iter2->addr;
					info2.has_supervirus = Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer;
					if (node_iter2 == new_node)
					{
						NetGame_Iterate(&node_iter2);
						continue;
					}
					p = Net_CreatePacket(NET_CONNECTION_ACCEPT, 1, &node_iter->addr, sizeof(info2), &info2);
					Net_Send(p);
					NetGame_Iterate(&node_iter2);
				}
			}
			else
			{
				info.for_your_connection = CNM_FALSE;
				p = Net_CreatePacket(NET_CONNECTION_ACCEPT, 1, &node_iter->addr, sizeof(info), &info);
				Net_Send(p);
			}
		} while (node_iter != NULL);
	}

	if (packet->hdr.type == NET_DISCONNECT)
	{
		if (NetGame_FindNodeFromAddr(packet->hdr.addr) == NULL || NetGame_FindNodeFromAddr(packet->hdr.addr)->active == CNM_FALSE)
			return;
		node_iter = NULL;
		int node = *((int *)packet->data);
		Console_Print("Player %s disconnected with node id %d.", NetGame_GetNode(node)->name, NetGame_GetNode(node)->id);
		if (_players_finished[NetGame_GetNode(node)->id] > -1) {
			_players_finished[NetGame_GetNode(node)->id] = -1;
			_num_finished--;
		}
		NetGame_ConvertClientItemsToServer(node, CNM_TRUE);
		for (i = 0; i < NETGAME_MAX_OBJECTS && NetGame_GetNode(node)->client_wobjs[i] != NULL; i++)
		{
			Wobj_DestroyWobj(NetGame_GetNode(node)->client_wobjs[i]);
			NetGame_GetNode(node)->client_wobjs[i] = NULL;
		}
		NetGame_GetNode(node)->active = CNM_FALSE;
		do
		{
			NetGame_Iterate(&node_iter);
			if (!node_iter)
				break;
			if (node_iter == NETGAME_SERVER_NODE)
				continue;
			if (node_iter->id == node)
				continue;
			p = Net_CreatePacket(NET_DISCONNECT_BROADCAST, 1, &node_iter->addr, sizeof(node), &node);
			Net_Send(p);
		} while (node_iter != NULL);
		NetGame_GetNode(node)->active = CNM_FALSE;
		NetGame_GetNode(node)->timed_out_timer = 0;
	}

	if (packet->hdr.type == NET_CLIENT_SENT_CAMERA_POSITION)
	{
		if (NetGame_FindNodeFromAddr(packet->hdr.addr) == NULL || NetGame_FindNodeFromAddr(packet->hdr.addr)->active == CNM_FALSE)
			return;

		CLIENT_CAMERA_POSITION *cpos = (void *)packet->data;
		NetGame_GetNode(cpos->node)->x = cpos->x;
		NetGame_GetNode(cpos->node)->y = cpos->y;
		NetGame_GetNode(cpos->node)->timed_out_timer = 0;
	}

	if (packet->hdr.type == NET_CLIENT_OWNED_OBJECTS)
	{
		if (NetGame_FindNodeFromAddr(packet->hdr.addr) == NULL || NetGame_FindNodeFromAddr(packet->hdr.addr)->active == CNM_FALSE)
			return;
		WOBJ wobj_data;
		NET_OWNED_OBJECTS header;
		NETGAME_NODE *node;
		memcpy(&header, packet->data, sizeof(NET_OWNED_OBJECTS));
		int i, head = 0;
		node = NetGame_GetNode(header.node);
		node->timed_out_timer = 0;
		node->client_player = NULL;
		//ptr += sizeof(NET_OWNED_OBJECTS);
		netgame_newbytes_per_wobj();
		NetGame_GenInterpCanidates(node->id);
		NetGame_ClearHistoryWobjsForNode(node->id, header.frame);
		//for (i = 0; i < NETGAME_MAX_OBJECTS; i++)
		//{
		//	if (node->client_wobjs[i] != NULL)
		//		Wobj_DestroyWobj(node->client_wobjs[i]);
		//	node->client_wobjs[i] = NULL;
		//}
		node->frame = header.frame;
		node->last_frame = header.last_frame;
		struct wobjdata *last_data;
		int delta_compressed, delta_frame;
		delta_frame = header.frame - header.delta_frame;

		int num_errors = 0, num_deltas = 0;
		// First decompress the objects into a temporary buffer
		NET_RECV_OBJECTS_BUFFER *rbuf = NetGame_RecvBuf();
		rbuf->num_objects = 0;
		for (i = 0; i < header.num_objects; i++)
		{
			//memcpy(&wobj_data, packet->data + ptr, WOBJ_NET_SIZE);
			//ptr += WOBJ_NET_SIZE;
			delta_compressed = parse_wobj_packet(&wobj_data, (uint8_t*)packet->data+sizeof(NET_OWNED_OBJECTS), &head);
			if (delta_compressed && header.delta_frame != 0)
			{
				// Uncompress the delta
				num_deltas++;
				last_data = NetGame_GetHistoryWobjFromNode(node->id, delta_frame, wobj_data.node_id, wobj_data.uuid);
				if (last_data != NULL) {
					Wobj_UncompressDelta((struct wobjdata *)&wobj_data, (struct wobjdata *)&wobj_data, last_data);
				}
				else {
					num_errors++;
					continue;
				}
			}
			NetGame_SetHistoryWobjFromNode(node->id, header.frame, (struct wobjdata *)&wobj_data);

			if (netgame_should_create_unowned(wobj_data.node_id, wobj_data.uuid)) {
				memcpy(rbuf->wobjs + rbuf->num_objects, (struct wobjdata *)&wobj_data, sizeof(struct wobjdata));
				rbuf->num_objects++;
				if (rbuf->num_objects >= sizeof(rbuf->wobjs) / sizeof(struct wobjdata))
					break;
			}
		}
		NetGame_RemoveAndAddNewUnownedServerWobjs(node->id);
		node->nodes_first_update = CNM_FALSE;
		//Interaction_SendWobjHurtPackets(node->id);
		Interaction_ClearDestroyedWobjsBuffer();
		if ((float)num_errors / (float)num_deltas >= 0.25f) {
			// We need a full update....
			node->frame = -1;
		}
	}

	if (packet->hdr.type == NET_CLIENT_OBJECT_UPDATE_REQUEST)
	{
		if (NetGame_FindNodeFromAddr(packet->hdr.addr) == NULL || NetGame_FindNodeFromAddr(packet->hdr.addr)->active == CNM_FALSE)
			return;

		CLIENT_WOBJ_UPDATE_REQUEST *req = parse_wobj_update_packet((uint8_t *)packet->data);
		if (req->obj_node == 0)
		{
			WOBJ *wobj = Wobj_GetOwnedWobjFromUUID(req->obj_uuid);
			if (wobj != NULL)
			{
				switch (req->mode)
				{
				case CLIENT_WOBJ_UPDATE_DESTROY:
					if (wobj_types[wobj->type].hurt != NULL)
						wobj_types[wobj->type].hurt(wobj, NULL);
					Wobj_DestroyWobj(wobj);
					break;
				case CLIENT_WOBJ_UPDATE_HEALTH:
					wobj->health -= req->hp_taken_away;
					if (wobj == Game_GetVar(GAME_VAR_PLAYER)->data.pointer) {
						Player_OnRecievePVPDamage(wobj);
					}
					break;
				case CLIENT_WOBJ_UPDATE_LOCATION:
					if (wobj->type == WOBJ_PLAYER && wobj->flags & WOBJ_PLAYER_IS_RESPAWNING) {
						break;
					}
					wobj->x = req->posx;
					wobj->y = req->posy;
					wobj->vel_x = 0.0f;
					wobj->vel_y = 0.0f;
					Wobj_UpdateGridPos(wobj);
					break;
				}
			}
		}
		else
		{
			NET_PACKET *return_packet = Net_CreatePacket(packet->hdr.type, CNM_TRUE, &NetGame_GetNode(req->obj_node)->addr, packet->hdr.size, packet->data);
			Net_Send(return_packet);
		}
	}

	if (packet->hdr.type == NET_DAMAGED_OBJECTS)
	{
		NETGAME_NODE *n = NetGame_FindNodeFromAddr(packet->hdr.addr);
		if (n != NULL)
			NetGame_GetClientDamages(packet->data, n->id);
	}

	if (packet->hdr.type == NET_CHAT_MESSAGE_BROADCAST)
	{
		NET_CHAT_MESSAGE *msg = (void *)packet->data;
		Server_SendChatMessage(msg->source_node, msg->messagebuf);
	}

	if (packet->hdr.type == NET_PLAYER_FINISHED)
	{
		NETGAME_NODE *n = NetGame_FindNodeFromAddr(packet->hdr.addr);
		if (n != NULL) {
			netgame_playerfinish_t *finish = (void *)packet->data;
			//Console_Print("node %d finished using level: %s", finish->node, EndingText_GetLine(finish->ending_text_line));
			Server_PlayerFinish(finish->node, finish->ending_text_line);
		}
	}
}
static void Server_ResetFinishedPlayers(void) {
	memset(_players_finished, 0xff, sizeof(_players_finished));
	_num_finished = 0;
	_level_transition_timer = -1;
}
void Server_PlayerFinish(int node, int text_line) {
	if (_players_finished[node] < 0) _num_finished++;
	_players_finished[node] = text_line;
	//char textbuf[32];
	//strcpy(
	//Util_StringPrintF(textbuf, 32, "finished!", 0);
	Server_SendChatMessage(node, "finished!");
}
void Server_Tick(void)
{
	int gx, gy, i, /*ptr, num_objs,*/ j;
	NET_OWNED_OBJECTS_BUFFER buffer;
	WOBJ_GRID_ITER iter;
	NETGAME_NODE *node_iter = NULL, *node_iter2;
	NET_PACKET *packet;
	WOBJ *cplayer;

	// Stuff to do with finishing the level
	if (_num_finished >= NetGame_GetNumActiveNodes()) {
		int num_got_there[ENDING_TEXT_MAX_LINES] = { 0 };
		for (int i = 0; i < NETGAME_MAX_NODES; i++) {
			if (_players_finished[i] != -1) num_got_there[_players_finished[i]]++;
		}
		int winning_line = 0, most = 0;
		for (int i = 0; i < ENDING_TEXT_MAX_LINES; i++) {
			if (num_got_there[i] > most) {
				most = num_got_there[i];
				winning_line = i;
			}
		}
		netgame_changemap_t changemap;
		strcpy(changemap.level_name, EndingText_GetLine(winning_line));
		strcpy(Game_GetVar(GAME_VAR_LEVEL)->data.string, "levels/");
		strcat(Game_GetVar(GAME_VAR_LEVEL)->data.string, EndingText_GetLine(winning_line));
		NetGame_Iterate(&node_iter);
		while (node_iter != NULL)
		{
			if (node_iter == NetGame_GetNode(0))
			{
				NetGame_Iterate(&node_iter);
				continue;
			}
			NET_PACKET *packet = Net_CreatePacket(
				NET_CHANGE_MAP,
				1,
				&node_iter->addr, sizeof(changemap), &changemap);
			Net_Send(packet);
			NetGame_Iterate(&node_iter);
		}
		Fadeout_FadeToBlack(30, 20, 30);
		Server_ResetFinishedPlayers();
		_level_transition_timer = 40;
	}
	if (_level_transition_timer > -1) _level_transition_timer--;
	if (_level_transition_timer == 0) {
		World_Stop();
		//Console_Print(Game_GetVar(GAME_VAR_LEVEL)->data.string);
		World_Start(WORLD_MODE_HOSTED_SERVER);
		return;
	}

	if (Game_GetFrame() % 20 == 0)
		NetGame_ServerSendDamages();

	if (Game_GetFrame() % 5 == 0)
		NetGame_SendMiscWobjForcedChanges(0);

	if (Game_GetFrame() % (30*8) == 0 && Game_GetVar(GAME_VAR_ADVERTISE_SERVER)->data.integer)
	{
		// Send Master Server Ping over
		SERVER_INFO sinfo;
		NET_ADDR msaddr;
		strncpy(sinfo.level, Game_GetVar(GAME_VAR_FANCY_LEVEL_NAME)->data.string, sizeof(sinfo.level) - 1);
		sinfo.level[sizeof(sinfo.level) - 1] = '\0';
		strcpy(sinfo.name, Game_GetVar(GAME_VAR_SERVER_NAME)->data.string);
		sinfo.num_players = 0;
		NetGame_Iterate(&node_iter);
		while (node_iter != NULL)
		{
			if (node_iter == NetGame_GetNode(0) && Game_GetDedicatedGameInfo()->dedicated)
			{
				NetGame_Iterate(&node_iter);
				continue;
			}
			sinfo.num_players++;
			NetGame_Iterate(&node_iter);
		}
		sinfo.timeout_period = 1;
		msaddr = Net_GetIpFromString(Game_GetVar(GAME_VAR_MASTER_SERVER_ADDR)->data.string);
		msaddr.port = Net_HostToNetU16(NET_MSERVER_PORT);
		packet = Net_CreatePacket(NET_MASTER_SERVER_SERVER_PING, 0, &msaddr, sizeof(sinfo), &sinfo);
		Net_Send(packet);
	}

	// For when a player times out
	NetGame_Iterate(&node_iter);
	while (node_iter != NULL)
	{
		if (node_iter == NetGame_GetNode(0) || node_iter->timed_out_timer < 30*10)
		{
			if (node_iter != NetGame_GetNode(0))
				node_iter->timed_out_timer++;
			NetGame_Iterate(&node_iter);
			continue;
		}

		node_iter2 = NULL;
		int node = node_iter->id;
		Console_Print("Player %s timed out node id %d.", NetGame_GetNode(node)->name, NetGame_GetNode(node)->id);
		if (_players_finished[NetGame_GetNode(node)->id] > -1) {
			_players_finished[NetGame_GetNode(node)->id] = -1;
			_num_finished--;
		}
		NetGame_ConvertClientItemsToServer(node, CNM_TRUE);
		NetGame_GetNode(node)->active = CNM_FALSE;
		NetGame_GetNode(node)->addr.host = NET_IP_NULL;
		for (i = 0; i < NETGAME_MAX_OBJECTS && node_iter->client_wobjs[i] != NULL; i++)
		{
			Wobj_DestroyWobj(node_iter->client_wobjs[i]);
			node_iter->client_wobjs[i] = NULL;
		}
		do
		{
			NetGame_Iterate(&node_iter2);
			if (!node_iter2)
				break;
			if (node_iter == NETGAME_SERVER_NODE)
				continue;
			if (node_iter2->id == node)
				continue;
			packet = Net_CreatePacket(NET_DISCONNECT_BROADCAST, 1, &node_iter2->addr, sizeof(node), &node);
			Net_Send(packet);
		} while (node_iter2 != NULL);

		NetGame_Iterate(&node_iter);
	}

	node_iter = NULL;
	if (Game_GetFrame() % (30*5) == 0)
	{
		NetGame_Iterate(&node_iter);
		while (node_iter != NULL)
		{
			if (node_iter != NetGame_GetNode(0)) {
				packet = Net_CreatePacket(NET_SERVER_ISOKAY_PING, 0, &node_iter->addr, 0, NULL);
				Net_Send(packet);
			}
			NetGame_Iterate(&node_iter);
		}
	}

	// Convert Client objects to the server
	if (Game_GetFrame() % (30*10) == 0)
	{
		NetGame_Iterate(&node_iter);
		while (node_iter != NULL)
		{
			if (node_iter == NetGame_GetNode(0))
			{
				NetGame_Iterate(&node_iter);
				continue;
			}
			NetGame_ConvertClientItemsToServer(node_iter->id, CNM_FALSE);
			NetGame_Iterate(&node_iter);
		}
	}

	node_iter = NULL;
	int bufsize = sizeof(buffer.wobjs) / sizeof(buffer.wobjs[0]);
	if (Game_GetFrame() % NETGAME_TICK_RATE == 0)
	{
		netgame_inc_sendframe();
		Wobj_RecordWobjHistory(netgame_get_sendframe());
		//Wobj_ClearHistoryBitsOnOwnedWobjs(netgame_get_sendframe() % NETGAME_MAX_HISTORY);
		NetGame_Iterate(&node_iter);
		while (node_iter != NULL)
		{
			if (node_iter == NetGame_GetNode(0))
			{
				NetGame_Iterate(&node_iter);
				continue;
			}
			buffer.num_objects = 0;

			node_iter2 = NULL;
			buffer.wobjs[buffer.num_objects++] = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
			NetGame_Iterate(&node_iter2);
			while (node_iter2 != NULL) {
#define PLAYER_SEND_PAD 5.0f
				cplayer = node_iter2->client_player;
				if (cplayer != NULL && (cplayer->x / OBJGRID_SIZE) > (node_iter->x / OBJGRID_SIZE) - PLAYER_SEND_PAD &&
					(cplayer->x / OBJGRID_SIZE) < ((node_iter->x+RENDERER_WIDTH) / OBJGRID_SIZE) + PLAYER_SEND_PAD &&
					(cplayer->y / OBJGRID_SIZE) > (node_iter->y / OBJGRID_SIZE) - PLAYER_SEND_PAD &&
					(cplayer->y / OBJGRID_SIZE) < ((node_iter->y+RENDERER_HEIGHT) / OBJGRID_SIZE) + PLAYER_SEND_PAD &&
					buffer.num_objects < bufsize &&
					cplayer->node_id != node_iter->id)
					buffer.wobjs[buffer.num_objects++] = cplayer;
				NetGame_Iterate(&node_iter2);
			}

			for (gx = (int)(node_iter->x / OBJGRID_SIZE) - 2; gx < (int)((node_iter->x + RENDERER_WIDTH) / OBJGRID_SIZE) + 2; gx++)
			{
				for (gy = (int)(node_iter->y / OBJGRID_SIZE) - 2; gy < (int)((node_iter->y + RENDERER_HEIGHT) / OBJGRID_SIZE) + 2; gy++)
				{
					Wobj_InitGridIteratorOwned(&iter, gx, gy);
					
					while (iter.wobj != NULL)
					{
						if (buffer.num_objects < bufsize &&
							iter.wobj->type != WOBJ_PLAYER)
							buffer.wobjs[buffer.num_objects++] = iter.wobj;
						Wobj_GridIteratorIterate(&iter);
					}

					Wobj_InitGridIteratorUnowned(&iter, gx, gy);
					while (iter.wobj != NULL)
					{
						if (buffer.num_objects < bufsize &&
							iter.wobj->node_id != node_iter->id && iter.wobj->type != WOBJ_PLAYER)
							buffer.wobjs[buffer.num_objects++] = iter.wobj;
						Wobj_GridIteratorIterate(&iter);
					}
				}
			}

			if (buffer.num_objects == 0)
			{
				NET_OWNED_OBJECTS header;
				header.frame = netgame_get_sendframe();
				header.last_frame = node_iter->frame;
				header.num_objects = 0;
				header.node = 0;
				header.delta_frame = 0;
				packet = Net_CreatePacket(NET_SERVER_OWNED_OBJECTS, 0, &node_iter->addr, sizeof(NET_OWNED_OBJECTS), &header);
				Net_Send(packet);
				NetGame_Iterate(&node_iter);
				continue;
			}

			int num_objs = buffer.num_objects;
			j = 0;
			while (buffer.num_objects > 0)
			{
				num_objs = (buffer.num_objects > NETGAME_MAX_OBJECTS) ? (NETGAME_MAX_OBJECTS) : (buffer.num_objects);
			//	ptr = 0;
			//	NET_OWNED_OBJECTS header;
			//	packet = Net_CreatePacket(NET_SERVER_OWNED_OBJECTS, 0, &node_iter->addr, sizeof(NET_OWNED_OBJECTS) + num_objs * WOBJ_NET_SIZE, NULL);
			//	header.frame = Game_GetFrame();
			//	header.num_objects = num_objs;
			//	header.node = 0;
			//	memcpy(packet->data + ptr, &header, sizeof(NET_OWNED_OBJECTS));
			//	ptr += sizeof(NET_OWNED_OBJECTS);
			//	for (i = 0; i < num_objs; i++)
			//	{
			//		memcpy(packet->data + ptr, buffer.wobjs[j + i], WOBJ_NET_SIZE);
			//		ptr += WOBJ_NET_SIZE;
			//	}
			//	Net_Send(packet);
				packet = Net_CreatePacket(NET_SERVER_OWNED_OBJECTS, 0, &node_iter->addr, 0, NULL);
				packet->hdr.size = NetGame_ConvertOwnedBufferToOwnedPacket(	!(Game_GetFrame() % (30*3) == 0), &buffer,
																		   (NET_OWNED_OBJECTS *)packet->data,
																		   0,
																		   netgame_get_sendframe(),
																		   num_objs,
																		   j,
																		   node_iter->id);
				Net_Send(packet);
				
				j += num_objs;
				buffer.num_objects -= num_objs;
			}
			NetGame_Iterate(&node_iter);
		}
	}
}

void Server_SendChatMessage(int node, const char *message)
{
	NET_CHAT_MESSAGE msg;
	strncpy(msg.messagebuf, message, UTIL_MAX_TEXT_WIDTH * 2);
	msg.messagebuf[UTIL_MAX_TEXT_WIDTH * 2] = '\0';
	msg.source_node = node;
	NetGame_PrintChatMessage(&msg);
	NETGAME_NODE *iter = NULL;
	NetGame_Iterate(&iter);
	while (iter != NULL)
	{
		if (iter == NetGame_GetNode(0) || iter->id == msg.source_node)
		{
			NetGame_Iterate(&iter);
			continue;
		}
		NET_PACKET *packet = Net_CreatePacket(NET_CHAT_MESSAGE_BROADCAST, 1, &iter->addr, sizeof(msg), &msg);
		Net_Send(packet);
		NetGame_Iterate(&iter);
	}
}
