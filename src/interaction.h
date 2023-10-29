#ifndef _interaction_h_
#define _interaction_h_

#define INTERACTION_MODE_SINGLEPLAYER 0
#define INTERACTION_MODE_CLIENT 1
#define INTERACTION_MODE_HOSTED_SERVER 2
#define INTERACTION_MODE_DEDICATED_SERVER 3

typedef struct _WOBJ WOBJ;
typedef void(*INTERACTION_TAKE_DAMAGE)(WOBJ *victim, WOBJ *inflictor);

void Interaction_Init(void);
void Interaction_SetMode(int mode);
void Interaction_SetClientPlayerWObj(WOBJ *player);
WOBJ *Interaction_GetVictim(WOBJ *inflictor, int flags);
void Interaction_DamageWobj(WOBJ *inflictor, WOBJ *victim);
void Interaction_DamageFoundWobjs(WOBJ *inflictor);
void Interaction_DamageOtherPlayers(WOBJ *inflictor);
WOBJ *Interaction_GetPlayerAsVictim(WOBJ *inflictor);
int Interaction_PlayerRecieveDamage(void);
int Interaction_WobjReceiveBlockDamage(WOBJ *wobj);
void Interaction_DestroyWobj(WOBJ *wobj);
void Interaction_DestroyWobjInstant(WOBJ *wobj);
WOBJ *Interaction_CreateWobj(int type, float x, float y, int ci, float cf);
void Interaction_FinishLevel(int ending_text_line);
void Interaction_Tick(void);
//void Interaction_SendWobjHurtPackets(int to_node);
//void Interaction_ApplyHurtPacketsToWobj(WOBJ *wobj);
void Interaction_ClearDestroyedWobjsBuffer(void);
int Interaction_GetMode(void);
void Interaction_PlaySound(WOBJ *wobj, int sound_id);
void Interaction_ResetAudioUUID(void);
WOBJ *Interaction_GetNearestPlayerToPoint(float px, float py);
float Interaction_GetDistanceToWobj(WOBJ *a, WOBJ *b);
void Interaction_ForceWobjPosition(WOBJ *wobj, float x, float y);

#endif
