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
// s_main.c - Primary sound file
//


#include "s_local.h"


sndGlobals_t				snd;

alConfig_t					alConfig;

cvar_t *					s_logFile;
cvar_t *					s_ignoreALErrors;
cvar_t *					s_singleSoundShader;
cvar_t *					s_showStreaming;
cvar_t *					s_skipStreaming;
cvar_t *					s_skipShakes;
cvar_t *					s_skipSpatialization;
cvar_t *					s_alDriver;
cvar_t *					s_deviceName;
cvar_t *					s_captureDeviceName;
cvar_t *					s_masterVolume;
cvar_t *					s_voiceCapture;
cvar_t *					s_maxChannels;
cvar_t *					s_musicVolume;
cvar_t *					s_maxSoundsPerShader;
cvar_t *					s_soundQuality;
cvar_t *					s_playDefaultSound;


/*
 ==================
 S_CheckForErrors
 ==================
*/
void S_CheckForErrors (){

	const char	*string;
	int			error;

	error = qalGetError();
	
	if (error == AL_NO_ERROR)
		return;

	switch (error){
	case AL_INVALID_NAME:
		string = "AL_INVALID_NAME";
		break;
	case AL_INVALID_ENUM:
		string = "AL_INVALID_ENUM";
		break;
	case AL_INVALID_VALUE:
		string = "AL_INVALID_VALUE";
		break;
	case AL_INVALID_OPERATION:
		string = "AL_INVALID_OPERATION";
		break;
	case AL_OUT_OF_MEMORY:
		string = "AL_OUT_OF_MEMORY";
		break;
	default:
		string = "UNKNOWN ERROR";
		break;
	}

	Com_Error(ERR_FATAL, "S_CheckForErrors: %s", string);
}

/*
 ==================
 S_GetALConfig
 ==================
*/
alConfig_t S_GetALConfig (){

	return alConfig;
}

/*
 ==================
 S_Activate

 Called when the main window gains or loses focus.
 The window may have been destroyed and recreated between a deactivate and an
 activate.
 ==================
*/
void S_Activate (bool active){

	if (!alConfig.initialized)
		return;

	snd.active = active;

	// Set master volume
	if (!snd.active)
		qalListenerf(AL_GAIN, 0.0f);
	else
		qalListenerf(AL_GAIN, s_masterVolume->floatValue);
}


// ============================================================================


/*
 ==================
 S_AllocPlaySound
 ==================
*/
static playSound_t *S_AllocPlaySound (){

	playSound_t	*playSound;

	playSound = snd.freePlaySounds.next;
	if (playSound == &snd.freePlaySounds)
		return NULL;		// No free playSounds

	playSound->prev->next = playSound->next;
	playSound->next->prev = playSound->prev;

	return playSound;
}

/*
 ==================
 S_FreePlaySound
 ==================
*/
static void S_FreePlaySound (playSound_t *playSound){

	playSound->prev->next = playSound->next;
	playSound->next->prev = playSound->prev;

	// Add to free list
	playSound->next = snd.freePlaySounds.next;
	snd.freePlaySounds.next->prev = playSound;
	playSound->prev = &snd.freePlaySounds;
	snd.freePlaySounds.next = playSound;
}

/*
 ==================
 S_SortPlaySound
 ==================
*/
static void S_SortPlaySound (playSound_t *playSound){

	playSound_t *sort;

	// Sort into the pending playSounds list
	for (sort = snd.pendingPlaySounds.next; sort != &snd.pendingPlaySounds&& sort->beginTime < playSound->beginTime; sort = sort->next)
		;

	playSound->next = sort;
	playSound->prev = sort->prev;

	playSound->next->prev = playSound;
	playSound->prev->next = playSound;
}

/*
 ==================
 S_SetupPlaySound
 ==================
*/
static void S_SetupPlaySound (const vec3_t position, int emitter, int channelId, sound_t *sound, float volume, float attenuation, int timeOfs){

	playSound_t *playSound;

	// Allocate a play sound
	playSound = S_AllocPlaySound();
	if (!playSound){
		if (sound->name[0] == '#')
			Com_DPrintf(S_COLOR_RED "Dropped sound %s\n", &sound->name[1]);
		else
			Com_DPrintf(S_COLOR_RED "Dropped sound sound/%s\n", sound->name);

		return;
	}

	// Fill it in
	playSound->sound = sound;
	playSound->emitter = emitter;
	playSound->channelId = channelId;

	if (position){
		playSound->fixedPosition = true;
		VectorCopy(position, playSound->position);
	}
	else
		playSound->fixedPosition = false;

	playSound->volume = volume;
	playSound->attenuation = attenuation;
	playSound->beginTime = cl.time + timeOfs;

	// Sort it
	S_SortPlaySound(playSound);
}

/*
 ==================
 S_IssuePlaySounds

 TODO: is the channel state right?
 TODO: might need to fill in some more parameters
 ==================
*/
static void S_IssuePlaySounds (){

	playSound_t	*playSound;
	channel_t	*channel;

	while (1){
		playSound = snd.pendingPlaySounds.next;
		if (playSound == &snd.pendingPlaySounds)
			break;		// No more pending playSounds

		if (playSound->beginTime > cl.time)
			break;		// No more pending playSounds this frame

		// Pick a channel and start the sound effect
		channel = S_PickChannel(playSound->emitter, playSound->channelId);
		if (!channel){
			if (playSound->sound->name[0] == '#')
				Com_DPrintf(S_COLOR_RED "Dropped sound %s\n", &playSound->sound->name[1]);
			else
				Com_DPrintf(S_COLOR_RED "Dropped sound sound/%s\n", playSound->sound->name);

			S_FreePlaySound(playSound);
			continue;
		}

		// Set up a play sound
		channel->state = CS_NORMAL;

		channel->fixedPosition = playSound->fixedPosition;
		VectorCopy(playSound->position, channel->position);

		channel->volume = playSound->volume;

		if (playSound->attenuation != ATTN_NONE)
			channel->distanceMult = 1.0f / playSound->attenuation;
		else
			channel->distanceMult = 0.0f;

		// Play it
		S_PlayChannel(channel, playSound->sound);

		// Free it
		S_FreePlaySound(playSound);
	}
}

/*
 ==================
 S_PlaySound
 ==================
*/
void S_PlaySound (const vec3_t position, int emitter, int channelId, sound_t *sound, float volume, float attenuation, int timeOfs){

	if (!sound)
		return;

	// Check if we have a sexed sound
	if (sound->name[0] == '*')
		sound = S_FindSexedSound(sound->name, &cl.entities[emitter].current, 0);

	// Make sure the sound is loaded
	if (!S_FindSound(sound->name, 0))
		return;

	// Set up a play sound
	S_SetupPlaySound(position, emitter, channelId, sound, volume, attenuation, timeOfs);
}

/*
 ==================
 S_PlayLocalSound
 ==================
*/
void S_PlayLocalSound (sound_t *sound){

	if (!sound)
		return;

	S_PlaySound(NULL, cl.clientNum, SOUND_CHANNEL_ANY, sound, 1.0f, ATTN_NONE, 0.0f);
}

/*
 ==================
 S_StopAllSounds
 ==================
*/
void S_StopAllSounds (){

	channel_t	*channel;
	int			i;

	// Reset frame count
	snd.frameCount = 0;

	// Clear all the playSounds
	memset(snd.playSounds, 0, sizeof(snd.playSounds));

	snd.freePlaySounds.next = snd.freePlaySounds.prev = &snd.freePlaySounds;
	snd.pendingPlaySounds.next = snd.pendingPlaySounds.prev = &snd.pendingPlaySounds;

	for (i = 0; i < MAX_PLAYSOUNDS; i++){
		snd.playSounds[i].prev = &snd.freePlaySounds;
		snd.playSounds[i].next = snd.freePlaySounds.next;
		snd.playSounds[i].prev->next = &snd.playSounds[i];
		snd.playSounds[i].next->prev = &snd.playSounds[i];
	}

	// Flush raw samples
	S_FlushRawSamples(true);

	// Stop music
	S_StopMusic();

	// Stop all channels
	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		if (channel->state == CS_FREE)
			continue;

		S_StopChannel(channel);
	}
}

/*
 ==================
 S_UpdateSounds
 ==================
*/
static void S_UpdateSounds (){

	channel_t	*channel;
	int			i;

	QAL_LogPrintf("---------- S_UpdateSounds ----------\n");

	// Set the AL state
	qalDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

	// Flush raw samples
	S_FlushRawSamples(false);

	// Update music
	S_UpdateMusic();

	// Update listener
	S_UpdateListener();

	// Update looping sounds
	S_UpdateLoopingSounds();

	// Issue playSounds
	S_IssuePlaySounds();

	// Update all channels
	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		if (channel->state == CS_FREE)
			continue;

		snd.pc.channels++;

		S_UpdateChannel(channel);
	}

	// Check for errors
	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();

	QAL_LogPrintf("--------------------\n\n\n");
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 S_PlaySound_f
 ==================
*/
static void S_PlaySound_f (){

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: playSound <name>\n");
		return;
	}

	S_PlayLocalSound(S_FindSound(Cmd_Argv(1), 0));
}

/*
 ==================
 S_StopSounds_f
 ==================
*/
static void S_StopSounds_f (){

	S_StopAllSounds();
}

/*
 ==================
 S_ListSoundDevices_f
 ==================
*/
static void S_ListSoundDevices_f (){

	const char	*ptr = alConfig.deviceList;
	int			count = 0;

	while (*ptr){
		Com_Printf("%s\n", ptr);

		ptr += Str_Length(ptr) + 1;
		count++;
	}

	Com_Printf("--------------------\n");
	Com_Printf("%i sound devices available\n", count);
}

/*
 ==================
 S_ListSoundCaptureDevices_f
 ==================
*/
static void S_ListSoundCaptureDevices_f (){

	const char	*ptr = alConfig.captureDeviceList;
	int			count = 0;

	while (*ptr){
		Com_Printf("%s\n", ptr);

		ptr += Str_Length(ptr) + 1;
		count++;
	}

	Com_Printf("--------------------\n");
	Com_Printf("%i sound capture devices available\n", count);
}

/*
 ==================
 S_SfxInfo_f
 ==================
*/
static void S_SfxInfo_f (){

	Com_Printf("\n");
	Com_Printf("AL_VENDOR: %s\n", alConfig.vendorString);
	Com_Printf("AL_RENDERER: %s\n", alConfig.rendererString);
	Com_Printf("AL_VERSION: %s\n", alConfig.versionString);
	Com_Printf("AL_EXTENSIONS: %s\n", alConfig.extensionsString);
	Com_Printf("ALC_EXTENSIONS: %s\n", alConfig.alcExtensionsString);
	Com_Printf("\n");
	Com_Printf("AL_MAX_AUXILIARY_SENDS: %i\n", alConfig.maxAuxiliarySends);
	Com_Printf("\n");
	Com_Printf("DEVICE: %s\n", alConfig.deviceName);
	Com_Printf("CAPTURE DEVICE: %s\n", (alConfig.captureDeviceName) ? alConfig.captureDeviceName : "none");
	Com_Printf("MIXER: %i Hz (%s refresh)\n", alConfig.mixerFrequency, (alConfig.mixerSync) ? "synchronous" : Str_VarArgs("%i Hz", alConfig.mixerRefresh));
	Com_Printf("SOURCES: %i mono, %i stereo\n", alConfig.monoSources, alConfig.stereoSources);
	Com_Printf("CPU: %s\n", Sys_GetProcessorString());
	Com_Printf("\n");

	if (alConfig.eaxRAMAvailable)
		Com_Printf("Using EAX-RAM (%.2f MB total, %.2f MB free)\n", qalGetInteger(AL_EAX_RAM_SIZE) * (1.0f / 1048576.0f), qalGetInteger(AL_EAX_RAM_FREE) * (1.0f / 1048576.0f));
	else
		Com_Printf("EAX-RAM not available\n");

	Com_Printf("\n");
}


/*
 ==============================================================================

 CONFIG REGISTRATION

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void S_Register (){

	// Register variables
	s_logFile = CVar_Register("s_logFile", "0", CVAR_INTEGER, CVAR_CHEAT, "Number of frames to log AL calls", 0, 0);
	s_ignoreALErrors = CVar_Register("s_ignoreALErrors", "1", CVAR_BOOL, CVAR_CHEAT, "Ignore AL errors", 0, 0);
	s_singleSoundShader = CVar_Register("s_singleSoundShader", "0", CVAR_BOOL, CVAR_CHEAT | CVAR_LATCH, "Use a single default sound shader on every emitter", 0, 0);
	s_showStreaming = CVar_Register("s_showStreaming", "0", CVAR_BOOL, CVAR_CHEAT, "Show streaming sounds activity", 0, 0);
	s_skipStreaming = CVar_Register("s_skipStreaming", "0", CVAR_BOOL, CVAR_CHEAT, "Skip playing streaming sounds", 0, 0);
	s_skipShakes = CVar_Register("s_skipShakes", "0", CVAR_BOOL, CVAR_CHEAT, "Skip sound shakes", 0, 0);
	s_skipSpatialization = CVar_Register("s_skipSpatialization", "0", CVAR_BOOL, CVAR_CHEAT, "Skip sound spatialization", 0, 0);
	s_alDriver = CVar_Register("s_alDriver", "", CVAR_STRING, CVAR_ARCHIVE | CVAR_LATCH, "AL driver", 0, 0);
	s_deviceName = CVar_Register("s_deviceName", "", CVAR_STRING, CVAR_ARCHIVE | CVAR_LATCH, "Sound device name", 0, 0);
	s_captureDeviceName = CVar_Register("s_captureDeviceName", "", CVAR_STRING, CVAR_ARCHIVE | CVAR_LATCH, "Sound capture device name", 0, 0);
	s_masterVolume = CVar_Register("s_masterVolume", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Master volume", 0.0f, 1.0f);
	s_maxChannels = CVar_Register("s_maxChannels", "64", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Maximum number of mixing channels", 32, MAX_SOUND_CHANNELS);
	s_musicVolume = CVar_Register("s_musicVolume", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Music volume", 0.0f, 1.0f);
	s_voiceCapture = CVar_Register("s_voiceCapture", "0", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Enable voice capture", 0, 0);
	s_maxSoundsPerShader = CVar_Register("s_maxSoundsPerShader", "2", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Maximum number of sounds per shader", 1, MAX_SOUNDS_PER_SHADER);
	s_soundQuality = CVar_Register("s_soundQuality", "1", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Sound quality (0 = low, 1 = medium, 2 = high)", 0, 2);
	s_playDefaultSound = CVar_Register("s_playDefaultSound", "1", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Play a beep for missing sounds", 0, 0);

	// Add commands
	Cmd_AddCommand("playSound", S_PlaySound_f, "Plays a sound", Cmd_ArgCompletion_SoundName);
	Cmd_AddCommand("stopSounds", S_StopSounds_f, "Stops all playing sounds", NULL);
	Cmd_AddCommand("listSoundDevices", S_ListSoundDevices_f, "Lists sound devices", NULL);
	Cmd_AddCommand("listSoundCaptureDevices", S_ListSoundCaptureDevices_f, "Lists sound capture devices", NULL);
	Cmd_AddCommand("sfxInfo", S_SfxInfo_f, "Shows sound information", NULL);
}

/*
 ==================
 S_Unregister
 ==================
*/
static void S_Unregister (){

	// Remove commands
	Cmd_RemoveCommand("playSound");
	Cmd_RemoveCommand("stopSounds");
	Cmd_RemoveCommand("listSoundDevices");
	Cmd_RemoveCommand("listSoundCaptureDevices");
	Cmd_RemoveCommand("sfxInfo");
}


/*
 ==============================================================================

 FRAMEWORK

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void S_ShowSounds (){

}

/*
 ==================
 
 ==================
*/
static void S_PerformanceCounters (){

}

/*
 ==================
 S_Update
 ==================
*/
void S_Update (int time){

	int		timeSound;

	if (com_speeds->integerValue)
		timeSound = Sys_Milliseconds();

	// Log file
	if (s_logFile->modified){
		QAL_EnableLogging(s_logFile->integerValue);

		s_logFile->modified = false;
	}

	// Set the time
	snd.time = time;

	// Update all active sounds
	S_UpdateSounds();

	// Development tool
	S_ShowSounds();

	// Look at the performance counters
	S_PerformanceCounters();

	// Log file
	if (s_logFile->integerValue > 0)
		CVar_SetInteger(s_logFile, s_logFile->integerValue - 1);

	if (com_speeds->integerValue)
		com_timeSound += (Sys_Milliseconds() - timeSound);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 S_Init
 ==================
*/
void S_Init (bool all){

	Com_Printf("-------- Sound Initialization --------\n");

	if (all){
		// Register commands and variables
		S_Register();

		// Initialize OpenAL subsystem
		ALImp_Init();
	}

	// Set active
	snd.active = Sys_IsWindowActive();

	// Set master volume
	if (!snd.active)
		qalListenerf(AL_GAIN, 0.0f);
	else
		qalListenerf(AL_GAIN, s_masterVolume->floatValue);

	// Initialize all the sound system modules
	S_InitSounds();
	S_InitSoundShaders();
	S_InitMusic();
	S_InitRawSamples();
	S_InitChannels();

	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();

	Com_Printf("--------------------------------------\n");
}

/*
 ==================
 S_Shutdown
 ==================
*/
void S_Shutdown (bool all){

	if (!alConfig.initialized)
		return;

	// Shutdown all the sound system modules
	S_ShutdownChannels();
	S_ShutdownRawSamples();
	S_ShutdownMusic();
	S_ShutdownSoundShaders();
	S_ShutdownSounds();

	if (all){
		// Unregister commands
		S_Unregister();

		// Shutdown OpenAL subsystem
		ALImp_Shutdown();
	}

	// Free all sound system allocations
	Mem_FreeAll(TAG_SOUND, false);

	Mem_Fill(&snd, 0, sizeof(sndGlobals_t));
}