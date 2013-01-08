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
// s_listener.c - Listener code
//


#include "s_local.h"


/*
 ==================
 S_PlaceListener
 ==================
*/
void S_PlaceListener (const soundListener_t *soundListener){

	// If the current area has changed, edit the reverb parameters
	if (snd.listener.area != soundListener->area)
		S_EditAreaReverb(soundListener->area);

	// Update listener
	snd.listener = *soundListener;
}

/*
 ==================
 S_ShakeAmplitudeForListener
 ==================
*/
float S_ShakeAmplitudeForListener (){

	channel_t		*channel;
	soundShader_t	*soundShader;
	float			distance, attenuation;
	float			angle, scale;
	float			amplitude, shakeAmplitude;
	int				i;

	if (s_skipShakes->integerValue)
		return 0.0f;

	// Accumulate the shake amplitude of all active sounds with shakes
	shakeAmplitude = 0.0f;

	for (i = 0, channel = snd.channels; i < snd.numChannels; i++, channel++){
		if (channel->state == CS_FREE)
			continue;

		if (channel->emitter == snd.localEmitter)
			continue;		// Local sound

		if (!channel->soundShader->shakes)
			continue;		// No shakes

		if (!channel->p.reachable)
			continue;		// Unreachable

		soundShader = channel->soundShader;

		// Scale the base amplitude by the computed volume
		amplitude = channel->amplitude * channel->p.volume;

		// If low-pass filters are enabled, scale the amplitude by the computed
		// direct path's attenuation level. Otherwise the computed volume has
		// already been scaled, so don't rescale.
		if (snd.filter.enabled)
			amplitude *= channel->p.dryFilter.gain;

		// If not spatialized, just add to the shake amplitude
		if (!channel->p.spatialized){
			shakeAmplitude += soundShader->shakes * amplitude;
			continue;
		}

		// Compute distance attenuation if necessary and scale the amplitude
		if (channel->p.minDistance < channel->p.maxDistance && soundShader->rolloffFactor != 0.0f){
			distance = ClampFloat(channel->p.distToListener, channel->p.minDistance, channel->p.maxDistance);
			attenuation = 1.0f - soundShader->rolloffFactor * (distance - channel->p.minDistance) / (channel->p.maxDistance - channel->p.minDistance);

			amplitude *= ClampFloat(attenuation, 0.0f, 1.0f);
		}

		// Compute directional cones if necessary and scale the amplitude
		if (soundShader->coneInnerAngle < soundShader->coneOuterAngle && soundShader->coneOuterVolume != 1.0f){
			if (!VectorIsCleared(channel->p.direction)){
				angle = RAD2DEG(ACos(DotProduct(channel->p.dirToListener, channel->p.direction)));
				angle = ClampFloat(angle, soundShader->coneInnerAngle, soundShader->coneOuterAngle);

				scale = 1.0f + (soundShader->coneOuterVolume - 1.0f) * (angle - soundShader->coneInnerAngle) / (soundShader->coneOuterAngle - soundShader->coneInnerAngle);

				amplitude *= ClampFloat(scale, 0.0f, 1.0f);
			}
		}

		// Add to the shake amplitude
		shakeAmplitude += soundShader->shakes * amplitude;
	}

	return ClampFloat(shakeAmplitude, 0.0f, 1.0f);
}

/*
 ==================
 S_UpdateListener
 ==================
*/
void S_UpdateListener (){

	float	position[3];
	float	orientation[6];

	QAL_LogPrintf("----- S_UpdateListener -----\n");

	// If the client is not active, just set master volume
	if (cls.state != CA_ACTIVE){
		if (!snd.active)
			qalListenerf(AL_GAIN, 0.0f);
		else
			qalListenerf(AL_GAIN, s_masterVolume->floatValue);

		// Check for errors
		if (!s_ignoreALErrors->integerValue)
			S_CheckForErrors();

		QAL_LogPrintf("--------------------\n");

		return;
	}

	// Transform position
	position[0] = snd.listener.origin[1];
	position[1] = snd.listener.origin[2];
	position[2] = snd.listener.origin[0];

	// Transform orientation
	orientation[0] = -snd.listener.axis[0][1];
	orientation[1] = snd.listener.axis[0][2];
	orientation[2] = -snd.listener.axis[0][0];
	orientation[3] = -snd.listener.axis[2][1];
	orientation[4] = snd.listener.axis[2][2];
	orientation[5] = -snd.listener.axis[2][0];

	// Update listener parameters
	qalListenerfv(AL_POSITION, position);
	qalListenerfv(AL_ORIENTATION, orientation);

	if (!snd.active)
		qalListenerf(AL_GAIN, 0.0f);
	else
		qalListenerf(AL_GAIN, s_masterVolume->floatValue);

	// Check for errors
	if (!s_ignoreALErrors->integerValue)
		S_CheckForErrors();

	QAL_LogPrintf("--------------------\n");
}