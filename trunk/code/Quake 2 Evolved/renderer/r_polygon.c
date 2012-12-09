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
// r_polygon.c - Polygon functions
//


#include "r_local.h"


/*
 ==================
 R_ClipPolygon
 ==================
*/
bool R_ClipPolygon (int numPoints, vec3_t *points, const cplane_t plane, float epsilon, int *numClipped, vec3_t *clipped){

	vec3_t	tmpVector, tmpVector2;
	float	dists[MAX_POLYGON_POINTS];
	int		sides[MAX_POLYGON_POINTS];
	bool	frontSide, backSide;
	float	frac;
	int		i;

	if (numPoints >= MAX_POLYGON_POINTS - 2)
		Com_Error(ERR_DROP, "R_ClipPolygon: MAX_POLYGON_POINTS hit");

	*numClipped = 0;

	// Determine sides for each point
	frontSide = false;
	backSide = false;

	for (i = 0; i < numPoints; i++){
		dists[i] = PlaneDistance(plane.normal, plane.dist, points[i]);

		if (dists[i] > epsilon){
			sides[i] = PLANESIDE_FRONT;
			frontSide = true;
			continue;
		}

		if (dists[i] < -epsilon){
			sides[i] = PLANESIDE_BACK;
			backSide = true;
			continue;
		}

		sides[i] = PLANESIDE_ON;
	}

	if (!frontSide)
		return false;	// Not clipped

	if (!backSide){
		*numClipped = numPoints;
		Mem_Copy(clipped, points, numPoints * sizeof(vec3_t));

		return true;
	}

	// Clip it
	VectorCopy(points[0], points[i]);

	dists[i] = dists[0];
	sides[i] = sides[0];

	for (i = 0; i < numPoints; i++){
		if (sides[i] == PLANESIDE_ON){
			VectorCopy(points[i], clipped[(*numClipped)++]);
			continue;
		}

		if (sides[i] == PLANESIDE_FRONT)
			VectorCopy(points[i], clipped[(*numClipped)++]);

		if (sides[i+1] == PLANESIDE_ON || sides[i+1] == sides[i])
			continue;

		if (dists[i] == dists[i+1])
			VectorCopy(points[i], clipped[(*numClipped)++]);
		else {
			frac = dists[i] / (dists[i] - dists[i+1]);

			VectorSubtract(points[i+1], points[i], tmpVector);
			VectorMA(points[i], frac, tmpVector, tmpVector2);
			VectorCopy(tmpVector2, clipped[(*numClipped)++]);
		}
	}

	return true;
}