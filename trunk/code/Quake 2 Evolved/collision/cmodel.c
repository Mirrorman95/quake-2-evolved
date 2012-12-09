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
// cm_model.c - BSP model collision
//

// TODO!!!


#include "cm_local.h"



/*
 ==================
 
 ==================
*/
int	CM_NumInlineModels (void){

	return cm.numModels;
}

/*
 ==================
 
 ==================
*/
clipInlineModel_t *CM_InlineModel (const char *name){

	int		num;

	if (!name || name[0] != '*')
		Com_Error(ERR_DROP, "CM_InlineModel: bad name");

	num = atoi(name+1);
	if (num < 1 || num >= cm.numModels)
		Com_Error(ERR_DROP, "CM_InlineModel: bad number");

	return &cm.models[num];
}

/*
 ==================
 
 ==================
*/
char *CM_EntityString (void){

	if (!cm.numEntityChars)
		return "";

	return cm.entityString;
}

/*
 ==================
 
 ==================
*/
int	CM_NumClusters (void){

	return cm.numClusters;
}

/*
 ==================
 
 ==================
*/
int	CM_LeafContents (int leafNum){

	if (leafNum < 0 || leafNum >= cm.numLeafs)
		Com_Error(ERR_DROP, "CM_LeafContents: bad number");

	return cm.leafs[leafNum].contents;
}

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


// =====================================================================

static cplane_t	*cm_boxPlanes;
int		cm_boxHeadNode;
static clipBrush_t	*cm_boxBrush;
static clipLeaf_t	*cm_boxLeaf;


/*
 =================
 CM_InitBoxHull

 Set up the planes and nodes so that the six floats of a bounding box
 can just be stored out and get a proper clipping hull structure.
 =================
*/
void CM_InitBoxHull (){

	int				i;
	int				side;
	clipNode_t			*n;
	cplane_t		*p;
	clipBrushSide_t	*s;

	cm_boxPlanes = &cm.planes[cm.numPlanes];
	cm_boxHeadNode = cm.numNodes;

	cm_boxBrush = &cm.brushes[cm.numBrushes];
	cm_boxBrush->numSides = 6;
	cm_boxBrush->firstBrushSide = cm.numBrushSides;
	cm_boxBrush->contents = CONTENTS_MONSTER;

	cm_boxLeaf = &cm.leafs[cm.numLeafs];
	cm_boxLeaf->numLeafBrushes = 1;
	cm_boxLeaf->firstLeafBrush = cm.numLeafBrushes;
	cm_boxLeaf->contents = CONTENTS_MONSTER;

	cm.leafBrushes[cm.numLeafBrushes] = cm.numBrushes;

	for (i = 0; i < 6; i++){
		side = i & 1;

		// Brush sides
		s = &cm.brushSides[cm.numBrushSides+i];
		s->plane = &cm.planes[cm.numPlanes+i*2+side];
		s->surface = &cm.nullSurface;

		// Nodes
		n = &cm.nodes[cm.numNodes+i];
		n->plane = &cm.planes[cm.numPlanes+i*2];
		n->children[side] = -1 - cm.numLeafs;
		if (i != 5)
			n->children[side^1] = cm_boxHeadNode+i + 1;
		else
			n->children[side^1] = -1 - cm.numLeafs;

		// Planes
		p = &cm_boxPlanes[i*2+0];
		VectorClear(p->normal);
		p->normal[i>>1] = 1;
		p->type = i>>1;
		p->signbits = 0;

		p = &cm_boxPlanes[i*2+1];
		VectorClear(p->normal);
		p->normal[i>>1] = -1;
		p->type = 3;
		p->signbits = 0;
	}	
}

/*
 =================
 CM_HeadNodeForBox
 
 To keep everything totally uniform, bounding boxes are turned into 
 small BSP trees instead of being compared directly
 =================
*/
int	CM_HeadNodeForBox (const vec3_t mins, const vec3_t maxs){

	cm_boxPlanes[0].dist = maxs[0];
	cm_boxPlanes[1].dist = -maxs[0];
	cm_boxPlanes[2].dist = mins[0];
	cm_boxPlanes[3].dist = -mins[0];
	cm_boxPlanes[4].dist = maxs[1];
	cm_boxPlanes[5].dist = -maxs[1];
	cm_boxPlanes[6].dist = mins[1];
	cm_boxPlanes[7].dist = -mins[1];
	cm_boxPlanes[8].dist = maxs[2];
	cm_boxPlanes[9].dist = -maxs[2];
	cm_boxPlanes[10].dist = mins[2];
	cm_boxPlanes[11].dist = -mins[2];

	return cm_boxHeadNode;
}


// =====================================================================

static int		cm_leafCount, cm_leafMaxCount;
static int		*cm_leafList;
static vec3_t	cm_leafMins, cm_leafMaxs;
static int		cm_leafTopNode;


/*
 =================
 CM_RecursiveBoxLeafNums

 Fills in a list of all the leafs touched
 =================
*/
static void CM_RecursiveBoxLeafNums (int nodeNum){

	clipNode_t	*node;
	cplane_t	*plane;
	int			side;

	while (1){
		if (nodeNum < 0){
			if (cm_leafCount >= cm_leafMaxCount)
				return;
			
			cm_leafList[cm_leafCount++] = -1 - nodeNum;
			return;
		}
	
		node = &cm.nodes[nodeNum];
		plane = node->plane;

		side = BoxOnPlaneSide(cm_leafMins, cm_leafMaxs, plane);

		if (side == PLANESIDE_FRONT)
			nodeNum = node->children[0];
		else if (side == PLANESIDE_BACK)
			nodeNum = node->children[1];
		else {
			// Go down both
			if (cm_leafTopNode == -1)
				cm_leafTopNode = nodeNum;
		
			CM_RecursiveBoxLeafNums(node->children[0]);
			nodeNum = node->children[1];
		}
	}
}

/*
 =================
 CM_BoxLeafNumsHeadNode
 =================
*/
int CM_BoxLeafNumsHeadNode (const vec3_t mins, const vec3_t maxs, int *list, int listSize, int headNode, int *topNode){

	cm_leafList = list;
	cm_leafCount = 0;
	cm_leafMaxCount = listSize;
	VectorCopy(mins, cm_leafMins);
	VectorCopy(maxs, cm_leafMaxs);
	cm_leafTopNode = -1;

	CM_RecursiveBoxLeafNums(headNode);

	if (topNode)
		*topNode = cm_leafTopNode;

	return cm_leafCount;
}

/*
 =================
 CM_BoxLeafNums
 =================
*/
int	CM_BoxLeafNums (const vec3_t mins, const vec3_t maxs, int *list, int listSize, int *topNode){

	return CM_BoxLeafNumsHeadNode(mins, maxs, list, listSize, cm.models[0].headNode, topNode);
}

/*
 =================
 CM_RecursivePointLeafNum
 =================
*/
static int CM_RecursivePointLeafNum (const vec3_t p, int nodeNum){

	float		d;
	clipNode_t		*node;
	cplane_t	*plane;

//	cm_pointContents++;		// Optimize counter

	while (nodeNum >= 0){
		node = &cm.nodes[nodeNum];
		plane = node->plane;

		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct(p, plane->normal) - plane->dist;

		if (d < 0)
			nodeNum = node->children[1];
		else
			nodeNum = node->children[0];
	}

	return -1 - nodeNum;
}

/*
 =================
 CM_PointLeafNum
 =================
*/
int CM_PointLeafNum (const vec3_t p){

	if (!cm.loaded)
		return 0;		// Map not loaded
	
	return CM_RecursivePointLeafNum(p, 0);
}

/*
 =================
 CM_PointContents
 =================
*/
int CM_PointContents (const vec3_t p, int headNode){

	int		l;

	if (!cm.loaded)
		return 0;		// Map not loaded

	l = CM_RecursivePointLeafNum(p, headNode);

	return cm.leafs[l].contents;
}

/*
 =================
 CM_TransformedPointContents

 Handles offseting and rotation of the point for moving and rotating
 entities
 =================
*/
int	CM_TransformedPointContents (const vec3_t p, int headNode, const vec3_t origin, const vec3_t angles){

	vec3_t	p2, temp;
	vec3_t	axis[3];
	int		l;

	if (!cm.loaded)
		return 0;		// Map not loaded

	if (headNode != cm_boxHeadNode && !VectorCompare(angles, vec3_origin)){
		AnglesToMat3(angles, axis);

		VectorSubtract(p, origin, temp);
		VectorRotate(temp, axis, p2);
	}
	else
		VectorSubtract(p, origin, p2);

	l = CM_RecursivePointLeafNum(p2, headNode);

	return cm.leafs[l].contents;
}


/*
 =======================================================================

 BOX TRACING

 =======================================================================
*/


/*
 =======================================================================

 PVS / PHS

 =======================================================================
*/

static byte	cm_pvsRow[MAX_MAP_LEAFS/8];
static byte	cm_phsRow[MAX_MAP_LEAFS/8];


/*
 =================
 CM_DecompressVis
 =================
*/
static void CM_DecompressVis (const byte *in, byte *out){

	byte	*out_p;
	int		c, row;

	row = (cm.numClusters+7)>>3;	
	out_p = out;

	if (!in){
		// No vis info, so make all visible
		while (row){
			*out_p++ = 0xff;
			row--;
		}

		return;		
	}

	do {
		if (*in){
			*out_p++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((out_p - out) + c > row){
			c = row - (out_p - out);
			Com_DPrintf(S_COLOR_YELLOW "Vis decompression overrun\n");
		}

		while (c){
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}

/*
 =================
 CM_ClusterPVS
 =================
*/
byte *CM_ClusterPVS (int cluster){

	if (cluster == -1 || cm.numVisibility == 0)
		Mem_Fill(cm_pvsRow, 0, (cm.numClusters+7)>>3);
	else
		CM_DecompressVis((byte *)cm.visibility + cm.visibility->bitOfs[cluster][VIS_PVS], cm_pvsRow);

	return cm_pvsRow;
}

/*
 =================
 CM_ClusterPHS
 =================
*/
byte *CM_ClusterPHS (int cluster){

	if (cluster == -1 || cm.numVisibility == 0)
		Mem_Fill(cm_phsRow, 0, (cm.numClusters+7)>>3);
	else
		CM_DecompressVis((byte *)cm.visibility + cm.visibility->bitOfs[cluster][VIS_PHS], cm_phsRow);

	return cm_phsRow;
}


/*
 =======================================================================

 AREAPORTALS

 =======================================================================
*/

// =====================================================================

/*
 =================
 CM_HeadNodeVisible

 Returns true if any leaf under headNode has a cluster that is 
 potentially visible
 =================
*/
bool CM_HeadNodeVisible (int headNode, const byte *visBits){

	int		leafNum;
	int		cluster;
	clipNode_t	*node;

	if (!cm.loaded)
		return false;		// Map not loaded

	if (headNode < 0){
		leafNum = -1 - headNode;
		cluster = cm.leafs[leafNum].cluster;
		if (cluster == -1)
			return false;
		
		if (visBits[cluster>>3] & (1<<(cluster&7)))
			return true;

		return false;
	}

	node = &cm.nodes[headNode];

	if (CM_HeadNodeVisible(node->children[0], visBits))
		return true;

	return CM_HeadNodeVisible(node->children[1], visBits);
}