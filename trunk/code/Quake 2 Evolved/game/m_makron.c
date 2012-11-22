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
// m_makron.c - Makron monster
//

// TODO:
// - Makron_Railgun, Makron_Hyperblaster, Makron_CheckAttack


#include "g_local.h"
#include "m_boss32.h"


typedef struct {
	int					pain1Sound;
	int					pain2Sound;
	int					pain3Sound;
	int					dieSound;
	int					stepLeftSound;
	int					stepRightSound;
	int					BFGAttackSound;
	int					brainSplorchSound;
	int					preRailgunSound;
	int					popupSound;
	int					taunt1Sound;
	int					taunt2Sound;
	int					taunt3Sound;
	int					hitSound;
} makron_t;

static makron_t			makron;

static void Makron_SaveAimingLocaction (edict_t *self);
static void Makron_Dead (edict_t *self);
extern void SG_MonsterMakron_Spawn (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Makron_Pain1Sound
 ==================
*/
static void Makron_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, makron.pain1Sound, 1.0f, ATTN_NONE, 0.0f);
}

/*
 ==================
 Makron_Pain2Sound
 ==================
*/
static void Makron_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, makron.pain2Sound, 1.0f, ATTN_NONE, 0.0f);
}

/*
 ==================
 Makron_Pain3Sound
 ==================
*/
static void Makron_Pain3Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, makron.pain3Sound, 1.0f, ATTN_NONE, 0.0f);
}

/*
 ==================
 Makron_DieSound
 ==================
*/
static void Makron_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, makron.dieSound, 1.0f, ATTN_NONE, 0.0f);
}

/*
 ==================
 Makron_StepLeftSound
 ==================
*/
static void Makron_StepLeftSound (edict_t *self){

	gi.sound(self, CHAN_BODY, makron.stepLeftSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Makron_StepRightSound
 ==================
*/
static void Makron_StepRightSound (edict_t *self){

	gi.sound(self, CHAN_BODY, makron.stepRightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Makron_BFGAttackSound
 ==================
*/
static void Makron_BFGAttackSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, makron.BFGAttackSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Makron_BrainSplorchSound
 ==================
*/
static void Makron_BrainSplorchSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, makron.brainSplorchSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Makron_PreRailgunSound
 ==================
*/
static void Makron_PreRailgunSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, makron.preRailgunSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Makron_PopupSound
 ==================
*/
static void Makron_PopupSound (edict_t *self){

	gi.sound(self, CHAN_BODY, makron.popupSound, 1.0f, ATTN_NONE, 0.0f);
}

/*
 ==================
 Makron_TauntSound
 ==================
*/
static void Makron_TauntSound (edict_t *self){

	float	r;

	r = random();

	if (r <= 0.3f)
		gi.sound(self, CHAN_AUTO, makron.taunt1Sound, 1.0f, ATTN_NONE, 0.0f);
	else if (r <= 0.6f)
		gi.sound(self, CHAN_AUTO, makron.taunt2Sound, 1.0f, ATTN_NONE, 0.0f);
	else
		gi.sound(self, CHAN_AUTO, makron.taunt3Sound, 1.0f, ATTN_NONE, 0.0f);
}

/*
 ==================
 Makron_HitSound
 ==================
*/
static void Makron_HitSound (edict_t *self){

	gi.sound(self, CHAN_AUTO, makron.hitSound, 1.0f, ATTN_NONE, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			makron_standFrames[] = {
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

static mmove_t			makron_standMove = {FRAME_stand201, FRAME_stand260, makron_standFrames, NULL};


/*
 ==================
 Makron_StandAnimation
 ==================
*/
static void Makron_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &makron_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			makron_walkFrames[] = {
	{SG_AIWalk,		3.0f,	Makron_StepLeftSound},
	{SG_AIWalk,	   12.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		8.0f,	Makron_StepRightSound},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,	   12.0f,	NULL},
	{SG_AIWalk,		9.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,	   12.0f,	NULL}
};

static mmove_t			makron_walkMove = {FRAME_walk204, FRAME_walk213, makron_walkFrames, NULL};


/*
 ==================
 Makron_WalkAnimation
 ==================
*/
static void Makron_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &makron_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			makron_runFrames[] = {
	{SG_AIRun,		3.0f,	Makron_StepLeftSound},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	Makron_StepRightSound},
	{SG_AIRun,		6.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,		9.0f,	NULL},
	{SG_AIRun,		6.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL}
};

static mmove_t			makron_runMove = {FRAME_walk204, FRAME_walk213, makron_runFrames, NULL};


/*
 ==================
 Makron_RunAnimation
 ==================
*/
static void Makron_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &makron_standMove;
	else
		self->monsterinfo.currentmove = &makron_runMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Makron_BFG
 ==================
*/
static void Makron_BFG (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_MAKRON_BFG], forward, right, start);

	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	Makron_BFGAttackSound(self);

	SG_MonsterFireBFG10K(self, start, dir, 50, 300, 100, 300, MZ2_MAKRON_BFG);
}

/*
 ==================
 Makron_Railgun

 FIXME: He's not firing from the proper Z
 ==================
*/
static void Makron_Railgun (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_MAKRON_RAILGUN_1], forward, right, start);

	// Calculate direction to where we targted
	VectorSubtract(self->pos1, start, dir);
	VectorNormalize(dir);

	SG_MonsterFireRailgun(self, start, dir, 50, 100, MZ2_MAKRON_RAILGUN_1);
}

/*
 ==================
 Makron_Hyperblaster

 FIXME: This is all wrong. He's not firing at the proper angles.
 ==================
*/
static void Makron_Hyperblaster (edict_t *self){

	vec3_t	forward, right;
	vec3_t	dir;
	vec3_t	vec;
	vec3_t	start;
	int		flashNumber;

	flashNumber = MZ2_MAKRON_BLASTER_1 + (self->s.frame - FRAME_attak405);

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

	if (self->enemy){
		VectorCopy(self->enemy->s.origin, vec);
		vec[2] += self->enemy->viewheight;
		VectorSubtract(vec, start, vec);
		SG_VectorToAngles(vec, vec);
		dir[0] = vec[0];
	}
	else
		dir[0] = 0.0f;

	if (self->s.frame <= FRAME_attak413)
		dir[1] = self->s.angles[1] - 10.0f * (self->s.frame - FRAME_attak413);
	else
		dir[1] = self->s.angles[1] + 10.0f * (self->s.frame - FRAME_attak421);

	dir[2] = 0.0f;

	AngleVectors(dir, forward, NULL, NULL);

	SG_MonsterFireBlaster(self, start, forward, 15, 1000, MZ2_MAKRON_BLASTER_1, EF_BLASTER);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			makron_BFGAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Makron_BFG},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			makron_hyperblasterAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	Makron_Hyperblaster},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			makron_railgunAttackFrames[] = {
	{SG_AICharge,	0.0f,	Makron_PreRailgunSound},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Makron_SaveAimingLocaction},
	{SG_AIMove,		0.0f,	Makron_Railgun},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			makron_BFGAttackMove = {FRAME_attak301, FRAME_attak308, makron_BFGAttackFrames, Makron_RunAnimation};
static mmove_t			makron_hyperblasterAttackMove = {FRAME_attak401, FRAME_attak426, makron_hyperblasterAttackFrames, Makron_RunAnimation};
static mmove_t			makron_railgunAttackMove = {FRAME_attak501, FRAME_attak516, makron_railgunAttackFrames, Makron_RunAnimation};


/*
 ==================
 Makron_SaveAimingLocaction
 ==================
*/
static void Makron_SaveAimingLocaction (edict_t *self){

	// Save for aiming the shot
	VectorCopy(self->enemy->s.origin, self->pos1);
	self->pos1[2] += self->enemy->viewheight;
}

/*
 ==================
 Makron_AttackAnimation
 ==================
*/
static void Makron_AttackAnimation (edict_t *self){

	float	r;
	vec3_t	vec;
	float	range;

	r = random();

	VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
	range = VectorLength(vec);

	if (r <= 0.3f)
		self->monsterinfo.currentmove = &makron_BFGAttackMove;
	else if (r <= 0.6f)
		self->monsterinfo.currentmove = &makron_hyperblasterAttackMove;
	else
		self->monsterinfo.currentmove = &makron_railgunAttackMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			makron_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			makron_pain2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			makron_pain3Frames[] = {
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
	{SG_AIMove,		0.0f,	Makron_PopupSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	Makron_TauntSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			makron_pain1Move = {FRAME_pain401, FRAME_pain404, makron_pain1Frames, Makron_RunAnimation};
static mmove_t			makron_pain2Move = {FRAME_pain501, FRAME_pain504, makron_pain2Frames, Makron_RunAnimation};
static mmove_t			makron_pain3Move = {FRAME_pain601, FRAME_pain627, makron_pain3Frames, Makron_RunAnimation};


/*
 ==================
 Makron_PainAnimation
 ==================
*/
static void Makron_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	// Lessen the chance of him going into his pain frames
	if (damage <= 25){
		if (random() < 0.2f)
			return;
	}

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	if (damage <= 40){
		Makron_Pain1Sound(self);
		self->monsterinfo.currentmove = &makron_pain1Move;
	}
	else if (damage <= 110){
		Makron_Pain2Sound(self);
		self->monsterinfo.currentmove = &makron_pain1Move;
	}
	else {
		if (damage <= 150){
			if (random() <= 0.45f){
				Makron_Pain3Sound(self);
				self->monsterinfo.currentmove = &makron_pain3Move;
			}
		}
		else {
			if (random() <= 0.35f){
				Makron_Pain3Sound(self);
				self->monsterinfo.currentmove = &makron_pain3Move;
			}
		}
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			makron_death1Frames[] = {
	{SG_AIMove,	  -15.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,	  -12.0f,	NULL},
	{SG_AIMove,		0.0f,	Makron_StepLeftSound},
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
	{SG_AIMove,	   11.0f,	NULL},
	{SG_AIMove,	   12.0f,	NULL},
	{SG_AIMove,	   11.0f,	Makron_StepRightSound},
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
	{SG_AIMove,		5.0f,	NULL},
	{SG_AIMove,		7.0f,	NULL},
	{SG_AIMove,		6.0f,	Makron_StepLeftSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
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
	{SG_AIMove,	   -6.0f,	NULL},
	{SG_AIMove,	   -4.0f,	NULL},
	{SG_AIMove,	   -6.0f,	Makron_StepRightSound},
	{SG_AIMove,	   -4.0f,	NULL},
	{SG_AIMove,	   -4.0f,	Makron_StepLeftSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},			
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,	   -5.0f,	NULL},
	{SG_AIMove,	   -3.0f,	Makron_StepRightSound},
	{SG_AIMove,	   -8.0f,	NULL},
	{SG_AIMove,	   -3.0f,	Makron_StepLeftSound},
	{SG_AIMove,	   -7.0f,	NULL},
	{SG_AIMove,	   -4.0f,	NULL},
	{SG_AIMove,	   -4.0f,	Makron_StepRightSound},
	{SG_AIMove,	   -6.0f,	NULL},			
	{SG_AIMove,	   -7.0f,	NULL},
	{SG_AIMove,		0.0f,	Makron_StepLeftSound},
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
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   27.0f,	Makron_HitSound},			
	{SG_AIMove,	   26.0f,	NULL},
	{SG_AIMove,		0.0f,	Makron_BrainSplorchSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			makron_death2Frames[] = {
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

static mmove_t			makron_death1Move = {FRAME_death201, FRAME_death295, makron_death1Frames, Makron_Dead};
static mmove_t			makron_death2Move = {FRAME_death301, FRAME_death320, makron_death2Frames, NULL};


/*
 ==================
 Makron_Dead
 ==================
*/
static void Makron_Dead (edict_t *self){

	VectorSet(self->mins, -60.0f, -60.0f, 0.0f);
	VectorSet(self->maxs, 60.0f, 60.0f, 72.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Makron_TorsoThink
 ==================
*/
static void Makron_TorsoThink (edict_t *self){

	if (++self->s.frame < 365)
		self->nextthink = level.time + FRAMETIME;
	else {		
		self->s.frame = 346;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
 ==================
 Makron_Torso
 ==================
*/
static void Makron_Torso (edict_t *entity){

	entity->movetype = MOVETYPE_NONE;
	entity->solid = SOLID_NOT;
	VectorSet(entity->mins, -8.0f, -8.0f, 0.0f);
	VectorSet(entity->maxs, 8.0f, 8.0f, 8.0f);
	entity->s.frame = 346;
	entity->s.modelindex = gi.modelindex("models/monsters/boss3/rider/tris.md2");
	entity->think = Makron_TorsoThink;
	entity->nextthink = level.time + 2.0f * FRAMETIME;
	entity->s.sound = gi.soundindex("makron/spine.wav");

	gi.linkentity(entity);
}

/*
 ==================
 Makron_SpawnTorso
 ==================
*/
static void Makron_SpawnTorso (edict_t *self){

	edict_t *entity;

	entity = SG_AllocEntity();

	VectorCopy(self->s.origin, entity->s.origin);
	VectorCopy(self->s.angles, entity->s.angles);
	entity->s.origin[1] -= 84.0f;

	Makron_Torso(entity);
}

/*
 ==================
 Makron_DieAnimation
 ==================
*/
static void Makron_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	self->s.sound = 0;

	// Check for gib
	if (self->health <= self->gib_health){
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1.0f, ATTN_NORM, 0.0f);

		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);

		ThrowHead(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);

		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// Regular death
	Makron_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	Makron_SpawnTorso(self);

	self->monsterinfo.currentmove = &makron_death1Move;
}


/*
 ==============================================================================

 SIGHT ANIMATION

 ==============================================================================
*/

static mframe_t			makron_sightFrames[] = {
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

static mmove_t			makron_sightMove = {FRAME_active01, FRAME_active13, makron_sightFrames, Makron_RunAnimation};


/*
 ==================
 Makron_SightAnimation
 ==================
*/
static void Makron_SightAnimation (edict_t *self, edict_t *other){

	self->monsterinfo.currentmove = &makron_sightMove;
}


/*
 ==============================================================================

 MAKRON SPAWNING FROM JORG

 ==============================================================================
*/


/*
 ==================
 Makron_Spawn
 ==================
*/
static void Makron_Spawn (edict_t *self){

	edict_t	*player;
	vec3_t	vec;

	SG_MonsterMakron_Spawn(self);

	// Jump at the player
	player = level.sight_client;
	if (!player)
		return;

	VectorSubtract(player->s.origin, self->s.origin, vec);
	self->s.angles[YAW] = SG_VectorToYaw(vec);
	VectorNormalize(vec);
	VectorMA(vec3_origin, 400.0f, vec, self->velocity);
	self->velocity[2] = 200.0f;
	self->groundentity = NULL;
}

/*
 ==================
 Makron_JumpOut
 ==================
*/
void Makron_JumpOut (edict_t *self){

	edict_t	*entity;

	entity = SG_AllocEntity();

	entity->nextthink = level.time + 0.8f;
	entity->think = Makron_Spawn;
	entity->target = self->target;
	VectorCopy(self->s.origin, entity->s.origin);
}


/*
 ==============================================================================

 MONSTER_MAKRON

 ==============================================================================
*/


/*
 ==================
 SG_MonsterMakron_Spawn
 ==================
*/
void SG_MonsterMakron_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/boss3/rider/tris.md2");

	VectorSet(self->mins, -30.0f, -30.0f, 0.0f);
	VectorSet(self->maxs, 30.0f, 30.0f, 90.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	makron.pain1Sound = gi.soundindex("makron/pain3.wav");
	makron.pain2Sound = gi.soundindex("makron/pain2.wav");
	makron.pain3Sound = gi.soundindex("makron/pain1.wav");
	makron.dieSound = gi.soundindex("makron/death.wav");
	makron.stepLeftSound = gi.soundindex("makron/step1.wav");
	makron.stepRightSound = gi.soundindex("makron/step2.wav");
	makron.BFGAttackSound = gi.soundindex("makron/bfg_fire.wav");
	makron.brainSplorchSound = gi.soundindex("makron/brain1.wav");
	makron.preRailgunSound = gi.soundindex("makron/rail_up.wav");
	makron.popupSound = gi.soundindex("makron/popup.wav");
	makron.taunt1Sound = gi.soundindex("makron/voice4.wav");
	makron.taunt3Sound = gi.soundindex("makron/voice.wav");
	makron.hitSound = gi.soundindex("makron/bhit.wav");

	gi.modelindex("models/monsters/boss3/rider/tris.md2");

	self->mass = 500;

	self->health = 3000;
	self->gib_health = -2000;

	self->pain = Makron_PainAnimation;
	self->die = Makron_DieAnimation;

	self->monsterinfo.stand = Makron_StandAnimation;
	self->monsterinfo.walk = Makron_WalkAnimation;
	self->monsterinfo.run = Makron_RunAnimation;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = Makron_AttackAnimation;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = Makron_SightAnimation;
	self->monsterinfo.checkattack = SG_MonsterCheckAttack;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &makron_sightMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}