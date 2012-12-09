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
// r_scene.c - Scene rendering
//


#include "r_local.h"


/*
 ==============================================================================

 RENDER ENTITIES

 ==============================================================================
*/


/*
 ==================
 R_AddEntityToScene
 ==================
*/
void R_AddEntityToScene (const renderEntity_t *renderEntity){

	renderEntity_t	*entity;

	if (rg.scene.numEntities >= MAX_RENDER_ENTITIES){
		Com_DPrintf(S_COLOR_YELLOW "R_AddEntityToScene: MAX_RENDER_ENTITIES hit\n");
		return;
	}

	entity = &rg.scene.entities[rg.scene.numEntities++];

	*entity = *renderEntity;
	entity->index = rg.scene.numEntities - 1;
}


/*
 ==============================================================================

 RENDER LIGHTS

 ==============================================================================
*/


/*
 ==================
 R_AddLightToScene
 ==================
*/
void R_AddLightToScene (const renderLight_t *renderLight){

	renderLight_t	*light;

	if (rg.scene.numLights >= MAX_RENDER_LIGHTS){
		Com_DPrintf(S_COLOR_YELLOW "R_AddLightToScene: MAX_RENDER_LIGHTS hit\n");
		return;
	}

	light = &rg.scene.lights[rg.scene.numLights++];

	*light = *renderLight;
	light->index = rg.scene.numLights - 1;
}


/*
 ==============================================================================

 RENDER PARTICLES

 ==============================================================================
*/


/*
 ==================
 R_AddParticleToScene
 ==================
*/
void R_AddParticleToScene (const renderParticle_t *renderParticle){

	renderParticle_t	*particle;

	if (rg.scene.numParticles >= MAX_RENDER_PARTICLES){
		Com_DPrintf(S_COLOR_YELLOW "R_AddParticleToScene: MAX_RENDER_PARTICLES hit\n");
		return;
	}

	particle = &rg.scene.particles[rg.scene.numParticles++];

	*particle = *renderParticle;
}


/*
 ==============================================================================

 RENDER SCENE

 ==============================================================================
*/


/*
 ==================
 R_RenderScene
 ==================
*/
void R_RenderScene (const renderView_t *renderView, bool primaryView){

	int		timeFrontEnd;

	if (r_skipFrontEnd->integerValue)
		return;

	if (com_speeds->integerValue)
		timeFrontEnd = Sys_Milliseconds();

	// Copy render view
	rg.renderView = *renderView;

	// If a primary view
	if (primaryView){
		if (!rg.worldModel)
			Com_Error(ERR_DROP, "R_RenderScene: NULL world");

		// Save the primary view
		rg.primaryViewAvailable = true;

		rg.primaryView.numEntities = rg.scene.numEntities;
		rg.primaryView.firstEntity = rg.scene.firstEntity;

		rg.primaryView.numLights = rg.scene.numLights;
		rg.primaryView.firstLight = rg.scene.firstLight;

		rg.primaryView.numParticles = rg.scene.numParticles;
		rg.primaryView.firstParticle = rg.scene.firstParticle;

		rg.primaryView.renderView = rg.renderView;
	}

	// Set up scene render lists
	rg.viewParms.renderEntities = &rg.scene.entities[rg.scene.firstEntity];
	rg.viewParms.numRenderEntities = rg.scene.numEntities - rg.scene.firstEntity;

	rg.viewParms.renderLights = &rg.scene.lights[rg.scene.firstLight];
	rg.viewParms.numRenderLights = rg.scene.numLights - rg.scene.firstLight;

	rg.viewParms.renderParticles = &rg.scene.particles[rg.scene.firstParticle];
	rg.viewParms.numRenderParticles = rg.scene.numParticles - rg.scene.firstParticle;

	// Render main view
	R_RenderView(primaryView, VIEW_MAIN);

	if (com_speeds->integerValue)
		com_timeFrontEnd += (Sys_Milliseconds() - timeFrontEnd);
}

/*
 ==================
 R_ClearScene
 ==================
*/
void R_ClearScene (){

	rg.scene.firstEntity = rg.scene.numEntities;
	rg.scene.firstLight = rg.scene.numLights;
	rg.scene.firstParticle = rg.scene.numParticles;
}