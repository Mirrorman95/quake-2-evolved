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
// s_rawSamples.c - Raw samples streaming
//


#include "s_local.h"


/*
 ==================
 S_RawSamples
 ==================
*/
void S_RawSamples (const short *data, int samples, int rate, bool stereo, float volume){

	uint	buffer;
	int		state;

	if (!snd.rawSamplesSource)
		return;

	// Development tool
	if (s_skipStreaming->integerValue)
		return;

	QAL_LogPrintf("----- S_RawSamples -----\n");

	// Upload the samples to a new buffer
	qalGenBuffers(1, &buffer);

	if (alConfig.eaxRAMAvailable)
		qalEAXSetBufferMode(1, &buffer, AL_STORAGE_ACCESSIBLE);

	if (stereo)
		qalBufferData(buffer, AL_FORMAT_STEREO16, data, samples << 2, rate);
	else
		qalBufferData(buffer, AL_FORMAT_MONO16, data, samples << 1, rate);

	// Queue the buffer
	qalSourceQueueBuffers(snd.rawSamplesSource, 1, &buffer);

	if (s_showStreaming->integerValue)
		Com_Printf("raw samples: queue %u (%i samples)\n", buffer, samples);

	// Update volume
	qalSourcef(snd.rawSamplesSource, AL_GAIN, Clamp(volume, 0.0f, 1.0f));

	// Make sure the source is playing
	qalGetSourcei(snd.rawSamplesSource, AL_SOURCE_STATE, &state);

	if (state != AL_PLAYING){
		qalSourcePlay(snd.rawSamplesSource);

		if (s_showStreaming->integerValue)
			Com_Printf("raw samples: play\n");
	}

	// Check for errors
	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();

	QAL_LogPrintf("--------------------\n");
}

/*
 ==================
 S_FlushRawSamples
 ==================
*/
void S_FlushRawSamples (bool forceStop){

	uint	buffer;
	int		processed;
	int		state;

	if (!snd.rawSamplesSource)
		return;

	QAL_LogPrintf("----- S_FlushRawSamples -----\n");

	// Stop the source if desired
	if (forceStop){
		qalSourceStop(snd.rawSamplesSource);

		if (s_showStreaming->integerValue)
			Com_Printf("raw samples: stop\n");

		// Unqueue and delete all processed buffers
		qalGetSourcei(snd.rawSamplesSource, AL_BUFFERS_PROCESSED, &processed);

		while (processed--){
			qalSourceUnqueueBuffers(snd.rawSamplesSource, 1, &buffer);
			qalDeleteBuffers(1, &buffer);

			if (s_showStreaming->integerValue)
				Com_Printf("raw samples: unqueue %u\n", buffer);
		}

		// Reset the buffer
		qalSourcei(snd.rawSamplesSource, AL_BUFFER, 0);

		// Rewind the source
		qalSourceRewind(snd.rawSamplesSource);

		if (s_showStreaming->integerValue)
			Com_Printf("raw samples: rewind\n");

		// Check for errors
		if (!s_ignoreALErrors->integerValue)
			S_CheckForErrors();

		QAL_LogPrintf("--------------------\n");

		return;
	}

	// Get the source state
	qalGetSourcei(snd.rawSamplesSource, AL_SOURCE_STATE, &state);

	// Unqueue and delete all processed buffers
	qalGetSourcei(snd.rawSamplesSource, AL_BUFFERS_PROCESSED, &processed);

	while (processed--){
		qalSourceUnqueueBuffers(snd.rawSamplesSource, 1, &buffer);
		qalDeleteBuffers(1, &buffer);

		if (s_showStreaming->integerValue)
			Com_Printf("raw samples: unqueue %u\n", buffer);
	}

	// If the source is done playing
	if (state == AL_STOPPED){
		// Reset the buffer
		qalSourcei(snd.rawSamplesSource, AL_BUFFER, 0);

		// Rewind the source
		qalSourceRewind(snd.rawSamplesSource);

		if (s_showStreaming->integerValue)
			Com_Printf("raw samples: rewind\n");
	}

	// Check for errors
	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();

	QAL_LogPrintf("--------------------\n");
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 S_InitRawSamples
 ==================
*/
void S_InitRawSamples (){

	// Allocate the streaming source
	qalGenSources(1, &snd.rawSamplesSource);

	if (qalGetError() != AL_NO_ERROR)
		Com_Error(ERR_FATAL, "Couldn't allocate streaming raw samples source");

	// Set it up
	qalSourcei(snd.rawSamplesSource, AL_SOURCE_RELATIVE, AL_TRUE);
}

/*
 ==================
 S_ShutdownRawSamples
 ==================
*/
void S_ShutdownRawSamples (){

	if (!snd.rawSamplesSource)
		return;

	// Flush raw samples
	S_FlushRawSamples(true);

	// Free the streaming source
	qalDeleteSources(1, &snd.rawSamplesSource);
}