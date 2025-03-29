#ifndef _world_h_
#define _world_h_

#include "wobj.h"

#define WORLD_MODE_SINGLEPLAYER		0
#define WORLD_MODE_HOSTED_SERVER	1
#define WORLD_MODE_CLIENT			2

void World_Start(int mode);
void World_Stop(void);
void World_Update(int mode);
void World_Draw(int mode);

// Checks for objects and blocks
bool world_check_pt(const float x, const float y, const struct wobj_coll_query query);

// Restart for level select
void World_SoftRestart(void);

#endif
