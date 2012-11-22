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
// m_brain.c - Brain monster
//


#include "g_local.h"
#include "m_brain.h"


typedef struct {
	int					sightSound;
	int					idle1Sound;
	int					idle2Sound;
	int					idle3Sound;
	int					pain1Sound;
	int					pain2Sound;
	int					dieSound;
	int					searchSound;
	int					chestOpenSound;
	int					tentaclesExtendSound;
	int					tentaclesRetractSound;
	int					swingRightSound;
	int					swingLeftSound;
	int					hitSound;
} brain_t;

static brain_t			brain;

static void Brain_Chest_Closed (edict_t *self);
static void Brain_Dead (edict_t *self);
static void Brain_Duck_Down (edict_t *self);
static void Brain_Duck_Hold (edict_t *self);
static void Brain_Duck_Up (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Brain_SightSound
 ==================
*/
static void Brain_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, brain.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Brain_Idle3Sound
 ==================
*/
static void Brain_Idle3Sound (edict_t *self){

	gi.sound(self, CHAN_AUTO, brain.idle3Sound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Brain_Pain1Sound
 ==================
*/
static void Brain_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, brain.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Brain_Pain2Sound
 ==================
*/
static void Brain_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, brain.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Brain_DieSound
 ==================
*/
static void Brain_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, brain.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Brain_SearchSound
 ==================
*/
static void Brain_SearchSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, brain.searchSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Brain_ChestOpenSound
 ==================
*/
static void Brain_ChestOpenSound (edict_t *self){

	gi.sound(self, CHAN_BODY, brain.chestOpenSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Brain_TentaclesRetractSound
 ==================
*/
static void Brain_TentaclesRetractSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, brain.tentaclesRetractSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Brain_Swing_RightSound
 ==================
*/
static void Brain_Swing_RightSound (edict_t *self){

	gi.sound(self, CHAN_BODY, brain.swingRightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Brain_Swing_LeftSound
 ==================
*/
static void Brain_Swing_LeftSound (edict_t *self){

	gi.sound(self, CHAN_BODY, brain.swingLeftSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Brain_HitSound
 ==================
*/
static void Brain_HitSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, brain.hitSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			brain_standFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mmove_t			brain_standMove = {FRAME_stand01, FRAME_stand30, brain_standFrames, NULL};


/*
 ==================
 Brain_StandAnimation
 ==================
*/
static void Brain_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &brain_standMove;
}


/*
 ==============================================================================

 IDLE ANIMATION

 ==============================================================================
*/

static mframe_t			brain_idleFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mmove_t			brain_idleMove = {FRAME_stand31, FRAME_stand60, brain_idleFrames, Brain_StandAnimation};


/*
 ==================
 Brain_IdleAnimation
 ==================
*/
static void Brain_IdleAnimation (edict_t *self){

	Brain_Idle3Sound(self);
	self->monsterinfo.currentmove = &brain_idleMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			brain_walkFrames[] = {
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		1.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		9.0f,	NULL},
	{SG_AIWalk,	   -4.0f,	NULL},
	{SG_AIWalk,	   -1.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL}
};

static mmove_t			brain_walkMove = {FRAME_walk101, FRAME_walk111, brain_walkFrames, NULL};


/*
 ==================
 Brain_WalkAnimation
 ==================
*/
static void Brain_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &brain_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			brain_runFrames[] = {
	{SG_AIRun,		9.0f,	NULL},
	{SG_AIRun,		2.0f,	NULL},
	{SG_AIRun,		3.0f,	NULL},
	{SG_AIRun,		3.0f,	NULL},
	{SG_AIRun,		1.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   -4.0f,	NULL},
	{SG_AIRun,	   -1.0f,	NULL},
	{SG_AIRun,		2.0f,	NULL}
};

static mmove_t			brain_runMove = {FRAME_walk101, FRAME_walk111, brain_runFrames, NULL};


/*
 ==================
 Brain_RunAnimation
 ==================
*/
static void Brain_RunAnimation (edict_t *self){

	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &brain_standMove;
	else
		self->monsterinfo.currentmove = &brain_runMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Brain_Hit_Right
 ==================
*/
static void Brain_Hit_Right (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, self->maxs[0], 8.0f);

	if (SG_MeleeHit(self, aim, (15 + (rand() % 5)), 40))
		Brain_HitSound(self);
}

/*
 ==================
 Brain_Hit_Left
 ==================
*/
static void Brain_Hit_Left (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 8.0f);

	if (SG_MeleeHit(self, aim, (15 + (rand() % 5)), 40))
		Brain_HitSound(self);
}

/*
 ==================
 Brain_Chest_Open
 ==================
*/
static void Brain_Chest_Open (edict_t *self){

	self->spawnflags &= ~65536;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;

	Brain_ChestOpenSound(self);
}

/*
 ==================
 Brain_Tentacle_Attack
 ==================
*/
static void Brain_Tentacle_Attack (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, 0.0f, 8.0f);

	if (SG_MeleeHit(self, aim, (10 + (rand() % 5)), -600) && skill->value > 0)
		self->spawnflags |= 65536;

	Brain_TentaclesRetractSound(self);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			brain_swingAttackFrames[] = {
	{SG_AICharge,	8.0f,	NULL},
	{SG_AICharge,	3.0f,	NULL},
	{SG_AICharge,	5.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,  -3.0f,	Brain_Swing_RightSound},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,  -5.0f,	NULL},
	{SG_AICharge,  -7.0f,	Brain_Hit_Right},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	6.0f,	Brain_Swing_LeftSound},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	2.0f,	Brain_Hit_Left},
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,	6.0f,	NULL},
	{SG_AICharge,  -1.0f,	NULL},
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,	2.0f,	NULL},
	{SG_AICharge,  -11.0f,	NULL}
};

static mframe_t			brain_tentacleAttackFrames[] = {
	{SG_AICharge,	5.0f,	NULL},
	{SG_AICharge,  -4.0f,	NULL},
	{SG_AICharge,  -4.0f,	NULL},
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,	0.0f,	Brain_Chest_Open},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,  13.0f,	Brain_Tentacle_Attack},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	2.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,  -9.0f,	Brain_Chest_Closed},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	4.0f,	NULL},
	{SG_AICharge,	3.0f,	NULL},
	{SG_AICharge,	2.0f,	NULL},
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,  -6.0f,	NULL}
};

static mmove_t			brain_swingAttackMove = {FRAME_attak101, FRAME_attak118, brain_swingAttackFrames, Brain_RunAnimation};
static mmove_t			brain_tentacleAttackMove = {FRAME_attak201, FRAME_attak217, brain_tentacleAttackFrames, Brain_RunAnimation};


/*
 ==================
 Brain_Chest_Closed
 ==================
*/
static void Brain_Chest_Closed (edict_t *self){

	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;

	if (self->spawnflags & 65536){
		self->spawnflags &= ~65536;
		self->monsterinfo.currentmove = &brain_swingAttackMove;
	}
}

/*
 ==================
 Brain_MeleeAnimation
 ==================
*/
static void Brain_MeleeAnimation (edict_t *self){

	if (random() <= 0.5f)
		self->monsterinfo.currentmove = &brain_swingAttackMove;
	else
		self->monsterinfo.currentmove = &brain_tentacleAttackMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			brain_defenseFrames[] = {
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

static mframe_t			brain_pain3Frames[] = {
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -4.0f,	NULL}
};

static mframe_t			brain_pain2Frames[] = {
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL}
};

static mframe_t			brain_pain1Frames[] = {
	{SG_AIMove,	   -6.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,	   -6.0f,	NULL},
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
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		7.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL}
};

static mmove_t			brain_defenseMove = {FRAME_defens01, FRAME_defens08, brain_defenseFrames, NULL};
static mmove_t			brain_pain3Move = {FRAME_pain301, FRAME_pain306, brain_pain3Frames, Brain_RunAnimation};
static mmove_t			brain_pain2Move = {FRAME_pain201, FRAME_pain208, brain_pain2Frames, Brain_RunAnimation};
static mmove_t			brain_pain1Move = {FRAME_pain101, FRAME_pain121, brain_pain1Frames, Brain_RunAnimation};


/*
 ==================
 Brain_PainAnimation
 ==================
*/
static void Brain_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

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
		Brain_Pain1Sound(self);
		self->monsterinfo.currentmove = &brain_pain1Move;
	}
	else if (r < 0.66f){
		Brain_Pain2Sound(self);
		self->monsterinfo.currentmove = &brain_pain2Move;
	}
	else {
		Brain_Pain1Sound(self);
		self->monsterinfo.currentmove = &brain_pain3Move;
	}
}

/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			brain_death2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		9.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			brain_death1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,		9.0f,	NULL},
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

static mmove_t			brain_death2Move = {FRAME_death201, FRAME_death205, brain_death2Frames, Brain_Dead};
static mmove_t			brain_death1Move = {FRAME_death101, FRAME_death118, brain_death1Frames, Brain_Dead};


/*
 ==================
 Brain_Dead
 ==================
*/
static void Brain_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);

	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Brain_DieAnimation
 ==================
*/
static void Brain_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	self->s.effects = 0;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;

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
	Brain_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	if (random() <= 0.5f)
		self->monsterinfo.currentmove = &brain_death1Move;
	else
		self->monsterinfo.currentmove = &brain_death2Move;
}


/*
 ==============================================================================

 DUCK ANIMATION

 ==============================================================================
*/

static mframe_t			brain_duckFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -2.0f,	Brain_Duck_Down},
	{SG_AIMove,	   17.0f,	Brain_Duck_Hold},
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,	   -1.0f,	Brain_Duck_Up},
	{SG_AIMove,	   -5.0f,	NULL},
	{SG_AIMove,	   -6.0f,	NULL},
	{SG_AIMove,	   -6.0f,	NULL}
};

static mmove_t			brain_duckMove = {FRAME_duck01, FRAME_duck08, brain_duckFrames, Brain_RunAnimation};


/*
 ==================
 Brain_Duck_Down
 ==================
*/
static void Brain_Duck_Down (edict_t *self){

	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32.0f;
	self->takedamage = DAMAGE_YES;

	gi.linkentity(self);
}

/*
 ==================
 Brain_Duck_Hold
 ==================
*/
static void Brain_Duck_Hold (edict_t *self){

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

/*
 ==================
 Brain_Duck_Up
 ==================
*/
static void Brain_Duck_Up (edict_t *self){

	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32.0f;
	self->takedamage = DAMAGE_AIM;

	gi.linkentity(self);
}

/*
 ==================
 Brain_DodgeAnimation
 ==================
*/
static void Brain_DodgeAnimation (edict_t *self, edict_t *attacker, float eta){

	if (random() > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.pausetime = level.time + eta + 0.5f;
	self->monsterinfo.currentmove = &brain_duckMove;
}


/*
 ==============================================================================

 MONSTER_BRAIN

 ==============================================================================
*/


/*
 ==================
 SG_MonsterBrain_Spawn
 ==================
*/
void SG_MonsterBrain_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/brain/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	brain.sightSound = gi.soundindex("brain/brnsght1.wav");
	brain.idle1Sound = gi.soundindex("brain/brnidle1.wav");
	brain.idle2Sound = gi.soundindex("brain/brnidle2.wav");
	brain.idle3Sound = gi.soundindex("brain/brnlens1.wav");
	brain.pain1Sound = gi.soundindex("brain/brnpain1.wav");
	brain.pain2Sound = gi.soundindex("brain/brnpain2.wav");
	brain.dieSound = gi.soundindex("brain/brndeth1.wav");
	brain.searchSound = gi.soundindex("brain/brnsrch1.wav");
	brain.chestOpenSound = gi.soundindex("brain/brnatck1.wav");
	brain.tentaclesExtendSound = gi.soundindex("brain/brnatck2.wav");
	brain.tentaclesRetractSound = gi.soundindex("brain/brnatck3.wav");
	brain.swingRightSound = gi.soundindex("brain/melee1.wav");
	brain.swingLeftSound = gi.soundindex("brain/melee2.wav");
	brain.hitSound = gi.soundindex("brain/melee3.wav");

	self->mass = 400;

	self->health = 300;
	self->gib_health = -150;

	self->pain = Brain_PainAnimation;
	self->die = Brain_DieAnimation;

	self->monsterinfo.stand = Brain_StandAnimation;
	self->monsterinfo.walk = Brain_WalkAnimation;
	self->monsterinfo.run = Brain_RunAnimation;
	self->monsterinfo.dodge = Brain_DodgeAnimation;
	self->monsterinfo.melee = Brain_MeleeAnimation;
	self->monsterinfo.sight = Brain_SightSound;
	self->monsterinfo.search = Brain_SearchSound;
	self->monsterinfo.idle = Brain_IdleAnimation;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	self->monsterinfo.power_armor_power = 100;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &brain_standMove;	
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}