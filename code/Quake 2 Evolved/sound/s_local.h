/*
 ------------------------------------------------------------------------------
 Copyright (C) 1997-2001 Id Software.

 This file is part of the Quake 2 source code.

 The Quake 2 source code is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

 The Quake 2 source code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.

 You should have received a copy of the GNU General Public License along with
 the Quake 2 source code; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ------------------------------------------------------------------------------
*/


//
// s_local.h - Local header file to all sound files
//


#ifndef __S_LOCAL_H__
#define __S_LOCAL_H__


#include "../client/client.h"
#include "../windows/qal.h"

#include "../include/OggVorbis/vorbisfile.h"


// This is defined in shared.h
#undef MAX_SOUNDS


/*
 ==============================================================================

 SOUND MANAGER

 ==============================================================================
*/

#define MAX_SOUNDS					4096

typedef enum {
	SF_INTERNAL				= BIT(0)
} soundFlags_t;

typedef struct {
	int						rate;
	int						width;
	int						channels;
	int						samples;
} wavInfo_t;

typedef struct sound_s {
	char					name[MAX_PATH_LENGTH];

	int						flags;
	int						rate;
	int						samples;
	int						length;
	int						size;

	int						tableSize;
	float *					tableValues;

	uint					bufferId;

	struct sound_s *		nextHash;
} sound_t;

sound_t *			S_FindSound (const char *name, int flags);
sound_t *			S_FindSexedSound (const char *name, entity_state_t *entity, int flags);
sound_t *			S_RegisterSexedSound (const char *name, entity_state_t *entity, int flags);

void				S_InitSounds ();
void				S_ShutdownSounds ();

/*
 ==============================================================================

 BACKGROUND MUSIC

 ==============================================================================
*/

#define MUSIC_RATE					44100

#define MUSIC_FRAMERATE				25
#define MUSIC_FRAMEMSEC				(1000 / MUSIC_FRAMERATE)

#define MUSIC_BUFFERS				8
#define MUSIC_BUFFER_SAMPLES		(MUSIC_RATE / MUSIC_FRAMERATE)

#define MAX_PLAYSOUNDS				128

typedef enum {
	MS_STOPPED,
	MS_PLAYING,
	MS_LOOPING,
	MS_WAITING
} musicState_t;

typedef struct musicQueue_s {
	char					introTrack[MAX_PATH_LENGTH];
	char					loopTrack[MAX_PATH_LENGTH];

	int						fadeUpTime;

	struct musicQueue_s *	next;
} musicQueue_t;

typedef struct {
	musicState_t			state;

	musicQueue_t *			queue;

	char					introTrack[MAX_PATH_LENGTH];
	char					loopTrack[MAX_PATH_LENGTH];

	int						time;
	int						fadeStartTime;
	int						fadeEndTime;

	float					volume;
	float					fromVolume;
	float					toVolume;

	short					samples[MUSIC_BUFFER_SAMPLES][2];

	fileHandle_t			oggFile;
	int						oggSize;
	int						oggSkip;
	int						oggOffset;

	OggVorbis_File *		oggVorbisBitstream;
} music_t;

void				S_UpdateMusic ();

void				S_InitMusic ();
void				S_ShutdownMusic ();

/*
 ==============================================================================

 RAW SAMPLES

 ==============================================================================
*/

void				S_FlushRawSamples (bool forceStop);

void				S_InitRawSamples ();
void				S_ShutdownRawSamples ();

/*
 ==============================================================================

 VOICE OVER NETWORK (TODO?)

 ==============================================================================
*/

#define MAX_VOICE_SAMPLES			16000

#define VOICE_RATE					8000

/*
 ==============================================================================

 SOUND CHANNELS

 ==============================================================================
*/

#define MAX_SOUND_CHANNELS			256

typedef enum {
	CS_FREE,									// Free channel
	CS_NORMAL,									// Playing normal sound
	CS_LOOPED									// Playing looped sound
} channelState_t;

typedef struct {
	bool					spatialized;

	float					minDistance;
	float					maxDistance;

	vec3_t					dirToListener;
	float					distToListener;

	vec3_t					origin;
} channelParms_t;

typedef struct {
	channelState_t			state;

	bool					streaming;			// If true, we're streaming sound buffers into a queue

	float					amplitude;			// Current sound amplitude for amplitude queries

	int						emitter;			// To allow overriding a specific sound
	int						channelId;			// To allow overriding a specific sound	
	int						allocTime;			// To allow overriding oldest sounds
	
	int						loopNum;			// Looping entity number
	int						loopFrame;			// For stopping looping sounds
	
	bool					fixedPosition;		// Use position instead of fetching entity's origin
	vec3_t					position;			// Only use if fixedPosition is set
	
	float					volume;
	
	float					distanceMult;

	sound_t *				sound;				// NULL if unused

	channelParms_t			p;

	uint					sourceId;			// OpenAL source
} channel_t;

channel_t *			S_PickChannel (int entNum, int entChannel);
void				S_PlayChannel (channel_t *channel, sound_t *sound);
void				S_StopChannel (channel_t *channel);
void				S_UpdateChannel (channel_t *channel);

void				S_UpdateLoopingSounds ();

void				S_InitChannels ();
void				S_ShutdownChannels ();

/*
 ==============================================================================

 SOUND LISTENER

 ==============================================================================
*/

void				S_UpdateListener ();

/*
 ==============================================================================

 GLOBALS

 ==============================================================================
*/

// A playSound will be generated by each call to S_PlaySound.
// When the mixer reaches playSound->beginTime, the playSound will be
// assigned to a channel.
typedef struct playSound_s {
	struct playSound_s *	prev, *next;

	sound_t *				sound;

	int						emitter;
	int						channelId;
	
	bool					fixedPosition;	// Use position instead of fetching entity's origin
	
	vec3_t					position;		// Only use if fixedPosition is set
	float					volume;
	float					attenuation;

	int						beginTime;		// Begin at this time
} playSound_t;

typedef struct {
	int						emitterUpdates;

	int						channels;
	int						localChannels;
	int						worldChannels;

	int						portals;
} performanceCounters_t;

typedef struct {
	int						time;

	bool					active;

	// Frame counters
	int						frameCount;

	// Streaming sources
	uint					musicSource;
	uint					rawSamplesSource;

	// Background music
	music_t					music;

	// Listener
	soundListener_t			listener;

	// Channels
	int						numChannels;
	channel_t				channels[MAX_SOUND_CHANNELS];

	// Looping entity sounds
	entity_state_t *		soundEntities[MAX_PARSE_ENTITIES];
	int						numSoundEntities;

	// Play sound
	playSound_t				playSounds[MAX_PLAYSOUNDS];
	playSound_t				freePlaySounds;
	playSound_t				pendingPlaySounds;

	// Performance counters
	performanceCounters_t	pc;

	// Internal assets
	sound_t *				defaultSound;
} sndGlobals_t;

extern sndGlobals_t			snd;

extern alConfig_t			alConfig;

extern bool					s_initialized;

extern cvar_t *				s_logFile;
extern cvar_t *				s_ignoreALErrors;
extern cvar_t *				s_showStreaming;
extern cvar_t *				s_skipStreaming;
extern cvar_t *				s_skipShakes;
extern cvar_t *				s_skipSpatialization;
extern cvar_t *				s_alDriver;
extern cvar_t *				s_deviceName;
extern cvar_t *				s_captureDeviceName;
extern cvar_t *				s_masterVolume;
extern cvar_t *				s_voiceCapture;
extern cvar_t *				s_maxChannels;
extern cvar_t *				s_musicVolume;
extern cvar_t *				s_soundQuality;
extern cvar_t *				s_playDefaultSound;

#if 0
extern cvar_t *				s_initSound;
extern cvar_t *				s_show;
extern cvar_t *				s_alDriver;
extern cvar_t *				s_alDevice;
extern cvar_t *				s_allowExtensions;
extern cvar_t *				s_ext_eax;
extern cvar_t *				s_masterVolume;
extern cvar_t *				s_sfxVolume;
extern cvar_t *				s_musicVolume;
extern cvar_t *				s_minDistance;
extern cvar_t *				s_maxDistance;
extern cvar_t *				s_rolloffFactor;
extern cvar_t *				s_dopplerFactor;
extern cvar_t *				s_dopplerVelocity;
#endif

void			S_CheckForErrors ();

/*
 ==============================================================================

 IMPLEMENTATION SPECIFIC FUNCTIONS

 ==============================================================================
*/

#ifdef _WIN32

#define AL_DRIVER_OPENAL				"OpenAL32"

#define ALImp_StartCapture				ALW_StartCapture
#define ALImp_StopCapture				ALW_StopCapture
#define ALImp_AvailableCaptureSamples	ALW_AvailableCaptureSamples
#define ALImp_CaptureSamples			ALW_CaptureSamples
#define ALImp_Init						ALW_Init
#define ALImp_Shutdown					ALW_Shutdown

void				ALW_StartCapture ();
void				ALW_StopCapture (bool flush);
int					ALW_AvailableCaptureSamples ();
void				ALW_CaptureSamples (void *buffer, int samples);
void				ALW_Init ();
void				ALW_Shutdown ();

#else

#error "ALImp not available for this platform"

#endif


#endif	// __S_LOCAL_H__