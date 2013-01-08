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
#include "../common/editor.h"

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

 SOUND SHADER MANAGER

 ==============================================================================
*/

#define MAX_SOUND_SHADERS			1024
#define MAX_SOUNDS_PER_SHADER		8

typedef enum {
	SSF_EXPLICIT			= BIT(0),
	SSF_DEFAULTED			= BIT(1),
	SSF_PRIVATE				= BIT(2),
	SSF_ANTIPRIVATE			= BIT(3),
	SSF_PLAYONCE			= BIT(4),
	SSF_GLOBAL				= BIT(5),
	SSF_OMNIDIRECTIONAL		= BIT(6),
	SSF_VOLUMETRIC			= BIT(7),
	SSF_LOOPING				= BIT(8),
	SSF_RANDOMIZE			= BIT(9),
	SSF_NORESTART			= BIT(10),
	SSF_NOOVERRIDE			= BIT(11),
	SSF_NODUPS				= BIT(12),
	SSF_NOOFFSET			= BIT(13),
	SSF_NOTIMEOFFSET		= BIT(14),
	SSF_NOFLICKER			= BIT(15),
	SSF_NOPORTALFLOW		= BIT(16),
	SSF_NODYNAMICPARMS		= BIT(17),
	SSF_NOREVERB			= BIT(18),
	SSF_NOOBSTRUCTION		= BIT(19),
	SSF_NOEXCLUSION			= BIT(20),
	SSF_NOOCCLUSION			= BIT(21)
} soundShaderFlags_t;

typedef enum {
	PRIORITY_BAD,
	PRIORITY_LOWEST,
	PRIORITY_LOW,
	PRIORITY_NORMAL,
	PRIORITY_HIGH,
	PRIORITY_HIGHEST
} priority_t;

typedef struct soundShader_s {
	char					name[MAX_PATH_LENGTH];

	int						flags;

	priority_t				priority;

	float					shakes;

	float					volume;
	float					pitch;

	float					dryFilter;
	float					dryFilterHF;

	float					wetFilter;
	float					wetFilterHF;

	float					minDistance;
	float					maxDistance;

	float					coneInnerAngle;
	float					coneOuterAngle;
	float					coneOuterVolume;

	float					rolloffFactor;
	float					roomRolloffFactor;
	float					airAbsorptionFactor;

	float					dopplerFactor;

	int						minSamples;

	int						numLeadIns;
	sound_t *				leadIns[MAX_SOUNDS_PER_SHADER];

	int						numEntries;
	sound_t *				entries[MAX_SOUNDS_PER_SHADER];

	struct soundShader_s *	nextHash;
} soundShader_t;

soundShader_t *		S_FindSoundShader (const char *name);

void				S_InitSoundShaders ();
void				S_ShutdownSoundShaders ();

/*
 ==============================================================================

 REVERB EFFECTS

 ==============================================================================
*/

#define MAX_REVERB_AREAS			BSP_MAX_AREAS

typedef struct {
	bool					enabled;

	uint					effectId;
	uint					effectSlotId;

	bool					editing;
	int						editingArea;

	reverbParms_t			reverbGeneric;
	reverbParms_t			reverbUnderwater;

	char					reverbName[MAX_PATH_LENGTH];
	reverbParms_t			reverbList[MAX_REVERB_AREAS];
	
	reverbParms_t			reverbParms;

	// Interpolation parameters
	int						time;

	reverbParms_t			previous;
	reverbParms_t *			current;
} reverb_t;

void				S_EditAreaReverb (int area);

void				S_UpdateReverb ();

void				S_InitReverbs ();
void				S_ShutdownReverbs ();

/*
 ==============================================================================

 LOW-PASS FILTERS

 ==============================================================================
*/

#define MAX_FILTERS					256

typedef struct {
	float					gain;
	float					gainHF;
} filterParms_t;

typedef struct {
	bool					enabled;

	uint					dryFilterId[MAX_FILTERS];
	uint					wetFilterId[MAX_FILTERS];
} filter_t;

void				S_ObstructionFilter (const vec3_t srcOrigin, const vec3_t dstOrigin, float distance, float minDistance, float maxDistance, filterParms_t *dryFilter);
void				S_ExclusionFilter (int portalsPassed, filterParms_t *wetFilter);
void				S_OcclusionFilter (int portalsBlocked, filterParms_t *dryFilter, filterParms_t *wetFilter);

void				S_UpdateFilters (int index, filterParms_t *dryFilter, filterParms_t *wetFilter);

void				S_InitFilters ();
void				S_ShutdownFilters ();

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

 SOUND EMITTERS

 ==============================================================================
*/

#define MAX_SOUND_EMITTERS			2048

typedef struct emitter_s {
	bool					active;
	int						index;

	soundEmitter_t			e;

	sound_t *				lastLeadIn;
	sound_t *				lastEntry;

	struct emitter_s *		prev;
	struct emitter_s *		next;
} emitter_t;

void				S_InitEmitters ();
void				S_ShutdownEmitters ();

/*
 ==============================================================================

 SOUND LISTENER

 ==============================================================================
*/

void				S_UpdateListener ();

/*
 ==============================================================================

 SOUND CHANNELS

 ==============================================================================
*/

#define MAX_SOUND_CHANNELS			256

typedef enum {
	CS_FREE,									// Free channel
	CS_NORMAL,									// Playing normal sound
	CS_LOOPED,									// Playing looped sound
	CS_RANDOM,									// Playing random sounds continuously
	CS_LEADIN_NORMAL,							// Playing lead-in + normal sound
	CS_LEADIN_LOOPED,							// Playing lead-in + looped sound
	CS_LEADIN_RANDOM							// Playing lead-in + random sounds continuously
} channelState_t;

typedef struct {
	bool					spatialized;

	bool					reachable;

	float					minDistance;
	float					maxDistance;

	vec3_t					dirToListener;
	float					distToListener;

	int						portalsPassed;
	int						portalsBlocked;

	vec3_t					origin;
	vec3_t					direction;

	float					volume;
	float					pitch;

	bool					feedReverb;

	filterParms_t			dryFilter;
	filterParms_t			wetFilter;
} channelParms_t;

typedef struct {
	channelState_t			state;
	int						index;

	bool					streaming;			// If true, we're streaming sound buffers into a queue

	sound_t *				lastSound;			// Last sound selected for playing

	float					amplitude;			// Current sound amplitude for amplitude queries

	int						emitterId;			// To allow overriding a specific sound
	int						channelId;			// To allow overriding a specific sound
	int						allocTime;			// To allow overriding oldest sounds

	struct emitter_s *		emitter;
	soundShader_t *			soundShader;

	soundEmitter_t			e;
	channelParms_t			p;

	uint					sourceId;
} channel_t;

channel_t *			S_PickChannel (emitter_t *emitter, int emitterId, int channelId, soundShader_t *soundShader);

void				S_PlayChannel (channel_t *channel, bool allowLeadIn);
void				S_StopChannel (channel_t *channel);
void				S_UpdateChannel (channel_t *channel);

void				S_UpdateLoopingSounds ();

void				S_InitChannels ();
void				S_ShutdownChannels ();

/*
 ==============================================================================

 GLOBALS

 ==============================================================================
*/

typedef struct {
	int						emitterUpdates;

	int						channels;
	int						localChannels;
	int						worldChannels;

	int						portals;
} sndPerformanceCounters_t;

typedef struct {
	int						time;

	bool					active;

	// Frame counters
	int						frameCount;

	// Streaming sources
	uint					musicSource;
	uint					rawSamplesSource;

	// Reverb effects and low-pass filters
	reverb_t				reverb;
	filter_t				filter;

	// Background music
	music_t					music;

	// Local emitter for non-spatialized local sounds
	emitter_t *				localEmitter;

	// Listener
	soundListener_t			listener;

	// Channels
	int						numChannels;
	channel_t				channels[MAX_SOUND_CHANNELS];

	// Looping entity sounds
	entity_state_t *		soundEntities[MAX_PARSE_ENTITIES];
	int						numSoundEntities;

	// Performance counters
	sndPerformanceCounters_t	pc;

	// Internal assets
	sound_t *				defaultSound;

	soundShader_t *			defaultSoundShader;
} sndGlobals_t;

extern sndGlobals_t			snd;

extern alConfig_t			alConfig;

extern bool					s_initialized;

extern cvar_t *				s_logFile;
extern cvar_t *				s_ignoreALErrors;
extern cvar_t *				s_speedOfSound;
extern cvar_t *				s_reverbTime;
extern cvar_t *				s_obstructionScale;
extern cvar_t *				s_exclusionScale;
extern cvar_t *				s_occlusionScale;
extern cvar_t *				s_singleSoundShader;
extern cvar_t *				s_singleEmitter;
extern cvar_t *				s_showUpdates;
extern cvar_t *				s_showEmitters;
extern cvar_t *				s_showStreaming;
extern cvar_t *				s_showChannels;
extern cvar_t *				s_showPortals;
extern cvar_t *				s_showSounds;
extern cvar_t *				s_skipUpdates;
extern cvar_t *				s_skipEmitters;
extern cvar_t *				s_skipStreaming;
extern cvar_t *				s_skipShakes;
extern cvar_t *				s_skipFlicker;
extern cvar_t *				s_skipSpatialization;
extern cvar_t *				s_skipAttenuation;
extern cvar_t *				s_skipCones;
extern cvar_t *				s_skipPortals;
extern cvar_t *				s_skipDynamic;
extern cvar_t *				s_skipObstructions;
extern cvar_t *				s_skipExclusions;
extern cvar_t *				s_skipOcclusions;
extern cvar_t *				s_skipReverbs;
extern cvar_t *				s_skipFilters;
extern cvar_t *				s_alDriver;
extern cvar_t *				s_deviceName;
extern cvar_t *				s_captureDeviceName;
extern cvar_t *				s_masterVolume;
extern cvar_t *				s_emitterVolume;
extern cvar_t *				s_reverbVolume;
extern cvar_t *				s_musicVolume;
extern cvar_t *				s_voiceVolume;
extern cvar_t *				s_dopplerShifts;
extern cvar_t *				s_airAbsorption;
extern cvar_t *				s_reverbEffects;
extern cvar_t *				s_lowPassFilters;
extern cvar_t *				s_voiceCapture;
extern cvar_t *				s_voiceScale;
extern cvar_t *				s_voiceLatency;
extern cvar_t *				s_maxChannels;
extern cvar_t *				s_maxSoundsPerShader;
extern cvar_t *				s_soundQuality;
extern cvar_t *				s_playDefaultSound;
extern cvar_t *				s_muteOnLostFocus;

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