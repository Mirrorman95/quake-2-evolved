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
// matrix.c - Matrix math
//


#include "../../common/common.h"


vec3_t						mat3_identity[3] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
mat4_t						mat4_identity = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};


/*
 ==============================================================================

 3x3 MATRIX

 ==============================================================================
*/


/*
 ==================
 Matrix3_Copy
 ==================
*/
void Matrix3_Copy (const vec3_t in[3], vec3_t out[3]){

	out[0][0] = in[0][0];
	out[0][1] = in[0][1];
	out[0][2] = in[0][2];
	out[1][0] = in[1][0];
	out[1][1] = in[1][1];
	out[1][2] = in[1][2];
	out[2][0] = in[2][0];
	out[2][1] = in[2][1];
	out[2][2] = in[2][2];
}

/*
 ==================
 Matrix3_Compare
 ==================
*/
bool Matrix3_Compare (const vec3_t axis1[3], const vec3_t axis2[3]){

	if (axis1[0][0] != axis2[0][0] || axis1[0][1] != axis2[0][1] || axis1[0][2] != axis2[0][2])
		return false;
	if (axis1[1][0] != axis2[1][0] || axis1[1][1] != axis2[1][1] || axis1[1][2] != axis2[1][2])
		return false;
	if (axis1[2][0] != axis2[2][0] || axis1[2][1] != axis2[2][1] || axis1[2][2] != axis2[2][2])
		return false;

	return true;
}

/*
 ==================
 Matrix3_Multiply
 ==================
*/
void Matrix3_Multiply (const vec3_t in1[3], const vec3_t in2[3], vec3_t out[3]){

	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

/*
 ==================
 Matrix3_Identity
 ==================
*/
void Matrix3_Identity (vec3_t axis[3]){

	axis[0][0] = 1.0f;
	axis[0][1] = 0.0f;
	axis[0][2] = 0.0f;
	axis[1][0] = 0.0f;
	axis[1][1] = 1.0f;
	axis[1][2] = 0.0f;
	axis[2][0] = 0.0f;
	axis[2][1] = 0.0f;
	axis[2][2] = 1.0f;
}


/*
 ==============================================================================

 4x4 MATRIX

 ==============================================================================
*/


/*
 ==================
 Matrix4_Copy
 ==================
*/
void Matrix4_Copy (const mat4_t in, mat4_t out){

	out[ 0] = in[ 0];
	out[ 1] = in[ 1];
	out[ 2] = in[ 2];
	out[ 3] = in[ 3];
	out[ 4] = in[ 4];
	out[ 5] = in[ 5];
	out[ 6] = in[ 6];
	out[ 7] = in[ 7];
	out[ 8] = in[ 8];
	out[ 9] = in[ 9];
	out[10] = in[10];
	out[11] = in[11];
	out[12] = in[12];
	out[13] = in[13];
	out[14] = in[14];
	out[15] = in[15];
}

/*
 ==================
 Matrix4_Compare
 ==================
*/
bool Matrix4_Compare (const mat4_t m1, const mat4_t m2){

	if (m1[ 0] != m2[ 0])
		return false;
	if (m1[ 1] != m2[ 1])
		return false;
	if (m1[ 2] != m2[ 2])
		return false;
	if (m1[ 3] != m2[ 3])
		return false;
	if (m1[ 4] != m2[ 4])
		return false;
	if (m1[ 5] != m2[ 5])
		return false;
	if (m1[ 6] != m2[ 6])
		return false;
	if (m1[ 7] != m2[ 7])
		return false;
	if (m1[ 8] != m2[ 8])
		return false;
	if (m1[ 9] != m2[ 9])
		return false;
	if (m1[10] != m2[10])
		return false;
	if (m1[11] != m2[11])
		return false;
	if (m1[12] != m2[12])
		return false;
	if (m1[13] != m2[13])
		return false;
	if (m1[14] != m2[14])
		return false;
	if (m1[15] != m2[15])
		return false;

	return true;
}

/*
 ==================
 Matrix4_Set
 ==================
*/
void Matrix4_Set (mat4_t m, const vec3_t rotation[3], const vec3_t translation){

	m[ 0] = rotation[0][0];
	m[ 1] = rotation[0][1];
	m[ 2] = rotation[0][2];
	m[ 3] = 0.0f;
	m[ 4] = rotation[1][0];
	m[ 5] = rotation[1][1];
	m[ 6] = rotation[1][2];
	m[ 7] = 0.0f;
	m[ 8] = rotation[2][0];
	m[ 9] = rotation[2][1];
	m[10] = rotation[2][2];
	m[11] = 0.0f;
	m[12] = translation[0];
	m[13] = translation[1];
	m[14] = translation[2];
	m[15] = 1.0f;
}

/*
 ==================
 Matrix4_Transpose
 ==================
*/
void Matrix4_Transpose (const mat4_t m, mat4_t out){

	out[ 0] = m[ 0];
	out[ 1] = m[ 4];
	out[ 2] = m[ 8];
	out[ 3] = m[12];
	out[ 4] = m[ 1];
	out[ 5] = m[ 5];
	out[ 6] = m[ 9];
	out[ 7] = m[13];
	out[ 8] = m[ 2];
	out[ 9] = m[ 6];
	out[10] = m[10];
	out[11] = m[14];
	out[12] = m[ 3];
	out[13] = m[ 7];
	out[14] = m[11];
	out[15] = m[15];
}

/*
 ==================
 Matrix4_Multiply
 ==================
*/
void Matrix4_Multiply (const mat4_t m1, const mat4_t m2, mat4_t out){

	out[ 0] = m1[ 0] * m2[ 0] + m1[ 4] * m2[ 1] + m1[ 8] * m2[ 2] + m1[12] * m2[ 3];
	out[ 1] = m1[ 1] * m2[ 0] + m1[ 5] * m2[ 1] + m1[ 9] * m2[ 2] + m1[13] * m2[ 3];
	out[ 2] = m1[ 2] * m2[ 0] + m1[ 6] * m2[ 1] + m1[10] * m2[ 2] + m1[14] * m2[ 3];
	out[ 3] = m1[ 3] * m2[ 0] + m1[ 7] * m2[ 1] + m1[11] * m2[ 2] + m1[15] * m2[ 3];
	out[ 4] = m1[ 0] * m2[ 4] + m1[ 4] * m2[ 5] + m1[ 8] * m2[ 6] + m1[12] * m2[ 7];
	out[ 5] = m1[ 1] * m2[ 4] + m1[ 5] * m2[ 5] + m1[ 9] * m2[ 6] + m1[13] * m2[ 7];
	out[ 6] = m1[ 2] * m2[ 4] + m1[ 6] * m2[ 5] + m1[10] * m2[ 6] + m1[14] * m2[ 7];
	out[ 7] = m1[ 3] * m2[ 4] + m1[ 7] * m2[ 5] + m1[11] * m2[ 6] + m1[15] * m2[ 7];
	out[ 8] = m1[ 0] * m2[ 8] + m1[ 4] * m2[ 9] + m1[ 8] * m2[10] + m1[12] * m2[11];
	out[ 9] = m1[ 1] * m2[ 8] + m1[ 5] * m2[ 9] + m1[ 9] * m2[10] + m1[13] * m2[11];
	out[10] = m1[ 2] * m2[ 8] + m1[ 6] * m2[ 9] + m1[10] * m2[10] + m1[14] * m2[11];
	out[11] = m1[ 3] * m2[ 8] + m1[ 7] * m2[ 9] + m1[11] * m2[10] + m1[15] * m2[11];
	out[12] = m1[ 0] * m2[12] + m1[ 4] * m2[13] + m1[ 8] * m2[14] + m1[12] * m2[15];
	out[13] = m1[ 1] * m2[12] + m1[ 5] * m2[13] + m1[ 9] * m2[14] + m1[13] * m2[15];
	out[14] = m1[ 2] * m2[12] + m1[ 6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15];
	out[15] = m1[ 3] * m2[12] + m1[ 7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15];
}

/*
 ==================
 Matrix4_MultiplyFast
 ==================
*/
void Matrix4_MultiplyFast (const mat4_t m1, const mat4_t m2, mat4_t out){

	out[ 0] = m1[ 0] * m2[ 0] + m1[ 4] * m2[ 1] + m1[ 8] * m2[ 2];
	out[ 1] = m1[ 1] * m2[ 0] + m1[ 5] * m2[ 1] + m1[ 9] * m2[ 2];
	out[ 2] = m1[ 2] * m2[ 0] + m1[ 6] * m2[ 1] + m1[10] * m2[ 2];
	out[ 3] = 0.0f;
	out[ 4] = m1[ 0] * m2[ 4] + m1[ 4] * m2[ 5] + m1[ 8] * m2[ 6];
	out[ 5] = m1[ 1] * m2[ 4] + m1[ 5] * m2[ 5] + m1[ 9] * m2[ 6];
	out[ 6] = m1[ 2] * m2[ 4] + m1[ 6] * m2[ 5] + m1[10] * m2[ 6];
	out[ 7] = 0.0f;
	out[ 8] = m1[ 0] * m2[ 8] + m1[ 4] * m2[ 9] + m1[ 8] * m2[10];
	out[ 9] = m1[ 1] * m2[ 8] + m1[ 5] * m2[ 9] + m1[ 9] * m2[10];
	out[10] = m1[ 2] * m2[ 8] + m1[ 6] * m2[ 9] + m1[10] * m2[10];
	out[11] = 0.0f;
	out[12] = m1[ 0] * m2[12] + m1[ 4] * m2[13] + m1[ 8] * m2[14] + m1[12];
	out[13] = m1[ 1] * m2[12] + m1[ 5] * m2[13] + m1[ 9] * m2[14] + m1[13];
	out[14] = m1[ 2] * m2[12] + m1[ 6] * m2[13] + m1[10] * m2[14] + m1[14];
	out[15] = 1.0f;
}

/*
 ==================
 Matrix4_Identity
 ==================
*/
void Matrix4_Identity (mat4_t m){

	m[ 0] = 1.0f;
	m[ 1] = 0.0f;
	m[ 2] = 0.0f;
	m[ 3] = 0.0f;
	m[ 4] = 0.0f;
	m[ 5] = 1.0f;
	m[ 6] = 0.0f;
	m[ 7] = 0.0f;
	m[ 8] = 0.0f;
	m[ 9] = 0.0f;
	m[10] = 1.0f;
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[15] = 1.0f;
}

/*
 ==================
 Matrix4_Rotate
 ==================
*/
void Matrix4_Rotate (mat4_t m, float angle, float x, float y, float z){

	mat4_t	rotation, tmp;
	float	mag, rad, s, c, i;
	float	xx, yy, zz, xy, yz, zx, xs, ys, zs;

	mag = sqrt(x*x + y*y + z*z);
	if (mag == 0.0f)
		return;

	mag = 1.0f / mag;

	x *= mag;
	y *= mag;
	z *= mag;

	rad = DEG2RAD(angle);
	s = sin(rad);
	c = cos(rad);

	i = 1.0f - c;

	xx = (x * x) * i;
	yy = (y * y) * i;
	zz = (z * z) * i;
	xy = (x * y) * i;
	yz = (y * z) * i;
	zx = (z * x) * i;

	xs = x * s;
	ys = y * s;
	zs = z * s;

	rotation[ 0] = xx + c;
	rotation[ 1] = xy + zs;
	rotation[ 2] = zx - ys;
	rotation[ 3] = 0.0f;
	rotation[ 4] = xy - zs;
	rotation[ 5] = yy + c;
	rotation[ 6] = yz + xs;
	rotation[ 7] = 0.0f;
	rotation[ 8] = zx + ys;
	rotation[ 9] = yz - xs;
	rotation[10] = zz + c;
	rotation[11] = 0.0f;
	rotation[12] = 0.0f;
	rotation[13] = 0.0f;
	rotation[14] = 0.0f;
	rotation[15] = 1.0f;

	Matrix4_Copy(m, tmp);
	Matrix4_MultiplyFast(tmp, rotation, m);
}

/*
 ==================
 Matrix4_Scale
 ==================
*/
void Matrix4_Scale (mat4_t m, float x, float y, float z){

	m[ 0] *= x;
	m[ 1] *= x;
	m[ 2] *= x;
	m[ 3] *= x;
	m[ 4] *= y;
	m[ 5] *= y;
	m[ 6] *= y;
	m[ 7] *= y;
	m[ 8] *= z;
	m[ 9] *= z;
	m[10] *= z;
	m[11] *= z;
}

/*
 ==================
 Matrix4_Translate
 ==================
*/
void Matrix4_Translate (mat4_t m, float x, float y, float z){

	m[12] = m[ 0] * x + m[ 4] * y + m[ 8] * z + m[12];
	m[13] = m[ 1] * x + m[ 5] * y + m[ 9] * z + m[13];
	m[14] = m[ 2] * x + m[ 6] * y + m[10] * z + m[14];
	m[15] = m[ 3] * x + m[ 7] * y + m[11] * z + m[15];
}

/*
 ==================
 Matrix4_Shear
 ==================
*/
void Matrix4_Shear (mat4_t m, float x, float y, float z){

	vec4_t	mx, my, mz;

	mx[0] = m[ 0] * x;
	mx[1] = m[ 1] * x;
	mx[2] = m[ 2] * x;
	mx[3] = m[ 3] * x;
	my[0] = m[ 4] * y;
	my[1] = m[ 5] * y;
	my[2] = m[ 6] * y;
	my[3] = m[ 7] * y;
	mz[0] = m[ 8] * z;
	mz[1] = m[ 9] * z;
	mz[2] = m[10] * z;
	mz[3] = m[11] * z;

	m[ 0] += my[0] + mz[0];
	m[ 1] += my[1] + mz[1];
	m[ 2] += my[2] + mz[2];
	m[ 3] += my[3] + mz[3];
	m[ 4] += mx[0] + mz[0];
	m[ 5] += mx[1] + mz[1];
	m[ 6] += mx[2] + mz[2];
	m[ 7] += mx[3] + mz[3];
	m[ 8] += mx[0] + my[0];
	m[ 9] += mx[1] + my[1];
	m[10] += mx[2] + my[2];
	m[11] += mx[3] + my[3];
}