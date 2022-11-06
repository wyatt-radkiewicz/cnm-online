#ifndef _serial_h_
#define _serial_h_
#include "utility.h"

void Serial_CondenceBlockFileAndSave(const char *cnmb_file);
void Serial_LoadBlocks(const char *cnmb_file);
void Serial_SaveBlocks(const char *cnmb_file);
void Serial_LoadBlocksLevelPreview(const char *cnmb_file, CNM_RECT *r, int *dif);
void Serial_LoadSpawnersLevelName(const char *cnms_file, char *name_buf);
void Serial_LoadSpawners(const char *cnms_file);
void Serial_SaveSpawners(const char *cnms_file);
void Serial_LoadAudioCfg(const char *cnma_file);
void Serial_SaveConfig(void);
void Serial_LoadConfig(void);

#endif