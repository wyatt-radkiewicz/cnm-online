#ifndef _teleport_infos_h_
#define _teleport_infos_h_
#include "utility.h"

#define TELEPORT_INFOS_MAX_TELEPORTS_SERIALOLD 2048
#define TELEPORT_INFOS_MAX_TELEPORTS 512
#define TELEPORT_NAMESZ (UTIL_MAX_TEXT_WIDTH + 1)

#define SERIAL_TELEPORT_INFO_SIZE 64U

typedef struct _TELEPORT_INFO
{
	char name[TELEPORT_NAMESZ];
	int cost;
	int index;
	float x, y;
	int allocated;
} TELEPORT_INFO;

void TeleportInfos_Init(void);
void TeleportInfos_FreeLegacyLevelInfo(void);
void TeleportInfos_AllocLegacyLevelInfo(void);
void TeleportInfos_ClearAllTeleports(void);
TELEPORT_INFO *TeleportInfos_AllocTeleportInfo(void);
void TeleportInfos_FreeTeleportInfo(int index);
TELEPORT_INFO *TeleportInfos_GetTeleport(int index);

#endif