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
// sg_ai.c - AI behaviour
//

// TODO:
// - SG_AIRun, SG_AIFindTarget


#include "g_local.h"


/*
 ==============================================================================
 .enemy
 Will be world if not currently angry at anyone.

 .movetarget
 The next path spot to walk toward.  If .enemy, ignore .movetarget.
 When an enemy is killed, the monster will try to return to it's path.

 .hunt_time
 Set to time + something when the player is in sight, but movement straight for
 him is blocked.  This causes the monster to use wall following code for
 movement direction instead of sighting on the player.

 .ideal_yaw
 A yaw angle of the intended direction, which will be turned towards at up
 to 45 deg / state.  If the enemy is in view and hunt_time is not active,
 this will be the exact line towards the enemy.

 .pausetime
 A monster will leave it's stand state and head towards it's .movetarget when
 time > .pausetime.

 walkmove(angle, speed) primitive is all or nothing
 ==============================================================================
*/

typedef struct {
	qboolean			enemyVisible;
	qboolean			enemyInFront;
	int					enemyRange;
	float				enemyYaw;
} ai_t;

static ai_t				ai;

static void SG_AIStrafe (edict_t *self, float distance);
static qboolean SG_AICheckAttack (edict_t *self, float dist);


/*
 ==============================================================================

 AI MOVEMENT TYPES

 ==============================================================================
*/


/*
 ==================
 SG_AIMove

 Move the specified distance at current facing
 ==================
*/
void SG_AIMove (edict_t *self, float dist){

	SG_MonsterMove(self, self->s.angles[YAW], dist);
}

/*
 ==================
 SG_AIStand

 Used for standing around and looking for players
 Distance is for slight position adjustments needed by the animations
 ==================
*/
void SG_AIStand (edict_t *self, float dist){

	vec3_t	v;

	if (dist)
		SG_MonsterMove(self, self->s.angles[YAW], dist);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND){
		if (self->enemy){
			VectorSubtract(self->enemy->s.origin, self->s.origin, v);
			self->idealYaw = SG_VectorToYaw(v);

			if (self->s.angles[YAW] != self->idealYaw && self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND){
				self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
				self->monsterinfo.run(self);
			}

			SG_MonsterChangeYaw(self);
			SG_AICheckAttack(self, 0.0f);
		}
		else
			SG_AIFindTarget(self);

		return;
	}

	if (SG_AIFindTarget(self))
		return;

	if (level.time > self->monsterinfo.pausetime){
		self->monsterinfo.walk(self);
		return;
	}

	if (!(self->spawnflags & 1) && (self->monsterinfo.idle) && (level.time > self->monsterinfo.idle_time)){
		if (self->monsterinfo.idle_time){
			self->monsterinfo.idle(self);
			self->monsterinfo.idle_time = level.time + 15.0f + random() * 15.0f;
		}
		else
			self->monsterinfo.idle_time = level.time + random() * 15.0f;
	}
}

/*
 ==================
 SG_AIWalk

 The monster is walking it's beat
 ==================
*/
void SG_AIWalk (edict_t *self, float dist){

	SG_MonsterMoveToGoal(self, dist);

	// Check for noticing a player
	if (SG_AIFindTarget(self))
		return;

	if ((self->monsterinfo.search) && (level.time > self->monsterinfo.idle_time)){
		if (self->monsterinfo.idle_time){
			self->monsterinfo.search(self);
			self->monsterinfo.idle_time = level.time + 15.0f + random() * 15.0f;
		}
		else
			self->monsterinfo.idle_time = level.time + random() * 15.0f;
	}
}

/*
 ==================
 SG_AICharge

 Turns towards target and advances
 Use this call with a distnace of 0 to replace ai_face
 ==================
*/
void SG_AICharge (edict_t *self, float dist){

	vec3_t	v;

	VectorSubtract(self->enemy->s.origin, self->s.origin, v);
	self->idealYaw = SG_VectorToYaw(v);
	SG_MonsterChangeYaw(self);

	if (dist)
		SG_MonsterMove(self, self->s.angles[YAW], dist);
}

/*
 ==================
 SG_AITurn

 Don't move, but turn towards ideal_yaw
 Distance is for slight position adjustments needed by the animations
 ==================
*/
void SG_AITurn (edict_t *self, float dist){

	if (dist)
		SG_MonsterMove(self, self->s.angles[YAW], dist);

	if (SG_AIFindTarget(self))
		return;
	
	SG_MonsterChangeYaw(self);
}

/*
 ==================
 SG_AIRun

 The monster has an enemy it is trying to kill
 ==================
*/
void SG_AIRun (edict_t *self, float dist){

	vec3_t		v;
	edict_t		*tempGoal;
	edict_t		*save;
	qboolean	newTarget;
	edict_t		*marker;
	float		d1, d2;
	trace_t		trace;
	vec3_t		v_forward, v_right;
	float		left, center, right;
	vec3_t		leftTarget, rightTarget;

	// If we're going to a combat point, just proceed
	if (self->monsterinfo.aiflags & AI_COMBAT_POINT){
		SG_MonsterMoveToGoal(self, dist);
		return;
	}

	if (self->monsterinfo.aiflags & AI_SOUND_TARGET){
		VectorSubtract(self->s.origin, self->enemy->s.origin, v);

		if (VectorLength(v) < 64.0f){
			self->monsterinfo.aiflags |= (AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			self->monsterinfo.stand(self);
			return;
		}

		SG_MonsterMoveToGoal(self, dist);

		if (!SG_AIFindTarget(self))
			return;
	}

	if (SG_AICheckAttack(self, dist))
		return;

	if (self->monsterinfo.attack_state == AS_SLIDING){
		SG_AIStrafe(self, dist);
		return;
	}

	if (ai.enemyVisible){
		SG_MonsterMoveToGoal(self, dist);

		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
		VectorCopy(self->enemy->s.origin, self->monsterinfo.last_sighting);
		self->monsterinfo.trail_time = level.time;
		return;
	}

	// Coop will change to another enemy if visible
	if (coop->value){
		// FIXME: Insane guys get mad with this, which causes crashes!
		if (SG_AIFindTarget(self))
			return;
	}

	if ((self->monsterinfo.search_time) && (level.time > (self->monsterinfo.search_time + 20.0f))){
		SG_MonsterMoveToGoal(self, dist);
		self->monsterinfo.search_time = 0.0f;
		return;
	}

	save = self->goalEntity;
	tempGoal = SG_AllocEntity();
	self->goalEntity = tempGoal;

	newTarget = false;

	if (!(self->monsterinfo.aiflags & AI_LOST_SIGHT)){
		// Just lost sight of the player, decide where to go first
		self->monsterinfo.aiflags |= (AI_LOST_SIGHT | AI_PURSUIT_LAST_SEEN);
		self->monsterinfo.aiflags &= ~(AI_PURSUE_NEXT | AI_PURSUE_TEMP);
		newTarget = true;
	}

	if (self->monsterinfo.aiflags & AI_PURSUE_NEXT){
		self->monsterinfo.aiflags &= ~AI_PURSUE_NEXT;

		// Give ourself more time since we got this far
		self->monsterinfo.search_time = level.time + 5.0f;

		if (self->monsterinfo.aiflags & AI_PURSUE_TEMP){
			self->monsterinfo.aiflags &= ~AI_PURSUE_TEMP;
			marker = NULL;
			VectorCopy (self->monsterinfo.saved_goal, self->monsterinfo.last_sighting);
			newTarget = true;
		}
		else if (self->monsterinfo.aiflags & AI_PURSUIT_LAST_SEEN){
			self->monsterinfo.aiflags &= ~AI_PURSUIT_LAST_SEEN;
			marker = SG_ClientTrail_PickFirst(self);
		}
		else
			marker = SG_ClientTrail_PickNext(self);

		if (marker){
			VectorCopy(marker->s.origin, self->monsterinfo.last_sighting);
			self->monsterinfo.trail_time = marker->timestamp;
			self->s.angles[YAW] = self->idealYaw = marker->s.angles[YAW];
			newTarget = true;
		}
	}

	VectorSubtract(self->s.origin, self->monsterinfo.last_sighting, v);

	d1 = VectorLength(v);

	if (d1 <= dist){
		self->monsterinfo.aiflags |= AI_PURSUE_NEXT;
		dist = d1;
	}

	VectorCopy(self->monsterinfo.last_sighting, self->goalEntity->s.origin);

	if (newTarget){
		trace = gi.trace(self->s.origin, self->mins, self->maxs, self->monsterinfo.last_sighting, self, MASK_PLAYERSOLID);

		if (trace.fraction < 1.0f){
			VectorSubtract(self->goalEntity->s.origin, self->s.origin, v);
			d1 = VectorLength(v);
			center = trace.fraction;
			d2 = d1 * ((center + 1.0f) / 2.0f);
			self->s.angles[YAW] = self->idealYaw = SG_VectorToYaw(v);
			AngleVectors(self->s.angles, v_forward, v_right, NULL);

			VectorSet(v, d2, -16.0f, 0.0f);
			SG_ProjectSource(self->s.origin, v, v_forward, v_right, leftTarget);
			trace = gi.trace(self->s.origin, self->mins, self->maxs, leftTarget, self, MASK_PLAYERSOLID);
			left = trace.fraction;

			VectorSet(v, d2, 16.0f, 0.0f);
			SG_ProjectSource(self->s.origin, v, v_forward, v_right, rightTarget);
			trace = gi.trace(self->s.origin, self->mins, self->maxs, rightTarget, self, MASK_PLAYERSOLID);
			right = trace.fraction;

			center = (d1 * center) / d2;

			if (left >= center && left > right){
				if (left < 1.0f){
					VectorSet(v, d2 * left * 0.5f, -16.0f, 0.0f);
					SG_ProjectSource (self->s.origin, v, v_forward, v_right, leftTarget);
				}

				VectorCopy(self->monsterinfo.last_sighting, self->monsterinfo.saved_goal);
				self->monsterinfo.aiflags |= AI_PURSUE_TEMP;
				VectorCopy(leftTarget, self->goalEntity->s.origin);
				VectorCopy(leftTarget, self->monsterinfo.last_sighting);
				VectorSubtract(self->goalEntity->s.origin, self->s.origin, v);
				self->s.angles[YAW] = self->idealYaw = SG_VectorToYaw(v);
			}
			else if (right >= center && right > left){
				if (right < 1.0f){
					VectorSet(v, d2 * right * 0.5f, 16.0f, 0.0f);
					SG_ProjectSource (self->s.origin, v, v_forward, v_right, rightTarget);
				}

				VectorCopy (self->monsterinfo.last_sighting, self->monsterinfo.saved_goal);
				self->monsterinfo.aiflags |= AI_PURSUE_TEMP;
				VectorCopy(rightTarget, self->goalEntity->s.origin);
				VectorCopy(rightTarget, self->monsterinfo.last_sighting);
				VectorSubtract(self->goalEntity->s.origin, self->s.origin, v);
				self->s.angles[YAW] = self->idealYaw = SG_VectorToYaw(v);
			}
		}
	}

	SG_MonsterMoveToGoal(self, dist);

	SG_FreeEntity(tempGoal);

	if (self)
		self->goalEntity = save;
}


/*
 ==============================================================================

 AI TARGET

 ==============================================================================
*/


/*
 ==================
 SG_AISetSightClient

 Called once each frame to set level.sight_client to the
 player to be checked for in findtarget.

 If all clients are either dead or in notarget, sight_client
 will be null.

 In coop games, sight_client will cycle between the clients.
 ==================
*/
void SG_AISetSightClient (){

	int		start, check;
	edict_t	*entity;

	if (level.sight_client == NULL)
		start = 1;
	else
		start = level.sight_client - g_edicts;

	check = start;

	while (1){
		check++;

		if (check > game.maxclients)
			check = 1;

		entity = &g_edicts[check];

		if (entity->inuse && entity->health > 0 && !(entity->flags & FL_NOTARGET) && !entity->client->pers.spectator){
			level.sight_client = entity;
			return;		// Found one
		}

		if (check == start){
			level.sight_client = NULL;
			return;		// Nobody to see
		}
	}
}

/*
 ==================
 SG_AIHuntTarget
 ==================
*/
static void SG_AIHuntTarget (edict_t *self){

	vec3_t	vec;

	self->goalEntity = self->enemy;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.stand(self);
	else
		self->monsterinfo.run(self);

	VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
	self->idealYaw = SG_VectorToYaw(vec);

	// Wait a while before first attack
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		SG_AttackFinished(self, 1);
}

/*
 ==================
 SG_AIFoundTarget
 ==================
*/
void SG_AIFoundTarget (edict_t *self){

	// Let other monsters see this monster for a while
	if (self->enemy->client){
		level.sight_entity = self;
		level.sight_entity_framenum = level.framenum;
		level.sight_entity->lightLevel = 128;
	}

	// Wake up other monsters
	self->show_hostile = level.time + 1.0f;

	VectorCopy(self->enemy->s.origin, self->monsterinfo.last_sighting);
	self->monsterinfo.trail_time = level.time;

	if (!self->combattarget){
		SG_AIHuntTarget(self);
		return;
	}

	self->goalEntity = self->moveTarget = SG_PickTarget(self->combattarget);

	if (!self->moveTarget){
		self->goalEntity = self->moveTarget = self->enemy;
		SG_AIHuntTarget(self);
		gi.dprintf("%s at %s, combattarget %s not found\n", self->className, SG_VectorToString(self->s.origin), self->combattarget);
		return;
	}

	// Clear out our combat target, these are a one shot deal
	self->combattarget = NULL;
	self->monsterinfo.aiflags |= AI_COMBAT_POINT;

	// Clear the targetname, that point is ours!
	self->moveTarget->targetname = NULL;
	self->monsterinfo.pausetime = 0.0f;

	// Run for it
	self->monsterinfo.run(self);
}

/*
 ==================
 SG_AIFindTarget

 Self is currently not attacking anything, so try to find a target

 Returns TRUE if an enemy was sighted

 When a player fires a missile, the point of impact becomes a fakeplayer so
 that monsters that see the impact will respond as if they had seen the
 player.

 To avoid spending too much time, only a single client (or fakeclient) is
 checked each frame. This means multi player games will have slightly
 slower noticing monsters.
 ==================
*/
qboolean SG_AIFindTarget (edict_t *self){

	edict_t		*client;
	qboolean	heardIt;
	int			r;
	vec3_t		temp;

	if (self->monsterinfo.aiflags & AI_GOOD_GUY){
		if (self->goalEntity && self->goalEntity->inuse && self->goalEntity->className){
			if (!strcmp(self->goalEntity->className, "target_actor"))
				return false;
		}

		// FIXME: Look for monsters?
		return false;
	}

	// If we're going to a combat point, just proceed
	if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
		return false;

	// If the first spawnflag bit is set, the monster will only wake up on
	// really seeing the player, not another monster getting angry or hearing
	// something

	// Revised behavior so they will wake up if they "see" a player make a noise
	// but not weapon impact/explosion noises

	heardIt = false;

	if ((level.sight_entity_framenum >= (level.framenum - 1)) && !(self->spawnflags & 1)){
		client = level.sight_entity;

		if (client->enemy == self->enemy)
			return false;
	}
	else if (level.sound_entity_framenum >= (level.framenum - 1)){
		client = level.sound_entity;
		heardIt = true;
	}
	else if (!(self->enemy) && (level.sound2_entity_framenum >= (level.framenum - 1)) && !(self->spawnflags & 1)){
		client = level.sound2_entity;
		heardIt = true;
	}
	else {
		client = level.sight_client;

		// No clients to get mad at
		if (!client)
			return false;
	}

	// If the entity went away, forget it
	if (!client->inuse)
		return false;

	if (client == self->enemy)
		return true;

	if (client->client){
		if (client->flags & FL_NOTARGET)
			return false;
	}
	else if (client->svflags & SVF_MONSTER){
		if (!client->enemy)
			return false;
		if (client->enemy->flags & FL_NOTARGET)
			return false;
	}
	else if (heardIt){
		if (client->owner->flags & FL_NOTARGET)
			return false;
	}
	else
		return false;

	if (!heardIt){
		r = SG_EntityRange(self, client);

		if (r == RANGE_FAR)
			return false;

		// FIXME?: This is where we would check invisibility

		// Is client in an spot too dark to be seen?
		if (client->lightLevel <= 5)
			return false;

		if (!SG_IsEntityVisible(self, client))
			return false;

		if (r == RANGE_NEAR){
			if (client->show_hostile < level.time && !SG_IsEntityInFront(self, client))
				return false;
		}
		else if (r == RANGE_MID){
			if (!SG_IsEntityInFront(self, client))
				return false;
		}

		self->enemy = client;

		if (strcmp(self->enemy->className, "player_noise") != 0){
			self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

			if (!self->enemy->client){
				self->enemy = self->enemy->enemy;

				if (!self->enemy->client){
					self->enemy = NULL;
					return false;
				}
			}
		}
	}
	else {	// Heard it
		if (self->spawnflags & 1){
			if (!SG_IsEntityVisible(self, client))
				return false;
		}
		else {
			if (!gi.inPHS(self->s.origin, client->s.origin))
				return false;
		}

		VectorSubtract(client->s.origin, self->s.origin, temp);

		// Too far to hear
		if (VectorLength(temp) > 1000.0f)
			return false;

		// Check area portals, if they are different and not connected then we can't hear it
		if (client->areanum != self->areanum){
			if (!gi.AreasConnected(self->areanum, client->areanum))
				return false;
		}

		self->idealYaw = SG_VectorToYaw(temp);
		SG_MonsterChangeYaw(self);

		// Hunt the sound for a bit; hopefully find the real player
		self->monsterinfo.aiflags |= AI_SOUND_TARGET;
		self->enemy = client;
	}

	// Got one
	SG_AIFoundTarget(self);

	if (!(self->monsterinfo.aiflags & AI_SOUND_TARGET) && (self->monsterinfo.sight))
		self->monsterinfo.sight(self, self->enemy);

	return true;
}


/*
 ==============================================================================

 AI ATTACK TYPES

 ==============================================================================
*/


/*
 ==================
 SG_FacingIdealYaw
 ==================
*/
static qboolean SG_FacingIdealYaw (edict_t *self){

	float	delta;

	delta = anglemod(self->s.angles[YAW] - self->idealYaw);

	if (delta > 45.0f && delta < 315.0f)
		return false;

	return true;
}

/*
 ==================
 SG_AICheckMonsterAttack
 ==================
*/
qboolean SG_AICheckMonsterAttack (edict_t *self){

	vec3_t	spot1, spot2;
	trace_t	trace;
	float	chance;

	if (self->enemy->health > 0){
		// See if any entities are in the way of the shot
		VectorCopy(self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy(self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;

		trace = gi.trace(spot1, NULL, NULL, spot2, self, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_WINDOW);

		// Do we have a clear shot?
		if (trace.ent != self->enemy)
			return false;
	}

	// Melee attack
	if (ai.enemyRange == RANGE_MELEE){
		// Don't always melee in easy mode
		if (skill->value == 0 && (rand() & 3))
			return false;

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

	if (ai.enemyRange == RANGE_FAR)
		return false;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		chance = 0.4f;
	else if (ai.enemyRange == RANGE_MELEE)
		chance = 0.2f;
	else if (ai.enemyRange == RANGE_NEAR)
		chance = 0.1f;
	else if (ai.enemyRange == RANGE_MID)
		chance = 0.02f;
	else
		return false;

	if (skill->value == 0)
		chance *= 0.5f;
	else if (skill->value >= 2)
		chance *= 2.0f;

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
 SG_AIMissile

 Turn in place until within an angle to launch a missile attack
 ==================
*/
static void SG_AIMissile (edict_t *self){

	self->idealYaw = ai.enemyYaw;
	SG_MonsterChangeYaw(self);

	if (SG_FacingIdealYaw(self)){
		self->monsterinfo.attack(self);
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
}

/*
 ==================
 SG_AIMelee

 Turn and close until within an angle to launch a melee attack
 ==================
*/
static void SG_AIMelee (edict_t *self){

	self->idealYaw = ai.enemyYaw;
	SG_MonsterChangeYaw(self);

	if (SG_FacingIdealYaw(self)){
		self->monsterinfo.melee(self);
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
}

/*
 ==================
 SG_AIStrafe

 Strafe sideways, but stay at aproximately the same range
 ==================
*/
static void SG_AIStrafe (edict_t *self, float distance){

	float	offset;
	
	self->idealYaw = ai.enemyYaw;;
	SG_MonsterChangeYaw(self);

	if (self->monsterinfo.lefty)
		offset = 90.0f;
	else
		offset = -90.0f;
	
	if (SG_MonsterMove(self, self->idealYaw + offset, distance))
		return;
		
	self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
	SG_MonsterMove(self, self->idealYaw - offset, distance);
}

/*
 ==================
 SG_AICheckAttack

 Decides if we're going to attack or do something else
 used by SG_AIRun and SG_AIStand
 ==================
*/
static qboolean SG_AICheckAttack (edict_t *self, float dist){

	vec3_t		temp;
	qboolean	hesDeadJim;

	// This causes monsters to run blindly to the combat point w/o firing
	if (self->goalEntity){
		if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
			return false;

		if (self->monsterinfo.aiflags & AI_SOUND_TARGET){
			if ((level.time - self->enemy->teleportTime) > 5.0f){
				if (self->goalEntity == self->enemy){
					if (self->moveTarget)
						self->goalEntity = self->moveTarget;
					else
						self->goalEntity = NULL;
				}

				self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

				if (self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
					self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			}
			else {
				self->show_hostile = level.time + 1.0f;
				return false;
			}
		}
	}

	ai.enemyVisible = false;

	// See if the enemy is dead
	hesDeadJim = false;

	if (deathmatch->value){
		if (self->enemy){
			if (self->enemy->client->pers.spectator)
				hesDeadJim = true;
		}
	}

	if ((!self->enemy) || (!self->enemy->inuse) || (level.noTarget))
		hesDeadJim = true;
	else if (self->monsterinfo.aiflags & AI_MEDIC){
		if (self->enemy->health > 0){
			hesDeadJim = true;
			self->monsterinfo.aiflags &= ~AI_MEDIC;
		}
	}
	else {
		if (self->monsterinfo.aiflags & AI_BRUTAL){
			if (self->enemy->health <= -80)
				hesDeadJim = true;
		}
		else {
			if (self->enemy->health <= 0){
				hesDeadJim = true;
			}
		}
	}

	if (hesDeadJim){
		self->enemy = NULL;

		// FIXME: Look all around for other targets
		if (self->oldenemy && self->oldenemy->health > 0){
			self->enemy = self->oldenemy;
			self->oldenemy = NULL;

			SG_AIHuntTarget(self);
		}
		else {
			if (self->moveTarget){
				self->goalEntity = self->moveTarget;
				self->monsterinfo.walk(self);
			}
			else {
				// We need the pausetime otherwise the stand code
				// will just revert to walking with no target and
				// the monsters will wonder around aimlessly trying
				// to hunt the world entity
				self->monsterinfo.pausetime = level.time + 100000000.0f;
				self->monsterinfo.stand(self);
			}

			return true;
		}
	}

	// Wake up other monsters
	self->show_hostile = level.time + 1.0f;

	// Check knowledge of enemy
	ai.enemyVisible = SG_IsEntityVisible(self, self->enemy);

	if (ai.enemyVisible){
		self->monsterinfo.search_time = level.time + 5.0f;
		VectorCopy(self->enemy->s.origin, self->monsterinfo.last_sighting);
	}

	ai.enemyInFront = SG_IsEntityInFront(self, self->enemy);
	ai.enemyRange = SG_EntityRange(self, self->enemy);
	VectorSubtract(self->enemy->s.origin, self->s.origin, temp);
	ai.enemyYaw = SG_VectorToYaw(temp);

	if (self->monsterinfo.attack_state == AS_MISSILE){
		SG_AIMissile(self);
		return true;
	}

	if (self->monsterinfo.attack_state == AS_MELEE){
		SG_AIMelee(self);
		return true;
	}

	// If enemy is not currently visible, we will never attack
	if (!ai.enemyVisible)
		return false;

	return self->monsterinfo.checkattack(self);
}