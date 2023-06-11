#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "renderer.h"
#include "blocks.h"
#include "game.h"

typedef struct _BLOCKS_WORLD
{
	BLOCK *blocks[2];
	unsigned char *light[2];
	int w, h;
} BLOCKS_WORLD;

static BLOCK_PROPS *blocks_props = NULL;
static int blocks_props_count = 0;
static BLOCKS_WORLD blocks_world = {{NULL, NULL}, {NULL, NULL}, 0, 0};

static int Blocks_IsBlockSolid(int x, int y);
static int Blocks_IsBlockDamageable(int x, int y);

void Blocks_Unload(void)
{
	free(blocks_props);
	blocks_props = NULL;
	blocks_props_count = 0;
	Blocks_SetWorldSize(0, 0);
}

void Blocks_GenBlockProps(int count)
{
	int i;

	if (blocks_props != NULL)
		free(blocks_props);
	blocks_props = malloc(count * sizeof(BLOCK_PROPS));
	blocks_props_count = count;

	for (i = 0; i < count; i++)
	{
		blocks_props[i].flags = 0;
		blocks_props[i].flags |= i ? BLOCK_FLAG_SOLID : 0;
		blocks_props[i].transparency = 0;
		blocks_props[i].dmg_type = BLOCK_DMG_TYPE_NONE;
		blocks_props[i].dmg = 0;
		blocks_props[i].anim_speed = 1;
		blocks_props[i].num_frames = 1;
		blocks_props[i].frames_x[0] = 0;
		blocks_props[i].frames_y[0] = 0;
		blocks_props[i].coll_type = BLOCKS_COLL_BOX;
		blocks_props[i].coll_data.hitbox.x = 0;
		blocks_props[i].coll_data.hitbox.y = 0;
		blocks_props[i].coll_data.hitbox.w = BLOCK_SIZE;
		blocks_props[i].coll_data.hitbox.h = BLOCK_SIZE;
	}
}
void Blocks_LoadBlockProps(const BLOCK_PROPS *props, int count)
{
	blocks_props = malloc(count * sizeof(BLOCK_PROPS));
	blocks_props_count = count;
	if (props != NULL)
	{
		memcpy(blocks_props, props, count * sizeof(BLOCK_PROPS));
	}
	else
	{
		memset(blocks_props, 0, count * sizeof(BLOCK_PROPS));
	}
}
BLOCK_PROPS *Blocks_GetBlockProp(BLOCK block_id)
{
	if (block_id < blocks_props_count)
		return blocks_props + block_id;
	else
		return NULL;
}
int Blocks_GetBlockPropsCount(void)
{
	return blocks_props_count;
}

void Blocks_SetWorldSize(int w, int h)
{
	int x, y;

	/* Free the blocks */
	free(blocks_world.blocks[0]);
	free(blocks_world.blocks[1]);
	free(blocks_world.light[0]);
	free(blocks_world.light[1]);
	memset(&blocks_world, 0, sizeof(BLOCKS_WORLD));

	if (w && h)
	{
		/* Create a new layer */
		blocks_world.w = w;
		blocks_world.h = h;
		blocks_world.blocks[0] = malloc(w * (size_t)h * sizeof(BLOCK));
		blocks_world.light[0] = malloc(w * (size_t)h);
		memset(blocks_world.blocks[0], 0, w * (size_t)h * sizeof(BLOCK));
		blocks_world.blocks[1] = malloc(w * (size_t)h * sizeof(BLOCK));
		blocks_world.light[1] = malloc(w * (size_t)h);
		memset(blocks_world.blocks[1], 0, w * (size_t)h * sizeof(BLOCK));
		for (y = 0; y < h; y++)
		{
			for (x = 0; x < w; x++)
			{
				Blocks_SetBlockAmbientLight(x, y, RENDERER_LIGHT);
				Blocks_SetBlockDirectLight(x, y, 0);
			}
		}
	}
}
void Blocks_ResizeWorld(int w, int h) {
	BLOCKS_WORLD world;
	int x, y, mx, my;

	memset(&world, 0, sizeof(world));
	world.w = w;
	world.h = h;
	if (w && h)
	{
		/* Create a new layer */
		world.blocks[0] = malloc(w * (size_t)h * sizeof(BLOCK));
		world.light[0] = malloc(w * (size_t)h);
		world.blocks[1] = malloc(w * (size_t)h * sizeof(BLOCK));
		world.light[1] = malloc(w * (size_t)h);
		mx = CNM_MIN(w, blocks_world.w);
		my = CNM_MIN(h, blocks_world.h);
		memset(world.blocks[0], 0, w * (size_t)h);
		memset(world.blocks[1], 0, w * (size_t)h);
		for (y = 0; y < h; y++)
		{
			for (x = 0; x < w; x++)
			{
				world.light[0][y * w + x] = RENDERER_LIGHT;
				world.light[1][y * w + x] = 0;
			}
		}
		for (y = 0; y < my; y++)
		{
			memcpy(world.blocks[0] + y*w, blocks_world.blocks[0] + y*blocks_world.w, mx*sizeof(BLOCK));
			memcpy(world.blocks[1] + y*w, blocks_world.blocks[1] + y*blocks_world.w, mx*sizeof(BLOCK));
			memcpy(world.light[0] + y*w, blocks_world.light[0] + y*blocks_world.w, mx);
			memcpy(world.light[1] + y*w, blocks_world.light[1] + y*blocks_world.w, mx);
		}
	}

	/* Free the blocks */
	free(blocks_world.blocks[0]);
	free(blocks_world.blocks[1]);
	free(blocks_world.light[0]);
	free(blocks_world.light[1]);
	memcpy(&blocks_world, &world, sizeof(world));
}
int Blocks_GetWorldWidth(void)
{
	return blocks_world.w;
}
int Blocks_GetWorldHeight(void)
{
	return blocks_world.h;
}
void Blocks_SetBlock(int layer, int x, int y, BLOCK block)
{
	if (x > -1 && x < blocks_world.w && y > -1 && y < blocks_world.h)
		blocks_world.blocks[layer][y * blocks_world.w + x] = block;
}
static int Min(int x, int min)
{
	return (x < min) ? min : x;
}
static int Max(int x, int max)
{
	return (x > max) ? max : x;
}
BLOCK Blocks_GetBlock(int layer, int x, int y)
{
	int clamped_x = Min(Max(x, blocks_world.w - 1), 0);
	int clamped_y = Min(Max(y, blocks_world.h - 1), 0);
	return blocks_world.blocks[layer][clamped_y * blocks_world.w + clamped_x];
}
void Blocks_SetBlockAmbientLight(int x, int y, int light)
{
	if (x > -1 && x < blocks_world.w && y > -1 && y < blocks_world.h)
		blocks_world.light[0][y * blocks_world.w + x] = light;
}
int Blocks_GetBlockAmbientLight(int x, int y)
{
	int clamped_x = Min(Max(x, blocks_world.w - 1), 0);
	int clamped_y = Min(Max(y, blocks_world.h - 1), 0);
	return blocks_world.light[0][clamped_y * blocks_world.w + clamped_x];
}
void Blocks_SetBlockDirectLight(int x, int y, int light)
{
	if (x > -1 && x < blocks_world.w && y > -1 && y < blocks_world.h)
		blocks_world.light[1][y * blocks_world.w + x] = light;
}
int Blocks_GetBlockDirectLight(int x, int y)
{
	int clamped_x = Min(Max(x, blocks_world.w - 1), 0);
	int clamped_y = Min(Max(y, blocks_world.h - 1), 0);
	return blocks_world.light[1][clamped_y * blocks_world.w + clamped_x];
}
int Blocks_GetCalculatedBlockLight(int x, int y)
{
	int calculated;
	if (Blocks_GetBlockAmbientLight(x, y) >= RENDERER_LIGHT)
	{
		calculated = Blocks_GetBlockAmbientLight(x, y);
		if (Blocks_GetBlockAmbientLight(x, y) == RENDERER_LIGHT)
		{
			calculated = RENDERER_LIGHT - (Blocks_GetBlockDirectLight(x, y) >= 2 ? 1 : 0);
		}
		if (Blocks_GetBlockAmbientLight(x, y) > RENDERER_LIGHT)
		{
			calculated = Blocks_GetBlockAmbientLight(x, y) - Blocks_GetBlockDirectLight(x, y);
			if (calculated < 0)
				calculated = 0;
		}
		return calculated;
	}
	else
	{
		return Blocks_GetBlockAmbientLight(x, y);
	}
}
static int Blocks_IsCollidingWithBlock(const CNM_BOX *b, int layer, int x, int y)
{
	BLOCK_PROPS *bp = Blocks_GetBlockProp(Blocks_GetBlock(layer, x, y));

	if (!(bp->flags & BLOCK_FLAG_SOLID))
		return CNM_FALSE;

	if (bp->coll_type == BLOCKS_COLL_BOX)
	{
		CNM_BOX block;
		block.x = (float)(x * BLOCK_SIZE + bp->coll_data.hitbox.x);
		block.y = (float)(y * BLOCK_SIZE + bp->coll_data.hitbox.y);
		block.w = (float)bp->coll_data.hitbox.w;
		block.h = (float)bp->coll_data.hitbox.h;
		return Util_AABBCollision(b, &block);
	}
	else
	{
		int start = (int)floorf(b->x) - (x * BLOCK_SIZE);
		if (start < 0)
			start = 0;
		int end = (int)ceilf(b->x + b->w) - (x * BLOCK_SIZE);
		if (end > BLOCK_SIZE - 1)
			end = BLOCK_SIZE - 1;
		for (int i = start; i < end; i++)
		{
			int slice_size = bp->coll_data.heightmap[i];
			if (slice_size)
			{
				CNM_BOX slice;
				slice.x = (float)(x * BLOCK_SIZE + i);
				slice.y = (float)((BLOCK_SIZE - slice_size) + y * BLOCK_SIZE);
				slice.w = 1.0f;
				slice.h = (float)slice_size;
				if (Util_AABBCollision(&slice, b))
					return CNM_TRUE;
			}
		}

		return CNM_FALSE;
	}

	return CNM_FALSE;
}
int Blocks_IsCollidingWithSolid(const CNM_BOX *b)
{
	int x, y;
	for (y = (int)b->y / BLOCK_SIZE - 1; y < (int)(b->y + b->h) / BLOCK_SIZE + 1; y++)
	{
		for (x = (int)b->x / BLOCK_SIZE - 1; x < (int)(b->x + b->w) / BLOCK_SIZE + 1; x++)
		{
			if (Blocks_IsCollidingWithBlock(b, BLOCKS_FG, x, y) || Blocks_IsCollidingWithBlock(b, BLOCKS_BG, x, y))
				return CNM_TRUE;
		}
	}

	return CNM_FALSE;
}
BLOCK_PROPS *Blocks_IsCollidingWithDamage(const CNM_BOX *b)
{
	CNM_BOX block;
	int x, y;
	for (y = (int)b->y / BLOCK_SIZE - 1; y < (int)(b->y + b->h) / BLOCK_SIZE + 1; y++)
	{
		for (x = (int)b->x / BLOCK_SIZE - 1; x < (int)(b->x + b->w) / BLOCK_SIZE + 1; x++)
		{
			if (Blocks_IsBlockDamageable(x, y))
			{
				block.x = (float)(x * BLOCK_SIZE);
				block.y = (float)(y * BLOCK_SIZE);
				block.w = (float)BLOCK_SIZE;
				block.h = (float)BLOCK_SIZE;
				if (Util_AABBCollision(b, &block))
				{
					if (Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_FG, x, y))->dmg_type != BLOCK_DMG_TYPE_NONE)
						return Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_FG, x, y));
					else
						return Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_BG, x, y));
				}
			}
		}
	}

	return NULL;
}
static void Blocks_ResolveCollisionWithBlock(CNM_BOX *b,
											 int layer,
											 int bx, int by,
											 int *resolved_in_x,
											 int *resolved_in_y)
{
	BLOCK_PROPS *bp = Blocks_GetBlockProp(Blocks_GetBlock(layer, bx, by));
	if (!(bp->flags & BLOCK_FLAG_SOLID))
		return;
	if (bp->coll_type == BLOCKS_COLL_BOX)
	{
		CNM_BOX block;
		Util_SetBox
		(
			&block,
			(float)(bx * BLOCK_SIZE + bp->coll_data.hitbox.x),
			(float)(by * BLOCK_SIZE + bp->coll_data.hitbox.y),
			(float)bp->coll_data.hitbox.w,
			(float)bp->coll_data.hitbox.h
		);
		Util_ResolveAABBCollision(b, &block, resolved_in_x, resolved_in_y);
	}
	else if (bp->coll_type == BLOCKS_COLL_HEIGHT)
	{
		int start = (int)floorf(b->x) - (bx * BLOCK_SIZE);
		if (start < 0)
			start = 0;
		int end = (int)ceilf(b->x + b->w) - (bx * BLOCK_SIZE);
		if (end > BLOCK_SIZE - 1)
			end = BLOCK_SIZE - 1;
		for (int i = start; i < end; i++)
		{
			int slice_size = bp->coll_data.heightmap[i];
			if (slice_size)
			{
				CNM_BOX slice;
				slice.x = (float)(bx * BLOCK_SIZE + i);
				slice.y = (float)((BLOCK_SIZE - slice_size) + by * BLOCK_SIZE);
				slice.w = 1.0f;
				slice.h = (float)slice_size;
				if (Util_AABBCollision(&slice, b))
				{
					b->y = slice.y - b->h;
				}
			}
		}
	}
}

typedef struct _BLOCKS_COLLISON_INFO
{
	int x, y;
	float dist;
} BLOCKS_COLLISION_INFO;
int Blocks_ResolveCollisionSortFunc(const void *a, const void *b)
{
	float da = ((BLOCKS_COLLISION_INFO *)a)->dist;
	float db = ((BLOCKS_COLLISION_INFO *)b)->dist;
	if (da < db) return -1;
	if (da > db) return 1;
	return 0;
}

void Blocks_ResolveCollision(CNM_BOX *b, int *resolved_in_x, int *resolved_in_y)
{
	int x, y;
	float cx, cy, dx, dy;
	BLOCKS_COLLISION_INFO blocks[128];
	int num_blocks = 0;
	*resolved_in_x = CNM_FALSE;
	*resolved_in_y = CNM_FALSE;
	if (!Blocks_IsCollidingWithSolid(b))
		return;
	// Order the blocks from farthest to closest
	cx = b->x + b->w / 2.0f;
	cy = b->y + b->h / 2.0f;
	for (x = (int)floorf(b->x / (float)BLOCK_SIZE); x < (int)ceilf((b->x + b->w) / (float)BLOCK_SIZE); x++)
	{
		for (y = (int)floorf(b->y / (float)BLOCK_SIZE); y < (int)ceilf((b->y + b->h) / (float)BLOCK_SIZE); y++)
		{
			if (Blocks_IsBlockSolid(x, y) && num_blocks < 128)
			{
				blocks[num_blocks].x = x;
				blocks[num_blocks].y = y;
				dx = cx - (float)(x * BLOCK_SIZE + BLOCK_SIZE / 2);
				dy = cy - (float)(y * BLOCK_SIZE + BLOCK_SIZE / 2);
				blocks[num_blocks].dist = sqrtf(dx*dx + dy*dy);
				num_blocks++;
			}
		}
	}
	qsort(blocks, num_blocks, sizeof(BLOCKS_COLLISION_INFO), Blocks_ResolveCollisionSortFunc);

	for (x = 0; x < num_blocks; x++)
	{
		Blocks_ResolveCollisionWithBlock(b, BLOCKS_FG, blocks[x].x, blocks[x].y, resolved_in_x, resolved_in_y);
		Blocks_ResolveCollisionWithBlock(b, BLOCKS_BG, blocks[x].x, blocks[x].y, resolved_in_x, resolved_in_y);
	}
}
void Blocks_StickBoxToGround(CNM_BOX *b)
{
	CNM_BOX c;
	memcpy(&c, b, sizeof(CNM_BOX));

	c.y = floorf(c.y);
	c.y += 16.0f;
	if (Blocks_IsCollidingWithSolid(&c))
	{
		c.y = b->y;
		for (int y = 0; y < 16; y++, c.y += 1.0f)
		{
			if (Blocks_IsCollidingWithSolid(&c))
			{
				b->y = c.y - 1.0f;
				return;
			}
		}
	}
}

int Blocks_IsBlockSolid(int x, int y)
{
	return Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_FG, x, y))->flags & BLOCK_FLAG_SOLID ||
		Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_BG, x, y))->flags & BLOCK_FLAG_SOLID;
}
int Blocks_IsBlockDamageable(int x, int y)
{
	return Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_FG, x, y))->dmg_type != BLOCK_DMG_TYPE_NONE ||
		Blocks_GetBlockProp(Blocks_GetBlock(BLOCKS_BG, x, y))->dmg_type != BLOCK_DMG_TYPE_NONE;
}

void Blocks_GetBlockDrawRect(BLOCK block, CNM_RECT *src)
{
	int frame;
	if ((int)block < blocks_props_count)
	{
		frame = (Game_GetFrame() / blocks_props[block].anim_speed) % blocks_props[block].num_frames;
		src->x = blocks_props[block].frames_x[frame] * BLOCK_SIZE;
		src->y = blocks_props[block].frames_y[frame] * BLOCK_SIZE;
		src->w = BLOCK_SIZE;
		src->h = BLOCK_SIZE;
	}
	else
	{
		memset(src, 0, sizeof(CNM_RECT));
	}
}
void Blocks_DrawBlock(int x, int y, BLOCK block, int light)
{
	CNM_RECT src;
	if (block < 0 || block > 4095)
		return;
	Blocks_GetBlockDrawRect(block, &src);
	Renderer_DrawBitmap
	(
		x,
		y,
		&src,
		Blocks_GetBlockProp(block)->transparency,
		light
	);
}
void Blocks_DrawBlocks(int layer, int camx, int camy)
{
	int x, y;
	for (y = camy / BLOCK_SIZE - 1; y < (camy + RENDERER_HEIGHT) / BLOCK_SIZE + 1; y++)
	{
		for (x = camx / BLOCK_SIZE - 1; x < (camx + RENDERER_WIDTH) / BLOCK_SIZE + 1; x++)
		{
			Blocks_DrawBlock(x * BLOCK_SIZE - camx, y * BLOCK_SIZE - camy, Blocks_GetBlock(layer, x, y), Blocks_GetCalculatedBlockLight(x, y));
		}
	}
}