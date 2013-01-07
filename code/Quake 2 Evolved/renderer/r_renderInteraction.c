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
// r_renderInteraction.c - Light interaction rendering passes
//


#include "r_local.h"


/*
 ==============================================================================

 INTERACTION RENDER PATHS

 ==============================================================================
*/


/*
 ==================
 RB_DrawInteraction_Generic
 ==================
*/
static void RB_DrawInteraction_Generic (const interaction_t *i){

	interactionParms_t	*parms;

	// Bind the program
	GL_BindProgram(rg.interactionPrograms[INTERACTION_GENERIC][backEnd.light->data.type]);

	// Set up the program uniforms
	parms = &backEnd.interactionParms[INTERACTION_GENERIC][backEnd.light->data.type];

	R_UniformVector3(parms->viewOrigin, backEnd.localParms.viewOrigin);

	if (backEnd.light->data.type != RL_DIRECTIONAL)
		R_UniformVector3(parms->lightOrigin, backEnd.localParms.lightOrigin);
	else
		R_UniformVector3(parms->lightDirection, backEnd.localParms.lightDirection);

	if (backEnd.light->data.type == RL_CUBIC)
		R_UniformMatrix3(parms->lightAxis, GL_TRUE, (const float *)backEnd.localParms.lightAxis);

	R_UniformMatrix4(parms->bumpMatrix, GL_TRUE, i->bumpMatrix);
	R_UniformMatrix4(parms->diffuseMatrix, GL_TRUE, i->diffuseMatrix);
	R_UniformMatrix4(parms->specularMatrix, GL_TRUE, i->specularMatrix);
	R_UniformMatrix4(parms->lightMatrix, GL_TRUE, backEnd.localParms.lightMatrix);

	R_UniformVector2(parms->colorScaleAndBias, i->colorScaleAndBias);
	R_UniformVector3(parms->diffuseColor, i->diffuseColor);
	R_UniformVector3(parms->specularColor, i->specularColor);
	R_UniformVector2(parms->specularParms, i->specularParms);
	R_UniformVector3(parms->lightColor, i->lightColor);

	if (backEnd.light->data.type == RL_PROJECTED || backEnd.light->data.type == RL_DIRECTIONAL)
		R_UniformVector4(parms->lightPlane, backEnd.localParms.lightPlane);

	// Bind the textures
	GL_BindMultitexture(i->bumpTexture, TMU_BUMP);
	GL_BindMultitexture(i->diffuseTexture, TMU_DIFFUSE);
	GL_BindMultitexture(i->specularTexture, TMU_SPECULAR);
	GL_BindMultitexture(i->lightProjectionTexture, TMU_LIGHTPROJECTION);
	GL_BindMultitexture(i->lightFalloffTexture, TMU_LIGHTFALLOFF);

	if (backEnd.light->data.type == RL_CUBIC)
		GL_BindMultitexture(i->lightCubeTexture, TMU_LIGHTCUBE);

	// Draw it
	RB_DrawElementsWithCounters(&rg.pc.interactionIndices, &rg.pc.interactionVertices);
}


/*
 ==============================================================================

 AMBIENT INTERACTION RENDER PATHS

 ==============================================================================
*/


/*
 ==================
 RB_DrawAmbientInteraction_Generic
 ==================
*/
static void RB_DrawAmbientInteraction_Generic (const interaction_t *i){

	ambientLightParms_t	*parms;

	// Bind the program
	GL_BindProgram(rg.ambientLightPrograms[AMBIENT_GENERIC]);

	// Set up the program uniforms
	parms = &backEnd.ambientLightParms[AMBIENT_GENERIC];

	R_UniformMatrix4(parms->bumpMatrix, GL_TRUE, i->bumpMatrix);
	R_UniformMatrix4(parms->diffuseMatrix, GL_TRUE, i->diffuseMatrix);
	R_UniformMatrix4(parms->lightMatrix, GL_TRUE, backEnd.localParms.lightMatrix);
	R_UniformVector2(parms->colorScaleAndBias, i->colorScaleAndBias);
	R_UniformVector3(parms->diffuseColor, i->diffuseColor);
	R_UniformVector3(parms->lightColor, i->lightColor);

	// Bind the textures
	GL_BindMultitexture(i->bumpTexture, TMU_BUMP);
	GL_BindMultitexture(i->diffuseTexture, TMU_DIFFUSE);
	GL_BindMultitexture(i->lightProjectionTexture, TMU_LIGHTPROJECTION);
	GL_BindMultitexture(i->lightFalloffTexture, TMU_LIGHTFALLOFF);

	// Draw it
	RB_DrawElementsWithCounters(&rg.pc.interactionIndices, &rg.pc.interactionVertices);
}


// ============================================================================


/*
 ==================
 RB_DrawInteraction
 ==================
*/
void RB_DrawInteraction (interaction_t *i){

	// Development tools
	if (r_skipBump->integerValue)
		i->bumpTexture = NULL;
	if (r_skipDiffuse->integerValue)
		i->diffuseTexture = NULL;
	if (r_skipSpecular->integerValue)
		i->specularTexture = NULL;

	// Make sure we always have a bump stage
	if (i->bumpTexture == NULL){
		Matrix4_Identity(i->bumpMatrix);
		i->bumpTexture = rg.flatTexture;
	}

	// Make sure we always have a diffuse stage
	if (i->diffuseTexture == NULL){
		VectorSet(i->diffuseColor, 1.0f, 1.0f, 1.0f);
		Matrix4_Identity(i->diffuseMatrix);
		i->diffuseTexture = rg.whiteTexture;
	}

	// Make sure we always have a specular stage
	if (i->specularTexture == NULL){
		VectorSet(i->specularColor, 1.0f, 1.0f, 1.0f);
		i->specularParms[0] = 16.0f;
		i->specularParms[1] = 2.0f;
		Matrix4_Identity(i->specularMatrix);
		i->specularTexture = rg.blackTexture;
	}

	// Dispatch to the appropriate render path
	if (backEnd.lightMaterial->lightType == LT_AMBIENT){
		RB_DrawAmbientInteraction_Generic(i);
		return;
	}

	RB_DrawInteraction_Generic(i);
}