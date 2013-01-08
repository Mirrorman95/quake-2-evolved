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
// s_emitter.c - Sound emitter
//


#include "s_local.h"


static int					s_numEmitters;
static emitter_t *			s_emitterList;

static emitter_t			s_activeEmitter;
static emitter_t *			s_freeEmitter;


/*
 ==================
 S_AddSoundEmitter
 ==================
*/
int S_AddSoundEmitter (const soundEmitter_t *soundEmitter){

	emitter_t	*emitter;

	// Allocate a slot
	if (!s_freeEmitter){
		Com_Printf(S_COLOR_YELLOW "S_AddSoundEmitter: no free emitter slots\n");
		return 0;
	}

	emitter = s_freeEmitter;

	s_numEmitters++;

	// Set up
	emitter->active = true;

	emitter->e = *soundEmitter;

	// Link
	s_freeEmitter = s_freeEmitter->next;

	emitter->prev = &s_activeEmitter;
	emitter->next = s_activeEmitter.next;

	s_activeEmitter.next->prev = emitter;
	s_activeEmitter.next = emitter;

	return emitter->index + 1;
}

/*
 ==================
 S_UpdateSoundEmitter
 ==================
*/
void S_UpdateSoundEmitter (int emitterHandle, const soundEmitter_t *soundEmitter){

	emitter_t	*emitter;

	if (emitterHandle <= 0 || emitterHandle > MAX_SOUND_EMITTERS)
		Com_Error(ERR_DROP, "S_UpdateSoundEmitter: emitterHandle out of range");

	emitter = &s_emitterList[emitterHandle - 1];

	if (!emitter->active){
		Com_Printf(S_COLOR_YELLOW "S_UpdateSoundEmitter: emitterHandle %i is not active\n", emitterHandle);
		return;
	}

	// Development tool
	if (s_skipUpdates->integerValue)
		return;

	snd.pc.emitterUpdates++;

	// Update
	emitter->e = *soundEmitter;
}

/*
 ==================
 S_RemoveSoundEmitter
 ==================
*/
void S_RemoveSoundEmitter (int emitterHandle, bool stopAllSounds){

	emitter_t	*emitter;
	channel_t	*channel;
	int			i;

	if (emitterHandle <= 0 || emitterHandle > MAX_SOUND_EMITTERS)
		Com_Error(ERR_DROP, "S_RemoveSoundEmitter: emitterHandle out of range");

	emitter = &s_emitterList[emitterHandle - 1];

	if (!emitter->active){
		Com_Printf(S_COLOR_YELLOW "S_RemoveSoundEmitter: emitterHandle %i is not active\n", emitterHandle);
		return;
	}

	// Stop all sounds playing from this emitter if desired. Otherwise let the
	// affected channels know that this emitter is no longer active.
	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		if (channel->state == CS_FREE)
			continue;

		if (channel->emitter != emitter)
			continue;

		if (stopAllSounds){
			S_StopChannel(channel);
			continue;
		}

		channel->emitter = NULL;
	}

	emitter->lastLeadIn = NULL;
	emitter->lastEntry = NULL;

	// No longer active
	emitter->active = false;

	// Unlink
	emitter->prev->next = emitter->next;
	emitter->next->prev = emitter->prev;

	// Free the slot
	emitter->next = s_freeEmitter;
	s_freeEmitter = emitter;

	s_numEmitters--;
}


// ============================================================================


/*
 ==================
 S_PlaySoundFromEmitter
 ==================
*/
void S_PlaySoundFromEmitter (int emitterHandle, int channelId, int startTime, soundShader_t *soundShader){

	emitter_t	*emitter;
	channel_t	*channel;
	int			i;

	if (emitterHandle <= 0 || emitterHandle > MAX_SOUND_EMITTERS)
		Com_Error(ERR_DROP, "S_PlaySoundFromEmitter: emitterHandle out of range");

	emitter = &s_emitterList[emitterHandle - 1];

	if (!emitter->active){
		Com_Printf(S_COLOR_YELLOW "S_PlaySoundFromEmitter: emitterHandle %i is not active\n", emitterHandle);
		return;
	}

	// Development tool
	if (s_showEmitters->integerValue)
		Com_Printf("S_PlaySoundFromEmitter( %i, %i, %i, %s )\n", emitterHandle, channelId, startTime, soundShader->name);

	// If this sound should only be played once, check if it is already playing
	// on the given channel
	if (soundShader->flags & SSF_PLAYONCE){
		for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
			if (channel->state == CS_FREE)
				continue;

			if (channel->emitter != emitter)
				continue;

			if (channel->channelId == channelId || channelId == SOUND_CHANNEL_ANY){
				if (channel->soundShader == soundShader)
					return;
			}
		}
	}

	// Pick a channel and play the sound
	channel = S_PickChannel(emitter, emitter->e.emitterId, channelId, soundShader);
	if (!channel)
		return;

	if (snd.listener.time - startTime < 250 || !(soundShader->flags & SSF_LOOPING))
		S_PlayChannel(channel, true);
	else
		S_PlayChannel(channel, false);
}

/*
 ==================
 S_StopSoundFromEmitter
 ==================
*/
void S_StopSoundFromEmitter (int emitterHandle, int channelId){

	emitter_t	*emitter;
	channel_t	*channel;
	int			i;

	if (emitterHandle <= 0 || emitterHandle > MAX_SOUND_EMITTERS)
		Com_Error(ERR_DROP, "S_StopSoundFromEmitter: emitterHandle out of range");

	emitter = &s_emitterList[emitterHandle - 1];

	if (!emitter->active){
		Com_Printf(S_COLOR_YELLOW "S_StopSoundFromEmitter: emitterHandle %i is not active\n", emitterHandle);
		return;
	}

	// Development tool
	if (s_showEmitters->integerValue)
		Com_Printf("S_StopSoundFromEmitter( %i, %i )\n", emitterHandle, channelId);

	// Stop all sounds playing on the given channel
	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		if (channel->state == CS_FREE)
			continue;

		if (channel->emitter != emitter)
			continue;

		if (channel->channelId == channelId || channelId == SOUND_CHANNEL_ANY)
			S_StopChannel(channel);
	}
}

/*
 ==================
 S_AmplitudeFromEmitter
 ==================
*/
float S_AmplitudeFromEmitter (int emitterHandle, int channelId){

	emitter_t	*emitter;
	channel_t	*channel;
	float		amplitude;
	int			i;

	if (emitterHandle <= 0 || emitterHandle > MAX_SOUND_EMITTERS)
		Com_Error(ERR_DROP, "S_AmplitudeFromEmitter: emitterHandle out of range");

	emitter = &s_emitterList[emitterHandle - 1];

	if (!emitter->active){
		Com_Printf(S_COLOR_YELLOW "S_AmplitudeFromEmitter: emitterHandle %i is not active\n", emitterHandle);
		return 0.0f;
	}

	// Development tool
	if (s_showEmitters->integerValue)
		Com_Printf("S_AmplitudeFromEmitter( %i, %i )\n", emitterHandle, channelId);

	// Accumulate the amplitude of all sounds playing on the given channel
	amplitude = 0.0f;

	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		if (channel->state == CS_FREE)
			continue;

		if (channel->emitter != emitter)
			continue;

		if (channel->channelId == channelId || channelId == SOUND_CHANNEL_ANY){
			if (s_skipFlicker->integerValue || (channel->soundShader->flags & SSF_NOFLICKER)){
				amplitude += 1.0f;
				continue;
			}

			amplitude += channel->amplitude;
		}
	}

	return ClampFloat(amplitude, 0.0f, 1.0f);
}

/*
 ==================
 S_PlayingFromEmitter
 ==================
*/
bool S_PlayingFromEmitter (int emitterHandle, int channelId){

	emitter_t	*emitter;
	channel_t	*channel;
	int			i;

	if (emitterHandle <= 0 || emitterHandle > MAX_SOUND_EMITTERS)
		Com_Error(ERR_DROP, "S_PlayingFromEmitter: emitterHandle out of range");

	emitter = &s_emitterList[emitterHandle - 1];

	if (!emitter->active){
		Com_Printf(S_COLOR_YELLOW "S_PlayingFromEmitter: emitterHandle %i is not active\n", emitterHandle);
		return false;
	}

	// Development tool
	if (s_showEmitters->integerValue)
		Com_Printf("S_PlayingFromEmitter( %i, %i )\n", emitterHandle, channelId);

	// Check for any sounds playing on the given channel
	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		if (channel->state == CS_FREE)
			continue;

		if (channel->emitter != emitter)
			continue;

		if (channel->channelId == channelId || channelId == SOUND_CHANNEL_ANY)
			return true;
	}

	return false;
}


// ============================================================================


/*
 ==================
 S_ClearAllSoundEmitters
 ==================
*/
void S_ClearAllSoundEmitters (){

	emitter_t	*emitter, *next;
	channel_t	*channel;
	int			i;

	if (!s_emitterList)
		return;

	// Free all the emitters
	for (emitter = s_activeEmitter.next; emitter != &s_activeEmitter; emitter = next){
		// Grab next now, so if the emitter is freed we still have it
		next = emitter->next;

		// If this is the local emitter, ignore it
		if (emitter == snd.localEmitter)
			continue;

		// Stop all sounds playing from this emitter
		for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
			if (channel->state == CS_FREE)
				continue;

			if (channel->emitter != emitter)
				continue;

			S_StopChannel(channel);
		}

		emitter->lastLeadIn = NULL;
		emitter->lastEntry = NULL;

		// No longer active
		emitter->active = false;

		// Unlink
		emitter->prev->next = emitter->next;
		emitter->next->prev = emitter->prev;

		// Free the slot
		emitter->next = s_freeEmitter;
		s_freeEmitter = emitter;

		s_numEmitters--;
	}
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 S_ListSoundEmitters_f
 ==================
*/
static void S_ListSoundEmitters_f (){

	emitter_t	*emitter;
	channel_t	*channel;
	int			i, j;

	for (i = 0, emitter = s_emitterList; i < MAX_SOUND_EMITTERS; i++, emitter++){
		if (!emitter->active && emitter != snd.localEmitter)
			continue;

		Com_Printf("%5i: ", emitter->index + 1);

		Com_Printf("(%7i, %7i, %7i) ", FloatToInt(emitter->e.origin[0]),FloatToInt(emitter->e.origin[1]), FloatToInt(emitter->e.origin[2]));

		if (emitter == snd.localEmitter)
			Com_Printf("LOCAL");
		else
			Com_Printf("WORLD");

		Com_Printf("\n");

		for (j = 0, channel = snd.channels; j < snd.numChannels; j++, channel++){
			if (channel->state == CS_FREE)
				continue;

			if (channel->emitter != emitter)
				continue;

			Com_Printf("       channel %2i: %s\n", channel->channelId, channel->soundShader->name);
		}
	}

	Com_Printf("--------------------\n");
	Com_Printf("%i active sound emitters\n", s_numEmitters);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 S_InitEmitters
 ==================
*/
void S_InitEmitters (){

	int		i;

	// Add commands
	Cmd_AddCommand("listSoundEmitters", S_ListSoundEmitters_f, "Lists sound emitters", NULL);

	// Allocate and set up the emitter list
	s_numEmitters = 1;
	s_emitterList = (emitter_t *)Mem_ClearedAlloc(MAX_SOUND_EMITTERS * sizeof(emitter_t), TAG_SOUND);

	for (i = 0; i < MAX_SOUND_EMITTERS; i++){
		s_emitterList[i].index = i;

		if (i < MAX_SOUND_EMITTERS - 1)
			s_emitterList[i].next = &s_emitterList[i+1];
	}

	// Set up the active list
	s_activeEmitter.prev = &s_activeEmitter;
	s_activeEmitter.next = &s_activeEmitter;

	// Set up the free list
	s_freeEmitter = s_emitterList;

	// Set up the local emitter
	snd.localEmitter = s_freeEmitter;

	snd.localEmitter->e.emitterId = -1;
	VectorClear(snd.localEmitter->e.origin);
	VectorClear(snd.localEmitter->e.velocity);
	VectorClear(snd.localEmitter->e.direction);
	Matrix3_Identity(snd.localEmitter->e.axis);
	snd.localEmitter->e.area = -1;
	snd.localEmitter->e.underwater = false;
	snd.localEmitter->e.soundParms[SOUNDPARM_VOLUME] = 1.0f;
	snd.localEmitter->e.soundParms[SOUNDPARM_PITCH] = 1.0f;
	snd.localEmitter->e.soundParms[SOUNDPARM_DRYFILTER] = 1.0f;
	snd.localEmitter->e.soundParms[SOUNDPARM_DRYFILTERHF] = 1.0f;
	snd.localEmitter->e.soundParms[SOUNDPARM_WETFILTER] = 1.0f;
	snd.localEmitter->e.soundParms[SOUNDPARM_WETFILTERHF] = 1.0f;
	snd.localEmitter->e.soundParms[SOUNDPARM_MINDISTANCE] = 0.0f;
	snd.localEmitter->e.soundParms[SOUNDPARM_MAXDISTANCE] = 0.0f;

	s_freeEmitter = s_freeEmitter->next;

	snd.localEmitter->prev = &s_activeEmitter;
	snd.localEmitter->next = s_activeEmitter.next;

	s_activeEmitter.next->prev = snd.localEmitter;
	s_activeEmitter.next = snd.localEmitter;
}

/*
 ==================
 S_ShutdownEmitters
 ==================
*/
void S_ShutdownEmitters (){

	// Remove commands
	Cmd_RemoveCommand("listSoundEmitters");

	// Clear emitter list
	if (!s_emitterList)
		return;

	s_activeEmitter.prev = &s_activeEmitter;
	s_activeEmitter.next = &s_activeEmitter;

	s_freeEmitter = NULL;
}