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

// TODO:
// - move cull/surface list into their own files


#include "r_local.h"


#define BACKFACE_EPSILON	0.01f


/*
 ==================
 
 ==================
*/
static bool R_CullSurface (surface_t *surf, const vec3_t origin, int clipFlags){

	cplane_t	*plane;
	float		dist;

	if (r_skipCulling->integerValue)
		return false;

	// Find which side of the node we are on
	plane = surf->plane;
	if (plane->type < 3)
		dist = origin[plane->type] - plane->dist;
	else
		dist = DotProduct(origin, plane->normal) - plane->dist;

	if (!(surf->flags & SURF_PLANEBACK)){
		if (dist <= BACKFACE_EPSILON)
			return true;	// Wrong side
	}
	else {
		if (dist >= -BACKFACE_EPSILON)
			return true;	// Wrong side
	}

	// Cull
	if (clipFlags){
		if (R_CullBox(surf->mins, surf->maxs, clipFlags))
			return true;
	}

	return false;
}

/*
 ==================
 
 ==================
*/
static void R_AddSurfaceToList (surface_t *surface, renderEntity_t *entity){

	texInfo_t	*tex = surface->texInfo;
	material_t	*material;
	int			c, map, lmNum;

	if (tex->flags & SURF_NODRAW)
		return;

	// Select shader
	if (tex->next){
		c = entity->frame % tex->numFrames;
		while (c){
			tex = tex->next;
			c--;
		}
	}

	material = tex->material;

	// Select lightmap
	lmNum = surface->lmNum;

	// Check for lightmap modification
	if (r_dynamicLights->integerValue /*&& (material->flags & MF_HASLIGHTMAP)*/){
		if (surface->dlightFrame == rg.frameCount)
			lmNum = 255;
		else {
			for (map = 0; map < surface->numStyles; map++){
				if (surface->cachedLight[map] != rg.lightStyles[surface->styles[map]].white){
					lmNum = 255;
					break;
				}
			}
		}
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
 
 ==================
*/
void R_AddInlineModel (renderEntity_t *entity){

	model_t			*model = entity->model;
	surface_t		*surface;
	renderLight_t	*dl;
	vec3_t			origin, tmp;
	vec3_t			mins, maxs;
	int				i, l;

	if (!model->numModelSurfaces)
		return;

	// Cull
	if (!Matrix3_Compare(entity->axis, mat3_identity)){
		for (i = 0; i < 3; i++){
			mins[i] = entity->origin[i] - model->radius;
			maxs[i] = entity->origin[i] + model->radius;
		}

		if (R_CullSphere(entity->origin, model->radius, 15))
			return;

		VectorSubtract(rg.renderView.origin, entity->origin, tmp);
		VectorRotate(tmp, entity->axis, origin);
	}
	else {
		VectorAdd(entity->origin, model->mins, mins);
		VectorAdd(entity->origin, model->maxs, maxs);

		if (R_CullBox(mins, maxs, 15))
			return;

		VectorSubtract(rg.renderView.origin, entity->origin, origin);
	}

	// Calculate dynamic lighting
	if (r_dynamicLights->integerValue){
		for (l = 0, dl = rg.scene.lights; l < rg.scene.numLights; l++, dl++){
			if (!BoundsAndSphereIntersect(mins, maxs, dl->origin, dl->intensity))
				continue;

			surface = model->surfaces + model->firstModelSurface;
			for (i = 0; i < model->numModelSurfaces; i++, surface++){
				if (surface->dlightFrame != rg.frameCount){
					surface->dlightFrame = rg.frameCount;
					surface->dlightBits = (1 << l);
				}
				else
					surface->dlightBits |= (1 << l);
			}
		}
	}

	// Add all the surfaces
	surface = model->surfaces + model->firstModelSurface;
	for (i = 0; i < model->numModelSurfaces; i++, surface++){
		// Cull
		if (R_CullSurface(surface, origin, 0))
			continue;

		// Add the surface
		R_AddSurfaceToList(surface, entity);
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

	byte	*vis, fatVis[MAX_MAP_LEAFS / 8];
	node_t	*node;
	leaf_t	*leaf;
	vec3_t	tmp;
	int		i, c;

	// Current view cluster
	rg.oldViewCluster = rg.viewCluster;
	rg.oldViewCluster2 = rg.viewCluster2;

	leaf = R_PointInLeaf(rg.renderView.origin);

	// Development tool
	if (r_showCluster->integerValue)
		Com_Printf("Cluster: %i, Area: %i\n", leaf->cluster, leaf->area);

	rg.viewCluster = rg.viewCluster2 = leaf->cluster;

	// Check above and below so crossing solid water doesn't draw wrong
	if (!leaf->contents){
		// Look down a bit
		VectorCopy(rg.renderView.origin, tmp);
		tmp[2] -= 16.0f;
		leaf = R_PointInLeaf(tmp);
		if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != rg.viewCluster2))
			rg.viewCluster2 = leaf->cluster;
	}
	else {
		// Look up a bit
		VectorCopy(rg.renderView.origin, tmp);
		tmp[2] += 16.0f;
		leaf = R_PointInLeaf(tmp);
		if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != rg.viewCluster2))
			rg.viewCluster2 = leaf->cluster;
	}

	if (rg.viewCluster == rg.oldViewCluster && rg.viewCluster2 == rg.oldViewCluster2 && !r_skipVisibility->integerValue && rg.viewCluster != -1)
		return;

	// Development aid to let you run around and see exactly where the
	// PVS ends
	if (r_lockVisibility->integerValue)
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
	vis = R_ClusterPVS(rg.viewCluster);
	if (rg.viewCluster != rg.viewCluster2){
		Mem_Copy(fatVis, vis, (rg.worldModel->numLeafs + 7) / 8);
		vis = R_ClusterPVS(rg.viewCluster2);
		c = (rg.worldModel->numLeafs + 31) / 32;
		for (i = 0; i < c; i++)
			((int *)fatVis)[i] |= ((int *)vis)[i];

		vis = fatVis;
	}
	
	// Mark the leaves and nodes that are visible from the current cluster
	for (i = 0, leaf = rg.worldModel->leafs; i < rg.worldModel->numLeafs; i++, leaf++){
		if (leaf->cluster == -1)
			continue;

		if (!(vis[leaf->cluster >> 3] & (1<<(leaf->cluster & 7))))
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
 
 ==================
*/
static void R_RecursiveWorldNode (node_t *node, int clipFlags){

	leaf_t		*leaf;
	surface_t	*surface, **mark;
	cplane_t	*plane;
	int			clipped;
	int			i;

	// Check for solid content
	if (node->contents == CONTENTS_SOLID)
		return;

	// Check visibility
	if (node->visCount != rg.visCount)
		return;

	// Cull
	if (clipFlags){
		for (i = 0, plane = rg.viewParms.frustum; i < 4; i++, plane++){
			if (!(clipFlags & (1 << i)))
				continue;

			clipped = BoxOnPlaneSide(node->mins, node->maxs, plane);
			if (clipped == 2)
				return;

			if (clipped == 1)
				clipFlags &= ~(1 << i);
		}
	}

	// Recurse down the children
	if (node->contents == -1){
		R_RecursiveWorldNode(node->children[0], clipFlags);
		R_RecursiveWorldNode(node->children[1], clipFlags);
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

	// Add to world mins/maxs
	AddPointToBounds(leaf->mins, rg.viewParms.worldMins, rg.viewParms.worldMaxs);
	AddPointToBounds(leaf->maxs, rg.viewParms.worldMins, rg.viewParms.worldMaxs);

	rg.pc.leafs++;

	// Add all the surfaces
	for (i = 0, mark = leaf->firstMarkSurface; i < leaf->numMarkSurfaces; i++, mark++){
		surface = *mark;

		if (surface->worldCount == rg.frameCount)
			continue;	// Already added this surface from another leaf
		surface->worldCount = rg.frameCount;

		// Cull
		if (R_CullSurface(surface, rg.renderView.origin, clipFlags))
			continue;

		// Add the surface
		R_AddSurfaceToList(surface, rg.worldEntity);
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
	ClearBounds(rg.viewParms.worldMins, rg.viewParms.worldMaxs);

	// Mark leaves and lights
	R_MarkLeaves();
	R_MarkLights();

	// Recurse down the BSP tree
	if (r_skipCulling->integerValue)
		R_RecursiveWorldNode(rg.worldModel->nodes, 0);
	else
		R_RecursiveWorldNode(rg.worldModel->nodes, 15);
}