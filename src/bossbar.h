#ifndef _BOSSBAR_H_
#define _BOSSBAR_H_

void BossBar_Init(void);
void BossBar_Update(void);
void BossBar_Draw(void);
void BossBar_RegisterBar(unsigned int wobj_node, unsigned int wobj_uuid, float maxhp, const char *bossname);

#endif
