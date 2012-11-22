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
// matrix.h - Matrix math
//


#ifndef __MATH_MATRIX_H__
#define __MATH_MATRIX_H__


typedef float				mat3_t[9];
typedef float				mat4_t[16];

extern vec3_t				mat3_identity[3];
extern mat4_t				mat4_identity;

/*
 ==============================================================================

 3x3 MATRIX

 ==============================================================================
*/

void			Matrix3_Copy (const vec3_t in[3], vec3_t out[3]);
bool			Matrix3_Compare (const vec3_t axis1[3], const vec3_t axis2[3]);
void			Matrix3_Multiply (const vec3_t in1[3], const vec3_t in2[3], vec3_t out[3]);
void			Matrix3_Identity (vec3_t axis[3]);

/*
 ==============================================================================

 4x4 MATRIX

 ==============================================================================
*/

void			Matrix4_Copy (const mat4_t in, mat4_t out);
bool			Matrix4_Compare (const mat4_t m1, const mat4_t m2);
void			Matrix4_Set (mat4_t m, const vec3_t rotation[3], const vec3_t translation);
void			Matrix4_Transpose (const mat4_t m, mat4_t out);
void			Matrix4_Multiply (const mat4_t m1, const mat4_t m2, mat4_t out);
void			Matrix4_MultiplyFast (const mat4_t m1, const mat4_t m2, mat4_t out);
void			Matrix4_Identity (mat4_t m);
void			Matrix4_Rotate (mat4_t m, float angle, float x, float y, float z);
void			Matrix4_Scale (mat4_t m, float x, float y, float z);
void			Matrix4_Translate (mat4_t m, float x, float y, float z);
void			Matrix4_Shear (mat4_t m, float x, float y, float z);


#endif	// __MATH_MATRIX_H__