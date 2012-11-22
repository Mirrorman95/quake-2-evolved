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
// sg_monster.c - Monster back-end
//

// TODO:
// - SG_MonsterCheckAttack


#include "g_local.h"


static void SG_MonsterStartWalking (edict_t *self);


/*
 ==============================================================================

 WEAPONS

 FIXME:
 Monsters should call these with a totally accurate direction
 and we can mess it up based on skill.  Spread should be for normal
 and we can tighten or loosen based on skill.  We could muck with
 the damages too, but I'm not sure that's such a good idea.

 ==============================================================================
*/


/*
 ==================
 SG_MonsterFireBullet
 ==================
*/
void SG_MonsterFireBullet (edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hSpread, int vSpread, int flashType){

	SG_FireBullet(self, start, dir, damage, kick, hSpread, vSpread, MOD_UNKNOWN);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashType);
	gi.multicast(start, MULTICAST_PVS);
}

/*
 ==================
 SG_MonsterFireShotgun
 ==================
*/
void SG_MonsterFireShotgun (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int kick, int hSpread, int vSpread, int count, int flashType){

	SG_FirePellet(self, start, aimDir, damage, kick, hSpread, vSpread, count, MOD_UNKNOWN);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashType);
	gi.multicast(start, MULTICAST_PVS);
}

/*
 ==================
 SG_MonsterFireBlaster
 ==================
*/
void SG_MonsterFireBlaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashType, int effect){

	SG_FireBlasterProjectile(self, start, dir, damage, speed, effect, false);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashType);
	gi.multicast(start, MULTICAST_PVS);
}

/*
 ==================
 SG_MonsterFireGrenade
 ==================
*/
void SG_MonsterFireGrenade (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int speed, int flashType){

	SG_FireGrenadeProjectile(self, start, aimDir, damage, speed, 2.5f, damage + 40);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashType);
	gi.multicast(start, MULTICAST_PVS);
}

/*
 ==================
 SG_MonsterFireRocket
 ==================
*/
void SG_MonsterFireRocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashType){

	SG_FireRocketProjectile(self, start, dir, damage, speed, damage + 20, damage);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashType);
	gi.multicast(start, MULTICAST_PVS);
}

/*
 ==================
 SG_MonsterFireRailgun
 ==================
*/
void SG_MonsterFireRailgun (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int kick, int flashType){

	SG_FireRailProjectile(self, start, aimDir, damage, kick);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashType);
	gi.multicast(start, MULTICAST_PVS);
}

/*
 ==================
 SG_MonsterFireBFG10K
 ==================
*/
void SG_MonsterFireBFG10K (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int speed, int kick, float damageRadius, int flashType){

	SG_FireBFGProjectile(self, start, aimDir, damage, speed, damageRadius);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flashType);
	gi.multicast(start, MULTICAST_PVS);
}


/*
 ==============================================================================

 VAILDATE ATTACK

 ==============================================================================
*/


/*
 ==================
 SG_MonsterCheckAttack
 ==================
*/
qboolean SG_MonsterCheckAttack (edict_t *self){

	vec3_t	spot1, spot2;
	trace_t	trace;
	int		enemyRange;
	float	enemyYaw;
	vec3_t	temp;
	float	chance;

	if (self->enemy->health > 0){
		// See if any entities are in the way of the shot
		VectorCopy(self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy(self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;

		trace = gi.trace(spot1, NULL, NULL, spot2, self, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_SLIME|CONTENTS_LAVA);

		// Do we have a clear shot?
		if (trace.ent != self->enemy)
			return false;
	}

	enemyRange = SG_EntityRange(self, self->enemy);
	VectorSubtract(self->enemy->s.origin, self->s.origin, temp);
	enemyYaw = SG_VectorToYaw(temp);

	self->idealYaw = enemyYaw;

	// Melee attack
	if (enemyRange == RANGE_MELEE){
		if (self->monsterinfo.melee)
			self->monsterinfo.attack_state = AS_MELEE;
		else
			self->monsterinfo.attack_state = AS_MISSILE;

		return true;
	}

	// Missile attack
	if (!self->monsterinfo.attack)
		return false;

	if (level.time < self->monsterinfo.attack_finished)
		return false;

	if (enemyRange == RANGE_FAR)
		return false;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		chance = 0.4f;
	else if (enemyRange == RANGE_MELEE)
		chance = 0.8f;
	else if (enemyRange == RANGE_NEAR)
		chance = 0.4f;
	else if (enemyRange == RANGE_MID)
		chance = 0.2f;
	else
		return false;

	if (random () < chance){
		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time + 2.0f * random();
		return true;
	}

	if (self->flags & FL_FLY){
		if (random() < 0.3f)
			self->monsterinfo.attack_state = AS_SLIDING;
		else
			self->monsterinfo.attack_state = AS_STRAIGHT;
	}

	return false;
}

/*
 ==================
 SG_MonsterCheckAttack2

 Uses more range chance
 ==================
*/
qboolean SG_MonsterCheckAttack2 (edict_t *self){

	vec3_t	spot1, spot2;
	trace_t	trace;
	int		enemyRange;
	float	enemyYaw;
	vec3_t	temp;
	float	chance;

	if (self->enemy->health > 0){
		// See if any entities are in the way of the shot
		VectorCopy(self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy(self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;

		trace = gi.trace(spot1, NULL, NULL, spot2, self, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_SLIME|CONTENTS_LAVA);

		// Do we have a clear shot?
		if (trace.ent != self->enemy)
			return false;
	}

	enemyRange = SG_EntityRange(self, self->enemy);
	VectorSubtract(self->enemy->s.origin, self->s.origin, temp);
	enemyYaw = SG_VectorToYaw(temp);

	self->idealYaw = enemyYaw;

	// Melee attack
	if (enemyRange == RANGE_MELEE){
		if (self->monsterinfo.melee)
			self->monsterinfo.attack_state = AS_MELEE;
		else
			self->monsterinfo.attack_state = AS_MISSILE;

		return true;
	}

	// Missile attack
	if (!self->monsterinfo.attack)
		return false;

	if (level.time < self->monsterinfo.attack_finished)
		return false;

	if (enemyRange == RANGE_FAR)
		return false;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		chance = 0.4f;
	else if (enemyRange == RANGE_MELEE)
		chance = 0.8f;
	else if (enemyRange == RANGE_NEAR)
		chance = 0.8f;
	else if (enemyRange == RANGE_MID)
		chance = 0.8f;
	else
		return false;

	if (random () < chance){
		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time + 2.0f * random();
		return true;
	}

	if (self->flags & FL_FLY){
		if (random() < 0.3f)
			self->monsterinfo.attack_state = AS_SLIDING;
		else
			self->monsterinfo.attack_state = AS_STRAIGHT;
	}

	return false;
}


/*
 ==============================================================================

 MONSTER EXPLODE

 ==============================================================================
*/


/*
 ==================
 SG_MonsterExplode
 ==================
*/
void SG_MonsterExplode (edict_t *self){

	vec3_t	origin;

	self->think = SG_MonsterExplode;
	VectorCopy(self->s.origin, origin);
	origin[2] += 24.0f + (rand() & 15);

	switch (self->count++){
	case 0:
		origin[0] -= 24.0f;
		origin[1] -= 24.0f;
		break;
	case 1:
		origin[0] += 24.0f;
		origin[1] += 24.0f;
		break;
	case 2:
		origin[0] += 24.0f;
		origin[1] -= 24.0f;
		break;
	case 3:
		origin[0] -= 24.0f;
		origin[1] += 24.0f;
		break;
	case 4:
		origin[0] -= 48.0f;
		origin[1] -= 48.0f;
		break;
	case 5:
		origin[0] += 48.0f;
		origin[1] += 48.0f;
		break;
	case 6:
		origin[0] -= 48.0f;
		origin[1] += 48.0f;
		break;
	case 7:
		origin[0] += 48.0f;
		origin[1] -= 48.0f;
		break;
	case 8:
		self->s.sound = 0;

		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", 500, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", 500, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", 500, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", 500, GIB_ORGANIC);

		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);

		ThrowGib(self, "models/objects/gibs/chest/tris.md2", 500, GIB_ORGANIC);

		ThrowHead(self, "models/objects/gibs/gear/tris.md2", 500, GIB_METALLIC);

		self->deadflag = DEAD_DEAD;
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	self->nextthink = level.time + 0.1f;
}


/*
 ==============================================================================

 FLIES EFFECT

 ==============================================================================
*/


/*
 ==================
 SG_FliesOff
 ==================
*/
static void SG_FliesOff (edict_t *self){

	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
}

/*
 ==================
 SG_FliesOn
 ==================
*/
static void SG_FliesOn (edict_t *self){

	if (self->waterLevel)
		return;

	self->s.effects |= EF_FLIES;
	self->s.sound = gi.soundindex("infantry/inflies1.wav");
	self->think = SG_FliesOff;
	self->nextthink = level.time + 60.0f;
}

/*
 ==================
 SG_FlyCheck
 ==================
*/
void SG_FlyCheck (edict_t *self){

	if (self->waterLevel)
		return;

	if (random() > 0.5f)
		return;

	self->think = SG_FliesOn;
	self->nextthink = level.time + 5.0f + 10.0f * random();
}


/*
 ==============================================================================

 FRAMEWORK

 ==============================================================================
*/


/*
 ==================
 SG_MoveFrameAnimation
 ==================
*/
static void SG_MoveFrameAnimation (edict_t *self){

	mmove_t	*move;
	int		index;

	move = self->monsterinfo.currentmove;
	self->nextthink = level.time + FRAMETIME;

	if ((self->monsterinfo.nextframe) && (self->monsterinfo.nextframe >= move->firstframe) && (self->monsterinfo.nextframe <= move->lastframe)){
		self->s.frame = self->monsterinfo.nextframe;
		self->monsterinfo.nextframe = 0;
	}
	else {
		if (self->s.frame == move->lastframe){
			if (move->endfunc){
				move->endfunc(self);

				// Regrab move, endfunc is very likely to change it
				move = self->monsterinfo.currentmove;

				// Check for death
				if (self->svflags & SVF_DEADMONSTER)
					return;
			}
		}

		if (self->s.frame < move->firstframe || self->s.frame > move->lastframe){
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			self->s.frame = move->firstframe;
		}
		else {
			if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME)){
				self->s.frame++;
				if (self->s.frame > move->lastframe)
					self->s.frame = move->firstframe;
			}
		}
	}

	index = self->s.frame - move->firstframe;
	if (move->frame[index].aifunc){
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			move->frame[index].aifunc(self, move->frame[index].dist * self->monsterinfo.scale);
		else
			move->frame[index].aifunc(self, 0.0f);
	}

	if (move->frame[index].thinkfunc)
		move->frame[index].thinkfunc(self);
}

/*
 ==================
 SG_CheckGround
 ==================
*/
void SG_CheckGround (edict_t *entity){

	vec3_t	point;
	trace_t	trace;

	if (entity->flags & (FL_SWIM|FL_FLY))
		return;

	if (entity->velocity[2] > 100.0f){
		entity->groundentity = NULL;
		return;
	}

	// If the hull point one-quarter unit down is solid the entity is on ground
	point[0] = entity->s.origin[0];
	point[1] = entity->s.origin[1];
	point[2] = entity->s.origin[2] - 0.25f;

	trace = gi.trace(entity->s.origin, entity->mins, entity->maxs, point, entity, MASK_MONSTERSOLID);

	// Check steepness
	if (trace.plane.normal[2] < 0.7f && !trace.startsolid){
		entity->groundentity = NULL;
		return;
	}

	if (!trace.startsolid && !trace.allsolid){
		VectorCopy(trace.endpos, entity->s.origin);
		entity->groundentity = trace.ent;
		entity->groundentity_linkcount = trace.ent->linkcount;
		entity->velocity[2] = 0.0f;
	}
}

/*
 ==================
 SG_GetWaterLevel
 ==================
*/
static void SG_GetWaterLevel (edict_t *ent){

	vec3_t	point;
	int		content;

	// Find out the water level
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] + ent->mins[2] + 1.0f;	
	content = gi.pointcontents(point);

	if (!(content & MASK_WATER)){
		ent->waterLevel = WATERLEVEL_NONE;
		ent->waterType = 0;
		return;
	}

	ent->waterType = content;
	ent->waterLevel = WATERLEVEL_FEET;
	point[2] += 26.0f;

	content = gi.pointcontents(point);
	if (!(content & MASK_WATER))
		return;

	ent->waterLevel = WATERLEVEL_WAIST;
	point[2] += 22.0f;

	content = gi.pointcontents(point);
	if (content & MASK_WATER)
		ent->waterLevel = WATERLEVEL_HEAD;
}

/*
 ==================
 SG_WorldContentEffects
 ==================
*/
static void SG_WorldContentEffects (edict_t *entity){

	int		dmg;

	// Water content damage
	if (entity->health > 0){
		if (!(entity->flags & FL_SWIM)){
			if (entity->waterLevel < WATERLEVEL_HEAD)
				entity->air_finished = level.time + 12;
			else if (entity->air_finished < level.time){
				// Drown!
				if (entity->pain_debounce_time < level.time){
					dmg = 2 + 2 * floor(level.time - entity->air_finished);
					if (dmg > 15)
						dmg = 15;

					SG_TargetDamage(entity, world, world, vec3_origin, entity->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					entity->pain_debounce_time = level.time + 1.0f;
				}
			}
		}
		else {
			if (entity->waterLevel > WATERLEVEL_NONE)
				entity->air_finished = level.time + 9.0f;
			else if (entity->air_finished < level.time){
				// Suffocate!
				if (entity->pain_debounce_time < level.time){
					dmg = 2 + 2 * floor(level.time - entity->air_finished);
					if (dmg > 15)
						dmg = 15;

					SG_TargetDamage(entity, world, world, vec3_origin, entity->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					entity->pain_debounce_time = level.time + 1.0f;
				}
			}
		}
	}
	
	// Exiting water sound
	if (entity->waterLevel == WATERLEVEL_NONE){
		if (entity->flags & FL_INWATER){	
			gi.sound(entity, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1.0f, ATTN_NORM, 0);
			entity->flags &= ~FL_INWATER;
		}

		return;
	}

	// Lava and slime content damage
	if ((entity->waterType & CONTENTS_LAVA) && !(entity->flags & FL_IMMUNE_LAVA)){
		if (entity->damage_debounce_time < level.time){
			entity->damage_debounce_time = level.time + 0.2f;
			SG_TargetDamage(entity, world, world, vec3_origin, entity->s.origin, vec3_origin, 10 * entity->waterLevel, 0, 0, MOD_LAVA);
		}
	}

	if ((entity->waterType & CONTENTS_SLIME) && !(entity->flags & FL_IMMUNE_SLIME)){
		if (entity->damage_debounce_time < level.time){
			entity->damage_debounce_time = level.time + 1.0f;
			SG_TargetDamage(entity, world, world, vec3_origin, entity->s.origin, vec3_origin, 4 * entity->waterLevel, 0, 0, MOD_SLIME);
		}
	}
	
	// Content interaction sounds
	if (!(entity->flags & FL_INWATER)){	
		if (!(entity->svflags & SVF_DEADMONSTER)){
			if (entity->waterType & CONTENTS_LAVA){
				if (random() <= 0.5f)
					gi.sound(entity, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1.0f, ATTN_NORM, 0.0f);
				else
					gi.sound(entity, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1.0f, ATTN_NORM, 0.0f);
			}
			else if (entity->waterType & CONTENTS_SLIME)
				gi.sound(entity, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1.0f, ATTN_NORM, 0.0f);
			else if (entity->waterType & CONTENTS_WATER)
				gi.sound(entity, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1.0f, ATTN_NORM, 0.0f);
		}

		entity->flags |= FL_INWATER;
		entity->damage_debounce_time = 0;
	}
}

/*
 ==================
 SG_SetMonsterEffects
 ==================
*/
static void SG_SetMonsterEffects (edict_t *entity){

	// Clear all effects
	entity->s.effects &= ~(EF_COLOR_SHELL|EF_POWERSCREEN);
	entity->s.renderfx &= ~(RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);

	// Resurrecting effect
	if (entity->monsterinfo.aiflags & AI_RESURRECTING){
		entity->s.effects |= EF_COLOR_SHELL;
		entity->s.renderfx |= RF_SHELL_RED;
	}

	if (entity->health <= 0)
		return;

	// Armor effects
	if (entity->powerarmor_time > level.time){
		if (entity->monsterinfo.power_armor_type == POWER_ARMOR_SCREEN)
			entity->s.effects |= EF_POWERSCREEN;
		else if (entity->monsterinfo.power_armor_type == POWER_ARMOR_SHIELD){
			entity->s.effects |= EF_COLOR_SHELL;
			entity->s.renderfx |= RF_SHELL_GREEN;
		}
	}
}

/*
 ==================
 SG_MonsterThinkFramework
 ==================
*/
static void SG_MonsterThinkFramework (edict_t *self){

	SG_MoveFrameAnimation(self);

	if (self->linkcount != self->monsterinfo.linkcount){
		self->monsterinfo.linkcount = self->linkcount;
		SG_CheckGround(self);
	}

	SG_GetWaterLevel(self);
	SG_WorldContentEffects(self);
	SG_SetMonsterEffects(self);
}


/*
 ==============================================================================

 TRIGGER ACTION

 ==============================================================================
*/


/*
 ==================
 SG_MonsterTriggeredSpawn
 ==================
*/
static void SG_MonsterTriggeredSpawn (edict_t *self){

	self->s.origin[2] += 1.0f;
	SG_KillBox(self);

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12.0f;

	gi.linkentity(self);

	SG_MonsterStartWalking(self);

	if (self->enemy && !(self->spawnflags & 1) && !(self->enemy->flags & FL_NOTARGET))
		SG_AIFoundTarget(self);
	else
		self->enemy = NULL;
}

/*
 ==================
 SG_MonsterTriggeredSpawnUse
 ==================
*/
static void SG_MonsterTriggeredSpawnUse (edict_t *self, edict_t *other, edict_t *activator){

	// We have a one frame delay here so we don't telefrag the guy who activated us
	self->think = SG_MonsterTriggeredSpawn;
	self->nextthink = level.time + FRAMETIME;

	if (activator->client)
		self->enemy = activator;

	self->use = SG_MonsterUse;
}

/*
 ==================
 SG_MonsterTriggeredStart
 ==================
*/
static void SG_MonsterTriggeredStart (edict_t *self){

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0.0f;
	self->use = SG_MonsterTriggeredSpawnUse;
}


/*
 ==============================================================================

 ACTIONS

 ==============================================================================
*/


/*
 ==================
 SG_MonsterUse

 Using a monster makes it angry at the current activator
 ==================
*/
void SG_MonsterUse (edict_t *self, edict_t *other, edict_t *activator){

	if (self->enemy)
		return;
	if (self->health <= 0)
		return;
	if (activator->flags & FL_NOTARGET)
		return;
	if (!(activator->client) && !(activator->monsterinfo.aiflags & AI_GOOD_GUY))
		return;

	// Delay reaction so if the monster is teleported, it's sound is still heard
	self->enemy = activator;
	SG_AIFoundTarget(self);
}

/*
 ==================
 SG_MonsterDeathUse

 When a monster dies, it fires all of its targets with the current
 enemy as activator.
 ==================
*/
void SG_MonsterDeathUse (edict_t *self){

	self->flags &= ~(FL_FLY|FL_SWIM);
	self->monsterinfo.aiflags &= AI_GOOD_GUY;

	if (self->item){
		SG_Drop_Item(self, self->item);
		self->item = NULL;
	}

	if (self->deathtarget)
		self->target = self->deathtarget;

	if (!self->target)
		return;

	SG_UseTargets(self, self->enemy);
}

/*
 ==================
 SG_MonsterStartActing
 ==================
*/
static qboolean SG_MonsterStartActing (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return false;
	}

	if ((self->spawnflags & 4) && !(self->monsterinfo.aiflags & AI_GOOD_GUY)){
		self->spawnflags &= ~4;
		self->spawnflags |= 1;
	}

	if (!(self->monsterinfo.aiflags & AI_GOOD_GUY))
		level.total_monsters++;

	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_MONSTER;
	self->s.renderfx |= RF_FRAMELERP;
	self->takedamage = DAMAGE_AIM;
	self->air_finished = level.time + 12.0f;
	self->use = SG_MonsterUse;
	self->max_health = self->health;
	self->clipmask = MASK_MONSTERSOLID;

	self->s.skinnum = 0;
	self->deadflag = DEAD_NO;
	self->svflags &= ~SVF_DEADMONSTER;

	if (!self->monsterinfo.checkattack)
		self->monsterinfo.checkattack = SG_AICheckMonsterAttack;

	VectorCopy(self->s.origin, self->s.old_origin);

	if (st.item){
		self->item = SG_FindItemByClassname(st.item);
		if (!self->item)
			gi.dprintf("%s at %s has bad item: %s\n", self->className, SG_VectorToString(self->s.origin), st.item);
	}

	// Randomize what frame they start on
	if (self->monsterinfo.currentmove)
		self->s.frame = self->monsterinfo.currentmove->firstframe + (rand() % (self->monsterinfo.currentmove->lastframe - self->monsterinfo.currentmove->firstframe + 1));

	return true;
}

/*
 ==================
 SG_MonsterStartWalking
 ==================
*/
static void SG_MonsterStartWalking (edict_t *self){

	qboolean	notCombat;
	qboolean	fixUp;
	edict_t		*target;
	vec3_t		v;

	if (self->health <= 0)
		return;

	// Check for target to combat_point and change to combat target
	if (self->target){
		target = NULL;
		notCombat = false;
		fixUp = false;

		while ((target = SG_FindEntity(target, FOFS(targetname), self->target)) != NULL){
			if (!strcmp(target->className, "point_combat")){
				self->combattarget = self->target;
				fixUp = true;
			}
			else
				notCombat = true;
		}

		if (notCombat && self->combattarget)
			gi.dprintf("%s at %s has target with mixed types\n", self->className, SG_VectorToString(self->s.origin));

		if (fixUp)
			self->target = NULL;
	}

	// Validate combat target
	if (self->combattarget){
		target = NULL;

		while ((target = SG_FindEntity(target, FOFS(targetname), self->combattarget)) != NULL){
			if (strcmp(target->className, "point_combat") != 0){
				gi.dprintf("%s at (%i %i %i) has a bad combattarget %s : %s at (%i %i %i)\n",
					self->className, (int)self->s.origin[0], (int)self->s.origin[1], (int)self->s.origin[2],
					self->combattarget, target->className, (int)target->s.origin[0], (int)target->s.origin[1],
					(int)target->s.origin[2]);
			}
		}
	}

	if (self->target){
		self->goalEntity = self->moveTarget = SG_PickTarget(self->target);

		if (!self->moveTarget){
			gi.dprintf("%s can't find target %s at %s\n", self->className, self->target, SG_VectorToString(self->s.origin));

			self->target = NULL;
			self->monsterinfo.pausetime = 100000000.0f;
			self->monsterinfo.stand(self);
		}
		else if (!strcmp(self->moveTarget->className, "path_corner")){
			VectorSubtract(self->goalEntity->s.origin, self->s.origin, v);
			self->idealYaw = self->s.angles[YAW] = SG_VectorToYaw(v);
			self->monsterinfo.walk(self);
			self->target = NULL;
		}
		else {
			self->goalEntity = self->moveTarget = NULL;
			self->monsterinfo.pausetime = 100000000.0f;
			self->monsterinfo.stand(self);
		}
	}
	else {
		self->monsterinfo.pausetime = 100000000.0f;
		self->monsterinfo.stand(self);
	}

	self->think = SG_MonsterThinkFramework;
	self->nextthink = level.time + FRAMETIME;
}


/*
 ==============================================================================

 GROUND MONSTER ACTION

 ==============================================================================
*/


/*
 ==================
 SG_GroundMonsterThinkFramework
 ==================
*/
static void SG_GroundMonsterThinkFramework (edict_t *self){

	if (!(self->spawnflags & 2) && level.time < 1.0f){
		SG_DropToFloor(self);

		if (self->groundentity){
			if (!SG_MonsterMove(self, 0.0f, 0.0f))
				gi.dprintf("%s in solid at %s\n", self->className, SG_VectorToString(self->s.origin));
		}
	}
	
	if (!self->yawSpeed)
		self->yawSpeed = 20.0f;

	self->viewheight = 25;

	SG_MonsterStartWalking(self);

	if (self->spawnflags & 2)
		SG_MonsterTriggeredStart(self);
}

/*
 ==================
 SG_GroundMonster
 ==================
*/
void SG_GroundMonster (edict_t *self){

	self->think = SG_GroundMonsterThinkFramework;
	SG_MonsterStartActing(self);
}


/*
 ==============================================================================

 AIR MONSTER ACTION

 ==============================================================================
*/


/*
 ==================
 SG_AirMonsterThinkFramework
 ==================
*/
static void SG_AirMonsterThinkFramework (edict_t *self){

	if (!SG_MonsterMove(self, 0.0f, 0.0f))
		gi.dprintf("%s in solid at %s\n", self->className, SG_VectorToString(self->s.origin));

	if (!self->yawSpeed)
		self->yawSpeed = 10.0f;

	self->viewheight = 25;

	SG_MonsterStartWalking(self);

	if (self->spawnflags & 2)
		SG_MonsterTriggeredStart(self);
}

/*
 ==================
 SG_AirMonster
 ==================
*/
void SG_AirMonster (edict_t *self){

	self->flags |= FL_FLY;
	self->think = SG_AirMonsterThinkFramework;

	SG_MonsterStartActing(self);
}


/*
 ==============================================================================

 WATER MONSTER ACTION

 ==============================================================================
*/


/*
 ==================
 SG_WaterMonsterThinkFramework
 ==================
*/
static void SG_WaterMonsterThinkFramework (edict_t *self){

	if (!self->yawSpeed)
		self->yawSpeed = 10.0f;

	self->viewheight = 10;

	SG_MonsterStartWalking(self);

	if (self->spawnflags & 2)
		SG_MonsterTriggeredStart(self);
}

/*
 ==================
 SG_WaterMonster
 ==================
*/
void SG_WaterMonster (edict_t *self){

	self->flags |= FL_SWIM;
	self->think = SG_WaterMonsterThinkFramework;

	SG_MonsterStartActing(self);
}


/*
 ==============================================================================

 MISCELLANEOUS FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 SG_AttackFinished
 ==================
*/
void SG_AttackFinished (edict_t *self, float time){

	self->monsterinfo.attack_finished = level.time + time;
}

/*
 ==================
 SG_DropToFloor
 ==================
*/
void SG_DropToFloor (edict_t *entity){

	vec3_t		end;
	trace_t		trace;

	entity->s.origin[2] += 1.0f;
	VectorCopy(entity->s.origin, end);
	end[2] -= 256.0f;
	
	trace = gi.trace(entity->s.origin, entity->mins, entity->maxs, end, entity, MASK_MONSTERSOLID);

	if (trace.fraction == 1.0f || trace.allsolid)
		return;

	VectorCopy(trace.endpos, entity->s.origin);

	gi.linkentity(entity);
	SG_CheckGround(entity);
	SG_GetWaterLevel(entity);
}