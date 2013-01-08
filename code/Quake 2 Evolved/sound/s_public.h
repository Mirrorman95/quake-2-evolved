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
// s_public.h - Public header for all other systems
//


#ifndef __S_PUBLIC_H__
#define __S_PUBLIC_H__


// Sound channels must be defined by the game code.
// This is the only channel used internally by the sound system.
#define SOUND_CHANNEL_ANY			0

// Sound parms
#define MAX_SOUND_PARMS				8

#define SOUNDPARM_VOLUME			0			// Volume scale
#define SOUNDPARM_PITCH				1			// Pitch scale
#define SOUNDPARM_DRYFILTER			2			// Low-pass filter gain scale (direct path)
#define SOUNDPARM_DRYFILTERHF		3			// Low-pass filter gain HF scale (direct path)
#define SOUNDPARM_WETFILTER			4			// Low-pass filter gain scale (reverb path)
#define SOUNDPARM_WETFILTERHF		5			// Low-pass filter gain HF scale (reverb path)
#define SOUNDPARM_MINDISTANCE		6			// Used to override sound shader settings
#define SOUNDPARM_MAXDISTANCE		7			// Used to override sound shader settings

typedef enum {
	ALHW_GENERIC,
	ALHW_CREATIVE
} alHardwareType_t;

typedef struct soundShader_s		soundShader_t;

typedef struct soundEmitter_s {
	int						emitterId;

	// Transformation matrix
	vec3_t					origin;
	vec3_t					velocity;			// For doppler shifts
	vec3_t					direction;			// For directional cones (should be normalized)
	vec3_t					axis[3];

	// Environment information
	int						area;
	bool					underwater;

	// Sound parms
	float					soundParms[MAX_SOUND_PARMS];
} soundEmitter_t;

typedef struct {
	int						listenerId;

	// Transformation matrix
	vec3_t					origin;
	vec3_t					velocity;
	vec3_t					axis[3];

	// Environment information
	int						area;
	bool					underwater;

	// Time in milliseconds for time dependent effects
	int						time;
} soundListener_t;

typedef struct {
	bool					initialized;

	alHardwareType_t		hardwareType;

	const char *			vendorString;
	const char *			rendererString;
	const char *			versionString;
	const char *			extensionsString;
	const char *			alcExtensionsString;

	bool					efxAvailable;
	bool					eaxRAMAvailable;

	int						maxAuxiliarySends;

	const char *			deviceName;
	const char *			deviceList;

	const char *			captureDeviceName;
	const char *			captureDeviceList;

	int						mixerFrequency;
	int						mixerRefresh;
	bool					mixerSync;

	int						monoSources;
	int						stereoSources;
} alConfig_t;

// Loads and prepares environmental reverberation effects for the given map
void				S_LoadReverb (const char *name);

// Enumerates sound shader definitions using a callback
void				S_EnumSoundShaderDefs (void (*callback)(const char *name));

// Loads and registers the given sound shader
soundShader_t *		S_RegisterSoundShader (const char *name);

// Loads and registers the given sound
sound_t *			S_FindSound (const char *name, int flags);
sound_t *			S_RegisterSound (const char *name, int flags);
sound_t *			S_FindSexedSound (const char *name, entity_state_t *entity, int flags);

// Emitters are used to play sounds in the world.
// When an emitter is removed all sounds can continue playing until completed,
// or they can be stopped immediately.
int					S_AddSoundEmitter (const soundEmitter_t *soundEmitter);
void				S_UpdateSoundEmitter (int emitterHandle, const soundEmitter_t *soundEmitter);
void				S_RemoveSoundEmitter (int emitterHandle, bool stopAllSounds);

// Plays a sound from the given emitter on the given channel.
// SOUND_CHANNEL_ANY will never override playing sounds.
void				S_PlaySoundFromEmitter (int emitterHandle, int channelId, int startTime, soundShader_t *soundShader);

// Stops all sounds playing from the given emitter on the given channel.
// SOUND_CHANNEL_ANY will stop all sounds.
void				S_StopSoundFromEmitter (int emitterHandle, int channelId);

// Returns the amplitude of all sounds playing from the given emitter on the
// given channel.
// SOUND_CHANNEL_ANY will account for all sounds.
float				S_AmplitudeFromEmitter (int emitterHandle, int channelId);

// Returns true if there are any sounds playing from the given emitter on the
// given channel.
// SOUND_CHANNEL_ANY will check all sounds.
bool				S_PlayingFromEmitter (int emitterHandle, int channelId);

// Clears all the active sound emitters, stopping all sounds immediately
void				S_ClearAllSoundEmitters ();

// Updates listener parameters
void			S_PlaceListener (const soundListener_t *soundListener);

// Returns a shake amplitude for the current listener parameters
float				S_ShakeAmplitudeForListener ();

// Local sounds always play at full volume with no spatialization
void				S_PlayLocalSound (soundShader_t *soundShader);

// Plays background music.
// If already playing, the given tracks are added to a queue and played after
// the current tracks have completely faded down.
void				S_PlayMusic (const char *introTrack, const char *loopTrack, int fadeUpTime);

// Fades down and stops playing background music.
// If there are any queued tracks, they will start playing immediately after
// the current tracks have completely faded down.
void				S_FadeMusic (int fadeDownTime);

// Stops playing background music immediately
void				S_StopMusic ();

// Raw samples for cinematics and voice over network.
// Does not support multiple concurrent raw sample sources, nor does it handle
// byte ordering.
void				S_RawSamples (const short *data, int samples, int rate, bool stereo, float volume);

// Enumerates sound shader definitions using a callback
void				S_EnumSoundShaderDefs (void (*callback)(const char *name));

// Returns information about the OpenAL subsystem
alConfig_t			S_GetALConfig (void);

// This is called by the system when the main window gains or loses focus
void				S_Activate (bool active);

// Stops all sounds
void				S_StopAllSounds ();

// Updates sounds
void				S_Update (int time);

// Initializes the sound subsystem.
// If all is false, the OpenAL subsystem will not be initialized, which is
// useful for flushing all data and resetting the state.
void				S_Init (bool all);

// Shuts down the sound subsystem.
// If all is false, the OpenAL subsystem will not be shut down, which is useful
// for flushing all data and resetting the state.
void				S_Shutdown (bool all);


#endif	// __S_PUBLIC_H__