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
// m_berserk.c - Berserk monster
//


#include "g_local.h"
#include "m_berserk.h"


typedef struct {
	int					sightSound;
	int					idleSound;
	int					painSound;
	int					dieSound;
	int					searchSound;
	int					swingSound;
} berserk_t;

static berserk_t		berserk;

static void Berserk_StandAnimation (edict_t *self);
static void Berserk_FidgetAnimation (edict_t *self);
static void Berserk_Dead (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Berserk_SightSound
 ==================
*/
static void Berserk_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, berserk.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Berserk_IdleSound
 ==================
*/
static void Berserk_IdleSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, berserk.idleSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Berserk_PainSound
 ==================
*/
static void Berserk_PainSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, berserk.painSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Berserk_DieSound
 ==================
*/
static void Berserk_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, berserk.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Berserk_SearchSound
 ==================
*/
static void Berserk_SearchSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, berserk.searchSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Berserk_SwingSound
 ==================
*/
static void Berserk_SwingSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, berserk.swingSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Berserk_StrikeSound
 ==================
*/
static void Berserk_StrikeSound (edict_t *self){

	// FIXME: Play impact sound
}


/*
 ==============================================================================

 FIDGET ANIMATION

 ==============================================================================
*/

static mframe_t			berserk_standFidgetFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mmove_t			berserk_standFidgetMove = {FRAME_standb1, FRAME_standb20, berserk_standFidgetFrames, Berserk_StandAnimation};


/*
 ==================
 Berserk_FidgetAnimation
 ==================
*/
static void Berserk_FidgetAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	if (random() > 0.15f)
		return;

	self->monsterinfo.currentmove = &berserk_standFidgetMove;
	Berserk_IdleSound(self);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			berserk_standFrames[] = {
	{SG_AIStand,	0.0f,	Berserk_FidgetAnimation},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL}
};

static mmove_t			berserk_standMove = {FRAME_stand1, FRAME_stand5, berserk_standFrames, NULL};


/*
 ==================
 Berserk_StandAnimation
 ==================
*/
static void Berserk_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &berserk_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			berserk_walkFrames[] = {
	{SG_AIWalk,		9.1f,	NULL},
	{SG_AIWalk,		6.3f,	NULL},
	{SG_AIWalk,		4.9f,	NULL},
	{SG_AIWalk,		6.7f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		8.2f,	NULL},
	{SG_AIWalk,		7.2f,	NULL},
	{SG_AIWalk,		6.1f,	NULL},
	{SG_AIWalk,		4.9f,	NULL},
	{SG_AIWalk,		4.7f,	NULL},
	{SG_AIWalk,		4.7f,	NULL},
	{SG_AIWalk,		4.8f,	NULL}
};

static mmove_t			berserk_walkMove = {FRAME_walkc1, FRAME_walkc11, berserk_walkFrames, NULL};


/*
 ==================
 Berserk_WalkAnimation
 ==================
*/
static void Berserk_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &berserk_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			berserk_runFrames[] = {
	{SG_AIRun,	   21.0f,	NULL},
	{SG_AIRun,	   11.0f,	NULL},
	{SG_AIRun,	   21.0f,	NULL},
	{SG_AIRun,	   25.0f,	NULL},
	{SG_AIRun,	   18.0f,	NULL},
	{SG_AIRun,	   19.0f,	NULL}
};

static mmove_t			berserk_runMove = {FRAME_run1, FRAME_run6, berserk_runFrames, NULL};


/*
 ==================
 Berserk_RunAnimation
 ==================
*/
static void Berserk_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &berserk_standMove;
	else
		self->monsterinfo.currentmove = &berserk_runMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Berserk_SpikeAttack
 ==================
*/
static void Berserk_SpikeAttack (edict_t *self){

	static	vec3_t	aim = {MELEE_DISTANCE, 0.0f, -24.0f};

	// Faster attack, upwards and backwards
	SG_MeleeHit(self, aim, (15 + (rand() % 6)), 400);
}

/*
 ==================
 Berserk_ClubAttack
 ==================
*/
static void Berserk_ClubAttack (edict_t *self){

	vec3_t	aim;

	// Slower attack
	VectorSet(aim, MELEE_DISTANCE, self->mins[0], -4.0f);
	SG_MeleeHit(self, aim, (5 + (rand() % 6)), 400);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			berserk_spikeAttackFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Berserk_SwingSound},
	{SG_AICharge,	0.0f,	Berserk_SpikeAttack},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			berserk_clubAttackFrames[] = {	
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Berserk_SwingSound},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Berserk_ClubAttack},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			berserk_strikeAttackFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	Berserk_SwingSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	Berserk_StrikeSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		9.7f,	NULL},
	{SG_AIMove,	   13.6f,	NULL}
};

static mmove_t			berserk_spikeAttackMove = {FRAME_att_c1, FRAME_att_c8, berserk_spikeAttackFrames, Berserk_RunAnimation};
static mmove_t			berserk_clubAttackMove = {FRAME_att_c9, FRAME_att_c20, berserk_clubAttackFrames, Berserk_RunAnimation};
static mmove_t			berserk_strikeAttackMove = {FRAME_att_c21, FRAME_att_c34, berserk_strikeAttackFrames, Berserk_RunAnimation};


/*
 ==================
 Berserk_MeleeAnimation
 ==================
*/
static void Berserk_MeleeAnimation (edict_t *self){

	if ((rand() % 2) == 0)
		self->monsterinfo.currentmove = &berserk_spikeAttackMove;
	else
		self->monsterinfo.currentmove = &berserk_clubAttackMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			berserk_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			berserk_pain2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
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

static mmove_t			berserk_pain1Move = {FRAME_painc1, FRAME_painc4, berserk_pain1Frames, Berserk_RunAnimation};
static mmove_t			berserk_pain2Move = {FRAME_painb1, FRAME_painb20, berserk_pain2Frames, Berserk_RunAnimation};


/*
 ==================
 Berserk_PainAnimation
 ==================
*/
static void Berserk_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;
	Berserk_PainSound(self);

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	if ((damage < 20) || (random() < 0.5f))
		self->monsterinfo.currentmove = &berserk_pain1Move;
	else
		self->monsterinfo.currentmove = &berserk_pain2Move;
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			berserk_death1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
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

static mframe_t			berserk_death2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			berserk_death1Move = {FRAME_death1, FRAME_death13, berserk_death1Frames, Berserk_Dead};
static mmove_t			berserk_death2Move = {FRAME_deathc1, FRAME_deathc8, berserk_death2Frames, Berserk_Dead};


/*
 ==================
 Berserk_Dead
 ==================
*/
static void Berserk_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);

	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Berserk_DieAnimation
 ==================
*/
static void Berserk_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

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

	Berserk_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	if (damage >= 50)
		self->monsterinfo.currentmove = &berserk_death1Move;
	else
		self->monsterinfo.currentmove = &berserk_death2Move;
}


/*
 ==============================================================================

 MONSTER_BERSERK

 ==============================================================================
*/


/*
 ==================
 SG_MonsterBerserk_Spawn
 ==================
*/
void SG_MonsterBerserk_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/berserk/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	berserk.sightSound = gi.soundindex("berserk/sight.wav");
	berserk.idleSound  = gi.soundindex("berserk/beridle1.wav");
	berserk.painSound  = gi.soundindex("berserk/berpain2.wav");
	berserk.dieSound   = gi.soundindex("berserk/berdeth2.wav");
	berserk.searchSound = gi.soundindex("berserk/bersrch1.wav");
	berserk.swingSound = gi.soundindex("berserk/attack.wav");

	self->mass = 250;

	self->health = 240;
	self->gib_health = -60;

	self->pain = Berserk_PainAnimation;
	self->die = Berserk_DieAnimation;

	self->monsterinfo.stand = Berserk_StandAnimation;
	self->monsterinfo.walk = Berserk_WalkAnimation;
	self->monsterinfo.run = Berserk_RunAnimation;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = NULL;
	self->monsterinfo.melee = Berserk_MeleeAnimation;
	self->monsterinfo.sight = Berserk_SightSound;
	self->monsterinfo.search = Berserk_SearchSound;

	self->monsterinfo.currentmove = &berserk_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	gi.linkentity(self);

	SG_GroundMonster(self);
}
