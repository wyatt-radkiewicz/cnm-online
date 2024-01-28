#include <assert.h>
#include <stdlib.h>
#include "teleport_infos.h"
#include "console.h"

static TELEPORT_INFO *infos;
static TELEPORT_INFO infosbuf[TELEPORT_INFOS_MAX_TELEPORTS];

void TeleportInfos_Init(void)
{
	int i;
	infos = infosbuf;
	for (i = 0; i < TELEPORT_INFOS_MAX_TELEPORTS; i++)
	{
		infos[i].index = i;
		TeleportInfos_FreeTeleportInfo(i);
	}
}
void TeleportInfos_FreeLegacyLevelInfo(void) {

	if (infos != infosbuf) {
		free(infos);
	}
	infos = infosbuf;
}
void TeleportInfos_AllocLegacyLevelInfo(void) {
	int i;
	if (infos != infosbuf) return;
	assert(0 && "FIXME @ TeleportInfos_AllocLegacyLevelInfo");
	//infos = m_alloc(sizeof(TELEPORT_INFO)*TELEPORT_INFOS_MAX_TELEPORTS_SERIALOLD);
	for (i = 0; i < TELEPORT_INFOS_MAX_TELEPORTS_SERIALOLD; i++)
	{
		infos[i].index = i;
		TeleportInfos_FreeTeleportInfo(i);
	}
}
void TeleportInfos_ClearAllTeleports(void)
{
	int i;
	for (i = 0; i < TELEPORT_INFOS_MAX_TELEPORTS; i++)
	{
		TeleportInfos_FreeTeleportInfo(i);
	}
}
TELEPORT_INFO *TeleportInfos_AllocTeleportInfo(void)
{
	int i;
	for (i = 0; i < TELEPORT_INFOS_MAX_TELEPORTS; i++)
	{
		if (!infos[i].allocated)
		{
			infos[i].allocated = CNM_TRUE;
			return infos + i;
		}
	}

	Console_Print("ALL %d TELEPORTERS HAVE BEEN USED UP!!! WHAT THE HELL ARE YOU DOING IDIOT?!?!?", TELEPORT_INFOS_MAX_TELEPORTS);
	return NULL;
}
void TeleportInfos_FreeTeleportInfo(int index)
{
	infos[index].allocated = CNM_FALSE;
}
TELEPORT_INFO *TeleportInfos_GetTeleport(int index)
{
	return infos + index;
}
