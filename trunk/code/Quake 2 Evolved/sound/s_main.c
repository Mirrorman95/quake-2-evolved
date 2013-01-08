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
cvar_t *					s_speedOfSound;
cvar_t *					s_reverbTime;
cvar_t *					s_obstructionScale;
cvar_t *					s_exclusionScale;
cvar_t *					s_occlusionScale;
cvar_t *					s_singleSoundShader;
cvar_t *					s_singleEmitter;
cvar_t *					s_showUpdates;
cvar_t *					s_showEmitters;
cvar_t *					s_showStreaming;
cvar_t *					s_showChannels;
cvar_t *					s_showPortals;
cvar_t *					s_showSounds;
cvar_t *					s_skipUpdates;
cvar_t *					s_skipEmitters;
cvar_t *					s_skipStreaming;
cvar_t *					s_skipShakes;
cvar_t *					s_skipFlicker;
cvar_t *					s_skipSpatialization;
cvar_t *					s_skipAttenuation;
cvar_t *					s_skipCones;
cvar_t *					s_skipPortals;
cvar_t *					s_skipDynamic;
cvar_t *					s_skipObstructions;
cvar_t *					s_skipExclusions;
cvar_t *					s_skipOcclusions;
cvar_t *					s_skipReverbs;
cvar_t *					s_skipFilters;
cvar_t *					s_alDriver;
cvar_t *					s_deviceName;
cvar_t *					s_captureDeviceName;
cvar_t *					s_masterVolume;
cvar_t *					s_emitterVolume;
cvar_t *					s_reverbVolume;
cvar_t *					s_musicVolume;
cvar_t *					s_voiceVolume;
cvar_t *					s_dopplerShifts;
cvar_t *					s_airAbsorption;
cvar_t *					s_reverbEffects;
cvar_t *					s_lowPassFilters;
cvar_t *					s_voiceCapture;
cvar_t *					s_voiceScale;
cvar_t *					s_voiceLatency;
cvar_t *					s_maxChannels;
cvar_t *					s_maxSoundsPerShader;
cvar_t *					s_soundQuality;
cvar_t *					s_playDefaultSound;
cvar_t *					s_muteOnLostFocus;


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
 S_PlayLocalSound
 ==================
*/
void S_PlayLocalSound (soundShader_t *soundShader){

	channel_t	*channel;

	// Pick a channel and play the sound
	channel = S_PickChannel(snd.localEmitter, snd.localEmitter->e.emitterId, SOUND_CHANNEL_ANY, soundShader);
	if (!channel)
		return;

	S_PlayChannel(channel, true);
}

/*
 ==================
 S_StopAllSounds
 ==================
*/
void S_StopAllSounds (){

	channel_t	*channel;
	int			i;

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

	S_PlayLocalSound(S_FindSoundShader(Cmd_Argv(1)));
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

	if (alConfig.efxAvailable){
		if (snd.reverb.enabled)
			Com_Printf("Using reverb effects\n");
		else
			Com_Printf("Reverb effects are disabled\n");

		if (snd.filter.enabled)
			Com_Printf("Using low-pass filters\n");
		else
			Com_Printf("Low-pass filters are disabled\n");
	}
	else {
		Com_Printf("Reverb effects not available\n");
		Com_Printf("Low-pass filters not available\n");
	}

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
 S_Register
 ==================
*/
static void S_Register (){

	// Register variables
	s_logFile = CVar_Register("s_logFile", "0", CVAR_INTEGER, CVAR_CHEAT, "Number of frames to log AL calls", 0, 0);
	s_ignoreALErrors = CVar_Register("s_ignoreALErrors", "1", CVAR_BOOL, CVAR_CHEAT, "Ignore AL errors", 0, 0);
	s_speedOfSound = CVar_Register("s_speedOfSound", "343.3", CVAR_FLOAT, CVAR_CHEAT, "Speed of sound (in meters per second)", 1.0f, 10000.0f);
	s_reverbTime = CVar_Register("s_reverbTime", "1.0", CVAR_FLOAT, CVAR_CHEAT, "Reverb transition time in seconds", 0.0f, 60.0f);
	s_obstructionScale = CVar_Register("s_obstructionScale", "1.0", CVAR_FLOAT, CVAR_CHEAT, "Sound obstruction scale factor", 0.0f, 10.0f);
	s_exclusionScale = CVar_Register("s_exclusionScale", "1.0", CVAR_FLOAT, CVAR_CHEAT, "Sound exclusion scale factor", 0.0f, 10.0f);
	s_occlusionScale = CVar_Register("s_occlusionScale", "1.0", CVAR_FLOAT, CVAR_CHEAT, "Sound occlusion scale factor", 0.0f, 10.0f);
	s_singleSoundShader = CVar_Register("s_singleSoundShader", "0", CVAR_BOOL, CVAR_CHEAT | CVAR_LATCH, "Use a single default sound shader on every emitter", 0, 0);
	s_singleEmitter = CVar_Register("s_singleEmitter", "-1", CVAR_INTEGER, CVAR_CHEAT, "Only play the specified emitter", -1, MAX_SOUND_EMITTERS - 1);
	s_showUpdates = CVar_Register("s_showUpdates", "0", CVAR_BOOL, CVAR_CHEAT, "Show number of emitter updates", 0, 0);
	s_showEmitters = CVar_Register("s_showEmitters", "0", CVAR_BOOL, CVAR_CHEAT, "Show sound emitters activity", 0, 0);
	s_showStreaming = CVar_Register("s_showStreaming", "0", CVAR_BOOL, CVAR_CHEAT, "Show streaming sounds activity", 0, 0);
	s_showChannels = CVar_Register("s_showChannels", "0", CVAR_BOOL, CVAR_CHEAT, "Show number of active channels", 0, 0);
	s_showPortals = CVar_Register("s_showPortals", "0", CVAR_BOOL, CVAR_CHEAT, "Show number of portals checked during flowing", 0, 0);
	s_showSounds = CVar_Register("s_showSounds", "0", CVAR_INTEGER, CVAR_CHEAT, "Draw active sounds (1 = draw audible ones, 2 = draw everything)", 0, 2);
	s_skipUpdates = CVar_Register("s_skipUpdates", "0", CVAR_BOOL, CVAR_CHEAT, "Skip emitter updates, making everything static", 0, 0);
	s_skipEmitters = CVar_Register("s_skipEmitters", "0", CVAR_BOOL, CVAR_CHEAT, "Skip playing sound emitters", 0, 0);
	s_skipStreaming = CVar_Register("s_skipStreaming", "0", CVAR_BOOL, CVAR_CHEAT, "Skip playing streaming sounds", 0, 0);
	s_skipShakes = CVar_Register("s_skipShakes", "0", CVAR_BOOL, CVAR_CHEAT, "Skip sound shakes", 0, 0);
	s_skipFlicker = CVar_Register("s_skipFlicker", "0", CVAR_BOOL, CVAR_CHEAT, "Skip sound flicker", 0, 0);
	s_skipSpatialization = CVar_Register("s_skipSpatialization", "0", CVAR_BOOL, CVAR_CHEAT, "Skip sound spatialization", 0, 0);
	s_skipAttenuation = CVar_Register("s_skipAttenuation", "0", CVAR_BOOL, CVAR_CHEAT, "Skip sound distance attenuation", 0, 0);
	s_skipCones = CVar_Register("s_skipCones", "0", CVAR_BOOL, CVAR_CHEAT, "Skip directional sound cones", 0, 0);
	s_skipPortals = CVar_Register("s_skipPortals", "0", CVAR_BOOL, CVAR_CHEAT, "Skip sound portal flowing", 0, 0);
	s_skipDynamic = CVar_Register("s_skipDynamic", "0", CVAR_BOOL, CVAR_CHEAT, "Skip dynamically modifying sounds", 0, 0);
	s_skipObstructions = CVar_Register("s_skipObstructions", "0", CVAR_BOOL, CVAR_CHEAT, "Skip sound obstructions", 0, 0);
	s_skipExclusions = CVar_Register("s_skipExclusions", "0", CVAR_BOOL, CVAR_CHEAT, "Skip sound exclusions", 0, 0);
	s_skipOcclusions = CVar_Register("s_skipOcclusions", "0", CVAR_BOOL, CVAR_CHEAT, "Skip sound occlusions", 0, 0);
	s_skipReverbs = CVar_Register("s_skipReverbs", "0", CVAR_BOOL, CVAR_CHEAT, "Skip reverb effects", 0, 0);
	s_skipFilters = CVar_Register("s_skipFilters", "0", CVAR_BOOL, CVAR_CHEAT, "Skip low-pass filters", 0, 0);
	s_alDriver = CVar_Register("s_alDriver", "", CVAR_STRING, CVAR_ARCHIVE | CVAR_LATCH, "AL driver", 0, 0);
	s_deviceName = CVar_Register("s_deviceName", "", CVAR_STRING, CVAR_ARCHIVE | CVAR_LATCH, "Sound device name", 0, 0);
	s_captureDeviceName = CVar_Register("s_captureDeviceName", "", CVAR_STRING, CVAR_ARCHIVE | CVAR_LATCH, "Sound capture device name", 0, 0);
	s_masterVolume = CVar_Register("s_masterVolume", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Master volume", 0.0f, 1.0f);
	s_emitterVolume = CVar_Register("s_emitterVolume", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Emitter volume", 0.0f, 1.0f);
	s_reverbVolume = CVar_Register("s_reverbVolume", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Reverb volume", 0.0f, 1.0f);
	s_musicVolume = CVar_Register("s_musicVolume", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Music volume", 0.0f, 1.0f);
	s_voiceVolume = CVar_Register("s_voiceVolume", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Voice volume", 0.0f, 1.0f);
	s_dopplerShifts = CVar_Register("s_dopplerShifts", "1", CVAR_BOOL, CVAR_ARCHIVE, "Enable doppler shifts", 0, 0);
	s_airAbsorption = CVar_Register("s_airAbsorption", "0", CVAR_BOOL, CVAR_ARCHIVE, "Enable air absorption", 0, 0);
	s_reverbEffects = CVar_Register("s_reverbEffects", "0", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Enable reverb effects", 0, 0);
	s_lowPassFilters = CVar_Register("s_lowPassFilters", "0", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Enable low-pass filters", 0, 0);
	s_voiceCapture = CVar_Register("s_voiceCapture", "0", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Enable voice capture", 0, 0);
	s_voiceScale = CVar_Register("s_voiceScale", "2.0", CVAR_FLOAT, CVAR_ARCHIVE, "Voice amplitude scale factor", 0.0f, 5.0f);
	s_voiceLatency = CVar_Register("s_voiceLatency", "0", CVAR_INTEGER, CVAR_ARCHIVE, "Latency in milliseconds for voice playback", 0, 1000);
	s_maxChannels = CVar_Register("s_maxChannels", "64", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Maximum number of mixing channels", 32, MAX_SOUND_CHANNELS);
	s_maxSoundsPerShader = CVar_Register("s_maxSoundsPerShader", "2", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Maximum number of sounds per shader", 1, MAX_SOUNDS_PER_SHADER);
	s_soundQuality = CVar_Register("s_soundQuality", "1", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Sound quality (0 = low, 1 = medium, 2 = high)", 0, 2);
	s_playDefaultSound = CVar_Register("s_playDefaultSound", "1", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Play a beep for missing sounds", 0, 0);
	s_muteOnLostFocus = CVar_Register("s_muteOnLostFocus", "1", CVAR_BOOL, CVAR_ARCHIVE, "Mute all sounds if focus is lost", 0, 0);

	// Add commands
	Cmd_AddCommand("playSound", S_PlaySound_f, "Plays a sound", Cmd_ArgCompletion_SoundShaderName);
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
	S_InitReverbs();
	S_InitMusic();
	S_InitRawSamples();
	S_InitEmitters();
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
	S_ShutdownEmitters();
	S_ShutdownRawSamples();
	S_ShutdownMusic();
	S_ShutdownReverbs();
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