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
// sg_miscellaneous.c - Miscellaneous functions used in the map editor and some other stuff
//


#include "g_local.h"


extern void SG_FuncTrain_Find (edict_t *self);
extern void SG_FuncTrain_Trigger (edict_t *self, edict_t *other, edict_t *activator);


/*
 ==============================================================================

 EXPLOSION ENTITY

 ==============================================================================
*/


/*
 ==================
 SG_BecomeExplosion1
 ==================
*/
void SG_BecomeExplosion1 (edict_t *self){

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	SG_FreeEntity(self);
}

/*
 ==================
 SG_BecomeExplosion2
 ==================
*/
void SG_BecomeExplosion2 (edict_t *self){

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION2);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	SG_FreeEntity(self);
}


/*
 ==============================================================================

 GIBS

 ==============================================================================
*/


/*
 ==================
 SG_Gib_Die
 ==================
*/
static void SG_Gib_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	SG_FreeEntity(self);
}

/*
 ==================
 SG_Gib_Think
 ==================
*/
static void SG_Gib_Think (edict_t *self){

	self->s.frame++;
	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == 10){
		self->think = SG_FreeEntity;
		self->nextthink = level.time + 8.0f + random() * 10.0f;
	}
}

/*
 ==================
 SG_Gib_Touch
 ==================
*/
static void SG_Gib_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	vec3_t	normalAngles, right;

	if (!self->groundentity)
		return;

	self->touch = NULL;

	if (plane){
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/fhit3.wav"), 1.0f, ATTN_NORM, 0.0f);

		SG_VectorToAngles(plane->normal, normalAngles);
		AngleVectors(normalAngles, NULL, right, NULL);
		SG_VectorToAngles(right, self->s.angles);

		if (self->s.modelindex == sm_meat_index){
			self->s.frame++;
			self->think = SG_Gib_Think;
			self->nextthink = level.time + FRAMETIME;
		}
	}
}

/*
 ==================
 SG_GibVelocityForDamage
 ==================
*/
static void SG_GibVelocityForDamage (int damage, vec3_t v){

	v[0] = 100.0f * crandom();
	v[1] = 100.0f * crandom();
	v[2] = 200.0f + 100.0f * random();

	if (damage < 50)
		VectorScale(v, 0.7f, v);
	else 
		VectorScale(v, 1.2f, v);
}

/*
 ==================
 SG_ClipGibVelocity
 ==================
*/
static void SG_ClipGibVelocity (edict_t *entity){

	// X
	if (entity->velocity[0] < -300.0f)
		entity->velocity[0] = -300.0f;
	else if (entity->velocity[0] > 300.0f)
		entity->velocity[0] = 300.0f;

	// Y
	if (entity->velocity[1] < -300.0f)
		entity->velocity[1] = -300.0f;
	else if (entity->velocity[1] > 300.0f)
		entity->velocity[1] = 300.0f;

	//  Z
	if (entity->velocity[2] < 200.0f)
		entity->velocity[2] = 200.0f;
	else if (entity->velocity[2] > 500.0f)
		entity->velocity[2] = 500.0f;
}

/*
 ==================
 SG_ThrowGib
 ==================
*/
void ThrowGib (edict_t *self, char *gibname, int damage, int type){

	edict_t *gib;
	vec3_t	vd;
	vec3_t	origin;
	vec3_t	size;
	float	vscale;

	gib = SG_AllocEntity();

	VectorScale(self->size, 0.5f, size);
	VectorAdd(self->absmin, size, origin);
	gib->s.origin[0] = origin[0] + crandom() * size[0];
	gib->s.origin[1] = origin[1] + crandom() * size[1];
	gib->s.origin[2] = origin[2] + crandom() * size[2];

	gi.setmodel(gib, gibname);
	gib->solid = SOLID_NOT;
	gib->s.effects |= EF_GIB;
	gib->flags |= FL_NO_KNOCKBACK;
	gib->takedamage = DAMAGE_YES;
	gib->die = SG_Gib_Die;

	if (type == GIB_ORGANIC){
		gib->movetype = MOVETYPE_TOSS;
		gib->touch = SG_Gib_Touch;
		vscale = 0.5f;
	}
	else {
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0f;
	}

	SG_GibVelocityForDamage(damage, vd);
	VectorMA(self->velocity, vscale, vd, gib->velocity);

	SG_ClipGibVelocity(gib);

	gib->avelocity[0] = random() * 600.0f;
	gib->avelocity[1] = random() * 600.0f;
	gib->avelocity[2] = random() * 600.0f;

	gib->think = SG_FreeEntity;
	gib->nextthink = level.time + 10.0f + random() * 10.0f;

	gi.linkentity(gib);
}

/*
 ==================
 SG_ThrowHead
 ==================
*/
void ThrowHead (edict_t *self, char *gibname, int damage, int type){

	vec3_t	vd;
	float	vscale;

	self->s.skinnum = 0;
	self->s.frame = 0;
	VectorClear(self->mins);
	VectorClear(self->maxs);

	self->s.modelindex2 = 0;
	gi.setmodel(self, gibname);
	self->solid = SOLID_NOT;
	self->s.effects |= EF_GIB;
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;
	self->svflags &= ~SVF_MONSTER;
	self->takedamage = DAMAGE_YES;
	self->die = SG_Gib_Die;

	if (type == GIB_ORGANIC){
		self->movetype = MOVETYPE_TOSS;
		self->touch = SG_Gib_Touch;
		vscale = 0.5f;
	}
	else {
		self->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0f;
	}

	SG_GibVelocityForDamage(damage, vd);
	VectorMA(self->velocity, vscale, vd, self->velocity);
	SG_ClipGibVelocity(self);

	self->avelocity[YAW] = crandom() * 600.0f;

	self->think = SG_FreeEntity;
	self->nextthink = level.time + 10.0f + random() * 10.0f;

	gi.linkentity (self);
}

/*
 ==================
 SG_ThrowClientHead
 ==================
*/
void ThrowClientHead (edict_t *self, int damage){

	vec3_t	vd;
	char	*gibname;

	if (rand() & 1){
		gibname = "models/objects/gibs/head2/tris.md2";
		self->s.skinnum = 1;	// Second skin is player
	}
	else {
		gibname = "models/objects/gibs/skull/tris.md2";
		self->s.skinnum = 0;
	}

	self->s.origin[2] += 32.0f;
	self->s.frame = 0;
	gi.setmodel(self, gibname);
	VectorSet(self->mins, -16.0f, -16.0f, 0.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 16.0f);

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->s.effects = EF_GIB;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;

	self->movetype = MOVETYPE_BOUNCE;
	SG_GibVelocityForDamage(damage, vd);
	VectorAdd(self->velocity, vd, self->velocity);

	// Bodies in the queue don't have a client anymore
	if (self->client){
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = self->s.frame;
	}
	else {
		self->think = NULL;
		self->nextthink = 0.0f;
	}

	gi.linkentity(self);
}


/*
 ==============================================================================

 DEBRIS

 ==============================================================================
*/


/*
 ==================
 SG_Debris_Die
 ==================
*/
static void SG_Debris_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	SG_FreeEntity(self);
}

/*
 ==================
 SG_ThrowDebris
 ==================
*/
void ThrowDebris (edict_t *self, char *modelname, float speed, vec3_t origin){

	edict_t	*chunk;
	vec3_t	v;

	chunk = SG_AllocEntity();

	VectorCopy(origin, chunk->s.origin);
	gi.setmodel(chunk, modelname);
	v[0] = 100.0f * crandom();
	v[1] = 100.0f * crandom();
	v[2] = 100.0f + 100.0f * crandom();
	VectorMA(self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random() * 600.0f;
	chunk->avelocity[1] = random() * 600.0f;
	chunk->avelocity[2] = random() * 600.0f;
	chunk->think = SG_FreeEntity;
	chunk->nextthink = level.time + 5.0f + random() * 5.0f;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->className = "debris";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = SG_Debris_Die;

	gi.linkentity(chunk);
}


/*
 ==============================================================================

 PATH_CORNER

 ==============================================================================
*/


/*
 ==================
 SG_PathCorner_Touch
 ==================
*/
static void SG_PathCorner_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	char	*savetarget;
	vec3_t	v;
	edict_t	*next;

	if (other->moveTarget != self)
		return;

	if (other->enemy)
		return;

	if (self->pathtarget){
		savetarget = self->target;
		self->target = self->pathtarget;
		SG_UseTargets(self, other);
		self->target = savetarget;
	}

	if (self->target)
		next = SG_PickTarget(self->target);
	else
		next = NULL;

	if ((next) && (next->spawnflags & 1)){
		VectorCopy(next->s.origin, v);
		v[2] += next->mins[2];
		v[2] -= other->mins[2];
		VectorCopy(v, other->s.origin);
		next = SG_PickTarget(next->target);
		other->s.event = EV_OTHER_TELEPORT;
	}

	other->goalEntity = other->moveTarget = next;

	if (self->wait){
		other->monsterinfo.pausetime = level.time + self->wait;
		other->monsterinfo.stand(other);
		return;
	}

	if (!other->moveTarget){
		other->monsterinfo.pausetime = level.time + 100000000.0f;
		other->monsterinfo.stand(other);
	}
	else {
		VectorSubtract(other->goalEntity->s.origin, other->s.origin, v);
		other->idealYaw = SG_VectorToYaw(v);
	}
}

/*
 ==================
 SG_PathCorner_Touch
 ==================
*/
void SG_PathCorner_Spawn (edict_t *self){

	if (!self->targetname){
		gi.dprintf("path_corner with no targetname at %s\n", SG_VectorToString(self->s.origin));
		SG_FreeEntity(self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = SG_PathCorner_Touch;
	VectorSet(self->mins, -8.0f, -8.0f, -8.0f);
	VectorSet(self->maxs, 8.0f, 8.0f, 8.0f);
	self->svflags |= SVF_NOCLIENT;

	gi.linkentity(self);
}

/*
 ==============================================================================

 POINT_COMBAT

 ==============================================================================
*/


/*
 ==================
 SG_PointCombat_Touch
 ==================
*/
static void SG_PointCombat_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	edict_t	*activator;
	char	*savetarget;

	if (other->moveTarget != self)
		return;

	if (self->target){
		other->target = self->target;
		other->goalEntity = other->moveTarget = SG_PickTarget(other->target);

		if (!other->goalEntity){
			gi.dprintf("%s at %s target %s does not exist\n", self->className, SG_VectorToString(self->s.origin), self->target);
			other->moveTarget = self;
		}

		self->target = NULL;
	}
	else if ((self->spawnflags & 1) && !(other->flags & (FL_SWIM|FL_FLY))){
		other->monsterinfo.pausetime = level.time + 100000000.0f;
		other->monsterinfo.aiflags |= AI_STAND_GROUND;
		other->monsterinfo.stand(other);
	}

	if (other->moveTarget == self){
		other->target = NULL;
		other->moveTarget = NULL;
		other->goalEntity = other->enemy;
		other->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	}

	if (self->pathtarget){
		savetarget = self->target;
		self->target = self->pathtarget;

		if (other->enemy && other->enemy->client)
			activator = other->enemy;
		else if (other->oldenemy && other->oldenemy->client)
			activator = other->oldenemy;
		else if (other->activator && other->activator->client)
			activator = other->activator;
		else
			activator = other;

		SG_UseTargets(self, activator);
		self->target = savetarget;
	}
}

/*
 ==================
 SG_PointCombat_Spawn
 ==================
*/
void SG_PointCombat_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = SG_PointCombat_Touch;
	VectorSet(self->mins, -8.0f, -8.0f, -16.0f);
	VectorSet(self->maxs, 8.0f, 8.0f, 16.0f);
	self->svflags = SVF_NOCLIENT;

	gi.linkentity(self);
}


/*
 ==============================================================================

 VIEWTHING

 ==============================================================================
*/


/*
 ==================
 SG_ViewThing_Think
 ==================
*/
static void SG_ViewThing_Think (edict_t *entity){

	entity->s.frame = (entity->s.frame + 1) % 7;
	entity->nextthink = level.time + FRAMETIME;
}

/*
 ==================
 SG_ViewThing_Spawn

 Only used for debugging purposes
 ==================
*/
void SG_ViewThing_Spawn (edict_t *entity){

	gi.dprintf("viewthing spawned\n");

	entity->movetype = MOVETYPE_NONE;
	entity->solid = SOLID_BBOX;
	entity->s.renderfx = RF_FRAMELERP;
	VectorSet(entity->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(entity->maxs, 16.0f, 16.0f, 32.0f);
	entity->s.modelindex = gi.modelindex("models/objects/banner/tris.md2");

	gi.linkentity(entity);

	entity->nextthink = level.time + 0.5f;
	entity->think = SG_ViewThing_Think;

	return;
}


/*
 ==============================================================================

 INFO_NULLL

 ==============================================================================
*/


/*
 ==================
 SG_InfoNull_Spawn
 ==================
*/
void SG_InfoNull_Spawn (edict_t *self){

	SG_FreeEntity(self);
}


/*
 ==============================================================================

 INFO_NOTNULL

 ==============================================================================
*/


/*
 ==================
 SG_InfoNotNull_Spawn
 ==================
*/
void SG_InfoNotNull_Spawn (edict_t *self){

	VectorCopy(self->s.origin, self->absmin);
	VectorCopy(self->s.origin, self->absmax);
}


/*
 ==============================================================================

 LIGHT

 ==============================================================================
*/

#define START_OFF	1


/*
 ==================
 SG_Light_Trigger
 ==================
*/
static void SG_Light_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	if (self->spawnflags & START_OFF){
		gi.configstring(CS_LIGHTS+self->style, "m");
		self->spawnflags &= ~START_OFF;
	}
	else {
		gi.configstring(CS_LIGHTS+self->style, "a");
		self->spawnflags |= START_OFF;
	}
}

/*
 ==================
 SG_Light_Spawn
 ==================
*/
void SG_Light_Spawn (edict_t *self){

	// No targeted lights in deathmatch, because they cause global messages
	if (!self->targetname || deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	if (self->style >= 32){
		self->use = SG_Light_Trigger;

		if (self->spawnflags & START_OFF)
			gi.configstring(CS_LIGHTS+self->style, "a");
		else
			gi.configstring(CS_LIGHTS+self->style, "m");
	}
}


/*
 ==============================================================================

 LIGHT_MINE1

 ==============================================================================
*/


/*
 ==================
 SG_LightMine1_Spawn
 ==================
*/
void SG_LightMine1_Spawn (edict_t *self){

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/objects/minelite/light1/tris.md2");

	gi.linkentity(self);
}


/*
 ==============================================================================

 LIGHT_MINE2

 ==============================================================================
*/


/*
 ==================
 SG_LightMine2_Spawn
 ==================
*/
void SG_LightMine2_Spawn (edict_t *self){

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/objects/minelite/light2/tris.md2");

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_EXPLOBOX

 ==============================================================================
*/


/*
 ==================
 SG_MiscBarrel_Touch
 ==================
*/
static void SG_MiscBarrel_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	float	ratio;
	vec3_t	v;

	if ((!other->groundentity) || (other->groundentity == self))
		return;

	ratio = (float)other->mass / (float)self->mass;
	VectorSubtract(self->s.origin, other->s.origin, v);
	SG_MonsterMove(self, SG_VectorToYaw(v), 20.0f * ratio * FRAMETIME);
}

/*
 ==================
 SG_MiscBarrel_Think
 ==================
*/
static void SG_MiscBarrel_Think (edict_t *self){

	vec3_t	org;
	float	spd;
	vec3_t	save;

	SG_RadiusTargetDamage(self, self->activator, self->dmg, NULL, self->dmg + 40, MOD_BARREL);

	VectorCopy(self->s.origin, save);
	VectorMA(self->absmin, 0.5f, self->size, self->s.origin);

	// A few big chunks
	spd = 1.5f * (float)self->dmg / 200.0f;
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris1/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris1/tris.md2", spd, org);

	// Bottom corners
	spd = 1.75f * (float)self->dmg / 200.0f;
	VectorCopy(self->absmin, org);
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy(self->absmin, org);
	org[0] += self->size[0];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy(self->absmin, org);
	org[1] += self->size[1];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);
	VectorCopy(self->absmin, org);
	org[0] += self->size[0];
	org[1] += self->size[1];
	ThrowDebris(self, "models/objects/debris3/tris.md2", spd, org);

	// A bunch of little chunks
	spd = 2 * self->dmg / 200;
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org);
	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	ThrowDebris(self, "models/objects/debris2/tris.md2", spd, org);

	VectorCopy(save, self->s.origin);

	if (self->groundentity)
		SG_BecomeExplosion2(self);
	else
		SG_BecomeExplosion1(self);
}

/*
 ==================
 SG_MiscBarrel_Die
 ==================
*/
static void SG_MiscBarrel_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	self->takedamage = DAMAGE_NO;
	self->nextthink = level.time + 2.0f * FRAMETIME;
	self->think = SG_MiscBarrel_Think;
	self->activator = attacker;
}

/*
 ==================
 SG_MiscBarrel_Spawn
 ==================
*/
void SG_MiscBarrel_Spawn (edict_t *self){

	// Auto-remove for deathmatch
	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");
	gi.modelindex("models/objects/debris3/tris.md2");

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;

	self->model = "models/objects/barrels/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	VectorSet(self->mins, -16.0f, -16.0f, 0.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 40.0f);

	if (!self->mass)
		self->mass = 400;
	if (!self->health)
		self->health = 10;
	if (!self->dmg)
		self->dmg = 150;

	self->die = SG_MiscBarrel_Die;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.aiflags = AI_NOSTEP;

	self->touch = SG_MiscBarrel_Touch;

	self->think = SG_DropToFloor;
	self->nextthink = level.time + 2.0f * FRAMETIME;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_DEADSOLDIER

 ==============================================================================
*/


/*
 ==================
 SG_MiscDeadSoldier_Die
 ==================
*/
static void SG_MiscDeadSoldier_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	if (self->health > -80)
		return;

	gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1.0f, ATTN_NORM, 0.0f);

	ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

	ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
}

/*
 ==================
 SG_MiscDeadSoldier_Spawn
 ==================
*/
void SG_MiscDeadSoldier_Spawn (edict_t *self){

	// Auto-remove for deathmatch
	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/deadbods/dude/tris.md2");

	// Defaults to frame 0
	if (self->spawnflags & 2)
		self->s.frame = 1;
	else if (self->spawnflags & 4)
		self->s.frame = 2;
	else if (self->spawnflags & 8)
		self->s.frame = 3;
	else if (self->spawnflags & 16)
		self->s.frame = 4;
	else if (self->spawnflags & 32)
		self->s.frame = 5;
	else
		self->s.frame = 0;

	VectorSet(self->mins, -16.0f, -16.0f, 0.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 16.0f);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->svflags |= SVF_MONSTER|SVF_DEADMONSTER;
	self->die = SG_MiscDeadSoldier_Die;
	self->monsterinfo.aiflags |= AI_GOOD_GUY;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_VIPER

 ==============================================================================
*/


/*
 ==================
 SG_MiscViper_Touch
 ==================
*/
static void SG_MiscViper_Touch  (edict_t *self, edict_t *other, edict_t *activator){

	self->svflags &= ~SVF_NOCLIENT;
	self->use = SG_FuncTrain_Trigger;
	SG_FuncTrain_Trigger(self, other, activator);
}

/*
 ==================
 SG_MiscViper_Spawn
 ==================
*/
void SG_MiscViper_Spawn (edict_t *self){

	if (!self->target){
		gi.dprintf("misc_viper without a target at %s\n", SG_VectorToString(self->absmin));
		SG_FreeEntity(self);
		return;
	}

	if (!self->speed)
		self->speed = 300.0f;

	self->movetype = MOVETYPE_PUSH;
	self->solid = SOLID_NOT;
	self->s.modelindex = gi.modelindex("models/ships/viper/tris.md2");
	VectorSet(self->mins, -16.0f, -16.0f, 0.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);

	self->think = SG_FuncTrain_Find;
	self->nextthink = level.time + FRAMETIME;
	self->use = SG_MiscViper_Touch;
	self->svflags |= SVF_NOCLIENT;
	self->moveInfo.accel = self->moveInfo.decel = self->moveInfo.speed = self->speed;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_BLACKHOLE

 ==============================================================================
*/


/*
 ==================
 SG_MiscBlackhole_Trigger
 ==================
*/
static void SG_MiscBlackhole_Trigger (edict_t *entity, edict_t *other, edict_t *activator){

	SG_FreeEntity(entity);
}

/*
 ==================
 SG_MiscBlackhole_Think
 ==================
*/
static void SG_MiscBlackhole_Think (edict_t *self){

	if (++self->s.frame < 19)
		self->nextthink = level.time + FRAMETIME;
	else {		
		self->s.frame = 0;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
 ==================
 SG_MiscBlackhole_Spawn
 ==================
*/
void SG_MiscBlackhole_Spawn (edict_t *self){

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	VectorSet(self->mins, -64.0f, -64.0f, 0.0f);
	VectorSet(self->maxs, 64.0f, 64.0f, 8.0f);
	self->s.modelindex = gi.modelindex("models/objects/black/tris.md2");
	self->s.renderfx = RF_TRANSLUCENT;
	self->use = SG_MiscBlackhole_Trigger;
	self->think = SG_MiscBlackhole_Think;
	self->nextthink = level.time + 2.0f * FRAMETIME;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_EASTERTANK

 ==============================================================================
*/


/*
 ==================
 SG_MiscEastertank_Think
 ==================
*/
static void SG_MiscEastertank_Think (edict_t *self){

	if (++self->s.frame < 293)
		self->nextthink = level.time + FRAMETIME;
	else {		
		self->s.frame = 254;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
 ==================
 SG_MiscEastertank_Spawn
 ==================
*/
void SG_MiscEastertank_Spawn (edict_t *self){

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	VectorSet(self->mins, -32.0f, -32.0f, -16.0f);
	VectorSet(self->maxs, 32.0f, 32.0f, 32.0f);
	self->s.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
	self->s.frame = 254;
	self->think = SG_MiscEastertank_Think;
	self->nextthink = level.time + 2.0f * FRAMETIME;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_EASTERCHICK

 ==============================================================================
*/


/*
 ==================
 SG_MiscEasterchick_Think
 ==================
*/
static void SG_MiscEasterchick_Think (edict_t *self){

	if (++self->s.frame < 247)
		self->nextthink = level.time + FRAMETIME;
	else {		
		self->s.frame = 208;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
 ==================
 SG_MiscEasterchick_Spawn
 ==================
*/
void SG_MiscEasterchick_Spawn (edict_t *self){

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	VectorSet(self->mins, -32.0f, -32.0f, 0.0f);
	VectorSet(self->maxs, 32.0f, 32.0f, 32.0f);
	self->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	self->s.frame = 208;
	self->think = SG_MiscEasterchick_Think;
	self->nextthink = level.time + 2.0f * FRAMETIME;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_EASTERCHICK2

 ==============================================================================
*/


/*
 ==================
 SG_MiscEasterchick2_Think
 ==================
*/
static void SG_MiscEasterchick2_Think (edict_t *self){

	if (++self->s.frame < 287)
		self->nextthink = level.time + FRAMETIME;
	else {		
		self->s.frame = 248;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
 ==================
 SG_MiscEasterchick2_Spawn
 ==================
*/
void SG_MiscEasterchick2_Spawn (edict_t *self){

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	VectorSet(self->mins, -32.0f, -32.0f, 0.0f);
	VectorSet(self->maxs, 32.0f, 32.0f, 32.0f);
	self->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	self->s.frame = 248;
	self->think = SG_MiscEasterchick2_Think;
	self->nextthink = level.time + 2.0f * FRAMETIME;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_BANNER

 ==============================================================================
*/


/*
 ==================
 SG_MiscBanner_Think
 ==================
*/
static void SG_MiscBanner_Think (edict_t *self){

	self->s.frame = (self->s.frame + 1) % 16;
	self->nextthink = level.time + FRAMETIME;
}

/*
 ==================
 SG_MiscBanner_Spawn
 ==================
*/
void SG_MiscBanner_Spawn (edict_t *self){

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->s.modelindex = gi.modelindex("models/objects/banner/tris.md2");
	self->s.frame = rand() % 16;

	gi.linkentity(self);

	self->think = SG_MiscBanner_Think;
	self->nextthink = level.time + FRAMETIME;
}


/*
 ==============================================================================

 MISC_SATELLITE_DISH

 ==============================================================================
*/


/*
 ==================
 SG_MiscSatelliteDish_Think
 ==================
*/
static void SG_MiscSatelliteDish_Think (edict_t *self){

	self->s.frame++;
	if (self->s.frame < 38)
		self->nextthink = level.time + FRAMETIME;
}

/*
 ==================
 SG_MiscSatelliteDish_Trigger
 ==================
*/
static void SG_MiscSatelliteDish_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->s.frame = 0;
	self->think = SG_MiscSatelliteDish_Think;
	self->nextthink = level.time + FRAMETIME;
}

/*
 ==================
 SG_MiscSatelliteDish_Spawn
 ==================
*/
void SG_MiscSatelliteDish_Spawn (edict_t *self){

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	VectorSet(self->mins, -64.0f, -64.0f, 0.0f);
	VectorSet(self->maxs, 64.0f, 64.0f, 128.0f);
	self->s.modelindex = gi.modelindex("models/objects/satellite/tris.md2");
	self->use = SG_MiscSatelliteDish_Trigger;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_GIB_ARM

 ==============================================================================
*/


/*
 ==================
 SG_MiscGibArm_Spawn
 ==================
*/
void SG_MiscGibArm_Spawn (edict_t *self){

	gi.setmodel(self, "models/objects/gibs/arm/tris.md2");
	self->solid = SOLID_NOT;
	self->s.effects |= EF_GIB;
	self->takedamage = DAMAGE_YES;
	self->die = SG_Gib_Die;
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_MONSTER;
	self->deadflag = DEAD_DEAD;
	self->avelocity[0] = random() * 200.0f;
	self->avelocity[1] = random() * 200.0f;
	self->avelocity[2] = random() * 200.0f;
	self->think = SG_FreeEntity;
	self->nextthink = level.time + 30.0f;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_GIB_LEG

 ==============================================================================
*/


/*
 ==================
 SG_MiscGibLeg_Spawn
 ==================
*/
void SG_MiscGibLeg_Spawn (edict_t *self){

	gi.setmodel(self, "models/objects/gibs/leg/tris.md2");
	self->solid = SOLID_NOT;
	self->s.effects |= EF_GIB;
	self->takedamage = DAMAGE_YES;
	self->die = SG_Gib_Die;
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_MONSTER;
	self->deadflag = DEAD_DEAD;
	self->avelocity[0] = random() * 200.0f;
	self->avelocity[1] = random() * 200.0f;
	self->avelocity[2] = random() * 200.0f;
	self->think = SG_FreeEntity;
	self->nextthink = level.time + 30.0f;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_GIB_HEAD

 ==============================================================================
*/


/*
 ==================
 SG_MiscGibHead_Spawn
 ==================
*/
void SG_MiscGibHead_Spawn (edict_t *self){

	gi.setmodel(self, "models/objects/gibs/head/tris.md2");
	self->solid = SOLID_NOT;
	self->s.effects |= EF_GIB;
	self->takedamage = DAMAGE_YES;
	self->die = SG_Gib_Die;
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_MONSTER;
	self->deadflag = DEAD_DEAD;
	self->avelocity[0] = random() * 200.0f;
	self->avelocity[1] = random() * 200.0f;
	self->avelocity[2] = random() * 200.0f;
	self->think = SG_FreeEntity;
	self->nextthink = level.time + 30.0f;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_VIPER_BOMB

 ==============================================================================
*/


/*
 ==================
 SG_MiscViperBomb_Touch
 ==================
*/
static void SG_MiscViperBomb_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf){

	SG_UseTargets(self, self->activator);

	self->s.origin[2] = self->absmin[2] + 1.0f;
	SG_RadiusTargetDamage(self, self, self->dmg, NULL, self->dmg + 40, MOD_BOMB);
	SG_BecomeExplosion2(self);
}

/*
 ==================
 SG_MiscViperBomb_PreThink
 ==================
*/
static void SG_MiscViperBomb_PreThink (edict_t *self){

	float	diff;
	vec3_t	v;

	self->groundentity = NULL;

	diff = self->timestamp - level.time;
	if (diff < -1.0f)
		diff = -1.0f;

	VectorScale(self->moveInfo.dir, 1.0f + diff, v);
	v[2] = diff;

	diff = self->s.angles[2];
	SG_VectorToAngles(v, self->s.angles);
	self->s.angles[2] = diff + 10.0f;
}

/*
 ==================
 SG_MiscViperBomb_Trigger
 ==================
*/
static void SG_MiscViperBomb_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	edict_t	*viper;

	self->solid = SOLID_BBOX;
	self->svflags &= ~SVF_NOCLIENT;
	self->s.effects |= EF_ROCKET;
	self->use = NULL;
	self->movetype = MOVETYPE_TOSS;
	self->prethink = SG_MiscViperBomb_PreThink;
	self->touch = SG_MiscViperBomb_Touch;
	self->activator = activator;

	viper = SG_FindEntity(NULL, FOFS(className), "misc_viper");
	VectorScale(viper->moveInfo.dir, viper->moveInfo.speed, self->velocity);

	self->timestamp = level.time;
	VectorCopy(viper->moveInfo.dir, self->moveInfo.dir);
}

/*
 ==================
 SG_MiscViperBomb_Spawn
 ==================
*/
void SG_MiscViperBomb_Spawn (edict_t *self){

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	VectorSet(self->mins, -8.0f, -8.0f, -8.0f);
	VectorSet(self->maxs, 8.0f, 8.0f, 8.0f);

	self->s.modelindex = gi.modelindex("models/objects/bomb/tris.md2");

	if (!self->dmg)
		self->dmg = 1000;

	self->use = SG_MiscViperBomb_Trigger;
	self->svflags |= SVF_NOCLIENT;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_BIGVIPER

 ==============================================================================
*/


/*
 ==================
 SG_MiscBigViper_Spawn
 ==================
*/
void SG_MiscBigViper_Spawn (edict_t *ent){

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet(ent->mins, -176.0f, -120.0f, -24.0f);
	VectorSet(ent->maxs, 176.0f, 120.0f, 72.0f);
	ent->s.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");

	gi.linkentity(ent);
}


/*
 ==============================================================================

 MISC_STROGG_SHIP

 ==============================================================================
*/


/*
 ==================
 SG_MiscStroggShip_Trigger
 ==================
*/
static void SG_MiscStroggShip_Trigger  (edict_t *self, edict_t *other, edict_t *activator){

	self->svflags &= ~SVF_NOCLIENT;
	self->use = SG_FuncTrain_Trigger;
	SG_FuncTrain_Trigger(self, other, activator);
}

/*
 ==================
 SG_MiscStroggShip_Spawn
 ==================
*/
void SG_MiscStroggShip_Spawn (edict_t *self){

	if (!self->target){
		gi.dprintf("%s without a target at %s\n", self->className, SG_VectorToString(self->absmin));
		SG_FreeEntity(self);
		return;
	}

	if (!self->speed)
		self->speed = 300.0f;

	self->movetype = MOVETYPE_PUSH;
	self->solid = SOLID_NOT;
	self->s.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
	VectorSet(self->mins, -16.0f, -16.0f, 0.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);

	self->think = SG_FuncTrain_Find;
	self->nextthink = level.time + FRAMETIME;
	self->use = SG_MiscStroggShip_Trigger;
	self->svflags |= SVF_NOCLIENT;
	self->moveInfo.accel = self->moveInfo.decel = self->moveInfo.speed = self->speed;

	gi.linkentity(self);
}


/*
 ==============================================================================

 MISC_TELEPORTER

 ==============================================================================
*/


/*
 ==================
 SG_MiscTeleporter_Touch
 ==================
*/
static void SG_MiscTeleporter_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	edict_t	*dest;

	if (!other->client)
		return;

	dest = SG_FindEntity (NULL, FOFS(targetname), self->target);

	if (!dest){
		gi.dprintf("Couldn't find destination\n");
		return;
	}

	// Unlink to make sure it can't possibly interfere with SG_KillBox
	gi.unlinkentity(other);

	VectorCopy(dest->s.origin, other->s.origin);
	VectorCopy(dest->s.origin, other->s.old_origin);
	other->s.origin[2] += 10.0f;

	// Clear the velocity and hold them in place briefly
	VectorClear(other->velocity);
	other->client->ps.pmove.pm_time = 160 >> 3;		// Hold time
	other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	// Draw the teleport splash at source and on the player
	self->owner->s.event = EV_PLAYER_TELEPORT;
	other->s.event = EV_PLAYER_TELEPORT;

	// Set angles
	other->client->ps.pmove.delta_angles[0] = ANGLE2SHORT(dest->s.angles[0] - other->client->resp.cmd_angles[0]);
	other->client->ps.pmove.delta_angles[1] = ANGLE2SHORT(dest->s.angles[1] - other->client->resp.cmd_angles[1]);
	other->client->ps.pmove.delta_angles[2] = ANGLE2SHORT(dest->s.angles[2] - other->client->resp.cmd_angles[2]);

	VectorClear(other->s.angles);
	VectorClear(other->client->ps.viewangles);
	VectorClear(other->client->v_angle);

	// Kill anything at the destination
	SG_KillBox(other);

	gi.linkentity(other);
}

/*
 ==================
 SG_MiscTeleporter_Spawn
 ==================
*/
void SG_MiscTeleporter_Spawn (edict_t *self){

	edict_t	*trigger;

	if (!self->target){
		gi.dprintf("teleporter without a target.\n");
		SG_FreeEntity(self);
		return;
	}

	gi.setmodel(self, "models/objects/dmspot/tris.md2");
	self->s.skinnum = 1;
	self->s.effects = EF_TELEPORTER;
	self->s.sound = gi.soundindex("world/amb10.wav");
	self->solid = SOLID_BBOX;

	VectorSet(self->mins, -32.0f, -32.0f, -24.0f);
	VectorSet(self->maxs, 32.0f, 32.0f, -16.0f);

	gi.linkentity(self);

	trigger = SG_AllocEntity();
	trigger->touch = SG_MiscTeleporter_Touch;
	trigger->solid = SOLID_TRIGGER;
	trigger->target = self->target;
	trigger->owner = self;
	VectorCopy(self->s.origin, trigger->s.origin);
	VectorSet(trigger->mins, -8.0f, -8.0f, 8.0f);
	VectorSet(trigger->maxs, 8.0f, 8.0f, 24.0f);

	gi.linkentity(trigger);
	
}


/*
 ==============================================================================

 MISC_TELEPORTER_DEST

 ==============================================================================
*/


/*
 ==================
 SG_MiscTeleporterDest_Spawn
 ==================
*/
void SG_MiscTeleporterDest_Spawn (edict_t *self){

	gi.setmodel(self, "models/objects/dmspot/tris.md2");
	self->s.skinnum = 0;
	self->solid = SOLID_BBOX;
	VectorSet(self->mins, -32.0f, -32.0f, -24.0f);
	VectorSet(self->maxs, 32.0f, 32.0f, -16.0f);

	gi.linkentity(self);
}


/*
 ==============================================================================

 MONSTER_COMMANDER_BODY

 ==============================================================================
*/


/*
 ==================
 SG_MonsterCommanderBody_Think
 ==================
*/
static void SG_MonsterCommanderBody_Think (edict_t *self){

	if (++self->s.frame < 24)
		self->nextthink = level.time + FRAMETIME;
	else
		self->nextthink = 0.0f;

	if (self->s.frame == 22)
		gi.sound (self, CHAN_BODY, gi.soundindex("tank/thud.wav"), 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SG_MonsterCommanderBody_Trigger
 ==================
*/
static void SG_MonsterCommanderBody_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->think = SG_MonsterCommanderBody_Think;
	self->nextthink = level.time + FRAMETIME;
	gi.sound(self, CHAN_BODY, gi.soundindex("tank/pain.wav"), 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SG_MonsterCommanderBody_Think2
 ==================
*/
static void SG_MonsterCommanderBody_Think2 (edict_t *self){

	self->movetype = MOVETYPE_TOSS;
	self->s.origin[2] += 2.0f;
}

/*
 ==================
 SG_MonsterCommanderBody_Spawn
 ==================
*/
void SG_MonsterCommanderBody_Spawn (edict_t *self){

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->model = "models/monsters/commandr/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	VectorSet(self->mins, -32.0f, -32.0f, 0.0f);
	VectorSet(self->maxs, 32.0f, 32.0f, 48.0f);
	self->use = SG_MonsterCommanderBody_Trigger;
	self->takedamage = DAMAGE_YES;
	self->flags = FL_GODMODE;
	self->s.renderfx |= RF_FRAMELERP;

	gi.linkentity(self);

	gi.soundindex("tank/thud.wav");
	gi.soundindex("tank/pain.wav");

	self->think = SG_MonsterCommanderBody_Think2;
	self->nextthink = level.time + 5.0f * FRAMETIME;
}