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
// s_channel.c - Sound channels
//


#include "s_local.h"


/*
 ==================
 
 Computes all the spatialization parameters for the given channel
 ==================
*/
static void S_SpatializeChannel (channel_t *channel){

}


// ============================================================================


/*
 ==================
 S_PickChannel
 ==================
*/
channel_t *S_PickChannel (int emitter, int channelId){

	channel_t	*channel;
	uint		buffer;
	int			processed;
	int			index = -1;
	int			oldestTime = cl.time;
	int			i;

	if (emitter < 0 || channelId < 0)
		Com_Error(ERR_DROP, "S_PickChannel: emitter < 0 || channelId < 0");

	// Try to override an active channel
	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		// Don't let game sounds override streaming sounds
		if (channel->streaming)
			continue;

		// Check if this channel is active
		if (channel->state == CS_FREE){
			// Free  the channel
			index = i;
			break;
		}

		// Always override sounds from the same emitter on the same channel
		if (channelId != 0 && (channel->emitter == emitter && channel->channelId == channelId)){
			index = i;
			break;
		}

		// Don't let monster sounds override player sounds
		if (emitter != cl.clientNum && channel->emitter == cl.clientNum)
			continue;

		// Replace the oldest sound
		if (channel->allocTime < oldestTime){
			oldestTime = channel->allocTime;
			index = i;
		}
	}

	// Do we have a channel?
	if (index == -1){
		Com_DPrintf(S_COLOR_YELLOW "Dropped sound '%s'\n", channel->sound->name);

		return NULL;
	}

	channel = &snd.channels[index];

	// If overriding a sound
	if (channel->state != CS_FREE){
		Com_DPrintf(S_COLOR_YELLOW "Overridden sound '%s'\n", channel->sound->name);

		// Stop the source
		qalSourceStop(channel->sourceId);

		// Unqueue all processed buffers if needed
		if (channel->streaming){
			qalGetSourcei(channel->sourceId, AL_BUFFERS_PROCESSED, &processed);

			while (processed--)
				qalSourceUnqueueBuffers(channel->sourceId, 1, &buffer);
		}

		// Reset the buffer
		qalSourcei(channel->sourceId, AL_BUFFER, 0);

		// Rewind the source
		qalSourceRewind(channel->sourceId);
	}

	// Set up the channel
	channel->state = CS_FREE;

	channel->streaming = false;

	channel->amplitude = 0.0f;

	channel->channelId = channelId;
	channel->allocTime = snd.time;

	channel->emitter = emitter;

	return channel;
}

/*
 ==================
 S_PlayChannel

 TODO: streaming sounds
 ==================
*/
void S_PlayChannel (channel_t *channel, sound_t *sound){

	// TODO: check if the channel is already active?

	QAL_LogPrintf("----- S_PlayChannel ( %s ) -----\n", sound->name);

	// Get the sound
	channel->sound = sound;

	// Set sound
	if (channel->state == CS_NORMAL || channel->state == CS_LOOPED){
		// Disable streaming
		channel->streaming = false;

		// Set the buffer
		qalSourcei(channel->sourceId, AL_BUFFER, sound->bufferId);
	}
	else {
		// Enable streaming
		channel->streaming = true;

		// Queue the buffers
	}

	// Set looping
	if (channel->state == CS_LOOPED)
		qalSourcei(channel->sourceId, AL_LOOPING, AL_TRUE);
	else
		qalSourcei(channel->sourceId, AL_LOOPING, AL_FALSE);

	qalSourcei(channel->sourceId, AL_SOURCE_RELATIVE, AL_FALSE);

	// Check for errors
	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();

	QAL_LogPrintf("--------------------\n");
}

/*
 ==================
 S_StopChannel
 ==================
*/
void S_StopChannel (channel_t *channel){

	sound_t *sound = channel->sound;
	uint	buffer;
	int		processed;

	// TODO: check if the channel is active?

	QAL_LogPrintf("----- S_StopChannel ( %s ) -----\n", sound->name);

	// Stop the source
	qalSourceStop(channel->sourceId);

	// Unqueue all processed buffers if needed
	if (channel->streaming){
		qalGetSourcei(channel->sourceId, AL_BUFFERS_PROCESSED, &processed);

		while (processed--)
			qalSourceUnqueueBuffers(channel->sourceId, 1, &buffer);
	}

	// Reset the buffer
	qalSourcei(channel->sourceId, AL_BUFFER, 0);

	// Rewind the source
	qalSourceRewind(channel->sourceId);

	// Free the channel
	channel->state = CS_FREE;

	// Remove the sound
	channel->sound = NULL;

	// Check for errors
	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();

	QAL_LogPrintf("--------------------\n");
}

/*
 ==================
 S_UpdateChannel

 TODO: should we stop looped sounds too?
 TODO: queue more buffers if needed
 TODO: should we stop for "local" sounds?, because local sounds do
 not use spatializeation
 TODO: source parameters
 ==================
*/
void S_UpdateChannel (channel_t *channel){

	sound_t *sound = channel->sound;
	uint	buffer;
	int		processed;
	int		state;

	// TODO: check if the channel is active?

	QAL_LogPrintf("----- S_UpdateChannel ( %s ) -----\n", sound->name);

	// Get the source state
	qalGetSourcei(channel->sourceId, AL_SOURCE_STATE, &state);

	// Unqueue all processed buffers if needed
	if (channel->streaming){
		qalGetSourcei(channel->sourceId, AL_BUFFERS_PROCESSED, &processed);

		while (processed--)
			qalSourceUnqueueBuffers(channel->sourceId, 1, &buffer);
	}

	// If the source is done playing
	if (state == AL_STOPPED){
		if (channel->state == CS_NORMAL){
			// Reset the buffer
			qalSourcei(channel->sourceId, AL_BUFFER, 0);

			// Rewind the source
			qalSourceRewind(channel->sourceId);

			// Free the channel
			channel->state = CS_FREE;

			// Check for errors
			if (!s_ignoreALErrors->integerValue)
				S_CheckForErrors();

			QAL_LogPrintf("--------------------\n");

			return;
		}
	}

	// Queue more buffers if needed

	// Compute the spatialization parameters
	S_SpatializeChannel(channel);

	// Update source parameters

	// Play the source if needed
	if (state != AL_PLAYING)
		qalSourcePlay(channel->sourceId);

	// Check for errors
	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();

	QAL_LogPrintf("--------------------\n");
}


/*
 ==============================================================================

 LOOPING SOUNDS

 ==============================================================================
*/


/*
 ==================
 S_AddLoopingSounds
 ==================
*/
void S_AddLoopingSounds (){

	entity_state_t	*entity;
	int				i;

	snd.numSoundEntities = 0;

	for (i = 0; i < cl.frame.numEntities; i++){
		entity = &cl.parseEntities[(cl.frame.parseEntitiesIndex+i) & (MAX_PARSE_ENTITIES-1)];
		if (!entity->sound)
			continue;

		snd.soundEntities[snd.numSoundEntities++] = entity;
	}
}

/*
 ==================
 S_UpdateLoopingSounds

 Take all the sound entities and begin playing them, or update them if
 already playing
 ==================
*/
void S_UpdateLoopingSounds (){

	entity_state_t	*entity;
	sound_t			*sound;
	channel_t		*channel;
	int				i, j;

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || com_paused->integerValue)
		return;

	for (i = 0; i < snd.numSoundEntities; i++){
		entity = snd.soundEntities[i];

		sound = cl.media.gameSounds[entity->sound];
		if (!sound)
			continue;	// Bad sound effect

		// If this entity is already playing the same sound effect on an
		// active channel, then simply update it
		for (j = 0, channel = snd.channels; j < snd.numChannels; j++, channel++){
			if (channel->sound != sound)
				continue;

			if (channel->state != CS_LOOPED)
				continue;
			if (channel->loopNum != entity->number)
				continue;
			if (channel->loopFrame + 1 != snd.frameCount)
				continue;

			channel->loopFrame = snd.frameCount;
			break;
		}

		if (j != snd.numChannels)
			continue;

		// Otherwise pick a channel and start the sound effect
		channel = S_PickChannel(0, 0);
		if (!channel){
			if (sound->name[0] == '#')
				Com_DPrintf(S_COLOR_RED "Dropped sound %s\n", &sound->name[1]);
			else
				Com_DPrintf(S_COLOR_RED "Dropped sound sound/%s\n", sound->name);

			continue;
		}

		// Set up the channel
		channel->state = CS_LOOPED;

		channel->loopNum = entity->number;
		channel->loopFrame = snd.frameCount;

		channel->fixedPosition = false;

		channel->volume = 1.0f;

		channel->distanceMult = 1.0f / ATTN_STATIC;

		// Play it
		S_PlayChannel(channel, sound);
	}
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 S_ListSoundChannels_f
 ==================
*/
static void S_ListSoundChannels_f (){

	channel_t	*channel;
	int			channels = 0;
	int			i;

	Com_Printf("Current active channels:\n");

	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		if (channel->state == CS_FREE)
			continue;

		channels++;

		Com_Printf("%3i ", i+1);

		// TODO: print the emitter and sound
	}

	Com_Printf("--------------------\n");
	Com_Printf("%i active channels\n", channels);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 S_InitChannels
 ==================
*/
void S_InitChannels (){

	channel_t	*channel;
	int			i;

	// Add commands
	Cmd_AddCommand("listSoundChannels", S_ListSoundChannels_f, "Lists active sound channels", NULL);

	// Allocate the channels
	for (i = 0, channel = snd.channels; i < s_maxChannels->integerValue; i++, channel++){
		qalGenSources(1, &channel->sourceId);

		if (qalGetError() != AL_NO_ERROR)
			break;

		snd.numChannels++;
	}
}

/*
 ==================
 S_ShutdownChannels
 ==================
*/
void S_ShutdownChannels (){

	channel_t	*channel;
	int			i;

	// Remove commands
	Cmd_RemoveCommand("listSoundChannels");

	// Delete all the channels
	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++)
		qalDeleteSources(1, &channel->sourceId);
}