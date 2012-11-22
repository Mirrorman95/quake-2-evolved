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
// m_gladiator.c - Gladiator monster
//


#include "g_local.h"
#include "m_gladiator.h"


typedef struct {
	int					sightSound;
	int					idleSound;
	int					pain1Sound;
	int					pain2Sound;
	int					dieSound;
	int					searchSound;
	int					gunshotSound;
	int					cleaverSwingSound;
	int					cleaverHitSound;
	int					cleaverMissSound;
} gladiator_t;

static gladiator_t		gladiator;

static void Gladiator_Dead (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Gladiator_SightSound
 ==================
*/
static void Gladiator_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, gladiator.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gladiator_IdleSound
 ==================
*/
static void Gladiator_IdleSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, gladiator.idleSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Gladiator_Pain1Sound
 ==================
*/
static void Gladiator_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, gladiator.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gladiator_Pain2Sound
 ==================
*/
static void Gladiator_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, gladiator.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gladiator_DieSound
 ==================
*/
static void Gladiator_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, gladiator.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gladiator_SearchSound
 ==================
*/
static void Gladiator_SearchSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, gladiator.searchSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gladiator_GunshotSound
 ==================
*/
static void Gladiator_GunshotSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, gladiator.gunshotSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gladiator_CleaverSwingSound
 ==================
*/
static void Gladiator_CleaverSwingSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, gladiator.cleaverSwingSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gladiator_CleaverHitSound
 ==================
*/
static void Gladiator_CleaverHitSound (edict_t *self){

	gi.sound(self, CHAN_AUTO, gladiator.cleaverHitSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gladiator_CleaverMissSound
 ==================
*/
static void Gladiator_CleaverMissSound (edict_t *self){

	gi.sound(self, CHAN_AUTO, gladiator.cleaverMissSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			gladiator_standFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL}
};

static mmove_t			gladiator_standMove = {FRAME_stand1, FRAME_stand7, gladiator_standFrames, NULL};


/*
 ==================
 Gladiator_StandAnimation
 ==================
*/
static void Gladiator_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &gladiator_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/


static mframe_t			gladiator_walkFrames[] = {
	{SG_AIWalk,	   15.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,	   12.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		1.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL}
};

static mmove_t			gladiator_walkMove = {FRAME_walk1, FRAME_walk16, gladiator_walkFrames, NULL};


/*
 ==================
 Gladiator_WalkAnimation
 ==================
*/
static void Gladiator_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &gladiator_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/


static mframe_t			gladiator_runFrames[] = {
	{SG_AIRun,	   23.0f,	NULL},
	{SG_AIRun,	   14.0f,	NULL},
	{SG_AIRun,	   14.0f,	NULL},
	{SG_AIRun,	   21.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL}
};

static mmove_t			gladiator_runMove = {FRAME_run1, FRAME_run6, gladiator_runFrames, NULL};


/*
 ==================
 Gladiator_RunAnimation
 ==================
*/
static void Gladiator_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &gladiator_standMove;
	else
		self->monsterinfo.currentmove = &gladiator_runMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Gladiator_Melee
 ==================
*/
static void Gladiator_Melee (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, self->mins[0], -4.0f);

	if (SG_MeleeHit(self, aim, (20 + (rand() %5)), 300))
		Gladiator_CleaverHitSound(self);
	else
		Gladiator_CleaverMissSound(self);
}

/*
 ==================
 Gladiator_Railgun
 ==================
*/
static void Gladiator_Railgun (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1], forward, right, start);

	// Calculate the direction to where we targted
	VectorSubtract(self->pos1, start, dir);
	VectorNormalize(dir);

	SG_MonsterFireRailgun(self, start, dir, 50, 100, MZ2_GLADIATOR_RAILGUN_1);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			gladiator_meleeAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Gladiator_CleaverSwingSound},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Gladiator_Melee},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Gladiator_CleaverSwingSound},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Gladiator_Melee},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			gladiator_railgunAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Gladiator_Railgun},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mmove_t			gladiator_meleeAttackMove = {FRAME_melee1, FRAME_melee17, gladiator_meleeAttackFrames, Gladiator_RunAnimation};
static mmove_t			gladiator_railgunAttackMove = {FRAME_attack1, FRAME_attack9, gladiator_railgunAttackFrames, Gladiator_RunAnimation};


/*
 ==================
 Gladiator_MeleeAttackAnimation
 ==================
*/
static void Gladiator_MeleeAttackAnimation (edict_t *self){

	self->monsterinfo.currentmove = &gladiator_meleeAttackMove;
}

/*
 ==================
 Gladiator_AttackAnimation
 ==================
*/
static void Gladiator_AttackAnimation (edict_t *self){

	vec3_t	v;
	float	range;

	// A small safe zone
	VectorSubtract(self->s.origin, self->enemy->s.origin, v);
	range = VectorLength(v);
	if (range <= (MELEE_DISTANCE + 32))
		return;

	// Charge up the railgun
	Gladiator_GunshotSound(self);
	
	VectorCopy(self->enemy->s.origin, self->pos1);	// Save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
	self->monsterinfo.currentmove = &gladiator_railgunAttackMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/


static mframe_t			gladiator_painAirFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			gladiator_painFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			gladiator_painAirmove = {FRAME_painup1, FRAME_painup7, gladiator_painAirFrames, Gladiator_RunAnimation};
static mmove_t			gladiator_painMove = {FRAME_pain1, FRAME_pain6, gladiator_painFrames, Gladiator_RunAnimation};

/*
 ==================
 Gladiator_PainAnimation
 ==================
*/
static void Gladiator_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time){
		if ((self->velocity[2] > 100.0f) && (self->monsterinfo.currentmove == &gladiator_painMove))
			self->monsterinfo.currentmove = &gladiator_painAirmove;

		return;
	}

	self->pain_debounce_time = level.time + 3.0f;

	if (random() < 0.5f)
		Gladiator_Pain1Sound(self);
	else
		Gladiator_Pain2Sound(self);

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	if (self->velocity[2] > 100.0f)
		self->monsterinfo.currentmove = &gladiator_painAirmove;
	else
		self->monsterinfo.currentmove = &gladiator_painMove;
	
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			gladiator_deathFrames[] = {
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

static mmove_t		gladiator_deathMove = {FRAME_death1, FRAME_death22, gladiator_deathFrames, Gladiator_Dead};


/*
 ==================
 Gladiator_Dead
 ==================
*/
static void Gladiator_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);

	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;

	gi.linkentity(self);
}

/*
 ==================
 Gladiator_DieAnimation
 ==================
*/
static void Gladiator_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

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
	Gladiator_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &gladiator_deathMove;
}


/*
 ==============================================================================

 MONSTER_GLADIATOR

 ==============================================================================
*/


/*
 ==================
 SG_MonsterGladiator_Spawn
 ==================
*/
void SG_MonsterGladiator_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/gladiatr/tris.md2");
	
	VectorSet(self->mins, -32.0f, -32.0f, -24.0f);
	VectorSet(self->maxs, 32.0f, 32.0f, 64.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	gladiator.sightSound = gi.soundindex("gladiator/sight.wav");
	gladiator.idleSound = gi.soundindex("gladiator/gldidle1.wav");
	gladiator.pain1Sound = gi.soundindex("gladiator/pain.wav");	
	gladiator.pain2Sound = gi.soundindex("gladiator/gldpain2.wav");	
	gladiator.dieSound = gi.soundindex("gladiator/glddeth2.wav");	
	gladiator.searchSound = gi.soundindex("gladiator/gldsrch1.wav");
	gladiator.gunshotSound = gi.soundindex("gladiator/railgun.wav");
	gladiator.cleaverSwingSound = gi.soundindex("gladiator/melee1.wav");
	gladiator.cleaverHitSound = gi.soundindex("gladiator/melee2.wav");
	gladiator.cleaverMissSound = gi.soundindex("gladiator/melee3.wav");

	self->mass = 400;

	self->health = 400;
	self->gib_health = -175;

	self->pain = Gladiator_PainAnimation;
	self->die = Gladiator_DieAnimation;

	self->monsterinfo.stand = Gladiator_StandAnimation;
	self->monsterinfo.walk = Gladiator_WalkAnimation;
	self->monsterinfo.run = Gladiator_RunAnimation;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = Gladiator_AttackAnimation;
	self->monsterinfo.melee = Gladiator_MeleeAttackAnimation;
	self->monsterinfo.sight = Gladiator_SightSound;
	self->monsterinfo.idle = Gladiator_IdleSound;
	self->monsterinfo.search = Gladiator_SearchSound;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &gladiator_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}