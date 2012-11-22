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
// m_floater.c - Floater monster
//

// NOTE:
// - This monster has a "activation" animation that is left unused

// FIXME:
// - Floater_Zap


#include "g_local.h"
#include "m_float.h"


typedef struct {
	int					sightSound;
	int					idleSound;
	int					pain1Sound;
	int					pain2Sound;
	int					dieSound;
	int					searchSound;
	int					chompAttackSound;
	int					meleeAttackSound;
} floater_t;

static floater_t		floater;

static void Floater_Dead (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Floater_SightSound
 ==================
*/
static void Floater_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, floater.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Floater_IdleSound
 ==================
*/
static void Floater_IdleSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, floater.idleSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Floater_Pain1Sound
 ==================
*/
static void Floater_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, floater.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Floater_DieSound
 ==================
*/
static void Floater_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, floater.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Floater_Pain2Sound
 ==================
*/
static void Floater_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, floater.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Floater_ChompAttackSound
 ==================
*/
static void Floater_ChompAttackSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, floater.chompAttackSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Floater_MeleeAttackSound
 ==================
*/
static void Floater_MeleeAttackSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, floater.meleeAttackSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			floater_stand1Frames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mframe_t			floater_stand2Frames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mmove_t			floater_stand1Move = {FRAME_stand101, FRAME_stand152, floater_stand1Frames, NULL};
static mmove_t			floater_stand2Move = {FRAME_stand201, FRAME_stand252, floater_stand2Frames, NULL};


/*
 ==================
 Floater_StandAnimation
 ==================
*/
static void Floater_StandAnimation (edict_t *self){

	if (random() <= 0.5f)		
		self->monsterinfo.currentmove = &floater_stand1Move;
	else
		self->monsterinfo.currentmove = &floater_stand2Move;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			floater_walkFrames[] = {
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
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL}
};

static mmove_t			floater_walkMove = {FRAME_stand101, FRAME_stand152, floater_walkFrames, NULL};


/*
 ==================
 Floater_WalkAnimation
 ==================
*/
static void Floater_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &floater_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			floater_runFrames[] = {
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL}
};

static mmove_t			floater_runMove = {FRAME_stand101, FRAME_stand152, floater_runFrames, NULL};


/*
 ==================
 Floater_RunAnimation
 ==================
*/
static void Floater_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &floater_stand1Move;
	else
		self->monsterinfo.currentmove = &floater_runMove;
}


/*
 ==============================================================================

 ACTIVATION ANIMATION

 ==============================================================================
*/

static mframe_t			floater_activateFrames[] = {
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

static mmove_t			floater_activateMove = {FRAME_actvat01, FRAME_actvat31, floater_activateFrames, NULL};


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Floater_Blaster
 ==================
*/
static void Floater_Blaster (edict_t *self){

	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	end;
	vec3_t	dir;
	int		effect;

	if ((self->s.frame == FRAME_attak104) || (self->s.frame == FRAME_attak107))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_FLOAT_BLASTER_1], forward, right, start);

	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);

	SG_MonsterFireBlaster(self, start, dir, 1, 1000, MZ2_FLOAT_BLASTER_1, effect);
}

/*
 ==================
 Floater_Melee
 ==================
*/
static void Floater_Melee (edict_t *self){

	static vec3_t	aim = {MELEE_DISTANCE, 0.0f, 0.0f};

	Floater_MeleeAttackSound(self);

	SG_MeleeHit(self, aim, 5 + rand() % 6, -50);
}

/*
 ==================
 Floater_Zap
 ==================
*/
static void Floater_Zap (edict_t *self){

	vec3_t	forward, right;
	vec3_t	origin;
	vec3_t	dir;
	vec3_t	offset;

	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);

	AngleVectors(self->s.angles, forward, right, NULL);

	// FIXME: Use a flash and replace these two lines with the commented one
	VectorSet(offset, 18.5f, -0.9f, 10.0f);
	SG_ProjectSource(self->s.origin, offset, forward, right, origin);
//	SG_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, origin);

	Floater_ChompAttackSound(self);

	// FIXME: Use the flash, Luke
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPLASH);
	gi.WriteByte(32);
	gi.WritePosition(origin);
	gi.WriteDir(dir);
	gi.WriteByte(1);	//sparks
	gi.multicast(origin, MULTICAST_PVS);

	SG_TargetDamage(self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, 5 + rand() % 6, -10, DAMAGE_ENERGY, MOD_UNKNOWN);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			floater_blasterAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Floater_Blaster},
	{SG_AICharge,	0.0f,	Floater_Blaster},
	{SG_AICharge,	0.0f,	Floater_Blaster},
	{SG_AICharge,	0.0f,	Floater_Blaster},
	{SG_AICharge,	0.0f,	Floater_Blaster},
	{SG_AICharge,	0.0f,	Floater_Blaster},
	{SG_AICharge,	0.0f,	Floater_Blaster},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			floater_meleeAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Floater_Melee},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			floater_zapAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Floater_Zap},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mmove_t			floater_blasterAttackMove = {FRAME_attak101, FRAME_attak114, floater_blasterAttackFrames, Floater_RunAnimation};
static mmove_t			floater_meleeAttackMove = {FRAME_attak201, FRAME_attak225, floater_meleeAttackFrames, Floater_RunAnimation};
static mmove_t			floater_zapAttackMove = {FRAME_attak301, FRAME_attak334, floater_zapAttackFrames, Floater_RunAnimation};


/*
 ==================
 Floater_BlasterAttackAnimation
 ==================
*/
static void Floater_BlasterAttackAnimation (edict_t *self){

	self->monsterinfo.currentmove = &floater_blasterAttackMove;
}

/*
 ==================
 Floater_MeleeAttackAnimation
 ==================
*/
static void Floater_MeleeAttackAnimation (edict_t *self){

	if (random() < 0.5f)		
		self->monsterinfo.currentmove = &floater_zapAttackMove;
	else
		self->monsterinfo.currentmove = &floater_meleeAttackMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			floater_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			floater_pain2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			floater_pain3Frames [] = {
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

static mmove_t			floater_pain1Move = {FRAME_pain101, FRAME_pain107, floater_pain1Frames, Floater_RunAnimation};
static mmove_t			floater_pain2Move = {FRAME_pain201, FRAME_pain208, floater_pain2Frames, Floater_RunAnimation};
static mmove_t			floater_pain3Move = {FRAME_pain301, FRAME_pain312, floater_pain3Frames, Floater_RunAnimation};


/*
 ==================
 Floater_PainAnimation
 ==================
*/
static void Floater_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	n = (rand() + 1) % 3;

	if (n == 0){
		Floater_Pain1Sound(self);
		self->monsterinfo.currentmove = &floater_pain1Move;
	}
	else {
		Floater_Pain2Sound(self);
		self->monsterinfo.currentmove = &floater_pain2Move;
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			floater_deathFrames[] = {
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

static mmove_t			floater_deathMove = {FRAME_death01, FRAME_death13, floater_deathFrames, Floater_Dead};


/*
 ==================
 Floater_Dead
 ==================
*/
static void Floater_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Floater_DieAnimation
 ==================
*/
static void Floater_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	Floater_DieSound(self);
	SG_BecomeExplosion1(self);
}


/*
 ==============================================================================

 MONSTER_FLOATER

 ==============================================================================
*/


/*
 ==================
 SG_MonsterFloater_Spawn
 ==================
*/
void SG_MonsterFloater_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/float/tris.md2");

	VectorSet(self->mins, -24.0f, -24.0f, -24.0f);
	VectorSet(self->maxs, 24.0f, 24.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;
	
	floater.idleSound = gi.soundindex("floater/fltidle1.wav");
	floater.sightSound = gi.soundindex("floater/fltsght1.wav");
	floater.pain1Sound = gi.soundindex("floater/fltpain1.wav");
	floater.pain2Sound = gi.soundindex("floater/fltpain2.wav");
	floater.dieSound = gi.soundindex("floater/fltdeth1.wav");
	floater.chompAttackSound = gi.soundindex("floater/fltatck2.wav");
	floater.meleeAttackSound = gi.soundindex("floater/fltatck3.wav");

	gi.soundindex("floater/fltatck1.wav");

	self->s.sound = gi.soundindex("floater/fltsrch1.wav");

	self->mass = 300;

	self->health = 200;
	self->gib_health = -80;

	self->pain = Floater_PainAnimation;
	self->die = Floater_DieAnimation;

	self->monsterinfo.stand = Floater_StandAnimation;
	self->monsterinfo.walk = Floater_WalkAnimation;
	self->monsterinfo.run = Floater_RunAnimation;
	self->monsterinfo.attack = Floater_BlasterAttackAnimation;
	self->monsterinfo.melee = Floater_MeleeAttackAnimation;
	self->monsterinfo.sight = Floater_SightSound;
	self->monsterinfo.idle = Floater_IdleSound;

	gi.linkentity(self);

	if (random() <= 0.5f)		
		self->monsterinfo.currentmove = &floater_stand1Move;
	else
		self->monsterinfo.currentmove = &floater_stand2Move;
	
	self->monsterinfo.scale = MODEL_SCALE;

	SG_AirMonster(self);
}