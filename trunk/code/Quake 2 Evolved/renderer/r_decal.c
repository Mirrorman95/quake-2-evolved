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
// r_decal.c - Decal clipping and management
//


#include "r_local.h"


#define MAX_FRAGMENTS				128
#define MAX_FRAGMENT_VERTICES		384

typedef struct {
	surface_t *				parent;

	int						firstVertex;
	int						numVertices;
} fragment_t;

static fragment_t			r_fragments[MAX_FRAGMENTS];
static int					r_numFragments;

static vec3_t				r_fragmentVertices[MAX_FRAGMENT_VERTICES];
static int					r_numFragmentVertices;

static cplane_t				r_fragmentClipPlanes[6];

static decal_t				r_decalList[MAX_DECALS];
static decal_t				r_activeDecals;
static decal_t *			r_freeDecals;


/*
 ==============================================================================

 DECAL ALLOCATION AND FREEING

 ==============================================================================
*/


/*
 ==================
 R_FreeDecal
 ==================
*/
static void R_FreeDecal (decal_t *decal){

	if (!decal->prev)
		return;

	// Unlink
	decal->prev->next = decal->next;
	decal->next->prev = decal->prev;

	// Free the slot
	decal->next = r_freeDecals;
	r_freeDecals = decal;
}

/*
 ==================
 R_AllocDecal
 ==================
*/
static decal_t *R_AllocDecal (){

	decal_t	*decal;

	// Allocate a slot, freeing the oldest if necessary
	if (!r_freeDecals)
		R_FreeDecal(r_activeDecals.prev);

	decal = r_freeDecals;

	// Link
	r_freeDecals = r_freeDecals->next;

	Mem_Fill(decal, 0, sizeof(decal_t));

	decal->next = r_activeDecals.next;
	decal->prev = &r_activeDecals;

	r_activeDecals.next->prev = decal;
	r_activeDecals.next = decal;

	return decal;
}


/*
 ==============================================================================

 DECAL CLIPPING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void R_ClipFragment (int stage, int numVertices, vec3_t vertices, surface_t *surface, fragment_t *fragment){

}

/*
 ==================
 
 ==================
*/
static void R_ClipFragmentToSurface (surface_t *surface){

}

/*
 ==================
 
 ==================
*/
static void R_RecursiveFragmentNode (node_t *node, const vec3_t origin, const vec3_t normal, float radius){

}

/*
 ==================
 
 ==================
*/
static bool R_DecalFragments (const vec3_t origin, const vec3_t axis[3], float radius){

}


/*
 ==============================================================================

 DECAL PROJECTION

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
void R_ProjectDecalOntoWorld (const vec3_t origin, const vec3_t direction, float rotation, float radius, int startTime, material_t *material){

	if (!rg.worldModel)
		Com_Error(ERR_DROP, "R_ProjectDecalOntoWorld: NULL world");

	if (material->decalInfo.fadeInTime + material->decalInfo.stayTime + material->decalInfo.fadeOutTime <= 0.0f)
		return;		// Don't bother clipping

	// Compute orientation
}


// ============================================================================


/*
 ==================
 R_ClearDecals
 ==================
*/
void R_ClearDecals (){

	int		i;

	// Free the list
	Mem_Fill(r_decalList, 0, sizeof(r_decalList));

	// Link
	r_activeDecals.next = &r_activeDecals;
	r_activeDecals.prev = &r_activeDecals;

	// Get the free list
	r_freeDecals = r_decalList;

	for (i = 0; i < MAX_DECALS - 1; i++)
		r_decalList[i].next = &r_decalList[i+1];
}

/*
 ==================
 
 ==================
*/
void R_AddDecals (){

	decal_t	*decal, *next;

	if (r_skipDecals->integerValue)
		return;

	if (!rg.viewParms.primaryView)
		return;

	for (decal = r_activeDecals.next; decal != &r_activeDecals; decal = next){
		// Grab next now, so if the decal is freed we still have it
		next = decal->next;

		// Check if completely faded out

		// Check parent surface visibility
		if (decal->parentSurface->viewCount != rg.viewCount)
			continue;

		rg.pc.decals++;

		// Add it
	}
}