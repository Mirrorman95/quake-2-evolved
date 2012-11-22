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
// m_flipper.c - Flipper monster
//


#include "g_local.h"
#include "m_flipper.h"


typedef struct {
	int					sightSound;
	int					idleSound;
	int					pain1Sound;
	int					pain2Sound;
	int					dieSound;
	int					searchSound;
	int					chompAttackSound;
	int					meleeAttackSound;
} flipper_t;

static flipper_t		flipper;

static void Flipper_RunLoop (edict_t *self);
static void Flipper_Run (edict_t *self);
static void Flipper_Dead (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Flipper_SightSound
 ==================
*/
static void Flipper_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, flipper.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Flipper_Pain1Sound
 ==================
*/
static void Flipper_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, flipper.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Flipper_Pain2Sound
 ==================
*/
static void Flipper_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, flipper.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Flipper_DieSound
 ==================
*/
static void Flipper_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, flipper.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Flipper_ChompAttackSound
 ==================
*/
static void Flipper_ChompAttackSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, flipper.chompAttackSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			flipper_standFrames[] = {
	{SG_AIStand,	0.0f,	NULL}
};

static mmove_t			flipper_standMove = {FRAME_flphor01, FRAME_flphor01, flipper_standFrames, NULL};


/*
 ==================
 Flipper_StandAnimation
 ==================
*/
static void Flipper_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &flipper_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			flipper_walkFrames[] = {
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
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL}
};

static mmove_t			flipper_walkMove = {FRAME_flphor01, FRAME_flphor24, flipper_walkFrames, NULL};


/*
 ==================
 Flipper_WalkAnimation
 ==================
*/
static void Flipper_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &flipper_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			flipper_runFrames[] = {
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL}
};

static mframe_t			flipper_runStartFrames[] = {
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL}
};

static mframe_t			flipper_startRunFrames[] = {
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,		8.0f,	Flipper_Run}
};

static mmove_t			flipper_runMove = {FRAME_flpver06, FRAME_flpver29, flipper_runFrames, NULL};
static mmove_t			flipper_runStartMove = {FRAME_flpver01, FRAME_flpver06, flipper_runStartFrames, Flipper_RunLoop};
static mmove_t			flipper_startRunMove = {FRAME_flphor01, FRAME_flphor05, flipper_startRunFrames, NULL};


/*
 ==================
 Flipper_RunLoop
 ==================
*/
static void Flipper_RunLoop (edict_t *self){

	self->monsterinfo.currentmove = &flipper_runMove;
}

/*
 ==================
 Flipper_Run
 ==================
*/
static void Flipper_Run (edict_t *self){

	self->monsterinfo.currentmove = &flipper_runStartMove;
}

/*
 ==================
 Flipper_StartRunAnimation
 ==================
*/
static void Flipper_StartRunAnimation (edict_t *self){

	self->monsterinfo.currentmove = &flipper_startRunMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Flipper_Bite
 ==================
*/
static void Flipper_Bite (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, 0.0f, 0.0f);
	SG_MeleeHit(self, aim, 5.0f, 0.0f);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			flipper_attackFrames[] = {
	{SG_AICharge,	0.0f,	Flipper_ChompAttackSound},
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
	{SG_AICharge,	0.0f,	Flipper_Bite},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Flipper_Bite},
	{SG_AICharge,	0.0f,	NULL}
};

static mmove_t			flipper_attackMove = {FRAME_flpbit01, FRAME_flpbit20, flipper_attackFrames, Flipper_Run};


/*
 ==================
 Flipper_MeleeAnimation
 ==================
*/
static void Flipper_MeleeAnimation (edict_t *self){

	self->monsterinfo.currentmove = &flipper_attackMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			flipper_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			flipper_pain2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			flipper_pain1Move = {FRAME_flppn201, FRAME_flppn205, flipper_pain1Frames, Flipper_Run};
static mmove_t			flipper_pain2Move = {FRAME_flppn101, FRAME_flppn105, flipper_pain2Frames, Flipper_Run};


/*
 ==================
 Flipper_PainAnimation
 ==================
*/
static void Flipper_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;
	
	if (skill->value == 3)
		return;		// No pain anims in nightmare

	n = (rand() + 1) % 2;

	if (n == 0){
		Flipper_Pain1Sound(self);
		self->monsterinfo.currentmove = &flipper_pain1Move;
	}
	else {
		Flipper_Pain2Sound(self);
		self->monsterinfo.currentmove = &flipper_pain2Move;
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			flipper_deathFrames[] = {
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
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			flipper_deathMove = {FRAME_flpdth01, FRAME_flpdth56, flipper_deathFrames, Flipper_Dead};


/*
 ==================
 Flipper_Dead
 ==================
*/
static void Flipper_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Flipper_DieAnimation
 ==================
*/
static void Flipper_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	// Check for gib
	if (self->health <= self->gib_health){
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1.0f, ATTN_NORM, 0.0f);

		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);

		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		ThrowHead(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// Regular death
	Flipper_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &flipper_deathMove;
}


/*
 ==============================================================================

 MONSTER_FLIPPER

 ==============================================================================
*/


/*
 ==================
 SG_MonsterFlipper_Spawn
 ==================
*/
void SG_MonsterFlipper_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/flipper/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, 0.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	flipper.sightSound = gi.soundindex("flipper/flpsght1.wav");
	flipper.idleSound = gi.soundindex("flipper/flpidle1.wav");
	flipper.pain1Sound = gi.soundindex("flipper/flppain1.wav");	
	flipper.pain2Sound = gi.soundindex("flipper/flppain2.wav");	
	flipper.dieSound = gi.soundindex("flipper/flpdeth1.wav");	
	flipper.searchSound = gi.soundindex("flipper/flpsrch1.wav");
	flipper.chompAttackSound = gi.soundindex("flipper/flpatck1.wav");
	flipper.meleeAttackSound = gi.soundindex("flipper/flpatck2.wav");

	self->mass = 100;

	self->health = 50;
	self->gib_health = -30;

	self->pain = Flipper_PainAnimation;
	self->die = Flipper_DieAnimation;

	self->monsterinfo.stand = Flipper_StandAnimation;
	self->monsterinfo.walk = Flipper_WalkAnimation;
	self->monsterinfo.run = Flipper_StartRunAnimation;
	self->monsterinfo.melee = Flipper_MeleeAnimation;
	self->monsterinfo.sight = Flipper_SightSound;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &flipper_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_WaterMonster(self);
}