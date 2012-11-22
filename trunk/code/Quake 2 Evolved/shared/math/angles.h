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
// angles.h - Angles math
//


#ifndef __MATH_ANGLES_H__
#define __MATH_ANGLES_H__


// Angle indices
#define	PITCH						0		// Up / Down
#define	YAW							1		// Left / Right
#define	ROLL						2		// Fall over

void			AngleToVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void			AnglesToMat3 (const vec3_t angles, vec3_t axis[3]);
float			AngleMod (float angle);
float			LerpAngle (float from, float to, float frac);
void			LerpAngles (const vec3_t from, const vec3_t to, float frac, vec3_t out);


#endif	// __MATH_ANGLES_H__