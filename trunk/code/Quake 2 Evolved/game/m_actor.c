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
// m_actor.c - ???
//


#include "g_local.h"
#include "m_actor.h"

#define	MAX_ACTOR_NAMES		8

static char *actor_names[MAX_ACTOR_NAMES] = {
	"Hellrot",
	"Tokay",
	"Killme",
	"Disruptor",
	"Adrianator",
	"Rambear",
	"Titus",
	"Bitterman"
};

static char *messages[] = {
	"Watch it",
	"#$@*&",
	"Idiot",
	"Check your targets"
};

static void Actor_MachinegunAttack (edict_t *self);
static void Actor_Dead (edict_t *self);


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			actor_standFrames[] = {
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

static mmove_t			actor_standMove = {FRAME_stand101, FRAME_stand140, actor_standFrames, NULL};


/*
 ==================
 Actor_StandAnimation
 ==================
*/
void Actor_StandAnimation (edict_t *self){

	self->monsterinfo.currentmove = &actor_standMove;

	// Randomize on startup
	if (level.time < 1.0f)
		self->s.frame = self->monsterinfo.currentmove->firstframe + (rand() % (self->monsterinfo.currentmove->lastframe - self->monsterinfo.currentmove->firstframe + 1));
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			actor_walkFrames[] = {
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		6.0f,	NULL},
	{SG_AIWalk,    10.0f,	NULL},
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		7.0f,	NULL},
	{SG_AIWalk,    10.0f,	NULL},
	{SG_AIWalk,		1.0f,	NULL},
	{SG_AIWalk,		4.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL}
};

static mmove_t			actor_walkMove = {FRAME_walk01, FRAME_walk08, actor_walkFrames, NULL};


/*
 ==================
 Actor_WalkAnimation
 ==================
*/
static void Actor_WalkAnimation (edict_t *self){

	self->monsterinfo.currentmove = &actor_walkMove;
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mframe_t			actor_frames_run[] = {
	SG_AIRun, 4,  NULL,
	SG_AIRun, 15, NULL,
	SG_AIRun, 15, NULL,
	SG_AIRun, 8,  NULL,
	SG_AIRun, 20, NULL,
	SG_AIRun, 15, NULL,
	SG_AIRun, 8,  NULL,
	SG_AIRun, 17, NULL,
	SG_AIRun, 12, NULL,
	SG_AIRun, -2, NULL,
	SG_AIRun, -2, NULL,
	SG_AIRun, -1, NULL
};

static mmove_t			actor_move_run = {FRAME_run02, FRAME_run07, actor_frames_run, NULL};


/*
 ==================
 Actor_RunAnimation
 ==================
*/
void Actor_RunAnimation (edict_t *self){

	if ((level.time < self->pain_debounce_time) && (!self->enemy)){
		if (self->moveTarget)
			Actor_WalkAnimation(self);
		else
			Actor_StandAnimation(self);

		return;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND){
		Actor_StandAnimation(self);
		return;
	}

	self->monsterinfo.currentmove = &actor_move_run;
}


/*
 ==============================================================================

 WEAPON ATTACKS

 ==============================================================================
*/


/*
 ==================
 Actor_Machinegun
 ==================
*/
static void Actor_Machinegun (edict_t *self){

	vec3_t	start, target;
	vec3_t	forward, right;

	AngleVectors(self->s.angles, forward, right, NULL);
	SG_ProjectSource(self->s.origin, monster_flash_offset[MZ2_ACTOR_MACHINEGUN_1], forward, right, start);

	if (self->enemy){
		if (self->enemy->health > 0){
			VectorMA(self->enemy->s.origin, -0.2f, self->enemy->velocity, target);
			target[2] += self->enemy->viewheight;
		}
		else {
			VectorCopy(self->enemy->absmin, target);
			target[2] += (self->enemy->size[2] / 2.0f);
		}

		VectorSubtract(target, start, forward);
		VectorNormalize(forward);
	}
	else
		AngleVectors(self->s.angles, forward, NULL, NULL);

	SG_MonsterFireBullet(self, start, forward, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_ACTOR_MACHINEGUN_1);
}


/*
 ==============================================================================

 WEAPON ANIMATION

 ==============================================================================
*/

static mframe_t			actor_attackFrames[] = {
	{SG_AICharge,  -2.0f,	Actor_MachinegunAttack},
	{SG_AICharge,  -2.0f,	NULL},
	{SG_AICharge,	3.0f,   NULL},
	{SG_AICharge,	2.0f,   NULL}
};

static mmove_t			actor_attackMove = {FRAME_attak01, FRAME_attak04, actor_attackFrames, Actor_RunAnimation};


/*
 ==================
 Actor_MachinegunAttack
 ==================
*/
static void Actor_MachinegunAttack (edict_t *self){

	Actor_Machinegun(self);

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

/*
 ==================
 Actor_AttackAnimation
 ==================
*/
static void Actor_AttackAnimation (edict_t *self){

	int		n;

	self->monsterinfo.currentmove = &actor_attackMove;

	n = (rand() & 15) + 3 + 7;

	self->monsterinfo.pausetime = level.time + n * FRAMETIME;
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			actor_flipOffFrames[] = {
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL}
};

static mframe_t			actor_tauntFrames[] = {
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL},
	{SG_AITurn,		0.0f,	NULL}
};

static mframe_t			actor_pain1Frames[] = {
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL}
};

static mframe_t			actor_pain2Frames[] = {
	{SG_AIMove,    -4.0f,	NULL},
	{SG_AIMove,		4.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			actor_pain3Frames[] = {
	{SG_AIMove,    -1.0f,	NULL},
	{SG_AIMove,		1.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			actor_flipOffMove = {FRAME_flip01, FRAME_flip14, actor_flipOffFrames, Actor_RunAnimation};
static mmove_t			actor_tauntMove = {FRAME_taunt01, FRAME_taunt17, actor_tauntFrames, Actor_RunAnimation};
static mmove_t			actor_pain1Move = {FRAME_pain101, FRAME_pain103, actor_pain1Frames, Actor_RunAnimation};
static mmove_t			actor_pain2Move = {FRAME_pain201, FRAME_pain203, actor_pain2Frames, Actor_RunAnimation};
static mmove_t			actor_pain3Move = {FRAME_pain301, FRAME_pain303, actor_pain3Frames, Actor_RunAnimation};


/*
 ==================
 Actor_PainAnimation
 ==================
*/
static void Actor_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	vec3_t	v;
	char	*name;
	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	if ((other->client) && (random() < 0.4f)){
		VectorSubtract(other->s.origin, self->s.origin, v);

		self->idealYaw = SG_VectorToYaw(v);

		if (random() < 0.5f)
			self->monsterinfo.currentmove = &actor_flipOffMove;
		else
			self->monsterinfo.currentmove = &actor_tauntMove;

		name = actor_names[(self - g_edicts) % MAX_ACTOR_NAMES];

		gi.cprintf(other, PRINT_CHAT, "%s: %s!\n", name, messages[rand() % 3]);
		return;
	}

	n = rand() % 3;

	if (n == 0)
		self->monsterinfo.currentmove = &actor_pain1Move;
	else if (n == 1)
		self->monsterinfo.currentmove = &actor_pain2Move;
	else
		self->monsterinfo.currentmove = &actor_pain3Move;
}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			actor_death1Frames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,   -13.0f,	NULL},
	{SG_AIMove,    14.0f,	NULL},
	{SG_AIMove,		3.0f,   NULL},
	{SG_AIMove,    -2.0f,	NULL},
	{SG_AIMove,		1.0f,   NULL}
};

static mframe_t			actor_death2Frames[] = {
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,		7.0f,   NULL},
	{SG_AIMove,    -6.0f,	NULL},
	{SG_AIMove,    -5.0f,	NULL},
	{SG_AIMove,		1.0f,   NULL},
	{SG_AIMove,		0.0f,   NULL},
	{SG_AIMove,    -1.0f,	NULL},
	{SG_AIMove,    -2.0f,	NULL},
	{SG_AIMove,    -1.0f,	NULL},
	{SG_AIMove,    -9.0f,	NULL},
	{SG_AIMove,   -13.0f,	NULL},
	{SG_AIMove,   -13.0f,	NULL},
	{SG_AIMove,		0.0f,   NULL}
};

static mmove_t			actor_death1Move = {FRAME_death101, FRAME_death107, actor_death1Frames, Actor_Dead};
static mmove_t			actor_death2Move = {FRAME_death201, FRAME_death213, actor_death2Frames, Actor_Dead};


/*
 ==================
 Actor_Dead
 ==================
*/
static void Actor_Dead (edict_t *self){

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Actor_DieAnimation
 ==================
*/
static void Actor_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	int		n;

	// Check for gib
	if (self->health <= -80){
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

	if (n == 0)
		self->monsterinfo.currentmove = &actor_death1Move;
	else
		self->monsterinfo.currentmove = &actor_death2Move;
}

/*
 ==================
 Actor_Trigger
 ==================
*/
static void Actor_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	vec3_t		v;

	self->goalEntity = self->moveTarget = SG_PickTarget(self->target);

	if ((!self->moveTarget) || (strcmp(self->moveTarget->className, "target_actor") != 0)){
		gi.dprintf("%s has bad target %s at %s\n", self->className, self->target, SG_VectorToString(self->s.origin));

		self->target = NULL;
		self->monsterinfo.pausetime = 100000000.0f;
		self->monsterinfo.stand(self);
		return;
	}

	VectorSubtract(self->goalEntity->s.origin, self->s.origin, v);
	self->idealYaw = self->s.angles[YAW] = SG_VectorToYaw(v);
	self->monsterinfo.walk(self);
	self->target = NULL;
}


/*
 ==============================================================================

 MISC_ACTOR

 ==============================================================================
*/


/*
 ==================
 SG_MonsterActor_Spawn
 ==================
*/
void SG_MonsterActor_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	if (!self->targetname){
		gi.dprintf("untargeted %s at %s\n", self->className, SG_VectorToString(self->s.origin));
		SG_FreeEntity(self);
		return;
	}

	if (!self->target){
		gi.dprintf("%s with no target at %s\n", self->className, SG_VectorToString(self->s.origin));
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("players/male/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	self->mass = 200;

	if (!self->health)
		self->health = 100;

	self->pain = Actor_PainAnimation;
	self->die = Actor_DieAnimation;

	self->monsterinfo.stand = Actor_StandAnimation;
	self->monsterinfo.walk = Actor_WalkAnimation;
	self->monsterinfo.run = Actor_RunAnimation;
	self->monsterinfo.attack = Actor_AttackAnimation;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = NULL;

	self->monsterinfo.aiflags |= AI_GOOD_GUY;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &actor_standMove;
	self->monsterinfo.scale = MODEL_SCALE;

	SG_GroundMonster(self);

	// Actors always start in a dormant state, they *must* be used to get going
	self->use = Actor_Trigger;
}