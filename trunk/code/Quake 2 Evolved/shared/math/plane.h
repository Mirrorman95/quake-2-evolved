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
// plane.h - Plane math
//


#ifndef __MATH_PLANE_H__
#define __MATH_PLANE_H__


#define ON_EPSILON					0.1f

// Plane sides
#define	PLANESIDE_FRONT				0
#define	PLANESIDE_BACK				1
#define	PLANESIDE_ON				2
#define PLANESIDE_CROSS				3

// 0-2 are axial planes
#define	PLANE_X						0
#define	PLANE_Y						1
#define	PLANE_Z						2
#define PLANE_NON_AXIAL				3

typedef struct cplane_s {
	vec3_t					normal;
	float					dist;
	byte					type;				// For fast side tests
	byte					signbits;			// signx + (signy<<1) + (signz<<1)
	byte					pad[2];
} cplane_t;

bool			PlaneFromPoints (cplane_t *plane, const vec3_t a, const vec3_t b, const vec3_t c);
void			SetPlaneSignbits (cplane_t *plane);
int				PlaneTypeForNormal (const vec3_t normal);
int				BoxOnPlaneSide (const vec3_t mins, const vec3_t maxs, cplane_t *p);
int				SphereOnPlaneSide (const vec3_t center, float radius, const cplane_t *plane);
int				PointOnPlaneSide (const vec3_t point, float epsilon, const cplane_t *plane);


#endif	// __MATH_PLANE_H__