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
// r_postProcess.c - Post-Processing effects management
//


#include "r_local.h"


/*
 ==============================================================================

 POST-PROCESSING COLOR TABLE

 ==============================================================================
*/


/*
 ==================
 R_LoadColorTable
 ==================
*/
void R_LoadColorTable (const char *name, byte colorTable[256][4]){

	tgaHeader_t	header;
	byte		*data;
	byte		*image;
	int			i;

	// If the default color table is specified
	if (!Str_ICompare(name, "_default")){
		for (i = 0; i < 256; i++){
			colorTable[i][0] = i;
			colorTable[i][1] = i;
			colorTable[i][2] = i;
			colorTable[i][3] = 255;
		}

		return;
	}

	// Load the file
	FS_ReadFile(name, (void **)&data);
	if (!data){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find color table '%s', using default\n", name);

		// Create a default color table
		for (i = 0; i < 256; i++){
			colorTable[i][0] = i;
			colorTable[i][1] = i;
			colorTable[i][2] = i;
			colorTable[i][3] = 255;
		}

		return;
	}

	// Parse the TGA header
	header.bIdLength = data[0];
	header.bColormapType = data[1];
	header.bImageType = data[2];
	header.wColormapIndex = data[3] | (data[4] << 8);
	header.wColormapLength = data[5] | (data[6] << 8);
	header.bColormapSize = data[7];
	header.wXOrigin = data[8] | (data[9] << 8);
	header.wYOrigin = data[10] | (data[11] << 8);
	header.wWidth = data[12] | (data[13] << 8);
	header.wHeight = data[14] | (data[15] << 8);
	header.bPixelSize = data[16];
	header.bAttributes = data[17];

	if (header.bImageType != TGA_IMAGE_TRUECOLOR && header.bImageType != TGA_IMAGE_MONOCHROME)
		Com_Error(ERR_DROP, "R_LoadColorTable: only type %i (RGB) and %i (gray) images supported (%s)", TGA_IMAGE_TRUECOLOR, TGA_IMAGE_MONOCHROME, name);

	if (header.bPixelSize != 8 && header.bPixelSize != 24)
		Com_Error(ERR_DROP, "R_LoadColorTable: only 8 (gray) and 24 (RGB) bit images supported (%s)", name);

	if (header.wWidth != 256 || header.wHeight != 1)
		Com_Error(ERR_DROP, "R_LoadColorTable: bad image size (%i x %i) (%s)", header.wWidth, header.wHeight, name);

	// Read the image pixels
	image = data + 18 + header.bIdLength;

	switch (header.bPixelSize){
	case 8:
		for (i = 0; i < 256; i++, image += 1){
			colorTable[i][0] = image[0];
			colorTable[i][1] = image[0];
			colorTable[i][2] = image[0];
			colorTable[i][3] = 255;
		}

		break;
	case 24:
		for (i = 0; i < 256; i++, image += 3){
			colorTable[i][0] = image[2];
			colorTable[i][1] = image[1];
			colorTable[i][2] = image[0];
			colorTable[i][3] = 255;
		}

		break;
	}

	// Free file data
	FS_FreeFile(data);
}

/*
 ==================
 R_BlendColorTables
 ==================
*/
void R_BlendColorTables (byte colorTable[256][4], const byte previousTable[256][4], const byte currentTable[256][4], float frac){

#if defined SIMD_X86

	__m128i	xmmZero, xmmColor[2];
	__m128i	xmmScale, xmmInvScale;
	__m128i	xmmSrc, xmmSrcScaled;
	__m128i	xmmDst, xmmDstScaled;
	float	f;
	int		i;

	f = 256.0f * frac;
	i = _mm_cvtt_ss2si(_mm_load_ss(&f));

	xmmZero = _mm_setzero_si128();

	xmmScale = _mm_set1_epi16(i);
	xmmInvScale = _mm_set1_epi16(256 - i);

	for (i = 0; i < 256; i += 4){
		xmmSrc = _mm_loadu_si128((const __m128i *)(previousTable[i]));
		xmmDst = _mm_loadu_si128((const __m128i *)(currentTable[i]));

		xmmSrcScaled = _mm_mullo_epi16(_mm_unpacklo_epi8(xmmSrc, xmmZero), xmmInvScale);
		xmmDstScaled = _mm_mullo_epi16(_mm_unpacklo_epi8(xmmDst, xmmZero), xmmScale);

		xmmColor[0] = _mm_srli_epi16(_mm_add_epi16(xmmSrcScaled, xmmDstScaled), 8);

		xmmSrcScaled = _mm_mullo_epi16(_mm_unpackhi_epi8(xmmSrc, xmmZero), xmmInvScale);
		xmmDstScaled = _mm_mullo_epi16(_mm_unpackhi_epi8(xmmDst, xmmZero), xmmScale);

		xmmColor[1] = _mm_srli_epi16(_mm_add_epi16(xmmSrcScaled, xmmDstScaled), 8);

		_mm_storeu_si128((__m128i *)(colorTable[i]), _mm_packus_epi16(xmmColor[0], xmmColor[1]));
	}

#else

	int		scale, invScale;
	int		i;

	scale = FloatToInt(256.0f * frac);
	invScale = 256 - scale;

	for (i = 0; i < 256; i++){
		colorTable[i][0] = (previousTable[i][0] * invScale + currentTable[i][0] * scale) >> 8;
		colorTable[i][1] = (previousTable[i][1] * invScale + currentTable[i][1] * scale) >> 8;
		colorTable[i][2] = (previousTable[i][2] * invScale + currentTable[i][2] * scale) >> 8;
		colorTable[i][3] = (previousTable[i][3] * invScale + currentTable[i][3] * scale) >> 8;
	}

#endif
}


/*
 ==============================================================================

 POST-PROCESSING EFFECTS PARSING

 ==============================================================================
*/


/*
 ==================
 R_ParsePostProcess
 ==================
*/
static bool R_ParsePostProcess (script_t *script){

	token_t				token;
	postProcessParms_t	*postProcessParms;
	int					area;
	int					i;

	// Parse the area number
	if (!PS_ReadInteger(script, &area)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing area number in post-process file\n");
		return false;
	}

	if (area < 0 || area >= CM_NumAreas()){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %i for area number in post-process file\n", area);
		return false;
	}

	// Parse the post-process parameters
	postProcessParms = &rg.postProcess.postProcessList[area];

	if (!PS_ExpectTokenString(script, &token, "{", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in post-process file (area %i)\n", token.string, area);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in post-process file (area %i)\n", area);
			return false;	// End of script
		}

		if (!Str_ICompare(token.string, "}"))
			break;			// End of post-process

		// Parse the parameter
		if (!Str_ICompare(token.string, "bloomContrast")){
			if (!PS_ReadFloat(script, &postProcessParms->bloomContrast)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'bloomContrast' parameter in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "bloomThreshold")){
			if (!PS_ReadFloat(script, &postProcessParms->bloomThreshold)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'bloomThreshold' parameter in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "baseIntensity")){
			if (!PS_ReadFloat(script, &postProcessParms->baseIntensity)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'baseIntensity' parameter in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "glowIntensity")){
			if (!PS_ReadFloat(script, &postProcessParms->glowIntensity)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'glowIntensity' parameter in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "colorShadows")){
			if (!PS_ReadMatrix1D(script, 3, postProcessParms->colorShadows)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'colorShadows' parameters in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "colorHighlights")){
			if (!PS_ReadMatrix1D(script, 3, postProcessParms->colorHighlights)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'colorHighlights' parameters in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "colorMidtones")){
			if (!PS_ReadMatrix1D(script, 3, postProcessParms->colorMidtones)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'colorMidtones' parameters in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "colorMinOutput")){
			if (!PS_ReadMatrix1D(script, 3, postProcessParms->colorMinOutput)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'colorMinOutput' parameter in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "colorMaxOutput")){
			if (!PS_ReadMatrix1D(script, 3, postProcessParms->colorMaxOutput)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'colorMaxOutput' parameter in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "colorSaturation")){
			if (!PS_ReadMatrix1D(script, 3, postProcessParms->colorSaturation)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'colorSaturation' parameter in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "colorTint")){
			if (!PS_ReadMatrix1D(script, 3, postProcessParms->colorTint)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'colorTint' parameters in post-process file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "colorTable")){
			if (!PS_ReadToken(script, &token)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'colorTable' parameter in post-process file (area %i)\n", area);
				return false;
			}

			Str_Copy(postProcessParms->colorTableName, token.string, sizeof(postProcessParms->colorTableName));
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' in post-process file (area %i)\n", token.string, area);
			return false;
		}
	}

	// Clamp the post-process parameters
	postProcessParms->bloomContrast = ClampFloat(postProcessParms->bloomContrast, 0.0f, 4.0f);
	postProcessParms->bloomThreshold = ClampFloat(postProcessParms->bloomThreshold, 0.0f, 1.0f);
	postProcessParms->baseIntensity = ClampFloat(postProcessParms->baseIntensity, 0.0f, 4.0f);
	postProcessParms->glowIntensity = ClampFloat(postProcessParms->glowIntensity, 0.0f, 4.0f);
	VectorClamp(0.0f, 1.0f, postProcessParms->colorShadows);
	VectorClamp(0.0f, 1.0f, postProcessParms->colorHighlights);
	VectorClamp(0.1f, 9.99f, postProcessParms->colorMidtones);
	VectorClamp(0.0f, 1.0f, postProcessParms->colorMinOutput);
	VectorClamp(0.0f, 1.0f, postProcessParms->colorMaxOutput);
	VectorClamp(0.0f, 4.0f, postProcessParms->colorSaturation);
	VectorClamp(0.0f, 1.0f, postProcessParms->colorTint);

	for (i = 0; i < 3; i++){
		if (postProcessParms->colorShadows[i] >= postProcessParms->colorHighlights[i]){
			postProcessParms->colorShadows[i] = 0.0f;
			postProcessParms->colorHighlights[i] = 1.0f;
		}

		if (postProcessParms->colorMinOutput[i] >= postProcessParms->colorMaxOutput[i]){
			postProcessParms->colorMinOutput[i] = 0.0f;
			postProcessParms->colorMaxOutput[i] = 1.0f;
		}
	}

	// Load the color table
	R_LoadColorTable(postProcessParms->colorTableName, postProcessParms->colorTable);

	return true;
}

/*
 ==================
 R_LoadPostProcess
 ==================
*/
void R_LoadPostProcess (const char *name){

	script_t	*script;
	token_t		token;

	if (!r_postProcess->integerValue)
		return;

	// Copy the name
	Str_Copy(rg.postProcess.postProcessName, name, sizeof(rg.postProcess.postProcessName));

	// Load the script file
	script = PS_LoadScriptFile(name);
	if (!script){
		Com_Printf(S_COLOR_RED "Post-process file %s not found\n", name);
		return;
	}

	PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

	// Parse it
	while (1){
		if (!PS_ReadToken(script, &token))
			break;		// End of script

		if (!Str_ICompare(token.string, "postProcess")){
			if (!R_ParsePostProcess(script))
				break;
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: expected 'postProcess', found '%s' instead in post-process file\n", token.string);
			break;
		}
	}

	// Free the script file
	PS_FreeScript(script);
}