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
// qal.h - Binding of AL to QAL function pointers
//


#ifndef __QAL_H__
#define __QAL_H__


#include "../include/OpenAL/al.h"
#include "../include/OpenAL/alc.h"
#include "../include/OpenAL/efx.h"
#include "../include/OpenAL/efx-creative.h"


// ============================================================================

extern ALenum				AL_EAX_RAM_SIZE;
extern ALenum				AL_EAX_RAM_FREE;
extern ALenum				AL_STORAGE_AUTOMATIC;
extern ALenum				AL_STORAGE_HARDWARE;
extern ALenum				AL_STORAGE_ACCESSIBLE;

// ============================================================================

typedef ALCboolean			(ALCAPIENTRY * ALCCAPTURECLOSEDEVICE)(ALCdevice *);
typedef ALCdevice *			(ALCAPIENTRY * ALCCAPTUREOPENDEVICE)(const ALCchar *, ALCuint, ALCenum, ALCsizei);
typedef ALCvoid				(ALCAPIENTRY * ALCCAPTURESAMPLES)(ALCdevice *, ALCvoid *, ALCsizei);
typedef ALCvoid				(ALCAPIENTRY * ALCCAPTURESTART)(ALCdevice *);
typedef ALCvoid				(ALCAPIENTRY * ALCCAPTURESTOP)(ALCdevice *);
typedef ALCboolean			(ALCAPIENTRY * ALCCLOSEDEVICE)(ALCdevice *);
typedef ALCcontext *		(ALCAPIENTRY * ALCCREATECONTEXT)(ALCdevice *, ALCint *);
typedef ALCvoid				(ALCAPIENTRY * ALCDESTROYCONTEXT)(ALCcontext *);
typedef ALCdevice *			(ALCAPIENTRY * ALCGETCONTEXTSDEVICE)(ALCcontext *);
typedef ALCcontext *		(ALCAPIENTRY * ALCGETCURRENTCONTEXT)(ALCvoid);
typedef ALCenum				(ALCAPIENTRY * ALCGETENUMVALUE)(ALCdevice *, const ALCchar *);
typedef ALCenum				(ALCAPIENTRY * ALCGETERROR)(ALCdevice *);
typedef ALCvoid				(ALCAPIENTRY * ALCGETINTEGERV)(ALCdevice *, ALCenum, ALCsizei, ALCint *);
typedef ALCvoid *			(ALCAPIENTRY * ALCGETPROCADDRESS)(ALCdevice *, const ALCchar *);
typedef const ALCchar *		(ALCAPIENTRY * ALCGETSTRING)(ALCdevice *, ALCenum);
typedef ALCboolean			(ALCAPIENTRY * ALCISEXTENSIONPRESENT)(ALCdevice *, const ALCchar *);
typedef ALCboolean			(ALCAPIENTRY * ALCMAKECONTEXTCURRENT)(ALCcontext *);
typedef ALCdevice *			(ALCAPIENTRY * ALCOPENDEVICE)(const ALCchar *);
typedef ALCvoid				(ALCAPIENTRY * ALCPROCESSCONTEXT)(ALCcontext *);
typedef ALCvoid				(ALCAPIENTRY * ALCSUSPENDCONTEXT)(ALCcontext *);

typedef ALvoid				(ALAPIENTRY * ALBUFFERDATA)(ALuint, ALenum, const ALvoid *, ALsizei, ALsizei);
typedef ALvoid				(ALAPIENTRY * ALBUFFER3F)(ALuint, ALenum, ALfloat, ALfloat, ALfloat);
typedef ALvoid				(ALAPIENTRY * ALBUFFER3I)(ALuint, ALenum, ALint, ALint, ALint);
typedef ALvoid				(ALAPIENTRY * ALBUFFERF)(ALuint, ALenum, ALfloat);
typedef ALvoid				(ALAPIENTRY * ALBUFFERFV)(ALuint, ALenum, const ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALBUFFERI)(ALuint, ALenum, ALint);
typedef ALvoid				(ALAPIENTRY * ALBUFFERIV)(ALuint, ALenum, const ALint *);
typedef ALvoid				(ALAPIENTRY * ALDELETEBUFFERS)(ALsizei, const ALuint *);
typedef ALvoid				(ALAPIENTRY * ALDELETESOURCES)(ALsizei, const ALuint *);
typedef ALvoid				(ALAPIENTRY * ALDISABLE)(ALenum);
typedef ALvoid				(ALAPIENTRY * ALDISTANCEMODEL)(ALenum);
typedef ALvoid				(ALAPIENTRY * ALDOPPLERFACTOR)(ALfloat);
typedef ALvoid				(ALAPIENTRY * ALDOPPLERVELOCITY)(ALfloat);
typedef ALvoid				(ALAPIENTRY * ALENABLE)(ALenum);
typedef ALvoid				(ALAPIENTRY * ALGENBUFFERS)(ALsizei, ALuint *);
typedef ALvoid				(ALAPIENTRY * ALGENSOURCES)(ALsizei, ALuint *);
typedef ALboolean			(ALAPIENTRY * ALGETBOOLEAN)(ALenum);
typedef ALvoid				(ALAPIENTRY * ALGETBOOLEANV)(ALenum, ALboolean *);
typedef ALvoid				(ALAPIENTRY * ALGETBUFFER3F)(ALuint, ALenum, ALfloat *, ALfloat *, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETBUFFER3I)(ALuint, ALenum, ALint *, ALint *, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETBUFFERF)(ALuint, ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETBUFFERFV)(ALuint, ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETBUFFERI)(ALuint, ALenum, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETBUFFERIV)(ALuint, ALenum, ALint *);
typedef ALdouble			(ALAPIENTRY * ALGETDOUBLE)(ALenum);
typedef ALvoid				(ALAPIENTRY * ALGETDOUBLEV)(ALenum, ALdouble *);
typedef ALenum				(ALAPIENTRY * ALGETENUMVALUE)(const ALchar *);
typedef ALenum				(ALAPIENTRY * ALGETERROR)(ALvoid);
typedef ALfloat				(ALAPIENTRY * ALGETFLOAT)(ALenum);
typedef ALvoid				(ALAPIENTRY * ALGETFLOATV)(ALenum, ALfloat *);
typedef ALint				(ALAPIENTRY * ALGETINTEGER)(ALenum);
typedef ALvoid				(ALAPIENTRY * ALGETINTEGERV)(ALenum, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETLISTENER3F)(ALenum, ALfloat *, ALfloat *, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETLISTENER3I)(ALenum, ALint *, ALint *, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETLISTENERF)(ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETLISTENERFV)(ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETLISTENERI)(ALenum, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETLISTENERIV)(ALenum, ALint *);
typedef ALvoid *			(ALAPIENTRY * ALGETPROCADDRESS)(const ALchar *);
typedef ALvoid				(ALAPIENTRY * ALGETSOURCE3F)(ALuint, ALenum, ALfloat *, ALfloat *, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETSOURCE3I)(ALuint, ALenum, ALint *, ALint *, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETSOURCEF)(ALuint, ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETSOURCEFV)(ALuint, ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETSOURCEI)(ALuint, ALenum, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETSOURCEIV)(ALuint, ALenum, ALint *);
typedef const ALchar *		(ALAPIENTRY * ALGETSTRING)(ALenum);
typedef ALboolean			(ALAPIENTRY * ALISBUFFER)(ALuint);
typedef ALboolean			(ALAPIENTRY * ALISENABLED)(ALenum);
typedef ALboolean			(ALAPIENTRY * ALISEXTENSIONPRESENT)(const ALchar *);
typedef ALboolean			(ALAPIENTRY * ALISSOURCE)(ALuint);
typedef ALvoid				(ALAPIENTRY * ALLISTENER3F)(ALenum, ALfloat, ALfloat, ALfloat);
typedef ALvoid				(ALAPIENTRY * ALLISTENER3I)(ALenum, ALint, ALint, ALint);
typedef ALvoid				(ALAPIENTRY * ALLISTENERF)(ALenum, ALfloat);
typedef ALvoid				(ALAPIENTRY * ALLISTENERFV)(ALenum, const ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALLISTENERI)(ALenum, ALint);
typedef ALvoid				(ALAPIENTRY * ALLISTENERIV)(ALenum, const ALint *);
typedef ALvoid				(ALAPIENTRY * ALSOURCE3F)(ALuint, ALenum, ALfloat, ALfloat, ALfloat);
typedef ALvoid				(ALAPIENTRY * ALSOURCE3I)(ALuint, ALenum, ALint, ALint, ALint);
typedef ALvoid				(ALAPIENTRY * ALSOURCEF)(ALuint, ALenum, ALfloat);
typedef ALvoid				(ALAPIENTRY * ALSOURCEFV)(ALuint, ALenum, const ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALSOURCEI)(ALuint, ALenum, ALint);
typedef ALvoid				(ALAPIENTRY * ALSOURCEIV)(ALuint, ALenum, const ALint *);
typedef ALvoid				(ALAPIENTRY * ALSOURCEPAUSE)(ALuint);
typedef ALvoid				(ALAPIENTRY * ALSOURCEPAUSEV)(ALsizei, const ALuint *);
typedef ALvoid				(ALAPIENTRY * ALSOURCEPLAY)(ALuint);
typedef ALvoid				(ALAPIENTRY * ALSOURCEPLAYV)(ALsizei, const ALuint *);
typedef ALvoid				(ALAPIENTRY * ALSOURCEQUEUEBUFFERS)(ALuint, ALsizei, const ALuint *);
typedef ALvoid				(ALAPIENTRY * ALSOURCEREWIND)(ALuint);
typedef ALvoid				(ALAPIENTRY * ALSOURCEREWINDV)(ALsizei, const ALuint *);
typedef ALvoid				(ALAPIENTRY * ALSOURCESTOP)(ALuint);
typedef ALvoid				(ALAPIENTRY * ALSOURCESTOPV)(ALsizei, const ALuint *);
typedef ALvoid				(ALAPIENTRY * ALSOURCEUNQUEUEBUFFERS)(ALuint, ALsizei, ALuint *);
typedef ALvoid				(ALAPIENTRY * ALSPEEDOFSOUND)(ALfloat);

typedef ALvoid				(ALAPIENTRY * ALAUXILIARYEFFECTSLOTF)(ALuint, ALenum, ALfloat);
typedef ALvoid				(ALAPIENTRY * ALAUXILIARYEFFECTSLOTFV)(ALuint, ALenum, const ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALAUXILIARYEFFECTSLOTI)(ALuint, ALenum, ALint);
typedef ALvoid				(ALAPIENTRY * ALAUXILIARYEFFECTSLOTIV)(ALuint, ALenum, const ALint *);
typedef ALvoid				(ALAPIENTRY * ALDELETEAUXILIARYEFFECTSLOTS)(ALsizei n, const ALuint *);
typedef ALvoid				(ALAPIENTRY * ALDELETEEFFECTS)(ALsizei, const ALuint *);
typedef ALvoid				(ALAPIENTRY * ALDELETEFILTERS)(ALsizei, const ALuint *);
typedef ALvoid				(ALAPIENTRY * ALEFFECTF)(ALuint, ALenum, ALfloat);
typedef ALvoid				(ALAPIENTRY * ALEFFECTFV)(ALuint, ALenum, const ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALEFFECTI)(ALuint, ALenum, ALint);
typedef ALvoid				(ALAPIENTRY * ALEFFECTIV)(ALuint, ALenum, const ALint *);
typedef ALvoid				(ALAPIENTRY * ALFILTERF)(ALuint, ALenum, ALfloat);
typedef ALvoid				(ALAPIENTRY * ALFILTERFV)(ALuint, ALenum, const ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALFILTERI)(ALuint, ALenum, ALint);
typedef ALvoid				(ALAPIENTRY * ALFILTERIV)(ALuint, ALenum, const ALint *);
typedef ALvoid				(ALAPIENTRY * ALGENAUXILIARYEFFECTSLOTS)(ALsizei, ALuint *);
typedef ALvoid				(ALAPIENTRY * ALGENEFFECTS)(ALsizei, ALuint *);
typedef ALvoid				(ALAPIENTRY * ALGENFILTERS)(ALsizei, ALuint *);
typedef ALvoid				(ALAPIENTRY * ALGETAUXILIARYEFFECTSLOTF)(ALuint, ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETAUXILIARYEFFECTSLOTFV)(ALuint, ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETAUXILIARYEFFECTSLOTI)(ALuint, ALenum, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETAUXILIARYEFFECTSLOTIV)(ALuint, ALenum, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETEFFECTF)(ALuint, ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETEFFECTFV)(ALuint, ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETEFFECTI)(ALuint, ALenum, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETEFFECTIV)(ALuint, ALenum, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETFILTERF)(ALuint, ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETFILTERFV)(ALuint, ALenum, ALfloat *);
typedef ALvoid				(ALAPIENTRY * ALGETFILTERI)(ALuint, ALenum, ALint *);
typedef ALvoid				(ALAPIENTRY * ALGETFILTERIV)(ALuint, ALenum, ALint *);
typedef ALboolean			(ALAPIENTRY * ALISAUXILIARYEFFECTSLOT)(ALuint);
typedef ALboolean			(ALAPIENTRY * ALISEFFECT)(ALuint);
typedef ALboolean			(ALAPIENTRY * ALISFILTER)(ALuint);

typedef ALenum				(ALAPIENTRY * ALEAXGETBUFFERMODE)(ALuint, ALenum *);
typedef ALboolean			(ALAPIENTRY * ALEAXSETBUFFERMODE)(ALsizei, const ALuint *, ALenum);

// ============================================================================

extern ALCboolean			(ALCAPIENTRY * qalcCaptureCloseDevice)(ALCdevice *device);
extern ALCdevice *			(ALCAPIENTRY * qalcCaptureOpenDevice)(const ALCchar *deviceName, ALCuint frequency, ALCenum format, ALCsizei samples);
extern ALCvoid				(ALCAPIENTRY * qalcCaptureSamples)(ALCdevice *device, ALCvoid *buffer, ALCsizei samples);
extern ALCvoid				(ALCAPIENTRY * qalcCaptureStart)(ALCdevice *device);
extern ALCvoid				(ALCAPIENTRY * qalcCaptureStop)(ALCdevice *device);
extern ALCboolean			(ALCAPIENTRY * qalcCloseDevice)(ALCdevice *device);
extern ALCcontext *			(ALCAPIENTRY * qalcCreateContext)(ALCdevice *device, ALCint *attrList);
extern ALCvoid				(ALCAPIENTRY * qalcDestroyContext)(ALCcontext *context);
extern ALCdevice *			(ALCAPIENTRY * qalcGetContextsDevice)(ALCcontext *context);
extern ALCcontext *			(ALCAPIENTRY * qalcGetCurrentContext)(ALCvoid);
extern ALCenum				(ALCAPIENTRY * qalcGetEnumValue)(ALCdevice *device, const ALCchar *enumName);
extern ALCenum				(ALCAPIENTRY * qalcGetError)(ALCdevice *device);
extern ALCvoid				(ALCAPIENTRY * qalcGetIntegerv)(ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data);
extern ALCvoid *			(ALCAPIENTRY * qalcGetProcAddress)(ALCdevice *device, const ALCchar *funcName);
extern const ALCchar *		(ALCAPIENTRY * qalcGetString)(ALCdevice *device, ALCenum param);
extern ALCboolean			(ALCAPIENTRY * qalcIsExtensionPresent)(ALCdevice *device, const ALCchar *extName);
extern ALCboolean			(ALCAPIENTRY * qalcMakeContextCurrent)(ALCcontext *context);
extern ALCdevice *			(ALCAPIENTRY * qalcOpenDevice)(const ALCchar *deviceName);
extern ALCvoid				(ALCAPIENTRY * qalcProcessContext)(ALCcontext *context);
extern ALCvoid				(ALCAPIENTRY * qalcSuspendContext)(ALCcontext *context);

extern ALvoid				(ALAPIENTRY * qalBufferData)(ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq);
extern ALvoid				(ALAPIENTRY * qalBuffer3f)(ALuint buffer, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
extern ALvoid				(ALAPIENTRY * qalBuffer3i)(ALuint buffer, ALenum param, ALint value1, ALint value2, ALint value3);
extern ALvoid				(ALAPIENTRY * qalBufferf)(ALuint buffer, ALenum param, ALfloat value);
extern ALvoid				(ALAPIENTRY * qalBufferfv)(ALuint buffer, ALenum param, const ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalBufferi)(ALuint buffer, ALenum param, ALint value);
extern ALvoid				(ALAPIENTRY * qalBufferiv)(ALuint buffer, ALenum param, const ALint *values);
extern ALvoid				(ALAPIENTRY * qalDeleteBuffers)(ALsizei n, const ALuint *buffers);
extern ALvoid				(ALAPIENTRY * qalDeleteSources)(ALsizei n, const ALuint *sources);
extern ALvoid				(ALAPIENTRY * qalDisable)(ALenum capability);
extern ALvoid				(ALAPIENTRY * qalDistanceModel)(ALenum distanceModel);
extern ALvoid				(ALAPIENTRY * qalDopplerFactor)(ALfloat value);
extern ALvoid				(ALAPIENTRY * qalDopplerVelocity)(ALfloat value);
extern ALvoid				(ALAPIENTRY * qalEnable)(ALenum capability);
extern ALvoid				(ALAPIENTRY * qalGenBuffers)(ALsizei n, ALuint *buffers);
extern ALvoid				(ALAPIENTRY * qalGenSources)(ALsizei n, ALuint *sources);
extern ALboolean			(ALAPIENTRY * qalGetBoolean)(ALenum param);
extern ALvoid				(ALAPIENTRY * qalGetBooleanv)(ALenum param, ALboolean *data);
extern ALvoid				(ALAPIENTRY * qalGetBuffer3f)(ALuint buffer, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
extern ALvoid				(ALAPIENTRY * qalGetBuffer3i)(ALuint buffer, ALenum param, ALint *value1, ALint *value2, ALint *value3);
extern ALvoid				(ALAPIENTRY * qalGetBufferf)(ALuint buffer, ALenum param, ALfloat *value);
extern ALvoid				(ALAPIENTRY * qalGetBufferfv)(ALuint buffer, ALenum param, ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalGetBufferi)(ALuint buffer, ALenum param, ALint *value);
extern ALvoid				(ALAPIENTRY * qalGetBufferiv)(ALuint buffer, ALenum param, ALint *values);
extern ALdouble				(ALAPIENTRY * qalGetDouble)(ALenum param);
extern ALvoid				(ALAPIENTRY * qalGetDoublev)(ALenum param, ALdouble *data);
extern ALenum				(ALAPIENTRY * qalGetEnumValue)(const ALchar *enumName);
extern ALenum				(ALAPIENTRY * qalGetError)(ALvoid);
extern ALfloat				(ALAPIENTRY * qalGetFloat)(ALenum param);
extern ALvoid				(ALAPIENTRY * qalGetFloatv)(ALenum param, ALfloat *data);
extern ALint				(ALAPIENTRY * qalGetInteger)(ALenum param);
extern ALvoid				(ALAPIENTRY * qalGetIntegerv)(ALenum param, ALint *data);
extern ALvoid				(ALAPIENTRY * qalGetListener3f)(ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
extern ALvoid				(ALAPIENTRY * qalGetListener3i)(ALenum param, ALint *value1, ALint *value2, ALint *value3);
extern ALvoid				(ALAPIENTRY * qalGetListenerf)(ALenum param, ALfloat *value);
extern ALvoid				(ALAPIENTRY * qalGetListenerfv)(ALenum param, ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalGetListeneri)(ALenum param, ALint *value);
extern ALvoid				(ALAPIENTRY * qalGetListeneriv)(ALenum param, ALint *values);
extern ALvoid *				(ALAPIENTRY * qalGetProcAddress)(const ALchar *funcName);
extern ALvoid				(ALAPIENTRY * qalGetSource3f)(ALuint source, ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3);
extern ALvoid				(ALAPIENTRY * qalGetSource3i)(ALuint source, ALenum param, ALint *value1, ALint *value2, ALint *value3);
extern ALvoid				(ALAPIENTRY * qalGetSourcef)(ALuint source, ALenum param, ALfloat *value);
extern ALvoid				(ALAPIENTRY * qalGetSourcefv)(ALuint source, ALenum param, ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalGetSourcei)(ALuint source, ALenum param, ALint *value);
extern ALvoid				(ALAPIENTRY * qalGetSourceiv)(ALuint source, ALenum param, ALint *values);
extern const ALchar *		(ALAPIENTRY * qalGetString)(ALenum param);
extern ALboolean			(ALAPIENTRY * qalIsBuffer)(ALuint buffer);
extern ALboolean			(ALAPIENTRY * qalIsEnabled)(ALenum capability);
extern ALboolean			(ALAPIENTRY * qalIsExtensionPresent)(const ALchar *extName);
extern ALboolean			(ALAPIENTRY * qalIsSource)(ALuint source);
extern ALvoid				(ALAPIENTRY * qalListener3f)(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
extern ALvoid				(ALAPIENTRY * qalListener3i)(ALenum param, ALint value1, ALint value2, ALint value3);
extern ALvoid				(ALAPIENTRY * qalListenerf)(ALenum param, ALfloat value);
extern ALvoid				(ALAPIENTRY * qalListenerfv)(ALenum param, const ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalListeneri)(ALenum param, ALint value);
extern ALvoid				(ALAPIENTRY * qalListeneriv)(ALenum param, const ALint *values);
extern ALvoid				(ALAPIENTRY * qalSource3f)(ALuint source, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
extern ALvoid				(ALAPIENTRY * qalSource3i)(ALuint source, ALenum param, ALint value1, ALint value2, ALint value3);
extern ALvoid				(ALAPIENTRY * qalSourcef)(ALuint source, ALenum param, ALfloat value);
extern ALvoid				(ALAPIENTRY * qalSourcefv)(ALuint source, ALenum param, const ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalSourcei)(ALuint source, ALenum param, ALint value);
extern ALvoid				(ALAPIENTRY * qalSourceiv)(ALuint source, ALenum param, const ALint *values);
extern ALvoid				(ALAPIENTRY * qalSourcePause)(ALuint source);
extern ALvoid				(ALAPIENTRY * qalSourcePausev)(ALsizei n, const ALuint *sources);
extern ALvoid				(ALAPIENTRY * qalSourcePlay)(ALuint source);
extern ALvoid				(ALAPIENTRY * qalSourcePlayv)(ALsizei n, const ALuint *sources);
extern ALvoid				(ALAPIENTRY * qalSourceQueueBuffers)(ALuint source, ALsizei n, const ALuint *buffers);
extern ALvoid				(ALAPIENTRY * qalSourceRewind)(ALuint source);
extern ALvoid				(ALAPIENTRY * qalSourceRewindv)(ALsizei n, const ALuint *sources);
extern ALvoid				(ALAPIENTRY * qalSourceStop)(ALuint source);
extern ALvoid				(ALAPIENTRY * qalSourceStopv)(ALsizei n, const ALuint *sources);
extern ALvoid				(ALAPIENTRY * qalSourceUnqueueBuffers)(ALuint source, ALsizei n, ALuint *buffers);
extern ALvoid				(ALAPIENTRY * qalSpeedOfSound)(ALfloat value);

extern ALvoid				(ALAPIENTRY * qalAuxiliaryEffectSlotf)(ALuint slot, ALenum param, ALfloat value);
extern ALvoid				(ALAPIENTRY * qalAuxiliaryEffectSlotfv)(ALuint slot, ALenum param, const ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalAuxiliaryEffectSloti)(ALuint slot, ALenum param, ALint value);
extern ALvoid				(ALAPIENTRY * qalAuxiliaryEffectSlotiv)(ALuint slot, ALenum param, const ALint *values);
extern ALvoid				(ALAPIENTRY * qalDeleteAuxiliaryEffectSlots)(ALsizei n, const ALuint *slots);
extern ALvoid				(ALAPIENTRY * qalDeleteEffects)(ALsizei n, const ALuint *effects);
extern ALvoid				(ALAPIENTRY * qalDeleteFilters)(ALsizei n, const ALuint *filters);
extern ALvoid				(ALAPIENTRY * qalEffectf)(ALuint effect, ALenum param, ALfloat value);
extern ALvoid				(ALAPIENTRY * qalEffectfv)(ALuint effect, ALenum param, const ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalEffecti)(ALuint effect, ALenum param, ALint value);
extern ALvoid				(ALAPIENTRY * qalEffectiv)(ALuint effect, ALenum param, const ALint *values);
extern ALvoid				(ALAPIENTRY * qalFilterf)(ALuint filter, ALenum param, ALfloat value);
extern ALvoid				(ALAPIENTRY * qalFilterfv)(ALuint filter, ALenum param, const ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalFilteri)(ALuint filter, ALenum param, ALint value);
extern ALvoid				(ALAPIENTRY * qalFilteriv)(ALuint filter, ALenum param, const ALint *values);
extern ALvoid				(ALAPIENTRY * qalGenAuxiliaryEffectSlots)(ALsizei n, ALuint *slots);
extern ALvoid				(ALAPIENTRY * qalGenEffects)(ALsizei n, ALuint *effects);
extern ALvoid				(ALAPIENTRY * qalGenFilters)(ALsizei n, ALuint *filters);
extern ALvoid				(ALAPIENTRY * qalGetAuxiliaryEffectSlotf)(ALuint slot, ALenum param, ALfloat *value);
extern ALvoid				(ALAPIENTRY * qalGetAuxiliaryEffectSlotfv)(ALuint slot, ALenum param, ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalGetAuxiliaryEffectSloti)(ALuint slot, ALenum param, ALint *value);
extern ALvoid				(ALAPIENTRY * qalGetAuxiliaryEffectSlotiv)(ALuint slot, ALenum param, ALint *values);
extern ALvoid				(ALAPIENTRY * qalGetEffectf)(ALuint effect, ALenum param, ALfloat *value);
extern ALvoid				(ALAPIENTRY * qalGetEffectfv)(ALuint effect, ALenum param, ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalGetEffecti)(ALuint effect, ALenum param, ALint *value);
extern ALvoid				(ALAPIENTRY * qalGetEffectiv)(ALuint effect, ALenum param, ALint *values);
extern ALvoid				(ALAPIENTRY * qalGetFilterf)(ALuint filter, ALenum param, ALfloat *value);
extern ALvoid				(ALAPIENTRY * qalGetFilterfv)(ALuint filter, ALenum param, ALfloat *values);
extern ALvoid				(ALAPIENTRY * qalGetFilteri)(ALuint filter, ALenum param, ALint *value);
extern ALvoid				(ALAPIENTRY * qalGetFilteriv)(ALuint filter, ALenum param, ALint *values);
extern ALboolean			(ALAPIENTRY * qalIsAuxiliaryEffectSlot)(ALuint slot);
extern ALboolean			(ALAPIENTRY * qalIsEffect)(ALuint effect);
extern ALboolean			(ALAPIENTRY * qalIsFilter)(ALuint filter);

extern ALenum				(ALAPIENTRY * qalEAXGetBufferMode)(ALuint buffer, ALenum *value);
extern ALboolean			(ALAPIENTRY * qalEAXSetBufferMode)(ALsizei n, const ALuint *buffers, ALenum value);

// ============================================================================

void			QAL_EnableLogging (bool enable);
void			QAL_LogPrintf (const char *fmt, ...);

bool			QAL_Init (const char *driver);
void			QAL_Shutdown ();


#endif	// __QAL_H__