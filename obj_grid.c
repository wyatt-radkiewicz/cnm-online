#include <stdlib.h>
#include <string.h>
#include "obj_grid.h"

typedef struct _OBJGRID_CHUNK
{
	OBJGRID_OBJECT *first;
	int x, y;
	OBJGRID *grid;
} OBJGRID_CHUNK;
typedef struct _OBJGRID
{
	OBJGRID_CHUNK *chunks;
	int w, h;
} OBJGRID;

static OBJGRID_CHUNK *ObjGrid_GetChunk(OBJGRID *grid, int x, int y);

static int objgrid_uuid = 0;

OBJGRID *ObjGrid_Create(int w, int h)
{
	int x, y;
	OBJGRID *grid;
	OBJGRID_CHUNK *chunk;
	grid = malloc(sizeof(OBJGRID));
	grid->chunks = malloc(w * (size_t)h * sizeof(OBJGRID_CHUNK));
	grid->w = w;
	grid->h = h;
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			chunk = ObjGrid_GetChunk(grid, x, y);
			chunk->first = NULL;
			chunk->grid = grid;
			chunk->x = x;
			chunk->y = y;
		}
	}
	return grid;
}
void ObjGrid_Destroy(OBJGRID *grid)
{
	ObjGrid_Clear(grid);
	free(grid->chunks);
	free(grid);
}
void ObjGrid_Clear(OBJGRID *grid)
{
	int x, y;
	OBJGRID_OBJECT *o, *n;
	for (y = 0; y < grid->h; y++)
	{
		for (x = 0; x < grid->w; x++)
		{
			o = ObjGrid_GetChunk(grid, x, y)->first;
			while (o != NULL)
			{
				n = o->next;
				o->chunk = NULL;
				o->last = NULL;
				o->next = NULL;
				o = n;
			}
			ObjGrid_GetChunk(grid, x, y)->first = NULL;
		}
	}
}
void ObjGrid_FastClear(OBJGRID *grid)
{
	int x, y;
	for (y = 0; y < grid->h; y++)
		for (x = 0; x < grid->w; x++)
			ObjGrid_GetChunk(grid, x, y)->first = NULL;
}
void ObjGrid_AddObject(OBJGRID *grid, OBJGRID_OBJECT *obj)
{
	OBJGRID_CHUNK *chunk = ObjGrid_GetChunk(grid, (int)(obj->x / OBJGRID_SIZE), (int)(obj->y / OBJGRID_SIZE));
	obj->uuid = objgrid_uuid++;
	obj->chunk = chunk;
	if (chunk != NULL)
	{
		obj->last = NULL;
		obj->next = chunk->first;
		chunk->first = obj;
		if (obj->next != NULL)
			obj->next->last = obj;
	}
	else
	{
		obj->next = NULL;
		obj->last = NULL;
	}
}
void ObjGrid_MoveObject(OBJGRID *grid, OBJGRID_OBJECT *obj, float x, float y)
{
	int newx = (int)(x / OBJGRID_SIZE), newy = (int)(y / OBJGRID_SIZE);
	OBJGRID_CHUNK *chunk = ObjGrid_GetChunk(grid, newx, newy);
	if (obj->chunk != chunk)
	{
		if (obj->chunk == NULL && chunk != NULL)
		{
			ObjGrid_AddObject(grid, obj);
		}
		else if (obj->chunk != NULL && chunk == NULL)
		{
			ObjGrid_RemoveObject(grid, obj);
		}
		else
		{
			ObjGrid_RemoveObject(grid, obj);
			obj->x = x;
			obj->y = y;
			ObjGrid_AddObject(grid, obj);
		}
	}
	obj->x = x;
	obj->y = y;
}
void ObjGrid_RemoveObject(OBJGRID *grid, OBJGRID_OBJECT *obj)
{
	OBJGRID_CHUNK *chunk = obj->chunk;
	if (chunk != NULL)
	{
		if (obj->last != NULL)
			obj->last->next = obj->next;
		else
			chunk->first = obj->next;
		if (obj->next != NULL)
			obj->next->last = obj->last;
		obj->chunk = NULL;
	}
	memset(obj, 0, sizeof(OBJGRID_OBJECT));
}
void ObjGrid_MakeIter(OBJGRID *grid, int x, int y, OBJGRID_ITER *iter)
{
	OBJGRID_CHUNK *chunk = ObjGrid_GetChunk(grid, x, y);
	if (chunk != NULL && chunk->first != NULL)
		*iter = chunk->first;
	else
		*iter = NULL;
}
int ObjGrid_AdvanceIter(OBJGRID_ITER *iter)
{
	if (*iter != NULL)
	{
		*iter = (*iter)->next;
		return CNM_TRUE;
	}
	else
	{
		return CNM_FALSE;
	}
}

static OBJGRID_CHUNK *ObjGrid_GetChunk(OBJGRID *grid, int x, int y)
{
	if (x > -1 && x < grid->w && y > -1 && y < grid->h)
		return &grid->chunks[y * grid->w + x];
	else
		return NULL;
}