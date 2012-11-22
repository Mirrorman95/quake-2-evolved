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
// cm_trace.c - Box tracing
//


#include "cm_local.h"


// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON				(0.03125f)

static vec3_t				cm_traceStart, cm_traceEnd;
static vec3_t				cm_traceMins, cm_traceMaxs;
static vec3_t				cm_traceExtents;

static trace_t				cm_trace;
static int					cm_traceContents;
static bool					cm_traceIsPoint;		// Optimized case

static int					cm_traceCheckCount;

// TODO: just here for now
extern int		cm_boxHeadNode;
int CM_BoxLeafNumsHeadNode (const vec3_t mins, const vec3_t maxs, int *list, int listSize, int headNode, int *topNode);


/*
 ==================
 
 ==================
*/
static void CM_ClipBoxToBrush (const vec3_t mins, const vec3_t maxs, const vec3_t p1, const vec3_t p2, trace_t *trace, clipBrush_t *brush){

	clipBrushSide_t	*side, *leadSide;
	cplane_t		*plane, *clipPlane;
	vec3_t			ofs;
	float			dist, d1, d2;
	float			enterFrac, leaveFrac;
	float			f;
	bool			getOut, startOut;
	int				i, j;

	enterFrac = -1;
	leaveFrac = 1;
	clipPlane = NULL;

	if (!brush->numSides)
		return;

	getOut = false;
	startOut = false;
	leadSide = NULL;

	for (i = 0; i < brush->numSides; i++){
		side = &cm.brushSides[brush->firstBrushSide+i];
		plane = side->plane;

		if (!cm_traceIsPoint){
			// General box case
			if (plane->type < 3){
				// Push the plane out appropriately for mins/maxs
				if (plane->normal[plane->type] < 0)
					dist = plane->dist - maxs[plane->type];
				else
					dist = plane->dist - mins[plane->type];				

				d1 = p1[plane->type] - dist;
				d2 = p2[plane->type] - dist;
			}
			else {
				// Push the plane out appropriately for mins/maxs
				for (j = 0; j < 3; j++){
					if (plane->normal[j] < 0.0f)
						ofs[j] = maxs[j];
					else
						ofs[j] = mins[j];
				}

				dist = plane->dist - DotProduct(ofs, plane->normal);

				d1 = DotProduct(p1, plane->normal) - dist;
				d2 = DotProduct(p2, plane->normal) - dist;
			}
		}
		else {
			// Special point case
			if (plane->type < 3){
				d1 = p1[plane->type] - plane->dist;
				d2 = p2[plane->type] - plane->dist;
			}
			else {
				d1 = DotProduct(p1, plane->normal) - plane->dist;
				d2 = DotProduct(p2, plane->normal) - plane->dist;
			}
		}

		if (d2 > 0.0f)
			getOut = true;	// End point is not in solid
		if (d1 > 0.0f)
			startOut = true;

		// If completely in front of face, no intersection
		if (d1 > 0.0f && d2 >= d1)
			return;

		if (d1 <= 0.0f && d2 <= 0.0f)
			continue;

		// Crosses face
		if (d1 > d2){
			// Enter
			f = (d1 - DIST_EPSILON) / (d1 - d2);
			if (f > enterFrac){
				enterFrac = f;
				clipPlane = plane;
				leadSide = side;
			}
		}
		else {
			// Leave
			f = (d1 + DIST_EPSILON) / (d1 - d2);
			if (f < leaveFrac)
				leaveFrac = f;
		}
	}

	if (!startOut){
		// Original point was inside brush
		trace->startsolid = true;
		if (!getOut)
			trace->allsolid = true;

		return;
	}

	if (enterFrac < leaveFrac){
		if (enterFrac > -1.0f && enterFrac < trace->fraction){
			if (enterFrac < 0.0f)
				enterFrac = 0.0f;

			trace->fraction = enterFrac;
			trace->plane = *clipPlane;
			trace->surface = &(leadSide->surface->c);
			trace->contents = brush->contents;
		}
	}
}

/*
 ==================
 
 ==================
*/
static void CM_TestBoxInBrush (const vec3_t mins, const vec3_t maxs, const vec3_t p, trace_t *trace, clipBrush_t *brush){

	clipBrushSide_t	*side;
	cplane_t		*plane;
	vec3_t			ofs;
	float			dist, d;
	int				i, j;

	if (!brush->numSides)
		return;

	for (i = 0; i < brush->numSides; i++){
		side = &cm.brushSides[brush->firstBrushSide+i];
		plane = side->plane;

		// General box case
		if (plane->type < 3){
			// Push the plane out appropriately for mins/maxs
			if (plane->normal[plane->type] < 0)
				dist = plane->dist - maxs[plane->type];
			else
				dist = plane->dist - mins[plane->type];

			d = p[plane->type] - dist;
		}
		else {
			// Push the plane out appropriately for mins/maxs
			for (j = 0; j < 3; j++){
				if (plane->normal[j] < 0.0f)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}

			dist = plane->dist - DotProduct(ofs, plane->normal);

			d = DotProduct(p, plane->normal) - dist;
		}

		// If completely in front of face, no intersection
		if (d > 0.0f)
			return;
	}

	// Inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}

/*
 ==================
 
 ==================
*/
static void CM_TraceToLeaf (int leafNum){

	clipLeaf_t	*leaf;
	clipBrush_t	*brush;
	int			brushNum;
	int			i;

	leaf = &cm.leafs[leafNum];
	if (!(leaf->contents & cm_traceContents))
		return;

	// Trace line against all brushes in the leaf
	for (i = 0; i < leaf->numLeafBrushes; i++){
		brushNum = cm.leafBrushes[leaf->firstLeafBrush+i];
		brush = &cm.brushes[brushNum];

		if (brush->checkCount == cm_traceCheckCount)
			continue;		// Already checked this brush in another leaf
		brush->checkCount = cm_traceCheckCount;

		if (!(brush->contents & cm_traceContents))
			continue;

		CM_ClipBoxToBrush(cm_traceMins, cm_traceMaxs, cm_traceStart, cm_traceEnd, &cm_trace, brush);
		if (!cm_trace.fraction)
			return;
	}
}

/*
 ==================
 
 ==================
*/
static void CM_TestInLeaf (int leafNum){

	clipLeaf_t	*leaf;
	clipBrush_t	*brush;
	int			brushNum;
	int			i;

	leaf = &cm.leafs[leafNum];
	if (!(leaf->contents & cm_traceContents))
		return;

	// Trace line against all brushes in the leaf
	for (i = 0; i < leaf->numLeafBrushes; i++){
		brushNum = cm.leafBrushes[leaf->firstLeafBrush+i];
		brush = &cm.brushes[brushNum];

		if (brush->checkCount == cm_traceCheckCount)
			continue;		// Already checked this brush in another leaf
		brush->checkCount = cm_traceCheckCount;

		if (!(brush->contents & cm_traceContents))
			continue;

		CM_TestBoxInBrush(cm_traceMins, cm_traceMaxs, cm_traceStart, &cm_trace, brush);
		if (!cm_trace.fraction)
			return;
	}
}

/*
 ==================
 
 ==================
*/
static void CM_RecursiveHullCheck (int num, float pf1, float pf2, const vec3_t p1, const vec3_t p2){

	clipNode_t	*node;
	cplane_t	*plane;
	float		d1, d2, offset;
	float		frac1, frac2;
	float		dist;
	vec3_t		mid;
	int			side;
	float		midf;

	if (cm_trace.fraction <= pf1)
		return;		// Already hit something nearer

	// If < 0, we are in a leaf node
	if (num < 0){
		CM_TraceToLeaf(-1-num);
		return;
	}

	// Find the point distances to the separating plane and the offset
	// for the size of the box
	node = &cm.nodes[num];
	plane = node->plane;

	if (plane->type < 3){
		d1 = p1[plane->type] - plane->dist;
		d2 = p2[plane->type] - plane->dist;

		offset = cm_traceExtents[plane->type];
	}
	else {
		d1 = DotProduct(p1, plane->normal) - plane->dist;
		d2 = DotProduct(p2, plane->normal) - plane->dist;

		if (cm_traceIsPoint)
			offset = 0.0f;
		else
			offset = fabs(cm_traceExtents[0]*plane->normal[0]) + fabs(cm_traceExtents[1]*plane->normal[1]) + fabs(cm_traceExtents[2]*plane->normal[2]);
	}

	// See which sides we need to consider
	if (d1 >= offset && d2 >= offset){
		CM_RecursiveHullCheck(node->children[0], pf1, pf2, p1, p2);
		return;
	}
	if (d1 < -offset && d2 < -offset){
		CM_RecursiveHullCheck(node->children[1], pf1, pf2, p1, p2);
		return;
	}
	
	// Put the crosspoint DIST_EPSILON pixels on the near side
	if (d1 < d2){
		dist = 1.0f / (d1 - d2);
		side = 1;
		frac1 = (d1 - offset + DIST_EPSILON) * dist;
		frac2 = (d1 + offset + DIST_EPSILON) * dist;
	}
	else if (d1 > d2){
		dist = 1.0f / (d1 - d2);
		side = 0;
		frac1 = (d1 + offset + DIST_EPSILON) * dist;
		frac2 = (d1 - offset - DIST_EPSILON) * dist;
	}
	else {
		side = 0;
		frac1 = 1.0f;
		frac2 = 0.0f;
	}

	// Move up to the node
	if (frac1 < 0.0f)
		frac1 = 0.0f;
	else if (frac1 > 1.0f)
		frac1 = 1.0f;

	midf = pf1 + (pf2 - pf1) * frac1;

	mid[0] = p1[0] + (p2[0] - p1[0]) * frac1;
	mid[1] = p1[1] + (p2[1] - p1[1]) * frac1;
	mid[2] = p1[2] + (p2[2] - p1[2]) * frac1;

	CM_RecursiveHullCheck(node->children[side], pf1, midf, p1, mid);

	// Go past the node
	if (frac2 < 0.0f)
		frac2 = 0.0f;
	else if (frac2 > 1.0f)
		frac2 = 1.0f;

	midf = pf1 + (pf2 - pf1) * frac2;

	mid[0] = p1[0] + (p2[0] - p1[0]) * frac2;
	mid[1] = p1[1] + (p2[1] - p1[1]) * frac2;
	mid[2] = p1[2] + (p2[2] - p1[2]) * frac2;

	CM_RecursiveHullCheck(node->children[side^1], midf, pf2, mid, p2);
}

/*
 ==================
 
 ==================
*/
trace_t CM_BoxTrace (const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, int headNode, int brushMask){

	// Fill in a default trace
	Mem_Fill(&cm_trace, 0, sizeof(cm_trace));
	cm_trace.fraction = 1;
	cm_trace.surface = &(cm.nullSurface.c);

	if (!cm.loaded)
		return cm_trace;	// Map not loaded

	cm_traceCheckCount++;	// For multi-check avoidance
//	cm_traces++;			// Optimize counter

	cm_traceContents = brushMask;
	VectorCopy(start, cm_traceStart);
	VectorCopy(end, cm_traceEnd);
	VectorCopy(mins, cm_traceMins);
	VectorCopy(maxs, cm_traceMaxs);

	// Check for position test special case
	if (VectorCompare(start, end)){
		int		i;
		vec3_t	c1, c2;
		int		leafs[1024], numLeafs;
		int		topNode;

		for (i = 0; i < 3; i++){
			c1[i] = (start[i] + mins[i]) - 1;
			c2[i] = (start[i] + maxs[i]) + 1;
		}

		numLeafs = CM_BoxLeafNumsHeadNode(c1, c2, leafs, 1024, headNode, &topNode);

		for (i = 0; i < numLeafs; i++){
			CM_TestInLeaf(leafs[i]);
			if (cm_trace.allsolid)
				break;
		}

		VectorCopy(start, cm_trace.endpos);

		return cm_trace;
	}

	// Check for point special case
	if (VectorCompare(mins, vec3_origin) && VectorCompare(maxs, vec3_origin)){
		cm_traceIsPoint = true;

		VectorClear(cm_traceExtents);
	}
	else {
		cm_traceIsPoint = false;

		cm_traceExtents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		cm_traceExtents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		cm_traceExtents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	// General sweeping through world
	CM_RecursiveHullCheck(headNode, 0.0f, 1.0f, start, end);

	if (cm_trace.fraction == 1.0f){
		cm_trace.endpos[0] = end[0];
		cm_trace.endpos[1] = end[1];
		cm_trace.endpos[2] = end[2];
	}
	else {
		cm_trace.endpos[0] = start[0] + (end[0] - start[0]) * cm_trace.fraction;
		cm_trace.endpos[1] = start[1] + (end[1] - start[1]) * cm_trace.fraction;
		cm_trace.endpos[2] = start[2] + (end[2] - start[2]) * cm_trace.fraction;
	}

	return cm_trace;
}

/*
 ==================
 
 Handles offseting and rotation of the points for moving and rotating
 entities
 ==================
*/
trace_t	CM_TransformedBoxTrace (const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, int headNode, int brushMask, const vec3_t origin, const vec3_t angles){

	trace_t	trace;
	vec3_t	start2, end2, angles2, temp;
	vec3_t	axis[3];
	bool	rotated;

	if (headNode != cm_boxHeadNode && !VectorCompare(angles, vec3_origin)){
		rotated = true;

		AnglesToMat3(angles, axis);

		VectorSubtract(start, origin, temp);
		VectorRotate(temp, axis, start2);

		VectorSubtract(end, origin, temp);
		VectorRotate(temp, axis, end2);
	}
	else {
		rotated = false;

		VectorSubtract(start, origin, start2);
		VectorSubtract(end, origin, end2);
	}

	// Sweep the box through the world
	trace = CM_BoxTrace(start2, end2, mins, maxs, headNode, brushMask);

	if (rotated && trace.fraction != 1.0){
		VectorNegate(angles, angles2);
		AnglesToMat3(angles2, axis);

		VectorCopy(trace.plane.normal, temp);
		VectorRotate(temp, axis, trace.plane.normal);
	}

	if (trace.fraction == 1.0f){
		trace.endpos[0] = end[0];
		trace.endpos[1] = end[1];
		trace.endpos[2] = end[2];
	}
	else {
		trace.endpos[0] = start[0] + (end[0] - start[0]) * trace.fraction;
		trace.endpos[1] = start[1] + (end[1] - start[1]) * trace.fraction;
		trace.endpos[2] = start[2] + (end[2] - start[2]) * trace.fraction;
	}

	return trace;
}