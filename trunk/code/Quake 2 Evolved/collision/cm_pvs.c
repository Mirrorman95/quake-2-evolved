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
// cm_pvs.c - PVS related code
//


#include "cm_local.h"


static byte					cm_pvsRow[MAX_MAP_LEAFS/8];
static byte					cm_phsRow[MAX_MAP_LEAFS/8];


/*
 ==================
 CM_NumClusters
 ==================
*/
int	CM_NumClusters (){

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_NumClusters: map not loaded");

	return cm.numClusters;
}

/*
 ==================
 CM_DecompressVis
 ==================
*/
static void CM_DecompressVis (const byte *in, byte *out){

	byte	*vis;
	int		c, row;

	row = (cm.numClusters + 7) >> 3;
	vis = out;

	// No PVS info, so make all visible
	if (!in){
		while (row){
			*vis++ = 0xFF;
			row--;
		}

		return;		
	}

	// Decompress the PVS
	do {
		if (*in){
			*vis++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((vis - out) + c > row){
			c = row - (vis - out);
			Com_DPrintf(S_COLOR_YELLOW "CM_DecompressVis: vis decompression overrun\n");
		}

		while (c){
			*vis++ = 0;
			c--;
		}
	} while (vis - out < row);
}

/*
 ==================
 
 ==================
*/
byte *CM_ClusterPVS (int cluster){

	if (!cm.loaded || !cm.numClusters){
		Mem_Fill(cm_pvsRow, 0xFF, 1);
		return cm_pvsRow;
	}

	if (cluster == -1 || cm.numVisibility == 0)
		Mem_Fill(cm_pvsRow, 0xFF, (cm.numClusters + 7) >> 3);
	else
		CM_DecompressVis((byte *)cm.visibility + cm.visibility->bitOfs[cluster][VIS_PVS], cm_pvsRow);

	return cm_pvsRow;
}

/*
 ==================
 
 ==================
*/
byte *CM_ClusterPHS (int cluster){

	if (cluster == -1 || cm.numVisibility == 0)
		Mem_Fill(cm_phsRow, 0, (cm.numClusters + 7) >> 3);
	else
		CM_DecompressVis((byte *)cm.visibility + cm.visibility->bitOfs[cluster][VIS_PHS], cm_phsRow);

	return cm_phsRow;
}