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
// m_infantry.c - Jorg monster
//


#include "g_local.h"
#include "m_boss31.h"


typedef struct {
	int					sightSound;
	int					idleSound;
	int					pain1Sound;
	int					pain2Sound;
	int					pain3Sound;
	int					dieSound;
	int					search1Sound;
	int					search2Sound;
	int					search3Sound;
	int					attack1Sound;
	int					attack2Sound;
	int					fireGunSound;
	int					stepLeftSound;
	int					stepRightSound;
	int					deathHitSound;
} jorg_t;

static jorg_t			jorg;

static void Jorg_Attack1Animation (edict_t *self);
static void Jorg_ReAttack1Animation (edict_t *self);
void Makron_JumpOut (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Jorg_IdleSound
 ==================
*/
static void Jorg_IdleSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, jorg.idleSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Jorg_Pain1Sound
 ==================
*/
static void Jorg_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, jorg.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Jorg_Pain2Sound
 ==================
*/
static void Jorg_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, jorg.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Jorg_Pain3Sound
 ==================
*/
static void Jorg_Pain3Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, jorg.pain3Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Jorg_DieSound
 ==================
*/
static void Jorg_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, jorg.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Jorg_SearchSound
 ==================
*/
static void Jorg_SearchSound (edict_t *self){

	float	r;

	r = random();

	if (r <= 0.3f)
		gi.sound(self, CHAN_VOICE, jorg.search1Sound, 1.0f, ATTN_NORM, 0.0f);
	else if (r <= 0.6f)
		gi.sound(self, CHAN_VOICE, jorg.search2Sound, 1.0f, ATTN_NORM, 0.0f);
	else
		gi.sound(self, CHAN_VOICE, jorg.search3Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Jorg_Attack1Sound
 ==================
*/
static void Jorg_Attack1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, jorg.attack1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Jorg_Attack2Sound
 ==================
*/
static void Jorg_Attack2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, jorg.attack2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Jorg_StepLeftSound
 ==================
*/
static void Jorg_StepLeftSound (edict_t *self){

	gi.sound(self, CHAN_BODY, jorg.stepLeftSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Jorg_StepRightSound
 ==================
*/
static void Jorg_StepRightSound (edict_t *self){

	gi.sound(self, CHAN_BODY, jorg.stepRightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Jorg_DeathHitSound
 ==================
*/
static void Jorg_DeathHitSound (edict_t *self){

	gi.sound(self, CHAN_BODY, jorg.deathHitSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			jorg_standFrames[] = {
	{SG_AIStand,	0.0f,	Jorg_IdleSound},
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
	{SG_AIStand,   19.0f,	NULL},
	{SG_AIStand,   11.0f,	Jorg_StepLeftSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	6.0f,	NULL},
	{SG_AIStand,	9.0f,	Jorg_StepRightSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,   -2.0f,	NULL},
	{SG_AIStand,  -17.0f,	Jorg_StepLeftSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,  -12.0f,	NULL},
	{SG_AIStand,  -14.0f,	Jorg_StepRightSound}
};

static mmove_t			jorg_standMove = {FRAME_stand01, FRAME_stand51, jorg_standFrames, NULL};


/*
 ==================
 Jorg_StandAnimation
 ==================
*/
static void Jorg_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &jorg_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			jorg_startWalkFrames[] = {
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,		9.0f,	NULL},
	{SG_AIWalk,	   15.0f,	NULL}
};

static mframe_t			jorg_walkFrames[] = {
	{SG_AIWalk,    17.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,    12.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,    10.0f,	NULL},
	{SG_AIWalk,    33.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		9.0f,	NULL},
	{SG_AIWalk,		9.0f,	NULL},
	{SG_AIWalk,		9.0f,	NULL}
};

static mframe_t			jorg_endWalkFrames[] = {
	{SG_AIWalk,	   11.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,	   -8.0f,	NULL}
};

static mmove_t			jorg_startWalkMove = {FRAME_walk01, FRAME_walk05, jorg_startWalkFrames, NULL};
static mmove_t			jorg_walkMove = {FRAME_walk06, FRAME_walk19, jorg_walkFrames, NULL};
static mmove_t			jorg_endWalkMove = {FRAME_walk20, FRAME_walk25, jorg_endWalkFrames, NULL};


/*
 ==================
 Jorg_WalkAnimation
 ==================
*/
static void Jorg_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &jorg_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			jorg_runFrames[] = {
	{SG_AIRun,	   17.0f,	Jorg_StepLeftSound},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   33.0f,	Jorg_StepRightSound},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,		9.0f,	NULL},
	{SG_AIRun,		9.0f,	NULL},
	{SG_AIRun,		9.0f,	NULL}
};

static mmove_t			jorg_runMove = {FRAME_walk06, FRAME_walk19, jorg_runFrames, NULL};


/*
 ==================
 Jorg_RunAnimation
 ==================
*/
static void Jorg_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &jorg_standMove;
	else
		self->monsterinfo.currentmove = &jorg_runMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Jorg_RunAnimation
 ==================
*/
static void Jorg_BFK (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_JORG_BFG_1], forward, right, start);

	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	Jorg_Attack2Sound(self);

	SG_MonsterFireBFG10K(self, start, dir, 50, 300, 100, 200, MZ2_JORG_BFG_1);
}

/*
 ==================
 Jorg_FireBullet_Left
 ==================
*/
static void Jorg_FireBullet_Left (edict_t *self){

	vec3_t forward, right;
	vec3_t start;
	vec3_t target;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_JORG_MACHINEGUN_L1], forward, right, start);

	VectorMA(self->enemy->s.origin, -0.2f, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, forward);
	VectorNormalize(forward);

	SG_MonsterFireBullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_JORG_MACHINEGUN_L1);
}

/*
 ==================
 Jorg_FireBullet_Right
 ==================
*/
static void Jorg_FireBullet_Right (edict_t *self){

	vec3_t forward, right;
	vec3_t start;
	vec3_t target;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_JORG_MACHINEGUN_R1], forward, right, start);

	VectorMA(self->enemy->s.origin, -0.2f, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, forward);
	VectorNormalize(forward);

	SG_MonsterFireBullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_JORG_MACHINEGUN_R1);
}

/*
 ==================
 Jorg_Machinegun
 ==================
*/
static void Jorg_Machinegun (edict_t *self){

	Jorg_FireBullet_Left(self);
	Jorg_FireBullet_Right(self);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			jorg_attack2Frames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Jorg_BFK},		
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			jorg_startAttack1Frames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			jorg_attack1Frames[] = {
	{SG_AICharge,	0.0f,	Jorg_Machinegun},
	{SG_AICharge,	0.0f,	Jorg_Machinegun},
	{SG_AICharge,	0.0f,	Jorg_Machinegun},
	{SG_AICharge,	0.0f,	Jorg_Machinegun},
	{SG_AICharge,	0.0f,	Jorg_Machinegun},
	{SG_AICharge,	0.0f,	Jorg_Machinegun}
};

static mframe_t			jorg_endAttack1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			jorg_attack2Move = {FRAME_attak201, FRAME_attak213, jorg_attack2Frames, Jorg_RunAnimation};
static mmove_t			jorg_startAttack1Move = {FRAME_attak101, FRAME_attak108, jorg_startAttack1Frames, Jorg_Attack1Animation};
static mmove_t			jorg_attack1Move = {FRAME_attak109, FRAME_attak114, jorg_attack1Frames, Jorg_ReAttack1Animation};
static mmove_t			jorg_endAttack1Move = {FRAME_attak115, FRAME_attak118, jorg_endAttack1Frames, Jorg_RunAnimation};


/*
 ==================
 Jorg_Attack1Animation
 ==================
*/
static void Jorg_Attack1Animation (edict_t *self){

	self->monsterinfo.currentmove = &jorg_attack1Move;
}

/*
 ==================
 Jorg_ReAttack1Animation
 ==================
*/
static void Jorg_ReAttack1Animation (edict_t *self){

	if (SG_IsEntityVisible(self, self->enemy)){
		if (random() < 0.9f)
			self->monsterinfo.currentmove = &jorg_attack1Move;
		else {
			self->s.sound = 0;
			self->monsterinfo.currentmove = &jorg_endAttack1Move;	
		}
	}
	else {
		self->s.sound = 0;
		self->monsterinfo.currentmove = &jorg_endAttack1Move;	
	}
}

/*
 ==================
 Jorg_AttackAnimation
 ==================
*/
static void Jorg_AttackAnimation (edict_t *self){

	vec3_t	vec;
	float	range;

	VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
	range = VectorLength(vec);

	if (random() <= 0.75f){
		Jorg_Attack1Sound(self);
		self->s.sound = gi.soundindex("boss3/w_loop.wav");
		self->monsterinfo.currentmove = &jorg_startAttack1Move;
	}
	else {
		Jorg_Attack2Sound(self);
		self->monsterinfo.currentmove = &jorg_attack2Move;
	}
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			jorg_pain3Frames[] = {
	{SG_AIMove,	  -28.0f,	NULL},
	{SG_AIMove,	   -6.0f,	NULL},
	{SG_AIMove,	   -3.0f,	Jorg_StepLeftSound},
	{SG_AIMove,	   -9.0f,	NULL},
	{SG_AIMove,		0.0f,	Jorg_StepRightSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -7.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,	  -11.0f,	NULL},
	{SG_AIMove,	   -4.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   10.0f,	NULL},
	{SG_AIMove,	   11.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   10.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,	   10.0f,	NULL},
	{SG_AIMove,		7.0f,	Jorg_StepLeftSound},
	{SG_AIMove,	   17.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	Jorg_StepRightSound}
};

static mframe_t			jorg_pain2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			jorg_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			jorg_pain3Move = {FRAME_pain301, FRAME_pain325, jorg_pain3Frames, Jorg_RunAnimation};
static mmove_t			jorg_pain2Move = {FRAME_pain201, FRAME_pain203, jorg_pain2Frames, Jorg_RunAnimation};
static mmove_t			jorg_pain1Move = {FRAME_pain101, FRAME_pain103, jorg_pain1Frames, Jorg_RunAnimation};


/*
 ==================
 Jorg_PainAnimation
 ==================
*/
static void Jorg_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	self->s.sound = 0;

	if (level.time < self->pain_debounce_time)
		return;

	// Lessen the chance of him going into his pain frames if he takes little damage
	if (damage <= 40){
		if (random() <= 0.6f)
			return;
	}

	// If he's entering his attack1 or using attack1, lessen the chance of him
	// going into pain
	if ((self->s.frame >= FRAME_attak101) && (self->s.frame <= FRAME_attak108)){
		if (random() <= 0.005f)
			return;
	}

	if ((self->s.frame >= FRAME_attak109) && (self->s.frame <= FRAME_attak114)){
		if (random() <= 0.00005f)
			return;
	}

	if ((self->s.frame >= FRAME_attak201) && (self->s.frame <= FRAME_attak208)){
		if (random() <= 0.005f)
			return;
	}

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	if (damage <= 50){
		Jorg_Pain1Sound(self);
		self->monsterinfo.currentmove = &jorg_pain1Move;
	}
	else if (damage <= 100){
		Jorg_Pain2Sound(self);
		self->monsterinfo.currentmove = &jorg_pain2Move;
	}
	else {
		if (random() <= 0.3f){
			Jorg_Pain3Sound(self);
			self->monsterinfo.currentmove = &jorg_pain3Move;
		}
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			jorg_death1Frames[] = {
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
	{SG_AIMove,		0.0f,	Makron_JumpOut},
	{SG_AIMove,		0.0f,	SG_MonsterExplode}
};

static mmove_t			jorg_deathMove = {FRAME_death01, FRAME_death50, jorg_death1Frames, NULL};


/*
 ==================
 Jorg_DieAnimation
 ==================
*/
static void Jorg_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	Jorg_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->s.sound = 0;
	self->count = 0;
	self->monsterinfo.currentmove = &jorg_deathMove;
}


/*
 ==============================================================================

 MONSTER_JORG

 ==============================================================================
*/


/*
 ==================
 SG_MonsterJorg_Spawn
 ==================
*/
void SG_MonsterJorg_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/boss3/rider/tris.md2");
	self->s.modelindex2 = gi.modelindex("models/monsters/boss3/jorg/tris.md2");

	VectorSet(self->mins, -80.0f, -80.0f, 0.0f);
	VectorSet(self->maxs, 80.0f, 80.0f, 140.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	jorg.idleSound = gi.soundindex("boss3/bs3idle1.wav");
	jorg.pain1Sound = gi.soundindex("boss3/bs3pain1.wav");
	jorg.pain2Sound = gi.soundindex("boss3/bs3pain2.wav");
	jorg.pain3Sound = gi.soundindex("boss3/bs3pain3.wav");
	jorg.dieSound = gi.soundindex("boss3/bs3deth1.wav");
	jorg.search1Sound = gi.soundindex("boss3/bs3srch1.wav");
	jorg.search2Sound = gi.soundindex("boss3/bs3srch2.wav");
	jorg.search3Sound = gi.soundindex("boss3/bs3srch3.wav");
	jorg.attack1Sound = gi.soundindex("boss3/bs3atck1.wav");
	jorg.attack2Sound = gi.soundindex("boss3/bs3atck2.wav");
	jorg.fireGunSound = gi.soundindex("boss3/xfire.wav");
	jorg.stepLeftSound = gi.soundindex("boss3/step1.wav");
	jorg.stepRightSound = gi.soundindex("boss3/step2.wav");	
	jorg.deathHitSound = gi.soundindex("boss3/d_hit.wav");

//	MakronPrecache ();

	self->mass = 1000;

	self->health = 3000;
	self->gib_health = -2000;

	self->pain = Jorg_PainAnimation;
	self->die = Jorg_DieAnimation;

	self->monsterinfo.stand = Jorg_StandAnimation;
	self->monsterinfo.walk = Jorg_WalkAnimation;
	self->monsterinfo.run = Jorg_RunAnimation;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = Jorg_AttackAnimation;
	self->monsterinfo.search = Jorg_SearchSound;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = NULL;
	self->monsterinfo.checkattack = SG_MonsterCheckAttack;

	gi.linkentity(self);
	
	self->monsterinfo.currentmove = &jorg_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}