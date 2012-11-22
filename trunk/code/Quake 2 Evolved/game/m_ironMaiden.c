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
// m_ironMaiden.c - Iron maiden monster
//

// NOTE:
// - This monster has few sounds that is left unused



#include "g_local.h"
#include "m_chick.h"


typedef struct {
	int					sightSound;
	int					idle1Sound;
	int					idle2Sound;
	int					pain1Sound;
	int					pain2Sound;
	int					pain3Sound;
	int					die1Sound;
	int					die2Sound;
	int					searchSound;
	int					fallDownSound;
	int					misslePreLaunchSound;
	int					missleLaunchSound;
	int					missileReloadSound;
	int					meleeSwingSound;
	int					meleeHitSound;
} ironMaiden_t;

static ironMaiden_t		ironMaiden;

static void IronMaiden_StandAnimation (edict_t *self);
static void IronMaiden_RunAnimation (edict_t *self);
static void IronMaiden_ReRocket (edict_t *self);
static void IronMaiden_RocketAttack (edict_t *self);
static void IronMaiden_ReSlash (edict_t *self);
static void IronMaiden_SlashAttack (edict_t *self);
static void IronMaiden_Dead (edict_t *self);
static void IronMaiden_Duck_Down (edict_t *self);
static void IronMaiden_Duck_Hold (edict_t *self);
static void IronMaiden_Duck_Up (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 IronMaiden_SightSound
 ==================
*/
static void IronMaiden_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, ironMaiden.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 IronMaiden_IdleSound
 ==================
*/
static void IronMaiden_IdleSound (edict_t *self){

	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, ironMaiden.idle1Sound, 1.0f, ATTN_IDLE, 0.0f);
	else
		gi.sound(self, CHAN_VOICE, ironMaiden.idle2Sound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 IronMaiden_Pain1Sound
 ==================
*/
static void IronMaiden_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, ironMaiden.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 IronMaiden_Pain2Sound
 ==================
*/
static void IronMaiden_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, ironMaiden.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 IronMaiden_Pain3Sound
 ==================
*/
static void IronMaiden_Pain3Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, ironMaiden.pain3Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 IronMaiden_Die1Sound
 ==================
*/
static void IronMaiden_Die1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, ironMaiden.die1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 IronMaiden_Die2Sound
 ==================
*/
static void IronMaiden_Die2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, ironMaiden.die2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 IronMaiden_MisslePreLaunchSound
 ==================
*/
static void IronMaiden_MisslePreLaunchSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, ironMaiden.misslePreLaunchSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 IronMaiden_MissileReloadSound
 ==================
*/
static void IronMaiden_MissileReloadSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, ironMaiden.missileReloadSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 IronMaiden_MeleeSwingSound
 ==================
*/
static void IronMaiden_MeleeSwingSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, ironMaiden.meleeSwingSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 FIDGET ANIMATION

 ==============================================================================
*/

static mframe_t			ironMaiden_fidgetFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	IronMaiden_IdleSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mmove_t			ironMaiden_fidgetMove = {FRAME_stand201, FRAME_stand230, ironMaiden_fidgetFrames, IronMaiden_StandAnimation};


/*
 ==================
 IronMaiden_FidgetAnimation
 ==================
*/
static void IronMaiden_FidgetAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;

	if (random() <= 0.3f)
		self->monsterinfo.currentmove = &ironMaiden_fidgetMove;
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			ironMaiden_standFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	IronMaiden_FidgetAnimation},

};

static mmove_t			ironMaiden_standMove = {FRAME_stand101, FRAME_stand130, ironMaiden_standFrames, NULL};


/*
 ==================
 IronMaiden_StandAnimation
 ==================
*/
static void IronMaiden_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &ironMaiden_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			ironMaiden_walkFrames[] = {
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,    13.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,    11.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		9.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL}
};

static mmove_t			ironMaiden_walkMove = {FRAME_walk11, FRAME_walk20, ironMaiden_walkFrames, NULL};


/*
 ==================
 IronMaiden_WalkAnimation
 ==================
*/
static void IronMaiden_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &ironMaiden_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			ironMaiden_startRunFrames[] = {
	{SG_AIRun,		1.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,	   -1.0f,	NULL},
	{SG_AIRun,	   -1.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,		1.0f,	NULL},
	{SG_AIRun,		3.0f,	NULL},
	{SG_AIRun,		6.0f,	NULL},
	{SG_AIRun,		3.0f,	NULL}
};

static mframe_t			ironMaiden_runFrames[] = {
	{SG_AIRun,		6.0f,	NULL},
	{SG_AIRun,		8.0f,	NULL},
	{SG_AIRun,	   13.0f,	NULL},
	{SG_AIRun,		5.0f,	NULL},
	{SG_AIRun,		7.0f,	NULL},
	{SG_AIRun,		4.0f,	NULL},
	{SG_AIRun,	   11.0f,	NULL},
	{SG_AIRun,		5.0f,	NULL},
	{SG_AIRun,		9.0f,	NULL},
	{SG_AIRun,		7.0f,	NULL}
};

static mmove_t			ironMaiden_startRunMove = {FRAME_walk01, FRAME_walk10, ironMaiden_startRunFrames, IronMaiden_RunAnimation};
static mmove_t			ironMaiden_runMove = {FRAME_walk11, FRAME_walk20, ironMaiden_runFrames, NULL};


/*
 ==================
 IronMaiden_RunAnimation
 ==================
*/
static void IronMaiden_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND){
		self->monsterinfo.currentmove = &ironMaiden_standMove;
		return;
	}

	if (self->monsterinfo.currentmove == &ironMaiden_walkMove || self->monsterinfo.currentmove == &ironMaiden_startRunMove)
		self->monsterinfo.currentmove = &ironMaiden_runMove;
	else
		self->monsterinfo.currentmove = &ironMaiden_startRunMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 IronMaiden_Rocket
 ==================
*/
static void IronMaiden_Rocket (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_CHICK_ROCKET_1], forward, right, start);

	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	SG_MonsterFireRocket(self, start, dir, 50, 500, MZ2_CHICK_ROCKET_1);
}

/*
 ==================
 IronMaiden_Slash
 ==================
*/
static void IronMaiden_Slash (edict_t *self){

	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 10.0f);
	SG_MeleeHit(self, aim, (10 + (rand() %6)), 100);

	IronMaiden_MeleeSwingSound(self);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			chick_frames_start_attack1[] = {
	{SG_AICharge,	0.0f,	IronMaiden_MisslePreLaunchSound},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	4.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,	3.0f,	NULL},
	{SG_AICharge,	5.0f,	NULL},
	{SG_AICharge,	7.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	IronMaiden_RocketAttack}
};

static mframe_t			chick_frames_attack1[] = {
	{SG_AICharge,  19.0f,	IronMaiden_Rocket},
	{SG_AICharge,  -6.0f,	NULL},
	{SG_AICharge,  -5.0f,	NULL},
	{SG_AICharge,  -2.0f,	NULL},
	{SG_AICharge,  -7.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,  10.0f,	IronMaiden_MissileReloadSound},
	{SG_AICharge,	4.0f,	NULL},
	{SG_AICharge,	5.0f,	NULL},
	{SG_AICharge,	6.0f,	NULL},
	{SG_AICharge,	6.0f,	NULL},
	{SG_AICharge,	4.0f,	NULL},
	{SG_AICharge,	3.0f,	IronMaiden_ReRocket}
};

static mframe_t			chick_frames_end_attack1[] = {
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,  -6.0f,	NULL},
	{SG_AICharge,  -4.0f,	NULL},
	{SG_AICharge,  -2.0f,	NULL}
};

static mframe_t			chick_frames_slash[] = {
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	7.0f,	IronMaiden_Slash},
	{SG_AICharge,  -7.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,  -1.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,  -2.0f,	IronMaiden_ReSlash}
};

static mframe_t			chick_frames_end_slash[] = {
	{SG_AICharge,  -6.0f,	NULL},
	{SG_AICharge,  -1.0f,	NULL},
	{SG_AICharge,  -6.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			chick_frames_start_slash[] = {
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	8.0f,	NULL},
	{SG_AICharge,	3.0f,	NULL}
};

static mmove_t			chick_move_start_attack1 = {FRAME_attak101, FRAME_attak113, chick_frames_start_attack1, NULL};
static mmove_t			chick_move_attack1 = {FRAME_attak114, FRAME_attak127, chick_frames_attack1, NULL};
static mmove_t			chick_move_end_attack1 = {FRAME_attak128, FRAME_attak132, chick_frames_end_attack1, IronMaiden_RunAnimation};
static mmove_t			chick_move_slash = {FRAME_attak204, FRAME_attak212, chick_frames_slash, NULL};
static mmove_t			chick_move_end_slash = {FRAME_attak213, FRAME_attak216, chick_frames_end_slash, IronMaiden_RunAnimation};
static mmove_t			chick_move_start_slash = {FRAME_attak201, FRAME_attak203, chick_frames_start_slash, IronMaiden_SlashAttack};


/*
 ==================
 IronMaiden_ReRocket
 ==================
*/
static void IronMaiden_ReRocket (edict_t *self){

	if (self->enemy->health > 0){
		if (SG_EntityRange(self, self->enemy) > RANGE_MELEE){
			if (SG_IsEntityVisible(self, self->enemy)){
				if (random() <= 0.6f){
					self->monsterinfo.currentmove = &chick_move_attack1;
					return;
				}
			}
		}
	}	

	self->monsterinfo.currentmove = &chick_move_end_attack1;
}

/*
 ==================
 IronMaiden_RocketAttack
 ==================
*/
static void IronMaiden_RocketAttack (edict_t *self){

	self->monsterinfo.currentmove = &chick_move_attack1;
}

/*
 ==================
 IronMaiden_ReSlash
 ==================
*/
static void IronMaiden_ReSlash (edict_t *self){

	if (self->enemy->health > 0){
		if (SG_EntityRange(self, self->enemy) == RANGE_MELEE)
			if (random() <= 0.9f){				
				self->monsterinfo.currentmove = &chick_move_slash;
				return;
			}
			else {
				self->monsterinfo.currentmove = &chick_move_end_slash;
				return;
			}
	}

	self->monsterinfo.currentmove = &chick_move_end_slash;
}

/*
 ==================
 IronMaiden_SlashAttack
 ==================
*/
static void IronMaiden_SlashAttack (edict_t *self){

	self->monsterinfo.currentmove = &chick_move_slash;
}

/*
 ==================
 IronMaiden_AttackAnimation
 ==================
*/
static void IronMaiden_AttackAnimation (edict_t *self){

	self->monsterinfo.currentmove = &chick_move_start_attack1;
}

/*
 ==================
 IronMaiden_MeleeAnimation
 ==================
*/
static void IronMaiden_MeleeAnimation (edict_t *self){

	self->monsterinfo.currentmove = &chick_move_start_slash;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			ironMaiden_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			ironMaiden_pain2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			ironMaiden_pain3Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,    -6.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,    11.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,    -3.0f,	NULL},
	{SG_AIMove,    -4.0f,	NULL},
	{SG_AIMove,		5.0f,	NULL},
	{SG_AIMove,		7.0f,	NULL},
	{SG_AIMove,    -2.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,    -2.0f,	NULL},
	{SG_AIMove,    -8.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL}
};

static mmove_t			ironMaiden_pain1Move = {FRAME_pain101, FRAME_pain105, ironMaiden_pain1Frames, IronMaiden_RunAnimation};
static mmove_t			ironMaiden_pain2Move = {FRAME_pain201, FRAME_pain205, ironMaiden_pain2Frames, IronMaiden_RunAnimation};
static mmove_t			ironMaiden_pain3Move = {FRAME_pain301, FRAME_pain321, ironMaiden_pain3Frames, IronMaiden_RunAnimation};


/*
 ==================
 IronMaiden_PainAnimation
 ==================
*/
static void IronMaiden_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	float	r;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	r = random();

	if (r < 0.33f)
		IronMaiden_Pain1Sound(self);
	else if (r < 0.66f)
		IronMaiden_Pain2Sound(self);
	else
		IronMaiden_Pain3Sound(self);

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	if (damage <= 10)
		self->monsterinfo.currentmove = &ironMaiden_pain1Move;
	else if (damage <= 25)
		self->monsterinfo.currentmove = &ironMaiden_pain2Move;
	else
		self->monsterinfo.currentmove = &ironMaiden_pain3Move;
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			ironMaiden_death1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,    -7.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL},
	{SG_AIMove,    11.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			ironMaiden_death2Frames[] = {
	{SG_AIMove,    -6.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,    -1.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,    -1.0f,	NULL},
	{SG_AIMove,    -2.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,    10.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,    -3.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL},
	{SG_AIMove,    15.0f,	NULL},
	{SG_AIMove,    14.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL}
};

static mmove_t			ironMaiden_death1Move = {FRAME_death101, FRAME_death112, ironMaiden_death1Frames, IronMaiden_Dead};
static mmove_t			ironMaiden_death2Move = {FRAME_death201, FRAME_death223, ironMaiden_death2Frames, IronMaiden_Dead};


/*
 ==================
 IronMaiden_Dead
 ==================
*/
static void IronMaiden_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, 0.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 16.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 IronMaiden_DieAnimation
 ==================
*/
static void IronMaiden_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	int		n;

	// Check for gib
	if (self->health <= self->gib_health){
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

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

	n = rand() % 2;

	if (n == 0){
		self->monsterinfo.currentmove = &ironMaiden_death1Move;
		IronMaiden_Die1Sound(self);
	}
	else {
		self->monsterinfo.currentmove = &ironMaiden_death2Move;
		IronMaiden_Die2Sound(self);
	}
}


/*
 ==============================================================================

 DUCK ANIMATION

 ==============================================================================
*/

static mframe_t			ironMaiden_duckFrames[] = {
	{SG_AIMove,		0.0f,	IronMaiden_Duck_Down},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		4.0f,	IronMaiden_Duck_Hold},
	{SG_AIMove,    -4.0f,	NULL},
	{SG_AIMove,    -5.0f,	IronMaiden_Duck_Up},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL}
};

static mmove_t			ironMaiden_duckMove = {FRAME_duck01, FRAME_duck07, ironMaiden_duckFrames, IronMaiden_RunAnimation};


/*
 ==================
 IronMaiden_Duck_Down
 ==================
*/
static void IronMaiden_Duck_Down (edict_t *self){

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
 IronMaiden_Duck_Hold
 ==================
*/
static void IronMaiden_Duck_Hold (edict_t *self){

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

/*
 ==================
 IronMaiden_Duck_Up
 ==================
*/
static void IronMaiden_Duck_Up (edict_t *self){

	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32.0f;
	self->takedamage = DAMAGE_AIM;

	gi.linkentity(self);
}

/*
 ==================
 IronMaiden_DodgeAnimation
 ==================
*/
static void IronMaiden_DodgeAnimation (edict_t *self, edict_t *attacker, float eta){

	if (random() > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = &ironMaiden_duckMove;
}


/*
 ==============================================================================

 MONSTER_CHICK

 ==============================================================================
*/


/*
 ==================
 SG_MonsterIronMaiden_Spawn
 ==================
*/
void SG_MonsterIronMaiden_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, 0.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 56.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	ironMaiden.sightSound = gi.soundindex("chick/chksght1.wav");
	ironMaiden.idle1Sound = gi.soundindex("chick/chkidle1.wav");
	ironMaiden.idle2Sound = gi.soundindex("chick/chkidle2.wav");
	ironMaiden.pain1Sound = gi.soundindex("chick/chkpain1.wav");
	ironMaiden.pain2Sound = gi.soundindex("chick/chkpain2.wav");
	ironMaiden.pain3Sound = gi.soundindex("chick/chkpain3.wav");
	ironMaiden.die1Sound = gi.soundindex("chick/chkdeth1.wav");
	ironMaiden.die2Sound = gi.soundindex("chick/chkdeth2.wav");
	ironMaiden.searchSound = gi.soundindex("chick/chksrch1.wav");
	ironMaiden.fallDownSound = gi.soundindex("chick/chkfall1.wav");
	ironMaiden.misslePreLaunchSound = gi.soundindex("chick/chkatck1.wav");
	ironMaiden.missleLaunchSound = gi.soundindex("chick/chkatck2.wav");
	ironMaiden.missileReloadSound = gi.soundindex("chick/chkatck5.wav");
	ironMaiden.meleeSwingSound = gi.soundindex("chick/chkatck3.wav");
	ironMaiden.meleeHitSound = gi.soundindex("chick/chkatck4.wav");

	self->mass = 200;

	self->health = 175;
	self->gib_health = -70;

	self->pain = IronMaiden_PainAnimation;
	self->die = IronMaiden_DieAnimation;

	self->monsterinfo.stand = IronMaiden_StandAnimation;
	self->monsterinfo.walk = IronMaiden_WalkAnimation;
	self->monsterinfo.run = IronMaiden_RunAnimation;
	self->monsterinfo.dodge = IronMaiden_DodgeAnimation;
	self->monsterinfo.attack = IronMaiden_AttackAnimation;
	self->monsterinfo.melee = IronMaiden_MeleeAnimation;
	self->monsterinfo.sight = IronMaiden_SightSound;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &ironMaiden_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}