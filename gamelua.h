#ifndef _gamelua_h_
#define _gamelua_h_

#define GAMELUA_LINE_SIZE 256

void AutorunLua_ClearPrgm(void);
void AutorunLua_AddLine(const char *line);
void AutorunLua_Run(void);

int NormalID_To_LuaID(int luaid);
int LuaID_To_NormalID(int normalid);

typedef struct _WOBJ WOBJ;
void WobjLua_Create(WOBJ *wobj);
void WobjLua_Update(WOBJ *wobj);
void WobjLua_Draw(WOBJ *wobj, int camx, int camy);

#endif
