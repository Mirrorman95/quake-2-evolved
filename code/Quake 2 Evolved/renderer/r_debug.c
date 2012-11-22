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
// r_debug.c - Debug tools rendering
//


#include "r_local.h"


/*
 ==================
 RB_ShowDepth
 ==================
*/
static void RB_ShowDepth (){

	float	*buffer;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Read the depth buffer
	buffer = (float *)Mem_ClearedAlloc(backEnd.viewport.width * backEnd.viewport.height * sizeof(float), TAG_TEMPORARY);

	qglReadPixels(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height, GL_DEPTH_COMPONENT, GL_FLOAT, buffer);

	// Draw the depth buffer
	qglWindowPos2i(backEnd.viewport.x, backEnd.viewport.y);
	qglDrawPixels(backEnd.viewport.width, backEnd.viewport.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

	Mem_Free(buffer);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 ==================
 RB_ShowVertexColors
 ==================
*/
static void RB_ShowVertexColors (int numMeshes, mesh_t *meshes){

	mesh_t		*mesh;
	glIndex_t	*indices;
	glVertex_t	*vertices;
	int			i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Evaluate registers
		RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

		// Skip if condition evaluated to false
		if (!mesh->material->expressionRegisters[mesh->material->conditionRegister])
			continue;

		// Set the entity state
		RB_EntityState(mesh->entity);

		// Set the batch state
		backEnd.entity = mesh->entity;
		backEnd.material = mesh->material;

		// Batch the mesh geometry
		RB_BatchGeometry(mesh->type, mesh->data);

		// Draw the vertex colors
		indices = backEnd.indices;
		vertices = backEnd.vertices;

		RB_Deform(backEnd.material);

		RB_PolygonOffset(backEnd.material);

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			qglColor4ubv(vertices[indices[0]].color);
			qglVertex3fv(vertices[indices[0]].xyz);

			qglColor4ubv(vertices[indices[1]].color);
			qglVertex3fv(vertices[indices[1]].xyz);

			qglColor4ubv(vertices[indices[2]].color);
			qglVertex3fv(vertices[indices[2]].xyz);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 ==================
 RB_ShowTextureCoords
 ==================
*/
static void RB_ShowTextureCoords (int numMeshes, mesh_t *meshes){

	mesh_t		*mesh;
	glIndex_t	*indices;
	glVertex_t	*vertices;
	int			i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_BlendEquation(GL_FUNC_ADD);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Evaluate registers
		RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

		// Skip if condition evaluated to false
		if (!mesh->material->expressionRegisters[mesh->material->conditionRegister])
			continue;

		// Set the entity state
		RB_EntityState(mesh->entity);

		// Set the batch state
		backEnd.entity = mesh->entity;
		backEnd.material = mesh->material;

		// Batch the mesh geometry
		RB_BatchGeometry(mesh->type, mesh->data);

		// Draw the texture coords
		indices = backEnd.indices;
		vertices = backEnd.vertices;

		RB_Deform(backEnd.material);

		RB_PolygonOffset(backEnd.material);

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			qglColor4f(Frac(vertices[indices[0]].st[0]), Frac(vertices[indices[0]].st[1]), 0.0f, 0.5f);
			qglVertex3fv(vertices[indices[0]].xyz);

			qglColor4f(Frac(vertices[indices[1]].st[0]), Frac(vertices[indices[1]].st[1]), 0.0f, 0.5f);
			qglVertex3fv(vertices[indices[1]].xyz);

			qglColor4f(Frac(vertices[indices[2]].st[0]), Frac(vertices[indices[2]].st[1]), 0.0f, 0.5f);
			qglVertex3fv(vertices[indices[2]].xyz);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 ==================
 RB_ShowTangentSpace
 ==================
*/
static void RB_ShowTangentSpace (int numMeshes, mesh_t *meshes){

	mesh_t		*mesh;
	glIndex_t	*indices;
	glVertex_t	*vertices;
	vec3_t		color[3];
	int			i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_BlendEquation(GL_FUNC_ADD);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Skip if it doesn't have normals
		if (mesh->type != MESH_SURFACE && mesh->type != MESH_ALIASMODEL)
			continue;

		// Skip if it has a deform that invalidates the normals
		if (mesh->material->deform != DFRM_NONE && mesh->material->deform != DFRM_EXPAND && mesh->material->deform != DFRM_MOVE)
			continue;

		// Evaluate registers
		RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

		// Skip if condition evaluated to false
		if (!mesh->material->expressionRegisters[mesh->material->conditionRegister])
			continue;

		// Set the entity state
		RB_EntityState(mesh->entity);

		// Set the batch state
		backEnd.entity = mesh->entity;
		backEnd.material = mesh->material;

		// Batch the mesh geometry
		RB_BatchGeometry(mesh->type, mesh->data);

		// Draw the tangent space
		indices = backEnd.indices;
		vertices = backEnd.vertices;

		RB_Deform(backEnd.material);

		RB_PolygonOffset(backEnd.material);

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			// Select color from tangent or normal vector
			switch (r_showTangentSpace->integerValue){
			case 1:
				VectorCopy(vertices[indices[0]].tangents[0], color[0]);
				VectorCopy(vertices[indices[1]].tangents[0], color[1]);
				VectorCopy(vertices[indices[2]].tangents[0], color[2]);

				break;
			case 2:
				VectorCopy(vertices[indices[0]].tangents[1], color[0]);
				VectorCopy(vertices[indices[1]].tangents[1], color[1]);
				VectorCopy(vertices[indices[2]].tangents[1], color[2]);

				break;
			default:
				VectorCopy(vertices[indices[0]].normal, color[0]);
				VectorCopy(vertices[indices[1]].normal, color[1]);
				VectorCopy(vertices[indices[2]].normal, color[2]);

				break;
			}

			qglColor4f(0.5f + 0.5f * color[0][0], 0.5f + 0.5f * color[0][1], 0.5f + 0.5f * color[0][2], 0.5f);
			qglVertex3fv(vertices[indices[0]].xyz);

			qglColor4f(0.5f + 0.5f * color[1][0], 0.5f + 0.5f * color[1][1], 0.5f + 0.5f * color[1][2], 0.5f);
			qglVertex3fv(vertices[indices[1]].xyz);

			qglColor4f(0.5f + 0.5f * color[2][0], 0.5f + 0.5f * color[2][1], 0.5f + 0.5f * color[2][2], 0.5f);
			qglVertex3fv(vertices[indices[2]].xyz);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 ==================
 RB_ShowTris
 ==================
*/
static void RB_ShowTris (int numMeshes, mesh_t *meshes){

	mesh_t	*mesh;
	int		i;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_LINE);

	GL_Disable(GL_CULL_FACE);

	if (r_showTris->integerValue != 1)
		GL_Disable(GL_POLYGON_OFFSET_LINE);
	else {
		GL_Enable(GL_POLYGON_OFFSET_LINE);
		GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);
	}

	GL_Disable(GL_BLEND);

	GL_Disable(GL_ALPHA_TEST);

	if (r_showTris->integerValue != 1)
		GL_Disable(GL_DEPTH_TEST);
	else {
		GL_Enable(GL_DEPTH_TEST);
		GL_DepthFunc(GL_LEQUAL);
	}

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Set the color
	qglColor3f(1.0f, 1.0f, 1.0f);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Evaluate registers
		RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

		// Skip if condition evaluated to false
		if (!mesh->material->expressionRegisters[mesh->material->conditionRegister])
			continue;

		// Set the entity state
		RB_EntityState(mesh->entity);

		// Set the batch state
		backEnd.entity = mesh->entity;
		backEnd.material = mesh->material;

		// Batch the mesh geometry
		RB_BatchGeometry(mesh->type, mesh->data);

		// Draw the tris
		RB_Deform(backEnd.material);

		qglVertexPointer(3, GL_FLOAT, sizeof(glVertex_t), backEnd.vertices);

		qglDrawElements(GL_TRIANGLES, backEnd.numIndices, GL_INDEX_TYPE, backEnd.indices);

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 ==================
 RB_ShowNormals
 ==================
*/
static void RB_ShowNormals (int numMeshes, mesh_t *meshes){

	mesh_t		*mesh;
	glVertex_t	*vertices;
	vec3_t		point;
	int			i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Skip if it doesn't have normals
		if (mesh->type != MESH_SURFACE && mesh->type != MESH_ALIASMODEL)
			continue;

		// Skip if it has a deform that invalidates the normals
		if (mesh->material->deform != DFRM_NONE && mesh->material->deform != DFRM_EXPAND && mesh->material->deform != DFRM_MOVE)
			continue;

		// Evaluate registers
		RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

		// Skip if condition evaluated to false
		if (!mesh->material->expressionRegisters[mesh->material->conditionRegister])
			continue;

		// Set the entity state
		RB_EntityState(mesh->entity);

		// Set the batch state
		backEnd.entity = mesh->entity;
		backEnd.material = mesh->material;

		// Batch the mesh geometry
		RB_BatchGeometry(mesh->type, mesh->data);

		// Draw the normals
		vertices = backEnd.vertices;

		RB_Deform(backEnd.material);

		qglBegin(GL_LINES);
		for (j = 0; j < backEnd.numVertices; j++){
			// First tangent
			VectorMA(vertices->xyz, r_showNormals->floatValue, vertices->tangents[0], point);

			qglColor3f(1.0f, 0.0f, 0.0f);
			qglVertex3fv(vertices->xyz);
			qglVertex3fv(point);

			// Second tangent
			VectorMA(vertices->xyz, r_showNormals->floatValue, vertices->tangents[1], point);

			qglColor3f(0.0f, 1.0f, 0.0f);
			qglVertex3fv(vertices->xyz);
			qglVertex3fv(point);

			// Normal
			VectorMA(vertices->xyz, r_showNormals->floatValue, vertices->normal, point);

			qglColor3f(0.0f, 0.0f, 1.0f);
			qglVertex3fv(vertices->xyz);
			qglVertex3fv(point);

			vertices++;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 ==================
 RB_ShowTextureVectors
 ==================
*/
static void RB_ShowTextureVectors (int numMeshes, mesh_t *meshes){

	mesh_t		*mesh;
	glIndex_t	*indices;
	glVertex_t	*vertices;
	vec3_t		center, point;
	vec3_t		normal, tangents[2];
	int			i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Skip if it doesn't have normals
		if (mesh->type != MESH_SURFACE && mesh->type != MESH_ALIASMODEL)
			continue;

		// Skip if it has a deform that invalidates the normals
		if (mesh->material->deform != DFRM_NONE && mesh->material->deform != DFRM_EXPAND && mesh->material->deform != DFRM_MOVE)
			continue;

		// Evaluate registers
		RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

		// Skip if condition evaluated to false
		if (!mesh->material->expressionRegisters[mesh->material->conditionRegister])
			continue;

		// Set the entity state
		RB_EntityState(mesh->entity);

		// Set the batch state
		backEnd.entity = mesh->entity;
		backEnd.material = mesh->material;

		// Batch the mesh geometry
		RB_BatchGeometry(mesh->type, mesh->data);

		// Draw the texture vectors
		indices = backEnd.indices;
		vertices = backEnd.vertices;

		RB_Deform(backEnd.material);

		qglBegin(GL_LINES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			// Compute center of triangle
			center[0] = (vertices[indices[0]].xyz[0] + vertices[indices[1]].xyz[0] + vertices[indices[2]].xyz[0]) * (1.0f / 3.0f);
			center[1] = (vertices[indices[0]].xyz[1] + vertices[indices[1]].xyz[1] + vertices[indices[2]].xyz[1]) * (1.0f / 3.0f);
			center[2] = (vertices[indices[0]].xyz[2] + vertices[indices[1]].xyz[2] + vertices[indices[2]].xyz[2]) * (1.0f / 3.0f);

			// Compute triangle normal
			normal[0] = vertices[indices[0]].normal[0] + vertices[indices[1]].normal[0] + vertices[indices[2]].normal[0];
			normal[1] = vertices[indices[0]].normal[1] + vertices[indices[1]].normal[1] + vertices[indices[2]].normal[1];
			normal[2] = vertices[indices[0]].normal[2] + vertices[indices[1]].normal[2] + vertices[indices[2]].normal[2];
			VectorNormalizeFast(normal);

			// Offset the center
			center[0] += normal[0] * ON_EPSILON;
			center[1] += normal[1] * ON_EPSILON;
			center[2] += normal[2] * ON_EPSILON;

			// First tangent
			tangents[0][0] = vertices[indices[0]].tangents[0][0] + vertices[indices[1]].tangents[0][0] + vertices[indices[2]].tangents[0][0];
			tangents[0][1] = vertices[indices[0]].tangents[0][1] + vertices[indices[1]].tangents[0][1] + vertices[indices[2]].tangents[0][1];
			tangents[0][2] = vertices[indices[0]].tangents[0][2] + vertices[indices[1]].tangents[0][2] + vertices[indices[2]].tangents[0][2];
			VectorNormalizeFast(tangents[0]);

			VectorMA(center, r_showTextureVectors->floatValue, tangents[0], point);

			qglColor3f(1.0f, 0.0f, 0.0f);
			qglVertex3fv(center);
			qglVertex3fv(point);

			// Second tangent
			tangents[1][0] = vertices[indices[0]].tangents[1][0] + vertices[indices[1]].tangents[1][0] + vertices[indices[2]].tangents[1][0];
			tangents[1][1] = vertices[indices[0]].tangents[1][1] + vertices[indices[1]].tangents[1][1] + vertices[indices[2]].tangents[1][1];
			tangents[1][2] = vertices[indices[0]].tangents[1][2] + vertices[indices[1]].tangents[1][2] + vertices[indices[2]].tangents[1][2];
			VectorNormalizeFast(tangents[1]);

			VectorMA(center, r_showTextureVectors->floatValue, tangents[1], point);

			qglColor3f(0.0f, 1.0f, 0.0f);
			qglVertex3fv(center);
			qglVertex3fv(point);

			indices += 3;
		}
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		// Clear the arrays
		backEnd.numIndices = 0;
		backEnd.numVertices = 0;
	}
}

/*
 ==================
 RB_ShowBatchSizeColored
 ==================
*/
static void RB_ShowBatchSizeColored (){

	// Set the color based on triangle count
	if (backEnd.numIndices >= 1500)
		qglColor3f(0.0f, 1.0f, 0.0f);
	else if (backEnd.numIndices >= 300)
		qglColor3f(1.0f, 1.0f, 0.0f);
	else if (backEnd.numIndices >= 150)
		qglColor3f(1.0f, 0.5f, 0.0f);
	else if (backEnd.numIndices >= 30)
		qglColor3f(1.0f, 0.0f, 0.0f);
	else
		qglColor3f(1.0f, 0.0f, 1.0f);

	// Draw the batch
	qglDrawElements(GL_TRIANGLES, backEnd.numIndices, GL_INDEX_TYPE, backEnd.indices);
}

/*
 ==================
 RB_ShowBatchSize
 ==================
*/
static void RB_ShowBatchSize (int numMeshes, mesh_t *meshes){

	mesh_t	*mesh;
	uint	sort;
	bool	skip;
	int		i;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_LINE);

	GL_Disable(GL_CULL_FACE);

	if (r_showBatchSize->integerValue != 1)
		GL_Disable(GL_POLYGON_OFFSET_LINE);
	else {
		GL_Enable(GL_POLYGON_OFFSET_LINE);
		GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);
	}

	GL_Disable(GL_BLEND);

	GL_Disable(GL_ALPHA_TEST);

	if (r_showBatchSize->integerValue != 1)
		GL_Disable(GL_DEPTH_TEST);
	else {
		GL_Enable(GL_DEPTH_TEST);
		GL_DepthFunc(GL_LEQUAL);
	}

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Clear the batch state
	backEnd.entity = NULL;
	backEnd.material = NULL;

	sort = 0;

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		if (mesh->sort != sort){
			sort = mesh->sort;

			// Draw the last batch
			RB_RenderBatch();

			// Evaluate registers if needed
			if (mesh->entity != backEnd.entity || mesh->material != backEnd.material)
				RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

			// Skip if condition evaluated to false
			if (!mesh->material->expressionRegisters[mesh->material->conditionRegister]){
				skip = true;
				continue;
			}

			// Set the entity state if needed
			if (mesh->entity != backEnd.entity)
				RB_EntityState(mesh->entity);

			// Create a new batch
			RB_SetupBatch(mesh->entity, mesh->material, RB_ShowBatchSizeColored);

			skip = false;
		}

		if (skip)
			continue;

		// Batch the mesh geometry
		RB_BatchGeometry(mesh->type, mesh->data);
	}

	// Draw the last batch
	RB_RenderBatch();
}

/*
 ==================
 RB_ShowModelBounds
 ==================
*/
static void RB_ShowModelBounds (int numMeshes, mesh_t *meshes){

	mesh_t		*mesh;
	model_t		*model;
	mdl_t		*alias;
	mdlFrame_t	*curFrame, *oldFrame;
	vec3_t		mins, maxs, corners[8];
	int			i, j;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Skip if world entity
		if (mesh->entity == rg.worldEntity)
			continue;

		// Skip if not surface or alias model
		if (mesh->type != MESH_SURFACE && mesh->type != MESH_ALIASMODEL)
			continue;

		// Evaluate registers
		RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

		// Skip if condition evaluated to false
		if (!mesh->material->expressionRegisters[mesh->material->conditionRegister])
			continue;

		// Set the entity state
		RB_EntityState(mesh->entity);

		// Set the batch state
		backEnd.entity = mesh->entity;
		backEnd.material = mesh->material;

		// Draw the model bounds
		model = backEnd.entity->model;

		switch (mesh->type){
		case MODEL_INLINE:
			for (j = 0; j < 8; j++){
				corners[j][0] = (j & 1) ? model->mins[0] : model->maxs[0];
				corners[j][1] = (j & 2) ? model->mins[1] : model->maxs[1];
				corners[j][2] = (j & 4) ? model->mins[2] : model->maxs[2];
			}

			qglColor3f(1.0f, 1.0f, 0.0f);

			break;
		case MODEL_MD2:
		case MODEL_MD3:
			// Compute axially aligned mins and maxs
			curFrame = alias->frames + backEnd.entity->frame;
			oldFrame = alias->frames + backEnd.entity->oldFrame;

			if (curFrame == oldFrame){
				VectorCopy(curFrame->mins, mins);
				VectorCopy(curFrame->maxs, maxs);
			}
			else {
				VectorMin(curFrame->mins, oldFrame->mins, mins);
				VectorMax(curFrame->maxs, oldFrame->maxs, maxs);
			}

			// Compute the corners of the bounding volume
			for (j = 0; j < 8; j++){
				corners[j][0] = (j & 1) ? mins[0] : maxs[0];
				corners[j][1] = (j & 2) ? mins[1] : maxs[1];
				corners[j][2] = (j & 4) ? mins[2] : maxs[2];
			}

			if (mesh->type == MODEL_MD2)
				qglColor3f(0.0f, 1.0f, 1.0f);
			else
				qglColor3f(1.0f, 0.0f, 1.0f);

			break;
		}

		// Draw it
		qglBegin(GL_LINE_LOOP);
		qglVertex3fv(corners[0]);
		qglVertex3fv(corners[2]);
		qglVertex3fv(corners[3]);
		qglVertex3fv(corners[1]);
		qglEnd();

		qglBegin(GL_LINE_LOOP);
		qglVertex3fv(corners[4]);
		qglVertex3fv(corners[6]);
		qglVertex3fv(corners[7]);
		qglVertex3fv(corners[5]);
		qglEnd();

		qglBegin(GL_LINES);
		qglVertex3fv(corners[0]);
		qglVertex3fv(corners[4]);
		qglVertex3fv(corners[1]);
		qglVertex3fv(corners[5]);
		qglVertex3fv(corners[2]);
		qglVertex3fv(corners[6]);
		qglVertex3fv(corners[3]);
		qglVertex3fv(corners[7]);
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();
	}
}


// ============================================================================


/*
 ==================
 RB_RenderDebugTools
 ==================
*/
void RB_RenderDebugTools (){

	QGL_LogPrintf("---------- RB_RenderDebugTools ----------\n");

	if (r_showDepth->integerValue)
		RB_ShowDepth();

	if (r_showVertexColors->integerValue){
		RB_ShowVertexColors(backEnd.viewParms.numMeshes[0], backEnd.viewParms.meshes[0]);
		RB_ShowVertexColors(backEnd.viewParms.numMeshes[1], backEnd.viewParms.meshes[1]);
		RB_ShowVertexColors(backEnd.viewParms.numMeshes[2], backEnd.viewParms.meshes[2]);
		RB_ShowVertexColors(backEnd.viewParms.numMeshes[3], backEnd.viewParms.meshes[3]);
	}

	if (r_showTextureCoords->integerValue){
		RB_ShowTextureCoords(backEnd.viewParms.numMeshes[0], backEnd.viewParms.meshes[0]);
		RB_ShowTextureCoords(backEnd.viewParms.numMeshes[1], backEnd.viewParms.meshes[1]);
		RB_ShowTextureCoords(backEnd.viewParms.numMeshes[2], backEnd.viewParms.meshes[2]);
		RB_ShowTextureCoords(backEnd.viewParms.numMeshes[3], backEnd.viewParms.meshes[3]);
	}

	if (r_showTangentSpace->integerValue){
		RB_ShowTangentSpace(backEnd.viewParms.numMeshes[0], backEnd.viewParms.meshes[0]);
		RB_ShowTangentSpace(backEnd.viewParms.numMeshes[1], backEnd.viewParms.meshes[1]);
		RB_ShowTangentSpace(backEnd.viewParms.numMeshes[2], backEnd.viewParms.meshes[2]);
		RB_ShowTangentSpace(backEnd.viewParms.numMeshes[3], backEnd.viewParms.meshes[3]);
	}

	if (r_showTris->integerValue){
		RB_ShowTris(backEnd.viewParms.numMeshes[0], backEnd.viewParms.meshes[0]);
		RB_ShowTris(backEnd.viewParms.numMeshes[1], backEnd.viewParms.meshes[1]);
		RB_ShowTris(backEnd.viewParms.numMeshes[2], backEnd.viewParms.meshes[2]);
		RB_ShowTris(backEnd.viewParms.numMeshes[3], backEnd.viewParms.meshes[3]);
	}

	if (r_showNormals->floatValue){
		RB_ShowNormals(backEnd.viewParms.numMeshes[0], backEnd.viewParms.meshes[0]);
		RB_ShowNormals(backEnd.viewParms.numMeshes[1], backEnd.viewParms.meshes[1]);
		RB_ShowNormals(backEnd.viewParms.numMeshes[2], backEnd.viewParms.meshes[2]);
		RB_ShowNormals(backEnd.viewParms.numMeshes[3], backEnd.viewParms.meshes[3]);
	}

	if (r_showTextureVectors->floatValue){
		RB_ShowTextureVectors(backEnd.viewParms.numMeshes[0], backEnd.viewParms.meshes[0]);
		RB_ShowTextureVectors(backEnd.viewParms.numMeshes[1], backEnd.viewParms.meshes[1]);
		RB_ShowTextureVectors(backEnd.viewParms.numMeshes[2], backEnd.viewParms.meshes[2]);
		RB_ShowTextureVectors(backEnd.viewParms.numMeshes[3], backEnd.viewParms.meshes[3]);
	}

	if (r_showBatchSize->integerValue){
		RB_ShowBatchSize(backEnd.viewParms.numMeshes[0], backEnd.viewParms.meshes[0]);
		RB_ShowBatchSize(backEnd.viewParms.numMeshes[1], backEnd.viewParms.meshes[1]);
		RB_ShowBatchSize(backEnd.viewParms.numMeshes[2], backEnd.viewParms.meshes[2]);
		RB_ShowBatchSize(backEnd.viewParms.numMeshes[3], backEnd.viewParms.meshes[3]);
	}

	if (r_showModelBounds->integerValue){
		RB_ShowModelBounds(backEnd.viewParms.numMeshes[0], backEnd.viewParms.meshes[0]);
		RB_ShowModelBounds(backEnd.viewParms.numMeshes[1], backEnd.viewParms.meshes[1]);
		RB_ShowModelBounds(backEnd.viewParms.numMeshes[2], backEnd.viewParms.meshes[2]);
		RB_ShowModelBounds(backEnd.viewParms.numMeshes[3], backEnd.viewParms.meshes[3]);
	}

	QGL_LogPrintf("--------------------\n");
}