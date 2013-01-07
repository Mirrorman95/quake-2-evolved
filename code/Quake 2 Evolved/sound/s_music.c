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
// s_music.c - Background music streaming
//


#include "s_local.h"


/*
 ==============================================================================

 OGG LOADING

 ==============================================================================
*/


/*
 ==================
 S_ReadOGG
 ==================
*/
static size_t S_ReadOGG (void *buffer, size_t size, size_t count, void *data){

	int		bytes;

	bytes = size * count;

	if (snd.music.oggOffset + bytes > snd.music.oggSize)
		bytes = snd.music.oggSize - snd.music.oggOffset;

	bytes = FS_Read(snd.music.oggFile, buffer, bytes);
	snd.music.oggOffset += bytes;

	return bytes / size;
}

/*
 ==================
 S_TellOGG
 ==================
*/
static long S_TellOGG (void *data){

	return FS_Tell(snd.music.oggFile);
}

/*
 ==================
 S_SeekOGG
 ==================
*/
static int S_SeekOGG (void *data, ogg_int64_t offset, int origin){

	switch (origin){
	case SEEK_SET:
		FS_Seek(snd.music.oggFile, (int)offset, FS_SEEK_SET);
		break;
	case SEEK_CUR:
		FS_Seek(snd.music.oggFile, (int)offset, FS_SEEK_CUR);
		break;
	case SEEK_END:
		FS_Seek(snd.music.oggFile, (int)offset, FS_SEEK_END);
		break;
	default:
		return -1;
	}

	snd.music.oggOffset = FS_Tell(snd.music.oggFile);

	return 0;
}

/*
 ==================
 S_CloseOGG
 ==================
*/
static int S_CloseOGG (void *data){

	return 0;
}


/*
 ==============================================================================

 MUSIC TRACK

 ==============================================================================
*/


/*
 ==================
 S_QueueMusicTrack
 ==================
*/
static void S_QueueMusicTrack (const char *introTrack, const char *loopTrack, int fadeUpTime){

	musicQueue_t	*queue, *last;

	queue = (musicQueue_t *)Mem_Alloc(sizeof(musicQueue_t), TAG_SOUND);

	Str_Copy(queue->introTrack, introTrack, sizeof(queue->introTrack));
	Str_Copy(queue->loopTrack, loopTrack, sizeof(queue->loopTrack));
	queue->fadeUpTime = max(fadeUpTime, 0);
	queue->next = NULL;

	if (!snd.music.queue){
		snd.music.queue = queue;
		return;
	}

	last = snd.music.queue;
	while (last->next)
		last = last->next;

	last->next = queue;
}

/*
 ==================
 S_UnqueueMusicTrack
 ==================
*/
static bool S_UnqueueMusicTrack (){

	musicQueue_t	*queue;

	if (!snd.music.queue)
		return false;

	queue = snd.music.queue;
	snd.music.queue = queue->next;

	Str_Copy(snd.music.introTrack, queue->introTrack, sizeof(snd.music.introTrack));
	Str_Copy(snd.music.loopTrack, queue->loopTrack, sizeof(snd.music.loopTrack));

	snd.music.time = 0;
	snd.music.fadeStartTime = 0;
	snd.music.fadeEndTime = queue->fadeUpTime;

	snd.music.volume = 0.0f;
	snd.music.fromVolume = 0.0f;
	snd.music.toVolume = 1.0f;

	Mem_Free(queue);

	return true;
}

/*
 ==================
 S_OpenMusicTrack
 ==================
*/
static bool S_OpenMusicTrack (const char *name){

	ov_callbacks	callbacks = {S_ReadOGG, S_SeekOGG, S_CloseOGG, S_TellOGG};
	vorbis_info		*info;

	// Open the file
	snd.music.oggSize = FS_OpenFile(name, FS_READ, &snd.music.oggFile);
	if (!snd.music.oggFile){
		Com_Printf("Music file %s not found\n", name);
		return false;
	}

	snd.music.oggOffset = 0;

	// Allocate and open the Ogg Vorbis bitstream
	snd.music.oggVorbisBitstream = (OggVorbis_File *)Mem_Alloc(sizeof(OggVorbis_File), TAG_SOUND);

	if (ov_open_callbacks(&snd.music, snd.music.oggVorbisBitstream, NULL, 0, callbacks) < 0){
		Com_DPrintf(S_COLOR_RED "Couldn't open Ogg Vorbis bitstream '%s'\n", name);
		return false;
	}

	// Parse the Ogg Vorbis header
	info = ov_info(snd.music.oggVorbisBitstream, -1);

	if (info->channels != 2 || info->rate != MUSIC_RATE){
		Com_DPrintf(S_COLOR_RED "Ogg Vorbis bitstream '%s' is not %i KHz stereo\n", name, MUSIC_RATE / 1000);
		return false;
	}

	snd.music.oggSkip = (int)ov_raw_tell(snd.music.oggVorbisBitstream);

	return true;
}

/*
 ==================
 S_CloseMusicTrack
 ==================
*/
static void S_CloseMusicTrack (){

	// Clear and free the Ogg Vorbis bitstream
	if (snd.music.oggVorbisBitstream){
		ov_clear(snd.music.oggVorbisBitstream);

		Mem_Free(snd.music.oggVorbisBitstream);
		snd.music.oggVorbisBitstream = NULL;
	}

	// Close the file
	if (snd.music.oggFile){
		FS_CloseFile(snd.music.oggFile);
		snd.music.oggFile = 0;
	}
}

/*
 ==================
 S_StreamMusicTrack
 ==================
*/
static bool S_StreamMusicTrack (){

#if defined SIMD_X86
	__m128		xmmScale;
	__m128		xmmSamples[2];
	__m128i		xmmSamplesPCM[2];
#else
	float		scale;
#endif
	vorbis_info	*info;
	float		**data, samples[MUSIC_BUFFER_SAMPLES][2];
	float		frac;
	int			offset, remaining;
	int			count, link;
	int			i;

	// Stream from disk
	offset = 0;
	remaining = MUSIC_BUFFER_SAMPLES;

	while (remaining){
		count = ov_read_float(snd.music.oggVorbisBitstream, &data, remaining, &link);

		if (count == 0){
			// End of track
			if (snd.music.state == MS_PLAYING){
				snd.music.state = MS_LOOPING;

				// Close the intro track
				S_CloseMusicTrack();

				// Open the loop track
				if (!S_OpenMusicTrack(snd.music.loopTrack))
					return false;
			}

			// Restart the loop track
			ov_raw_seek(snd.music.oggVorbisBitstream, (ogg_int64_t)snd.music.oggSkip);

			// Try streaming again
			count = ov_read_float(snd.music.oggVorbisBitstream, &data, remaining, &link);
		}

		if (count <= 0){
			Com_DPrintf(S_COLOR_RED "Failed to read from Ogg Vorbis bitstream '%s'\n", (snd.music.state == MS_PLAYING) ? snd.music.introTrack : snd.music.loopTrack);
			return false;
		}

		// Make sure the bitstream is valid
		info = ov_info(snd.music.oggVorbisBitstream, link);

		if (info->channels != 2 || info->rate != MUSIC_RATE){
			Com_DPrintf(S_COLOR_RED "Ogg Vorbis bitstream '%s' is not %i KHz stereo\n", (snd.music.state == MS_PLAYING) ? snd.music.introTrack : snd.music.loopTrack, MUSIC_RATE / 1000);
			return false;
		}

		// Interleave the samples
		for (i = 0; i < count; i++){
			samples[offset + i][0] = data[0][i];
			samples[offset + i][1] = data[1][i];
		}

		offset += count;
		remaining -= count;
	}

	// Compute volume
	if (snd.music.fadeStartTime == snd.music.fadeEndTime)
		snd.music.volume = snd.music.toVolume;
	else {
		frac = (float)(snd.music.time - snd.music.fadeStartTime) / (snd.music.fadeEndTime - snd.music.fadeStartTime);

		snd.music.volume = Lerp(snd.music.fromVolume, snd.music.toVolume, frac);
	}

#if defined SIMD_X86

	// Scale music amplitude and convert the samples
	xmmScale = _mm_set1_ps(32768.0f * snd.music.volume);

	for (i = 0; i < MUSIC_BUFFER_SAMPLES; i += 4){
		xmmSamples[0] = _mm_loadu_ps((const float *)(samples[i + 0]));
		xmmSamples[1] = _mm_loadu_ps((const float *)(samples[i + 2]));

		xmmSamples[0] = _mm_mul_ps(xmmSamples[0], xmmScale);
		xmmSamples[1] = _mm_mul_ps(xmmSamples[1], xmmScale);

		xmmSamplesPCM[0] = _mm_cvttps_epi32(xmmSamples[0]);
		xmmSamplesPCM[1] = _mm_cvttps_epi32(xmmSamples[1]);

		_mm_storeu_si128((__m128i *)(snd.music.samples[i]), _mm_packs_epi32(xmmSamplesPCM[0], xmmSamplesPCM[1]));
	}

#else

	// Scale music amplitude and convert the samples
	scale = 32768.0f * snd.music.volume;

	for (i = 0; i < MUSIC_BUFFER_SAMPLES; i++){
		snd.music.samples[i][0] = FloatToShort(samples[i][0] * scale);
		snd.music.samples[i][1] = FloatToShort(samples[i][1] * scale);
	}

#endif

	// Advance timer
	snd.music.time += MUSIC_FRAMEMSEC;

	// If not completely faded out, continue playing the current track
	if (snd.music.volume || snd.music.toVolume)
		return true;

	// Play another from the queue if possible
	if (S_UnqueueMusicTrack()){
		snd.music.state = MS_PLAYING;

		// Close the current track
		S_CloseMusicTrack();

		// Open the intro track
		if (!S_OpenMusicTrack(snd.music.introTrack))
			return false;

		return true;
	}

	// Done
	snd.music.state = MS_WAITING;

	// Close the current track
	S_CloseMusicTrack();

	return true;
}


// ============================================================================


/*
 ==================
 S_PlayMusic
 ==================
*/
void S_PlayMusic (const char *introTrack, const char *loopTrack, int fadeUpTime){

	if (!snd.musicSource)
		return;

	// If already playing, queue it for later
	if (snd.music.state == MS_PLAYING || snd.music.state == MS_LOOPING){
		S_QueueMusicTrack(introTrack, loopTrack, fadeUpTime);
		return;
	}

	// Play the music tracks
	snd.music.state = MS_PLAYING;

	Str_Copy(snd.music.introTrack, introTrack, sizeof(snd.music.introTrack));
	Str_Copy(snd.music.loopTrack, loopTrack, sizeof(snd.music.loopTrack));

	snd.music.time = 0;
	snd.music.fadeStartTime = 0;
	snd.music.fadeEndTime = max(fadeUpTime, 0);

	snd.music.volume = 0.0f;
	snd.music.fromVolume = 0.0f;
	snd.music.toVolume = 1.0f;

	// Open the intro track
	if (!S_OpenMusicTrack(snd.music.introTrack)){
		S_StopMusic();
		return;
	}
}

/*
 ==================
 S_FadeMusic
 ==================
*/
void S_FadeMusic (int fadeDownTime){

	if (!snd.musicSource)
		return;

	if (snd.music.state == MS_STOPPED)
		return;			// Not playing

	snd.music.fadeStartTime = snd.music.time;
	snd.music.fadeEndTime = snd.music.time + max(fadeDownTime, 0);

	snd.music.fromVolume = snd.music.volume;
	snd.music.toVolume = 0.0f;
}

/*
 ==================
 S_StopMusic
 ==================
*/
void S_StopMusic (){

	musicQueue_t	*queue, *next;
	uint			buffer;
	int				processed;

	if (!snd.musicSource)
		return;

	if (snd.music.state == MS_STOPPED)
		return;			// Not playing

	// Stop the source
	qalSourceStop(snd.musicSource);

	if (s_showStreaming->integerValue)
		Com_Printf("music: stop\n");

	// Unqueue and delete all processed buffers
	qalGetSourcei(snd.musicSource, AL_BUFFERS_PROCESSED, &processed);

	while (processed--){
		qalSourceUnqueueBuffers(snd.musicSource, 1, &buffer);
		qalDeleteBuffers(1, &buffer);

		if (s_showStreaming->integerValue)
			Com_Printf("music: unqueue %u\n", buffer);
	}

	// Reset the buffer
	qalSourcei(snd.musicSource, AL_BUFFER, 0);

	// Rewind the source
	qalSourceRewind(snd.musicSource);

	if (s_showStreaming->integerValue)
		Com_Printf("music: rewind\n");

	// Clear the music queue
	for (queue = snd.music.queue; queue; queue = next){
		next = queue->next;

		Mem_Free(queue);
	}

	// Close the music track
	S_CloseMusicTrack();

	Mem_Fill(&snd.music, 0, sizeof(music_t));
}

/*
 ==================
 S_UpdateMusic
 ==================
*/
void S_UpdateMusic (){

	uint	buffer;
	int		processed, queued;
	int		state;

	if (!snd.musicSource)
		return;

	if (snd.music.state == MS_STOPPED)
		return;			// Not playing

	// Development tool
	if (s_skipStreaming->integerValue)
		return;

	QAL_LogPrintf("----- S_UpdateMusic -----\n");

	// Unqueue and delete all processed buffers
	qalGetSourcei(snd.musicSource, AL_BUFFERS_PROCESSED, &processed);

	while (processed--){
		qalSourceUnqueueBuffers(snd.musicSource, 1, &buffer);
		qalDeleteBuffers(1, &buffer);

		if (s_showStreaming->integerValue)
			Com_Printf("music: unqueue %u\n", buffer);
	}

	// Make sure we always have a few buffers in the queue
	qalGetSourcei(snd.musicSource, AL_BUFFERS_QUEUED, &queued);

	while (queued < MUSIC_BUFFERS){
		if (snd.music.state == MS_WAITING)
			break;			// Done

		// Stream some samples
		if (!S_StreamMusicTrack()){
			S_StopMusic();

			// Check for errors
			if (!s_ignoreALErrors->integerValue)
				S_CheckForErrors();

			QAL_LogPrintf("--------------------\n");

			return;
		}

		// Upload the samples to a new buffer
		qalGenBuffers(1, &buffer);

		if (alConfig.eaxRAMAvailable)
			qalEAXSetBufferMode(1, &buffer, AL_STORAGE_ACCESSIBLE);

		qalBufferData(buffer, AL_FORMAT_STEREO16, snd.music.samples, MUSIC_BUFFER_SAMPLES << 2, MUSIC_RATE);

		// Queue the buffer
		qalSourceQueueBuffers(snd.musicSource, 1, &buffer);

		if (s_showStreaming->integerValue)
			Com_Printf("music: queue %u (%i samples)\n", buffer, MUSIC_BUFFER_SAMPLES);

		queued++;
	}

	// If the queue is empty, we're done
	if (!queued){
		S_StopMusic();

		// Check for errors
		if (!s_ignoreALErrors->integerValue)
			S_CheckForErrors();

		QAL_LogPrintf("--------------------\n");

		return;
	}

	// Update volume
	qalSourcef(snd.musicSource, AL_GAIN, s_musicVolume->floatValue);

	// Make sure the source is playing
	qalGetSourcei(snd.musicSource, AL_SOURCE_STATE, &state);

	if (state != AL_PLAYING){
		qalSourcePlay(snd.musicSource);

		if (s_showStreaming->integerValue)
			Com_Printf("music: play\n");
	}

	// Check for errors
	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();

	QAL_LogPrintf("--------------------\n");
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 S_PlayMusic_f
 ==================
*/
static void S_PlayMusic_f (){

	char	name[MAX_PATH_LENGTH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: playMusic <name>\n");
		return;
	}

	Str_Copy(name, Cmd_Argv(1), sizeof(name));
	Str_DefaultFilePath(name, sizeof(name), "music");
	Str_DefaultFileExtension(name, sizeof(name), ".ogg");

	// If already playing music, stop it
	S_StopMusic();

	// Play the music
	S_PlayMusic(name, name, 0);
}

/*
 ==================
 S_StopMusic_f
 ==================
*/
static void S_StopMusic_f (){

	S_StopMusic();
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 S_InitMusic
 ==================
*/
void S_InitMusic (){

	// Add commands
	Cmd_AddCommand("playMusic", S_PlayMusic_f, "Plays music", Cmd_ArgCompletion_MusicName);
	Cmd_AddCommand("stopMusic", S_StopMusic_f, "Stops playing music", NULL);

	// Allocate the streaming source
	qalGenSources(1, &snd.musicSource);

	if (qalGetError() != AL_NO_ERROR)
		Com_Error(ERR_FATAL, "Couldn't allocate streaming music source");

	// Set it up
	qalSourcei(snd.musicSource, AL_SOURCE_RELATIVE, AL_TRUE);
}

/*
 ==================
 S_ShutdownMusic
 ==================
*/
void S_ShutdownMusic (){

	if (!snd.musicSource)
		return;

	// Remove commands
	Cmd_RemoveCommand("playMusic");
	Cmd_RemoveCommand("stopMusic");

	// If playing music, stop it
	S_StopMusic();

	// Free the streaming source
	qalDeleteSources(1, &snd.musicSource);
}