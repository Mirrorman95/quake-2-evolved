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
// cl_tempEntity.c - Client side temporary entities
//


#include "client.h"


/*
 ==============================================================================

 BEAM AND STEAM PARSING

 ==============================================================================
*/


#define MAX_BEAMS					32
#define MAX_STEAMS					32

typedef enum {
	BEAM_PARASITE,
	BEAM_GRAPPLE,
	BEAM_LIGHTNING,
	BEAM_HEAT
} beamType_t;

typedef struct {
	beamType_t				type;
	int						entity;
	int						destEntity;
	int						endTime;
	vec3_t					start;
	vec3_t					end;
	vec3_t					offset;
} beam_t;

typedef struct {
	int						id;
	int						count;
	vec3_t					origin;
	vec3_t					dir;
	int						color;
	int						magnitude;
	int						endTime;
	int						thinkInterval;
	int						nextThink;
} steam_t;

static beam_t				cl_beams[MAX_BEAMS];
static steam_t				cl_steams[MAX_STEAMS];


/*
 ==================
 CL_ParseParasite
 ==================
*/
static int CL_ParseParasite (){

	beam_t	*beam;
	int		i;
	int		entity;
	vec3_t	start, end;

	// Parse the entity
	entity = MSG_ReadShort(&net_message);

	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);

	// Override any beam with the same entity
	for (i = 0, beam = cl_beams; i < MAX_BEAMS; i++, beam++){
		if (beam->entity == entity){
			beam->type = BEAM_PARASITE;
			beam->entity = entity;
			beam->endTime = cl.time + 200;
			VectorCopy(start, beam->start);
			VectorCopy(end, beam->end);
			VectorClear(beam->offset);

			return entity;
		}
	}

	// Find a free beam
	for (i = 0, beam = cl_beams; i < MAX_BEAMS; i++, beam++){
		if (beam->endTime < cl.time)
			break;
	}

	if (i == MAX_BEAMS){
		Com_DPrintf(S_COLOR_RED "CL_ParseParasite: list overflow\n");
		return entity;
	}

	beam->type = BEAM_PARASITE;
	beam->entity = entity;
	beam->endTime = cl.time + 200;
	VectorCopy(start, beam->start);
	VectorCopy(end, beam->end);
	VectorClear(beam->offset);

	return entity;
}

/*
 ==================
 CL_ParseGrapple
 ==================
*/
static int CL_ParseGrapple (){

	beam_t	*beam;
	int		i;
	int		entity;
	vec3_t	start, end, offset;

	// Parse the entity
	entity = MSG_ReadShort(&net_message);

	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);
	MSG_ReadPos(&net_message, offset);

	// Override any beam with the same entity
	for (i = 0, beam = cl_beams; i < MAX_BEAMS; i++, beam++){
		if (beam->entity == entity){
			beam->type = BEAM_GRAPPLE;
			beam->entity = entity;
			beam->endTime = cl.time + 200;
			VectorCopy(start, beam->start);
			VectorCopy(end, beam->end);
			VectorCopy(offset, beam->offset);
			
			return entity;
		}
	}

	// Find a free beam
	for (i = 0, beam = cl_beams; i < MAX_BEAMS; i++, beam++){
		if (beam->endTime < cl.time)
			break;
	}

	if (i == MAX_BEAMS){
		Com_DPrintf(S_COLOR_RED "CL_ParseGrapple: list overflow\n");
		return entity;
	}

	beam->type = BEAM_GRAPPLE;
	beam->entity = entity;
	beam->endTime = cl.time + 200;
	VectorCopy(start, beam->start);
	VectorCopy(end, beam->end);
	VectorCopy(offset, beam->offset);

	return entity;
}

/*
 ==================
 CL_ParseLightning
 ==================
*/
static int CL_ParseLightning (){

	beam_t	*beam;
	int		i;
	int		entity, destEntity;
	vec3_t	start, end;

	// Parse the entity
	entity = MSG_ReadShort(&net_message);
	destEntity = MSG_ReadShort(&net_message);

	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);

	// Override any beam with the same entity
	for (i = 0, beam = cl_beams; i < MAX_BEAMS; i++, beam++){
		if (beam->entity == entity && beam->destEntity == destEntity){
			beam->type = BEAM_LIGHTNING;
			beam->entity = entity;
			beam->destEntity = destEntity;
			beam->endTime = cl.time + 200;
			VectorCopy(start, beam->start);
			VectorSet(beam->end, end[0], end[1], end[2] - 5.0f);
			VectorClear(beam->offset);

			return entity;
		}
	}

	// Find a free beam
	for (i = 0, beam = cl_beams; i < MAX_BEAMS; i++, beam++){
		if (beam->endTime < cl.time)
			break;
	}

	if (i == MAX_BEAMS){
		Com_DPrintf(S_COLOR_RED "CL_ParseLightning: list overflow\n");
		return entity;
	}

	beam->type = BEAM_LIGHTNING;
	beam->entity = entity;
	beam->destEntity = destEntity;
	beam->endTime = cl.time + 200;
	VectorCopy(start, beam->start);
	VectorSet(beam->end, end[0], end[1], end[2] - 5.0f);
	VectorClear(beam->offset);

	return entity;
}

/*
 ==================
 CL_ParseHeatBeam
 ==================
*/
static int CL_ParseHeatBeam (bool player){

	beam_t	*beam;
	int		i;
	int		entity;
	vec3_t	start, end, offset;

	// Parse the entity
	entity = MSG_ReadShort(&net_message);

	MSG_ReadPos(&net_message, start);
	MSG_ReadPos(&net_message, end);

	// Network optimization
	if (player)
		VectorSet(offset, 2.0f, 7.0f, -3.0f);
	else
		VectorSet(offset, 0.0f, 0.0f, 0.0f);

	// Override any beam with the same entity
	for (i = 0, beam = cl_beams; i < MAX_BEAMS; i++, beam++){
		if (beam->entity == entity){
			beam->type = BEAM_HEAT;
			beam->entity = entity;
			beam->endTime = cl.time + 200;
			VectorCopy(start, beam->start);
			VectorCopy(end, beam->end);
			VectorCopy(offset, beam->offset);

			return entity;
		}
	}

	// Find a free beam
	for (i = 0, beam = cl_beams; i < MAX_BEAMS; i++, beam++){
		if (beam->endTime < cl.time)
			break;
	}

	if (i == MAX_BEAMS){
		Com_DPrintf(S_COLOR_RED "CL_ParseHeatBeam: list overflow\n");
		return entity;
	}

	beam->type = BEAM_HEAT;
	beam->entity = entity;
	beam->endTime = cl.time + 100;
	VectorCopy(start, beam->start);
	VectorCopy(end, beam->end);
	VectorCopy(offset, beam->offset);

	return entity;
}

/*
 ==================
 CL_ParseSteam
 ==================
*/
static void CL_ParseSteam (){

	steam_t	*steam;
	int		i, id;
	vec3_t	pos, dir;
	int		count, color, magnitude, time;

	// Parse the data
	id = MSG_ReadShort(&net_message);
	count = MSG_ReadByte(&net_message);
	MSG_ReadPos(&net_message, pos);
	MSG_ReadDir(&net_message, dir);
	color = MSG_ReadByte(&net_message);
	magnitude = MSG_ReadShort(&net_message);

	// And id of -1 is an instant effect
	if (id == -1){
		CL_SteamParticles(pos, dir, count, color, magnitude);
		return;
	}

	time = MSG_ReadLong(&net_message);

	// Find a free steam
	for (i = 0, steam = cl_steams; i < MAX_STEAMS; i++, steam++){
		if (steam->id == 0)
			break;
	}

	if (i == MAX_STEAMS){
		Com_DPrintf(S_COLOR_RED "CL_ParseSteam: list overflow\n");
		return;
	}

	steam->id = id;
	steam->count = count;
	VectorCopy(pos, steam->origin);
	VectorCopy(dir, steam->dir);
	steam->color = color;
	steam->magnitude = magnitude;
	steam->endTime = cl.time + time;
	steam->thinkInterval = 100;
	steam->nextThink = cl.time;
}


/*
 ==============================================================================

 PLANE HACK

 ==============================================================================
*/


/*
 ==================
 CL_FindTrailPlane

 Disgusting hack
 ==================
*/
static void CL_FindTrailPlane (const vec3_t start, const vec3_t end, vec3_t dir){

	trace_t	trace;
	vec3_t	vec, point;
	float	length;

	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);
	VectorMA(start, length + 0.5f, vec, point);

	trace = CM_BoxTrace(start, point, vec3_origin, vec3_origin, 0, MASK_SOLID);
	if (trace.allsolid || trace.fraction == 1.0f){
		VectorClear(dir);
		return;
	}

	VectorCopy(trace.plane.normal, dir);
}

/*
 ==================
 CL_FindExplosionPlane

 Disgusting hack
 ==================
*/
static void CL_FindExplosionPlane (const vec3_t org, float radius, vec3_t dir){

	static vec3_t	planes[6] = {{0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
	trace_t			trace;
	vec3_t			point;
	float			best = 1.0f;
	int				i;

	VectorClear(dir);

	for (i = 0; i < 6; i++){
		VectorMA(org, radius, planes[i], point);

		trace = CM_BoxTrace(org, point, vec3_origin, vec3_origin, 0, MASK_SOLID);
		if (trace.allsolid || trace.fraction == 1.0f)
			continue;

		if (trace.fraction < best){
			best = trace.fraction;
			VectorCopy(trace.plane.normal, dir);
		}
	}
}


/*
 ==============================================================================

 ENTITY PARSING

 ==============================================================================
*/


/*
 ==================
 CL_ParseTempEntity
 ==================
*/
void CL_ParseTempEntity (){

	int		type;
	vec3_t	pos, pos2, dir;
	int		count, color, i;
	int		ent;

	type = MSG_ReadByte(&net_message);

	switch (type){
	case TE_BLOOD:			// Bullet hitting flesh
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_Bleed(pos, dir, 4, false);
		
		break;
	case TE_GUNSHOT:		// Bullet hitting wall
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BulletParticles(pos, dir);
		R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 3.0f, cl.time, cl.media.bulletMarkMaterial);

		i = rand() & 15;
		if (i >= 1 && i <= 3)
			S_PlaySound(pos, 0, 0, cl.media.richotecSounds[i - 1], 1.0f, ATTN_NORM, 0.0f);

		break;
	case TE_SHOTGUN:		// Bullet hitting wall
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BulletParticles(pos, dir);
		R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 3.0f, cl.time, cl.media.bulletMarkMaterial);

		break;
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_DamageSparkParticles(pos, dir, 6, 0xe0);

		if (type == TE_BULLET_SPARKS){
			i = rand() & 15;
			if (i >= 1 && i <= 3)
				S_PlaySound(pos, 0, 0, cl.media.richotecSounds[i - 1], 1.0f, ATTN_NORM, 0.0f);
		}

		break;
	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		if (type == TE_SCREEN_SPARKS)
			CL_DamageSparkParticles(pos, dir, 40, 0xd0);
		else
			CL_DamageSparkParticles(pos, dir, 40, 0xb0);

		S_PlaySound(pos, 0, 0, cl.media.laserHitSound, 1.0f, ATTN_NORM, 0.0f);

		break;
	case TE_LASER_SPARKS:
		count = MSG_ReadByte(&net_message);
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		color = MSG_ReadByte(&net_message);

		CL_LaserSparkParticles(pos, dir, count, color);

		break;
	case TE_SPLASH:
		count = MSG_ReadByte(&net_message);
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		i = MSG_ReadByte(&net_message);

		switch (i){
		case SPLASH_SPARKS:
			CL_SparkParticles(pos, dir, count);
			S_PlaySound(pos, 0, 0, cl.media.sparkSounds[rand() & 3], 1.0f, ATTN_STATIC, 0.0f);
			break;
		case SPLASH_BLUE_WATER:
			CL_WaterSplash(pos, dir);
			CL_SplashParticles(pos, dir, count * 2, 1.0f, 25.0f);
			break;
		case SPLASH_BROWN_WATER:
			CL_WaterSplash(pos, dir);
			CL_SplashParticles(pos, dir, count * 2, 1.0f, 25.0f);
			break;
		case SPLASH_SLIME:
			CL_WaterSplash(pos, dir);
			CL_SplashParticles(pos, dir, count * 2, 1.0f, 25.0f);
			break;
		case SPLASH_LAVA:
			CL_LavaSteamParticles(pos, dir, count);
			break;
		case SPLASH_BLOOD:
			CL_Bleed(pos, dir, count / 15, false);
			break;
		default:
			CL_SplashParticles(pos, dir, count * 2, 1.0f, 25.0f);
			break;
		}

		break;
	case TE_BLUEHYPERBLASTER:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BlasterParticles(pos, dir, 0.00f, 0.00f, 1.00f);
		R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 3.0f, cl.time, cl.media.energyMarkMaterial);

		break;
	case TE_BLASTER:		// Blaster hitting wall
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BlasterParticles(pos, dir, 0.97f, 0.46f, 0.14f);
		R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 3.0f, cl.time, cl.media.energyMarkMaterial);
		CL_DynamicLight(pos, 150.0f, 1.0f, 1.0f, 0.0f, true, 350);
		S_PlaySound(pos, 0, 0, cl.media.laserHitSound, 1.0f, ATTN_NORM, 0.0f);

		break;
	case TE_RAILTRAIL:		// Railgun effect
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);

		// HACK!!!
		CL_FindTrailPlane(pos, pos2, dir);

		CL_RailTrail(pos, pos2);
		R_ProjectDecalOntoWorld(pos2, dir, rand() % 360, 3.0f, cl.time, cl.media.energyMarkMaterial);
		S_PlaySound(pos2, 0, 0, cl.media.railgunSound, 1.0f, ATTN_NORM, 0.0f);

		break;
	case TE_GRENADE_EXPLOSION:
	case TE_GRENADE_EXPLOSION_WATER:
		MSG_ReadPos(&net_message, pos);
		VectorSet(dir, 0, 0, 1);

		if (type != TE_GRENADE_EXPLOSION_WATER){
			CL_Explosion(pos, dir, 40.0f, rand() % 360, 350.0f, 1.0f, 0.5f, 0.5f, cl.media.grenadeExplosionMaterial);
			CL_ExplosionParticles(pos);
			R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 40.0f, cl.time, cl.media.burnMarkMaterial);
			S_PlaySound(pos, 0, 0, cl.media.grenadeExplosionSound, 1.0f, ATTN_NORM, 0.0f);
		}
		else {
			CL_Explosion(pos, dir, 40.0f, rand() % 360, 350.0f, 1.0f, 0.5f, 0.5f, cl.media.grenadeExplosionWaterMaterial);
			CL_ExplosionWaterSplash(pos);
			CL_BubbleParticles(pos, 384.0f, 30.0f);
			R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 40.0f, cl.time, cl.media.burnMarkMaterial);
			S_PlaySound(pos, 0, 0, cl.media.waterExplosionSound, 1.0f, ATTN_NORM, 0.0f);
		}

		break;
	case TE_EXPLOSION1:
	case TE_EXPLOSION2:
	case TE_ROCKET_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
		MSG_ReadPos(&net_message, pos);

		// HACK!!!
		CL_FindExplosionPlane(pos, 40.0f, dir);

		if (type != TE_ROCKET_EXPLOSION_WATER){
			CL_Explosion(pos, dir, 40.0f, rand() % 360, 350.0f, 1.0f, 0.5f, 0.5f, cl.media.rocketExplosionMaterial);
			CL_ExplosionParticles(pos);
			R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 40.0f, cl.time, cl.media.burnMarkMaterial);
			S_PlaySound(pos, 0, 0, cl.media.rocketExplosionSound, 1.0f, ATTN_NORM, 0.0f);
		}
		else {
			CL_Explosion(pos, dir, 40.0f, rand() % 360, 350, 1.0, 0.5, 0.5, cl.media.rocketExplosionWaterMaterial);
			CL_ExplosionWaterSplash(pos);
			CL_BubbleParticles(pos, 384, 30);
			R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 40.0f, cl.time, cl.media.burnMarkMaterial);
			S_PlaySound(pos, 0, 0, cl.media.waterExplosionSound, 1, ATTN_NORM, 0);
		}

		break;
	case TE_EXPLOSION1_NP:
	case TE_EXPLOSION1_BIG:
	case TE_PLAIN_EXPLOSION:
	case TE_PLASMA_EXPLOSION:
		MSG_ReadPos(&net_message, pos);

		if (type != TE_EXPLOSION1_BIG){
			// HACK!!!
			CL_FindExplosionPlane(pos, 40.0f, dir);

			CL_Explosion(pos, dir, 40, rand() % 360, 350, 1.0, 0.5, 0.5, cl.media.rocketExplosionMaterial);
			CL_ExplosionParticles(pos);
			R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 40.0f, cl.time, cl.media.burnMarkMaterial);
			S_PlaySound(pos, 0, 0, cl.media.rocketExplosionSound, 1, ATTN_NORM, 0);
		}
		else {
			// HACK!!!
			CL_FindExplosionPlane(pos, 60, dir);

			CL_Explosion(pos, dir, 60, rand() % 360, 500, 1.0, 0.5, 0.5, cl.media.rocketExplosionMaterial);
			CL_ExplosionParticles(pos);
			R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 60.0f, cl.time, cl.media.burnMarkMaterial);
			S_PlaySound(pos, 0, 0, cl.media.rocketExplosionSound, 1, ATTN_NORM, 0);
		}

		break;
	case TE_BFG_EXPLOSION:
	case TE_BFG_BIGEXPLOSION:
		MSG_ReadPos(&net_message, pos);
		
		if (type != TE_BFG_BIGEXPLOSION){
			// HACK!!!
			CL_FindExplosionPlane(pos, 40.0f, dir);

			CL_Explosion(pos, vec3_origin, 40.0f, 0.0f, 350.0f, 0.0f, 1.0f, 0.0f, cl.media.bfgExplosionMaterial);
			R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 40.0f, cl.time, cl.media.burnMarkMaterial);
		}
		else {
			// HACK!!!
			CL_FindExplosionPlane(pos, 60.0f, dir);

			CL_Explosion(pos, vec3_origin, 60.0f, 0.0f, 500.0f, 0.0f, 1.0f, 0.0f, cl.media.bfgExplosionMaterial);
			CL_BFGExplosionParticles(pos);
			R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 60.0f, cl.time, cl.media.burnMarkMaterial);
		}

		break;
	case TE_BFG_LASER:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);

		CL_LaserBeam(pos, pos2, 4, 0xD0D1D2D3, 75, 100, cl.media.laserBeamBFGMaterial);

		break;
	case TE_BUBBLETRAIL:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);

		CL_BubbleTrail(pos, pos2, 32.0f, 1.0f);

		break;
	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
		CL_ParseParasite();

		break;
	case TE_GRAPPLE_CABLE:
		CL_ParseGrapple();

		break;
	case TE_BOSSTPORT:		// Boss teleporting to station
		MSG_ReadPos(&net_message, pos);

		CL_BigTeleportParticles(pos);
		S_PlaySound(pos, 0, 0, S_RegisterSound("misc/bigtele.wav", 0), 1.0f, ATTN_NONE, 0.0f);

		break;
	case TE_WELDING_SPARKS:
		count = MSG_ReadByte(&net_message);
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		color = MSG_ReadByte(&net_message);

		CL_WeldingSparkParticles(pos, dir, count, color);
		CL_DynamicLight(pos, 100.0f + (rand() & 75), 1.0f, 1.0f, 0.3f, true, 100);

		break;
	case TE_TUNNEL_SPARKS:
		count = MSG_ReadByte(&net_message);
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
		color = MSG_ReadByte(&net_message);

		CL_TunnelSparkParticles(pos, dir, count, color);

		break;
	case TE_GREENBLOOD:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_Bleed(pos, dir, 4, true);

		break;
	case TE_MOREBLOOD:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_Bleed(pos, dir, 16, false);
		
		break;
	case TE_BLASTER2:		// Green blaster hitting wall
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BlasterParticles(pos, dir, 0.00f, 1.00f, 0.00f);
		R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 3.0f, cl.time, cl.media.energyMarkMaterial);
		CL_DynamicLight(pos, 150.0f, 0.0f, 1.0f, 0.0f, true, 350);
		S_PlaySound(pos, 0, 0, cl.media.laserHitSound, 1.0f, ATTN_NORM, 0.0f);

		break;
	case TE_FLECHETTE:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BulletParticles(pos, dir);
		R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 3.0f, cl.time, cl.media.bulletMarkMaterial);
		CL_DynamicLight(pos, 150.0f, 0.19f, 0.41f, 0.75f, true, 350);
		S_PlaySound(pos, 0, 0, cl.media.laserHitSound, 1.0f, ATTN_NORM, 0.0f);

		break;
	case TE_LIGHTNING:
		ent = CL_ParseLightning();
		S_PlaySound(NULL, ent, CHAN_WEAPON, cl.media.lightningSound, 1.0f, ATTN_NORM, 0.0f);

		break;
	case TE_FLASHLIGHT:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadShort(&net_message);

		CL_DynamicLight(pos, 400.0f, 1.0f, 1.0f, 1.0f, false, 100);

		break;
	case TE_FORCEWALL:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);
		color = MSG_ReadByte(&net_message);

		CL_ForceWallParticles(pos, pos2, color);

		break;
	case TE_HEATBEAM:
		CL_ParseHeatBeam(true);

		break;
	case TE_MONSTER_HEATBEAM:
		CL_ParseHeatBeam(false);

		break;
	case TE_HEATBEAM_SPARKS:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BlasterParticles(pos, dir, 0.97f, 0.46f, 0.14f);
		S_PlaySound(pos, 0, 0, cl.media.laserHitSound, 1.0f, ATTN_NORM, 0.0f);
		
		break;
	case TE_HEATBEAM_STEAM:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);
	
		CL_BlasterParticles(pos, dir, 0.97f, 0.46f, 0.14f);
		R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 3.0f, cl.time, cl.media.energyMarkMaterial);
		S_PlaySound(pos, 0, 0, cl.media.laserHitSound, 1.0f, ATTN_NORM, 0.0f);
		
		break;
	case TE_STEAM:
		CL_ParseSteam();

		break;
	case TE_BUBBLETRAIL2:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);

		CL_BubbleTrail(pos, pos2, 8.0f, 1.0f);
		S_PlaySound(pos, 0, 0, cl.media.laserHitSound, 1.0f, ATTN_NORM, 0.0f);
		
		break;
	case TE_CHAINFIST_SMOKE:
		MSG_ReadPos(&net_message, pos);

		CL_SmokePuffParticles(pos, 3.0f, 10);

		break;
	case TE_ELECTRIC_SPARKS:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadDir(&net_message, dir);

		CL_BulletParticles(pos, dir);
		S_PlaySound(pos, 0, 0, cl.media.laserHitSound, 1.0f, ATTN_NORM, 0.0f);
		
		break;
	case TE_TRACKER_EXPLOSION:
		MSG_ReadPos(&net_message, pos);

		CL_TrackerExplosionParticles(pos);
		CL_DynamicLight(pos, 150.0f, -1.0f, -1.0f, -1.0f, false, 100);
		S_PlaySound(pos, 0, 0, cl.media.disruptorExplosionSound, 1.0f, ATTN_NORM, 0.0f);
		
		break;
	case TE_TELEPORT_EFFECT:
	case TE_DBALL_GOAL:
		MSG_ReadPos(&net_message, pos);

		CL_TeleportParticles(pos);

		break;
	case TE_WIDOWBEAMOUT:
		MSG_ReadShort(&net_message);
		MSG_ReadPos(&net_message, pos);

		break;
	case TE_WIDOWSPLASH:
		MSG_ReadPos(&net_message, pos);

		break;
	case TE_NUKEBLAST:
		MSG_ReadPos(&net_message, pos);
		VectorSet(dir, 0.0f, 0.0f, 1.0f);

		CL_NukeShockwave(pos);

		for (i = 0; i < 5; i++){
			pos2[0] = pos[0];
			pos2[1] = pos[1];
			pos2[2] = pos[2] + 60.0f + (i * 60);

			CL_Explosion(pos2, dir, 90.0f, rand() % 360, 0.0f, 0.0f, 0.0f, 0.0f, cl.media.rocketExplosionMaterial);
			CL_NukeSmokeParticles(pos2);
		}

		for (i = 0; i < 25; i++){
			pos2[0] = pos[0] + crand() * 150.0f;
			pos2[1] = pos[1] + crand() * 150.0f;
			pos2[2] = pos[2] + crand() * 150.0f + 360.0f;

			CL_Explosion(pos2, dir, 90.0f, rand() % 360, 0.0f, 0.0f, 0.0f, 0.0f, cl.media.rocketExplosionMaterial);
			CL_NukeSmokeParticles(pos2);
		}

		R_ProjectDecalOntoWorld(pos, dir, rand() % 360, 240.0f, cl.time, cl.media.burnMarkMaterial);

		break;
	default:
		Com_Error(ERR_DROP, "CL_ParseTempEntity: bad teType (%i)", type);
	}
}

/*
 ==================
 CL_AddBeams
 ==================
*/
static void CL_AddBeams (){

	beam_t			*beam;
	int				i;
	renderEntity_t	entity;
	vec3_t			origin, vec, angles;
	float			distance, length, handMult;

	if (cl_hand->integerValue == 2)
		handMult = 0.0f;
	else if (cl_hand->integerValue == 1)
		handMult = -1.0f;
	else
		handMult = 1.0f;

	for (i = 0, beam = cl_beams; i < MAX_BEAMS; i++, beam++){
		if (beam->endTime < cl.time)
			continue;

		if (beam->type != BEAM_HEAT){
			// If coming from the player, update the start position
			if (beam->entity == cl.clientNum && !cl_thirdPerson->integerValue){
				beam->start[0] = cl.renderView.origin[0];
				beam->start[1] = cl.renderView.origin[1];
				beam->start[2] = cl.renderView.origin[2] - 22.0f;
			}

			VectorAdd(beam->start, beam->offset, origin);
			VectorSubtract(beam->end, origin, vec);
		}
		else {
			// If coming from the player, update the start position
			if (beam->entity == cl.clientNum && !cl_thirdPerson->integerValue){
				// Set up gun position
				beam->start[0] = cl.renderView.origin[0] + cl.oldPlayerState->gunoffset[0] + (cl.playerState->gunoffset[0] - cl.oldPlayerState->gunoffset[0]) * cl.lerpFrac;
				beam->start[1] = cl.renderView.origin[1] + cl.oldPlayerState->gunoffset[1] + (cl.playerState->gunoffset[1] - cl.oldPlayerState->gunoffset[1]) * cl.lerpFrac;
				beam->start[2] = cl.renderView.origin[2] + cl.oldPlayerState->gunoffset[2] + (cl.playerState->gunoffset[2] - cl.oldPlayerState->gunoffset[2]) * cl.lerpFrac;

				VectorMA(beam->start, -(handMult * beam->offset[0]), cl.renderView.axis[1], origin);
				VectorMA(origin, beam->offset[1], cl.renderView.axis[0], origin);
				VectorMA(origin, beam->offset[2], cl.renderView.axis[2], origin);

				if (cl_hand->integerValue == 2)
					VectorMA(origin, -1.0f, cl.renderView.axis[2], origin);

				VectorSubtract(beam->end, origin, vec);

				length = VectorLength(vec);
				VectorScale(cl.renderView.axis[0], length, vec);

				VectorMA(vec, -(handMult * beam->offset[0]), cl.renderView.axis[1], vec);
				VectorMA(vec, beam->offset[1], cl.renderView.axis[0], vec);
				VectorMA(vec, beam->offset[2], cl.renderView.axis[2], vec);
			}
			else {
				VectorAdd(beam->start, beam->offset, origin);
				VectorSubtract(beam->end, origin, vec);
			}

			// Add the particle trail
			CL_HeatBeamTrail(origin, vec);
		}

		// Add the beam entity
		Mem_Fill(&entity, 0, sizeof(renderEntity_t));

		if (beam->type != BEAM_PARASITE){
			entity.type = RE_BEAM;
			VectorCopy(origin, entity.origin);
			MakeRGBA(entity.materialParms, 1.0f, 1.0f, 1.0f, 1.0f);

			switch (beam->type){
			case BEAM_GRAPPLE:
				entity.frame = 2;
				entity.oldFrame = 30;
				entity.material = cl.media.grappleBeamMaterial;
				break;
			case BEAM_LIGHTNING:
				entity.frame = 10;
				entity.oldFrame = 30;
				entity.material = cl.media.lightningBeamMaterial;
				break;
			case BEAM_HEAT:
				entity.frame = 2;
				entity.oldFrame = 30;
				entity.material = cl.media.heatBeamMaterial;
				break;
			}

			R_AddEntityToScene(&entity);
			continue;
		}

		VectorToAngles(vec, angles);
		angles[2] = rand() % 360;

		entity.type = RE_MODEL;
		entity.model = cl.media.parasiteBeamModel;
		AnglesToMat3(angles, entity.axis);
		MakeRGBA(entity.materialParms, 1.0f, 1.0f, 1.0f, 1.0f);

		distance = VectorNormalize(vec);
		length = (distance - 30.0f) / (ceil(distance / 30.0f) - 1.0f);

		while (distance > 0.0f){
			VectorCopy(origin, entity.origin);

			R_AddEntityToScene(&entity);

			VectorMA(origin, length, vec, origin);
			distance -= 30.0f;
		}
	}
}


/*
 ==============================================================================

 ADD ENTITIES TO SCENE

 ==============================================================================
*/


/*
 ==================
 CL_AddSteams
 ==================
*/
static void CL_AddSteams (){

	steam_t	*steam;
	int		i;

	for (i = 0, steam = cl_steams; i < MAX_STEAMS; i++, steam++){
		if (steam->id == 0)
			continue;

		if (steam->endTime < cl.time){
			steam->id = 0;
			continue;
		}

		if (cl.time >= steam->nextThink){
			CL_SteamParticles(steam->origin, steam->dir, steam->count, steam->color, steam->magnitude);

			steam->nextThink += steam->thinkInterval;
		}
	}
}

/*
 ==================
 CL_ClearTempEntities
 ==================
*/
void CL_ClearTempEntities (){

	Mem_Fill(cl_beams, 0, sizeof(cl_beams));
	Mem_Fill(cl_steams, 0, sizeof(cl_steams));
}

/*
 ==================
 CL_AddTempEntities
 ==================
*/
void CL_AddTempEntities (){

	CL_AddBeams();
	CL_AddSteams();
}