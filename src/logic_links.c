#include <string.h>
#include "console.h"
#include "wobj.h"
#include "logic_links.h"

static WOBJ *links[LOGIC_LINKS_MAX];

void LogicLinks_ResetLinks(void) {
	memset(links, 0, sizeof(links));
}
WOBJ *LogicLinks_GetWobjFromLink(int link) {
	if (link >= LOGIC_LINKS_MAX) {
		Console_Print("Attempting to link above %d!", LOGIC_LINKS_MAX);
		return NULL;
	}
	return links[link];
}
void LogicLinks_AddWobjToLink(WOBJ *w, int link) {
	if (link >= LOGIC_LINKS_MAX) {
		Console_Print("Attempting to link above %d!", LOGIC_LINKS_MAX);
		return;
	}
	links[link] = w;
}
