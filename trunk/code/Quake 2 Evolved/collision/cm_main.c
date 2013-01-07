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
// cm_main.c - Primary collision file
//


#include "cm_local.h"


clipMap_t					cm;

clipStats_t					cm_stats;

static bool					cm_initialized = false;

cvar_t *					cm_showStats;
cvar_t *					cm_skipAreas;


/*
 ==================
 
 ==================
*/
void CM_PrintStats (){

	if (!cm.loaded)
		return;

	switch (cm_showStats->integerValue){
	case 1:

		break;
	case 2:

		break;
	case 3:

		break;
	case 4:

		break;
	case 5:
		Com_Printf("leafs: %i (points: %i, bounds: %i)\n", cm_stats.leafPoints + cm_stats.leafBounds, cm_stats.leafPoints, cm_stats.leafBounds);
		break;
	case 6:

		break;
	case 7:

		break;
	}

	// Clear for next frame
	Mem_Fill(&cm_stats, 0, sizeof(clipStats_t));
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 CM_Init
 ==================
*/
void CM_Init (){

	// Register variables
	cm_showStats = CVar_Register("cm_showStats", "0", CVAR_INTEGER, CVAR_CHEAT, "Show collision statistics (TODO)", 0, 0);
	cm_skipAreas = CVar_Register("cm_skipAreas", "0", CVAR_BOOL, CVAR_CHEAT, "Skip area portal connections", 0, 0);

	// Initialize all the collision system modules
	CM_InitModels();

	cm_initialized = true;
}

/*
 ==================
 CM_Shutdown
 ==================
*/
void CM_Shutdown (){

	if (!cm_initialized)
		return;

	// Shutdown all the collision system modules
	CM_ShutdownModels();

	// Free all collision system allocations
	Mem_FreeAll(TAG_COLLISION, false);

	Mem_Fill(&cm, 0, sizeof(clipMap_t));

	Mem_Fill(&cm_stats, 0, sizeof(clipStats_t));
}