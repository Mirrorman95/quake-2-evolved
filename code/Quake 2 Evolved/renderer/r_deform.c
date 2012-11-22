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

	for (i = 0; i < backEnd.numVertices; i++)
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

	for (i = 0; i < backEnd.numVertices; i++)
		VectorAdd(vertices->xyz, move, vertices->xyz);
}

/*
 ==================
 
 ==================
*/
static void RB_DeformSprite (material_t *material){

	glIndex_t	*indices = backEnd.indices;
	glVertex_t	*vertices = backEnd.vertices;
	vec3_t		lVector, uVector;
	vec3_t		center;
	float		radius;
	int			i;

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

		// Compute left and up vectors

		// Modify indices

		// Modify vertices
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
	vec3_t		dir, mid[2];
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

		// Get indices

		// Compute lengths

		// Compute mid points

		// Compute direction

		// Modify vertices, leaving texture coords unchanged
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
	vec3_t		dir, mid[2];
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

		// Get indices

		// Compute lengths

		// Compute mid points

		// Compute direction

		// Modify vertices, leaving texture coords unchanged
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