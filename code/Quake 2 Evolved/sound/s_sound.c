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
// s_sound.c - Sound loading
//


#include "s_local.h"


#define SOUNDS_HASH_SIZE			(MAX_SOUNDS >> 2)

typedef struct {
	byte *					data;
	byte *					dataPtr;
	byte *					dataEnd;

	int						chunkSize;
} wavFile_t;

static sound_t *			s_soundsHashTable[SOUNDS_HASH_SIZE];
static sound_t *			s_sounds[MAX_SOUNDS];
static int					s_numSounds;


/*
 ==============================================================================

 WAV LOADING

 ==============================================================================
*/


/*
 ==================
 S_FindChunk
 ==================
*/
static bool S_FindChunk (wavFile_t *file, const char *name){

	byte	*lastChunk;

	lastChunk = file->data;

	while (1){
		file->dataPtr = lastChunk;

		if (file->dataPtr >= file->dataEnd){
			// Didn't find the chunk
			file->dataPtr = NULL;
			return false;
		}

		file->chunkSize = file->dataPtr[4] | (file->dataPtr[5] << 8) | (file->dataPtr[6] << 16) | (file->dataPtr[7] << 24);
		if (file->chunkSize < 0){
			// Invalid chunk
			file->dataPtr = NULL;
			return false;
		}

		lastChunk = file->dataPtr + 8 + ALIGN(file->chunkSize, 2);

		if (!Str_CompareChars((const char *)file->dataPtr, name, 4)){
			// Found it
			file->dataPtr += 8;
			return true;
		}
	}
}

/*
 ==================
 S_LoadWAV

 TODO: some mods uses bad sounds with lower bits, should be ignore them and return a default sound?
 ==================
*/
static bool S_LoadWAV (const char *name, short **wave, int *rate, int *samples){

	wavFile_t	file;
	wavFormat_t	header;
	byte		*data;
	short		*in, *out;
	int			i, size;

	// Load the file
	size = FS_ReadFile(name, (void **)&data);
	if (!data)
		return false;

	file.data = data;
	file.dataEnd = data + size;

	// Find the RIFF/WAVE chunk
	if (!S_FindChunk(&file, "RIFF") || Str_CompareChars((const char *)file.dataPtr, "WAVE", 4))
		Com_Error(ERR_DROP, "S_LoadWAV: missing RIFF/WAVE chunk (%s)", name);

	file.data = data + 12;
	file.dataEnd = data + size;

	// Find the format chunk
	if (!S_FindChunk(&file, "fmt "))
		Com_Error(ERR_DROP, "S_LoadWAV: missing format chunk (%s)", name);

	if (file.chunkSize < sizeof(wavFormat_t))
		Com_Error(ERR_DROP, "S_LoadWAV: bad format chunk size (%i) (%s)", file.chunkSize, name);

	// Parse the WAV format
	header.wFormat = file.dataPtr[0] | (file.dataPtr[1] << 8);
	header.wChannels = file.dataPtr[2] | (file.dataPtr[3] << 8);
	header.dwSamplesPerSec = file.dataPtr[4] | (file.dataPtr[5] << 8) | (file.dataPtr[6] << 16) | (file.dataPtr[7] << 24);
	header.dwAvgBytesPerSec = file.dataPtr[8] | (file.dataPtr[9] << 8) | (file.dataPtr[10] << 16) | (file.dataPtr[11] << 24);
	header.wBlockAlign = file.dataPtr[12] | (file.dataPtr[13] << 8);
	header.wBitsPerSample = file.dataPtr[14] | (file.dataPtr[15] << 8);

	if (header.wFormat != WAV_FORMAT_PCM)
		Com_Error(ERR_DROP, "S_LoadWAV: only Microsoft PCM sound format supported (%s)", name);

	if (header.wChannels != 1)
		Com_Error(ERR_DROP, "S_LoadWAV: only mono sounds supported (%s)", name);

//	if (header.wBitsPerSample != 16)
//		Com_Error(ERR_DROP, "S_LoadWAV: only 16 bit sounds supported (%s)", name);

	// Find the data chunk
	if (!S_FindChunk(&file, "data"))
		Com_Error(ERR_DROP, "S_LoadWAV: missing data chunk (%s)", name);

	if (file.chunkSize <= 0)
		Com_Error(ERR_DROP, "S_LoadWAV: bad data chunk size (%i) (%s)", file.chunkSize, name);

	// Read the sound samples
	in = (short *)file.dataPtr;

	*wave = out = (short *)Mem_ClearedAlloc(file.chunkSize, TAG_TEMPORARY);

	*rate = header.dwSamplesPerSec;
	*samples = file.chunkSize >> 1;

	for (i = 0; i < file.chunkSize / 2; i++)
		out[i] = LittleShort(in[i]);

	// Free file data
	FS_FreeFile(data);

	return true;
}


/*
 ==============================================================================

 SOUND REGISTRATION

 ==============================================================================
*/


/*
 ==================
 S_ResampleSound
 ==================
*/
static short *S_ResampleSound (sound_t *sound, const short *wave){

	short	*buffer;
	float	stepScale;
	uint	frac, fracStep;
	int		rate, samples;
	int		offset;
	int		i;

	rate = 11025 << s_soundQuality->integerValue;
	stepScale = (float)sound->rate / rate;
	samples = (int)(sound->samples / stepScale);

	sound->rate = rate;
	sound->samples = samples;
	sound->length = (samples * 1000) / rate;
	sound->size = samples << 1;

	buffer = (short *)Mem_ClearedAlloc(samples * sizeof(short), TAG_TEMPORARY);

	frac = 0;
	fracStep = (uint)(256.0f * stepScale);

	for (i = 0; i < samples; i++){
		offset = (frac >> 8);
		frac += fracStep;

		buffer[i] = wave[offset];
	}

	return buffer;
}

/*
 ==================
 S_BuildAmplitudeTable
 ==================
*/
static void S_BuildAmplitudeTable (sound_t *sound, const short *wave){

	float	amplitude;
	int		samples, offset = 0;
	int		i, j;

	sound->tableSize = (sound->length + 9) / 10;
	sound->tableValues = (float *)Mem_ClearedAlloc(sound->tableSize * sizeof(float), TAG_SOUND);

	for (i = 0; i < sound->tableSize; i++){
		samples = (sound->rate / 100) % sound->samples;

		if (offset + samples > sound->samples)
			samples = sound->samples - offset;

		if (!samples){
			sound->tableValues[i] = 0.0f;
			continue;
		}

		amplitude = 0.0f;

		for (j = 0; j < samples; j++){
			if (wave[offset] < 0)
				amplitude -= wave[offset] * (1.0f / 32768.0f);
			else if (wave[offset] > 0)
				amplitude += wave[offset] * (1.0f / 32767.0f);

			offset++;
		}

		sound->tableValues[i] = amplitude / samples;
	}
}

/*
 ==================
 S_UploadSound
 ==================
*/
static void S_UploadSound (sound_t *sound, short *wave){

	short	*data;

	// Resample the sound if needed
	if (sound->rate == (11025 << s_soundQuality->integerValue))
		data = wave;
	else
		data = S_ResampleSound(sound, wave);

	// Build amplitude table
	S_BuildAmplitudeTable(sound, data);

	// Upload the sound
	qalGenBuffers(1, &sound->bufferId);

	if (alConfig.eaxRAMAvailable)
		qalEAXSetBufferMode(1, &sound->bufferId, AL_STORAGE_AUTOMATIC);

	qalBufferData(sound->bufferId, AL_FORMAT_MONO16, data, sound->size, sound->rate);

	if (data != wave)
		Mem_Free(data);

	// Check for errors
	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();
}

/*
 ==================
 S_LoadSound
 ==================
*/
static sound_t *S_LoadSound (const char *name, short *wave, int rate, int samples, int flags){

	sound_t	*sound;
	uint	hashKey;

	if (s_numSounds == MAX_SOUNDS)
		Com_Error(ERR_DROP, "S_LoadSound: MAX_SOUNDS hit");

	s_sounds[s_numSounds++] = sound = (sound_t *)Mem_ClearedAlloc(sizeof(sound_t), TAG_SOUND);

	// Fill it in
	Str_Copy(sound->name, name, sizeof(sound->name));
	sound->flags = flags;
	sound->rate = rate;
	sound->samples = samples;
	sound->length = (samples * 1000) / rate;
	sound->size = samples << 1;

	S_UploadSound(sound, wave);

	// Add to hash table
	hashKey = Str_HashKey(sound->name, SOUNDS_HASH_SIZE, false);

	sound->nextHash = s_soundsHashTable[hashKey];
	s_soundsHashTable[hashKey] = sound;

	return sound;
}

/*
 ==================
 S_FindSound
 ==================
*/
sound_t *S_FindSound (const char *name, int flags){

	sound_t	*sound;
	char	loadName[MAX_QPATH];
	short	*wave;
	int		rate, samples;
	uint	hashKey;

	// Check if already loaded
	hashKey = Str_HashKey(name, SOUNDS_HASH_SIZE, false);

	for (sound = s_soundsHashTable[hashKey]; sound; sound = sound->nextHash){
		if (!Str_ICompare(sound->name, name)){
			if (sound->flags & SF_INTERNAL)
				return sound;

			if (sound->flags != flags)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused sound '%s' with mixed flags parameter\n", name);

			return sound;
		}
	}

	// Correct the path
	if (name[0] == '#')
		Str_SPrintf(loadName, sizeof(loadName), "%s", &name[1]);
	else
		Str_SPrintf(loadName, sizeof(loadName), "sound/%s", name);

	// Load it from disk
	if (!S_LoadWAV(loadName, &wave, &rate, &samples))
		return NULL;

	// Load the sound
	sound = S_LoadSound(loadName, wave, rate, samples, flags);

	Mem_Free(wave);

	return sound;
}

/*
 ==================
 S_RegisterSound
 ==================
*/
sound_t *S_RegisterSound (const char *name, int flags){

	return S_FindSound(name, flags);
}

/*
 ==================
 S_FindSexedSound

 This is used for loading male or female sounds
 ==================
*/
sound_t *S_FindSexedSound (const char *name, entity_state_t *entity, int flags){

	sound_t			*sound;
	clientInfo_t	*clientInfo;
	char			model[MAX_QPATH], sexedName[MAX_QPATH];
	char			*ch;

	// Determine what model the client is using
	clientInfo = &cl.clientInfo[entity->number - 1];
	if (!clientInfo->valid)
		clientInfo = &cl.baseClientInfo;

	Str_Copy(model, clientInfo->info, sizeof(model));
	ch = Str_FindChar(model, '/');
	if (!ch)
		ch = Str_FindChar(model, '\\');
	if (ch)
		*ch = 0;

	// See if the model specific sound exists
	Str_SPrintf(sexedName, sizeof(sexedName), "#players/%s/%s", model, name+1);
	if (FS_FileExists(&sexedName[1])){
		// Yes, register it
		sound = S_FindSound(sexedName, 0);
	}
	else {
		// No, revert to the male sound
		Str_SPrintf(sexedName, sizeof(sexedName), "#players/male/%s", name+1);

		sound = S_FindSound(sexedName, 0);
	}

	return sound;
}

/*
 ==================
 S_RegisterSexedSound
 ==================
*/
sound_t *S_RegisterSexedSound (const char *name, entity_state_t *entity, int flags){

	return S_FindSexedSound(name, entity, flags);
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 S_ListSounds_f
 ==================
*/
static void S_ListSounds_f (){

	sound_t	*sound;
	int		bytes = 0;
	int		i;

	Com_Printf("\n");
	Com_Printf("      -hz-- -samples -size- length -name-----------\n");

	for (i = 0; i < s_numSounds; i++){
		sound = s_sounds[i];

		bytes += sound->size;

		Com_Printf("%4i: ", i);

		Com_Printf("%5i %8i ", sound->rate, sound->samples);

		Com_Printf("%5ik ", sound->size >> 10);

		Com_Printf("%5.2fs ", MS2SEC(sound->length));

		Com_Printf("%s\n", sound->name);
	}

	Com_Printf("---------------------------------------------------\n");
	Com_Printf("%i total sounds\n", s_numSounds);
	Com_Printf("%.2f MB of sound data\n", bytes * (1.0f / 1048576.0f));
	Com_Printf("\n");
}


/*
 ==============================================================================

 INTERNAL SOUNDS

 ==============================================================================
*/


/*
 ==================
 S_CreateInternalSounds
 ==================
*/
static void S_CreateInternalSounds (){

	short	wave[22050];
	float	amplitude;
	int		i;

	// Default sound
	if (!s_playDefaultSound->integerValue)
		Mem_Fill(wave, 0, sizeof(wave));
	else {
		for (i = 0; i < 22050; i++){
			amplitude = sin((float)i * 0.1f);

			wave[i] = (short)(amplitude * 20000.0f);
		}
	}

	snd.defaultSound = S_LoadSound("_default", wave, 44100, 22050, SF_INTERNAL);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 S_InitSounds
 ==================
*/
void S_InitSounds (){

	// Add commands
	Cmd_AddCommand("listSounds", S_ListSounds_f, "Lists loaded sounds", NULL);

	// Create internal sounds
	S_CreateInternalSounds();
}

/*
 ==================
 S_ShutdownSounds
 ==================
*/
void S_ShutdownSounds (){

	sound_t	*sound;
	int		i;

	// Remove commands
	Cmd_RemoveCommand("listSounds");

	// Delete all the sounds
	for (i = 0; i < s_numSounds; i++){
		sound = s_sounds[i];

		qalDeleteBuffers(1, &sound->bufferId);
	}

	// Clear sound list
	Mem_Fill(s_soundsHashTable, 0, sizeof(s_soundsHashTable));
	Mem_Fill(s_sounds, 0, sizeof(s_sounds));

	s_numSounds = 0;
}