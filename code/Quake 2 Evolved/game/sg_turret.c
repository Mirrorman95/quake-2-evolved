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
// sg_turret.c - Turret entity
//


#include "g_local.h"


void Infantry_StandAnimation (edict_t *self);
void Infantry_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);


/*
 ==================
 SG_AnglesNormalize
 ==================
*/
static void SG_AnglesNormalize (vec3_t vec){

	while (vec[0] > 360.0f)
		vec[0] -= 360.0f;
	while (vec[0] < 0.0f)
		vec[0] += 360.0f;
	while (vec[1] > 360.0f)
		vec[1] -= 360.0f;
	while (vec[1] < 0.0f)
		vec[1] += 360.0f;
}

/*
 ==================
 SG_SnapToEights
 ==================
*/
static float SG_SnapToEights (float x){

	x *= 8.0f;

	if (x > 0.0f)
		x += 0.5f;
	else
		x -= 0.5f;

	return 0.125f * (int)x;
}

/*
 ==================
 Turret_Blocked
 ==================
*/
static void Turret_Blocked (edict_t *self, edict_t *other){

	edict_t	*attacker;

	if (other->takedamage){
		if (self->teammaster->owner)
			attacker = self->teammaster->owner;
		else
			attacker = self->teammaster;

		SG_TargetDamage(other, self, attacker, vec3_origin, other->s.origin, vec3_origin, self->teammaster->dmg, 10, 0, MOD_CRUSH);
	}
}


/*
 ==============================================================================

 TURRET_BREACH

 ==============================================================================
*/


/*
 ==================
 Turret_Breach_Fire
 ==================
*/
static void Turret_Breach_Fire (edict_t *self){

	vec3_t	f, r, u;
	vec3_t	start;
	int		damage;
	int		speed;

	AngleVectors(self->s.angles, f, r, u);
	VectorMA(self->s.origin, self->moveOrigin[0], f, start);
	VectorMA(start, self->moveOrigin[1], r, start);
	VectorMA(start, self->moveOrigin[2], u, start);

	damage = 100 + random() * 50;
	speed = 550 + 50 * skill->value;
	SG_FireRocketProjectile(self->teammaster->owner, start, f, damage, speed, 150, damage);

	gi.positioned_sound(start, self, CHAN_WEAPON, gi.soundindex("weapons/rocklf1a.wav"), 1, ATTN_NORM, 0);
}

/*
 ==================
 Turret_Breach_Think
 ==================
*/
static void Turret_Breach_Think (edict_t *self){

	edict_t	*entity;
	vec3_t	currentAngles;
	vec3_t	delta;
	float	dmin, dmax;
	float	angle;
	float	targetZ;
	float	diff;
	vec3_t	target;
	vec3_t	dir;

	VectorCopy(self->s.angles, currentAngles);
	SG_AnglesNormalize(currentAngles);

	SG_AnglesNormalize(self->moveAngles);
	if (self->moveAngles[PITCH] > 180.0f)
		self->moveAngles[PITCH] -= 360.0f;

	// Clamp angles to mins/maxs
	if (self->moveAngles[PITCH] > self->pos1[PITCH])
		self->moveAngles[PITCH] = self->pos1[PITCH];
	else if (self->moveAngles[PITCH] < self->pos2[PITCH])
		self->moveAngles[PITCH] = self->pos2[PITCH];

	if ((self->moveAngles[YAW] < self->pos1[YAW]) || (self->moveAngles[YAW] > self->pos2[YAW])){
		dmin = fabs(self->pos1[YAW] - self->moveAngles[YAW]);
		if (dmin < -180.0f)
			dmin += 360.0f;
		else if (dmin > 180.0f)
			dmin -= 360.0f;

		dmax = fabs(self->pos2[YAW] - self->moveAngles[YAW]);
		if (dmax < -180.0f)
			dmax += 360.0f;
		else if (dmax > 180.0f)
			dmax -= 360.0f;

		if (fabs(dmin) < fabs(dmax))
			self->moveAngles[YAW] = self->pos1[YAW];
		else
			self->moveAngles[YAW] = self->pos2[YAW];
	}

	VectorSubtract(self->moveAngles, currentAngles, delta);

	if (delta[0] < -180.0f)
		delta[0] += 360.0f;
	else if (delta[0] > 180.0f)
		delta[0] -= 360.0f;

	if (delta[1] < -180.0f)
		delta[1] += 360.0f;
	else if (delta[1] > 180.0f)
		delta[1] -= 360.0f;

	delta[2] = 0.0f;

	if (delta[0] > self->speed * FRAMETIME)
		delta[0] = self->speed * FRAMETIME;
	if (delta[0] < -1.0f * self->speed * FRAMETIME)
		delta[0] = -1.0f * self->speed * FRAMETIME;
	if (delta[1] > self->speed * FRAMETIME)
		delta[1] = self->speed * FRAMETIME;
	if (delta[1] < -1.0f * self->speed * FRAMETIME)
		delta[1] = -1.0f * self->speed * FRAMETIME;

	VectorScale(delta, 1.0f / FRAMETIME, self->avelocity);

	self->nextthink = level.time + FRAMETIME;

	for (entity = self->teammaster; entity; entity = entity->teamchain)
		entity->avelocity[1] = self->avelocity[1];

	// If we have a driver, adjust his velocities
	if (self->owner){
		// Angular is easy, just copy ours
		self->owner->avelocity[0] = self->avelocity[0];
		self->owner->avelocity[1] = self->avelocity[1];

		// X/Y
		angle = self->s.angles[1] + self->owner->moveOrigin[1];
		angle *= (M_PI * 2.0f / 360.0f);
		target[0] = SG_SnapToEights(self->s.origin[0] + cos(angle) * self->owner->moveOrigin[0]);
		target[1] = SG_SnapToEights(self->s.origin[1] + sin(angle) * self->owner->moveOrigin[0]);
		target[2] = self->owner->s.origin[2];

		VectorSubtract(target, self->owner->s.origin, dir);
		self->owner->velocity[0] = dir[0] * 1.0f / FRAMETIME;
		self->owner->velocity[1] = dir[1] * 1.0f / FRAMETIME;

		// Z
		angle = self->s.angles[PITCH] * (M_PI * 2.0f / 360.0f);
		targetZ = SG_SnapToEights(self->s.origin[2] + self->owner->moveOrigin[0] * tan(angle) + self->owner->moveOrigin[2]);

		diff = targetZ - self->owner->s.origin[2];
		self->owner->velocity[2] = diff * 1.0f / FRAMETIME;

		if (self->spawnflags & 65536){
			Turret_Breach_Fire(self);
			self->spawnflags &= ~65536;
		}
	}
}

/*
 ==================
 Turret_BreachFinish_Think
 ==================
*/
void Turret_BreachFinish_Think (edict_t *self){

	// Get and save info for muzzle location
	if (!self->target)
		gi.dprintf("%s at %s needs a target\n", self->className, SG_VectorToString(self->s.origin));
	else {
		self->target_ent = SG_PickTarget(self->target);
		VectorSubtract(self->target_ent->s.origin, self->s.origin, self->moveOrigin);

		SG_FreeEntity(self->target_ent);
	}

	self->teammaster->dmg = self->dmg;
	self->think = Turret_Breach_Think;
	self->think(self);
}

/*
 ==================
 Turret_Breach_Spawn
 ==================
*/
void Turret_Breach_Spawn (edict_t *self){

	self->solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel (self, self->model);

	if (!self->speed)
		self->speed = 50.0f;
	if (!self->dmg)
		self->dmg = 10;

	if (!st.minpitch)
		st.minpitch = -30.0f;
	if (!st.maxpitch)
		st.maxpitch = 30.0f;
	if (!st.maxyaw)
		st.maxyaw = 360.0f;

	self->pos1[PITCH] = -1.0f * st.minpitch;
	self->pos1[YAW]   = st.minyaw;
	self->pos2[PITCH] = -1.0f * st.maxpitch;
	self->pos2[YAW]   = st.maxyaw;

	self->idealYaw = self->s.angles[YAW];
	self->moveAngles[YAW] = self->idealYaw;

	self->blocked = Turret_Blocked;

	self->think = Turret_BreachFinish_Think;
	self->nextthink = level.time + FRAMETIME;

	gi.linkentity(self);
}


/*
 ==============================================================================

 TURRET_BASE

 ==============================================================================
*/


/*
 ==================
 Turret_Base_Spawn
 ==================
*/
void Turret_Base_Spawn (edict_t *self){

	self->solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->blocked = Turret_Blocked;

	gi.linkentity(self);
}


/*
 ==============================================================================

 TURRET_DRIVER

 ==============================================================================
*/


/*
 ==================
 Turret_Driver_Die
 ==================
*/
static void Turret_Driver_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	edict_t	*entity;

	// Level the gun
	self->target_ent->moveAngles[0] = 0.0f;

	// Remove the driver from the end of them team chain
	for (entity = self->target_ent->teammaster; entity->teamchain != self; entity = entity->teamchain);

	entity->teamchain = NULL;
	self->teammaster = NULL;
	self->flags &= ~FL_TEAMSLAVE;

	self->target_ent->owner = NULL;
	self->target_ent->teammaster->owner = NULL;

	Infantry_DieAnimation(self, inflictor, attacker, damage, 0);
}

/*
 ==================
 Turret_Driver_Think
 ==================
*/
static void Turret_Driver_Think (edict_t *self){

	vec3_t	target;
	vec3_t	dir;
	float	reactionTime;

	self->nextthink = level.time + FRAMETIME;

	if (self->enemy && (!self->enemy->inuse || self->enemy->health <= 0))
		self->enemy = NULL;

	if (!self->enemy){
		if (!SG_AIFindTarget (self))
			return;

		self->monsterinfo.trail_time = level.time;
		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
	}
	else {
		if (SG_IsEntityVisible(self, self->enemy)){
			if (self->monsterinfo.aiflags & AI_LOST_SIGHT){
				self->monsterinfo.trail_time = level.time;
				self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
			}
		}
		else {
			self->monsterinfo.aiflags |= AI_LOST_SIGHT;
			return;
		}
	}

	// Let the turret know where we want it to aim
	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, self->target_ent->s.origin, dir);
	SG_VectorToAngles(dir, self->target_ent->moveAngles);

	// Decide if we should shoot
	if (level.time < self->monsterinfo.attack_finished)
		return;

	reactionTime = (3 - skill->value) * 1.0f;

	if ((level.time - self->monsterinfo.trail_time) < reactionTime)
		return;

	self->monsterinfo.attack_finished = level.time + reactionTime + 1.0f;

	// FIXME: How do we really want to pass this along?
	self->target_ent->spawnflags |= 65536;
}

/*
 ==================
 Turret_DriverLink_Think
 ==================
*/
static void Turret_DriverLink_Think (edict_t *self){

	vec3_t	vec;
	edict_t	*entity;

	self->think = Turret_Driver_Think;
	self->nextthink = level.time + FRAMETIME;

	self->target_ent = SG_PickTarget(self->target);
	self->target_ent->owner = self;
	self->target_ent->teammaster->owner = self;
	VectorCopy(self->target_ent->s.angles, self->s.angles);

	vec[0] = self->target_ent->s.origin[0] - self->s.origin[0];
	vec[1] = self->target_ent->s.origin[1] - self->s.origin[1];
	vec[2] = 0.0f;
	self->moveOrigin[0] = VectorLength(vec);

	VectorSubtract(self->s.origin, self->target_ent->s.origin, vec);
	SG_VectorToAngles(vec, vec);
	SG_AnglesNormalize(vec);
	self->moveOrigin[1] = vec[1];

	self->moveOrigin[2] = self->s.origin[2] - self->target_ent->s.origin[2];

	// Add the driver to the end of them team chain
	for (entity = self->target_ent->teammaster; entity->teamchain; entity = entity->teamchain);

	entity->teamchain = self;
	self->teammaster = self->target_ent->teammaster;
	self->flags |= FL_TEAMSLAVE;
}

/*
 ==================
 Turret_Driver_Spawn
 ==================
*/
void Turret_Driver_Spawn (edict_t *self){

/*
	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}
*/
	self->s.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_PUSH;

	self->mass = 200;

	self->health = 100;
	self->gib_health = 0;

	self->viewheight = 24;

	self->die = Turret_Driver_Die;
	self->monsterinfo.stand = Infantry_StandAnimation;

	self->flags |= FL_NO_KNOCKBACK;

	level.total_monsters++;

	self->svflags |= SVF_MONSTER;
	self->s.renderfx |= RF_FRAMELERP;
	self->takedamage = DAMAGE_AIM;
	self->use = SG_MonsterUse;
	self->clipmask = MASK_MONSTERSOLID;
	VectorCopy(self->s.origin, self->s.old_origin);
	self->monsterinfo.aiflags |= AI_STAND_GROUND|AI_DUCKED;

	if (st.item){
		self->item = SG_FindItemByClassname (st.item);
		if (!self->item)
			gi.dprintf("%s at %s has bad item: %s\n", self->className, SG_VectorToString(self->s.origin), st.item);
	}

	self->think = Turret_DriverLink_Think;
	self->nextthink = level.time + FRAMETIME;

	gi.linkentity(self);
}