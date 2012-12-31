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
// cm_local.h - Local header file to all collision files
//


#ifndef __CM_LOCAL_H__
#define __CM_LOCAL_H__


#include "../common/common.h"


/*
 ==============================================================================

 MODEL MANAGER

 ==============================================================================
*/

typedef struct {					// Used internally due to name length problems
	char					name[32];
	csurface_t				c;
} clipSurface_t;

typedef struct {
	int						numClusters;
	int						bitOfs[8][2];
} clipVis_t;

typedef struct {
	int						contents;
	int						cluster;
	int						area;
	ushort					firstLeafBrush;
	ushort					numLeafBrushes;
} clipLeaf_t;

typedef struct {
	int						contents;
	int						numSides;
	int						firstBrushSide;
	int						checkCount;		// To avoid repeated testings
} clipBrush_t;

typedef struct {
	cplane_t *				plane;
	clipSurface_t *			surface;
} clipBrushSide_t;

typedef struct {
	cplane_t *				plane;
	int						children[2];	// Negative numbers are leafs
} clipNode_t;

typedef struct {
	int						numAreaPortals;
	int						firstAreaPortal;
	int						floodNum;		// If two areas have equal floodNums, they are connected
	int						floodValid;
} clipArea_t;

typedef struct {
	int						portalNum;
	int						otherArea;
} clipAreaPortal_t;

typedef struct {
	bool					loaded;

	char					name[MAX_PATH_LENGTH];
	int						size;

	uint					checkCount;		// For multi-check avoidance

	// BSP data structures
	int						numPlanes;
	cplane_t *				planes;

	int						numSurfaces;
	clipSurface_t *			surfaces;

	int						numVisibility;
	clipVis_t *				visibility;

	int						numLeafs;
	int						numClusters;
	clipLeaf_t *			leafs;

	int						numLeafBrushes;
	ushort *				leafBrushes;

	int						numBrushes;
	clipBrush_t *			brushes;

	int						numBrushSides;
	clipBrushSide_t *		brushSides;

	int						numNodes;
	clipNode_t *			nodes;

	int						numModels;
	clipInlineModel_t *		models;

	int						numAreas;
	clipArea_t *			areas;

	int						numAreaPortals;
	clipAreaPortal_t *		areaPortals;

	int						numEntityChars;
	char *					entityString;

	clipSurface_t			nullSurface;
	clipInlineModel_t		nullModel;
} clipMap_t;


/*
 ==============================================================================

 GLOBALS

 ==============================================================================
*/

typedef struct {
	int todo;
} clipStats_t;

extern clipMap_t			cm;

extern clipStats_t			cm_stats;

extern cvar_t *				cm_showStats;
extern cvar_t *				cm_skipAreas;


#endif	// __CM_LOCAL_H__