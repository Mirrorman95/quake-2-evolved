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
// sg_utilities.c - Utility functions
//


#include "g_local.h"


#define MAX_CHOICES						8

static vec3_t			vec3_up = {0.0f, -1.0f, 0.0f};
static vec3_t			vec3_down = {0.0f, -2.0f, 0.0f};

static vec3_t			moveDir_up = {0.0f, 0.0f, 1.0f};
static vec3_t			moveDir_down = {0.0f, 0.0f, -1.0f};


/*
 ==================
 SG_ProjectSource

 The source origin from which to project
 ==================
*/
void SG_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result){

	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}

/*
 ==================
 SG_EntityRange

 Returns the range catagorization of an entity reletive to self

 0 melee range, will become hostile even if back is turned
 1 visibility and infront, or visibility and show hostile
 2 infront and show hostile
 3 only triggered by damage
 ==================
*/
int SG_EntityRange (edict_t *self, edict_t *other){

	vec3_t	v;
	float	length;

	VectorSubtract(self->s.origin, other->s.origin, v);
	length = VectorLength(v);

	if (length < MELEE_DISTANCE)
		return RANGE_MELEE;
	if (length < 500.0f)
		return RANGE_NEAR;
	if (length < 1000.0f)
		return RANGE_MID;

	return RANGE_FAR;
}

/*
 ==================
 SG_EntityDistance

 Returns the distance of the specified entity
 ==================
*/
float SG_EntityDistance (edict_t *self, edict_t *other){

	vec3_t	v;
	float	dist;

	VectorSubtract(self->s.origin, other->s.origin, v);
	dist = VectorLength(v);

	return dist;
}

/*
 ==================
 SG_IsEntityVisible

 Returns 1 if the entity is visible to self, even if not infront()
 ==================
*/
qboolean SG_IsEntityVisible (edict_t *self, edict_t *other){

	vec3_t	spot1;
	vec3_t	spot2;
	trace_t	trace;

	// Spectators cannot be visible
	if (deathmatch->value){
		if (self->enemy){
			if (self->enemy->client->pers.spectator)
				return false;
		}
	}

	VectorCopy(self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy(other->s.origin, spot2);
	spot2[2] += other->viewheight;

	// TODO: Do a opaque water check, only allow the monster to see it's target if the water volume is
	// translucent

	trace = gi.trace(spot1, vec3_origin, vec3_origin, spot2, self, MASK_OPAQUE);

	if (trace.fraction == 1.0f)
		return true;

	return false;
}

/*
 ==================
 SG_IsEntityInFront

 Returns 1 if the entity is in front (in sight) of self
 ==================
*/
qboolean SG_IsEntityInFront (edict_t *self, edict_t *other){

	vec3_t	forward;
	vec3_t	vec;
	float	dot;

	// Spectators cannot be visible
	if (deathmatch->value){
		if (self->enemy){
			if (self->enemy->client->pers.spectator)
				return false;
		}
	}

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);

	dot = DotProduct(vec, forward);
	if (dot > 0.3f)
		return true;

	return false;
}

/*
 ==================
 SG_IsInFOV
 ==================
*/
qboolean SG_IsInFOV (edict_t *self, edict_t *other, int degrees){

	vec3_t  forward;
	vec3_t  vec;
	float   dot, value;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);

	dot = DotProduct(vec, forward);
	value = 1.0f - (float)degrees / 180.0f;

	if (dot > value)
		return true;

	return false;
}

/*
 ==================
 SG_IsOnSameTeam
 ==================
*/
qboolean SG_IsOnSameTeam (edict_t *entity1, edict_t *entity2){

	char	entity1Team[512];
	char	entity2Team[512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy(entity1Team, SG_ClientTeam(entity1));
	strcpy(entity2Team, SG_ClientTeam(entity2));

	if (!strcmp(entity1Team, entity2Team))
		return true;

	return false;
}

/*
 ==================
 SG_SpawnEntity
 ==================
*/
void SG_SpawnEntity (edict_t *self, char *cmd){

	vec3_t		forward, right;
	vec3_t		offset;
	vec3_t		start, end;
	edict_t		*entity;
	trace_t		trace;

	// Compute muzzleflash origin
	AngleVectors(self->s.angles, forward, right, NULL);
	VectorSet(offset, 0.0f, 7.0f,  self->viewheight - 8);

	SG_ProjectSource(self->s.origin, offset, forward, right, start);

	// Spawn it
	entity = SG_AllocEntity();

	entity->className = cmd;

	SG_CallSpawn(entity);

	// Trace
	VectorMA(start, 96.0f, forward, end);

	trace = gi.trace(start, NULL, NULL, end, self, MASK_MONSTERSOLID);

	if (trace.fraction < 1.0f){
		if (trace.endpos[2] <= self->s.origin[2])
			trace.endpos[2] += abs(entity->mins[2]);	// Spawned below us
		else
			trace.endpos[2] -= entity->maxs[2];		// Spawned above
	}

	trace = gi.trace(trace.endpos, entity->mins, entity->maxs, trace.endpos, NULL, MASK_MONSTERSOLID);

	if (trace.contents & MASK_MONSTERSOLID){
		SG_FreeEntity(entity);
		return;
	}

	VectorCopy(trace.endpos, entity->s.origin);
	entity->s.angles[YAW] = self->s.angles[YAW];
}

/*
 ==================
 SG_FindEntity

 Searches all active entities for the next one that holds
 the matching string at fieldofs (use the FOFS() macro) in the structure.

 Searches beginning at the edict after from, or the beginning if NULL
 NULL will be returned if the end of the list is reached.
 ==================
*/
edict_t *SG_FindEntity (edict_t *from, int offset, char *match){

	char	*string;

	if (!from)
		from = g_edicts;
	else
		from++;

	for ( ; from < &g_edicts[globals.num_edicts]; from++){
		if (!from->inuse)
			continue;

		string = *(char **)((byte *)from + offset);

		if (!string)
			continue;

		if (!Q_stricmp(string, match))
			return from;
	}

	return NULL;
}

/*
 ==================
 SG_FindEntityWithinRadius

 Returns entities that have origins within a spherical area
 ==================
*/
edict_t *SG_FindEntityWithinRadius (edict_t *from, vec3_t origin, float radius){

	vec3_t	entityOrigin;

	if (!from)
		from = g_edicts;
	else
		from++;

	for ( ; from < &g_edicts[globals.num_edicts]; from++){
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;

		entityOrigin[0] = origin[0] - (from->s.origin[0] + (from->mins[0] + from->maxs[0]) * 0.5f);
		entityOrigin[1] = origin[1] - (from->s.origin[1] + (from->mins[1] + from->maxs[1]) * 0.5f);
		entityOrigin[2] = origin[2] - (from->s.origin[2] + (from->mins[2] + from->maxs[2]) * 0.5f);

		if (VectorLength(entityOrigin) > radius)
			continue;

		return from;
	}

	return NULL;
}

/*
 ==================
 SG_InitEntity
 ==================
*/
void SG_InitEntity (edict_t *entity){

	entity->inuse = true;
	entity->className = "noclass";
	entity->gravity = 1.0f;
	entity->s.number = entity - g_edicts;
}

/*
 ==================
 SG_AllocEntity

 Either finds a free entity, or allocates a new one.

 Try to avoid reusing an entity that was recently freed, because it can cause
 the clients to think the entity morphed into something else instead of being
 removed and recreated, which can cause interpolated angles and bad trails.
 ==================
*/
edict_t *SG_AllocEntity (){

	edict_t	*entity;
	int		i;

	entity = &g_edicts[(int)maxclients->value + 1];

	for (i = maxclients->value + 1; i < globals.num_edicts; i++, entity++){
		// The first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!entity->inuse && (entity->freetime < 2.0f || level.time - entity->freetime > 0.5f)){
			SG_InitEntity(entity);
			return entity;
		}
	}

	if (i == game.maxentities)
		gi.error("SG_AllocEntity: MAX_ENTITIES hit");

	globals.num_edicts++;

	SG_InitEntity(entity);

	return entity;
}

/*
 ==================
 SG_FreeEntity

 Marks the entity as free
 ==================
*/
void SG_FreeEntity (edict_t *entity){

	// Unlink from world
	gi.unlinkentity(entity);

	if ((entity - g_edicts) <= (maxclients->value + BODY_QUEUE_SIZE))
		return;

	memset(entity, 0, sizeof(*entity));

	entity->className = "freed";
	entity->freetime = level.time;
	entity->inuse = false;
}

/*
 ==================
 SG_PickTarget

 Searches all active entities for the next one that holds
 the matching string at fieldofs (use the FOFS() macro) in the structure.

 Searches beginning at the edict after from, or the beginning if NULL
 NULL will be returned if the end of the list is reached.
 ==================
*/
edict_t *SG_PickTarget (char *name){

	edict_t	*entity = NULL;
	edict_t	*choice[MAX_CHOICES];
	int		numChoices = 0;

	if (!name){
		gi.dprintf("SG_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while (1){
		entity = SG_FindEntity(entity, FOFS(targetname), name);
		if (!entity)
			break;

		choice[numChoices++] = entity;

		if (numChoices == MAX_CHOICES)
			break;
	}

	if (!numChoices){
		gi.dprintf("SG_PickTarget: target %s not found\n", name);
		return NULL;
	}

	return choice[rand() % numChoices];
}

/*
 ==================
 SG_UseTargetsDelay_Think
 ==================
*/
static void SG_UseTargetsDelay_Think (edict_t *entity){

	SG_UseTargets(entity, entity->activator);
	SG_FreeEntity(entity);
}

/*
 ==================
 SG_UseTargets

 The global "activator" should be set to the entity that initiated the firing.

 If self.delay is set, a DelayedUse entity will be created that will actually
 do the SUB_UseTargets after that many seconds have passed.

 Centerprints any self.message to the activator.

 Search for (string)targetname in all entities that
 match (string)self.target and call their .use function
 ==================
*/
void SG_UseTargets (edict_t *entity, edict_t *activator){

	edict_t	*t;

	// Check for a delay
	if (entity->delay){
		// Create a temporary object to fire at a later time
		t = SG_AllocEntity();

		t->className = "DelayedUse";
		t->nextthink = level.time + entity->delay;
		t->think = SG_UseTargetsDelay_Think;
		t->activator = activator;

		if (!activator)
			gi.dprintf("Think_Delay with no activator\n");

		t->message = entity->message;
		t->target = entity->target;
		t->killtarget = entity->killtarget;

		return;
	}


	// Print the message
	if ((entity->message) && !(activator->svflags & SVF_MONSTER)){
		gi.centerprintf(activator, "%s", entity->message);

		if (entity->noiseIndex)
			gi.sound(activator, CHAN_AUTO, entity->noiseIndex, 1.0f, ATTN_NORM, 0.0f);
		else
			gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1.0f, ATTN_NORM, 0.0f);
	}

	// Kill targets
	if (entity->killtarget){
		t = NULL;

		while ((t = SG_FindEntity(t, FOFS(targetname), entity->killtarget))){
			SG_FreeEntity(t);

			if (!entity->inuse){
				gi.dprintf("entity was removed while using killtargets\n");
				return;
			}
		}
	}

	// Fire targets
	if (entity->target){
		t = NULL;

		while ((t = SG_FindEntity(t, FOFS(targetname), entity->target))){
			// Doors fire area portals in a specific way
			if (!Q_stricmp(t->className, "func_areaportal") && (!Q_stricmp(entity->className, "func_door") || !Q_stricmp(entity->className, "func_door_rotating")))
				continue;

			if (t == entity)
				gi.dprintf("WARNING: Entity used itself.\n");
			else {
				if (t->use)
					t->use(t, entity, activator);
			}

			if (!entity->inuse){
				gi.dprintf("entity was removed while using targets\n");
				return;
			}
		}
	}
}

/*
 ==================
 SG_TouchTriggers
 ==================
*/
void SG_TouchTriggers (edict_t *entity){

	int		i, num;
	edict_t	*touch[MAX_EDICTS], *hit;

	// Dead things don't activate triggers!
	if ((entity->client || (entity->svflags & SVF_MONSTER)) && (entity->health <= 0))
		return;

	num = gi.BoxEdicts(entity->absmin, entity->absmax, touch, MAX_EDICTS, AREA_TRIGGERS);

	// Be careful, it is possible to have an entity in this
	// list removed before we get to it (kill triggered)
	for (i = 0; i < num; i++){
		hit = touch[i];

		if (!hit->inuse)
			continue;
		if (!hit->touch)
			continue;

		hit->touch(hit, entity, NULL, NULL);
	}
}

/*
 ==================
 SG_TouchSolids

 Call after linking a new trigger in during gameplay
 to force all entities it covers to immediately touch it
 ==================
*/
void SG_TouchSolids (edict_t *entity){

	int		i, num;
	edict_t	*touch[MAX_EDICTS], *hit;

	num = gi.BoxEdicts(entity->absmin, entity->absmax, touch, MAX_EDICTS, AREA_SOLID);

	// Be careful, it is possible to have an entity in this
	// list removed before we get to it (kill triggered)
	for (i = 0; i < num; i++){
		hit = touch[i];

		if (!hit->inuse)
			continue;
		if (entity->touch)
			entity->touch(hit, entity, NULL, NULL);
		if (!entity->inuse)
			break;
	}
}

/*
 ==================
 SG_KillBox

 Kills all entities that would touch the proposed new positioning of the entity
 Entity should be unlinked before calling this!
 ==================
*/
qboolean SG_KillBox (edict_t *entity){

	trace_t	trace;

	while (1){
		trace = gi.trace(entity->s.origin, entity->mins, entity->maxs, entity->s.origin, NULL, MASK_PLAYERSOLID);
		if (!trace.ent)
			break;

		// Nail it
		SG_TargetDamage(trace.ent, entity, entity, vec3_origin, entity->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

		// If we didn't kill it, fail
		if (trace.ent->solid)
			return false;
	}

	// All clear
	return true;
}

/*
 ==================
 SG_TempVector

 This is just a convenience function for making temporary
 vectors for function calls
 ==================
*/
float *SG_TempVector (float x, float y, float z){

	static	int		index;
	static	vec3_t	vecs[8];
	float			*v;

	// Use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1) & 7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}

/*
 ==================
 SG_VectorToString

 This is just a convenience function for printing vectors
 ==================
*/
char *SG_VectorToString (vec3_t vec){

	static	int		index;
	static	char	string[8][32];
	char			*s;

	// Use an array so that multiple vector to strings won't collide
	s = string[index];
	index = (index + 1) & 7;

	Com_sprintf(s, 32, "(%i %i %i)", (int)vec[0], (int)vec[1], (int)vec[2]);

	return s;
}

/*
 ==================
 SG_VectorToYaw
 ==================
*/
float SG_VectorToYaw (vec3_t vec){

	float	yaw;

	if (vec[PITCH] == 0.0f) {
		yaw = 0.0f;

		if (vec[YAW] > 0.0f)
			yaw = 90.0f;
		else if (vec[YAW] < 0)
			yaw = -90.0f;
	} 
	else {
		yaw = (int)(atan2(vec[YAW], vec[PITCH]) * 180.0f / M_PI);
		if (yaw < 0.0f)
			yaw += 360.0f;
	}

	return yaw;
}

/*
 ==================
 SG_VectorToAngles
 ==================
*/
void SG_VectorToAngles (vec3_t value1, vec3_t angles){

	float	forward;
	float	yaw, pitch;

	if (value1[1] == 0.0f && value1[0] == 0.0f){
		yaw = 0.0f;

		if (value1[2] > 0.0f)
			pitch = 90.0f;
		else
			pitch = 270.0f;
	}
	else {
		if (value1[0])
			yaw = (int) (atan2(value1[1], value1[0]) * 180.0f / M_PI);
		else if (value1[1] > 0.0f)
			yaw = 90.0f;
		else
			yaw = -90.0f;

		if (yaw < 0.0f)
			yaw += 360.0f;

		forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);

		pitch = (int)(atan2(value1[2], forward) * 180.0f / M_PI);
		if (pitch < 0)
			pitch += 360.0f;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0.0f;
}

/*
 ==================
 SG_SetMoveDirection
 ==================
*/
void SG_SetMoveDirection (vec3_t angles, vec3_t dir){

	if (VectorCompare(angles, vec3_up))
		VectorCopy(moveDir_up, dir);
	else if (VectorCompare (angles, vec3_down))
		VectorCopy(moveDir_down, dir);
	else
		AngleVectors(angles, dir, NULL, NULL);

	VectorClear(angles);
}

/*
 ==================
 SG_CopyString
 ==================
*/
char *SG_CopyString (char *in){

	char	*out;
	
	out = (char *)gi.TagMalloc(strlen(in) + 1, TAG_LEVEL);
	strcpy(out, in);

	return out;
}