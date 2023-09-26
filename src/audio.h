#ifndef _audio_h_
#define _audio_h_

#define AUDIO_MAX_IDS 64

void Audio_Init(void);
void Audio_Quit(void);
void Audio_SetGlobalVolume(float volume);
float Audio_GetGlobalVolume(void);
void Audio_AddMusicEntry(int music_id, const char *music_file);
void Audio_AddSoundEntry(int sound_id, const char *sound_file);
void Audio_PlayMusic(int index, int loop);
int Audio_GetCurrentPlayingMusic(void);
void Audio_StopMusic(void);
void Audio_PlaySound(int index, int altchannel, int x, int y);
void Audio_PlaySoundscape(int index, int x, int y);
void Audio_SetListenerOrigin(int x, int y);
int Audio_IsSoundPlaying(int index);
int Audio_GetListenerX(void);
int Audio_GetListenerY(void);
// Function is deprecated, does nothing now
void Audio_AutoBalanceMusic(int musid, int volume_percent);

#endif