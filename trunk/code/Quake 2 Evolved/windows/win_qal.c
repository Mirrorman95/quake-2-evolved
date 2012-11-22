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
// win_qal.c - Binding of AL to QAL function pointers
//


#include "../sound/s_local.h"
#include "win_local.h"


typedef struct {
	HMODULE					hModule;

	bool					pointersCopied;

	FILE *					logFile;
	char					logFileName[MAX_PATH_LENGTH];
	int						logFileFrames;
	int						logFileNumber;
} qalState_t;

static qalState_t			qalState;

ALenum						AL_EAX_RAM_SIZE;
ALenum						AL_EAX_RAM_FREE;
ALenum						AL_STORAGE_AUTOMATIC;
ALenum						AL_STORAGE_HARDWARE;
ALenum						AL_STORAGE_ACCESSIBLE;


// ============================================================================


ALCboolean					(ALCAPIENTRY * qalcCaptureCloseDevice)(ALCdevice *device);
ALCdevice *					(ALCAPIENTRY * qalcCaptureOpenDevice)(const ALCchar *deviceName, ALCuint frequency, ALCenum format, ALCsizei samples);
ALCvoid						(ALCAPIENTRY * qalcCaptureSamples)(ALCdevice *device, ALCvoid *buffer, ALCsizei samples);
ALCvoid						(ALCAPIENTRY * qalcCaptureStart)(ALCdevice *device);
ALCvoid						(ALCAPIENTRY * qalcCaptureStop)(ALCdevice *device);
ALCboolean					(ALCAPIENTRY * qalcCloseDevice)(ALCdevice *device);
ALCcontext *				(ALCAPIENTRY * qalcCreateContext)(ALCdevice *device, ALCint *attrList);
ALCvoid						(ALCAPIENTRY * qalcDestroyContext)(ALCcontext *context);
ALCdevice *					(ALCAPIENTRY * qalcGetContextsDevice)(ALCcontext *context);
ALCcontext *				(ALCAPIENTRY * qalcGetCurrentContext)(ALCvoid);
ALCenum						(ALCAPIENTRY * qalcGetEnumValue)(ALCdevice *device, const ALCchar *enumName);
ALCenum						(ALCAPIENTRY * qalcGetError)(ALCdevice *device);
ALCvoid						(ALCAPIENTRY * qalcGetIntegerv)(ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data);
ALCvoid *					(ALCAPIENTRY * qalcGetProcAddress)(ALCdevice *device, const ALCchar *funcName);
const ALCchar *				(ALCAPIENTRY * qalcGetString)(ALCdevice *device, ALCenum param);
ALCboolean					(ALCAPIENTRY * qalcIsExtensionPresent)(ALCdevice *device, const ALCchar *extName);
ALCboolean					(ALCAPIENTRY * qalcMakeContextCurrent)(ALCcontext *context);
ALCdevice *					(ALCAPIENTRY * qalcOpenDevice)(const ALCchar *deviceName);
ALCvoid						(ALCAPIENTRY * qalcProcessContext)(ALCcontext *context);
ALCvoid						(ALCAPIENTRY * qalcSuspendContext)(ALCcontext *context);

ALvoid						(ALAPIENTRY * qalBufferData)(ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq);
ALvoid						(ALAPIENTRY * qalBuffer3f)(ALuint buffer, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
ALvoid						(ALAPIENTRY * qalBuffer3i)(ALuint buffer, ALenum param, ALint value1, ALint value2, ALint value3);
ALvoid						(ALAPIENTRY * qalBufferf)(ALuint buffer, ALenum param, ALfloat value);
ALvoid						(ALAPIENTRY * qalBufferfv)(ALuint buffer, ALenum param, const ALfloat *values);
ALvoid						(ALAPIENTRY * qalBufferi)(ALuint buffer, ALenum param, ALint value);
ALvoid						(ALAPIENTRY * qalBufferiv)(ALuint buffer, ALenum param, const ALint *values);
ALvoid						(ALAPIENTRY * qalDeleteBuffers)(ALsizei n, const ALuint *buffers);
ALvoid						(ALAPIENTRY * qalDeleteSources)(ALsizei n, const ALuint *sources);
ALvoid						(ALAPIENTRY * qalDisable)(ALenum capability);
ALvoid						(ALAPIENTRY * qalDistanceModel)(ALenum distanceModel);
ALvoid						(ALAPIENTRY * qalDopplerFactor)(ALfloat value);
ALvoid						(ALAPIENTRY * qalDopplerVelocity)(ALfloat value);
ALvoid						(ALAPIENTRY * qalEnable)(ALenum capability);
ALvoid						(ALAPIENTRY * qalGenBuffers)(ALsizei n, ALuint *buffers);
ALvoid						(ALAPIENTRY * qalGenSources)(ALsizei n, ALuint *sources);
ALboolean					(ALAPIENTRY * qalGetBoolean)(ALenum param);
ALvoid						(ALAPIENTRY * qalGetBooleanv)(ALenum param, ALboolean *data);
ALvoid						(ALAPIENTRY * qalGetBuffer3f)(ALuint buffer, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
ALvoid						(ALAPIENTRY * qalGetBuffer3i)(ALuint buffer, ALenum param, ALint *value1, ALint *value2, ALint *value3);
ALvoid						(ALAPIENTRY * qalGetBufferf)(ALuint buffer, ALenum param, ALfloat *value);
ALvoid						(ALAPIENTRY * qalGetBufferfv)(ALuint buffer, ALenum param, ALfloat *values);
ALvoid						(ALAPIENTRY * qalGetBufferi)(ALuint buffer, ALenum param, ALint *value);
ALvoid						(ALAPIENTRY * qalGetBufferiv)(ALuint buffer, ALenum param, ALint *values);
ALdouble					(ALAPIENTRY * qalGetDouble)(ALenum param);
ALvoid						(ALAPIENTRY * qalGetDoublev)(ALenum param, ALdouble *data);
ALenum						(ALAPIENTRY * qalGetEnumValue)(const ALchar *enumName);
ALenum						(ALAPIENTRY * qalGetError)(ALvoid);
ALfloat						(ALAPIENTRY * qalGetFloat)(ALenum param);
ALvoid						(ALAPIENTRY * qalGetFloatv)(ALenum param, ALfloat *data);
ALint						(ALAPIENTRY * qalGetInteger)(ALenum param);
ALvoid						(ALAPIENTRY * qalGetIntegerv)(ALenum param, ALint *data);
ALvoid						(ALAPIENTRY * qalGetListener3f)(ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
ALvoid						(ALAPIENTRY * qalGetListener3i)(ALenum param, ALint *value1, ALint *value2, ALint *value3);
ALvoid						(ALAPIENTRY * qalGetListenerf)(ALenum param, ALfloat *value);
ALvoid						(ALAPIENTRY * qalGetListenerfv)(ALenum param, ALfloat *values);
ALvoid						(ALAPIENTRY * qalGetListeneri)(ALenum param, ALint *value);
ALvoid						(ALAPIENTRY * qalGetListeneriv)(ALenum param, ALint *values);
ALvoid *					(ALAPIENTRY * qalGetProcAddress)(const ALchar *funcName);
ALvoid						(ALAPIENTRY * qalGetSource3f)(ALuint source, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
ALvoid						(ALAPIENTRY * qalGetSource3i)(ALuint source, ALenum param, ALint *value1, ALint *value2, ALint *value3);
ALvoid						(ALAPIENTRY * qalGetSourcef)(ALuint source, ALenum param, ALfloat *value);
ALvoid						(ALAPIENTRY * qalGetSourcefv)(ALuint source, ALenum param, ALfloat *values);
ALvoid						(ALAPIENTRY * qalGetSourcei)(ALuint source, ALenum param, ALint *value);
ALvoid						(ALAPIENTRY * qalGetSourceiv)(ALuint source, ALenum param, ALint *values);
const ALchar *				(ALAPIENTRY * qalGetString)(ALenum param);
ALboolean					(ALAPIENTRY * qalIsBuffer)(ALuint buffer);
ALboolean					(ALAPIENTRY * qalIsEnabled)(ALenum capability);
ALboolean					(ALAPIENTRY * qalIsExtensionPresent)(const ALchar *extName);
ALboolean					(ALAPIENTRY * qalIsSource)(ALuint source);
ALvoid						(ALAPIENTRY * qalListener3f)(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
ALvoid						(ALAPIENTRY * qalListener3i)(ALenum param, ALint value1, ALint value2, ALint value3);
ALvoid						(ALAPIENTRY * qalListenerf)(ALenum param, ALfloat value);
ALvoid						(ALAPIENTRY * qalListenerfv)(ALenum param, const ALfloat *values);
ALvoid						(ALAPIENTRY * qalListeneri)(ALenum param, ALint value);
ALvoid						(ALAPIENTRY * qalListeneriv)(ALenum param, const ALint *values);
ALvoid						(ALAPIENTRY * qalSource3f)(ALuint source, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
ALvoid						(ALAPIENTRY * qalSource3i)(ALuint source, ALenum param, ALint value1, ALint value2, ALint value3);
ALvoid						(ALAPIENTRY * qalSourcef)(ALuint source, ALenum param, ALfloat value);
ALvoid						(ALAPIENTRY * qalSourcefv)(ALuint source, ALenum param, const ALfloat *values);
ALvoid						(ALAPIENTRY * qalSourcei)(ALuint source, ALenum param, ALint value);
ALvoid						(ALAPIENTRY * qalSourceiv)(ALuint source, ALenum param, const ALint *values);
ALvoid						(ALAPIENTRY * qalSourcePause)(ALuint source);
ALvoid						(ALAPIENTRY * qalSourcePausev)(ALsizei n, const ALuint *sources);
ALvoid						(ALAPIENTRY * qalSourcePlay)(ALuint source);
ALvoid						(ALAPIENTRY * qalSourcePlayv)(ALsizei n, const ALuint *sources);
ALvoid						(ALAPIENTRY * qalSourceQueueBuffers)(ALuint source, ALsizei n, const ALuint *buffers);
ALvoid						(ALAPIENTRY * qalSourceRewind)(ALuint source);
ALvoid						(ALAPIENTRY * qalSourceRewindv)(ALsizei n, const ALuint *sources);
ALvoid						(ALAPIENTRY * qalSourceStop)(ALuint source);
ALvoid						(ALAPIENTRY * qalSourceStopv)(ALsizei n, const ALuint *sources);
ALvoid						(ALAPIENTRY * qalSourceUnqueueBuffers)(ALuint source, ALsizei n, ALuint *buffers);
ALvoid						(ALAPIENTRY * qalSpeedOfSound)(ALfloat value);

ALvoid						(ALAPIENTRY * qalAuxiliaryEffectSlotf)(ALuint slot, ALenum param, ALfloat value);
ALvoid						(ALAPIENTRY * qalAuxiliaryEffectSlotfv)(ALuint slot, ALenum param, const ALfloat *values);
ALvoid						(ALAPIENTRY * qalAuxiliaryEffectSloti)(ALuint slot, ALenum param, ALint value);
ALvoid						(ALAPIENTRY * qalAuxiliaryEffectSlotiv)(ALuint slot, ALenum param, const ALint *values);
ALvoid						(ALAPIENTRY * qalDeleteAuxiliaryEffectSlots)(ALsizei n, const ALuint *slots);
ALvoid						(ALAPIENTRY * qalDeleteEffects)(ALsizei n, const ALuint *effects);
ALvoid						(ALAPIENTRY * qalDeleteFilters)(ALsizei n, const ALuint *filters);
ALvoid						(ALAPIENTRY * qalEffectf)(ALuint effect, ALenum param, ALfloat value);
ALvoid						(ALAPIENTRY * qalEffectfv)(ALuint effect, ALenum param, const ALfloat *values);
ALvoid						(ALAPIENTRY * qalEffecti)(ALuint effect, ALenum param, ALint value);
ALvoid						(ALAPIENTRY * qalEffectiv)(ALuint effect, ALenum param, const ALint *values);
ALvoid						(ALAPIENTRY * qalFilterf)(ALuint filter, ALenum param, ALfloat value);
ALvoid						(ALAPIENTRY * qalFilterfv)(ALuint filter, ALenum param, const ALfloat *values);
ALvoid						(ALAPIENTRY * qalFilteri)(ALuint filter, ALenum param, ALint value);
ALvoid						(ALAPIENTRY * qalFilteriv)(ALuint filter, ALenum param, const ALint *values);
ALvoid						(ALAPIENTRY * qalGenAuxiliaryEffectSlots)(ALsizei n, ALuint *slots);
ALvoid						(ALAPIENTRY * qalGenEffects)(ALsizei n, ALuint *effects);
ALvoid						(ALAPIENTRY * qalGenFilters)(ALsizei n, ALuint *filters);
ALvoid						(ALAPIENTRY * qalGetAuxiliaryEffectSlotf)(ALuint slot, ALenum param, ALfloat *value);
ALvoid						(ALAPIENTRY * qalGetAuxiliaryEffectSlotfv)(ALuint slot, ALenum param, ALfloat *values);
ALvoid						(ALAPIENTRY * qalGetAuxiliaryEffectSloti)(ALuint slot, ALenum param, ALint *value);
ALvoid						(ALAPIENTRY * qalGetAuxiliaryEffectSlotiv)(ALuint slot, ALenum param, ALint *values);
ALvoid						(ALAPIENTRY * qalGetEffectf)(ALuint effect, ALenum param, ALfloat *value);
ALvoid						(ALAPIENTRY * qalGetEffectfv)(ALuint effect, ALenum param, ALfloat *values);
ALvoid						(ALAPIENTRY * qalGetEffecti)(ALuint effect, ALenum param, ALint *value);
ALvoid						(ALAPIENTRY * qalGetEffectiv)(ALuint effect, ALenum param, ALint *values);
ALvoid						(ALAPIENTRY * qalGetFilterf)(ALuint filter, ALenum param, ALfloat *value);
ALvoid						(ALAPIENTRY * qalGetFilterfv)(ALuint filter, ALenum param, ALfloat *values);
ALvoid						(ALAPIENTRY * qalGetFilteri)(ALuint filter, ALenum param, ALint *value);
ALvoid						(ALAPIENTRY * qalGetFilteriv)(ALuint filter, ALenum param, ALint *values);
ALboolean					(ALAPIENTRY * qalIsAuxiliaryEffectSlot)(ALuint slot);
ALboolean					(ALAPIENTRY * qalIsEffect)(ALuint effect);
ALboolean					(ALAPIENTRY * qalIsFilter)(ALuint filter);

ALenum						(ALAPIENTRY * qalEAXGetBufferMode)(ALuint buffer, ALenum *value);
ALboolean					(ALAPIENTRY * qalEAXSetBufferMode)(ALsizei n, const ALuint *buffers, ALenum value);


// ============================================================================


static ALvoid				(ALAPIENTRY * dllBufferData)(ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq);
static ALvoid				(ALAPIENTRY * dllBuffer3f)(ALuint buffer, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
static ALvoid				(ALAPIENTRY * dllBuffer3i)(ALuint buffer, ALenum param, ALint value1, ALint value2, ALint value3);
static ALvoid				(ALAPIENTRY * dllBufferf)(ALuint buffer, ALenum param, ALfloat value);
static ALvoid				(ALAPIENTRY * dllBufferfv)(ALuint buffer, ALenum param, const ALfloat *values);
static ALvoid				(ALAPIENTRY * dllBufferi)(ALuint buffer, ALenum param, ALint value);
static ALvoid				(ALAPIENTRY * dllBufferiv)(ALuint buffer, ALenum param, const ALint *values);
static ALvoid				(ALAPIENTRY * dllDeleteBuffers)(ALsizei n, const ALuint *buffers);
static ALvoid				(ALAPIENTRY * dllDeleteSources)(ALsizei n, const ALuint *sources);
static ALvoid				(ALAPIENTRY * dllDisable)(ALenum capability);
static ALvoid				(ALAPIENTRY * dllDistanceModel)(ALenum distanceModel);
static ALvoid				(ALAPIENTRY * dllDopplerFactor)(ALfloat value);
static ALvoid				(ALAPIENTRY * dllDopplerVelocity)(ALfloat value);
static ALvoid				(ALAPIENTRY * dllEnable)(ALenum capability);
static ALvoid				(ALAPIENTRY * dllGenBuffers)(ALsizei n, ALuint *buffers);
static ALvoid				(ALAPIENTRY * dllGenSources)(ALsizei n, ALuint *sources);
static ALboolean			(ALAPIENTRY * dllGetBoolean)(ALenum param);
static ALvoid				(ALAPIENTRY * dllGetBooleanv)(ALenum param, ALboolean *data);
static ALvoid				(ALAPIENTRY * dllGetBuffer3f)(ALuint buffer, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
static ALvoid				(ALAPIENTRY * dllGetBuffer3i)(ALuint buffer, ALenum param, ALint *value1, ALint *value2, ALint *value3);
static ALvoid				(ALAPIENTRY * dllGetBufferf)(ALuint buffer, ALenum param, ALfloat *value);
static ALvoid				(ALAPIENTRY * dllGetBufferfv)(ALuint buffer, ALenum param, ALfloat *values);
static ALvoid				(ALAPIENTRY * dllGetBufferi)(ALuint buffer, ALenum param, ALint *value);
static ALvoid				(ALAPIENTRY * dllGetBufferiv)(ALuint buffer, ALenum param, ALint *values);
static ALdouble				(ALAPIENTRY * dllGetDouble)(ALenum param);
static ALvoid				(ALAPIENTRY * dllGetDoublev)(ALenum param, ALdouble *data);
static ALenum				(ALAPIENTRY * dllGetEnumValue)(const ALchar *enumName);
static ALenum				(ALAPIENTRY * dllGetError)(ALvoid);
static ALfloat				(ALAPIENTRY * dllGetFloat)(ALenum param);
static ALvoid				(ALAPIENTRY * dllGetFloatv)(ALenum param, ALfloat *data);
static ALint				(ALAPIENTRY * dllGetInteger)(ALenum param);
static ALvoid				(ALAPIENTRY * dllGetIntegerv)(ALenum param, ALint *data);
static ALvoid				(ALAPIENTRY * dllGetListener3f)(ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
static ALvoid				(ALAPIENTRY * dllGetListener3i)(ALenum param, ALint *value1, ALint *value2, ALint *value3);
static ALvoid				(ALAPIENTRY * dllGetListenerf)(ALenum param, ALfloat *value);
static ALvoid				(ALAPIENTRY * dllGetListenerfv)(ALenum param, ALfloat *values);
static ALvoid				(ALAPIENTRY * dllGetListeneri)(ALenum param, ALint *value);
static ALvoid				(ALAPIENTRY * dllGetListeneriv)(ALenum param, ALint *values);
static ALvoid *				(ALAPIENTRY * dllGetProcAddress)(const ALchar *funcName);
static ALvoid				(ALAPIENTRY * dllGetSource3f)(ALuint source, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
static ALvoid				(ALAPIENTRY * dllGetSource3i)(ALuint source, ALenum param, ALint *value1, ALint *value2, ALint *value3);
static ALvoid				(ALAPIENTRY * dllGetSourcef)(ALuint source, ALenum param, ALfloat *value);
static ALvoid				(ALAPIENTRY * dllGetSourcefv)(ALuint source, ALenum param, ALfloat *values);
static ALvoid				(ALAPIENTRY * dllGetSourcei)(ALuint source, ALenum param, ALint *value);
static ALvoid				(ALAPIENTRY * dllGetSourceiv)(ALuint source, ALenum param, ALint *values);
static const ALchar *		(ALAPIENTRY * dllGetString)(ALenum param);
static ALboolean			(ALAPIENTRY * dllIsBuffer)(ALuint buffer);
static ALboolean			(ALAPIENTRY * dllIsEnabled)(ALenum capability);
static ALboolean			(ALAPIENTRY * dllIsExtensionPresent)(const ALchar *extName);
static ALboolean			(ALAPIENTRY * dllIsSource)(ALuint source);
static ALvoid				(ALAPIENTRY * dllListener3f)(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
static ALvoid				(ALAPIENTRY * dllListener3i)(ALenum param, ALint value1, ALint value2, ALint value3);
static ALvoid				(ALAPIENTRY * dllListenerf)(ALenum param, ALfloat value);
static ALvoid				(ALAPIENTRY * dllListenerfv)(ALenum param, const ALfloat *values);
static ALvoid				(ALAPIENTRY * dllListeneri)(ALenum param, ALint value);
static ALvoid				(ALAPIENTRY * dllListeneriv)(ALenum param, const ALint *values);
static ALvoid				(ALAPIENTRY * dllSource3f)(ALuint source, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
static ALvoid				(ALAPIENTRY * dllSource3i)(ALuint source, ALenum param, ALint value1, ALint value2, ALint value3);
static ALvoid				(ALAPIENTRY * dllSourcef)(ALuint source, ALenum param, ALfloat value);
static ALvoid				(ALAPIENTRY * dllSourcefv)(ALuint source, ALenum param, const ALfloat *values);
static ALvoid				(ALAPIENTRY * dllSourcei)(ALuint source, ALenum param, ALint value);
static ALvoid				(ALAPIENTRY * dllSourceiv)(ALuint source, ALenum param, const ALint *values);
static ALvoid				(ALAPIENTRY * dllSourcePause)(ALuint source);
static ALvoid				(ALAPIENTRY * dllSourcePausev)(ALsizei n, const ALuint *sources);
static ALvoid				(ALAPIENTRY * dllSourcePlay)(ALuint source);
static ALvoid				(ALAPIENTRY * dllSourcePlayv)(ALsizei n, const ALuint *sources);
static ALvoid				(ALAPIENTRY * dllSourceQueueBuffers)(ALuint source, ALsizei n, const ALuint *buffers);
static ALvoid				(ALAPIENTRY * dllSourceRewind)(ALuint source);
static ALvoid				(ALAPIENTRY * dllSourceRewindv)(ALsizei n, const ALuint *sources);
static ALvoid				(ALAPIENTRY * dllSourceStop)(ALuint source);
static ALvoid				(ALAPIENTRY * dllSourceStopv)(ALsizei n, const ALuint *sources);
static ALvoid				(ALAPIENTRY * dllSourceUnqueueBuffers)(ALuint source, ALsizei n, ALuint *buffers);
static ALvoid				(ALAPIENTRY * dllSpeedOfSound)(ALfloat value);

static ALvoid				(ALAPIENTRY * dllAuxiliaryEffectSlotf)(ALuint slot, ALenum param, ALfloat value);
static ALvoid				(ALAPIENTRY * dllAuxiliaryEffectSlotfv)(ALuint slot, ALenum param, const ALfloat *values);
static ALvoid				(ALAPIENTRY * dllAuxiliaryEffectSloti)(ALuint slot, ALenum param, ALint value);
static ALvoid				(ALAPIENTRY * dllAuxiliaryEffectSlotiv)(ALuint slot, ALenum param, const ALint *values);
static ALvoid				(ALAPIENTRY * dllDeleteAuxiliaryEffectSlots)(ALsizei n, const ALuint *slots);
static ALvoid				(ALAPIENTRY * dllDeleteEffects)(ALsizei n, const ALuint *effects);
static ALvoid				(ALAPIENTRY * dllDeleteFilters)(ALsizei n, const ALuint *filters);
static ALvoid				(ALAPIENTRY * dllEffectf)(ALuint effect, ALenum param, ALfloat value);
static ALvoid				(ALAPIENTRY * dllEffectfv)(ALuint effect, ALenum param, const ALfloat *values);
static ALvoid				(ALAPIENTRY * dllEffecti)(ALuint effect, ALenum param, ALint value);
static ALvoid				(ALAPIENTRY * dllEffectiv)(ALuint effect, ALenum param, const ALint *values);
static ALvoid				(ALAPIENTRY * dllFilterf)(ALuint filter, ALenum param, ALfloat value);
static ALvoid				(ALAPIENTRY * dllFilterfv)(ALuint filter, ALenum param, const ALfloat *values);
static ALvoid				(ALAPIENTRY * dllFilteri)(ALuint filter, ALenum param, ALint value);
static ALvoid				(ALAPIENTRY * dllFilteriv)(ALuint filter, ALenum param, const ALint *values);
static ALvoid				(ALAPIENTRY * dllGenAuxiliaryEffectSlots)(ALsizei n, ALuint *slots);
static ALvoid				(ALAPIENTRY * dllGenEffects)(ALsizei n, ALuint *effects);
static ALvoid				(ALAPIENTRY * dllGenFilters)(ALsizei n, ALuint *filters);
static ALvoid				(ALAPIENTRY * dllGetAuxiliaryEffectSlotf)(ALuint slot, ALenum param, ALfloat *value);
static ALvoid				(ALAPIENTRY * dllGetAuxiliaryEffectSlotfv)(ALuint slot, ALenum param, ALfloat *values);
static ALvoid				(ALAPIENTRY * dllGetAuxiliaryEffectSloti)(ALuint slot, ALenum param, ALint *value);
static ALvoid				(ALAPIENTRY * dllGetAuxiliaryEffectSlotiv)(ALuint slot, ALenum param, ALint *values);
static ALvoid				(ALAPIENTRY * dllGetEffectf)(ALuint effect, ALenum param, ALfloat *value);
static ALvoid				(ALAPIENTRY * dllGetEffectfv)(ALuint effect, ALenum param, ALfloat *values);
static ALvoid				(ALAPIENTRY * dllGetEffecti)(ALuint effect, ALenum param, ALint *value);
static ALvoid				(ALAPIENTRY * dllGetEffectiv)(ALuint effect, ALenum param, ALint *values);
static ALvoid				(ALAPIENTRY * dllGetFilterf)(ALuint filter, ALenum param, ALfloat *value);
static ALvoid				(ALAPIENTRY * dllGetFilterfv)(ALuint filter, ALenum param, ALfloat *values);
static ALvoid				(ALAPIENTRY * dllGetFilteri)(ALuint filter, ALenum param, ALint *value);
static ALvoid				(ALAPIENTRY * dllGetFilteriv)(ALuint filter, ALenum param, ALint *values);
static ALboolean			(ALAPIENTRY * dllIsAuxiliaryEffectSlot)(ALuint slot);
static ALboolean			(ALAPIENTRY * dllIsEffect)(ALuint effect);
static ALboolean			(ALAPIENTRY * dllIsFilter)(ALuint filter);

static ALenum				(ALAPIENTRY * dllEAXGetBufferMode)(ALuint buffer, ALenum *value);
static ALboolean			(ALAPIENTRY * dllEAXSetBufferMode)(ALsizei n, const ALuint *buffers, ALenum value);


// ============================================================================


static ALvoid ALAPIENTRY logBufferData (ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq){

	const char	*f;

	switch (format){
	case AL_FORMAT_MONO8:		f = "AL_FORMAT_MONO8";				break;
	case AL_FORMAT_MONO16:		f = "AL_FORMAT_MONO16";				break;
	case AL_FORMAT_STEREO8:		f = "AL_FORMAT_STEREO8";			break;
	case AL_FORMAT_STEREO16:	f = "AL_FORMAT_STEREO16";			break;
	default:					f = Str_VarArgs("0x%08X", format);	break;
	}

	fprintf(qalState.logFile, "alBufferData( %u, %s, %p, %i, %i )\n", buffer, f, data, size, freq);
	dllBufferData(buffer, format, data, size, freq);
}

static ALvoid ALAPIENTRY logBuffer3f (ALuint buffer, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3){

	fprintf(qalState.logFile, "alBuffer3f( %u, 0x%08X, %g, %g, %g )\n", buffer, param, value1, value2, value3);
	dllBuffer3f(buffer, param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logBuffer3i (ALuint buffer, ALenum param, ALint value1, ALint value2, ALint value3){

	fprintf(qalState.logFile, "alBuffer3i( %u, 0x%08X, %i, %i, %i )\n", buffer, param, value1, value2, value3);
	dllBuffer3i(buffer, param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logBufferf (ALuint buffer, ALenum param, ALfloat value){

	fprintf(qalState.logFile, "alBufferf( %u, 0x%08X, %g )\n", buffer, param, value);
	dllBufferf(buffer, param, value);
}

static ALvoid ALAPIENTRY logBufferfv (ALuint buffer, ALenum param, const ALfloat *values){

	fprintf(qalState.logFile, "alBufferfv( %u, 0x%08X, %p )\n", buffer, param, values);
	dllBufferfv(buffer, param, values);
}

static ALvoid ALAPIENTRY logBufferi (ALuint buffer, ALenum param, ALint value){

	fprintf(qalState.logFile, "alBufferi( %u, 0x%08X, %i )\n", buffer, param, value);
	dllBufferi(buffer, param, value);
}

static ALvoid ALAPIENTRY logBufferiv (ALuint buffer, ALenum param, const ALint *values){

	fprintf(qalState.logFile, "alBufferiv( %u, 0x%08X, %p )\n", buffer, param, values);
	dllBufferiv(buffer, param, values);
}

static ALvoid ALAPIENTRY logDeleteBuffers (ALsizei n, const ALuint *buffers){

	fprintf(qalState.logFile, "alDeleteBuffers( %i, %p )\n", n, buffers);
	dllDeleteBuffers(n, buffers);
}

static ALvoid ALAPIENTRY logDeleteSources (ALsizei n, const ALuint *sources){

	fprintf(qalState.logFile, "alDeleteSources( %i, %p )\n", n, sources);
	dllDeleteSources(n, sources);
}

static ALvoid ALAPIENTRY logDisable (ALenum capability){

	fprintf(qalState.logFile, "alDisable( 0x%08X )\n", capability);
	dllDisable(capability);
}

static ALvoid ALAPIENTRY logDistanceModel (ALenum distanceModel){

	const char	*dm;

	switch (distanceModel){
	case AL_NONE:						dm = "AL_NONE";								break;
	case AL_INVERSE_DISTANCE:			dm = "AL_INVERSE_DISTANCE";					break;
	case AL_INVERSE_DISTANCE_CLAMPED:	dm = "AL_INVERSE_DISTANCE_CLAMPED";			break;
	case AL_LINEAR_DISTANCE:			dm = "AL_LINEAR_DISTANCE";					break;
	case AL_LINEAR_DISTANCE_CLAMPED:	dm = "AL_LINEAR_DISTANCE_CLAMPED";			break;
	case AL_EXPONENT_DISTANCE:			dm = "AL_EXPONENT_DISTANCE";				break;
	case AL_EXPONENT_DISTANCE_CLAMPED:	dm = "AL_EXPONENT_DISTANCE_CLAMPED";		break;
	default:							dm = Str_VarArgs("0x%08X", distanceModel);	break;
	}

	fprintf(qalState.logFile, "alDistanceModel( %s )\n", dm);
	dllDistanceModel(distanceModel);
}

static ALvoid ALAPIENTRY logDopplerFactor (ALfloat value){

	fprintf(qalState.logFile, "alDopplerFactor( %g )\n", value);
	dllDopplerFactor(value);
}

static ALvoid ALAPIENTRY logDopplerVelocity (ALfloat value){

	fprintf(qalState.logFile, "alDopplerVelocity( %g )\n", value);
	dllDopplerVelocity(value);
}

static ALvoid ALAPIENTRY logEnable (ALenum capability){

	fprintf(qalState.logFile, "alEnable( 0x%08X )\n", capability);
	dllEnable(capability);
}

static ALvoid ALAPIENTRY logGenBuffers (ALsizei n, ALuint *buffers){

	fprintf(qalState.logFile, "alGenBuffers( %i, %p )\n", n, buffers);
	dllGenBuffers(n, buffers);
}

static ALvoid ALAPIENTRY logGenSources (ALsizei n, ALuint *sources){

	fprintf(qalState.logFile, "alGenSources( %i, %p )\n", n, sources);
	dllGenSources(n, sources);
}

static ALboolean ALAPIENTRY logGetBoolean (ALenum param){

	fprintf(qalState.logFile, "alGetBoolean( 0x%08X )\n", param);
	return dllGetBoolean(param);
}

static ALvoid ALAPIENTRY logGetBooleanv (ALenum param, ALboolean *data){

	fprintf(qalState.logFile, "alGetBooleanv( 0x%08X, %p )\n", param, data);
	dllGetBooleanv(param, data);
}

static ALvoid ALAPIENTRY logGetBuffer3f (ALuint buffer, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3){

	fprintf(qalState.logFile, "alGetBuffer3f( %u, 0x%08X, %p, %p, %p )\n", buffer, param, value1, value2, value3);
	dllGetBuffer3f(buffer, param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logGetBuffer3i (ALuint buffer, ALenum param, ALint *value1, ALint *value2, ALint *value3){

	fprintf(qalState.logFile, "alGetBuffer3i( %u, 0x%08X, %p, %p, %p )\n", buffer, param, value1, value2, value3);
	dllGetBuffer3i(buffer, param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logGetBufferf (ALuint buffer, ALenum param, ALfloat *value){

	fprintf(qalState.logFile, "alGetBufferf( %u, 0x%08X, %p )\n", buffer, param, value);
	dllGetBufferf(buffer, param, value);
}

static ALvoid ALAPIENTRY logGetBufferfv (ALuint buffer, ALenum param, ALfloat *values){

	fprintf(qalState.logFile, "alGetBufferfv( %u, 0x%08X, %p )\n", buffer, param, values);
	dllGetBufferfv(buffer, param, values);
}

static ALvoid ALAPIENTRY logGetBufferi (ALuint buffer, ALenum param, ALint *value){

	fprintf(qalState.logFile, "alGetBufferi( %u, 0x%08X, %p )\n", buffer, param, value);
	dllGetBufferi(buffer, param, value);
}

static ALvoid ALAPIENTRY logGetBufferiv (ALuint buffer, ALenum param, ALint *values){

	fprintf(qalState.logFile, "alGetBufferiv( %u, 0x%08X, %p )\n", buffer, param, values);
	dllGetBufferiv(buffer, param, values);
}

static ALdouble ALAPIENTRY logGetDouble (ALenum param){

	fprintf(qalState.logFile, "alGetDouble( 0x%08X )\n", param);
	return dllGetDouble(param);
}

static ALvoid ALAPIENTRY logGetDoublev (ALenum param, ALdouble *data){

	fprintf(qalState.logFile, "alGetDoublev( 0x%08X, %p )\n", param, data);
	dllGetDoublev(param, data);
}

static ALenum ALAPIENTRY logGetEnumValue (const ALchar *enumName){

	fprintf(qalState.logFile, "alGetEnumValue( %s )\n", enumName);
	return dllGetEnumValue(enumName);
}

static ALenum ALAPIENTRY logGetError (ALvoid){

	fprintf(qalState.logFile, "alGetError()\n");
	return dllGetError();
}

static ALfloat ALAPIENTRY logGetFloat (ALenum param){

	fprintf(qalState.logFile, "alGetFloat( 0x%08X )\n", param);
	return dllGetFloat(param);
}

static ALvoid ALAPIENTRY logGetFloatv (ALenum param, ALfloat *data){

	fprintf(qalState.logFile, "alGetFloatv( 0x%08X, %p )\n", param, data);
	dllGetFloatv(param, data);
}

static ALint ALAPIENTRY logGetInteger (ALenum param){

	fprintf(qalState.logFile, "alGetInteger( 0x%08X )\n", param);
	return dllGetInteger(param);
}

static ALvoid ALAPIENTRY logGetIntegerv (ALenum param, ALint *data){

	fprintf(qalState.logFile, "alGetIntegerv( 0x%08X, %p )\n", param, data);
	dllGetIntegerv(param, data);
}

static ALvoid ALAPIENTRY logGetListener3f (ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3){

	fprintf(qalState.logFile, "alGetListener3f( 0x%08X, %p, %p, %p )\n", param, value1, value2, value3);
	dllGetListener3f(param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logGetListener3i (ALenum param, ALint *value1, ALint *value2, ALint *value3){

	fprintf(qalState.logFile, "alGetListener3i( 0x%08X, %p, %p, %p )\n", param, value1, value2, value3);
	dllGetListener3i(param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logGetListenerf (ALenum param, ALfloat *value){

	fprintf(qalState.logFile, "alGetListenerf( 0x%08X, %p )\n", param, value);
	dllGetListenerf(param, value);
}

static ALvoid ALAPIENTRY logGetListenerfv (ALenum param, ALfloat *values){

	fprintf(qalState.logFile, "alGetListenerfv( 0x%08X, %p )\n", param, values);
	dllGetListenerfv(param, values);
}

static ALvoid ALAPIENTRY logGetListeneri (ALenum param, ALint *value){

	fprintf(qalState.logFile, "alGetListeneri( 0x%08X, %p )\n", param, value);
	dllGetListeneri(param, value);
}

static ALvoid ALAPIENTRY logGetListeneriv (ALenum param, ALint *values){

	fprintf(qalState.logFile, "alGetListeneriv( 0x%08X, %p )\n", param, values);
	dllGetListeneriv(param, values);
}

static ALvoid * ALAPIENTRY logGetProcAddress (const ALchar *funcName){

	fprintf(qalState.logFile, "alGetProcAddress( %s )\n", funcName);
	return dllGetProcAddress(funcName);
}

static ALvoid ALAPIENTRY logGetSource3f (ALuint source, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3){

	fprintf(qalState.logFile, "alGetSource3f( %u, 0x%08X, %p, %p, %p )\n", source, param, value1, value2, value3);
	dllGetSource3f(source, param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logGetSource3i (ALuint source, ALenum param, ALint *value1, ALint *value2, ALint *value3){

	fprintf(qalState.logFile, "alGetSource3i( %u, 0x%08X, %p, %p, %p )\n", source, param, value1, value2, value3);
	dllGetSource3i(source, param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logGetSourcef (ALuint source, ALenum param, ALfloat *value){

	fprintf(qalState.logFile, "alGetSourcef( %u, 0x%08X, %p )\n", source, param, value);
	dllGetSourcef(source, param, value);
}

static ALvoid ALAPIENTRY logGetSourcefv (ALuint source, ALenum param, ALfloat *values){

	fprintf(qalState.logFile, "alGetSourcefv( %u, 0x%08X, %p )\n", source, param, values);
	dllGetSourcefv(source, param, values);
}

static ALvoid ALAPIENTRY logGetSourcei (ALuint source, ALenum param, ALint *value){

	fprintf(qalState.logFile, "alGetSourcei( %u, 0x%08X, %p )\n", source, param, value);
	dllGetSourcei(source, param, value);
}

static ALvoid ALAPIENTRY logGetSourceiv (ALuint source, ALenum param, ALint *values){

	fprintf(qalState.logFile, "alGetSourceiv( %u, 0x%08X, %p )\n", source, param, values);
	dllGetSourceiv(source, param, values);
}

static const ALchar * ALAPIENTRY logGetString (ALenum param){

	fprintf(qalState.logFile, "alGetString( 0x%08X )\n", param);
	return dllGetString(param);
}

static ALboolean ALAPIENTRY logIsBuffer (ALuint buffer){

	fprintf(qalState.logFile, "alIsBuffer( %u )\n", buffer);
	return dllIsBuffer(buffer);
}

static ALboolean ALAPIENTRY logIsEnabled (ALenum capability){

	fprintf(qalState.logFile, "alIsEnabled( 0x%08X )\n", capability);
	return dllIsEnabled(capability);
}

static ALboolean ALAPIENTRY logIsExtensionPresent (const ALchar *extName){

	fprintf(qalState.logFile, "alIsExtensionPresent( %s )\n", extName);
	return dllIsExtensionPresent(extName);
}

static ALboolean ALAPIENTRY logIsSource (ALuint source){

	fprintf(qalState.logFile, "alIsSource( %u )\n", source);
	return dllIsSource(source);
}

static ALvoid ALAPIENTRY logListener3f (ALenum param, ALfloat value1, ALfloat value2, ALfloat value3){

	const char	*p;

	switch (param){
	case AL_POSITION:	p = "AL_POSITION";				break;
	case AL_VELOCITY:	p = "AL_VELOCITY";				break;
	default:			p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qalState.logFile, "alListener3f( %s, %g, %g, %g )\n", p, value1, value2, value3);
	dllListener3f(param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logListener3i (ALenum param, ALint value1, ALint value2, ALint value3){

	fprintf(qalState.logFile, "alListener3i( 0x%08X, %i, %i, %i )\n", param, value1, value2, value3);
	dllListener3i(param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logListenerf (ALenum param, ALfloat value){

	const char	*p;

	switch (param){
	case AL_GAIN:				p = "AL_GAIN";					break;
	case AL_METERS_PER_UNIT:	p = "AL_METERS_PER_UNIT";		break;
	default:					p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qalState.logFile, "alListenerf( %s, %g )\n", p, value);
	dllListenerf(param, value);
}

static ALvoid ALAPIENTRY logListenerfv (ALenum param, const ALfloat *values){

	const char	*p;

	switch (param){
	case AL_ORIENTATION:	p = "AL_ORIENTATION";			break;
	case AL_POSITION:		p = "AL_POSITION";				break;
	case AL_VELOCITY:		p = "AL_VELOCITY";				break;
	default:				p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qalState.logFile, "alListenerfv( %s, %p )\n", p, values);
	dllListenerfv(param, values);
}

static ALvoid ALAPIENTRY logListeneri (ALenum param, ALint value){

	fprintf(qalState.logFile, "alListeneri( 0x%08X, %i )\n", param, value);
	dllListeneri(param, value);
}

static ALvoid ALAPIENTRY logListeneriv (ALenum param, const ALint *values){

	fprintf(qalState.logFile, "alListeneriv( 0x%08X, %p )\n", param, values);
	dllListeneriv(param, values);
}

static ALvoid ALAPIENTRY logSource3f (ALuint source, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3){

	const char	*p;

	switch (param){
	case AL_DIRECTION:	p = "AL_DIRECTION";				break;
	case AL_POSITION:	p = "AL_POSITION";				break;
	case AL_VELOCITY:	p = "AL_VELOCITY";				break;
	default:			p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qalState.logFile, "alSource3f( %u, %s, %g, %g, %g )\n", source, p, value1, value2, value3);
	dllSource3f(source, param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logSource3i (ALuint source, ALenum param, ALint value1, ALint value2, ALint value3){

	const char	*p;

	switch (param){
	case AL_AUXILIARY_SEND_FILTER:	p = "AL_AUXILIARY_SEND_FILTER";	break;
	default:						p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qalState.logFile, "alSource3i( %u, %s, %i, %i, %i )\n", source, p, value1, value2, value3);
	dllSource3i(source, param, value1, value2, value3);
}

static ALvoid ALAPIENTRY logSourcef (ALuint source, ALenum param, ALfloat value){

	const char	*p;

	switch (param){
	case AL_AIR_ABSORPTION_FACTOR:	p = "AL_AIR_ABSORPTION_FACTOR";	break;
	case AL_BYTE_OFFSET:			p = "AL_BYTE_OFFSET";			break;
	case AL_CONE_INNER_ANGLE:		p = "AL_CONE_INNER_ANGLE";		break;
	case AL_CONE_OUTER_ANGLE:		p = "AL_CONE_OUTER_ANGLE";		break;
	case AL_CONE_OUTER_GAIN:		p = "AL_CONE_OUTER_GAIN";		break;
	case AL_CONE_OUTER_GAINHF:		p = "AL_CONE_OUTER_GAINHF";		break;
	case AL_GAIN:					p = "AL_GAIN";					break;
	case AL_MAX_DISTANCE:			p = "AL_MAX_DISTANCE";			break;
	case AL_MAX_GAIN:				p = "AL_MAX_GAIN";				break;
	case AL_MIN_GAIN:				p = "AL_MIN_GAIN";				break;
	case AL_PITCH:					p = "AL_PITCH";					break;
	case AL_REFERENCE_DISTANCE:		p = "AL_REFERENCE_DISTANCE";	break;
	case AL_ROLLOFF_FACTOR:			p = "AL_ROLLOFF_FACTOR";		break;
	case AL_ROOM_ROLLOFF_FACTOR:	p = "AL_ROOM_ROLLOFF_FACTOR";	break;
	case AL_SAMPLE_OFFSET:			p = "AL_SAMPLE_OFFSET";			break;
	case AL_SEC_OFFSET:				p = "AL_SEC_OFFSET";			break;
	default:						p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qalState.logFile, "alSourcef( %u, %s, %g )\n", source, p, value);
	dllSourcef(source, param, value);
}

static ALvoid ALAPIENTRY logSourcefv (ALuint source, ALenum param, const ALfloat *values){

	const char	*p;

	switch (param){
	case AL_DIRECTION:	p = "AL_DIRECTION";				break;
	case AL_POSITION:	p = "AL_POSITION";				break;
	case AL_VELOCITY:	p = "AL_VELOCITY";				break;
	default:			p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qalState.logFile, "alSourcefv( %u, %s, %p )\n", source, p, values);
	dllSourcefv(source, param, values);
}

static ALvoid ALAPIENTRY logSourcei (ALuint source, ALenum param, ALint value){

	const char	*p, *v;

	switch (param){
	case AL_AUXILIARY_SEND_FILTER_GAIN_AUTO:	p = "AL_AUXILIARY_SEND_FILTER_GAIN_AUTO";	break;
	case AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO:	p = "AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO";	break;
	case AL_BUFFER:								p = "AL_BUFFER";							break;
	case AL_BYTE_OFFSET:						p = "AL_BYTE_OFFSET";						break;
	case AL_DIRECT_FILTER:						p = "AL_DIRECT_FILTER";						break;
	case AL_DIRECT_FILTER_GAINHF_AUTO:			p = "AL_DIRECT_FILTER_GAINHF_AUTO";			break;
	case AL_LOOPING:							p = "AL_LOOPING";							break;
	case AL_SAMPLE_OFFSET:						p = "AL_SAMPLE_OFFSET";						break;
	case AL_SEC_OFFSET:							p = "AL_SEC_OFFSET";						break;
	case AL_SOURCE_RELATIVE:					p = "AL_SOURCE_RELATIVE";					break;
	default:									p = Str_VarArgs("0x%08X", param);				break;
	}

	if (param == AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO || param == AL_AUXILIARY_SEND_FILTER_GAIN_AUTO || param == AL_DIRECT_FILTER_GAINHF_AUTO || param == AL_LOOPING || param == AL_SOURCE_RELATIVE){
		switch (value){
		case AL_FALSE:	v = "AL_FALSE";					break;
		case AL_TRUE:	v = "AL_TRUE";					break;
		default:		v = Str_VarArgs("0x%08X", value);	break;
		}
	}
	else
		v = Str_VarArgs("%i", value);

	fprintf(qalState.logFile, "alSourcei( %u, %s, %s )\n", source, p, v);
	dllSourcei(source, param, value);
}

static ALvoid ALAPIENTRY logSourceiv (ALuint source, ALenum param, const ALint *values){

	fprintf(qalState.logFile, "alSourceiv( %u, 0x%08X, %p )\n", source, param, values);
	dllSourceiv(source, param, values);
}

static ALvoid ALAPIENTRY logSourcePause (ALuint source){

	fprintf(qalState.logFile, "alSourcePause( %u )\n", source);
	dllSourcePause(source);
}

static ALvoid ALAPIENTRY logSourcePausev (ALsizei n, const ALuint *sources){

	fprintf(qalState.logFile, "alSourcePausev( %i, %p )\n", n, sources);
	dllSourcePausev(n, sources);
}

static ALvoid ALAPIENTRY logSourcePlay (ALuint source){

	fprintf(qalState.logFile, "alSourcePlay( %u )\n", source);
	dllSourcePlay(source);
}

static ALvoid ALAPIENTRY logSourcePlayv (ALsizei n, const ALuint *sources){

	fprintf(qalState.logFile, "alSourcePlayv( %i, %p )\n", n, sources);
	dllSourcePlayv(n, sources);
}

static ALvoid ALAPIENTRY logSourceQueueBuffers (ALuint source, ALsizei n, const ALuint *buffers){

	fprintf(qalState.logFile, "alSourceQueueBuffers( %u, %i, %p )\n", source, n, buffers);
	dllSourceQueueBuffers(source, n, buffers);
}

static ALvoid ALAPIENTRY logSourceRewind (ALuint source){

	fprintf(qalState.logFile, "alSourceRewind( %u )\n", source);
	dllSourceRewind(source);
}

static ALvoid ALAPIENTRY logSourceRewindv (ALsizei n, const ALuint *sources){

	fprintf(qalState.logFile, "alSourceRewindv( %i, %p )\n", n, sources);
	dllSourceRewindv(n, sources);
}

static ALvoid ALAPIENTRY logSourceStop (ALuint source){

	fprintf(qalState.logFile, "alSourceStop( %u )\n", source);
	dllSourceStop(source);
}

static ALvoid ALAPIENTRY logSourceStopv (ALsizei n, const ALuint *sources){

	fprintf(qalState.logFile, "alSourceStopv( %i, %p )\n", n, sources);
	dllSourceStopv(n, sources);
}

static ALvoid ALAPIENTRY logSourceUnqueueBuffers (ALuint source, ALsizei n, ALuint *buffers){

	fprintf(qalState.logFile, "alSourceUnqueueBuffers( %u, %i, %p )\n", source, n, buffers);
	dllSourceUnqueueBuffers(source, n, buffers);
}

static ALvoid ALAPIENTRY logSpeedOfSound (ALfloat value){

	fprintf(qalState.logFile, "alSpeedOfSound( %g )\n", value);
	dllSpeedOfSound(value);
}

static ALvoid ALAPIENTRY logAuxiliaryEffectSlotf (ALuint slot, ALenum param, ALfloat value){

	const char	*p;

	switch (param){
	case AL_EFFECTSLOT_GAIN:	p = "AL_EFFECTSLOT_GAIN";		break;
	default:					p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qalState.logFile, "alAuxiliaryEffectSlotf( %u, %s, %g )\n", slot, p, value);
	dllAuxiliaryEffectSlotf(slot, param, value);
}

static ALvoid ALAPIENTRY logAuxiliaryEffectSlotfv (ALuint slot, ALenum param, const ALfloat *values){

	fprintf(qalState.logFile, "alAuxiliaryEffectSlotfv( %u, 0x%08X, %p )\n", slot, param, values);
	dllAuxiliaryEffectSlotfv(slot, param, values);
}

static ALvoid ALAPIENTRY logAuxiliaryEffectSloti (ALuint slot, ALenum param, ALint value){

	const char	*p, *v;

	switch (param){
	case AL_EFFECTSLOT_AUXILIARY_SEND_AUTO:	p = "AL_EFFECTSLOT_AUXILIARY_SEND_AUTO";	break;
	case AL_EFFECTSLOT_EFFECT:				p = "AL_EFFECTSLOT_EFFECT";					break;
	default:								p = Str_VarArgs("0x%08X", param);				break;
	}

	if (param == AL_EFFECTSLOT_AUXILIARY_SEND_AUTO){
		switch (value){
		case AL_FALSE:	v = "AL_FALSE";					break;
		case AL_TRUE:	v = "AL_TRUE";					break;
		default:		v = Str_VarArgs("0x%08X", value);	break;
		}
	}
	else
		v = Str_VarArgs("%i", value);

	fprintf(qalState.logFile, "alAuxiliaryEffectSloti( %u, %s, %s )\n", slot, p, v);
	dllAuxiliaryEffectSloti(slot, param, value);
}

static ALvoid ALAPIENTRY logAuxiliaryEffectSlotiv (ALuint slot, ALenum param, const ALint *values){

	fprintf(qalState.logFile, "alAuxiliaryEffectSlotiv( %u, 0x%08X, %p )\n", slot, param, values);
	dllAuxiliaryEffectSlotiv(slot, param, values);
}

static ALvoid ALAPIENTRY logDeleteAuxiliaryEffectSlots (ALsizei n, const ALuint *slots){

	fprintf(qalState.logFile, "alDeleteAuxiliaryEffectSlots( %i, %p )\n", n, slots);
	dllDeleteAuxiliaryEffectSlots(n, slots);
}

static ALvoid ALAPIENTRY logDeleteEffects (ALsizei n, const ALuint *effects){

	fprintf(qalState.logFile, "alDeleteEffects( %i, %p )\n", n, effects);
	dllDeleteEffects(n, effects);
}

static ALvoid ALAPIENTRY logDeleteFilters (ALsizei n, const ALuint *filters){

	fprintf(qalState.logFile, "alDeleteFilters( %i, %p )\n", n, filters);
	dllDeleteFilters(n, filters);
}

static ALvoid ALAPIENTRY logEffectf (ALuint effect, ALenum param, ALfloat value){

	const char	*p;

	switch (param){
	case AL_EAXREVERB_DENSITY:					p = "AL_EAXREVERB_DENSITY";					break;
	case AL_EAXREVERB_DIFFUSION:				p = "AL_EAXREVERB_DIFFUSION";				break;
	case AL_EAXREVERB_GAIN:						p = "AL_EAXREVERB_GAIN";					break;
	case AL_EAXREVERB_GAINHF:					p = "AL_EAXREVERB_GAINHF";					break;
	case AL_EAXREVERB_GAINLF:					p = "AL_EAXREVERB_GAINLF";					break;
	case AL_EAXREVERB_DECAY_TIME:				p = "AL_EAXREVERB_DECAY_TIME";				break;
	case AL_EAXREVERB_DECAY_HFRATIO:			p = "AL_EAXREVERB_DECAY_HFRATIO";			break;
	case AL_EAXREVERB_DECAY_LFRATIO:			p = "AL_EAXREVERB_DECAY_LFRATIO";			break;
	case AL_EAXREVERB_REFLECTIONS_GAIN:			p = "AL_EAXREVERB_REFLECTIONS_GAIN";		break;
	case AL_EAXREVERB_REFLECTIONS_DELAY:		p = "AL_EAXREVERB_REFLECTIONS_DELAY";		break;
	case AL_EAXREVERB_LATE_REVERB_GAIN:			p = "AL_EAXREVERB_LATE_REVERB_GAIN";		break;
	case AL_EAXREVERB_LATE_REVERB_DELAY:		p = "AL_EAXREVERB_LATE_REVERB_DELAY";		break;
	case AL_EAXREVERB_ECHO_TIME:				p = "AL_EAXREVERB_ECHO_TIME";				break;
	case AL_EAXREVERB_ECHO_DEPTH:				p = "AL_EAXREVERB_ECHO_DEPTH";				break;
	case AL_EAXREVERB_MODULATION_TIME:			p = "AL_EAXREVERB_MODULATION_TIME";			break;
	case AL_EAXREVERB_MODULATION_DEPTH:			p = "AL_EAXREVERB_MODULATION_DEPTH";		break;
	case AL_EAXREVERB_HFREFERENCE:				p = "AL_EAXREVERB_HFREFERENCE";				break;
	case AL_EAXREVERB_LFREFERENCE:				p = "AL_EAXREVERB_LFREFERENCE";				break;
	case AL_EAXREVERB_AIR_ABSORPTION_GAINHF:	p = "AL_EAXREVERB_AIR_ABSORPTION_GAINHF";	break;
	case AL_EAXREVERB_ROOM_ROLLOFF_FACTOR:		p = "AL_EAXREVERB_ROOM_ROLLOFF_FACTOR";		break;
	default:									p = Str_VarArgs("0x%08X", param);				break;
	}

	fprintf(qalState.logFile, "alEffectf( %u, %s, %g )\n", effect, p, value);
	dllEffectf(effect, param, value);
}

static ALvoid ALAPIENTRY logEffectfv (ALuint effect, ALenum param, const ALfloat *values){

	const char	*p;

	switch (param){
	case AL_EAXREVERB_REFLECTIONS_PAN:	p = "AL_EAXREVERB_REFLECTIONS_PAN";	break;
	case AL_EAXREVERB_LATE_REVERB_PAN:	p = "AL_EAXREVERB_LATE_REVERB_PAN";	break;
	default:							p = Str_VarArgs("0x%08X", param);		break;
	}

	fprintf(qalState.logFile, "alEffectfv( %u, %s, %p )\n", effect, p, values);
	dllEffectfv(effect, param, values);
}

static ALvoid ALAPIENTRY logEffecti (ALuint effect, ALenum param, ALint value){

	const char	*p, *v;

	switch (param){
	case AL_EFFECT_TYPE:				p = "AL_EFFECT_TYPE";				break;
	case AL_EAXREVERB_DECAY_HFLIMIT:	p = "AL_EAXREVERB_DECAY_HFLIMIT";	break;
	default:							p = Str_VarArgs("0x%08X", param);		break;
	}

	if (param == AL_EFFECT_TYPE){
		switch (value){
		case AL_EFFECT_EAXREVERB:	v = "AL_EFFECT_EAXREVERB";		break;
		default:					v = Str_VarArgs("0x%08X", value);	break;
		}
	}
	else
		v = Str_VarArgs("%i", value);

	fprintf(qalState.logFile, "alEffecti( %u, %s, %s )\n", effect, p, v);
	dllEffecti(effect, param, value);
}

static ALvoid ALAPIENTRY logEffectiv (ALuint effect, ALenum param, const ALint *values){

	fprintf(qalState.logFile, "alEffectiv( %u, 0x%08X, %p )\n", effect, param, values);
	dllEffectiv(effect, param, values);
}

static ALvoid ALAPIENTRY logFilterf (ALuint filter, ALenum param, ALfloat value){

	const char	*p;

	switch (param){
	case AL_LOWPASS_GAIN:	p = "AL_LOWPASS_GAIN";			break;
	case AL_LOWPASS_GAINHF:	p = "AL_LOWPASS_GAINHF";		break;
	default:				p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qalState.logFile, "alFilterf( %u, %s, %g )\n", filter, p, value);
	dllFilterf(filter, param, value);
}

static ALvoid ALAPIENTRY logFilterfv (ALuint effect, ALenum param, const ALfloat *values){

	fprintf(qalState.logFile, "alFilterfv( %u, 0x%08X, %p )\n", effect, param, values);
	dllFilterfv(effect, param, values);
}

static ALvoid ALAPIENTRY logFilteri (ALuint filter, ALenum param, ALint value){

	const char	*p, *v;

	switch (param){
	case AL_FILTER_TYPE:	p = "AL_FILTER_TYPE";			break;
	default:				p = Str_VarArgs("0x%08X", param);	break;
	}

	if (param == AL_FILTER_TYPE){
		switch (value){
		case AL_FILTER_LOWPASS:	v = "AL_FILTER_LOWPASS";		break;
		default:				v = Str_VarArgs("0x%08X", value);	break;
		}
	}
	else
		v = Str_VarArgs("%i", value);

	fprintf(qalState.logFile, "alFilteri( %u, %s, %s )\n", filter, p, v);
	dllFilteri(filter, param, value);
}

static ALvoid ALAPIENTRY logFilteriv (ALuint effect, ALenum param, const ALint *values){

	fprintf(qalState.logFile, "alFilteriv( %u, 0x%08X, %p )\n", effect, param, values);
	dllFilteriv(effect, param, values);
}

static ALvoid ALAPIENTRY logGenAuxiliaryEffectSlots (ALsizei n, ALuint *slots){

	fprintf(qalState.logFile, "alGenAuxiliaryEffectSlots( %i, %p )\n", n, slots);
	dllGenAuxiliaryEffectSlots(n, slots);
}

static ALvoid ALAPIENTRY logGenEffects (ALsizei n, ALuint *effects){

	fprintf(qalState.logFile, "alGenEffects( %i, %p )\n", n, effects);
	dllGenEffects(n, effects);
}

static ALvoid ALAPIENTRY logGenFilters (ALsizei n, ALuint *filters){

	fprintf(qalState.logFile, "alGenFilters( %i, %p )\n", n, filters);
	dllGenFilters(n, filters);
}

static ALvoid ALAPIENTRY logGetAuxiliaryEffectSlotf (ALuint slot, ALenum param, ALfloat *value){

	fprintf(qalState.logFile, "alGetAuxiliaryEffectSlotf( %u, 0x%08X, %p )\n", slot, param, value);
	dllGetAuxiliaryEffectSlotf(slot, param, value);
}

static ALvoid ALAPIENTRY logGetAuxiliaryEffectSlotfv (ALuint slot, ALenum param, ALfloat *values){

	fprintf(qalState.logFile, "alGetAuxiliaryEffectSlotfv( %u, 0x%08X, %p )\n", slot, param, values);
	dllGetAuxiliaryEffectSlotfv(slot, param, values);
}

static ALvoid ALAPIENTRY logGetAuxiliaryEffectSloti (ALuint slot, ALenum param, ALint *value){

	fprintf(qalState.logFile, "alGetAuxiliaryEffectSloti( %u, 0x%08X, %p )\n", slot, param, value);
	dllGetAuxiliaryEffectSloti(slot, param, value);
}

static ALvoid ALAPIENTRY logGetAuxiliaryEffectSlotiv (ALuint slot, ALenum param, ALint *values){

	fprintf(qalState.logFile, "alGetAuxiliaryEffectSlotiv( %u, 0x%08X, %p )\n", slot, param, values);
	dllGetAuxiliaryEffectSlotiv(slot, param, values);
}

static ALvoid ALAPIENTRY logGetEffectf (ALuint effect, ALenum param, ALfloat *value){

	fprintf(qalState.logFile, "alGetEffectf( %u, 0x%08X, %p )\n", effect, param, value);
	dllGetEffectf(effect, param, value);
}

static ALvoid ALAPIENTRY logGetEffectfv (ALuint effect, ALenum param, ALfloat *values){

	fprintf(qalState.logFile, "alGetEffectfv( %u, 0x%08X, %p )\n", effect, param, values);
	dllGetEffectfv(effect, param, values);
}

static ALvoid ALAPIENTRY logGetEffecti (ALuint effect, ALenum param, ALint *value){

	fprintf(qalState.logFile, "alGetEffecti( %u, 0x%08X, %p )\n", effect, param, value);
	dllGetEffecti(effect, param, value);
}

static ALvoid ALAPIENTRY logGetEffectiv (ALuint effect, ALenum param, ALint *values){

	fprintf(qalState.logFile, "alGetEffectiv( %u, 0x%08X, %p )\n", effect, param, values);
	dllGetEffectiv(effect, param, values);
}

static ALvoid ALAPIENTRY logGetFilterf (ALuint filter, ALenum param, ALfloat *value){

	fprintf(qalState.logFile, "alGetFilterf( %u, 0x%08X, %p )\n", filter, param, value);
	dllGetFilterf(filter, param, value);
}

static ALvoid ALAPIENTRY logGetFilterfv (ALuint filter, ALenum param, ALfloat *values){

	fprintf(qalState.logFile, "alGetFilterfv( %u, 0x%08X, %p )\n", filter, param, values);
	dllGetFilterfv(filter, param, values);
}

static ALvoid ALAPIENTRY logGetFilteri (ALuint filter, ALenum param, ALint *value){

	fprintf(qalState.logFile, "alGetFilteri( %u, 0x%08X, %p )\n", filter, param, value);
	dllGetFilteri(filter, param, value);
}

static ALvoid ALAPIENTRY logGetFilteriv (ALuint filter, ALenum param, ALint *values){

	fprintf(qalState.logFile, "alGetFilteriv( %u, 0x%08X, %p )\n", filter, param, values);
	dllGetFilteriv(filter, param, values);
}

static ALboolean ALAPIENTRY logIsAuxiliaryEffectSlot (ALuint slot){

	fprintf(qalState.logFile, "alIsAuxiliaryEffectSlot( %u )\n", slot);
	return dllIsAuxiliaryEffectSlot(slot);
}

static ALboolean ALAPIENTRY logIsEffect (ALuint effect){

	fprintf(qalState.logFile, "alIsEffect( %u )\n", effect);
	return dllIsEffect(effect);
}

static ALboolean ALAPIENTRY logIsFilter (ALuint filter){

	fprintf(qalState.logFile, "alIsFilter( %u )\n", filter);
	return dllIsFilter(filter);
}

static ALenum ALAPIENTRY logEAXGetBufferMode (ALuint buffer, ALenum *value){

	fprintf(qalState.logFile, "alEAXGetBufferMode( %u, %p )\n", buffer, value);
	return dllEAXGetBufferMode(buffer, value);
}

static ALboolean ALAPIENTRY logEAXSetBufferMode (ALsizei n, const ALuint *buffers, ALenum value){

	const char	*v;

	if (value == AL_STORAGE_AUTOMATIC)
		v = "AL_STORAGE_AUTOMATIC";
	else if (value == AL_STORAGE_HARDWARE)
		v = "AL_STORAGE_HARDWARE";
	else if (value == AL_STORAGE_ACCESSIBLE)
		v = "AL_STORAGE_ACCESSIBLE";
	else
		v = Str_VarArgs("0x%08X", value);

	fprintf(qalState.logFile, "alEAXSetBufferMode( %i, %p, %s )\n", n, buffers, v);
	return dllEAXSetBufferMode(n, buffers, value);
}


/*
 ==============================================================================

 LOG FILE

 ==============================================================================
*/


/*
 ==================
 QAL_CopyPointers
 ==================
*/
static void QAL_CopyPointers (){

	dllBufferData					= qalBufferData;
	dllBuffer3f						= qalBuffer3f;
	dllBuffer3i						= qalBuffer3i;
	dllBufferf						= qalBufferf;
	dllBufferfv						= qalBufferfv;
	dllBufferi						= qalBufferi;
	dllBufferiv						= qalBufferiv;
	dllDeleteBuffers				= qalDeleteBuffers;
	dllDeleteSources				= qalDeleteSources;
	dllDisable						= qalDisable;
	dllDistanceModel				= qalDistanceModel;
	dllDopplerFactor				= qalDopplerFactor;
	dllDopplerVelocity				= qalDopplerVelocity;
	dllEnable						= qalEnable;
	dllGenBuffers					= qalGenBuffers;
	dllGenSources					= qalGenSources;
	dllGetBoolean					= qalGetBoolean;
	dllGetBooleanv					= qalGetBooleanv;
	dllGetBuffer3f					= qalGetBuffer3f;
	dllGetBuffer3i					= qalGetBuffer3i;
	dllGetBufferf					= qalGetBufferf;
	dllGetBufferfv					= qalGetBufferfv;
	dllGetBufferi					= qalGetBufferi;
	dllGetBufferiv					= qalGetBufferiv;
	dllGetDouble					= qalGetDouble;
	dllGetDoublev					= qalGetDoublev;
	dllGetEnumValue					= qalGetEnumValue;
	dllGetError						= qalGetError;
	dllGetFloat						= qalGetFloat;
	dllGetFloatv					= qalGetFloatv;
	dllGetInteger					= qalGetInteger;
	dllGetIntegerv					= qalGetIntegerv;
	dllGetListener3f				= qalGetListener3f;
	dllGetListener3i				= qalGetListener3i;
	dllGetListenerf					= qalGetListenerf;
	dllGetListenerfv				= qalGetListenerfv;
	dllGetListeneri					= qalGetListeneri;
	dllGetListeneriv				= qalGetListeneriv;
	dllGetProcAddress				= qalGetProcAddress;
	dllGetSource3f					= qalGetSource3f;
	dllGetSource3i					= qalGetSource3i;
	dllGetSourcef					= qalGetSourcef;
	dllGetSourcefv					= qalGetSourcefv;
	dllGetSourcei					= qalGetSourcei;
	dllGetSourceiv					= qalGetSourceiv;
	dllGetString					= qalGetString;
	dllIsBuffer						= qalIsBuffer;
	dllIsEnabled					= qalIsEnabled;
	dllIsExtensionPresent			= qalIsExtensionPresent;
	dllIsSource						= qalIsSource;
	dllListener3f					= qalListener3f;
	dllListener3i					= qalListener3i;
	dllListenerf					= qalListenerf;
	dllListenerfv					= qalListenerfv;
	dllListeneri					= qalListeneri;
	dllListeneriv					= qalListeneriv;
	dllSource3f						= qalSource3f;
	dllSource3i						= qalSource3i;
	dllSourcef						= qalSourcef;
	dllSourcefv						= qalSourcefv;
	dllSourcei						= qalSourcei;
	dllSourceiv						= qalSourceiv;
	dllSourcePause					= qalSourcePause;
	dllSourcePausev					= qalSourcePausev;
	dllSourcePlay					= qalSourcePlay;
	dllSourcePlayv					= qalSourcePlayv;
	dllSourceQueueBuffers			= qalSourceQueueBuffers;
	dllSourceRewind					= qalSourceRewind;
	dllSourceRewindv				= qalSourceRewindv;
	dllSourceStop					= qalSourceStop;
	dllSourceStopv					= qalSourceStopv;
	dllSourceUnqueueBuffers			= qalSourceUnqueueBuffers;
	dllSpeedOfSound					= qalSpeedOfSound;

	dllAuxiliaryEffectSlotf			= qalAuxiliaryEffectSlotf;
	dllAuxiliaryEffectSlotfv		= qalAuxiliaryEffectSlotfv;
	dllAuxiliaryEffectSloti			= qalAuxiliaryEffectSloti;
	dllAuxiliaryEffectSlotiv		= qalAuxiliaryEffectSlotiv;
	dllDeleteAuxiliaryEffectSlots	= qalDeleteAuxiliaryEffectSlots;
	dllDeleteEffects				= qalDeleteEffects;
	dllDeleteFilters				= qalDeleteFilters;
	dllEffectf						= qalEffectf;
	dllEffectfv						= qalEffectfv;
	dllEffecti						= qalEffecti;
	dllEffectiv						= qalEffectiv;
	dllFilterf						= qalFilterf;
	dllFilterfv						= qalFilterfv;
	dllFilteri						= qalFilteri;
	dllFilteriv						= qalFilteriv;
	dllGenAuxiliaryEffectSlots		= qalGenAuxiliaryEffectSlots;
	dllGenEffects					= qalGenEffects;
	dllGenFilters					= qalGenFilters;
	dllGetAuxiliaryEffectSlotf		= qalGetAuxiliaryEffectSlotf;
	dllGetAuxiliaryEffectSlotfv		= qalGetAuxiliaryEffectSlotfv;
	dllGetAuxiliaryEffectSloti		= qalGetAuxiliaryEffectSloti;
	dllGetAuxiliaryEffectSlotiv		= qalGetAuxiliaryEffectSlotiv;
	dllGetEffectf					= qalGetEffectf;
	dllGetEffectfv					= qalGetEffectfv;
	dllGetEffecti					= qalGetEffecti;
	dllGetEffectiv					= qalGetEffectiv;
	dllGetFilterf					= qalGetFilterf;
	dllGetFilterfv					= qalGetFilterfv;
	dllGetFilteri					= qalGetFilteri;
	dllGetFilteriv					= qalGetFilteriv;
	dllIsAuxiliaryEffectSlot		= qalIsAuxiliaryEffectSlot;
	dllIsEffect						= qalIsEffect;
	dllIsFilter						= qalIsFilter;

	dllEAXGetBufferMode				= qalEAXGetBufferMode;
	dllEAXSetBufferMode				= qalEAXSetBufferMode;
}

/*
 ==================
 QAL_EnableLogging
 ==================
*/
void QAL_EnableLogging (bool enable){

	FILE		*f;
	time_t		t;
	struct tm	*lt;

	// Copy the function pointers if needed
	if (!qalState.pointersCopied){
		qalState.pointersCopied = true;

		QAL_CopyPointers();
	}

	// Enable or disable logging
	if (enable){
		if (qalState.logFile){
			qalState.logFileFrames++;
			return;
		}

		// Find a file name to save it to
		while (qalState.logFileNumber <= 9999){
			Str_SPrintf(qalState.logFileName, sizeof(qalState.logFileName), "%s/sound_%04i.log", Sys_DefaultSaveDirectory(), qalState.logFileNumber);

			f = fopen(qalState.logFileName, "rt");
			if (!f)
				break;

			fclose(f);

			qalState.logFileNumber++;
		}

		if (qalState.logFileNumber == 10000)
			return;
		qalState.logFileNumber++;

		// Open the file
		qalState.logFile = fopen(qalState.logFileName, "wt");
		if (!qalState.logFile){
			Com_Printf("Couldn't open log file '%s'\n", qalState.logFileName);
			return;
		}

		Com_Printf("Opened log file '%s'\n", qalState.logFileName);

		qalState.logFileFrames = 1;

		time(&t);
		lt = localtime(&t);

		fprintf(qalState.logFile, "%s %s (%s %s)", ENGINE_VERSION, BUILD_STRING, __DATE__, __TIME__);
		fprintf(qalState.logFile, "\nLog file opened on %s\n\n", asctime(lt));

		// Change the function pointers
		qalBufferData					= logBufferData;
		qalBuffer3f						= logBuffer3f;
		qalBuffer3i						= logBuffer3i;
		qalBufferf						= logBufferf;
		qalBufferfv						= logBufferfv;
		qalBufferi						= logBufferi;
		qalBufferiv						= logBufferiv;
		qalDeleteBuffers				= logDeleteBuffers;
		qalDeleteSources				= logDeleteSources;
		qalDisable						= logDisable;
		qalDistanceModel				= logDistanceModel;
		qalDopplerFactor				= logDopplerFactor;
		qalDopplerVelocity				= logDopplerVelocity;
		qalEnable						= logEnable;
		qalGenBuffers					= logGenBuffers;
		qalGenSources					= logGenSources;
		qalGetBoolean					= logGetBoolean;
		qalGetBooleanv					= logGetBooleanv;
		qalGetBuffer3f					= logGetBuffer3f;
		qalGetBuffer3i					= logGetBuffer3i;
		qalGetBufferf					= logGetBufferf;
		qalGetBufferfv					= logGetBufferfv;
		qalGetBufferi					= logGetBufferi;
		qalGetBufferiv					= logGetBufferiv;
		qalGetDouble					= logGetDouble;
		qalGetDoublev					= logGetDoublev;
		qalGetEnumValue					= logGetEnumValue;
		qalGetError						= logGetError;
		qalGetFloat						= logGetFloat;
		qalGetFloatv					= logGetFloatv;
		qalGetInteger					= logGetInteger;
		qalGetIntegerv					= logGetIntegerv;
		qalGetListener3f				= logGetListener3f;
		qalGetListener3i				= logGetListener3i;
		qalGetListenerf					= logGetListenerf;
		qalGetListenerfv				= logGetListenerfv;
		qalGetListeneri					= logGetListeneri;
		qalGetListeneriv				= logGetListeneriv;
		qalGetProcAddress				= logGetProcAddress;
		qalGetSource3f					= logGetSource3f;
		qalGetSource3i					= logGetSource3i;
		qalGetSourcef					= logGetSourcef;
		qalGetSourcefv					= logGetSourcefv;
		qalGetSourcei					= logGetSourcei;
		qalGetSourceiv					= logGetSourceiv;
		qalGetString					= logGetString;
		qalIsBuffer						= logIsBuffer;
		qalIsEnabled					= logIsEnabled;
		qalIsExtensionPresent			= logIsExtensionPresent;
		qalIsSource						= logIsSource;
		qalListener3f					= logListener3f;
		qalListener3i					= logListener3i;
		qalListenerf					= logListenerf;
		qalListenerfv					= logListenerfv;
		qalListeneri					= logListeneri;
		qalListeneriv					= logListeneriv;
		qalSource3f						= logSource3f;
		qalSource3i						= logSource3i;
		qalSourcef						= logSourcef;
		qalSourcefv						= logSourcefv;
		qalSourcei						= logSourcei;
		qalSourceiv						= logSourceiv;
		qalSourcePause					= logSourcePause;
		qalSourcePausev					= logSourcePausev;
		qalSourcePlay					= logSourcePlay;
		qalSourcePlayv					= logSourcePlayv;
		qalSourceQueueBuffers			= logSourceQueueBuffers;
		qalSourceRewind					= logSourceRewind;
		qalSourceRewindv				= logSourceRewindv;
		qalSourceStop					= logSourceStop;
		qalSourceStopv					= logSourceStopv;
		qalSourceUnqueueBuffers			= logSourceUnqueueBuffers;
		qalSpeedOfSound					= logSpeedOfSound;

		qalAuxiliaryEffectSlotf			= logAuxiliaryEffectSlotf;
		qalAuxiliaryEffectSlotfv		= logAuxiliaryEffectSlotfv;
		qalAuxiliaryEffectSloti			= logAuxiliaryEffectSloti;
		qalAuxiliaryEffectSlotiv		= logAuxiliaryEffectSlotiv;
		qalDeleteAuxiliaryEffectSlots	= logDeleteAuxiliaryEffectSlots;
		qalDeleteEffects				= logDeleteEffects;
		qalDeleteFilters				= logDeleteFilters;
		qalEffectf						= logEffectf;
		qalEffectfv						= logEffectfv;
		qalEffecti						= logEffecti;
		qalEffectiv						= logEffectiv;
		qalFilterf						= logFilterf;
		qalFilterfv						= logFilterfv;
		qalFilteri						= logFilteri;
		qalFilteriv						= logFilteriv;
		qalGenAuxiliaryEffectSlots		= logGenAuxiliaryEffectSlots;
		qalGenEffects					= logGenEffects;
		qalGenFilters					= logGenFilters;
		qalGetAuxiliaryEffectSlotf		= logGetAuxiliaryEffectSlotf;
		qalGetAuxiliaryEffectSlotfv		= logGetAuxiliaryEffectSlotfv;
		qalGetAuxiliaryEffectSloti		= logGetAuxiliaryEffectSloti;
		qalGetAuxiliaryEffectSlotiv		= logGetAuxiliaryEffectSlotiv;
		qalGetEffectf					= logGetEffectf;
		qalGetEffectfv					= logGetEffectfv;
		qalGetEffecti					= logGetEffecti;
		qalGetEffectiv					= logGetEffectiv;
		qalGetFilterf					= logGetFilterf;
		qalGetFilterfv					= logGetFilterfv;
		qalGetFilteri					= logGetFilteri;
		qalGetFilteriv					= logGetFilteriv;
		qalIsAuxiliaryEffectSlot		= logIsAuxiliaryEffectSlot;
		qalIsEffect						= logIsEffect;
		qalIsFilter						= logIsFilter;

		qalEAXGetBufferMode				= logEAXGetBufferMode;
		qalEAXSetBufferMode				= logEAXSetBufferMode;
	}
	else {
		if (!qalState.logFile)
			return;

		// Close the file
		time(&t);
		lt = localtime(&t);

		fprintf(qalState.logFile, "\nLog file closed on %s\n\n", asctime(lt));

		fclose(qalState.logFile);
		qalState.logFile = NULL;

		Com_Printf("Closed log file '%s' after %i frames\n", qalState.logFileName, qalState.logFileFrames);

		// Reset the function pointers
		qalBufferData					= dllBufferData;
		qalBuffer3f						= dllBuffer3f;
		qalBuffer3i						= dllBuffer3i;
		qalBufferf						= dllBufferf;
		qalBufferfv						= dllBufferfv;
		qalBufferi						= dllBufferi;
		qalBufferiv						= dllBufferiv;
		qalDeleteBuffers				= dllDeleteBuffers;
		qalDeleteSources				= dllDeleteSources;
		qalDisable						= dllDisable;
		qalDistanceModel				= dllDistanceModel;
		qalDopplerFactor				= dllDopplerFactor;
		qalDopplerVelocity				= dllDopplerVelocity;
		qalEnable						= dllEnable;
		qalGenBuffers					= dllGenBuffers;
		qalGenSources					= dllGenSources;
		qalGetBoolean					= dllGetBoolean;
		qalGetBooleanv					= dllGetBooleanv;
		qalGetBuffer3f					= dllGetBuffer3f;
		qalGetBuffer3i					= dllGetBuffer3i;
		qalGetBufferf					= dllGetBufferf;
		qalGetBufferfv					= dllGetBufferfv;
		qalGetBufferi					= dllGetBufferi;
		qalGetBufferiv					= dllGetBufferiv;
		qalGetDouble					= dllGetDouble;
		qalGetDoublev					= dllGetDoublev;
		qalGetEnumValue					= dllGetEnumValue;
		qalGetError						= dllGetError;
		qalGetFloat						= dllGetFloat;
		qalGetFloatv					= dllGetFloatv;
		qalGetInteger					= dllGetInteger;
		qalGetIntegerv					= dllGetIntegerv;
		qalGetListener3f				= dllGetListener3f;
		qalGetListener3i				= dllGetListener3i;
		qalGetListenerf					= dllGetListenerf;
		qalGetListenerfv				= dllGetListenerfv;
		qalGetListeneri					= dllGetListeneri;
		qalGetListeneriv				= dllGetListeneriv;
		qalGetProcAddress				= dllGetProcAddress;
		qalGetSource3f					= dllGetSource3f;
		qalGetSource3i					= dllGetSource3i;
		qalGetSourcef					= dllGetSourcef;
		qalGetSourcefv					= dllGetSourcefv;
		qalGetSourcei					= dllGetSourcei;
		qalGetSourceiv					= dllGetSourceiv;
		qalGetString					= dllGetString;
		qalIsBuffer						= dllIsBuffer;
		qalIsEnabled					= dllIsEnabled;
		qalIsExtensionPresent			= dllIsExtensionPresent;
		qalIsSource						= dllIsSource;
		qalListener3f					= dllListener3f;
		qalListener3i					= dllListener3i;
		qalListenerf					= dllListenerf;
		qalListenerfv					= dllListenerfv;
		qalListeneri					= dllListeneri;
		qalListeneriv					= dllListeneriv;
		qalSource3f						= dllSource3f;
		qalSource3i						= dllSource3i;
		qalSourcef						= dllSourcef;
		qalSourcefv						= dllSourcefv;
		qalSourcei						= dllSourcei;
		qalSourceiv						= dllSourceiv;
		qalSourcePause					= dllSourcePause;
		qalSourcePausev					= dllSourcePausev;
		qalSourcePlay					= dllSourcePlay;
		qalSourcePlayv					= dllSourcePlayv;
		qalSourceQueueBuffers			= dllSourceQueueBuffers;
		qalSourceRewind					= dllSourceRewind;
		qalSourceRewindv				= dllSourceRewindv;
		qalSourceStop					= dllSourceStop;
		qalSourceStopv					= dllSourceStopv;
		qalSourceUnqueueBuffers			= dllSourceUnqueueBuffers;
		qalSpeedOfSound					= dllSpeedOfSound;

		qalAuxiliaryEffectSlotf			= dllAuxiliaryEffectSlotf;
		qalAuxiliaryEffectSlotfv		= dllAuxiliaryEffectSlotfv;
		qalAuxiliaryEffectSloti			= dllAuxiliaryEffectSloti;
		qalAuxiliaryEffectSlotiv		= dllAuxiliaryEffectSlotiv;
		qalDeleteAuxiliaryEffectSlots	= dllDeleteAuxiliaryEffectSlots;
		qalDeleteEffects				= dllDeleteEffects;
		qalDeleteFilters				= dllDeleteFilters;
		qalEffectf						= dllEffectf;
		qalEffectfv						= dllEffectfv;
		qalEffecti						= dllEffecti;
		qalEffectiv						= dllEffectiv;
		qalFilterf						= dllFilterf;
		qalFilterfv						= dllFilterfv;
		qalFilteri						= dllFilteri;
		qalFilteriv						= dllFilteriv;
		qalGenAuxiliaryEffectSlots		= dllGenAuxiliaryEffectSlots;
		qalGenEffects					= dllGenEffects;
		qalGenFilters					= dllGenFilters;
		qalGetAuxiliaryEffectSlotf		= dllGetAuxiliaryEffectSlotf;
		qalGetAuxiliaryEffectSlotfv		= dllGetAuxiliaryEffectSlotfv;
		qalGetAuxiliaryEffectSloti		= dllGetAuxiliaryEffectSloti;
		qalGetAuxiliaryEffectSlotiv		= dllGetAuxiliaryEffectSlotiv;
		qalGetEffectf					= dllGetEffectf;
		qalGetEffectfv					= dllGetEffectfv;
		qalGetEffecti					= dllGetEffecti;
		qalGetEffectiv					= dllGetEffectiv;
		qalGetFilterf					= dllGetFilterf;
		qalGetFilterfv					= dllGetFilterfv;
		qalGetFilteri					= dllGetFilteri;
		qalGetFilteriv					= dllGetFilteriv;
		qalIsAuxiliaryEffectSlot		= dllIsAuxiliaryEffectSlot;
		qalIsEffect						= dllIsEffect;
		qalIsFilter						= dllIsFilter;

		qalEAXGetBufferMode				= dllEAXGetBufferMode;
		qalEAXSetBufferMode				= dllEAXSetBufferMode;
	}
}

/*
 ==================
 QAL_LogPrintf
 ==================
*/
void QAL_LogPrintf (const char *fmt, ...){

	va_list	argPtr;

	if (!qalState.logFile)
		return;

	va_start(argPtr, fmt);
	vfprintf(qalState.logFile, fmt, argPtr);
	va_end(argPtr);
}

/*
 ==================
 QAL_GetProcAddress
 ==================
*/
static void *QAL_GetProcAddress (const char *procName){

	void	*procAddress;

	procAddress = GetProcAddress(qalState.hModule, procName);
	if (!procAddress){
		FreeLibrary(qalState.hModule);
		qalState.hModule = NULL;

		Com_Error(ERR_FATAL, "QAL_GetProcAddress: GetProcAddress() failed for '%s'", procName);
	}

	return procAddress;
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 QAL_Init

 Loads the specified DLL then binds our QAL function pointers to the
 appropriate AL stuff
 ==================
*/
bool QAL_Init (const char *driver){

	char	name[MAX_PATH_LENGTH];

	Com_Printf("...initializing QAL\n");

	Str_Copy(name, driver, sizeof(name));
	Str_DefaultFileExtension(name, sizeof(name), LIBRARY_EXTENSION);

	Com_Printf("...calling LoadLibrary( '%s' ): ", name);

	if ((qalState.hModule = LoadLibrary(name)) == NULL){
		Com_Printf("failed\n");
		return false;
	}

	Com_Printf("succeeded\n");

	qalcCaptureCloseDevice			= (ALCCAPTURECLOSEDEVICE)QAL_GetProcAddress("alcCaptureCloseDevice");
	qalcCaptureOpenDevice			= (ALCCAPTUREOPENDEVICE)QAL_GetProcAddress("alcCaptureOpenDevice");
	qalcCaptureSamples				= (ALCCAPTURESAMPLES)QAL_GetProcAddress("alcCaptureSamples");
	qalcCaptureStart				= (ALCCAPTURESTART)QAL_GetProcAddress("alcCaptureStart");
	qalcCaptureStop					= (ALCCAPTURESTOP)QAL_GetProcAddress("alcCaptureStop");
	qalcCloseDevice					= (ALCCLOSEDEVICE)QAL_GetProcAddress("alcCloseDevice");
	qalcCreateContext				= (ALCCREATECONTEXT)QAL_GetProcAddress("alcCreateContext");
	qalcDestroyContext				= (ALCDESTROYCONTEXT)QAL_GetProcAddress("alcDestroyContext");
	qalcGetContextsDevice			= (ALCGETCONTEXTSDEVICE)QAL_GetProcAddress("alcGetContextsDevice");
	qalcGetCurrentContext			= (ALCGETCURRENTCONTEXT)QAL_GetProcAddress("alcGetCurrentContext");
	qalcGetEnumValue				= (ALCGETENUMVALUE)QAL_GetProcAddress("alcGetEnumValue");
	qalcGetError					= (ALCGETERROR)QAL_GetProcAddress("alcGetError");
	qalcGetIntegerv					= (ALCGETINTEGERV)QAL_GetProcAddress("alcGetIntegerv");
	qalcGetProcAddress				= (ALCGETPROCADDRESS)QAL_GetProcAddress("alcGetProcAddress");
	qalcGetString					= (ALCGETSTRING)QAL_GetProcAddress("alcGetString");
	qalcIsExtensionPresent			= (ALCISEXTENSIONPRESENT)QAL_GetProcAddress("alcIsExtensionPresent");
	qalcMakeContextCurrent			= (ALCMAKECONTEXTCURRENT)QAL_GetProcAddress("alcMakeContextCurrent");
	qalcOpenDevice					= (ALCOPENDEVICE)QAL_GetProcAddress("alcOpenDevice");
	qalcProcessContext				= (ALCPROCESSCONTEXT)QAL_GetProcAddress("alcProcessContext");
	qalcSuspendContext				= (ALCSUSPENDCONTEXT)QAL_GetProcAddress("alcSuspendContext");

	qalBufferData					= (ALBUFFERDATA)QAL_GetProcAddress("alBufferData");
	qalBuffer3f						= (ALBUFFER3F)QAL_GetProcAddress("alBuffer3f");
	qalBuffer3i						= (ALBUFFER3I)QAL_GetProcAddress("alBuffer3i");
	qalBufferf						= (ALBUFFERF)QAL_GetProcAddress("alBufferf");
	qalBufferfv						= (ALBUFFERFV)QAL_GetProcAddress("alBufferfv");
	qalBufferi						= (ALBUFFERI)QAL_GetProcAddress("alBufferi");
	qalBufferiv						= (ALBUFFERIV)QAL_GetProcAddress("alBufferiv");
	qalDeleteBuffers				= (ALDELETEBUFFERS)QAL_GetProcAddress("alDeleteBuffers");
	qalDeleteSources				= (ALDELETESOURCES)QAL_GetProcAddress("alDeleteSources");
	qalDisable						= (ALDISABLE)QAL_GetProcAddress("alDisable");
	qalDistanceModel				= (ALDISTANCEMODEL)QAL_GetProcAddress("alDistanceModel");
	qalDopplerFactor				= (ALDOPPLERFACTOR)QAL_GetProcAddress("alDopplerFactor");
	qalDopplerVelocity				= (ALDOPPLERVELOCITY)QAL_GetProcAddress("alDopplerVelocity");
	qalEnable						= (ALENABLE)QAL_GetProcAddress("alEnable");
	qalGenBuffers					= (ALGENBUFFERS)QAL_GetProcAddress("alGenBuffers");
	qalGenSources					= (ALGENSOURCES)QAL_GetProcAddress("alGenSources");
	qalGetBoolean					= (ALGETBOOLEAN)QAL_GetProcAddress("alGetBoolean");
	qalGetBooleanv					= (ALGETBOOLEANV)QAL_GetProcAddress("alGetBooleanv");
	qalGetBuffer3f					= (ALGETBUFFER3F)QAL_GetProcAddress("alGetBuffer3f");
	qalGetBuffer3i					= (ALGETBUFFER3I)QAL_GetProcAddress("alGetBuffer3i");
	qalGetBufferf					= (ALGETBUFFERF)QAL_GetProcAddress("alGetBufferf");
	qalGetBufferfv					= (ALGETBUFFERFV)QAL_GetProcAddress("alGetBufferfv");
	qalGetBufferi					= (ALGETBUFFERI)QAL_GetProcAddress("alGetBufferi");
	qalGetBufferiv					= (ALGETBUFFERIV)QAL_GetProcAddress("alGetBufferiv");
	qalGetDouble					= (ALGETDOUBLE)QAL_GetProcAddress("alGetDouble");
	qalGetDoublev					= (ALGETDOUBLEV)QAL_GetProcAddress("alGetDoublev");
	qalGetEnumValue					= (ALGETENUMVALUE)QAL_GetProcAddress("alGetEnumValue");
	qalGetError						= (ALGETERROR)QAL_GetProcAddress("alGetError");
	qalGetFloat						= (ALGETFLOAT)QAL_GetProcAddress("alGetFloat");
	qalGetFloatv					= (ALGETFLOATV)QAL_GetProcAddress("alGetFloatv");
	qalGetInteger					= (ALGETINTEGER)QAL_GetProcAddress("alGetInteger");
	qalGetIntegerv					= (ALGETINTEGERV)QAL_GetProcAddress("alGetIntegerv");
	qalGetListener3f				= (ALGETLISTENER3F)QAL_GetProcAddress("alGetListener3f");
	qalGetListener3i				= (ALGETLISTENER3I)QAL_GetProcAddress("alGetListener3i");
	qalGetListenerf					= (ALGETLISTENERF)QAL_GetProcAddress("alGetListenerf");
	qalGetListenerfv				= (ALGETLISTENERFV)QAL_GetProcAddress("alGetListenerfv");
	qalGetListeneri					= (ALGETLISTENERI)QAL_GetProcAddress("alGetListeneri");
	qalGetListeneriv				= (ALGETLISTENERIV)QAL_GetProcAddress("alGetListeneriv");
	qalGetProcAddress				= (ALGETPROCADDRESS)QAL_GetProcAddress("alGetProcAddress");
	qalGetSource3f					= (ALGETSOURCE3F)QAL_GetProcAddress("alGetSource3f");
	qalGetSource3i					= (ALGETSOURCE3I)QAL_GetProcAddress("alGetSource3i");
	qalGetSourcef					= (ALGETSOURCEF)QAL_GetProcAddress("alGetSourcef");
	qalGetSourcefv					= (ALGETSOURCEFV)QAL_GetProcAddress("alGetSourcefv");
	qalGetSourcei					= (ALGETSOURCEI)QAL_GetProcAddress("alGetSourcei");
	qalGetSourceiv					= (ALGETSOURCEIV)QAL_GetProcAddress("alGetSourceiv");
	qalGetString					= (ALGETSTRING)QAL_GetProcAddress("alGetString");
	qalIsBuffer						= (ALISBUFFER)QAL_GetProcAddress("alIsBuffer");
	qalIsEnabled					= (ALISENABLED)QAL_GetProcAddress("alIsEnabled");
	qalIsExtensionPresent			= (ALISEXTENSIONPRESENT)QAL_GetProcAddress("alIsExtensionPresent");
	qalIsSource						= (ALISSOURCE)QAL_GetProcAddress("alIsSource");
	qalListener3f					= (ALLISTENER3F)QAL_GetProcAddress("alListener3f");
	qalListener3i					= (ALLISTENER3I)QAL_GetProcAddress("alListener3i");
	qalListenerf					= (ALLISTENERF)QAL_GetProcAddress("alListenerf");
	qalListenerfv					= (ALLISTENERFV)QAL_GetProcAddress("alListenerfv");
	qalListeneri					= (ALLISTENERI)QAL_GetProcAddress("alListeneri");
	qalListeneriv					= (ALLISTENERIV)QAL_GetProcAddress("alListeneriv");
	qalSource3f						= (ALSOURCE3F)QAL_GetProcAddress("alSource3f");
	qalSource3i						= (ALSOURCE3I)QAL_GetProcAddress("alSource3i");
	qalSourcef						= (ALSOURCEF)QAL_GetProcAddress("alSourcef");
	qalSourcefv						= (ALSOURCEFV)QAL_GetProcAddress("alSourcefv");
	qalSourcei						= (ALSOURCEI)QAL_GetProcAddress("alSourcei");
	qalSourceiv						= (ALSOURCEIV)QAL_GetProcAddress("alSourceiv");
	qalSourcePause					= (ALSOURCEPAUSE)QAL_GetProcAddress("alSourcePause");
	qalSourcePausev					= (ALSOURCEPAUSEV)QAL_GetProcAddress("alSourcePausev");
	qalSourcePlay					= (ALSOURCEPLAY)QAL_GetProcAddress("alSourcePlay");
	qalSourcePlayv					= (ALSOURCEPLAYV)QAL_GetProcAddress("alSourcePlayv");
	qalSourceQueueBuffers			= (ALSOURCEQUEUEBUFFERS)QAL_GetProcAddress("alSourceQueueBuffers");
	qalSourceRewind					= (ALSOURCEREWIND)QAL_GetProcAddress("alSourceRewind");
	qalSourceRewindv				= (ALSOURCEREWINDV)QAL_GetProcAddress("alSourceRewindv");
	qalSourceStop					= (ALSOURCESTOP)QAL_GetProcAddress("alSourceStop");
	qalSourceStopv					= (ALSOURCESTOPV)QAL_GetProcAddress("alSourceStopv");
	qalSourceUnqueueBuffers			= (ALSOURCEUNQUEUEBUFFERS)QAL_GetProcAddress("alSourceUnqueueBuffers");
	qalSpeedOfSound					= (ALSPEEDOFSOUND)QAL_GetProcAddress("alSpeedOfSound");

	qalAuxiliaryEffectSlotf			= NULL;
	qalAuxiliaryEffectSlotfv		= NULL;
	qalAuxiliaryEffectSloti			= NULL;
	qalAuxiliaryEffectSlotiv		= NULL;
	qalDeleteAuxiliaryEffectSlots	= NULL;
	qalDeleteEffects				= NULL;
	qalDeleteFilters				= NULL;
	qalEffectf						= NULL;
	qalEffectfv						= NULL;
	qalEffecti						= NULL;
	qalEffectiv						= NULL;
	qalFilterf						= NULL;
	qalFilterfv						= NULL;
	qalFilteri						= NULL;
	qalFilteriv						= NULL;
	qalGenAuxiliaryEffectSlots		= NULL;
	qalGenEffects					= NULL;
	qalGenFilters					= NULL;
	qalGetAuxiliaryEffectSlotf		= NULL;
	qalGetAuxiliaryEffectSlotfv		= NULL;
	qalGetAuxiliaryEffectSloti		= NULL;
	qalGetAuxiliaryEffectSlotiv		= NULL;
	qalGetEffectf					= NULL;
	qalGetEffectfv					= NULL;
	qalGetEffecti					= NULL;
	qalGetEffectiv					= NULL;
	qalGetFilterf					= NULL;
	qalGetFilterfv					= NULL;
	qalGetFilteri					= NULL;
	qalGetFilteriv					= NULL;
	qalIsAuxiliaryEffectSlot		= NULL;
	qalIsEffect						= NULL;
	qalIsFilter						= NULL;

	qalEAXGetBufferMode				= NULL;
	qalEAXSetBufferMode				= NULL;

	return true;
}

/*
 ==================
 QAL_Shutdown

 Unloads the specified DLL then nulls out all the QAL function pointers
 ==================
*/
void QAL_Shutdown (){

	Com_Printf("...shutting down QAL\n");

	if (qalState.logFile){
		fclose(qalState.logFile);
		qalState.logFile = NULL;
	}

	if (qalState.hModule){
		Com_Printf("...unloading OpenAL DLL\n");

		FreeLibrary(qalState.hModule);
		qalState.hModule = NULL;
	}

	Mem_Fill(&qalState, 0, sizeof(qalState_t));

	qalcCaptureCloseDevice			= NULL;
	qalcCaptureOpenDevice			= NULL;
	qalcCaptureSamples				= NULL;
	qalcCaptureStart				= NULL;
	qalcCaptureStop					= NULL;
	qalcCloseDevice					= NULL;
	qalcCreateContext				= NULL;
	qalcDestroyContext				= NULL;
	qalcGetContextsDevice			= NULL;
	qalcGetCurrentContext			= NULL;
	qalcGetEnumValue				= NULL;
	qalcGetError					= NULL;
	qalcGetIntegerv					= NULL;
	qalcGetProcAddress				= NULL;
	qalcGetString					= NULL;
	qalcIsExtensionPresent			= NULL;
	qalcMakeContextCurrent			= NULL;
	qalcOpenDevice					= NULL;
	qalcProcessContext				= NULL;
	qalcSuspendContext				= NULL;

	qalBufferData					= NULL;
	qalBuffer3f						= NULL;
	qalBuffer3i						= NULL;
	qalBufferf						= NULL;
	qalBufferfv						= NULL;
	qalBufferi						= NULL;
	qalBufferiv						= NULL;
	qalDeleteBuffers				= NULL;
	qalDeleteSources				= NULL;
	qalDisable						= NULL;
	qalDistanceModel				= NULL;
	qalDopplerFactor				= NULL;
	qalDopplerVelocity				= NULL;
	qalEnable						= NULL;
	qalGenBuffers					= NULL;
	qalGenSources					= NULL;
	qalGetBoolean					= NULL;
	qalGetBooleanv					= NULL;
	qalGetBuffer3f					= NULL;
	qalGetBuffer3i					= NULL;
	qalGetBufferf					= NULL;
	qalGetBufferfv					= NULL;
	qalGetBufferi					= NULL;
	qalGetBufferiv					= NULL;
	qalGetDouble					= NULL;
	qalGetDoublev					= NULL;
	qalGetEnumValue					= NULL;
	qalGetError						= NULL;
	qalGetFloat						= NULL;
	qalGetFloatv					= NULL;
	qalGetInteger					= NULL;
	qalGetIntegerv					= NULL;
	qalGetListener3f				= NULL;
	qalGetListener3i				= NULL;
	qalGetListenerf					= NULL;
	qalGetListenerfv				= NULL;
	qalGetListeneri					= NULL;
	qalGetListeneriv				= NULL;
	qalGetProcAddress				= NULL;
	qalGetSource3f					= NULL;
	qalGetSource3i					= NULL;
	qalGetSourcef					= NULL;
	qalGetSourcefv					= NULL;
	qalGetSourcei					= NULL;
	qalGetSourceiv					= NULL;
	qalGetString					= NULL;
	qalIsBuffer						= NULL;
	qalIsEnabled					= NULL;
	qalIsExtensionPresent			= NULL;
	qalIsSource						= NULL;
	qalListener3f					= NULL;
	qalListener3i					= NULL;
	qalListenerf					= NULL;
	qalListenerfv					= NULL;
	qalListeneri					= NULL;
	qalListeneriv					= NULL;
	qalSource3f						= NULL;
	qalSource3i						= NULL;
	qalSourcef						= NULL;
	qalSourcefv						= NULL;
	qalSourcei						= NULL;
	qalSourceiv						= NULL;
	qalSourcePause					= NULL;
	qalSourcePausev					= NULL;
	qalSourcePlay					= NULL;
	qalSourcePlayv					= NULL;
	qalSourceQueueBuffers			= NULL;
	qalSourceRewind					= NULL;
	qalSourceRewindv				= NULL;
	qalSourceStop					= NULL;
	qalSourceStopv					= NULL;
	qalSourceUnqueueBuffers			= NULL;
	qalSpeedOfSound					= NULL;

	qalAuxiliaryEffectSlotf			= NULL;
	qalAuxiliaryEffectSlotfv		= NULL;
	qalAuxiliaryEffectSloti			= NULL;
	qalAuxiliaryEffectSlotiv		= NULL;
	qalDeleteAuxiliaryEffectSlots	= NULL;
	qalDeleteEffects				= NULL;
	qalDeleteFilters				= NULL;
	qalEffectf						= NULL;
	qalEffectfv						= NULL;
	qalEffecti						= NULL;
	qalEffectiv						= NULL;
	qalFilterf						= NULL;
	qalFilterfv						= NULL;
	qalFilteri						= NULL;
	qalFilteriv						= NULL;
	qalGenAuxiliaryEffectSlots		= NULL;
	qalGenEffects					= NULL;
	qalGenFilters					= NULL;
	qalGetAuxiliaryEffectSlotf		= NULL;
	qalGetAuxiliaryEffectSlotfv		= NULL;
	qalGetAuxiliaryEffectSloti		= NULL;
	qalGetAuxiliaryEffectSlotiv		= NULL;
	qalGetEffectf					= NULL;
	qalGetEffectfv					= NULL;
	qalGetEffecti					= NULL;
	qalGetEffectiv					= NULL;
	qalGetFilterf					= NULL;
	qalGetFilterfv					= NULL;
	qalGetFilteri					= NULL;
	qalGetFilteriv					= NULL;
	qalIsAuxiliaryEffectSlot		= NULL;
	qalIsEffect						= NULL;
	qalIsFilter						= NULL;

	qalEAXGetBufferMode				= NULL;
	qalEAXSetBufferMode				= NULL;
}