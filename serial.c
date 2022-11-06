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
#include "lparse.h"

typedef struct _SERIAL_BHDR
{
	int num_flags;
	int w, h;
} SERIAL_BHDR;

void Serial_LoadBlocks_Old(const char *cnmb_file);
void Serial_LoadBlocksLevelPreview_Old(const char *cnmb_file, CNM_RECT *r, int *dif);
void Serial_LoadSpawnersLevelName_Old(const char *cnms_file, char *name_buf);
void Serial_LoadSpawners_Old(const char *cnms_file);

static int Serial_GetLParser(const char *cnm_file, LParse **lp);
static int Serial_GetWriteLParser(const char *cnm_file, LParse **lp);

void Serial_LoadBlocks(const char *cnmb_file)
{
	LParse *lp;
	LParseEntry *e;
	unsigned char tmp1;
	unsigned short blk;
	int numprops, numblks, h, w;
	int i, j;

	if (cnmb_file[strlen(cnmb_file) - 1] != 'b')
		return;

	if (Serial_GetLParser(cnmb_file, &lp) == -1) Serial_LoadBlocks_Old(cnmb_file); if (lp == NULL) return;
	
	e = lparse_get_entry(lp, "BG_POS");
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_get_data(lp, e, i*2, 2, &Background_GetLayer(i)->pos);
	e = lparse_get_entry(lp, "BG_ORIGIN");
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_get_data(lp, e, i*2, 2, &Background_GetLayer(i)->origin);
	e = lparse_get_entry(lp, "BG_SCROLL");
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_get_data(lp, e, i*2, 2, &Background_GetLayer(i)->scroll);
	e = lparse_get_entry(lp, "BG_SPACING");
	for (i = 0; i < BACKGROUND_MAX_LAYERS && e != NULL; i++) lparse_get_data(lp, e, i*2, 2, &Background_GetLayer(i)->spacing);
	e = lparse_get_entry(lp, "BG_SPEED");
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_get_data(lp, e, i*2, 2, &Background_GetLayer(i)->speed);
	e = lparse_get_entry(lp, "BG_REPEAT");
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) {
		lparse_get_data(lp, e, i*2, 1, &tmp1);
		Background_GetLayer(i)->repeat[0] = (int)tmp1;
		lparse_get_data(lp, e, i*2+1, 1, &tmp1);
		Background_GetLayer(i)->repeat[1] = (int)tmp1;
	}
	e = lparse_get_entry(lp, "BG_RECT");
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_get_data(lp, e, i, 1, &Background_GetLayer(i)->bitmap);
	e = lparse_get_entry(lp, "BG_CLEAR_COLOR");
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_get_data(lp, e, i, 1, &Background_GetLayer(i)->clear_color);
	e = lparse_get_entry(lp, "BG_HIGHLAYER");
	if (e != NULL) {
		for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) {
			lparse_get_data(lp, e, i, 1, &tmp1);
			Background_GetLayer(i)->high = (int)tmp1;
		}
	}
	else for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) Background_GetLayer(i)->high = 0;
	e = lparse_get_entry(lp, "BG_TRANS");
	if (e != NULL) {
		for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) {
			lparse_get_data(lp, e, i, 1, &tmp1);
			Background_GetLayer(i)->transparency = (int)tmp1;
		}
	}
	else for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) Background_GetLayer(i)->transparency = 0;

	e = lparse_get_entry(lp, "BLOCKS_HEADER");
	lparse_get_data(lp, e, 0, 1, &w);
	lparse_get_data(lp, e, 1, 1, &h);
	lparse_get_data(lp, e, 2, 1, &numprops);
	numblks = w*h;
	Blocks_SetWorldSize(w, h);
	Blocks_GenBlockProps(numprops);

	e = lparse_get_entry(lp, "BLK_LAYER0");
	for (i = 0; i < numblks; i++) {
		lparse_get_data(lp, e, i, 1, &blk);
		Blocks_SetBlock(BLOCKS_FG, i % w, i / w, blk);
	}
	e = lparse_get_entry(lp, "BLK_LAYER1");
	for (i = 0; i < numblks; i++) {
		lparse_get_data(lp, e, i, 1, &blk);
		Blocks_SetBlock(BLOCKS_BG, i % w, i / w, blk);
	}
	e = lparse_get_entry(lp, "BLK_LIGHT");
	for (i = 0; i < numblks; i++) {
		lparse_get_data(lp, e, i, 1, &blk);
		Blocks_SetBlockAmbientLight(i % w, i / w, blk);
	}

	e = lparse_get_entry(lp, "BP_FLAGS");
	for (i = 0; i < numprops; i++) lparse_get_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->flags);
	e = lparse_get_entry(lp, "BP_TRANS");
	for (i = 0; i < numprops; i++) lparse_get_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->transparency);
	e = lparse_get_entry(lp, "BP_DMG_TYPE");
	for (i = 0; i < numprops; i++) lparse_get_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->dmg_type);
	e = lparse_get_entry(lp, "BP_DMG");
	for (i = 0; i < numprops; i++) lparse_get_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->dmg);
	e = lparse_get_entry(lp, "BP_ANIM_SPEED");
	for (i = 0; i < numprops; i++) lparse_get_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->anim_speed);
	e = lparse_get_entry(lp, "BP_NUM_FRAMES");
	for (i = 0; i < numprops; i++) lparse_get_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->num_frames);
	e = lparse_get_entry(lp, "BP_FRAMESX");
	for (i = 0; i < numprops; i++) lparse_get_data(lp, e, i*BLOCKS_MAX_FRAMES, BLOCKS_MAX_FRAMES, &Blocks_GetBlockProp(i)->frames_x);
	e = lparse_get_entry(lp, "BP_FRAMESY");
	for (i = 0; i < numprops; i++) lparse_get_data(lp, e, i*BLOCKS_MAX_FRAMES, BLOCKS_MAX_FRAMES, &Blocks_GetBlockProp(i)->frames_y);
	e = lparse_get_entry(lp, "BP_COLLTYPE");
	for (i = 0; i < numprops; i++) lparse_get_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->coll_type);
	e = lparse_get_entry(lp, "BP_HEIGHTMAP");
	for (i = 0; i < numprops; i++) {
		for (j = 0; j < BLOCK_SIZE; j++) {
			if (Blocks_GetBlockProp(i)->coll_type != BLOCKS_COLL_HEIGHT) continue;
			lparse_get_data(lp, e, i*BLOCK_SIZE+j, 1, &tmp1);
			Blocks_GetBlockProp(i)->coll_data.heightmap[j] = (int)tmp1;
		}
	}
	e = lparse_get_entry(lp, "BP_HITBOX");
	for (i = 0; i < numprops; i++) {
		if (Blocks_GetBlockProp(i)->coll_type != BLOCKS_COLL_BOX) continue;
		lparse_get_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->coll_data.hitbox);
	}
	FileSystem_RegisterBlocks(cnmb_file);

	lparse_close(lp);
}
void Serial_CondenceBlockFileAndSave(const char *cnmb_file)
{
	int x;
	int y;
	int neww;
	int newh;
	
	neww = 2;
	newh = 2;
	for (x = 0; x < Blocks_GetWorldWidth(); x++) {
		for (y = 0; y < Blocks_GetWorldHeight(); y++) {
			if (Blocks_GetBlock(BLOCKS_FG, x, y) != 0 ||
				Blocks_GetBlock(BLOCKS_BG, x, y) != 0 ||
				Blocks_GetBlockAmbientLight(x, y) != RENDERER_LIGHT) {
				if (x + 2 >= neww) neww = x + 2;
				if (y + 2 >= newh) newh = y + 2;
			}
		}
	}
	Blocks_ResizeWorld(neww, newh);
	Serial_SaveBlocks(cnmb_file);
}
void Serial_SaveBlocks(const char *cnmb_file)
{
	LParse *lp;
	LParseEntry *e;
	unsigned char tmp1;
	unsigned short blk;
	int numprops, numblks, w;
	int i, j;
	
	if (cnmb_file[strlen(cnmb_file) - 1] != 'b')
		return;

	if (!Serial_GetWriteLParser(cnmb_file, &lp)) return;
	
	e = lparse_make_entry(lp, "BG_POS", lparse_float, BACKGROUND_MAX_LAYERS * 2);
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_set_data(lp, e, i*2, 2, &Background_GetLayer(i)->pos);
	e = lparse_make_entry(lp, "BG_ORIGIN", lparse_float, BACKGROUND_MAX_LAYERS * 2);
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_set_data(lp, e, i*2, 2, &Background_GetLayer(i)->origin);
	e = lparse_make_entry(lp, "BG_SCROLL", lparse_float, BACKGROUND_MAX_LAYERS * 2);
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_set_data(lp, e, i*2, 2, &Background_GetLayer(i)->scroll);
	e = lparse_make_entry(lp, "BG_SPACING", lparse_i32, BACKGROUND_MAX_LAYERS * 2);
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_set_data(lp, e, i*2, 2, &Background_GetLayer(i)->spacing);
	e = lparse_make_entry(lp, "BG_SPEED", lparse_float, BACKGROUND_MAX_LAYERS * 2);
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_set_data(lp, e, i*2, 2, &Background_GetLayer(i)->speed);
	e = lparse_make_entry(lp, "BG_REPEAT", lparse_u8, BACKGROUND_MAX_LAYERS * 2);
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) {
		tmp1 = (unsigned char)Background_GetLayer(i)->repeat[0];
		lparse_set_data(lp, e, i*2, 1, &tmp1);
		tmp1 = (unsigned char)Background_GetLayer(i)->repeat[1];
		lparse_set_data(lp, e, i*2+1, 1, &tmp1);
	}
	e = lparse_make_entry(lp, "BG_RECT", lparse_cnm_rect, BACKGROUND_MAX_LAYERS);
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_set_data(lp, e, i, 1, &Background_GetLayer(i)->bitmap);
	e = lparse_make_entry(lp, "BG_CLEAR_COLOR", lparse_i32, BACKGROUND_MAX_LAYERS);
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) lparse_set_data(lp, e, i, 1, &Background_GetLayer(i)->clear_color);
	e = lparse_make_entry(lp, "BG_HIGHLAYER", lparse_u8, BACKGROUND_MAX_LAYERS);
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) {
		tmp1 = (unsigned char)Background_GetLayer(i)->high;
		lparse_set_data(lp, e, i, 1, &tmp1);
	}
	e = lparse_make_entry(lp, "BG_TRANS", lparse_u8, BACKGROUND_MAX_LAYERS);
	for (i = 0; i < BACKGROUND_MAX_LAYERS; i++) {
		tmp1 = (unsigned char)Background_GetLayer(i)->transparency;
		lparse_set_data(lp, e, i, 1, &tmp1);
	}

	e = lparse_make_entry(lp, "BLOCKS_HEADER", lparse_i32, 3);
	w = Blocks_GetWorldWidth();
	lparse_set_data(lp, e, 0, 1, &w);
	numprops = Blocks_GetWorldHeight();
	lparse_set_data(lp, e, 1, 1, &numprops);
	numblks = w*numprops;
	numprops = Blocks_GetBlockPropsCount();
	lparse_set_data(lp, e, 2, 1, &numprops);

	e = lparse_make_entry(lp, "BLK_LAYER0", lparse_u16, numblks);
	for (i = 0; i < numblks; i++) {
		blk = Blocks_GetBlock(BLOCKS_FG, i % w, i / w);
		lparse_set_data(lp, e, i, 1, &blk);
	}
	e = lparse_make_entry(lp, "BLK_LAYER1", lparse_u16, numblks);
	for (i = 0; i < numblks; i++) {
		blk = Blocks_GetBlock(BLOCKS_BG, i % w, i / w);
		lparse_set_data(lp, e, i, 1, &blk);
	}
	e = lparse_make_entry(lp, "BLK_LIGHT", lparse_u16, numblks);
	for (i = 0; i < numblks; i++) {
		blk = Blocks_GetBlockAmbientLight(i % w, i / w);
		lparse_set_data(lp, e, i, 1, &blk);
	}

	e = lparse_make_entry(lp, "BP_FLAGS", lparse_u32, numprops);
	for (i = 0; i < numprops; i++) lparse_set_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->flags);
	e = lparse_make_entry(lp, "BP_TRANS", lparse_i32, numprops);
	for (i = 0; i < numprops; i++) lparse_set_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->transparency);
	e = lparse_make_entry(lp, "BP_DMG_TYPE", lparse_i32, numprops);
	for (i = 0; i < numprops; i++) lparse_set_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->dmg_type);
	e = lparse_make_entry(lp, "BP_DMG", lparse_i32, numprops);
	for (i = 0; i < numprops; i++) lparse_set_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->dmg);
	e = lparse_make_entry(lp, "BP_ANIM_SPEED", lparse_i32, numprops);
	for (i = 0; i < numprops; i++) lparse_set_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->anim_speed);
	e = lparse_make_entry(lp, "BP_NUM_FRAMES", lparse_i32, numprops);
	for (i = 0; i < numprops; i++) lparse_set_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->num_frames);
	e = lparse_make_entry(lp, "BP_FRAMESX", lparse_i32, numprops*BLOCKS_MAX_FRAMES);
	for (i = 0; i < numprops; i++) lparse_set_data(lp, e, i*BLOCKS_MAX_FRAMES, BLOCKS_MAX_FRAMES, &Blocks_GetBlockProp(i)->frames_x);
	e = lparse_make_entry(lp, "BP_FRAMESY", lparse_i32, numprops*BLOCKS_MAX_FRAMES);
	for (i = 0; i < numprops; i++) lparse_set_data(lp, e, i*BLOCKS_MAX_FRAMES, BLOCKS_MAX_FRAMES, &Blocks_GetBlockProp(i)->frames_y);
	e = lparse_make_entry(lp, "BP_HEIGHTMAP", lparse_u8, numprops*BLOCK_SIZE);
	for (i = 0; i < numprops; i++) {
		for (j = 0; j < BLOCK_SIZE; j++) {
			tmp1 = (unsigned char)Blocks_GetBlockProp(i)->coll_data.heightmap[j];
			lparse_set_data(lp, e, i*BLOCK_SIZE+j, 1, &tmp1);
		}
	}
	e = lparse_make_entry(lp, "BP_HITBOX", lparse_cnm_rect, numprops);
	for (i = 0; i < numprops; i++) lparse_set_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->coll_data.hitbox);
	e = lparse_make_entry(lp, "BP_COLLTYPE", lparse_i32, numprops);
	for (i = 0; i < numprops; i++) lparse_set_data(lp, e, i, 1, &Blocks_GetBlockProp(i)->coll_type);

	lparse_close(lp);
}
void Serial_LoadBlocksLevelPreview(const char *cnmb_file, CNM_RECT *r, int *dif)
{
	LParse *lp;
	LParseEntry *e;

	if (Serial_GetLParser(cnmb_file, &lp) == -1) Serial_LoadBlocksLevelPreview_Old(cnmb_file, r, dif); if (lp == NULL) return;
	e = lparse_get_entry(lp, "BP_FRAMESX");
	lparse_get_data(lp, e, 256*BLOCKS_MAX_FRAMES, 1, &r->x);
	r->x *= BLOCK_SIZE;
	e = lparse_get_entry(lp, "BP_FRAMESY");
	lparse_get_data(lp, e, 256*BLOCKS_MAX_FRAMES, 1, &r->y);
	r->y *= BLOCK_SIZE;
	r->w = BLOCK_SIZE * 3;
	r->h = BLOCK_SIZE * 2;
	e = lparse_get_entry(lp, "BP_DMG");
	lparse_get_data(lp, e, 256, 1, dif);
	lparse_close(lp);
}
void Serial_LoadSpawnersLevelName(const char *cnms_file, char *name_buf)
{
	LParse *lp;
	LParseEntry *e;
	char buf[UTIL_MAX_TEXT_WIDTH + 1];

	if (Serial_GetLParser(cnms_file, &lp) == -1) Serial_LoadSpawnersLevelName_Old(cnms_file, name_buf); if (lp == NULL) return;

	memset(buf, 0, sizeof(buf));
	e = lparse_get_entry(lp, "ENDINGTEXT");
	lparse_get_data(lp, e, 47*ENDING_TEXT_MAX_WIDTH, ENDING_TEXT_MAX_WIDTH, buf);
	buf[UTIL_MAX_TEXT_WIDTH - 1] = '\0';
	char *lb;
	while ((lb = strchr(buf, '\\')) != NULL)
		*lb = ' ';
	strcpy(name_buf, buf);
	lparse_close(lp);
}
void Serial_LoadSpawners(const char *cnms_file)
{
	LParse *lp;
	LParseEntry *e, *ee[7];
	TELEPORT_INFO *ti;
	char ending_text_line[ENDING_TEXT_MAX_WIDTH];
	int num_spawners, i;
	SPAWNER *created;
	SPAWNER tmpspawner;
	unsigned char tmp2;
	float tmp1, tmp3;

	if (cnms_file[strlen(cnms_file) - 1] != 's')
		return;

	if (Serial_GetLParser(cnms_file, &lp) == -1) Serial_LoadSpawners_Old(cnms_file); if (lp == NULL) return;

	PlayerSpawn_ClearAllSpawns();
	ee[0] = lparse_get_entry(lp, "PLAYERSPAWNX");
	ee[1] = lparse_get_entry(lp, "PLAYERSPAWNY");
	for (i = 0; i < (int)lparse_get_size(ee[0]); i++) {
		lparse_get_data(lp, ee[0], i, 1, &tmp1);
		lparse_get_data(lp, ee[1], i, 1, &tmp3);
		PlayerSpawns_SetSpawnLoc(i, tmp1, tmp3);
	}

	ee[0] = lparse_get_entry(lp, "TI_NAME");
	ee[1] = lparse_get_entry(lp, "TI_COST");
	ee[2] = lparse_get_entry(lp, "TI_POS");
	ee[3] = lparse_get_entry(lp, "TI_ALLOCED");
	for (i = 0; i < TELEPORT_INFOS_MAX_TELEPORTS; i++) {
		ti = TeleportInfos_GetTeleport(i);
		lparse_get_data(lp, ee[0], i*TELEPORT_NAMESZ, TELEPORT_NAMESZ, ti->name);
		lparse_get_data(lp, ee[1], i, 1, &ti->cost);
		lparse_get_data(lp, ee[2], i*2, 1, &ti->x);
		lparse_get_data(lp, ee[2], i*2+1, 1, &ti->y);
		lparse_get_data(lp, ee[3], i, 1, &tmp2);
		ti->allocated = (int)tmp2;
	}

	EndingText_ClearAllLines();
	e = lparse_get_entry(lp, "ENDINGTEXT");
	for (i = 0; i < ENDING_TEXT_MAX_LINES; i++) {
		lparse_get_data(lp, e, i*ENDING_TEXT_MAX_WIDTH, ENDING_TEXT_MAX_WIDTH, ending_text_line);
		EndingText_SetLine(i, ending_text_line);
	}

	e = lparse_get_entry(lp, "NUM_SPAWNERS");
	lparse_get_data(lp, e, 0, 1, &num_spawners);

	ee[0] = lparse_get_entry(lp, "SP_POS");
	ee[1] = lparse_get_entry(lp, "SP_TYPE");
	ee[2] = lparse_get_entry(lp, "SP_DURATION");
	ee[3] = lparse_get_entry(lp, "SP_MAX");
	ee[4] = lparse_get_entry(lp, "SP_CI");
	ee[5] = lparse_get_entry(lp, "SP_CF");
	ee[6] = lparse_get_entry(lp, "SP_DITEM");
	Spawners_UnloadSpawners();
	for (i = 0; i < num_spawners; i++) {
		lparse_get_data(lp, ee[0], i*2, 1, &tmpspawner.x);
		lparse_get_data(lp, ee[0], i*2+1, 1, &tmpspawner.y);
		lparse_get_data(lp, ee[1], i, 1, &tmpspawner.wobj_type);
		lparse_get_data(lp, ee[2], i, 1, &tmpspawner.duration);
		lparse_get_data(lp, ee[3], i, 1, &tmpspawner.max);
		created = Spawners_CreateSpawner(tmpspawner.x, tmpspawner.y, tmpspawner.wobj_type, tmpspawner.duration, tmpspawner.max);
		lparse_get_data(lp, ee[4], i, 1, &created->custom_int);
		lparse_get_data(lp, ee[5], i, 1, &created->custom_float);
		lparse_get_data(lp, ee[6], i, 1, &created->dropped_item);
	}
	FileSystem_RegisterSpawners(cnms_file);
	
	lparse_close(lp);
}
void Serial_SaveSpawners(const char *cnms_file)
{
	LParse *lp;
	LParseEntry *e, *ee[7];
	TELEPORT_INFO *ti;
	int num_spawners, i;
	SPAWNER *iter;
	unsigned char tmp2;
	float tmp1;

	if (cnms_file[strlen(cnms_file) - 1] != 's')
		return;

	if (!Serial_GetWriteLParser(cnms_file, &lp)) return;
	e = lparse_make_entry(lp, "PLAYERSPAWNX", lparse_float, PLAYER_SPAWNS_MAX * PLAYER_SPAWN_TYPE_MAX);
	for (i = 0; i < (int)lparse_get_size(e); i++) {
		tmp1 = PlayerSpawn_GetSpawnLocX(i);
		lparse_set_data(lp, e, i, 1, &tmp1);
	}
	e = lparse_make_entry(lp, "PLAYERSPAWNY", lparse_float, PLAYER_SPAWNS_MAX * PLAYER_SPAWN_TYPE_MAX);
	for (i = 0; i < (int)lparse_get_size(e); i++) {
		tmp1 = PlayerSpawn_GetSpawnLocY(i);
		lparse_set_data(lp, e, i, 1, &tmp1);
	}

	ee[0] = lparse_make_entry(lp, "TI_NAME", lparse_u8, TELEPORT_INFOS_MAX_TELEPORTS * TELEPORT_NAMESZ);
	ee[1] = lparse_make_entry(lp, "TI_COST", lparse_i32, TELEPORT_INFOS_MAX_TELEPORTS);
	ee[2] = lparse_make_entry(lp, "TI_POS", lparse_float, TELEPORT_INFOS_MAX_TELEPORTS * 2);
	ee[3] = lparse_make_entry(lp, "TI_ALLOCED", lparse_u8, TELEPORT_INFOS_MAX_TELEPORTS);
	for (i = 0; i < TELEPORT_INFOS_MAX_TELEPORTS; i++) {
		ti = TeleportInfos_GetTeleport(i);
		lparse_set_data(lp, ee[0], i*TELEPORT_NAMESZ, TELEPORT_NAMESZ, ti->name);
		lparse_set_data(lp, ee[1], i, 1, &ti->cost);
		lparse_set_data(lp, ee[2], i*2, 1, &ti->x);
		lparse_set_data(lp, ee[2], i*2+1, 1, &ti->y);
		tmp2 = (unsigned char)ti->allocated;
		lparse_set_data(lp, ee[3], i, 1, &tmp2);
	}

	e = lparse_make_entry(lp, "ENDINGTEXT", lparse_u8, ENDING_TEXT_MAX_LINES * ENDING_TEXT_MAX_WIDTH);
	for (i = 0; i < ENDING_TEXT_MAX_LINES; i++) lparse_set_data(lp, e, i*ENDING_TEXT_MAX_WIDTH, ENDING_TEXT_MAX_WIDTH, EndingText_GetLine(i));

	num_spawners = 0;
	iter = Spawners_Iterate(NULL);
	while (iter != NULL)
	{
		num_spawners++;
		iter = Spawners_Iterate(iter);
	}

	ee[0] = lparse_make_entry(lp, "SP_POS", lparse_float, num_spawners*2);
	ee[1] = lparse_make_entry(lp, "SP_TYPE", lparse_i32, num_spawners);
	ee[2] = lparse_make_entry(lp, "SP_DURATION", lparse_i32, num_spawners);
	ee[3] = lparse_make_entry(lp, "SP_MAX", lparse_i32, num_spawners);
	ee[4] = lparse_make_entry(lp, "SP_CI", lparse_i32, num_spawners);
	ee[5] = lparse_make_entry(lp, "SP_CF", lparse_float, num_spawners);
	ee[6] = lparse_make_entry(lp, "SP_DITEM", lparse_u32, num_spawners);
	i = 0;
	iter = Spawners_Iterate(NULL);
	while (iter != NULL)
	{
		lparse_set_data(lp, ee[0], i*2, 1, &iter->x);
		lparse_set_data(lp, ee[0], i*2+1, 1, &iter->y);
		lparse_set_data(lp, ee[1], i, 1, &iter->wobj_type);
		lparse_set_data(lp, ee[2], i, 1, &iter->duration);
		lparse_set_data(lp, ee[3], i, 1, &iter->max);
		lparse_set_data(lp, ee[4], i, 1, &iter->custom_int);
		lparse_set_data(lp, ee[5], i, 1, &iter->custom_float);
		lparse_set_data(lp, ee[6], i, 1, &iter->dropped_item);
		iter = Spawners_Iterate(iter);
		i++;
	}

	e = lparse_make_entry(lp, "NUM_SPAWNERS", lparse_i32, 1);
	lparse_set_data(lp, e, 0, 1, &num_spawners);

	lparse_close(lp);
}


static int Serial_GetLParser(const char *cnm_file, LParse **lp) {
	FILE *fp;
	*lp = NULL;
	fp = fopen(cnm_file, "rb");
	if (fp == NULL) {
		Console_Print("Cannot load the level file: \"%s\"!", cnm_file);
		return 0;
	}
	*lp = lparse_open_from_file(fp, lparse_read);
	if (*lp == NULL) {
		Console_Print("Level file \"%s\" uses outdated data format!", cnm_file);
		return -1;
	}
	return 1;
}
static int Serial_GetWriteLParser(const char *cnm_file, LParse **lp) {
	FILE *fp;
	*lp = NULL;
	fp = fopen(cnm_file, "wb");
	if (fp == NULL) {
		Console_Print("Cannot write the level file: \"%s\"!", cnm_file);
		return 0;
	}
	*lp = lparse_open_from_file(fp, lparse_write);
	return 1;
}
