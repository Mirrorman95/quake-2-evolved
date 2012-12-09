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
// r_mesh.c - Mesh buffer
//


#include "r_local.h"


/*
 ==================
 R_AddMeshToList
 ==================
*/
void R_AddMeshToList (meshType_t type, meshData_t *data, renderEntity_t *entity, material_t *material){

	mesh_t	*mesh;
	int		index;

	if (material->coverage != MC_TRANSLUCENT)
		index = 0;
	else {
		if (material->sort < SORT_REFRACTIVE)
			index = 1;
		else if (material->sort < SORT_POST_PROCESS)
			index = 2;
		else
			index = 3;
	}

	// Add a new mesh
	if (rg.numMeshes[index] == rg.maxMeshes[index]){
		Com_DPrintf(S_COLOR_YELLOW "R_AddDrawSurface: overflow\n");
		return;
	}

	mesh = &rg.meshes[index][rg.numMeshes[index]++];

	// Fill it in
	mesh->type = type;
	mesh->data = data;

	mesh->entity = entity;
	mesh->material = material;

	mesh->sort = (material->sort << MESH_SHIFT_SORT) | (entity->index << MESH_SHIFT_ENTITY) | (material->index << MESH_SHIFT_MATERIAL) | type;
}


/*
 ==============================================================================

 SPRITE LIST

 ==============================================================================
*/


/*
 ==================
 R_AddSprite
 ==================
*/
static void R_AddSprite (renderEntity_t *entity){

	material_t	*material;
	vec3_t		vec;

	// Cull
	if (!r_skipEntityCulling->integerValue && !entity->depthHack){
		VectorSubtract(entity->origin, rg.renderView.origin, vec);
		if (DotProduct(vec, rg.renderView.axis[0]) < 0.0f)
			return;
	}

	// Mark as visible for this view
	entity->viewCount = rg.viewCount;

	rg.pc.entities++;

	// Get the material
	if (entity->material)
		material = entity->material;
	else
		material = rg.defaultMaterial;

	// Add it
	R_AddMeshToList(MESH_SPRITE, NULL, entity, material);
}


/*
 ==============================================================================

 BEAM LIST

 ==============================================================================
*/


/*
 ==================
 R_AddBeam
 ==================
*/
static void R_AddBeam (renderEntity_t *entity){

	material_t	*material;
	vec3_t		vec;

	// Cull
	if (!r_skipEntityCulling->integerValue && !entity->depthHack){
		VectorSubtract(entity->origin, rg.renderView.origin, vec);
		if (DotProduct(vec, rg.renderView.axis[0]) < 0.0f)
			return;
	}

	// Mark as visible for this view
	entity->viewCount = rg.viewCount;

	rg.pc.entities++;

	// Get the material
	if (entity->material)
		material = entity->material;
	else
		material = rg.defaultMaterial;

	// Add it
	R_AddMeshToList(MESH_BEAM, NULL, entity, material);
}


/*
 ==============================================================================

 ENTITY LIST

 ==============================================================================
*/


/*
 ==================
 R_AddEntities
 ==================
*/
static void R_AddEntities (){

	renderEntity_t	*entity;
	int				i;

	if (r_skipEntities->integerValue)
		return;

	for (i = 0, entity = rg.viewParms.renderEntities; i < rg.viewParms.numRenderEntities; i++, entity++){
		// Development tool
		if (r_singleEntity->integerValue != -1){
			if (r_singleEntity->integerValue != entity->index)
				continue;
		}

		// Add the entity
		switch (entity->type){
		case RE_MODEL:
			if (!entity->model){
				Com_DPrintf(S_COLOR_YELLOW "R_AddEntities: entity with no model at %i, %i, %i\n", (int)entity->origin[0], (int)entity->origin[1], (int)entity->origin[2]);
				break;
			}

			switch (entity->model->type){
			case MODEL_INLINE:
				R_AddInlineModel(entity);
				break;
			case MODEL_MD3:
			case MODEL_MD2:
				R_AddAliasModel(entity);
				break;
			default:
				Com_Error(ERR_DROP, "R_AddEntities: bad model type (%i)", entity->model->type);
			}

			break;
		case RE_SPRITE:
			R_AddSprite(entity);
			break;
		case RE_BEAM:
			R_AddBeam(entity);
			break;
		default:
			Com_Error(ERR_DROP, "R_AddEntities: bad entity type (%i)", entity->type);
		}
	}
}


/*
 ==============================================================================

 PARTICLE LIST

 ==============================================================================
*/


/*
 ==================
 R_AddParticles
 ==================
*/
static void R_AddParticles (){

	renderParticle_t	*particle;
	int					i;

	if (r_skipParticles->integerValue)
		return;

	for (i = 0, particle = rg.viewParms.renderParticles; i < rg.viewParms.numRenderParticles; i++, particle++){
		if (!particle->material->numStages)
			continue;

		rg.pc.particles++;

		// Add it
		R_AddMeshToList(MESH_PARTICLE, particle, rg.worldEntity, particle->material);
	}
}


// ============================================================================


/*
 ==================
 R_SortMeshes
 ==================
*/
void R_SortMeshes (int numMeshes, mesh_t *meshes){

	mesh_t	tmp;
	int		lo, loGuy, loStack[32];
	int		hi, hiGuy, hiStack[32];
	int		mid, count, stackDepth;
	int		i, max;

	if (numMeshes < 2)
		return;

	// Initialize limits
	lo = 0;
	hi = numMeshes - 1;

	// Initialize stack
	stackDepth = 0;

	// Entry point for recursion
recurse:

	// Number of elements to sort
	count = (hi - lo) + 1;

	// Below a certain count, it is faster to use insertion sort
	if (count <= 8){
		while (hi > lo){
			max = lo;

			for (i = lo + 1; i <= hi; i++){
				if (meshes[i].sort > meshes[max].sort)
					max = i;
			}

			// Swap elements
			tmp = meshes[max];
			meshes[max] = meshes[hi];
			meshes[hi] = tmp;

			hi--;
		}
	}
	else {
		// Pick a partitioning element
		mid = lo + (count >> 1);

		// Sort the first, middle, and last elements into order
		if (meshes[lo].sort > meshes[mid].sort){
			tmp = meshes[lo];
			meshes[lo] = meshes[mid];
			meshes[mid] = tmp;
		}
		if (meshes[lo].sort > meshes[hi].sort){
			tmp = meshes[lo];
			meshes[lo] = meshes[hi];
			meshes[hi] = tmp;
		}
		if (meshes[mid].sort > meshes[hi].sort){
			tmp = meshes[mid];
			meshes[mid] = meshes[hi];
			meshes[hi] = tmp;
		}

		// Now partition the array into three pieces
		loGuy = lo;
		hiGuy = hi;

		while (1){
			if (mid > loGuy){
				do {
					loGuy++;
				} while (loGuy < mid && meshes[loGuy].sort <= meshes[mid].sort);
			}
			if (mid <= loGuy){
				do {
					loGuy++;
				} while (loGuy <= hi && meshes[loGuy].sort <= meshes[mid].sort);
			}

			do {
				hiGuy--;
			} while (hiGuy > mid && meshes[hiGuy].sort > meshes[mid].sort);

			if (hiGuy < loGuy)
				break;

			// Swap elements
			tmp = meshes[loGuy];
			meshes[loGuy] = meshes[hiGuy];
			meshes[hiGuy] = tmp;

			if (mid == hiGuy)
				mid = loGuy;
		}

		// Find adjacent elements equal to the partition element
		hiGuy++;

		if (mid < hiGuy){
			do {
				hiGuy--;
			} while (hiGuy > mid && meshes[hiGuy].sort == meshes[mid].sort);
		}
		if (mid >= hiGuy){
			do {
				hiGuy--;
			} while (hiGuy > lo && meshes[hiGuy].sort == meshes[mid].sort);
		}

		// We've finished the partition, now we want to sort the subarrays
		if (hiGuy - lo >= hi - loGuy){
			if (lo < hiGuy){
				loStack[stackDepth] = lo;
				hiStack[stackDepth] = hiGuy;

				stackDepth++;
			}

			if (loGuy < hi){
				lo = loGuy;
				goto recurse;
			}
		}
		else {
			if (loGuy < hi){
				loStack[stackDepth] = loGuy;
				hiStack[stackDepth] = hi;

				stackDepth++;
			}

			if (lo < hiGuy){
				hi = hiGuy;
				goto recurse;
			}
		}
	}

	// We have sorted the array, except for any pending sorts on the stack.
	// Check if there are any, and do them.
	if (--stackDepth >= 0){
		lo = loStack[stackDepth];
		hi = hiStack[stackDepth];

		goto recurse;
	}
}

/*
 ==================
 R_AllocMeshes
 ==================
*/
void R_AllocMeshes (){

	rg.maxMeshes[0] = MAX_MESHES;
	rg.maxMeshes[1] = MAX_MESHES >> 1;
	rg.maxMeshes[2] = MAX_MESHES >> 1;
	rg.maxMeshes[3] = MAX_MESHES >> 6;

	rg.meshes[0] = (mesh_t *)Mem_Alloc(rg.maxMeshes[0] * sizeof(mesh_t), TAG_RENDERER);
	rg.meshes[1] = (mesh_t *)Mem_Alloc(rg.maxMeshes[1] * sizeof(mesh_t), TAG_RENDERER);
	rg.meshes[2] = (mesh_t *)Mem_Alloc(rg.maxMeshes[2] * sizeof(mesh_t), TAG_RENDERER);
	rg.meshes[3] = (mesh_t *)Mem_Alloc(rg.maxMeshes[3] * sizeof(mesh_t), TAG_RENDERER);
}

/*
 ==================
 R_GenerateMeshes
 ==================
*/
void R_GenerateMeshes (){

	// Add the meshes
	R_AddWorldSurfaces();
	R_AddEntities();
	R_AddDecals();
	R_AddParticles();

	// Set up the meshes
	rg.viewParms.numMeshes[0] = rg.numMeshes[0] - rg.firstMesh[0];
	rg.viewParms.numMeshes[1] = rg.numMeshes[1] - rg.firstMesh[1];
	rg.viewParms.numMeshes[2] = rg.numMeshes[2] - rg.firstMesh[2];
	rg.viewParms.numMeshes[3] = rg.numMeshes[3] - rg.firstMesh[3];

	rg.viewParms.meshes[0] = &rg.meshes[0][rg.firstMesh[0]];
	rg.viewParms.meshes[1] = &rg.meshes[1][rg.firstMesh[1]];
	rg.viewParms.meshes[2] = &rg.meshes[2][rg.firstMesh[2]];
	rg.viewParms.meshes[3] = &rg.meshes[3][rg.firstMesh[3]];

	rg.pc.meshes += rg.viewParms.numMeshes[0] + rg.viewParms.numMeshes[1] + rg.viewParms.numMeshes[2] + rg.viewParms.numMeshes[3];

	// The next view rendered in this frame will tack on after this one
	rg.firstMesh[0] = rg.numMeshes[0];
	rg.firstMesh[1] = rg.numMeshes[1];
	rg.firstMesh[2] = rg.numMeshes[2];
	rg.firstMesh[3] = rg.numMeshes[3];

	// Sort the meshes
	if (r_skipSorting->integerValue)
		return;

	R_SortMeshes(rg.viewParms.numMeshes[0], rg.viewParms.meshes[0]);
	R_SortMeshes(rg.viewParms.numMeshes[1], rg.viewParms.meshes[1]);
	R_SortMeshes(rg.viewParms.numMeshes[2], rg.viewParms.meshes[2]);
	R_SortMeshes(rg.viewParms.numMeshes[3], rg.viewParms.meshes[3]);
}

/*
 ==================
 R_ClearMeshes
 ==================
*/
void R_ClearMeshes (){

	rg.numMeshes[0] = rg.firstMesh[0] = 0;
	rg.numMeshes[1] = rg.firstMesh[1] = 0;
	rg.numMeshes[2] = rg.firstMesh[2] = 0;
	rg.numMeshes[3] = rg.firstMesh[3] = 0;
}