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
// m_superTank.c - Super tank monster
//

// NOTE:
// - This monster has a "turn left/right", "backwards" and two "weapon attacks" animations that isn't used


#include "g_local.h"
#include "m_supertank.h"


typedef struct {
	int					pain1Sound;
	int					pain2Sound;
	int					pain3Sound;
	int					dieSound;
	int					search1Sound;
	int					search2Sound;
	int					treadSound;
} superTank_t;

static superTank_t		superTank;

static void SuperTank_ReAttack1 (edict_t *self);
static void SuperTank_Dead (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 SuperTank_Pain1Sound
 ==================
*/
static void SuperTank_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, superTank.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SuperTank_Pain2Sound
 ==================
*/
static void SuperTank_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, superTank.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SuperTank_Pain3Sound
 ==================
*/
static void SuperTank_Pain3Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, superTank.pain3Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SuperTank_DieSound
 ==================
*/
static void SuperTank_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, superTank.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SuperTank_SearchSound
 ==================
*/
static void SuperTank_SearchSound (edict_t *self){

	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, superTank.search1Sound, 1.0f, ATTN_NORM, 0.0f);
	else
		gi.sound(self, CHAN_VOICE, superTank.search2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SuperTank_TreadSound
 ==================
*/
static void SuperTank_TreadSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, superTank.treadSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			superTank_standFrames[]= {
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

static mmove_t			superTank_standMove = {FRAME_stand_1, FRAME_stand_60, superTank_standFrames, NULL};


/*
 ==================
 SuperTank_StandAnimation
 ==================
*/
static void SuperTank_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &superTank_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			superTank_forwardFrames[] = {
	{SG_AIWalk,		4.0f,	SuperTank_TreadSound},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL}
};

static mmove_t			superTank_forwardMove = {FRAME_forwrd_1, FRAME_forwrd_18, superTank_forwardFrames, NULL};


/*
 ==================
 SuperTank_Forward
 ==================
*/
static void SuperTank_Forward (edict_t *self){

	self->monsterinfo.currentmove = &superTank_forwardMove;
}

/*
 ==================
 SuperTank_WalkAnimation
 ==================
*/
static void SuperTank_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &superTank_forwardMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			superTank_runFrames[] = {
	{SG_AIRun,	   12.0f,	SuperTank_TreadSound},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL}
};

static mmove_t			superTank_runMove = {FRAME_forwrd_1, FRAME_forwrd_18, superTank_runFrames, NULL};


/*
 ==================
 SuperTank_RunAnimation
 ==================
*/
static void SuperTank_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &superTank_standMove;
	else
		self->monsterinfo.currentmove = &superTank_runMove;
}


/*
 ==============================================================================

 TURN LEFT/RIGHT ANIMATION

 ==============================================================================
*/

static mframe_t			superTank_turnRightFrames[] = {
	{SG_AIMove,		0.0f,	SuperTank_TreadSound},
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

static mframe_t			superTank_turnLeftFrames[] = {
	{SG_AIMove,		0.0f,	SuperTank_TreadSound},
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

static mmove_t			superTank_turnRightMove = {FRAME_right_1, FRAME_right_18, superTank_turnRightFrames, SuperTank_RunAnimation};
static mmove_t			superTank_turnLeftMove = {FRAME_left_1, FRAME_left_18, superTank_turnLeftFrames, SuperTank_RunAnimation};


/*
 ==============================================================================

 BACKWARDS ANIMATION

 ==============================================================================
*/

static mframe_t			superTank_backwardFrames[] = {
	{SG_AIWalk,		0.0f,	SuperTank_TreadSound},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL}
};

static mmove_t			superTank_backwardMove = {FRAME_backwd_1, FRAME_backwd_18, superTank_backwardFrames, NULL};


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 SuperTank_MachineGun
 ==================
*/
static void SuperTank_MachineGun (edict_t *self){

	vec3_t	dir;
	vec3_t	vec;
	vec3_t	start;
	vec3_t	forward, right;
	int		flashNumber;

	flashNumber = MZ2_SUPERTANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak1_1);

	//FIXME!!!
	dir[0] = 0.0f;
	dir[1] = self->s.angles[1];
	dir[2] = 0.0f;

	AngleVectors(dir, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

	if (self->enemy){
		VectorCopy(self->enemy->s.origin, vec);
		VectorMA(vec, 0, self->enemy->velocity, vec);
		vec[2] += self->enemy->viewheight;
		VectorSubtract(vec, start, forward);
		VectorNormalize(forward);
	}

	SG_MonsterFireBullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flashNumber);
}

/*
 ==================
 SuperTank_Rocket
 ==================
*/
static void SuperTank_Rocket (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	int		flashNumber;

	if (self->s.frame == FRAME_attak2_8)
		flashNumber = MZ2_SUPERTANK_ROCKET_1;
	else if (self->s.frame == FRAME_attak2_11)
		flashNumber = MZ2_SUPERTANK_ROCKET_2;
	else
		flashNumber = MZ2_SUPERTANK_ROCKET_3;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	SG_MonsterFireRocket(self, start, dir, 50, 500, flashNumber);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			superTank_machinegunAttackFrames[]= {
	{SG_AICharge,	0.0f,	SuperTank_MachineGun},
	{SG_AICharge,	0.0f,	SuperTank_MachineGun},
	{SG_AICharge,	0.0f,	SuperTank_MachineGun},
	{SG_AICharge,	0.0f,	SuperTank_MachineGun},
	{SG_AICharge,	0.0f,	SuperTank_MachineGun},
	{SG_AICharge,	0.0f,	SuperTank_MachineGun},
};

static mframe_t			superTank_endMachinegunAttackFrames[]= {
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

static mframe_t			superTank_rocketLauncherAttackFrames[]= {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	SuperTank_Rocket},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	SuperTank_Rocket},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	SuperTank_Rocket},
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

static mframe_t			superTank_attack3Frames[]= {
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

static mframe_t			superTank_attack4Frames[]= {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			superTank_machinegunAttackMove = {FRAME_attak1_1, FRAME_attak1_6, superTank_machinegunAttackFrames, SuperTank_ReAttack1};
static mmove_t			superTank_endMachinegunAttackMove = {FRAME_attak1_7, FRAME_attak1_20, superTank_endMachinegunAttackFrames, SuperTank_RunAnimation};
static mmove_t			superTank_rocketLauncherAttackMove = {FRAME_attak2_1, FRAME_attak2_27, superTank_rocketLauncherAttackFrames, SuperTank_RunAnimation};
static mmove_t			superTank_attack3Move = {FRAME_attak3_1, FRAME_attak3_27, superTank_attack3Frames, SuperTank_RunAnimation};
static mmove_t			superTank_attack4Move = {FRAME_attak4_1, FRAME_attak4_6, superTank_attack4Frames, SuperTank_RunAnimation};


/*
 ==================
 SuperTank_ReAttack1
 ==================
*/
static void SuperTank_ReAttack1 (edict_t *self){

	if (SG_IsEntityVisible(self, self->enemy)){
		if (random() < 0.9f)
			self->monsterinfo.currentmove = &superTank_machinegunAttackMove;
		else
			self->monsterinfo.currentmove = &superTank_endMachinegunAttackMove;
	}
	else
		self->monsterinfo.currentmove = &superTank_endMachinegunAttackMove;
}

/*
 ==================
 SuperTank_AttackAnimation
 ==================
*/
static void SuperTank_AttackAnimation (edict_t *self){

	vec3_t	vec;
	float	range;

	VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
	range = VectorLength(vec);

	if (range <= 160.0f)
		self->monsterinfo.currentmove = &superTank_machinegunAttackMove;
	else {
		// Fire rockets more often at distance
		if (random() < 0.3f)
			self->monsterinfo.currentmove = &superTank_machinegunAttackMove;
		else
			self->monsterinfo.currentmove = &superTank_rocketLauncherAttackMove;
	}
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			superTank_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			superTank_pain2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			superTank_pain3Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			superTank_pain1Move = {FRAME_pain1_1, FRAME_pain1_4, superTank_pain1Frames, SuperTank_RunAnimation};
static mmove_t			superTank_pain2Move = {FRAME_pain2_5, FRAME_pain2_8, superTank_pain2Frames, SuperTank_RunAnimation};
static mmove_t			superTank_pain3Move = {FRAME_pain3_9, FRAME_pain3_12, superTank_pain3Frames, SuperTank_RunAnimation};


/*
 ==================
 SuperTank_PainAnimation
 ==================
*/
static void SuperTank_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	// Lessen the chance of him going into his pain frames
	if (damage <= 25){
		if (random() < 0.2f)
			return;
	}

	// Don't go into pain if he's firing his rockets
	if (skill->value >= 2){
		if ((self->s.frame >= FRAME_attak2_1) && (self->s.frame <= FRAME_attak2_14))
			return;
	}

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	if (damage <= 10){
		SuperTank_Pain1Sound(self);
		self->monsterinfo.currentmove = &superTank_pain1Move;
	}
	else if (damage <= 25){
		SuperTank_Pain3Sound(self);
		self->monsterinfo.currentmove = &superTank_pain2Move;
	}
	else {
		SuperTank_Pain2Sound(self);
		self->monsterinfo.currentmove = &superTank_pain3Move;
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			superTank_deathFrames[] = {
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

static mmove_t			superTank_deathMove = {FRAME_death_1, FRAME_death_24, superTank_deathFrames, SuperTank_Dead};


/*
 ==================
 SuperTank_Dead
 ==================
*/
static void SuperTank_Dead (edict_t *self){

	VectorSet(self->mins, -60.0f, -60.0f, 0.0f);
	VectorSet(self->maxs, 60.0f, 60.0f, 72.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 SuperTank_DieAnimation
 ==================
*/
static void SuperTank_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	SuperTank_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->count = 0;

	self->monsterinfo.currentmove = &superTank_deathMove;
}


/*
 ==============================================================================

 MONSTER_SUPERTANK

 ==============================================================================
*/


/*
 ==================
 SG_MonsterSuperTank_Spawn
 ==================
*/
void SG_MonsterSuperTank_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/boss1/tris.md2");

	VectorSet(self->mins, -64.0f, -64.0f, 0.0f);
	VectorSet(self->maxs, 64.0f, 64.0f, 112.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	superTank.pain1Sound = gi.soundindex("bosstank/btkpain1.wav");
	superTank.pain2Sound = gi.soundindex("bosstank/btkpain2.wav");
	superTank.pain3Sound = gi.soundindex("bosstank/btkpain3.wav");
	superTank.dieSound = gi.soundindex("bosstank/btkdeth1.wav");
	superTank.search1Sound = gi.soundindex("bosstank/btkunqv1.wav");
	superTank.search2Sound = gi.soundindex("bosstank/btkunqv2.wav");
	superTank.treadSound = gi.soundindex ("bosstank/btkengn1.wav");

	self->mass = 800;

	self->health = 1500;
	self->gib_health = -500;

	self->pain = SuperTank_PainAnimation;
	self->die = SuperTank_DieAnimation;

	self->monsterinfo.stand = SuperTank_StandAnimation;
	self->monsterinfo.walk = SuperTank_WalkAnimation;
	self->monsterinfo.run = SuperTank_RunAnimation;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = SuperTank_AttackAnimation;
	self->monsterinfo.search = SuperTank_SearchSound;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = NULL;

	gi.linkentity(self);
	
	self->monsterinfo.currentmove = &superTank_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}