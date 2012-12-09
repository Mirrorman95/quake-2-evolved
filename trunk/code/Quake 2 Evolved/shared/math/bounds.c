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
// bounds.c - Bounds math
//


#include "../../common/common.h"


/*
 ==================
 ClearBounds
 ==================
*/
void ClearBounds (vec3_t mins, vec3_t maxs){

	mins[0] = mins[1] = mins[2] = M_INFINITY;
	maxs[0] = maxs[1] = maxs[2] = -M_INFINITY;
}

/*
 ==================
 BoundsIsCleared
 ==================
*/
bool BoundsIsCleared (vec3_t mins, vec3_t maxs){

	if (mins[0] <= maxs[0] || mins[1] <= maxs[1] || mins[2] <= maxs[2])
		return false;

	return true;
}

/*
 ==================
 AddPointToBounds
 ==================
*/
void AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs){

	if (v[0] < mins[0])
		mins[0] = v[0];
	if (v[0] > maxs[0])
		maxs[0] = v[0];

	if (v[1] < mins[1])
		mins[1] = v[1];
	if (v[1] > maxs[1])
		maxs[1] = v[1];

	if (v[2] < mins[2])
		mins[2] = v[2];
	if (v[2] > maxs[2])
		maxs[2] = v[2];
}

/*
 ==================
 BoundsToPoints
 ==================
*/
void BoundsToPoints (const vec3_t mins, const vec3_t maxs, vec3_t points[8]){

	int		i;

	for (i = 0; i < 8; i++){
		points[i][0] = (i & 1) ? mins[0] : maxs[0];
		points[i][1] = (i & 2) ? mins[1] : maxs[1];
		points[i][2] = (i & 4) ? mins[2] : maxs[2];
	}
}

/*
 ==================
 BoundsFromPoints
 ==================
*/
void BoundsFromPoints (vec3_t mins, vec3_t maxs, const vec3_t points[8]){

	int		i;

	mins[0] = mins[1] = mins[2] = M_INFINITY;
	maxs[0] = maxs[1] = maxs[2] = -M_INFINITY;

	for (i = 0; i < 8; i++){
		if (points[i][0] < mins[0])
			mins[0] = points[i][0];
		if (points[i][0] > mins[0])
			maxs[0] = points[i][0];

		if (points[i][1] < mins[1])
			mins[1] = points[i][1];
		if (points[i][1] > mins[1])
			maxs[1] = points[i][1];

		if (points[i][2] < mins[2])
			mins[2] = points[i][2];
		if (points[i][2] > mins[2])
			maxs[2] = points[i][2];
	}
}

/*
 ==================
 RadiusFromBounds
 ==================
*/
float RadiusFromBounds (const vec3_t mins, const vec3_t maxs){

	vec3_t	corner;
	float	a, b;

	a = fabs(mins[0]);
	b = fabs(maxs[0]);
	corner[0] = a > b ? a : b;

	a = fabs(mins[1]);
	b = fabs(maxs[1]);
	corner[1] = a > b ? a : b;

	a = fabs(mins[2]);
	b = fabs(maxs[2]);
	corner[2] = a > b ? a : b;

	return VectorLength(corner);
}

/*
 ==================
 BoundsIntersect
 ==================
*/
bool BoundsIntersect (const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2){

	if (mins1[0] > maxs2[0] || mins1[1] > maxs2[1] || mins1[2] > maxs2[2])
		return false;
	if (maxs1[0] < mins2[0] || maxs1[1] < mins2[1] || maxs1[2] < mins2[2])
		return false;

	return true;
}

/*
 ==================
 BoundsAndSphereIntersect
 ==================
*/
bool BoundsAndSphereIntersect (const vec3_t mins, const vec3_t maxs, const vec3_t origin, float radius){

	if (mins[0] > origin[0] + radius || mins[1] > origin[1] + radius || mins[2] > origin[2] + radius)
		return false;
	if (maxs[0] < origin[0] - radius || maxs[1] < origin[1] - radius || maxs[2] < origin[2] - radius)
		return false;

	return true;
}

/*
 ==================
 BoundsAndPointIntersect
 ==================
*/
bool BoundsAndPointIntersect (const vec3_t mins, const vec3_t maxs, const vec3_t point){

	if (point[0] > maxs[0] || point[1] > maxs[1] || point[2] > maxs[2])
		return false;
	if (point[0] < mins[0] || point[1] < mins[1] || point[2] < mins[2])
		return false;

	return true;
}

/*
 ==================
 BoundsContainsPoint
 ==================
*/
bool BoundsContainsPoint (const vec3_t mins, const vec3_t maxs, const vec3_t point){

	if (maxs[0] > point[0] || maxs[1] > point[1] || maxs[2] > point[2])
		return false;
	if (mins[0] < point[0] || mins[1] < point[1] || mins[2] < point[2])
		return false;

	return true;
}