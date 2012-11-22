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
// r_light.c - Light management
//


#include "r_local.h"


/*
 ==============================================================================

 DYNAMIC LIGHTS

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void R_RecursiveLightNode (node_t *node, renderLight_t *renderLight, int bit){

}

/*
 ==================
 
 ==================
*/
void R_MarkLights (){

}


/*
 ==============================================================================

 AMBIENT AND DIFFUSE LIGHTING

 ==============================================================================
*/

static vec3_t				r_pointColor;
static vec3_t				r_lightColors[MAX_VERTICES];


/*
 ==================
 
 ==================
*/
static bool R_RecursiveLightPoint (node_t *node, const vec3_t start, const vec3_t end){

}

/*
 ==================
 
 ==================
*/
void R_LightForPoint (const vec3_t point, vec3_t ambientLight){

}

/*
 ==================
 
 ==================
*/
static void R_ReadLightGrid (const vec3_t origin, vec3_t lightDir){

}

/*
 ==================
 
 ==================
*/
void R_LightDirection (const vec3_t origin, vec3_t lightDir){

}

/*
 ==================
 
 ==================
*/
void R_LightingAmbient (){

}

/*
 ==================
 
 ==================
*/
void R_LightingDiffuse (){

}


/*
 ==============================================================================

 LIGHT SAMPLING

 ==============================================================================
*/

static vec3_t				r_blockLights[128*128];


/*
 ==================
 
 ==================
*/
static void R_SetCacheState (surface_t *surface){

}

/*
 ==================
 
 ==================
*/
static void R_AddDynamicLights (surface_t *surface){

}

/*
 ==================
 
 Combine and scale multiple lightmaps into the floating format in 
 r_blockLights
 ==================
*/
static void R_BuildLightmap (surface_t *surface, byte *dest, int stride){

}


/*
 ==============================================================================

 LIGHTMAP ALLOCATION

 ==============================================================================
*/

typedef struct {
	int						currentNum;
	int						allocated[LIGHTMAP_WIDTH];
	byte					buffer[LIGHTMAP_WIDTH*LIGHTMAP_HEIGHT*4];
} lmState_t;

static lmState_t			r_lmState;


/*
 ==================
 
 ==================
*/
static void R_UploadLightmap (){

}

/*
 ==================
 
 ==================
*/
static byte *R_AllocLightmapBlock (int width, int height, int *s, int *t){

}

/*
 ==================
 
 ==================
*/
void R_BeginBuildingLightmaps (){

}

/*
 ==================
 
 ==================
*/
void R_EndBuildingLightmaps (){

}

/*
 ==================
 
 ==================
*/
void R_BuildSurfaceLightmap (surface_t *surface){

}

/*
 ==================
 
 ==================
*/
void R_UpdateSurfaceLightmap (surface_t *surface){

}