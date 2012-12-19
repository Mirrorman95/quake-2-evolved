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
// vector.h - Vector math
//


#ifndef __MATH_VECTOR_H__
#define __MATH_VECTOR_H__


typedef float				vec_t;
typedef vec_t				vec2_t[2];
typedef vec_t				vec3_t[3];
typedef vec_t				vec4_t[4];
typedef vec_t				vec5_t[5];

extern vec3_t				vec3_origin;


/*
 ==============================================================================

 3D VECTOR

 ==============================================================================
*/

void			ProjectPointOnPlane (vec3_t dst, const vec3_t p, const vec3_t normal);
void			VectorToAngles (const vec3_t vec, vec3_t angles);
void			VectorToMatrix (const vec3_t vec, vec3_t matrix[3]);
vec_t			DotProduct (const vec3_t v1, const vec3_t v2);
void			CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross);
vec_t			Distance (const vec3_t v1, const vec3_t v2);
vec_t			DistanceFast (const vec3_t v1, const vec3_t v2);
vec_t			DistanceSquared (const vec3_t v1, const vec3_t v2);
void			SnapVector (vec3_t v);
void			VectorCopy (const vec3_t in, vec3_t out);
void			VectorSet (vec3_t v, float x, float y, float z);
void			VectorClear (vec3_t v);
bool			VectorCompare (const vec3_t v1, const vec3_t v2);
void			VectorAdd (const vec3_t v1, const vec3_t v2, vec3_t out);
void			VectorSubtract (const vec3_t v1, const vec3_t v2, vec3_t out);
void			VectorScale (const vec3_t v, float scale, vec3_t out);
void			VectorMultiply (const vec3_t v1, const vec3_t v2, vec3_t out);
void			VectorMA (const vec3_t v1, float scale, const vec3_t v2, vec3_t out);
void			VectorSA (const vec3_t v1, float scale, const vec3_t v2, vec3_t out);
void			VectorAverage (const vec3_t v1, const vec3_t v2, vec3_t out);
void			VectorMin (const vec3_t v1, const vec3_t v2, vec3_t out);
void			VectorMax (const vec3_t v1, const vec3_t v2, vec3_t out);
void			VectorNegate (const vec3_t v, vec3_t out);
void			VectorInverse (vec3_t v);
vec_t			VectorLength (const vec3_t v);
vec_t			VectorLengthSquared (const vec3_t v);
void			VectorLerp (const vec3_t from, const vec3_t to, float frac, vec3_t out);
void			VectorRotate (const vec3_t v, const vec3_t matrix[3], vec3_t out);
void			VectorReflect (const vec3_t v, const vec3_t normal, vec3_t out);
vec_t			VectorNormalize (vec3_t v);
vec_t			VectorNormalize2 (const vec3_t v, vec3_t out);
void			VectorNormalizeFast (vec3_t v);

/*
 ==============================================================================

 4D VECTOR

 ==============================================================================
*/

void			Vector4Copy (const vec4_t in, vec4_t out);


#endif	// __MATH_VECTOR_H__