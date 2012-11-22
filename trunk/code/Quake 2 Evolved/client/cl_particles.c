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
// cl_particles.c - Particle effects
//


#include "client.h"


#define PARTICLE_BOUNCE				1
#define PARTICLE_FRICTION			2
#define PARTICLE_VERTEXLIGHT		4
#define PARTICLE_STRETCH			8
#define PARTICLE_UNDERWATER			16
#define PARTICLE_INSTANT			32

typedef struct cparticle_s {
	struct cparticle_s *	next;

	material_t *			material;
	int						time;
	int						flags;

	vec3_t					origin;
	vec3_t					velocity;
	vec3_t					accel;
	vec3_t					color;
	vec3_t					colorVel;
	float					alpha;
	float					alphaVel;
	float					radius;
	float					radiusVel;
	float					length;
	float					lengthVel;
	float					rotation;
	float					bounceFactor;

	vec3_t					lastOrigin;
} cparticle_t;

static cparticle_t	*		cl_activeParticles;
static cparticle_t	*		cl_freeParticles;
static cparticle_t			cl_particleList[MAX_RENDER_PARTICLES];

static vec3_t				cl_particleVelocities[NUM_VERTEX_NORMALS];
static vec3_t				cl_particlePalette[256];


/*
 ==============================================================================

 ALLOCATING AND FREEING

 ==============================================================================
*/


/*
 ==================
 CL_FreeParticle
 ==================
*/
static void CL_FreeParticle (cparticle_t *p){

	p->next = cl_freeParticles;
	cl_freeParticles = p;
}

/*
 ==================
 CL_AllocParticle
 ==================
*/
static cparticle_t *CL_AllocParticle (){

	cparticle_t	*p;

	if (!cl_freeParticles)
		return NULL;

	if (cl_particleLOD->integerValue > 1){
		if (!(rand() % cl_particleLOD->integerValue))
			return NULL;
	}

	p = cl_freeParticles;
	cl_freeParticles = p->next;
	p->next = cl_activeParticles;
	cl_activeParticles = p;

	return p;
}


/*
 ==============================================================================

 EFFECTS

 ==============================================================================
*/


/*
 ==================
 CL_BlasterTrail
 ==================
*/
void CL_BlasterTrail (const vec3_t start, const vec3_t end, float r, float g, float b){

	cparticle_t	*p;
	vec3_t		move, vec;
	float		length, dist;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dist = 1.5f;
	VectorScale(vec, dist, vec);

	while (length > 0.0f){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = move[0] + crand();
		p->origin[1] = move[1] + crand();
		p->origin[2] = move[2] + crand();
		p->velocity[0] = crand() * 5.0f;
		p->velocity[1] = crand() * 5.0f;
		p->velocity[2] = crand() * 5.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f / (0.3f + frand() * 0.2f);
		p->radius = 2.4f + (1.2f * crand());
		p->radiusVel = -2.4f + (1.2f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_GrenadeTrail
 ==================
*/
void CL_GrenadeTrail (const vec3_t start, const vec3_t end){

	cparticle_t	*p;
	vec3_t		move, vec;
	float		length, dist;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(end, -1) & MASK_WATER){
		if (CL_PointContents(start, -1) & MASK_WATER)
			CL_BubbleTrail(start, end, 16.0f, 1.0f);

		return;
	}
	
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dist = 20.0f;
	VectorScale(vec, dist, vec);

	while (length > 0.0f){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.liteSmokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = move[0] + crand();
		p->origin[1] = move[1] + crand();
		p->origin[2] = move[2] + crand();
		p->velocity[0] = crand() * 5.0f;
		p->velocity[1] = crand() * 5.0f;
		p->velocity[2] = crand() * 5.0f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.4f + frand() * 0.2f);
		p->radius = 3.0f + (1.5f * crand());
		p->radiusVel = 3.0f + (1.5f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_RocketTrail
 ==================
*/
void CL_RocketTrail (const vec3_t start, const vec3_t end){

	cparticle_t	*p;
	int			flags = 0;
	vec3_t		move, vec;
	float		length, dist;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(end, -1) & MASK_WATER){
		if (CL_PointContents(start, -1) & MASK_WATER)
			CL_BubbleTrail(start, end, 8.0f, 3.0f);

		return;
	}

	// Flames
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dist = 1.0f;
	VectorScale(vec, dist, vec);

	while (length > 0.0f){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.flameParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = move[0] + crand();
		p->origin[1] = move[1] + crand();
		p->origin[2] = move[2] + crand();
		p->velocity[0] = crand() * 5.0f;
		p->velocity[1] = crand() * 5.0f;
		p->velocity[2] = crand() * 5.0f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -2.0f / (0.2f + frand() * 0.1f);
		p->radius = 3.0f + (1.5f * crand());
		p->radiusVel = -6.0f + (3.0f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}
	
	// Smoke
	flags = 0;

	if (cl_particleVertexLight->integerValue)
		flags |= PARTICLE_VERTEXLIGHT;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dist = 10.0f;
	VectorScale(vec, dist, vec);

	while (length > 0){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = move[0] + crand();
		p->origin[1] = move[1] + crand();
		p->origin[2] = move[2] + crand();
		p->velocity[0] = crand() * 5.0f;
		p->velocity[1] = crand() * 5.0f;
		p->velocity[2] = crand() * 5.0f + (25 + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.0f;
		p->color[1] = 0.0f;
		p->color[2] = 0.0f;
		p->colorVel[0] = 0.75f;
		p->colorVel[1] = 0.75f;
		p->colorVel[2] = 0.75f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.2f + frand() * 0.1f);
		p->radius = 3.0f + (1.5f * crand());
		p->radiusVel = 15.0f + (7.5f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_RailTrail
 ==================
*/
void CL_RailTrail (const vec3_t start, const vec3_t end){

	cparticle_t	*p;
	int			flags;
	vec3_t		move, vec;
	float		length, dist;
	int			i;
	vec3_t		right, up, dir;
	float		d, s, c;

	if (!cl_particles->integerValue)
		return;

	// Core
	flags = 0;

	if (cl_particleVertexLight->integerValue)
		flags |= PARTICLE_VERTEXLIGHT;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dist = 2.0f;
	VectorScale(vec, dist, vec);

	while (length > 0.0f){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = move[0] + crand();
		p->origin[1] = move[1] + crand();
		p->origin[2] = move[2] + crand();
		p->velocity[0] = crand() * 1.5f;
		p->velocity[1] = crand() * 1.5f;
		p->velocity[2] = crand() * 1.5f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 0.5f;
		p->alphaVel = -0.5f / (1.0f + frand() * 0.2f);
		p->radius = 1.5f + (0.5f * crand());
		p->radiusVel = 3.0f + (1.5f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}

	// Spiral
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	MakeNormalVectors(vec, right, up);

	for (i = 0; i < length; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		d = i * 0.1f;
		s = sin(d);
		c = cos(d);

		dir[0] = right[0] * c + up[0] * s;
		dir[1] = right[1] * c + up[1] * s;
		dir[2] = right[2] * c + up[2] * s;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = move[0] + dir[0] * 2.5f;
		p->origin[1] = move[1] + dir[1] * 2.5f;
		p->origin[2] = move[2] + dir[2] * 2.5f;
		p->velocity[0] = dir[0] * 5.0f;
		p->velocity[1] = dir[1] * 5.0f;
		p->velocity[2] = dir[2] * 5.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.09f;
		p->color[1] = 0.32f;
		p->color[2] = 0.43f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f;
		p->radius = 2.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_BFGTrail
 ==================
*/
void CL_BFGTrail (const vec3_t start, const vec3_t end){

	cparticle_t	*p;
	vec3_t		move, vec, org;
	float		length, dist, d, time;
	float		angle, sy, cy, sp, cp;
	int			i;

	if (!cl_particles->integerValue)
		return;

	// Particles
	time = cl.time * 0.001f;

	org[0] = start[0] + (end[0] - start[0]) * cl.lerpFrac;
	org[1] = start[1] + (end[1] - start[1]) * cl.lerpFrac;
	org[2] = start[2] + (end[2] - start[2]) * cl.lerpFrac;
	
	for (i = 0; i < NUM_VERTEX_NORMALS; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		angle = time * cl_particleVelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = time * cl_particleVelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);

		vec[0] = cp * cy;
		vec[1] = cp * sy;
		vec[2] = -sp;

		d = sin(time + i) * 64.0f;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_INSTANT;

		p->origin[0] = org[0] + byteDirs[i][0] * d + vec[0] * 16.0f;
		p->origin[1] = org[1] + byteDirs[i][1] * d + vec[1] * 16.0f;
		p->origin[2] = org[2] + byteDirs[i][2] * d + vec[2] * 16.0f;
		p->velocity[0] = 0.0f;
		p->velocity[1] = 0.0f;
		p->velocity[2] = 0.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.24f;
		p->color[1] = 0.82f;
		p->color[2] = 0.10f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f - Distance(org, p->origin) / 90.0f;
		p->alphaVel = 0.0f;
		p->radius = 2.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
	}

	if (CL_PointContents(end, -1) & MASK_WATER){
		CL_BubbleTrail(start, end, 75.0f, 10.0f);
		return;
	}

	// Smoke
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dist = 75.0f;
	VectorScale(vec, dist, vec);

	while (length > 0.0f){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.liteSmokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = move[0] + crand();
		p->origin[1] = move[1] + crand();
		p->origin[2] = move[2] + crand();
		p->velocity[0] = crand() * 5.0f;
		p->velocity[1] = crand() * 5.0f;
		p->velocity[2] = crand() * 5.0f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.4f + frand() * 0.2f);
		p->radius = 25.0f + (5.0f * crand());
		p->radiusVel = 25.0f + (5.0f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_HeatBeamTrail
 ==================
*/
void CL_HeatBeamTrail (const vec3_t start, const vec3_t forward){

	cparticle_t	*p;
	vec3_t		move, vec, end;
	float		length, dist, step;
	vec3_t		dir;
	float		rot, s, c;

	if (!cl_particles->integerValue)
		return;

	length = VectorNormalize2(forward, vec);
	VectorMA(start, length, vec, end);

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	VectorNormalize(vec);

	dist = fmod(cl.time * 0.096f, 32.0f);
	length -= dist;
	VectorMA(move, dist, vec, move);

	dist = 32.0f;
	VectorScale(vec, dist, vec);

	step = M_PI * 0.1f;

	while (length > 0.0f){
		length -= dist;

		for (rot = 0; rot < M_PI_TWO; rot += step){
			p = CL_AllocParticle();
			if (!p)
				return;

			s = sin(rot) * 0.5f;
			c = cos(rot) * 0.5f;

			dir[0] = cl.renderView.axis[1][0] * c + cl.renderView.axis[2][0] * s;
			dir[1] = cl.renderView.axis[1][1] * c + cl.renderView.axis[2][1] * s;
			dir[2] = cl.renderView.axis[1][2] * c + cl.renderView.axis[2][2] * s;

			p->material = cl.media.energyParticleMaterial;
			p->time = cl.time;
			p->flags = PARTICLE_INSTANT;

			p->origin[0] = move[0] + dir[0] * 3.0f;
			p->origin[1] = move[1] + dir[1] * 3.0f;
			p->origin[2] = move[2] + dir[2] * 3.0f;
			p->velocity[0] = 0.0f;
			p->velocity[1] = 0.0f;
			p->velocity[2] = 0.0f;
			p->accel[0] = 0.0f;
			p->accel[1] = 0.0f;
			p->accel[2] = 0.0f;
			p->color[0] = 0.97f;
			p->color[1] = 0.46f;
			p->color[2] = 0.14f;
			p->colorVel[0] = 0.0f;
			p->colorVel[1] = 0.0f;
			p->colorVel[2] = 0.0f;
			p->alpha = 0.5f;
			p->alphaVel = 0.0f;
			p->radius = 1.5f;
			p->radiusVel = 0.0f;
			p->length = 1.0f;
			p->lengthVel = 0.0f;
			p->rotation = 0.0f;
		}

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_TrackerTrail
 ==================
*/
void CL_TrackerTrail (const vec3_t start, const vec3_t end){

	cparticle_t	*p;
	vec3_t		move, vec;
	vec3_t		angles, forward, up;
	float		length, dist, c;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	VectorToAngles(vec, angles);
	AngleToVectors(angles, forward, NULL, up);

	dist = 2.5f;
	VectorScale(vec, dist, vec);

	while (length > 0.0f){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		c = 8.0f * cos(DotProduct(move, forward));

		p->material = cl.media.trackerParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = move[0] + up[0] * c;
		p->origin[1] = move[1] + up[1] * c;
		p->origin[2] = move[2] + up[2] * c;
		p->velocity[0] = 0.0f;
		p->velocity[1] = 0.0f;
		p->velocity[2] = 5.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.0f;
		p->color[1] = 0.0f;
		p->color[2] = 0.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -2.0f;
		p->radius = 2.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_TagTrail
 ==================
*/
void CL_TagTrail (const vec3_t start, const vec3_t end){

	cparticle_t	*p;
	vec3_t		move, vec;
	float		length, dist;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dist = 5.0f;
	VectorScale(vec, dist, vec);

	while (length > 0){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = move[0] + crand() * 16.0f;
		p->origin[1] = move[1] + crand() * 16.0f;
		p->origin[2] = move[2] + crand() * 16.0f;
		p->velocity[0] = crand() * 5.0f;
		p->velocity[1] = crand() * 5.0f;
		p->velocity[2] = crand() * 5.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 1.00f;
		p->color[1] = 1.00f;
		p->color[2] = 0.15f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f / (0.8f + frand() * 0.2f);
		p->radius = 1.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_BubbleTrail
 ==================
*/
void CL_BubbleTrail (const vec3_t start, const vec3_t end, float dist, float radius){

	cparticle_t	*p;
	vec3_t		move, vec;
	float		length;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	VectorScale(vec, dist, vec);

	while (length > 0.0f){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.bubbleParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_UNDERWATER;

		p->origin[0] = move[0] + crand();
		p->origin[1] = move[1] + crand();
		p->origin[2] = move[2] + crand();
		p->velocity[0] = crand() * 5.0f;
		p->velocity[1] = crand() * 5.0f;
		p->velocity[2] = crand() * 5.0f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -(0.4f + frand() * 0.2f);
		p->radius = radius + ((radius * 0.5f) * crand());
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_FlagTrail
 ==================
*/
void CL_FlagTrail (const vec3_t start, const vec3_t end, float r, float g, float b){

	cparticle_t	*p;
	vec3_t		move, vec;
	float		length, dist;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dist = 5.0f;
	VectorScale(vec, dist, vec);

	while (length > 0.0f){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.glowParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = move[0] + crand() * 16.0f;
		p->origin[1] = move[1] + crand() * 16.0f;
		p->origin[2] = move[2] + crand() * 16.0f;
		p->velocity[0] = crand() * 5.0f;
		p->velocity[1] = crand() * 5.0f;
		p->velocity[2] = crand() * 5.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f / (0.8f + frand() * 0.2f);
		p->radius = 2.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_BlasterParticles
 ==================
*/
void CL_BlasterParticles (const vec3_t origin, const vec3_t dir, float r, float g, float b){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integerValue)
		return;

	// Sparks
	flags = 0;

	if (cl_particleBounce->integerValue)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integerValue)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < 40; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + dir[0] * 3.0f + crand();
		p->origin[1] = origin[1] + dir[1] * 3.0f + crand();
		p->origin[2] = origin[2] + dir[2] * 3.0f + crand();
		p->velocity[0] = dir[0] * 25.0f + crand() * 20.0f;
		p->velocity[1] = dir[1] * 25.0f + crand() * 20.0f;
		p->velocity[2] = dir[2] * 50.0f + crand() * 20.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -120.0f + (40.0f * crand());
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -0.25f / (0.3f + frand() * 0.2f);
		p->radius = 2.4f + (1.2f * crand());
		p->radiusVel = -1.2f + (0.6f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
		p->bounceFactor = 0.7f;

		VectorCopy(p->origin, p->lastOrigin);
	}

	if (CL_PointContents(origin, -1) & MASK_WATER)
		return;

	// Steam
	for (i = 0; i < 3; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.steamParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = origin[0] + dir[0] * 5.0f + crand();
		p->origin[1] = origin[1] + dir[1] * 5.0f + crand();
		p->origin[2] = origin[2] + dir[2] * 5.0f + crand();
		p->velocity[0] = crand() * 2.5f;
		p->velocity[1] = crand() * 2.5f;
		p->velocity[2] = crand() * 2.5f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.4f + frand() * 0.2f);
		p->radius = 3.0f + (1.5f * crand());
		p->radiusVel = 5.0f + (2.5f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;
	}
}

/*
 ==================
 CL_BulletParticles
 ==================
*/
void CL_BulletParticles (const vec3_t origin, const vec3_t dir){

	cparticle_t	*p;
	int			flags;
	int			i, count;

	if (!cl_particles->integerValue)
		return;

	count = 3 + (rand() % 5);

	if (CL_PointContents(origin, -1) & MASK_WATER){
		CL_BubbleParticles(origin, count, 0.0f);
		return;
	}

	// Sparks
	flags = PARTICLE_STRETCH;

	if (cl_particleBounce->integerValue)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integerValue)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.sparkParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + dir[0] * 2.0f + crand();
		p->origin[1] = origin[1] + dir[1] * 2.0f + crand();
		p->origin[2] = origin[2] + dir[2] * 2.0f + crand();
		p->velocity[0] = dir[0] * 180.0f + crand() * 60.0f;
		p->velocity[1] = dir[1] * 180.0f + crand() * 60.0f;
		p->velocity[2] = dir[2] * 180.0f + crand() * 60.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -120.0f + (60.0f * crand());
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -8.0f;
		p->radius = 0.4f + (0.2f * crand());
		p->radiusVel = 0.0f;
		p->length = 8.0f + (4.0f * crand());
		p->lengthVel = 8.0f + (4.0f * crand());
		p->rotation = 0.0f;
		p->bounceFactor = 0.2f;

		VectorCopy(p->origin, p->lastOrigin);
	}

	// Smoke
	flags = 0;

	if (cl_particleVertexLight->integerValue)
		flags |= PARTICLE_VERTEXLIGHT;

	for (i = 0; i < 3; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + dir[0] * 5.0f + crand();
		p->origin[1] = origin[1] + dir[1] * 5.0f + crand();
		p->origin[2] = origin[2] + dir[2] * 5.0f + crand();
		p->velocity[0] = crand() * 2.5f;
		p->velocity[1] = crand() * 2.5f;
		p->velocity[2] = crand() * 2.5f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.4f;
		p->color[1] = 0.4f;
		p->color[2] = 0.4f;
		p->colorVel[0] = 0.2f;
		p->colorVel[1] = 0.2f;
		p->colorVel[2] = 0.2f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.4f + frand() * 0.2f);
		p->radius = 3.0f + (1.5f * crand());
		p->radiusVel = 5.0f + (2.5f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;
	}
}

/*
 ==================
 CL_ExplosionParticles
 ==================
*/
void CL_ExplosionParticles (const vec3_t origin){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integerValue)
		return;

	// Sparks
	flags = PARTICLE_STRETCH;

	if (cl_particleBounce->integerValue)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integerValue)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < 384; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.sparkParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + ((rand() % 32) - 16.0f);
		p->origin[1] = origin[1] + ((rand() % 32) - 16.0f);
		p->origin[2] = origin[2] + ((rand() % 32) - 16.0f);
		p->velocity[0] = (rand() % 512) - 256.0f;
		p->velocity[1] = (rand() % 512) - 256.0f;
		p->velocity[2] = (rand() % 512) - 256.0f;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -60.0f + (30.0f * crand());
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -3.0f;
		p->radius = 0.5f + (0.2f * crand());
		p->radiusVel = 0.0f;
		p->length = 8.0f + (4.0f * crand());
		p->lengthVel = 8.0f + (4.0f * crand());
		p->rotation = 0.0f;
		p->bounceFactor = 0.2f;

		VectorCopy(p->origin, p->lastOrigin);
	}

	// Smoke
	flags = 0;

	if (cl_particleVertexLight->integerValue)
		flags |= PARTICLE_VERTEXLIGHT;

	for (i = 0; i < 5; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + crand() * 10.0f;
		p->origin[1] = origin[1] + crand() * 10.0f;
		p->origin[2] = origin[2] + crand() * 10.0f;
		p->velocity[0] = crand() * 10.0f;
		p->velocity[1] = crand() * 10.0f;
		p->velocity[2] = crand() * 10.0f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.0f;
		p->color[1] = 0.0f;
		p->color[2] = 0.0f;
		p->colorVel[0] = 0.75f;
		p->colorVel[1] = 0.75f;
		p->colorVel[2] = 0.75f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.1f + frand() * 0.1f);
		p->radius = 30.0f + (15.0f * crand());
		p->radiusVel = 15.0f + (7.5f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;
	}
}

/*
 ==================
 CL_BFGExplosionParticles
 ==================
*/
void CL_BFGExplosionParticles (const vec3_t origin){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integerValue)
		return;

	// Particles
	flags = 0;

	if (cl_particleBounce->integerValue)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integerValue)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < 384; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + ((rand() % 32) - 16.0f);
		p->origin[1] = origin[1] + ((rand() % 32) - 16.0f);
		p->origin[2] = origin[2] + ((rand() % 32) - 16.0f);
		p->velocity[0] = (rand() % 384) - 192.0f;
		p->velocity[1] = (rand() % 384) - 192.0f;
		p->velocity[2] = (rand() % 384) - 192.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -40.0f + (10.0f * crand());
		p->color[0] = 0.24f;
		p->color[1] = 0.82f;
		p->color[2] = 0.10f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -0.8f / (0.5f + frand() * 0.3f);
		p->radius = 2.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
		p->bounceFactor = 0.7f;

		VectorCopy(p->origin, p->lastOrigin);
	}

	if (CL_PointContents(origin, -1) & MASK_WATER)
		return;

	// Smoke
	for (i = 0; i < 5; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.liteSmokeParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = origin[0] + crand() * 10.0f;
		p->origin[1] = origin[1] + crand() * 10.0f;
		p->origin[2] = origin[2] + crand() * 10.0f;
		p->velocity[0] = crand() * 10.0f;
		p->velocity[1] = crand() * 10.0f;
		p->velocity[2] = crand() * 10.0f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.1f + frand() * 0.1f);
		p->radius = 50.0f + (25.0f * crand());
		p->radiusVel = 15.0f + (7.5f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;
	}
}

/*
 ==================
 CL_TrackerExplosionParticles
 ==================
*/
void CL_TrackerExplosionParticles (const vec3_t origin){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integerValue)
		return;

	flags = 0;

	if (cl_particleBounce->integerValue)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integerValue)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < 384; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.trackerParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + ((rand() % 32) - 16.0f);
		p->origin[1] = origin[1] + ((rand() % 32) - 16.0f);
		p->origin[2] = origin[2] + ((rand() % 32) - 16.0f);
		p->velocity[0] = (rand() % 256) - 128.0f;
		p->velocity[1] = (rand() % 256) - 128.0f;
		p->velocity[2] = (rand() % 256) - 128.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -40.0f + (10.0f * crand());
		p->color[0] = 0.0f;
		p->color[1] = 0.0f;
		p->color[2] = 0.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -0.4f / (0.6f + frand() * 0.2f);
		p->radius = 2.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
		p->bounceFactor = 0.7f;

		VectorCopy(p->origin, p->lastOrigin);
	}
}

/*
 ==================
 CL_SmokePuffParticles
 ==================
*/
void CL_SmokePuffParticles (const vec3_t origin, float radius, int count){

	cparticle_t	*p;
	int			flags;
	int			i;

	flags = 0;

	if (cl_particleVertexLight->integerValue)
		flags |= PARTICLE_VERTEXLIGHT;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + crand();
		p->origin[1] = origin[1] + crand();
		p->origin[2] = origin[2] + crand();
		p->velocity[0] = crand() * 5.0f;
		p->velocity[1] = crand() * 5.0f;
		p->velocity[2] = crand() * 5.0f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.4f;
		p->color[1] = 0.4f;
		p->color[2] = 0.4f;
		p->colorVel[0] = 0.75f;
		p->colorVel[1] = 0.75f;
		p->colorVel[2] = 0.75f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.4f + frand() * 0.2f);
		p->radius = radius + ((radius * 0.5f) * crand());
		p->radiusVel = (radius * 2.0f) + ((radius * 4.0f) * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;
	}
}

/*
 ==================
 CL_BubbleParticles
 ==================
*/
void CL_BubbleParticles (const vec3_t origin, int count, float magnitude){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.bubbleParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_UNDERWATER;

		p->origin[0] = origin[0] + (magnitude * crand());
		p->origin[1] = origin[1] + (magnitude * crand());
		p->origin[2] = origin[2] + (magnitude * crand());
		p->velocity[0] = crand() * 5.0f;
		p->velocity[1] = crand() * 5.0f;
		p->velocity[2] = crand() * 5.0f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -(0.4f + frand() * 0.2f);
		p->radius = 1.0f + (0.5f * crand());
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
	}
}

/*
 ==================
 CL_SparkParticles
 ==================
*/
void CL_SparkParticles (const vec3_t origin, const vec3_t dir, int count){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(origin, -1) & MASK_WATER){
		CL_BubbleParticles(origin, count, 0.0f);
		return;
	}

	// Sparks
	flags = PARTICLE_STRETCH;

	if (cl_particleBounce->integerValue)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integerValue)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.sparkParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + dir[0] * 2.0f + crand();
		p->origin[1] = origin[1] + dir[1] * 2.0f + crand();
		p->origin[2] = origin[2] + dir[2] * 2.0f + crand();
		p->velocity[0] = dir[0] * 180.0f + crand() * 60.0f;
		p->velocity[1] = dir[1] * 180.0f + crand() * 60.0f;
		p->velocity[2] = dir[2] * 180.0f + crand() * 60.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -120.0f + (60.0f * crand());
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.5f;
		p->radius = 0.4f + (0.2f * crand());
		p->radiusVel = 0.0f;
		p->length = 8.0f + (4.0f * crand());
		p->lengthVel = 8.0f + (4.0f * crand());
		p->rotation = 0.0f;
		p->bounceFactor = 0.2f;

		VectorCopy(p->origin, p->lastOrigin);
	}

	// Smoke
	flags = 0;

	if (cl_particleVertexLight->integerValue)
		flags |= PARTICLE_VERTEXLIGHT;

	for (i = 0; i < 3; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + dir[0] * 5.0f + crand();
		p->origin[1] = origin[1] + dir[1] * 5.0f + crand();
		p->origin[2] = origin[2] + dir[2] * 5.0f + crand();
		p->velocity[0] = crand() * 2.5f;
		p->velocity[1] = crand() * 2.5f;
		p->velocity[2] = crand() * 2.5f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.4f;
		p->color[1] = 0.4f;
		p->color[2] = 0.4f;
		p->colorVel[0] = 0.2f;
		p->colorVel[1] = 0.2f;
		p->colorVel[2] = 0.2f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.4f + frand() * 0.2f);
		p->radius = 3.0f + (1.5f * crand());
		p->radiusVel = 5.0f + (2.5f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;
	}
}

/*
 ==================
 CL_DamageSparkParticles
 ==================
*/
void CL_DamageSparkParticles (const vec3_t origin, const vec3_t dir, int count, int color){

	cparticle_t	*p;
	int			flags;
	int			i, index;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(origin, -1) & MASK_WATER){
		CL_BubbleParticles(origin, count, 2.5f);
		return;
	}

	flags = 0;

	if (cl_particleBounce->integerValue)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integerValue)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		index = (color + (rand() & 7)) & 0xff;

		p->material = cl.media.impactSparkParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + dir[0] * 2.0f + crand();
		p->origin[1] = origin[1] + dir[1] * 2.0f + crand();
		p->origin[2] = origin[2] + dir[2] * 2.0f + crand();
		p->velocity[0] = crand() * 60.0f;
		p->velocity[1] = crand() * 60.0f;
		p->velocity[2] = crand() * 60.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -180.0f + (30.0f * crand());
		p->color[0] = cl_particlePalette[index][0];
		p->color[1] = cl_particlePalette[index][1];
		p->color[2] = cl_particlePalette[index][2];
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -(0.75f + frand());
		p->radius = 0.4f + (0.2f * crand());
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
		p->bounceFactor = 0.6f;
	}
}

/*
 ==================
 CL_LaserSparkParticles
 ==================
*/
void CL_LaserSparkParticles (const vec3_t origin, const vec3_t dir, int count, int color){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(origin, -1) & MASK_WATER){
		CL_BubbleParticles(origin, count, 2.5f);
		return;
	}

	flags = 0;

	if (cl_particleBounce->integerValue)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integerValue)
		flags |= PARTICLE_FRICTION;

	color &= 0xff;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.impactSparkParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + dir[0] * 2.0f + crand();
		p->origin[1] = origin[1] + dir[1] * 2.0f + crand();
		p->origin[2] = origin[2] + dir[2] * 2.0f + crand();
		p->velocity[0] = dir[0] * 180.0f + crand() * 60.0f;
		p->velocity[1] = dir[1] * 180.0f + crand() * 60.0f;
		p->velocity[2] = dir[2] * 180.0f + crand() * 60.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -120.0f + (60.0f * crand());
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.5f;
		p->radius = 0.4f + (0.2f * crand());
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
		p->bounceFactor = 0.2f;

		VectorCopy(p->origin, p->lastOrigin);
	}
}

/*
 ==================
 CL_SplashParticles
 ==================
*/
void CL_SplashParticles (const vec3_t origin, const vec3_t dir, int count, float magnitude, float spread){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.dropletParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = origin[0] + dir[0] * 3.0f + crand() * magnitude;
		p->origin[1] = origin[1] + dir[1] * 3.0f + crand() * magnitude;
		p->origin[2] = origin[2] + dir[2] * 3.0f + crand() * magnitude;
		p->velocity[0] = dir[0] * spread + crand() * spread;
		p->velocity[1] = dir[1] * spread + crand() * spread;
		p->velocity[2] = dir[2] * spread + crand() * spread + 4 * spread;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -150.0f + (25.0f * crand());
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.25f + frand() * 0.25f);
		p->radius = 0.5f + (0.25f * crand());
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;
	}
}

/*
 ==================
 CL_LavaSteamParticles
 ==================
*/
void CL_LavaSteamParticles (const vec3_t origin, const vec3_t dir, int count){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.steamParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = origin[0] + dir[0] * 2.0f + crand();
		p->origin[1] = origin[1] + dir[1] * 2.0f + crand();
		p->origin[2] = origin[2] + dir[2] * 2.0f + crand();
		p->velocity[0] = crand() * 2.5f;
		p->velocity[1] = crand() * 2.5f;
		p->velocity[2] = crand() * 2.5f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.82f;
		p->color[1] = 0.34f;
		p->color[2] = 0.00f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.4f + frand() * 0.2f);
		p->radius = 1.5f + (0.75f * crand());
		p->radiusVel = 5.0f + (2.5f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;
	}
}

/*
 ==================
 CL_FlyParticles
 ==================
*/
void CL_FlyParticles (const vec3_t origin, int count){

	cparticle_t	*p;
	vec3_t		vec;
	float		d, time;
	float		angle, sy, cy, sp, cp;
	int			i;

	if (!cl_particles->integerValue)
		return;

	if (CL_PointContents(origin, -1) & MASK_WATER)
		return;

	time = cl.time * 0.001f;

	for (i = 0; i < count ; i += 2){
		p = CL_AllocParticle();
		if (!p)
			return;

		angle = time * cl_particleVelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = time * cl_particleVelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);

		vec[0] = cp * cy;
		vec[1] = cp * sy;
		vec[2] = -sp;

		d = sin(time + i) * 64.0f;

		p->material = cl.media.flyParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_INSTANT;

		p->origin[0] = origin[0] + byteDirs[i][0] * d + vec[0] * 16.0f;
		p->origin[1] = origin[1] + byteDirs[i][1] * d + vec[1] * 16.0f;
		p->origin[2] = origin[2] + byteDirs[i][2] * d + vec[2] * 16.0f;
		p->velocity[0] = 0.0f;
		p->velocity[1] = 0.0f;
		p->velocity[2] = 0.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 1.0f;
		p->color[1] = 1.0f;
		p->color[2] = 1.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = 0.0f;
		p->radius = 1.0f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
	}
}

/*
 ==================
 CL_TeleportParticles
 ==================
*/
void CL_TeleportParticles (const vec3_t origin){

	cparticle_t	*p;
	vec3_t		dir;
	float		vel, color;
	int			x, y, z;

	if (!cl_particles->integerValue)
		return;

	for (x = -16; x <= 16; x += 4){
		for (y = -16; y <= 16; y += 4){
			for (z = -16; z <= 32; z += 4){
				p = CL_AllocParticle();
				if (!p)
					return;

				VectorSet(dir, y * 8.0f, x * 8.0f, z * 8.0f);
				VectorNormalizeFast(dir);

				vel = 50.0f + (rand() & 63);

				color = 0.1f + (0.2f * frand());

				p->material = cl.media.glowParticleMaterial;
				p->time = cl.time;
				p->flags = 0;

				p->origin[0] = origin[0] + x + (rand() & 3);
				p->origin[1] = origin[1] + y + (rand() & 3);
				p->origin[2] = origin[2] + z + (rand() & 3);
				p->velocity[0] = dir[0] * vel;
				p->velocity[1] = dir[1] * vel;
				p->velocity[2] = dir[2] * vel;
				p->accel[0] = 0.0f;
				p->accel[1] = 0.0f;
				p->accel[2] = -40.0f;
				p->color[0] = color;
				p->color[1] = color;
				p->color[2] = color;
				p->colorVel[0] = 0.0f;
				p->colorVel[1] = 0.0f;
				p->colorVel[2] = 0.0f;
				p->alpha = 1.0f;
				p->alphaVel = -1.0f / (0.3f + (rand() & 7) * 0.02f);
				p->radius = 2.0f;
				p->radiusVel = 0.0f;
				p->length = 1.0f;
				p->lengthVel = 0.0f;
				p->rotation = 0.0f;
			}
		}
	}
}

/*
 ==================
 CL_BigTeleportParticles
 ==================
*/
void CL_BigTeleportParticles (const vec3_t origin){

	cparticle_t	*p;
	float		d, angle, s, c, color;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < 4096; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		d = rand() & 31;

		angle = M_PI_TWO * (rand() & 1023) / 1023.0f;
		s = sin(angle);
		c = cos(angle);

		color = 0.1f + (0.2f * frand());

		p->material = cl.media.glowParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = origin[0] + c * d;
		p->origin[1] = origin[1] + s * d;
		p->origin[2] = origin[2] + 8.0f + (rand() % 90);
		p->velocity[0] = c * (70.0f + (rand() & 63));
		p->velocity[1] = s * (70.0f + (rand() & 63));
		p->velocity[2] = -100.0f + (rand() & 31);
		p->accel[0] = -100.0f * c;
		p->accel[1] = -100.0f * s;
		p->accel[2] = 160.0f;
		p->color[0] = color;
		p->color[1] = color;
		p->color[2] = color;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -0.3f / (0.5f + frand() * 0.3f);
		p->radius = 2.0f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
	}
}

/*
 ==================
 CL_TeleporterParticles
 ==================
*/
void CL_TeleporterParticles (const vec3_t origin){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < 8; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = origin[0] - 16.0f + (rand() & 31);
		p->origin[1] = origin[1] - 16.0f + (rand() & 31);
		p->origin[2] = origin[2] - 8.0f + (rand() & 7);
		p->velocity[0] = crand() * 14.0f;
		p->velocity[1] = crand() * 14.0f;
		p->velocity[2] = 80.0f + (rand() & 7);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -40.0f;
		p->color[0] = 0.97f;
		p->color[1] = 0.46f;
		p->color[2] = 0.14f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -0.5f;
		p->radius = 3.0f + (1.5f * crand());
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
	}
}

/*
 ==================
 CL_TrapParticles
 ==================
*/
void CL_TrapParticles (const vec3_t origin){

	cparticle_t	*p;
	vec3_t		start, end, move, vec, dir;
	float		length, dist, vel;
	int			x, y, z;

	if (!cl_particles->integerValue)
		return;

	VectorSet(start, origin[0], origin[1], origin[2] + 18.0f);
	VectorSet(end, origin[0], origin[1], origin[2] + 82.0f);

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dist = 5.0f;
	VectorScale(vec, dist, vec);

	while (length > 0.0f){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = move[0] + crand();
		p->origin[1] = move[1] + crand();
		p->origin[2] = move[2] + crand();
		p->velocity[0] = 15.0f * crand();
		p->velocity[1] = 15.0f * crand();
		p->velocity[2] = 15.0f * crand();
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 40.0f;
		p->color[0] = 0.97f;
		p->color[1] = 0.46f;
		p->color[2] = 0.14f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f / (0.3f + frand() * 0.2f);
		p->radius = 3.0f + (1.5f * crand());
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;

		VectorAdd(move, vec, move);
	}

	for (x = -2; x <= 2; x += 4){
		for (y = -2; y <= 2; y += 4){
			for (z = -2; z <= 4; z += 4){
				p = CL_AllocParticle();
				if (!p)
					return;

				VectorSet(dir, y * 8.0f, x * 8.0f, z * 8.0f);
				VectorNormalizeFast(dir);

				vel = 50 + (rand() & 63);

				p->material = cl.media.energyParticleMaterial;
				p->time = cl.time;
				p->flags = 0;

				p->origin[0] = origin[0] + x + ((rand() & 23) * crand());
				p->origin[1] = origin[1] + y + ((rand() & 23) * crand());
				p->origin[2] = origin[2] + z + ((rand() & 23) * crand()) + 32.0f;
				p->velocity[0] = dir[0] * vel;
				p->velocity[1] = dir[1] * vel;
				p->velocity[2] = dir[2] * vel;
				p->accel[0] = 0.0f;
				p->accel[1] = 0.0f;
				p->accel[2] = -40.0f;
				p->color[0] = 0.96f;
				p->color[1] = 0.46f;
				p->color[2] = 0.14f;
				p->colorVel[0] = 0.0f;
				p->colorVel[1] = 0.0f;
				p->colorVel[2] = 0.0f;
				p->alpha = 1.0f;
				p->alphaVel = -1.0f / (0.3f + (rand() & 7) * 0.02f);
				p->radius = 3.0f + (1.5f * crand());
				p->radiusVel = 0.0f;
				p->length = 1.0f;
				p->lengthVel = 0.0f;
				p->rotation = 0.0f;
			}
		}
	}
}

/*
 ==================
 CL_LogParticles
 ==================
*/
void CL_LogParticles (const vec3_t origin, float r, float g, float b){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < 512; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.glowParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = origin[0] + crand() * 20.0f;
		p->origin[1] = origin[1] + crand() * 20.0f;
		p->origin[2] = origin[2] + crand() * 20.0f;
		p->velocity[0] = crand() * 20.0f;
		p->velocity[1] = crand() * 20.0f;
		p->velocity[2] = crand() * 20.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f / (1.0f + frand() * 0.3f);
		p->radius = 2.0f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
	}
}

/*
 ==================
 CL_ItemRespawnParticles
 ==================
*/
void CL_ItemRespawnParticles (const vec3_t origin){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integerValue)
		return;

	for (i = 0; i < 64; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.glowParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = origin[0] + crand() * 8.0f;
		p->origin[1] = origin[1] + crand() * 8.0f;
		p->origin[2] = origin[2] + crand() * 8.0f;
		p->velocity[0] = crand() * 8.0f;
		p->velocity[1] = crand() * 8.0f;
		p->velocity[2] = crand() * 8.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.0f;
		p->color[1] = 0.3f;
		p->color[2] = 0.5f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f / (1.0f + frand() * 0.3f);
		p->radius = 2.0f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
	}
}

/*
 ==================
 CL_TrackerShellParticles
 ==================
*/
void CL_TrackerShellParticles (const vec3_t origin){

	cparticle_t	*p;
	vec3_t		vec;
	float		d, time;
	float		angle, sy, cy, sp, cp;
	int			i;

	if (!cl_particles->integerValue)
		return;

	time = cl.time * 0.001f;

	for (i = 0; i < NUM_VERTEX_NORMALS; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		angle = time * cl_particleVelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = time * cl_particleVelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);

		vec[0] = cp * cy;
		vec[1] = cp * sy;
		vec[2] = -sp;

		d = sin(time + i) * 64.0f;

		p->material = cl.media.trackerParticleMaterial;
		p->time = cl.time;
		p->flags = PARTICLE_INSTANT;

		p->origin[0] = origin[0] + byteDirs[i][0] * d + vec[0] * 16.0f;
		p->origin[1] = origin[1] + byteDirs[i][1] * d + vec[1] * 16.0f;
		p->origin[2] = origin[2] + byteDirs[i][2] * d + vec[2] * 16.0f;
		p->velocity[0] = 0.0f;
		p->velocity[1] = 0.0f;
		p->velocity[2] = 0.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.0f;
		p->color[1] = 0.0f;
		p->color[2] = 0.0f;
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = 0.0f;
		p->radius = 2.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
	}
}

/*
 ==================
 CL_NukeSmokeParticles
 ==================
*/
void CL_NukeSmokeParticles (const vec3_t origin){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integerValue)
		return;

	flags = 0;

	if (cl_particleVertexLight->integerValue)
		flags |= PARTICLE_VERTEXLIGHT;

	for (i = 0; i < 5; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.smokeParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + crand() * 20.0f;
		p->origin[1] = origin[1] + crand() * 20.0f;
		p->origin[2] = origin[2] + crand() * 20.0f;
		p->velocity[0] = crand() * 20.0f;
		p->velocity[1] = crand() * 20.0f;
		p->velocity[2] = crand() * 20.0f + (25.0f + crand() * 5.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = 0.0f;
		p->color[1] = 0.0f;
		p->color[2] = 0.0f;
		p->colorVel[0] = 0.75f;
		p->colorVel[1] = 0.75f;
		p->colorVel[2] = 0.75f;
		p->alpha = 0.5f;
		p->alphaVel = -(0.1f + frand() * 0.1f);
		p->radius = 60.0f + (30.0f * crand());
		p->radiusVel = 30.0f + (15.0f * crand());
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;
	}
}

/*
 ==================
 CL_WeldingSparkParticles
 ==================
*/
void CL_WeldingSparkParticles (const vec3_t origin, const vec3_t dir, int count, int color){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integerValue)
		return;

	flags = 0;

	if (cl_particleBounce->integerValue)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integerValue)
		flags |= PARTICLE_FRICTION;

	color &= 0xFF;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + dir[0] * 5.0f + (5.0f * crand());
		p->origin[1] = origin[1] + dir[1] * 5.0f + (5.0f * crand());
		p->origin[2] = origin[2] + dir[2] * 5.0f + (5.0f * crand());
		p->velocity[0] = crand() * 20.0f;
		p->velocity[1] = crand() * 20.0f;
		p->velocity[2] = crand() * 20.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -40.0f + (5.0f * crand());
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f / (0.5f + frand() * 0.3f);
		p->radius = 1.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
		p->bounceFactor = 0.7f;
	}
}

/*
 ==================
 CL_TunnelSparkParticles
 ==================
*/
void CL_TunnelSparkParticles (const vec3_t origin, const vec3_t dir, int count, int color){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integerValue)
		return;

	flags = 0;

	if (cl_particleBounce->integerValue)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integerValue)
		flags |= PARTICLE_FRICTION;

	color &= 0xFF;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = flags;

		p->origin[0] = origin[0] + dir[0] * 5.0f + (5.0f * crand());
		p->origin[1] = origin[1] + dir[1] * 5.0f + (5.0f * crand());
		p->origin[2] = origin[2] + dir[2] * 5.0f + (5.0f * crand());
		p->velocity[0] = crand() * 20.0f;
		p->velocity[1] = crand() * 20.0f;
		p->velocity[2] = crand() * 20.0f;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 40.0f + (5.0f * crand());
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f / (0.5f + frand() * 0.3f);
		p->radius = 1.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;
		p->bounceFactor = 0.7f;
	}
}

/*
 ==================
 CL_ForceWallParticles
 ==================
*/
void CL_ForceWallParticles (const vec3_t start, const vec3_t end, int color){

	cparticle_t	*p;
	vec3_t		move, vec;
	float		length, dist;

	if (!cl_particles->integerValue)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = VectorNormalize(vec);

	dist = 4.0f;
	VectorScale(vec, dist, vec);

	color &= 0xFF;

	while (length > 0.0f){
		length -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->material = cl.media.energyParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = move[0] + crand() * 3.0f;
		p->origin[1] = move[1] + crand() * 3.0f;
		p->origin[2] = move[2] + crand() * 3.0f;
		p->velocity[0] = 0.0f;
		p->velocity[1] = 0.0f;
		p->velocity[2] = -40.0f - (crand() * 10.0f);
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f / (3.0f + frand() * 0.5f);
		p->radius = 1.5f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = 0.0f;

		VectorAdd(move, vec, move);
	}
}

/*
 ==================
 CL_SteamParticles
 ==================
*/
void CL_SteamParticles (const vec3_t origin, const vec3_t dir, int count, int color, float magnitude){

	cparticle_t	*p;
	vec3_t		r, u;
	float		rd, ud;
	int			index;
	int			i;

	if (!cl_particles->integerValue)
		return;

	MakeNormalVectors(dir, r, u);

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		rd = crand() * magnitude / 3.0f;
		ud = crand() * magnitude / 3.0f;

		index = (color + (rand() & 7)) & 0xFF;

		p->material = cl.media.steamParticleMaterial;
		p->time = cl.time;
		p->flags = 0;

		p->origin[0] = origin[0] + magnitude * 0.1f * crand();
		p->origin[1] = origin[1] + magnitude * 0.1f * crand();
		p->origin[2] = origin[2] + magnitude * 0.1f * crand();
		p->velocity[0] = dir[0] * magnitude + r[0] * rd + u[0] * ud;
		p->velocity[1] = dir[1] * magnitude + r[1] * rd + u[1] * ud;
		p->velocity[2] = dir[2] * magnitude + r[2] * rd + u[2] * ud;
		p->accel[0] = 0.0f;
		p->accel[1] = 0.0f;
		p->accel[2] = -20.0f;
		p->color[0] = cl_particlePalette[index][0];
		p->color[1] = cl_particlePalette[index][1];
		p->color[2] = cl_particlePalette[index][2];
		p->colorVel[0] = 0.0f;
		p->colorVel[1] = 0.0f;
		p->colorVel[2] = 0.0f;
		p->alpha = 1.0f;
		p->alphaVel = -1.0f / (0.5f + frand() * 0.3f);
		p->radius = 2.0f;
		p->radiusVel = 0.0f;
		p->length = 1.0f;
		p->lengthVel = 0.0f;
		p->rotation = rand() % 360;
	}
}


/*
 ==============================================================================

 ADD PARTICLE TO SCENE

 ==============================================================================
*/


/*
 ==================
 CL_ClearParticles
 ==================
*/
void CL_ClearParticles (){

	int		i;
	byte	palette[] = {
#include "../renderer/palette.h"
	};

	cl_activeParticles = NULL;
	cl_freeParticles = cl_particleList;

	for (i = 0; i < MAX_RENDER_PARTICLES; i++)
		cl_particleList[i].next = &cl_particleList[i + 1];

	cl_particleList[MAX_RENDER_PARTICLES - 1].next = NULL;

	for (i = 0; i < NUM_VERTEX_NORMALS; i++){
		cl_particleVelocities[i][0] = (rand() & 255) * 0.01f;
		cl_particleVelocities[i][1] = (rand() & 255) * 0.01f;
		cl_particleVelocities[i][2] = (rand() & 255) * 0.01f;
	}

	for (i = 0; i < 256; i++){
		cl_particlePalette[i][0] = palette[i * 3 + 0] * (1.0f / 255);
		cl_particlePalette[i][1] = palette[i * 3 + 1] * (1.0f / 255);
		cl_particlePalette[i][2] = palette[i * 3 + 2] * (1.0f / 255);
	}
}

/*
 ==================
 CL_AddParticles
 ==================
*/
void CL_AddParticles (){

	cparticle_t			*p, *next;
	cparticle_t			*active = NULL, *tail = NULL;
	renderParticle_t	renderParticle;
	color_t				modulate;
	vec3_t				org, org2, vel, color;
	vec3_t				ambientLight;
	float				alpha, radius, length;
	float				time, time2, gravity, dot;
	int					contents;
	vec3_t				mins, maxs;
	trace_t				trace;

	if (!cl_particles->integerValue)
		return;

	gravity = cl.playerState->pmove.gravity / 800.0f;

	for (p = cl_activeParticles; p; p = next){
		// Grab next now, so if the particle is freed we still have it
		next = p->next;

		time = (cl.time - p->time) * 0.001f;
		time2 = time * time;

		alpha = p->alpha + p->alphaVel * time;
		radius = p->radius + p->radiusVel * time;
		length = p->length + p->lengthVel * time;

		if (alpha <= 0.0f || radius <= 0.0f || length <= 0.0f){
			// Faded out
			CL_FreeParticle(p);
			continue;
		}

		color[0] = p->color[0] + p->colorVel[0] * time;
		color[1] = p->color[1] + p->colorVel[1] * time;
		color[2] = p->color[2] + p->colorVel[2] * time;

		org[0] = p->origin[0] + p->velocity[0] * time + p->accel[0] * time2;
		org[1] = p->origin[1] + p->velocity[1] * time + p->accel[1] * time2;
		org[2] = p->origin[2] + p->velocity[2] * time + p->accel[2] * time2 * gravity;

		if (p->flags & PARTICLE_UNDERWATER){
			// Underwater particle
			VectorSet(org2, org[0], org[1], org[2] + radius);

			if (!(CL_PointContents(org2, -1) & MASK_WATER)){
				// Not underwater
				CL_FreeParticle(p);
				continue;
			}
		}

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else {
			tail->next = p;
			tail = p;
		}

		if (p->flags & PARTICLE_FRICTION){
			// Water friction affected particle
			contents = CL_PointContents(org, -1);
			if (contents & MASK_WATER){
				// Add friction
				if (contents & CONTENTS_WATER){
					VectorScale(p->velocity, 0.25f, p->velocity);
					VectorScale(p->accel, 0.25f, p->accel);
				}
				if (contents & CONTENTS_SLIME){
					VectorScale(p->velocity, 0.20f, p->velocity);
					VectorScale(p->accel, 0.20f, p->accel);
				}
				if (contents & CONTENTS_LAVA){
					VectorScale(p->velocity, 0.10f, p->velocity);
					VectorScale(p->accel, 0.10f, p->accel);
				}

				length = 1.0f;

				// Don't add friction again
				p->flags &= ~PARTICLE_FRICTION;

				// Reset
				p->time = cl.time;
				VectorCopy(org, p->origin);
				VectorCopy(color, p->color);
				p->alpha = alpha;
				p->radius = radius;

				// Don't stretch
				p->flags &= ~PARTICLE_STRETCH;

				p->length = length;
				p->lengthVel = 0;
			}
		}

		if (p->flags & PARTICLE_BOUNCE){
			// Bouncy particle
			VectorSet(mins, -radius, -radius, -radius);
			VectorSet(maxs, radius, radius, radius);

			trace = CL_Trace(p->lastOrigin, mins, maxs, org, cl.clientNum, MASK_SOLID, true, NULL);
			if (trace.fraction != 0.0f && trace.fraction != 1.0f){
				// Reflect velocity
				time = cl.time - (cls.frameTime + cls.frameTime * trace.fraction) * 1000;
				time = (time - p->time) * 0.001f;

				VectorSet(vel, p->velocity[0], p->velocity[1], p->velocity[2] + p->accel[2] * gravity * time);
				VectorReflect(vel, trace.plane.normal, p->velocity);
				VectorScale(p->velocity, p->bounceFactor, p->velocity);

				// Check for stop or slide along the plane
				if (trace.plane.normal[2] > 0 && p->velocity[2] < 1){
					if (trace.plane.normal[2] == 1.0f){
						VectorClear(p->velocity);
						VectorClear(p->accel);

						p->flags &= ~PARTICLE_BOUNCE;
					}
					else {
						// FIXME: check for new plane or free fall
						dot = DotProduct(p->velocity, trace.plane.normal);
						VectorMA(p->velocity, -dot, trace.plane.normal, p->velocity);

						dot = DotProduct(p->accel, trace.plane.normal);
						VectorMA(p->accel, -dot, trace.plane.normal, p->accel);
					}
				}

				VectorCopy(trace.endpos, org);
				length = 1.0f;

				// Reset
				p->time = cl.time;
				VectorCopy(org, p->origin);
				VectorCopy(color, p->color);
				p->alpha = alpha;
				p->radius = radius;

				// Don't stretch
				p->flags &= ~PARTICLE_STRETCH;

				p->length = length;
				p->lengthVel = 0.0f;
			}
		}

		// Save current origin if needed
		if (p->flags & (PARTICLE_BOUNCE | PARTICLE_STRETCH)){
			VectorCopy(p->lastOrigin, org2);
			VectorCopy(org, p->lastOrigin);	// FIXME: pause
		}

		if (p->flags & PARTICLE_VERTEXLIGHT){
			// Vertex lit particle
			R_LightForPoint(org, ambientLight);

			VectorMultiply(color, ambientLight, color);
		}

		// Clamp color and alpha and convert to byte
		modulate[0] = 255 * Clamp(color[0], 0.0f, 1.0f);
		modulate[1] = 255 * Clamp(color[1], 0.0f, 1.0f);
		modulate[2] = 255 * Clamp(color[2], 0.0f, 1.0f);
		modulate[3] = 255 * Clamp(alpha, 0.0f, 1.0f);

		if (p->flags & PARTICLE_INSTANT){
			// Instant particle
			p->alpha = 0.0f;
			p->alphaVel = 0.0f;
		}

		// Copy values into for the render particle
		renderParticle.material = p->material;
		VectorCopy(org, renderParticle.origin);
		VectorCopy(org2, renderParticle.oldOrigin);
		renderParticle.radius = radius;
		renderParticle.length = length;
		renderParticle.rotation = p->rotation;
		MakeRGBA(renderParticle.modulate, modulate[0], modulate[1], modulate[2], modulate[3]);

		// Send the particle to the renderer
		R_AddParticleToScene(&renderParticle);
	}

	cl_activeParticles = active;
}