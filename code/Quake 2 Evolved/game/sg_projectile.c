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
// sg_projectile.c - Weapon projectiles
//


#include "g_local.h"


/*
 ==================
 SG_CheckDodge

 This is a support routine used when a client is firing
 a non-instant attack weapon.  It checks to see if a
 monster's dodge function should be called.
 ==================
*/
static void SG_CheckDodge (edict_t *self, vec3_t start, vec3_t dir, int speed){

	vec3_t	end;
	trace_t	trace;
	vec3_t	v;
	float	eta;

	// Easy mode only ducks one quarter the time
	if (skill->value == 0){
		if (random() > 0.25f)
			return;
	}

	VectorMA(start, 8192.0f, dir, end);

	trace = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	if ((trace.ent) && (trace.ent->svflags & SVF_MONSTER) && (trace.ent->health > 0) && (trace.ent->monsterinfo.dodge) && SG_IsEntityInFront(trace.ent, self)){
		VectorSubtract(trace.endpos, start, v);
		eta = (VectorLength(v) - trace.ent->maxs[0]) / speed;
		trace.ent->monsterinfo.dodge(trace.ent, self, eta);
	}
}


/*
 ==============================================================================

 WEAPON HIT TYPES

 ==============================================================================
*/


/*
 ==================
 SG_MeleeHit
 ==================
*/
qboolean SG_MeleeHit (edict_t *self, vec3_t aim, int damage, int kick){

	trace_t		trace;
	vec3_t		forward, right, up;
	vec3_t		v;
	vec3_t		point;
	float		range;
	vec3_t		dir;

	// See if enemy is in range
	VectorSubtract (self->enemy->s.origin, self->s.origin, dir);

	range = VectorLength(dir);
	if (range > aim[0])
		return false;

	// The hit is straight on so back the range up to the edge of their bbox
	if (aim[1] > self->mins[0] && aim[1] < self->maxs[0])
		range -= self->enemy->maxs[0];
	else {
		// This is a side hit so adjust the "right" value out to the edge of their bbox
		if (aim[1] < 0)
			aim[1] = self->enemy->mins[0];
		else
			aim[1] = self->enemy->maxs[0];
	}

	VectorMA(self->s.origin, range, dir, point);

	trace = gi.trace(self->s.origin, NULL, NULL, point, self, MASK_SHOT);

	if (trace.fraction < 1.0f){
		if (!trace.ent->takedamage)
			return false;

		// If it will hit any client/monster then hit the one we wanted to hit
		if ((trace.ent->svflags & SVF_MONSTER) || (trace.ent->client))
			trace.ent = self->enemy;
	}

	AngleVectors(self->s.angles, forward, right, up);
	VectorMA(self->s.origin, range, forward, point);
	VectorMA(point, aim[1], right, point);
	VectorMA(point, aim[2], up, point);
	VectorSubtract(point, self->enemy->s.origin, dir);

	// Do the damage
	SG_TargetDamage(trace.ent, self, self, dir, point, vec3_origin, damage, kick / 2, DAMAGE_NO_KNOCKBACK, MOD_HIT);

	if (!(trace.ent->svflags & SVF_MONSTER) && (!trace.ent->client))
		return false;

	// Do our special form of knockback here
	VectorMA(self->enemy->absmin, 0.5f, self->enemy->size, v);
	VectorSubtract(v, point, v);
	VectorNormalize(v);
	VectorMA (self->enemy->velocity, kick, v, self->enemy->velocity);

	if (self->enemy->velocity[2] > 0.0f)
		self->enemy->groundentity = NULL;

	return true;
}

/*
 ==================
 SG_MissleHit

 This is an internal support routine used for bullet/pellet based weapons
 ==================
*/
static void SG_MissleHit (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, int mod){

	trace_t		trace;
	vec3_t		dir;
	vec3_t		forward, right, up;
	vec3_t		end;
	float		r;
	float		u;
	vec3_t		water_start;
	qboolean	water = false;
	int			content_mask = MASK_SHOT | MASK_WATER;

	trace = gi.trace (self->s.origin, NULL, NULL, start, self, MASK_SHOT);

	if (!(trace.fraction < 1.0f)){
		SG_VectorToAngles(aimdir, dir);
		AngleVectors(dir, forward, right, up);

		r = crandom() * hspread;
		u = crandom() * vspread;

		VectorMA(start, 8192.0f, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		if (gi.pointcontents(start) & MASK_WATER){
			water = true;

			VectorCopy(start, water_start);
			content_mask &= ~MASK_WATER;
		}

		trace = gi.trace(start, NULL, NULL, end, self, content_mask);

		// See if we hit water
		if (trace.contents & MASK_WATER){
			int		color;

			water = true;
			VectorCopy(trace.endpos, water_start);

			if (!VectorCompare(start, trace.endpos)){
				if (trace.contents & CONTENTS_WATER){
					if (!strcmp(trace.surface->name, "*brwater"))
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (trace.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (trace.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN){
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(TE_SPLASH);
					gi.WriteByte(8);
					gi.WritePosition(trace.endpos);
					gi.WriteDir(trace.plane.normal);
					gi.WriteByte(color);
					gi.multicast(trace.endpos, MULTICAST_PVS);
				}

				// Change bullet's course when it enters water
				VectorSubtract(end, start, dir);
				SG_VectorToAngles(dir, dir);
				AngleVectors(dir, forward, right, up);

				r = crandom() * hspread * 2;
				u = crandom() * vspread * 2;

				VectorMA(water_start, 8192.0f, forward, end);
				VectorMA(end, r, right, end);
				VectorMA(end, u, up, end);
			}

			// Re-trace ignoring water this time
			trace = gi.trace(water_start, NULL, NULL, end, self, MASK_SHOT);
		}
	}

	// Send gun puff / flash
	if (!((trace.surface) && (trace.surface->flags & SURF_SKY))){
		if (trace.fraction < 1.0f){
			if (trace.ent->takedamage)
				SG_TargetDamage(trace.ent, self, self, aimdir, trace.endpos, trace.plane.normal, damage, kick, DAMAGE_BULLET, mod);
			else {
				if (strncmp (trace.surface->name, "sky", 3) != 0){
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(te_impact);
					gi.WritePosition(trace.endpos);
					gi.WriteDir(trace.plane.normal);
					gi.multicast(trace.endpos, MULTICAST_PVS);

					if (self->client)
						SG_PlayerNoise(self, trace.endpos, PNOISE_IMPACT);
				}
			}
		}
	}

	// If went through water, determine where the end and make a bubble trail
	if (water){
		vec3_t	pos;

		VectorSubtract(trace.endpos, water_start, dir);
		VectorNormalize(dir);
		VectorMA(trace.endpos, -2.0f, dir, pos);

		if (gi.pointcontents(pos) & MASK_WATER)
			VectorCopy(pos, trace.endpos);
		else
			trace = gi.trace(pos, NULL, NULL, water_start, trace.ent, MASK_WATER);

		VectorAdd(water_start, trace.endpos, pos);
		VectorScale(pos, 0.5f, pos);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BUBBLETRAIL);
		gi.WritePosition(water_start);
		gi.WritePosition(trace.endpos);
		gi.multicast(pos, MULTICAST_PVS);
	}
}

/*
 ==================
 SG_FireBullet

 Fires a single round.  Used for machinegun and chaingun.  Would be fine for
 pistols, rifles, etc....
 ==================
*/
void SG_FireBullet (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int kick, int hSpread, int vSpread, int mod){

	SG_MissleHit(self, start, aimDir, damage, kick, TE_GUNSHOT, hSpread, vSpread, mod);
}

/*
 ==================
 SG_FirePellet

 Shoots shotgun pellets. Used by shotgun and super shotgun.
 ==================
*/
void SG_FirePellet (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int kick, int hSpread, int vSpread, int count, int mod){

	int		i;

	for (i = 0; i < count; i++)
		SG_MissleHit(self, start, aimDir, damage, kick, TE_SHOTGUN, hSpread, vSpread, mod);
}


/*
 ==============================================================================

 BLASTER PROJECTILE

 ==============================================================================
*/


/*
 ==================
 SG_BlasterTouch

 Fires a single blaster bolt. Used by the blaster and hyper blaster
 ==================
*/
static void SG_BlasterTouch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	int		mod;

	if (other == self->owner)
		return;

	if (surface && (surface->flags & SURF_SKY)){
		SG_FreeEntity(self);
		return;
	}

	if (self->owner->client)
		SG_PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage){
		if (self->spawnflags & 1)
			mod = MOD_HYPERBLASTER;
		else
			mod = MOD_BLASTER;

		SG_TargetDamage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 1, DAMAGE_ENERGY, mod);
	}
	else {
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BLASTER);
		gi.WritePosition(self->s.origin);

		if (!plane)
			gi.WriteDir(vec3_origin);
		else
			gi.WriteDir(plane->normal);

		gi.multicast(self->s.origin, MULTICAST_PVS);
	}

	SG_FreeEntity(self);
}

/*
 ==================
 SG_FireBlasterProjectile
 ==================
*/
void SG_FireBlasterProjectile (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect, qboolean hyper){

	edict_t	*bolt;
	trace_t	tr;

	VectorNormalize(dir);

	bolt = SG_AllocEntity();

	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	bolt->svflags = SVF_DEADMONSTER;

	VectorCopy(start, bolt->s.origin);
	VectorCopy(start, bolt->s.old_origin);
	SG_VectorToAngles(dir, bolt->s.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	VectorClear(bolt->mins);
	VectorClear(bolt->maxs);
	bolt->s.modelindex = gi.modelindex("models/objects/laser/tris.md2");
	bolt->s.sound = gi.soundindex("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = SG_BlasterTouch;
	bolt->nextthink = level.time + 2.0f;
	bolt->think = SG_FreeEntity;
	bolt->dmg = damage;
	bolt->className = "bolt";

	if (hyper)
		bolt->spawnflags = 1;

	gi.linkentity(bolt);

	if (self->client)
		SG_CheckDodge(self, bolt->s.origin, dir, speed);

	tr = gi.trace(self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);

	if (tr.fraction < 1.0f){
		VectorMA(bolt->s.origin, -10.0f, dir, bolt->s.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}
}	


/*
 ==============================================================================

 GRENADE LAUNCHER PROJECTILE

 ==============================================================================
*/


/*
 ==================
 SG_GrenadeExplode
 ==================
*/
static void SG_GrenadeExplode (edict_t *entity){

	vec3_t	origin;
	int		mod;
	float	points;
	vec3_t	v;
	vec3_t	dir;

	if (entity->owner->client)
		SG_PlayerNoise(entity->owner, entity->s.origin, PNOISE_IMPACT);

	// FIXME: if we are onground then raise our Z just a bit since we are a point?
	if (entity->enemy){
		VectorAdd(entity->enemy->mins, entity->enemy->maxs, v);
		VectorMA(entity->enemy->s.origin, 0.5f, v, v);
		VectorSubtract(entity->s.origin, v, v);
		points = entity->dmg - 0.5f * VectorLength(v);
		VectorSubtract(entity->enemy->s.origin, entity->s.origin, dir);

		if (entity->spawnflags & 1)
			mod = MOD_HANDGRENADE;
		else
			mod = MOD_GRENADE;

		SG_TargetDamage(entity->enemy, entity, entity->owner, dir, entity->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
	}

	if (entity->spawnflags & 2)
		mod = MOD_HELD_GRENADE;
	else if (entity->spawnflags & 1)
		mod = MOD_HG_SPLASH;
	else
		mod = MOD_G_SPLASH;

	SG_RadiusTargetDamage(entity, entity->owner, entity->dmg, entity->enemy, entity->dmg_radius, mod);

	VectorMA(entity->s.origin, -0.02f, entity->velocity, origin);
	gi.WriteByte(svc_temp_entity);

	if (entity->waterLevel){
		if (entity->groundentity)
			gi.WriteByte(TE_GRENADE_EXPLOSION_WATER);
		else
			gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
	}
	else {
		if (entity->groundentity)
			gi.WriteByte(TE_GRENADE_EXPLOSION);
		else
			gi.WriteByte(TE_ROCKET_EXPLOSION);
	}

	gi.WritePosition(origin);
	gi.multicast(entity->s.origin, MULTICAST_PHS);

	SG_FreeEntity(entity);
}

/*
 ==================
 SG_GrenadeTouch
 ==================
*/
static void SG_GrenadeTouch (edict_t *entity, edict_t *other, cplane_t *plane, csurface_t *surface){

	if (other == entity->owner)
		return;

	if (surface && (surface->flags & SURF_SKY)){
		SG_FreeEntity(entity);
		return;
	}

	if (!other->takedamage){
		if (entity->spawnflags & 1){
			if (random() > 0.5f)
				gi.sound(entity, CHAN_VOICE, gi.soundindex("weapons/hgrenb1a.wav"), 1.0f, ATTN_NORM, 0.0f);
			else
				gi.sound(entity, CHAN_VOICE, gi.soundindex("weapons/hgrenb2a.wav"), 1.0f, ATTN_NORM, 0.0f);
		}
		else
			gi.sound(entity, CHAN_VOICE, gi.soundindex("weapons/grenlb1b.wav"), 1.0f, ATTN_NORM, 0.0f);

		return;
	}

	entity->enemy = other;

	SG_GrenadeExplode(entity);
}

/*
 ==================
 SG_FireGrenadeProjectile
 ==================
*/
void SG_FireGrenadeProjectile (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int speed, float timer, float damageRadius){

	vec3_t	dir;
	vec3_t	forward, right, up;
	edict_t	*grenade;

	SG_VectorToAngles(aimDir, dir);
	AngleVectors(dir, forward, right, up);

	grenade = SG_AllocEntity();

	VectorCopy(start, grenade->s.origin);
	VectorScale(aimDir, speed, grenade->velocity);
	VectorMA(grenade->velocity, 200.0f + crandom() * 10.0f, up, grenade->velocity);
	VectorMA(grenade->velocity, crandom() * 10.0f, right, grenade->velocity);
	VectorSet(grenade->avelocity, 300.0f, 300.0f, 300.0f);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	VectorClear(grenade->mins);
	VectorClear(grenade->maxs);
	grenade->s.modelindex = gi.modelindex("models/objects/grenade/tris.md2");
	grenade->owner = self;
	grenade->touch = SG_GrenadeTouch;
	grenade->nextthink = level.time + timer;
	grenade->think = SG_GrenadeExplode;
	grenade->dmg = damage;
	grenade->dmg_radius = damageRadius;
	grenade->className = "grenade";

	gi.linkentity(grenade);
}

/*
 ==================
 SG_FireGrenadeProjectile2
 ==================
*/
void SG_FireGrenadeProjectile2 (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int speed, float timer, float damageRadius, qboolean held){

	vec3_t	dir;
	vec3_t	forward, right, up;
	edict_t	*grenade;

	SG_VectorToAngles(aimDir, dir);
	AngleVectors(dir, forward, right, up);

	grenade = SG_AllocEntity();

	VectorCopy(start, grenade->s.origin);
	VectorScale(aimDir, speed, grenade->velocity);
	VectorMA(grenade->velocity, 200.0f + crandom() * 10.0f, up, grenade->velocity);
	VectorMA(grenade->velocity, crandom() * 10.0f, right, grenade->velocity);
	VectorSet(grenade->avelocity, 300.0f, 300.0f, 300.0f);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	VectorClear(grenade->mins);
	VectorClear(grenade->maxs);
	grenade->s.modelindex = gi.modelindex("models/objects/grenade2/tris.md2");
	grenade->owner = self;
	grenade->touch = SG_GrenadeTouch;
	grenade->nextthink = level.time + timer;
	grenade->think = SG_GrenadeExplode;
	grenade->dmg = damage;
	grenade->dmg_radius = damageRadius;
	grenade->className = "hgrenade";

	if (held)
		grenade->spawnflags = 3;
	else
		grenade->spawnflags = 1;

	grenade->s.sound = gi.soundindex("weapons/hgrenc1b.wav");

	if (timer <= 0.0f)
		SG_GrenadeExplode(grenade);
	else {
		gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/hgrent1a.wav"), 1.0f, ATTN_NORM, 0.0f);
		gi.linkentity(grenade);
	}
}


/*
 ==============================================================================

 ROCKET LAUNCHER PROJECTILE

 ==============================================================================
*/


/*
 ==================
 SG_RocketTouch
 ==================
*/
static void SG_RocketTouch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf){

	vec3_t		origin;
	int			n;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY)){
		SG_FreeEntity(ent);
		return;
	}

	if (ent->owner->client)
		SG_PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	// Calculate position for the explosion entity
	VectorMA(ent->s.origin, -0.02f, ent->velocity, origin);

	if (other->takedamage)
		SG_TargetDamage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, 0, MOD_ROCKET);
	else {
		// Don't throw any debris in net games
		if (!deathmatch->value && !coop->value){
			if ((surf) && !(surf->flags & (SURF_WARP|SURF_TRANS33|SURF_TRANS66|SURF_FLOWING))){
				n = rand() % 5;

				while (n--)
					ThrowDebris(ent, "models/objects/debris2/tris.md2", 2, ent->s.origin);
			}
		}
	}

	SG_RadiusTargetDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, MOD_R_SPLASH);

	gi.WriteByte (svc_temp_entity);

	if (ent->waterLevel)
		gi.WriteByte(TE_ROCKET_EXPLOSION_WATER);
	else
		gi.WriteByte(TE_ROCKET_EXPLOSION);

	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS);

	SG_FreeEntity(ent);
}

/*
 ==================
 SG_FireRocketProjectile
 ==================
*/
void SG_FireRocketProjectile (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damageRadius, int radiusDamage){

	edict_t	*rocket;

	rocket = SG_AllocEntity();

	VectorCopy(start, rocket->s.origin);
	VectorCopy(dir, rocket->movedir);
	SG_VectorToAngles(dir, rocket->s.angles);
	VectorScale(dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ROCKET;
	VectorClear(rocket->mins);
	VectorClear(rocket->maxs);
	rocket->s.modelindex = gi.modelindex("models/objects/rocket/tris.md2");
	rocket->owner = self;
	rocket->touch = SG_RocketTouch;
	rocket->nextthink = level.time + 8000.0f / speed;
	rocket->think = SG_FreeEntity;
	rocket->dmg = damage;
	rocket->radius_dmg = radiusDamage;
	rocket->dmg_radius = damageRadius;
	rocket->s.sound = gi.soundindex("weapons/rockfly.wav");
	rocket->className = "rocket";

	if (self->client)
		SG_CheckDodge(self, rocket->s.origin, dir, speed);

	gi.linkentity(rocket);
}

/*
 ==============================================================================

 RAILGUN PROJECTILE

 ==============================================================================
*/


/*
 ==================
 SG_FireRailProjectile
 ==================
*/
void SG_FireRailProjectile (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int kick){

	vec3_t		end;
	vec3_t		from;
	trace_t		trace;
	edict_t		*ignore;
	int			mask;
	qboolean	water;

	VectorMA(start, 8192.0f, aimDir, end);
	VectorCopy(start, from);

	ignore = self;
	water = false;
	mask = MASK_SHOT|CONTENTS_SLIME|CONTENTS_LAVA;

	while (ignore){
		trace = gi.trace(from, NULL, NULL, end, ignore, mask);

		if (trace.contents & (CONTENTS_SLIME|CONTENTS_LAVA)){
			mask &= ~(CONTENTS_SLIME|CONTENTS_LAVA);
			water = true;
		}
		else {
			// Added so rail goes through SOLID_BBOX entities (gibs, etc)
			if ((trace.ent->svflags & SVF_MONSTER) || (trace.ent->client) || (trace.ent->solid == SOLID_BBOX))
				ignore = trace.ent;
			else
				ignore = NULL;

			if ((trace.ent != self) && (trace.ent->takedamage))
				SG_TargetDamage(trace.ent, self, self, aimDir, trace.endpos, trace.plane.normal, damage, kick, 0, MOD_RAILGUN);
		}

		VectorCopy(trace.endpos, from);
	}

	// Send gun puff / flash
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_RAILTRAIL);
	gi.WritePosition(start);
	gi.WritePosition(trace.endpos);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	if (water){
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_RAILTRAIL);
		gi.WritePosition(start);
		gi.WritePosition(trace.endpos);
		gi.multicast(trace.endpos, MULTICAST_PHS);
	}

	if (self->client)
		SG_PlayerNoise(self, trace.endpos, PNOISE_IMPACT);
}

/*
 ==============================================================================

 BFG PROJECTILE

 ==============================================================================
*/


/*
 ==================
 SG_BFGExplode
 ==================
*/
static void SG_BFGExplode (edict_t *self){

	edict_t	*entity;
	float	points;
	vec3_t	v;
	float	dist;

	if (self->s.frame == 0){
		// The BFG effect
		entity = NULL;

		while ((entity = SG_FindEntityWithinRadius(entity, self->s.origin, self->dmg_radius)) != NULL){
			if (!entity->takedamage)
				continue;
			if (entity == self->owner)
				continue;
			if (!SG_InflictorCanDamage(entity, self))
				continue;
			if (!SG_InflictorCanDamage(entity, self->owner))
				continue;

			VectorAdd(entity->mins, entity->maxs, v);
			VectorMA(entity->s.origin, 0.5f, v, v);
			VectorSubtract(self->s.origin, v, v);
			dist = VectorLength(v);

			points = self->radius_dmg * (1.0f - sqrt(dist / self->dmg_radius));

			if (entity == self->owner)
				points = points * 0.5f;

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_BFG_EXPLOSION);
			gi.WritePosition(entity->s.origin);
			gi.multicast(entity->s.origin, MULTICAST_PHS);

			SG_TargetDamage(entity, self, self->owner, self->velocity, entity->s.origin, vec3_origin, (int)points, 0, DAMAGE_ENERGY, MOD_BFG_EFFECT);
		}
	}

	self->nextthink = level.time + FRAMETIME;
	self->s.frame++;

	if (self->s.frame == 5)
		self->think = SG_FreeEntity;
}

/*
 ==================
 SG_BFGTouch
 ==================
*/
static void SG_BFGTouch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	if (other == self->owner)
		return;

	if (surface && (surface->flags & SURF_SKY)){
		SG_FreeEntity (self);
		return;
	}

	if (self->owner->client)
		SG_PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	// Core explosion - prevents firing it into the wall/floor
	if (other->takedamage)
		SG_TargetDamage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, 200, 0, 0, MOD_BFG_BLAST);

	SG_RadiusTargetDamage(self, self->owner, 200.0f, other, 100, MOD_BFG_BLAST);

	gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/bfg__x1b.wav"), 1.0f, ATTN_NORM, 0.0f);
	self->solid = SOLID_NOT;
	self->touch = NULL;
	VectorMA(self->s.origin, -1.0f * FRAMETIME, self->velocity, self->s.origin);
	VectorClear(self->velocity);
	self->s.modelindex = gi.modelindex("sprites/s_bfg3.sp2");
	self->s.frame = 0;
	self->s.sound = 0;
	self->s.effects &= ~EF_ANIM_ALLFAST;
	self->think = SG_BFGExplode;
	self->nextthink = level.time + FRAMETIME;
	self->enemy = other;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BFG_BIGEXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

/*
 ==================
 BFG_ThinkFramework
 ==================
*/
static void BFG_ThinkFramework (edict_t *self){

	edict_t	*entity;
	edict_t	*ignore;
	vec3_t	point;
	vec3_t	dir;
	vec3_t	start;
	vec3_t	end;
	int		dmg;
	trace_t	trace;

	if (deathmatch->value)
		dmg = 5;
	else
		dmg = 10;

	entity = NULL;

	while ((entity = SG_FindEntityWithinRadius(entity, self->s.origin, 256.0f)) != NULL){
		if (entity == self)
			continue;

		if (entity == self->owner)
			continue;

		if (!entity->takedamage)
			continue;

		if (!(entity->svflags & SVF_MONSTER) && (!entity->client) && (strcmp(entity->className, "misc_explobox") != 0))
			continue;

		VectorMA(entity->absmin, 0.5f, entity->size, point);

		VectorSubtract(point, self->s.origin, dir);
		VectorNormalize(dir);

		ignore = self;
		VectorCopy(self->s.origin, start);
		VectorMA(start, 2048.0f, dir, end);

		while (1){
			trace = gi.trace(start, NULL, NULL, end, ignore, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

			if (!trace.ent)
				break;

			// Hurt it if we can
			if ((trace.ent->takedamage) && !(trace.ent->flags & FL_IMMUNE_LASER) && (trace.ent != self->owner))
				SG_TargetDamage(trace.ent, self, self->owner, dir, trace.endpos, vec3_origin, dmg, 1, DAMAGE_ENERGY, MOD_BFG_LASER);

			// If we hit something that's not a monster or player we're done
			if (!(trace.ent->svflags & SVF_MONSTER) && (!trace.ent->client)){
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_LASER_SPARKS);
				gi.WriteByte(4);
				gi.WritePosition(trace.endpos);
				gi.WriteDir(trace.plane.normal);
				gi.WriteByte(self->s.skinnum);
				gi.multicast(trace.endpos, MULTICAST_PVS);

				break;
			}

			ignore = trace.ent;
			VectorCopy(trace.endpos, start);
		}

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BFG_LASER);
		gi.WritePosition(self->s.origin);
		gi.WritePosition(trace.endpos);
		gi.multicast(self->s.origin, MULTICAST_PHS);
	}

	self->nextthink = level.time + FRAMETIME;
}

/*
 ==================
 SG_FireBFGProjectile
 ==================
*/
void SG_FireBFGProjectile (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius){

	edict_t	*bfg;

	bfg = SG_AllocEntity();

	VectorCopy(start, bfg->s.origin);
	VectorCopy(dir, bfg->movedir);
	SG_VectorToAngles(dir, bfg->s.angles);
	VectorScale(dir, speed, bfg->velocity);
	bfg->movetype = MOVETYPE_FLYMISSILE;
	bfg->clipmask = MASK_SHOT;
	bfg->solid = SOLID_BBOX;
	bfg->s.effects |= EF_BFG | EF_ANIM_ALLFAST;
	VectorClear(bfg->mins);
	VectorClear(bfg->maxs);
	bfg->s.modelindex = gi.modelindex("sprites/s_bfg1.sp2");
	bfg->owner = self;
	bfg->touch = SG_BFGTouch;
	bfg->nextthink = level.time + 8000.0f / speed;
	bfg->think = SG_FreeEntity;
	bfg->radius_dmg = damage;
	bfg->dmg_radius = damage_radius;
	bfg->className = "bfg blast";
	bfg->s.sound = gi.soundindex("weapons/bfg__l1a.wav");

	bfg->think = BFG_ThinkFramework;
	bfg->nextthink = level.time + FRAMETIME;
	bfg->teammaster = bfg;
	bfg->teamchain = NULL;

	if (self->client)
		SG_CheckDodge(self, bfg->s.origin, dir, speed);

	gi.linkentity(bfg);
}