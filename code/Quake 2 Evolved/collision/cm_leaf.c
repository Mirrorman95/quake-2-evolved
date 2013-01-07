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
// cm_main.c - BSP leaf related code
//


#include "cm_local.h"


typedef struct {
	int *					leafs;
	int						numLeafs;
	int						maxLeafs;

	vec3_t					mins;
	vec3_t					maxs;

	int						topNode;
} leafList_t;


/*
 ==================
 
 ==================
*/
int	CM_LeafCluster (int leafNum){

	if (leafNum < 0 || leafNum >= cm.numLeafs)
		Com_Error(ERR_DROP, "CM_LeafCluster: bad number");

	return cm.leafs[leafNum].cluster;
}

/*
 ==================
 
 ==================
*/
int	CM_LeafArea (int leafNum){

	if (leafNum < 0 || leafNum >= cm.numLeafs)
		Com_Error(ERR_DROP, "CM_LeafArea: bad number");
	
	return cm.leafs[leafNum].area;
}

/*
 ==================
 CM_PointInLeaf

 TODO: make sure this is valid
 ==================
*/
int CM_PointInLeaf (const vec3_t point, int nodeNum){

	clipNode_t	*node;
	int			side;

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_PointInLeaf: map not loaded");

	cm_stats.leafPoints++;

	while (nodeNum >= 0){
		// Find which side of the node we are on
		node = &cm.nodes[nodeNum];

		side = PointOnPlaneSide(point, 0.0f, node->plane);

		// Go down the appropriate side
		if (side == PLANESIDE_BACK)
			nodeNum = node->children[1];
		else
			nodeNum = node->children[0];
	}

	return -1 - nodeNum;
}

/*
 ==================
 CM_RecursiveBoundsInLeaves

 TODO: make sure this is valid
 ==================
*/
static void CM_RecursiveBoundsInLeaves (int nodeNum, leafList_t *leafList){

	clipNode_t	*node;
	cplane_t	*plane;
	int			side;

	while (1){
		// If < 0, we are in a leaf node
		if (nodeNum < 0){
			if (leafList->numLeafs >= leafList->maxLeafs)
				return;

			leafList->leafs[leafList->numLeafs++] = -1 - nodeNum;

			return;
		}

		// Find which side of the node we are on
		node = &cm.nodes[nodeNum];
		plane = node->plane;

		side = BoxOnPlaneSide(leafList->mins, leafList->maxs, plane);

		// Go down the appropriate sides
		if (side == PLANESIDE_FRONT){
			nodeNum = node->children[0];
			continue;
		}
		if (side == PLANESIDE_BACK){
			nodeNum = node->children[1];
			continue;
		}

		// Go down both sides
		if (leafList->topNode == -1)
			leafList->topNode = nodeNum;

		// Recurse down the children, front side first
		CM_RecursiveBoundsInLeaves(node->children[0], leafList);

		// Tail recurse
		nodeNum = node->children[1];
	}
}

/*
 ==================
 CM_BoundsInLeaves
 ==================
*/
int CM_BoundsInLeaves (const vec3_t mins, const vec3_t maxs, int *leafs, int maxLeafs, int headNode, int *topNode){

	leafList_t	leafList;

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_BoundsInLeaves: map not loaded");

	cm_stats.leafBounds++;

	// Fill it in
	leafList.leafs = leafs;
	leafList.numLeafs = 0;
	leafList.maxLeafs = maxLeafs;

	VectorCopy(mins, leafList.mins);
	VectorCopy(maxs, leafList.maxs);

	leafList.topNode = -1;

	// Recurse down the BSP tree
	CM_RecursiveBoundsInLeaves(headNode, &leafList);

	if (topNode)
		*topNode = leafList.topNode;

	return leafList.numLeafs;
}

/*
 ==================
 
 TODO: replace this
 ==================
*/
int	CM_BoxLeafNums (const vec3_t mins, const vec3_t maxs, int *list, int listSize, int *topNode){

	return CM_BoundsInLeaves(mins, maxs, list, listSize, cm.models[0].headNode, topNode);
}