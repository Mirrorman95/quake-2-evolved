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
// r_batch.c - Geometry batching for back-end
//


#include "r_local.h"


/*
 ==================
 RB_CheckMeshOverflow
 ==================
*/
void RB_CheckMeshOverflow (int numIndices, int numVertices){

	if (backEnd.numIndices + numIndices <= MAX_INDICES && backEnd.numVertices + numVertices <= MAX_VERTICES)
		return;

	RB_RenderBatch();

	if (numIndices > MAX_INDICES)
		Com_Error(ERR_DROP, "RB_CheckOverflow: numIndices > MAX_INDICES (%i > %i)", numIndices, MAX_INDICES);
	if (numVertices > MAX_VERTICES)
		Com_Error(ERR_DROP, "RB_CheckOverflow: numVertices > MAX_VERTICES (%i > %i)", numVertices, MAX_VERTICES);

	RB_SetupBatch(backEnd.entity, backEnd.material, backEnd.drawBatch);
}

/*
 ==================
 RB_SetupBatch
 ==================
*/
void RB_SetupBatch (renderEntity_t *entity, material_t *material, void (*drawBatch)()){

	// Set the batch state
	backEnd.entity = entity;
	backEnd.material = material;

	backEnd.vertexBuffer = NULL;
	backEnd.vertexPointer = backEnd.vertices;

	// Set the draw function
	backEnd.drawBatch = drawBatch;
}

/*
 ==================
 RB_RenderBatch
 ==================
*/
void RB_RenderBatch (){

	if (!backEnd.numIndices || !backEnd.numVertices)
		return;

	// Deform geometry
	RB_Deform(backEnd.material);

	// Set up the vertex array
	qglVertexPointer(3, GL_FLOAT, sizeof(glVertex_t), GL_VERTEX_XYZ(backEnd.vertexPointer));

	// Draw the batch
	backEnd.drawBatch();

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	// Clear the arrays
	backEnd.numIndices = 0;
	backEnd.numVertices = 0;
}


/*
 ==============================================================================

 GENERIC BATCHING

 ==============================================================================
*/


/*
 ==================
 RB_BatchSurface
 ==================
*/
static void RB_BatchSurface (meshData_t *data){

	surface_t		*surface = (surface_t *)data;
	surfTriangle_t	*triangle;
	surfVertex_t	*vertex;
	glIndex_t		*indices;
	glVertex_t		*vertices;
	int				i;

	// Check for overflow
	RB_CheckMeshOverflow(surface->numTriangles * 3, surface->numVertices);

	// Batch indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
		indices[0] = backEnd.numVertices + triangle->index[0];
		indices[1] = backEnd.numVertices + triangle->index[1];
		indices[2] = backEnd.numVertices + triangle->index[2];

		indices += 3;
	}

	backEnd.numIndices += surface->numTriangles * 3;

	// Batch vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		vertices->xyz[0] = vertex->xyz[0];
		vertices->xyz[1] = vertex->xyz[1];
		vertices->xyz[2] = vertex->xyz[2];
		vertices->normal[0] = vertex->normal[0];
		vertices->normal[1] = vertex->normal[1];
		vertices->normal[2] = vertex->normal[2];
		vertices->tangents[0][0] = vertex->tangents[0][0];
		vertices->tangents[0][1] = vertex->tangents[0][1];
		vertices->tangents[0][2] = vertex->tangents[0][2];
		vertices->tangents[1][0] = vertex->tangents[1][0];
		vertices->tangents[1][1] = vertex->tangents[1][1];
		vertices->tangents[1][2] = vertex->tangents[1][2];
		vertices->st[0] = vertex->st[0];
		vertices->st[1] = vertex->st[1];
		vertices->color[0] = vertex->color[0];
		vertices->color[1] = vertex->color[1];
		vertices->color[2] = vertex->color[2];
		vertices->color[3] = vertex->color[3];

		vertices++;
	}

	backEnd.numVertices += surface->numVertices;
}

/*
 ==================
 R_BatchAliasModel
 ==================
*/
static void R_BatchAliasModel (meshData_t *data){

	mdlSurface_t	*surface = (mdlSurface_t *)data;
	mdlXyzNormal_t	*curXyzNormal, *oldXyzNormal;
	mdlTriangle_t	*triangle;
	mdlSt_t			*st;
	glIndex_t		*indices;
	glVertex_t		*vertices;
	float			backLerp;
	int				i;

	// Interpolate frames
	curXyzNormal = surface->xyzNormals + surface->numVertices * backEnd.entity->frame;
	oldXyzNormal = surface->xyzNormals + surface->numVertices * backEnd.entity->oldFrame;

	if (backEnd.entity->frame == backEnd.entity->oldFrame)
		backLerp = 0.0;
	else
		backLerp = backEnd.entity->backLerp;

	// Check for overflow
	RB_CheckMeshOverflow(surface->numTriangles * 3, surface->numVertices);

	// Batch indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 0, triangle = surface->triangles; i < surface->numTriangles; i++, triangle++){
		indices[0] = backEnd.numVertices + triangle->index[0];
		indices[1] = backEnd.numVertices + triangle->index[1];
		indices[2] = backEnd.numVertices + triangle->index[2];

		indices += 3;
	}

	backEnd.numIndices += surface->numTriangles * 3;

	// Batch vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	if (backLerp == 0.0f){
		// Optimized case
		for (i = 0, st = surface->st; i < surface->numVertices; i++, curXyzNormal++, st++){
			vertices->xyz[0] = curXyzNormal->xyz[0];
			vertices->xyz[1] = curXyzNormal->xyz[1];
			vertices->xyz[2] = curXyzNormal->xyz[2];
			vertices->normal[0] = curXyzNormal->normal[0];
			vertices->normal[1] = curXyzNormal->normal[1];
			vertices->normal[2] = curXyzNormal->normal[2];
			vertices->tangents[0][0] = curXyzNormal->tangents[0][0];
			vertices->tangents[0][1] = curXyzNormal->tangents[0][1];
			vertices->tangents[0][2] = curXyzNormal->tangents[0][2];
			vertices->tangents[1][0] = curXyzNormal->tangents[1][0];
			vertices->tangents[1][1] = curXyzNormal->tangents[1][1];
			vertices->tangents[1][2] = curXyzNormal->tangents[1][2];
			vertices->st[0] = st->st[0];
			vertices->st[1] = st->st[1];
			vertices->color[0] = 255;
			vertices->color[1] = 255;
			vertices->color[2] = 255;
			vertices->color[3] = 255;

			vertices++;
		}
	}
	else {
		// General case
		for (i = 0, st = surface->st; i < surface->numVertices; i++, curXyzNormal++, oldXyzNormal++, st++){
			vertices->xyz[0] = curXyzNormal->xyz[0] + (oldXyzNormal->xyz[0] - curXyzNormal->xyz[0]) * backLerp;
			vertices->xyz[1] = curXyzNormal->xyz[1] + (oldXyzNormal->xyz[1] - curXyzNormal->xyz[1]) * backLerp;
			vertices->xyz[2] = curXyzNormal->xyz[2] + (oldXyzNormal->xyz[2] - curXyzNormal->xyz[2]) * backLerp;
			vertices->normal[0] = curXyzNormal->normal[0] + (oldXyzNormal->normal[0] - curXyzNormal->normal[0]) * backLerp;
			vertices->normal[1] = curXyzNormal->normal[1] + (oldXyzNormal->normal[1] - curXyzNormal->normal[1]) * backLerp;
			vertices->normal[2] = curXyzNormal->normal[2] + (oldXyzNormal->normal[2] - curXyzNormal->normal[2]) * backLerp;
			vertices->tangents[0][0] = curXyzNormal->tangents[0][0] + (oldXyzNormal->tangents[0][0] - curXyzNormal->tangents[0][0]) * backLerp;
			vertices->tangents[0][1] = curXyzNormal->tangents[0][1] + (oldXyzNormal->tangents[0][1] - curXyzNormal->tangents[0][1]) * backLerp;
			vertices->tangents[0][2] = curXyzNormal->tangents[0][2] + (oldXyzNormal->tangents[0][2] - curXyzNormal->tangents[0][2]) * backLerp;
			vertices->tangents[1][0] = curXyzNormal->tangents[1][0] + (oldXyzNormal->tangents[1][0] - curXyzNormal->tangents[1][0]) * backLerp;
			vertices->tangents[1][1] = curXyzNormal->tangents[1][1] + (oldXyzNormal->tangents[1][1] - curXyzNormal->tangents[1][1]) * backLerp;
			vertices->tangents[1][2] = curXyzNormal->tangents[1][2] + (oldXyzNormal->tangents[1][2] - curXyzNormal->tangents[1][2]) * backLerp;
			vertices->st[0] = st->st[0];
			vertices->st[1] = st->st[1];
			vertices->color[0] = 255;
			vertices->color[1] = 255;
			vertices->color[2] = 255;
			vertices->color[3] = 255;

			VectorNormalizeFast(vertices->normal);
			VectorNormalizeFast(vertices->tangents[0]);
			VectorNormalizeFast(vertices->tangents[1]);

			vertices++;
		}
	}

	backEnd.numVertices += surface->numVertices;
}

/*
 ==================
 RB_BatchSprite

 TODO: utilize spriteOriented
 ==================
*/
static void RB_BatchSprite (){

	glIndex_t	*indices;
	glVertex_t	*vertices;
	vec3_t		axis[3];
	int			i;

	if (backEnd.entity->spriteRotation){
		// Rotate it around its normal
		RotatePointAroundVector(axis[1], rg.renderView.axis[0], rg.renderView.axis[1], backEnd.entity->spriteRotation);
		CrossProduct(rg.renderView.axis[0], axis[1], axis[2]);

		// The normal should point at the viewer
		VectorNegate(rg.renderView.axis[0], axis[0]);

		// Scale the axes by radius
		VectorScale(axis[1], backEnd.entity->spriteRadius, axis[1]);
		VectorScale(axis[2], backEnd.entity->spriteRadius, axis[2]);
	}
	else {
		// The normal should point at the viewer
		VectorNegate(rg.renderView.axis[0], axis[0]);

		// Scale the axes by radius
		VectorScale(rg.renderView.axis[1], backEnd.entity->spriteRadius, axis[1]);
		VectorScale(rg.renderView.axis[2], backEnd.entity->spriteRadius, axis[2]);
	}

	// Check for overflow
	RB_CheckMeshOverflow(6, 4);
	
	// Batch indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 2; i < 4; i++){
		indices[0] = backEnd.numVertices + 0;
		indices[1] = backEnd.numVertices + i-1;
		indices[2] = backEnd.numVertices + i;

		indices += 3;
	}

	backEnd.numIndices += 6;

	// Batch vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	vertices[0].xyz[0] = backEnd.entity->origin[0] + axis[1][0] + axis[2][0];
	vertices[0].xyz[1] = backEnd.entity->origin[1] + axis[1][1] + axis[2][1];
	vertices[0].xyz[2] = backEnd.entity->origin[2] + axis[1][2] + axis[2][2];
	vertices[1].xyz[0] = backEnd.entity->origin[0] - axis[1][0] + axis[2][0];
	vertices[1].xyz[1] = backEnd.entity->origin[1] - axis[1][1] + axis[2][1];
	vertices[1].xyz[2] = backEnd.entity->origin[2] - axis[1][2] + axis[2][2];
	vertices[2].xyz[0] = backEnd.entity->origin[0] - axis[1][0] - axis[2][0];
	vertices[2].xyz[1] = backEnd.entity->origin[1] - axis[1][1] - axis[2][1];
	vertices[2].xyz[2] = backEnd.entity->origin[2] - axis[1][2] - axis[2][2];
	vertices[3].xyz[0] = backEnd.entity->origin[0] + axis[1][0] - axis[2][0];
	vertices[3].xyz[1] = backEnd.entity->origin[1] + axis[1][1] - axis[2][1];
	vertices[3].xyz[2] = backEnd.entity->origin[2] + axis[1][2] - axis[2][2];

	vertices[0].st[0] = 0.0f;
	vertices[0].st[1] = 0.0f;
	vertices[1].st[0] = 1.0f;
	vertices[1].st[1] = 0.0f;
	vertices[2].st[0] = 1.0f;
	vertices[2].st[1] = 1.0f;
	vertices[3].st[0] = 0.0f;
	vertices[3].st[1] = 1.0f;

	for (i = 0; i < 4; i++){
		vertices->normal[0] = axis[0][0];
		vertices->normal[1] = axis[0][1];
		vertices->normal[2] = axis[0][2];
		vertices->color[0] = 255;
		vertices->color[1] = 255;
		vertices->color[2] = 255;
		vertices->color[3] = 255;

		vertices++;
	}

	backEnd.numVertices += 4;
}

/*
 ==================
 RB_BatchBeam
 ==================
*/
static void RB_BatchBeam (){

	glIndex_t	*indices;
	glVertex_t	*vertices;
	vec3_t		axis[3];
	float		length;
	int			i;

	// Find orientation vectors
	VectorSubtract(rg.renderView.origin, backEnd.entity->origin, axis[0]);
	VectorSubtract(backEnd.entity->beamEnd, backEnd.entity->origin, axis[1]);

	CrossProduct(axis[0], axis[1], axis[2]);
	VectorNormalizeFast(axis[2]);

	// Find normal
	CrossProduct(axis[1], axis[2], axis[0]);
	VectorNormalizeFast(axis[0]);

	// Scale by radius
	VectorScale(axis[2], backEnd.entity->beamWidth * 0.5f, axis[2]);

	// Find segment length
	if (!backEnd.entity->beamLength)
		length = VectorLength(axis[1]) / 100.0f;
	else
		length = VectorLength(axis[1]) / backEnd.entity->beamLength;

	// Check for overflow
	RB_CheckMeshOverflow(6, 4);
	
	// Batch indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 2; i < 4; i++){
		indices[0] = backEnd.numVertices + 0;
		indices[1] = backEnd.numVertices + i-1;
		indices[2] = backEnd.numVertices + i;

		indices += 3;
	}

	backEnd.numIndices += 6;

	// Batch vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	vertices[0].xyz[0] = backEnd.entity->origin[0] + axis[2][0];
	vertices[0].xyz[1] = backEnd.entity->origin[1] + axis[2][1];
	vertices[0].xyz[2] = backEnd.entity->origin[2] + axis[2][2];
	vertices[1].xyz[0] = backEnd.entity->beamEnd[0] + axis[2][0];
	vertices[1].xyz[1] = backEnd.entity->beamEnd[1] + axis[2][1];
	vertices[1].xyz[2] = backEnd.entity->beamEnd[2] + axis[2][2];
	vertices[2].xyz[0] = backEnd.entity->beamEnd[0] - axis[2][0];
	vertices[2].xyz[1] = backEnd.entity->beamEnd[1] - axis[2][1];
	vertices[2].xyz[2] = backEnd.entity->beamEnd[2] - axis[2][2];
	vertices[3].xyz[0] = backEnd.entity->origin[0] - axis[2][0];
	vertices[3].xyz[1] = backEnd.entity->origin[1] - axis[2][1];
	vertices[3].xyz[2] = backEnd.entity->origin[2] - axis[2][2];

	vertices[0].st[0] = 0.0f;
	vertices[0].st[1] = 0.0f;
	vertices[1].st[0] = length;
	vertices[1].st[1] = 0.0f;
	vertices[2].st[0] = length;
	vertices[2].st[1] = 1.0f;
	vertices[3].st[0] = 0.0f;
	vertices[3].st[1] = 1.0f;

	for (i = 0; i < 4; i++){
		vertices->normal[0] = axis[0][0];
		vertices->normal[1] = axis[0][1];
		vertices->normal[2] = axis[0][2];
		vertices->color[0] = 255;
		vertices->color[1] = 255;
		vertices->color[2] = 255;
		vertices->color[3] = 255;

		vertices++;
	}

	backEnd.numVertices += 4;
}

/*
 ==================
 RB_BatchParticle
 ==================
*/
static void RB_BatchParticle (meshData_t *data){

	renderParticle_t	*particle = (renderParticle_t *)data;
	glIndex_t			*indices;
	glVertex_t			*vertices;
	vec3_t				axis[3];
	int					i;

	// Check for overflow
	RB_CheckMeshOverflow(6, 4);
	
	// Batch indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 2; i < 4; i++){
		indices[0] = backEnd.numVertices + 0;
		indices[1] = backEnd.numVertices + i-1;
		indices[2] = backEnd.numVertices + i;

		indices += 3;
	}

	backEnd.numIndices += 6;

	// Batch vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	if (particle->length != 1.0f){
		// Find orientation vectors
		VectorSubtract(rg.renderView.origin, particle->origin, axis[0]);
		VectorSubtract(particle->oldOrigin, particle->origin, axis[1]);
		CrossProduct(axis[0], axis[1], axis[2]);

		VectorNormalizeFast(axis[1]);
		VectorNormalizeFast(axis[2]);

		// Find normal
		CrossProduct(axis[1], axis[2], axis[0]);
		VectorNormalizeFast(axis[0]);

		VectorMA(particle->origin, -particle->length, axis[1], particle->oldOrigin);
		VectorScale(axis[2], particle->radius, axis[2]);

		vertices[0].xyz[0] = particle->oldOrigin[0] + axis[2][0];
		vertices[0].xyz[1] = particle->oldOrigin[1] + axis[2][1];
		vertices[0].xyz[2] = particle->oldOrigin[2] + axis[2][2];
		vertices[1].xyz[0] = particle->origin[0] + axis[2][0];
		vertices[1].xyz[1] = particle->origin[1] + axis[2][1];
		vertices[1].xyz[2] = particle->origin[2] + axis[2][2];
		vertices[2].xyz[0] = particle->origin[0] - axis[2][0];
		vertices[2].xyz[1] = particle->origin[1] - axis[2][1];
		vertices[2].xyz[2] = particle->origin[2] - axis[2][2];
		vertices[3].xyz[0] = particle->oldOrigin[0] - axis[2][0];
		vertices[3].xyz[1] = particle->oldOrigin[1] - axis[2][1];
		vertices[3].xyz[2] = particle->oldOrigin[2] - axis[2][2];
	}
	else {
		if (particle->rotation){
			// Rotate it around its normal
			RotatePointAroundVector(axis[1], rg.renderView.axis[0], rg.renderView.axis[1], particle->rotation);
			CrossProduct(rg.renderView.axis[0], axis[1], axis[2]);

			// The normal should point at the viewer
			VectorNegate(rg.renderView.axis[0], axis[0]);

			// Scale the axes by radius
			VectorScale(axis[1], particle->radius, axis[1]);
			VectorScale(axis[2], particle->radius, axis[2]);
		}
		else {
			// The normal should point at the viewer
			VectorNegate(rg.renderView.axis[0], axis[0]);

			// Scale the axes by radius
			VectorScale(rg.renderView.axis[1], particle->radius, axis[1]);
			VectorScale(rg.renderView.axis[2], particle->radius, axis[2]);
		}

		vertices[0].xyz[0] = particle->origin[0] + axis[1][0] + axis[2][0];
		vertices[0].xyz[1] = particle->origin[1] + axis[1][1] + axis[2][1];
		vertices[0].xyz[2] = particle->origin[2] + axis[1][2] + axis[2][2];
		vertices[1].xyz[0] = particle->origin[0] - axis[1][0] + axis[2][0];
		vertices[1].xyz[1] = particle->origin[1] - axis[1][1] + axis[2][1];
		vertices[1].xyz[2] = particle->origin[2] - axis[1][2] + axis[2][2];
		vertices[2].xyz[0] = particle->origin[0] - axis[1][0] - axis[2][0];
		vertices[2].xyz[1] = particle->origin[1] - axis[1][1] - axis[2][1];
		vertices[2].xyz[2] = particle->origin[2] - axis[1][2] - axis[2][2];
		vertices[3].xyz[0] = particle->origin[0] + axis[1][0] - axis[2][0];
		vertices[3].xyz[1] = particle->origin[1] + axis[1][1] - axis[2][1];
		vertices[3].xyz[2] = particle->origin[2] + axis[1][2] - axis[2][2];
	}

	vertices[0].st[0] = 0.0f;
	vertices[0].st[1] = 0.0f;
	vertices[1].st[0] = 1.0f;
	vertices[1].st[1] = 0.0f;
	vertices[2].st[0] = 1.0f;
	vertices[2].st[1] = 1.0f;
	vertices[3].st[0] = 0.0f;
	vertices[3].st[1] = 1.0f;

	for (i = 0; i < 4; i++){
		vertices->normal[0] = axis[0][0];
		vertices->normal[1] = axis[0][1];
		vertices->normal[2] = axis[0][2];
		vertices->color[0] = particle->modulate[0];
		vertices->color[1] = particle->modulate[1];
		vertices->color[2] = particle->modulate[2];
		vertices->color[3] = particle->modulate[3];

		vertices++;
	}

	backEnd.numVertices += 4;
}

/*
 ==================

 ==================
*/
static void RB_BatchDecal (meshData_t *data){

}

/*
 ==================
 RB_BatchGeometry
 ==================
*/
void RB_BatchGeometry (meshType_t type, meshData_t *data){

	backEnd.meshData = data;

	switch (type){
	case MESH_SURFACE:
		RB_BatchSurface(data);
		break;
	case MESH_ALIASMODEL:
		R_BatchAliasModel(data);
		break;
	case MESH_SPRITE:
		RB_BatchSprite();
		break;
	case MESH_BEAM:
		RB_BatchBeam();
		break;
	case MESH_PARTICLE:
		RB_BatchParticle(data);
		break;
	case MESH_DECAL:
		RB_BatchDecal(data);
		break;
	default:
		Com_Error(ERR_DROP, "RB_BatchGeometry: bad mesh type (%i)", type);
	}
}


/*
 ==============================================================================

 SHADOW BATCHING

 ==============================================================================
*/


/*
 ==================

 ==================
*/
static void RB_BatchSurfaceShadow (meshData_t *data){

}

/*
 ==================

 ==================
*/
static void RB_BatchAliasModelShadow (meshData_t *data){

}

/*
 ==================
 RB_BatchShadowGeometry
 ==================
*/
void RB_BatchShadowGeometry (meshType_t type, meshData_t *data){

	backEnd.meshData = data;

	switch (type){
	case MESH_SURFACE:
		RB_BatchSurfaceShadow(data);
		break;
	case MESH_ALIASMODEL:
		RB_BatchAliasModelShadow(data);
		break;
	default:
		Com_Error(ERR_DROP, "RB_BatchShadowGeometry: bad mesh type (%i)", type);
	}
}