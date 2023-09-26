#ifndef _logic_links_h_
#define _logic_links_h_

#define LOGIC_LINKS_MAX 256

typedef struct _WOBJ WOBJ;

void LogicLinks_ResetLinks(void);
WOBJ *LogicLinks_GetWobjFromLink(int link);
void LogicLinks_AddWobjToLink(WOBJ *w, int link);

#endif
