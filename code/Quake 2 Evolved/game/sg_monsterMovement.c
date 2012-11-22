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
// sg_monsterMovement.c - Monster movement behaviour
//


#include "g_local.h"


#define	STEP_SIZE						18.0f
#define	NO_DIRECTION					-1.0f

static int				c_yes, c_no;


/*
 ==============================================================================

 MONSTER MOVEMENT

 ==============================================================================
*/


/*
 ==================
 SG_MonsterCheckBottom

 Returns false if any part of the bottom of the entity is off an edge that
 is not a staircase
 ==================
*/
qboolean SG_MonsterCheckBottom (edict_t *entity){

	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;
	
	VectorAdd(entity->s.origin, entity->mins, mins);
	VectorAdd(entity->s.origin, entity->maxs, maxs);

// If all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1.0f;

	for	(x = 0; x <= 1; x++)
		for	(y = 0; y <= 1; y++){
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];

			if (gi.pointcontents(start) != CONTENTS_SOLID)
				goto realCheck;
		}

	c_yes++;
	return true;	// We got out easy

realCheck:
	c_no++;

// Check it for real...
	start[2] = mins[2];
	
// The midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0]) * 0.5f;
	start[1] = stop[1] = (mins[1] + maxs[1]) * 0.5f;
	stop[2] = start[2] - 2.0f * STEP_SIZE;

	trace = gi.trace(start, vec3_origin, vec3_origin, stop, entity, MASK_MONSTERSOLID);

	if (trace.fraction == 1.0f)
		return false;

	mid = bottom = trace.endpos[2];
	
// The corners must be within 16 of the midpoint	
	for	(x = 0; x <= 1; x++)
		for	(y = 0; y <= 1; y++){
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];
			
			trace = gi.trace(start, vec3_origin, vec3_origin, stop, entity, MASK_MONSTERSOLID);
			
			if (trace.fraction != 1.0f && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];

			if (trace.fraction == 1.0f || mid - trace.endpos[2] > STEP_SIZE)
				return false;
		}

	c_yes++;
	return true;
}

/*
 ==================
 SV_Footstep

 Called by monster program code.
 The move will be adjusted for slopes and stairs, but if the move isn't
 possible, no move is done, false is returned, and
 pr_global_struct->trace_normal is set to the normal of the blocking wall

 FIXME: Since we need to test end position contents here, can we avoid doing
 it again later in catagorize position?
 ==================
*/
static qboolean SV_Footstep (edict_t *entity, vec3_t move, qboolean relink){

	vec3_t	oldOrigin, newOrigin, end;
	float	dz;
	trace_t	trace;
	int		i;
	float	stepSize;
	vec3_t	test;
	int		contents;

	// Try the move	
	VectorCopy(entity->s.origin, oldOrigin);
	VectorAdd(entity->s.origin, move, newOrigin);

	// Flying monsters don't step up
	if (entity->flags & (FL_SWIM | FL_FLY)){
		// Try one move with vertical motion, then one without
		for (i = 0; i < 2; i++){
			VectorAdd(entity->s.origin, move, newOrigin);

			if (i == 0 && entity->enemy){
				if (!entity->goalEntity)
					entity->goalEntity = entity->enemy;

				dz = entity->s.origin[2] - entity->goalEntity->s.origin[2];

				if (entity->goalEntity->client){
					if (dz > 40.0f)
						newOrigin[2] -= 8.0f;
					if (!((entity->flags & FL_SWIM) && (entity->waterLevel < WATERLEVEL_WAIST))){
						if (dz < 30.0f)
							newOrigin[2] += 8.0f;
					}
				}
				else {
					if (dz > 8.0f)
						newOrigin[2] -= 8.0f;
					else if (dz > 0.0f)
						newOrigin[2] -= dz;
					else if (dz < -8.0f)
						newOrigin[2] += 8.0f;
					else
						newOrigin[2] += dz;
				}
			}

			trace = gi.trace(entity->s.origin, entity->mins, entity->maxs, newOrigin, entity, MASK_MONSTERSOLID);

			// Flying monsters don't enter water voluntarily
			if (entity->flags & FL_FLY){
				if (!entity->waterLevel){
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + entity->mins[2] + 1.0f;

					contents = gi.pointcontents(test);

					if (contents & MASK_WATER)
						return false;
				}
			}

			// Swiming monsters don't exit water voluntarily
			if (entity->flags & FL_SWIM){
				if (entity->waterLevel < WATERLEVEL_WAIST){
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + entity->mins[2] + 1.0f;

					contents = gi.pointcontents(test);

					if (!(contents & MASK_WATER))
						return false;
				}
			}

			if (trace.fraction == 1.0f){
				VectorCopy(trace.endpos, entity->s.origin);

				if (relink){
					gi.linkentity(entity);
					SG_TouchTriggers(entity);
				}

				return true;
			}

			if (!entity->enemy)
				break;
		}

		return false;
	}

	// Push down from a step height above the wished position
	if (!(entity->monsterinfo.aiflags & AI_NOSTEP))
		stepSize = STEP_SIZE;
	else
		stepSize = 1.0f;

	newOrigin[2] += stepSize;
	VectorCopy(newOrigin, end);
	end[2] -= stepSize * 2.0f;

	trace = gi.trace(newOrigin, entity->mins, entity->maxs, end, entity, MASK_MONSTERSOLID);

	if (trace.allsolid)
		return false;

	if (trace.startsolid){
		newOrigin[2] -= stepSize;

		trace = gi.trace(newOrigin, entity->mins, entity->maxs, end, entity, MASK_MONSTERSOLID);

		if (trace.allsolid || trace.startsolid)
			return false;
	}

	// Don't go in to water
	if (entity->waterLevel == WATERLEVEL_NONE){
		test[0] = trace.endpos[0];
		test[1] = trace.endpos[1];
		test[2] = trace.endpos[2] + entity->mins[2] + 1.0f;

		contents = gi.pointcontents(test);

		if (contents & MASK_WATER)
			return false;
	}

	if (trace.fraction == 1.0f){
		// If monster had the ground pulled out, go ahead and fall
		if (entity->flags & FL_PARTIALGROUND){
			VectorAdd(entity->s.origin, move, entity->s.origin);

			if (relink){
				gi.linkentity(entity);
				SG_TouchTriggers(entity);
			}

			entity->groundentity = NULL;

			return true;
		}

		// Walked off an edge
		return false;
	}

	// Check point traces down for dangling corners
	VectorCopy(trace.endpos, entity->s.origin);

	if (!SG_MonsterCheckBottom(entity)){
		if (entity->flags & FL_PARTIALGROUND){
			// The entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink){
				gi.linkentity(entity);
				SG_TouchTriggers(entity);
			}

			return true;
		}

		VectorCopy(oldOrigin, entity->s.origin);
		return false;
	}

	if (entity->flags & FL_PARTIALGROUND)
		entity->flags &= ~FL_PARTIALGROUND;

	entity->groundentity = trace.ent;
	entity->groundentity_linkcount = trace.ent->linkcount;

	// The move is ok
	if (relink){
		gi.linkentity(entity);
		SG_TouchTriggers(entity);
	}

	return true;
}

/*
 ==================
 SG_MonsterMove
 ==================
*/
qboolean SG_MonsterMove (edict_t *entity, float yaw, float dist){

	vec3_t	move;
	
	if (!entity->groundentity && !(entity->flags & (FL_FLY|FL_SWIM)))
		return false;

	yaw = yaw * M_PI * 2.0f / 360.0f;
	
	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0.0f;

	return SV_Footstep(entity, move, true);
}


/*
 ==============================================================================

 MONSTER GOAL

 ==============================================================================
*/


/*
 ==================
 SG_MonsterChangeYaw
 ==================
*/
void SG_MonsterChangeYaw (edict_t *entity){

	float	current;
	float	ideal;
	float	move;
	float	speed;
	
	current = anglemod(entity->s.angles[YAW]);
	ideal = entity->idealYaw;

	if (current == ideal)
		return;

	move = ideal - current;
	speed = entity->yawSpeed;

	if (ideal > current){
		if (move >= 180.0f)
			move = move - 360.0f;
	}
	else {
		if (move <= -180.0f)
			move = move + 360.0f;
	}

	if (move > 0.0f){
		if (move > speed)
			move = speed;
	}
	else {
		if (move < -speed)
			move = -speed;
	}
	
	entity->s.angles[YAW] = anglemod(current + move);
}

/*
 ==================
 SV_IsStepDirectionValid

 Turns to the movement direction, and walks the current distance if
 facing it
 ==================
*/
static qboolean SV_IsStepDirectionValid (edict_t *entity, float yaw, float dist){

	vec3_t	move, oldOrigin;
	float	delta;
	
	entity->idealYaw = yaw;
	SG_MonsterChangeYaw(entity);
	
	yaw = yaw * M_PI * 2.0f / 360.0f;
	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0.0f;

	VectorCopy(entity->s.origin, oldOrigin);

	if (SV_Footstep(entity, move, false)){
		delta = entity->s.angles[YAW] - entity->idealYaw;

		// Not turned far enough, so don't take the step
		if (delta > 45.0f && delta < 315.0f)
			VectorCopy(oldOrigin, entity->s.origin);

		gi.linkentity(entity);

		SG_TouchTriggers(entity);

		return true;
	}

	gi.linkentity(entity);

	SG_TouchTriggers(entity);

	return false;
}

/*
 ==================
 SV_FixCheckBottom
 ==================
*/
static void SV_FixCheckBottom (edict_t *entity){

	entity->flags |= FL_PARTIALGROUND;
}

/*
 ==================
 SV_IsGoalCloseEnough
 ==================
*/
static qboolean SV_IsGoalCloseEnough (edict_t *entity, edict_t *goal, float dist){

	int		i;

	for (i = 0; i < 3; i++){
		if (goal->absmin[i] > entity->absmax[i] + dist)
			return false;
		if (goal->absmax[i] < entity->absmin[i] - dist)
			return false;
	}

	return true;
}

/*
 ==================
 SV_CalculateNewChaseDirection
 ==================
*/
static void SV_CalculateNewChaseDirection (edict_t *actor, edict_t *enemy, float dist){

	float	deltaX, deltaY;
	float	dir[3];
	float	targetDir, oldTargetDir, turnAround;

	// FIXME: How did we get here with no enemy
	if (!enemy)
		return;

	oldTargetDir = anglemod((int)(actor->idealYaw / 45.0f) * 45.0f);
	turnAround = anglemod(oldTargetDir - 180.0f);

	deltaX = enemy->s.origin[0] - actor->s.origin[0];
	deltaY = enemy->s.origin[1] - actor->s.origin[1];

	if (deltaX > 10.0f)
		dir[1]= 0.0f;
	else if (deltaX < -10.0f)
		dir[1]= 180.0f;
	else
		dir[1]= NO_DIRECTION;

	if (deltaY < -10.0f)
		dir[2]= 270.0f;
	else if (deltaY > 10.0f)
		dir[2]= 90.0f;
	else
		dir[2]= NO_DIRECTION;

	// Try direct route
	if (dir[1] != NO_DIRECTION && dir[2] != NO_DIRECTION){
		if (dir[1] == 0)
			targetDir = dir[2] == 90.0f ? 45.0f : 315.0f;
		else
			targetDir = dir[2] == 90.0f ? 135.0f : 215.0f;

		if (targetDir != turnAround && SV_IsStepDirectionValid(actor, targetDir, dist))
			return;
	}

	// Try other directions
	if (((rand() & 3) & 1) || abs(deltaY) > abs(deltaX)){
		targetDir = dir[1];
		dir[1] = dir[2];
		dir[2] = targetDir;
	}

	if (dir[1] != NO_DIRECTION && dir[1] != turnAround && SV_IsStepDirectionValid(actor, dir[1], dist))
		return;

	if (dir[2] != NO_DIRECTION && dir[2] != turnAround && SV_IsStepDirectionValid(actor, dir[2], dist))
		return;

	// There is no direct path to the player, so pick another direction
	if (oldTargetDir != NO_DIRECTION && SV_IsStepDirectionValid(actor, oldTargetDir, dist))
		return;

	// Randomly determine direction of search
	if (rand() & 1){
		for (targetDir = 0.0f; targetDir <= 315.0f; targetDir += 45.0f){
			if (targetDir != turnAround && SV_IsStepDirectionValid(actor, targetDir, dist))
				return;
		}
	}
	else {
		for (targetDir = 315.0f; targetDir >= 0.0f; targetDir -= 45.0f){
			if (targetDir != turnAround && SV_IsStepDirectionValid(actor, targetDir, dist))
				return;
		}
	}

	if (turnAround != NO_DIRECTION && SV_IsStepDirectionValid(actor, turnAround, dist))
		return;

	// Can't move
	actor->idealYaw = oldTargetDir;

	// If a bridge was pulled out from underneath a monster, it may not have
	// a valid standing position at all
	if (!SG_MonsterCheckBottom(actor))
		SV_FixCheckBottom(actor);
}

/*
 ==================
 SG_MonsterMoveToGoal
 ==================
*/
void SG_MonsterMoveToGoal (edict_t *entity, float dist){

	edict_t	*goal;

	goal = entity->goalEntity;

	if (!entity->groundentity && !(entity->flags & (FL_FLY|FL_SWIM)))
		return;

	// If the next step hits the enemy, return immediately
	if (entity->enemy && SV_IsGoalCloseEnough(entity, entity->enemy, dist))
		return;

	// Bump around...
	if ((rand() & 3) == 1 || !SV_IsStepDirectionValid(entity, entity->idealYaw, dist)){
		if (entity->inuse)
			SV_CalculateNewChaseDirection(entity, goal, dist);
	}
}