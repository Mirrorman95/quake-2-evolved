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
// math.h - Primary math file
//


#ifndef __MATH_MATH_H__
#define __MATH_MATH_H__


#define M_E							2.71828182845904523536f
#define M_PI						3.14159265358979323846f
#define M_PI_TWO					6.28318530717958647692f
#define M_PI_HALF					1.57079632679489661923f
#define M_SQRT_TWO					1.41421356237309504880f
#define M_SQRT_THREE				1.73205080756887729352f
#define M_SQRT_1OVER2				0.70710678118654752440f
#define M_SQRT_1OVER3				0.57735026918962576450f
#define M_EPSILON					1.192092896e-07f
#define M_INFINITY					1e30f

#define MS2SEC(t)					((t) * 0.001f)
#define SEC2MS(t)					((t) * 1000.0f)

#define DEG2RAD(a)					((a) * M_PI / 180.0f)
#define RAD2DEG(a)					((a) * 180.0f / M_PI)

#define SqrtFast(x)					((x) * Q_rsqrt(x))

#define	frand()						((rand() & 0x7FFF) * (1.0/0x7FFF))		// 0 to 1
#define crand()						((rand() & 0x7FFF) * (2.0/0x7FFF) - 1)	// -1 to 1

#ifndef Min
#define Min(a,b)					(((a) < (b)) ? (a) : (b))
#endif
#ifndef Max
#define Max(a,b)					(((a) > (b)) ? (a) : (b))
#endif

#define Clamp(a,b,c)				(((a) < (b)) ? (b) : ((a) > (c)) ? (c) : (a))

#define MakeRGBA(c,r,g,b,a)			((c)[0]=(r),(c)[1]=(g),(c)[2]=(b),(c)[3]=(a))

int				ClampInt (int value, int min, int max);
void			SinCos (float x, float s, float c);
float			Floor (float x);
float			Frac (float x);
float			Pow (float x, float y);
float			FMod (float x, float y);
bool			IsPowerOfTwo (int i);
int				FloorPowerOfTwo (int i);
int				CeilPowerOfTwo (int i);
float			LerpFast (float from, float to, float frac);

byte			FloatToByte (float f);
int				FloatToInt (float f);


#endif	// __MATH_MATH_H__