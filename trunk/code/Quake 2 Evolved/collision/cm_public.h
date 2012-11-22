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

// Returns true if the given areas are connected
bool			CM_AreasAreConnected (int areaNum1, int areaNum2);

// Statistics for debugging and optimization
void			CM_PrintStats ();

// Initializes the collision subsystem
void			CM_Init ();

// Shuts down the collision subsystem
void			CM_Shutdown ();







int			CM_NumInlineModels (void);
clipInlineModel_t	*CM_InlineModel (const char *name);

char		*CM_EntityString (void);

int			CM_NumClusters (void);
int			CM_LeafContents (int leafNum);
int			CM_LeafCluster (int leafNum);
int			CM_LeafArea (int leafNum);

void		CM_InitBoxHull ();

// Creates a clipping hull for an arbitrary box
int			CM_HeadNodeForBox (const vec3_t mins, const vec3_t maxs);

// Call with topNode set to the headNode, returns with topNode set to
// the first node that splits the box
int			CM_BoxLeafNums (const vec3_t mins, const vec3_t maxs, int *list, int listSize, int *topNode);

int			CM_PointLeafNum (const vec3_t p);

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