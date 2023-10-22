#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <SDL_mixer.h>
#include "console.h"
#include "utility.h"
#include "audio.h"
#include "game.h"

#define MAX_CHANNELS 8

static int audio_initialized = CNM_FALSE;
static Mix_Music *audio_musics[AUDIO_MAX_IDS];
//static int audio_musvolume;
static Mix_Chunk *audio_chunks[AUDIO_MAX_IDS];
static int audio_x, audio_y;
static int audio_current_mus = -1;
static int audio_no_audio = CNM_FALSE;
static int audio_chunk_volume = MIX_MAX_VOLUME/2;
static float set_volume = 0.0;

void Audio_Init(void)
{
	Mix_Init(0);
	if (Mix_OpenAudio(44100, AUDIO_U16, 2, 4096) == -1)
	{
		Console_Print("An error occured while trying to initialize the audio sub system!");
		return;
	}

	//Mix_AllocateChannels(MAX_CHANNELS);
	memset(audio_musics, 0, sizeof(audio_musics));
	memset(audio_chunks, 0, sizeof(audio_chunks));
	audio_x = 0;
	audio_y = 0;
	audio_current_mus = -1;
	audio_chunk_volume = MIX_MAX_VOLUME;

	audio_no_audio = CNM_FALSE;
	audio_initialized = CNM_TRUE;

	Audio_SetGlobalVolume(1.0f);
}
void Audio_Quit(void)
{
	if (!audio_initialized)
		return;

	for (int i = 0; i < AUDIO_MAX_IDS; i++)
	{
		if (audio_musics[i] != NULL)
			Mix_FreeMusic(audio_musics[i]);
		if (audio_chunks[i] != NULL)
			Mix_FreeChunk(audio_chunks[i]);
	}
	Mix_CloseAudio();
	Mix_Quit();
	audio_initialized = CNM_FALSE;
}
void Audio_SetGlobalVolume(float volume)
{
	int old_audio;
	if (!audio_initialized)
		return;
	set_volume = volume;
	old_audio = audio_no_audio;
	audio_no_audio = (volume == 0.0f);
	if (audio_no_audio)
	{
		Mix_HaltMusic();
		for (int i = 0; i < MAX_CHANNELS; i++)
			Mix_HaltChannel(i);
	}
	else if (old_audio && !audio_no_audio)
	{
		Mix_PlayMusic(audio_musics[audio_current_mus], CNM_TRUE);
	}

	//audio_musvolume = (int)(volume * (float)MIX_MAX_VOLUME);
	Mix_VolumeMusic((int)(volume * (float)MIX_MAX_VOLUME));
	audio_chunk_volume = (int)(volume * 0.35 * (float)MIX_MAX_VOLUME);
}
float Audio_GetGlobalVolume(void)
{
	return set_volume;
}
void Audio_AddMusicEntry(int music_id, const char *music_file)
{
	//audio_initialized = CNM_TRUE;
	if (!audio_initialized)
		return;
	//if (strcmp(music_file, "2drend.dll") == 0) goto skip_file_check;

	//if (strchr(music_file, '.') == NULL || (strcmp(strchr(music_file, '.'), ".wav") != 0 && strcmp(strchr(music_file, '.'), ".mid") != 0))
	//{
	//	Console_Print("music file %s is in an invalid format.", music_file);
	//	return;
	//}

//skip_file_check:
	Audio_StopMusic();
	if (audio_musics[music_id] != NULL)
		Mix_FreeMusic(audio_musics[music_id]);
	//audio_musvolume[music_id] = MIX_MAX_VOLUME / 2;
	audio_musics[music_id] = Mix_LoadMUS(music_file);
}
void Audio_AddSoundEntry(int sound_id, const char *sound_file)
{
	if (!audio_initialized)
		return;

	//if (strchr(sound_file, '.') == NULL || strcmp(strchr(sound_file, '.'), ".wav") != 0)
	//{
	//	Console_Print("sound %s is in an invalid format.", sound_file);
	//	return;
	//}

	if (audio_chunks[sound_id] != NULL)
		Mix_FreeChunk(audio_chunks[sound_id]);
	audio_chunks[sound_id] = Mix_LoadWAV(sound_file);
}
void Audio_PlayMusic(int index, int loop)
{
	if (Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer) index = AUDIO_MAX_IDS - 1;
	if (audio_current_mus == index)
		return;
	audio_current_mus = index;
	if (!audio_initialized)
		return;
	if (Game_GetVar(GAME_VAR_SUPERVIRUS)->data.integer) {
		Mix_PlayMusic(audio_musics[AUDIO_MAX_IDS - 1], -1);
		return;
	}
	if (audio_no_audio)
		return;
	Mix_PlayMusic(audio_musics[index], loop ? -1 : 0);
	//Mix_VolumeMusic(audio_chunk_volume);
}
int Audio_GetCurrentPlayingMusic(void)
{
	return audio_current_mus;
}
void Audio_StopMusic(void)
{
	if (!audio_initialized)
		return;
	audio_current_mus = -1;
	Mix_HaltMusic();
}
void Audio_PlaySound(int index, int altchannel, int x, int y)
{
	if (!audio_initialized || audio_no_audio)
		return;

	int dx = abs(x - audio_x);
	int dy = abs(y - audio_y);
	float dist = sqrtf((float)(dx*dx + dy*dy));
	if (dx == 0 && dy == 0)
		dist = 0.0f;
#define MAX_AUDIO_DIST (160.0f*3.0f)
	if (dist <= MAX_AUDIO_DIST)
	{
		int channel = Mix_PlayChannel(-1, audio_chunks[index], 0);//Mix_PlayChannel((index + (altchannel ? 1 : 0)) % MAX_CHANNELS, audio_chunks[index], 0);
		Mix_VolumeChunk(audio_chunks[index], audio_chunk_volume);
		if (dist > MAX_AUDIO_DIST / 3.0f * 2.0f)
			Mix_VolumeChunk(audio_chunks[index], audio_chunk_volume / 3);
		else if (dist > MAX_AUDIO_DIST / 3.0f)
			Mix_VolumeChunk(audio_chunks[index], audio_chunk_volume / 3 * 2);
	}
}
void Audio_PlaySoundscape(int index, int x, int y) {
	if (!audio_initialized || audio_no_audio)
		return;

	int dx = abs(x - audio_x);
	int dy = abs(y - audio_y);
	float dist = sqrtf((float)(dx * dx + dy * dy));
	if (dx == 0 && dy == 0)
		dist = 0.0f;
#define MAX_AUDIO_DIST (160.0f*3.0f)
	if (dist <= MAX_AUDIO_DIST && !Mix_Playing(0))
	{
		int channel = Mix_PlayChannel(0, audio_chunks[index], 0);//Mix_PlayChannel((index + (altchannel ? 1 : 0)) % MAX_CHANNELS, audio_chunks[index], 0);
		Mix_VolumeChunk(audio_chunks[index], audio_chunk_volume);
		if (dist > MAX_AUDIO_DIST / 3.0f * 2.0f)
			Mix_VolumeChunk(audio_chunks[index], audio_chunk_volume / 3);
		else if (dist > MAX_AUDIO_DIST / 3.0f)
			Mix_VolumeChunk(audio_chunks[index], audio_chunk_volume / 3 * 2);
	}
}
void Audio_SetListenerOrigin(int x, int y)
{
	audio_x = x;
	audio_y = y;
}
int Audio_IsSoundPlaying(int index)
{
	if (!audio_initialized)
		return 0;
	else
		return Mix_Playing(index);
}
int Audio_GetListenerX(void)
{
	return audio_x;
}
int Audio_GetListenerY(void)
{
	return audio_y;
}
void Audio_AutoBalanceMusic(int musid, int volume_percent)
{
	//if (musid > -1 && musid < AUDIO_MAX_IDS)
	//	audio_musvolume[musid] = (int)(((float)volume_percent / 100.0f) * (float)MIX_MAX_VOLUME);
}
