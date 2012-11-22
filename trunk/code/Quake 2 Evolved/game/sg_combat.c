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
// sg_combat.c - Combat target events
//

// TODO:
// - SG_CheckTeamDamage


#include "g_local.h"


/*
 ==============================================================================

 TARGET KILLED

 ==============================================================================
*/


/*
 ==================
 SG_TargetKilled
 ==================
*/
static void SG_TargetKilled (edict_t *target, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	if (target->health < -999)
		target->health = -999;

	target->enemy = attacker;

	// Miscellaneous stuff
	if ((target->svflags & SVF_MONSTER) && (target->deadflag != DEAD_DEAD)){
		if (!(target->monsterinfo.aiflags & AI_GOOD_GUY)){
			level.killed_monsters++;

			if (coop->value && attacker->client)
				attacker->client->resp.score++;

			// Medics won't heal monsters that they kill themselves
			if (!strcmp(attacker->className, "monster_medic"))
				target->owner = attacker;
		}
	}

	// Doors, triggers, etc
	if (target->movetype == MOVETYPE_PUSH || target->movetype == MOVETYPE_STOP || target->movetype == MOVETYPE_NONE){
		target->die(target, inflictor, attacker, damage, point);
		return;
	}

	// Monster
	if ((target->svflags & SVF_MONSTER) && (target->deadflag != DEAD_DEAD)){
		target->touch = NULL;
		SG_MonsterDeathUse(target);
	}

	// Client
	target->die(target, inflictor, attacker, damage, point);
}


/*
 ==============================================================================

 TARGET DAMAGE

 target		Entity that is being damaged
 inflictor	Entity that is causing the damage
 attacker	Entity that caused the inflictor to damage targ
	example: target=monster, inflictor=rocket, attacker=player

 dir		Direction of the attack
 point		Point at which the damage is being inflicted
 normal		Normal vector from that point
 damage		Amount of damage being inflicted
 knockback	Force to be applied against targ as a result of the damage

 dflags		these flags are used to control how SG_TargetDamage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_ENERGY			damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_BULLET			damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
 ==============================================================================
*/


/*
 ==================
 SG_InflictorCanDamage

 Returns true if the inflictor can directly damage the target. Used for
 explosions and melee attacks.
 ==================
*/
qboolean SG_InflictorCanDamage (edict_t *target, edict_t *inflictor){

	vec3_t	dest;
	trace_t	trace;

	// Inline models need special checking because their origin is 0,0,0
	if (target->movetype == MOVETYPE_PUSH){
		VectorAdd(target->absmin, target->absmax, dest);
		VectorScale(dest, 0.5f, dest);

		trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);

		if (trace.fraction == 1.0f)
			return true;
		if (trace.ent == target)
			return true;

		return false;
	}

	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, target->s.origin, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	VectorCopy(target->s.origin, dest);
	dest[0] += 15.0f;
	dest[1] += 15.0f;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	VectorCopy(target->s.origin, dest);
	dest[0] += 15.0f;
	dest[1] -= 15.0f;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	VectorCopy(target->s.origin, dest);
	dest[0] -= 15.0f;
	dest[1] += 15.0f;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	VectorCopy(target->s.origin, dest);
	dest[0] -= 15.0f;
	dest[1] -= 15.0f;
	trace = gi.trace(inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	return false;
}

/*
 ==================
 SG_SpawnDamage
 ==================
*/
static void SG_SpawnDamage (int type, vec3_t origin, vec3_t normal, int damage){

	if (damage > 255)
		damage = 255;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	gi.WritePosition(origin);
	gi.WriteDir(normal);
	gi.multicast(origin, MULTICAST_PVS);
}

/*
 ==================
 SG_CheckPowerArmor
 ==================
*/
static int SG_CheckPowerArmor (edict_t *entity, vec3_t point, vec3_t normal, int damage, int dmgFlags){

	gclient_t	*client;
	int			save;
	int			type;
	vec3_t		vec;
	float		dot;
	vec3_t		forward;
	int			index;
	int			damagePerCell;
	int			teType;
	int			power;
	int			powerUsed;

	if (!damage)
		return 0;

	client = entity->client;

	if (dmgFlags & DAMAGE_NO_ARMOR)
		return 0;

	if (client){
		type = SG_FindPowerArmorByType(entity);

		if (type != POWER_ARMOR_NONE){
			index = ITEM_INDEX(SG_FindItem("Cells"));
			power = client->pers.inventory[index];
		}
	}
	else if (entity->svflags & SVF_MONSTER){
		type = entity->monsterinfo.power_armor_type;
		power = entity->monsterinfo.power_armor_power;
	}
	else
		return 0;

	if (type == POWER_ARMOR_NONE)
		return 0;
	if (!power)
		return 0;

	if (type == POWER_ARMOR_SCREEN){
		// Only works if damage point is in front
		AngleVectors(entity->s.angles, forward, NULL, NULL);
		VectorSubtract(point, entity->s.origin, vec);
		VectorNormalize(vec);
		dot = DotProduct(vec, forward);
		if (dot <= 0.3f)
			return 0;

		damagePerCell = 1;
		teType = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else {
		damagePerCell = 2;
		teType = TE_SHIELD_SPARKS;
		damage = (2 * damage) / 3;
	}

	save = power * damagePerCell;

	if (!save)
		return 0;

	if (save > damage)
		save = damage;

	SG_SpawnDamage(teType, point, normal, save);
	entity->powerarmor_time = level.time + 0.2f;

	powerUsed = save / damagePerCell;

	if (client)
		client->pers.inventory[index] -= powerUsed;
	else
		entity->monsterinfo.power_armor_power -= powerUsed;

	return save;
}

/*
 ==================
 SG_CheckArmor
 ==================
*/
static int SG_CheckArmor (edict_t *entity, vec3_t point, vec3_t normal, int damage, int teSparks, int dmgFlags){

	gclient_t	*client;
	int			index;
	gItem_t		*armor;
	int			save;

	if (!damage)
		return 0;

	client = entity->client;

	if (!client)
		return 0;

	if (dmgFlags & DAMAGE_NO_ARMOR)
		return 0;

	index = SG_FindArmorByIndex(entity);

	if (!index)
		return 0;

	armor = SG_GetItemByIndex(index);

	if (dmgFlags & DAMAGE_ENERGY)
		save = ceil(((gItemArmor_t *)armor->info)->energyProtection * damage);
	else
		save = ceil(((gItemArmor_t *)armor->info)->normalProtection * damage);

	if (save >= client->pers.inventory[index])
		save = client->pers.inventory[index];

	if (!save)
		return 0;

	client->pers.inventory[index] -= save;
	SG_SpawnDamage(teSparks, point, normal, save);

	return save;
}

/*
 ==================
 SG_MonsterReactToDamage
 ==================
*/
static void SG_MonsterReactToDamage (edict_t *target, edict_t *attacker){

	if (!(attacker->client) && !(attacker->svflags & SVF_MONSTER))
		return;

	if (attacker == target || attacker == target->enemy)
		return;

	// If we are a good guy monster and our attacker is a player
	// or another good guy, do not get mad at them
	if (target->monsterinfo.aiflags & AI_GOOD_GUY){
		if (attacker->client || (attacker->monsterinfo.aiflags & AI_GOOD_GUY))
			return;
	}

	// We now know that we are not both good guys

	// If attacker is a client, get mad at them because he's good and we're not
	if (attacker->client){
		target->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

		// This can only happen in coop (both new and old enemies are clients)
		// only switch if can't see the current enemy
		if (target->enemy && target->enemy->client){
			if (SG_IsEntityVisible(target, target->enemy)){
				target->oldenemy = attacker;
				return;
			}

			target->oldenemy = target->enemy;
		}

		target->enemy = attacker;

		if (!(target->monsterinfo.aiflags & AI_DUCKED))
			SG_AIFoundTarget(target);

		return;
	}

	// It's the same base (walk/swim/fly) type and a different classname and it's not a tank
	// (they spray too much), get mad at them
	if (((target->flags & (FL_FLY|FL_SWIM)) == (attacker->flags & (FL_FLY|FL_SWIM))) &&
		(strcmp(target->className, attacker->className) != 0) &&
		(strcmp(attacker->className, "monster_tank") != 0) &&
		(strcmp(attacker->className, "monster_supertank") != 0) &&
		(strcmp(attacker->className, "monster_makron") != 0) &&
		(strcmp(attacker->className, "monster_jorg") != 0))
	{
		if (target->enemy && target->enemy->client)
			target->oldenemy = target->enemy;

		target->enemy = attacker;

		if (!(target->monsterinfo.aiflags & AI_DUCKED))
			SG_AIFoundTarget(target);
	}
	else if (attacker->enemy == target){
		// If they *meant* to shoot us, then shoot back
		if (target->enemy && target->enemy->client)
			target->oldenemy = target->enemy;

		target->enemy = attacker;

		if (!(target->monsterinfo.aiflags & AI_DUCKED))
			SG_AIFoundTarget(target);
	}
	else if (attacker->enemy && attacker->enemy != target){
		// Otherwise get mad at whoever they are mad at (help our buddy) unless it is us!
		if (target->enemy && target->enemy->client)
			target->oldenemy = target->enemy;

		target->enemy = attacker->enemy;

		if (!(target->monsterinfo.aiflags & AI_DUCKED))
			SG_AIFoundTarget(target);
	}
}

/*
 ==================
 SG_CheckTeamDamage
 ==================
*/
static qboolean SG_CheckTeamDamage (edict_t *target, edict_t *attacker){

	// FIXME: Make the next line real and uncomment this block
	// if ((ability to damage a teammate == OFF) && (targ's team == attacker's team))

	return false;
}

/*
 ==================
 SG_TargetDamage
 ==================
*/
void SG_TargetDamage (edict_t *target, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dmgFlags, int mod){

	gclient_t	*client;
	int			take;
	int			save;
	int			armorSave;
	int			powerArmorSave;
	int			teSparks;

	if (!target->takedamage)
		return;

	// Friendly fire avoidance
	// If enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if ((target != attacker) && ((deathmatch->value && ((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS))) || coop->value)){
		if (SG_IsOnSameTeam(target, attacker)){
			if ((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE)
				damage = 0;
			else
				mod |= MOD_FRIENDLY_FIRE;
		}
	}

	meansOfDeath = mod;

	// Easy mode takes half damage
	if (skill->value == 0 && deathmatch->value == 0 && target->client){
		damage *= 0.5f;

		if (!damage)
			damage = 1;
	}

	client = target->client;

	if (dmgFlags & DAMAGE_BULLET)
		teSparks = TE_BULLET_SPARKS;
	else
		teSparks = TE_SPARKS;

	VectorNormalize(dir);

	// Bonus damage for suprising a monster
	if (!(dmgFlags & DAMAGE_RADIUS) && (target->svflags & SVF_MONSTER) && (attacker->client) && (!target->enemy) && (target->health > 0))
		damage *= 2;

	if (target->flags & FL_NO_KNOCKBACK)
		knockback = 0;

	// Figure momentum add
	if (!(dmgFlags & DAMAGE_NO_KNOCKBACK)){
		if ((knockback) && (target->movetype != MOVETYPE_NONE) && (target->movetype != MOVETYPE_BOUNCE) && (target->movetype != MOVETYPE_PUSH) && (target->movetype != MOVETYPE_STOP)){
			vec3_t	kvel;
			float	mass;

			if (target->mass < 50)
				mass = 50.0f;
			else
				mass = target->mass;

			if (target->client  && attacker == target)
				VectorScale(dir, 1600.0f * (float)knockback / mass, kvel);		// The rocket jump hack...
			else
				VectorScale(dir, 500.0f * (float)knockback / mass, kvel);

			VectorAdd(target->velocity, kvel, target->velocity);
		}
	}

	take = damage;
	save = 0;

	// Check for godmode
	if ((target->flags & FL_GODMODE) && !(dmgFlags & DAMAGE_NO_PROTECTION)){
		take = 0;
		save = damage;

		SG_SpawnDamage(teSparks, point, normal, save);
	}

	// Check for invincibility
	if ((client && client->invincible_framenum > level.framenum) && !(dmgFlags & DAMAGE_NO_PROTECTION)){
		if (target->pain_debounce_time < level.time){
			gi.sound(target, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1.0f, ATTN_NORM, 0.0f);
			target->pain_debounce_time = level.time + 2;
		}

		take = 0;
		save = damage;
	}

	powerArmorSave = SG_CheckPowerArmor(target, point, normal, take, dmgFlags);
	take -= powerArmorSave;

	armorSave = SG_CheckArmor(target, point, normal, take, teSparks, dmgFlags);
	take -= armorSave;

	// Treat cheat/powerup savings the same as armor
	armorSave += save;

	// Team damage avoidance
	if (!(dmgFlags & DAMAGE_NO_PROTECTION) && SG_CheckTeamDamage(target, attacker))
		return;

	// Do the damage
	if (take){
		if ((target->svflags & SVF_MONSTER) || (client))
			SG_SpawnDamage(TE_BLOOD, point, normal, take);
		else
			SG_SpawnDamage(teSparks, point, normal, take);

		target->health = target->health - take;

		if (target->health <= 0){
			if ((target->svflags & SVF_MONSTER) || (client))
				target->flags |= FL_NO_KNOCKBACK;

			SG_TargetKilled(target, inflictor, attacker, take, point);
			return;
		}
	}

	if (target->svflags & SVF_MONSTER){
		SG_MonsterReactToDamage(target, attacker);

		if (!(target->monsterinfo.aiflags & AI_DUCKED) && (take)){
			target->pain(target, attacker, knockback, take);

			// Nightmare mode monsters don't go into pain frames often
			if (skill->value == 3)
				target->pain_debounce_time = level.time + 5.0f;
		}
	}
	else if (client){
		if (!(target->flags & FL_GODMODE) && (take))
			target->pain(target, attacker, knockback, take);
	}
	else if (take){
		if (target->pain)
			target->pain(target, attacker, knockback, take);
	}

	// Add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client){
		client->damage_parmor += powerArmorSave;
		client->damage_armor += armorSave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy(point, client->damage_from);
	}
}

/*
 ==================
 SG_RadiusTargetDamage
 ==================
*/
void SG_RadiusTargetDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod){

	edict_t	*entity = NULL;
	vec3_t	v;
	float	points;
	vec3_t	dir;

	while ((entity = SG_FindEntityWithinRadius(entity, inflictor->s.origin, radius)) != NULL){
		if (entity == ignore)
			continue;
		if (!entity->takedamage)
			continue;

		VectorAdd(entity->mins, entity->maxs, v);
		VectorMA(entity->s.origin, 0.5f, v, v);
		VectorSubtract(inflictor->s.origin, v, v);
		points = damage - 0.5f * VectorLength(v);

		if (entity == attacker)
			points = points * 0.5f;

		if (points > 0.0f){
			if (SG_InflictorCanDamage(entity, inflictor)){
				VectorSubtract(entity->s.origin, inflictor->s.origin, dir);
				SG_TargetDamage(entity, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
			}
		}
	}
}