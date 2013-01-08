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
// vector.c - Vector math
//

// TODO:
// - rename to Vector3_* like matrix


#include "../../common/common.h"


vec3_t						vec3_origin = {0.0f, 0.0f, 0.0f};


/*
 ==============================================================================

 3D VECTOR

 ==============================================================================
*/


/*
 ==================
 ProjectPointOnPlane
 ==================
*/
void ProjectPointOnPlane (vec3_t dst, const vec3_t p, const vec3_t normal){

	float	d, invDenom;

	invDenom = 1.0f / DotProduct(normal, normal);

	d = DotProduct(normal, p) * invDenom;

	dst[0] = p[0] - d * (normal[0] * invDenom);
	dst[1] = p[1] - d * (normal[1] * invDenom);
	dst[2] = p[2] - d * (normal[2] * invDenom);
}

/*
 ==================
 VectorToAngles
 ==================
*/
void VectorToAngles (const vec3_t vec, vec3_t angles){

	float	forward;
	float	pitch, yaw;

	if (vec[1] == 0.0f && vec[0] == 0.0f){
		yaw = 0.0f;

		if (vec[2] > 0.0f)
			pitch = 90.0f;
		else
			pitch = 270.0f;
	}
	else {
		if (vec[0])
			yaw = RAD2DEG(atan2(vec[1], vec[0]));
		else if (vec[1] > 0.0f)
			yaw = 90.0f;
		else
			yaw = 270.0f;

		if (yaw < 0.0f)
			yaw += 360.0f;

		forward = sqrt(vec[0]*vec[0] + vec[1]*vec[1]);

		pitch = RAD2DEG(atan2(vec[2], forward));
		if (pitch < 0.0f)
			pitch += 360.0f;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0.0f;
}

/*
 ==================
 
 ==================
*/
void VectorToMatrix (const vec3_t vec, vec3_t matrix[3]){

}

/*
 ==================
 DotProduct
 ==================
*/
vec_t DotProduct (const vec3_t v1, const vec3_t v2){

	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

/*
 ==================
 CrossProduct
 ==================
*/
void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross){

	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

/*
 ==================
 VectorLerpFast
 ==================
*/
void VectorLerpFast (const vec3_t from, const vec3_t to, const float frac, vec3_t out){

	out[0] = from[0] + (to[0] - from[0]) * frac;
	out[1] = from[1] + (to[1] - from[1]) * frac;
	out[2] = from[2] + (to[2] - from[2]) * frac;
}

/*
 ==================
 Distance
 ==================
*/
vec_t Distance (const vec3_t v1, const vec3_t v2){

	vec3_t	v;

	v[0] = v2[0] - v1[0];
	v[1] = v2[1] - v1[1];
	v[2] = v2[2] - v1[2];

	return Sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

/*
 ==================
 DistanceFast
 ==================
*/
vec_t DistanceFast (const vec3_t v1, const vec3_t v2){

	vec3_t	v;

	v[0] = v2[0] - v1[0];
	v[1] = v2[1] - v1[1];
	v[2] = v2[2] - v1[2];

	return SqrtFast(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

/*
 ==================
 DistanceSquared
 ==================
*/
vec_t DistanceSquared (const vec3_t v1, const vec3_t v2){

	vec3_t	v;

	v[0] = v2[0] - v1[0];
	v[1] = v2[1] - v1[1];
	v[2] = v2[2] - v1[2];

	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

/*
 ==================
 SnapVector
 ==================
*/
void SnapVector (vec3_t v){

	v[0] = (int)v[0];
	v[1] = (int)v[1];
	v[2] = (int)v[2];
}

/*
 ==================
 VectorCopy
 ==================
*/
void VectorCopy (const vec3_t in, vec3_t out){

	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

/*
 ==================
 VectorSet
 ==================
*/
void VectorSet (vec3_t v, float x, float y, float z){

	v[0] = x;
	v[1] = y;
	v[2] = z;
}

/*
 ==================
 VectorClear
 ==================
*/
void VectorClear (vec3_t v){

	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 0.0f;
}

/*
 ==================
 VectorIsCleared
 ==================
*/
bool VectorIsCleared (vec3_t v){

	if (v[0] != 0.0f || v[1] != 0.0f || v[2] != 0.0f)
		return false;

	return true;
}

/*
 ==================
 VectorCompare
 ==================
*/
bool VectorCompare (const vec3_t v1, const vec3_t v2){

	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
		return false;
			
	return true;
}

/*
 ==================
 VectorCompareEpsilon
 ==================
*/
bool VectorCompareEpsilon (const vec3_t v1, const vec3_t v2, const float epsilon){

	if (FAbs(v1[0] - v2[0]) > epsilon)
		return false;
	if (FAbs(v1[1] - v2[1]) > epsilon)
		return false;
	if (FAbs(v1[2] - v2[2]) > epsilon)
		return false;
			
	return true;
}

/*
 ==================
 VectorAdd
 ==================
*/
void VectorAdd (const vec3_t v1, const vec3_t v2, vec3_t out){

	out[0] = v1[0] + v2[0];
	out[1] = v1[1] + v2[1];
	out[2] = v1[2] + v2[2];
}

/*
 ==================
 VectorSubtract
 ==================
*/
void VectorSubtract (const vec3_t v1, const vec3_t v2, vec3_t out){

	out[0] = v1[0] - v2[0];
	out[1] = v1[1] - v2[1];
	out[2] = v1[2] - v2[2];
}

/*
 ==================
 VectorScale
 ==================
*/
void VectorScale (const vec3_t v, float scale, vec3_t out){

	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
}

/*
 ==================
 VectorMultiply
 ==================
*/
void VectorMultiply (const vec3_t v1, const vec3_t v2, vec3_t out){

	out[0] = v1[0] * v2[0];
	out[1] = v1[1] * v2[1];
	out[2] = v1[2] * v2[2];
}

/*
 ==================
 VectorMA
 ==================
*/
void VectorMA (const vec3_t v1, float scale, const vec3_t v2, vec3_t out){

	out[0] = v1[0] + v2[0] * scale;
	out[1] = v1[1] + v2[1] * scale;
	out[2] = v1[2] + v2[2] * scale;
}

/*
 ==================
 VectorSA
 ==================
*/
void VectorSA (const vec3_t v1, float scale, const vec3_t v2, vec3_t out){

	out[0] = v1[0] - v2[0] * scale;
	out[1] = v1[1] - v2[1] * scale;
	out[2] = v1[2] - v2[2] * scale;
}

/*
 ==================
 VectorAverage
 ==================
*/
void VectorAverage (const vec3_t v1, const vec3_t v2, vec3_t out){

	out[0] = (v1[0] + v2[0]) * 0.5f;
	out[1] = (v1[1] + v2[1]) * 0.5f;
	out[2] = (v1[2] + v2[2]) * 0.5f;
}

/*
 ==================
 VectorClamp
 ==================
*/
void VectorClamp (const float min, const float max, vec3_t out){

	out[0] = (out[0] < min) ? min : (out[0] > max) ? max : out[0];
	out[1] = (out[1] < min) ? min : (out[1] > max) ? max : out[1];
	out[2] = (out[2] < min) ? min : (out[2] > max) ? max : out[2];
}

/*
 ==================
 VectorMin
 ==================
*/
void VectorMin (const vec3_t v1, const vec3_t v2, vec3_t out){

	out[0] = (v1[0] < v2[0]) ? v1[0] : v2[0];
	out[1] = (v1[1] < v2[1]) ? v1[1] : v2[1];
	out[2] = (v1[2] < v2[2]) ? v1[2] : v2[2];
}

/*
 ==================
 VectorMax
 ==================
*/
void VectorMax (const vec3_t v1, const vec3_t v2, vec3_t out){

	out[0] = (v1[0] > v2[0]) ? v1[0] : v2[0];
	out[1] = (v1[1] > v2[1]) ? v1[1] : v2[1];
	out[2] = (v1[2] > v2[2]) ? v1[2] : v2[2];
}

/*
 ==================
 VectorNegate
 ==================
*/
void VectorNegate (const vec3_t v, vec3_t out){

	out[0] = -v[0];
	out[1] = -v[1];
	out[2] = -v[2];
}

/*
 ==================
 VectorInverse
 ==================
*/
void VectorInverse (vec3_t v){

	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

/*
 ==================
 VectorLength
 ==================
*/
vec_t VectorLength (const vec3_t v){

	return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

/*
 ==================
 VectorLengthFast
 ==================
*/
vec_t VectorLengthFast (const vec3_t v){

	return SqrtFast(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

/*
 ==================
 VectorLengthSquared
 ==================
*/
vec_t VectorLengthSquared (const vec3_t v){

	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

/*
 ==================
 VectorLerp
 ==================
*/
void VectorLerp (const vec3_t from, const vec3_t to, float frac, vec3_t out){

	out[0] = from[0] + (to[0] - from[0]) * frac;
	out[1] = from[1] + (to[1] - from[1]) * frac;
	out[2] = from[2] + (to[2] - from[2]) * frac;
}

/*
 ==================
 VectorRotate
 ==================
*/
void VectorRotate (const vec3_t v, const vec3_t matrix[3], vec3_t out){

	out[0] = v[0] * matrix[0][0] + v[1] * matrix[0][1] + v[2] * matrix[0][2];
	out[1] = v[0] * matrix[1][0] + v[1] * matrix[1][1] + v[2] * matrix[1][2];
	out[2] = v[0] * matrix[2][0] + v[1] * matrix[2][1] + v[2] * matrix[2][2];
}

/*
 ==================
 VectorReflect
 ==================
*/
void VectorReflect (const vec3_t v, const vec3_t normal, vec3_t out){

	float	d;

	d = 2.0f * (v[0] * normal[0] + v[1] * normal[1] + v[2] * normal[2]);

	out[0] = v[0] - normal[0] * d;
	out[1] = v[1] - normal[1] * d;
	out[2] = v[2] - normal[2] * d;
}

/*
 ==================
 VectorNormalize
 ==================
*/
vec_t VectorNormalize (vec3_t v){

	float	length, invLength;

	length = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

	if (length){
		invLength = 1.0f / length;

		v[0] *= invLength;
		v[1] *= invLength;
		v[2] *= invLength;
	}
	
	return length;
}

/*
 ==================
 VectorNormalize2
 ==================
*/
vec_t VectorNormalize2 (const vec3_t v, vec3_t out){

	float	length, invLength;

	length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

	if (length){
		invLength = 1.0f / length;

		out[0] = v[0] * invLength;
		out[1] = v[1] * invLength;
		out[2] = v[2] * invLength;
	}
	else {
		out[0] = 0.0f;
		out[1] = 0.0f;
		out[2] = 0.0f;
	}

	return length;
}

/*
 ==================
 VectorNormalizeFast

 Fast vector normalize routine that does not check to make sure that
 length != 0, nor does it return length. Uses rsqrt approximation.
 ==================
*/
void VectorNormalizeFast (vec3_t v){

	float	invLength;

	invLength = Q_rsqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

	v[0] *= invLength;
	v[1] *= invLength;
	v[2] *= invLength;
}


/*
 ==============================================================================

 4D VECTOR

 ==============================================================================
*/


/*
 ==================
 Vector4Copy
 ==================
*/
void Vector4Copy (const vec4_t in, vec4_t out){

	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
}