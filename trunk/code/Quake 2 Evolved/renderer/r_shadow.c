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
// r_shadow.c - Shadow management
//


#include "r_local.h"


/*
 ==================
 R_AddAliasModelShadows
 ==================
*/
static void R_AddAliasModelShadows (renderEntity_t *entity){

	mdl_t			*alias = entity->model->alias;
	mdlSurface_t	*surface;
	material_t		*material;
	int				i;

	// Skip casting shadows from the viewer model if desierd
	if (entity->renderFX & RF_VIEWERMODEL){
		if (!r_playerShadow->integerValue)
			return;
	}

	// Never cast shadows from weapon model
	if (entity->renderFX & RF_WEAPONMODEL)
		return;

	// Add all the surfaces
	for (i = 0, surface = alias->surfaces; i < alias->numSurfaces; i++, surface++){
		// Select material
		if (entity->material)
			material = entity->material;
		else {
			if (surface->numMaterials){
				if (entity->skinIndex < 0 || entity->skinIndex >= surface->numMaterials){
					Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelShadows: no such material %i (%s)\n", entity->skinIndex, entity->model->name);

					entity->skinIndex = 0;
				}

				material = surface->materials[entity->skinIndex].material;
			}
			else {
				Com_DPrintf(S_COLOR_YELLOW "R_AddAliasModelShadows: no materials for surface (%s)\n", entity->model->name);

				material = rg.defaultMaterial;
			}
		}

		if (material->flags & MF_NOSHADOWS)
			continue;		// Don't bother drawing

		// Add the surface
		R_AddMeshToList(MESH_ALIASMODEL, surface, entity, material);
	}
}

/*
 ==================
 R_AddEntityShadows
 ==================
*/
void R_AddEntityShadows (){

	renderEntity_t	*entity;
	int				i;

	if (r_skipEntities->integerValue)
		return;

	for (i = 0, entity = rg.viewParms.renderEntities; i < rg.viewParms.numRenderEntities; i++, entity++){
		if (entity->type != RE_MODEL)
			continue;

		if (!entity->model)
			continue;

		switch (entity->model->type){
		case MODEL_INLINE:
			break;
		case MODEL_MD3:
		case MODEL_MD2:
			R_AddAliasModelShadows(entity);
			break;
		}
	}
}