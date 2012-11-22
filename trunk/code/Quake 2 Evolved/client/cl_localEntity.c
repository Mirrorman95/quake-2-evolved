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
// cl_localEntity.c - Client side local entities
//


#include "client.h"


#define MAX_LOCAL_ENTITIES			512

typedef enum {
	LE_FADE,
	LE_SCALE,
	LE_SCALE_FADE,
	LE_MOVE_FADE,
	LE_MOVE_SCALE,
	LE_MOVE_SCALE_FADE,
	LE_ENTITY,
	LE_EJECT_BRASS,
	LE_BLOOD,
	LE_WATER_WAKE,
	LE_NUKE_SHOCKWAVE,
} leType_t;

typedef enum {
	LE_STATIONARY					= BIT(0),
	LE_INWATER						= BIT(1),
	LE_BOUNCESOUND					= BIT(2),
	LE_LEAVEMARK					= BIT(3),
	LE_INSTANT						= BIT(4)
} leFlags_t;

typedef struct localEntity_s {
	struct					localEntity_s *prev, *next;
	leType_t				type;
	int						flags;

	int						startTime;
	int						endTime;

	vec3_t					origin;
	vec3_t					velocity;
	float					gravity;
	float					radius;
	color_t					color;

	float					bounceFactor;
	sound_t *				bounceSound;

	material_t *			markMaterial;
	material_t *			remapMaterial;

	float					light;
	vec3_t					lightColor;

	renderEntity_t			entity;
} localEntity_t;

static localEntity_t		cl_activeLocalEntities;
static localEntity_t *		cl_freeLocalEntities;
static localEntity_t		cl_localEntities[MAX_LOCAL_ENTITIES];

static byte					cl_colorPalette[256][3];


/*
 ==============================================================================

 ALLOCATING AND FREEING

 ==============================================================================
*/


/*
 ==================
 CL_FreeLocalEntity
 ==================
*/
static void CL_FreeLocalEntity (localEntity_t *le){

	if (!le->prev)
		return;

	le->prev->next = le->next;
	le->next->prev = le->prev;

	le->next = cl_freeLocalEntities;
	cl_freeLocalEntities = le;
}

/*
 ==================
 CL_AllocLocalEntity

 Will always succeed, even if it requires freeing an old active entity
 ==================
*/
static localEntity_t *CL_AllocLocalEntity (){

	localEntity_t	*le;

	if (!cl_freeLocalEntities)
		CL_FreeLocalEntity(cl_activeLocalEntities.prev);

	le = cl_freeLocalEntities;
	cl_freeLocalEntities = cl_freeLocalEntities->next;

	Mem_Fill(le, 0, sizeof(localEntity_t));

	le->next = cl_activeLocalEntities.next;
	le->prev = &cl_activeLocalEntities;
	cl_activeLocalEntities.next->prev = le;
	cl_activeLocalEntities.next = le;

	return le;
}


/*
 ==============================================================================

 THINK FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 CL_AddFade
 ==================
*/
static void CL_AddFade (localEntity_t *le){

	float	time, light;

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0f)
		time = 1.0f;

	le->entity.materialParms[MATERIALPARM_RED] = le->color[0] * time;
	le->entity.materialParms[MATERIALPARM_GREEN] = le->color[1] * time;
	le->entity.materialParms[MATERIALPARM_BLUE] = le->color[2] * time;
	le->entity.materialParms[MATERIALPARM_ALPHA] = le->color[3] * time;

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0f - light);

		CL_DynamicLight(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2], false, 0);
	}
}

/*
 ==================
 CL_AddScale
 ==================
*/
static void CL_AddScale (localEntity_t *le){

	float	time, light;

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0f)
		time = 1.0f;

	le->entity.spriteRadius = le->radius * (1.0f - time) + (le->radius * 0.75f);

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0f - light);

		CL_DynamicLight(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2], false, 0);
	}
}

/*
 ==================
 CL_AddScaleFade
 ==================
*/
static void CL_AddScaleFade (localEntity_t *le){

	float	time, light;

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0f)
		time = 1.0f;

	le->entity.spriteRadius = le->radius * (1.0f - time) + (le->radius * 0.75f);

	le->entity.materialParms[MATERIALPARM_RED] = le->color[0] * time;
	le->entity.materialParms[MATERIALPARM_GREEN] = le->color[1] * time;
	le->entity.materialParms[MATERIALPARM_BLUE] = le->color[2] * time;
	le->entity.materialParms[MATERIALPARM_ALPHA] = le->color[3] * time;

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0f - light);

		CL_DynamicLight(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2], false, 0);
	}
}

/*
 ==================
 CL_AddMoveFade
 ==================
*/
static void CL_AddMoveFade (localEntity_t *le){

	float	time, gravity, light;

	gravity = le->gravity * (cl.playerState->pmove.gravity / 800.0f);

	time = (cl.time - le->startTime) * 0.001f;

	le->entity.origin[0] = le->origin[0] + le->velocity[0] * time;
	le->entity.origin[1] = le->origin[1] + le->velocity[1] * time;
	le->entity.origin[2] = le->origin[2] + le->velocity[2] * time + (gravity * time * time);

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0f)
		time = 1.0f;

	le->entity.materialParms[MATERIALPARM_RED] = le->color[0] * time;
	le->entity.materialParms[MATERIALPARM_GREEN] = le->color[1] * time;
	le->entity.materialParms[MATERIALPARM_BLUE] = le->color[2] * time;
	le->entity.materialParms[MATERIALPARM_ALPHA] = le->color[3] * time;

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0f - light);

		CL_DynamicLight(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2], false, 0);
	}
}

/*
 ==================
 CL_AddMoveScale
 ==================
*/
static void CL_AddMoveScale (localEntity_t *le){

	float	time, gravity, light;

	gravity = le->gravity * (cl.playerState->pmove.gravity / 800.0f);

	time = (cl.time - le->startTime) * 0.001f;

	le->entity.origin[0] = le->origin[0] + le->velocity[0] * time;
	le->entity.origin[1] = le->origin[1] + le->velocity[1] * time;
	le->entity.origin[2] = le->origin[2] + le->velocity[2] * time + (gravity * time * time);

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0f)
		time = 1.0f;

	le->entity.spriteRadius = le->radius * (1.0f - time) + (le->radius * 0.75f);

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0f - light);

		CL_DynamicLight(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2], false, 0);
	}
}

/*
 ==================
 CL_AddMoveScaleFade
 ==================
*/
static void CL_AddMoveScaleFade (localEntity_t *le){

	float	time, gravity, light;

	gravity = le->gravity * (cl.playerState->pmove.gravity / 800.0f);

	time = (cl.time - le->startTime) * 0.001f;

	le->entity.origin[0] = le->origin[0] + le->velocity[0] * time;
	le->entity.origin[1] = le->origin[1] + le->velocity[1] * time;
	le->entity.origin[2] = le->origin[2] + le->velocity[2] * time + (gravity * time * time);

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0f)
		time = 1.0f;

	le->entity.spriteRadius = le->radius * (1.0f - time) + (le->radius * 0.75f);

	le->entity.materialParms[MATERIALPARM_RED] = le->color[0] * time;
	le->entity.materialParms[MATERIALPARM_GREEN] = le->color[1] * time;
	le->entity.materialParms[MATERIALPARM_BLUE] = le->color[2] * time;
	le->entity.materialParms[MATERIALPARM_ALPHA] = le->color[3] * time;

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0f - light);

		CL_DynamicLight(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2], false, 0);
	}
}

/*
 ==================
 CL_AddEntity
 ==================
*/
static void CL_AddEntity (localEntity_t *le){

	R_AddEntityToScene(&le->entity);

	if (le->flags & LE_INSTANT)
		CL_FreeLocalEntity(le);
}


/*
 ==============================================================================

 EFFECTS

 ==============================================================================
*/


/*
 ==================
 CL_AddEjectBrass
 ==================
*/
static void CL_AddEjectBrass (localEntity_t *le){

	float	time, gravity;
	vec3_t	origin, velocity;
	int		contents;
	vec3_t	mins, maxs;
	trace_t	trace;

	if (cl_brassTime->integerValue <= 0){
		CL_FreeLocalEntity(le);
		return;
	}

	if (le->flags & LE_STATIONARY){
		// Entity is stationary
		R_AddEntityToScene(&le->entity);
		return;
	}

	gravity = le->gravity * (cl.playerState->pmove.gravity / 800.0f);

	// Calculate origin
	time = (cl.time - le->startTime) * 0.001f;
	
	origin[0] = le->origin[0] + le->velocity[0] * time;
	origin[1] = le->origin[1] + le->velocity[1] * time;
	origin[2] = le->origin[2] + le->velocity[2] * time + (gravity * time * time);

	// Trace a line from previous origin to new origin
	R_ModelBounds(le->entity.model, mins, maxs);

	trace = CL_Trace(le->entity.origin, mins, maxs, origin, cl.clientNum, MASK_SOLID, true, NULL);
	if (trace.fraction != 0.0f && trace.fraction != 1.0f){
		// Reflect velocity
		time = cl.time - (cls.frameTime + cls.frameTime * trace.fraction) * 1000;
		time = (time - le->startTime) * 0.001f;

		VectorSet(velocity, le->velocity[0], le->velocity[1], le->velocity[2] + gravity * time);
		VectorReflect(velocity, trace.plane.normal, le->velocity);
		VectorScale(le->velocity, le->bounceFactor, le->velocity);

		// Check for stop
		if (trace.plane.normal[2] > 0 && le->velocity[2] < 1)
			le->flags |= LE_STATIONARY;

		// Reset
		le->startTime = cl.time;
		VectorCopy(trace.endpos, le->origin);

		// Play a bounce sound
		if (le->flags & LE_BOUNCESOUND){
			S_PlaySound(trace.endpos, 0, 0, le->bounceSound, 1.0f, ATTN_NORM, 0.0f);

			// Only play it once, otherwise it gets too noisy
			le->flags &= ~LE_BOUNCESOUND;
		}

		VectorCopy(trace.endpos, le->entity.origin);
		VectorCopy(trace.endpos, le->entity.oldOrigin);

		R_AddEntityToScene(&le->entity);
		return;
	}

	if (!(le->flags & LE_INWATER)){
		// If just entered a water volume, add friction
		contents = CL_PointContents(origin, -1);
		if (contents & MASK_WATER){
			if (contents & CONTENTS_WATER){
				VectorScale(le->velocity, 0.25f, le->velocity);
				le->gravity *= 0.25f;
			}
			if (contents & CONTENTS_SLIME){
				VectorScale(le->velocity, 0.20f, le->velocity);
				le->gravity *= 0.20f;
			}
			if (contents & CONTENTS_LAVA){
				VectorScale(le->velocity, 0.10f, le->velocity);
				le->gravity *= 0.10f;
			}

			// Don't check again later
			le->flags |= LE_INWATER;
		}
	}

	// Still in free fall
	VectorCopy(origin, le->entity.origin);
	VectorCopy(origin, le->entity.oldOrigin);

	R_AddEntityToScene(&le->entity);
}

/*
 ==================
 CL_AddBlood
 ==================
*/
static void CL_AddBlood (localEntity_t *le){

	float	time, gravity;
	vec3_t	origin;
	int		contents;
	trace_t	trace;

	if (!cl_blood->integerValue){
		CL_FreeLocalEntity(le);
		return;
	}

	gravity = le->gravity * (cl.playerState->pmove.gravity / 800.0f);

	// Calculate origin
	time = (cl.time - le->startTime) * 0.001f;
	
	origin[0] = le->origin[0] + le->velocity[0] * time;
	origin[1] = le->origin[1] + le->velocity[1] * time;
	origin[2] = le->origin[2] + le->velocity[2] * time + (gravity * time * time);

	// Calculate radius and color
	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0f)
		time = 1.0f;

	le->entity.spriteRadius = le->radius * (1.0f - time) + (le->radius * 0.75f);

	le->entity.materialParms[MATERIALPARM_RED] = le->color[0] * time;
	le->entity.materialParms[MATERIALPARM_GREEN] = le->color[1] * time;
	le->entity.materialParms[MATERIALPARM_BLUE] = le->color[2] * time;
	le->entity.materialParms[MATERIALPARM_ALPHA] = le->color[3] * time;

	// Trace a line from previous origin to new origin
	trace = CL_Trace(le->entity.origin, vec3_origin, vec3_origin, origin, cl.clientNum, MASK_SOLID, true, NULL);
	if (trace.fraction != 0.0f && trace.fraction != 1.0f){
		VectorCopy(trace.endpos, le->entity.origin);

		R_AddEntityToScene(&le->entity);

		// Add a decal if needed
		if (le->flags & LE_LEAVEMARK){
			R_ProjectDecalOntoWorld(trace.endpos, trace.plane.normal, rand() % 360, le->entity.spriteRadius * 2.5f, cl.time, le->markMaterial);

			// Only add it once, otherwise it gets too much overdraw
			le->flags &= ~LE_LEAVEMARK;
		}

		// No longer needed, so free it now
		CL_FreeLocalEntity(le);
		return;
	}

	// Still in free fall
	VectorCopy(origin, le->entity.origin);

	if (!(le->flags & LE_INWATER)){
		// If completely underwater, make a blood cloud
		origin[2] += le->entity.spriteRadius;

		contents = CL_PointContents(origin, -1);
		if (contents & MASK_WATER){
			if (contents & CONTENTS_WATER){
				VectorScale(le->velocity, 0.25f, le->velocity);
				le->gravity *= 0.125f;
			}
			if (contents & CONTENTS_SLIME){
				VectorScale(le->velocity, 0.20f, le->velocity);
				le->gravity *= 0.10f;
			}
			if (contents & CONTENTS_LAVA){
				VectorScale(le->velocity, 0.10f, le->velocity);
				le->gravity *= 0.05f;
			}

			le->startTime = cl.time;
			le->endTime = le->startTime + 750 + (rand() % 250);

			VectorCopy(le->entity.origin, le->origin);
			le->radius = le->entity.spriteRadius * 2.5f;
			le->entity.material = le->remapMaterial;

			// Don't leave marks underwater
			le->flags &= ~LE_LEAVEMARK;

			// Don't check again later
			le->flags |= LE_INWATER;
		}
	}

	R_AddEntityToScene(&le->entity);
}

/*
 ==================
 CL_AddWaterWake
 ==================
*/
static void CL_AddWaterWake (localEntity_t *le){

	float	time;

	// Calculate radius
	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0f)
		time = 1.0f;

	le->entity.spriteRadius = le->radius * (1.0f - time) + (le->radius * 0.75f);

	le->entity.materialParms[MATERIALPARM_RED] = le->color[0] * time;
	le->entity.materialParms[MATERIALPARM_GREEN] = le->color[1] * time;
	le->entity.materialParms[MATERIALPARM_BLUE] = le->color[2] * time;
	le->entity.materialParms[MATERIALPARM_ALPHA] = le->color[3] * time;

	R_AddEntityToScene(&le->entity);
}

/*
 ==================
 CL_AddNukeShockwave
 ==================
*/
static void CL_AddNukeShockwave (localEntity_t *le){

	float	time;

	// Calculate radius
	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0f)
		time = 1.0f;

	le->entity.spriteRadius = (cl.time - le->startTime) * 2.0f;

	le->entity.materialParms[MATERIALPARM_RED] = le->color[0] * time;
	le->entity.materialParms[MATERIALPARM_GREEN] = le->color[1] * time;
	le->entity.materialParms[MATERIALPARM_BLUE] = le->color[2] * time;
	le->entity.materialParms[MATERIALPARM_ALPHA] = le->color[3] * time;

	R_AddEntityToScene(&le->entity);
}

/*
 ==================
 CL_Explosion
 ==================
*/
void CL_Explosion (const vec3_t org, const vec3_t dir, float radius, float rotation, float light, float lightRed, float lightGreen, float lightBlue, material_t *material){

	localEntity_t	*le;

	le = CL_AllocLocalEntity();
	le->type = LE_SCALE_FADE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 1000;
	le->radius = radius;
	MakeRGBA(le->color, 255, 255, 255, 255);
	le->light = light;
	le->lightColor[0] = lightRed;
	le->lightColor[1] = lightGreen;
	le->lightColor[2] = lightBlue;

	le->entity.type = RE_SPRITE;
	VectorMA(org, radius * 0.5f, dir, le->entity.origin);
	le->entity.spriteRotation = rotation;
	le->entity.material = material;
	le->entity.materialParms[MATERIALPARM_TIMEOFFSET] = MS2SEC(le->startTime);
}

/*
 ==================
 CL_WaterSplash
 ==================
*/
void CL_WaterSplash (const vec3_t org, const vec3_t dir){

	localEntity_t	*le;

	// Plume
	le = CL_AllocLocalEntity();
	le->type = LE_FADE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 300;
	MakeRGBA(le->color, 255, 255, 255, 255);

	le->entity.type = RE_BEAM;
	VectorCopy(org, le->entity.origin);
	VectorMA(le->entity.origin, 15.0f, dir, le->entity.oldOrigin);
	le->entity.frame = 7.5;
	le->entity.oldFrame = 15;
	le->entity.material = cl.media.waterPlumeMaterial;

	// Spray
	le = CL_AllocLocalEntity();
	le->type = LE_SCALE_FADE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 400;
	le->radius = 6.0f + (3 * crand());
	MakeRGBA(le->color, 255, 255, 255, 255);

	le->entity.type = RE_SPRITE;
	VectorMA(org, 3.0f, dir, le->entity.origin);
	le->entity.spriteRotation = rand() % 360;
	le->entity.material = cl.media.waterSprayMaterial;

	// Wake
	le = CL_AllocLocalEntity();
	le->type = LE_WATER_WAKE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 1000;
	VectorCopy(org, le->origin);
	VectorCopy(dir, le->velocity);
	le->radius = 10.0f + (5 * crand());
	MakeRGBA(le->color, 255, 255, 255, 255);
}

/*
 ==================
 CL_ExplosionWaterSplash
 ==================
*/
void CL_ExplosionWaterSplash (const vec3_t org){

	localEntity_t	*le;
	vec3_t			above;
	trace_t			trace;

	// Make sure it is close enough to a water (or slime) surface
	VectorSet(above, org[0], org[1], org[2] + 80.0f);

	trace = CL_Trace(above, vec3_origin, vec3_origin, org, cl.clientNum, CONTENTS_WATER | CONTENTS_SLIME, true, NULL);
	if (trace.fraction == 0.0f || trace.fraction == 1.0f)
		return;

	// Spawn splash particles
	CL_SplashParticles(trace.endpos, trace.plane.normal, 768, 30.0f, 50.0f);

	// Plume
	le = CL_AllocLocalEntity();
	le->type = LE_FADE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 300;
	MakeRGBA(le->color, 255, 255, 255, 255);

	le->entity.type = RE_BEAM;
	VectorCopy(trace.endpos, le->entity.origin);
	VectorMA(le->entity.origin, 60.0f, trace.plane.normal, le->entity.oldOrigin);
	le->entity.frame = 80;
	le->entity.oldFrame = 60;
	le->entity.material = cl.media.waterPlumeMaterial;

	// Spray
	le = CL_AllocLocalEntity();
	le->type = LE_SCALE_FADE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 400;
	le->radius = 40.0f + (20 * crand());
	MakeRGBA(le->color, 255, 255, 255, 255);

	le->entity.type = RE_SPRITE;
	VectorMA(trace.endpos, 20.0f, trace.plane.normal, le->entity.origin);
	le->entity.spriteRotation = rand() % 360;
	le->entity.material = cl.media.waterSprayMaterial;

	// Wake
	le = CL_AllocLocalEntity();
	le->type = LE_WATER_WAKE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 1000;
	VectorCopy(trace.endpos, le->origin);
	VectorCopy(trace.plane.normal, le->velocity);
	le->radius = 40.0f + (20 * crand());
	MakeRGBA(le->color, 255, 255, 255, 255);
}

/*
 ==================
 CL_Sprite
 ==================
*/
void CL_Sprite (const vec3_t org, float radius, material_t *material){

	localEntity_t	*le;

	le = CL_AllocLocalEntity();
	le->type = LE_ENTITY;
	le->flags = LE_INSTANT;

	le->startTime = cl.time;
	le->endTime = le->startTime + 1;

	le->entity.type = RE_SPRITE;
	VectorCopy(org, le->entity.origin);
	le->entity.spriteRadius = radius;
	le->entity.material = material;
	MakeRGBA(le->entity.materialParms, 1.0f, 1.0f, 1.0f, 1.0f);
}

/*
 ==================
 CL_LaserBeam
 ==================
*/
void CL_LaserBeam (const vec3_t start, const vec3_t end, int width, int color, byte alpha, int duration, material_t *material){

	localEntity_t	*le;

	le = CL_AllocLocalEntity();
	le->type = LE_ENTITY;
	le->flags = (duration == 1) ? LE_INSTANT : 0;

	le->startTime = cl.time;
	le->endTime = le->startTime + duration;

	le->entity.type = RE_BEAM;
	VectorCopy(start, le->entity.origin);
	VectorCopy(end, le->entity.oldOrigin);
	le->entity.frame = width;
	le->entity.oldFrame = 50;
	le->entity.material = material;

	// HACK: the four beam colors are encoded in 32 bits
	color = (color >> ((rand() % 4) * 8)) & 0xFF;

	le->entity.materialParms[MATERIALPARM_RED] = cl_colorPalette[color][0] * (1.0f/255);
	le->entity.materialParms[MATERIALPARM_GREEN] = cl_colorPalette[color][1] * (1.0f/255);
	le->entity.materialParms[MATERIALPARM_BLUE] = cl_colorPalette[color][2] * (1.0f/255);
	le->entity.materialParms[MATERIALPARM_ALPHA] = alpha * (1.0f/255);
}

/*
 ==================
 CL_MachinegunEjectBrass
 ==================
*/
void CL_MachinegunEjectBrass (const centity_t *cent, int count, float x, float y, float z){

	localEntity_t	*le;
	vec3_t			velocity;
	vec3_t			angles, axis[3];
	int				i;

	if (cl_brassTime->integerValue <= 0)
		return;

	if (cent->current.number == cl.clientNum){
		if (cl_hand->integerValue == 2 && !cl_thirdPerson->integerValue)
			return;

		if (cl_hand->integerValue == 1 && !cl_thirdPerson->integerValue)
			y = -y;

		if (cl.playerState->pmove.pm_flags & PMF_DUCKED)
			z -= 24.0f;
	}

	AnglesToMat3(cent->current.angles, axis);

	for (i = 0; i < count; i++){
		le = CL_AllocLocalEntity();
		le->type = LE_EJECT_BRASS;
		le->flags = LE_BOUNCESOUND;

		le->startTime = cl.time;
		le->endTime = le->startTime + cl_brassTime->integerValue + (cl_brassTime->integerValue / 4) * frand();

		le->origin[0] = cent->current.origin[0] + (x * axis[0][0] + y * axis[1][0] + z * axis[2][0]);
		le->origin[1] = cent->current.origin[1] + (x * axis[0][1] + y * axis[1][1] + z * axis[2][1]);
		le->origin[2] = cent->current.origin[2] + (x * axis[0][2] + y * axis[1][2] + z * axis[2][2]);

		VectorSet(velocity, 20.0f * crand(), -50.0f + 40.0f * crand(), 100.0f + 50.0f * crand());

		if (cent->current.number == cl.clientNum){
			if (cl_hand->integerValue == 1 && !cl_thirdPerson->integerValue)
				velocity[1] = -velocity[1];
		}

		le->velocity[0] = velocity[0] * axis[0][0] + velocity[1] * axis[1][0] + velocity[2] * axis[2][0];
		le->velocity[1] = velocity[0] * axis[0][1] + velocity[1] * axis[1][1] + velocity[2] * axis[2][1];
		le->velocity[2] = velocity[0] * axis[0][2] + velocity[1] * axis[1][2] + velocity[2] * axis[2][2];

		le->gravity = -400.0f;

		le->bounceFactor = 1.2f;
		le->bounceSound = cl.media.sfxMachinegunBrass;

		angles[0] = cent->current.angles[0] + (rand() & 31);
		angles[1] = cent->current.angles[1] + (rand() & 31);
		angles[2] = cent->current.angles[2] + (rand() & 31);

		le->entity.type = RE_MODEL;
		le->entity.model = cl.media.machinegunBrassModel;
		VectorCopy(le->origin, le->entity.origin);
		AnglesToMat3(angles, le->entity.axis);
		MakeRGBA(le->entity.materialParms, 1.0f, 1.0f, 1.0f, 1.0f);
	}
}

/*
 ==================
 CL_ShotgunEjectBrass
 ==================
*/
void CL_ShotgunEjectBrass (const centity_t *cent, int count, float x, float y, float z){

	localEntity_t	*le;
	vec3_t			velocity;
	vec3_t			angles, axis[3];
	int				i;

	if (cl_brassTime->integerValue <= 0)
		return;

	if (cent->current.number == cl.clientNum){
		if (cl_hand->integerValue == 2 && !cl_thirdPerson->integerValue)
			return;

		if (cl_hand->integerValue == 1 && !cl_thirdPerson->integerValue)
			y = -y;

		if (cl.playerState->pmove.pm_flags & PMF_DUCKED)
			z -= 24.0f;
	}

	AnglesToMat3(cent->current.angles, axis);

	for (i = 0; i < count; i++){
		le = CL_AllocLocalEntity();
		le->type = LE_EJECT_BRASS;
		le->flags = LE_BOUNCESOUND;

		le->startTime = cl.time;
		le->endTime = le->startTime + cl_brassTime->integerValue + (cl_brassTime->integerValue / 4) * frand();

		le->origin[0] = cent->current.origin[0] + (x * axis[0][0] + y * axis[1][0] + z * axis[2][0]);
		le->origin[1] = cent->current.origin[1] + (x * axis[0][1] + y * axis[1][1] + z * axis[2][1]);
		le->origin[2] = cent->current.origin[2] + (x * axis[0][2] + y * axis[1][2] + z * axis[2][2]);

		VectorSet(velocity, 60.0f + 60.0f * crand(), 10.0f * crand(), 100.0f + 50.0f * crand());

		if (count == 2){
			if (i == 0)
				velocity[1] += 40.0f;
			else
				velocity[1] -= 40.0f;
		}

		if (cent->current.number == cl.clientNum){
			if (cl_hand->integerValue == 1 && !cl_thirdPerson->integerValue)
				velocity[1] = -velocity[1];
		}

		le->velocity[0] = velocity[0] * axis[0][0] + velocity[1] * axis[1][0] + velocity[2] * axis[2][0];
		le->velocity[1] = velocity[0] * axis[0][1] + velocity[1] * axis[1][1] + velocity[2] * axis[2][1];
		le->velocity[2] = velocity[0] * axis[0][2] + velocity[1] * axis[1][2] + velocity[2] * axis[2][2];

		le->gravity = -300.0f;

		le->bounceFactor = 0.8f;
		le->bounceSound = cl.media.sfxShotgunBrass;

		angles[0] = cent->current.angles[0] + (rand() & 31);
		angles[1] = cent->current.angles[1] + (rand() & 31);
		angles[2] = cent->current.angles[2] + (rand() & 31);

		le->entity.type = RE_MODEL;
		le->entity.model = cl.media.shotgunBrassModel;
		VectorCopy(le->origin, le->entity.origin);
		AnglesToMat3(angles, le->entity.axis);
		MakeRGBA(le->entity.materialParms, 1.0f, 1.0f, 1.0f, 1.0f);
	}
}

/*
 ==================
 CL_Bleed
 ==================
*/
void CL_Bleed (const vec3_t org, const vec3_t dir, int count, bool green){

	localEntity_t	*le;
	int				type = green & 1;
	int				i;

	if (!cl_blood->integerValue)
		return;

	for (i = 0; i < count; i++){
		le = CL_AllocLocalEntity();
		le->type = LE_BLOOD;

		le->startTime = cl.time;
		le->endTime = le->startTime + 750 + (rand() % 250);

		le->origin[0] = org[0] + crand() * 3;
		le->origin[1] = org[1] + crand() * 3;
		le->origin[2] = org[2] + crand() * 3;
		le->velocity[0] = dir[0] * 30 + crand() * 15;
		le->velocity[1] = dir[1] * 30 + crand() * 15;
		le->velocity[2] = dir[2] * 10 + crand() * 5;

		le->gravity = -300.0f;
		le->radius = 6.0f + (6 * crand());
		MakeRGBA(le->color, 255, 255, 255, 255);
		le->remapMaterial = cl.media.bloodCloudMaterial[type];

		if (!(i % 4)){
			le->flags |= LE_LEAVEMARK;
			le->markMaterial = cl.media.bloodMarkMaterials[type][rand() % 6];
		}

		le->entity.type = RE_SPRITE;
		VectorCopy(le->origin, le->entity.origin);
		le->entity.spriteRotation = rand() % 360;
		le->entity.material = cl.media.bloodSplatMaterial[type];
	}
}

/*
 ==================
 CL_BloodTrail
 ==================
*/
void CL_BloodTrail (const vec3_t start, const vec3_t end, bool green){

	localEntity_t	*le;
	vec3_t			move, vec;
	float			length, dec;
	int				type = green & 1;
	int				i;

	if (!cl_blood->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dec = 10.0f + (20 * type);
	VectorScale(vec, dec, vec);

	i = 0;

	while (length > 0.0f){
		length -= dec;
		i++;

		le = CL_AllocLocalEntity();
		le->type = LE_BLOOD;

		le->startTime = cl.time;
		le->endTime = le->startTime + 750 + (rand() % 250);

		le->origin[0] = move[0] + crand() * 3;
		le->origin[1] = move[1] + crand() * 3;
		le->origin[2] = move[2] + crand() * 3;
		le->velocity[0] = vec[0] * 30 + crand() * 15;
		le->velocity[1] = vec[1] * 30 + crand() * 15;
		le->velocity[2] = vec[2] * 10 + crand() * 5;

		le->gravity = -300.0f;
		le->radius = 6.0f + (3 * crand());
		MakeRGBA(le->color, 255, 255, 255, 255);
		le->remapMaterial = cl.media.bloodCloudMaterial[type];

		if (!(i % 2)){
			le->flags |= LE_LEAVEMARK;
			le->markMaterial = cl.media.bloodMarkMaterials[type][rand() % 6];
		}

		le->entity.type = RE_SPRITE;
		VectorCopy(le->origin, le->entity.origin);
		le->entity.spriteRotation = rand() % 360;
		le->entity.material = cl.media.bloodSplatMaterial[type];

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_NukeShockwave
 ==================
*/
void CL_NukeShockwave (const vec3_t org){

	localEntity_t	*le;

	le = CL_AllocLocalEntity();
	le->type = LE_NUKE_SHOCKWAVE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 500;

	VectorSet(le->origin, org[0], org[1], org[2] + 30.0f);
	MakeRGBA(le->color, 255, 255, 255, 255);
}


/*
 ==============================================================================

 ADD ENTITY TO SCENE

 ==============================================================================
*/


/*
 ==================
 CL_ClearLocalEntities
 ==================
*/
void CL_ClearLocalEntities (){

	int		i;
	byte	palette[] = {
#include "../renderer/palette.h"
	};

	Mem_Fill(cl_localEntities, 0, sizeof(cl_localEntities));

	cl_activeLocalEntities.next = &cl_activeLocalEntities;
	cl_activeLocalEntities.prev = &cl_activeLocalEntities;
	cl_freeLocalEntities = cl_localEntities;

	for (i = 0; i < MAX_LOCAL_ENTITIES - 1; i++)
		cl_localEntities[i].next = &cl_localEntities[i+1];

	for (i = 0; i < 256; i++){
		cl_colorPalette[i][0] = palette[i*3+0];
		cl_colorPalette[i][1] = palette[i*3+1];
		cl_colorPalette[i][2] = palette[i*3+2];
	}
}

/*
 ==================
 CL_AddLocalEntities
 ==================
*/
void CL_AddLocalEntities (){

	localEntity_t	*le, *prev;

	for (le = cl_activeLocalEntities.prev; le != &cl_activeLocalEntities; le = prev){
		// Grab prev now, so if the entity is freed we still have it
		prev = le->prev;

		if (cl.time >= le->endTime){
			CL_FreeLocalEntity(le);
			continue;
		}

		switch (le->type){
		case LE_FADE:
			CL_AddFade(le);
			break;
		case LE_SCALE:
			CL_AddScale(le);
			break;
		case LE_SCALE_FADE:
			CL_AddScaleFade(le);
			break;
		case LE_MOVE_FADE:
			CL_AddMoveFade(le);
			break;
		case LE_MOVE_SCALE:
			CL_AddMoveScale(le);
			break;
		case LE_MOVE_SCALE_FADE:
			CL_AddMoveScaleFade(le);
			break;
		case LE_ENTITY:
			CL_AddEntity(le);
			break;
		case LE_EJECT_BRASS:
			CL_AddEjectBrass(le);
			break;
		case LE_BLOOD:
			CL_AddBlood(le);
			break;
		case LE_WATER_WAKE:
			CL_AddWaterWake(le);
			break;
		case LE_NUKE_SHOCKWAVE:
			CL_AddNukeShockwave(le);
			break;
		default:
			Com_Error(ERR_DROP, "CL_AddLocalEntities: bad type (%i)", le->type);
		}
	}
}