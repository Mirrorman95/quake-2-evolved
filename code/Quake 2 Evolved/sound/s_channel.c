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


#define WATER_SPEED_SCALE			2.2f
#define WATER_DISTANCE_SCALE		(1.0f / 2.2f)

#define UNDERWATER_SPEED_SCALE		4.4f
#define UNDERWATER_DISTANCE_SCALE	(1.0f / 4.4f)


/*
 ==================
 S_CurrentSound

 Returns the sound currently playing on the given channel
 ==================
*/
static sound_t *S_CurrentSound (channel_t *channel, soundShader_t *soundShader){

	uint	buffer;
	int		i;

	qalGetSourcei(channel->sourceId, AL_BUFFER, (int *)&buffer);

	for (i = 0; i < soundShader->numLeadIns; i++){
		if (soundShader->leadIns[i]->bufferId == buffer)
			return soundShader->leadIns[i];
	}

	for (i = 0; i < soundShader->numEntries; i++){
		if (soundShader->entries[i]->bufferId == buffer)
			return soundShader->entries[i];
	}

	// Should never happen
	return NULL;
}

/*
 ==================
 S_SelectSound

 Selects a random sound for playing on the given channel
 ==================
*/
static sound_t *S_SelectSound (channel_t *channel, soundShader_t *soundShader, bool leadIn){

	sound_t	*sound;
	int		index;

	// Select a lead-in sound if desired
	if (leadIn){
		if (soundShader->numLeadIns == 1)
			index = 0;
		else {
			index = rand() % soundShader->numLeadIns;

			// If desired, try not to play the same random sound twice in a row
			if (channel->emitter && (soundShader->flags & SSF_NODUPS)){
				if (channel->emitter->lastLeadIn == soundShader->leadIns[index])
					index = (index + 1) % soundShader->numLeadIns;
			}
		}

		sound = soundShader->leadIns[index];

		if (channel->emitter)
			channel->emitter->lastLeadIn = sound;

		return sound;
	}

	// Select a sound
	if (soundShader->numEntries == 1)
		index = 0;
	else {
		index = rand() % soundShader->numEntries;

		// If desired, try not to play the same random sound twice in a row
		if (channel->emitter && (soundShader->flags & SSF_NODUPS)){
			if (channel->emitter->lastEntry == soundShader->entries[index])
				index = (index + 1) % soundShader->numEntries;
		}
	}

	sound = soundShader->entries[index];

	if (channel->emitter)
		channel->emitter->lastEntry = sound;

	return sound;
}

/*
 ==================
 S_SpatializeChannel

 TODO: vectors, portal, and filters

 Computes all the spatialization parameters for the given channel
 ==================
*/
static void S_SpatializeChannel (channel_t *channel, soundShader_t *soundShader){

	vec3_t	direction, tmp;
	float	distance, fraction;
	float	speedOfSound, dopplerShift;
	float	maxVelocity, lVelocity, sVelocity;
	float	attenuation, filtering;

	// If spatialization is disabled or not required
	if (s_skipSpatialization->integerValue || channel->emitterId == snd.listener.listenerId || (soundShader->flags & SSF_GLOBAL)){
		channel->p.spatialized = false;

		// Calculate reachability
		if (channel->e.area == -1 || snd.listener.area == -1)
			channel->p.reachable = false;
		else
			channel->p.reachable = true;

		// Clear min/max distances
		channel->p.minDistance = 0.0f;
		channel->p.maxDistance = 0.0f;

		// Clear direction and distance to listener
		VectorClear(channel->p.dirToListener);
		channel->p.distToListener = 0.0f;

		// Clear number of portals passed and blocked
		channel->p.portalsPassed = 0;
		channel->p.portalsBlocked = 0;

		// Clear origin and direction
		VectorClear(channel->p.origin);
		VectorClear(channel->p.direction);

		// Compute volume
		if (s_skipEmitters->integerValue || !channel->p.reachable)
			channel->p.volume = 0.0f;
		else {
			if (s_skipDynamic->integerValue || (soundShader->flags & SSF_NODYNAMICPARMS))
				channel->p.volume = soundShader->volume;
			else
				channel->p.volume = ClampFloat(soundShader->volume * channel->e.soundParms[SOUNDPARM_VOLUME], 0.0f, 1.0f);
		}

		// Compute pitch
		if (s_skipDynamic->integerValue || (soundShader->flags & SSF_NODYNAMICPARMS))
			channel->p.pitch = soundShader->pitch;
		else
			channel->p.pitch = ClampFloat(soundShader->pitch * channel->e.soundParms[SOUNDPARM_PITCH], 0.1f, 10.0f);

		// Compute low-pass filters
		if (s_skipFilters->integerValue || !channel->p.reachable){
			channel->p.dryFilter.gain = 1.0f;
			channel->p.dryFilter.gainHF = 1.0f;

			channel->p.wetFilter.gain = 1.0f;
			channel->p.wetFilter.gainHF = 1.0f;
		}
		else {
			if (s_skipDynamic->integerValue || (soundShader->flags & SSF_NODYNAMICPARMS)){
				channel->p.dryFilter.gain = soundShader->dryFilter;
				channel->p.dryFilter.gainHF = soundShader->dryFilterHF;

				channel->p.wetFilter.gain = soundShader->wetFilter;
				channel->p.wetFilter.gainHF = soundShader->wetFilterHF;
			}
			else {
				channel->p.dryFilter.gain = ClampFloat(soundShader->dryFilter * channel->e.soundParms[SOUNDPARM_DRYFILTER], 0.0f, 1.0f);
				channel->p.dryFilter.gainHF = ClampFloat(soundShader->dryFilterHF * channel->e.soundParms[SOUNDPARM_DRYFILTERHF], 0.0f, 1.0f);

				channel->p.wetFilter.gain = ClampFloat(soundShader->wetFilter * channel->e.soundParms[SOUNDPARM_WETFILTER], 0.0f, 1.0f);
				channel->p.wetFilter.gainHF = ClampFloat(soundShader->wetFilterHF * channel->e.soundParms[SOUNDPARM_WETFILTERHF], 0.0f, 1.0f);
			}
		}

		// Compute environmental effects
		if (!channel->p.reachable)
			channel->p.feedReverb = false;
		else {
			// Determine if it needs to feed the reverb
			if (!s_skipReverbs->integerValue && !(soundShader->flags & SSF_NOREVERB))
				channel->p.feedReverb = true;
			else
				channel->p.feedReverb = false;
		}

		// If this is a private sound, check if it should be muted
		if (soundShader->flags & SSF_PRIVATE){
			if (channel->emitterId != snd.listener.listenerId)
				channel->p.volume = 0.0f;
		}

		// If this is an anti-private sound, check if it should be muted
		if (soundShader->flags & SSF_ANTIPRIVATE){
			if (channel->emitterId == snd.listener.listenerId)
				channel->p.volume = 0.0f;
		}

		// If low-pass filters are disabled, scale the computed volume by the
		// computed direct path's attenuation level
		if (!snd.filter.enabled)
			channel->p.volume *= channel->p.dryFilter.gain;

		return;
	}

	channel->p.spatialized = true;

	// Set up min/max distances
	if ((channel->e.soundParms[SOUNDPARM_MINDISTANCE] || channel->e.soundParms[SOUNDPARM_MAXDISTANCE]) && !(soundShader->flags & SSF_NODYNAMICPARMS)){
		channel->p.minDistance = channel->e.soundParms[SOUNDPARM_MINDISTANCE];
		channel->p.maxDistance = channel->e.soundParms[SOUNDPARM_MAXDISTANCE];
	}
	else {
		channel->p.minDistance = soundShader->minDistance;
		channel->p.maxDistance = soundShader->maxDistance;
	}

	// Calculate direction to listener
	VectorSubtract(snd.listener.origin, channel->e.origin, channel->p.dirToListener);
	VectorNormalize(channel->p.dirToListener);

	// Calculate reachability, distance to listener, and number of portals
	// passed and blocked, flowing through the portals if desired
	if (!s_skipPortals->integerValue && !(soundShader->flags & SSF_NOPORTALFLOW)){
//		channel->p.reachable = S_PortalFlow(channel->e.origin, channel->e.area, snd.listener.origin, snd.listener.area, channel->e.portal, channel->p.maxDistance, &channel->p.distToListener, &channel->p.portalsPassed, &channel->p.portalsBlocked);
	}
	else {
		if (channel->e.area == -1 || snd.listener.area == -1){
			channel->p.reachable = false;

			channel->p.distToListener = 0.0f;

			channel->p.portalsPassed = 0;
			channel->p.portalsBlocked = 0;
		}
		else {
			channel->p.reachable = true;

			channel->p.distToListener = Distance(channel->e.origin, snd.listener.origin);

			if (channel->e.area == snd.listener.area){
				channel->p.portalsPassed = 0;
				channel->p.portalsBlocked = 0;
			}
			else {
				if (CM_AreasAreConnected(channel->e.area, snd.listener.area/*, PC_SOUND*/)){
					channel->p.portalsPassed = 1;
					channel->p.portalsBlocked = 0;
				}
				else {
					channel->p.portalsPassed = 0;
					channel->p.portalsBlocked = 1;
				}
			}
		}
	}

	if (s_skipAttenuation->integerValue)
		channel->p.distToListener = 0.0f;

	// Adjust distance to listener if underwater or crossing a water surface
	if (channel->e.underwater && snd.listener.underwater)
		channel->p.distToListener *= UNDERWATER_DISTANCE_SCALE;
	else if (channel->e.underwater || snd.listener.underwater)
		channel->p.distToListener *= WATER_DISTANCE_SCALE;

	// Set up origin
	if (s_skipAttenuation->integerValue || channel->p.minDistance >= channel->p.maxDistance || soundShader->rolloffFactor == 0.0f)
		VectorCopy(snd.listener.origin, channel->p.origin);
	else {
		if (snd.listener.underwater)
			VectorMA(snd.listener.origin, channel->p.distToListener, snd.listener.axis[2], channel->p.origin);
		else {
			if (soundShader->flags & SSF_OMNIDIRECTIONAL)
				VectorMA(snd.listener.origin, channel->p.distToListener, snd.listener.axis[2], channel->p.origin);
			else if (soundShader->flags & SSF_VOLUMETRIC){
				distance = ClampFloat(channel->p.distToListener, channel->p.minDistance, channel->p.maxDistance);
				fraction = 1.0f - (distance - channel->p.minDistance) / (channel->p.maxDistance - channel->p.minDistance);
#if 0
				direction.LerpFast(-channel->p.dirToListener, snd.listener.axis[2], fraction);
				VectorNormalize(direction);

				channel->p.origin = snd.listener.origin + direction * channel->p.distToListener;
#endif
			}
			else {
				// FIXME: not sure if this is right
				VectorSubtract(snd.listener.origin, channel->p.dirToListener, tmp);
				VectorScale(tmp, channel->p.distToListener, channel->p.origin);
			}
		}
	}

	// Set up direction
	if (s_skipCones->integerValue || soundShader->coneInnerAngle >= soundShader->coneOuterAngle || soundShader->coneOuterVolume == 1.0f)
		VectorClear(channel->p.direction);
	else {
		if (snd.listener.underwater)
			VectorClear(channel->p.direction);
		else
			VectorRotate(channel->e.direction, channel->e.axis, channel->p.direction);		// FIXME: not sure if this is right
	}

	// Compute volume
	if (s_skipEmitters->integerValue || !channel->p.reachable)
		channel->p.volume = 0.0f;
	else {
		if (s_skipDynamic->integerValue || (soundShader->flags & SSF_NODYNAMICPARMS))
			channel->p.volume = soundShader->volume;
		else
			channel->p.volume = ClampFloat(soundShader->volume * channel->e.soundParms[SOUNDPARM_VOLUME], 0.0f, 1.0f);
	}

	// Compute pitch
	if (s_skipDynamic->integerValue || (soundShader->flags & SSF_NODYNAMICPARMS))
		channel->p.pitch = soundShader->pitch;
	else
		channel->p.pitch = ClampFloat(soundShader->pitch * channel->e.soundParms[SOUNDPARM_PITCH], 0.1f, 10.0f);

	// Unfortunately OpenAL lacks the ability to set the speed of sound and
	// doppler factor on a per-source basis, plus we mess with the sound origin
	// and that would result in an incorrect direction to listener, so we must
	// compute the doppler shift internally and modify the computed pitch
	if (s_dopplerShifts->integerValue && soundShader->dopplerFactor){
		speedOfSound = METERS2UNITS(s_speedOfSound->floatValue);

		// Adjust speed of sound if underwater or crossing a water surface
		if (channel->e.underwater && snd.listener.underwater)
			speedOfSound *= UNDERWATER_SPEED_SCALE;
		else if (channel->e.underwater || snd.listener.underwater)
			speedOfSound *= WATER_SPEED_SCALE;

		// Compute velocities
		maxVelocity = (speedOfSound / soundShader->dopplerFactor) - 1.0f;

		lVelocity = DotProduct(channel->p.dirToListener, snd.listener.velocity);
		lVelocity = ClampFloat(lVelocity, -maxVelocity, maxVelocity);

		sVelocity = DotProduct(channel->p.dirToListener, channel->e.velocity);
		sVelocity = ClampFloat(sVelocity, -maxVelocity, maxVelocity);

		// Compute doppler shift
		dopplerShift = (speedOfSound - lVelocity * soundShader->dopplerFactor) / (speedOfSound - sVelocity * soundShader->dopplerFactor);

		// Modify pitch
		channel->p.pitch = Max(channel->p.pitch * dopplerShift, 0.001f);
	}

	// Compute low-pass filters
	if (s_skipFilters->integerValue || !channel->p.reachable){
		channel->p.dryFilter.gain = 1.0f;
		channel->p.dryFilter.gainHF = 1.0f;

		channel->p.wetFilter.gain = 1.0f;
		channel->p.wetFilter.gainHF = 1.0f;
	}
	else {
		if (s_skipDynamic->integerValue || (soundShader->flags & SSF_NODYNAMICPARMS)){
			channel->p.dryFilter.gain = soundShader->dryFilter;
			channel->p.dryFilter.gainHF = soundShader->dryFilterHF;

			channel->p.wetFilter.gain = soundShader->wetFilter;
			channel->p.wetFilter.gainHF = soundShader->wetFilterHF;
		}
		else {
			channel->p.dryFilter.gain = ClampFloat(soundShader->dryFilter * channel->e.soundParms[SOUNDPARM_DRYFILTER], 0.0f, 1.0f);
			channel->p.dryFilter.gainHF = ClampFloat(soundShader->dryFilterHF * channel->e.soundParms[SOUNDPARM_DRYFILTERHF], 0.0f, 1.0f);

			channel->p.wetFilter.gain = ClampFloat(soundShader->wetFilter * channel->e.soundParms[SOUNDPARM_WETFILTER], 0.0f, 1.0f);
			channel->p.wetFilter.gainHF = ClampFloat(soundShader->wetFilterHF * channel->e.soundParms[SOUNDPARM_WETFILTERHF], 0.0f, 1.0f);
		}
	}

	// Compute environmental effects
	if (!channel->p.reachable)
		channel->p.feedReverb = false;
	else {
		// Determine if it needs to feed the reverb
		if (!s_skipReverbs->integerValue && !(soundShader->flags & SSF_NOREVERB))
			channel->p.feedReverb = true;
		else
			channel->p.feedReverb = false;
#if 0
		// Compute obstruction if desired
		if (!s_skipObstructions->integerValue && !(soundShader->flags & SSF_NOOBSTRUCTION)){
			S_ObstructionFilter(channel->e.origin, snd.listener.origin, channel->p.distToListener, channel->p.minDistance, channel->p.maxDistance, &channel->p.dryFilter);

			// Account for underwater
			if (channel->e.underwater && snd.listener.underwater){
				attenuation = 1.0f - (0.25f * s_obstructionScale->floatValue);
				attenuation = Max(attenuation, 0.0f);

				filtering = 1.0f - (0.95f * s_obstructionScale->floatValue);
				filtering = Max(filtering, 0.0f);

				// Apply attenuation and filtering to the direct path
				channel->p.dryFilter.gain *= attenuation;
				channel->p.dryFilter.gainHF *= filtering;
			}
		}

		// Compute exclusion if desired
		if (!s_skipExclusions->integerValue && !(soundShader->flags & SSF_NOEXCLUSION))
			S_ExclusionFilter(channel->p.portalsPassed, &channel->p.wetFilter);

		// Compute occlusion if desired
		if (!s_skipOcclusions->integerValue && !(soundShader->flags & SSF_NOOCCLUSION)){
			S_OcclusionFilter(channel->p.portalsBlocked, &channel->p.dryFilter, &channel->p.wetFilter);

			// Account for crossing a water surface
			if (channel->e.underwater ^ snd.listener.underwater){
				attenuation = 1.0f - (0.5f * s_obstructionScale->floatValue);
				attenuation = Max(attenuation, 0.0f);

				filtering = 1.0f - (0.95f * s_obstructionScale->floatValue);
				filtering = Max(filtering, 0.0f);

				// Apply attenuation and filtering to the direct path
				channel->p.dryFilter.gain *= attenuation;
				channel->p.dryFilter.gainHF *= filtering;

				// Apply attenuation and filtering to the reverb path
				channel->p.wetFilter.gain *= attenuation * 0.5f;
				channel->p.wetFilter.gainHF *= filtering;
			}
		}
#endif
	}

	// If this is a private sound, check if it should be muted
	if (soundShader->flags & SSF_PRIVATE){
		if (channel->emitterId != snd.listener.listenerId)
			channel->p.volume = 0.0f;
	}

	// If this is an anti-private sound, check if it should be muted
	if (soundShader->flags & SSF_ANTIPRIVATE){
		if (channel->emitterId == snd.listener.listenerId)
			channel->p.volume = 0.0f;
	}

	// If low-pass filters are disabled, scale the computed volume by the
	// computed direct path's attenuation level
	if (!snd.filter.enabled)
		channel->p.volume *= channel->p.dryFilter.gain;
}


// ============================================================================


/*
 ==================
 S_PickChannel
 ==================
*/
channel_t *S_PickChannel (emitter_t *emitter, int emitterId, int channelId, soundShader_t *soundShader){

	channel_t	*channel;
	uint		buffer;
	int			processed;
	int			index;
	int			i;

	// Try to find a free channel
	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		if (channel->state == CS_FREE){
			index = i;
			break;
		}
	}

	// If we didn't find a free channel
	if (i == snd.numChannels){
		index = -1;

		// Try to override an active channel
		for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
			// Never override local sounds with other sounds
			if (channel->emitter == snd.localEmitter && emitter != snd.localEmitter)
				continue;

			// Never override local player sounds with other sounds
			if (channel->emitterId == snd.listener.listenerId && emitterId != snd.listener.listenerId)
				continue;

			// Never override sounds with higher priority
			if (channel->soundShader->priority > soundShader->priority)
				continue;

			// Always override sounds from the same emitter on the same channel
			if (channelId != SOUND_CHANNEL_ANY){
				if (channel->emitter == emitter && channel->channelId == channelId){
					// If this sound shouldn't be restarted or overridden
					if (channel->soundShader == soundShader){
						if (channel->soundShader->flags & SSF_NORESTART)
							continue;
					}
					else {
						if (channel->soundShader->flags & SSF_NOOVERRIDE)
							continue;
					}

					// Override this sound
					index = i;
					break;
				}
			}

			// If we don't have a channel yet, override this sound
			if (index == -1){
				index = i;
				continue;
			}

			// Always override sounds with lower priority
			if (channel->soundShader->priority < snd.channels[index].soundShader->priority){
				index = i;
				continue;
			}

			// Always override the oldest sounds if they have equal priority
			if (channel->soundShader->priority == snd.channels[index].soundShader->priority){
				if (channel->allocTime < snd.channels[index].allocTime){
					index = i;
					continue;
				}
			}
		}
	}

	// Do we have a channel?
	if (index == -1){
		Com_DPrintf(S_COLOR_YELLOW "Dropped sound '%s'\n", soundShader->name);

		return NULL;
	}

	channel = &snd.channels[index];

	// If overriding a sound
	if (channel->state != CS_FREE){
		Com_DPrintf(S_COLOR_YELLOW "Overridden sound '%s'\n", channel->soundShader->name);

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
	channel->index = index;

	channel->streaming = false;

	channel->lastSound = NULL;

	channel->amplitude = 0.0f;

	channel->emitterId = emitterId;
	channel->channelId = channelId;
	channel->allocTime = snd.time;

	channel->emitter = emitter;
	channel->soundShader = soundShader;

	// Also copy the sound emitter parameters because it may be our only chance
	channel->e = emitter->e;

	return channel;
}

/*
 ==================
 S_PlayChannel
 ==================
*/
void S_PlayChannel (channel_t *channel, bool allowLeadIn){

	soundShader_t	*soundShader = channel->soundShader;
	sound_t			*sound, *nextSound;
	int				offset;

	if (channel->state != CS_FREE)
		return;		// Already active

	QAL_LogPrintf("----- S_PlayChannel ( %s ) -----\n", soundShader->name);

	// Set the channel state
	if (soundShader->numLeadIns){
		if (soundShader->flags & SSF_LOOPING){
			if (soundShader->flags & SSF_RANDOMIZE)
				channel->state = CS_LEADIN_RANDOM;
			else
				channel->state = CS_LEADIN_LOOPED;
		}
		else
			channel->state = CS_LEADIN_NORMAL;
	}
	else {
		if (soundShader->flags & SSF_LOOPING){
			if (soundShader->flags & SSF_RANDOMIZE)
				channel->state = CS_RANDOM;
			else
				channel->state = CS_LOOPED;
		}
		else
			channel->state = CS_NORMAL;
	}

	if (!allowLeadIn){
		if (channel->state == CS_LEADIN_NORMAL)
			channel->state = CS_NORMAL;
		else if (channel->state == CS_LEADIN_LOOPED)
			channel->state = CS_LOOPED;
		else if (channel->state == CS_LEADIN_RANDOM)
			channel->state = CS_RANDOM;
	}

	// Set sound
	if (channel->state == CS_NORMAL || channel->state == CS_LOOPED){
		// Disable streaming
		channel->streaming = false;

		// Select a sound
		sound = S_SelectSound(channel, soundShader, false);

		channel->lastSound = sound;

		// Set the buffer
		qalSourcei(channel->sourceId, AL_BUFFER, sound->bufferId);
	}
	else {
		// Enable streaming
		channel->streaming = true;

		// Select two sounds
		if (channel->state != CS_RANDOM)
			sound = S_SelectSound(channel, soundShader, true);
		else
			sound = S_SelectSound(channel, soundShader, false);

		nextSound = S_SelectSound(channel, soundShader, false);

		channel->lastSound = nextSound;

		// Queue the buffers
		qalSourceQueueBuffers(channel->sourceId, 1, &sound->bufferId);
		qalSourceQueueBuffers(channel->sourceId, 1, &nextSound->bufferId);
	}

	// Set looping
	if (channel->state == CS_LOOPED)
		qalSourcei(channel->sourceId, AL_LOOPING, AL_TRUE);
	else
		qalSourcei(channel->sourceId, AL_LOOPING, AL_FALSE);

	// Set sample offset
	if (channel->state != CS_LOOPED && channel->state != CS_RANDOM)
		qalSourcei(channel->sourceId, AL_SAMPLE_OFFSET, 0);
	else {
		if (soundShader->flags & SSF_NOOFFSET)
			offset = 0;
		else if (soundShader->flags & SSF_NOTIMEOFFSET)
			offset = FloatToInt(sound->samples * frand()) % sound->samples;
		else {
			if (channel->emitter == snd.localEmitter)
				offset = FloatToInt(sound->rate * MS2SEC(snd.time)) % sound->samples;
			else
				offset = FloatToInt(sound->rate * MS2SEC(snd.listener.time)) % sound->samples;
		}

		qalSourcei(channel->sourceId, AL_SAMPLE_OFFSET, offset);
	}

	// If a local sound
	if (channel->emitter == snd.localEmitter){
		// Set source parameters
		qalSourcei(channel->sourceId, AL_SOURCE_RELATIVE, AL_TRUE);

		qalSource3f(channel->sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
		qalSource3f(channel->sourceId, AL_DIRECTION, 0.0f, 0.0f, 0.0f);

		qalSourcef(channel->sourceId, AL_GAIN, soundShader->volume);
		qalSourcef(channel->sourceId, AL_PITCH, soundShader->pitch);

		qalSourcef(channel->sourceId, AL_REFERENCE_DISTANCE, 0.0f);
		qalSourcef(channel->sourceId, AL_MAX_DISTANCE, 0.0f);

		qalSourcef(channel->sourceId, AL_CONE_INNER_ANGLE, 360.0f);
		qalSourcef(channel->sourceId, AL_CONE_OUTER_ANGLE, 360.0f);
		qalSourcef(channel->sourceId, AL_CONE_OUTER_GAIN, 1.0f);

		qalSourcef(channel->sourceId, AL_ROLLOFF_FACTOR, 0.0f);

		if (alConfig.efxAvailable){
			qalSourcef(channel->sourceId, AL_ROOM_ROLLOFF_FACTOR, 0.0f);
			qalSourcef(channel->sourceId, AL_AIR_ABSORPTION_FACTOR, 0.0f);

			qalSourcei(channel->sourceId, AL_DIRECT_FILTER, 0);
			qalSource3i(channel->sourceId, AL_AUXILIARY_SEND_FILTER, 0, 0, 0);
		}
	}

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

	soundShader_t	*soundShader = channel->soundShader;
	uint			buffer;
	int				processed;

	if (channel->state == CS_FREE)
		return;		// Not active

	QAL_LogPrintf("----- S_StopChannel ( %s ) -----\n", soundShader->name);

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

	// Check for errors
	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();

	QAL_LogPrintf("--------------------\n");
}

/*
 ==================
 S_UpdateChannel

 TODO: filters
 ==================
*/
void S_UpdateChannel (channel_t *channel){

	soundShader_t	*soundShader = channel->soundShader;
	sound_t			*sound;
	uint			buffer;
	int				processed, queued;
	int				state, offset;

	if (channel->state == CS_FREE)
		return;		// Not active

	QAL_LogPrintf("----- S_UpdateChannel ( %s ) -----\n", soundShader->name);

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
		if (channel->state == CS_NORMAL || channel->state == CS_LEADIN_NORMAL){
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
	if (channel->streaming){
		qalGetSourcei(channel->sourceId, AL_BUFFERS_QUEUED, &queued);

		while (queued < 2){
			if (channel->state == CS_LEADIN_NORMAL)
				break;		// Don't queue any more

			// Select a sound
			if (channel->state == CS_LEADIN_LOOPED)
				sound = channel->lastSound;
			else
				sound = S_SelectSound(channel, soundShader, false);

			channel->lastSound = sound;

			// Queue the buffer
			qalSourceQueueBuffers(channel->sourceId, 1, &sound->bufferId);

			queued++;
		}
	}

	// If a local sound
	if (channel->emitter == snd.localEmitter){
		snd.pc.localChannels++;

		// Play the source if needed
		if (state != AL_PLAYING)
			qalSourcePlay(channel->sourceId);

		// Check for errors
		if (!s_ignoreALErrors->integerValue)
			S_CheckForErrors();

		QAL_LogPrintf("--------------------\n");

		return;
	}

	snd.pc.worldChannels++;

	// Update the current base amplitude
	sound = S_CurrentSound(channel, soundShader);

	if (sound == NULL)
		channel->amplitude = 0.0f;
	else {
		qalGetSourcei(channel->sourceId, AL_SAMPLE_OFFSET, &offset);

		offset = (offset * 100) / sound->rate;

		channel->amplitude = sound->tableValues[offset % sound->tableSize];
	}

	// Update the sound emitter parameters if possible
	if (channel->emitter)
		channel->e = channel->emitter->e;

	// Compute the spatialization parameters
	S_SpatializeChannel(channel, soundShader);
#if 0
	// Update the low-pass filters
	S_UpdateFilters(channel->index, &channel->p.dryFilter, &channel->p.wetFilter);
#endif
	// Development tool
	if (s_singleEmitter->integerValue != -1){
		if (channel->emitter == NULL || s_singleEmitter->integerValue != channel->emitter->index)
			channel->p.volume = 0.0f;
	}

	// Update source parameters
	if (!channel->p.spatialized){
		qalSourcei(channel->sourceId, AL_SOURCE_RELATIVE, AL_TRUE);

		qalSource3f(channel->sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
		qalSource3f(channel->sourceId, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
	}
	else {
		qalSourcei(channel->sourceId, AL_SOURCE_RELATIVE, AL_FALSE);

		qalSource3f(channel->sourceId, AL_POSITION, -UNITS2METERS(channel->p.origin[1]), UNITS2METERS(channel->p.origin[2]), -UNITS2METERS(channel->p.origin[0]));
		qalSource3f(channel->sourceId, AL_DIRECTION, -channel->p.direction[1], channel->p.direction[2], -channel->p.direction[0]);
	}

	qalSourcef(channel->sourceId, AL_GAIN, channel->p.volume * s_emitterVolume->floatValue);
	qalSourcef(channel->sourceId, AL_PITCH, channel->p.pitch);

	if (!channel->p.spatialized){
		qalSourcef(channel->sourceId, AL_REFERENCE_DISTANCE, 0.0f);
		qalSourcef(channel->sourceId, AL_MAX_DISTANCE, 0.0f);

		qalSourcef(channel->sourceId, AL_CONE_INNER_ANGLE, 360.0f);
		qalSourcef(channel->sourceId, AL_CONE_OUTER_ANGLE, 360.0f);
		qalSourcef(channel->sourceId, AL_CONE_OUTER_GAIN, 1.0f);

		qalSourcef(channel->sourceId, AL_ROLLOFF_FACTOR, 0.0f);
	}
	else {
		qalSourcef(channel->sourceId, AL_REFERENCE_DISTANCE, UNITS2METERS(channel->p.minDistance));
		qalSourcef(channel->sourceId, AL_MAX_DISTANCE, UNITS2METERS(channel->p.maxDistance));

		if (VectorIsCleared(channel->p.direction)){
			qalSourcef(channel->sourceId, AL_CONE_INNER_ANGLE, 360.0f);
			qalSourcef(channel->sourceId, AL_CONE_OUTER_ANGLE, 360.0f);
			qalSourcef(channel->sourceId, AL_CONE_OUTER_GAIN, 1.0f);
		}
		else {
			qalSourcef(channel->sourceId, AL_CONE_INNER_ANGLE, soundShader->coneInnerAngle);
			qalSourcef(channel->sourceId, AL_CONE_OUTER_ANGLE, soundShader->coneOuterAngle);
			qalSourcef(channel->sourceId, AL_CONE_OUTER_GAIN, soundShader->coneOuterVolume);
		}

		qalSourcef(channel->sourceId, AL_ROLLOFF_FACTOR, soundShader->rolloffFactor);
	}

	if (alConfig.efxAvailable){
		if (!channel->p.spatialized){
			qalSourcef(channel->sourceId, AL_ROOM_ROLLOFF_FACTOR, 0.0f);
			qalSourcef(channel->sourceId, AL_AIR_ABSORPTION_FACTOR, 0.0f);
		}
		else {
			qalSourcef(channel->sourceId, AL_ROOM_ROLLOFF_FACTOR, soundShader->roomRolloffFactor);

			if (!s_airAbsorption->integerValue)
				qalSourcef(channel->sourceId, AL_AIR_ABSORPTION_FACTOR, 0.0f);
			else
				qalSourcef(channel->sourceId, AL_AIR_ABSORPTION_FACTOR, soundShader->airAbsorptionFactor);
		}

		qalSourcei(channel->sourceId, AL_DIRECT_FILTER, snd.filter.dryFilterId[channel->index]);

		if (!channel->p.feedReverb)
			qalSource3i(channel->sourceId, AL_AUXILIARY_SEND_FILTER, 0, 0, 0);
		else
			qalSource3i(channel->sourceId, AL_AUXILIARY_SEND_FILTER, snd.reverb.effectSlotId, 0, snd.filter.wetFilterId[channel->index]);
	}

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

 Take all the sound entities and begin playing them, or update them if
 already playing
 ==================
*/
void S_UpdateLoopingSounds (){

	entity_state_t	*entity;
	soundShader_t	*soundShader;
	int				i;

	for (i = 0; i < snd.numSoundEntities; i++){
		entity = snd.soundEntities[i];

		soundShader = cl.media.gameSoundShaders[entity->sound];
		if (!soundShader)
			return;

		// TODO: Play the sound..do this similar to testSound?
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
	sound_t		*sound;
	int			channels = 0;
	int			i;

	Com_Printf("Current active channels:\n");

	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		if (channel->state == CS_FREE)
			continue;

		channels++;

		Com_Printf("%3i ", i+1);

		if (channel->emitter == snd.localEmitter)
			Com_Printf("(L)");
		else
			Com_Printf("(W)");

		sound = S_CurrentSound(channel, channel->soundShader);
		if (sound)
			Com_Printf(": %s\n", sound->name);
		else
			Com_Printf(": %s\n", "<STOPPED>");
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