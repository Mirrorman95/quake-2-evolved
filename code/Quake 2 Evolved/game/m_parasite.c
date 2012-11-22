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
// m_parasite.c - Parsite monster
//

// NOTE:
// - This monster has a "break" animation that isn't used


#include "g_local.h"
#include "m_parasite.h"


typedef struct {
	int					sightSound;
	int					pain1Sound;
	int					pain2Sound;
	int					dieSound;
	int					searchSound;
	int					launchSound;
	int					impactSound;
	int					suckSound;
	int					reelinSound;
	int					tapSound;
	int					scratchSound;
} parsite_t;

static parsite_t		parsite;

static void Parasite_DoFidgetAnimation (edict_t *self);
static void Parasite_ReFidgetAnimation (edict_t *self);
static void Parasite_StandAnimation (edict_t *self);
static void Parasite_WalkAnimation (edict_t *self);
static void Parasite_RunAnimation (edict_t *self);
static void Parasite_Dead (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Parasite_SightSound
 ==================
*/
static void Parasite_SightSound (edict_t *self, edict_t *other){

	gi.sound(self, CHAN_WEAPON, parsite.sightSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Parasite_Pain1Sound
 ==================
*/
static void Parasite_Pain1Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, parsite.pain1Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Parasite_Pain2Sound
 ==================
*/
static void Parasite_Pain2Sound (edict_t *self){

	gi.sound(self, CHAN_VOICE, parsite.pain2Sound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Parasite_DieSound
 ==================
*/
static void Parasite_DieSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, parsite.dieSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Parasite_SearchSound
 ==================
*/
static void Parasite_SearchSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, parsite.searchSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Parasite_LaunchSound
 ==================
*/
static void Parasite_LaunchSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, parsite.launchSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Parasite_ImpactSound
 ==================
*/
static void Parasite_ImpactSound (edict_t *self){

	gi.sound(self->enemy, CHAN_AUTO, parsite.impactSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Parasite_SuckSound
 ==================
*/
static void Parasite_SuckSound (edict_t *self){

	gi.sound (self, CHAN_WEAPON, parsite.suckSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Parasite_ReelInSound
 ==================
*/
static void Parasite_ReelInSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, parsite.reelinSound, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 Parasite_TapSound
 ==================
*/
static void Parasite_TapSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, parsite.tapSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Parasite_ScratchSound
 ==================
*/
static void Parasite_ScratchSound (edict_t *self){

	gi.sound(self, CHAN_WEAPON, parsite.scratchSound, 1.0f, ATTN_IDLE, 0.0f);
}


/*
 ==============================================================================

 FIDGET ANIMATION

 ==============================================================================
*/

static mframe_t			parasite_startFidgetFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL}
};

static mframe_t			parasite_fidgetFrames[] = {	
	{SG_AIStand,	0.0f,	Parasite_ScratchSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Parasite_ScratchSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL}
};

static mframe_t			parasite_endFidgetFrames[] = {
	{SG_AIStand,	0.0f,	Parasite_ScratchSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL}
};

static mmove_t			parasite_startFidgetMove = {FRAME_stand18, FRAME_stand21, parasite_startFidgetFrames, Parasite_DoFidgetAnimation};
static mmove_t			parasite_fidgetMove = {FRAME_stand22, FRAME_stand27, parasite_fidgetFrames, Parasite_ReFidgetAnimation};
static mmove_t			parasite_endFidgetMove = {FRAME_stand28, FRAME_stand35, parasite_endFidgetFrames, Parasite_StandAnimation};


/*
 ==================
 Parasite_EndFidgetAnimation
 ==================
*/
static void Parasite_EndFidgetAnimation (edict_t *self){

	self->monsterinfo.currentmove = &parasite_endFidgetMove;
}

/*
 ==================
 Parasite_DoFidgetAnimation
 ==================
*/
static void Parasite_DoFidgetAnimation (edict_t *self){

	self->monsterinfo.currentmove = &parasite_fidgetMove;
}

/*
 ==================
 Parasite_ReFidgetAnimation
 ==================
*/
static void Parasite_ReFidgetAnimation (edict_t *self){

	if (random() <= 0.8f)
		self->monsterinfo.currentmove = &parasite_fidgetMove;
	else
		self->monsterinfo.currentmove = &parasite_endFidgetMove;
}

/*
 ==================
 Parasite_IdleAnimation
 ==================
*/
static void Parasite_IdleAnimation (edict_t *self){

	self->monsterinfo.currentmove = &parasite_startFidgetMove;
}



/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			parasite_standFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Parasite_TapSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Parasite_TapSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Parasite_TapSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Parasite_TapSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Parasite_TapSound},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Parasite_TapSound}
};

static mmove_t			parasite_standMove = {FRAME_stand01, FRAME_stand17, parasite_standFrames, Parasite_StandAnimation};


/*
 ==================
 Parasite_StandAnimation
 ==================
*/
static void Parasite_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &parasite_standMove;
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			parasite_walkFrames[] = {
	{SG_AIWalk,	   30.0f,	NULL},
	{SG_AIWalk,	   30.0f,	NULL},
	{SG_AIWalk,	   22.0f,	NULL},
	{SG_AIWalk,	   19.0f,	NULL},
	{SG_AIWalk,	   24.0f,	NULL},
	{SG_AIWalk,	   28.0f,	NULL},
	{SG_AIWalk,	   25.0f,	NULL}
};

static mframe_t			parasite_startWalkFrames[] = {
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,	   30.0f,	Parasite_WalkAnimation}
};

static mframe_t			parasite_stopWalkFrames[] = {	
	{SG_AIWalk,	   20.0f,	NULL},
	{SG_AIWalk,	   20.0f,	NULL},
	{SG_AIWalk,	   12.0f,	NULL},
	{SG_AIWalk,	   10.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL}
};

static mmove_t			parasite_walkMove = {FRAME_run03, FRAME_run09, parasite_walkFrames, Parasite_WalkAnimation};
static mmove_t			parasite_startWalkMove = {FRAME_run01, FRAME_run02, parasite_startWalkFrames, NULL};
static mmove_t			parasite_stopWalkMove = {FRAME_run10, FRAME_run15, parasite_stopWalkFrames, NULL};


/*
 ==================
 Parasite_StartWalkAnimation
 ==================
*/
static void Parasite_StartWalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &parasite_startWalkMove;
}

/*
 ==================
 Parasite_WalkAnimation
 ==================
*/
static void Parasite_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &parasite_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			parasite_runFrames[] = {
	{SG_AIRun,	   30.0f,	NULL},
	{SG_AIRun,	   30.0f,	NULL},
	{SG_AIRun,	   22.0f,	NULL},
	{SG_AIRun,	   19.0f,	NULL},
	{SG_AIRun,	   24.0f,	NULL},
	{SG_AIRun,	   28.0f,	NULL},
	{SG_AIRun,	   25.0f,	NULL}
};

static mframe_t			parasite_startRunFrames[] = {
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,	   30.0f,	NULL},
};

static mframe_t			parasite_stopRunFrames[] = {	
	{SG_AIRun,	   20.0f,	NULL},
	{SG_AIRun,	   20.0f,	NULL},
	{SG_AIRun,	   12.0f,	NULL},
	{SG_AIRun,	   10.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL},
	{SG_AIRun,		0.0f,	NULL}
};

static mmove_t			parasite_runMove = {FRAME_run03, FRAME_run09, parasite_runFrames, NULL};
static mmove_t			parasite_startRunMove = {FRAME_run01, FRAME_run02, parasite_startRunFrames, Parasite_RunAnimation};
static mmove_t			parasite_stopRunMove = {FRAME_run10, FRAME_run15, parasite_stopRunFrames, NULL};


/*
 ==================
 Parasite_StartRunAnimation
 ==================
*/
static void Parasite_StartRunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &parasite_standMove;
	else
		self->monsterinfo.currentmove = &parasite_startRunMove;
}

/*
 ==================
 Parasite_RunAnimation
 ==================
*/
static void Parasite_RunAnimation (edict_t *self){

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &parasite_standMove;
	else
		self->monsterinfo.currentmove = &parasite_runMove;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Parasite_DrainAttackCheck
 ==================
*/
static qboolean Parasite_DrainAttackCheck (vec3_t start, vec3_t end){

	vec3_t	dir, angles;

	// Check for max distance
	VectorSubtract(start, end, dir);

	if (VectorLength(dir) > 256.0f)
		return false;

	// Check for min/max pitch
	SG_VectorToAngles(dir, angles);

	if (angles[0] < -180.0f)
		angles[0] += 360.0f;

	if (fabs(angles[0]) > 30.0f)
		return false;

	return true;
}

/*
 ==================
 Parasite_DrainAttack
 ==================
*/
static void Parasite_DrainAttack (edict_t *self){

	vec3_t	offset, start, f, r, end, dir;
	trace_t	trace;
	int		damage;

	AngleVectors(self->s.angles, f, r, NULL);
	VectorSet(offset, 24.0f, 0.0f, 6.0f);

	SG_ProjectSource(self->s.origin, offset, f, r, start);

	// Make sure we can do the attack
	VectorCopy(self->enemy->s.origin, end);

	if (!Parasite_DrainAttackCheck(start, end)){
		end[2] = self->enemy->s.origin[2] + self->enemy->maxs[2] - 8.0f;

		if (!Parasite_DrainAttackCheck(start, end)){
			end[2] = self->enemy->s.origin[2] + self->enemy->mins[2] + 8.0f;

			if (!Parasite_DrainAttackCheck(start, end))
				return;
		}
	}

	VectorCopy(self->enemy->s.origin, end);

	// Trace
	trace = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	if (trace.ent != self->enemy)
		return;

	// Attack
	if (self->s.frame == FRAME_drain03){
		damage = 5;
		Parasite_ImpactSound(self);
	}
	else {
		if (self->s.frame == FRAME_drain04)
			Parasite_SuckSound(self);

		damage = 2;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_PARASITE_ATTACK);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	VectorSubtract(start, end, dir);
	SG_TargetDamage(self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, damage, 0, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			parasite_drainFrames[] = {
	{SG_AICharge,	0.0f,	Parasite_LaunchSound},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,  15.0f,	Parasite_DrainAttack},
	{SG_AICharge,	0.0f,	Parasite_DrainAttack},
	{SG_AICharge,	0.0f,	Parasite_DrainAttack},
	{SG_AICharge,	0.0f,	Parasite_DrainAttack},
	{SG_AICharge,	0.0f,	Parasite_DrainAttack},
	{SG_AICharge,  -2.0f,   Parasite_DrainAttack},
	{SG_AICharge,  -2.0f,	Parasite_DrainAttack},
	{SG_AICharge,  -3.0f,	Parasite_DrainAttack},
	{SG_AICharge,  -2.0f,	Parasite_DrainAttack},
	{SG_AICharge,	0.0f,	Parasite_DrainAttack},
	{SG_AICharge,  -1.0f,   Parasite_DrainAttack},
	{SG_AICharge,	0.0f,	Parasite_ReelInSound},
	{SG_AICharge,  -2.0f,	NULL},
	{SG_AICharge,  -2.0f,	NULL},
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL}
};

static mframe_t			parasite_breakFrames[] = {
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	2.0f,	NULL},
	{SG_AICharge,  -3.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL},
	{SG_AICharge,	3.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge, -18.0f,	NULL},
	{SG_AICharge,	3.0f,	NULL},
	{SG_AICharge,	9.0f,	NULL},
	{SG_AICharge,	6.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge, -18.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	8.0f,	NULL},
	{SG_AICharge,	9.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge, -18.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	0.0f,	NULL},
	{SG_AICharge,	4.0f,	NULL},
	{SG_AICharge,  11.0f,	NULL},
	{SG_AICharge,  -2.0f,	NULL},
	{SG_AICharge,  -5.0f,	NULL},
	{SG_AICharge,	1.0f,	NULL}
};

static mmove_t			parasite_drainMove = {FRAME_drain01, FRAME_drain18, parasite_drainFrames, Parasite_StartRunAnimation};
static mmove_t			parasite_breakMove = {FRAME_break01, FRAME_break32, parasite_breakFrames, Parasite_StartRunAnimation};


/*
 ==================
 Parasite_AttackAnimation
 ==================
*/
static void Parasite_AttackAnimation (edict_t *self){

//	if (random() <= 0.2)
//		self->monsterinfo.currentmove = &parasite_breakMove;
//	else
		self->monsterinfo.currentmove = &parasite_drainMove;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			parasite_pain1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		6.0f,	NULL},
	{SG_AIMove,	   16.0f,	NULL},
	{SG_AIMove,	   -6.0f,	NULL},
	{SG_AIMove,	   -7.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			parasite_pain1Move = {FRAME_pain101, FRAME_pain111, parasite_pain1Frames, Parasite_StartRunAnimation};


/*
 ==================
 Parasite_PainAnimation
 ==================
*/
static void Parasite_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	if (random() < 0.5f)
		Parasite_Pain1Sound(self);
	else
		Parasite_Pain2Sound(self);

	self->monsterinfo.currentmove = &parasite_pain1Move;
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			parasite_deathFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			parasite_deathMove = {FRAME_death101, FRAME_death107, parasite_deathFrames, Parasite_Dead};


/*
 ==================
 Parasite_Dead
 ==================
*/
static void Parasite_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Parasite_DieAnimation
 ==================
*/
static void Parasite_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

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
	Parasite_DieSound(self);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &parasite_deathMove;
}


/*
 ==============================================================================

 MONSTER_PARSITE

 ==============================================================================
*/


/*
 ==================
 SG_MonsterParasite_Spawn
 ==================
*/
void SG_MonsterParasite_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/parasite/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 24.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	parsite.sightSound = gi.soundindex("parasite/parsght1.wav");
	parsite.pain1Sound = gi.soundindex ("parasite/parpain1.wav");	
	parsite.pain2Sound = gi.soundindex ("parasite/parpain2.wav");	
	parsite.dieSound = gi.soundindex ("parasite/pardeth1.wav");	
	parsite.searchSound = gi.soundindex("parasite/parsrch1.wav");
	parsite.launchSound = gi.soundindex("parasite/paratck1.wav");
	parsite.impactSound = gi.soundindex("parasite/paratck2.wav");
	parsite.suckSound = gi.soundindex("parasite/paratck3.wav");
	parsite.reelinSound = gi.soundindex("parasite/paratck4.wav");
	parsite.tapSound = gi.soundindex("parasite/paridle1.wav");
	parsite.scratchSound = gi.soundindex("parasite/paridle2.wav");

	self->mass = 250;

	self->health = 175;
	self->gib_health = -50;

	self->pain = Parasite_PainAnimation;
	self->die = Parasite_DieAnimation;

	self->monsterinfo.stand = Parasite_StandAnimation;
	self->monsterinfo.walk = Parasite_StartWalkAnimation;
	self->monsterinfo.run = Parasite_StartRunAnimation;
	self->monsterinfo.attack = Parasite_AttackAnimation;
	self->monsterinfo.sight = Parasite_SightSound;
	self->monsterinfo.idle = Parasite_IdleAnimation;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &parasite_standMove;	
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);
}