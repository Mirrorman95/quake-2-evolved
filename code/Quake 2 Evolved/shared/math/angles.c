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
// angles.c - Angles math
//


#include "../../common/common.h"


/*
 ==================
 AngleToVectors
 ==================
*/
void AngleToVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up){

	static float	sp, sy, sr, cp, cy, cr;
	float			angle;

	angle = DEG2RAD(angles[PITCH]);
	sp = sin(angle);
	cp = cos(angle);
	angle = DEG2RAD(angles[YAW]);
	sy = sin(angle);
	cy = cos(angle);
	angle = DEG2RAD(angles[ROLL]);
	sr = sin(angle);
	cr = cos(angle);

	if (forward){
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}

	if (right){
		right[0] = -1 * sr * sp * cy + -1 * cr * -sy;
		right[1] = -1 * sr * sp * sy + -1 * cr * cy;
		right[2] = -1 * sr * cp;
	}

	if (up){
		up[0] = cr * sp * cy + -sr * -sy;
		up[1] = cr * sp * sy + -sr * cy;
		up[2] = cr * cp;
	}
}

/*
 ==================
 AnglesToMat3
 ==================
*/
void AnglesToMat3 (const vec3_t angles, vec3_t axis[3]){

	static float	sp, sy, sr, cp, cy, cr;
	float			angle;

	angle = DEG2RAD(angles[PITCH]);
	sp = sin(angle);
	cp = cos(angle);
	angle = DEG2RAD(angles[YAW]);
	sy = sin(angle);
	cy = cos(angle);
	angle = DEG2RAD(angles[ROLL]);
	sr = sin(angle);
	cr = cos(angle);

	axis[0][0] = cp * cy;
	axis[0][1] = cp * sy;
	axis[0][2] = -sp;
	axis[1][0] = sr * sp * cy + cr * -sy;
	axis[1][1] = sr * sp * sy + cr * cy;
	axis[1][2] = sr * cp;
	axis[2][0] = cr * sp * cy + -sr * -sy;
	axis[2][1] = cr * sp * sy + -sr * cy;
	axis[2][2] = cr * cp;
}

/*
 ==================
 AngleMod
 ==================
*/
float AngleMod (float angle){

	return (360.0f/65536) * ((int)(angle * (65536/360.0f)) & 65535);
}

/*
 ==================
 LerpAngle
 ==================
*/
float LerpAngle (float from, float to, float frac){

	if (to - from > 180.0f)
		to -= 360.0f;
	if (to - from < -180.0f)
		to += 360.0f;

	return from + (to - from) * frac;
}

/*
 ==================
 LerpAngles
 ==================
*/
void LerpAngles (const vec3_t from, const vec3_t to, float frac, vec3_t out){

	int		i;

	for (i = 0; i < 3; i++){
		if (to[i] - from[i] > 180.0f){
			out[i] = from[i] + ((to[i] - 360.0f) - from[i]) * frac;
			continue;
		}
		if (to[i] - from[i] < -180.0f){
			out[i] = from[i] + ((to[i] + 360.0f) - from[i]) * frac;
			continue;
		}

		out[i] = from[i] + (to[i] - from[i]) * frac;
	}
}

/*
 ==================
 AnglesNormalize360
 ==================
*/
void AnglesNormalize360 (float pitch, float yaw, float roll){

	if (pitch < 0.0f || pitch >= 360.0f)
		pitch -= 360.0f * Floor(pitch * (1.0f / 360.0f));

	if (yaw < 0.0f || yaw >= 360.0f)
		yaw -= 360.0f * Floor(yaw * (1.0f / 360.0f));

	if (roll < 0.0f || roll >= 360.0f)
		roll -= 360.0f * Floor(roll * (1.0f / 360.0f));
}