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
// m_medic.c - Medic monster
//

// TODO:
// - The resurrected target is bugged, weird class name
// - Make the medic prioritize it's attack so we dont run around when there is dead monsters around
// - Make a "continue" check if we want to continue the attack


#include "g_local.h"
#include "m_medic.h"


typedef struct {
	int					sightSound;
	int					idleSound;
	int					pain1Sound;
	int					pain2Sound;
	int					dieSound;
	int					searchSound;
	int					hookLaunchSound;
	int					hookHitSound;
	int					hookHealSound;
	int					hookRetractSound;
} medic_t;

static medic_t			medic;

static void Medic_Dead (edict_t *self);
static void Medic_Duck_Down (edict_t *self);
static void Medic_Duck_Hold (edict_t *self);
static void Medic_Duck_Up (edict_t *self);
static void Medic_ContinueFire (edict_t *self);
static void Medic_Cable_Attack (edict_t *self);
static void Medic_Hook_Retract (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/



/*
 ==================
 Medic_SightSound
 ==================
*/
static void Medic_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, medic.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Medic_IdleSound
 ==================
*/
static void Medic_IdleSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, medic.idleSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Medic_Pain1Sound
 ==================
*/
static void Medic_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, medic.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Medic_Pain2Sound
 ==================
*/
static void Medic_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, medic.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Medic_DieSound
 ==================
*/
static void Medic_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE,  medic.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Medic_SearchSound
 ==================
*/
static void Medic_SearchSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, medic.searchSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Medic_HookLaunchSound
 ==================
*/
static void Medic_HookLaunchSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, medic.hookLaunchSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Medic_HookHitSound
 ==================
*/
static void Medic_HookHitSound (edict_t *self){

	gi.sound(self->enemy, CHAN_AUTO, medic.hookHitSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Medic_HookHealSound
 ==================
*/
static void Medic_HookHealSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, medic.hookHealSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Medic_HookRetractSound
 ==================
*/
static void Medic_HookRetractSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, medic.hookRetractSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 SEARCH FOR DEAD MONSTERS

 ==============================================================================
*/


/*
 ==================
 Medic_FindDeadMonster
 ==================
*/
static edict_t *Medic_FindDeadMonster (edict_t *self){

	edict_t	*entity = NULL;
	edict_t	*bestChoice = NULL;

	while ((entity = SG_FindEntityWithinRadius(entity, self->s.origin, 1024.0f)) != NULL){
		if (entity == self)
			continue;
		if (!(entity->svflags & SVF_MONSTER))
			continue;
		if (entity->monsterinfo.aiflags & AI_GOOD_GUY)
			continue;
		if (entity->owner)
			continue;
		if (entity->health > 0)
			continue;
		if (entity->nextthink)
			continue;
		if (!SG_IsEntityVisible(self, entity))
			continue;

		if (!bestChoice){
			bestChoice = entity;
			continue;
		}

		if (entity->max_health <= bestChoice->max_health)
			continue;

		bestChoice = entity;
	}

	return bestChoice;
}

/*
 ==================
 Medic_Idle
 ==================
*/
static void Medic_Idle (edict_t *self){

	edict_t	*entity;

	Medic_IdleSound(self);

	entity = Medic_FindDeadMonster(self);

	if (entity){
		self->enemy = entity;
		self->enemy->owner = self;
		self->monsterinfo.aiflags |= AI_MEDIC;

		SG_AIFoundTarget(self);
	}
}

/*
 ==================
 Medic_Search
 ==================
*/
static void Medic_Search (edict_t *self){

	edict_t	*entity;

	Medic_SearchSound(self);

	if (!self->oldenemy){
		entity = Medic_FindDeadMonster(self);

		if (entity){
			self->oldenemy = self->enemy;
			self->enemy = entity;
			self->enemy->owner = self;
			self->monsterinfo.aiflags |= AI_MEDIC;

			SG_AIFoundTarget(self);
		}
	}
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			medic_standFrames[] = {
	{SG_AIStand,	0.0f,	Medic_Idle},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL}
};

static mmove_t			medic_standMove = {FRAME_wait1, FRAME_wait90, medic_standFrames, NULL};


/*
 ==================
 Medic_StandAnimation
 ==================
*/
static void Medic_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &medic_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			medic_walkFrames[] = {
	{SG_AIWalk,		6.2f,	NULL},
	{SG_AIWalk,	   18.1f,   NULL},
	{SG_AIWalk,		1.0f,	NULL},
	{SG_AIWalk,		9.0f,	NULL},
	{SG_AIWalk,	   10.0f,	NULL},
	{SG_AIWalk,		9.0f,	NULL},
	{SG_AIWalk,	   11.0f,	NULL},
	{SG_AIWalk,	   11.6f,   NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		9.9f,	NULL},
	{SG_AIWalk,	   14.0f,	NULL},
	{SG_AIWalk,		9.3f,	NULL}
};

static mmove_t			medic_walkMove = {FRAME_walk1, FRAME_walk12, medic_walkFrames, NULL};

/*
 ==================
 Medic_WalkAnimation
 ==================
*/
static void Medic_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &medic_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			medic_runFrames[] = {
	{SG_AIRun,	   18.0f,	NULL},
	{SG_AIRun,	   22.5f,	NULL},
	{SG_AIRun,	   25.4f,	NULL},
	{SG_AIRun,	   23.4f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   35.6f,	NULL}
};

static mmove_t			medic_runMove = {FRAME_run1, FRAME_run6, medic_runFrames, NULL};


/*
 ==================
 Medic_RunAnimation
 ==================
*/
static void Medic_RunAnimation (edict_t *self){

	edict_t	*entity;

	if (!(self->monsterinfo.aiflags & AI_MEDIC)){
		entity = Medic_FindDeadMonster(self);

		if (entity){
			self->oldenemy = self->enemy;
			self->enemy = entity;
			self->enemy->owner = self;
			self->monsterinfo.aiflags |= AI_MEDIC;

			SG_AIFoundTarget(self);

			return;
		}
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &medic_standMove;
	else
		self->monsterinfo.currentmove = &medic_runMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Medic_Blaster
 ==================
*/
static void Medic_Blaster (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start, end;
	vec3_t	dir;
	int		effect;

	if ((self->s.frame == FRAME_attack9) || (self->s.frame == FRAME_attack12))
		effect = EF_BLASTER;
	else if ((self->s.frame == FRAME_attack19) || (self->s.frame == FRAME_attack22) || (self->s.frame == FRAME_attack25) || (self->s.frame == FRAME_attack28))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_MEDIC_BLASTER_1], forward, right, start);

	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);

	SG_MonsterFireBlaster(self, start, dir, 2, 1000, MZ2_MEDIC_BLASTER_1, effect);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			medic_hyperBlasterAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	Medic_Blaster}
};

static mframe_t			medic_blasterAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	5.0f,	NULL},
	{SG_AICharge,	5.0f,	NULL},
	{SG_AICharge,	3.0f,	NULL},
	{SG_AICharge,	2.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Medic_Blaster},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Medic_Blaster},	
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Medic_ContinueFire}
};

static vec3_t			medic_cable_offsets[] = {
	{45.0f,		   -9.2f,  15.5f},
	{48.4f,		   -9.7f,  15.2f},
	{47.8f,		   -9.8f,  15.8f},
	{47.3f,		   -9.3f,  14.3f},
	{45.4f,		  -10.1f,  13.1f},
	{41.9f,		  -12.7f,  12.0f},
	{37.8f,		  -15.8f,  11.2f},
	{34.3f,		  -18.4f,  10.7f},
	{32.7f,		  -19.7f,  10.4f},
	{32.7f,		  -19.7f,  10.4f}
};

static mframe_t			medic_cableAttackFrames[] = {
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AIMove,		0.0f,	Medic_HookLaunchSound},
	{SG_AIMove,		0.0f,	Medic_Cable_Attack},
	{SG_AIMove,		0.0f,	Medic_Cable_Attack},
	{SG_AIMove,		0.0f,	Medic_Cable_Attack},
	{SG_AIMove,		0.0f,	Medic_Cable_Attack},
	{SG_AIMove,		0.0f,	Medic_Cable_Attack},
	{SG_AIMove,		0.0f,	Medic_Cable_Attack},
	{SG_AIMove,		0.0f,	Medic_Cable_Attack},
	{SG_AIMove,		0.0f,	Medic_Cable_Attack},
	{SG_AIMove,		0.0f,	Medic_Cable_Attack},
	{SG_AIMove,	    0.0f,	Medic_Hook_Retract},
	{SG_AIMove,	    0.0f,	NULL},
	{SG_AIMove,	    0.0f,	NULL},
	{SG_AIMove,	    0.0f,	NULL},
	{SG_AIMove,	    0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			medic_hyperBlasterAttackMove = {FRAME_attack15, FRAME_attack30, medic_hyperBlasterAttackFrames, Medic_RunAnimation};
static mmove_t			medic_blasterAttackMove = {FRAME_attack1, FRAME_attack14, medic_blasterAttackFrames, Medic_RunAnimation};
static mmove_t			medic_cableAttackMove = {FRAME_attack33, FRAME_attack60, medic_cableAttackFrames, Medic_RunAnimation};


/*
 ==================
 Medic_ContinueFire
 ==================
*/
static void Medic_ContinueFire (edict_t *self){

	if (SG_IsEntityVisible(self, self->enemy)){
		if (random() <= 0.95f)
			self->monsterinfo.currentmove = &medic_hyperBlasterAttackMove;
	}
}

/*
 ==================
 Medic_Cable_Attack
 ==================
*/
static void Medic_Cable_Attack (edict_t *self){

	vec3_t		forward, right;
	vec3_t		offset;
	vec3_t		start, end;
	float		dist;
	vec3_t		dir, angles;
	trace_t		trace;

	if (!self->enemy || !self->enemy->inuse)
		return;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(medic_cable_offsets[self->s.frame - FRAME_attack42], offset);

	SG_ProjectSource(self->s.origin, offset, forward, right, start);

	// Check for max distance
	VectorSubtract(start, self->enemy->s.origin, dir);

	dist = VectorLength(dir);
	if (dist > 256.0f)
		return;

	// Check for min/max pitch
	SG_VectorToAngles(dir, angles);
	if (angles[0] < -180.0f)
		angles[0] += 360.0f;

	if (fabs(angles[0]) > 45.0f)
		return;

	// Trace to see if the cable is blocked
	trace = gi.trace(start, NULL, NULL, self->enemy->s.origin, self, MASK_SHOT);
	
	if (trace.fraction != 1.0f && trace.ent != self->enemy)
		return;

	if (self->s.frame == FRAME_attack43){
		Medic_HookHitSound(self);
		self->enemy->monsterinfo.aiflags |= AI_RESURRECTING;
	}
	else if (self->s.frame == FRAME_attack50){
		self->enemy->spawnflags = 0;
		self->enemy->monsterinfo.aiflags = 0;
		self->enemy->target = NULL;
		self->enemy->targetname = NULL;
		self->enemy->combattarget = NULL;
		self->enemy->deathtarget = NULL;
		self->enemy->owner = self;

		SG_CallSpawn(self->enemy);

		self->enemy->owner = NULL;

		if (self->enemy->think){
			self->enemy->nextthink = level.time;
			self->enemy->think(self->enemy);
		}

		self->enemy->monsterinfo.aiflags |= AI_RESURRECTING;

		if (self->oldenemy && self->oldenemy->client){
			self->enemy->enemy = self->oldenemy;
			SG_AIFoundTarget(self->enemy);
		}
	}
	else {
		if (self->s.frame == FRAME_attack44)
			Medic_HookHealSound(self);
	}

	// Adjust start for beam origin being in middle of a segment
	VectorMA(start, 8.0f, forward, start);

	// Adjust end z for end spot since the monster is currently dead
	VectorCopy(self->enemy->s.origin, end);
	end[2] = self->enemy->absmin[2] + self->enemy->size[2] / 2.0f;

	// Write to the client
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_MEDIC_CABLE_ATTACK);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

/*
 ==================
 Medic_Hook_Retract
 ==================
*/
static void Medic_Hook_Retract (edict_t *self){

	Medic_HookRetractSound(self);
	self->enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
}

/*
 ==================
 Medic_Attack
 ==================
*/
static void Medic_Attack (edict_t *self){

	float	dist;

	dist = SG_EntityDistance(self, self->enemy);

	if (self->monsterinfo.aiflags & AI_MEDIC){
		if (dist < 256.0f)
			self->monsterinfo.currentmove = &medic_cableAttackMove;
		else
			self->enemy = self->oldenemy;
	}
	else
		self->monsterinfo.currentmove = &medic_blasterAttackMove;
}


/*
 ==============================================================================

 WEAPON CHECK

 ==============================================================================
*/


/*
 ==================
 Medic_Check_Attack
 ==================
*/
static qboolean Medic_Check_Attack (edict_t *self){

	if (self->monsterinfo.aiflags & AI_MEDIC){
		Medic_Attack(self);
		return true;
	}

	return SG_AICheckMonsterAttack(self);
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			medic_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			medic_pain2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			medic_pain1Move = {FRAME_paina1, FRAME_paina8, medic_pain1Frames, Medic_RunAnimation};
static mmove_t			medic_pain2Move = {FRAME_painb1, FRAME_painb15, medic_pain2Frames, Medic_RunAnimation};


/*
 ==================
 Medic_PainAnimation
 ==================
*/
static void Medic_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == 3.0f)
		return;		// No pain anims in nightmare

	if (random() < 0.5f){
		self->monsterinfo.currentmove = &medic_pain1Move;
		Medic_Pain1Sound(self);
	}
	else {
		self->monsterinfo.currentmove = &medic_pain2Move;
		Medic_Pain2Sound(self);
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			medic_deathFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			medic_deathMove = {FRAME_death1, FRAME_death30, medic_deathFrames, Medic_Dead};


/*
 ==================
 Medic_Dead
 ==================
*/
static void Medic_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Medic_DieAnimation
 ==================
*/
static void Medic_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	// If we had a pending patient, free him up for another medic
	if ((self->enemy) && (self->enemy->owner == self))
		self->enemy->owner = NULL;

	// Check for gib
	if (self->health <= self->gib_health){
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1.0f, ATTN_NORM, 0.0f);

		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);

		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);

		self->deadflag = DEAD_DEAD;

		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// Regular death
	Medic_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &medic_deathMove;
}


/*
 ==============================================================================

 DUCK ANIMATION

 ==============================================================================
*/

static mframe_t			medic_duckFrames[] = {
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	Medic_Duck_Down},
	{SG_AIMove,	   -1.0f,	Medic_Duck_Hold},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	Medic_Duck_Up},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL}
};

static mmove_t			medic_duckMove = {FRAME_duck1, FRAME_duck16, medic_duckFrames, Medic_RunAnimation};


/*
 ==================
 Medic_Duck_Down
 ==================
*/
static void Medic_Duck_Down (edict_t *self){

	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32.0f;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + 1.0f;

	gi.linkentity(self);
}

/*
 ==================
 Medic_Duck_Hold
 ==================
*/
static void Medic_Duck_Hold (edict_t *self){

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

/*
 ==================
 Medic_Duck_Up
 ==================
*/
static void Medic_Duck_Up (edict_t *self){

	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32.0f;
	self->takedamage = DAMAGE_AIM;

	gi.linkentity(self);
}

/*
 ==================
 Medic_DodgeAnimation
 ==================
*/
static void Medic_DodgeAnimation (edict_t *self, edict_t *attacker, float eta){

	if (random() > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = &medic_duckMove;
}


/*
 ==============================================================================

 MONSTER_MEDIC

 ==============================================================================
*/


/*
 ==================
 SG_MonsterMedic_Spawn
 ==================
*/
void SG_MonsterMedic_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/medic/tris.md2");

	VectorSet(self->mins, -24.0f, -24.0f, -24.0f);
	VectorSet(self->maxs, 24.0f, 24.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	medic.sightSound = gi.soundindex("medic/medsght1.wav");
	medic.idleSound = gi.soundindex("medic/idle.wav");
	medic.pain1Sound = gi.soundindex("medic/medpain1.wav");
	medic.pain2Sound = gi.soundindex("medic/medpain2.wav");
	medic.dieSound = gi.soundindex("medic/meddeth1.wav");
	medic.searchSound = gi.soundindex("medic/medsrch1.wav");
	medic.hookLaunchSound = gi.soundindex("medic/medatck2.wav");
	medic.hookHitSound = gi.soundindex("medic/medatck3.wav");
	medic.hookHealSound = gi.soundindex("medic/medatck4.wav");
	medic.hookRetractSound = gi.soundindex("medic/medatck5.wav");

	gi.soundindex("medic/medatck1.wav");

	self->mass = 400;

	self->health = 300;
	self->gib_health = -130;

	self->pain = Medic_PainAnimation;
	self->die = Medic_DieAnimation;

	self->monsterinfo.stand = Medic_StandAnimation;
	self->monsterinfo.walk = Medic_WalkAnimation;
	self->monsterinfo.run = Medic_RunAnimation;
	self->monsterinfo.dodge = Medic_DodgeAnimation;
	self->monsterinfo.attack = Medic_Attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = Medic_SightSound;
	self->monsterinfo.idle = Medic_IdleSound;
	self->monsterinfo.search = Medic_SearchSound;
	self->monsterinfo.checkattack = Medic_Check_Attack;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &medic_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}