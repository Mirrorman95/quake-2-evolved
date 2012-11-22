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
// parser.h - Script parser
//


#ifndef __PARSER_H__
#define __PARSER_H__


/*
 ==============================================================================

 Lexicographical Parser:

 A script can be loaded from either a file or an existing memory buffer.
 No memory is allocated during parsing.

 The parser can handle any set of punctuations. A C/C++ set is used by default,
 but it can be changed as desired during parsing.

 Numbers are automatically converted to a value and stored in the token
 structure for convenience.
 Several notations are supported: binary, octal, decimal, and hexadecimal.

 Escape characters inside quoted strings and literals are supported, but can be
 ignored and parsed as normal text if desired.

 The parser skips C/C++ style comments from scripts.

 ==============================================================================
*/

typedef enum {
	SF_NOWARNINGS					= BIT(0),	// Don't print warning messages
	SF_NOERRORS						= BIT(1),	// Don't print error messages
	SF_NOFATALERRORS				= BIT(2),	// Errors aren't fatal
	SF_ALLOWSTRINGCONCAT			= BIT(3),	// Allow multiple strings to be concatenated
	SF_ALLOWESCAPECHARS				= BIT(4),	// Allow escape characters inside strings
	SF_ALLOWMULTICHARLITERALS		= BIT(5),	// Allow multi-character literals
	SF_ALLOWPATHNAMES				= BIT(6),	// Allow path separators in names
	SF_PARSEPRIMITIVES				= BIT(7)	// Parse primitives separated by spaces
} scriptFlags_t;

typedef enum {
	TT_INVALID,									// Invalid (cleared to zero)
	TT_PRIMITIVE,								// Primitive (separated by whitespaces)
	TT_STRING,									// String (enclosed in double quotes)
	TT_LITERAL,									// Literal (enclosed in single quotes)
	TT_NUMBER,									// Number
	TT_NAME,									// Name
	TT_PUNCTUATION								// Punctuation
} tokenType_t;

typedef enum {
	NT_BINARY						= BIT(0),
	NT_OCTAL						= BIT(1),
	NT_DECIMAL						= BIT(2),
	NT_HEXADECIMAL					= BIT(3),
	NT_INTEGER						= BIT(4),
	NT_FLOAT						= BIT(5),
	NT_UNSIGNED						= BIT(6),
	NT_LONG							= BIT(7),
	NT_SINGLE						= BIT(8),
	NT_DOUBLE						= BIT(9),
	NT_EXTENDED						= BIT(10)
} numberType_t;

typedef enum {
	PT_RSHIFT_ASSIGN,
	PT_LSHIFT_ASSIGN,
	PT_PARAMETERS,
	PT_PRECOMPILER_MERGE,
	PT_LOGIC_AND,
	PT_LOGIC_OR,
	PT_LOGIC_GEQUAL,
	PT_LOGIC_LEQUAL,
	PT_LOGIC_EQUAL,
	PT_LOGIC_NOTEQUAL,
	PT_MUL_ASSIGN,
	PT_DIV_ASSIGN,
	PT_MOD_ASSIGN,
	PT_ADD_ASSIGN,
	PT_SUB_ASSIGN,
	PT_INCREMENT,
	PT_DECREMENT,
	PT_BINARY_AND_ASSIGN,
	PT_BINARY_OR_ASSIGN,
	PT_BINARY_XOR_ASSIGN,
	PT_RSHIFT,
	PT_LSHIFT,
	PT_POINTER_REFERENCE,
	PT_CPP_1,
	PT_CPP_2,
	PT_MUL,
	PT_DIV,
	PT_MOD,
	PT_ADD,
	PT_SUB,
	PT_ASSIGN,
	PT_BINARY_AND,
	PT_BINARY_OR,
	PT_BINARY_XOR,
	PT_BINARY_NOT,
	PT_LOGIC_NOT,
	PT_LOGIC_GREATER,
	PT_LOGIC_LESS,
	PT_REFERENCE,
	PT_COLON,
	PT_COMMA,
	PT_SEMICOLON,
	PT_QUESTION_MARK,
	PT_BRACE_OPEN,
	PT_BRACE_CLOSE,
	PT_BRACKET_OPEN,
	PT_BRACKET_CLOSE,
	PT_PARENTHESIS_OPEN,
	PT_PARENTHESIS_CLOSE,
	PT_PRECOMPILER,
	PT_DOLLAR,
	PT_BACKSLASH
} punctuationType_t;

typedef struct {
	tokenType_t				type;
	int						subType;

	int						line;
	int						linesCrossed;

	char					string[MAX_STRING_LENGTH];
	int						length;

	int						integerValue;
	float					floatValue;
} token_t;

typedef struct {
	const char *			name;
	int						type;
} punctuation_t;

typedef struct {
	char					name[MAX_PATH_LENGTH];

	const char *			buffer;
	int						size;
	int						start;
	bool					allocated;

	const char *			text;
	const char *			lastText;

	int						line;
	int						lastLine;

	int						flags;
	punctuation_t *			punctuations;

	bool					tokenAvailable;
	token_t					token;
} script_t;

// Reads a token from the script
bool			PS_ReadToken (script_t *script, token_t *token);

// Unreads the given token
void			PS_UnreadToken (script_t *script, const token_t *token);

// Reads a bool from the script
bool			PS_ReadBool (script_t *script, bool *value);

// Reads an integer from the script
bool			PS_ReadInteger (script_t *script, int *value);

// Reads a float from the script
bool			PS_ReadFloat (script_t *script, float *value);

// Reads a 1D matrix from the script
bool			PS_ReadMatrix1D (script_t *script, int x, float *matrix);

// Reads a 2D matrix from the script
bool			PS_ReadMatrix2D (script_t *script, int y, int x, float *matrix);

// Reads a 3D matrix from the script
bool			PS_ReadMatrix3D (script_t *script, int z, int y, int x, float *matrix);

// Expects a given token string. Returns true if found.
bool			PS_ExpectTokenString (script_t *script, token_t *token, const char *string, bool caseSensitive);

// Expects a given token type. Returns true if found.
bool			PS_ExpectTokenType (script_t *script, token_t *token, tokenType_t type, int subType);

// Checks a token string. Returns true and reads the token if available.
bool			PS_CheckTokenString (script_t *script, token_t *token, const char *string, bool caseSensitive);

// Checks a token type. Returns true and reads the token if available.
bool			PS_CheckTokenType (script_t *script, token_t *token, tokenType_t type, int subType);

// Returns true if the next token equals the given string
bool			PS_PeekTokenString (script_t *script, token_t *token, const char *string, bool caseSensitive);

// Returns true if the next token equals the given type
bool			PS_PeekTokenType (script_t *script, token_t *token, tokenType_t type, int subType);

// Returns true if there is any token available
bool			PS_TokenAvailable (script_t *script, bool crossLines);

// Skips until the given string is found
bool			PS_SkipUntilString (script_t *script, const char *string, bool caseSensitive);

// Skips the rest of the line
bool			PS_SkipRestOfLine (script_t *script);

// Skips until a matching close brace is found. Internal brace depths are
// properly skipped.
bool			PS_SkipBracedSection (script_t *script, int depth);

// Prints a warning message
void			PS_ScriptWarning (script_t *script, const char *fmt, ...);

// Prints an error message
void			PS_ScriptError (script_t *script, const char *fmt, ...);

// Resets the script
void			PS_ResetScript (script_t *script);

// Returns true if the end of the script has been reached
bool			PS_EndOfScript (script_t *script);

// Sets script flags
void			PS_SetScriptFlags (script_t *script, int flags);

// Sets a punctuations table. NULL will restore the default C/C++ set.
// The punctuations should be sorted by length (longest to shortest).
void			PS_SetPunctuationsTable (script_t *script, punctuation_t *punctuationsTable);

// Loads a script from the given file.
// Allocates memory to load the complete file.
// Returns NULL if the file wasn't found.
script_t *		PS_LoadScriptFile (const char *name);

// Loads a script from the given memory buffer.
// Does not allocate memory, so the buffer must remain valid during parsing.
// Returns NULL if the buffer or size is invalid.
script_t *		PS_LoadScriptMemory (const char *name, const char *buffer, int size, int line);

// Frees the memory allocated by PS_LoadScriptFile and PS_LoadScriptMemory
void			PS_FreeScript (script_t *script);


#endif	// __PARSER_H__