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
// r_renderCommon.c - Common rendering passes
//


#include "r_local.h"


/*
 ==============================================================================

 Z-FILL RENDERING

 ==============================================================================
*/


/*
 ==================
 RB_DrawDepth
 ==================
*/
static void RB_DrawDepth (){

	stage_t	*stage;
	bool	alphaTested = false;
	int		i;

	RB_Cull(backEnd.material);

	if (backEnd.material->coverage == MC_PERFORATED){
		for (i = 0, stage = backEnd.material->stages; i < backEnd.material->numStages; i++, stage++){
			if (stage->lighting != SL_AMBIENT && stage->lighting != SL_DIFFUSE)
				continue;

			if (!backEnd.material->expressionRegisters[stage->conditionRegister])
				continue;

			if (stage->shaderStage.program)
				continue;

			if (!(stage->drawState & DS_ALPHATEST))
				continue;

			if (!alphaTested){
				alphaTested = true;

				if (glConfig.multiSamples > 1 && r_alphaToCoverage->integerValue)
					qglEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			}

			if (stage->drawState & DS_POLYGONOFFSET){
				GL_Enable(GL_POLYGON_OFFSET_FILL);
				GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue * stage->polygonOffset);
			}
			else
				GL_Disable(GL_POLYGON_OFFSET_FILL);

			GL_Enable(GL_ALPHA_TEST);
			GL_AlphaFunc(GL_GREATER, backEnd.material->expressionRegisters[stage->alphaTestRegister]);

			RB_SetupTextureStage(backEnd.material, &stage->textureStage);
			RB_SetupColorStage(backEnd.material, &stage->colorStage);

			RB_DrawElements();

			RB_CleanupColorStage(backEnd.material, &stage->colorStage);
			RB_CleanupTextureStage(backEnd.material, &stage->textureStage);
		}

		if (alphaTested){
			qglColor3f(0.0f, 0.0f, 0.0f);

			GL_DisableTexture();

			GL_Disable(GL_ALPHA_TEST);

			if (glConfig.multiSamples > 1 && r_alphaToCoverage->integerValue)
				qglDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

			return;
		}
	}

	RB_PolygonOffset(backEnd.material);

	RB_DrawElements();
}

/*
 ==================
 RB_FillDepthBuffer
 ==================
*/
void RB_FillDepthBuffer (int numMeshes, mesh_t *meshes){

	mesh_t	*mesh;
	uint	sort;
	bool	skip;
	int		i;

	if (!numMeshes)
		return;

	QGL_LogPrintf("---------- RB_FillDepthBuffer ----------\n");

	// Set depth filling mode
	backEnd.depthFilling = true;

	// Set the GL state
	GL_DisableTexture();

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_BLEND);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_LEQUAL);

	GL_Disable(GL_STENCIL_TEST);

	if (backEnd.viewParms.primaryView)
		GL_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	else
		GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	GL_DepthMask(GL_TRUE);
	GL_StencilMask(0);

	qglColor3f(0.0f, 0.0f, 0.0f);

	// Clear the batch state
	backEnd.entity = NULL;
	backEnd.material = NULL;

	sort = 0;

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Check if the state changed
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
			RB_SetupBatch(mesh->entity, mesh->material, false, false, RB_DrawDepth);

			skip = false;
		}

		if (skip)
			continue;

		// Batch the surface geometry
		RB_BatchGeometry(mesh->type, mesh->data);
	}

	// Draw the last batch
	RB_RenderBatch();

	// Clear depth filling mode
	backEnd.depthFilling = false;

	QGL_LogPrintf("--------------------\n");
}


/*
 ==============================================================================

 AMBIENT RENDERING

 ==============================================================================
*/


/*
 ==================
 RB_DrawMaterial
 ==================
*/
static void RB_DrawMaterial (){

	stage_t	*stage;
	int		i;

	QGL_LogPrintf("----- RB_DrawMaterial ( %s ) -----\n", backEnd.material->name);

	RB_Cull(backEnd.material);

	for (i = 0, stage = backEnd.material->stages; i < backEnd.material->numStages; i++, stage++){
		if (stage->lighting != SL_AMBIENT)
			continue;

		if (!backEnd.material->expressionRegisters[stage->conditionRegister])
			continue;

		// Special case for custom shaders
		if (stage->shaderStage.program){
			if (r_skipShaders->integerValue)
				continue;

			RB_DrawState(backEnd.material, stage);

			RB_SetupShaderStage(backEnd.material, &stage->shaderStage);

			RB_DrawElements();

			RB_CleanupShaderStage(backEnd.material, &stage->shaderStage);

			continue;
		}

		// General case
		RB_DrawState(backEnd.material, stage);

		RB_SetupTextureStage(backEnd.material, &stage->textureStage);
		RB_SetupColorStage(backEnd.material, &stage->colorStage);

		RB_DrawElements();

		RB_CleanupColorStage(backEnd.material, &stage->colorStage);
		RB_CleanupTextureStage(backEnd.material, &stage->textureStage);
	}

	QGL_LogPrintf("--------------------\n");
}

/*
 ==================
 RB_RenderMaterialPasses
 ==================
*/
void RB_RenderMaterialPasses (int numMeshes, mesh_t *meshes, ambientPass_t pass){

	mesh_t	*mesh;
	uint	sort;
	bool	skip;
	int		i;

	if (!numMeshes)
		return;

	// Development tools
	if (r_skipAmbient->integerValue)
		return;

	if (r_skipTranslucent->integerValue){
		if (pass == AP_TRANSLUCENT)
			return;
	}

	if (r_skipPostProcess->integerValue){
		if (pass == AP_POST_PROCESS)
			return;
	}

	QGL_LogPrintf("---------- RB_RenderMaterialPasses ----------\n");

	// Set the GL state
	GL_PolygonMode(GL_FILL);

	GL_Enable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	if (pass == AP_POST_PROCESS)
		GL_DepthMask(GL_TRUE);
	else
		GL_DepthMask(GL_FALSE);

	GL_StencilMask(0);

	// Enable seamless cube maps if desired
	if (r_seamlessCubeMaps->integerValue)
		qglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// Clear the batch state
	backEnd.entity = NULL;
	backEnd.material = NULL;

	sort = 0;

	// Run through the meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Check if the state changed
		if (mesh->sort != sort){
			sort = mesh->sort;

			// Draw the last batch
			RB_RenderBatch();

			// Skip if it doesn't have ambient stages
			if (!mesh->material->numAmbientStages){
				skip = true;
				continue;
			}

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

			// Capture the current color if needed
			if ((mesh->material->flags & MF_NEEDCURRENTCOLOR) && !(mesh->material->flags & MF_UPDATECURRENTCOLOR)){
				if (backEnd.currentColorCaptured != mesh->material->sort){
					backEnd.currentColorCaptured = mesh->material->sort;

					R_CopyFramebufferToTexture(rg.currentColorTexture, 0, backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);
				}
			}

			// Capture the current depth if needed
			if ((mesh->material->flags & MF_NEEDCURRENTDEPTH) && !(mesh->material->flags & MF_UPDATECURRENTDEPTH)){
				if (backEnd.currentDepthCaptured != true){
					backEnd.currentDepthCaptured = true;

					R_CopyFramebufferToTexture(rg.currentDepthTexture, 0, backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);
				}
			}

			// Create a new batch
			RB_SetupBatch(mesh->entity, mesh->material, false, false, RB_DrawMaterial);

			skip = false;
		}

		if (skip)
			continue;

		// Batch the surface geometry
		RB_BatchGeometry(mesh->type, mesh->data);
	}

	// Draw the last batch
	RB_RenderBatch();

	// Disable seamless cube maps if desired
	if (r_seamlessCubeMaps->integerValue)
		qglDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	QGL_LogPrintf("--------------------\n");
}


/*
 ==============================================================================

 SHADOW & INTERACTION RENDERING

 ==============================================================================
*/


/*
 ==================
 RB_DrawShadow
 ==================
*/
static void RB_DrawShadow (){

	// TODO: update the vertex buffer and enable GL_VERTEX_ARRAY since shadows has it's own
	// buffer

	RB_Cull(backEnd.material);
	RB_PolygonOffset(backEnd.material);

	if (backEnd.shadowCaps){
		if (glConfig.stencilTwoSideAvailable){
			qglActiveStencilFaceEXT(GL_BACK);
			GL_StencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
			qglActiveStencilFaceEXT(GL_FRONT);
			GL_StencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);

			qglEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);

			RB_DrawElements();

			qglDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		}
		else if (glConfig.atiSeparateStencilAvailable){
			qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
			qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);

			RB_DrawElements();

			qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
			qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
		}
		else {
			GL_CullFace(GL_BACK);
			GL_StencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);

			RB_DrawElements();

			GL_CullFace(GL_FRONT);
			GL_StencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);

			RB_DrawElements();
		}
	}
	else {
		if (glConfig.stencilTwoSideAvailable){
			qglActiveStencilFaceEXT(GL_BACK);
			GL_StencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP_EXT);
			qglActiveStencilFaceEXT(GL_FRONT);
			GL_StencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP_EXT);

			qglEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);

			RB_DrawElements();

			qglDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		}
		else if (glConfig.atiSeparateStencilAvailable){
			qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP_EXT);
			qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP_EXT);

			RB_DrawElements();

			qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
			qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
		}
		else {
			GL_CullFace(GL_FRONT);
			GL_StencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP_EXT);

			RB_DrawElements();

			GL_CullFace(GL_BACK);
			GL_StencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP_EXT);

			RB_DrawElements();
		}
	}
}

/*
 ==================
 
 ==================
*/
static void RB_RenderStencilShadows (int numMeshes, mesh_t *meshes){

	mesh_t	*mesh;
	uint	sort;
	bool	skip;
	int		i;

	if (!numMeshes)
		return;

	// Development tool
	if (r_skipShadows->integerValue)
		return;

	QGL_LogPrintf("---------- RB_RenderStencilShadows ----------\n");

	// Set the GL state

	// Clear the batch state
	backEnd.entity = NULL;
	backEnd.material = NULL;

	sort = 0;

	// Run through meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Check if the state changed
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
			if (mesh->entity != backEnd.entity){
				RB_EntityState(mesh->entity);

				// Transform the light for this entity
				RB_TransformLightForEntity(backEnd.light, mesh->entity);
			}

			// Create a new batch
			RB_SetupBatch(mesh->entity, mesh->material, true, mesh->caps, RB_DrawShadow);

			skip = false;
		}

		if (skip)
			continue;

		// Batch the surface geometry
		RB_BatchShadowGeometry(mesh->type, mesh->data);
	}

	// Draw the last batch
	RB_RenderBatch();

	QGL_LogPrintf("--------------------\n");
}

/*
 ==================
 
 ==================
*/
static void RB_DrawInteractions (){

	interaction_t	in;
	stage_t			*stage, *lightStage;
//	cinData_t		data;
	int				i, j;

	QGL_LogPrintf("----- RB_DrawInteractions ( %s on %s ) -----\n", backEnd.lightMaterial->name, backEnd.material->name);

	RB_Cull(backEnd.material);
	RB_PolygonOffset(backEnd.material);

	qglVertexAttribPointer(GL_ATTRIB_NORMAL, 3, GL_FLOAT, false, sizeof(glVertex_t), GL_VERTEX_NORMAL(backEnd.vertexPointer));
	qglVertexAttribPointer(GL_ATTRIB_TANGENT1, 3, GL_FLOAT, false, sizeof(glVertex_t), GL_VERTEX_TANGENT1(backEnd.vertexPointer));
	qglVertexAttribPointer(GL_ATTRIB_TANGENT2, 3, GL_FLOAT, false, sizeof(glVertex_t), GL_VERTEX_TANGENT2(backEnd.vertexPointer));
	qglVertexAttribPointer(GL_ATTRIB_TEXCOORD, 2, GL_FLOAT, false, sizeof(glVertex_t), GL_VERTEX_TEXCOORD(backEnd.vertexPointer));
	qglVertexAttribPointer(GL_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, true, sizeof(glVertex_t), GL_VERTEX_COLOR(backEnd.vertexPointer));

	// Run through the light stages
	for (i = 0, lightStage = backEnd.lightMaterial->stages; i < backEnd.lightMaterial->numStages; i++, lightStage++){
		if (!backEnd.lightMaterial->expressionRegisters[lightStage->conditionRegister])
			continue;

		// Compute the light matrix
		RB_ComputeLightMatrix(backEnd.light, backEnd.entity, backEnd.lightMaterial, &lightStage->textureStage);

		// Set up the interaction
		in.bumpTexture = NULL;
		in.diffuseTexture = NULL;
		in.specularTexture = NULL;
		in.lightProjectionTexture = lightStage->textureStage.texture;
		in.lightFalloffTexture = backEnd.lightMaterial->lightFalloffImage;
		in.lightCubeTexture = backEnd.lightMaterial->lightCubeImage;

		in.colorScaleAndBias[0] = 0.0f;
		in.colorScaleAndBias[1] = 1.0f;

		in.lightColor[0] = backEnd.lightMaterial->expressionRegisters[lightStage->colorStage.registers[0]] * r_lightScale->floatValue;
		in.lightColor[1] = backEnd.lightMaterial->expressionRegisters[lightStage->colorStage.registers[1]] * r_lightScale->floatValue;
		in.lightColor[2] = backEnd.lightMaterial->expressionRegisters[lightStage->colorStage.registers[2]] * r_lightScale->floatValue;

		// If we have a cinematic
		if (lightStage->textureStage.cinematicHandle){

		}

		// Run through the surface stages
		for (j = 0, stage = backEnd.material->stages; j < backEnd.material->numStages; j++, stage++){
			if (stage->lighting == SL_AMBIENT)
				continue;

			if (!backEnd.material->expressionRegisters[stage->conditionRegister])
				continue;

			// Combine multiple stages and draw interactions
			switch (stage->lighting){
			case SL_BUMP:
				if (in.bumpTexture){
					RB_DrawInteraction(&in);

					in.diffuseTexture = NULL;
					in.specularTexture = NULL;
				}

				in.bumpTexture = stage->textureStage.texture;

				RB_ComputeTextureMatrix(backEnd.material, &stage->textureStage, in.bumpMatrix);

				break;
			case SL_DIFFUSE:
				if (in.diffuseTexture)
					RB_DrawInteraction(&in);

				in.diffuseTexture = stage->textureStage.texture;

				in.colorScaleAndBias[0] = stage->colorStage.scale;
				in.colorScaleAndBias[1] = stage->colorStage.bias;

				in.diffuseColor[0] = backEnd.material->expressionRegisters[stage->colorStage.registers[0]];
				in.diffuseColor[1] = backEnd.material->expressionRegisters[stage->colorStage.registers[1]];
				in.diffuseColor[2] = backEnd.material->expressionRegisters[stage->colorStage.registers[2]];

				RB_ComputeTextureMatrix(backEnd.material, &stage->textureStage, in.diffuseMatrix);

				break;
			case SL_SPECULAR:
				if (in.specularTexture)
					RB_DrawInteraction(&in);

				in.specularTexture = stage->textureStage.texture;

				in.specularColor[0] = backEnd.material->expressionRegisters[stage->colorStage.registers[0]];
				in.specularColor[1] = backEnd.material->expressionRegisters[stage->colorStage.registers[1]];
				in.specularColor[2] = backEnd.material->expressionRegisters[stage->colorStage.registers[2]];

				in.specularParms[0] = stage->parms[0];
				in.specularParms[1] = stage->parms[1];

				RB_ComputeTextureMatrix(backEnd.material, &stage->textureStage, in.specularMatrix);

				break;
			}
		}

		// Draw the last interaction
		if (!in.bumpTexture && !in.diffuseTexture && !in.specularTexture)
			continue;

		RB_DrawInteraction(&in);
	}

	QGL_LogPrintf("--------------------\n");
}

/*
 ==================
 RB_InteractionPass
 ==================
*/
static void RB_InteractionPass (int numMeshes, mesh_t *meshes){

	mesh_t	*mesh;
	uint	sort;
	bool	skip;
	int		i;

	if (!numMeshes)
		return;

	QGL_LogPrintf("---------- RB_InteractionPass ----------\n");

	// Set the GL state
	GL_PolygonMode(GL_FILL);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_ONE, GL_ONE);
	GL_BlendEquation(GL_FUNC_ADD);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_DEPTH_TEST);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Enable the arrays
	qglEnableVertexAttribArray(GL_ATTRIB_NORMAL);
	qglEnableVertexAttribArray(GL_ATTRIB_TANGENT1);
	qglEnableVertexAttribArray(GL_ATTRIB_TANGENT2);
	qglEnableVertexAttribArray(GL_ATTRIB_TEXCOORD);
	qglEnableVertexAttribArray(GL_ATTRIB_COLOR);

	// Clear the batch state
	backEnd.entity = NULL;
	backEnd.material = NULL;

	sort = 0;

	// Run through meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Check if the state changed
		if (mesh->sort != sort){
			sort = mesh->sort;

			// Draw the last batch
			RB_RenderBatch();

			// Development tool
			if (r_skipTranslucent->integerValue && mesh->material->coverage == MC_TRANSLUCENT){
				skip = true;
				continue;
			}

			// Evaluate registers if needed
			if (mesh->entity != backEnd.entity || mesh->material != backEnd.material)
				RB_EvaluateRegisters(mesh->material, backEnd.floatTime, mesh->entity->materialParms);

			// Skip if condition evaluated to false
			if (!mesh->material->expressionRegisters[mesh->material->conditionRegister]){
				skip = true;
				continue;
			}

			// Set the entity state if needed
			if (mesh->entity != backEnd.entity){
				RB_EntityState(mesh->entity);

				// Transform the light for this entity
				RB_TransformLightForEntity(backEnd.light, mesh->entity);
			}

			// Set the GL state
			if (mesh->material->coverage != MC_TRANSLUCENT)
				GL_DepthFunc(GL_EQUAL);
			else
				GL_DepthFunc(GL_LEQUAL);

			// Create a new batch
			RB_SetupBatch(mesh->entity, mesh->material, false, false, RB_DrawInteractions);

			skip = false;
		}

		if (skip)
			continue;

		// Batch the surface geometry
		RB_BatchGeometry(mesh->type, mesh->data);
	}

	// Draw the last batch
	RB_RenderBatch();

	// Restore the GL state
	GL_SelectTexture(0);

	// Unbind the program
	GL_BindProgram(NULL);

	// Disable the arrays
	qglDisableVertexAttribArray(GL_ATTRIB_COLOR);
	qglDisableVertexAttribArray(GL_ATTRIB_TEXCOORD);
	qglDisableVertexAttribArray(GL_ATTRIB_TANGENT2);
	qglDisableVertexAttribArray(GL_ATTRIB_TANGENT1);
	qglDisableVertexAttribArray(GL_ATTRIB_NORMAL);

	QGL_LogPrintf("--------------------\n");
}

/*
 ==================
 RB_RenderLights

 FIXME: find out what causes it not to draw, everything else seems fine
 - matrix issue?
 - texture issue?
 - light issue?
 - materialParm issue?
 - worldEntity issue?
 - backEnd.light issue?
 ==================
*/
void RB_RenderLights (int numLights, light_t *lights){

	light_t	*light;
	int		i;

	if (!numLights)
		return;

	// Development tool
	if (r_skipInteractions->integerValue)
		return;

	QGL_LogPrintf("---------- RB_RenderLights ----------\n");

	// Run through the lights
	for (i = 0, light = lights; i < numLights; i++, light++){
		if (!light->numInteractionMeshes){
			if (light->material->lightType == LT_AMBIENT)
				continue;
		}

		// Development tool
		if (r_skipAmbientLights->integerValue){
			if (light->material->lightType == LT_AMBIENT)
				continue;
		}

		// Set the light
		backEnd.light = light;
		backEnd.lightMaterial = light->material;

		// Evaluate registers
		RB_EvaluateRegisters(light->material, backEnd.floatTime, light->materialParms);

		// Skip if condition evaluated to false
		if (!light->material->expressionRegisters[light->material->conditionRegister])
			continue;

		// Render the stencil shadow volume if needed
//		RB_RenderStencilShadows(light->numShadowMeshes, light->shadowMeshes);

		// Set up the scissor
		GL_Scissor(light->scissor.x, light->scissor.y, light->scissor.width, light->scissor.height);

		// Draw the surfaces
		RB_InteractionPass(light->numInteractionMeshes, light->interactionMeshes);
	}

	// Restore the scissor
	GL_Scissor(light->scissor.x, light->scissor.y, light->scissor.width, light->scissor.height);

	QGL_LogPrintf("--------------------\n");
}


/*
 ==============================================================================

 BLEND LIGHT INTERACTION RENDERING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void RB_DrawBlendLight (){

}

/*
 ==================
 RB_BlendLightPass
 ==================
*/
static void RB_BlendLightPass (int numMeshes, mesh_t *meshes){

	mesh_t	*mesh;
	uint	sort;
	bool	skip;
	int		i;

	if (!numMeshes)
		return;

	QGL_LogPrintf("---------- RB_BlendLightPass ----------\n");

	// Set the GL state
	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_EQUAL);

	GL_Disable(GL_STENCIL_TEST);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Bind the program
	GL_BindProgram(rg.blendLightProgram);

	// Clear the batch state
	backEnd.entity = NULL;
	backEnd.material = NULL;

	sort = 0;

	// Run through meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Check if the state changed
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
			if (mesh->entity != backEnd.entity){
				RB_EntityState(mesh->entity);

				// Transform the light for this entity
				RB_TransformLightForEntity(backEnd.light, mesh->entity);
			}

			// Create a new batch
			RB_SetupBatch(mesh->entity, mesh->material, false, false, RB_DrawBlendLight);

			skip = false;
		}

		if (skip)
			continue;

		// Batch the surface geometry
		RB_BatchGeometry(mesh->type, mesh->data);
	}

	// Draw the last batch
	RB_RenderBatch();

	// Restore the GL state
	GL_SelectTexture(0);

	// Unbind the program
	GL_BindProgram(NULL);

	QGL_LogPrintf("--------------------\n");
}

/*
 ==================
 
 ==================
*/
void RB_RenderBlendLights (int numLights, light_t *lights){

	if (!numLights)
		return;
}


/*
 ==============================================================================

 FOG LIGHT INTERACTION RENDERING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void RB_DrawFogLight (){

}

/*
 ==================
 
 ==================
*/
static void RB_DrawFogLightPlane (){

}

/*
 ==================
 
 ==================
*/
static void RB_DrawFogLightVolume (){

}

/*
 ==================
 RB_FogLightPass
 ==================
*/
static void RB_FogLightPass (int numMeshes, mesh_t *meshes){

	mesh_t	*mesh;
	uint	sort;
	bool	skip;
	int		i;

	if (!numMeshes)
		return;

	QGL_LogPrintf("---------- RB_FogLightPass ----------\n");

	// Set the GL state
	GL_PolygonMode(GL_FILL);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_BlendEquation(GL_FUNC_ADD);

	GL_Disable(GL_ALPHA_TEST);

	GL_Enable(GL_DEPTH_TEST);
	GL_DepthFunc(GL_EQUAL);

	GL_Enable(GL_STENCIL_TEST);
	GL_StencilFunc(GL_LESS, 128, 255);
	GL_StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Bind the program
	GL_BindProgram(rg.fogLightProgram);

	// Clear the batch state
	backEnd.entity = NULL;
	backEnd.material = NULL;

	sort = 0;

	// Run through meshes
	for (i = 0, mesh = meshes; i < numMeshes; i++, mesh++){
		// Check if the state changed
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
			if (mesh->entity != backEnd.entity){
				RB_EntityState(mesh->entity);

				// Transform the light for this entity
				RB_TransformLightForEntity(backEnd.light, mesh->entity);
			}

			// Create a new batch
			RB_SetupBatch(mesh->entity, mesh->material, false, false, RB_DrawFogLight);

			skip = false;
		}

		if (skip)
			continue;

		// Batch the surface geometry
		RB_BatchGeometry(mesh->type, mesh->data);
	}

	// Draw the last batch
	RB_RenderBatch();

	// Restore the GL state
	GL_SelectTexture(0);

	// Unbind the program
	GL_BindProgram(NULL);

	QGL_LogPrintf("--------------------\n");
}

/*
 ==================
 
 ==================
*/
void RB_RenderFogLights (int numLights, light_t *lights){

	if (!numLights)
		return;
}


/*
 ==============================================================================

 2D RENDERING

 ==============================================================================
*/


/*
 ==================
 RB_DrawMaterial2D
 ==================
*/
void RB_DrawMaterial2D (){

	stage_t	*stage;
	int		i;

	QGL_LogPrintf("----- RB_DrawMaterial2D ( %s ) -----\n", backEnd.material->name);

	for (i = 0, stage = backEnd.material->stages; i < backEnd.material->numStages; i++, stage++){
		if (stage->lighting != SL_AMBIENT)
			continue;

		if (!backEnd.material->expressionRegisters[stage->conditionRegister])
			continue;

		// Special case for custom shaders
		if (stage->shaderStage.program){
			if (r_skipShaders->integerValue)
				continue;

			RB_DrawState(backEnd.material, stage);

			RB_SetupShaderStage(backEnd.material, &stage->shaderStage);

			RB_DrawElements();

			RB_CleanupShaderStage(backEnd.material, &stage->shaderStage);

			continue;
		}

		// General case
		RB_DrawState(backEnd.material, stage);

		RB_SetupTextureStage(backEnd.material, &stage->textureStage);
		RB_SetupColorStage(backEnd.material, &stage->colorStage);

		RB_DrawElements();

		RB_CleanupColorStage(backEnd.material, &stage->colorStage);
		RB_CleanupTextureStage(backEnd.material, &stage->textureStage);
	}

	QGL_LogPrintf("--------------------\n");
}