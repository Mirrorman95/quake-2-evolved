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
// m_hornet.c - Hornet monster
//

// NOTE:
// - This monster has a "fidget" animation that is left unused

// FIXME:
// - Hornet_CheckAttack


#include "g_local.h"
#include "m_boss2.h"


typedef struct {
	int					pain1Sound;
	int					pain2Sound;
	int					pain3Sound;
	int					dieSound;
	int					searchSound;
} hornet_t;

static hornet_t			hornet;

static void Hornet_MachinegunAttackAnimation (edict_t *self);
static void Hornet_MachinegunReAttackAnimation (edict_t *self);
static void Hornet_Dead (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Hornet_Pain1Sound
 ==================
*/
static void Hornet_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, hornet.pain1Sound, 1.0f, ATTN_NONE, 0);
}

/*
 ==================
 Hornet_Pain2Sound
 ==================
*/
static void Hornet_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, hornet.pain2Sound, 1.0f, ATTN_NONE, 0);
}

/*
 ==================
 Hornet_Pain3Sound
 ==================
*/
static void Hornet_Pain3Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, hornet.pain3Sound, 1.0f, ATTN_NONE, 0);
}

/*
 ==================
 Hornet_DieSound
 ==================
*/
static void Hornet_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, hornet.dieSound, 1.0f, ATTN_NONE, 0.0f);
}

/*
 ==================
 Hornet_SearchSound
 ==================
*/
static void Hornet_SearchSound (edict_t *self){

	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, hornet.searchSound, 1.0f, ATTN_NONE, 0.0f);
}


/*
 ==============================================================================

 FIDGET ANIMATION

 ==============================================================================
*/

static mframe_t			hornet_fidgetFrames[] = {
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

static mmove_t			hornet_fidgetMove = {FRAME_stand1, FRAME_stand30, hornet_fidgetFrames, NULL};


/*
 ==================
 Hornet_FidgetAnimation
 ==================
*/
static void Hornet_FidgetAnimation (edict_t *self){

	self->monsterinfo.currentmove = &hornet_fidgetMove;
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			hornet_standFrames[] = {
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

static mmove_t			hornet_standMove = {FRAME_stand30, FRAME_stand50, hornet_standFrames, NULL};


/*
 ==================
 Hornet_StandAnimation
 ==================
*/
static void Hornet_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &hornet_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			hornet_walkFrames[] = {
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL}
};

static mmove_t			hornet_walkMove = {FRAME_walk1, FRAME_walk20, hornet_walkFrames, NULL};


/*
 ==================
 Hornet_WalkAnimation
 ==================
*/
static void Hornet_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &hornet_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			hornet_runFrames[] = {
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL}
};

static mmove_t			hornet_runMove = {FRAME_walk1, FRAME_walk20, hornet_runFrames, NULL};


/*
 ==================
 Hornet_RunAnimation
 ==================
*/
static void Hornet_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &hornet_standMove;
	else
		self->monsterinfo.currentmove = &hornet_runMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Hornet_RocketLauncher
 ==================
*/
static void Hornet_RocketLauncher (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;

	AngleVectors(self->s.angles, forward, right, NULL);

	// Rocket one
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_1], forward, right, start);

	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	SG_MonsterFireRocket(self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_1);

	// Rocket two
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_2], forward, right, start);

	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	SG_MonsterFireRocket(self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_2);

	// Rocket three
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_3], forward, right, start);

	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	SG_MonsterFireRocket(self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_3);

	// Rocket four
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_4], forward, right, start);

	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	SG_MonsterFireRocket(self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_4);
}	

/*
 ==================
 Hornet_FireBullet_Left
 ==================
*/
static void Hornet_FireBullet_Left (edict_t *self){

	vec3_t	forward, right, target;
	vec3_t	start;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_BOSS2_MACHINEGUN_L1], forward, right, start);

	VectorMA(self->enemy->s.origin, -0.2f, self->enemy->velocity, target);

	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, forward);
	VectorNormalize(forward);

	SG_MonsterFireBullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_BOSS2_MACHINEGUN_L1);
}

/*
 ==================
 Hornet_FireBullet_Right
 ==================
*/
static void Hornet_FireBullet_Right (edict_t *self){

	vec3_t	forward, right, target;
	vec3_t	start;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_BOSS2_MACHINEGUN_R1], forward, right, start);

	VectorMA(self->enemy->s.origin, -0.2f, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, forward);
	VectorNormalize(forward);

	SG_MonsterFireBullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_BOSS2_MACHINEGUN_R1);
}

/*
 ==================
 Hornet_Machinegun
 ==================
*/
static void Hornet_Machinegun (edict_t *self){

	Hornet_FireBullet_Left(self);
	Hornet_FireBullet_Right(self);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			hornet_preMachinegunAttackFrames[] = {
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,	Hornet_MachinegunAttackAnimation}
};

static mframe_t			hornet_machinegunAttackFrames[] = {
	{SG_AICharge,	1.0f,   Hornet_Machinegun},
	{SG_AICharge,	1.0f,   Hornet_Machinegun},
	{SG_AICharge,	1.0f,   Hornet_Machinegun},
	{SG_AICharge,	1.0f,   Hornet_Machinegun},
	{SG_AICharge,	1.0f,   Hornet_Machinegun},
	{SG_AICharge,	1.0f,	Hornet_MachinegunReAttackAnimation}
};

static mframe_t			hornet_postMachinegunAttackFrames[] = {
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL}
};

static mframe_t			hornet_rocketLauncherAttackFrames[] = {
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AIMove,	  -20.0f,	Hornet_RocketLauncher},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL}
};

static mmove_t			hornet_preMachinegunAttackMove = {FRAME_attack1, FRAME_attack9, hornet_preMachinegunAttackFrames, NULL};
static mmove_t			hornet_machinegunAttackMove = {FRAME_attack10, FRAME_attack15, hornet_machinegunAttackFrames, NULL};
static mmove_t			hornet_postMachinegunAttackMove = {FRAME_attack16, FRAME_attack19, hornet_postMachinegunAttackFrames, Hornet_RunAnimation};
static mmove_t			hornet_rocketLauncherAttackMove = {FRAME_attack20, FRAME_attack40, hornet_rocketLauncherAttackFrames, Hornet_RunAnimation};


/*
 ==================
 Hornet_AttackAnimation
 ==================
*/
static void Hornet_AttackAnimation (edict_t *self){

	vec3_t	vec;
	float	range;

	VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
	range = VectorLength(vec);
	
	if (range <= 125.0f)
		self->monsterinfo.currentmove = &hornet_preMachinegunAttackMove;
	else {
		if (random() <= 0.6f)
			self->monsterinfo.currentmove = &hornet_preMachinegunAttackMove;
		else
			self->monsterinfo.currentmove = &hornet_rocketLauncherAttackMove;
	}
}

/*
 ==================
 Hornet_MachinegunAttackAnimation
 ==================
*/
static void Hornet_MachinegunAttackAnimation (edict_t *self){

	self->monsterinfo.currentmove = &hornet_machinegunAttackMove;
}

/*
 ==================
 Hornet_MachinegunReAttackAnimation
 ==================
*/
static void Hornet_MachinegunReAttackAnimation (edict_t *self){

	if (SG_IsEntityInFront(self, self->enemy)){
		if (random() <= 0.7f)
			self->monsterinfo.currentmove = &hornet_machinegunAttackMove;
		else
			self->monsterinfo.currentmove = &hornet_postMachinegunAttackMove;
	}
	else
		self->monsterinfo.currentmove = &hornet_postMachinegunAttackMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			hornet_heavyPainFrames[] = {
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

static mframe_t			hornet_lightPainFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			hornet_heavyPainMove = {FRAME_pain2, FRAME_pain19, hornet_heavyPainFrames, Hornet_RunAnimation};
static mmove_t			hornet_lightPainMove = {FRAME_pain20, FRAME_pain23, hornet_lightPainFrames, Hornet_RunAnimation};


/*
 ==================
 Hornet_PainAnimation
 ==================
*/
static void Hornet_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	if (damage < 10){
		Hornet_Pain3Sound(self);
		self->monsterinfo.currentmove = &hornet_lightPainMove;
	}
	else if (damage < 30){
		Hornet_Pain1Sound(self);
		self->monsterinfo.currentmove = &hornet_lightPainMove;
	}
	else {
		Hornet_Pain2Sound(self);
		self->monsterinfo.currentmove = &hornet_heavyPainMove;
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			hornet_deathFrames[] = {
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
	{SG_AIMove,		0.0f,	SG_MonsterExplode}
};

static mmove_t			hornet_deathMove = {FRAME_death2, FRAME_death50, hornet_deathFrames, Hornet_Dead};


/*
 ==================
 Hornet_Dead
 ==================
*/
static void Hornet_Dead (edict_t *self){

	VectorSet(self->mins, -56.0f, -56.0f, 0.0f);
	VectorSet(self->maxs, 56.0f, 56.0f, 80.0f);

	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Hornet_DieAnimation
 ==================
*/
static void Hornet_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	Hornet_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->count = 0;
	self->monsterinfo.currentmove = &hornet_deathMove;
}


/*
 ==============================================================================

 MONSTER_BOSS2

 ==============================================================================
*/


/*
 ==================
 SG_MonsterHornet_Spawn
 ==================
*/
void SG_MonsterHornet_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/boss2/tris.md2");

	VectorSet(self->mins, -56.0f, -56.0f, 0.0f);
	VectorSet(self->maxs, 56.0f, 56.0f, 80.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	hornet.pain1Sound = gi.soundindex("bosshovr/bhvpain1.wav");
	hornet.pain2Sound = gi.soundindex("bosshovr/bhvpain2.wav");
	hornet.pain3Sound = gi.soundindex("bosshovr/bhvpain3.wav");
	hornet.dieSound = gi.soundindex("bosshovr/bhvdeth1.wav");
	hornet.searchSound = gi.soundindex("bosshovr/bhvunqv1.wav");

	self->s.sound = gi.soundindex("bosshovr/bhvengn1.wav");

	self->mass = 1000;

	self->health = 2000;
	self->gib_health = -200;

	self->flags |= FL_IMMUNE_LASER;

	self->pain = Hornet_PainAnimation;
	self->die = Hornet_DieAnimation;

	self->monsterinfo.stand = Hornet_StandAnimation;
	self->monsterinfo.walk = Hornet_WalkAnimation;
	self->monsterinfo.run = Hornet_RunAnimation;
	self->monsterinfo.attack = Hornet_AttackAnimation;
	self->monsterinfo.search = Hornet_SearchSound;
	self->monsterinfo.checkattack = SG_MonsterCheckAttack2;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &hornet_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_AirMonster(self);
}