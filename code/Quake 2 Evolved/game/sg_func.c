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
// sg_func.c - Platform functions used in the map editor
//

// TODO:
// - Double check func_door_touch crap


#include "g_local.h"


#define PLAT_LOW_TRIGGER				1

typedef enum {
	STATE_TOP,
	STATE_BOTTOM,
	STATE_UP,
	STATE_DOWN
} moverState_t;

#define DOOR_START_OPEN					1
#define DOOR_REVERSE					2
#define DOOR_CRUSHER					4
#define DOOR_NOMONSTER					8
#define DOOR_TOGGLE						32
#define DOOR_X_AXIS						64
#define DOOR_Y_AXIS						128

#define AccelerationDistance(target, rate)	(target * ((target / rate) + 1) / 2)

void SG_PlatAccelerateMovement_Think (edict_t *entity);
void SG_PlatGoDown (edict_t *entity);


/*
 ==============================================================================

 MOVEMENT

 Changes in origin using velocity

 ==============================================================================
*/


/*
 ==================
 SG_MovementDone
 ==================
*/
static void SG_MovementDone (edict_t *entity){

	VectorClear(entity->velocity);
	entity->moveInfo.endfunc(entity);
}

/*
 ==================
 SG_MovementFinal
 ==================
*/
static void SG_MovementFinal (edict_t *entity){

	if (entity->moveInfo.remaining_distance == 0.0f){
		SG_MovementDone(entity);
		return;
	}

	VectorScale(entity->moveInfo.dir, entity->moveInfo.remaining_distance / FRAMETIME, entity->velocity);

	entity->think = SG_MovementDone;
	entity->nextthink = level.time + FRAMETIME;
}

/*
 ==================
 SG_MovementBegin
 ==================
*/
static void SG_MovementBegin (edict_t *entity){

	float	frames;

	if ((entity->moveInfo.speed * FRAMETIME) >= entity->moveInfo.remaining_distance){
		SG_MovementFinal(entity);
		return;
	}

	VectorScale(entity->moveInfo.dir, entity->moveInfo.speed, entity->velocity);
	frames = floor((entity->moveInfo.remaining_distance / entity->moveInfo.speed) / FRAMETIME);
	entity->moveInfo.remaining_distance -= frames * entity->moveInfo.speed * FRAMETIME;
	entity->nextthink = level.time + (frames * FRAMETIME);
	entity->think = SG_MovementFinal;
}

/*
 ==================
 SG_MovementCalculate
 ==================
*/
static void SG_MovementCalculate (edict_t *entity, vec3_t dest, void (*func)(edict_t *)){

	VectorClear(entity->velocity);
	VectorSubtract(dest, entity->s.origin, entity->moveInfo.dir);
	entity->moveInfo.remaining_distance = VectorNormalize (entity->moveInfo.dir);
	entity->moveInfo.endfunc = func;

	if (entity->moveInfo.speed == entity->moveInfo.accel && entity->moveInfo.speed == entity->moveInfo.decel){
		if (level.current_entity == ((entity->flags & FL_TEAMSLAVE) ? entity->teammaster : entity))
			SG_MovementBegin(entity);
		else {
			entity->nextthink = level.time + FRAMETIME;
			entity->think = SG_MovementBegin;
		}
	}
	else {
		// Accelerative
		entity->moveInfo.current_speed = 0.0f;
		entity->think = SG_PlatAccelerateMovement_Think;
		entity->nextthink = level.time + FRAMETIME;
	}
}


/*
 ==============================================================================

 ANGULAR MOVEMENT

 Changes in angle using avelocity

 ==============================================================================
*/


/*
 ==================
 SG_AngularMovementDone
 ==================
*/
static void SG_AngularMovementDone (edict_t *entity){

	VectorClear(entity->avelocity);
	entity->moveInfo.endfunc(entity);
}

/*
 ==================
 SG_AngularMovementFinal
 ==================
*/
static void SG_AngularMovementFinal (edict_t *entity){

	vec3_t	move;

	if (entity->moveInfo.state == STATE_UP)
		VectorSubtract(entity->moveInfo.end_angles, entity->s.angles, move);
	else
		VectorSubtract(entity->moveInfo.start_angles, entity->s.angles, move);

	if (VectorCompare (move, vec3_origin)){
		SG_AngularMovementDone(entity);
		return;
	}

	VectorScale(move, 1.0f / FRAMETIME, entity->avelocity);

	entity->think = SG_AngularMovementDone;
	entity->nextthink = level.time + FRAMETIME;
}

/*
 ==================
 SG_AngularMovementBegin
 ==================
*/
static void SG_AngularMovementBegin (edict_t *entity){

	vec3_t	destdelta;
	float	len;
	float	traveltime;
	float	frames;

	// set destdelta to the vector needed to move
	if (entity->moveInfo.state == STATE_UP)
		VectorSubtract(entity->moveInfo.end_angles, entity->s.angles, destdelta);
	else
		VectorSubtract(entity->moveInfo.start_angles, entity->s.angles, destdelta);
	
	// calculate length of vector
	len = VectorLength (destdelta);
	
	// divide by speed to get time to reach dest
	traveltime = len / entity->moveInfo.speed;

	if (traveltime < FRAMETIME){
		SG_AngularMovementFinal(entity);
		return;
	}

	frames = floor(traveltime / FRAMETIME);

	// scale the destdelta vector by the time spent traveling to get velocity
	VectorScale(destdelta, 1.0f / traveltime, entity->avelocity);

	// set nextthink to trigger a think when dest is reached
	entity->nextthink = level.time + frames * FRAMETIME;
	entity->think = SG_AngularMovementFinal;
}

/*
 ==================
 SG_AngularMovementCalculate
 ==================
*/
static void SG_AngularMovementCalculate (edict_t *entity, void (*func)(edict_t *)){

	VectorClear(entity->avelocity);

	entity->moveInfo.endfunc = func;

	if (level.current_entity == ((entity->flags & FL_TEAMSLAVE) ? entity->teammaster : entity))
		SG_AngularMovementBegin(entity);
	else {
		entity->nextthink = level.time + FRAMETIME;
		entity->think = SG_AngularMovementBegin;
	}
}


/*
 ==============================================================================

 PLATS

 Movement options:

 linear
 smooth start, hard stop
 smooth start, smooth stop

 start
 end
 acceleration
 speed
 deceleration
 begin sound
 end sound
 target fired when reaching end
 wait at end

 Object characteristics that use move segments
 ---------------------------------------------
 movetype_push, or movetype_stop
 action when touched
 action when blocked
 action when used
	disabled?
 auto trigger spawning

 ==============================================================================
*/


/*
 ==================
 SG_PlatCalculateAcceleratedMovement
 ==================
*/
static void SG_PlatCalculateAcceleratedMovement (moveInfo_t *moveInfo){

	float	accel_dist;
	float	decel_dist;
	float	f;

	moveInfo->move_speed = moveInfo->speed;

	if (moveInfo->remaining_distance < moveInfo->accel){
		moveInfo->current_speed = moveInfo->remaining_distance;
		return;
	}

	accel_dist = AccelerationDistance(moveInfo->speed, moveInfo->accel);
	decel_dist = AccelerationDistance(moveInfo->speed, moveInfo->decel);

	if ((moveInfo->remaining_distance - accel_dist - decel_dist) < 0.0f){
		f = (moveInfo->accel + moveInfo->decel) / (moveInfo->accel * moveInfo->decel);
		moveInfo->move_speed = (-2.0f + sqrt(4.0f - 4.0f * f * (-2.0f * moveInfo->remaining_distance))) / (2.0f * f);
		decel_dist = AccelerationDistance(moveInfo->move_speed, moveInfo->decel);
	}

	moveInfo->decel_distance = decel_dist;
}

/*
 ==================
 SG_PlatAccelerateMovement
 ==================
*/
static void SG_PlatAccelerateMovement (moveInfo_t *moveInfo){

	float	p1_distance;
	float	p2_distance;
	float	distance;
	float	old_speed;	
	float	p1_speed;
		
	// Are we decelerating?
	if (moveInfo->remaining_distance <= moveInfo->decel_distance){
		if (moveInfo->remaining_distance < moveInfo->decel_distance){
			if (moveInfo->next_speed){
				moveInfo->current_speed = moveInfo->next_speed;
				moveInfo->next_speed = 0.0f;
				return;
			}

			if (moveInfo->current_speed > moveInfo->decel)
				moveInfo->current_speed -= moveInfo->decel;
		}

		return;
	}

	// Are we at full speed and need to start decelerating during this move?
	if (moveInfo->current_speed == moveInfo->move_speed)
		if ((moveInfo->remaining_distance - moveInfo->current_speed) < moveInfo->decel_distance){
			p1_distance = moveInfo->remaining_distance - moveInfo->decel_distance;
			p2_distance = moveInfo->move_speed * (1.0f - (p1_distance / moveInfo->move_speed));
			distance = p1_distance + p2_distance;
			moveInfo->current_speed = moveInfo->move_speed;
			moveInfo->next_speed = moveInfo->move_speed - moveInfo->decel * (p2_distance / distance);
			return;
		}

	// Are we accelerating?
	if (moveInfo->current_speed < moveInfo->speed){
		old_speed = moveInfo->current_speed;

		// figure simple acceleration up to move_speed
		moveInfo->current_speed += moveInfo->accel;
		if (moveInfo->current_speed > moveInfo->speed)
			moveInfo->current_speed = moveInfo->speed;

		// Are we accelerating throughout this entire move?
		if ((moveInfo->remaining_distance - moveInfo->current_speed) >= moveInfo->decel_distance)
			return;

		// During this move we will accelrate from current_speed to move_speed
		// and cross over the decel_distance; figure the average speed for the
		// entire move
		p1_distance = moveInfo->remaining_distance - moveInfo->decel_distance;
		p1_speed = (old_speed + moveInfo->move_speed) / 2.0f;
		p2_distance = moveInfo->move_speed * (1.0f - (p1_distance / p1_speed));
		distance = p1_distance + p2_distance;
		moveInfo->current_speed = (p1_speed * (p1_distance / distance)) + (moveInfo->move_speed * (p2_distance / distance));
		moveInfo->next_speed = moveInfo->move_speed - moveInfo->decel * (p2_distance / distance);
		return;
	}

	// We are at constant velocity (move_speed)
	return;
}

/*
 ==================
 SG_PlatAccelerateMovement_Think
 ==================
*/
static void SG_PlatAccelerateMovement_Think (edict_t *entity){

	entity->moveInfo.remaining_distance -= entity->moveInfo.current_speed;

	// Starting or blocked
	if (entity->moveInfo.current_speed == 0.0f)
		SG_PlatCalculateAcceleratedMovement(&entity->moveInfo);

	SG_PlatAccelerateMovement(&entity->moveInfo);

	// Will the entire move complete on next frame?
	if (entity->moveInfo.remaining_distance <= entity->moveInfo.current_speed){
		SG_MovementFinal(entity);
		return;
	}

	VectorScale(entity->moveInfo.dir, entity->moveInfo.current_speed * 10.0f, entity->velocity);
	entity->nextthink = level.time + FRAMETIME;
	entity->think = SG_PlatAccelerateMovement_Think;
}

/*
 ==================
 SG_PlatHitTop
 ==================
*/
static void SG_PlatHitTop (edict_t *entity){

	if (!(entity->flags & FL_TEAMSLAVE)){
		if (entity->moveInfo.sound_end)
			gi.sound(entity, CHAN_NO_PHS_ADD + CHAN_VOICE, entity->moveInfo.sound_end, 1.0f, ATTN_STATIC, 0.0f);

		entity->s.sound = 0;
	}

	entity->moveInfo.state = STATE_TOP;

	entity->think = SG_PlatGoDown;
	entity->nextthink = level.time + 3.0f;
}

/*
 ==================
 SG_PlatHitBottom
 ==================
*/
static void SG_PlatHitBottom (edict_t *entity){

	if (!(entity->flags & FL_TEAMSLAVE)){
		if (entity->moveInfo.sound_end)
			gi.sound(entity, CHAN_NO_PHS_ADD + CHAN_VOICE, entity->moveInfo.sound_end, 1.0f, ATTN_STATIC, 0.0f);

		entity->s.sound = 0;
	}

	entity->moveInfo.state = STATE_BOTTOM;
}

/*
 ==================
 SG_PlatGoDown
 ==================
*/
static void SG_PlatGoDown (edict_t *entity){

	if (!(entity->flags & FL_TEAMSLAVE)){
		if (entity->moveInfo.sound_start)
			gi.sound(entity, CHAN_NO_PHS_ADD + CHAN_VOICE, entity->moveInfo.sound_start, 1.0f, ATTN_STATIC, 0.0f);

		entity->s.sound = entity->moveInfo.sound_middle;
	}

	entity->moveInfo.state = STATE_DOWN;
	SG_MovementCalculate(entity, entity->moveInfo.end_origin, SG_PlatHitBottom);
}

/*
 ==================
 SG_PlatGoUp
 ==================
*/
static void SG_PlatGoUp (edict_t *entity){

	if (!(entity->flags & FL_TEAMSLAVE)){
		if (entity->moveInfo.sound_start)
			gi.sound(entity, CHAN_NO_PHS_ADD + CHAN_VOICE, entity->moveInfo.sound_start, 1.0f, ATTN_STATIC, 0.0f);

		entity->s.sound = entity->moveInfo.sound_middle;
	}

	entity->moveInfo.state = STATE_UP;
	SG_MovementCalculate(entity, entity->moveInfo.start_origin, SG_PlatHitTop);
}

/*
 ==================
 SG_PlatBlocked
 ==================
*/
static void SG_PlatBlocked (edict_t *self, edict_t *other){

	if (!(other->svflags & SVF_MONSTER) && (!other->client)){
		// Give it a chance to go away on it's own terms (like gibs)
		SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);

		// If it's still there, nuke it
		if (other)
			SG_BecomeExplosion1(other);

		return;
	}

	SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);

	if (self->moveInfo.state == STATE_UP)
		SG_PlatGoDown(self);
	else if (self->moveInfo.state == STATE_DOWN)
		SG_PlatGoUp(self);
}

/*
 ==================
 SG_Plat_Trigger
 ==================
*/
static void SG_Plat_Trigger (edict_t *entity, edict_t *other, edict_t *activator){ 

	if (entity->think)
		return;		// Already down

	SG_PlatGoDown(entity);
}

/*
 ==================
 SG_PlatCenter_Touch
 ==================
*/
static void SG_PlatCenter_Touch (edict_t *entity, edict_t *other, cplane_t *plane, csurface_t *surface){

	if (!other->client)
		return;
		
	if (other->health <= 0)
		return;

	// Now point at the plat, not the trigger
	entity = entity->enemy;

	if (entity->moveInfo.state == STATE_BOTTOM)
		SG_PlatGoUp(entity);
	else if (entity->moveInfo.state == STATE_TOP)
		entity->nextthink = level.time + 1.0f;		// The player is still on the plat, so delay going down
}

/*
 ==================
 SG_PlatInsideTrigger_Spawn
 ==================
*/
static void SG_PlatInsideTrigger_Spawn (edict_t *entity){

	edict_t	*trigger;
	vec3_t	tmin, tmax;

	// Middle trigger	
	trigger = SG_AllocEntity();
	trigger->touch = SG_PlatCenter_Touch;
	trigger->movetype = MOVETYPE_NONE;
	trigger->solid = SOLID_TRIGGER;
	trigger->enemy = entity;

	tmin[0] = entity->mins[0] + 25.0f;
	tmin[1] = entity->mins[1] + 25.0f;
	tmin[2] = entity->mins[2];

	tmax[0] = entity->maxs[0] - 25.0f;
	tmax[1] = entity->maxs[1] - 25.0f;
	tmax[2] = entity->maxs[2] + 8.0f;

	tmin[2] = tmax[2] - (entity->pos1[2] - entity->pos2[2] + st.lip);

	if (entity->spawnflags & PLAT_LOW_TRIGGER)
		tmax[2] = tmin[2] + 8.0f;

	if (tmax[0] - tmin[0] <= 0){
		tmin[0] = (entity->mins[0] + entity->maxs[0]) * 0.5f;
		tmax[0] = tmin[0] + 1.0f;
	}

	if (tmax[1] - tmin[1] <= 0.0f){
		tmin[1] = (entity->mins[1] + entity->maxs[1]) * 0.5f;
		tmax[1] = tmin[1] + 1.0f;
	}

	VectorCopy(tmin, trigger->mins);
	VectorCopy(tmax, trigger->maxs);

	gi.linkentity(trigger);
}


/*
 ==============================================================================

 FUNC_PLAT

 ==============================================================================
*/


/*
 ==================
 SG_FuncPlat_Spawn
 ==================
*/
void SG_FuncPlat_Spawn (edict_t *entity){

	VectorClear(entity->s.angles);
	entity->solid = SOLID_BSP;
	entity->movetype = MOVETYPE_PUSH;

	gi.setmodel(entity, entity->model);

	entity->blocked = SG_PlatBlocked;

	if (!entity->speed)
		entity->speed = 20.0f;
	else
		entity->speed *= 0.1f;

	if (!entity->accel)
		entity->accel = 5.0f;
	else
		entity->accel *= 0.1f;

	if (!entity->decel)
		entity->decel = 5.0f;
	else
		entity->decel *= 0.1f;

	if (!entity->dmg)
		entity->dmg = 2;

	if (!st.lip)
		st.lip = 8;

	// Pos1 is the top position, pos2 is the bottom
	VectorCopy(entity->s.origin, entity->pos1);
	VectorCopy(entity->s.origin, entity->pos2);

	if (st.height)
		entity->pos2[2] -= st.height;
	else
		entity->pos2[2] -= (entity->maxs[2] - entity->mins[2]) - st.lip;

	entity->use = SG_Plat_Trigger;

	// The "start moving" trigger	
	SG_PlatInsideTrigger_Spawn(entity);

	if (entity->targetname)
		entity->moveInfo.state = STATE_UP;
	else {
		VectorCopy(entity->pos2, entity->s.origin);
		gi.linkentity(entity);
		entity->moveInfo.state = STATE_BOTTOM;
	}

	entity->moveInfo.speed = entity->speed;
	entity->moveInfo.accel = entity->accel;
	entity->moveInfo.decel = entity->decel;
	entity->moveInfo.wait = entity->wait;
	VectorCopy(entity->pos1, entity->moveInfo.start_origin);
	VectorCopy(entity->s.angles, entity->moveInfo.start_angles);
	VectorCopy(entity->pos2, entity->moveInfo.end_origin);
	VectorCopy(entity->s.angles, entity->moveInfo.end_angles);

	entity->moveInfo.sound_start = gi.soundindex("plats/pt1_strt.wav");
	entity->moveInfo.sound_middle = gi.soundindex("plats/pt1_mid.wav");
	entity->moveInfo.sound_end = gi.soundindex("plats/pt1_end.wav");
}


/*
 ==============================================================================

 FUNC_ROTATING

 ==============================================================================
*/


/*
 ==================
 SG_FuncRotating_Blocked
 ==================
*/
static void SG_FuncRotating_Blocked (edict_t *self, edict_t *other){

	SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

/*
 ==================
 SG_FuncRotating_Touch
 ==================
*/
static void SG_FuncRotating_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	if (self->avelocity[0] || self->avelocity[1] || self->avelocity[2])
		SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

/*
 ==================
 SG_FuncRotating_Trigger
 ==================
*/
static void SG_FuncRotating_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	if (!VectorCompare (self->avelocity, vec3_origin)){
		self->s.sound = 0;
		VectorClear(self->avelocity);
		self->touch = NULL;
	}
	else {
		self->s.sound = self->moveInfo.sound_middle;
		VectorScale(self->movedir, self->speed, self->avelocity);

		if (self->spawnflags & 16)
			self->touch = SG_FuncRotating_Touch;
	}
}

/*
 ==================
 SG_FuncRotating_Spawn
 ==================
*/
void SG_FuncRotating_Spawn (edict_t *entity){

	entity->solid = SOLID_BSP;

	if (entity->spawnflags & 32)
		entity->movetype = MOVETYPE_STOP;
	else
		entity->movetype = MOVETYPE_PUSH;

	// Set the axis of rotation
	VectorClear(entity->movedir);

	if (entity->spawnflags & 4)
		entity->movedir[2] = 1.0f;
	else if (entity->spawnflags & 8)
		entity->movedir[0] = 1.0f;
	else	// Z_AXIS
		entity->movedir[1] = 1.0f;

	// Check for reverse rotation
	if (entity->spawnflags & 2)
		VectorNegate(entity->movedir, entity->movedir);

	if (!entity->speed)
		entity->speed = 100.0f;
	if (!entity->dmg)
		entity->dmg = 2;

	entity->use = SG_FuncRotating_Trigger;

	if (entity->dmg)
		entity->blocked = SG_FuncRotating_Blocked;

	if (entity->spawnflags & 1)
		entity->use(entity, NULL, NULL);

	if (entity->spawnflags & 64)
		entity->s.effects |= EF_ANIM_ALL;
	if (entity->spawnflags & 128)
		entity->s.effects |= EF_ANIM_ALLFAST;

	gi.setmodel(entity, entity->model);
	gi.linkentity(entity);
}


/*
 ==============================================================================

 FUNC_BUTTON

 ==============================================================================
*/


/*
 ==================
 SG_FuncButton_Done
 ==================
*/
static void SG_FuncButton_Done (edict_t *self){

	self->moveInfo.state = STATE_BOTTOM;
	self->s.effects &= ~EF_ANIM23;
	self->s.effects |= EF_ANIM01;
}

/*
 ==================
 SG_FuncButton_Return
 ==================
*/
static void SG_FuncButton_Return (edict_t *self){

	self->moveInfo.state = STATE_DOWN;

	SG_MovementCalculate(self, self->moveInfo.start_origin, SG_FuncButton_Done);

	self->s.frame = 0;

	if (self->health)
		self->takedamage = DAMAGE_YES;
}

/*
 ==================
 SG_FuncButton_Wait
 ==================
*/
static void SG_FuncButton_Wait (edict_t *self){

	self->moveInfo.state = STATE_TOP;
	self->s.effects &= ~EF_ANIM01;
	self->s.effects |= EF_ANIM23;

	SG_UseTargets(self, self->activator);
	self->s.frame = 1;

	if (self->moveInfo.wait >= 0){
		self->nextthink = level.time + self->moveInfo.wait;
		self->think = SG_FuncButton_Return;
	}
}

/*
 ==================
 SG_FuncButton_Fire
 ==================
*/
static void SG_FuncButton_Fire (edict_t *self){

	if (self->moveInfo.state == STATE_UP || self->moveInfo.state == STATE_TOP)
		return;

	self->moveInfo.state = STATE_UP;

	if (self->moveInfo.sound_start && !(self->flags & FL_TEAMSLAVE))
		gi.sound(self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveInfo.sound_start, 1.0f, ATTN_STATIC, 0.0f);

	SG_MovementCalculate(self, self->moveInfo.end_origin, SG_FuncButton_Wait);
}

/*
 ==================
 SG_FuncButton_Trigger
 ==================
*/
static void SG_FuncButton_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->activator = activator;
	SG_FuncButton_Fire(self);
}

/*
 ==================
 SG_FuncButton_Touch
 ==================
*/
static void SG_FuncButton_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf){

	if (!other->client)
		return;
	if (other->health <= 0)
		return;

	self->activator = other;
	SG_FuncButton_Fire(self);
}

/*
 ==================
 SG_FuncButton_Die
 ==================
*/
static void SG_FuncButton_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	self->activator = attacker;
	self->health = self->max_health;
	self->takedamage = DAMAGE_NO;

	SG_FuncButton_Fire(self);
}

/*
 ==================
 SG_FuncButton_Spawn
 ==================
*/
void SG_FuncButton_Spawn (edict_t *entity){

	vec3_t	abs_movedir;
	float	dist;

	SG_SetMoveDirection(entity->s.angles, entity->movedir);
	entity->movetype = MOVETYPE_STOP;
	entity->solid = SOLID_BSP;
	gi.setmodel(entity, entity->model);

	if (entity->sounds != 1)
		entity->moveInfo.sound_start = gi.soundindex("switches/butn2.wav");
	
	if (!entity->speed)
		entity->speed = 40.0f;
	if (!entity->accel)
		entity->accel = entity->speed;
	if (!entity->decel)
		entity->decel = entity->speed;

	if (!entity->wait)
		entity->wait = 3.0f;
	if (!st.lip)
		st.lip = 4;

	VectorCopy(entity->s.origin, entity->pos1);
	abs_movedir[0] = fabs(entity->movedir[0]);
	abs_movedir[1] = fabs(entity->movedir[1]);
	abs_movedir[2] = fabs(entity->movedir[2]);
	dist = abs_movedir[0] * entity->size[0] + abs_movedir[1] * entity->size[1] + abs_movedir[2] * entity->size[2] - st.lip;
	VectorMA(entity->pos1, dist, entity->movedir, entity->pos2);

	entity->use = SG_FuncButton_Trigger;
	entity->s.effects |= EF_ANIM01;

	if (entity->health){
		entity->max_health = entity->health;
		entity->die = SG_FuncButton_Die;
		entity->takedamage = DAMAGE_YES;
	}
	else if (!entity->targetname)
		entity->touch = SG_FuncButton_Touch;

	entity->moveInfo.state = STATE_BOTTOM;

	entity->moveInfo.speed = entity->speed;
	entity->moveInfo.accel = entity->accel;
	entity->moveInfo.decel = entity->decel;
	entity->moveInfo.wait = entity->wait;
	VectorCopy(entity->pos1, entity->moveInfo.start_origin);
	VectorCopy(entity->s.angles, entity->moveInfo.start_angles);
	VectorCopy(entity->pos2, entity->moveInfo.end_origin);
	VectorCopy(entity->s.angles, entity->moveInfo.end_angles);

	gi.linkentity(entity);
}


/*
 ==============================================================================

 FUNC_DOOR

 ==============================================================================
*/

static void SG_FuncDoorGo_Down (edict_t *self);


/*
 ==================
 SG_FuncDoorAreaPortals
 ==================
*/
static void SG_FuncDoorAreaPortals (edict_t *self, qboolean open){

	edict_t	*t = NULL;

	if (!self->target)
		return;

	while ((t = SG_FindEntity(t, FOFS(targetname), self->target))){
		if (!Q_stricmp(t->className, "func_areaportal"))
			gi.SetAreaPortalState(t->style, open);
	}
}

/*
 ==================
 SG_FuncDoorHit_Top
 ==================
*/
static void SG_FuncDoorHit_Top (edict_t *self){

	if (!(self->flags & FL_TEAMSLAVE)){
		if (self->moveInfo.sound_end)
			gi.sound(self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveInfo.sound_end, 1.0f, ATTN_STATIC, 0.0f);

		self->s.sound = 0;
	}

	self->moveInfo.state = STATE_TOP;

	if (self->spawnflags & DOOR_TOGGLE)
		return;

	if (self->moveInfo.wait >= 0.0f){
		self->think = SG_FuncDoorGo_Down;
		self->nextthink = level.time + self->moveInfo.wait;
	}
}

/*
 ==================
 SG_FuncDoorHit_Bottom
 ==================
*/
static void SG_FuncDoorHit_Bottom (edict_t *self){

	if (!(self->flags & FL_TEAMSLAVE)){
		if (self->moveInfo.sound_end)
			gi.sound(self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveInfo.sound_end, 1.0f, ATTN_STATIC, 0.0f);

		self->s.sound = 0;
	}

	self->moveInfo.state = STATE_BOTTOM;
	SG_FuncDoorAreaPortals(self, false);
}

/*
 ==================
 SG_FuncDoorGo_Down
 ==================
*/
static void SG_FuncDoorGo_Down (edict_t *self){

	if (!(self->flags & FL_TEAMSLAVE)){
		if (self->moveInfo.sound_start)
			gi.sound(self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveInfo.sound_start, 1, ATTN_STATIC, 0);

		self->s.sound = self->moveInfo.sound_middle;
	}

	if (self->max_health){
		self->takedamage = DAMAGE_YES;
		self->health = self->max_health;
	}
	
	self->moveInfo.state = STATE_DOWN;

	if (!strcmp(self->className, "func_door"))
		SG_MovementCalculate(self, self->moveInfo.start_origin, SG_FuncDoorHit_Bottom);
	else if (!strcmp(self->className, "func_door_rotating"))
		SG_AngularMovementCalculate(self, SG_FuncDoorHit_Bottom);
}

/*
 ==================
 SG_FuncDoorGo_Up
 ==================
*/
static void SG_FuncDoorGo_Up (edict_t *self, edict_t *activator){

	if (self->moveInfo.state == STATE_UP)
		return;		// Already going up

	// Reset top wait time
	if (self->moveInfo.state == STATE_TOP){
		if (self->moveInfo.wait >= 0.0f)
			self->nextthink = level.time + self->moveInfo.wait;

		return;
	}

	if (!(self->flags & FL_TEAMSLAVE)){
		if (self->moveInfo.sound_start)
			gi.sound(self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveInfo.sound_start, 1.0f, ATTN_STATIC, 0.0f);

		self->s.sound = self->moveInfo.sound_middle;
	}

	self->moveInfo.state = STATE_UP;

	if (!strcmp(self->className, "func_door"))
		SG_MovementCalculate(self, self->moveInfo.end_origin, SG_FuncDoorHit_Top);
	else if (!strcmp(self->className, "func_door_rotating"))
		SG_AngularMovementCalculate(self, SG_FuncDoorHit_Top);

	SG_UseTargets(self, activator);
	SG_FuncDoorAreaPortals(self, true);
}

/*
 ==================
 SG_FuncDoor_Trigger
 ==================
*/
static void SG_FuncDoor_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	edict_t	*entity;

	if (self->flags & FL_TEAMSLAVE)
		return;

	if (self->spawnflags & DOOR_TOGGLE){
		if (self->moveInfo.state == STATE_UP || self->moveInfo.state == STATE_TOP){
			// Trigger all paired doors
			for (entity = self; entity; entity = entity->teamchain){
				entity->message = NULL;
				entity->touch = NULL;

				SG_FuncDoorGo_Down(entity);
			}

			return;
		}
	}

	// Trigger all paired doors
	for (entity = self; entity; entity = entity->teamchain){
		entity->message = NULL;
		entity->touch = NULL;

		SG_FuncDoorGo_Up(entity, activator);
	}
}

/*
 ==================
 SG_FuncDoor_Touch2
 ==================
*/
static void SG_FuncDoor_Touch2 (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	if (other->health <= 0)
		return;

	if (!(other->svflags & SVF_MONSTER) && (!other->client))
		return;

	if ((self->owner->spawnflags & DOOR_NOMONSTER) && (other->svflags & SVF_MONSTER))
		return;

	if (level.time < self->touch_debounce_time)
		return;

	self->touch_debounce_time = level.time + 1.0f;

	SG_FuncDoor_Trigger(self->owner, other, other);
}

/*
 ==================
 SG_FuncDoorCalculateMoveSpeed
 ==================
*/
static void SG_FuncDoorCalculateMoveSpeed (edict_t *self){

	edict_t	*entity;
	float	min;
	float	time;
	float	newspeed;
	float	ratio;
	float	dist;

	if (self->flags & FL_TEAMSLAVE)
		return;		// Only the team master does this

	// Find the smallest distance any member of the team will be moving
	min = fabs(self->moveInfo.distance);

	for (entity = self->teamchain; entity; entity = entity->teamchain){
		dist = fabs(entity->moveInfo.distance);
		if (dist < min)
			min = dist;
	}

	time = min / self->moveInfo.speed;

	// Adjust speeds so they will all complete at the same time
	for (entity = self; entity; entity = entity->teamchain){
		newspeed = fabs(entity->moveInfo.distance) / time;
		ratio = newspeed / entity->moveInfo.speed;

		if (entity->moveInfo.accel == entity->moveInfo.speed)
			entity->moveInfo.accel = newspeed;
		else
			entity->moveInfo.accel *= ratio;

		if (entity->moveInfo.decel == entity->moveInfo.speed)
			entity->moveInfo.decel = newspeed;
		else
			entity->moveInfo.decel *= ratio;

		entity->moveInfo.speed = newspeed;
	}
}

/*
 ==================
 SG_FuncDoorSpawnDoor_Trigger
 ==================
*/
static void SG_FuncDoorSpawnDoor_Trigger (edict_t *ent){

	edict_t	*other;
	vec3_t	mins, maxs;

	if (ent->flags & FL_TEAMSLAVE)
		return;		// Only the team leader spawns a trigger

	VectorCopy(ent->absmin, mins);
	VectorCopy(ent->absmax, maxs);

	for (other = ent->teamchain; other; other = other->teamchain){
		AddPointToBounds(other->absmin, mins, maxs);
		AddPointToBounds(other->absmax, mins, maxs);
	}

	// Expand 
	mins[0] -= 60.0f;
	mins[1] -= 60.0f;
	maxs[0] += 60.0f;
	maxs[1] += 60.0f;

	other = SG_AllocEntity();
	VectorCopy(mins, other->mins);
	VectorCopy(maxs, other->maxs);
	other->owner = ent;
	other->solid = SOLID_TRIGGER;
	other->movetype = MOVETYPE_NONE;
	other->touch = SG_FuncDoor_Touch2;

	gi.linkentity(other);

	if (ent->spawnflags & DOOR_START_OPEN)
		SG_FuncDoorAreaPortals(ent, true);

	SG_FuncDoorCalculateMoveSpeed(ent);
}

/*
 ==================
 SG_FuncDoor_Blocked
 ==================
*/
static void SG_FuncDoor_Blocked  (edict_t *self, edict_t *other){

	edict_t	*entity;

	if (!(other->svflags & SVF_MONSTER) && (!other->client) ){
		// Give it a chance to go away on it's own terms (like gibs)
		SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);

		// If it's still there, nuke it
		if (other)
			SG_BecomeExplosion1(other);

		return;
	}

	SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);

	if (self->spawnflags & DOOR_CRUSHER)
		return;

	// If a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast
	if (self->moveInfo.wait >= 0){
		if (self->moveInfo.state == STATE_DOWN){
			for (entity = self->teammaster; entity; entity = entity->teamchain)
				SG_FuncDoorGo_Up(entity, entity->activator);
		}
		else {
			for (entity = self->teammaster; entity; entity = entity->teamchain)
				SG_FuncDoorGo_Down(entity);
		}
	}
}

/*
 ==================
 SG_FuncDoor_Die
 ==================
*/
void SG_FuncDoor_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	edict_t	*entity;

	for (entity = self->teammaster; entity; entity = entity->teamchain){
		entity->health = entity->max_health;
		entity->takedamage = DAMAGE_NO;
	}

	SG_FuncDoor_Trigger(self->teammaster, attacker, attacker);
}

/*
 ==================
 SG_FuncDoor_Touch
 ==================
*/
static void SG_FuncDoor_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	if (!other->client)
		return;

	if (level.time < self->touch_debounce_time)
		return;

	self->touch_debounce_time = level.time + 5.0f;

	gi.centerprintf(other, "%s", self->message);
	gi.sound(other, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SG_FuncDoor_Spawn
 ==================
*/
void SG_FuncDoor_Spawn (edict_t *entity){

	vec3_t	abs_movedir;

	if (entity->sounds != 1){
		entity->moveInfo.sound_start = gi.soundindex("doors/dr1_strt.wav");
		entity->moveInfo.sound_middle = gi.soundindex("doors/dr1_mid.wav");
		entity->moveInfo.sound_end = gi.soundindex("doors/dr1_end.wav");
	}

	SG_SetMoveDirection(entity->s.angles, entity->movedir);
	entity->movetype = MOVETYPE_PUSH;
	entity->solid = SOLID_BSP;
	gi.setmodel(entity, entity->model);

	entity->blocked = SG_FuncDoor_Blocked;
	entity->use = SG_FuncDoor_Trigger;

	if (!entity->speed)
		entity->speed = 100.0f;
	if (deathmatch->value)
		entity->speed *= 2.0f;

	if (!entity->accel)
		entity->accel = entity->speed;
	if (!entity->decel)
		entity->decel = entity->speed;

	if (!entity->wait)
		entity->wait = 3.0f;
	if (!st.lip)
		st.lip = 8;
	if (!entity->dmg)
		entity->dmg = 2;

	// Calculate second position
	VectorCopy(entity->s.origin, entity->pos1);
	abs_movedir[0] = fabs(entity->movedir[0]);
	abs_movedir[1] = fabs(entity->movedir[1]);
	abs_movedir[2] = fabs(entity->movedir[2]);
	entity->moveInfo.distance = abs_movedir[0] * entity->size[0] + abs_movedir[1] * entity->size[1] + abs_movedir[2] * entity->size[2] - st.lip;
	VectorMA(entity->pos1, entity->moveInfo.distance, entity->movedir, entity->pos2);

	// If it starts open, switch the positions
	if (entity->spawnflags & DOOR_START_OPEN){
		VectorCopy(entity->pos2, entity->s.origin);
		VectorCopy(entity->pos1, entity->pos2);
		VectorCopy(entity->s.origin, entity->pos1);
	}

	entity->moveInfo.state = STATE_BOTTOM;

	if (entity->health){
		entity->takedamage = DAMAGE_YES;
		entity->die = SG_FuncDoor_Die;
		entity->max_health = entity->health;
	}
	else if (entity->targetname && entity->message){
		gi.soundindex("misc/talk.wav");
		entity->touch = SG_FuncDoor_Touch;
	}

	entity->moveInfo.speed = entity->speed;
	entity->moveInfo.accel = entity->accel;
	entity->moveInfo.decel = entity->decel;
	entity->moveInfo.wait = entity->wait;
	VectorCopy(entity->pos1, entity->moveInfo.start_origin);
	VectorCopy(entity->s.angles, entity->moveInfo.start_angles);
	VectorCopy(entity->pos2, entity->moveInfo.end_origin);
	VectorCopy(entity->s.angles, entity->moveInfo.end_angles);

	if (entity->spawnflags & 16)
		entity->s.effects |= EF_ANIM_ALL;
	if (entity->spawnflags & 64)
		entity->s.effects |= EF_ANIM_ALLFAST;

	// To simplify logic elsewhere, make non-teamed doors into a team of one
	if (!entity->team)
		entity->teammaster = entity;

	gi.linkentity(entity);

	entity->nextthink = level.time + FRAMETIME;

	if (entity->health || entity->targetname)
		entity->think = SG_FuncDoorCalculateMoveSpeed;
	else
		entity->think = SG_FuncDoorSpawnDoor_Trigger;
}


/*
 ==============================================================================

 FUNC_DOOR_SECRET

 ==============================================================================
*/

#define SECRET_ALWAYS_SHOOT	1
#define SECRET_1ST_LEFT		2
#define SECRET_1ST_DOWN		4

static void SG_FuncDoorSecret_Move1 (edict_t *self);
static void SG_FuncDoorSecret_Move2 (edict_t *self);
static void SG_FuncDoorSecret_Move3 (edict_t *self);
static void SG_FuncDoorSecret_Move4 (edict_t *self);
static void SG_FuncDoorSecret_Move5 (edict_t *self);
static void SG_FuncDoorSecret_Move6 (edict_t *self);
static void SG_FuncDoorSecret_Done (edict_t *self);


/*
 ==================
 SG_FuncDoorSecret_Trigger
 ==================
*/
static void SG_FuncDoorSecret_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	// make sure we're not already moving
	if (!VectorCompare(self->s.origin, vec3_origin))
		return;

	SG_MovementCalculate(self, self->pos1, SG_FuncDoorSecret_Move1);
	SG_FuncDoorAreaPortals(self, true);
}

/*
 ==================
 SG_FuncDoorSecret_Move1
 ==================
*/
static void SG_FuncDoorSecret_Move1 (edict_t *self){

	self->nextthink = level.time + 1.0f;
	self->think = SG_FuncDoorSecret_Move2;
}

/*
 ==================
 SG_FuncDoorSecret_Move2
 ==================
*/
static void SG_FuncDoorSecret_Move2 (edict_t *self){

	SG_MovementCalculate(self, self->pos2, SG_FuncDoorSecret_Move3);
}

/*
 ==================
 SG_FuncDoorSecret_Move3
 ==================
*/
static void SG_FuncDoorSecret_Move3 (edict_t *self){

	if (self->wait == -1.0f)
		return;

	self->nextthink = level.time + self->wait;
	self->think = SG_FuncDoorSecret_Move4;
}

/*
 ==================
 SG_FuncDoorSecret_Move4
 ==================
*/
static void SG_FuncDoorSecret_Move4 (edict_t *self){

	SG_MovementCalculate(self, self->pos1, SG_FuncDoorSecret_Move5);
}

/*
 ==================
 SG_FuncDoorSecret_Move5
 ==================
*/
static void SG_FuncDoorSecret_Move5 (edict_t *self){

	self->nextthink = level.time + 1.0f;
	self->think = SG_FuncDoorSecret_Move6;
}

/*
 ==================
 SG_FuncDoorSecret_Move6
 ==================
*/
static void SG_FuncDoorSecret_Move6 (edict_t *self){

	SG_MovementCalculate(self, vec3_origin, SG_FuncDoorSecret_Done);
}

/*
 ==================
 SG_FuncDoorSecret_Done
 ==================
*/
static void SG_FuncDoorSecret_Done (edict_t *self){

	if (!(self->targetname) || (self->spawnflags & SECRET_ALWAYS_SHOOT)){
		self->health = 0;
		self->takedamage = DAMAGE_YES;
	}

	SG_FuncDoorAreaPortals(self, false);
}

/*
 ==================
 SG_FuncDoorSecret_Blocked
 ==================
*/
static void SG_FuncDoorSecret_Blocked  (edict_t *self, edict_t *other){

	if (!(other->svflags & SVF_MONSTER) && (!other->client)){
		// Give it a chance to go away on it's own terms (like gibs)
		SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);

		// If it's still there, nuke it
		if (other)
			SG_BecomeExplosion1(other);

		return;
	}

	if (level.time < self->touch_debounce_time)
		return;

	self->touch_debounce_time = level.time + 0.5f;

	SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

/*
 ==================
 SG_FuncDoorSecret_Die
 ==================
*/
static void SG_FuncDoorSecret_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	self->takedamage = DAMAGE_NO;
	SG_FuncDoorSecret_Trigger(self, attacker, attacker);
}

/*
 ==================
 SG_FuncDoorSecret_Spawn
 ==================
*/
void SG_FuncDoorSecret_Spawn (edict_t *self){

	vec3_t	forward, right, up;
	float	side;
	float	width;
	float	length;

	self->moveInfo.sound_start = gi.soundindex("doors/dr1_strt.wav");
	self->moveInfo.sound_middle = gi.soundindex("doors/dr1_mid.wav");
	self->moveInfo.sound_end = gi.soundindex("doors/dr1_end.wav");

	self->movetype = MOVETYPE_PUSH;
	self->solid = SOLID_BSP;
	gi.setmodel(self, self->model);

	self->blocked = SG_FuncDoorSecret_Blocked;
	self->use = SG_FuncDoorSecret_Trigger;

	if (!(self->targetname) || (self->spawnflags & SECRET_ALWAYS_SHOOT)){
		self->health = 0;
		self->takedamage = DAMAGE_YES;
		self->die = SG_FuncDoorSecret_Die;
	}

	if (!self->dmg)
		self->dmg = 2;

	if (!self->wait)
		self->wait = 5.0f;

	self->moveInfo.accel = self->moveInfo.decel = self->moveInfo.speed = 50.0f;

	// Calculate positions
	AngleVectors(self->s.angles, forward, right, up);
	VectorClear(self->s.angles);

	side = 1.0f - (self->spawnflags & SECRET_1ST_LEFT);

	if (self->spawnflags & SECRET_1ST_DOWN)
		width = fabs(DotProduct(up, self->size));
	else
		width = fabs(DotProduct(right, self->size));
	
	length = fabs(DotProduct(forward, self->size));
	
	if (self->spawnflags & SECRET_1ST_DOWN)
		VectorMA(self->s.origin, -1.0f * width, up, self->pos1);
	else
		VectorMA(self->s.origin, side * width, right, self->pos1);

	VectorMA(self->pos1, length, forward, self->pos2);

	if (self->health){
		self->takedamage = DAMAGE_YES;
		self->die = SG_FuncDoor_Die;
		self->max_health = self->health;
	}
	else if (self->targetname && self->message){
		gi.soundindex("misc/talk.wav");
		self->touch = SG_FuncDoor_Touch;
	}

	self->className = "func_door";

	gi.linkentity(self);
}


/*
 ==============================================================================

 FUNC_DOOR_ROTATING

 ==============================================================================
*/


/*
 ==================
 SG_FuncDoorRotating_Spawn
 ==================
*/
void SG_FuncDoorRotating_Spawn (edict_t *entity){

	VectorClear(entity->s.angles);

	// Set the axis of rotation
	VectorClear(entity->movedir);

	if (entity->spawnflags & DOOR_X_AXIS)
		entity->movedir[2] = 1.0f;
	else if (entity->spawnflags & DOOR_Y_AXIS)
		entity->movedir[0] = 1.0f;
	else	// Z_AXIS
		entity->movedir[1] = 1.0f;

	// Check for reverse rotation
	if (entity->spawnflags & DOOR_REVERSE)
		VectorNegate(entity->movedir, entity->movedir);

	if (!st.distance){
		gi.dprintf("%s at %s with no distance set\n", entity->className, SG_VectorToString(entity->s.origin));
		st.distance = 90;
	}

	VectorCopy(entity->s.angles, entity->pos1);
	VectorMA(entity->s.angles, st.distance, entity->movedir, entity->pos2);
	entity->moveInfo.distance = st.distance;

	entity->movetype = MOVETYPE_PUSH;
	entity->solid = SOLID_BSP;
	gi.setmodel(entity, entity->model);

	entity->blocked = SG_FuncDoor_Blocked;
	entity->use = SG_FuncDoor_Trigger;

	if (!entity->speed)
		entity->speed = 100.0f;
	if (!entity->accel)
		entity->accel = entity->speed;
	if (!entity->decel)
		entity->decel = entity->speed;

	if (!entity->wait)
		entity->wait = 3.0f;
	if (!entity->dmg)
		entity->dmg = 2;

	if (entity->sounds != 1){
		entity->moveInfo.sound_start = gi.soundindex("doors/dr1_strt.wav");
		entity->moveInfo.sound_middle = gi.soundindex("doors/dr1_mid.wav");
		entity->moveInfo.sound_end = gi.soundindex("doors/dr1_end.wav");
	}

	// If it starts open, switch the positions
	if (entity->spawnflags & DOOR_START_OPEN){
		VectorCopy(entity->pos2, entity->s.angles);
		VectorCopy(entity->pos1, entity->pos2);
		VectorCopy(entity->s.angles, entity->pos1);
		VectorNegate(entity->movedir, entity->movedir);
	}

	if (entity->health){
		entity->takedamage = DAMAGE_YES;
		entity->die = SG_FuncDoor_Die;
		entity->max_health = entity->health;
	}

	if (entity->targetname && entity->message){
		gi.soundindex("misc/talk.wav");
		entity->touch = SG_FuncDoor_Touch;
	}

	entity->moveInfo.state = STATE_BOTTOM;
	entity->moveInfo.speed = entity->speed;
	entity->moveInfo.accel = entity->accel;
	entity->moveInfo.decel = entity->decel;
	entity->moveInfo.wait = entity->wait;
	VectorCopy(entity->s.origin, entity->moveInfo.start_origin);
	VectorCopy(entity->pos1, entity->moveInfo.start_angles);
	VectorCopy(entity->s.origin, entity->moveInfo.end_origin);
	VectorCopy(entity->pos2, entity->moveInfo.end_angles);

	if (entity->spawnflags & 16)
		entity->s.effects |= EF_ANIM_ALL;

	// To simplify logic elsewhere, make non-teamed doors into a team of one
	if (!entity->team)
		entity->teammaster = entity;

	gi.linkentity(entity);

	entity->nextthink = level.time + FRAMETIME;

	if (entity->health || entity->targetname)
		entity->think = SG_FuncDoorCalculateMoveSpeed;
	else
		entity->think = SG_FuncDoorSpawnDoor_Trigger;
}


/*
 ==============================================================================

 FUNC_WATER

 ==============================================================================
*/


/*
 ==================
 SG_FuncWater_Spawn
 ==================
*/
void SG_FuncWater_Spawn (edict_t *self){

	vec3_t	abs_movedir;

	SG_SetMoveDirection(self->s.angles, self->movedir);
	self->movetype = MOVETYPE_PUSH;
	self->solid = SOLID_BSP;
	gi.setmodel(self, self->model);

	switch (self->sounds){
	case 1:		// Water
		self->moveInfo.sound_start = gi.soundindex("world/mov_watr.wav");
		self->moveInfo.sound_end = gi.soundindex("world/stp_watr.wav");
		break;
	case 2:		// Lava
		self->moveInfo.sound_start = gi.soundindex("world/mov_watr.wav");
		self->moveInfo.sound_end = gi.soundindex("world/stp_watr.wav");
		break;
	default:
		break;
	}

	// Calculate second position
	VectorCopy(self->s.origin, self->pos1);
	abs_movedir[0] = fabs(self->movedir[0]);
	abs_movedir[1] = fabs(self->movedir[1]);
	abs_movedir[2] = fabs(self->movedir[2]);
	self->moveInfo.distance = abs_movedir[0] * self->size[0] + abs_movedir[1] * self->size[1] + abs_movedir[2] * self->size[2] - st.lip;
	VectorMA(self->pos1, self->moveInfo.distance, self->movedir, self->pos2);

	// If it starts open, switch the positions
	if (self->spawnflags & DOOR_START_OPEN){
		VectorCopy(self->pos2, self->s.origin);
		VectorCopy(self->pos1, self->pos2);
		VectorCopy(self->s.origin, self->pos1);
	}

	VectorCopy(self->pos1, self->moveInfo.start_origin);
	VectorCopy(self->s.angles, self->moveInfo.start_angles);
	VectorCopy(self->pos2, self->moveInfo.end_origin);
	VectorCopy(self->s.angles, self->moveInfo.end_angles);

	self->moveInfo.state = STATE_BOTTOM;

	if (!self->speed)
		self->speed = 25.0f;

	self->moveInfo.accel = self->moveInfo.decel = self->moveInfo.speed = self->speed;

	if (!self->wait)
		self->wait = -1.0f;

	self->moveInfo.wait = self->wait;

	self->use = SG_FuncDoor_Trigger;

	if (self->wait == -1)
		self->spawnflags |= DOOR_TOGGLE;

	self->className = "func_door";

	gi.linkentity(self);
}


/*
 ==============================================================================

 FUNC_CONVEYOR

 ==============================================================================
*/


/*
 ==================
 SG_FuncConveyor_Trigger
 ==================
*/
static void SG_FuncConveyor_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	if (self->spawnflags & 1){
		self->speed = 0;
		self->spawnflags &= ~1;
	}
	else {
		self->speed = self->count;
		self->spawnflags |= 1;
	}

	if (!(self->spawnflags & 2))
		self->count = 0;
}

/*
 ==================
 SG_FuncConveyor_Spawn
 ==================
*/
void SG_FuncConveyor_Spawn (edict_t *self){

	if (!self->speed)
		self->speed = 100.0f;

	if (!(self->spawnflags & 1)){
		self->count = self->speed;
		self->speed = 0.0f;
	}

	self->use = SG_FuncConveyor_Trigger;

	gi.setmodel(self, self->model);
	self->solid = SOLID_BSP;

	gi.linkentity(self);
}


/*
 ==============================================================================

 FUNC_AREAPORTAL

 ==============================================================================
*/


/*
 ==================
 SG_FuncAreaPortal_Trigger
 ==================
*/
static void SG_FuncAreaPortal_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->count ^= 1;	// Toggle state
	gi.SetAreaPortalState(self->style, (qboolean)self->count);
}


/*
 ==================
 SG_FuncAreaPortal_Spawn
 ==================
*/
void SG_FuncAreaPortal_Spawn (edict_t *self){

	self->use = SG_FuncAreaPortal_Trigger;
	self->count = 0;		// Always start closed
}


/*
 ==============================================================================

 FUNC_CLOCK

 ==============================================================================
*/

#define	CLOCK_MESSAGE_SIZE				16


/*
 ==================
 SG_FuncClock_Reset
 ==================
*/
static void SG_FuncClock_Reset (edict_t *self){

	self->activator = NULL;

	if (self->spawnflags & 1){
		self->health = 0;
		self->wait = self->count;
	}
	else if (self->spawnflags & 2){
		self->health = self->count;
		self->wait = 0;
	}
}

/*
 ==================
 SG_FuncClock_Format_Countdown
 ==================
*/
static void SG_FuncClock_Format_Countdown (edict_t *self){

	if (self->style == 0){
		Com_sprintf (self->message, CLOCK_MESSAGE_SIZE, "%2i", self->health);
		return;
	}

	if (self->style == 1){
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i", self->health / 60, self->health % 60);

		if (self->message[3] == ' ')
			self->message[3] = '0';

		return;
	}

	if (self->style == 2){
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", self->health / 3600, (self->health - (self->health / 3600) * 3600) / 60, self->health % 60);

		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';

		return;
	}
}

/*
 ==================
 SG_FuncClock_Think
 ==================
*/
static void SG_FuncClock_Think (edict_t *self){

	struct tm	*localTime;
	time_t		gameTime;
	char		*saveTarget;
	char		*saveMessage;

	if (!self->enemy){
		self->enemy = SG_FindEntity(NULL, FOFS(targetname), self->target);

		if (!self->enemy)
			return;
	}

	if (self->spawnflags & 1){
		SG_FuncClock_Format_Countdown(self);
		self->health++;
	}
	else if (self->spawnflags & 2){
		SG_FuncClock_Format_Countdown(self);
		self->health--;
	}
	else {
		time(&gameTime);
		localTime = localtime(&gameTime);

		Com_sprintf (self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", localTime->tm_hour, localTime->tm_min, localTime->tm_sec);

		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';
	}

	self->enemy->message = self->message;
	self->enemy->use(self->enemy, self, self);

	if (((self->spawnflags & 1) && (self->health > self->wait)) || ((self->spawnflags & 2) && (self->health < self->wait))){
		if (self->pathtarget){
			saveTarget = self->target;
			saveMessage = self->message;

			self->target = self->pathtarget;
			self->message = NULL;
			SG_UseTargets(self, self->activator);
			self->target = saveTarget;
			self->message = saveMessage;
		}

		if (!(self->spawnflags & 8))
			return;

		SG_FuncClock_Reset(self);

		if (self->spawnflags & 4)
			return;
	}

	self->nextthink = level.time + 1.0f;
}

/*
 ==================
 SG_FuncClock_Trigger
 ==================
*/
static void SG_FuncClock_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	if (!(self->spawnflags & 8))
		self->use = NULL;

	if (self->activator)
		return;

	self->activator = activator;
	self->think(self);
}

/*
 ==================
 SG_FuncClock_Spawn
 ==================
*/
void SG_FuncClock_Spawn (edict_t *self){

	if (!self->target){
		gi.dprintf("%s with no target at %s\n", self->className, SG_VectorToString(self->s.origin));
		SG_FreeEntity(self);
		return;
	}

	if ((self->spawnflags & 2) && (!self->count)){
		gi.dprintf("%s with no count at %s\n", self->className, SG_VectorToString(self->s.origin));
		SG_FreeEntity(self);
		return;
	}

	if ((self->spawnflags & 1) && (!self->count))
		self->count = 60 * 60;

	SG_FuncClock_Reset(self);

	self->message = (char *)gi.TagMalloc(CLOCK_MESSAGE_SIZE, TAG_LEVEL);

	self->think = SG_FuncClock_Think;

	if (self->spawnflags & 4)
		self->use = SG_FuncClock_Trigger;
	else
		self->nextthink = level.time + 1.0f;
}


/*
 ==============================================================================

 FUNC_WALL

 ==============================================================================
*/


/*
 ==================
 SG_FuncWall_Trigger
 ==================
*/
static void SG_FuncWall_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	if (self->solid == SOLID_NOT){
		self->solid = SOLID_BSP;
		self->svflags &= ~SVF_NOCLIENT;
		SG_KillBox(self);
	}
	else {
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}

	gi.linkentity(self);

	if (!(self->spawnflags & 2))
		self->use = NULL;
}

/*
 ==================
 SG_FuncWall_Spawn
 ==================
*/
void SG_FuncWall_Spawn (edict_t *self){

	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	if (self->spawnflags & 8)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 16)
		self->s.effects |= EF_ANIM_ALLFAST;

	// Just a wall
	if ((self->spawnflags & 7) == 0){
		self->solid = SOLID_BSP;
		gi.linkentity(self);
		return;
	}

	// It must be TRIGGER_SPAWN
	if (!(self->spawnflags & 1)){
		self->spawnflags |= 1;

		// Yell if the spawnflags are odd
		if (self->spawnflags & 4){
			if (!(self->spawnflags & 2)){
				gi.dprintf("func_wall START_ON without TOGGLE\n");
				self->spawnflags |= 2;
			}
		}

		self->use = SG_FuncWall_Trigger;

		if (self->spawnflags & 4)
			self->solid = SOLID_BSP;
		else {
			self->solid = SOLID_NOT;
			self->svflags |= SVF_NOCLIENT;
		}

		gi.linkentity(self);
	}
}


/*
 ==============================================================================

 FUNC_OBJECT

 ==============================================================================
*/


/*
 ==================
 SG_FuncObject_Touch
 ==================
*/
static void SG_FuncObject_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	// Only squash thing we fall on top of
	if (!plane)
		return;
	if (plane->normal[2] < 1.0f)
		return;
	if (other->takedamage == DAMAGE_NO)
		return;

	SG_TargetDamage(other, self, self, vec3_origin, self->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

/*
 ==================
 SG_FuncObject_Release
 ==================
*/
static void SG_FuncObject_Think (edict_t *self){

	self->movetype = MOVETYPE_TOSS;
	self->touch = SG_FuncObject_Touch;
}

/*
 ==================
 SG_FuncObject_Trigger
 ==================
*/
static void SG_FuncObject_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;

	SG_KillBox(self);
	SG_FuncObject_Think(self);
}

/*
 ==================
 SG_FuncObject_Spawn
 ==================
*/
void SG_FuncObject_Spawn (edict_t *self){

	gi.setmodel(self, self->model);

	self->mins[0] += 1.0f;
	self->mins[1] += 1.0f;
	self->mins[2] += 1.0f;
	self->maxs[0] -= 1.0f;
	self->maxs[1] -= 1.0f;
	self->maxs[2] -= 1.0f;

	if (!self->dmg)
		self->dmg = 100;

	if (self->spawnflags == 0){
		self->solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
		self->think = SG_FuncObject_Think;
		self->nextthink = level.time + 2.0f * FRAMETIME;
	}
	else {
		self->solid = SOLID_NOT;
		self->movetype = MOVETYPE_PUSH;
		self->use = SG_FuncObject_Trigger;
		self->svflags |= SVF_NOCLIENT;
	}

	if (self->spawnflags & 2)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 4)
		self->s.effects |= EF_ANIM_ALLFAST;

	self->clipmask = MASK_MONSTERSOLID;

	gi.linkentity(self);
}


/*
 ==============================================================================

 FUNC_TRAIN

 ==============================================================================
*/

#define TRAIN_START_ON		1
#define TRAIN_TOGGLE		2
#define TRAIN_BLOCK_STOPS	4

static void SG_FuncTrain_Think (edict_t *self);


/*
 ==================
 SG_FuncTrain_Blocked
 ==================
*/
static void SG_FuncTrain_Blocked (edict_t *self, edict_t *other){

	if (!(other->svflags & SVF_MONSTER) && (!other->client)){		
		// Give it a chance to go away on it's own terms (like gibs)
		SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);

		// If it's still there, nuke it
		if (other)
			SG_BecomeExplosion1(other);

		return;
	}

	if (level.time < self->touch_debounce_time)
		return;

	if (!self->dmg)
		return;

	self->touch_debounce_time = level.time + 0.5f;
	SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

/*
 ==================
 SG_FuncTrain_Wait
 ==================
*/
static void SG_FuncTrain_Wait (edict_t *self){

	edict_t	*entity;
	char	*savetarget;

	if (self->target_ent->pathtarget){
		entity = self->target_ent;
		savetarget = entity->target;
		entity->target = entity->pathtarget;
		SG_UseTargets(entity, self->activator);
		entity->target = savetarget;

		// Make sure we didn't get killed by a killtarget
		if (!self->inuse)
			return;
	}

	if (self->moveInfo.wait){
		if (self->moveInfo.wait > 0.0f){
			self->nextthink = level.time + self->moveInfo.wait;
			self->think = SG_FuncTrain_Think;
		}
		else if (self->spawnflags & TRAIN_TOGGLE) {
			SG_FuncTrain_Think(self);
			self->spawnflags &= ~TRAIN_START_ON;
			VectorClear(self->velocity);
			self->nextthink = 0;
		}

		if (!(self->flags & FL_TEAMSLAVE)){
			if (self->moveInfo.sound_end)
				gi.sound(self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveInfo.sound_end, 1, ATTN_STATIC, 0);

			self->s.sound = 0;
		}
	}
	else
		SG_FuncTrain_Think(self);
}

/*
 ==================
 SG_FuncTrain_Think
 ==================
*/
static void SG_FuncTrain_Think (edict_t *self){

	edict_t		*entity;
	vec3_t		dest;
	qboolean	first;

	first = true;

again:
	if (!self->target)
		return;

	entity = SG_PickTarget(self->target);
	if (!entity){
		gi.dprintf("SG_FuncTrain_Think: bad target %s\n", self->target);
		return;
	}

	self->target = entity->target;

	// Check for a teleport path_corner
	if (entity->spawnflags & 1){
		if (!first){
			gi.dprintf("connected teleport path_corners, see %s at %s\n", entity->className, SG_VectorToString(entity->s.origin));
			return;
		}

		first = false;

		VectorSubtract(entity->s.origin, self->mins, self->s.origin);
		VectorCopy(self->s.origin, self->s.old_origin);
		self->s.event = EV_OTHER_TELEPORT;

		gi.linkentity(self);

		goto again;
	}

	self->moveInfo.wait = entity->wait;
	self->target_ent = entity;

	if (!(self->flags & FL_TEAMSLAVE)){
		if (self->moveInfo.sound_start)
			gi.sound(self, CHAN_NO_PHS_ADD+CHAN_VOICE, self->moveInfo.sound_start, 1.0f, ATTN_STATIC, 0.0f);

		self->s.sound = self->moveInfo.sound_middle;
	}

	VectorSubtract(entity->s.origin, self->mins, dest);
	self->moveInfo.state = STATE_TOP;
	VectorCopy(self->s.origin, self->moveInfo.start_origin);
	VectorCopy(dest, self->moveInfo.end_origin);
	SG_MovementCalculate(self, dest, SG_FuncTrain_Wait);
	self->spawnflags |= TRAIN_START_ON;
}

/*
 ==================
 SG_FuncTrain_Resume
 ==================
*/
void SG_FuncTrain_Resume (edict_t *self){

	edict_t	*entity;
	vec3_t	dest;

	entity = self->target_ent;

	VectorSubtract(entity->s.origin, self->mins, dest);
	self->moveInfo.state = STATE_TOP;
	VectorCopy(self->s.origin, self->moveInfo.start_origin);
	VectorCopy(dest, self->moveInfo.end_origin);
	SG_MovementCalculate(self, dest, SG_FuncTrain_Wait);
	self->spawnflags |= TRAIN_START_ON;
}

/*
 ==================
 SG_FuncTrain_Find
 ==================
*/
void SG_FuncTrain_Find (edict_t *self){

	edict_t *entity;

	if (!self->target){
		gi.dprintf("SG_FuncTrain_Find: no target\n");
		return;
	}

	entity = SG_PickTarget(self->target);
	if (!entity){
		gi.dprintf("SG_FuncTrain_Find: target %s not found\n", self->target);
		return;
	}

	self->target = entity->target;

	VectorSubtract(entity->s.origin, self->mins, self->s.origin);
	gi.linkentity(self);

	// If not triggered, start immediately
	if (!self->targetname)
		self->spawnflags |= TRAIN_START_ON;

	if (self->spawnflags & TRAIN_START_ON){
		self->nextthink = level.time + FRAMETIME;
		self->think = SG_FuncTrain_Think;
		self->activator = self;
	}
}

/*
 ==================
 SG_FuncTrain_Trigger
 ==================
*/
void SG_FuncTrain_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->activator = activator;

	if (self->spawnflags & TRAIN_START_ON){
		if (!(self->spawnflags & TRAIN_TOGGLE))
			return;

		self->spawnflags &= ~TRAIN_START_ON;
		VectorClear (self->velocity);
		self->nextthink = 0.0f;
	}
	else {
		if (self->target_ent)
			SG_FuncTrain_Resume(self);
		else
			SG_FuncTrain_Think(self);
	}
}

/*
 ==================
 SG_FuncTrain_Spawn
 ==================
*/
void SG_FuncTrain_Spawn (edict_t *self){

	self->movetype = MOVETYPE_PUSH;

	VectorClear (self->s.angles);

	self->blocked = SG_FuncTrain_Blocked;

	if (self->spawnflags & TRAIN_BLOCK_STOPS)
		self->dmg = 0;
	else {
		if (!self->dmg)
			self->dmg = 100;
	}

	self->solid = SOLID_BSP;
	gi.setmodel(self, self->model);

	if (st.noise)
		self->moveInfo.sound_middle = gi.soundindex  (st.noise);

	if (!self->speed)
		self->speed = 100.0f;

	self->moveInfo.speed = self->speed;
	self->moveInfo.accel = self->moveInfo.decel = self->moveInfo.speed;

	self->use = SG_FuncTrain_Trigger;

	gi.linkentity(self);

	if (self->target){
		// Start trains on the second frame, to make sure their targets have had
		// a chance to spawn
		self->nextthink = level.time + FRAMETIME;
		self->think = SG_FuncTrain_Find;
	}
	else
		gi.dprintf("func_train without a target at %s\n", SG_VectorToString(self->absmin));
}


/*
 ==============================================================================

 FUNC_TIMER

 ==============================================================================
*/


/*
 ==================
 SG_FuncTimer_Think
 ==================
*/
static void SG_FuncTimer_Think (edict_t *self){

	SG_UseTargets(self, self->activator);
	self->nextthink = level.time + self->wait + crandom() * self->random;
}

/*
 ==================
 SG_FuncTimer_Trigger
 ==================
*/
static void SG_FuncTimer_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->activator = activator;

	// If on, turn it off
	if (self->nextthink){
		self->nextthink = 0.0f;
		return;
	}

	// Turn it on
	if (self->delay)
		self->nextthink = level.time + self->delay;
	else
		SG_FuncTimer_Think(self);
}

/*
 ==================
 SG_FuncTimer_Spawn
 ==================
*/
void SG_FuncTimer_Spawn (edict_t *self){

	if (!self->wait)
		self->wait = 1.0f;

	self->use = SG_FuncTimer_Trigger;
	self->think = SG_FuncTimer_Think;

	if (self->random >= self->wait){
		self->random = self->wait - FRAMETIME;
		gi.dprintf("SG_FuncTimer_Spawn: at %s has random >= wait\n", SG_VectorToString(self->s.origin));
	}

	if (self->spawnflags & 1){
		self->nextthink = level.time + 1.0f + st.pausetime + self->delay + self->wait + crandom() * self->random;
		self->activator = self;
	}

	self->svflags = SVF_NOCLIENT;
}


/*
 ==============================================================================

 FUNC_EXPLOSIVE

 ==============================================================================
*/


/*
 ==================
 SG_FuncExplosive_Die
 ==================
*/
static void SG_FuncExplosive_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	vec3_t	origin;
	vec3_t	chunkorigin;
	vec3_t	size;
	int		count;
	int		mass;

	// Inline model origins are (0 0 0), we need to adjust that here
	VectorScale(self->size, 0.5f, size);
	VectorAdd(self->absmin, size, origin);
	VectorCopy(origin, self->s.origin);

	self->takedamage = DAMAGE_NO;

	if (self->dmg)
		SG_RadiusTargetDamage(self, attacker, self->dmg, NULL, self->dmg+40, MOD_EXPLOSIVE);

	VectorSubtract(self->s.origin, inflictor->s.origin, self->velocity);
	VectorNormalize(self->velocity);
	VectorScale(self->velocity, 150.0f, self->velocity);

	// start chunks towards the center
	VectorScale(size, 0.5f, size);

	mass = self->mass;

	if (!mass)
		mass = 75;

	// Big chunks
	if (mass >= 100){
		count = mass / 100;
		if (count > 8)
			count = 8;

		while (count--){
			chunkorigin[0] = origin[0] + crandom() * size[0];
			chunkorigin[1] = origin[1] + crandom() * size[1];
			chunkorigin[2] = origin[2] + crandom() * size[2];

			ThrowDebris(self, "models/objects/debris1/tris.md2", 1.0f, chunkorigin);
		}
	}

	// Small chunks
	count = mass / 25;
	if (count > 16)
		count = 16;

	while (count--){
		chunkorigin[0] = origin[0] + crandom() * size[0];
		chunkorigin[1] = origin[1] + crandom() * size[1];
		chunkorigin[2] = origin[2] + crandom() * size[2];

		ThrowDebris(self, "models/objects/debris2/tris.md2", 2.0f, chunkorigin);
	}

	SG_UseTargets(self, attacker);

	if (self->dmg)
		SG_BecomeExplosion1(self);
	else
		SG_FreeEntity(self);
}

/*
 ==================
 SG_FuncExplosive_Trigger
 ==================
*/
static void SG_FuncExplosive_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	SG_FuncExplosive_Die(self, self, other, self->health, vec3_origin);
}

/*
 ==================
 SG_FuncExplosive_Create
 ==================
*/
static void SG_FuncExplosive_Create (edict_t *self, edict_t *other, edict_t *activator){

	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;

	SG_KillBox(self);

	gi.linkentity(self);
}

/*
 ==================
 SG_FuncExplosive_Spawn
 ==================
*/
void SG_FuncExplosive_Spawn (edict_t *self){

	// Auto-remove for deathmatch
	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->movetype = MOVETYPE_PUSH;

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");

	gi.setmodel(self, self->model);

	if (self->spawnflags & 1){
		self->svflags |= SVF_NOCLIENT;
		self->solid = SOLID_NOT;
		self->use = SG_FuncExplosive_Create;
	}
	else {
		self->solid = SOLID_BSP;

		if (self->targetname)
			self->use = SG_FuncExplosive_Trigger;
	}

	if (self->spawnflags & 2)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 4)
		self->s.effects |= EF_ANIM_ALLFAST;

	if (self->use != SG_FuncExplosive_Trigger){
		if (!self->health)
			self->health = 100;

		self->die = SG_FuncExplosive_Die;
		self->takedamage = DAMAGE_YES;
	}

	gi.linkentity(self);
}


/*
 ==============================================================================

 FUNC_KILLBOX

 ==============================================================================
*/


/*
 ==================
 SG_FuncKillbox_Trigger
 ==================
*/
static void SG_FuncKillbox_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	SG_KillBox(self);
}

/*
 ==================
 SG_FuncKillbox_Spawn
 ==================
*/
void SG_FuncKillbox_Spawn (edict_t *self){

	gi.setmodel(self, self->model);

	self->use = SG_FuncKillbox_Trigger;
	self->svflags = SVF_NOCLIENT;
}