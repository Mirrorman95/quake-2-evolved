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
// m_gunner.c - Gunner monster
//


#include "g_local.h"
#include "m_gunner.h"


typedef struct {
	int					sightSound;
	int					idleSound;
	int					pain1Sound;
	int					pain2Sound;
	int					dieSound;
	int					searchSound;
	int					openGunSound;
} gunner_t;

static gunner_t			gunner;

static void Gunner_StandAnimation (edict_t *self);
static void Gunner_FidgetAnimation (edict_t *self);
static void Gunner_FireChaingunAnimation (edict_t *self);
static void Gunner_ReFireChaingunAnimation (edict_t *self);
static void Gunner_Dead (edict_t *self);
static void Gunner_Duck_Down (edict_t *self);
static void Gunner_Duck_Hold (edict_t *self);
static void Gunner_Duck_Up (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Gunner_SightSound
 ==================
*/
static void Gunner_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, gunner.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gunner_IdleSound
 ==================
*/
static void Gunner_IdleSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, gunner.idleSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Gunner_Pain1Sound
 ==================
*/
static void Gunner_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, gunner.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gunner_Pain2Sound
 ==================
*/
static void Gunner_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, gunner.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gunner_DieSound
 ==================
*/
static void Gunner_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, gunner.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gunner_SearchSound
 ==================
*/
static void Gunner_SearchSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, gunner.searchSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Gunner_OpenGun
 ==================
*/
static void Gunner_OpenGun (edict_t *self){

	gi.sound(self, CHAN_VOICE, gunner.openGunSound, 1.0f, ATTN_IDLE, 0.0f);
}


/*
 ==============================================================================

 FIDGET ANIMATION

 ==============================================================================
*/

static mframe_t			gunner_fidgetFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Gunner_IdleSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mmove_t			gunner_fidgetMove = {FRAME_stand31, FRAME_stand70, gunner_fidgetFrames, Gunner_StandAnimation};


/*
 ==================
 Gunner_FidgetAnimation
 ==================
*/
static void Gunner_FidgetAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;

	if (random() <= 0.05f)
		self->monsterinfo.currentmove = &gunner_fidgetMove;
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			gunner_standFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Gunner_FidgetAnimation},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Gunner_FidgetAnimation},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Gunner_FidgetAnimation}
};

static mmove_t			gunner_standMove = {FRAME_stand01, FRAME_stand30, gunner_standFrames, NULL};


/*
 ==================
 Gunner_StandAnimation
 ==================
*/
static void Gunner_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &gunner_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			gunner_walkFrames[] = {
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL}
};

static mmove_t			gunner_walkMove = {FRAME_walk07, FRAME_walk19, gunner_walkFrames, NULL};


/*
 ==================
 Gunner_WalkAnimation
 ==================
*/
static void Gunner_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &gunner_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			gunner_frames_run[] = {
	{SG_AIRun,	   26.0f,	NULL},
	{SG_AIRun,		9.0f,	NULL},
	{SG_AIRun,		9.0f,	NULL},
	{SG_AIRun,		9.0f,	NULL},
	{SG_AIRun,	   15.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,		6.0f,	NULL}
};

static mframe_t			gunner_frames_runandshoot[] = {
	{SG_AIRun,	   32.0f,	NULL},
	{SG_AIRun,	   15.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   18.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,	   20.0f,	NULL}
};

static mmove_t			gunner_runMove = {FRAME_run01, FRAME_run08, gunner_frames_run, NULL};
static mmove_t			gunner_runAndShootMove = {FRAME_runs01, FRAME_runs06, gunner_frames_runandshoot, NULL};


/*
 ==================
 Gunner_RunAnimation
 ==================
*/
static void Gunner_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &gunner_standMove;
	else
		self->monsterinfo.currentmove = &gunner_runMove;
}

/*
 ==================
 Gunner_RunAndShootAnimation
 ==================
*/
static void Gunner_RunAndShootAnimation (edict_t *self){

	self->monsterinfo.currentmove = &gunner_runAndShootMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Gunner_Chaingun
 ==================
*/
static void Gunner_Chaingun (edict_t *self){

	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	target;
	vec3_t	aim;
	int		flashNumber;

	flashNumber = MZ2_GUNNER_MACHINEGUN_1 + (self->s.frame - FRAME_attak216);

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

	// Project enemy back a bit and target there
	VectorCopy(self->enemy->s.origin, target);
	VectorMA(target, -0.2f, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;

	VectorSubtract(target, start, aim);
	VectorNormalize(aim);
	SG_MonsterFireBullet(self, start, aim, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flashNumber);
}

/*
 ==================
 Gunner_GrenadeLauncher
 ==================
*/
static void Gunner_GrenadeLauncher (edict_t *self){

	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	aim;
	int		flashNumber;

	if (self->s.frame == FRAME_attak105)
		flashNumber = MZ2_GUNNER_GRENADE_1;
	else if (self->s.frame == FRAME_attak108)
		flashNumber = MZ2_GUNNER_GRENADE_2;
	else if (self->s.frame == FRAME_attak111)
		flashNumber = MZ2_GUNNER_GRENADE_3;
	else
		flashNumber = MZ2_GUNNER_GRENADE_4;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

	// FIXME: Do a spread -225 -75 75 225 degrees around forward
	VectorCopy(forward, aim);

	SG_MonsterFireGrenade(self, start, aim, 50, 600, flashNumber);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			gunner_chaingunAttackFrames[] = {
	{SG_AICharge,	0.0f,	Gunner_OpenGun},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			gunner_fireChaingunFrames[] = {
	{SG_AICharge,	0.0f,   Gunner_Chaingun},
	{SG_AICharge,	0.0f,   Gunner_Chaingun},
	{SG_AICharge,	0.0f,   Gunner_Chaingun},
	{SG_AICharge,	0.0f,   Gunner_Chaingun},
	{SG_AICharge,	0.0f,   Gunner_Chaingun},
	{SG_AICharge,	0.0f,   Gunner_Chaingun},
	{SG_AICharge,	0.0f,   Gunner_Chaingun},
	{SG_AICharge,	0.0f,	Gunner_Chaingun}
};

static mframe_t			gunner_endFireChaingunFrames[] = {
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			gunner_grenadeLauncherAttackFrames[] = {
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   Gunner_GrenadeLauncher},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   Gunner_GrenadeLauncher},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   Gunner_GrenadeLauncher},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   Gunner_GrenadeLauncher},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mmove_t			gunner_chaingunAttackMove = {FRAME_attak209, FRAME_attak215, gunner_chaingunAttackFrames, Gunner_FireChaingunAnimation};
static mmove_t			gunner_fireChaingunMove = {FRAME_attak216, FRAME_attak223, gunner_fireChaingunFrames, Gunner_ReFireChaingunAnimation};
static mmove_t			gunner_endFireChaingunMove = {FRAME_attak224, FRAME_attak230, gunner_endFireChaingunFrames, Gunner_RunAnimation};
static mmove_t			gunner_grenadeLauncherAttackMove = {FRAME_attak101, FRAME_attak121, gunner_grenadeLauncherAttackFrames, Gunner_RunAnimation};


/*
 ==================
 Gunner_AttackAnimation
 ==================
*/
static void Gunner_AttackAnimation (edict_t *self){

	if (SG_EntityRange(self, self->enemy) == RANGE_MELEE)
		self->monsterinfo.currentmove = &gunner_chaingunAttackMove;
	else {
		if (random() <= 0.5f)
			self->monsterinfo.currentmove = &gunner_grenadeLauncherAttackMove;
		else
			self->monsterinfo.currentmove = &gunner_chaingunAttackMove;
	}
}

/*
 ==================
 Gunner_FireChaingunAnimation
 ==================
*/
static void Gunner_FireChaingunAnimation (edict_t *self){

	self->monsterinfo.currentmove = &gunner_fireChaingunMove;
}

/*
 ==================
 Gunner_ReFireChaingunAnimation
 ==================
*/
static void Gunner_ReFireChaingunAnimation (edict_t *self){

	if (self->enemy->health > 0){
		if (SG_IsEntityVisible(self, self->enemy)){
			if (random() <= 0.5f){
				self->monsterinfo.currentmove = &gunner_fireChaingunMove;
				return;
			}
		}
	}

	self->monsterinfo.currentmove = &gunner_endFireChaingunMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			gunner_pain3Frames[] = {
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL}
};

static mframe_t			gunner_pain2Frames[] = {
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,	   11.0f,	NULL},
	{SG_AIMove,		6.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -7.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,	   -7.0f,	NULL}
};

static mframe_t			gunner_pain1Frames[] = {
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -5.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			gunner_pain3Move = {FRAME_pain301, FRAME_pain305, gunner_pain3Frames, Gunner_RunAnimation};
static mmove_t			gunner_pain2Move = {FRAME_pain201, FRAME_pain208, gunner_pain2Frames, Gunner_RunAnimation};
static mmove_t			gunner_pain1Move = {FRAME_pain101, FRAME_pain118, gunner_pain1Frames, Gunner_RunAnimation};


/*
 ==================
 Gunner_PainAnimation
 ==================
*/
static void Gunner_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	if (rand() & 1)
		Gunner_Pain1Sound(self);
	else
		Gunner_Pain2Sound(self);

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	if (damage <= 10)
		self->monsterinfo.currentmove = &gunner_pain3Move;
	else if (damage <= 25)
		self->monsterinfo.currentmove = &gunner_pain2Move;
	else
		self->monsterinfo.currentmove = &gunner_pain1Move;
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			gunner_deathFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -7.0f,	NULL},
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,	   -5.0f,	NULL},
	{SG_AIMove,		8.0f,	NULL},
	{SG_AIMove,		6.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			gunner_deathMove = {FRAME_death01, FRAME_death11, gunner_deathFrames, Gunner_Dead};


/*
 ==================
 Gunner_Dead
 ==================
*/
static void Gunner_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;

	gi.linkentity(self);
}

/*
 ==================
 Gunner_DieAnimation
 ==================
*/
static void Gunner_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

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
	Gunner_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &gunner_deathMove;
}


/*
 ==============================================================================

 DUCK ANIMATION

 ==============================================================================
*/

static mframe_t			gunner_duckFrames[] = {
	{SG_AIMove,		1.0f,	Gunner_Duck_Down},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		1.0f,	Gunner_Duck_Hold},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,		0.0f,	Gunner_Duck_Up},
	{SG_AIMove,	   -1.0f,	NULL}
};

static mmove_t			gunner_duckMove = {FRAME_duck01, FRAME_duck08, gunner_duckFrames, Gunner_RunAnimation};


/*
 ==================
 Gunner_Duck_Down
 ==================
*/
static void Gunner_Duck_Down (edict_t *self){

	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;

	if (skill->value >= 2){
		if (random() > 0.5f)
			Gunner_GrenadeLauncher(self);
	}

	self->maxs[2] -= 32.0f;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + 1.0f;

	gi.linkentity(self);
}

/*
 ==================
 Gunner_Duck_Hold
 ==================
*/
static void Gunner_Duck_Hold (edict_t *self){

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

/*
 ==================
 Gunner_Duck_Up
 ==================
*/
static void Gunner_Duck_Up (edict_t *self){

	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32.0f;
	self->takedamage = DAMAGE_AIM;

	gi.linkentity(self);
}

/*
 ==================
 Gunner_DodgeAnimation
 ==================
*/
static void Gunner_DodgeAnimation (edict_t *self, edict_t *attacker, float eta){

	if (random() > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = &gunner_duckMove;
}


/*
 ==============================================================================

 MONSTER_GUNNER

 ==============================================================================
*/


/*
 ==================
 SG_MonsterGunner_Spawn
 ==================
*/
void SG_MonsterGunner_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/gunner/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	gunner.sightSound = gi.soundindex("gunner/sight1.wav");
	gunner.idleSound = gi.soundindex("gunner/gunidle1.wav");
	gunner.pain1Sound = gi.soundindex("gunner/gunpain2.wav");
	gunner.pain2Sound = gi.soundindex("gunner/gunpain1.wav");
	gunner.dieSound = gi.soundindex("gunner/death1.wav");
	gunner.searchSound = gi.soundindex("gunner/gunsrch1.wav");
	gunner.openGunSound = gi.soundindex("gunner/gunatck1.wav");

	gi.soundindex("gunner/gunatck2.wav");
	gi.soundindex("gunner/gunatck3.wav");

	self->mass = 200;

	self->health = 175;
	self->gib_health = -70;

	self->pain = Gunner_PainAnimation;
	self->die = Gunner_DieAnimation;

	self->monsterinfo.stand = Gunner_StandAnimation;
	self->monsterinfo.walk = Gunner_WalkAnimation;
	self->monsterinfo.run = Gunner_RunAnimation;
	self->monsterinfo.dodge = Gunner_DodgeAnimation;
	self->monsterinfo.attack = Gunner_AttackAnimation;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = Gunner_SightSound;
	self->monsterinfo.search = Gunner_SearchSound;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &gunner_standMove;	
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}