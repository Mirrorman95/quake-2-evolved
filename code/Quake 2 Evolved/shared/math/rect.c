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
// rect.c - Rectangle math
//


#include "../../common/common.h"


/*
 ==================
 RectToBounds
 ==================
*/
void RectToBounds (rect_t rect, vec3_t mins, vec3_t maxs){

	mins[0] = rect.x;
	mins[1] = rect.y;
	mins[2] = 0.0f;
	maxs[0] = rect.width;
	maxs[1] = rect.height;
	maxs[2] = 1.0f;
}

/*
 ==================
 RectFromBounds
 ==================
*/
rect_t RectFromBounds (const vec3_t mins, const vec3_t maxs){

	rect_t rect;

	rect.x = FloatToShort(mins[0]);
	rect.y = FloatToShort(mins[1]);
	rect.width = FloatToShort(maxs[0]);
	rect.height = FloatToShort(maxs[1]);

	return rect;
}