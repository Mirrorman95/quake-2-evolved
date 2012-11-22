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
 
 ==================
*/
float S_ShakeAmplitudeForListener (){

	if (s_skipShakes->integerValue)
		return 0.0f;

	return 0.0f;
}

/*
 ==================
 S_UpdateListener

 TODO: add AL_VELOCITY ?
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