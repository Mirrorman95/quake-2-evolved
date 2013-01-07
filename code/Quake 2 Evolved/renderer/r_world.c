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
// r_world.c - World surfaces
//


#include "r_local.h"


/*
 ==================
 R_CullSurface

 FIXME: make sure SURF_PLANEBACK is valid
 ==================
*/
static bool R_CullSurface (surface_t *surface, material_t *material, renderEntity_t *entity, const vec3_t viewOrigin, int cullBits){

	// Cull face
	if (!r_skipFaceCulling->integerValue && surface->plane && material->deform == DFRM_NONE){
		if (material->cullType == CT_FRONT_SIDED){
			if (!(surface->flags & SURF_PLANEBACK)){
				if (PointOnPlaneSide(viewOrigin, 0.0f, surface->plane) != PLANESIDE_FRONT)
					return true;
			}
		}
		else if (material->cullType == CT_BACK_SIDED){
			if (!(surface->flags & SURF_PLANEBACK)){
				if (PointOnPlaneSide(viewOrigin, 0.0f, surface->plane) != PLANESIDE_BACK)
					return true;
			}
		}
	}

	// Cull bounds
	if (cullBits != CULL_IN){
		if (entity == rg.worldEntity){
			if (R_CullBounds(surface->mins, surface->maxs, cullBits) == CULL_OUT)
				return true;
		}
		else {
			if (R_CullLocalBounds(surface->mins, surface->maxs, entity->origin, entity->axis, cullBits) == CULL_OUT)
				return true;
		}
	}

	return false;
}

/*
 ==================
 R_AddSurface

 FIXME: texture frame animation does not work
 ==================
*/
static void R_AddSurface (surface_t *surface, renderEntity_t *entity){

	texInfo_t	*texInfo = surface->texInfo;
	material_t	*material;
	int			count;

	// Mark as visible for this view
	surface->viewCount = rg.viewCount;

	// Select the material
	if (texInfo->next){
		count = entity->frame % texInfo->numFrames;
		while (count){
			texInfo = texInfo->next;
			count--;
		}
	}

	material = texInfo->material;

	// Add a subview surface if needed
	if (material->subviewType != ST_NONE){
		if (!R_AddSubviewSurface(MESH_SURFACE, surface, entity, material))
			return;
	}

	// Add it
	R_AddMeshToList(MESH_SURFACE, surface, entity, material);

	// Also add caustics
	if (r_caustics->integerValue){
		if (surface->flags & SURF_WATERCAUSTICS)
			R_AddMeshToList(MESH_SURFACE, surface, entity, rg.waterCausticsMaterial);
		if (surface->flags & SURF_SLIMECAUSTICS)
			R_AddMeshToList(MESH_SURFACE, surface, entity, rg.slimeCausticsMaterial);
		if (surface->flags & SURF_LAVACAUSTICS)
			R_AddMeshToList(MESH_SURFACE, surface, entity, rg.lavaCausticsMaterial);
	}
}


/*
 ==============================================================================

 INLINE MODELS

 ==============================================================================
*/


/*
 ==================
 R_AddInlineModel

 TODO: rewrite culling
 ==================
*/
void R_AddInlineModel (renderEntity_t *entity){

	model_t		*model = entity->model;
	surface_t	*surface;
	vec3_t		viewOrigin, tmp;
	vec3_t		mins, maxs;
	int			i;

	if (!model->numModelSurfaces)
		return;

	// Cull
	if (!Matrix3_Compare(entity->axis, mat3_identity)){
		for (i = 0; i < 3; i++){
			mins[i] = entity->origin[i] - model->radius;
			maxs[i] = entity->origin[i] + model->radius;
		}

		if (R_CullSphere(entity->origin, model->radius, rg.viewParms.planeBits))
			return;

		VectorSubtract(rg.renderView.origin, entity->origin, tmp);
		VectorRotate(tmp, entity->axis, viewOrigin);
	}
	else {
		VectorAdd(entity->origin, model->mins, mins);
		VectorAdd(entity->origin, model->maxs, maxs);

		if (R_CullBox(mins, maxs, rg.viewParms.planeBits))
			return;

		VectorSubtract(rg.renderView.origin, entity->origin, viewOrigin);
	}

	// Mark as visible for this view
	entity->viewCount = rg.viewCount;

	rg.pc.entities++;

	// Add all the surfaces
	surface = model->surfaces + model->firstModelSurface;
	for (i = 0; i < model->numModelSurfaces; i++, surface++){
		if (surface->texInfo->flags & SURF_SKY)
			continue;		// Don't bother drawing

		if (!surface->texInfo->material->numStages)
			continue;		// Don't bother drawing

		// Cull
		if (R_CullSurface(surface, surface->texInfo->material, entity, viewOrigin, CULL_IN))
			continue;

		// Add the surface
		R_AddSurface(surface, entity);
	}
}


/*
 ==============================================================================

 WORLD SURFACES

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void R_MarkLeaves (){

	byte	vis[MAX_MAP_LEAFS/8], fatVis[MAX_MAP_LEAFS / 8];
	node_t	*node;
	leaf_t	*leaf;
	vec3_t	viewOrigin;
	int		i, c;

	if (!rg.worldModel)
		Com_Error(ERR_DROP, "R_MarkLeaves: NULL world");

	// Development tool
	if (r_lockVisibility->integerValue)
		return;

	// Get the current view leaf
	rg.oldViewCluster = rg.viewCluster;
	rg.oldViewCluster2 = rg.viewCluster2;

	leaf = R_PointInLeaf(rg.renderView.origin);

	// If the current view area has changed, edit the post-process parameters
	if (rg.viewParms.viewType == VIEW_MAIN){
		if (rg.viewArea != leaf->area)
			R_EditAreaPostProcess(leaf->area);

		rg.viewArea = leaf->area;
	}

	// Development tool
	if (r_showCluster->integerValue)
		Com_Printf("Cluster: %i, Area: %i\n", leaf->cluster, leaf->area);

	rg.viewCluster = rg.viewCluster2 = leaf->cluster;

	// Check above and below so crossing solid water doesn't draw wrong
	if (!leaf->contents){
		// Look down a bit
		VectorCopy(rg.renderView.origin, viewOrigin);

		viewOrigin[2] -= 16.0f;

		leaf = R_PointInLeaf(viewOrigin);
		if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != rg.viewCluster2))
			rg.viewCluster2 = leaf->cluster;
	}
	else {
		// Look up a bit
		VectorCopy(rg.renderView.origin, viewOrigin);

		viewOrigin[2] += 16.0f;

		leaf = R_PointInLeaf(viewOrigin);
		if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != rg.viewCluster2))
			rg.viewCluster2 = leaf->cluster;
	}

	if (rg.viewCluster == rg.oldViewCluster && rg.viewCluster2 == rg.oldViewCluster2 && !r_skipVisibility->integerValue && rg.viewCluster != -1)
		return;

	rg.visCount++;
	rg.oldViewCluster = rg.viewCluster;
	rg.oldViewCluster2 = rg.viewCluster2;

	// Mark everything if needed
	if (r_skipVisibility->integerValue || rg.viewCluster == -1 || !rg.worldModel->vis){
		for (i = 0, leaf = rg.worldModel->leafs; i < rg.worldModel->numLeafs; i++, leaf++)
			leaf->visCount = rg.visCount;
		for (i = 0, node = rg.worldModel->nodes; i < rg.worldModel->numNodes; i++, node++)
			node->visCount = rg.visCount;

		return;
	}

	// May have to combine two clusters because of solid water 
	// boundaries
	R_ClusterPVS(rg.viewCluster, vis);

	if (rg.viewCluster != rg.viewCluster2){
		R_ClusterPVS(rg.viewCluster2, vis);

		c = (rg.worldModel->numLeafs + 31) / 32;
		for (i = 0; i < c; i++)
			((int *)vis)[i] |= ((int *)fatVis)[i];
	}
	
	// Mark the leaves and nodes that are visible from the current cluster
	for (i = 0, leaf = rg.worldModel->leafs; i < rg.worldModel->numLeafs; i++, leaf++){
		if (leaf->cluster == -1)
			continue;

		if (!(vis[leaf->cluster >> 3] & (1 << (leaf->cluster & 7))))
			continue;

		node = (node_t *)leaf;
		do {
			if (node->visCount == rg.visCount)
				break;
			node->visCount = rg.visCount;

			node = node->parent;
		} while (node);
	}
}

/*
 ==================
 R_RecursiveWorldNode
 ==================
*/
static void R_RecursiveWorldNode (node_t *node, int cullBits){

	leaf_t		*leaf;
	surface_t	*surface, **mark;
	int			i;

	// Check for solid content
	if (node->contents == CONTENTS_SOLID)
		return;

	// Check visibility
	if (node->visCount != rg.visCount)
		return;

	// Cull
	if (cullBits != CULL_IN){
		cullBits = R_CullBounds(node->mins, node->maxs, cullBits);

		if (cullBits == CULL_OUT)
			return;
	}

	// Mark as visible for this view
	node->viewCount = rg.viewCount;

	// Recurse down the children
	if (node->contents == -1){
		R_RecursiveWorldNode(node->children[0], cullBits);
		R_RecursiveWorldNode(node->children[1], cullBits);
		return;
	}

	// If a leaf node, draw stuff
	leaf = (leaf_t *)node;

	if (!leaf->numMarkSurfaces)
		return;

	// Check for door connected areas
	if (rg.renderView.areaBits){
		if (!(rg.renderView.areaBits[leaf->area >> 3] & (1 << (leaf->area & 7))))
			return;		// Not visible
	}

	// Development tool
	if (r_showLeafBounds->integerValue)
		R_DebugBounds(colorWhite, leaf->mins, leaf->maxs, true, rg.viewParms.viewType);

	// Add to the vis bounds
	AddPointToBounds(leaf->mins, rg.viewParms.visMins, rg.viewParms.visMaxs);
	AddPointToBounds(leaf->maxs, rg.viewParms.visMins, rg.viewParms.visMaxs);

	rg.pc.leafs++;

	// If it has a single surface, no need to cull again
	if (leaf->numMarkSurfaces == 1)
		cullBits = CULL_IN;

	// Add all the surfaces
	for (i = 0, mark = leaf->firstMarkSurface; i < leaf->numMarkSurfaces; i++, mark++){
		surface = *mark;

		if (surface->worldCount == rg.frameCount)
			continue;		// Already added this surface from another leaf
		surface->worldCount = rg.frameCount;

		if (!surface->texInfo->material->numStages)
			continue;		// Don't bother drawing

		// Cull
		if (R_CullSurface(surface, surface->texInfo->material, rg.worldEntity, rg.renderView.origin, cullBits))
			continue;

		// Add the surface
		R_AddSurface(surface, rg.worldEntity);
	}
}

/*
 ==================
 R_AddWorldSurfaces
 ==================
*/
void R_AddWorldSurfaces (){

	if (!rg.viewParms.primaryView)
		return;

	// Bump frame count
	rg.frameCount++;

	// Auto cycle the world frame for texture animation
	rg.worldEntity->frame = (int)(rg.renderView.time * 2);

	// Clear world mins/maxs
	ClearBounds(rg.viewParms.visMins, rg.viewParms.visMaxs);

	// Mark leaves
	R_MarkLeaves();

	// Recurse down the BSP tree
	if (r_skipCulling->integerValue)
		R_RecursiveWorldNode(rg.worldModel->nodes, 0);
	else
		R_RecursiveWorldNode(rg.worldModel->nodes, rg.viewParms.planeBits);

	// Now that we have the vis bounds, set the far clip plane
	R_SetFarClip();
}