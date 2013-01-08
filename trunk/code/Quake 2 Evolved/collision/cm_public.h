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
// cm_public.h - Public header to all other systems
//


#ifndef __CM_PUBLIC_H__
#define __CM_PUBLIC_H__


typedef struct cmodel_s {
	vec3_t	mins;
	vec3_t	maxs;
	vec3_t	origin;		// For sounds or lights
	int		headNode;
} clipInlineModel_t;

// Loads and prepares the given map for collision detection
clipInlineModel_t *CM_LoadMap (const char *name, bool clientLoad, uint *checkCount);

// Frees the current map and all the models
void			CM_FreeMap ();

// Returns the number of inline models in the current map
int				CM_NumInlineModels ();

// Loads the given inline model
clipInlineModel_t *CM_LoadInlineModel (const char *name);

// Returns the entity string
char *			CM_GetEntityString ();

// Sets up a box model for collision detection
int				CM_SetupBoxModel (const vec3_t mins, const vec3_t maxs);

//
int				CM_PointInLeaf (const vec3_t point, int nodeNum);

// Fills in a list of all the leafs contacted by the given bounds
int				CM_BoundsInLeaves (const vec3_t mins, const vec3_t maxs, int *leafs, int maxLeafs, int headNode, int *topNode);

// Returns the number of clusters in the current map
int				CM_NumClusters ();

// Returns the number of areas in the current map
int				CM_NumAreas ();

// Returns the area contacted by the given point, or -1 if no area was
// contacted
int				CM_PointInArea (const vec3_t point, int nodeNum);

// Returns true if the given areas are connected
bool			CM_AreasAreConnected (int areaNum1, int areaNum2);

// Statistics for debugging and optimization
void			CM_PrintStats ();

// Initializes the collision subsystem
void			CM_Init ();

// Shuts down the collision subsystem
void			CM_Shutdown ();










int			CM_LeafContents (int leafNum);
int			CM_LeafCluster (int leafNum);
int			CM_LeafArea (int leafNum);

// Call with topNode set to the headNode, returns with topNode set to
// the first node that splits the box
int			CM_BoxLeafNums (const vec3_t mins, const vec3_t maxs, int *list, int listSize, int *topNode);

// Returns an ORed contents mask
int			CM_PointContents (const vec3_t p, int headNode);
int			CM_TransformedPointContents (const vec3_t p, int headNode, const vec3_t origin, const vec3_t angles);

trace_t		CM_BoxTrace (const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, int headNode, int brushMask);
trace_t		CM_TransformedBoxTrace (const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, int headNode, int brushMask, const vec3_t origin, const vec3_t angles);

byte		*CM_ClusterPVS (int cluster);
byte		*CM_ClusterPHS (int cluster);

void		CM_FloodAreaConnections (bool clear);

void		CM_SetAreaPortalState (int portalNum, bool open);

int			CM_WriteAreaBits (byte *buffer, int area);
void		CM_WritePortalState (fileHandle_t f);
void		CM_ReadPortalState (fileHandle_t f);
bool		CM_HeadNodeVisible (int headNode, const byte *visBits);


#endif	// __CM_PUBLIC_H__