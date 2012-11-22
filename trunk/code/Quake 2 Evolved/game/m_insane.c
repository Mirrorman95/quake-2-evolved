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
// m_insane.c - Insane marine monster
//


#include "g_local.h"
#include "m_insane.h"


typedef struct {
	int					fistSound;
	int					shakeSound;
	int					moanSound;
	int					screamSound[8];
} insane_t;

static insane_t			insane;

static void Insane_StandAnimation (edict_t *self);
static void Insane_WalkAnimation (edict_t *self);
static void Insane_RunAnimation (edict_t *self);
static void Insane_CrossAnimation (edict_t *self);
static void Insane_OnGround (edict_t *self);
static void Insane_CheckDown (edict_t *self);
static void Insane_CheckUp (edict_t *self);
static void Insane_Dead (edict_t *self);


/*
 ==============================================================================

 SOUNDS

 ==============================================================================
*/


/*
 ==================
 Insane_FistSound
 ==================
*/
static void Insane_FistSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, insane.fistSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Insane_ShakeSound
 ==================
*/
static void Insane_ShakeSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, insane.shakeSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Insane_MoanSound
 ==================
*/
static void Insane_MoanSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, insane.moanSound, 1.0f, ATTN_IDLE, 0.0f);
}

/*
 ==================
 Insane_ScreamSound
 ==================
*/
static void Insane_ScreamSound (edict_t *self){

	gi.sound(self, CHAN_VOICE, insane.screamSound[rand() % 8], 1.0f, ATTN_IDLE, 0.0f);
}


/*
 ==============================================================================

 CROSS ANIMATION

 ==============================================================================
*/

static mframe_t			insane_crossFrames[] = {
	{SG_AIMove,		0.0f,	Insane_MoanSound},
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

static mframe_t			insane_struggleCrossFrames[] = {
	{SG_AIMove,		0.0f,	Insane_ScreamSound},
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

static mmove_t			insane_crossMove = {FRAME_cross1, FRAME_cross15, insane_crossFrames, Insane_CrossAnimation};
static mmove_t			insane_struggleCrossMove = {FRAME_cross16, FRAME_cross30, insane_struggleCrossFrames, Insane_CrossAnimation};


/*
 ==================
 Insane_CrossAnimation
 ==================
*/
static void Insane_CrossAnimation (edict_t *self){

	if (random() < 0.8f)		
		self->monsterinfo.currentmove = &insane_crossMove;
	else
		self->monsterinfo.currentmove = &insane_struggleCrossMove;
}


/*
 ==============================================================================

 DOWN AND UP ANIMATION

 ==============================================================================
*/

static mframe_t			insane_downFrames[] = {
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
	{SG_AIMove,	   -1.7f,	NULL},
	{SG_AIMove,	   -1.6f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	Insane_FistSound},
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
	{SG_AIMove,		0.0f,	Insane_MoanSound},
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
	{SG_AIMove,		0.5f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -0.2f,	Insane_ScreamSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.2f,	NULL},
	{SG_AIMove,		0.4f,	NULL},
	{SG_AIMove,		0.6f,	NULL},
	{SG_AIMove,		0.8f,	NULL},
	{SG_AIMove,		0.7f,	NULL},
	{SG_AIMove,		0.0f,	Insane_CheckUp}
};

static mframe_t			insane_upToDownFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	Insane_MoanSound},
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
	{SG_AIMove,		2.7f,	NULL},
	{SG_AIMove,		4.1f,	NULL},
	{SG_AIMove,		6.0f,	NULL},
	{SG_AIMove,		7.6f,	NULL},
	{SG_AIMove,		3.6f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	Insane_FistSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	Insane_FistSound},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			insane_jumpDownFrames[] = {
	{SG_AIMove,		0.2f,	NULL},
	{SG_AIMove,	   11.5f,	NULL},
	{SG_AIMove,		5.1f,	NULL},
	{SG_AIMove,		7.1f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			insane_downToUpFrames[] = {
	{SG_AIMove,	   -0.7f,	NULL},
	{SG_AIMove,	   -1.2f,	NULL},
	{SG_AIMove,	   -1.5f,	NULL},
	{SG_AIMove,	   -4.5f,	NULL},
	{SG_AIMove,	   -3.5f,	NULL},
	{SG_AIMove,	   -0.2f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -1.3f,	NULL},
	{SG_AIMove,	   -3.0f,	NULL},
	{SG_AIMove,	   -2.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,	   -3.3f,	NULL},
	{SG_AIMove,	   -1.6f,	NULL},
	{SG_AIMove,	   -0.3f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mmove_t			insane_downMove = {FRAME_stand100, FRAME_stand160, insane_downFrames, Insane_OnGround};
static mmove_t			insane_upToDownMove = {FRAME_stand1, FRAME_stand40, insane_upToDownFrames, Insane_OnGround};
static mmove_t			insane_jumpDownMove = {FRAME_stand96, FRAME_stand100, insane_jumpDownFrames, Insane_OnGround};
static mmove_t			insane_downToUpMove = {FRAME_stand41, FRAME_stand59, insane_downToUpFrames, Insane_StandAnimation};


/*
 ==================
 Insane_OnGround
 ==================
*/
static void Insane_OnGround (edict_t *self){

	self->monsterinfo.currentmove = &insane_downMove;
}

/*
 ==================
 Insane_CheckDown
 ==================
*/
static void Insane_CheckDown (edict_t *self){

	// Always stand
	if (self->spawnflags & 32)
		return;

	if (random() < 0.3f){
		if (random() < 0.5f)
			self->monsterinfo.currentmove = &insane_upToDownMove;
		else
			self->monsterinfo.currentmove = &insane_jumpDownMove;
	}
}

/*
 ==================
 Insane_CheckUp
 ==================
*/
static void Insane_CheckUp (edict_t *self){

	// If Hold_Ground and Crawl are set
	if ((self->spawnflags & 4) && (self->spawnflags & 16))
		return;

	if (random() < 0.5f)
		self->monsterinfo.currentmove = &insane_downToUpMove;				

}


/*
 ==============================================================================

 STAND ANIMATION

 ==============================================================================
*/

static mframe_t			insane_standNormalFrames[] = {
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	NULL},
	{SG_AIStand,	0.0f,	Insane_CheckDown}
};

static mframe_t			insane_standInsaneFrames[] = {
	{SG_AIStand,	0.0f,	Insane_ShakeSound},
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
	{SG_AIStand,	0.0f,	Insane_CheckDown}
};

static mmove_t			insane_standNormalMove = {FRAME_stand60, FRAME_stand65, insane_standNormalFrames, Insane_StandAnimation};
static mmove_t			insane_standInsaneMove = {FRAME_stand65, FRAME_stand94, insane_standInsaneFrames, Insane_StandAnimation};


/*
 ==================
 Insane_StandAnimation
 ==================
*/
static void Insane_StandAnimation (edict_t *self){

	// If crucified
	if (self->spawnflags & 8){
		self->monsterinfo.currentmove = &insane_crossMove;
		self->monsterinfo.aiflags |= AI_STAND_GROUND;
	}
	// If Hold_Ground and Crawl are set
	else if ((self->spawnflags & 4) && (self->spawnflags & 16))
		self->monsterinfo.currentmove = &insane_downMove;
	else {
		if (random() < 0.5f)
			self->monsterinfo.currentmove = &insane_standNormalMove;
		else
			self->monsterinfo.currentmove = &insane_standInsaneMove;
	}
}


/*
 ==============================================================================

 WALK ANIMATION

 ==============================================================================
*/

static mframe_t			insane_crawlFrames[] = {
	{SG_AIWalk,		0.0f,	Insane_ScreamSound},
	{SG_AIWalk,		1.5f,	NULL},
	{SG_AIWalk,		2.1f,	NULL},
	{SG_AIWalk,		3.6f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		0.9f,	NULL},
	{SG_AIWalk,		3.0f,	NULL},
	{SG_AIWalk,		3.4f,	NULL},
	{SG_AIWalk,		2.4f,	NULL}
};

static mframe_t			insane_walkNormalFrames[] = {
	{SG_AIWalk,		0.0f,	Insane_ScreamSound},
	{SG_AIWalk,		2.5f,	NULL},
	{SG_AIWalk,		3.5f,	NULL},
	{SG_AIWalk,		1.7f,	NULL},
	{SG_AIWalk,		2.3f,	NULL},
	{SG_AIWalk,		2.4f,	NULL},
	{SG_AIWalk,		2.2f,	NULL},
	{SG_AIWalk,		4.2f,	NULL},
	{SG_AIWalk,		5.6f,	NULL},
	{SG_AIWalk,		3.3f,	NULL},
	{SG_AIWalk,		2.4f,	NULL},
	{SG_AIWalk,		0.9f,	NULL},
	{SG_AIWalk,		0.0f,	NULL}
};

static mframe_t			insane_walkInsaneFrames[] = {
	{SG_AIWalk,		0.0f,	Insane_ScreamSound},
	{SG_AIWalk,		3.4f,	NULL},
	{SG_AIWalk,		3.6f,	NULL},
	{SG_AIWalk,		2.9f,	NULL},
	{SG_AIWalk,		2.2f,	NULL},
	{SG_AIWalk,		2.6f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.7f,	NULL},
	{SG_AIWalk,		4.8f,	NULL},
	{SG_AIWalk,		5.3f,	NULL},
	{SG_AIWalk,		1.1f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		0.5f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		4.9f,	NULL},
	{SG_AIWalk,		6.7f,	NULL},
	{SG_AIWalk,		3.8f,	NULL},
	{SG_AIWalk,		2.0f,	NULL},
	{SG_AIWalk,		0.2f,	NULL},
	{SG_AIWalk,		0.0f,	NULL},
	{SG_AIWalk,		3.4f,	NULL},
	{SG_AIWalk,		6.4f,	NULL},
	{SG_AIWalk,		5.0f,	NULL},
	{SG_AIWalk,		1.8f,	NULL},
	{SG_AIWalk,		0.0f,	NULL}
};

static mmove_t			insane_crawlMove = {FRAME_crawl1, FRAME_crawl9, insane_crawlFrames, NULL};
static mmove_t			insane_walkNormalMove = {FRAME_walk27, FRAME_walk39, insane_walkNormalFrames, Insane_WalkAnimation};
static mmove_t			insane_walkInsaneMove = {FRAME_walk1, FRAME_walk26, insane_walkInsaneFrames, Insane_WalkAnimation};


/*
 ==================
 Insane_WalkAnimation
 ==================
*/
static void Insane_WalkAnimation (edict_t *self){

	// Hold Ground?
	if (self->spawnflags & 16){
		if (self->s.frame == FRAME_cr_pain10){
			self->monsterinfo.currentmove = &insane_downMove;
			return;
		}
	}

	if (self->spawnflags & 4)
		self->monsterinfo.currentmove = &insane_crawlMove;
	else {
		if (random() <= 0.5f)
			self->monsterinfo.currentmove = &insane_walkNormalMove;
		else
			self->monsterinfo.currentmove = &insane_walkInsaneMove;
	}
}


/*
 ==============================================================================

 RUN ANIMATION

 ==============================================================================
*/

static mmove_t			insane_runCrawlMove = {FRAME_crawl1, FRAME_crawl9, insane_crawlFrames, NULL};
static mmove_t			insane_runNormalMove = {FRAME_walk27, FRAME_walk39, insane_walkNormalFrames, Insane_RunAnimation};
static mmove_t			insane_runInsaneMove = {FRAME_walk1, FRAME_walk26, insane_walkInsaneFrames, Insane_RunAnimation};


/*
 ==================
 Insane_RunAnimation
 ==================
*/
static void Insane_RunAnimation (edict_t *self){

	// Hold Ground?
	if (self->spawnflags & 16){
		if (self->s.frame == FRAME_cr_pain10){
			self->monsterinfo.currentmove = &insane_downMove;
			return;
		}
	}

	// Crawling?
	if (self->spawnflags & 4)
		self->monsterinfo.currentmove = &insane_runCrawlMove;
	else {
		// Else, mix it up
		if (random() <= 0.5f)
			self->monsterinfo.currentmove = &insane_runNormalMove;
		else
			self->monsterinfo.currentmove = &insane_runInsaneMove;
	}
}


/*
 ==============================================================================

 PAIN ANIMATION

 ==============================================================================
*/

static mframe_t			insane_crawlPainFrames[] = {
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

static mframe_t			insane_standPainFrames[] = {
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

static mmove_t			insane_crawlPainMove = {FRAME_cr_pain2, FRAME_cr_pain10, insane_crawlPainFrames, Insane_RunAnimation};
static mmove_t			insane_standPainMove = {FRAME_st_pain2, FRAME_st_pain12, insane_standPainFrames, Insane_RunAnimation};


/*
 ==================
 Insane_PainAnimation
 ==================
*/
static void Insane_PainAnimation (edict_t *self, edict_t *other, float kick, int damage){

	int		r, l;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;

	r = 1 + (rand() & 1);

	if (self->health < 25)
		l = 25;
	else if (self->health < 50)
		l = 50;
	else if (self->health < 75)
		l = 75;
	else
		l = 100;

	gi.sound(self, CHAN_VOICE, gi.soundindex(va("player/male/pain%i_%i.wav", l, r)), 1.0f, ATTN_IDLE, 0.0f);

	if (skill->value == 3)
		return;		// No pain anims in nightmare

	// Don't go into pain frames if crucified.
	if (self->spawnflags & 8){
		self->monsterinfo.currentmove = &insane_struggleCrossMove;			
		return;
	}

	if  (((self->s.frame >= FRAME_crawl1) && (self->s.frame <= FRAME_crawl9)) || ((self->s.frame >= FRAME_stand99) && (self->s.frame <= FRAME_stand160)))
		self->monsterinfo.currentmove = &insane_crawlPainMove;
	else
		self->monsterinfo.currentmove = &insane_standPainMove;

}


/*
 ==============================================================================

 DEATH ANIMATION

 ==============================================================================
*/

static mframe_t			insane_crawlDeathFrames[] = {
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL},
	{SG_AIMove,		0.0f,	NULL}
};

static mframe_t			insane_standDeathFrames[] = {
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

static mmove_t			insane_crawlDeathMove = {FRAME_cr_death10, FRAME_cr_death16, insane_crawlDeathFrames, Insane_Dead};
static mmove_t			insane_standDeathMove = {FRAME_st_death2, FRAME_st_death18, insane_standDeathFrames, Insane_Dead};


/*
 ==================
 Insane_Dead
 ==================
*/
static void Insane_Dead (edict_t *self){

	if (self->spawnflags & 8)
		self->flags |= FL_FLY;
	else {
		VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
		VectorSet(self->maxs, 16.0f, 16.0f, -8.0f);
		self->movetype = MOVETYPE_TOSS;
	}

	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0.0f;

	gi.linkentity(self);
}

/*
 ==================
 Insane_DieAnimation
 ==================
*/
static void Insane_DieAnimation (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	if (self->health <= self->gib_health){
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1.0f, ATTN_IDLE, 0.0f);

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

	gi.sound(self, CHAN_VOICE, gi.soundindex(va("player/male/death%i.wav", (rand() % 4) + 1)), 1.0f, ATTN_IDLE, 0.0f);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	if (self->spawnflags & 8)
		Insane_Dead(self);
	else {
		if (((self->s.frame >= FRAME_crawl1) && (self->s.frame <= FRAME_crawl9)) || ((self->s.frame >= FRAME_stand99) && (self->s.frame <= FRAME_stand160)))		
			self->monsterinfo.currentmove = &insane_crawlDeathMove;
		else
			self->monsterinfo.currentmove = &insane_standDeathMove;
	}
}


/*
 ==============================================================================

 MONSTER_INSANE

 ==============================================================================
*/


/*
 ==================
 SG_MiscInsane_Spawn
 ==================
*/
void SG_MiscInsane_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/monsters/insane/tris.md2");

	VectorSet(self->mins, -16.0f, -16.0f, -24.0f);
	VectorSet(self->maxs, 16.0f, 16.0f, 32.0f);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	insane.fistSound = gi.soundindex("insane/insane11.wav");
	insane.shakeSound = gi.soundindex("insane/insane5.wav");
	insane.moanSound = gi.soundindex("insane/insane7.wav");
	insane.screamSound[0] = gi.soundindex("insane/insane1.wav");
	insane.screamSound[1] = gi.soundindex("insane/insane2.wav");
	insane.screamSound[2] = gi.soundindex("insane/insane3.wav");
	insane.screamSound[3] = gi.soundindex("insane/insane4.wav");
	insane.screamSound[4] = gi.soundindex("insane/insane6.wav");
	insane.screamSound[5] = gi.soundindex("insane/insane8.wav");
	insane.screamSound[6] = gi.soundindex("insane/insane9.wav");
	insane.screamSound[7] = gi.soundindex("insane/insane10.wav");

	self->mass = 300;

	self->health = 100;
	self->gib_health = -50;

	self->pain = Insane_PainAnimation;
	self->die = Insane_DieAnimation;

	self->monsterinfo.stand = Insane_StandAnimation;
	self->monsterinfo.walk = Insane_WalkAnimation;
	self->monsterinfo.run = Insane_RunAnimation;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = NULL;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = NULL;
	self->monsterinfo.aiflags |= AI_GOOD_GUY;

	gi.linkentity(self);

	// Stand Ground
	if (self->spawnflags & 16)
		self->monsterinfo.aiflags |= AI_STAND_GROUND;

	self->monsterinfo.currentmove = &insane_standNormalMove;
	
	self->monsterinfo.scale = MODEL_SCALE;

	// Crucified ?
	if (self->spawnflags & 8){
		VectorSet(self->mins, -16.0f, 0.0f, 0.0f);
		VectorSet(self->maxs, 16.0f, 8.0f, 32.0f);
		self->flags |= FL_NO_KNOCKBACK;
		SG_AirMonster(self);
	}
	else {
		SG_GroundMonster(self);
		self->s.skinnum = rand() % 3;
	}
}