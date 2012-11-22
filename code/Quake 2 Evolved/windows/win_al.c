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
// win_al.c - Windows-specific OpenAL code
//


#include "../sound/s_local.h"
#include "win_local.h"


typedef struct {
	ALCdevice *				hDevice;
	ALCdevice *				hCaptureDevice;

	ALCcontext *			hALC;
} alwState_t;

static alwState_t			alwState;


/*
 ==============================================================================

 EXTENSIONS

 ==============================================================================
*/


/*
 ==================
 ALW_IsExtensionPresent
 ==================
*/
static bool ALW_IsExtensionPresent (ALCdevice *hDevice, const char *extName){

	if (hDevice)
		return qalcIsExtensionPresent(hDevice, extName);

	return qalIsExtensionPresent(extName);
}

/*
 ==================
 ALW_GetProcAddress
 ==================
*/
static void *ALW_GetProcAddress (ALCdevice *hDevice, const char *procName){

	void	*procAddress;

	if (hDevice){
		procAddress = qalcGetProcAddress(hDevice, procName);
		if (!procAddress){
			ALW_Shutdown();

			Com_Error(ERR_FATAL, "ALW_GetProcAddress: alcGetProcAddress() failed for '%s'", procName);
		}

		return procAddress;
	}

	procAddress = qalGetProcAddress(procName);
	if (!procAddress){
		ALW_Shutdown();

		Com_Error(ERR_FATAL, "ALW_GetProcAddress: alGetProcAddress() failed for '%s'", procName);
	}

	return procAddress;
}

/*
 ==================
 ALW_InitExtensions
 ==================
*/
static void ALW_InitExtensions (){

	Com_Printf("Initializing OpenAL extensions\n");

	// Initialize the extensions
	if (ALW_IsExtensionPresent(alwState.hDevice, "ALC_EXT_EFX")){
		alConfig.efxAvailable = true;

		qalcGetIntegerv(alwState.hDevice, ALC_MAX_AUXILIARY_SENDS, 1, &alConfig.maxAuxiliarySends);

		qalAuxiliaryEffectSlotf			= (ALAUXILIARYEFFECTSLOTF)ALW_GetProcAddress(NULL, "alAuxiliaryEffectSlotf");
		qalAuxiliaryEffectSlotfv		= (ALAUXILIARYEFFECTSLOTFV)ALW_GetProcAddress(NULL, "alAuxiliaryEffectSlotfv");
		qalAuxiliaryEffectSloti			= (ALAUXILIARYEFFECTSLOTI)ALW_GetProcAddress(NULL, "alAuxiliaryEffectSloti");
		qalAuxiliaryEffectSlotiv		= (ALAUXILIARYEFFECTSLOTIV)ALW_GetProcAddress(NULL, "alAuxiliaryEffectSlotiv");
		qalDeleteAuxiliaryEffectSlots	= (ALDELETEAUXILIARYEFFECTSLOTS)ALW_GetProcAddress(NULL, "alDeleteAuxiliaryEffectSlots");
		qalDeleteEffects				= (ALDELETEEFFECTS)ALW_GetProcAddress(NULL, "alDeleteEffects");
		qalDeleteFilters				= (ALDELETEFILTERS)ALW_GetProcAddress(NULL, "alDeleteFilters");
		qalEffectf						= (ALEFFECTF)ALW_GetProcAddress(NULL, "alEffectf");
		qalEffectfv						= (ALEFFECTFV)ALW_GetProcAddress(NULL, "alEffectfv");
		qalEffecti						= (ALEFFECTI)ALW_GetProcAddress(NULL, "alEffecti");
		qalEffectiv						= (ALEFFECTIV)ALW_GetProcAddress(NULL, "alEffectiv");
		qalFilterf						= (ALFILTERF)ALW_GetProcAddress(NULL, "alFilterf");
		qalFilterfv						= (ALFILTERFV)ALW_GetProcAddress(NULL, "alFilterfv");
		qalFilteri						= (ALFILTERI)ALW_GetProcAddress(NULL, "alFilteri");
		qalFilteriv						= (ALFILTERIV)ALW_GetProcAddress(NULL, "alFilteriv");
		qalGenAuxiliaryEffectSlots		= (ALGENAUXILIARYEFFECTSLOTS)ALW_GetProcAddress(NULL, "alGenAuxiliaryEffectSlots");
		qalGenEffects					= (ALGENEFFECTS)ALW_GetProcAddress(NULL, "alGenEffects");
		qalGenFilters					= (ALGENFILTERS)ALW_GetProcAddress(NULL, "alGenFilters");
		qalGetAuxiliaryEffectSlotf		= (ALGETAUXILIARYEFFECTSLOTF)ALW_GetProcAddress(NULL, "alGetAuxiliaryEffectSlotf");
		qalGetAuxiliaryEffectSlotfv		= (ALGETAUXILIARYEFFECTSLOTFV)ALW_GetProcAddress(NULL, "alGetAuxiliaryEffectSlotfv");
		qalGetAuxiliaryEffectSloti		= (ALGETAUXILIARYEFFECTSLOTI)ALW_GetProcAddress(NULL, "alGetAuxiliaryEffectSloti");
		qalGetAuxiliaryEffectSlotiv		= (ALGETAUXILIARYEFFECTSLOTIV)ALW_GetProcAddress(NULL, "alGetAuxiliaryEffectSlotiv");
		qalGetEffectf					= (ALGETEFFECTF)ALW_GetProcAddress(NULL, "alGetEffectf");
		qalGetEffectfv					= (ALGETEFFECTFV)ALW_GetProcAddress(NULL, "alGetEffectfv");
		qalGetEffecti					= (ALGETEFFECTI)ALW_GetProcAddress(NULL, "alGetEffecti");
		qalGetEffectiv					= (ALGETEFFECTIV)ALW_GetProcAddress(NULL, "alGetEffectiv");
		qalGetFilterf					= (ALGETFILTERF)ALW_GetProcAddress(NULL, "alGetFilterf");
		qalGetFilterfv					= (ALGETFILTERFV)ALW_GetProcAddress(NULL, "alGetFilterfv");
		qalGetFilteri					= (ALGETFILTERI)ALW_GetProcAddress(NULL, "alGetFilteri");
		qalGetFilteriv					= (ALGETFILTERIV)ALW_GetProcAddress(NULL, "alGetFilteriv");
		qalIsAuxiliaryEffectSlot		= (ALISAUXILIARYEFFECTSLOT)ALW_GetProcAddress(NULL, "alIsAuxiliaryEffectSlot");
		qalIsEffect						= (ALISEFFECT)ALW_GetProcAddress(NULL, "alIsEffect");
		qalIsFilter						= (ALISFILTER)ALW_GetProcAddress(NULL, "alIsFilter");

		Com_Printf("...using ALC_EXT_EFX\n");
	}
	else
		Com_Printf("...ALC_EXT_EFX not found\n");

	if (ALW_IsExtensionPresent(NULL, "EAX-RAM")){
		alConfig.eaxRAMAvailable = true;

		AL_EAX_RAM_SIZE					= qalGetEnumValue("AL_EAX_RAM_SIZE");
		AL_EAX_RAM_FREE					= qalGetEnumValue("AL_EAX_RAM_FREE");
		AL_STORAGE_AUTOMATIC			= qalGetEnumValue("AL_STORAGE_AUTOMATIC");
		AL_STORAGE_HARDWARE				= qalGetEnumValue("AL_STORAGE_HARDWARE");
		AL_STORAGE_ACCESSIBLE			= qalGetEnumValue("AL_STORAGE_ACCESSIBLE");

		qalEAXGetBufferMode				= (ALEAXGETBUFFERMODE)ALW_GetProcAddress(NULL, "EAXGetBufferMode");
		qalEAXSetBufferMode				= (ALEAXSETBUFFERMODE)ALW_GetProcAddress(NULL, "EAXSetBufferMode");

		Com_Printf("...using EAX-RAM\n");
	}
	else
		Com_Printf("...EAX-RAM not found\n");
}


/*
 ==============================================================================

 DRIVER INITIALIZATION

 ==============================================================================
*/


/*
 ==================
 ALW_InitDriver
 ==================
*/
static bool ALW_InitDriver (){

	const char	*deviceName;
	int			attribs[] = {ALC_MONO_SOURCES, MAX_SOUND_CHANNELS, ALC_STEREO_SOURCES, 2, 0};

	Com_Printf("Initializing OpenAL driver\n");

	// Open the device
	Com_Printf("...opening device: ");

	if (s_deviceName->value[0])
		deviceName = s_deviceName->value;
	else {
		deviceName = qalcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

		// The "Generic Hardware" device tends to have issues due to very
		// crappy DirectSound3D drivers, so default to the "Generic Software"
		// device. This only affects Windows XP systems.
		if (!Str_ICompare(deviceName, "Generic Hardware"))
			deviceName = "Generic Software";
	}

	if ((alwState.hDevice = qalcOpenDevice(deviceName)) == NULL){
		Com_Printf("failed\n");

		// Try the software device, unless we just tried it and failed
		if (!Str_ICompare(deviceName, "Generic Software"))
			return false;

		Com_Printf("...trying software device: ");

		if ((alwState.hDevice = qalcOpenDevice("Generic Software")) == NULL){
			Com_Printf("failed\n");
			return false;
		}
	}

	Com_Printf("succeeded\n");

	// Open the capture device if desired
	if (s_voiceCapture->integerValue){
		Com_Printf("...opening capture device: ");

		if (s_captureDeviceName->value[0])
			deviceName = s_captureDeviceName->value;
		else
			deviceName = qalcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);

		if ((alwState.hCaptureDevice = qalcCaptureOpenDevice(deviceName, VOICE_RATE, AL_FORMAT_MONO16, MAX_VOICE_SAMPLES)) == NULL)
			Com_Printf("failed\n");
		else
			Com_Printf("succeeded\n");
	}

	// Create the AL context
	Com_Printf("...creating AL context: ");

	if ((alwState.hALC = qalcCreateContext(alwState.hDevice, attribs)) == NULL){
		Com_Printf("failed\n");

		if (alwState.hCaptureDevice){
			qalcCaptureCloseDevice(alwState.hCaptureDevice);
			alwState.hCaptureDevice = NULL;
		}

		qalcCloseDevice(alwState.hDevice);
		alwState.hDevice = NULL;

		return false;
	}

	Com_Printf("succeeded\n");

	// Make the AL context current
	Com_Printf("...making context current: ");

	if (!qalcMakeContextCurrent(alwState.hALC)){
		Com_Printf("failed\n");

		qalcDestroyContext(alwState.hALC);
		alwState.hALC = NULL;

		if (alwState.hCaptureDevice){
			qalcCaptureCloseDevice(alwState.hCaptureDevice);
			alwState.hCaptureDevice = NULL;
		}

		qalcCloseDevice(alwState.hDevice);
		alwState.hDevice = NULL;

		return false;
	}

	Com_Printf("succeeded\n");

	return true;
}


// ============================================================================


/*
 ==================
 ALW_CheckVersion
 ==================
*/
static bool ALW_CheckVersion (int major, int minor){

	int		majorVersion, minorVersion;

	qalcGetIntegerv(alwState.hDevice, ALC_MAJOR_VERSION, 1, &majorVersion);
	qalcGetIntegerv(alwState.hDevice, ALC_MINOR_VERSION, 1, &minorVersion);

	if (majorVersion < major || (majorVersion == major && minorVersion < minor))
		return false;

	return true;
}

/*
 ==================
 ALW_StartCapture
 ==================
*/
void ALW_StartCapture (){

	if (!alwState.hCaptureDevice)
		return;

	qalcCaptureStart(alwState.hCaptureDevice);
}

/*
 ==================
 ALW_StopCapture
 ==================
*/
void ALW_StopCapture (bool flush){

	short	dummy[MAX_VOICE_SAMPLES];
	int		samples;

	if (!alwState.hCaptureDevice)
		return;

	qalcCaptureStop(alwState.hCaptureDevice);

	// Flush the captured samples if desired
	if (flush){
		qalcGetIntegerv(alwState.hCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &samples);
		if (!samples)
			return;

		qalcCaptureSamples(alwState.hCaptureDevice, dummy, samples);
	}
}

/*
 ==================
 ALW_AvailableCaptureSamples
 ==================
*/
int ALW_AvailableCaptureSamples (){

	int		samples;

	if (!alwState.hCaptureDevice)
		return 0;

	qalcGetIntegerv(alwState.hCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &samples);

	return samples;
}

/*
 ==================
 ALW_CaptureSamples
 ==================
*/
void ALW_CaptureSamples (void *buffer, int samples){

	if (!alwState.hCaptureDevice)
		return;

	qalcCaptureSamples(alwState.hCaptureDevice, buffer, samples);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 ALW_Init
 ==================
*/
void ALW_Init (){

	const char	*driver;

	Com_Printf("Initializing OpenAL subsystem\n");

	// Initialize our QAL dynamic bindings
	if (s_alDriver->value[0])
		driver = s_alDriver->value;
	else
		driver = AL_DRIVER_OPENAL;

	if (!QAL_Init(driver))
		Com_Error(ERR_FATAL, "ALW_Init: could not load OpenAL subsystem");

	// Get device lists
	if (qalcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT")){
		alConfig.deviceList = qalcGetString(NULL, ALC_DEVICE_SPECIFIER);
		alConfig.captureDeviceList = qalcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
	}
	else {
		alConfig.deviceList = "Generic Software\0";
		alConfig.captureDeviceList = "\0";
	}

	// Initialize the device, context, etc
	if (!ALW_InitDriver()){
		QAL_Shutdown();

		Com_Error(ERR_FATAL, "ALW_Init: could not load OpenAL subsystem");
	}

	// Check for OpenAL 1.1 support
	if (!ALW_CheckVersion(1, 1)){
		ALW_Shutdown();

		Com_Error(ERR_FATAL, "The current sound card / driver combination does not support OpenAL 1.1");
	}

	// Get AL strings
	alConfig.vendorString = qalGetString(AL_VENDOR);
	alConfig.rendererString = qalGetString(AL_RENDERER);
	alConfig.versionString = qalGetString(AL_VERSION);
	alConfig.extensionsString = qalGetString(AL_EXTENSIONS);

	// Get ALC strings
	alConfig.alcExtensionsString = qalcGetString(alwState.hDevice, ALC_EXTENSIONS);

	// Get device names
	alConfig.deviceName = qalcGetString(alwState.hDevice, ALC_DEVICE_SPECIFIER);

	if (alwState.hCaptureDevice)
		alConfig.captureDeviceName = qalcGetString(alwState.hCaptureDevice, ALC_CAPTURE_DEVICE_SPECIFIER);
	else
		alConfig.captureDeviceName = NULL;

	// Get AL implementation details
	qalcGetIntegerv(alwState.hDevice, ALC_FREQUENCY, 1, &alConfig.mixerFrequency);
	qalcGetIntegerv(alwState.hDevice, ALC_REFRESH, 1, &alConfig.mixerRefresh);
	qalcGetIntegerv(alwState.hDevice, ALC_SYNC, 1, &alConfig.mixerSync);
	qalcGetIntegerv(alwState.hDevice, ALC_MONO_SOURCES, 1, &alConfig.monoSources);
	qalcGetIntegerv(alwState.hDevice, ALC_STEREO_SOURCES, 1, &alConfig.stereoSources);

	// Determine AL hardware type
	if (Str_FindText(alConfig.vendorString, "Creative Labs", false))
		alConfig.hardwareType = ALHW_CREATIVE;
	else
		alConfig.hardwareType = ALHW_GENERIC;

	// Initialize extensions
	ALW_InitExtensions();

	// Enable logging if requested
	QAL_EnableLogging(s_logFile->integerValue);

	// OpenAL fully initialized
	alConfig.initialized = true;
}

/*
 ==================
 ALW_Shutdown
 ==================
*/
void ALW_Shutdown (){

	Com_Printf("Shutting down OpenAL subsystem\n");

	if (alwState.hALC){
		Com_Printf("...alcMakeContextCurrent( NULL ): ");

		if (!qalcMakeContextCurrent(NULL))
			Com_Printf("failed\n");
		else
			Com_Printf("succeeded\n");

		Com_Printf("...destroying AL context\n");

		qalcDestroyContext(alwState.hALC);
		alwState.hALC = NULL;
	}

	if (alwState.hCaptureDevice){
		Com_Printf("...closing capture device: ");

		if (!qalcCaptureCloseDevice(alwState.hCaptureDevice))
			Com_Printf("failed\n");
		else
			Com_Printf("succeeded\n");

		alwState.hCaptureDevice = NULL;
	}

	if (alwState.hDevice){
		Com_Printf("...closing device: ");

		if (!qalcCloseDevice(alwState.hDevice))
			Com_Printf("failed\n");
		else
			Com_Printf("succeeded\n");

		alwState.hDevice = NULL;
	}

	QAL_Shutdown();

	Mem_Fill(&alConfig, 0, sizeof(alConfig_t));
	Mem_Fill(&alwState, 0, sizeof(alwState_t));
}