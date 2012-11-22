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
// m_mutant.c - Mutant monster
//

// NOTE:
// - ID Software left a FIXME in Mutant_Check_Attack


#include "g_local.h"
#include "m_mutant.h"


typedef struct {
	int					sightSound;
	int					idleSound;
	int					pain1Sound;
	int					pain2Sound;
	int					dieSound;
	int					searchSound;
	int					swingSound;
	int					hit1Sound;
	int					hit2Sound;
	int					step1Sound;
	int					step2Sound;
	int					step3Sound;
	int					thudSound;
} mutant_t;

static mutant_t			mutant;

static void Mutant_Idle_Loop (edict_t *self);
static void Mutant_Walk_Loop (edict_t *self);
static void Mutant_Check_ReFire (edict_t *self);
static void Mutant_Check_Landing (edict_t *self);
static void Mutant_Dead (edict_t *self);

/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Mutant_SightSound
 ==================
*/
static void Mutant_SightSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, mutant.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Mutant_SightSound2
 ==================
*/
static void Mutant_SightSound2 (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, mutant.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Mutant_IdleSound
 ==================
*/
static void Mutant_IdleSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, mutant.idleSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Mutant_Pain1Sound
 ==================
*/
static void Mutant_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, mutant.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Mutant_Pain2Sound
 ==================
*/
static void Mutant_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE,  mutant.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Mutant_DieSound
 ==================
*/
static void Mutant_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, mutant.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Mutant_SearchSound
 ==================
*/
static void Mutant_SearchSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, mutant.searchSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Mutant_SwingSound
 ==================
*/
static void Mutant_SwingSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, mutant.swingSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Mutant_Hit1Sound
 ==================
*/
static void Mutant_Hit1Sound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, mutant.hit1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Mutant_Hit2Sound
 ==================
*/
static void Mutant_Hit2Sound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, mutant.hit2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Mutant_StepSound
 ==================
*/
static void Mutant_StepSound (edict_t *self){

	int		n;

	n = (rand() + 1) % 3;

	if (n == 0)
		gi.sound(self, CHAN_VOICE, mutant.step1Sound, 1.0f, ATTN_NORM, 0.0f);		
	else if (n == 1)
		gi.sound(self, CHAN_VOICE, mutant.step2Sound, 1.0f, ATTN_NORM, 0.0f);
	else
		gi.sound(self, CHAN_VOICE, mutant.step3Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Mutant_ThudSound
 ==================
*/
static void Mutant_ThudSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, mutant.thudSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			mutant_standFrames[] = {
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
	SG_AIStand, 0, NULL
};

static mmove_t			mutant_standMove = {FRAME_stand101, FRAME_stand151, mutant_standFrames, NULL};


/*
 ==================
 Mutant_StandAnimation
 ==================
*/
static void Mutant_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &mutant_standMove;
}


/*
 ==============================================================================

 IDLE ANIMATION

 ==============================================================================
*/

static mframe_t			mutant_idleFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Mutant_Idle_Loop},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL}
};

static mmove_t			mutant_idleMove = {FRAME_stand152, FRAME_stand164, mutant_idleFrames, Mutant_StandAnimation};


/*
 ==================
 Mutant_Idle_Loop
 ==================
*/
static void Mutant_Idle_Loop (edict_t *self){

	if (random() < 0.75f)
		self->monsterinfo.nextframe = FRAME_stand155;
}

/*
 ==================
 Mutant_IdleAnimation
 ==================
*/
static void Mutant_IdleAnimation (edict_t *self){

	self->monsterinfo.currentmove = &mutant_idleMove;
	Mutant_IdleSound(self);
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			mutant_walkFrames[] = {
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		1.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,	   10.0f,	NULL},
	{SG_AIWalk,	   13.0f,	NULL},
	{SG_AIWalk,	   10.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,	   16.0f,	NULL},
	{SG_AIWalk,	   15.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL}
};

static mframe_t			mutant_startWalkFrames[] = {
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,	   -2.0f,	NULL},
	{SG_AIWalk,		1.0f,	NULL}
};

static mmove_t			mutant_walkMove = {FRAME_walk05, FRAME_walk16, mutant_walkFrames, NULL};
static mmove_t			mutant_startWalkMove = {FRAME_walk01, FRAME_walk04, mutant_startWalkFrames, Mutant_Walk_Loop};


/*
 ==================
 Mutant_Walk_Loop
 ==================
*/
static void Mutant_Walk_Loop (edict_t *self){

	self->monsterinfo.currentmove = &mutant_walkMove;
}

/*
 ==================
 Mutant_WalkAnimation
 ==================
*/
static void Mutant_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &mutant_startWalkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			mutant_runFrames[] = {
	{SG_AIRun,	   40.0f,	NULL},
	{SG_AIRun,	   40.0f,	Mutant_StepSound},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,		5.0f,	Mutant_StepSound},
	{SG_AIRun,	   17.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL}
};

static mmove_t			mutant_runMove = {FRAME_run03, FRAME_run08, mutant_runFrames, NULL};


/*
 ==================
 Mutant_RunAnimation
 ==================
*/
static void Mutant_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &mutant_standMove;
	else
		self->monsterinfo.currentmove = &mutant_runMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Mutant_Hit_Left
 ==================
*/
static void Mutant_Hit_Left (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 8.0f);

	if (SG_MeleeHit(self, aim, (10 + (rand() % 5)), 100))
		Mutant_Hit1Sound(self);
	else
		Mutant_SwingSound(self);
}

/*
 ==================
 Mutant_Hit_Right
 ==================
*/
static void Mutant_Hit_Right (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, self->maxs[0], 8.0f);

	if (SG_MeleeHit(self, aim, (10 + (rand() % 5)), 100))
		Mutant_Hit2Sound(self);
	else
		Mutant_SwingSound(self);
}

/*
 ==================
 Mutant_Jump_Touch
 ==================
*/
static void Mutant_Jump_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	vec3_t	normal;
	vec3_t	point;
	int		damage;

	if (self->health <= 0){
		self->touch = NULL;
		return;
	}

	if (other->takedamage){
		if (VectorLength(self->velocity) > 400.0f){
			VectorCopy(self->velocity, normal);
			VectorNormalize(normal);
			VectorMA(self->s.origin, self->maxs[0], normal, point);

			damage = 40 + 10 * random();
			SG_TargetDamage(other, self, self, self->velocity, point, normal, damage, damage, 0, MOD_UNKNOWN);
		}
	}

	if (!SG_MonsterCheckBottom(self)){
		if (self->groundentity){
			self->monsterinfo.nextframe = FRAME_attack02;
			self->touch = NULL;
		}

		return;
	}

	self->touch = NULL;
}

/*
 ==================
 Mutant_Jump_Takeoff
 ==================
*/
static void Mutant_Jump_Takeoff (edict_t *self){

	vec3_t	forward;

	Mutant_SightSound(self);

	AngleVectors(self->s.angles, forward, NULL, NULL);
	self->s.origin[2] += 1.0f;
	VectorScale(forward, 600.0f, self->velocity);
	self->velocity[2] = 250.0f;
	self->groundentity = NULL;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->monsterinfo.attack_finished = level.time + 3.0f;
	self->touch = Mutant_Jump_Touch;
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			mutant_attackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Mutant_Hit_Left},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Mutant_Hit_Right},
	{SG_AICharge,	0.0f,	Mutant_Check_ReFire}
};

static mframe_t			mutant_jumpFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,  17.0f,	NULL},
	{SG_AICharge,  15.0f,	Mutant_Jump_Takeoff},
	{SG_AICharge,  15.0f,	NULL},
	{SG_AICharge,  15.0f,	Mutant_Check_Landing},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	3.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mmove_t			mutant_attackMove = {FRAME_attack09, FRAME_attack15, mutant_attackFrames, Mutant_RunAnimation};
static mmove_t			mutant_jumpMove = {FRAME_attack01, FRAME_attack08, mutant_jumpFrames, Mutant_RunAnimation};


/*
 ==================
 Mutant_MeleeAnimation
 ==================
*/
static void Mutant_MeleeAnimation (edict_t *self){

	self->monsterinfo.currentmove = &mutant_attackMove;
}

/*
 ==================
 Mutant_JumpAnimation
 ==================
*/
static void Mutant_JumpAnimation (edict_t *self){

	self->monsterinfo.currentmove = &mutant_jumpMove;
}


/*
 ==============================================================================

 WEAPON CHECK

 ==============================================================================
*/


/*
 ==================
 Mutant_Check_ReFire
 ==================
*/
static void Mutant_Check_ReFire (edict_t *self){

	if (!self->enemy || !self->enemy->inuse || self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (SG_EntityRange(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attack09;
}

/*
 ==================
 Mutant_Check_Landing
 ==================
*/
static void Mutant_Check_Landing (edict_t *self){

	if (self->groundentity){
		Mutant_ThudSound(self);

		self->monsterinfo.attack_finished = 0.0f;
		self->monsterinfo.aiflags &= ~AI_DUCKED;
		return;
	}

	if (level.time > self->monsterinfo.attack_finished)
		self->monsterinfo.nextframe = FRAME_attack02;
	else
		self->monsterinfo.nextframe = FRAME_attack05;
}

/*
 ==================
 Mutant_Check_Melee
 ==================
*/
static qboolean Mutant_Check_Melee (edict_t *self){

	if (SG_EntityRange(self, self->enemy) == RANGE_MELEE)
		return true;

	return false;
}

/*
 ==================
 Mutant_Check_Jump
 ==================
*/
static qboolean Mutant_Check_Jump (edict_t *self){

	vec3_t	v;
	float	distance;

	if (self->absmin[2] > (self->enemy->absmin[2] + 0.75f * self->enemy->size[2]))
		return false;

	if (self->absmax[2] < (self->enemy->absmin[2] + 0.25f * self->enemy->size[2]))
		return false;

	v[0] = self->s.origin[0] - self->enemy->s.origin[0];
	v[1] = self->s.origin[1] - self->enemy->s.origin[1];
	v[2] = 0.0f;

	distance = VectorLength(v);

	if (distance < 100.0f)
		return false;

	if (distance > 100.0f){
		if (random() < 0.9f)
			return false;
	}

	return true;
}

/*
 ==================
 Mutant_Check_Attack
 ==================
*/
static qboolean Mutant_Check_Attack (edict_t *self){

	if (!self->enemy || self->enemy->health <= 0)
		return false;

	if (Mutant_Check_Melee(self)){
		self->monsterinfo.attack_state = AS_MELEE;
		return true;
	}

	if (Mutant_Check_Jump(self)){
		self->monsterinfo.attack_state = AS_MISSILE;
		// FIXME: Play a jump sound here
		return true;
	}

	return false;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			mutant_pain1Frames[] = {
	{SG_AIMove,		4.0f,	NULL},
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,	   -8.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		5.0f,	NULL}
};

static mframe_t			mutant_pain2Frames[] = {
	{SG_AIMove,	  -24.0f,	NULL},
	{SG_AIMove,	   11.0f,	NULL},
	{SG_AIMove,		5.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,		6.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL}
};

static mframe_t			mutant_pain3Frames[] = {
	{SG_AIMove,	   -22.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		6.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL}
};

static mmove_t			mutant_pain1Move = {FRAME_pain101, FRAME_pain105, mutant_pain1Frames, Mutant_RunAnimation};
static mmove_t			mutant_pain2Move = {FRAME_pain201, FRAME_pain206, mutant_pain2Frames, Mutant_RunAnimation};
static mmove_t			mutant_pain3Move = {FRAME_pain301, FRAME_pain311, mutant_pain3Frames, Mutant_RunAnimation};


/*
 ==================
 Mutant_PainAnimation
 ==================
*/
static void Mutant_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	float	r;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	r = random();

	if (r < 0.33f){
		Mutant_Pain1Sound(self);
		self->monsterinfo.currentmove = &mutant_pain1Move;
	}
	else if (r < 0.66f){
		Mutant_Pain2Sound(self);
		self->monsterinfo.currentmove = &mutant_pain2Move;
	}
	else {
		Mutant_Pain1Sound(self);
		self->monsterinfo.currentmove = &mutant_pain3Move;
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			mutant_death1Frames[] = {
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL}
};

static mframe_t			mutant_death2Frames[] = {
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL}
};

static mmove_t			mutant_death1Move = {FRAME_death101, FRAME_death109, mutant_death1Frames, Mutant_Dead};
static mmove_t			mutant_death2Move = {FRAME_death201, FRAME_death210, mutant_death2Frames, Mutant_Dead};


/*
 ==================
 Mutant_Dead
 ==================
*/
static void Mutant_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;

	gi.linkentity(self);

	SG_FlyCheck(self);
}

/*
 ==================
 Mutant_DieAnimation
 ==================
*/
static void Mutant_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

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
	Mutant_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->s.skinnum = 1;

	if (random() < 0.5f)
		self->monsterinfo.currentmove = &mutant_death1Move;
	else
		self->monsterinfo.currentmove = &mutant_death2Move;
}


/*
 ==============================================================================

 MONSTER_MUTANT

 ==============================================================================
*/


/*
 ==================
 SG_MonsterMutant_Spawn
 ==================
*/
void SG_MonsterMutant_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/mutant/tris.md2");

	VectorSet(self->mins, -32.0f, -32.0f, -24.0f);
	VectorSet(self->maxs, 32.0f, 32.0f, 48.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	mutant.sightSound = gi.soundindex("mutant/mutsght1.wav");
	mutant.idleSound = gi.soundindex("mutant/mutidle1.wav");
	mutant.pain1Sound = gi.soundindex("mutant/mutpain1.wav");
	mutant.pain2Sound = gi.soundindex("mutant/mutpain2.wav");
	mutant.dieSound = gi.soundindex("mutant/mutdeth1.wav");
	mutant.searchSound = gi.soundindex("mutant/mutsrch1.wav");
	mutant.swingSound = gi.soundindex("mutant/mutatck1.wav");
	mutant.hit1Sound = gi.soundindex("mutant/mutatck2.wav");
	mutant.hit2Sound = gi.soundindex("mutant/mutatck3.wav");
	mutant.step1Sound = gi.soundindex("mutant/step1.wav");
	mutant.step2Sound = gi.soundindex("mutant/step2.wav");
	mutant.step3Sound = gi.soundindex("mutant/step3.wav");
	mutant.thudSound = gi.soundindex("mutant/thud1.wav");

	self->mass = 300;

	self->health = 300;
	self->gib_health = -120;

	self->pain = Mutant_PainAnimation;
	self->die = Mutant_DieAnimation;

	self->monsterinfo.stand = Mutant_StandAnimation;
	self->monsterinfo.walk = Mutant_WalkAnimation;
	self->monsterinfo.run = Mutant_RunAnimation;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = Mutant_JumpAnimation;
	self->monsterinfo.melee = Mutant_MeleeAnimation;
	self->monsterinfo.sight = Mutant_SightSound2;
	self->monsterinfo.search = Mutant_SearchSound;
	self->monsterinfo.idle = Mutant_IdleSound;
	self->monsterinfo.checkattack = Mutant_Check_Attack;

	gi.linkentity(self);
	
	self->monsterinfo.currentmove = &mutant_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}
