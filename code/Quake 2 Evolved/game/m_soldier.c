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
// m_soldier.c - Soldier monster
//

// NOTE:
// - This monster has a "stand" and "attack" animation that isn't used


#include "g_local.h"
#include "m_soldier.h"


typedef struct {
	int					sight1Sound;
	int					sight2Sound;
	int					idleSound;
	int					pain1Sound;
	int					pain2Sound;
	int					pain3Sound;
	int					die1Sound;
	int					die2Sound;
	int					die3Sound;
	int					weaponCockSound;
} soldier_t;

static soldier_t		soldier;

static void Soldier_StandAnimation (edict_t *self);
static void Soldier_Walk1Random (edict_t *self);
static void Soldier_RunAnimation (edict_t *self);
static void Soldier_Attack1_ReFire1 (edict_t *self);
static void Soldier_Attack1_ReFire2 (edict_t *self);
static void Soldier_Attack2_ReFire1 (edict_t *self);
static void Soldier_Attack2_ReFire2 (edict_t *self);
static void Soldier_Attack3_ReFire1 (edict_t *self);
static void Soldier_Attack6_ReFire1 (edict_t *self);
static void Soldier_Fire1 (edict_t *self);
static void Soldier_Fire2 (edict_t *self);
static void Soldier_Fire3 (edict_t *self);
static void Soldier_Fire4 (edict_t *self);
static void Soldier_Fire8 (edict_t *self);
static void Soldier_Dead (edict_t *self);
static void Soldier_Duck_Down (edict_t *self);
static void Soldier_Duck_Hold (edict_t *self);
static void Soldier_Duck_Up (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Soldier_Sight1Sound
 ==================
*/
static void Soldier_Sight1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, soldier.sight1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Soldier_Sight2Sound
 ==================
*/
static void Soldier_Sight2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, soldier.sight2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Soldier_IdleSound
 ==================
*/
static void Soldier_IdleSound (edict_t *self){

	if (random() > 0.8f)
		gi.sound(self, CHAN_VOICE, soldier.idleSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Soldier_Pain1Sound
 ==================
*/
static void Soldier_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, soldier.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Soldier_Pain2Sound
 ==================
*/
static void Soldier_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, soldier.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Soldier_Pain3Sound
 ==================
*/
static void Soldier_Pain3Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, soldier.pain3Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Soldier_Die1Sound
 ==================
*/
static void Soldier_Die1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, soldier.die1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Soldier_Die2Sound
 ==================
*/
static void Soldier_Die2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, soldier.die2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Soldier_Die3Sound
 ==================
*/
static void Soldier_Die3Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, soldier.die3Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Soldier_WeaponCockSound
 ==================
*/
static void Soldier_WeaponCockSound (edict_t *self){

	if (self->s.frame == FRAME_stand322)
		gi.sound(self, CHAN_WEAPON, soldier.weaponCockSound, 1.0f, ATTN_IDLE, 0.0f);
	else
		gi.sound(self, CHAN_WEAPON, soldier.weaponCockSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			soldier_stand1Frames[] = {
	{SG_AIStand,	0.0f,	Soldier_IdleSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mframe_t			soldier_stand3Frames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Soldier_WeaponCockSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

#if 0
static mframe_t			soldier_frames_stand4[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	4.0f,	NULL},
	{SG_AIStand,	1.0f,	NULL},
	{SG_AIStand,   -1.0f,	NULL},
	{SG_AIStand,   -2.0f,	NULL}
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL}
};

static mmove_t			soldier_move_stand4 = {FRAME_stand401, FRAME_stand452, soldier_frames_stand4, NULL};
#endif

static mmove_t			soldier_stand1Move = {FRAME_stand101, FRAME_stand130, soldier_stand1Frames, Soldier_StandAnimation};
static mmove_t			soldier_stand3Move = {FRAME_stand301, FRAME_stand339, soldier_stand3Frames, Soldier_StandAnimation};


/*
 ==================
 Soldier_StandAnimation
 ==================
*/
static void Soldier_StandAnimation (edict_t *self){

	if ((self->monsterinfo.currentmove == &soldier_stand3Move) || (random() < 0.8f))
		self->monsterinfo.currentmove = &soldier_stand1Move;
	else
		self->monsterinfo.currentmove = &soldier_stand3Move;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			soldier_walk1Frames[] = {
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		1.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,    -1.0f,	Soldier_Walk1Random},
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
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL}
};

static mframe_t			soldier_walk2Frames[] = {
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		9.0f,	NULL},
	{SG_AIWalk,		8.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		1.0f,	NULL},
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL}
};

static mmove_t			soldier_walk1Move = {FRAME_walk101, FRAME_walk133, soldier_walk1Frames, NULL};
static mmove_t			soldier_walk2Move = {FRAME_walk209, FRAME_walk218, soldier_walk2Frames, NULL};


/*
 ==================
 Soldier_Walk1Random
 ==================
*/
static void Soldier_Walk1Random (edict_t *self){

	if (random() > 0.1f)
		self->monsterinfo.nextframe = FRAME_walk101;
}

/*
 ==================
 Soldier_WalkAnimation
 ==================
*/
static void Soldier_WalkAnimation (edict_t *self){

	if (random() < 0.5f)
		self->monsterinfo.currentmove = &soldier_walk1Move;
	else
		self->monsterinfo.currentmove = &soldier_walk2Move;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			soldier_startRunFrames[] = {
	{SG_AIRun,		7.0f,	NULL},
	{SG_AIRun,		5.0f,	NULL}
};

static mframe_t			soldier_runFrames[] = {
	{SG_AIRun,     10.0f,	NULL},
	{SG_AIRun,     11.0f,	NULL},
	{SG_AIRun,     11.0f,	NULL},
	{SG_AIRun,     16.0f,	NULL},
	{SG_AIRun,     10.0f,	NULL},
	{SG_AIRun,	   15.0f,	NULL}
};

static mmove_t			soldier_startRunMove = {FRAME_run01, FRAME_run02, soldier_startRunFrames, Soldier_RunAnimation};
static mmove_t			soldier_runMove = {FRAME_run03, FRAME_run08, soldier_runFrames, NULL};


/*
 ==================
 Soldier_RunAnimation
 ==================
*/
static void Soldier_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND){
		self->monsterinfo.currentmove = &soldier_stand1Move;
		return;
	}

	if (self->monsterinfo.currentmove == &soldier_walk1Move || self->monsterinfo.currentmove == &soldier_walk2Move || self->monsterinfo.currentmove == &soldier_startRunMove)
		self->monsterinfo.currentmove = &soldier_runMove;
	else
		self->monsterinfo.currentmove = &soldier_startRunMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/

static int				blasterFlash[] = {MZ2_SOLDIER_BLASTER_1, MZ2_SOLDIER_BLASTER_2, MZ2_SOLDIER_BLASTER_3, MZ2_SOLDIER_BLASTER_4, MZ2_SOLDIER_BLASTER_5, MZ2_SOLDIER_BLASTER_6, MZ2_SOLDIER_BLASTER_7, MZ2_SOLDIER_BLASTER_8};
static int				shotgunFlash[] = {MZ2_SOLDIER_SHOTGUN_1, MZ2_SOLDIER_SHOTGUN_2, MZ2_SOLDIER_SHOTGUN_3, MZ2_SOLDIER_SHOTGUN_4, MZ2_SOLDIER_SHOTGUN_5, MZ2_SOLDIER_SHOTGUN_6, MZ2_SOLDIER_SHOTGUN_7, MZ2_SOLDIER_SHOTGUN_8};
static int				machinegunFlash[] = {MZ2_SOLDIER_MACHINEGUN_1, MZ2_SOLDIER_MACHINEGUN_2, MZ2_SOLDIER_MACHINEGUN_3, MZ2_SOLDIER_MACHINEGUN_4, MZ2_SOLDIER_MACHINEGUN_5, MZ2_SOLDIER_MACHINEGUN_6, MZ2_SOLDIER_MACHINEGUN_7, MZ2_SOLDIER_MACHINEGUN_8};


/*
 ==================
 Soldier_WeaponFire
 ==================
*/
static void Soldier_WeaponFire (edict_t *self, int flashNumber){

	vec3_t	start;
	vec3_t	forward, right, up;
	vec3_t	aim;
	vec3_t	dir;
	vec3_t	end;
	float	r, u;
	int		flashIndex;

	if (self->s.skinnum < 2)
		flashIndex = blasterFlash[flashNumber];
	else if (self->s.skinnum < 4)
		flashIndex = shotgunFlash[flashNumber];
	else
		flashIndex = machinegunFlash[flashNumber];

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashIndex], forward, right, start);

	if (flashNumber == 5 || flashNumber == 6)
		VectorCopy (forward, aim);
	else {
		VectorCopy(self->enemy->s.origin, end);
		end[2] += self->enemy->viewheight;
		VectorSubtract(end, start, aim);
		SG_VectorToAngles(aim, dir);
		AngleVectors(dir, forward, right, up);

		r = crandom() * 1000.0f;
		u = crandom() * 500.0f;
		VectorMA(start, 8192, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		VectorSubtract(end, start, aim);
		VectorNormalize(aim);
	}

	if (self->s.skinnum <= 1)
		SG_MonsterFireBlaster(self, start, aim, 5, 600, flashIndex, EF_BLASTER);
	else if (self->s.skinnum <= 3)
		SG_MonsterFireShotgun(self, start, aim, 2, 1, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SHOTGUN_COUNT, flashIndex);
	else {
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			self->monsterinfo.pausetime = level.time + (3 + rand() % 8) * FRAMETIME;

		SG_MonsterFireBullet(self, start, aim, 2, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flashIndex);

		if (level.time >= self->monsterinfo.pausetime)
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		else
			self->monsterinfo.aiflags |= AI_HOLD_FRAME;
	}
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			soldier_attack1Frames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Soldier_Fire1},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Soldier_Attack1_ReFire1},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Soldier_WeaponCockSound},
	{SG_AICharge,	0.0f,	Soldier_Attack1_ReFire2},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			soldier_attack2Frames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Soldier_Fire2},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Soldier_Attack2_ReFire1},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Soldier_WeaponCockSound},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Soldier_Attack2_ReFire2},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			soldier_attack3Frames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Soldier_Fire3},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Soldier_Attack3_ReFire1},
	{SG_AICharge,	0.0f,	Soldier_Duck_Up},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			soldier_attack4Frames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	Soldier_Fire4},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			soldier_attack6Frames[] = {
	{SG_AICharge,  10.0f,	NULL},
	{SG_AICharge,	4.0f,	NULL},
	{SG_AICharge,  12.0f,	NULL},
	{SG_AICharge,  11.0f,	Soldier_Fire8},
	{SG_AICharge,  13.0f,	NULL},
	{SG_AICharge,  18.0f,	NULL},
	{SG_AICharge,  15.0f,	NULL},
	{SG_AICharge,  14.0f,	NULL},
	{SG_AICharge,  11.0f,	NULL},
	{SG_AICharge,	8.0f,	NULL},
	{SG_AICharge,  11.0f,	NULL},
	{SG_AICharge,  12.0f,	NULL},
	{SG_AICharge,  12.0f,	NULL},
	{SG_AICharge,  17.0f,	Soldier_Attack6_ReFire1}
};

static mmove_t			soldier_attack1Move = {FRAME_attak101, FRAME_attak112, soldier_attack1Frames, Soldier_RunAnimation};
static mmove_t			soldier_attack2Move = {FRAME_attak201, FRAME_attak218, soldier_attack2Frames, Soldier_RunAnimation};
static mmove_t			soldier_attack3Move = {FRAME_attak301, FRAME_attak309, soldier_attack3Frames, Soldier_RunAnimation};
static mmove_t			soldier_attack4Move = {FRAME_attak401, FRAME_attak406, soldier_attack4Frames, Soldier_RunAnimation};
static mmove_t			soldier_attack6Move = {FRAME_runs01, FRAME_runs14, soldier_attack6Frames, Soldier_RunAnimation};


#if 0
// ATTACK5 (prone)
static mframe_t soldier_frames_attack5[] = {
	SG_AICharge, 8, NULL,
	SG_AICharge, 8, NULL,
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	SG_AICharge, 0, soldier_fire5,
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	SG_AICharge, 0, soldier_attack5_refire
};

static mmove_t soldier_move_attack5 = {FRAME_attak501, FRAME_attak508, soldier_frames_attack5, soldier_run};

static void soldier_fire5 (edict_t *self){

	soldier_fire(self, 4);
}

static void soldier_attack5_refire (edict_t *self){

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (SG_EntityRange(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak505;
}
#endif

/*
 ==================
 Soldier_Attack1_ReFire1
 ==================
*/
static void Soldier_Attack1_ReFire1 (edict_t *self){

	if (self->s.skinnum > 1)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (SG_EntityRange(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak102;
	else
		self->monsterinfo.nextframe = FRAME_attak110;
}

/*
 ==================
 Soldier_Attack1_ReFire2
 ==================
*/
static void Soldier_Attack1_ReFire2 (edict_t *self){

	if (self->s.skinnum < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (SG_EntityRange(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak102;
}

/*
 ==================
 Soldier_Attack2_ReFire1
 ==================
*/
static void Soldier_Attack2_ReFire1 (edict_t *self){

	if (self->s.skinnum > 1)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (SG_EntityRange(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak204;
	else
		self->monsterinfo.nextframe = FRAME_attak216;
}

/*
 ==================
 Soldier_Attack2_ReFire2
 ==================
*/
static void Soldier_Attack2_ReFire2 (edict_t *self){

	if (self->s.skinnum < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((skill->value == 3) && (random() < 0.5f)) || (SG_EntityRange(self, self->enemy) == RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak204;
}

/*
 ==================
 Soldier_Attack3_ReFire1
 ==================
*/
static void Soldier_Attack3_ReFire1 (edict_t *self){

	if ((level.time + 0.4f) < self->monsterinfo.pausetime)
		self->monsterinfo.nextframe = FRAME_attak303;
}

/*
 ==================
 Soldier_Attack6_ReFire1
 ==================
*/
static void Soldier_Attack6_ReFire1 (edict_t *self){

	if (self->enemy->health <= 0)
		return;

	if (SG_EntityRange(self, self->enemy) < RANGE_MID)
		return;

	if (skill->value == 3)
		self->monsterinfo.nextframe = FRAME_runs03;
}

/*
 ==================
 Soldier_Fire1
 ==================
*/
static void Soldier_Fire1 (edict_t *self){

	Soldier_WeaponFire(self, 0);
}

/*
 ==================
 Soldier_Fire2
 ==================
*/
static void Soldier_Fire2 (edict_t *self){

	Soldier_WeaponFire(self, 1);
}

/*
 ==================
 Soldier_Fire3
 ==================
*/
static void Soldier_Fire3 (edict_t *self){

	Soldier_Duck_Down(self);
	Soldier_WeaponFire(self, 2);
}

/*
 ==================
 Soldier_Fire4
 ==================
*/
static void Soldier_Fire4 (edict_t *self){

	Soldier_WeaponFire(self, 3);
}

/*
 ==================
 Soldier_Fire6
 ==================
*/
static void Soldier_Fire6 (edict_t *self){

	Soldier_WeaponFire(self, 5);
}

/*
 ==================
 Soldier_Fire7
 ==================
*/
static void Soldier_Fire7 (edict_t *self){

	Soldier_WeaponFire(self, 6);
}

/*
 ==================
 Soldier_Fire8
 ==================
*/
static void Soldier_Fire8 (edict_t *self){

	Soldier_WeaponFire(self, 7);
}

/*
 ==================
 Soldier_AttackAnimation
 ==================
*/
static void Soldier_AttackAnimation (edict_t *self){

	if (self->s.skinnum < 4){
		if (random() < 0.5f)
			self->monsterinfo.currentmove = &soldier_attack1Move;
		else
			self->monsterinfo.currentmove = &soldier_attack2Move;
	}
	else
		self->monsterinfo.currentmove = &soldier_attack4Move;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/


static mframe_t			soldier_pain1Frames[] = {
	{SG_AIMove,    -3.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			soldier_pain2Frames[] = {
	{SG_AIMove,   -13.0f,	NULL},
	{SG_AIMove,    -1.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL}
};

static mframe_t			soldier_pain3Frames[] = {
	{SG_AIMove,    -8.0f,	NULL},
	{SG_AIMove,    10.0f,	NULL},
	{SG_AIMove,    -4.0f,	NULL},
	{SG_AIMove,    -1.0f,	NULL},
	{SG_AIMove,    -3.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL}
};

static mframe_t			soldier_pain4Frames[] = {
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,   -10.0f,	NULL},
	{SG_AIMove,    -6.0f,	NULL},
	{SG_AIMove,		8.0f,   NULL},
	{SG_AIMove,		4.0f,   NULL},
	{SG_AIMove,		1.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,		5.0f,   NULL},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,    -1.0f,	NULL},
	{SG_AIMove,    -1.0f,	NULL},
	{SG_AIMove,		3.0f,   NULL},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL}
};

static mmove_t			soldier_pain1Move = {FRAME_pain101, FRAME_pain105, soldier_pain1Frames, Soldier_RunAnimation};
static mmove_t			soldier_pain2Move = {FRAME_pain201, FRAME_pain207, soldier_pain2Frames, Soldier_RunAnimation};
static mmove_t			soldier_pain3Move = {FRAME_pain301, FRAME_pain318, soldier_pain3Frames, Soldier_RunAnimation};
static mmove_t			soldier_pain4Move = {FRAME_pain401, FRAME_pain417, soldier_pain4Frames, Soldier_RunAnimation};


/*
 ==================
 Soldier_PainAnimation
 ==================
*/
static void Soldier_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	int		n;
	float	r;

	if (self->health < (self->max_health / 2))
			self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time){
		if ((self->velocity[2] > 100.0f) && ((self->monsterinfo.currentmove == &soldier_pain1Move) || (self->monsterinfo.currentmove == &soldier_pain2Move) || (self->monsterinfo.currentmove == &soldier_pain3Move)))
			self->monsterinfo.currentmove = &soldier_pain4Move;

		return;
	}

	self->pain_debounce_time = level.time + 3.0f;

	n = self->s.skinnum | 1;

	if (n == 1)
		Soldier_Pain1Sound(self);
	else if (n == 3)
		Soldier_Pain2Sound(self);
	else
		Soldier_Pain3Sound(self);

	if (self->velocity[2] > 100.0f){
		self->monsterinfo.currentmove = &soldier_pain4Move;
		return;
	}

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	r = random();

	if (r < 0.33f)
		self->monsterinfo.currentmove = &soldier_pain1Move;
	else if (r < 0.66f)
		self->monsterinfo.currentmove = &soldier_pain2Move;
	else
		self->monsterinfo.currentmove = &soldier_pain3Move;
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			soldier_death1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,   -10.0f,	NULL},
	{SG_AIMove,   -10.0f,	NULL},
	{SG_AIMove,   -10.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
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
	{SG_AIMove,		0.0f,   Soldier_Fire6},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,   Soldier_Fire7},
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

static mframe_t			soldier_death2Frames[] = {
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
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

static mframe_t			soldier_death3Frames[] = {
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
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
};

static mframe_t			soldier_death4Frames[] = {
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

static mframe_t			soldier_death5Frames[] = {
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
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

static mframe_t			soldier_death6Frames[] = {
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

static mmove_t			soldier_death1Move = {FRAME_death101, FRAME_death136, soldier_death1Frames, Soldier_Dead};
static mmove_t			soldier_death2Move = {FRAME_death201, FRAME_death235, soldier_death2Frames, Soldier_Dead};
static mmove_t			soldier_death3Move = {FRAME_death301, FRAME_death345, soldier_death3Frames, Soldier_Dead};
static mmove_t			soldier_death4Move = {FRAME_death401, FRAME_death453, soldier_death4Frames, Soldier_Dead};
static mmove_t			soldier_death5Move = {FRAME_death501, FRAME_death524, soldier_death5Frames, Soldier_Dead};
static mmove_t			soldier_death6Move = {FRAME_death601, FRAME_death610, soldier_death6Frames, Soldier_Dead};


/*
 ==================
 Soldier_Dead
 ==================
*/
static void Soldier_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Soldier_DieAnimation
 ==================
*/
static void Soldier_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){
	
	int		n;

	// Check for gib
	if (self->health <= self->gib_health){
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1.0f, ATTN_NORM, 0.0f);

		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);

		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);

		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// Regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->s.skinnum |= 1;

	if (self->s.skinnum == 1)
		Soldier_Die1Sound(self);
	else if (self->s.skinnum == 3)
		Soldier_Die2Sound(self);
	else
		Soldier_Die3Sound(self);

	// Head shot
	if (fabs((self->s.origin[2] + self->viewheight) - point[2]) <= 4.0f){
		self->monsterinfo.currentmove = &soldier_death3Move;
		return;
	}

	n = rand() % 5;

	if (n == 0)
		self->monsterinfo.currentmove = &soldier_death1Move;
	else if (n == 1)
		self->monsterinfo.currentmove = &soldier_death2Move;
	else if (n == 2)
		self->monsterinfo.currentmove = &soldier_death4Move;
	else if (n == 3)
		self->monsterinfo.currentmove = &soldier_death5Move;
	else
		self->monsterinfo.currentmove = &soldier_death6Move;
}


/*
 ==============================================================================

 SIGHT ANIMATION

 ==============================================================================
*/


/*
 ==================
 Soldier_SightAnimation
 ==================
*/
static void Soldier_SightAnimation (edict_t *self, edict_t *other){

	if (random() < 0.5f)
		Soldier_Sight1Sound(self);
	else
		Soldier_Sight2Sound(self);

	if ((skill->value > 0) && (SG_EntityRange(self, self->enemy) >= RANGE_MID)){
		if (random() > 0.5f)
			self->monsterinfo.currentmove = &soldier_attack6Move;
	}
}


/*
 ==============================================================================

 DUCK ANIMATION

 ==============================================================================
*/

static mframe_t			soldier_duckFrames[] = {
	{SG_AIMove,		5.0f,	Soldier_Duck_Down},
	{SG_AIMove,    -1.0f,	Soldier_Duck_Hold},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	Soldier_Duck_Up},
	{SG_AIMove,		5.0f,	NULL}
};

static mmove_t			soldier_duckMove = {FRAME_duck01, FRAME_duck05, soldier_duckFrames, Soldier_RunAnimation};


/*
 ==================
 Soldier_Duck_Down
 ==================
*/
static void Soldier_Duck_Down (edict_t *self){

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
 Soldier_Duck_Hold
 ==================
*/
static void Soldier_Duck_Hold (edict_t *self){

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

/*
 ==================
 Soldier_Duck_Up
 ==================
*/
static void Soldier_Duck_Up (edict_t *self){

	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32.0f;
	self->takedamage = DAMAGE_AIM;

	gi.linkentity(self);
}

/*
 ==================
 Soldier_DodgeAnimation
 ==================
*/
static void Soldier_DodgeAnimation (edict_t *self, edict_t *attacker, float eta){

	float	r;

	r = random();

	if (r > 0.25f)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	if (skill->value == 0){
		self->monsterinfo.currentmove = &soldier_duckMove;
		return;
	}

	self->monsterinfo.pausetime = level.time + eta + 0.3f;

	r = random();

	if (skill->value == 1){
		if (r > 0.33f)
			self->monsterinfo.currentmove = &soldier_duckMove;
		else
			self->monsterinfo.currentmove = &soldier_attack3Move;

		return;
	}

	if (skill->value >= 2){
		if (r > 0.66f)
			self->monsterinfo.currentmove = &soldier_duckMove;
		else
			self->monsterinfo.currentmove = &soldier_attack3Move;

		return;
	}

	self->monsterinfo.currentmove = &soldier_attack3Move;
}


/*
 ==============================================================================

 MONSTER_SOLDIER

 ==============================================================================
*/


/*
 ==================
 SG_MonsterSoldier_X
 ==================
*/
static void SG_MonsterSoldier_X (edict_t *self){

	self->s.modelindex = gi.modelindex("models/monsters/soldier/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	soldier.idleSound = gi.soundindex("soldier/solidle1.wav");
	soldier.sight1Sound = gi.soundindex("soldier/solsght1.wav");
	soldier.sight2Sound = gi.soundindex("soldier/solsrch1.wav");
	soldier.weaponCockSound = gi.soundindex("infantry/infatck3.wav");

	self->mass = 100;

	self->pain = Soldier_PainAnimation;
	self->die = Soldier_DieAnimation;

	self->monsterinfo.stand = Soldier_StandAnimation;
	self->monsterinfo.walk = Soldier_WalkAnimation;
	self->monsterinfo.run = Soldier_RunAnimation;
	self->monsterinfo.dodge = Soldier_DodgeAnimation;
	self->monsterinfo.attack = Soldier_AttackAnimation;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = Soldier_SightAnimation;

	gi.linkentity(self);

	self->monsterinfo.scale = MODEL_SCALE;
	self->monsterinfo.stand(self);

	SG_GroundMonster(self);
}

/*
 ==================
 SG_MonsterSoldierLight_Spawn
 ==================
*/
void SG_MonsterSoldierLight_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	SG_MonsterSoldier_X(self);

	soldier.pain1Sound = gi.soundindex("soldier/solpain2.wav");
	soldier.die1Sound =	gi.soundindex("soldier/soldeth2.wav");
	gi.modelindex("models/objects/laser/tris.md2");
	gi.soundindex("misc/lasfly.wav");
	gi.soundindex("soldier/solatck2.wav");

	self->s.skinnum = 0;

	self->health = 20;
	self->gib_health = -30;
}

/*
 ==================
 SG_MonsterSoldier_Spawn
 ==================
*/
void SG_MonsterSoldier_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	SG_MonsterSoldier_X(self);

	soldier.pain2Sound = gi.soundindex("soldier/solpain1.wav");
	soldier.die2Sound = gi.soundindex("soldier/soldeth1.wav");
	gi.soundindex("soldier/solatck1.wav");

	self->s.skinnum = 2;

	self->health = 30;
	self->gib_health = -30;
}

/*
 ==================
 SG_MonsterSoldierSS_Spawn
 ==================
*/
void SG_MonsterSoldierSS_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	SG_MonsterSoldier_X(self);

	soldier.pain3Sound = gi.soundindex("soldier/solpain3.wav");
	soldier.die3Sound = gi.soundindex("soldier/soldeth3.wav");
	gi.soundindex("soldier/solatck3.wav");

	self->s.skinnum = 4;

	self->health = 40;
	self->gib_health = -30;
}