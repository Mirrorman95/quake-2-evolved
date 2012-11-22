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
// string.h - Library replacment functions
//


#ifndef __STRING_H__
#define __STRING_H__


#define MAX_STRING_LENGTH			1024		// Max length of a string

// Color escape strings
#define S_COLOR_DEFAULT				"^d"

#define S_COLOR_BLACK				"^0"
#define S_COLOR_RED					"^1"
#define	S_COLOR_GREEN				"^2"
#define S_COLOR_BLUE				"^3"
#define S_COLOR_YELLOW				"^4"
#define S_COLOR_CYAN				"^5"
#define S_COLOR_MAGENTA				"^6"
#define S_COLOR_WHITE				"^7"

// Color escape characters
#define C_COLOR_DEFAULT				'd'

#define C_COLOR_BLACK				'0'
#define C_COLOR_RED					'1'
#define	C_COLOR_GREEN				'2'
#define C_COLOR_BLUE				'3'
#define C_COLOR_YELLOW				'4'
#define C_COLOR_CYAN				'5'
#define C_COLOR_MAGENTA				'6'
#define C_COLOR_WHITE				'7'

// Colored string macros
#define Str_IsColor(s)				((s) && ((s)[0] == '^' && (s)[1] > ' '))
#define Str_ColorForChar(c)			(color_table[((c) - '0') & COLOR_MASK])
#define Str_ColorIndexForChar(c)	(((c) - '0') & COLOR_MASK)

// Character flags
typedef enum {
	CF_CONTROL						= BIT(0),
	CF_SPACE						= BIT(1),
	CF_BLANK						= BIT(2),
	CF_ALPHA						= BIT(3),
	CF_DIGIT						= BIT(4),
	CF_PUNCT						= BIT(5),
	CF_UPPER						= BIT(6),
	CF_LOWER						= BIT(7),
	CF_HEX							= BIT(8)
} charFlags_t;

extern short	str_charFlags[256];


INLINE bool Str_CharIsControl (int c){

	if (str_charFlags[c & 0xFF] & CF_CONTROL)
		return true;

	return false;
}

INLINE bool Str_CharIsSpace (int c){

	if (str_charFlags[c & 0xFF] & CF_SPACE)
		return true;

	return false;
}

INLINE bool Str_CharIsWhiteSpace (int c){

	if (str_charFlags[c & 0xFF] & (CF_CONTROL | CF_SPACE | CF_BLANK))
		return true;

	return false;
}

INLINE bool Str_CharIsAlpha (int c){

	if (str_charFlags[c & 0xFF] & CF_ALPHA)
		return true;

	return false;
}

INLINE bool Str_CharIsDigit (int c){

	if (str_charFlags[c & 0xFF] & CF_DIGIT)
		return true;

	return false;
}

INLINE bool Str_CharIsHexDigit (int c){

	if (str_charFlags[c & 0xFF] & (CF_DIGIT | CF_HEX))
		return true;

	return false;
}

INLINE bool Str_CharIsAlphaNumeric (int c){

	if (str_charFlags[c & 0xFF] & (CF_ALPHA | CF_DIGIT))
		return true;

	return false;
}

INLINE bool Str_CharIsPunctuation (int c){

	if (str_charFlags[c & 0xFF] & CF_PUNCT)
		return true;

	return false;
}

INLINE bool Str_CharIsPrintable (int c){

	if (str_charFlags[c & 0xFF] & (CF_BLANK | CF_ALPHA | CF_DIGIT | CF_PUNCT))
		return true;

	return false;
}

INLINE bool Str_CharIsUpper (int c){

	if (str_charFlags[c & 0xFF] & CF_UPPER)
		return true;

	return false;
}

INLINE bool Str_CharIsLower (int c){

	if (str_charFlags[c & 0xFF] & CF_LOWER)
		return true;

	return false;
}

INLINE int Str_CharToUpper (int c){

	if (str_charFlags[c & 0xFF] & CF_LOWER)
		c -= 32;

	return c;
}

INLINE int Str_CharToLower (int c){

	if (str_charFlags[c & 0xFF] & CF_UPPER)
		c += 32;

	return c;
}


// String manipulation functions
int				Str_Length (const char *string);
int				Str_LengthWithoutColors (const char *string);

void			Str_RemoveColors (char *string);

void			Str_Copy (char *dst, const char *src, int maxLength);
void			Str_Append (char *dst, const char *src, int maxLength);

int				Str_Compare (const char *string1, const char *string2);
int				Str_CompareChars (const char *string1, const char *string2, int n);
int				Str_CompareWithoutColors (const char *string1, const char *string2);
int				Str_ICompare (const char *string1, const char *string2);
int				Str_ICompareChars (const char *string1, const char *string2, int n);
int				Str_ICompareWithoutColors (const char *string1, const char *string2);

bool			Str_IsUpper (const char *string);
bool			Str_IsLower (const char *string);
void			Str_ToUpper (char *string);
void			Str_ToLower (char *string);

char *			Str_FindChar (const char *string, int c);
char *			Str_FindCharRev (const char *string, int c);
char *			Str_FindText (const char *string, const char *text, bool caseSensitive);
char *			Str_FindTextRev (const char *string, const char *text, bool caseSensitive);

int				Str_ToInteger (const char *string);
float			Str_ToFloat (const char *string);

const char *	Str_FromInteger (int value);
const char *	Str_FromFloat (float value, int precision);
const char *	Str_FromIntegerArray (const int *values, int elements);
const char *	Str_FromFloatArray (const float *values, int elements, int precision);

int				Str_SPrintf (char *dst, int maxLength, const char *fmt, ...);
int				Str_VSPrintf (char *dst, int maxLength, const char *fmt, va_list argPtr);

const char *	Str_VarArgs (const char *fmt, ...);

// Matches the given filter against the given string
bool			Str_MatchFilter (const char *string, const char *filter, bool caseSensitive);

// Returns a hash key for the given string.
// The hashSize parameter must be a power of two value.
uint			Str_HashKey (const char *string, uint hashSize, bool caseSensitive);

// Directory / file name manipulation functions
void			Str_DefaultFilePath (char *path, int maxLength, const char *defaultPath);
void			Str_DefaultFileExtension (char *path, int maxLength, const char *defaultExtension);

void			Str_StripFilePath (char *path);
void			Str_StripFileName (char *path);
void			Str_StripFileExtension (char *path);

void			Str_ExtractFilePath (const char *src, char *dst, int maxLength);
void			Str_ExtractFileName (const char *src, char *dst, int maxLength);
void			Str_ExtractFileBase (const char *src, char *dst, int maxLength);
void			Str_ExtractFileExtension (const char *src, char *dst, int maxLength);


#endif	// __STRING_H__