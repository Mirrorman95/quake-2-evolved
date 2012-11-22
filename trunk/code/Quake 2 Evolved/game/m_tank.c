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
// m_tank.c - Tank monster
//


#include "g_local.h"
#include "m_tank.h"


typedef struct {
	int					sightSound;
	int					stepSound;
	int					thudSound;
	int					windupSound;
	int					idleSound;
	int					strikeSound;
	int					painSound;
	int					dieSound;
} tank_t;

static tank_t			tank;

static void Tank_WalkAnimation (edict_t *self);
static void Tank_RunAnimation (edict_t *self);
static void Tank_Reattack_Blaster (edict_t *self);
static void Tank_Poststrike (edict_t *self);
static void Tank_Refire_Rocket (edict_t *self);
static void Tank_Doattack_Rocket (edict_t *self);
static void Tank_Dead (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Tank_SightSound
 ==================
*/
static void Tank_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_VOICE, tank.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Tank_FootstepSound
 ==================
*/
static void Tank_FootstepSound (edict_t *self){

	gi.sound(self, CHAN_BODY, tank.stepSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Tank_ThudSound
 ==================
*/
static void Tank_ThudSound (edict_t *self){

	gi.sound(self, CHAN_BODY, tank.thudSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Tank_WindupSound
 ==================
*/
static void Tank_WindupSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, tank.windupSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Tank_IdleSound
 ==================
*/
static void Tank_IdleSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, tank.idleSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Tank_StrikeSound
 ==================
*/
static void Tank_StrikeSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, tank.strikeSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Tank_PainSound
 ==================
*/
static void Tank_PainSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, tank.painSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Tank_DieSound
 ==================
*/
static void Tank_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE,  tank.dieSound, 1.0f, ATTN_NORM, 0.0f);
}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			tank_standFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
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

static mmove_t			tank_standMove = {FRAME_stand01, FRAME_stand30, tank_standFrames, NULL};


/*
 ==================
 Tank_StandAnimation
 ==================
*/
static void Tank_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &tank_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			tank_startWalkFrames[] = {
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		11.0f,	Tank_FootstepSound}
};

static mframe_t			tank_walkFrames[] = {
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		4.0f,	Tank_FootstepSound},
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,		6.0f,	Tank_FootstepSound}
};

static mframe_t			tank_stopWalkFrames[] = {
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		4.0f,	Tank_FootstepSound}
};

static mmove_t			tank_startWalkMove = {FRAME_walk01, FRAME_walk04, tank_startWalkFrames, Tank_WalkAnimation};
static mmove_t			tank_walkMove = {FRAME_walk05, FRAME_walk20, tank_walkFrames, NULL};
static mmove_t			tank_stopWalkMove = {FRAME_walk21, FRAME_walk25, tank_stopWalkFrames, Tank_StandAnimation};


/*
 ==================
 Tank_WalkAnimation
 ==================
*/
static void Tank_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &tank_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			tank_startRunFrames[] = {
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,		6.0f,	NULL},
	{SG_AIRun,		6.0f,	NULL},
	{SG_AIRun,		11.0f,  Tank_FootstepSound}
};

static mframe_t			tank_runFrames[] = {
	{SG_AIRun,		4.0f,	NULL},
	{SG_AIRun,		5.0f,	NULL},
	{SG_AIRun,		3.0f,	NULL},
	{SG_AIRun,		2.0f,	NULL},
	{SG_AIRun,		5.0f,	NULL},
	{SG_AIRun,		5.0f,	NULL},
	{SG_AIRun,		4.0f,	NULL},
	{SG_AIRun,		4.0f,	Tank_FootstepSound},
	{SG_AIRun,		3.0f,	NULL},
	{SG_AIRun,		5.0f,	NULL},
	{SG_AIRun,		4.0f,	NULL},
	{SG_AIRun,		5.0f,	NULL},
	{SG_AIRun,		7.0f,	NULL},
	{SG_AIRun,		7.0f,	NULL},
	{SG_AIRun,		6.0f,	NULL},
	{SG_AIRun,		6.0f,	Tank_FootstepSound}
};

static mframe_t			tank_stopRunFrames[] = {
	{SG_AIRun,		3.0f,	NULL},
	{SG_AIRun,		3.0f,	NULL},
	{SG_AIRun,		2.0f,	NULL},
	{SG_AIRun,		2.0f,	NULL},
	{SG_AIRun,		4.0f,	Tank_FootstepSound}
};

static mmove_t			tank_startRunMove = {FRAME_walk01, FRAME_walk04, tank_startRunFrames, Tank_RunAnimation};
static mmove_t			tank_runMove = {FRAME_walk05, FRAME_walk20, tank_runFrames, NULL};
static mmove_t			tank_stopRunMove = {FRAME_walk21, FRAME_walk25, tank_stopRunFrames, Tank_WalkAnimation};


/*
 ==================
 Tank_RunAnimation
 ==================
*/
static void Tank_RunAnimation (edict_t *self){

	if (self->enemy && self->enemy->client)
		self->monsterinfo.aiflags |= AI_BRUTAL;
	else
		self->monsterinfo.aiflags &= ~AI_BRUTAL;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND){
		self->monsterinfo.currentmove = &tank_standMove;
		return;
	}

	if (self->monsterinfo.currentmove == &tank_walkMove || self->monsterinfo.currentmove == &tank_startRunMove)
		self->monsterinfo.currentmove = &tank_runMove;
	else
		self->monsterinfo.currentmove = &tank_startRunMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Tank_Railgun
 ==================
*/
static void Tank_Railgun (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	end;
	vec3_t	dir;
	int		flashNumber;

	if (self->s.frame == FRAME_attak110)
		flashNumber = MZ2_TANK_BLASTER_1;
	else if (self->s.frame == FRAME_attak113)
		flashNumber = MZ2_TANK_BLASTER_2;
	else
		flashNumber = MZ2_TANK_BLASTER_3;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);

	SG_MonsterFireRailgun(self, start, dir, 50, 100, flashNumber);
}

/*
 ==================
 Tank_Blaster
 ==================
*/
static void Tank_Blaster (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	end;
	vec3_t	dir;
	int		flashNumber;

	if (self->s.frame == FRAME_attak110)
		flashNumber = MZ2_TANK_BLASTER_1;
	else if (self->s.frame == FRAME_attak113)
		flashNumber = MZ2_TANK_BLASTER_2;
	else
		flashNumber = MZ2_TANK_BLASTER_3;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract(end, start, dir);

	SG_MonsterFireBlaster(self, start, dir, 30, 800, flashNumber, EF_BLASTER);
}

/*
 ==================
 Tank_Rocket
 ==================
*/
static void Tank_Rocket (edict_t *self){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	int		flashNumber;

	if (self->s.frame == FRAME_attak324)
		flashNumber = MZ2_TANK_ROCKET_1;
	else if (self->s.frame == FRAME_attak327)
		flashNumber = MZ2_TANK_ROCKET_2;
	else
		flashNumber = MZ2_TANK_ROCKET_3;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

	VectorCopy(self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract(vec, start, dir);
	VectorNormalize(dir);

	SG_MonsterFireRocket(self, start, dir, 50, 550, flashNumber);
}

/*
 ==================
 Tank_MachineGun
 ==================
*/
static void Tank_MachineGun (edict_t *self){

	vec3_t	dir;
	vec3_t	vec;
	vec3_t	start;
	vec3_t	forward, right;
	int		flashNumber;

	flashNumber = MZ2_TANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak406);

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[flashNumber], forward, right, start);

	if (self->enemy){
		VectorCopy(self->enemy->s.origin, vec);
		vec[2] += self->enemy->viewheight;
		VectorSubtract(vec, start, vec);
		SG_VectorToAngles(vec, vec);
		dir[0] = vec[0];
	}
	else
		dir[0] = 0.0f;

	if (self->s.frame <= FRAME_attak415)
		dir[1] = self->s.angles[1] - 8 * (self->s.frame - FRAME_attak411);
	else
		dir[1] = self->s.angles[1] + 8 * (self->s.frame - FRAME_attak419);
	dir[2] = 0.0f;

	AngleVectors(dir, forward, NULL, NULL);

	SG_MonsterFireBullet(self, start, forward, 20, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flashNumber);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			tank_attackBlasterFrames[] = {
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,  -1.0f,   NULL},
	{SG_AICharge,  -2.0f,   NULL},
	{SG_AICharge,  -1.0f,   NULL},
	{SG_AICharge,  -1.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,   0.0f,   Tank_Blaster},
	{SG_AICharge,   0.0f,   NULL},
	{SG_AICharge,   0.0f,   NULL},
	{SG_AICharge,   0.0f,   Tank_Blaster},
	{SG_AICharge,   0.0f,   NULL},
	{SG_AICharge,   0.0f,   NULL},
	{SG_AICharge,   0.0f,   Tank_Blaster}
};

static mframe_t			tank_reattackBlasterFrames[] = {
	{SG_AICharge,   0.0f,   NULL},
	{SG_AICharge,   0.0f,   NULL},
	{SG_AICharge,   0.0f,   Tank_Blaster},
	{SG_AICharge,   0.0f,   NULL},
	{SG_AICharge,   0.0f,   NULL},
	{SG_AICharge,   0.0f,   Tank_Blaster}
};

static mframe_t			tank_attackPostBlasterFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,		3.0f,	NULL},
	{SG_AIMove,		2.0f,	NULL},
	{SG_AIMove,	   -2.0f,	Tank_FootstepSound}
};

static mframe_t			tank_attackStrikeFrames[] = {
	{SG_AIMove,		3.0f,   NULL},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,		1.0f,   NULL},
	{SG_AIMove,		6.0f,   NULL},
	{SG_AIMove,		7.0f,   NULL},
	{SG_AIMove,		9.0f,   Tank_FootstepSound},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,		1.0f,   NULL},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,		2.0f,   Tank_FootstepSound},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,	   -2.0f,   NULL},
	{SG_AIMove,	   -2.0f,   NULL},
	{SG_AIMove,		0.0f,   Tank_WindupSound},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   Tank_StrikeSound},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,	   -1.0f,   NULL},
	{SG_AIMove,	   -1.0f,   NULL},
	{SG_AIMove,	   -1.0f,   NULL},
	{SG_AIMove,	   -1.0f,   NULL},
	{SG_AIMove,	   -1.0f,   NULL},
	{SG_AIMove,	   -3.0f,   NULL},
	{SG_AIMove,	   -10.0f,  NULL},
	{SG_AIMove,	   -10.0f,  NULL},
	{SG_AIMove,	   -2.0f,   NULL},
	{SG_AIMove,	   -3.0f,   NULL},
	{SG_AIMove,	   -2.0f,   Tank_FootstepSound}
};

static mframe_t			tank_attackPreRocketFrames[] = {
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	1.0f,   NULL},
	{SG_AICharge,	2.0f,   NULL},
	{SG_AICharge,	7.0f,   NULL},
	{SG_AICharge,	7.0f,   NULL},
	{SG_AICharge,	7.0f,   Tank_FootstepSound},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,  -3.0f,   NULL}
};

static mframe_t			tank_attackFireRocketFrames[] = {
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   Tank_Rocket},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   Tank_Rocket},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,  -1.0f,   Tank_Rocket}
};

static mframe_t			tank_attackPostRocketFrames[] = {	
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,  -1.0f,   NULL},
	{SG_AICharge,  -1.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	2.0f,   NULL},
	{SG_AICharge,	3.0f,   NULL},
	{SG_AICharge,	4.0f,   NULL},
	{SG_AICharge,	2.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,  -9.0f,	NULL},
	{SG_AICharge,  -8.0f,	NULL},
	{SG_AICharge,  -7.0f,	NULL},
	{SG_AICharge,  -1.0f,	NULL},
	{SG_AICharge,  -1.0f,	Tank_FootstepSound},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			tank_attackChaingunFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   Tank_MachineGun},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL},
	{SG_AICharge,	0.0f,   NULL}
};

static mmove_t			tank_blasterAttackMove = {FRAME_attak101, FRAME_attak116, tank_attackBlasterFrames, Tank_Reattack_Blaster};
static mmove_t			tank_reattackBlasterMove = {FRAME_attak111, FRAME_attak116, tank_reattackBlasterFrames, Tank_Reattack_Blaster};
static mmove_t			tank_postBlasterAttackMove = {FRAME_attak117, FRAME_attak122, tank_attackPostBlasterFrames, Tank_RunAnimation};
static mmove_t			tank_strikeAttackMove = {FRAME_attak201, FRAME_attak238, tank_attackStrikeFrames, Tank_Poststrike};
static mmove_t			tank_preRocketAttackMove = {FRAME_attak301, FRAME_attak321, tank_attackPreRocketFrames, Tank_Doattack_Rocket};
static mmove_t			tank_fireRocketAttackMove = {FRAME_attak322, FRAME_attak330, tank_attackFireRocketFrames, Tank_Refire_Rocket};
static mmove_t			tank_postRocketAttackMove = {FRAME_attak331, FRAME_attak353, tank_attackPostRocketFrames, Tank_RunAnimation};
static mmove_t			tank_chaingunAttackMove = {FRAME_attak401, FRAME_attak429, tank_attackChaingunFrames, Tank_RunAnimation};


/*
 ==================
 Tank_Reattack_Blaster
 ==================
*/
static void Tank_Reattack_Blaster (edict_t *self){

	if (skill->value >= 2){
		if (SG_IsEntityVisible(self, self->enemy)){
			if (self->enemy->health > 0){
				if (random() <= 0.6f){
					self->monsterinfo.currentmove = &tank_reattackBlasterMove;
					return;
				}
			}
		}
	}

	self->monsterinfo.currentmove = &tank_postBlasterAttackMove;
}

/*
 ==================
 Tank_Poststrike
 ==================
*/
static void Tank_Poststrike (edict_t *self){

	self->enemy = NULL;
	Tank_RunAnimation(self);
}

/*
 ==================
 Tank_Refire_Rocket
 ==================
*/
static void Tank_Refire_Rocket (edict_t *self){

	// Only on hard or nightmare
	if (skill->value >= 2){
		if (self->enemy->health > 0){
			if (SG_IsEntityVisible(self, self->enemy)){
				if (random() <= 0.4f){
					self->monsterinfo.currentmove = &tank_fireRocketAttackMove;
					return;
				}
			}
		}
	}

	self->monsterinfo.currentmove = &tank_postRocketAttackMove;
}

/*
 ==================
 Tank_Doattack_Rocket
 ==================
*/
static void Tank_Doattack_Rocket (edict_t *self){

	self->monsterinfo.currentmove = &tank_fireRocketAttackMove;
}

/*
 ==================
 Tank_AttackAnimation
 ==================
*/
static void Tank_AttackAnimation (edict_t *self){

	vec3_t	vec;
	float	range;
	float	r;

	if (self->enemy->health < 0){
		self->monsterinfo.currentmove = &tank_strikeAttackMove;
		self->monsterinfo.aiflags &= ~AI_BRUTAL;
		return;
	}

	VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
	range = VectorLength(vec);

	r = random();

	if (range <= 125.0f){
		if (r < 0.4f)
			self->monsterinfo.currentmove = &tank_chaingunAttackMove;
		else 
			self->monsterinfo.currentmove = &tank_blasterAttackMove;
	}
	else if (range <= 250.0f){
		if (r < 0.5f)
			self->monsterinfo.currentmove = &tank_chaingunAttackMove;
		else
			self->monsterinfo.currentmove = &tank_blasterAttackMove;
	}
	else {
		if (r < 0.33f)
			self->monsterinfo.currentmove = &tank_chaingunAttackMove;
		else if (r < 0.66f){
			self->monsterinfo.currentmove = &tank_preRocketAttackMove;
			self->pain_debounce_time = level.time + 5.0f;	// No pain for a while
		}
		else
			self->monsterinfo.currentmove = &tank_blasterAttackMove;
	}
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			tank_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			tank_pain2Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			tank_pain3Frames[] = {
	{SG_AIMove,	   -7.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		3.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   Tank_FootstepSound}
};

static mmove_t			tank_pain1Move = {FRAME_pain101, FRAME_pain104, tank_pain1Frames, Tank_RunAnimation};
static mmove_t			tank_pain2Move = {FRAME_pain201, FRAME_pain205, tank_pain2Frames, Tank_RunAnimation};
static mmove_t			tank_pain3Move = {FRAME_pain301, FRAME_pain316, tank_pain3Frames, Tank_RunAnimation};


/*
 ==================
 Tank_PainAnimation
 ==================
*/
static void Tank_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (damage <= 10)
		return;

	if (level.time < self->pain_debounce_time)
		return;

	if (damage <= 30){
		if (random() > 0.2f)
			return;
	}

	// If hard or nightmare, don't go into pain while attacking
	if (skill->value >= 2){
		if ((self->s.frame >= FRAME_attak301) && (self->s.frame <= FRAME_attak330))
			return;
		if ((self->s.frame >= FRAME_attak101) && (self->s.frame <= FRAME_attak116))
			return;
	}

	self->pain_debounce_time = level.time + 3.0f;
	Tank_PainSound(self);

	if (skill->value == 3.0f)
		return;		// No pain anims in nightmare

	if (damage <= 30)
		self->monsterinfo.currentmove = &tank_pain1Move;
	else if (damage <= 60)
		self->monsterinfo.currentmove = &tank_pain2Move;
	else
		self->monsterinfo.currentmove = &tank_pain3Move;
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			tank_deathFrames[] = {
	{SG_AIMove,	   -7.0f,   NULL},
	{SG_AIMove,	   -2.0f,   NULL},
	{SG_AIMove,    -2.0f,   NULL},
	{SG_AIMove,		1.0f,   NULL},
	{SG_AIMove,		3.0f,   NULL},
	{SG_AIMove,		6.0f,   NULL},
	{SG_AIMove,		1.0f,   NULL},
	{SG_AIMove,		1.0f,   NULL},
	{SG_AIMove,		2.0f,   NULL},
	{SG_AIMove,     0.0f,   NULL},
	{SG_AIMove,     0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,	   -2.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,	   -3.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,	   -4.0f,   NULL},
	{SG_AIMove,	   -6.0f,   NULL},
	{SG_AIMove,	   -4.0f,   NULL},
	{SG_AIMove,	   -5.0f,   NULL},
	{SG_AIMove,	   -7.0f,   NULL},
	{SG_AIMove,	  -15.0f,   Tank_ThudSound},
	{SG_AIMove,	   -5.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL}
};

static mmove_t			tank_deathMove = {FRAME_death101, FRAME_death132, tank_deathFrames, Tank_Dead};


/*
 ==================
 Tank_Dead
 ==================
*/
static void Tank_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -16.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -0.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;

	gi.linkentity(self);
}

/*
 ==================
 Tank_DieAnimation
 ==================
*/
void Tank_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	// Check for gib
	if (self->health <= self->gib_health){
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1.0f, ATTN_NORM, 0.0f);

		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);

		ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);

		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// Regular death
	Tank_DieSound(self);

	self->deadflag = DEAD_DEAD;

	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &tank_deathMove;
}


/*
 ==============================================================================

 MONSTER_TANK

 ==============================================================================
*/


/*
 ==================
 SG_MonsterTank_Spawn
 ==================
*/
void SG_MonsterTank_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
	
	VectorSet(self->mins, -32.0f, -32.0f, -16.0f);
	VectorSet(self->maxs, 32.0f, 32.0f, 72.0f);	
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	tank.sightSound = gi.soundindex("tank/sight1.wav");
	tank.stepSound = gi.soundindex("tank/step.wav");
	tank.thudSound = gi.soundindex("tank/tnkdeth2.wav");
	tank.windupSound = gi.soundindex("tank/tnkatck4.wav");
	tank.idleSound = gi.soundindex("tank/tnkidle1.wav");
	tank.strikeSound = gi.soundindex("tank/tnkatck5.wav");
	tank.painSound = gi.soundindex("tank/tnkpain2.wav");
	tank.dieSound = gi.soundindex("tank/death.wav");

	gi.soundindex("tank/tnkatck1.wav");
	gi.soundindex("tank/tnkatk2a.wav");
	gi.soundindex("tank/tnkatk2b.wav");
	gi.soundindex("tank/tnkatk2c.wav");
	gi.soundindex("tank/tnkatk2d.wav");
	gi.soundindex("tank/tnkatk2e.wav");
	gi.soundindex("tank/tnkatck3.wav");

	self->mass = 500;

	if (!strcmp(self->className, "monster_tank_commander")){
		self->health = 1000;
		self->gib_health = -225;
	}
	else {
		self->health = 750;
		self->gib_health = -200;
	}

	self->pain = Tank_PainAnimation;
	self->die = Tank_DieAnimation;

	self->monsterinfo.stand = Tank_StandAnimation;
	self->monsterinfo.walk = Tank_WalkAnimation;
	self->monsterinfo.run = Tank_RunAnimation;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = Tank_AttackAnimation;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = Tank_SightSound;
	self->monsterinfo.idle = Tank_IdleSound;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &tank_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);

	if (strcmp(self->className, "monster_tank_commander") == 0)
		self->s.skinnum = 2;
}