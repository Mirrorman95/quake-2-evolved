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
// m_flyer.c - Flyer monster
//


// NOTE:
// - This monster has a "start/stop" and a "roll left/right" animation that is left unused


#include "g_local.h"
#include "m_flyer.h"


typedef struct {
	int					sightSound;
	int					idleSound;
	int					pain1Sound;
	int					pain2Sound;
	int					dieSound;
	int					slashAttackSound;
	int					sporingAttackSound;

	// Used for start/stop frames
	int					nextMove;
} flyer_t;

static flyer_t			flyer;

static void Flyer_NextMove (edict_t *self);
static void Flyer_LoopMelee (edict_t *self);
static void Flyer_CheckMelee (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Flyer_SightSound
 ==================
*/
static void Flyer_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, flyer.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Flyer_IdleSound
 ==================
*/
static void Flyer_IdleSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, flyer.idleSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Flyer_Pain1Sound
 ==================
*/
static void Flyer_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, flyer.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Flyer_Pain2Sound
 ==================
*/
static void Flyer_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, flyer.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Flyer_DieSound
 ==================
*/
static void Flyer_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, flyer.dieSound, 1, ATTN_NORM, 0);
}

/*
 ==================
 Flyer_SlashSound
 ==================
*/
static void Flyer_SlashSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, flyer.slashAttackSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Flyer_PopBladesSound
 ==================
*/
static void Flyer_PopBladesSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, flyer.sporingAttackSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			flyer_standFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mmove_t			flyer_standMove = {FRAME_stand01, FRAME_stand45, flyer_standFrames, NULL};


/*
 ==================
 Flyer_StandAnimation
 ==================
*/
static void Flyer_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &flyer_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			flyer_walkFrames[] = {
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL}
};

static mmove_t			flyer_walkMove = {FRAME_stand01, FRAME_stand45, flyer_walkFrames, NULL};


/*
 ==================
 Flyer_WalkAnimation
 ==================
*/
static void Flyer_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &flyer_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			flyer_runFrames[] = {
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL}
};

static mmove_t			flyer_runMove = {FRAME_stand01, FRAME_stand45, flyer_runFrames, NULL};


/*
 ==================
 Flyer_RunAnimation
 ==================
*/
static void Flyer_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &flyer_standMove;
	else
		self->monsterinfo.currentmove = &flyer_runMove;
}


/*
 ==============================================================================

 START/STOP ANIMATION

 ==============================================================================
*/

static mframe_t			flyer_startFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	Flyer_NextMove}
};

static mframe_t			flyer_stopFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	Flyer_NextMove}
};

static mmove_t			flyer_startMove = {FRAME_start01, FRAME_start06, flyer_startFrames, NULL};
static mmove_t			flyer_stopMove = {FRAME_stop01, FRAME_stop07, flyer_stopFrames, NULL};


/*
 ==================
 Flyer_StartAnimation
 ==================
*/
static void Flyer_StartAnimation (edict_t *self){

	self->monsterinfo.currentmove = &flyer_startMove;
}

/*
 ==================
 Flyer_StopAnimation
 ==================
*/
static void Flyer_StopAnimation (edict_t *self){

	self->monsterinfo.currentmove = &flyer_stopMove;
}

/*
 ==================
 Flyer_SetStart
 ==================
*/
static void Flyer_SetStart (edict_t *self){

	flyer.nextMove = ACTION_run;
	self->monsterinfo.currentmove = &flyer_startMove;
}


/*
 ==============================================================================

 ROLL LEFT/RIGHT ANIMATION

 ==============================================================================
*/

static mframe_t			flyer_rollLeftFrames[] = {
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

static mframe_t			flyer_rollRightFrames[] = {
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

static mmove_t			flyer_rollLeftMove = {FRAME_rollf01, FRAME_rollf09, flyer_rollLeftFrames, NULL};
static mmove_t			flyer_rollRightMove = {FRAME_rollr01, FRAME_rollr09, flyer_rollRightFrames, NULL};


/*
 ==============================================================================

 DEFENSE ANIMATION

 ==============================================================================
*/

static mframe_t			flyer_defenseFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			flyer_defenseMove = {FRAME_defens01, FRAME_defens06, flyer_defenseFrames, NULL};


/*
 ==============================================================================

 BANK LEFT/RIGHT ANIMATION

 ==============================================================================
*/


static mframe_t			flyer_bankLeftFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			flyer_bankRightFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			flyer_bankLeftMove = {FRAME_bankl01, FRAME_bankl07, flyer_bankLeftFrames, NULL};
static mmove_t			flyer_bankRightMove = {FRAME_bankr01, FRAME_bankr07, flyer_bankRightFrames, NULL};


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Flyer_Blaster
 ==================
*/
static void Flyer_Blaster (edict_t *self, int flashNumber){

	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	end;
	vec3_t	dir;
	int		effect;

	if ((self->s.frame == FRAME_attak204) || (self->s.frame == FRAME_attak207) || (self->s.frame == FRAME_attak210))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);

	SG_MonsterFireBlaster(self, start, dir, 1, 1000, flashNumber, effect);
}

/*
 ==================
 Flyer_Fire_Left
 ==================
*/
static void Flyer_Fire_Left (edict_t *self){

	Flyer_Blaster(self, MZ2_FLYER_BLASTER_1);
}

/*
 ==================
 Flyer_Fire_Right
 ==================
*/
static void Flyer_Fire_Right (edict_t *self){

	Flyer_Blaster(self, MZ2_FLYER_BLASTER_2);
}

/*
 ==================
 Flyer_Slash_Left
 ==================
*/
static void Flyer_Slash_Left (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 0.0f);
	SG_MeleeHit(self, aim, 5, 0);

	Flyer_SlashSound(self);
}

/*
 ==================
 Flyer_Slash_Right
 ==================
*/
static void Flyer_Slash_Right (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, self->maxs[0], 0);
	SG_MeleeHit(self, aim, 5, 0);

	Flyer_SlashSound(self);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			flyer_blasterAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge, -10.0f,	Flyer_Fire_Left},
	{SG_AICharge, -10.0f,	Flyer_Fire_Right},
	{SG_AICharge, -10.0f,	Flyer_Fire_Left},
	{SG_AICharge, -10.0f,	Flyer_Fire_Right},
	{SG_AICharge, -10.0f,	Flyer_Fire_Left},
	{SG_AICharge, -10.0f,	Flyer_Fire_Right},
	{SG_AICharge, -10.0f,	Flyer_Fire_Left},
	{SG_AICharge, -10.0f,	Flyer_Fire_Right},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			flyer_startMeleeFrames[] = {
	{SG_AICharge,	0.0f,	Flyer_PopBladesSound},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			flyer_endMeleeFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			flyer_loopMeleeFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Flyer_Slash_Left},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Flyer_Slash_Right},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mmove_t			flyer_blasterAttackMove = {FRAME_attak201, FRAME_attak217, flyer_blasterAttackFrames, Flyer_RunAnimation};
static mmove_t			flyer_startMeleeMove = {FRAME_attak101, FRAME_attak106, flyer_startMeleeFrames, Flyer_LoopMelee};
static mmove_t			flyer_endMeleeMove = {FRAME_attak119, FRAME_attak121, flyer_endMeleeFrames, Flyer_RunAnimation};
static mmove_t			flyer_loopMeleeMove = {FRAME_attak107, FRAME_attak118, flyer_loopMeleeFrames, Flyer_CheckMelee};


/*
 ==================
 Flyer_NextMove
 ==================
*/
static void Flyer_NextMove (edict_t *self){

	if (flyer.nextMove == ACTION_attack1)
		self->monsterinfo.currentmove = &flyer_startMeleeMove;
	else if (flyer.nextMove == ACTION_attack2)
		self->monsterinfo.currentmove = &flyer_blasterAttackMove;
	else if (flyer.nextMove == ACTION_run)
		self->monsterinfo.currentmove = &flyer_runMove;
}

/*
 ==================
 Flyer_LoopMelee
 ==================
*/
static void Flyer_LoopMelee (edict_t *self){

/*	if (random() <= 0.5)	
		self->monsterinfo.currentmove = &flyer_move_attack1;
	else */
	self->monsterinfo.currentmove = &flyer_loopMeleeMove;
}

/*
 ==================
 Flyer_CheckMelee
 ==================
*/
static void Flyer_CheckMelee (edict_t *self){

	if (SG_EntityRange(self, self->enemy) == RANGE_MELEE){
		if (random() <= 0.8f)
			self->monsterinfo.currentmove = &flyer_loopMeleeMove;
		else
			self->monsterinfo.currentmove = &flyer_endMeleeMove;
	}
	else
		self->monsterinfo.currentmove = &flyer_endMeleeMove;
}

/*
 ==================
 Flyer_AttackAnimation
 ==================
*/
static void Flyer_AttackAnimation (edict_t *self){

/*	if (random() <= 0.5)	
		self->monsterinfo.currentmove = &flyer_move_attack1;
	else */
	self->monsterinfo.currentmove = &flyer_blasterAttackMove;
}

/*
 ==================
 Flyer_MeleeAnimation
 ==================
*/
static void Flyer_MeleeAnimation (edict_t *self){

//	flyer.nextmove = ACTION_attack1;
//	self->monsterinfo.currentmove = &flyer_move_stop;
	self->monsterinfo.currentmove = &flyer_startMeleeMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			flyer_pain1Frames[] = {
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

static mframe_t			flyer_pain2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			flyer_pain3Frames[] = {	
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			flyer_pain1Move = {FRAME_pain101, FRAME_pain109, flyer_pain1Frames, Flyer_RunAnimation};
static mmove_t			flyer_pain2Move = {FRAME_pain201, FRAME_pain204, flyer_pain2Frames, Flyer_RunAnimation};
static mmove_t			flyer_pain3Move = {FRAME_pain301, FRAME_pain304, flyer_pain3Frames, Flyer_RunAnimation};


/*
 ==================
 Flyer_PainAnimation
 ==================
*/
static void Flyer_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	n = rand() % 3;

	if (n == 0){
		Flyer_Pain1Sound(self);
		self->monsterinfo.currentmove = &flyer_pain1Move;
	}
	else if (n == 1){
		Flyer_Pain2Sound(self);
		self->monsterinfo.currentmove = &flyer_pain2Move;
	}
	else {
		Flyer_Pain1Sound(self);
		self->monsterinfo.currentmove = &flyer_pain3Move;
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/


/*
 ==================
 Flyer_DieAnimation
 ==================
*/
static void Flyer_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	Flyer_DieSound(self);
	SG_BecomeExplosion1(self);
}
	

/*
 ==============================================================================

 MONSTER_FLYER

 ==============================================================================
*/


/*
 ==================
 SG_MonsterFlyer_Spawn
 ==================
*/
void SG_MonsterFlyer_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	// HACK: Fixes a map bug in jail5.bsp
	if (!Q_stricmp(level.mapname, "jail5") && (self->s.origin[2] == -104.0f)){
		self->targetname = self->target;
		self->target = NULL;
	}

	self->s.modelindex = gi.modelindex("models/monsters/flyer/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	flyer.sightSound = gi.soundindex("flyer/flysght1.wav");
	flyer.idleSound = gi.soundindex("flyer/flysrch1.wav");
	flyer.pain1Sound = gi.soundindex("flyer/flypain1.wav");
	flyer.pain2Sound = gi.soundindex("flyer/flypain2.wav");
	flyer.dieSound = gi.soundindex("flyer/flydeth1.wav");
	flyer.slashAttackSound = gi.soundindex("flyer/flyatck2.wav");
	flyer.sporingAttackSound = gi.soundindex("flyer/flyatck1.wav");

	gi.soundindex("flyer/flyatck3.wav");

	self->s.sound = gi.soundindex("flyer/flyidle1.wav");

	self->mass = 50;

	self->health = 50;

	self->pain = Flyer_PainAnimation;
	self->die = Flyer_DieAnimation;

	self->monsterinfo.stand = Flyer_StandAnimation;
	self->monsterinfo.walk = Flyer_WalkAnimation;
	self->monsterinfo.run = Flyer_RunAnimation;
	self->monsterinfo.attack = Flyer_AttackAnimation;
	self->monsterinfo.melee = Flyer_MeleeAnimation;
	self->monsterinfo.sight = Flyer_SightSound;
	self->monsterinfo.idle = Flyer_IdleSound;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &flyer_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_AirMonster(self);
}