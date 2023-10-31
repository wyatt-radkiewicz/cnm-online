#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "serial.h"
#include "console.h"
#include "blocks.h"
#include "wobj.h"
#include "spawners.h"
#include "game.h"
#include "teleport_infos.h"
#include "renderer.h"
#include "audio.h"
#include "player_spawns.h"
#include "ending_text.h"
#include "filesystem.h"
#include "background.h"
#include "player.h"

typedef struct _SERIAL_BHDR
{
	int num_flags;
	int w, h;
} SERIAL_BHDR;

void Serial_LoadBlocks_Old(const char *cnmb_file)
{
	int i;
	FILE *f;
	SERIAL_BHDR hdr;
	BLOCK_PROPS props;
	BLOCK block;
	unsigned char light;

	if (cnmb_file[strlen(cnmb_file) - 1] != 'b')
		return;

	Console_Print("Loading map blocks: %s...", cnmb_file);
	f = fopen(cnmb_file, "rb");
	if (f != NULL)
	{
		fread(&hdr, sizeof(hdr), 1, f);
		for (i = 0; i < BACKGROUND_MAX_LAYERS; i++)
		{
			fread(Background_GetLayer(i), SERIAL_BACKGROUND_SIZE, 1, f);
		}
		Blocks_GenBlockProps(hdr.num_flags);
		for (i = 0; i < hdr.num_flags; i++)
		{
			fread(&props, SERIAL_BLOCK_PROPS_SIZE, 1, f);
			memcpy(Blocks_GetBlockProp(i), &props, SERIAL_BLOCK_PROPS_SIZE);
		}
		Blocks_SetWorldSize(hdr.w, hdr.h);
		for (i = 0; i < hdr.w * hdr.h; i++)
		{
			fread(&block, sizeof(BLOCK), 1, f);
			Blocks_SetBlock(BLOCKS_FG, i % hdr.w, i / hdr.w, block);
			fread(&block, sizeof(BLOCK), 1, f);
			Blocks_SetBlock(BLOCKS_BG, i % hdr.w, i / hdr.w, block);
		}
		for (i = 0; i < hdr.w * hdr.h; i++)
		{
			fread(&light, sizeof(light), 1, f);
			Blocks_SetBlockAmbientLight(i % hdr.w, i / hdr.w, light);
		}
		FileSystem_RegisterBlocks(cnmb_file);
		fclose(f);
	}
	else
	{
		Console_Print("Couldn't open the map block file %s!", cnmb_file);
	}
}
void Serial_LoadBlocksLevelPreview_Old(const char *cnmb_file, CNM_RECT *r, int *dif)
{
	FILE *f;

	if (cnmb_file[strlen(cnmb_file) - 1] != 'b')
		return;

	//Console_Print("Loading map preview: %s...", cnmb_file);
	f = fopen(cnmb_file, "rb");
	if (f != NULL)
	{
		BLOCK_PROPS preview;
		fseek(f, sizeof(SERIAL_BHDR) + SERIAL_BACKGROUND_SIZE * BACKGROUND_MAX_LAYERS + 256 * SERIAL_BLOCK_PROPS_SIZE, SEEK_SET);
		fread(&preview, SERIAL_BLOCK_PROPS_SIZE, 1, f);
		r->x = preview.frames_x[0] * BLOCK_SIZE;
		r->y = preview.frames_y[0] * BLOCK_SIZE;
		r->w = BLOCK_SIZE * 3;
		r->h = BLOCK_SIZE * 2;
		*dif = preview.dmg;
		fclose(f);
	}
	else
	{
		Console_Print("Couldn't open the map block preview file %s!", cnmb_file);
	}
}
void Serial_LoadSpawnersLevelName_Old(const char *cnms_file, char *name_buf)
{
	FILE *f;

	if (cnms_file[strlen(cnms_file) - 1] != 's')
		return;

	//Console_Print("Loading map name: %s...", cnms_file);
	f = fopen(cnms_file, "rb");
	if (f != NULL)
	{
		char buf[UTIL_MAX_TEXT_WIDTH + 1];

		fseek
		(
			f,
			(PLAYER_SPAWNS_MAX * PLAYER_SPAWN_TYPE_MAX * sizeof(float) * 2) +
			(TELEPORT_INFOS_MAX_TELEPORTS_SERIALOLD * SERIAL_TELEPORT_INFO_SIZE) +
			((ENDING_TEXT_MAX_LINES - 1) * ENDING_TEXT_MAX_WIDTH),
			SEEK_SET
		);
		memset(buf, 0, sizeof(buf));
		fread(buf, ENDING_TEXT_MAX_WIDTH, 1, f);
		buf[UTIL_MAX_TEXT_WIDTH - 1] = '\0';
		char *lb;
		while ((lb = strchr(buf, '\\')) != NULL)
			*lb = ' ';
		strcpy(name_buf, buf);
		fclose(f);
	}
	else
	{
		Console_Print("Couldn't open the map spawner file for level name: %s!", cnms_file);
	}
}
void Serial_LoadSpawners_Old(const char *cnms_file)
{
	int num_spawners, i;
	SPAWNER s;
	SPAWNER *created;

	if (cnms_file[strlen(cnms_file) - 1] != 's')
		return;

	FILE *fp = fopen(cnms_file, "rb");
	Console_Print("Loading spawner file: %s!", cnms_file);
	if (fp != NULL)
	{
		TeleportInfos_AllocLegacyLevelInfo();
		for (i = 0; i < PLAYER_SPAWNS_MAX * PLAYER_SPAWN_TYPE_MAX; i++)
		{
			float ps[2];
			fread(ps, sizeof(ps), 1, fp);
			PlayerSpawns_SetSpawnLoc(i, ps[0], ps[1]);
		}
		for (i = 0; i < TELEPORT_INFOS_MAX_TELEPORTS_SERIALOLD; i++)
		{
			fread(TeleportInfos_GetTeleport(i), SERIAL_TELEPORT_INFO_SIZE, 1, fp);
		}

		char ending_text_buffer[ENDING_TEXT_MAX_WIDTH + 1];
		for (i = 0; i < ENDING_TEXT_MAX_LINES; i++)
		{
			fread(ending_text_buffer, ENDING_TEXT_MAX_WIDTH, 1, fp);
			ending_text_buffer[ENDING_TEXT_MAX_WIDTH] = '\0';
			EndingText_SetLine(i, ending_text_buffer);
		}

		fread(&num_spawners, sizeof(num_spawners), 1, fp);
		Spawners_UnloadSpawners();
		for (i = 0; i < num_spawners; i++)
		{
			fread(&s, SPAWNER_SERIALIZATION_SIZE, 1, fp);
			created = Spawners_CreateSpawner(s.x, s.y, s.wobj_type, s.duration, s.max, -1);
			created->custom_int = s.custom_int;
			created->custom_float = s.custom_float;
			created->dropped_item = s.dropped_item;
			//int s_item = SPAWNER_GET_DROPPED_ITEM(&s);
			//SPAWNER_SET_DROPPED_ITEM(created, s_item);
		}
		FileSystem_RegisterSpawners(cnms_file);
		fclose(fp);
	}
	else
	{
		Console_Print("Could not open the spawner file: %s!", cnms_file);
	}
}
