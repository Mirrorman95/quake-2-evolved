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
// r_alias.c - Alias model code
//


#include "r_local.h"


/*
 ==============================================================================

 RENDERING PASS

 ==============================================================================
*/


/*
 ==================
 R_CullAliasModel
 ==================
*/
static bool R_CullAliasModel (renderEntity_t *entity, mdl_t *alias){

	mdlFrame_t	*curFrame, *oldFrame;
	cplane_t	*plane;
	vec3_t		mins, maxs, corner, bbox[8];
	int			mask, aggregateMask = ~0;
	int			i, j;

	if (r_skipCulling->integerValue)
		return false;

	if (entity->renderFX & RF_VIEWERMODEL)
		return true;
	if (entity->renderFX & RF_WEAPONMODEL)
		return false;

	// Compute axially aligned mins and maxs
	curFrame = alias->frames + entity->frame;
	oldFrame = alias->frames + entity->oldFrame;

	if (curFrame == oldFrame){
		VectorCopy(curFrame->mins, mins);
		VectorCopy(curFrame->maxs, maxs);
	}
	else {
		VectorMin(curFrame->mins, oldFrame->mins, mins);
		VectorMax(curFrame->maxs, oldFrame->maxs, maxs);
	}

	// Compute the corners of the bounding volume
	for (i = 0; i < 8; i++){
		corner[0] = (i & 1) ? mins[0] : maxs[0];
		corner[1] = (i & 2) ? mins[1] : maxs[1];
		corner[2] = (i & 4) ? mins[2] : maxs[2];

		// Rotate and translate
		VectorRotate(corner, entity->axis, bbox[i]);
		VectorAdd(bbox[i], entity->origin, bbox[i]);
	}

	// Check against frustum planes
	for (i = 0; i < 8; i++){
		mask = 0;

		for (j = 0, plane = rg.viewParms.frustum; j < NUM_FRUSTUM_PLANES; j++, plane++){
			if (DotProduct(bbox[i], plane->normal) - plane->dist < 0.0f)
				mask |= BIT(j);
		}

		aggregateMask &= mask;
	}

	if (aggregateMask){
		rg.pc.cullBoundsOut++;
		return true;
	}

	rg.pc.cullBoundsIn++;

	return false;
}

/*
 ==================
 R_AddAliasModel
 ==================
*/
void R_AddAliasModel (renderEntity_t *entity){

	mdl_t			*alias = entity->model->alias;
	mdlSurface_t	*surface;
	material_t		*material;
	int				i;

	// Check frames
	if ((entity->frame < 0 || entity->frame >= alias->numFrames) || (entity->oldFrame < 0 || entity->oldFrame >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModel: no such frame %i to %i (%s)\n", entity->frame, entity->oldFrame, entity->model->name);
		
		entity->frame = 0;
		entity->oldFrame = 0;
	}

	// Cull
	if (R_CullAliasModel(entity, alias))
		return;

	// Mark as visible for this view
	entity->viewCount = rg.viewCount;

	rg.pc.entities++;

	// Add all the surfaces
	for (i = 0, surface = alias->surfaces; i < alias->numSurfaces; i++, surface++){
		// Select the material
		if (entity->material)
			material = entity->material;
		else {
			if (surface->numMaterials){
				if (entity->skinIndex < 0 || entity->skinIndex >= surface->numMaterials){
					Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModel: no such material %i (%s)\n", entity->skinIndex, entity->model->name);

					entity->skinIndex = 0;
				}

				material = surface->materials[entity->skinIndex].material;
			}
			else {
				Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModel: no materials for surface (%s)\n", entity->model->name);

				material = rg.defaultMaterial;
			}
		}

		if (!material->numStages)
			continue;

		// Add it
		R_AddMeshToList(MESH_ALIASMODEL, surface, entity, material);
	}
}