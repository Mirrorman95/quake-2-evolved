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
// cm_area.c - Area portals
//


#include "cm_local.h"


static bool					cm_areaPortalOpen[MAX_MAP_AREAPORTALS];
static int					cm_floodValid;


/*
 ==================
 CM_NumAreas
 ==================
*/
int CM_NumAreas (){

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_NumAreas: map not loaded");

	return cm.numAreas;
}

/*
 ==================
 CM_PointInArea
 ==================
*/
int CM_PointInArea (const vec3_t point, int nodeNum){

	clipNode_t	*node;
	clipLeaf_t	*leaf;
	int			side;

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_PointInArea: map not loaded");

	cm_stats.areaPoints++;

	while (1){
		// If < 0, we are in a leaf node
		if (nodeNum < 0){
			leaf = &cm.leafs[-1 - nodeNum];
			break;
		}

		// Find which side of the node we are on
		node = &cm.nodes[nodeNum];

		side = PointOnPlaneSide(point, 0.0f, node->plane);

		// Go down the appropriate side
		if (side == PLANESIDE_FRONT)
			nodeNum = node->children[0];
		else
			nodeNum = node->children[1];
	}

	return leaf->area;
}

/*
 ==================
 
 ==================
*/
static void CM_RecursiveFloodArea (clipArea_t *area, int floodNum){

	clipAreaPortal_t	*p;
	int					i;

	if (area->floodValid == cm_floodValid){
		if (area->floodNum == floodNum)
			return;

		Com_Error(ERR_DROP, "CM_RecursiveFloodArea: reflooded");
	}

	area->floodNum = floodNum;
	area->floodValid = cm_floodValid;

	p = &cm.areaPortals[area->firstAreaPortal];
	for (i = 0; i < area->numAreaPortals; i++, p++){
		if (!cm_areaPortalOpen[p->portalNum])
			continue;

		// Recurse down the next area
		CM_RecursiveFloodArea(&cm.areas[p->otherArea], floodNum);
	}
}

/*
 ==================
 
 ==================
*/
void CM_FloodAreaConnections (bool clear){

	clipArea_t	*area;
	int			floodNum = 0;
	int			i;

	if (clear)
		Mem_Fill(cm_areaPortalOpen, 0, sizeof(cm_areaPortalOpen));

	// All current floods are now invalid
	cm_floodValid++;

	// Area 0 is not used
	for (i = 1; i < cm.numAreas; i++){
		area = &cm.areas[i];

		if (area->floodValid == cm_floodValid)
			continue;		// Already flooded into

		floodNum++;
		CM_RecursiveFloodArea(area, floodNum);
	}
}

/*
 ==================
 
 ==================
*/
void CM_SetAreaPortalState (int portalNum, bool open){

	if (portalNum < 0 || portalNum >= cm.numAreaPortals)
		Com_Error(ERR_DROP, "CM_SetAreaPortalState: bad area portal");

	cm_areaPortalOpen[portalNum] = open;

	CM_FloodAreaConnections(false);
}

/*
 ==================
 CM_AreasAreConnected
 ==================
*/
bool CM_AreasAreConnected (int areaNum1, int areaNum2){

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_AreasAreConnected: map not loaded");

	if (cm_skipAreas->integerValue)
		return true;

	if (!cm.numAreas)
		return true;

	if ((areaNum1 < 0 || areaNum1 >= cm.numAreas) || (areaNum2 < 0 || areaNum2 >= cm.numAreas))
		Com_Error(ERR_DROP, "CM_AreasAreConnected: areaNum out of range");

	if (cm.areas[areaNum1].floodNum == cm.areas[areaNum2].floodNum)
		return true;

	return false;
}

/*
 ==================
 
 Writes a length byte followed by a bit vector of all the areas that 
 are in the same flood as the area parameter.

 This is used by the client refresh to cull visibility.
 ==================
*/
int CM_WriteAreaBits (byte *buffer, int area){

	int		floodNum;
	int		bytes;
	int		i;

	if (!cm.loaded || !cm.numAreas){
		Mem_Fill(buffer, 0xFF, 1);
		return 1;
	}

	bytes = (cm.numAreas + 7) >> 3;

	if (cm_skipAreas->integerValue){
		// For debugging, send everything
		Mem_Fill(buffer, 0xFF, bytes);
		return bytes;
	}

	Mem_Fill(buffer, 0, bytes);

	// If the current area bits are invalid, rebuild them
	floodNum = cm.areas[area].floodNum;

	for (i = 0; i < cm.numAreas; i++){
		if (cm.areas[i].floodNum == floodNum || !area)
			buffer[i >> 3] |= 1 << (i & 7);
	}
	
	return bytes;
}

/*
 ==================
 
 Writes the portal state to a savegame file
 ==================
*/
void CM_WritePortalState (fileHandle_t f){

	FS_Write(f, cm_areaPortalOpen, sizeof(cm_areaPortalOpen));
}

/*
 ==================
 
 Reads the portal state from a savegame file and recalculates the area 
 connections
 ==================
*/
void CM_ReadPortalState (fileHandle_t f){

	FS_Read(f, cm_areaPortalOpen, sizeof(cm_areaPortalOpen));

	CM_FloodAreaConnections(false);
}

/*
 ==================
 
 ==================
*/
bool CM_HeadNodeVisible (int headNode, const byte *visBits){

	clipNode_t	*node;
	int			leafNum;
	int			cluster;

	if (!cm.loaded)
		return false;		// Map not loaded

	if (headNode < 0){
		leafNum = -1 - headNode;

		cluster = cm.leafs[leafNum].cluster;
		if (cluster == -1)
			return false;

		if (visBits[cluster >> 3] & (1 << (cluster & 7)))
			return true;

		return false;
	}

	node = &cm.nodes[headNode];

	if (CM_HeadNodeVisible(node->children[0], visBits))
		return true;

	return CM_HeadNodeVisible(node->children[1], visBits);
}