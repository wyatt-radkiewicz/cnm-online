#include <string.h>
#include "wobj.h"
#include "logic_links.h"

static WOBJ *links[LOGIC_LINKS_MAX];

void LogicLinks_ResetLinks(void) {
	memset(links, 0, sizeof(links));
}
WOBJ *LogicLinks_GetWobjFromLink(int link) {
	return links[link];
}
void LogicLinks_AddWobjToLink(WOBJ *w, int link) {
	links[link] = w;
}
