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
// m_icarus.c - Icarus monster
//


#include "g_local.h"
#include "m_hover.h"


typedef struct {
	int					sightSound;
	int					pain1Sound;
	int					pain2Sound;
	int					die1Sound;
	int					die2Sound;
	int					search1Sound;
	int					search2Sound;
} icarus_t;

static icarus_t			icarus;

static void Icarus_RunAnimation (edict_t *self);
static void Icarus_ReAttack (edict_t *self);
static void Icarus_BlasterAttack (edict_t *self);
static void Icarus_Dead (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Icarus_SightSound
 ==================
*/
static void Icarus_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, icarus.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Icarus_Pain1Sound
 ==================
*/
static void Icarus_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, icarus.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Icarus_Pain2Sound
 ==================
*/
static void Icarus_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, icarus.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Icarus_Die1Sound
 ==================
*/
static void Icarus_Die1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, icarus.die1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Icarus_Die2Sound
 ==================
*/
static void Icarus_Die2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, icarus.die2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Icarus_SearchSound
 ==================
*/
static void Icarus_SearchSound (edict_t *self){

	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, icarus.search1Sound, 1.0f, ATTN_NORM, 0.0f);
	else
		gi.sound(self, CHAN_VOICE, icarus.search2Sound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			icarus_standFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mmove_t			icarus_standMove = {FRAME_stand01, FRAME_stand30, icarus_standFrames, NULL};


/*
 ==================
 Icarus_StandAnimation
 ==================
*/
static void Icarus_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &icarus_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			icarus_walkFrames[] = {
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

static mmove_t			icarus_walkMove = {FRAME_forwrd01, FRAME_forwrd35, icarus_walkFrames, NULL};


/*
 ==================
 Icarus_WalkAnimation
 ==================
*/
static void Icarus_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &icarus_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			icarus_runFrames[] = {
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

static mmove_t			icarus_runMove = {FRAME_forwrd01, FRAME_forwrd35, icarus_runFrames, NULL};


/*
 ==================
 Icarus_RunAnimation
 ==================
*/
static void Icarus_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &icarus_standMove;
	else
		self->monsterinfo.currentmove = &icarus_runMove;
}


/*
 ==============================================================================

 STOP ANIMATION

 ==============================================================================
*/

static mframe_t			icarus_stop1Frames[] = {
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

static mframe_t			icarus_stop2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			icarus_stop1Move = {FRAME_stop101, FRAME_stop109, icarus_stop1Frames, NULL};
static mmove_t			icarus_stop2Move = {FRAME_stop201, FRAME_stop208, icarus_stop2Frames, NULL};


/*
 ==============================================================================

 TAKE OFF/LAND ANIMATION

 ==============================================================================
*/

static mframe_t			icarus_takeOffFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,		5.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,	   -1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,	   -6.0f,	NULL},
	{SG_AIMove,	   -9.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			icarus_landFrames[] = {
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			icarus_takeOffMove = {FRAME_takeof01, FRAME_takeof30, icarus_takeOffFrames, NULL};
static mmove_t			icarus_landMove = {FRAME_land01, FRAME_land01, icarus_landFrames, NULL};


/*
 ==============================================================================

 FORWARD/BACKWARD ANIMATION

 ==============================================================================
*/

static mframe_t			icarus_forwardFrames[] = {
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

static mframe_t			icarus_backwardsFrames[] = {
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

static mmove_t			icarus_forwardMove = {FRAME_forwrd01, FRAME_forwrd35, icarus_forwardFrames, NULL};
static mmove_t			icarus_backwardsMove = {FRAME_backwd01, FRAME_backwd24, icarus_backwardsFrames, NULL};


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Icarus_Blaster
 ==================
*/
static void Icarus_Blaster (edict_t *self){

	int		effect;
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	end;
	vec3_t	dir;

	if (self->s.frame == FRAME_attak104)
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_HOVER_BLASTER_1], forward, right, start);

	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);

	SG_MonsterFireBlaster(self, start, dir, 1, 1000, MZ2_HOVER_BLASTER_1, effect);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			icarus_startAttackFrames[] = {
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL}
};

static mframe_t			icarus_blasterAttackFrames[] = {
	{SG_AICharge, -10.0f,	Icarus_Blaster},
	{SG_AICharge, -10.0f,	Icarus_Blaster},
	{SG_AICharge,	0.0f,	Icarus_ReAttack}
};

static mframe_t			icarus_endAttackFrames[] = {
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL}
};

static mmove_t			icarus_startAttackMove = {FRAME_attak101, FRAME_attak103, icarus_startAttackFrames, Icarus_BlasterAttack};
static mmove_t			icarus_blasterAttackMove = {FRAME_attak104, FRAME_attak106, icarus_blasterAttackFrames, NULL};
static mmove_t			icarus_endAttackMove = {FRAME_attak107, FRAME_attak108, icarus_endAttackFrames, Icarus_RunAnimation};


/*
 ==================
 Icarus_ReAttack
 ==================
*/
static void Icarus_ReAttack (edict_t *self){

	if (self->enemy->health > 0){
		if (SG_IsEntityVisible(self, self->enemy)){
			if (random() <= 0.6f){
				self->monsterinfo.currentmove = &icarus_blasterAttackMove;
				return;
			}
		}
	}

	self->monsterinfo.currentmove = &icarus_endAttackMove;
}

/*
 ==================
 Icarus_BlasterAttack
 ==================
*/
static void Icarus_BlasterAttack (edict_t *self){

	self->monsterinfo.currentmove = &icarus_blasterAttackMove;
}

/*
 ==================
 Icarus_AttackAnimation
 ==================
*/
static void Icarus_AttackAnimation (edict_t *self){

	self->monsterinfo.currentmove = &icarus_startAttackMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			icarus_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,	   -8.0f,	NULL},
	{SG_AIMove,	   -4.0f,	NULL},
	{SG_AIMove,	   -6.0f,	NULL},
	{SG_AIMove,	   -4.0f,	NULL},
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		7.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		5.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL}
};

static mframe_t			icarus_pain2Frames[] = {
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

static mframe_t			icarus_pain3Frames[] = {
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

static mmove_t			icarus_pain1Move = {FRAME_pain101, FRAME_pain128, icarus_pain1Frames, Icarus_RunAnimation};
static mmove_t			icarus_pain2Move = {FRAME_pain201, FRAME_pain212, icarus_pain2Frames, Icarus_RunAnimation};
static mmove_t			icarus_pain3Move = {FRAME_pain301, FRAME_pain309, icarus_pain3Frames, Icarus_RunAnimation};


/*
 ==================
 Icarus_PainAnimation
 ==================
*/
static void Icarus_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	if (damage <= 25){
		if (random() < 0.5f){
			Icarus_Pain1Sound(self);
			self->monsterinfo.currentmove = &icarus_pain3Move;
		}
		else {
			Icarus_Pain2Sound(self);
			self->monsterinfo.currentmove = &icarus_pain2Move;
		}
	}
	else {
		Icarus_Pain1Sound(self);
		self->monsterinfo.currentmove = &icarus_pain1Move;
	}
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			icarus_deathFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	  -10.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		5.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL},
	{SG_AIMove,		7.0f,	NULL}
};

static mmove_t			icarus_deathMove = {FRAME_death101, FRAME_death111, icarus_deathFrames, Icarus_Dead};


/*
 ==================
 Icarus_Dead_Think
 ==================
*/
static void Icarus_Dead_Think (edict_t *self){

	if (!self->groundentity && level.time < self->timestamp){
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	SG_BecomeExplosion1(self);
}

/*
 ==================
 Icarus_Dead
 ==================
*/
static void Icarus_Dead (edict_t *self){

	VectorSet (self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet (self->maxs, 16.0f, 16.0f, -8.0f);
	self->movetype = MOVETYPE_TOSS;
	self->think = Icarus_Dead_Think;
	self->nextthink = level.time + FRAMETIME;
	self->timestamp = level.time + 15.0f;

	gi.linkentity(self);
}

/*
 ==================
 Icarus_DieAnimation
 ==================
*/
static void Icarus_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

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
	if (random() < 0.5f)
		Icarus_Die1Sound(self);
	else
		Icarus_Die2Sound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &icarus_deathMove;
}


/*
 ==============================================================================

 MONSTER_HOVER

 ==============================================================================
*/


/*
 ==================
 SG_MonsterIcarus_Spawn
 ==================
*/
void SG_MonsterIcarus_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/hover/tris.md2");
	
	VectorSet(self->mins, -24.0f, -24.0f, -24.0f);
	VectorSet(self->maxs, 24.0f, 24.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	icarus.sightSound = gi.soundindex("hover/hovsght1.wav");	
	icarus.pain1Sound = gi.soundindex("hover/hovpain1.wav");	
	icarus.pain2Sound = gi.soundindex("hover/hovpain2.wav");	
	icarus.die1Sound = gi.soundindex("hover/hovdeth1.wav");	
	icarus.die2Sound = gi.soundindex("hover/hovdeth2.wav");	
	icarus.search1Sound = gi.soundindex("hover/hovsrch1.wav");	
	icarus.search2Sound = gi.soundindex("hover/hovsrch2.wav");	

	gi.soundindex("hover/hovatck1.wav");	

	self->s.sound = gi.soundindex("hover/hovidle1.wav");

	self->health = 240;
	self->gib_health = -100;

	self->mass = 150;

	self->pain = Icarus_PainAnimation;
	self->die = Icarus_DieAnimation;

	self->monsterinfo.stand = Icarus_StandAnimation;
	self->monsterinfo.walk = Icarus_WalkAnimation;
	self->monsterinfo.run = Icarus_RunAnimation;
	self->monsterinfo.attack = Icarus_AttackAnimation;
	self->monsterinfo.sight = Icarus_SightSound;
	self->monsterinfo.search = Icarus_SearchSound;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &icarus_standMove;	
	self->monsterinfo.scale = MODEL_SCALE;

	SG_AirMonster(self);
}