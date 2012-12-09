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
// plane.c - Plane math
//


#include "../../common/common.h"


/*
 ==================
 PlaneFromPoints

 Returns false if the triangle is degenerate.
 The normal will point out of the clock for clockwise ordered points.
 ==================
*/
bool PlaneFromPoints (cplane_t *plane, const vec3_t a, const vec3_t b, const vec3_t c){
	
	vec3_t	dir1, dir2;

	VectorSubtract(b, a, dir1);
	VectorSubtract(c, a, dir2);
	CrossProduct(dir2, dir1, plane->normal);

	if (!VectorNormalize(plane->normal)){
		plane->dist = 0.0f;
		return false;
	}

	plane->dist = DotProduct(a, plane->normal);

	return true;
}

/*
 ==================
 SetPlaneSignbits

 For fast box on plane side test
 ==================
*/
void SetPlaneSignbits (cplane_t *plane){

	plane->signbits = 0;

	if (plane->normal[0] < 0.0f)
		plane->signbits |= 1;
	if (plane->normal[1] < 0.0f)
		plane->signbits |= 2;
	if (plane->normal[2] < 0.0f)
		plane->signbits |= 4;
}

/*
 ==================
 PlaneTypeForNormal
 ==================
*/
int PlaneTypeForNormal (const vec3_t normal){

	if (normal[0] == 1.0f)
		return PLANE_X;
	if (normal[1] == 1.0f)
		return PLANE_Y;
	if (normal[2] == 1.0f)
		return PLANE_Z;

	return PLANE_NON_AXIAL;
}

/*
 ==================
 PlaneDistance
 ==================
*/
float PlaneDistance (const vec3_t normal, const float dist, const vec3_t point){

	return normal[0] * point[0] + normal[1] * point[1] + normal[2] * point[2] + -dist;
}

/*
 ==================
 
 TODO: rewrite?

 Returns 1, 2, or 1 + 2
 ==================
*/
int BoxOnPlaneSide (const vec3_t mins, const vec3_t maxs, const cplane_t *plane){

	float	dist1, dist2;

	// Fast axial cases
	if (plane->type < PLANE_NON_AXIAL){
		if (plane->dist <= mins[plane->type])
			return PLANESIDE_FRONT;
		if (plane->dist >= maxs[plane->type])
			return PLANESIDE_BACK;

		return PLANESIDE_CROSS;
	}
	
	// General case
	switch (plane->signbits){
	case 0:
		dist1 = plane->normal[0] * maxs[0] + plane->normal[1] * maxs[1] + plane->normal[2] * maxs[2];
		dist2 = plane->normal[0] * mins[0] + plane->normal[1] * mins[1] + plane->normal[2] * mins[2];
		break;
	case 1:
		dist1 = plane->normal[0] * mins[0] + plane->normal[1] * maxs[1] + plane->normal[2] * maxs[2];
		dist2 = plane->normal[0] * maxs[0] + plane->normal[1] * mins[1] + plane->normal[2] * mins[2];
		break;
	case 2:
		dist1 = plane->normal[0] * maxs[0] + plane->normal[1] * mins[1] + plane->normal[2] * maxs[2];
		dist2 = plane->normal[0] * mins[0] + plane->normal[1] * maxs[1] + plane->normal[2] * mins[2];
		break;
	case 3:
		dist1 = plane->normal[0] * mins[0] + plane->normal[1] * mins[1] + plane->normal[2] * maxs[2];
		dist2 = plane->normal[0] * maxs[0] + plane->normal[1] * maxs[1] + plane->normal[2] * mins[2];
		break;
	case 4:
		dist1 = plane->normal[0] * maxs[0] + plane->normal[1] * maxs[1] + plane->normal[2] * mins[2];
		dist2 = plane->normal[0] * mins[0] + plane->normal[1] * mins[1] + plane->normal[2] * maxs[2];
		break;
	case 5:
		dist1 = plane->normal[0] * mins[0] + plane->normal[1] * maxs[1] + plane->normal[2] * mins[2];
		dist2 = plane->normal[0] * maxs[0] + plane->normal[1] * mins[1] + plane->normal[2] * maxs[2];
		break;
	case 6:
		dist1 = plane->normal[0] * maxs[0] + plane->normal[1] * mins[1] + plane->normal[2] * mins[2];
		dist2 = plane->normal[0] * mins[0] + plane->normal[1] * maxs[1] + plane->normal[2] * maxs[2];
		break;
	case 7:
		dist1 = plane->normal[0] * mins[0] + plane->normal[1] * mins[1] + plane->normal[2] * mins[2];
		dist2 = plane->normal[0] * maxs[0] + plane->normal[1] * maxs[1] + plane->normal[2] * maxs[2];
		break;
	default:
		dist1 = 0.0f;
		dist2 = 0.0f;
		break;
	}

	if (dist2 < plane->dist){
		if (dist1 >= plane->dist)
			return PLANESIDE_CROSS;
		else
			return PLANESIDE_BACK;
	}

	return PLANESIDE_FRONT;
}

/*
 ==================
 SphereOnPlaneSide
 ==================
*/
int SphereOnPlaneSide (const vec3_t center, float radius, const cplane_t *plane){

	float	dist;

	if (plane->type < PLANE_NON_AXIAL)
		dist = center[plane->type] - plane->dist;
	else
		dist = DotProduct(center, plane->normal) - plane->dist;

	if (dist > radius)
		return PLANESIDE_FRONT;
	if (dist < -radius)
		return PLANESIDE_BACK;

	return PLANESIDE_CROSS;
}

/*
 ==================
 PointOnPlaneSide
 ==================
*/
int PointOnPlaneSide (const vec3_t point, float epsilon, const cplane_t *plane){

	float	dist;

	if (plane->type < PLANE_NON_AXIAL)
		dist = point[plane->type] - plane->dist;
	else
		dist = DotProduct(point, plane->normal) - plane->dist;

	if (dist > epsilon)
		return PLANESIDE_FRONT;
	if (dist < -epsilon)
		return PLANESIDE_BACK;

	return PLANESIDE_ON;
}