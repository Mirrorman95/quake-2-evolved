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
// r_lightInteraction.c - Light interaction meshes
//


#include "r_local.h"


/*
 ==================
 R_AddShadowMesh
 ==================
*/
static void R_AddShadowMesh (light_t *light, meshType_t type, meshData_t *data, renderEntity_t *entity, material_t *material, bool caps){

	mesh_t	*mesh;

	// Add a new mesh
	if (rg.lightMeshes.numShadows == rg.lightMeshes.maxShadows){
		Com_DPrintf(S_COLOR_YELLOW "R_AddShadowMesh: overflow\n");
		return;
	}

	mesh = &rg.lightMeshes.shadows[rg.lightMeshes.numShadows++];

	// Fill it in
	mesh->type = type;
	mesh->data = data;

	mesh->entity = entity;
	mesh->material = material;

	mesh->sort = (material->sort << MESH_SHIFT_SORT) | (entity->index << MESH_SHIFT_ENTITY) | (material->index << MESH_SHIFT_MATERIAL) | type;
	mesh->caps = caps;
}

/*
 ==================
 R_AddInteractionMesh
 ==================
*/
static void R_AddInteractionMesh (light_t *light, meshType_t type, meshData_t *data, renderEntity_t *entity, material_t *material, bool caps){

	mesh_t	*mesh;

	// Add a new mesh
	if (rg.lightMeshes.numInteractions == rg.lightMeshes.maxInteractions){
		Com_DPrintf(S_COLOR_YELLOW "R_AddInteractionMesh: overflow\n");
		return;
	}

	mesh = &rg.lightMeshes.interactions[rg.lightMeshes.numInteractions++];

	// Fill it in
	mesh->type = type;
	mesh->data = data;

	mesh->entity = entity;
	mesh->material = material;

	mesh->sort = (material->sort << MESH_SHIFT_SORT) | (entity->index << MESH_SHIFT_ENTITY) | (material->index << MESH_SHIFT_MATERIAL) | type;
	mesh->caps = caps;
}

/*
 ==================
 R_BoxInNearClipVolume
 ==================
*/
static bool R_BoxInNearClipVolume (light_t *light, const vec3_t mins, const vec3_t maxs){

	int		i;

	if (light->ncv.degenerate){
		if (BoxOnPlaneSide(mins, maxs, &rg.viewParms.frustum[FRUSTUM_NEAR]) == PLANESIDE_CROSS)
			return true;

		return false;
	}

	for (i = 0; i < NUM_FRUSTUM_PLANES; i++){
		if (BoxOnPlaneSide(mins, maxs, &light->ncv.frustum[i]) == PLANESIDE_BACK)
			return false;
	}

	return true;
}

/*
 ==================
 R_SphereInNearClipVolume
 ==================
*/
static bool R_SphereInNearClipVolume (light_t *light, const vec3_t center, float radius){

	int		i;

	if (light->ncv.degenerate){
		if (SphereOnPlaneSide(center, radius, &rg.viewParms.frustum[FRUSTUM_NEAR]) == PLANESIDE_CROSS)
			return true;

		return false;
	}

	for (i = 0; i < NUM_FRUSTUM_PLANES; i++){
		if (SphereOnPlaneSide(center, radius, &light->ncv.frustum[i]) == PLANESIDE_BACK)
			return false;
	}

	return true;
}

/*
 ==================

 TODO: make sure SURF_PLANEBACK is valid
 TODO: culling
 ==================
*/
static void R_AddLightSurface (light_t *light, surface_t *surface, material_t *material, renderEntity_t *entity, bool caps, const vec3_t lightOrigin, const vec3_t lightDirection, int cullBits, bool addShadow, bool addInteraction){

	// Cull face
	if (light->material->lightType == LT_GENERIC){
		if (!r_skipFaceCulling->integerValue && surface->plane && material->deform == DFRM_NONE){
			if (light->data.type == RL_DIRECTIONAL){
				if (material->cullType == CT_FRONT_SIDED){
					if (!(surface->flags & SURF_PLANEBACK)){
						if (DotProduct(lightDirection, surface->plane->normal) >= 0.0f)
							return;
					}
				}
				else if (material->cullType == CT_BACK_SIDED){
					if (DotProduct(lightDirection, surface->plane->normal) <= 0.0f)
						return;
				}
			}
			else {
				if (material->cullType == CT_FRONT_SIDED){
					if (!(surface->flags & SURF_PLANEBACK)){
						if (PointOnPlaneSide(lightOrigin, 0.0f, surface->plane) != PLANESIDE_FRONT)
							return;
					}
				}
				else if (material->cullType == CT_BACK_SIDED){
					if (PointOnPlaneSide(lightOrigin, 0.0f, surface->plane) != PLANESIDE_BACK)
						return;
				}
			}
		}
	}

	// Cull bounds

	// Add the draw surface
	if (addShadow)
		R_AddShadowMesh(light, MESH_SURFACE, surface, entity, material, caps);

	if (addInteraction)
		R_AddInteractionMesh(light, MESH_SURFACE, surface, entity, material, caps);
}


/*
 ==============================================================================

 WORLD LIGHT INTERACTION

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void R_PrecachedWorldLightNode (){

}

/*
 ==================
 R_RecursiveWorldLightNode
 ==================
*/
static void R_RecursiveWorldLightNode (light_t *light, node_t *node, bool lightInFrustum, bool castShadows, int skipFlag, int cullBits){

	cplane_t	*plane;
	leaf_t		*leaf;
	surface_t	*surface, **mark;
	bool		caps;
	bool		addShadow, addInteraction;
	int			side;
	int			i;

	// Check for solid content
	if (node->contents == CONTENTS_SOLID)
		return;

	// Check visibility
	if (node->viewCount != rg.viewCount){
		// Not visible, but may still cast visible shadows
		if (lightInFrustum || !castShadows)
			return;
	}

	// Cull
	if (cullBits){
		for (i = 0, plane = light->data.frustum; i < NUM_FRUSTUM_PLANES; i++, plane++){
			if (!(cullBits & BIT(i)))
				continue;

			side = BoxOnPlaneSide(node->mins, node->maxs, plane);

			if (side == PLANESIDE_BACK)
				return;

			if (side == PLANESIDE_FRONT)
				cullBits &= ~BIT(i);
		}
	}

	// Recurse down the children
	if (node->contents == -1){
		R_RecursiveWorldLightNode(light, node->children[0], lightInFrustum, castShadows, skipFlag, cullBits);
		R_RecursiveWorldLightNode(light, node->children[1], lightInFrustum, castShadows, skipFlag, cullBits);
		return;
	}

	// Leaf node
	leaf = (leaf_t *)node;

	if (!leaf->numMarkSurfaces)
		return;

	// Check if the node is in the light PVS
	if (!r_skipVisibility->integerValue){
		if (light->data.pvs){
			if (!(light->data.pvs[leaf->cluster >> 3] & BIT(leaf->cluster & 7)))
				return;
		}
	}

	// Add all the surfaces
	for (i = 0, mark = leaf->firstMarkSurface; i < leaf->numMarkSurfaces; i++, mark++){
		surface = *mark;

		if (surface->lightCount == rg.lightCount)
			continue;		// Already added this surface from another leaf
		surface->lightCount = rg.lightCount;

		if (surface->texInfo->material->spectrum != light->material->spectrum)
			continue;		// Not illuminated by this light

		// Check visibility
		if (surface->viewCount != rg.viewCount){
			// Not visible, but may still cast visible shadows
			if (lightInFrustum || !castShadows)
				return;
		}

		// Determine if we should add the shadow
		if (castShadows)
			addShadow = !(surface->texInfo->material->flags & MF_NOSHADOWS);
		else
			addShadow = false;

		// Determine if we should add the interaction
		if (surface->viewCount == rg.viewCount)
			addInteraction = !(surface->texInfo->material->flags & skipFlag);
		else
			addInteraction = false;

		// Check if there's nothing to be added
		if (!addShadow && !addInteraction)
			continue;

		// Select rendering method for shadows
		caps = R_BoxInNearClipVolume(light, surface->mins, surface->maxs);

		// Add the surface
		R_AddLightSurface(light, surface, surface->texInfo->material, rg.worldEntity, caps, light->data.origin, light->data.direction, cullBits, addShadow, addInteraction);
	}
}

/*
 ==================
 R_AddWorldLightInteractions
 ==================
*/
static void R_AddWorldLightInteractions (light_t *light, bool lightInFrustum, bool castShadows, int skipFlag){

	if (!rg.viewParms.primaryView)
		return;

	// Bump light count
	rg.lightCount++;

	// If we have precached lists of nodes and surfaces
	if (!r_skipLightCache->integerValue && light->data.precached){
		R_PrecachedWorldLightNode();
		return;
	}

	// Recurse down the BSP tree
	if (r_skipCulling->integerValue)
		R_RecursiveWorldLightNode(light, rg.worldModel->nodes, lightInFrustum, castShadows, skipFlag, 0);
	else
		R_RecursiveWorldLightNode(light, rg.worldModel->nodes, lightInFrustum, castShadows, skipFlag, 63);
}


/*
 ==============================================================================

 ENTITY LIGHT INTERACTION

 ==============================================================================
*/


/*
 ==================
 
 TODO: culling, caps
 ==================
*/
static void R_AddInlineModelLightInteractions (light_t *light, renderEntity_t *entity, bool lightInFrustum, bool castShadows, int skipFlag){

	model_t		*model = entity->model;
	surface_t	*surface;
	material_t	*material;
	bool		caps = false;
	vec3_t		lightOrigin, lightDirection;
	int			cullBits;
	bool		addShadow, addInteraction;
	int			i;

	// Check visibility
	if (entity->viewCount != rg.viewCount){
		// Not visible, but may still cast visible shadows
		if (lightInFrustum || !castShadows)
			return;
	}

	// Cull
	if (r_skipEntityCulling->integerValue || entity->depthHack)
		cullBits = 0;
	else {

	}

	// Transform light origin and light direction into local space
	R_WorldPointToLocal(light->data.origin, lightOrigin, entity->origin, entity->axis);
	R_WorldVectorToLocal(light->data.direction, lightDirection, entity->axis);

	// Add all the surfaces
	surface = model->surfaces + model->firstModelSurface;
	for (i = 0; i < model->numModelSurfaces; i++, surface++){
		// Get the material
		if (entity->material)
			material = entity->material;
		else
			material = surface->texInfo->material;

		if (material->spectrum != light->material->spectrum)
			continue;		// Not illuminated by this light

		// Determine if we should add the shadow
		if (castShadows)
			addShadow = !(material->flags & MF_NOSHADOWS);
		else
			addShadow = false;

		// Determine if we should add the interaction
		if (entity->viewCount == rg.viewCount)
			addInteraction = !(material->flags & skipFlag);
		else
			addInteraction = false;

		// Check if there's nothing to be added
		if (!addShadow && !addInteraction)
			continue;

		// Add the surface
		R_AddLightSurface(light, surface, material, entity, caps, lightOrigin, lightDirection, cullBits, addShadow, addInteraction);
	}
}

/*
 ==================
 
 TODO: culling
 TODO: might need some RF_ flag checks in here
 ==================
*/
static void R_AddAliasModelLightInteractions (light_t *light, renderEntity_t *entity, bool lightInFrustum, bool castShadows, int skipFlag){

	mdl_t			*alias = entity->model->alias;
	mdlSurface_t	*surface;
	material_t		*material;
	float			radius;
	bool			caps;
	int				cullBits;
	bool			addShadow, addInteraction;
	int				i;

	// Check visibility
	if (entity->viewCount != rg.viewCount){
		// Not visible, but may still cast visible shadows
		if (lightInFrustum || !castShadows)
			return;
	}

	// Find model radius
	radius = R_ModelRadius(alias, entity);

	// Cull
	if (r_skipEntityCulling->integerValue || entity->depthHack)
		cullBits = 0;
	else {

	}

	// Select rendering method for shadows
	caps = R_SphereInNearClipVolume(light, entity->origin, radius);

	// Add all the surfaces
	for (i = 0, surface = alias->surfaces; i < alias->numSurfaces; i++, surface++){
		// Get the material
		material = R_ModelMaterial(entity, surface);

		if (material->spectrum != light->material->spectrum)
			continue;		// Not illuminated by this light

		// Determine if we should add the shadow
		if (castShadows)
			addShadow = !(material->flags & MF_NOSHADOWS);
		else
			addShadow = false;

		// Determine if we should add the interaction
		if (entity->viewCount == rg.viewCount)
			addInteraction = !(material->flags & skipFlag);
		else
			addInteraction = false;

		// Check if there's nothing to be added
		if (!addShadow && !addInteraction)
			continue;

		// Cull

		// Add the surface
		if (addShadow)
			R_AddShadowMesh(light, MESH_ALIASMODEL, surface, entity, material, 63);

		if (addInteraction)
			R_AddInteractionMesh(light, MESH_ALIASMODEL, surface, entity, material, 63);
	}
}

/*
 ==================
 R_AddEntityLightInteractions
 ==================
*/
static void R_AddEntityLightInteractions (light_t *light, bool lightInFrustum, bool castShadows, int skipFlag){

	renderEntity_t	*entity;
	bool			suppressShadows;
	int				i;

	if (r_skipEntities->integerValue)
		return;

	for (i = 0, entity = rg.viewParms.renderEntities; i < rg.viewParms.numRenderEntities; i++, entity++){
		if (entity == rg.worldEntity)
			continue;		// World entity

		if (entity->type != RE_MODEL)
			continue;		// Not a model

		// Development tool
		if (r_singleEntity->integerValue != -1){
			if (r_singleEntity->integerValue != entity->index)
				continue;
		}

		// Check for view suppression
		if (!r_skipSuppress->integerValue)
			suppressShadows = !(entity->allowShadowInView & rg.viewParms.viewType);
		else
			suppressShadows = false;

		// FIXME: use Com_Error here or just ignore?, i did continue; for now since we SHOULD not
		// get a error...

		// Add the entity
		if (!entity->model)
			continue;

		switch (entity->model->type){
		case MODEL_INLINE:
			R_AddInlineModelLightInteractions(light, entity, lightInFrustum, castShadows && !suppressShadows && !entity->depthHack, skipFlag);
			break;
		case MODEL_MD3:
		case MODEL_MD2:
			R_AddAliasModelLightInteractions(light, entity, lightInFrustum, castShadows && !suppressShadows && !entity->depthHack, skipFlag);
			break;
		default:
			Com_Error(ERR_DROP, "R_AddEntityLightInteractions: bad model type (%i)", entity->model->type);
		}
	}
}


// ============================================================================


/*
 ==================
 R_AllocLightMeshes
 ==================
*/
void R_AllocLightMeshes (){

	rg.lightMeshes.maxShadows = MAX_MESHES << 2;
	rg.lightMeshes.maxInteractions = MAX_MESHES << 2;

	rg.lightMeshes.shadows = (mesh_t *)Mem_Alloc(rg.lightMeshes.maxShadows * sizeof(mesh_t), TAG_RENDERER);
	rg.lightMeshes.interactions = (mesh_t *)Mem_Alloc(rg.lightMeshes.maxInteractions * sizeof(mesh_t), TAG_RENDERER);
}

/*
 ==================
 
 ==================
*/
void R_GenerateLightMeshes (light_t *light){

	cplane_t	*plane;
	bool		lightInFrustum;
	bool		castShadows;
	int			skipFlag;
	int			i;

	// Clear
	light->numShadowMeshes = 0;
	light->shadowMeshes = NULL;

	light->numInteractionMeshes = 0;
	light->interactionMeshes = NULL;

	// Determine if the light origin is inside the view frustum
	if (light->data.type == RL_DIRECTIONAL)
		lightInFrustum = false;
	else {
		for (i = 0, plane = rg.viewParms.frustum; i < NUM_FRUSTUM_PLANES; i++, plane++){
			if (!(rg.viewParms.planeBits & BIT(i)))
				continue;

			// TODO: plane side
		}

		// TODO!!!
		lightInFrustum = false;
	}

	// Determine if it casts shadows
	if (light->data.noShadows || (light->material->flags & MF_NOSHADOWS))
		castShadows = false;
	else
		castShadows = true;

	// Determine the interaction skip flag
	if (light->material->lightType == LT_AMBIENT)
		skipFlag = MF_NOAMBIENT;
	else if (light->material->lightType == LT_BLEND)
		skipFlag = MF_NOBLEND;
	else if (light->material->lightType == LT_FOG)
		skipFlag = MF_NOFOG;
	else
		skipFlag = MF_NOINTERACTIONS;

	// Add the interaction meshes
	R_AddWorldLightInteractions(light, lightInFrustum, castShadows, skipFlag);
	R_AddEntityLightInteractions(light, lightInFrustum, castShadows, skipFlag);

	// Set up the interaction meshes
	light->numShadowMeshes = rg.lightMeshes.numShadows - rg.lightMeshes.firstShadow;
	light->shadowMeshes = &rg.lightMeshes.shadows[rg.lightMeshes.firstShadow];

	light->numInteractionMeshes = rg.lightMeshes.numInteractions - rg.lightMeshes.firstInteraction;
	light->interactionMeshes = &rg.lightMeshes.interactions[rg.lightMeshes.firstInteraction];

	rg.pc.shadowMeshes += light->numShadowMeshes;
	rg.pc.interactionMeshes += light->numInteractionMeshes;

	// The next light rendered in this frame will tack on after this one
	rg.lightMeshes.firstShadow = rg.lightMeshes.numShadows;
	rg.lightMeshes.firstInteraction = rg.lightMeshes.numInteractions;

	// Sort the interaction meshes
	if (r_skipSorting->integerValue)
		return;

	R_SortMeshes(light->numShadowMeshes, light->shadowMeshes);
	R_SortMeshes(light->numInteractionMeshes, light->interactionMeshes);
}

/*
 ==================
 R_ClearLightMeshes
 ==================
*/
void R_ClearLightMeshes (){

	rg.lightMeshes.numShadows = rg.lightMeshes.firstShadow = 0;
	rg.lightMeshes.numInteractions = rg.lightMeshes.firstInteraction = 0;
}