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
// sg_target.c - Target functions used in the map editor
//


#include "g_local.h"


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

void Actor_StandAnimation (edict_t *self);
void Actor_RunAnimation (edict_t *self);


/*
 ==============================================================================

 TARGET_TEMP_ENTITY

 ==============================================================================
*/


/*
 ==================
 SG_TargetTempEntity_Trigger
 ==================
*/
static void SG_TargetTempEntity_Trigger (edict_t *entity, edict_t *other, edict_t *activator){

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(entity->style);
	gi.WritePosition(entity->s.origin);
	gi.multicast(entity->s.origin, MULTICAST_PVS);
}

/*
 ==================
 SG_TargetTempEntity_Spawn
 ==================
*/
void SG_TargetTempEntity_Spawn (edict_t *entity){

	// Set the callback functions
	entity->use = SG_TargetTempEntity_Trigger;
}


/*
 ==============================================================================

 TARGET_SPEAKER

 ==============================================================================
*/


/*
 ==================
 SG_TargetSpeaker_Trigger
 ==================
*/
static void SG_TargetSpeaker_Trigger (edict_t *entity, edict_t *other, edict_t *activator){

	int		channel;

	// Looping sound toggles
	if (entity->spawnflags & 3){
		if (entity->s.sound)
			entity->s.sound = 0;	// Turn it off
		else
			entity->s.sound = entity->noiseIndex;	// Start it
	}
	else {
		// Normal sound
		if (entity->spawnflags & 4)
			channel = CHAN_VOICE|CHAN_RELIABLE;
		else
			channel = CHAN_VOICE;

		// Use a positioned_sound, because this entity won't normally be
		// sent to any clients because it is invisible
		gi.positioned_sound(entity->s.origin, entity, channel, entity->noiseIndex, entity->volume, entity->attenuation, 0.0f);
	}
}

/*
 ==================
 SG_TargetSpeaker_Spawn
 ==================
*/
void SG_TargetSpeaker_Spawn (edict_t *entity){

	char	buffer[MAX_QPATH];

	if (!st.noise){
		gi.dprintf("target_speaker with no noise set at %s\n", SG_VectorToString(entity->s.origin));
		return;
	}

	if (!strstr(st.noise, ".wav"))
		Com_sprintf(buffer, sizeof(buffer), "%s.wav", st.noise);
	else
		strncpy(buffer, st.noise, sizeof(buffer));

	entity->noiseIndex = gi.soundindex(buffer);

	if (!entity->volume)
		entity->volume = 1.0f;

	if (!entity->attenuation)
		entity->attenuation = 1.0f;
	else if (entity->attenuation == -1.0f)		// Yse -1 so 0 defaults to 1
		entity->attenuation = 0.0f;

	// Check for prestarted looping sound
	if (entity->spawnflags & 1)
		entity->s.sound = entity->noiseIndex;

	// Set the callback functions
	entity->use = SG_TargetSpeaker_Trigger;

	// Must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	gi.linkentity(entity);
}


/*
 ==============================================================================

 TARGET_EXPLOSION

 ==============================================================================
*/


/*
 ==================
 SG_TargetExplosion_Explode
 ==================
*/
static void SG_TargetExplosion_Explode (edict_t *self){

	float	save;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	SG_RadiusTargetDamage(self, self->activator, self->dmg, NULL, self->dmg + 40, MOD_EXPLOSIVE);

	save = self->delay;
	self->delay = 0.0f;
	SG_UseTargets(self, self->activator);
	self->delay = save;
}

/*
 ==================
 SG_TargetExplosion_Trigger
 ==================
*/
static void SG_TargetExplosion_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->activator = activator;

	if (!self->delay){
		SG_TargetExplosion_Explode (self);
		return;
	}

	self->think = SG_TargetExplosion_Explode;
	self->nextthink = level.time + self->delay;
}

/*
 ==================
 SG_TargetExplosion_Spawn
 ==================
*/
void SG_TargetExplosion_Spawn (edict_t *ent){

	// Set the callback functions
	ent->use = SG_TargetExplosion_Trigger;

	ent->svflags = SVF_NOCLIENT;
}


/*
 ==============================================================================

 TARGET_CHANGELEVEL

 ==============================================================================
*/


/*
 ==================
 SG_TargetChangeLevel_Trigger
 ==================
*/
static void SG_TargetChangeLevel_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	if (level.intermissiontime)
		return;		// Already activated

	if (!deathmatch->value && !coop->value){
		if (g_edicts[1].health <= 0)
			return;
	}

	// If noexit, do a ton of damage to other
	if (deathmatch->value && !((int)dmflags->value & DF_ALLOW_EXIT) && other != world){
		SG_TargetDamage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 10 * other->max_health, 1000, 0, MOD_EXIT);
		return;
	}

	// If multiplayer, let everyone know who hit the exit
	if (deathmatch->value){
		if (activator && activator->client)
			gi.bprintf(PRINT_HIGH, "%s exited the level.\n", activator->client->pers.netname);
	}

	// If going to a new unit, clear cross triggers
	if (strstr(self->map, "*"))	
		game.serverflags &= ~(SFL_CROSS_TRIGGER_MASK);

	SG_BeginIntermission(self);
}

/*
 ==================
 SG_TargetChangeLevel_Spawn
 ==================
*/
void SG_TargetChangeLevel_Spawn (edict_t *entity){

	if (!entity->map){
		gi.dprintf("target_changelevel with no map at %s\n", SG_VectorToString(entity->s.origin));
		SG_FreeEntity(entity);
		return;
	}

	// HACK: because *SOMEBODY* screwed up their map
	if((!Q_stricmp(level.mapname, "fact1")) && (!Q_stricmp(entity->map, "fact3")))
		entity->map = "fact3$secret1";

	entity->use = SG_TargetChangeLevel_Trigger;

	entity->svflags = SVF_NOCLIENT;
}


/*
 ==============================================================================

 TARGET_SECRET

 ==============================================================================
*/


/*
 ==================
 SG_TargetSecret_Trigger
 ==================
*/
static void SG_TargetSecret_Trigger (edict_t *entity, edict_t *other, edict_t *activator){

	gi.sound(entity, CHAN_VOICE, entity->noiseIndex, 1.0f, ATTN_NORM, 0.0f);

	level.found_secrets++;

	SG_UseTargets(entity, activator);
	SG_FreeEntity(entity);
}

/*
 ==================
 SG_TargetSecret_Spawn
 ==================
*/
void SG_TargetSecret_Spawn (edict_t *entity){

	// Auto-remove for deathmatch
	if (deathmatch->value){
		SG_FreeEntity(entity);
		return;
	}

	// Set the callback functions
	entity->use = SG_TargetSecret_Trigger;

	if (!st.noise)
		st.noise = "misc/secret.wav";

	entity->noiseIndex = gi.soundindex(st.noise);
	entity->svflags = SVF_NOCLIENT;

	level.total_secrets++;

	// HACK: Map bug hack
	if (!Q_stricmp(level.mapname, "mine3") && entity->s.origin[0] == 280.0f && entity->s.origin[1] == -2048.0f && entity->s.origin[2] == -624.0f)
		entity->message = "You have found a secret area.";
}


/*
 ==============================================================================

 TARGET_GOAL

 ==============================================================================
*/


/*
 ==================
 SG_TargetGoal_Trigger
 ==================
*/
static void SG_TargetGoal_Trigger (edict_t *entity, edict_t *other, edict_t *activator){

	gi.sound(entity, CHAN_VOICE, entity->noiseIndex, 1, ATTN_NORM, 0);

	level.found_goals++;

	if (level.found_goals == level.total_goals)
		gi.configstring(CS_CDTRACK, "0");

	SG_UseTargets(entity, activator);
	SG_FreeEntity(entity);
}

/*
 ==================
 SG_TargetGoal_Spawn
 ==================
*/
void SG_TargetGoal_Spawn (edict_t *entity){

	// Auto-remove for deathmatch
	if (deathmatch->value){
		SG_FreeEntity(entity);
		return;
	}

	// Set the callback functions
	entity->use = SG_TargetGoal_Trigger;

	if (!st.noise)
		st.noise = "misc/secret.wav";

	entity->noiseIndex = gi.soundindex(st.noise);
	entity->svflags = SVF_NOCLIENT;

	level.total_goals++;
}


/*
 ==============================================================================

 TARGET_SPLASH

 ==============================================================================
*/


/*
 ==================
 SG_TargetSplash_Trigger
 ==================
*/
static void SG_TargetSplash_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPLASH);
	gi.WriteByte(self->count);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(self->movedir);
	gi.WriteByte(self->sounds);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	if (self->dmg)
		SG_RadiusTargetDamage(self, activator, self->dmg, NULL, self->dmg+40, MOD_SPLASH);
}

/*
 ==================
 SG_TargetSplash_Spawn
 ==================
*/
void SG_TargetSplash_Spawn (edict_t *self){

	// Set the callback functions
	self->use = SG_TargetSplash_Trigger;

	SG_SetMoveDirection(self->s.angles, self->movedir);

	if (!self->count)
		self->count = 32;

	self->svflags = SVF_NOCLIENT;
}


/*
 ==============================================================================

 TARGET_SPAWNER

 ==============================================================================
*/


/*
 ==================
 SG_TargetSpawner_Trigger
 ==================
*/
void SG_TargetSpawner_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	edict_t	*entity;

	entity = SG_AllocEntity();

	entity->className = self->target;
	VectorCopy(self->s.origin, entity->s.origin);
	VectorCopy(self->s.angles, entity->s.angles);

	SG_CallSpawn(entity);

	gi.unlinkentity(entity);
	SG_KillBox(entity);

	gi.linkentity(entity);

	if (self->speed)
		VectorCopy(self->movedir, entity->velocity);
}

/*
 ==================
 SG_TargetSpawner_Spawn
 ==================
*/
void SG_TargetSpawner_Spawn (edict_t *self){

	// Set the callback functions
	self->use = SG_TargetSpawner_Trigger;

	self->svflags = SVF_NOCLIENT;

	if (self->speed){
		SG_SetMoveDirection(self->s.angles, self->movedir);
		VectorScale(self->movedir, self->speed, self->movedir);
	}
}


/*
 ==============================================================================

 TARGET_BLASTER

 ==============================================================================
*/


/*
 ==================
 SG_TargetBlaster_Trigger
 ==================
*/
static void SG_TargetBlaster_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	int		effect;

	if (self->spawnflags & 2)
		effect = 0;
	else if (self->spawnflags & 1)
		effect = EF_HYPERBLASTER;
	else
		effect = EF_BLASTER;

	SG_FireBlasterProjectile(self, self->s.origin, self->movedir, self->dmg, self->speed, EF_BLASTER, MOD_TARGET_BLASTER);
	
	gi.sound(self, CHAN_VOICE, self->noiseIndex, 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SG_TargetBlaster_Spawn
 ==================
*/
void SG_TargetBlaster_Spawn (edict_t *self){

	// Set the callback functions
	self->use = SG_TargetBlaster_Trigger;

	SG_SetMoveDirection(self->s.angles, self->movedir);
	
	self->noiseIndex = gi.soundindex("weapons/laser2.wav");

	if (!self->dmg)
		self->dmg = 15;
	if (!self->speed)
		self->speed = 1000.0f;

	self->svflags = SVF_NOCLIENT;
}


/*
 ==============================================================================

 CROSSLEVEL_TRIGGER

 ==============================================================================
*/


/*
 ==================
 SG_TargetCrossLevel_Trigger_Trigger
 ==================
*/
static void SG_TargetCrossLevel_Trigger_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	game.serverflags |= self->spawnflags;

	SG_FreeEntity(self);
}

/*
 ==================
 SG_TargetCrossLevel_Trigger_Spawn
 ==================
*/
void SG_TargetCrossLevel_Trigger_Spawn (edict_t *self){

	self->svflags = SVF_NOCLIENT;

	// Set the callback functions
	self->use = SG_TargetCrossLevel_Trigger_Trigger;
}


/*
 ==============================================================================

 CROSSLEVEL_TARGET

 ==============================================================================
*/


/*
 ==================
 SG_TargetCrossLevel_Target_Trigger
 ==================
*/
static void SG_TargetCrossLevel_Target_Trigger (edict_t *self){

	if (self->spawnflags == (game.serverflags & SFL_CROSS_TRIGGER_MASK & self->spawnflags)){
		SG_UseTargets(self, self);
		SG_FreeEntity(self);
	}
}

/*
 ==================
 SG_TargetCrossLevel_Target_Spawn
 ==================
*/
void SG_TargetCrossLevel_Target_Spawn (edict_t *self){

	if (!self->delay)
		self->delay = 1.0f;

	self->svflags = SVF_NOCLIENT;

	// Set the callback functions
	self->think = SG_TargetCrossLevel_Target_Trigger;

	self->nextthink = level.time + self->delay;
}


/*
 ==============================================================================

 TARGET_LASER

 ==============================================================================
*/


/*
 ==================
 SG_TargetLaser_Think
 ==================
*/
static void SG_TargetLaser_Think (edict_t *self){

	edict_t	*ignore;
	vec3_t	start;
	vec3_t	end;
	trace_t	trace;
	vec3_t	point;
	vec3_t	last_movedir;
	int		count;

	if (self->spawnflags & 0x80000000)
		count = 8;
	else
		count = 4;

	if (self->enemy){
		VectorCopy(self->movedir, last_movedir);
		VectorMA(self->enemy->absmin, 0.5f, self->enemy->size, point);
		VectorSubtract(point, self->s.origin, self->movedir);
		VectorNormalize(self->movedir);

		if (!VectorCompare(self->movedir, last_movedir))
			self->spawnflags |= 0x80000000;
	}

	ignore = self;
	VectorCopy(self->s.origin, start);
	VectorMA(start, 2048, self->movedir, end);

	while (1){
		trace = gi.trace(start, NULL, NULL, end, ignore, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

		if (!trace.ent)
			break;

		// Hurt it if we can
		if ((trace.ent->takedamage) && !(trace.ent->flags & FL_IMMUNE_LASER))
			SG_TargetDamage(trace.ent, self, self->activator, self->movedir, trace.endpos, vec3_origin, self->dmg, 1, DAMAGE_ENERGY, MOD_TARGET_LASER);

		// If we hit something that's not a monster or player or is immune to lasers, we're done
		if (!(trace.ent->svflags & SVF_MONSTER) && (!trace.ent->client)){
			if (self->spawnflags & 0x80000000){
				self->spawnflags &= ~0x80000000;
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_LASER_SPARKS);
				gi.WriteByte(count);
				gi.WritePosition(trace.endpos);
				gi.WriteDir(trace.plane.normal);
				gi.WriteByte(self->s.skinnum);
				gi.multicast(trace.endpos, MULTICAST_PVS);
			}

			break;
		}

		ignore = trace.ent;
		VectorCopy(trace.endpos, start);
	}

	VectorCopy(trace.endpos, self->s.old_origin);

	self->nextthink = level.time + FRAMETIME;
}

/*
 ==================
 SG_TargetLaser_On
 ==================
*/
static void SG_TargetLaser_On (edict_t *self){

	if (!self->activator)
		self->activator = self;

	self->spawnflags |= 0x80000001;
	self->svflags &= ~SVF_NOCLIENT;

	SG_TargetLaser_Think(self);
}

/*
 ==================
 SG_TargetLaser_Off
 ==================
*/
static void SG_TargetLaser_Off (edict_t *self){

	self->spawnflags &= ~1;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0.0f;
}

/*
 ==================
 SG_TargetLaser_Trigger
 ==================
*/
static void SG_TargetLaser_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->activator = activator;

	if (self->spawnflags & 1)
		SG_TargetLaser_Off(self);
	else
		SG_TargetLaser_On(self);
}

/*
 ==================
 SG_TargetLaser
 ==================
*/
static void SG_TargetLaser (edict_t *self){

	edict_t *entity;

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->s.renderfx |= RF_BEAM|RF_TRANSLUCENT;
	self->s.modelindex = 1;		// Must be non-zero

	// Set the beam diameter
	if (self->spawnflags & 64)
		self->s.frame = 16;
	else
		self->s.frame = 4;

	// Set the color
	if (self->spawnflags & 2)
		self->s.skinnum = 0xf2f2f0f0;
	else if (self->spawnflags & 4)
		self->s.skinnum = 0xd0d1d2d3;
	else if (self->spawnflags & 8)
		self->s.skinnum = 0xf3f3f1f1;
	else if (self->spawnflags & 16)
		self->s.skinnum = 0xdcdddedf;
	else if (self->spawnflags & 32)
		self->s.skinnum = 0xe0e1e2e3;

	if (!self->enemy){
		if (self->target){
			entity = SG_FindEntity (NULL, FOFS(targetname), self->target);

			if (!entity)
				gi.dprintf("%s at %s: %s is a bad target\n", self->className, SG_VectorToString(self->s.origin), self->target);

			self->enemy = entity;
		}
		else
			SG_SetMoveDirection(self->s.angles, self->movedir);
	}

	// Set the callback functions
	self->use = SG_TargetLaser_Trigger;
	self->think = SG_TargetLaser_Think;

	if (!self->dmg)
		self->dmg = 1;

	VectorSet(self->mins, -8.0f, -8.0f, -8.0f);
	VectorSet(self->maxs, 8.0f, 8.0f, 8.0f);

	gi.linkentity(self);

	if (self->spawnflags & 1)
		SG_TargetLaser_On(self);
	else
		SG_TargetLaser_Off(self);
}

/*
 ==================
 SG_TargetLaser_Spawn
 ==================
*/
void SG_TargetLaser_Spawn (edict_t *self){

	// Let everything else get spawned before we start firing
	self->think = SG_TargetLaser;

	self->nextthink = level.time + 1.0f;
}


/*
 ==============================================================================

 TARGET_HELP

 ==============================================================================
*/


/*
 ==================
 SG_TargetHelp_Trigger
 ==================
*/
static void SG_TargetHelp_Trigger (edict_t *entity, edict_t *other, edict_t *activator){

	if (entity->spawnflags & 1)
		strncpy(game.helpmessage1, entity->message, sizeof(game.helpmessage2) - 1);
	else
		strncpy(game.helpmessage2, entity->message, sizeof(game.helpmessage1) - 1);

	game.helpchanged++;
}

/*
 ==================
 SG_TargetHelp_Spawn
 ==================
*/
void SG_TargetHelp_Spawn (edict_t *entity){

	// Auto-remove for deathmatch
	if (deathmatch->value){
		SG_FreeEntity(entity);
		return;
	}

	if (!entity->message){
		gi.dprintf("%s with no message at %s\n", entity->className, SG_VectorToString(entity->s.origin));
		SG_FreeEntity(entity);
		return;
	}

	// Set the callback functions
	entity->use = SG_TargetHelp_Trigger;
}


/*
 ==============================================================================

 TARGET_ACTOR

 ==============================================================================
*/


/*
 ==================
 SG_TargetActor_Touch
 ==================
*/
static void SG_TargetActor_Touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surface){

	int		i;
	edict_t	*entity;
	vec3_t	v;
	char	*saveTarget;

	if (other->moveTarget != self)
		return;
	
	if (other->enemy)
		return;

	other->goalEntity = other->moveTarget = NULL;

	if (self->message){
		for (i = 1; i <= game.maxclients; i++){
			entity = &g_edicts[i];

			if (!entity->inuse)
				continue;

			gi.cprintf(entity, PRINT_CHAT, "%s: %s\n", actor_names[(other - g_edicts) % MAX_ACTOR_NAMES], self->message);
		}
	}

	// Jump
	if (self->spawnflags & 1){
		other->velocity[0] = self->movedir[0] * self->speed;
		other->velocity[1] = self->movedir[1] * self->speed;
		
		if (other->groundentity){
			other->groundentity = NULL;
			other->velocity[2] = self->movedir[2];

			gi.sound(other, CHAN_VOICE, gi.soundindex("player/male/jump1.wav"), 1.0f, ATTN_NORM, 0.0f);
		}
	}

	// Shoot
	if (self->spawnflags & 2){
	}
	else if (self->spawnflags & 4){
		// Attack
		other->enemy = SG_PickTarget(self->pathtarget);

		if (other->enemy){
			other->goalEntity = other->enemy;

			if (self->spawnflags & 32)
				other->monsterinfo.aiflags |= AI_BRUTAL;

			if (self->spawnflags & 16){
				other->monsterinfo.aiflags |= AI_STAND_GROUND;
				Actor_StandAnimation(other);
			}
			else
				Actor_RunAnimation(other);
		}
	}

	if (!(self->spawnflags & 6) && (self->pathtarget)){
		saveTarget = self->target;

		self->target = self->pathtarget;
		SG_UseTargets(self, other);
		self->target = saveTarget;
	}

	other->moveTarget = SG_PickTarget(self->target);

	if (!other->goalEntity)
		other->goalEntity = other->moveTarget;

	if (!other->moveTarget && !other->enemy){
		other->monsterinfo.pausetime = level.time + 100000000.0f;
		other->monsterinfo.stand(other);
	}
	else if (other->moveTarget == other->goalEntity){
		VectorSubtract(other->moveTarget->s.origin, other->s.origin, v);
		other->idealYaw = SG_VectorToYaw(v);
	}
}


/*
 ==================
 SG_TargetActor_Spawn
 ==================
*/
void SG_TargetActor_Spawn (edict_t *self){

	if (!self->targetname)
		gi.dprintf("%s with no targetname at %s\n", self->className, SG_VectorToString(self->s.origin));

	self->solid = SOLID_TRIGGER;
	self->touch = SG_TargetActor_Touch;
	VectorSet(self->mins, -8.0f, -8.0f, -8.0f);
	VectorSet(self->maxs, 8.0f, 8.0f, 8.0f);
	self->svflags = SVF_NOCLIENT;

	if (self->spawnflags & 1){
		if (!self->speed)
			self->speed = 200.0f;

		if (!st.height)
			st.height = 200;

		if (self->s.angles[YAW] == 0.0f)
			self->s.angles[YAW] = 360.0f;

		SG_SetMoveDirection (self->s.angles, self->movedir);
		self->movedir[2] = st.height;
	}

	gi.linkentity(self);
}


/*
 ==============================================================================

 TARGET_LIGHTRAMP

 ==============================================================================
*/


/*
 ==================
 SG_TargetLightRamp_Think
 ==================
*/
static void SG_TargetLightRamp_Think (edict_t *self){

	char	style[2];
	char	temp;

	style[0] = 'a' + self->movedir[0] + (level.time - self->timestamp) / FRAMETIME * self->movedir[2];
	style[1] = 0;

	gi.configstring(CS_LIGHTS+self->enemy->style, style);

	if ((level.time - self->timestamp) < self->speed)
		self->nextthink = level.time + FRAMETIME;
	else if (self->spawnflags & 1){
		temp = self->movedir[0];
		self->movedir[0] = self->movedir[1];
		self->movedir[1] = temp;
		self->movedir[2] *= -1.0f;
	}
}

/*
 ==================
 SG_TargetLightRamp_Trigger
 ==================
*/
static void SG_TargetLightRamp_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	edict_t	*entity;

	if (!self->enemy){
		// Check all the targets
		entity = NULL;

		while (1){
			entity = SG_FindEntity(entity, FOFS(targetname), self->target);

			if (!entity)
				break;

			if (strcmp(entity->className, "light") != 0){
				gi.dprintf("%s at %s ", self->className, SG_VectorToString(self->s.origin));
				gi.dprintf("target %s (%s at %s) is not a light\n", self->target, entity->className, SG_VectorToString(entity->s.origin));
			}
			else
				self->enemy = entity;
		}

		if (!self->enemy){
			gi.dprintf("%s target %s not found at %s\n", self->className, self->target, SG_VectorToString(self->s.origin));
			SG_FreeEntity(self);
			return;
		}
	}

	self->timestamp = level.time;
	SG_TargetLightRamp_Think(self);
}

/*
 ==================
 SG_TargetLightRamp_Spawn
 ==================
*/
void SG_TargetLightRamp_Spawn (edict_t *self){

	if (!self->message || strlen(self->message) != 2 || self->message[0] < 'a' || self->message[0] > 'z' || self->message[1] < 'a' || self->message[1] > 'z' || self->message[0] == self->message[1]){
		gi.dprintf("target_lightramp has bad ramp (%s) at %s\n", self->message, SG_VectorToString(self->s.origin));
		SG_FreeEntity(self);
		return;
	}

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	if (!self->target){
		gi.dprintf("%s with no target at %s\n", self->className, SG_VectorToString(self->s.origin));
		SG_FreeEntity(self);
		return;
	}

	self->svflags |= SVF_NOCLIENT;

	// Set the callback functions
	self->use = SG_TargetLightRamp_Trigger;
	self->think = SG_TargetLightRamp_Think;

	self->movedir[0] = self->message[0] - 'a';
	self->movedir[1] = self->message[1] - 'a';
	self->movedir[2] = (self->movedir[1] - self->movedir[0]) / (self->speed / FRAMETIME);
}


/*
 ==============================================================================

 TARGET_EARTHQUAKE

 ==============================================================================
*/


/*
 ==================
 SG_TargetEarthquake_Think
 ==================
*/
static void SG_TargetEarthquake_Think (edict_t *self){

	int		i;
	edict_t	*e;

	if (self->last_move_time < level.time){
		gi.positioned_sound(self->s.origin, self, CHAN_AUTO, self->noiseIndex, 1.0f, ATTN_NONE, 0.0f);
		self->last_move_time = level.time + 0.5f;
	}

	for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++){
		if (!e->inuse)
			continue;
		if (!e->client)
			continue;
		if (!e->groundentity)
			continue;

		e->groundentity = NULL;
		e->velocity[0] += crandom() * 150;
		e->velocity[1] += crandom() * 150;
		e->velocity[2] = self->speed * (100.0f / e->mass);
	}

	if (level.time < self->timestamp)
		self->nextthink = level.time + FRAMETIME;
}

/*
 ==================
 SG_TargetEarthquake_Trigger
 ==================
*/
static void SG_TargetEarthquake_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	self->timestamp = level.time + self->count;
	self->nextthink = level.time + FRAMETIME;
	self->activator = activator;
	self->last_move_time = 0.0f;
}

/*
 ==================
 SG_TargetEarthquake_Spawn
 ==================
*/
void SG_TargetEarthquake_Spawn (edict_t *self){

	if (!self->targetname)
		gi.dprintf("untargeted %s at %s\n", self->className, SG_VectorToString(self->s.origin));

	if (!self->count)
		self->count = 5;

	if (!self->speed)
		self->speed = 200.0f;

	self->svflags |= SVF_NOCLIENT;

	// Set the callback functions
	self->think = SG_TargetEarthquake_Think;
	self->use = SG_TargetEarthquake_Trigger;

	self->noiseIndex = gi.soundindex("world/quake.wav");
}


/*
 ==============================================================================

 TARGET_CHARACTER

 ==============================================================================
*/


/*
 ==================
 SG_TargetCharacter_Spawn
 ==================
*/
void SG_TargetCharacter_Spawn (edict_t *self){

	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->solid = SOLID_BSP;
	self->s.frame = 12;

	gi.linkentity(self);

	return;
}


/*
 ==============================================================================

 TARGET_STRING

 ==============================================================================
*/


/*
 ==================
 SG_TargetString_Trigger
 ==================
*/
static void SG_TargetString_Trigger (edict_t *self, edict_t *other, edict_t *activator){

	edict_t *entity;
	int		n, length;
	char	ch;

	length = strlen(self->message);

	for (entity = self->teammaster; entity; entity = entity->teamchain){
		if (!entity->count)
			continue;

		n = entity->count - 1;

		if (n > length){
			entity->s.frame = 12;
			continue;
		}

		ch = self->message[n];

		if (ch >= '0' && ch <= '9')
			entity->s.frame = ch - '0';
		else if (ch == '-')
			entity->s.frame = 10;
		else if (ch == ':')
			entity->s.frame = 11;
		else
			entity->s.frame = 12;
	}
}

/*
 ==================
 SG_TargetString_Spawn
 ==================
*/
void SG_TargetString_Spawn (edict_t *self){

	if (!self->message)
		self->message = "";

	self->use = SG_TargetString_Trigger;
}