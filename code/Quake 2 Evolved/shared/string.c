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
// string.c - Library replacment functions
//

#include "../common/common.h"


short	str_charFlags[256] = {
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL | CF_SPACE,
	CF_CONTROL | CF_SPACE,
	CF_CONTROL | CF_SPACE,
	CF_CONTROL | CF_SPACE,
	CF_CONTROL | CF_SPACE,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_SPACE | CF_BLANK,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_DIGIT | CF_HEX,
	CF_DIGIT | CF_HEX,
	CF_DIGIT | CF_HEX,
	CF_DIGIT | CF_HEX,
	CF_DIGIT | CF_HEX,
	CF_DIGIT | CF_HEX,
	CF_DIGIT | CF_HEX,
	CF_DIGIT | CF_HEX,
	CF_DIGIT | CF_HEX,
	CF_DIGIT | CF_HEX,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_ALPHA | CF_UPPER | CF_HEX,
	CF_ALPHA | CF_UPPER | CF_HEX,
	CF_ALPHA | CF_UPPER | CF_HEX,
	CF_ALPHA | CF_UPPER | CF_HEX,
	CF_ALPHA | CF_UPPER | CF_HEX,
	CF_ALPHA | CF_UPPER | CF_HEX,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_ALPHA | CF_LOWER | CF_HEX,
	CF_ALPHA | CF_LOWER | CF_HEX,
	CF_ALPHA | CF_LOWER | CF_HEX,
	CF_ALPHA | CF_LOWER | CF_HEX,
	CF_ALPHA | CF_LOWER | CF_HEX,
	CF_ALPHA | CF_LOWER | CF_HEX,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_CONTROL,
	CF_SPACE | CF_BLANK,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_PUNCT,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_PUNCT,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA | CF_UPPER,
	CF_ALPHA,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_PUNCT,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA | CF_LOWER,
	CF_ALPHA
};


/*
 ==================
 Str_Length
 ==================
*/
int Str_Length (const char *string){

	const char	*s;
	
	s = string;

	while (*s)
		s++;

	return s - string;
}

/*
 ==================
 Str_LengthWithoutColors

 Like Str_Length, but discounts color sequences
 ==================
*/
int Str_LengthWithoutColors (const char *string){

	int		length = 0;

	while (*string){
		if (Str_IsColor(string)){
			string += 2;
			continue;
		}

		string++;
		length++;
	}

	return length;
}

/*
 ==================
 Str_RemoveColors

 Removes color sequences from string
 ==================
*/
void Str_RemoveColors (char *string){

	char	*src, *dst;
	
	src = string;
	dst = string;

	while (*src){
		if (Str_IsColor(src)){
			src += 2;
			continue;
		}

		*dst++ = *src++;
	}

	*dst = 0;
}

/*
 ==================
 Str_Copy

 Ensures a trailing zero
 ==================
*/
void Str_Copy (char *dst, const char *src, int maxLength){

	while (--maxLength && *src)
		*dst++ = *src++;

	*dst = 0;
}

/*
 ==================
 Str_Append

 Ensures a trailing zero
 ==================
*/
void Str_Append (char *dst, const char *src, int maxLength){

	while (--maxLength && *dst)
		dst++;

	if (maxLength > 0){
		maxLength++;

		while (--maxLength && *src)
			*dst++ = *src++;

		*dst = 0;
	}
}

/*
 ==================
 Str_Compare
 ==================
*/
int Str_Compare (const char *string1, const char *string2){

	int		c1, c2;

	do {
		c1 = *string1++;
		c2 = *string2++;

		if (c1 != c2){
			// Strings not equal
			if (c1 < c2)
				return -1;
			else
				return 1;
		}
	} while (c1);

	return 0;	// Strings are equal
}

/*
 ==================
 Str_CompareChars
 ==================
*/
int Str_CompareChars (const char *string1, const char *string2, int n){

	int		c1, c2;

	do {
		c1 = *string1++;
		c2 = *string2++;

		if (!n--)
			return 0;	// Strings are equal until end point

		if (c1 != c2){
			// Strings not equal
			if (c1 < c2)
				return -1;
			else
				return 1;
		}
	} while (c1);

	return 0;	// Strings are equal
}

/*
 ==================
 Str_CompareWithoutColors
 ==================
*/
int Str_CompareWithoutColors (const char *string1, const char *string2){

	int		c1, c2;

	do {
		while (Str_IsColor(string1))
			string1 += 2;
		while (Str_IsColor(string2))
			string2 += 2;

		c1 = *string1++;
		c2 = *string2++;

		if (c1 != c2){
			// Strings not equal
			if (c1 < c2)
				return -1;
			else
				return 1;
		}
	} while (c1);

	return 0;	// Strings are equal
}

/*
 ==================
 Str_ICompare
 ==================
*/
int Str_ICompare (const char *string1, const char *string2){

	int		c1, c2;

	do {
		c1 = *string1++;
		c2 = *string2++;

		if (c1 != c2){
			if (str_charFlags[c1 & 0xFF] & CF_UPPER)
				c1 += 32;
			if (str_charFlags[c2 & 0xFF] & CF_UPPER)
				c2 += 32;

			if (c1 != c2){
				// Strings not equal
				if (c1 < c2)
					return -1;
				else
					return 1;
			}
		}
	} while (c1);

	return 0;	// Strings are equal
}

/*
 ==================
 Str_ICompareChars
 ==================
*/
int Str_ICompareChars (const char *string1, const char *string2, int n){

	int		c1, c2;

	do {
		c1 = *string1++;
		c2 = *string2++;

		if (!n--)
			return 0;	// Strings are equal until end point

		if (c1 != c2){
			if (str_charFlags[c1 & 0xFF] & CF_UPPER)
				c1 += 32;
			if (str_charFlags[c2 & 0xFF] & CF_UPPER)
				c2 += 32;

			if (c1 != c2){
				// Strings not equal
				if (c1 < c2)
					return -1;
				else
					return 1;
			}
		}
	} while (c1);

	return 0;	// Strings are equal
}

/*
 ==================
 Str_ICompareWithoutColors
 ==================
*/
int Str_ICompareWithoutColors (const char *string1, const char *string2){

	int		c1, c2;

	do {
		while (Str_IsColor(string1))
			string1 += 2;
		while (Str_IsColor(string2))
			string2 += 2;

		c1 = *string1++;
		c2 = *string2++;

		if (c1 != c2){
			if (str_charFlags[c1 & 0xFF] & CF_UPPER)
				c1 += 32;
			if (str_charFlags[c2 & 0xFF] & CF_UPPER)
				c2 += 32;

			if (c1 != c2){
				// Strings not equal
				if (c1 < c2)
					return -1;
				else
					return 1;
			}
		}
	} while (c1);

	return 0;	// Strings are equal
}

/*
 ==================
 Str_IsUpper
 ==================
*/
bool Str_IsUpper (const char *string){

	int		c;

	while (*string){
		c = *string;

		if (str_charFlags[c & 0xFF] & CF_LOWER)
			return false;

		string++;
	}

	return true;
}

/*
 ==================
 Str_IsLower
 ==================
*/
bool Str_IsLower (const char *string){

	int		c;

	while (*string){
		c = *string;

		if (str_charFlags[c & 0xFF] & CF_UPPER)
			return false;

		string++;
	}

	return true;
}

/*
 ==================
 Str_ToUpper
 ==================
*/
void Str_ToUpper (char *string){

	int		c;

	while (*string){
		c = *string;

		if (str_charFlags[c & 0xFF] & CF_LOWER)
			*string -= 32;

		string++;
	}
}

/*
 ==================
 Str_ToLower
 ==================
*/
void Str_ToLower (char *string){

	int		c;

	while (*string){
		c = *string;

		if (str_charFlags[c & 0xFF] & CF_UPPER)
			*string += 32;

		string++;
	}
}

/*
 ==================
 Str_FindChar
 ==================
*/
char *Str_FindChar (const char *string, int c){

	while (*string){
		if (*string == c)
			return (char *)string;

		string++;
	}

	return NULL;
}

/*
 ==================
 Str_FindCharRev
 ==================
*/
char *Str_FindCharRev (const char *string, int c){

	const char	*s;

	s = string + Str_Length(string);

	while (s >= string){
		if (*s == c)
			return (char *)s;

		s--;
	}

	return NULL;
}

/*
 ==================
 Str_FindText
 ==================
*/
char *Str_FindText (const char *string, const char *text, bool caseSensitive){

	int		c1, c2;
	int		i;

	while (*string){
		if (caseSensitive){
			for (i = 0; text[i]; i++){
				c1 = string[i];
				c2 = text[i];

				if (c1 != c2)
					break;
			}
		}
		else {
			for (i = 0; text[i]; i++){
				c1 = string[i];
				c2 = text[i];

				if (c1 != c2){
					if (str_charFlags[c1 & 0xFF] & CF_UPPER)
						c1 += 32;
					if (str_charFlags[c2 & 0xFF] & CF_UPPER)
						c2 += 32;

					if (c1 != c2)
						break;
				}
			}
		}

		if (!text[i])
			return (char *)string;

		string++;
	}

	return NULL;
}

/*
 ==================
 Str_FindTextRev
 ==================
*/
char *Str_FindTextRev (const char *string, const char *text, bool caseSensitive){

	const char	*s;
	int			c1, c2;
	int			i;

	s = string + Str_Length(string);

	while (s >= string){
		if (caseSensitive){
			for (i = 0; text[i]; i++){
				c1 = s[i];
				c2 = text[i];

				if (c1 != c2)
					break;
			}
		}
		else {
			for (i = 0; text[i]; i++){
				c1 = s[i];
				c2 = text[i];

				if (c1 != c2){
					if (str_charFlags[c1 & 0xFF] & CF_UPPER)
						c1 += 32;
					if (str_charFlags[c2 & 0xFF] & CF_UPPER)
						c2 += 32;

					if (c1 != c2)
						break;
				}
			}
		}

		if (!text[i])
			return (char *)s;

		s--;
	}

	return NULL;
}

/*
 ==================
 Str_ToInteger
 ==================
*/
int Str_ToInteger (const char *string){

	return atoi(string);
}

/*
 ==================
 Str_ToFloat
 ==================
*/
float Str_ToFloat (const char *string){

	return atof(string);
}

/*
 ==================
 Str_FromInteger
 ==================
*/
const char *Str_FromInteger (int value){

	static char	string[8][64];		// In case this is called by nested functions
	static int	index;
	char		*dst;
	int			maxLength;

	dst = string[index];
	maxLength = sizeof(string[index]);
	index = (index + 1) & 7;

	snprintf(dst, maxLength, "%i", value);

	dst[maxLength-1] = 0;

	return dst;
}

/*
 ==================
 Str_FromFloat
 ==================
*/
const char *Str_FromFloat (float value, int precision){

	static char	string[8][64];		// In case this is called by nested functions
	static int	index;
	char		fmt[16];
	char		*dst;
	int			maxLength;
	int			i;

	dst = string[index];
	maxLength = sizeof(string[index]);
	index = (index + 1) & 7;

	if (precision > 0){
		snprintf(fmt, sizeof(fmt), "%%.%if", precision);

		snprintf(dst, maxLength, fmt, value);
	}
	else {
		if (precision == 0)
			snprintf(dst, maxLength, "%i", (int)value);
		else
			snprintf(dst, maxLength, "%g", value);
	}

	if (precision == -1){
		for (i = 0; dst[i]; i++){
			if (dst[i] == '.')
				break;
		}

		if (!dst[i] && i < maxLength - 2){
			dst[i++] = '.';
			dst[i++] = '0';
			dst[i] = 0;
		}
	}

	dst[maxLength-1] = 0;

	return dst;
}

/*
 ==================
 Str_FromIntegerArray
 ==================
*/
const char *Str_FromIntegerArray (const int *values, int elements){

	static char	string[8][1024];	// In case this is called by nested functions
	static int	index;
	char		*dst;
	int			maxLength;
	int			length, ofs;
	int			i;

	dst = string[index];
	maxLength = sizeof(string[index]);
	index = (index + 1) & 7;

	ofs = snprintf(dst, maxLength, "%i", values[0]);

	for (i = 1; i < elements; i++){
		length = snprintf(dst + ofs, maxLength - ofs, " %i", values[i]);

		if (length < 0 || length >= maxLength - ofs)
			break;

		ofs += length;
	}

	dst[maxLength-1] = 0;

	return dst;
}

/*
 ==================
 Str_FromFloatArray
 ==================
*/
const char *Str_FromFloatArray (const float *values, int elements, int precision){

	static char	string[8][1024];	// In case this is called by nested functions
	static int	index;
	char		fmt[16];
	char		*dst;
	int			maxLength;
	int			length, ofs;
	int			i, j;

	dst = string[index];
	maxLength = sizeof(string[index]);
	index = (index + 1) & 7;

	if (precision > 0){
		snprintf(fmt, sizeof(fmt), "%%.%if", precision);

		ofs = snprintf(dst, maxLength, fmt, values[0]);
	}
	else {
		if (precision == 0)
			ofs = snprintf(dst, maxLength, "%i", (int)values[0]);
		else
			ofs = snprintf(dst, maxLength, "%g", values[0]);
	}

	if (precision == -1){
		for (j = 0; dst[j]; j++){
			if (dst[j] == '.')
				break;
		}

		if (!dst[j] && j < maxLength - 2){
			dst[j++] = '.';
			dst[j++] = '0';
			dst[j] = 0;

			ofs += 2;
		}
	}

	for (i = 1; i < elements; i++){
		if (precision > 0){
			snprintf(fmt, sizeof(fmt), " %%.%if", precision);

			length = snprintf(dst + ofs, maxLength - ofs, fmt, values[i]);
		}
		else {
			if (precision == 0)
				length = snprintf(dst + ofs, maxLength - ofs, " %i", (int)values[i]);
			else
				length = snprintf(dst + ofs, maxLength - ofs, " %g", values[i]);
		}

		if (length < 0 || length >= maxLength - ofs)
			break;

		if (precision == -1){
			for (j = ofs; dst[j]; j++){
				if (dst[j] == '.')
					break;
			}

			if (!dst[j] && j < maxLength - 2){
				dst[j++] = '.';
				dst[j++] = '0';
				dst[j] = 0;

				ofs += 2;
			}
		}

		ofs += length;
	}

	dst[maxLength-1] = 0;

	return dst;
}

/*
 ==================
 Str_SPrintf

 Ensures a trailing zero
 ==================
*/
int Str_SPrintf (char *dst, int maxLength, const char *fmt, ...){

	va_list	argPtr;
	int		length;

	va_start(argPtr, fmt);
	length = vsnprintf(dst, maxLength, fmt, argPtr);
	va_end(argPtr);

	dst[maxLength-1] = 0;

	if (length < 0 || length >= maxLength)
		return -1;

	return length;
}

/*
 ==================
 Str_VSPrintf

 Ensures a trailing zero
 ==================
*/
int Str_VSPrintf (char *dst, int maxLength, const char *fmt, va_list argPtr){

	int		length;

	length = vsnprintf(dst, maxLength, fmt, argPtr);

	dst[maxLength-1] = 0;

	if (length < 0 || length >= maxLength)
		return -1;

	return length;
}

/*
 ==================
 Str_VarArgs

 Does a varargs printf into a temporary buffer, so we don't need to have
 varargs versions of all text functions
 ==================
*/
const char *Str_VarArgs (const char *fmt, ...){

	static char	string[8][MAX_STRING_LENGTH];	// In case this is called by nested functions
	static int	index;
	va_list		argPtr;
	char		*dst;
	int			maxLength;
	int			length;

	dst = string[index];
	maxLength = sizeof(string[index]);
	index = (index + 1) & 7;

	va_start(argPtr, fmt);
	length = vsnprintf(dst, maxLength, fmt, argPtr);
	va_end(argPtr);

	dst[maxLength-1] = 0;

	if (length < 0 || length >= maxLength)
		return "";

	return dst;
}

/*
 ==================
 Str_MatchFilter

 Matches the given filter against the given string.
 Returns true if matches, false otherwise.

 A match means the entire string is used up in matching.

 In the filter string, '*' matches any sequence of characters, '?' matches any
 character, '[SET]' matches any character in the specified set, '[!SET]'
 matches any character not in the specified set.

 A set is composed of characters or ranges. A range contains a hyphen character
 (as in 0-9 or A-Z).
 [0-9a-zA-Z_] is the set of characters allowed in C/C++ identifiers.
 Any other character in the filter must be matched exactly.

 To suppress the special syntactic significance of any of *?[]!-\, and match
 the character exactly, precede it with a '\'.
 ==================
*/
bool Str_MatchFilter (const char *string, const char *filter, bool caseSensitive){

	int		c1, c2, start, end;
	bool	invert;

	while ((c1 = *filter++)){
		switch (c1){
		case '*':
			while ((c1 = *filter++) == '?' || c1 == '*'){
				if (c1 == '?' && !*string++)
					return false;
			}

			if (!c1)
				return true;

			if (c1 == '\\')
				c2 = *filter;
			else
				c2 = c1;

			while (1){
				if (caseSensitive){
					if (c1 == '[' || *string == c2){
						if (Str_MatchFilter(string, filter - 1, caseSensitive))
							return true;
					}
				}
				else {
					if (c1 == '[' || Str_CharToLower(*string) == Str_CharToLower(c2)){
						if (Str_MatchFilter(string, filter - 1, caseSensitive))
							return true;
					}
				}

				if (!*string++)
					return false;
			}

			break;
		case '?':
			if (!*string)
				return false;
			else
				++string;

			break;
		case '[':
			c2 = *string++;
			if (!c2)
				return false;

			invert = (*filter == '!');
			if (invert)
				filter++;

			c1 = *filter++;

			while (1){
				start = c1;
				end = c1;

				if (c1 == '\\'){
					start = *filter++;
					end = start;
				}
				if (!c1)
					return false;

				c1 = *filter++;

				if (c1 == '-' && *filter != ']'){
					end = *filter++;
					if (end == '\\')
						end = *filter++;
					if (!end)
						return false;

					c1 = *filter++;
				}

				if (caseSensitive){
					if (c2 >= start && c2 <= end)
						goto match;
				}
				else {
					if (Str_CharToLower(c2) >= Str_CharToLower(start) && Str_CharToLower(c2) <= Str_CharToLower(end))
						goto match;
				}

				if (c1 == ']')
					break;
			}

			if (!invert)
				return false;

			break;

match:
			while (c1 != ']'){
				if (!c1)
					return false;

				c1 = *filter++;
				if (!c1)
					return false;
				else if (c1 == '\\')
					++filter;
			}

			if (invert)
				return false;

			break;
		case '\\':
			if (caseSensitive){
				if (*filter++ != *string++)
					return false;
			}
			else {
				if (Str_CharToLower(*filter++) != Str_CharToLower(*string++))
					return false;
			}

			break;
		default:
			if (caseSensitive){
				if (c1 != *string++)
					return false;
			}
			else {
				if (Str_CharToLower(c1) != Str_CharToLower(*string++))
					return false;
			}

			break;
		}
	}

	if (*string)
		return false;

	return true;
}

/*
 ==================
 Str_HashKey

 Returns a hash key for the given string
 ==================
*/
uint Str_HashKey (const char *string, uint hashSize, bool caseSensitive){

	uint	hashKey = 0;
	int		i, c;

	if (caseSensitive){
		for (i = 0; string[i]; i++){
			c = string[i];

			hashKey += (i + 119) * c;
		}
	}
	else {
		for (i = 0; string[i]; i++){
			c = string[i];

			if (str_charFlags[c & 0xFF] & CF_UPPER)
				c += 32;

			hashKey += (i * 119) * c;
		}
	}

	hashKey = ((hashKey ^ (hashKey >> 10)) ^ (hashKey >> 20)) & (hashSize - 1);

	return hashKey;
}

/*
 ==================
 Str_DefaultFilePath

 If the path doesn't have a / or a \\, inserts defaultPath (which should not
 include the '/')
 ==================
*/
void Str_DefaultFilePath (char *path, int maxLength, const char *defaultPath){

	char	copy[MAX_PATH_LENGTH];
	char	*p;

	p = path;

	while (*p){
		if (*p == '/' || *p == '\\')
			return;		// It has a path
		
		p++;
	}

	Str_Copy(copy, path, sizeof(copy));
	Str_SPrintf(path, maxLength, "%s/%s", defaultPath, copy);
}

/*
 ==================
 Str_DefaultFileExtension

 If the path doesn't have a .EXT, appends defaultExtension (which should
 include the '.')
 ==================
*/
void Str_DefaultFileExtension (char *path, int maxLength, const char *defaultExtension){

	char	*p;

	p = path + Str_Length(path);

	while (p >= path){
		if (*p == '/' || *p == '\\')
			break;

		if (*p == '.')
			return;		// It has an extension

		p--;
	}

	Str_Append(path, defaultExtension, maxLength);
}

/*
 ==================
 Str_StripFilePath

 Removes the file path
 ==================
*/
void Str_StripFilePath (char *path){

	char	*p;

	p = path + Str_Length(path);

	while (p >= path){
		if (*p == '/' || *p == '\\'){
			p++;
			break;
		}

		p--;
	}

	if (p < path)
		return;		// It has no path

	while (*p)
		*path++ = *p++;

	*path = 0;
}

/*
 ==================
 Str_StripFileName

 Removes the file name
 ==================
*/
void Str_StripFileName (char *path){

	char	*p;

	p = path + Str_Length(path);

	while (p >= path){
		if (*p == '/' || *p == '\\'){
			*p = 0;
			break;
		}

		p--;
	}

	if (p < path)
		*path = 0;
}

/*
 ==================
 Str_StripFileExtension

 Removes the file extension
 ==================
*/
void Str_StripFileExtension (char *path){

	char	*p;

	p = path + Str_Length(path);

	while (p >= path){
		if (*p == '/' || *p == '\\')
			break;

		if (*p == '.'){
			*p = 0;
			break;
		}

		p--;
	}
}

/*
 ==================
 Str_ExtractFilePath

 Extracts the file path
 ==================
*/
void Str_ExtractFilePath (const char *src, char *dst, int maxLength){

	const char	*s;

	s = src + Str_Length(src);

	while (s >= src){
		if (*s == '/' || *s == '\\')
			break;

		s--;
	}

	if (s < src)
		s++;

	if (s - src < maxLength)
		Str_Copy(dst, src, (s - src) + 1);
	else
		Str_Copy(dst, src, maxLength);
}

/*
 ==================
 Str_ExtractFileName

 Extracts the file name
 ==================
*/
void Str_ExtractFileName (const char *src, char *dst, int maxLength){

	const char	*s;

	s = src + Str_Length(src);

	while (s >= src){
		if (*s == '/' || *s == '\\'){
			s++;
			break;
		}

		s--;
	}

	if (s < src)
		s++;

	Str_Copy(dst, s, maxLength);
}

/*
 ==================
 Str_ExtractFileBase

 Extracts the file name minus the extension
 ==================
*/
void Str_ExtractFileBase (const char *src, char *dst, int maxLength){

	const char	*s, *p;

	s = src + Str_Length(src);
	p = NULL;

	while (s >= src){
		if (*s == '/' || *s == '\\'){
			s++;
			break;
		}

		if (!p && *s == '.')
			p = s;

		s--;
	}

	if (s < src)
		s++;

	if (p && p - s < maxLength)
		Str_Copy(dst, s, (p - s) + 1);
	else
		Str_Copy(dst, s, maxLength);
}

/*
 ==================
 Str_ExtractFileExtension

 Extracts the file extension
 ==================
*/
void Str_ExtractFileExtension (const char *src, char *dst, int maxLength){

	const char	*s, *p;

	s = src + Str_Length(src);
	p = s;

	while (s >= src){
		if (*s == '/' || *s == '\\')
			break;

		if (*s == '.'){
			p = s;
			break;
		}

		s--;
	}

	Str_Copy(dst, p, maxLength);
}