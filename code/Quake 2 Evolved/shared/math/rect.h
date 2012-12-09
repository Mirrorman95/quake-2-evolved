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
// rect.h - Rectangle math
//


#ifndef __MATH_RECT_H__
#define __MATH_RECT_H__


typedef struct {
	short					x;
	short					y;
	short					width;
	short					height;
} rect_t;

INLINE bool RectIsCleared (rect_t rect){

	if (rect.x <= rect.width || rect.y <= rect.height)
		return false;

	return true;
}

INLINE void RectExpandSelf (rect_t rect, const short size){

	rect.x -= size;
	rect.y -= size;
	rect.width += size;
	rect.height += size;
}

INLINE void RectClip (rect_t rect, short x, short y, short width, short height){

	x = Max(x, rect.x);
	y = Max(y, rect.y);
	width = Min(width, rect.width);
	height = Min(height, rect.height);
}

INLINE int RectSize (rect_t rect){

	if (rect.x > rect.width || rect.y > rect.height)
		return 0;

	return (rect.width - rect.x) * (rect.height - rect.y);
}

void RectToBounds (rect_t rect, vec3_t mins, vec3_t maxs);
void RectFromBounds (rect_t rect, const vec3_t mins, const vec3_t maxs);


#endif	// __MATH_RECT_H__