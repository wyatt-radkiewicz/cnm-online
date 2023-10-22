#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "utility.h"
#include "client.h"
#include "game.h"
#include "console.h"
#include "wobj.h"
#include "player.h"
#include "packet.h"
#include "renderer.h"
#include "obj_grid.h"
#include "fadeout.h"
#include "world.h"
#include "audio.h"

static int connected_to_server;
static int downloading_files;
static NETGAME_NODE *client_node;
static WOBJ *player;
static int *camx;
static int *camy;
static int last_server_objects_frame = -1;
static int self_timeout = 0;
static int _level_transition_timer = -1;

NETGAME_NODE *Client_GetNode(void)
{
	return client_node;
}
void Client_Create(NET_ADDR addr)
{
	NET_PACKET *p;
	connected_to_server = CNM_FALSE;
	downloading_files = CNM_TRUE;
	client_node = NULL;
	self_timeout = 0;
	last_server_objects_frame = -1;
	Net_AddPollingFunc(Client_Update);
	Interaction_SetMode(INTERACTION_MODE_CLIENT);

	NetGame_GetNode(0)->active = CNM_TRUE;
	NetGame_GetNode(0)->addr = addr;
	NetGame_GetNode(0)->frame = -1;
	NetGame_GetNode(0)->last_frame = -1;
	strcpy(NetGame_GetNode(0)->name, "SERVER");

	netgame_reset_sendframe();
	CONNECTION_REQUEST con_req;
	strcpy(con_req.player_name, Game_GetVar(GAME_VAR_PLAYER_NAME)->data.string);
	strcpy(con_req.version, CNM_VERSION_STRING);
	con_req.has_supervirus = Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer;
	p = Net_CreatePacket(NET_CONNECTION_REQUEST, CNM_TRUE, &addr, sizeof(con_req), &con_req);
	Net_Send(p);
	_level_transition_timer = -1;
}
void Client_Destory(void)
{
	if (client_node != NULL)
	{
		NET_PACKET *p = Net_CreatePacket(NET_DISCONNECT, 1, &NetGame_GetNode(0)->addr, sizeof(int), &client_node->id);
		Net_Send(p);
	}
	Net_RemovePollingFunc(Client_Update);
}
void Client_Update(NET_PACKET *packet)
{
	self_timeout = 0;

	if (packet->hdr.type == NET_CONNECTION_ACCEPT)
	{
		NEW_CONNECTION_INFO *info = (void *)packet->data;
		if (info->for_your_connection)
		{
			client_node = NetGame_GetNode(info->node_id);
			client_node->active = CNM_TRUE;
			client_node->addr = Net_GetIpFromString("127.0.0.1");
			strcpy(client_node->name, Game_GetVar(GAME_VAR_PLAYER_NAME)->data.string);
			connected_to_server = CNM_TRUE;
			Wobj_SetNodeId(client_node->id);

			Game_GetVar(GAME_VAR_ENABLE_SERVER_PVP)->data.integer = info->enable_pvp;
			Game_GetVar(GAME_VAR_SV_CHEATS)->data.integer = info->sv_cheats;
			strcpy(Game_GetVar(GAME_VAR_LEVEL)->data.string, info->level);
			if (Game_TopState() != GAME_STATE_CLIENT)
				Game_SwitchState(GAME_STATE_CLIENT);
		}
		else
		{
			NetGame_GetNode(info->node_id)->active = CNM_TRUE;
			if (info->node_id != NETGAME_SERVER_NODE)
				NetGame_GetNode(info->node_id)->addr = info->node_addr;
			strcpy(NetGame_GetNode(info->node_id)->name, info->name);
			NetGame_GetNode(info->node_id)->current_audio_uuid = 0;
			NetGame_GetNode(info->node_id)->nodes_first_update = CNM_TRUE;
		}
		if (info->has_supervirus && !Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer) {
			Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer = 1;
			Audio_PlayMusic(AUDIO_MAX_IDS - 1, CNM_TRUE);
		}
		Console_Print("Player %s connected with node id %d!", info->name, info->node_id);
	}

	if (packet->hdr.type == NET_CONNECTION_DENIED) {
		int info = *((int *)packet->data);

		if (info == NETGAME_DENIED_ON_VERSION)
			Console_Print("Server has a different version!");
		if (info == NETGAME_DENIED_ON_PLAYER_COUNT)
			Console_Print("Too many players in the server!");
		if (info == NETGAME_DENIED_ON_BAN)
			Console_Print("You have been banned from the server!");

		Game_SwitchState(GAME_STATE_MAINMENU);
	}

	if (packet->hdr.type == NET_DISCONNECT_BROADCAST)
	{
		int node = *((int *)packet->data + 0);
		Console_Print("Player %s disconnected with node id %d.", NetGame_GetNode(node)->name, NetGame_GetNode(node)->id);
		NetGame_GetNode(node)->active = CNM_FALSE;
	}

	if (!connected_to_server)
		return;

	if (packet->hdr.type == NET_SERVER_OWNED_OBJECTS)
	{
		Wobj_CreateUnowned(WOBJ_SLIME, 0, 0, 0, 0, 0, 0, 0, 0);
		WOBJ wobj_data;
		NET_OWNED_OBJECTS header;
		memcpy(&header, packet->data, sizeof(NET_OWNED_OBJECTS));
		if (last_server_objects_frame != header.frame)
		{
			netgame_newbytes_per_wobj();
			NetGame_GenInterpCanidates(0);
			Wobj_DestroyUnownedWobjs();
			Interaction_ClearDestroyedWobjsBuffer();
			NetGame_ClearHistoryWobjsForNode(0, header.frame);
		}
		NetGame_GetNode(0)->frame = header.frame;
		NetGame_GetNode(0)->last_frame = header.last_frame;
		struct wobjdata *last_data;
		int i, ptr = 0, head = 0, delta_compressed, delta_frame, num_errors, num_deltas;
		delta_frame = header.frame - header.delta_frame;
		ptr += sizeof(NET_OWNED_OBJECTS);
		num_errors = 0;
		num_deltas = 0;
		
		for (i = 0; i < header.num_objects; i++)
		{
			//NetGame_ParseWobjFromOwnedPacket((NET_OWNED_OBJECTS*)packet->data, &head);
			delta_compressed = parse_wobj_packet(&wobj_data, (uint8_t*)packet->data+sizeof(NET_OWNED_OBJECTS), &head);
			if (delta_compressed && header.delta_frame != 0) {
				// Uncompress the delta
				num_deltas++;
				last_data = NetGame_GetHistoryWobjFromNode(0, delta_frame, wobj_data.node_id, wobj_data.uuid);
				if (last_data != NULL) {
					Wobj_UncompressDelta((struct wobjdata *)&wobj_data, (struct wobjdata *)&wobj_data, last_data);
				}
				else {
					num_errors++;
					continue;
				}
			}
			NetGame_SetHistoryWobjFromNode(0, header.frame, (struct wobjdata *)&wobj_data);
			//memcpy(&wobj_data, packet->data + ptr, WOBJ_NET_SIZE);
 			WOBJ *new_wobj = Wobj_CreateUnowned(wobj_data.type, wobj_data.x, wobj_data.y, wobj_data.anim_frame, wobj_data.flags, 0, 0.0f, wobj_data.node_id, wobj_data.uuid);
			memcpy(new_wobj, &wobj_data, WOBJ_NET_SIZE);
			new_wobj->interpolate = CNM_FALSE;
			NetGame_TryToApplyInterpToUnownedWobj(0, new_wobj);
			//Interaction_ApplyHurtPacketsToWobj(new_wobj);
			NetGame_AttemptWobjAudioPlayback(new_wobj);
			ptr += WOBJ_NET_SIZE;
			//Debug_PrintUnowneds();
		}
		if (last_server_objects_frame != header.frame)
		{
			//Interaction_SendWobjHurtPackets(-1);
		}
		last_server_objects_frame = header.frame;
		for (i = 0; i < NETGAME_MAX_NODES; i++)
			NetGame_GetNode(i)->nodes_first_update = CNM_FALSE;

		if ((float)num_errors / (float)num_deltas >= 0.25f) {
			// We need a full update....
			NetGame_GetNode(0)->frame = -1;
		}
	}

	if (packet->hdr.type == NET_CLIENT_OBJECT_UPDATE_REQUEST)
	{
		CLIENT_WOBJ_UPDATE_REQUEST *req = parse_wobj_update_packet((uint8_t *)packet->data);
		if (req->obj_node == client_node->id)
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
	}

	if (packet->hdr.type == NET_DAMAGED_OBJECTS)
	{
		NetGame_GetServerDamages(packet->data);
	}

	if (packet->hdr.type == NET_CHAT_MESSAGE_BROADCAST)
	{
		NET_CHAT_MESSAGE *msg = (void *)packet->data;
		NetGame_PrintChatMessage(msg);
	}

	if (packet->hdr.type == NET_CHANGE_MAP)
	{
		Fadeout_FadeToBlack(10, 40, 30);
		netgame_changemap_t *changemap = (void *)packet->data;
		strcpy(Game_GetVar(GAME_VAR_LEVEL)->data.string, "levels/");
		strcat(Game_GetVar(GAME_VAR_LEVEL)->data.string, changemap->level_name);
		_level_transition_timer = 40;
	}
}
void Client_OnLevelStart(void)
{
	player = Game_GetVar(GAME_VAR_PLAYER)->data.pointer;
	camx = &Game_GetVar(GAME_VAR_CAMERA_X)->data.integer;
	camy = &Game_GetVar(GAME_VAR_CAMERA_Y)->data.integer;
}
void Client_Tick(void)
{
	if (_level_transition_timer > -1) _level_transition_timer--;
	if (_level_transition_timer == 0) {
		World_Stop();
		//Console_Print(Game_GetVar(GAME_VAR_LEVEL)->data.string);
		World_Start(WORLD_MODE_CLIENT);
		Client_OnLevelStart();
		return;
	}

	if (Game_GetFrame() % 20 == 0)
		NetGame_ClientSendDamages();

	if (Game_GetFrame() % 5 == 0)
		NetGame_SendMiscWobjForcedChanges(client_node->id);

	if (self_timeout++ > 30*15) {
		Console_Print("Timed-out due to no server connection for 15 seconds!");
		Game_SwitchState(GAME_STATE_MAINMENU);
	}

	if (Game_GetFrame() % NETGAME_TICK_RATE == 0)
	{
		CLIENT_CAMERA_POSITION pos;
		pos.node = client_node->id;
		pos.x = (float)(*camx);
		pos.y = (float)(*camy);
		Net_Send(Net_CreatePacket(NET_CLIENT_SENT_CAMERA_POSITION, 0, &NetGame_GetNode(0)->addr, sizeof(pos), &pos));

		netgame_inc_sendframe();
		Wobj_RecordWobjHistory(netgame_get_sendframe());

		//int i, head;
		NET_OWNED_OBJECTS_BUFFER buffer;
		WOBJ *iter = NULL;
		NET_PACKET *packet;

		buffer.num_objects = 0;
		Wobj_IterateOverOwned(&iter);
		buffer.wobjs[buffer.num_objects++] = player;
		while (iter != NULL)
		{
			if (buffer.num_objects < NETGAME_MAX_OBJECTS && iter != player)
				buffer.wobjs[buffer.num_objects++] = iter;
			Wobj_IterateOverOwned(&iter);
		}
		//NET_OWNED_OBJECTS header;
		//int ptr, i;
		//packet = Net_CreatePacket(NET_CLIENT_OWNED_OBJECTS, 0, &NetGame_GetNode(0)->addr, sizeof(NET_OWNED_OBJECTS) + buffer.num_objects * WOBJ_NET_SIZE, NULL);
		//header.frame = Game_GetFrame();
		//header.num_objects = buffer.num_objects;
		//header.node = client_node->id;
		//ptr = 0;
		//memcpy(packet->data + ptr, &header, sizeof(NET_OWNED_OBJECTS));
		//ptr += sizeof(NET_OWNED_OBJECTS);
		//for (i = 0; i < buffer.num_objects; i++)
		//{
		//	memcpy(packet->data + ptr, buffer.wobjs[i], WOBJ_NET_SIZE);
		//	ptr += WOBJ_NET_SIZE;
		//}
		packet = Net_CreatePacket(NET_CLIENT_OWNED_OBJECTS, 0, &NetGame_GetNode(0)->addr, 0, NULL);
		packet->hdr.size = NetGame_ConvertOwnedBufferToOwnedPacket(!(Game_GetFrame() % (30 * 3) == 0), &buffer,
																	(NET_OWNED_OBJECTS *)packet->data,
																	client_node->id,
																	netgame_get_sendframe(),
																	buffer.num_objects,
																	0,
																	0);
		Net_Send(packet);
	}
}

void Client_SendChatMessage(const char *message)
{
	NET_CHAT_MESSAGE msg;
	strncpy(msg.messagebuf, message, UTIL_MAX_TEXT_WIDTH * 2);
	msg.messagebuf[UTIL_MAX_TEXT_WIDTH * 2] = '\0';
	msg.source_node = client_node->id;
	NetGame_PrintChatMessage(&msg);
	NET_PACKET *packet = Net_CreatePacket(NET_CHAT_MESSAGE_BROADCAST, 1, &NetGame_GetNode(0)->addr, sizeof(msg), &msg);
	Net_Send(packet);
}
