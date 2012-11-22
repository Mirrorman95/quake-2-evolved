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
// m_infantry.c - Infantry monster
//


#include "g_local.h"
#include "m_infantry.h"


typedef struct {
	int					sightSound;
	int					idleSound;
	int					pain1Sound;
	int					pain2Sound;
	int					die1Sound;
	int					die2Sound;
	int					searchSound;
	int					gunshotSound;
	int					weaponCockSound;
	int					punchSwingSound;
	int					punchHitSound;
} infantry_t;

static infantry_t		infantry;

void Infantry_StandAnimation (edict_t *self);
static void Infantry_MachineGun (edict_t *self);
static void Infantry_Cock_Gun (edict_t *self);
static void Infantry_Fire (edict_t *self);
static void Infantry_Smack (edict_t *self);
static void Infantry_Dead (edict_t *self);
static void Infantry_Duck_Down (edict_t *self);
static void Infantry_Duck_Hold (edict_t *self);
static void Infantry_Duck_Up (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Infantry_SightSound
 ==================
*/
static void Infantry_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_BODY, infantry.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Infantry_IdleSound
 ==================
*/
static void Infantry_IdleSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, infantry.idleSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Infantry_Die1Sound
 ==================
*/
static void Infantry_Die1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, infantry.die1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Infantry_Die2Sound
 ==================
*/
static void Infantry_Die2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, infantry.die2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Infantry_Pain1Sound
 ==================
*/
static void Infantry_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, infantry.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Infantry_Pain2Sound
 ==================
*/
static void Infantry_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, infantry.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Infantry_WeaponCockSound
 ==================
*/
static void Infantry_WeaponCockSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, infantry.weaponCockSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Infantry_PunchSwingSound
 ==================
*/
static void Infantry_PunchSwingSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, infantry.punchSwingSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Infantry_PunchHitSound
 ==================
*/
static void Infantry_PunchHitSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, infantry.punchHitSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 FIDGET ANIMATION

 ==============================================================================
*/

static mframe_t			infantry_fidgetFrames[] = {
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	3.0f,	NULL},
	{SG_AIStand,	6.0f,	NULL},
	{SG_AIStand,	3.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	2.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,   -1.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	3.0f,	NULL},
	{SG_AIStand,	2.0f,	NULL},
	{SG_AIStand,	3.0f,	NULL},
	{SG_AIStand,	3.0f,	NULL},
	{SG_AIStand,   -2.0f,	NULL}
};

static mmove_t			infantry_fidgetMove = {FRAME_stand01, FRAME_stand49, infantry_fidgetFrames, Infantry_StandAnimation};


/*
 ==================
 Infantry_FidgetAnimation
 ==================
*/
static void Infantry_FidgetAnimation (edict_t *self){

	self->monsterinfo.currentmove = &infantry_fidgetMove;
	Infantry_IdleSound(self);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			infantry_standFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mmove_t			infantry_standMove = {FRAME_stand50, FRAME_stand71, infantry_standFrames, NULL};


/*
 ==================
 Infantry_StandAnimation
 ==================
*/
void Infantry_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &infantry_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			infantry_walkFrames[] = {
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL}
};

static mmove_t			infantry_walkMove = {FRAME_walk03, FRAME_walk14, infantry_walkFrames, NULL};


/*
 ==================
 Infantry_WalkAnimation
 ==================
*/
static void Infantry_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &infantry_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			infantry_runFrames[] = {
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,	   20.0f,	NULL},
	{SG_AIRun,		5.0f,	NULL},
	{SG_AIRun,		7.0f,	NULL},
	{SG_AIRun,	   30.0f,	NULL},
	{SG_AIRun,	   35.0f,	NULL},
	{SG_AIRun,		2.0f,	NULL},
	{SG_AIRun,		6.0f,	NULL}
};

static mmove_t			infantry_runMove = {FRAME_run01, FRAME_run08, infantry_runFrames, NULL};


/*
 ==================
 Infantry_RunAnimation
 ==================
*/
static void Infantry_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &infantry_standMove;
	else
		self->monsterinfo.currentmove = &infantry_runMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/

static vec3_t			infantry_aimAngles[] = {
	{0.0f,			5.0f,	0.0f},
	{10.0f,		   15.0f,	0.0f},
	{20.0f,		   25.0f,	0.0f},
	{25.0f,		   35.0f,	0.0f},
	{30.0f,		   40.0f,	0.0f},
	{30.0f,		   45.0f,	0.0f},
	{25.0f,		   50.0f,	0.0f},
	{20.0f,		   40.0f,	0.0f},
	{15.0f,		   35.0f,	0.0f},
	{40.0f,		   35.0f,	0.0f},
	{70.0f,		   35.0f,	0.0f},
	{90.0f,		   35.0f,	0.0f}
};


/*
 ==================
 Infantry_MachineGun
 ==================
*/
static void Infantry_MachineGun (edict_t *self){

	vec3_t	start, target;
	vec3_t	forward, right;
	vec3_t	vec;
	int		flashNumber;

	if (self->s.frame == FRAME_attak111){
		flashNumber = MZ2_INFANTRY_MACHINEGUN_1;

		AngleVectors(self->s.angles, forward, right, NULL);
		SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

		if (self->enemy){
			VectorMA(self->enemy->s.origin, -0.2f, self->enemy->velocity, target);
			target[2] += self->enemy->viewheight;
			VectorSubtract(target, start, forward);
			VectorNormalize(forward);
		}
		else
			AngleVectors (self->s.angles, forward, right, NULL);
	}
	else {
		flashNumber = MZ2_INFANTRY_MACHINEGUN_2 + (self->s.frame - FRAME_death211);

		AngleVectors(self->s.angles, forward, right, NULL);
		SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

		VectorSubtract(self->s.angles, infantry_aimAngles[flashNumber - MZ2_INFANTRY_MACHINEGUN_2], vec);
		AngleVectors(vec, forward, NULL, NULL);
	}

	SG_MonsterFireBullet(self, start, forward, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flashNumber);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			infantry_attack1Frames[] = {
	{SG_AICharge,	4.0f,	NULL},
	{SG_AICharge,  -1.0f,	NULL},
	{SG_AICharge,  -1.0f,	NULL},
	{SG_AICharge,	0.0f,   Infantry_Cock_Gun},
	{SG_AICharge,  -1.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	2.0f,	NULL},
	{SG_AICharge,  -2.0f,	NULL},
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,	1.0f,	Infantry_Fire},
	{SG_AICharge,	5.0f,	NULL},
	{SG_AICharge,  -1.0f,	NULL},
	{SG_AICharge,  -2.0f,	NULL},
	{SG_AICharge,  -3.0f,	NULL}
};

static mframe_t			infantry_attack2Frames[] = {
	{SG_AICharge,	3.0f,	NULL},
	{SG_AICharge,	6.0f,	NULL},
	{SG_AICharge,	0.0f,	Infantry_PunchSwingSound},
	{SG_AICharge,	8.0f,	NULL},
	{SG_AICharge,	5.0f,	NULL},
	{SG_AICharge,	8.0f,	Infantry_Smack},
	{SG_AICharge,	6.0f,	NULL},
	{SG_AICharge,	3.0f,	NULL},
};

static mmove_t			infantry_attack1Move = {FRAME_attak101, FRAME_attak115, infantry_attack1Frames, Infantry_RunAnimation};
static mmove_t			infantry_attack2Move = {FRAME_attak201, FRAME_attak208, infantry_attack2Frames, Infantry_RunAnimation};


/*
 ==================
 Infantry_Cock_Gun
 ==================
*/
static void Infantry_Cock_Gun (edict_t *self){

	int		n;

	Infantry_WeaponCockSound(self);

	n = (rand() & 15) + 3 + 7;
	self->monsterinfo.pausetime = level.time + n * FRAMETIME;
}

/*
 ==================
 Infantry_Fire
 ==================
*/
static void Infantry_Fire (edict_t *self){

	Infantry_MachineGun(self);

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

/*
 ==================
 Infantry_Smack
 ==================
*/
static void Infantry_Smack (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, 0, 0);

	if (SG_MeleeHit(self, aim, (5 + (rand() % 5)), 50))
		Infantry_PunchHitSound(self);
}

/*
 ==================
 Infantry_AttackAnimation
 ==================
*/
static void Infantry_AttackAnimation (edict_t *self){

	if (SG_EntityRange(self, self->enemy) == RANGE_MELEE)
		self->monsterinfo.currentmove = &infantry_attack2Move;
	else
		self->monsterinfo.currentmove = &infantry_attack1Move;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			infantry_pain1Frames[] = {
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		6.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL}
};

static mframe_t			infantry_pain2Frames[] = {
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		5.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL}
};

static mmove_t			infantry_pain1Move = {FRAME_pain101, FRAME_pain110, infantry_pain1Frames, Infantry_RunAnimation};
static mmove_t			infantry_pain2Move = {FRAME_pain201, FRAME_pain210, infantry_pain2Frames, Infantry_RunAnimation};


/*
 ==================
 Infantry_PainAnimation
 ==================
*/
static void Infantry_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == 3.0f)
		return;		// No pain anims in nightmare

	n = rand() % 2;

	if (n == 0){
		self->monsterinfo.currentmove = &infantry_pain1Move;
		Infantry_Pain1Sound(self);
	}
	else {
		self->monsterinfo.currentmove = &infantry_pain2Move;
		Infantry_Pain2Sound(self);
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			infantry_death1Frames[] = {
	{SG_AIMove,	   -4.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -4.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		9.0f,	NULL},
	{SG_AIMove,		9.0f,	NULL},
	{SG_AIMove,		5.0f,	NULL},
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,	   -3.0f,	NULL}
};

static mframe_t			infantry_death2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		1.0f,   NULL},
	{SG_AIMove,		5.0f,   NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		1.0f,   NULL},
	{SG_AIMove,		1.0f,   NULL},
	{SG_AIMove,		4.0f,   NULL},
	{SG_AIMove,		3.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,	   -2.0f,	Infantry_MachineGun},
	{SG_AIMove,	   -2.0f,	Infantry_MachineGun},
	{SG_AIMove,	   -3.0f,	Infantry_MachineGun},
	{SG_AIMove,	   -1.0f,	Infantry_MachineGun},
	{SG_AIMove,	   -2.0f,	Infantry_MachineGun},
	{SG_AIMove,		0.0f,   Infantry_MachineGun},
	{SG_AIMove,		2.0f,   Infantry_MachineGun},
	{SG_AIMove,		2.0f,   Infantry_MachineGun},
	{SG_AIMove,		3.0f,   Infantry_MachineGun},
	{SG_AIMove,	  -10.0f,	Infantry_MachineGun},
	{SG_AIMove,	   -7.0f,	Infantry_MachineGun},
	{SG_AIMove,	   -8.0f,	Infantry_MachineGun},
	{SG_AIMove,	   -6.0f,	NULL},
	{SG_AIMove,		4.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL}
};

static mframe_t infantry_death3Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,	   -6.0f,	NULL},
	{SG_AIMove,	  -11.0f,	NULL},
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,	  -11.0f,	NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL}
};

static mmove_t			infantry_death1Move = {FRAME_death101, FRAME_death120, infantry_death1Frames, Infantry_Dead};
static mmove_t			infantry_death2Move = {FRAME_death201, FRAME_death225, infantry_death2Frames, Infantry_Dead};
static mmove_t			infantry_death3Move = {FRAME_death301, FRAME_death309, infantry_death3Frames, Infantry_Dead};


/*
 ==================
 Infantry_Dead
 ==================
*/
static void Infantry_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;

	gi.linkentity(self);

	SG_FlyCheck(self);
}

/*
 ==================
 Infantry_DieAnimation
 ==================
*/
void Infantry_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	int		n;

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
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	n = rand() % 3;

	if (n == 0){
		self->monsterinfo.currentmove = &infantry_death1Move;
		Infantry_Die2Sound(self);
	}
	else if (n == 1){
		self->monsterinfo.currentmove = &infantry_death2Move;
		Infantry_Die1Sound(self);
	}
	else {
		self->monsterinfo.currentmove = &infantry_death3Move;
		Infantry_Die2Sound(self);
	}
}


/*
 ==============================================================================

 DUCK ANIMATION

 ==============================================================================
*/

static mframe_t			infantry_duckFrames[] = {
	{SG_AIMove,	   -2.0f,	Infantry_Duck_Down},
	{SG_AIMove,	   -5.0f,	Infantry_Duck_Hold},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		4.0f,	Infantry_Duck_Up},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			infantry_duckMove = {FRAME_duck01, FRAME_duck05, infantry_duckFrames, Infantry_RunAnimation};


/*
 ==================
 Infantry_Duck_Down
 ==================
*/
static void Infantry_Duck_Down (edict_t *self){

	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32.0f;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + 1.0f;

	gi.linkentity(self);
}

/*
 ==================
 Infantry_Duck_Hold
 ==================
*/
static void Infantry_Duck_Hold (edict_t *self){

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

/*
 ==================
 Infantry_Duck_Up
 ==================
*/
static void Infantry_Duck_Up (edict_t *self){

	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32.0f;
	self->takedamage = DAMAGE_AIM;

	gi.linkentity(self);
}

/*
 ==================
 Infantry_DodgeAnimation
 ==================
*/
static void Infantry_DodgeAnimation (edict_t *self, edict_t *attacker, float eta){

	if (random() > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = &infantry_duckMove;
}


/*
 ==============================================================================

 MONSTER_INFANTRY

 ==============================================================================
*/


/*
 ==================
 SG_MonsterInfantry_Spawn
 ==================
*/
void SG_MonsterInfantry_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	infantry.sightSound = gi.soundindex("infantry/infsght1.wav");
	infantry.idleSound = gi.soundindex("infantry/infidle1.wav");
	infantry.pain1Sound = gi.soundindex("infantry/infpain1.wav");
	infantry.pain2Sound = gi.soundindex("infantry/infpain2.wav");
	infantry.die1Sound = gi.soundindex("infantry/infdeth1.wav");
	infantry.die2Sound = gi.soundindex("infantry/infdeth2.wav");
	infantry.searchSound = gi.soundindex("infantry/infsrch1.wav");
	infantry.gunshotSound = gi.soundindex("infantry/infatck1.wav");
	infantry.weaponCockSound = gi.soundindex("infantry/infatck3.wav");
	infantry.punchSwingSound = gi.soundindex("infantry/infatck2.wav");
	infantry.punchHitSound = gi.soundindex("infantry/melee2.wav");

	self->mass = 200;

	self->health = 100;
	self->gib_health = -40;

	self->pain = Infantry_PainAnimation;
	self->die = Infantry_DieAnimation;

	self->monsterinfo.stand = Infantry_StandAnimation;
	self->monsterinfo.walk = Infantry_WalkAnimation;
	self->monsterinfo.run = Infantry_RunAnimation;
	self->monsterinfo.dodge = Infantry_DodgeAnimation;
	self->monsterinfo.attack = Infantry_AttackAnimation;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = Infantry_SightSound;
	self->monsterinfo.idle = Infantry_FidgetAnimation;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &infantry_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}