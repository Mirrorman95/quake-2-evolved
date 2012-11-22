/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#include "q_shared.h"


vec3_t	byteDirs[NUM_VERTEX_NORMALS] = {
#include "anorms.h"
};


float Tan (float x){

	return tanf(x);
}

float ATan (float y, float x){

	return atan2f(y, x);
}

/*
 =================
 Square
 =================
*/
int Square (int x){

	return x * x;
}

// =====================================================================


/*
 =================
 DirToByte

 This isn't a real cheap function to call!
 =================
*/
int DirToByte (const vec3_t dir){

	int		i, best = 0;
	float	d, bestd = 0;

	if (!dir)
		return 0;

	for (i = 0; i < NUM_VERTEX_NORMALS; i++){
		d = DotProduct(dir, byteDirs[i]);
		if (d > bestd){
			bestd = d;
			best = i;
		}
	}

	return best;
}

/*
 =================
 ByteToDir
 =================
*/
void ByteToDir (int b, vec3_t dir){

	if (b < 0 || b >= NUM_VERTEX_NORMALS){
		VectorClear(dir);
		return;
	}

	VectorCopy(byteDirs[b], dir);
}


// =====================================================================


/*
 =================
 Q_rsqrt
 =================
*/
float Q_rsqrt (float number){

	int		i;
	float	x, y;

	x = number * 0.5f;
	i = *(int *)&number;			// Evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);		// What the fuck?
	y = *(float *)&i;
	y = y * (1.5f - (x * y * y));	// First iteration

	return y;
}

/*
 =================
 Q_log2
 =================
*/
int Q_log2 (int val){

	int	answer = 0;
	
	while (val >>= 1)
		answer++;

	return answer;
}


// =====================================================================


/*
 =================
 RotatePointAroundVector
 =================
*/
void RotatePointAroundVector (vec3_t dst, const vec3_t dir, const vec3_t point, float degrees){

	vec3_t	mat[3], tmp[3], rot[3];
	float	rad, s, c;

	VectorCopy(dir, mat[2]);
	MakeNormalVectors(mat[2], mat[0], mat[1]);

	rad = DEG2RAD(degrees);
	s = sin(rad);
	c = cos(rad);

	tmp[0][0] = mat[0][0] * c + mat[1][0] * -s;
	tmp[0][1] = mat[0][0] * s + mat[1][0] * c;
	tmp[0][2] = mat[2][0];
	tmp[1][0] = mat[0][1] * c + mat[1][1] * -s;
	tmp[1][1] = mat[0][1] * s + mat[1][1] * c;
	tmp[1][2] = mat[2][1];
	tmp[2][0] = mat[0][2] * c + mat[1][2] * -s;
	tmp[2][1] = mat[0][2] * s + mat[1][2] * c;
	tmp[2][2] = mat[2][2];

	Matrix3_Multiply(tmp, mat, rot);

	VectorRotate(point, rot, dst);
}

/*
 =================
 NormalToLatLong
 =================
*/
void NormalToLatLong (const vec3_t normal, byte bytes[2]){

	int		lat, lng;

	if (normal[0] == 0 && normal[1] == 0){
		if (normal[2] > 0){
			// Lattitude = 0, Longitude = 0
			bytes[0] = 0;
			bytes[1] = 0;
		}
		else {
			// Lattitude = 0, Longitude = 128
			bytes[0] = 128;
			bytes[1] = 0;
		}
	}
	else {
		lat = RAD2DEG(atan2(normal[1], normal[0])) * (255.0 / 360.0);
		lng = RAD2DEG(acos(normal[2])) * (255.0 / 360.0);

		bytes[0] = lng & 0xff;
		bytes[1] = lat & 0xff;
	}
}

/*
 =================
 PerpendicularVector

 Assumes "src" is normalized
 =================
*/
void PerpendicularVector (vec3_t dst, const vec3_t src){

	int		i, pos = 0;
	float	val, min = 1.0;
	vec3_t	tmp;

	// Find the smallest magnitude axially aligned vector
	for (i = 0; i < 3; i++){
		val = fabs(src[i]);
		if (val < min){
			min = val;
			pos = i;
		}
	}

	VectorClear(tmp);
	tmp[pos] = 1.0;

	// Project the point onto the plane defined by src
	ProjectPointOnPlane(dst, tmp, src);

	// Normalize the result
	VectorNormalize(dst);
}

/*
 =================
 MakeNormalVectors

 Given a normalized forward vector, create two other perpendicular
 vectors
 =================
*/
void MakeNormalVectors (const vec3_t forward, vec3_t right, vec3_t up){

	float	d;

	// This rotate and negate guarantees a vector not colinear with the 
	// original
	right[0] = forward[2];
	right[1] = -forward[0];
	right[2] = forward[1];

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize(right);
	CrossProduct(right, forward, up);
}