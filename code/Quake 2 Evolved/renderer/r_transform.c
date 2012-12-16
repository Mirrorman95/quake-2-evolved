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
// r_transform.c - Transforming utilities
//


#include "r_local.h"


/*
 ==================
 R_LocalPointToWorld
 ==================
*/
void R_LocalPointToWorld (const vec3_t in, vec3_t out, const vec3_t origin, const vec3_t axis[3]){

	vec3_t	tmp;

	tmp[0] = in[0] * axis[0][0] + in[1] * axis[1][0] + in[2] * axis[2][0];
	tmp[1] = in[0] * axis[0][1] + in[1] * axis[1][1] + in[2] * axis[2][1];
	tmp[2] = in[0] * axis[0][2] + in[1] * axis[1][2] + in[2] * axis[2][2];

	out[0] = tmp[0] + origin[0];
	out[1] = tmp[1] + origin[1];
	out[2] = tmp[2] + origin[2];
}

/*
 ==================
 R_WorldPointToLocal
 ==================
*/
void R_WorldPointToLocal (const vec3_t in, vec3_t out, const vec3_t origin, const vec3_t axis[3]){

	vec3_t	tmp;

	tmp[0] = in[0] - origin[0];
	tmp[1] = in[1] - origin[1];
	tmp[2] = in[2] - origin[2];

	out[0] = tmp[0] * axis[0][0] + tmp[1] * axis[0][1] + tmp[2] * axis[0][2];
	out[1] = tmp[0] * axis[1][0] + tmp[1] * axis[1][1] + tmp[2] * axis[1][2];
	out[2] = tmp[0] * axis[2][0] + tmp[1] * axis[2][1] + tmp[2] * axis[2][2];
}

/*
 ==================
 R_WorldVectorToLocal
 ==================
*/
void R_WorldVectorToLocal (const vec3_t in, vec3_t out, const vec3_t axis[3]){

	out[0] = in[0] * axis[0][0] + in[1] * axis[0][1] + in[2] * axis[0][2];
	out[1] = in[0] * axis[1][0] + in[1] * axis[1][1] + in[2] * axis[1][2];
	out[2] = in[0] * axis[2][0] + in[1] * axis[2][1] + in[2] * axis[2][2];
}

/*
 ==================
 R_WorldAxisToLocal
 ==================
*/
void R_WorldAxisToLocal (const vec3_t in[3], vec3_t out[3], const vec3_t axis[3]){

	out[0][0] = in[0][0] * axis[0][0] + in[0][1] * axis[0][1] + in[0][2] * axis[0][2];
	out[0][1] = in[0][0] * axis[1][0] + in[0][1] * axis[1][1] + in[0][2] * axis[1][2];
	out[0][2] = in[0][0] * axis[2][0] + in[0][1] * axis[2][1] + in[0][2] * axis[2][2];
	out[1][0] = in[1][0] * axis[0][0] + in[1][1] * axis[0][1] + in[1][2] * axis[0][2];
	out[1][1] = in[1][0] * axis[1][0] + in[1][1] * axis[1][1] + in[1][2] * axis[1][2];
	out[1][2] = in[1][0] * axis[2][0] + in[1][1] * axis[2][1] + in[1][2] * axis[2][2];
	out[2][0] = in[2][0] * axis[0][0] + in[2][1] * axis[0][1] + in[2][2] * axis[0][2];
	out[2][1] = in[2][0] * axis[1][0] + in[2][1] * axis[1][1] + in[2][2] * axis[1][2];
	out[2][2] = in[2][0] * axis[2][0] + in[2][1] * axis[2][1] + in[2][2] * axis[2][2];
}


// ============================================================================


/*
 ==================
 R_TransformWorldToDevice
 ==================
*/
void R_TransformWorldToDevice (const vec3_t world, vec3_t ndc, mat4_t modelviewProjectionMatrix[4]){

	vec4_t	clip;
	float	scale;

	clip[0] = world[0] * modelviewProjectionMatrix[0][0] + world[1] * modelviewProjectionMatrix[1][0] + world[2] * modelviewProjectionMatrix[2][0] + modelviewProjectionMatrix[3][0];
	clip[1] = world[0] * modelviewProjectionMatrix[0][1] + world[1] * modelviewProjectionMatrix[1][1] + world[2] * modelviewProjectionMatrix[2][1] + modelviewProjectionMatrix[3][1];
	clip[2] = world[0] * modelviewProjectionMatrix[0][2] + world[1] * modelviewProjectionMatrix[1][2] + world[2] * modelviewProjectionMatrix[2][2] + modelviewProjectionMatrix[3][2];
	clip[3] = world[0] * modelviewProjectionMatrix[0][3] + world[1] * modelviewProjectionMatrix[1][3] + world[2] * modelviewProjectionMatrix[2][3] + modelviewProjectionMatrix[3][3];

	scale = 1.0f / clip[3];

	ndc[0] = clip[0] * scale;
	ndc[1] = clip[1] * scale;
	ndc[2] = clip[2] * scale;
}

/*
 ==================
 R_TransformDeviceToScreen
 ==================
*/
void R_TransformDeviceToScreen (const vec3_t ndc, vec3_t screen, const rect_t viewport){

	screen[0] = (ndc[0] * 0.5f + 0.5f) * (viewport.width - viewport.x) + viewport.x;
	screen[1] = (ndc[1] * 0.5f + 0.5f) * (viewport.height - viewport.y) + viewport.y;
	screen[2] = (ndc[2] * 0.5f + 0.5f);
}