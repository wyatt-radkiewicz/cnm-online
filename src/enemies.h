#ifndef _enemies_h_
#define _enemies_h_

typedef struct _WOBJ WOBJ;

void Enemies_Reset(void);
void Enemies_ZoneAllocLocalDataPools(void);

// Slimes
void WobjSlime_Create(WOBJ *wobj);
void WobjSlime_Update(WOBJ *wobj);
void WobjSlime_Draw(WOBJ *wobj, int camx, int camy);

// Flying Slimes
void WobjFlyingSlime_Create(WOBJ *wobj);
void WobjFlyingSlime_Update(WOBJ *wobj);

// Heavys
void WobjHeavy_Create(WOBJ *wobj);
void WobjHeavy_Update(WOBJ *wobj);
void WobjHeavySheildBox_Create(WOBJ *wobj);
void WobjHeavySheildBox_Draw(WOBJ *wobj, int camx, int camy);

// Heavy Blasts
void WobjHeavyBlast_Create(WOBJ *wobj);
void WobjHeavyBlast_Update(WOBJ *wobj);

// Dragons
void WobjDragon_Create(WOBJ *wobj);
void WobjDragon_Update(WOBJ *wobj);

// Fireballs
void WobjFireball_Create(WOBJ *wobj);
void WobjFireball_Update(WOBJ *wobj);
void WobjFireball_Draw(WOBJ *wobj, int camx, int camy);

// Bozo Pin
void WobjBozoPin_Create(WOBJ *wobj);
void WobjBozoPin_Update(WOBJ *wobj);
void WobjBozoPin_Draw(WOBJ *wobj, int camx, int camy);

// Bozo
void WobjBozo_Create(WOBJ *wobj);
void WobjBozo_Update(WOBJ *wobj);
void WobjBozoWaypoint_Create(WOBJ *wobj);
void WobjBozoFireball_Create(WOBJ *wobj);
void WobjBozoFireball_Update(WOBJ *wobj);

// Silver Slime
void WobjSliverSlime_Create(WOBJ *wobj);

// Lava Monster
void WobjLavaMonster_Create(WOBJ *wobj);
void WobjLavaMonster_Update(WOBJ *wobj);

// TT Minion Small
void WobjTTMinionSmall_Create(WOBJ *wobj);
void WobjTTMinionSmall_Update(WOBJ *wobj);

// TT Minion Big
void WobjTTMinionBig_Create(WOBJ *wobj);
void WobjTTMinionBig_Update(WOBJ *wobj);

// Slime Walker
void WobjWolf_Create(WOBJ *wobj);
void WobjSlimeWalker_Create(WOBJ *wobj);
void WobjSlimeWalker_Update(WOBJ *wobj);
void WobjSlimeWalker_Draw(WOBJ *wobj, int camx, int camy);

// Mega Fish
void WobjMegaFish_Create(WOBJ *wobj);
void WobjMegaFish_Update(WOBJ *wobj);

// Lava Dragon
void WobjLavaDragonHead_Create(WOBJ *wobj);
void WobjLavaDragonHead_Update(WOBJ *wobj);
void WobjLavaDragonHead_Death(WOBJ *wobj);
void WobjLavaDragonBody_Create(WOBJ *wobj);

// Talking Teddy Boss
void WobjTTBossTriggerGeneric_Create(WOBJ *wobj);
void WobjTTBossWaypoint_Create(WOBJ *wobj);
void WobjTTBossTriggerNormal_Update(WOBJ *wobj);
void WobjTTBoss_Create(WOBJ *wobj);
void WobjTTBoss_Update(WOBJ *wobj);
void TTBoss_ResetOnLevelLoad(void);
void TTBoss_CalmDown(void);

// Eater Bug
void WobjEaterBug_Create(WOBJ *wobj);
void WobjEaterBug_Update(WOBJ *wobj);

// Spider Walker
void WobjSpiderWalker_Create(WOBJ *wobj);
void WobjSpiderWalker_Update(WOBJ *wobj);
void WobjSpiderWalkerWeb_Create(WOBJ *wobj);
void WobjSpiderWalkerWeb_Update(WOBJ *wobj);

// Spike Trap
void WobjSpikeTrap_Create(WOBJ *wobj);
void WobjSpikeTrap_Update(WOBJ *wobj);

// Rotating Fire Colunm Piece
void WobjRotatingFireColunmPiece_Create(WOBJ *wobj);
void WobjRotatingFireColunmPiece_Update(WOBJ *wobj);
void WobjRotatingFireColunmPiece_Draw(WOBJ *wobj, int camx, int camy);

// Moving Fire Objects
void WobjMovingFireVertical_Create(WOBJ *wobj);
void WobjMovingFireVertical_Update(WOBJ *wobj);
void WobjMovingFireHorizontal_Create(WOBJ *wobj);
void WobjMovingFireHorizontal_Update(WOBJ *wobj);
void WobjMovingFire_Draw(WOBJ *wobj, int camx, int camy);

// Super Dragon
void WobjSuperDragonBoss_Create(WOBJ *wobj);
void WobjSuperDragonBoss_Update(WOBJ *wobj);
void WobjSuperDragonLandingZone_Create(WOBJ *wobj);

// Bozo Laser Minion
void WobjBozoLaserMinion_Create(WOBJ *wobj);
void WobjBozoLaserMinion_Update(WOBJ *wobj);
void WobjBozoLaserLockon_Create(WOBJ *wobj);
void WobjBozoLaserLockon_Update(WOBJ *wobj);
void WobjBozoLaserPart_Create(WOBJ *wobj);
void WobjBozoLaserPart_Update(WOBJ *wobj);

// Bozo Mk.2
void WobjBozoMk2_Create(WOBJ *wobj);
void WobjBozoMk2_Update(WOBJ *wobj);
void WobjEnemyRocket_Create(WOBJ *wobj);
void WobjEnemyRocket_Update(WOBJ *wobj);

// Spike Guy
void WobjSpikeGuy_Create(WOBJ *wobj);
void WobjSpikeGuy_Update(WOBJ *wobj);
void WobjSpikeGuySpike_Create(WOBJ *wobj);
void WobjSpikeGuySpike_Update(WOBJ *wobj);

// Bandit Guy
void WobjBanditGuy_Create(WOBJ *wobj);
void WobjBanditGuy_Update(WOBJ *wobj);

// Kamakazi Slime
void WobjKamakaziSlime_Create(WOBJ *wobj);
void WobjKamakaziSlime_Update(WOBJ *wobj);
void WobjKamakaziSlime_Draw(WOBJ *wobj, int camx, int camy);
void WobjKamakaziSlimeExplosion_Create(WOBJ *wobj);
void WobjKamakaziSlimeExplosion_Update(WOBJ *wobj);

// Medium Rock Guy
void WobjRockGuyMedium_Create(WOBJ *wobj);
void WobjRockGuyMedium_Update(WOBJ *wobj);

// Small Rock Guy 1
void WobjRockGuySmall1_Create(WOBJ *wobj);
void WobjRockGuySmall1_Update(WOBJ *wobj);

// Small Rock Guy 2
void WobjRockGuySmall2_Create(WOBJ *wobj);
void WobjRockGuySmall2_Update(WOBJ *wobj);

// Slider Rock Guy
void WobjRockGuySlider_Create(WOBJ *wobj);
void WobjRockGuySlider_Update(WOBJ *wobj);

// Smasher Rock Guy
void WobjRockGuySmasher_Create(WOBJ *wobj);
void WobjRockGuySmasher_Update(WOBJ *wobj);

// Rock Guy Spear
void WobjRockGuySpear_Create(WOBJ *wobj);
void WobjRockGuySpear_Update(WOBJ *wobj);

// Supervirus
void WobjSupervirus_Create(WOBJ *wobj);
void WobjSupervirus_Update(WOBJ *wobj);
void WobjSupervirus_Draw(WOBJ *wobj, int camx, int camy);

#endif
