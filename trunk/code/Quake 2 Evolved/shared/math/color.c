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
// color.c - Color math
//


#include "../../common/common.h"


vec4_t		colorDefault		= {0.75f, 0.75f, 0.75f, 1.00f};

vec4_t		colorBlack			= {0.00f, 0.00f, 0.00f, 1.00f};
vec4_t		colorRed			= {1.00f, 0.00f, 0.00f, 1.00f};
vec4_t		colorGreen			= {0.00f, 1.00f, 0.00f, 1.00f};
vec4_t		colorBlue			= {0.00f, 0.00f, 1.00f, 1.00f};
vec4_t		colorYellow			= {1.00f, 1.00f, 0.00f, 1.00f};
vec4_t		colorCyan			= {0.00f, 1.00f, 1.00f, 1.00f};
vec4_t		colorMagenta		= {1.00f, 0.00f, 1.00f, 1.00f};
vec4_t		colorWhite			= {1.00f, 1.00f, 1.00f, 1.00f};
vec4_t		colorOrange			= {1.00f, 0.50f, 0.00f, 1.00f};
vec4_t		colorChartreuse		= {0.50f, 1.00f, 0.00f, 1.00f};
vec4_t		colorSpringGreen	= {0.00f, 1.00f, 0.50f, 1.00f};
vec4_t		colorAzure			= {0.00f, 0.50f, 1.00f, 1.00f};
vec4_t		colorViolet			= {0.50f, 0.00f, 1.00f, 1.00f};
vec4_t		colorRose			= {1.00f, 0.00f, 0.50f, 1.00f};
vec4_t		colorGray			= {0.50f, 0.50f, 0.50f, 1.00f};

vec4_t		color_table[64] = {
	{0.00f, 0.00f, 0.00f, 1.00f},		//  0 = Black
	{1.00f, 0.00f, 0.00f, 1.00f},		//  1 = Red
	{0.00f, 1.00f, 0.00f, 1.00f},		//  2 = Green
	{0.00f, 0.00f, 1.00f, 1.00f},		//  3 = Blue
	{1.00f, 1.00f, 0.00f, 1.00f},		//  4 = Yellow
	{0.00f, 1.00f, 1.00f, 1.00f},		//  5 = Cyan
	{1.00f, 0.00f, 1.00f, 1.00f},		//  6 = Magenta
	{1.00f, 1.00f, 1.00f, 1.00f},		//  7 = White
	{1.00f, 0.75f, 0.00f, 1.00f},		//  8 = Amber
	{0.55f, 0.71f, 0.00f, 1.00f},		//  9 = Apple green
	{0.18f, 0.55f, 0.34f, 1.00f},		// 10 = Aquamarine
	{0.65f, 0.16f, 0.16f, 1.00f},		// 11 = Auburn
	{0.00f, 0.50f, 1.00f, 1.00f},		// 12 = Azure
	{0.96f, 0.96f, 0.86f, 1.00f},		// 13 = Beige
	{0.59f, 0.29f, 0.00f, 1.00f},		// 14 = Brown
	{0.65f, 0.48f, 0.36f, 1.00f},		// 15 = Café au lait
	{0.00f, 0.75f, 1.00f, 1.00f},		// 16 = Capri
	{0.70f, 1.00f, 1.00f, 1.00f},		// 17 = Celeste
	{0.50f, 1.00f, 0.00f, 1.00f},		// 18 = Chartreuse
	{0.48f, 0.25f, 0.00f, 1.00f},		// 19 = Chocolate
	{1.00f, 0.50f, 0.31f, 1.00f},		// 20 = Coral
	{0.39f, 0.58f, 0.93f, 1.00f},		// 21 = Cornflower blue
	{1.00f, 0.99f, 0.82f, 1.00f},		// 22 = Cream
	{0.86f, 0.08f, 0.24f, 1.00f},		// 23 = Crimson
	{0.00f, 0.50f, 0.00f, 1.00f},		// 24 = Dark green
	{0.31f, 0.78f, 0.47f, 1.00f},		// 25 = Emerald
	{0.96f, 0.00f, 0.63f, 1.00f},		// 26 = Fuchsia
	{0.69f, 0.40f, 0.00f, 1.00f},		// 27 = Ginger
	{1.00f, 0.84f, 0.00f, 1.00f},		// 28 = Golden
	{0.85f, 0.65f, 0.13f, 1.00f},		// 29 = Goldenrod
	{0.50f, 0.50f, 0.50f, 1.00f},		// 30 = Gray
	{0.29f, 0.00f, 1.00f, 1.00f},		// 31 = Indigo
	{0.00f, 0.66f, 0.42f, 1.00f},		// 32 = Jade
	{0.76f, 0.69f, 0.57f, 1.00f},		// 33 = Khaki
	{0.71f, 0.49f, 0.86f, 1.00f},		// 34 = Lavender
	{0.68f, 0.85f, 0.90f, 1.00f},		// 35 = Light blue
	{0.78f, 0.64f, 0.78f, 1.00f},		// 36 = Lilac
	{0.75f, 1.00f, 0.00f, 1.00f},		// 37 = Lime
	{0.50f, 0.00f, 0.00f, 1.00f},		// 38 = Maroon
	{0.00f, 0.00f, 0.50f, 1.00f},		// 39 = Navy blue
	{0.80f, 0.47f, 0.13f, 1.00f},		// 40 = Ochre
	{0.50f, 0.50f, 0.00f, 1.00f},		// 41 = Olive
	{1.00f, 0.50f, 0.00f, 1.00f},		// 42 = Orange
	{0.85f, 0.44f, 0.84f, 1.00f},		// 43 = Orchid
	{1.00f, 0.75f, 0.80f, 1.00f},		// 44 = Pink
	{0.50f, 0.00f, 0.50f, 1.00f},		// 45 = Purple
	{1.00f, 0.00f, 0.50f, 1.00f},		// 46 = Rose
	{1.00f, 0.55f, 0.41f, 1.00f},		// 47 = Salmon
	{0.06f, 0.32f, 0.73f, 1.00f},		// 48 = Sapphire
	{1.00f, 0.14f, 0.00f, 1.00f},		// 49 = Scarlet
	{0.44f, 0.26f, 0.08f, 1.00f},		// 50 = Sepia
	{0.53f, 0.18f, 0.09f, 1.00f},		// 51 = Sienna
	{0.75f, 0.75f, 0.75f, 1.00f},		// 52 = Silver
	{0.44f, 0.50f, 0.56f, 1.00f},		// 53 = Slate gray
	{0.00f, 1.00f, 0.50f, 1.00f},		// 54 = Spring green
	{0.27f, 0.51f, 0.71f, 1.00f},		// 55 = Steel blue
	{0.82f, 0.71f, 0.55f, 1.00f},		// 56 = Tan
	{0.00f, 0.50f, 0.50f, 1.00f},		// 57 = Teal
	{0.89f, 0.45f, 0.36f, 1.00f},		// 58 = Terracotta
	{0.19f, 0.84f, 0.78f, 1.00f},		// 59 = Turquoise
	{0.95f, 0.90f, 0.67f, 1.00f},		// 60 = Vanilla
	{0.89f, 0.26f, 0.20f, 1.00f},		// 61 = Vermilion
	{0.50f, 0.00f, 1.00f, 1.00f},		// 62 = Violet
	{0.45f, 0.18f, 0.22f, 1.00f}		// 63 = Wine
};


/*
 ==================
 ColorPackL
 ==================
*/
void ColorPackL (byte color[4], const float l){

	color[0] = FloatToByte(l * 255.0f);
	color[1] = FloatToByte(l * 255.0f);
	color[2] = FloatToByte(l * 255.0f);
	color[3] = 255;
}

/*
 ==================
 ColorPackLA
 ==================
*/
void ColorPackLA (byte color[4], const float l, const float a){

	color[0] = FloatToByte(l * 255.0f);
	color[1] = FloatToByte(l * 255.0f);
	color[2] = FloatToByte(l * 255.0f);
	color[3] = FloatToByte(a * 255.0f);
}

/*
 ==================
 ColorPackFloatRGB
 ==================
*/
void ColorPackFloatRGB (byte color[4], const float r, const float g, const float b){

	color[0] = FloatToByte(r * 255.0f);
	color[1] = FloatToByte(g * 255.0f);
	color[2] = FloatToByte(b * 255.0f);
	color[3] = 255;
}

/*
 ==================
 ColorPackFloatRGBA
 ==================
*/
void ColorPackFloatRGBA (byte color[4], const float r, const float g, const float b, const float a){

	color[0] = FloatToByte(r * 255.0f);
	color[1] = FloatToByte(g * 255.0f);
	color[2] = FloatToByte(b * 255.0f);
	color[3] = FloatToByte(a * 255.0f);
}

/*
 ==================
 ColorPackVectorRGB
 ==================
*/
void ColorPackVectorRGB (byte color[4], const vec3_t rgb){

	color[0] = FloatToByte(rgb[0] * 255.0f);
	color[1] = FloatToByte(rgb[1] * 255.0f);
	color[2] = FloatToByte(rgb[2] * 255.0f);
	color[3] = 255;
}

/*
 ==================
 ColorPackVectorRGBA
 ==================
*/
void ColorPackVectorRGBA (byte color[4], const vec4_t rgba){

	color[0] = FloatToByte(rgba[0] * 255.0f);
	color[1] = FloatToByte(rgba[1] * 255.0f);
	color[2] = FloatToByte(rgba[2] * 255.0f);
	color[3] = FloatToByte(rgba[3] * 255.0f);
}


// ============================================================================


/*
 ==================
 ColorNormalize
 ==================
*/
uint ColorNormalize (vec3_t rgb){

	uint	c;
	float	max;

	// Catch negative colors
	if (rgb[0] < 0.0f)
		rgb[0] = 0.0f;
	if (rgb[1] < 0.0f)
		rgb[1] = 0.0f;
	if (rgb[2] < 0.0f)
		rgb[2] = 0.0f;

	// Determine the brightest of the three color components
	max = rgb[0];
	if (max < rgb[1])
		max = rgb[1];
	if (max < rgb[2])
		max = rgb[2];

	// Rescale all the color components if the intensity of the greatest
	// channel exceeds 1.0
	if (max > 1.0f){
		max = 255 * (1.0f / max);

		((byte *)&c)[0] = rgb[0] * max;
		((byte *)&c)[1] = rgb[1] * max;
		((byte *)&c)[2] = rgb[2] * max;
		((byte *)&c)[3] = 255;
	}
	else {
		((byte *)&c)[0] = rgb[0] * 255;
		((byte *)&c)[1] = rgb[1] * 255;
		((byte *)&c)[2] = rgb[2] * 255;
		((byte *)&c)[3] = 255;
	}
	
	return c;
}