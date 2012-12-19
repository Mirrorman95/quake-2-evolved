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
// r_deform.c - Geometry deforms used by materials
//


#include "r_local.h"


/*
 ==================
 RB_DeformExpand
 ==================
*/
static void RB_DeformExpand (material_t *material){

	glVertex_t	*vertices = backEnd.vertices;
	float		expand;
	int			i;

	rg.pc.deformExpand++;
	rg.pc.deformIndices += backEnd.numIndices;
	rg.pc.deformVertices += backEnd.numVertices;

	expand = material->expressionRegisters[material->deformRegisters[0]];

	for (i = 0; i < backEnd.numVertices; i++, vertices++)
		VectorMA(vertices->xyz, expand, vertices->normal, vertices->xyz);
}

/*
 ==================
 RB_DeformMove
 ==================
*/
static void RB_DeformMove (material_t *material){

	glVertex_t	*vertices = backEnd.vertices;
	vec3_t		move;
	int			i;

	rg.pc.deformMove++;
	rg.pc.deformIndices += backEnd.numIndices;
	rg.pc.deformVertices += backEnd.numVertices;

	move[0] = material->expressionRegisters[material->deformRegisters[0]];
	move[1] = material->expressionRegisters[material->deformRegisters[1]];
	move[2] = material->expressionRegisters[material->deformRegisters[2]];

	for (i = 0; i < backEnd.numVertices; i++, vertices++)
		VectorAdd(vertices->xyz, move, vertices->xyz);
}

/*
 ==================
 RB_DeformSprite
 ==================
*/
static void RB_DeformSprite (material_t *material){

	glIndex_t	*indices = backEnd.indices;
	glVertex_t	*vertices = backEnd.vertices;
	vec3_t		lVector, uVector;
	vec3_t		center;
	float		radius;
	int			i, j;

	if (backEnd.numIndices != (backEnd.numVertices >> 2) * 6){
		Com_DPrintf(S_COLOR_YELLOW "Material '%s' has 'deform sprite' with an odd index count\n", material->name);
		return;
	}
	if (backEnd.numVertices & 3){
		Com_DPrintf(S_COLOR_YELLOW "Material '%s' has 'deform sprite' with an odd vertex count\n", material->name);
		return;
	}

	rg.pc.deformSprite++;
	rg.pc.deformIndices += backEnd.numIndices;
	rg.pc.deformVertices += backEnd.numVertices;

	// Assume all the triangles are independent quads
	for (i = 0; i < backEnd.numVertices; i += 4){
		// Compute center
		center[0] = (vertices[0].xyz[0] + vertices[1].xyz[0] + vertices[2].xyz[0] + vertices[3].xyz[0]) * 0.25f;
		center[1] = (vertices[0].xyz[1] + vertices[1].xyz[1] + vertices[2].xyz[1] + vertices[3].xyz[1]) * 0.25f;
		center[2] = (vertices[0].xyz[2] + vertices[1].xyz[2] + vertices[2].xyz[2] + vertices[3].xyz[2]) * 0.25f;

		// Compute radius
		radius = DistanceFast(center, vertices->xyz) * M_SQRT_1OVER2;

		// Compute left and up vectors
		VectorScale(backEnd.localParms.viewAxis[1], radius, lVector);
		VectorScale(backEnd.localParms.viewAxis[2], radius, uVector);

		// Modify indices
		indices[0] = i;
		indices[1] = i + 1;
		indices[2] = i + 3;
		indices[3] = i + 3;
		indices[4] = i + 1;
		indices[5] = i + 2;

		indices += 6;

		// Modify vertices
		vertices[0].xyz[0] = center[0] + lVector[0] + uVector[0];
		vertices[0].xyz[1] = center[1] + lVector[1] + uVector[1];
		vertices[0].xyz[2] = center[2] + lVector[2] + uVector[2];
		vertices[1].xyz[0] = center[0] - lVector[0] + uVector[0];
		vertices[1].xyz[1] = center[1] - lVector[1] + uVector[1];
		vertices[1].xyz[2] = center[2] - lVector[2] + uVector[2];
		vertices[2].xyz[0] = center[0] - lVector[0] - uVector[0];
		vertices[2].xyz[1] = center[1] - lVector[1] - uVector[1];
		vertices[2].xyz[2] = center[2] - lVector[2] - uVector[2];
		vertices[3].xyz[0] = center[0] + lVector[0] - uVector[0];
		vertices[3].xyz[1] = center[1] + lVector[1] - uVector[1];
		vertices[3].xyz[2] = center[2] + lVector[2] - uVector[2];

		vertices[0].st[0] = 0.0f;
		vertices[0].st[1] = 0.0f;
		vertices[1].st[0] = 1.0f;
		vertices[1].st[1] = 0.0f;
		vertices[2].st[0] = 1.0f;
		vertices[2].st[1] = 1.0f;
		vertices[3].st[0] = 0.0f;
		vertices[3].st[1] = 1.0f;

		for (j = 0; j < 4; j++){
			vertices->color[0] = 255;
			vertices->color[1] = 255;
			vertices->color[2] = 255;
			vertices->color[3] = 255;

			vertices++;
		}
	}
}

/*
 ==================
 
 ==================
*/
static void RB_DeformTube (material_t *material){

	static int	edgeIndex[6][2] = {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5}, {5, 3}};
	glIndex_t	*indices = backEnd.indices;
	glVertex_t	*vertices = backEnd.vertices;
	vec3_t		tmp, tmp2, dir, mid[2];
	float		length, lengths[2];
	int			edges[2];
	int			index0, index1, index2, index3;
	int			i, j;

	if (backEnd.numIndices != (backEnd.numVertices >> 2) * 6){
		Com_DPrintf(S_COLOR_YELLOW "Material '%s' has 'deform tube' with an odd index count\n", material->name);
		return;
	}
	if (backEnd.numVertices & 3){
		Com_DPrintf(S_COLOR_YELLOW "Material '%s' has 'deform tube' with an odd vertex count\n", material->name);
		return;
	}

	rg.pc.deformTube++;
	rg.pc.deformIndices += backEnd.numIndices;
	rg.pc.deformVertices += backEnd.numVertices;

	// Assume all the triangles are independent quads
	for (i = 0; i < backEnd.numVertices; i += 4){
		// Identify the two shortest edges
		edges[0] = edges[1] = 0;
		lengths[0] = lengths[1] = M_INFINITY;

		for (j = 0; j < 6; j++){
			VectorSubtract(vertices[indices[edgeIndex[j][0]]].xyz, vertices[indices[edgeIndex[j][1]]].xyz, tmp);

			length = VectorLengthSquared(tmp);

			if (length < lengths[0]){
				edges[1] = edges[0];
				lengths[1] = lengths[0];

				edges[0] = j;
				lengths[0] = length;
			}
			else if (length < lengths[1]){
				edges[1] = j;
				lengths[1] = length;
			}
		}

		// Get indices
		index0 = indices[edgeIndex[edges[0]][0]];
		index1 = indices[edgeIndex[edges[0]][1]];
		index2 = indices[edgeIndex[edges[1]][0]];
		index3 = indices[edgeIndex[edges[1]][1]];

		indices += 6;

		// Compute lengths
		lengths[0] = SqrtFast(lengths[0]) * 0.5f;
		lengths[1] = SqrtFast(lengths[1]) * 0.5f;

		// Compute mid points
		VectorAverage(vertices[index0].xyz, vertices[index1].xyz, mid[0]);
		VectorAverage(vertices[index2].xyz, vertices[index3].xyz, mid[1]);

		// Compute direction
		VectorSubtract(mid[1], mid[0], tmp2);

		CrossProduct(tmp2, backEnd.localParms.viewAxis[0], dir);
		VectorNormalizeFast(dir);

		// Modify vertices, leaving texture coords unchanged

		for (j = 0; j < 4; j++){
			vertices->color[index0] = 255;
			vertices->color[index1] = 255;
			vertices->color[index2] = 255;
			vertices->color[index3] = 255;

			vertices++;
		}
	}
}

/*
 ==================
 
 ==================
*/
static void RB_DeformBeam (material_t *material){

	static int	edgeIndex[6][2] = {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5}, {5, 3}};
	glIndex_t	*indices = backEnd.indices;
	glVertex_t	*vertices = backEnd.vertices;
	vec3_t		tmp, tmp2, tmp3, dir, mid[2];
	float		length, lengths[2];
	int			edges[2];
	int			index0, index1, index2, index3;
	int			i, j;

	if (backEnd.numIndices != (backEnd.numVertices >> 2) * 6){
		Com_DPrintf(S_COLOR_YELLOW "Material '%s' has 'deform beam' with an odd index count\n", material->name);
		return;
	}
	if (backEnd.numVertices & 3){
		Com_DPrintf(S_COLOR_YELLOW "Material '%s' has 'deform beam' with an odd vertex count\n", material->name);
		return;
	}

	rg.pc.deformBeam++;
	rg.pc.deformIndices += backEnd.numIndices;
	rg.pc.deformVertices += backEnd.numVertices;

	// Assume all the triangles are independent quads
	for (i = 0; i < backEnd.numVertices; i += 4){
		// Identify the two shortest edges
		edges[0] = edges[1] = 0;
		lengths[0] = lengths[1] = M_INFINITY;

		for (j = 0; j < 6; j++){
			VectorSubtract(vertices[indices[edgeIndex[j][0]]].xyz, vertices[indices[edgeIndex[j][1]]].xyz, tmp);

			length = VectorLengthSquared(tmp);

			if (length < lengths[0]){
				edges[1] = edges[0];
				lengths[1] = lengths[0];

				edges[0] = j;
				lengths[0] = length;
			}
			else if (length < lengths[1]){
				edges[1] = j;
				lengths[1] = length;
			}
		}

		// Get indices
		index0 = indices[edgeIndex[edges[0]][0]];
		index1 = indices[edgeIndex[edges[0]][1]];
		index2 = indices[edgeIndex[edges[1]][0]];
		index3 = indices[edgeIndex[edges[1]][1]];

		indices += 6;

		// Compute lengths
		lengths[0] = SqrtFast(lengths[0]) * 0.5f;
		lengths[1] = SqrtFast(lengths[1]) * 0.5f;

		// Compute mid points
		VectorAverage(vertices[index0].xyz, vertices[index1].xyz, mid[0]);
		VectorAverage(vertices[index2].xyz, vertices[index3].xyz, mid[1]);

		// Compute direction
		VectorSubtract(backEnd.localParms.viewOrigin, mid[0], tmp2);
		VectorSubtract(mid[1], mid[0], tmp3);

		CrossProduct(tmp2, tmp3, dir);
		VectorNormalizeFast(dir);

		// Modify vertices, leaving texture coords unchanged

		for (j = 0; j < 4; j++){
			vertices->color[index0] = 255;
			vertices->color[index1] = 255;
			vertices->color[index2] = 255;
			vertices->color[index3] = 255;

			vertices++;
		}
	}
}

/*
 ==================
 RB_Deform
 ==================
*/
void RB_Deform (material_t *material){

	if (r_skipDeforms->integerValue || material->deform == DFRM_NONE)
		return;

	switch (material->deform){
	case DFRM_EXPAND:
		RB_DeformExpand(material);
		break;
	case DFRM_MOVE:
		RB_DeformMove(material);
		break;
	case DFRM_SPRITE:
		RB_DeformSprite(material);
		break;
	case DFRM_TUBE:
		RB_DeformTube(material);
		break;
	case DFRM_BEAM:
		RB_DeformBeam(material);
		break;
	default:
		Com_Error(ERR_DROP, "RB_Deform: unknown deform in material '%s'", material->name);
	}
}