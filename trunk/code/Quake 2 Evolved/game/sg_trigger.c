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
// sg_trigger.c - Trigger functions used in the map editor
//


#include "g_local.h"


#define PUSH_ONCE						1

static int				windSound;

void SG_TriggerMultiple_Spawn (edict_t *entity);
void SG_FuncTrain_Resume (edict_t *self);


/*
 ==============================================================================

 INITIALIZATION

 ==============================================================================
*/


/*
 ==================
 SG_Trigger_Init
 ==================
*/
static void SG_Trigger_Init (edict_t *self){

	if (!VectorCompare(self->s.angles, vec3_origin))
		SG_SetMoveDirection(self->s.angles, self->movedir);

	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
	gi.setmodel(self, self->model);
	self->svflags = SVF_NOCLIENT;
}


/*
 ==============================================================================

 TRIGGER_ALWAYS

 ==============================================================================
*/


/*
 ==================
 SG_TriggerAlways_Spawn
 ==================
*/
void SG_TriggerAlways_Spawn (edict_t *entity){

	// We must have some delay to make sure our use targets are present
	if (entity->delay < 0.2f)
		entity->delay = 0.2f;

	SG_UseTargets(entity, entity);
}


/*
 ==============================================================================

 TRIGGER_ONCE

 ==============================================================================
*/


/*
 ==================
 SG_TriggerOnce_Spawn
 ==================
*/
void SG_TriggerOnce_Spawn (edict_t *entity){

	vec3_t	v;

	// make old maps work because I messed up on flag assignments here
	// triggered was on bit 1 when it should have been on bit 4
	if (entity->spawnflags & 1){
		VectorMA(entity->mins, 0.5f, entity->size, v);
		entity->spawnflags &= ~1;
		entity->spawnflags |= 4;
		gi.dprintf("fixed TRIGGERED flag on %s at %s\n", entity->className, SG_VectorToString(v));
	}

	entity->wait = -1.0f;

	SG_TriggerMultiple_Spawn(entity);
}


/*
 ==============================================================================

 TRIGGER_MULTIPLE

 ==============================================================================
*/


/*
 ==================
 SG_TriggerMultiple_Wait

 The wait time has passed, so set back up for another activation
 ==================
*/
static void SG_TriggerMultiple_Wait (edict_t *entity){

	entity->nextthink = 0.0f;
}

/*
 ==================
 SG_TriggerMultiple

 The trigger was just activated
 ent->activator should be set to the activator so it can be held through a delay
 so wait for the delay time before firing
 ==================
*/
static void SG_TriggerMultiple (edict_t *entity){

	if (entity->nextthink)
		return;		// Already been triggered

	SG_UseTargets(entity, entity->activator);

	if (entity->wait > 0.0f){
		entity->think = SG_TriggerMultiple_Wait;
		entity->nextthink = level.time + entity->wait;
	}
	else {	
		// We can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		entity->touch = NULL;
		entity->nextthink = level.time + FRAMETIME;
		entity->think = SG_FreeEntity;
	}
}

/*
 ==================
 SG_TriggerMultiple_Use
 ==================
*/
static void SG_TriggerMultiple_Use (edict_t *entity, edict_t *other, edict_t *activator){

	entity->activator = activator;
	SG_TriggerMultiple(entity);
}

/*
 ==================
 SG_TriggerMultiple_Touch
 ==================
*/
static void SG_TriggerMultiple_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	vec3_t	forward;

	if (other->client){
		if (self->spawnflags & 2)
			return;
	}
	else if (other->svflags & SVF_MONSTER){
		if (!(self->spawnflags & 1))
			return;
	}
	else
		return;

	if (!VectorCompare(self->movedir, vec3_origin)){
		AngleVectors(other->s.angles, forward, NULL, NULL);

		if (_DotProduct(forward, self->movedir) < 0.0f)
			return;
	}

	self->activator = other;
	SG_TriggerMultiple(self);
}

/*
 ==================
 SG_TriggerMultiple_Trigger
 ==================
*/
static void SG_TriggerMultiple_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->solid = SOLID_TRIGGER;
	self->use = SG_TriggerMultiple_Use;

	gi.linkentity(self);
}

/*
 ==================
 SG_TriggerMultiple_Spawn
 ==================
*/
void SG_TriggerMultiple_Spawn (edict_t *entity){

	if (entity->sounds == 1)
		entity->noiseIndex = gi.soundindex("misc/secret.wav");
	else if (entity->sounds == 2)
		entity->noiseIndex = gi.soundindex("misc/talk.wav");
	else if (entity->sounds == 3)
		entity->noiseIndex = gi.soundindex("misc/trigger1.wav");

	if (!entity->wait)
		entity->wait = 0.2f;

	entity->touch = SG_TriggerMultiple_Touch;
	entity->movetype = MOVETYPE_NONE;
	entity->svflags |= SVF_NOCLIENT;


	if (entity->spawnflags & 4){
		entity->solid = SOLID_NOT;
		entity->use = SG_TriggerMultiple_Trigger;
	}
	else {
		entity->solid = SOLID_TRIGGER;
		entity->use = SG_TriggerMultiple_Use;
	}

	if (!VectorCompare(entity->s.angles, vec3_origin))
		SG_SetMoveDirection(entity->s.angles, entity->movedir);

	gi.setmodel(entity, entity->model);
	gi.linkentity(entity);
}


/*
 ==============================================================================

 TRIGGER_RELAY

 ==============================================================================
*/


/*
 ==================
 SG_TriggerRelay_Trigger
 ==================
*/
static void SG_TriggerRelay_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	SG_UseTargets(self, activator);
}

/*
 ==================
 SG_TriggerRelay_Spawn
 ==================
*/
void SG_TriggerRelay_Spawn (edict_t *self){

	// Set the callback functions
	self->use = SG_TriggerRelay_Trigger;
}


/*
 ==============================================================================

 TRIGGER_PUSH

 ==============================================================================
*/


/*
 ==================
 SG_TriggerPush_Touch
 ==================
*/
static void SG_TriggerPush_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	if (!strcmp(other->className, "grenade"))
		VectorScale (self->movedir, self->speed * 10.0f, other->velocity);
	else if (other->health > 0){
		VectorScale (self->movedir, self->speed * 10.0f, other->velocity);

		// Don't take falling damage immediately from this
		if (other->client){
			VectorCopy(other->velocity, other->client->oldvelocity);

			if (other->fly_sound_debounce_time < level.time){
				other->fly_sound_debounce_time = level.time + 1.5f;
				gi.sound(other, CHAN_AUTO, windSound, 1.0f, ATTN_NORM, 0.0f);
			}
		}
	}

	if (self->spawnflags & PUSH_ONCE)
		SG_FreeEntity(self);
}

/*
 ==================
 SG_TriggerPush_Spawn
 ==================
*/
void SG_TriggerPush_Spawn (edict_t *self){

	SG_Trigger_Init(self);

	windSound = gi.soundindex("misc/windfly.wav");

	self->touch = SG_TriggerPush_Touch;

	if (!self->speed)
		self->speed = 1000.0f;

	gi.linkentity(self);
}


/*
 ==============================================================================

 TRIGGER_HURT

 ==============================================================================
*/


/*
 ==================
 SG_TriggerHurt_Trigger
 ==================
*/
static void SG_TriggerHurt_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;

	gi.linkentity(self);

	if (!(self->spawnflags & 2))
		self->use = NULL;
}

/*
 ==================
 SG_TriggerHurt_Touch
 ==================
*/
static void SG_TriggerHurt_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	int		dmgFlags;

	if (!other->takedamage)
		return;

	if (self->timestamp > level.time)
		return;

	if (self->spawnflags & 16)
		self->timestamp = level.time + 1.0f;
	else
		self->timestamp = level.time + FRAMETIME;

	if (!(self->spawnflags & 4)){
		if ((level.framenum % 10) == 0)
			gi.sound(other, CHAN_AUTO, self->noiseIndex, 1, ATTN_NORM, 0);
	}

	if (self->spawnflags & 8)
		dmgFlags = DAMAGE_NO_PROTECTION;
	else
		dmgFlags = 0;

	SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, self->dmg, dmgFlags, MOD_TRIGGER_HURT);
}

/*
 ==================
 SG_TriggerHurt_Spawn
 ==================
*/
void SG_TriggerHurt_Spawn (edict_t *self){

	SG_Trigger_Init(self);

	self->noiseIndex = gi.soundindex("world/electro.wav");
	self->touch = SG_TriggerHurt_Touch;

	if (!self->dmg)
		self->dmg = 5;

	if (self->spawnflags & 1)
		self->solid = SOLID_NOT;
	else
		self->solid = SOLID_TRIGGER;

	if (self->spawnflags & 2)
		self->use = SG_TriggerHurt_Trigger;

	gi.linkentity(self);
}


/*
 ==============================================================================

 TRIGGER_KEY

 ==============================================================================
*/


/*
 ==================
 SG_TriggerKey_Trigger
 ==================
*/
static void SG_TriggerKey_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	int		index;
	int		player;
	edict_t	*ent;
	int		cube;

	if (!self->item)
		return;
	if (!activator->client)
		return;

	index = ITEM_INDEX(self->item);

	if (!activator->client->pers.inventory[index]){
		if (level.time < self->touch_debounce_time)
			return;

		self->touch_debounce_time = level.time + 5.0f;
		gi.centerprintf(activator, "You need the %s", self->item->pickupName);
		gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/keytry.wav"), 1.0f, ATTN_NORM, 0.0f);
		return;
	}

	gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/keyuse.wav"), 1.0f, ATTN_NORM, 0.0f);

	if (coop->value){
		if (!strcmp(self->item->className, "key_power_cube")){
			for (cube = 0; cube < 8; cube++){
				if (activator->client->pers.power_cubes & (1 << cube))
					break;
			}

			for (player = 1; player <= game.maxclients; player++){
				ent = &g_edicts[player];

				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;

				if (ent->client->pers.power_cubes & (1 << cube)){
					ent->client->pers.inventory[index]--;
					ent->client->pers.power_cubes &= ~(1 << cube);
				}
			}
		}
		else {
			for (player = 1; player <= game.maxclients; player++){
				ent = &g_edicts[player];

				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;

				ent->client->pers.inventory[index] = 0;
			}
		}
	}
	else
		activator->client->pers.inventory[index]--;

	SG_UseTargets(self, activator);

	self->use = NULL;
}

/*
 ==================
 SG_TriggerKey_Spawn
 ==================
*/
void SG_TriggerKey_Spawn (edict_t *self){

	if (!st.item){
		gi.dprintf("no key item for trigger_key at %s\n", SG_VectorToString(self->s.origin));
		return;
	}

	self->item = SG_FindItemByClassname (st.item);

	if (!self->item){
		gi.dprintf("item %s not found for trigger_key at %s\n", st.item, SG_VectorToString(self->s.origin));
		return;
	}

	if (!self->target){
		gi.dprintf("%s at %s has no target\n", self->className, SG_VectorToString(self->s.origin));
		return;
	}

	gi.soundindex("misc/keytry.wav");
	gi.soundindex("misc/keyuse.wav");

	// Set the callback functions
	self->use = SG_TriggerKey_Trigger;
}


/*
 ==============================================================================

 TRIGGER_COUNTER

 ==============================================================================
*/


/*
 ==================
 SG_TriggerCounter_Trigger
 ==================
*/
static void SG_TriggerCounter_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	if (self->count == 0)
		return;

	self->count--;

	if (self->count){
		if (!(self->spawnflags & 1)){
			gi.centerprintf(activator, "%i more to go...", self->count);
			gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1.0f, ATTN_NORM, 0.0f);
		}

		return;
	}

	if (!(self->spawnflags & 1)){
		gi.centerprintf(activator, "Sequence completed!");
		gi.sound(activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1.0f, ATTN_NORM, 0.0f);
	}

	self->activator = activator;
	SG_TriggerMultiple(self);
}

/*
 ==================
 SG_TriggerCounter_Spawn
 ==================
*/
void SG_TriggerCounter_Spawn (edict_t *self){

	self->wait = -1;

	if (!self->count)
		self->count = 2;

	// Set the callback functions
	self->use = SG_TriggerCounter_Trigger;
}


/*
 ==============================================================================

 TRIGGER_ELEVATOR

 ==============================================================================
*/


/*
 ==================
 SG_TriggerElevator_Trigger
 ==================
*/
static void SG_TriggerElevator_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	edict_t *target;

	if (self->moveTarget->nextthink)
		return;

	if (!other->pathtarget){
		gi.dprintf("elevator used with no pathtarget\n");
		return;
	}

	target = SG_PickTarget(other->pathtarget);
	if (!target){
		gi.dprintf("elevator used with bad pathtarget: %s\n", other->pathtarget);
		return;
	}

	self->moveTarget->target_ent = target;
	SG_FuncTrain_Resume(self->moveTarget);
}

/*
 ==================
 SG_TriggerElevator_Think
 ==================
*/
static void SG_TriggerElevator_Think (edict_t *self){

	if (!self->target){
		gi.dprintf("trigger_elevator has no target\n");
		return;
	}

	self->moveTarget = SG_PickTarget(self->target);
	if (!self->moveTarget){
		gi.dprintf("trigger_elevator unable to find target %s\n", self->target);
		return;
	}

	if (strcmp(self->moveTarget->className, "func_train") != 0){
		gi.dprintf("trigger_elevator target %s is not a train\n", self->target);
		return;
	}

	self->use = SG_TriggerElevator_Trigger;
	self->svflags = SVF_NOCLIENT;
}

/*
 ==================
 SG_TriggerElevator_Spawn
 ==================
*/
void SG_TriggerElevator_Spawn (edict_t *self){

	self->think = SG_TriggerElevator_Think;
	self->nextthink = level.time + FRAMETIME;
}


/*
 ==============================================================================

 TRIGGER_GRAVITY

 ==============================================================================
*/


/*
 ==================
 SG_TriggerTouch_Spawn
 ==================
*/
void SG_TriggerTouch_Spawn (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	other->gravity = self->gravity;
}

/*
 ==================
 SG_TriggerGravity_Spawn
 ==================
*/
void SG_TriggerGravity_Spawn (edict_t *self){

	if (st.gravity == 0){
		gi.dprintf("trigger_gravity without gravity set at %s\n", SG_VectorToString(self->s.origin));
		SG_FreeEntity(self);
		return;
	}

	SG_Trigger_Init(self);
	self->gravity = atoi(st.gravity);
	self->touch = SG_TriggerTouch_Spawn;
}


/*
 ==============================================================================

 TRIGGER_MONSTERJUMP

 ==============================================================================
*/


/*
 ==================
 SG_TriggerMonsterJump_Trigger
 ==================
*/
static void SG_TriggerMonsterJump_Trigger (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	if (other->flags & (FL_FLY | FL_SWIM))
		return;
	if (other->svflags & SVF_DEADMONSTER)
		return;
	if (!(other->svflags & SVF_MONSTER))
		return;

	// Set XY even if not on ground, so the jump will clear lips
	other->velocity[0] = self->movedir[0] * self->speed;
	other->velocity[1] = self->movedir[1] * self->speed;

	if (!other->groundentity)
		return;

	other->groundentity = NULL;
	other->velocity[2] = self->movedir[2];
}

/*
 ==================
 SG_TriggerMonsterJump_Spawn
 ==================
*/
void SG_TriggerMonsterJump_Spawn (edict_t *self){

	if (!self->speed)
		self->speed = 200.0f;

	if (!st.height)
		st.height = 200.0f;

	if (self->s.angles[YAW] == 0.0f)
		self->s.angles[YAW] = 360.0f;

	SG_Trigger_Init(self);

	self->touch = SG_TriggerMonsterJump_Trigger;
	self->movedir[2] = st.height;
}