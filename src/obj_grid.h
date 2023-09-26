#ifndef _obj_grid_h_
#define _obj_grid_h_
#include "utility.h"

#define OBJGRID_SIZE 128.0f

typedef struct _OBJGRID_CHUNK OBJGRID_CHUNK;
typedef struct _OBJGRID OBJGRID;
typedef struct _OBJGRID_OBJECT
{
	int uuid;
	float x, y;
	OBJGRID_CHUNK *chunk;
	struct _OBJGRID_OBJECT *next, *last;
} OBJGRID_OBJECT;
typedef OBJGRID_OBJECT *OBJGRID_ITER;

OBJGRID *ObjGrid_Create(int w, int h);
void ObjGrid_Destroy(OBJGRID *grid);
void ObjGrid_Clear(OBJGRID *grid);
void ObjGrid_FastClear(OBJGRID *grid);
void ObjGrid_AddObject(OBJGRID *grid, OBJGRID_OBJECT *obj);
void ObjGrid_MoveObject(OBJGRID *grid, OBJGRID_OBJECT *obj, float x, float y);
void ObjGrid_RemoveObject(OBJGRID *grid, OBJGRID_OBJECT *obj);
void ObjGrid_MakeIter(OBJGRID *grid, int x, int y, OBJGRID_ITER *iter);
int ObjGrid_AdvanceIter(OBJGRID_ITER *iter);

#endif