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
// sg_clientTrail.c - Client trail used by monsters for path finding
//

// TODO:
// - PlayerTrail_Init


#include "g_local.h"


/*
 ==============================================================================

 CLIENT TRAIL

 This is a circular list containing the a list of points of where
 the player has been recently.  It is used by monsters for pursuit.

 .origin	The spot
 .owner		Forward link
 .aiment	Backward link
 ==============================================================================
*/

#define	TRAIL_LENGTH					8

#define NEXT(n)							(((n) + 1) & (TRAIL_LENGTH - 1))
#define PREV(n)							(((n) - 1) & (TRAIL_LENGTH - 1))

edict_t *				trail[TRAIL_LENGTH];
int						trailHead;
qboolean				trailActive = false;


/*
 ==============================================================================

 INITIALIZATION

 ==============================================================================
*/


/*
 ==================
 SG_InitClientTrail
 ==================
*/
void SG_InitClientTrail (){

	int		i;

	// FIXME: COOP
	if (deathmatch->value)
		return;

	for (i = 0; i < TRAIL_LENGTH; i++){
		trail[i] = SG_AllocEntity();
		trail[i]->className = "player_trail";
	}

	trailHead = 0;
	trailActive = true;
}

/*
 ==================
 SG_AddClientTrail
 ==================
*/
void SG_AddClientTrail (vec3_t spot){

	vec3_t	temp;

	if (!trailActive)
		return;

	VectorCopy(spot, trail[trailHead]->s.origin);

	trail[trailHead]->timestamp = level.time;

	VectorSubtract(spot, trail[PREV(trailHead)]->s.origin, temp);
	trail[trailHead]->s.angles[1] = SG_VectorToYaw(temp);

	trailHead = NEXT(trailHead);
}


/*
 ==============================================================================

 TRAIL PICKING

 ==============================================================================
*/


/*
 ==================
 SG_ClientTrail_PickFirst
 ==================
*/
edict_t *SG_ClientTrail_PickFirst (edict_t *self){

	int		marker;
	int		i;

	if (!trailActive)
		return NULL;

	for (marker = trailHead, i = TRAIL_LENGTH; i; i--){
		if (trail[marker]->timestamp <= self->monsterinfo.trail_time)
			marker = NEXT(marker);
		else
			break;
	}

	if (SG_IsEntityVisible(self, trail[marker]))
		return trail[marker];

	if (SG_IsEntityVisible(self, trail[PREV(marker)]))
		return trail[PREV(marker)];

	return trail[marker];
}

/*
 ==================
 SG_ClientTrail_PickNext
 ==================
*/
edict_t *SG_ClientTrail_PickNext (edict_t *self){

	int		marker;
	int		i;

	if (!trailActive)
		return NULL;

	for (marker = trailHead, i = TRAIL_LENGTH; i; i--){
		if (trail[marker]->timestamp <= self->monsterinfo.trail_time)
			marker = NEXT(marker);
		else
			break;
	}

	return trail[marker];
}

/*
 ==================
 SG_ClientTrail_LastSpot
 ==================
*/
edict_t *SG_ClientTrail_LastSpot (){

	return trail[PREV(trailHead)];
}