#ifndef _blocks_h_
#define _blocks_h_
#include "utility.h"

#define BLOCK_SIZE 32

#define BLOCKS_FG 1
#define BLOCKS_BG 0
#define BLOCKS_DUMMY_EFFECTS 2
#define BLOCKS_DUMMY_EFFECTS_EX 3

#define BLOCKS_COLL_BOX		0
#define BLOCKS_COLL_HEIGHT	1

#define BLOCKS_MAX_FRAMES		32
#define BLOCK_FLAG_SOLID		(1 << 0)
#define BLOCK_DMG_TYPE_NONE		0
#define BLOCK_DMG_TYPE_LAVA		1
#define BLOCK_DMG_TYPE_SPIKES	2
#define BLOCK_DMG_TYPE_QUICKSAND 3
#define BLOCK_DMG_TYPE_ICE 4

#define SERIAL_BLOCK_PROPS_SIZE 412U

typedef struct _BLOCK_PROPS
{
	unsigned int flags;
	int transparency;
	int dmg_type;
	int dmg;
	int anim_speed;
	int num_frames;
	int frames_x[BLOCKS_MAX_FRAMES], frames_y[BLOCKS_MAX_FRAMES];
	
	union
	{
		int heightmap[BLOCK_SIZE];
		CNM_RECT hitbox;
	} coll_data;
	int coll_type;
} BLOCK_PROPS;
typedef unsigned short BLOCK;

void Blocks_Unload(void);

void Blocks_GenBlockProps(int count);
void Blocks_LoadBlockProps(const BLOCK_PROPS *props, int count);
BLOCK_PROPS *Blocks_GetBlockProp(BLOCK block_id);
int Blocks_GetBlockPropsCount(void);

void Blocks_SetWorldSize(int w, int h);
void Blocks_ResizeWorld(int w, int h);
int Blocks_GetWorldWidth(void);
int Blocks_GetWorldHeight(void);
void Blocks_SetBlock(int layer, int x, int y, BLOCK block);
BLOCK Blocks_GetBlock(int layer, int x, int y);
void Blocks_SetBlockAmbientLight(int x, int y, int light);
int Blocks_GetBlockAmbientLight(int x, int y);
void Blocks_SetBlockDirectLight(int x, int y, int light);
int Blocks_GetBlockDirectLight(int x, int y);
int Blocks_GetCalculatedBlockLight(int x, int y);
int Blocks_IsCollidingWithSolid(const CNM_BOX *b);
BLOCK_PROPS *Blocks_IsCollidingWithDamage(const CNM_BOX *b);
void Blocks_ResolveCollision(CNM_BOX *b, int *resolved_in_x, int *resolved_in_y);
void Blocks_StickBoxToGround(CNM_BOX *b);

void Blocks_GetBlockDrawRect(BLOCK block, CNM_RECT *src);
void Blocks_DrawBlock(int x, int y, BLOCK block, int light);
void Blocks_DrawBlocks(int layer, int camx, int camy);

#endif
