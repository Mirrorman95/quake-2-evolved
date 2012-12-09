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
// math.c - Primary math file
//


#include "../../common/common.h"


int ClampInt (int value, int min, int max){

	if (value < min)
		return min;

	if (value > max)
		return max;

	return value;
}

/*
 ==================
 SinCos
 ==================
*/
void SinCos (float x, float s, float c){

#ifdef _MSC_VER

	__asm {
		fld			x
		mov			ecx, s
		mov			edx, c
		fsincos
		fstp		DWORD PTR [edx]
		fstp		DWORD PTR [ecx]
	}

#else

	s = sinf(x);
	c = cosf(x);

#endif
}

/*
 ==================
 Floor
 ==================
*/
float Floor (float x){

	return floorf(x);
}

/*
 ==================
 Frac
 ==================
*/
float Frac (float x){

	return x - floorf(x);
}

/*
 ==================
 Pow
 ==================
*/
float Pow (float x, float y){

	return powf(x, y);
}

/*
 ==================
 FMod
 ==================
*/
float FMod (float x, float y){

	return fmodf(x, y);
}

/*
 ==================
 IsPowerOfTwo
 ==================
*/
bool IsPowerOfTwo (int i){

	return (i > 0 && !(i & (i - 1)));
}

/*
 ==================
 FloorPowerOfTwo
 ==================
*/
int FloorPowerOfTwo (int i){

	i |= (i >> 1);
	i |= (i >> 2);
	i |= (i >> 4);
	i |= (i >> 8);
	i |= (i >> 16);

	i -= (i >> 1);

	return i + (i == 0);
}

/*
 ==================
 CeilPowerOfTwo
 ==================
*/
int CeilPowerOfTwo (int i){

	i--;
	i |= (i >> 1);
	i |= (i >> 2);
	i |= (i >> 4);
	i |= (i >> 8);
	i |= (i >> 16);
	i++;

	return i + (i == 0);
}

/*
 ==================
 LerpFast
 ==================
*/
float LerpFast (float from, float to, float frac){

	return from + (to - from) * frac;
}


/*
 ==============================================================================

 CONVERTION FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 FloatToByte
 ==================
*/
byte FloatToByte (float f){

#if defined SIMD_X86

	__m128	xmm;
	int		i;

	xmm = _mm_load_ss(&f);
	xmm = _mm_max_ss(xmm, _mm_set_ss(0.0f));
	xmm = _mm_min_ss(xmm, _mm_set_ss(255.0f));
	i = _mm_cvtt_ss2si(xmm);

	return i;

#else

	int		i;

	i = (int)f;

	if (i < 0)
		return 0;

	if (i > 255)
		return 255;

	return i;

#endif
}

/*
 ==================
 FloatToInt
 ==================
*/
int FloatToInt (float f){

#if defined SIMD_X86

	__m128	xmm;
	int		i;

	xmm = _mm_load_ss(&f);
	i = _mm_cvtt_ss2si(xmm);

	return i;

#else

	return (int)f;

#endif
}