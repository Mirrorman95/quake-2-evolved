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
// color.h - Color math
//


#ifndef __MATH_COLOR_H__
#define __MATH_COLOR_H__


#define COLOR_MASK					63

// Color indices
#define COLOR_DEFAULT				52

#define COLOR_BLACK					0
#define COLOR_RED					1
#define COLOR_GREEN					2
#define COLOR_BLUE					3
#define COLOR_YELLOW				4
#define COLOR_CYAN					5
#define COLOR_MAGENTA				6
#define COLOR_WHITE					7

// Basic colors
extern vec4_t				colorDefault;

extern vec4_t				colorBlack;
extern vec4_t				colorRed;
extern vec4_t				colorGreen;
extern vec4_t				colorBlue;
extern vec4_t				colorYellow;
extern vec4_t				colorCyan;
extern vec4_t				colorMagenta;
extern vec4_t				colorWhite;
extern vec4_t				colorOrange;
extern vec4_t				colorChartreuse;
extern vec4_t				colorSpringGreen;
extern vec4_t				colorAzure;
extern vec4_t				colorViolet;
extern vec4_t				colorRose;
extern vec4_t				colorGray;

// Color table
extern vec4_t				color_table[64];

typedef byte				color_t[4];

void			ColorPackL (byte color[4], const float l);
void			ColorPackLA (byte color[4], const float l, const float a);
void			ColorPackFloatRGB (byte color[4], const float r, const float g, const float b);
void			ColorPackFloatRGBA (byte color[4], const float r, const float g, const float b, const float a);
void			ColorPackVectorRGB (byte color[4], const vec3_t rgb);
void			ColorPackVectorRGBA (byte color[4], const vec4_t rgba);

uint			ColorNormalize (vec3_t rgb);


#endif	// __MATH_COLOR_H__