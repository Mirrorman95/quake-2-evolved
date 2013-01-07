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
// cm_contents.c - Content related code
//


#include "cm_local.h"


// TODO: just here for now
extern int		cm_boxHeadNode;


/*
 ==================
 CM_LeafContents
 ==================
*/
int	CM_LeafContents (int leafNum){

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_LeafContents: map not loaded");

	if (leafNum < 0 || leafNum >= cm.numLeafs)
		Com_Error(ERR_DROP, "CM_LeafContents: bad number");

	return cm.leafs[leafNum].contents;
}

/*
 ==================
 CM_PointContents
 ==================
*/
int CM_PointContents (const vec3_t point, int headNode){

	int		leaf;

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_PointContents: map not loaded");

	leaf = CM_PointInLeaf(point, headNode);

	return cm.leafs[leaf].contents;
}

/*
 ==================
 CM_TransformedPointContents
 ==================
*/
int	CM_TransformedPointContents (const vec3_t point, int headNode, const vec3_t origin, const vec3_t angles){

	vec3_t	point2, tmp;
	vec3_t	axis[3];
	int		leaf;

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_TransformedPointContents: map not loaded");

	// Update the point if needed
	if (headNode != cm_boxHeadNode && !VectorCompare(angles, vec3_origin)){
		AnglesToMat3(angles, axis);

		VectorSubtract(point, origin, tmp);
		VectorRotate(tmp, axis, point2);
	}
	else
		VectorSubtract(point, origin, point2);

	// Get the current view leaf
	leaf = CM_PointInLeaf(point2, headNode);

	return cm.leafs[leaf].contents;
}