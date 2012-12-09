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
// r_font.c - TrueType font
//


#include "r_local.h"


#define FONTS_HASH_SIZE				(MAX_FONTS >> 2)

#define CHARSET_MATERIAL			"charset"
#define CHARSET_IMAGE_WIDTH			256
#define CHARSET_IMAGE_HEIGHT		512

typedef struct {
	byte *					data;
	int						offset;
} fontFile_t;

static font_t *				r_fontsHashTable[FONTS_HASH_SIZE];
static font_t *				r_fonts[MAX_FONTS];
static int					r_numFonts;


/*
 ==============================================================================

 FONT REGISTRATION

 ==============================================================================
*/


/*
 ==================
 R_ReadFontLong
 ==================
*/
static int R_ReadFontLong (fontFile_t *file){

	int		l;

	l = *(int *)(file->data + file->offset);
	file->offset += 4;

	return LittleLong(l);
}

/*
 ==================
 R_ReadFontFloat
 ==================
*/
static float R_ReadFontFloat (fontFile_t *file){

	float	f;

	f = *(float *)(file->data + file->offset);
	file->offset += 4;

	return LittleFloat(f);
}

/*
 ==================
 R_LoadFontSet
 ==================
*/
static bool R_LoadFontSet (const char *name, fontSet_t *fontSet){

	fontFile_t	file;
	fontInfo_t	*fontInfo;
	glyphInfo_t	*glyphInfo;
	char		realName[MAX_PATH_LENGTH];
	byte		*data;
	int			i, j;

	for (i = 0, fontInfo = fontSet->fontInfo; i < FONTS_PER_SET; i++, fontInfo++){
		// Load the file
		Str_SPrintf(realName, sizeof(realName), "fonts/%s/fontImage_%i.dat", name, 12 << i);

		FS_ReadFile(realName, (void **)&data);
		if (!data)
			return false;

		file.data = data;
		file.offset = 0;

		// Parse it
		fontInfo->maxWidth = R_ReadFontLong(&file);
		fontInfo->maxHeight = R_ReadFontLong(&file);

		fontInfo->glyphImages = R_ReadFontLong(&file);
		fontInfo->glyphScale = R_ReadFontFloat(&file);

		for (j = 0, glyphInfo = fontInfo->glyphs; j < GLYPHS_PER_FONT; j++, glyphInfo++){
			glyphInfo->image = R_ReadFontLong(&file);

			glyphInfo->xAdjust = R_ReadFontLong(&file);
			glyphInfo->yAdjust = R_ReadFontLong(&file);

			glyphInfo->width = R_ReadFontLong(&file);
			glyphInfo->height = R_ReadFontLong(&file);

			glyphInfo->s1 = R_ReadFontFloat(&file);
			glyphInfo->t1 = R_ReadFontFloat(&file);
			glyphInfo->s2 = R_ReadFontFloat(&file);
			glyphInfo->t2 = R_ReadFontFloat(&file);
		}

		// Free file data
		FS_FreeFile(data);
	}

	// Load the materials
	for (i = 0, fontInfo = fontSet->fontInfo; i < FONTS_PER_SET; i++, fontInfo++){
		for (j = 0, glyphInfo = fontInfo->glyphs; j < GLYPHS_PER_FONT; j++, glyphInfo++){
			Str_SPrintf(realName, sizeof(realName), "fonts/%s/fontImage_%i_%i", name, 12 << i, fontInfo->glyphs[j].image);

			glyphInfo->material = R_FindMaterial(realName, MT_NOMIP, SURFACEPARM_NONE);
		}
	}

	return true;
}

/*
 ==================
 R_LoadFont
 ==================
*/
static font_t *R_LoadFont (const char *name, bool defaulted, fontSet_t *fontSet){

	font_t	*font;
	uint	hashKey;

	if (r_numFonts == MAX_FONTS)
		Com_Error(ERR_DROP, "R_LoadFont: MAX_FONTS hit");

	r_fonts[r_numFonts++] = font = (font_t *)Mem_Alloc(sizeof(font_t), TAG_RENDERER);

	// Fill it in
	Str_Copy(font->name, name, sizeof(font->name));
	font->defaulted = defaulted;
	font->fontSet = (fontSet_t *)Mem_DupData(fontSet, sizeof(fontSet_t), TAG_RENDERER);

	// Add to hash table
	hashKey = Str_HashKey(font->name, FONTS_HASH_SIZE, false);

	font->nextHash = r_fontsHashTable[hashKey];
	r_fontsHashTable[hashKey] = font;

	return font;
}

/*
 ==================
 R_FindFont
 ==================
*/
font_t *R_FindFont (const char *name){

	font_t		*font;
	fontSet_t	fontSet;
	uint		hashKey;

	// Check if already loaded
	hashKey = Str_HashKey(name, FONTS_HASH_SIZE, false);

	for (font = r_fontsHashTable[hashKey]; font; font = font->nextHash){
		if (!Str_ICompare(font->name, name))
			return font;
	}

	// Load it from disk
	if (R_LoadFontSet(name, &fontSet))
		return R_LoadFont(name, false, &fontSet);

	// Not found
	return NULL;
}

/*
 ==================
 R_RegisterFont
 ==================
*/
fontSet_t *R_RegisterFont (const char *name){

	font_t	*font;

	font = R_FindFont(name);
	if (font)
		return font->fontSet;

	// Register the name even if not found
	Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find font '%s', using default\n", name);

	font = R_LoadFont(name, true, rg.defaultFont->fontSet);

	return font->fontSet;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_ListFonts_f
 ==================
*/
static void R_ListFonts_f (){

	font_t	*font;
	int		images;
	int		i, j;

	Com_Printf("\n");
	Com_Printf("      imgs -name-----------\n");

	for (i = 0; i < r_numFonts; i++){
		font = r_fonts[i];

		Com_Printf("%4i: ", i);

		images = 0;

		for (j = 0; j < FONTS_PER_SET; j++)
			images += font->fontSet->fontInfo[j].glyphImages;

		Com_Printf("%4i ", images);

		Com_Printf("%s%s\n", font->name, (font->defaulted) ? " (DEFAULTED)" : "");
	}

	Com_Printf("--------------------------\n");
	Com_Printf("%i total fonts\n", r_numFonts);
	Com_Printf("\n");
}


/*
 ==============================================================================

 DEFAULT FONT

 ==============================================================================
*/


/*
 ==================
 R_CreateDefaultFont
 ==================
*/
static void R_CreateDefaultFont (){

	fontSet_t	fontSet;
	fontInfo_t	*fontInfo;
	glyphInfo_t	*glyphInfo;
	float		col, row;
	int			i, j;

	for (i = 0, fontInfo = fontSet.fontInfo; i < FONTS_PER_SET; i++, fontInfo++){
		fontInfo->maxWidth = CHARSET_IMAGE_WIDTH >> 4;
		fontInfo->maxHeight = CHARSET_IMAGE_HEIGHT >> 4;

		fontInfo->glyphImages = 0;
		fontInfo->glyphScale = 1.0f;

		for (j = 0, glyphInfo = fontInfo->glyphs; j < GLYPHS_PER_FONT; j++, glyphInfo++){
			col = (j & 15) * 0.0625f;
			row = (j >> 4) * 0.0625f;

			glyphInfo->image = -1;

			glyphInfo->xAdjust = CHARSET_IMAGE_WIDTH >> 4;
			glyphInfo->yAdjust = CHARSET_IMAGE_HEIGHT >> 4;

			glyphInfo->width = CHARSET_IMAGE_WIDTH >> 4;
			glyphInfo->height = CHARSET_IMAGE_HEIGHT >> 4;

			glyphInfo->s1 = col;
			glyphInfo->t1 = row;
			glyphInfo->s2 = col + 0.0625f;
			glyphInfo->t2 = row + 0.0625f;

			glyphInfo->material = R_FindMaterial(CHARSET_MATERIAL, MT_NOMIP, SURFACEPARM_NONE);
		}
	}

	rg.defaultFont = R_LoadFont("_default", false, &fontSet);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_InitFonts
 ==================
*/
void R_InitFonts (){

	// Add commands
	Cmd_AddCommand("listFonts", R_ListFonts_f, "Lists loaded fonts", NULL);

	// Create the default font
	R_CreateDefaultFont();
}

/*
 ==================
 R_ShutdownFonts
 ==================
*/
void R_ShutdownFonts (){

	// Remove commands
	Cmd_RemoveCommand("listFonts");

	// Clear font list
	Mem_Fill(r_fontsHashTable, 0, sizeof(r_fontsHashTable));
	Mem_Fill(r_fonts, 0, sizeof(r_fonts));

	r_numFonts = 0;
}