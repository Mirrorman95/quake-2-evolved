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
	GL_StencilMask(255);

	// Read the depth buffer
	buffer = (float *)Mem_Alloc(RectSize(backEnd.viewport) * sizeof(float), TAG_TEMPORARY);

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
 RB_ShowOverdraw
 ==================
*/
static void RB_ShowOverdraw (int numMeshes, mesh_t *meshes, bool singlePass, bool ambient){

	mesh_t	*mesh;
	stage_t	*stage, *bumpStage, *diffuseStage, *specularStage;
	int		passes;
	int		i, j;

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

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 0, 255);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_INCR);

	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the surfaces
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

		// Batch the surface geometry
		RB_BatchGeometry(mesh->type, mesh->data);

		// Count number of passes
		if (singlePass)
			passes = 1;
		else {
			passes = 0;

			if (ambient){
				for (j = 0, stage = mesh->material->stages; j < mesh->material->numStages; j++, stage++){
					if (stage->lighting != SL_AMBIENT)
						continue;

					if (!mesh->material->expressionRegisters[stage->conditionRegister])
						continue;

					passes++;
				}
			}
			else {
				bumpStage = diffuseStage = specularStage = NULL;

				for (j = 0, stage = mesh->material->stages; j < mesh->material->numStages; j++, stage++){
					if (stage->lighting == SL_AMBIENT)
						continue;

					if (!mesh->material->expressionRegisters[stage->conditionRegister])
						continue;

					switch (stage->lighting){
					case SL_BUMP:
						if (bumpStage)
							passes++;

						bumpStage = stage;
						diffuseStage = NULL;
						specularStage = NULL;

						break;
					case SL_DIFFUSE:
						if (bumpStage && diffuseStage)
							passes++;

						diffuseStage = stage;

						break;
					case SL_SPECULAR:
						if (bumpStage && specularStage)
							passes++;

						specularStage = stage;

						break;
					}
				}

				if (bumpStage || diffuseStage || specularStage)
					passes++;
			}
		}

		// Add to overdraw
		RB_Deform(backEnd.material);

		qglVertexPointer(3, GL_FLOAT, sizeof(glVertex_t), backEnd.vertices);

		RB_PolygonOffset(backEnd.material);

		for (j = 0; j < passes; j++)
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
 RB_ShowLightCount
 ==================
*/
static void RB_ShowLightCount (int numMeshes, mesh_t *meshes){

	mesh_t	*mesh;
	int		i;

	if (!numMeshes)
		return;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_EQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_ALWAYS, 0, 255);

	if (r_showLightCount->integerValue == 1)
		GL_StencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	else
		GL_StencilOp(GL_KEEP, GL_INCR, GL_INCR);

	GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Run through the surfaces
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

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

		// Batch the surface geometry
		RB_BatchGeometry(mesh->type, mesh->data);

		// Add to light count
		RB_Deform(backEnd.material);

		qglVertexPointer(3, GL_FLOAT, sizeof(glVertex_t), backEnd.vertices);

		RB_PolygonOffset(backEnd.material);

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
 RB_ShowLightVolume
 ==================
*/
static void RB_ShowLightVolume (){

	// Set the GL state
	GL_LoadMatrix(GL_MODELVIEW, backEnd.viewParms.modelviewMatrix);

	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_DepthRange(0.0f, 1.0f);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	if (r_showLightVolumes->integerValue != 2){
		// Set the GL state
		GL_Enable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);

		GL_Enable(GL_BLEND);
		GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GL_BlendEquation(GL_FUNC_ADD);

		GL_Enable(GL_DEPTH_TEST);
		GL_DepthFunc(GL_LEQUAL);

		// Set the color
		qglColor4f(backEnd.light->materialParms[0], backEnd.light->materialParms[1], backEnd.light->materialParms[2], 0.5f);

		// Draw it
		qglBegin(GL_QUADS);
		qglVertex3fv(backEnd.light->data.corners[3]);
		qglVertex3fv(backEnd.light->data.corners[2]);
		qglVertex3fv(backEnd.light->data.corners[6]);
		qglVertex3fv(backEnd.light->data.corners[7]);
		qglVertex3fv(backEnd.light->data.corners[0]);
		qglVertex3fv(backEnd.light->data.corners[1]);
		qglVertex3fv(backEnd.light->data.corners[5]);
		qglVertex3fv(backEnd.light->data.corners[4]);
		qglVertex3fv(backEnd.light->data.corners[2]);
		qglVertex3fv(backEnd.light->data.corners[3]);
		qglVertex3fv(backEnd.light->data.corners[1]);
		qglVertex3fv(backEnd.light->data.corners[0]);
		qglVertex3fv(backEnd.light->data.corners[4]);
		qglVertex3fv(backEnd.light->data.corners[5]);
		qglVertex3fv(backEnd.light->data.corners[7]);
		qglVertex3fv(backEnd.light->data.corners[6]);
		qglVertex3fv(backEnd.light->data.corners[1]);
		qglVertex3fv(backEnd.light->data.corners[3]);
		qglVertex3fv(backEnd.light->data.corners[7]);
		qglVertex3fv(backEnd.light->data.corners[5]);
		qglVertex3fv(backEnd.light->data.corners[2]);
		qglVertex3fv(backEnd.light->data.corners[0]);
		qglVertex3fv(backEnd.light->data.corners[4]);
		qglVertex3fv(backEnd.light->data.corners[6]);
		qglEnd();
	}

	if (r_showLightVolumes->integerValue != 1){
		// Set the GL state
		GL_Disable(GL_POLYGON_OFFSET_FILL);

		GL_Disable(GL_BLEND);

		GL_Disable(GL_DEPTH_TEST);

		// Set the color
		qglColor3f(backEnd.light->materialParms[0], backEnd.light->materialParms[1], backEnd.light->materialParms[2]);

		// Draw it
		qglBegin(GL_LINE_LOOP);
		qglVertex3fv(backEnd.light->data.corners[0]);
		qglVertex3fv(backEnd.light->data.corners[2]);
		qglVertex3fv(backEnd.light->data.corners[3]);
		qglVertex3fv(backEnd.light->data.corners[1]);
		qglEnd();

		qglBegin(GL_LINE_LOOP);
		qglVertex3fv(backEnd.light->data.corners[4]);
		qglVertex3fv(backEnd.light->data.corners[6]);
		qglVertex3fv(backEnd.light->data.corners[7]);
		qglVertex3fv(backEnd.light->data.corners[5]);
		qglEnd();

		qglBegin(GL_LINES);
		qglVertex3fv(backEnd.light->data.corners[0]);
		qglVertex3fv(backEnd.light->data.corners[4]);
		qglVertex3fv(backEnd.light->data.corners[1]);
		qglVertex3fv(backEnd.light->data.corners[5]);
		qglVertex3fv(backEnd.light->data.corners[2]);
		qglVertex3fv(backEnd.light->data.corners[6]);
		qglVertex3fv(backEnd.light->data.corners[3]);
		qglVertex3fv(backEnd.light->data.corners[7]);
		qglEnd();
	}

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 ==================
 RB_ShowLightScissor
 ==================
*/
static void RB_ShowLightScissor (rect_t scissor){

	float	x, y, width, height;

	// Transform scissor into normalized device coordinates
	x = (scissor.x * backEnd.coordScale[0] + backEnd.coordBias[0]) * 2.0f - 1.0f;
	y = (scissor.y * backEnd.coordScale[1] + backEnd.coordBias[1]) * 2.0f - 1.0f;
	width = (scissor.width * backEnd.coordScale[0] + backEnd.coordBias[0]) * 2.0f - 1.0f;
	height = (scissor.height * backEnd.coordScale[1] + backEnd.coordBias[1]) * 2.0f - 1.0f;

	// Set the GL state
	GL_LoadIdentity(GL_PROJECTION);
	GL_LoadIdentity(GL_MODELVIEW);

	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_DepthRange(0.0f, 1.0f);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Disable the clip plane if needed
	if (backEnd.viewParms.viewType == VIEW_MIRROR)
		qglDisable(GL_CLIP_PLANE0);

	// Set the color
	qglColor3f(backEnd.light->materialParms[0], backEnd.light->materialParms[1], backEnd.light->materialParms[2]);

	// Draw it
	qglBegin(GL_LINE_LOOP);
	qglVertex2f(x, y);
	qglVertex2f(width, y);
	qglVertex2f(width, height);
	qglVertex2f(x, height);
	qglEnd();

	// Restore the GL state
	GL_LoadMatrix(GL_PROJECTION, backEnd.viewParms.projectionMatrix);

	// Enable the clip plane if needed
	if (backEnd.viewParms.viewType == VIEW_MIRROR)
		qglEnable(GL_CLIP_PLANE0);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 ==================
 
 ==================
*/
static void RB_ShowShadowTris (int numMeshes, mesh_t *meshes){

	if (!numMeshes)
		return;
}

/*
 ==================
 
 ==================
*/
static void RB_ShowShadowVolumes (int numMeshes, mesh_t *meshes){

	mesh_t				*mesh;
	glIndex_t			*indices;
	glShadowVertex_t	*vertices;
	int					i, j;

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
	GL_StencilMask(255);

	qglColor4f(0.0f, 1.0f, 1.0f, 0.5f);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Evaluate registers
		RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

		// Skip if condition evaluated to false
		if (!mesh->material->expressionRegisters[mesh->material->conditionRegister])
			continue;

		// Set the entity state
		RB_EntityState(mesh->entity);

		// TODO: light state?

		// Set the batch state
		backEnd.entity = mesh->entity;
		backEnd.material = mesh->material;

		backEnd.stencilShadow = true;
		backEnd.shadowCaps = mesh->caps;

		// Batch the mesh geometry
		RB_BatchShadowGeometry(mesh->type, mesh->data);

		// Draw the shadow volumes
		indices = backEnd.shadowIndices;
		vertices = backEnd.shadowVertices;

		RB_Deform(backEnd.material);

		RB_PolygonOffset(backEnd.material);

		qglBegin(GL_TRIANGLES);
		for (j = 0; j < backEnd.numIndices; j += 3){
			qglVertex4fv(vertices[indices[0]].xyzw);
			qglVertex4fv(vertices[indices[1]].xyzw);
			qglVertex4fv(vertices[indices[2]].xyzw);

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
 
 ==================
*/
static void RB_ShowShadowSilhouettes (int numMeshes, mesh_t *meshes){

	mesh_t				*mesh;
	glIndex_t			*indices;
	glShadowVertex_t	*vertices;
	int					i, j;

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
	GL_StencilMask(255);

	qglColor3f(1.0f, 1.0f, 0.0f);

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Evaluate registers
		RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

		// Skip if condition evaluated to false
		if (!mesh->material->expressionRegisters[mesh->material->conditionRegister])
			continue;

		// Set the entity state
		RB_EntityState(mesh->entity);

		// TODO: light state?

		// Set the batch state
		backEnd.entity = mesh->entity;
		backEnd.material = mesh->material;

		backEnd.stencilShadow = true;
		backEnd.shadowCaps = false;

		// Batch the mesh geometry
		RB_BatchShadowGeometry(mesh->type, mesh->data);

		// Draw the shadow volumes
		indices = backEnd.shadowIndices;
		vertices = backEnd.shadowVertices;

		RB_Deform(backEnd.material);

//		RB_PolygonOffset(backEnd.material);

		qglBegin(GL_LINES);
		for (j = 0; j < backEnd.numIndices; j += 6){
			qglVertex4fv(vertices[indices[0]].xyzw);
			qglVertex4fv(vertices[indices[1]].xyzw);

			indices += 6;
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
	GL_StencilMask(255);

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

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

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
	GL_StencilMask(255);

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

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

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
	GL_StencilMask(255);

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

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

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
	GL_StencilMask(255);

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

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

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
	GL_StencilMask(255);

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

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

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
	GL_StencilMask(255);

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

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

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
	GL_StencilMask(255);

	// Clear the batch state
	backEnd.entity = NULL;
	backEnd.material = NULL;

	backEnd.stencilShadow = false;
	backEnd.shadowCaps = false;

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
			RB_SetupBatch(mesh->entity, mesh->material, false, false, RB_ShowBatchSizeColored);

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
	GL_StencilMask(255);

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

		backEnd.stencilShadow = false;
		backEnd.shadowCaps = false;

		// Draw the model bounds
		model = backEnd.entity->model;

		switch (model->type){
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
			alias = model->alias;

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

/*
 ==================
 RB_ShowStencil
 ==================
*/
static float RB_ShowStencil (){

	byte	*buffer;
	int		overdraw = 0;
	int		i;

	// Set the GL state
	GL_LoadIdentity(GL_PROJECTION);
	GL_LoadIdentity(GL_MODELVIEW);

	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_ALPHA_TEST);

	GL_Disable(GL_DEPTH_TEST);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_DepthRange(0.0f, 1.0f);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Draw fullscreen quads
	for (i = 0; i <= COLOR_WHITE; i++){
		if (i != COLOR_WHITE)
			GL_StencilFunc(GL_EQUAL, i, 255);
		else
			GL_StencilFunc(GL_LEQUAL, i, 255);

		qglColor4fv(color_table[i]);

		qglBegin(GL_QUADS);
		qglVertex2f(-1.0f, -1.0f);
		qglVertex2f( 1.0f, -1.0f);
		qglVertex2f( 1.0f,  1.0f);
		qglVertex2f(-1.0f,  1.0f);
		qglEnd();
	}

	// Restore the GL state
	GL_LoadMatrix(GL_PROJECTION, backEnd.viewParms.projectionMatrix);

	// Read the stencil buffer
	buffer = (byte *)Mem_Alloc(RectSize(backEnd.viewport), TAG_TEMPORARY);

	qglPixelStorei(GL_PACK_ALIGNMENT, 1);

	qglReadPixels(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, buffer);

	qglPixelStorei(GL_PACK_ALIGNMENT, 4);

	// Measure overdraw
	for (i = 0; i < RectSize(backEnd.viewport); i++)
		overdraw += buffer[i];

	Mem_Free(buffer);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	return (float)overdraw / RectSize(backEnd.viewport);
}


/*
 ==============================================================================

 DEBUG VISUALIZATION FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 RB_AddDebugPolygon
 ==================
*/
void RB_AddDebugPolygon (const vec4_t color, int numPoints, const vec3_t *points, bool fill, bool depthTest, int allowInView){

	debugPolygon_t	*debugPolygon;

	if (backEnd.numDebugPolygons == backEnd.maxDebugPolygons)
		return;		// Silently ignore

	// Add a debug polygon
	debugPolygon = &backEnd.debugPolygons[backEnd.numDebugPolygons++];

	debugPolygon->allowInView = allowInView;
	debugPolygon->fill = fill;
	debugPolygon->depthTest = depthTest;
	MakeRGBA(debugPolygon->color, color[0], color[1], color[2], color[3]);
	debugPolygon->numPoints = numPoints;
	Mem_Copy(debugPolygon->points, points, Min(numPoints, MAX_POLYGON_POINTS >> 2) * sizeof(vec3_t));
}

/*
 ==================
 RB_ClearDebugPolygons
 ==================
*/
void RB_ClearDebugPolygons (){

	backEnd.numDebugPolygons = 0;

	// Reallocate the debug polygons buffer if needed
	if (backEnd.maxDebugPolygons == r_maxDebugPolygons->integerValue)
		return;

	backEnd.maxDebugPolygons = r_maxDebugPolygons->integerValue;

	if (backEnd.debugPolygons){
		Mem_Free(backEnd.debugPolygons);
		backEnd.debugPolygons = NULL;
	}

	if (backEnd.maxDebugPolygons <= 0)
		return;

	backEnd.debugPolygons = (debugPolygon_t *)Mem_Alloc(backEnd.maxDebugPolygons * sizeof(debugPolygon_t), TAG_RENDERER);
}

/*
 ==================
 RB_DrawDebugPolygons
 ==================
*/
static void RB_DrawDebugPolygons (){

	debugPolygon_t	*debugPolygon;
	int				i, j;

	if (!backEnd.numDebugPolygons)
		return;

	// Set the GL state
	GL_LoadMatrix(GL_MODELVIEW, backEnd.viewParms.modelviewMatrix);

	GL_DisableTexture();

	GL_Disable(GL_CULL_FACE);

	GL_Enable(GL_POLYGON_OFFSET_FILL);
	GL_Enable(GL_POLYGON_OFFSET_LINE);
	GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_DepthRange(0.0f, 1.0f);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Draw all the debug polygons
	for (i = 0, debugPolygon = backEnd.debugPolygons; i < backEnd.numDebugPolygons; i++, debugPolygon++){
		// Check for view suppression
		if (!r_skipSuppress->integerValue){
			if (!(debugPolygon->allowInView & backEnd.viewParms.viewType))
				continue;
		}

		// Set the GL state
		if (debugPolygon->fill){
			GL_PolygonMode(GL_FILL);

			GL_Enable(GL_BLEND);
			GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			GL_BlendEquation(GL_FUNC_ADD);

			GL_DepthMask(GL_TRUE);
		}
		else {
			GL_PolygonMode(GL_LINE);

			GL_Disable(GL_BLEND);

			GL_DepthMask(GL_FALSE);
		}

		if (debugPolygon->depthTest){
			GL_Enable(GL_DEPTH_TEST);
			GL_DepthFunc(GL_LEQUAL);
		}
		else
			GL_Disable(GL_DEPTH_TEST);

		// Set the color
		qglColor4f(debugPolygon->color[0], debugPolygon->color[1], debugPolygon->color[2], 0.5f);

		// Draw it
		qglBegin(GL_POLYGON);
		for (j = 0; j < debugPolygon->numPoints; j++)
			qglVertex3fv(debugPolygon->points[j]);
		qglEnd();
	}

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 ==================
 RB_AddDebugLine
 ==================
*/
void RB_AddDebugLine (const vec4_t color, const vec3_t start, const vec3_t end, bool depthTest, int allowInView){

	debugLine_t	*debugLine;

	if (backEnd.numDebugLines == backEnd.maxDebugLines)
		return;		// Silently ignore

	// Add a debug line
	debugLine = &backEnd.debugLines[backEnd.numDebugLines++];

	debugLine->allowInView = allowInView;
	debugLine->depthTest = depthTest;
	MakeRGBA(debugLine->color, color[0], color[1], color[2], color[3]);
	VectorCopy(start, debugLine->start);
	VectorCopy(end, debugLine->end);
}

/*
 ==================
 RB_ClearDebugLines
 ==================
*/
void RB_ClearDebugLines (){

	backEnd.numDebugLines = 0;

	// Reallocate the debug lines buffer if needed
	if (backEnd.maxDebugLines == r_maxDebugLines->integerValue)
		return;

	backEnd.maxDebugLines = r_maxDebugLines->integerValue;

	if (backEnd.debugLines){
		Mem_Free(backEnd.debugLines);
		backEnd.debugLines = NULL;
	}

	if (backEnd.maxDebugLines <= 0)
		return;

	backEnd.debugLines = (debugLine_t *)Mem_Alloc(backEnd.maxDebugLines * sizeof(debugLine_t), TAG_RENDERER);
}

/*
 ==================
 RB_DrawDebugLines
 ==================
*/
static void RB_DrawDebugLines (){

	debugLine_t	*debugLine;
	int			i;

	if (!backEnd.numDebugLines)
		return;

	// Set the GL state
	GL_LoadMatrix(GL_MODELVIEW, backEnd.viewParms.modelviewMatrix);

	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);

	GL_Enable(GL_POLYGON_OFFSET_LINE);
	GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_DepthRange(0.0f, 1.0f);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(255);

	// Draw all the debug lines
	for (i = 0, debugLine = backEnd.debugLines; i < backEnd.numDebugLines; i++, debugLine++){
		// Check for view suppression
		if (!r_skipSuppress->integerValue){
			if (!(debugLine->allowInView & backEnd.viewParms.viewType))
				continue;
		}

		// Set the GL state
		if (debugLine->depthTest){
			GL_Enable(GL_DEPTH_TEST);
			GL_DepthFunc(GL_LEQUAL);
		}
		else
			GL_Disable(GL_DEPTH_TEST);

		// Set the color
		qglColor3f(debugLine->color[0], debugLine->color[1], debugLine->color[2]);

		// Draw it
		qglBegin(GL_LINES);
		qglVertex3fv(debugLine->start);
		qglVertex3fv(debugLine->end);
		qglEnd();
	}

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 ==================
 RB_AddDebugText
 ==================
*/
void RB_AddDebugText (const vec4_t color, bool forceColor, const vec3_t origin, float cw, float ch, const char *text, bool depthTest, int allowInView){

	debugText_t	*debugText;

	if (backEnd.numDebugText == backEnd.maxDebugText)
		return;		// Silently ignore

	// Add a debug text string
	debugText = &backEnd.debugText[backEnd.numDebugText++];

	debugText->allowInView = allowInView;
	debugText->depthTest = depthTest;
	MakeRGBA(debugText->color, color[0], color[1], color[2], color[3]);
	debugText->forceColor = forceColor;
	VectorCopy(origin, debugText->origin);
	debugText->cw = cw;
	debugText->ch = ch;
	Str_Copy(debugText->text, text, MAX_STRING_LENGTH);
}

/*
 ==================
 RB_ClearDebugText
 ==================
*/
void RB_ClearDebugText (){

	backEnd.numDebugText = 0;

	// Reallocate the debug text buffer if needed
	if (backEnd.maxDebugText == r_maxDebugText->integerValue)
		return;

	backEnd.maxDebugText = r_maxDebugText->integerValue;

	if (backEnd.debugText){
		Mem_Free(backEnd.debugText);
		backEnd.debugText = NULL;
	}

	if (backEnd.maxDebugText <= 0)
		return;

	backEnd.debugText = (debugText_t *)Mem_Alloc(backEnd.maxDebugText * sizeof(debugText_t), TAG_RENDERER);
}

/*
 ==================
 
 ==================
*/
static void RB_DrawDebugText (){

}


// ============================================================================


/*
 ==================
 RB_RenderDebugTools
 ==================
*/
void RB_RenderDebugTools (){

	light_t	*light;
	stage_t	*stage;
	int		i, j, k;

	if (rg.envShotRendering)
		return;

	QGL_LogPrintf("---------- RB_RenderDebugTools ----------\n");

	// Set debug rendering mode
	backEnd.debugRendering = true;

	// Unbind the index buffer
	GL_BindIndexBuffer(NULL);

	// Unbind the vertex buffer
	GL_BindVertexBuffer(NULL);

	// Render debug tools
	if (r_showDepth->integerValue)
		RB_ShowDepth();

	if (r_showOverdraw->integerValue){
		// Clear the stencil buffer
		GL_StencilMask(255);

		qglClearStencil(0);
		qglClear(GL_STENCIL_BUFFER_BIT);

		if (r_showOverdraw->integerValue != 2){
			RB_ShowOverdraw(backEnd.viewParms.numMeshes[0], backEnd.viewParms.meshes[0], false, true);
			RB_ShowOverdraw(backEnd.viewParms.numMeshes[1], backEnd.viewParms.meshes[1], false, true);
			RB_ShowOverdraw(backEnd.viewParms.numMeshes[2], backEnd.viewParms.meshes[2], false, true);
			RB_ShowOverdraw(backEnd.viewParms.numMeshes[3], backEnd.viewParms.meshes[3], false, true);
		}

		if (r_showOverdraw->integerValue != 1){
			for (i = 0; i < 4; i++){
				// Run through the lights
				for (j = 0, light = backEnd.viewParms.lights[i]; j < backEnd.viewParms.numLights[i]; j++, light++){
					if (!light->numInteractionMeshes)
						continue;

					// Set the light
					backEnd.light = light;
					backEnd.lightMaterial = light->material;

					// Evaluate registers
					RB_EvaluateRegisters(light->material, backEnd.floatTime, light->materialParms);

					// Skip if condition evaluated to false
					if (!light->material->expressionRegisters[light->material->conditionRegister])
						continue;

					// Set up the scissor
					GL_Scissor(light->scissor);

					// Run through the light stages
					for (k = 0, stage = backEnd.lightMaterial->stages; k < backEnd.lightMaterial->numStages; k++, stage++){
						if (!backEnd.lightMaterial->expressionRegisters[stage->conditionRegister])
							continue;

						// Run through the surfaces
						if (backEnd.lightMaterial->lightType == LT_BLEND || backEnd.lightMaterial->lightType == LT_FOG)
							RB_ShowOverdraw(light->numInteractionMeshes, light->interactionMeshes, true, false);
						else
							RB_ShowOverdraw(light->numInteractionMeshes, light->interactionMeshes, false, false);
					}
				}
			}

			// Restore the scissor
			GL_Scissor(backEnd.scissor);
		}

		// Draw the stencil buffer contents
		rg.pc.overdraw += RB_ShowStencil();
	}

	if (r_showLightCount->integerValue){
		// Clear the stencil buffer
		GL_StencilMask(255);

		qglClearStencil(0);
		qglClear(GL_STENCIL_BUFFER_BIT);

		for (i = 0; i < 4; i++){
			// Run through the lights
			for (j = 0, light = backEnd.viewParms.lights[i]; j < backEnd.viewParms.numLights[i]; j++, light++){
				if (!light->numInteractionMeshes)
					continue;

				// Set the light
				backEnd.light = light;
				backEnd.lightMaterial = light->material;

				// Evaluate registers
				RB_EvaluateRegisters(light->material, backEnd.floatTime, light->materialParms);

				// Skip if condition evaluated to false
				if (!light->material->expressionRegisters[light->material->conditionRegister])
					continue;

				// Set up the scissor
				GL_Scissor(light->scissor);

				// Run through the surfaces
				RB_ShowLightCount(light->numInteractionMeshes, light->interactionMeshes);
			}
		}

		// Restore the scissor
		GL_Scissor(backEnd.scissor);

		// Draw the stencil buffer contents
		rg.pc.overdrawLights += RB_ShowStencil();
	}

	if (r_showLightVolumes->integerValue){
		for (i = 0; i < 4; i++){
			// Run through the lights
			for (j = 0, light = backEnd.viewParms.lights[i]; j < backEnd.viewParms.numLights[i]; j++, light++){
				// Set the light
				backEnd.light = light;
				backEnd.lightMaterial = light->material;

				// Evaluate registers
				RB_EvaluateRegisters(light->material, backEnd.floatTime, light->materialParms);

				// Skip if condition evaluated to false
				if (!light->material->expressionRegisters[light->material->conditionRegister])
					continue;

				// Draw the light volume
				RB_ShowLightVolume();
			}
		}
	}

	if (r_showLightScissors->integerValue){
		for (i = 0; i < 4; i++){
			// Run through the lights
			for (j = 0, light = backEnd.viewParms.lights[i]; j < backEnd.viewParms.numLights[i]; j++, light++){
				// Set the light
				backEnd.light = light;
				backEnd.lightMaterial = light->material;

				// Evaluate registers
				RB_EvaluateRegisters(light->material, backEnd.floatTime, light->materialParms);

				// Skip if condition evaluated to false
				if (!light->material->expressionRegisters[light->material->conditionRegister])
					continue;

				// Draw the light scissor
				RB_ShowLightScissor(light->scissor);
			}
		}
	}

	if (r_showShadowTris->integerValue || r_showShadowVolumes->integerValue || r_showShadowSilhouettes->integerValue){
		for (i = 0; i < 4; i++){
			// Run through the lights
			for (j = 0, light = backEnd.viewParms.lights[i]; j < backEnd.viewParms.numLights[i]; j++, light++){
				// Set the light
				backEnd.light = light;
				backEnd.lightMaterial = light->material;

				// Evaluate registers
				RB_EvaluateRegisters(light->material, backEnd.floatTime, light->materialParms);

				// Skip if condition evaluated to false
				if (!light->material->expressionRegisters[light->material->conditionRegister])
					continue;

				if (r_showShadowTris->integerValue)
					RB_ShowShadowTris(light->numShadowMeshes, light->shadowMeshes);

				if (r_showShadowVolumes->integerValue)
					RB_ShowShadowVolumes(light->numShadowMeshes, light->shadowMeshes);

				if (r_showShadowSilhouettes->integerValue)
					RB_ShowShadowSilhouettes(light->numShadowMeshes, light->shadowMeshes);
			}
		}
	}

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

	R_RefreshLightEditor();

	// If a primary view, draw any debug polygons, lines, and text
	if (backEnd.viewParms.primaryView){
		RB_DrawDebugPolygons();
		RB_DrawDebugLines();
		RB_DrawDebugText();
	}

	// Clear debug rendering mode
	backEnd.debugRendering = false;

	QGL_LogPrintf("--------------------\n");
}