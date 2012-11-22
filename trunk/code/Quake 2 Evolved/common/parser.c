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
// parser.c - Script parser
//


#include "common.h"


static punctuation_t	ps_punctuationsTable[] = {
	{">>=",		PT_RSHIFT_ASSIGN},
	{"<<=",		PT_LSHIFT_ASSIGN},
	{"...",		PT_PARAMETERS},
	{"##",		PT_PRECOMPILER_MERGE},
	{"&&",		PT_LOGIC_AND},
	{"||",		PT_LOGIC_OR},
	{">=",		PT_LOGIC_GEQUAL},
	{"<=",		PT_LOGIC_LEQUAL},
	{"==",		PT_LOGIC_EQUAL},
	{"!=",		PT_LOGIC_NOTEQUAL},
	{"*=",		PT_MUL_ASSIGN},
	{"/=",		PT_DIV_ASSIGN},
	{"%=",		PT_MOD_ASSIGN},
	{"+=",		PT_ADD_ASSIGN},
	{"-=",		PT_SUB_ASSIGN},
	{"++",		PT_INCREMENT},
	{"--",		PT_DECREMENT},
	{"&=",		PT_BINARY_AND_ASSIGN},
	{"|=",		PT_BINARY_OR_ASSIGN},
	{"^=",		PT_BINARY_XOR_ASSIGN},
	{">>",		PT_RSHIFT},
	{"<<",		PT_LSHIFT},
	{"->",		PT_POINTER_REFERENCE},
	{"::",		PT_CPP_1},
	{".*",		PT_CPP_2},
	{"*",		PT_MUL},
	{"/",		PT_DIV},
	{"%",		PT_MOD},
	{"+",		PT_ADD},
	{"-",		PT_SUB},
	{"=",		PT_ASSIGN},
	{"&",		PT_BINARY_AND},
	{"|",		PT_BINARY_OR},
	{"^",		PT_BINARY_XOR},
	{"~",		PT_BINARY_NOT},
	{"!",		PT_LOGIC_NOT},
	{">",		PT_LOGIC_GREATER},
	{"<",		PT_LOGIC_LESS},
	{".",		PT_REFERENCE},
	{":",		PT_COLON},
	{",",		PT_COMMA},
	{";",		PT_SEMICOLON},
	{"?",		PT_QUESTION_MARK},
	{"{",		PT_BRACE_OPEN},
	{"}",		PT_BRACE_CLOSE},
	{"[",		PT_BRACKET_OPEN},
	{"]",		PT_BRACKET_CLOSE},
	{"(",		PT_PARENTHESIS_OPEN},
	{")",		PT_PARENTHESIS_CLOSE},
	{"#",		PT_PRECOMPILER},
	{"$",		PT_DOLLAR},
	{"\\",		PT_BACKSLASH},
	{NULL,		0}
};


/*
 ==================
 PS_NumberValue
 ==================
*/
static void PS_NumberValue (token_t *token){

	char	*string = token->string;
	double	value = 0.0, fraction = 0.1, power = 1.0;
	int		exponent = 0;
	bool	negative = false;
	int		i;

	token->integerValue = 0;
	token->floatValue = 0.0f;

	if (token->type != TT_NUMBER)
		return;

	// If a binary number
	if (token->subType & NT_BINARY){
		string += 2;

		while (*string)
			token->integerValue = (token->integerValue << 1) + (*string++ - '0');

		token->floatValue = (float)token->integerValue;

		return;
	}

	// If an octal number
	if (token->subType & NT_OCTAL){
		string += 1;

		while (*string)
			token->integerValue = (token->integerValue << 3) + (*string++ - '0');

		token->floatValue = (float)token->integerValue;

		return;
	}

	// If a decimal number
	if (token->subType & NT_DECIMAL){
		// If an integer number
		if (token->subType & NT_INTEGER){
			while (*string)
				token->integerValue = token->integerValue * 10 + (*string++ - '0');

			token->floatValue = (float)token->integerValue;

			return;
		}

		// If a floating point number
		if (token->subType & NT_FLOAT){
			while (*string && *string != '.' && *string != 'e' && *string != 'E')
				value = value * 10.0 + (double)(*string++ - '0');

			if (*string == '.'){
				string++;

				while (*string && *string != 'e' && *string != 'E'){
					value = value + (double)(*string++ - '0') * fraction;
					fraction *= 0.1;
				}
			}

			if (*string == 'e' || *string == 'E'){
				string++;

				if (*string == '+'){
					string++;

					negative = false;
				}
				else if (*string == '-'){
					string++;

					negative = true;
				}

				while (*string)
					exponent = exponent * 10 + (*string++ - '0');

				for (i = 0; i < exponent; i++)
					power *= 10.0;

				if (negative)
					value /= power;
				else
					value *= power;
			}

			token->integerValue = (int)value;
			token->floatValue = (float)value;

			return;
		}

		return;
	}

	// If a hexadecimal number
	if (token->subType & NT_HEXADECIMAL){
		string += 2;

		while (*string){
			if (*string >= 'a' && *string <= 'f')
				token->integerValue = (token->integerValue << 4) + (*string++ - 'a' + 10);
			else if (*string >= 'A' && *string <= 'F')
				token->integerValue = (token->integerValue << 4) + (*string++ - 'A' + 10);
			else
				token->integerValue = (token->integerValue << 4) + (*string++ - '0');
		}

		token->floatValue = (float)token->integerValue;

		return;
	}
}

/*
 ==================
 PS_ReadWhiteSpace
 ==================
*/
static bool PS_ReadWhiteSpace (script_t *script){

	while (1){
		// Skip whitespace
		while (Str_CharIsWhiteSpace(*script->text)){
			if (!*script->text)
				return false;

			if (*script->text == '\n')
				script->line++;

			script->text++;
		}

		// Skip // comments
		if (*script->text == '/' && script->text[1] == '/'){
			while (*script->text && *script->text != '\n')
				script->text++;

			continue;
		}

		// Skip /* */ comments
		if (*script->text == '/' && script->text[1] == '*'){
			script->text += 2;

			while (*script->text && (*script->text != '*' || script->text[1] != '/')){
				if (*script->text == '\n')
					script->line++;

				script->text++;
			}

			if (*script->text)
				script->text += 2;

			continue;
		}

		// An actual token
		break;
	}

	return true;
}

/*
 ==================
 PS_ReadEscapeChar
 ==================
*/
static bool PS_ReadEscapeChar (script_t *script, char *ch){

	int		value;

	script->text++;

	switch (*script->text){
	case 'a':
		*ch = '\a';
		break;
	case 'b':
		*ch = '\b';
		break;
	case 'f':
		*ch = '\f';
		break;
	case 'n':
		*ch = '\n';
		break;
	case 'r':
		*ch = '\r';
		break;
	case 't':
		*ch = '\t';
		break;
	case 'v':
		*ch = '\v';
		break;
	case '\"':
		*ch = '\"';
		break;
	case '\'':
		*ch = '\'';
		break;
	case '\\':
		*ch = '\\';
		break;
	case '\?':
		*ch = '\?';
		break;
	case 'x':
		script->text++;

		for (value = 0; ; script->text++){
			if (*script->text >= 'a' && *script->text <= 'f')
				value = (value << 4) + (*script->text - 'a' + 10);
			else if (*script->text >= 'A' && *script->text <= 'F')
				value = (value << 4) + (*script->text - 'A' + 10);
			else if (*script->text >= '0' && *script->text <= '9')
				value = (value << 4) + (*script->text - '0');
			else
				break;
		}

		script->text--;

		if (value > 0xFF){
			PS_ScriptError(script, "too large value in escape character");
			return false;
		}

		*ch = value;
		break;
	default:
		if (*script->text < '0' || *script->text > '9'){
			PS_ScriptError(script, "unknown escape character");
			return false;
		}

		for (value = 0; ; script->text++){
			if (*script->text >= '0' && *script->text <= '9')
				value = value * 10 + (*script->text - '0');
			else
				break;
		}

		script->text--;

		if (value > 0xFF){
			PS_ScriptError(script, "too large value in escape character");
			return false;
		}

		*ch = value;
		break;
	}

	script->text++;

	return true;
}

/*
 ==================
 PS_ReadPrimitive
 ==================
*/
static bool PS_ReadPrimitive (script_t *script, token_t *token){

	token->type = TT_PRIMITIVE;
	token->subType = 0;

	token->line = script->line;
	token->linesCrossed = script->line - script->lastLine;

	while (1){
		if (Str_CharIsWhiteSpace(*script->text))
			break;

		if (token->length == MAX_STRING_LENGTH - 1){
			PS_ScriptError(script, "primitive longer than MAX_STRING_LENGTH");
			return false;
		}

		token->string[token->length++] = *script->text++;
	}

	token->string[token->length] = 0;

	PS_NumberValue(token);

	return true;
}

/*
 ==================
 PS_ReadString
 ==================
*/
static bool PS_ReadString (script_t *script, token_t *token){

	const char	*text;
	int			line;

	token->type = TT_STRING;
	token->subType = 0;

	token->line = script->line;
	token->linesCrossed = script->line - script->lastLine;

	script->text++;

	while (1){
		if (!*script->text){
			PS_ScriptError(script, "missing trailing quote");
			return false;
		}

		if (*script->text == '\n'){
			PS_ScriptError(script, "newline inside string");
			return false;
		}

		if (*script->text == '\"'){
			script->text++;

			if (!(script->flags & SF_ALLOWSTRINGCONCAT))
				break;

			text = script->text;
			line = script->line;

			if (PS_ReadWhiteSpace(script)){
				if (*script->text == '\"'){
					script->text++;
					continue;
				}
			}

			script->text = text;
			script->line = line;

			break;
		}

		if (token->length == MAX_STRING_LENGTH - 1){
			PS_ScriptError(script, "string longer than MAX_STRING_LENGTH");
			return false;
		}

		if (*script->text == '\\' && (script->flags & SF_ALLOWESCAPECHARS)){
			if (!PS_ReadEscapeChar(script, &token->string[token->length]))
				return false;

			token->length++;
			continue;
		}

		token->string[token->length++] = *script->text++;
	}

	token->string[token->length] = 0;

	PS_NumberValue(token);

	return true;
}

/*
 ==================
 PS_ReadLiteral
 ==================
*/
static bool PS_ReadLiteral (script_t *script, token_t *token){

	const char	*text;
	int			line;

	token->type = TT_LITERAL;
	token->subType = 0;

	token->line = script->line;
	token->linesCrossed = script->line - script->lastLine;

	script->text++;

	while (1){
		if (!*script->text){
			PS_ScriptError(script, "missing trailing quote");
			return false;
		}

		if (*script->text == '\n'){
			PS_ScriptError(script, "newline inside literal");
			return false;
		}

		if (*script->text == '\''){
			script->text++;

			if (!(script->flags & SF_ALLOWSTRINGCONCAT))
				break;

			text = script->text;
			line = script->line;

			if (PS_ReadWhiteSpace(script)){
				if (*script->text == '\''){
					script->text++;
					continue;
				}
			}

			script->text = text;
			script->line = line;

			break;
		}

		if (token->length == MAX_STRING_LENGTH - 1){
			PS_ScriptError(script, "literal longer than MAX_STRING_LENGTH");
			return false;
		}

		if (*script->text == '\\' && (script->flags & SF_ALLOWESCAPECHARS)){
			if (!PS_ReadEscapeChar(script, &token->string[token->length]))
				return false;

			token->length++;
			continue;
		}

		token->string[token->length++] = *script->text++;
	}

	if (!(script->flags & SF_ALLOWMULTICHARLITERALS)){
		if (token->length != 1){
			PS_ScriptError(script, "literal is not one character long");
			return false;
		}
	}

	token->string[token->length] = 0;

	PS_NumberValue(token);

	return true;
}

/*
 ==================
 PS_ReadNumber
 ==================
*/
static bool PS_ReadNumber (script_t *script, token_t *token){

	bool	hasDot = false;
	int		c;

	token->type = TT_NUMBER;
	token->subType = 0;

	token->line = script->line;
	token->linesCrossed = script->line - script->lastLine;

	if (*script->text == '0' && script->text[1] != '.'){
		if (script->text[1] == 'b' || script->text[1] == 'B'){
			token->string[token->length++] = *script->text++;
			token->string[token->length++] = *script->text++;

			while (1){
				c = *script->text;

				if (c < '0' || c > '1')
					break;

				if (token->length == MAX_STRING_LENGTH - 1){
					PS_ScriptError(script, "binary number longer than MAX_STRING_LENGTH");
					return false;
				}

				token->string[token->length++] = *script->text++;
			}

			token->subType |= (NT_BINARY | NT_INTEGER);
		}
		else if (script->text[1] == 'x' || script->text[1] == 'X'){
			token->string[token->length++] = *script->text++;
			token->string[token->length++] = *script->text++;

			while (1){
				c = *script->text;

				if ((c < 'a' || c > 'f') && (c < 'A' || c > 'F') && (c < '0' || c > '9'))
					break;

				if (token->length == MAX_STRING_LENGTH - 1){
					PS_ScriptError(script, "hexadecimal number longer than MAX_STRING_LENGTH");
					return false;
				}

				token->string[token->length++] = *script->text++;
			}

			token->subType |= (NT_HEXADECIMAL | NT_INTEGER);
		}
		else {
			token->string[token->length++] = *script->text++;

			while (1){
				c = *script->text;

				if (c < '0' || c > '7')
					break;

				if (token->length == MAX_STRING_LENGTH - 1){
					PS_ScriptError(script, "octal number longer than MAX_STRING_LENGTH");
					return false;
				}

				token->string[token->length++] = *script->text++;
			}

			token->subType |= (NT_OCTAL | NT_INTEGER);
		}

		token->string[token->length] = 0;

		PS_NumberValue(token);

		return true;
	}

	while (1){
		c = *script->text;

		if (c == '.'){
			if (hasDot)
				break;

			hasDot = true;
		}
		else if (c < '0' || c > '9')
			break;

		if (token->length == MAX_STRING_LENGTH - 1){
			PS_ScriptError(script, "number longer than MAX_STRING_LENGTH");
			return false;
		}

		token->string[token->length++] = *script->text++;
	}

	if (hasDot || (*script->text == 'e' || *script->text == 'E')){
		token->subType |= (NT_DECIMAL | NT_FLOAT);

		if (*script->text == 'e' || *script->text == 'E'){
			if (token->length == MAX_STRING_LENGTH - 1){
				PS_ScriptError(script, "number longer than MAX_STRING_LENGTH");
				return false;
			}

			token->string[token->length++] = *script->text++;

			if (*script->text == '+' || *script->text == '-'){
				if (token->length == MAX_STRING_LENGTH - 1){
					PS_ScriptError(script, "number longer than MAX_STRING_LENGTH");
					return false;
				}

				token->string[token->length++] = *script->text++;
			}

			while (1){
				c = *script->text;

				if (c < '0' || c > '9')
					break;

				if (token->length == MAX_STRING_LENGTH - 1){
					PS_ScriptError(script, "number longer than MAX_STRING_LENGTH");
					return false;
				}

				token->string[token->length++] = *script->text++;
			}
		}

		if (*script->text == 'f' || *script->text == 'F'){
			script->text++;

			token->subType |= NT_SINGLE;
		}
		else if (*script->text == 'l' || *script->text == 'L'){
			script->text++;

			token->subType |= NT_EXTENDED;
		}
		else
			token->subType |= NT_DOUBLE;
	}
	else {
		token->subType |= (NT_DECIMAL | NT_INTEGER);

		if (*script->text == 'u' || *script->text == 'U'){
			script->text++;

			token->subType |= NT_UNSIGNED;

			if (*script->text == 'l' || *script->text == 'L'){
				script->text++;

				token->subType |= NT_LONG;
			}
		}
		else if (*script->text == 'l' || *script->text == 'L'){
			script->text++;

			token->subType |= NT_LONG;

			if (*script->text == 'u' || *script->text == 'U'){
				script->text++;

				token->subType |= NT_UNSIGNED;
			}
		}
	}

	token->string[token->length] = 0;

	PS_NumberValue(token);

	return true;
}

/*
 ==================
 PS_ReadName
 ==================
*/
static bool PS_ReadName (script_t *script, token_t *token){

	int		c;

	token->type = TT_NAME;
	token->subType = 0;

	token->line = script->line;
	token->linesCrossed = script->line - script->lastLine;

	while (1){
		if (*script->text == '/' && (script->text[1] == '/' || script->text[1] == '*'))
			break;

		c = *script->text;

		if (script->flags & SF_ALLOWPATHNAMES){
			if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9') && c != '_' && c != ':' && c != '/' && c != '\\' && c != '.')
				break;
		}
		else {
			if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9') && c != '_')
				break;
		}

		if (token->length == MAX_STRING_LENGTH - 1){
			PS_ScriptError(script, "name longer than MAX_STRING_LENGTH");
			return false;
		}

		token->string[token->length++] = *script->text++;
	}

	token->string[token->length] = 0;

	PS_NumberValue(token);

	return true;
}

/*
 ==================
 PS_ReadPunctuation
 ==================
*/
static bool PS_ReadPunctuation (script_t *script, token_t *token){

	punctuation_t	*punctuation;
	int				i, length;

	for (i = 0; script->punctuations[i].name; i++){
		punctuation = &script->punctuations[i];

		for (length = 0; punctuation->name[length] && script->text[length]; length++){
			if (punctuation->name[length] != script->text[length])
				break;
		}

		if (!punctuation->name[length]){
			script->text += length;

			token->type = TT_PUNCTUATION;
			token->subType = punctuation->type;

			token->line = script->line;
			token->linesCrossed = script->line - script->lastLine;

			for (i = 0; i < length; i++){
				if (token->length == MAX_STRING_LENGTH - 1){
					PS_ScriptError(script, "punctuation longer than MAX_STRING_LENGTH");
					return false;
				}

				token->string[token->length++] = punctuation->name[i];
			}

			token->string[token->length] = 0;

			PS_NumberValue(token);

			return true;
		}
	}

	return false;
}

/*
 ==================
 PS_ReadToken
 ==================
*/
bool PS_ReadToken (script_t *script, token_t *token){

	// If there is a token available (from PS_UnreadToken)
	if (script->tokenAvailable){
		script->tokenAvailable = false;
		Mem_Copy(token, &script->token, sizeof(token_t));

		return true;
	}

	// Backup text and line
	script->lastText = script->text;
	script->lastLine = script->line;

	// Clear token
	Mem_Fill(token, 0, sizeof(token_t));

	// Skip whitespace and comments
	if (!PS_ReadWhiteSpace(script))
		return false;

	// If we just want to parse a primitive separated by spaces
	if (script->flags & SF_PARSEPRIMITIVES){
		// If it is a string
		if (*script->text == '\"'){
			if (PS_ReadString(script, token))
				return true;
		}
		// If it is a literal
		else if (*script->text == '\''){
			if (PS_ReadLiteral(script, token))
				return true;
		}
		// Check for a primitive
		else {
			if (PS_ReadPrimitive(script, token))
				return true;
		}
	}
	// If it is a string
	else if (*script->text == '\"'){
		if (PS_ReadString(script, token))
			return true;
	}
	// If it is a literal
	else if (*script->text == '\''){
		if (PS_ReadLiteral(script, token))
			return true;
	}
	// If it is a number
	else if ((*script->text >= '0' && *script->text <= '9') || (*script->text == '.' && (script->text[1] >= '0' && script->text[1] <= '9'))){
		if (PS_ReadNumber(script, token))
			return true;
	}
	// If it is a name
	else if ((*script->text >= 'a' && *script->text <= 'z') || (*script->text >= 'A' && *script->text <= 'Z') || *script->text == '_'){
		if (PS_ReadName(script, token))
			return true;
	}
	// Check for a punctuation
	else {
		if (PS_ReadPunctuation(script, token))
			return true;
	}

	// Couldn't parse a token
	PS_ScriptError(script, "couldn't read token");

	Mem_Fill(token, 0, sizeof(token_t));

	return false;
}

/*
 ==================
 PS_UnreadToken
 ==================
*/
void PS_UnreadToken (script_t *script, const token_t *token){

	script->tokenAvailable = true;
	Mem_Copy(&script->token, token, sizeof(token_t));
}

/*
 ==================
 PS_ReadBool
 ==================
*/
bool PS_ReadBool (script_t *script, bool *value){

	token_t	token;

	if (!PS_ReadToken(script, &token)){
		PS_ScriptError(script, "couldn't read expected bool");
		return false;
	}

	if (!Str_ICompare(token.string, "true"))
		*value = true;
	else if (!Str_ICompare(token.string, "false"))
		*value = false;
	else {
		if (token.type != TT_NUMBER || !(token.subType & NT_INTEGER)){
			PS_ScriptError(script, "expected integer value, found '%s' instead", token.string);
			return false;
		}

		*value = (token.integerValue != 0);
	}

	return true;
}

/*
 ==================
 PS_ReadInteger
 ==================
*/
bool PS_ReadInteger (script_t *script, int *value){

	token_t	token;

	if (!PS_ReadToken(script, &token)){
		PS_ScriptError(script, "couldn't read expected integer");
		return false;
	}

	if (token.type == TT_PUNCTUATION && !Str_Compare(token.string, "-")){
		if (!PS_ExpectTokenType(script, &token, TT_NUMBER, NT_INTEGER))
			return false;

		*value = -token.integerValue;

		return true;
	}

	if (token.type != TT_NUMBER || !(token.subType & NT_INTEGER)){
		PS_ScriptError(script, "expected integer value, found '%s' instead", token.string);
		return false;
	}

	*value = token.integerValue;

	return true;
}

/*
 ==================
 PS_ReadFloat
 ==================
*/
bool PS_ReadFloat (script_t *script, float *value){

	token_t	token;

	if (!PS_ReadToken(script, &token)){
		PS_ScriptError(script, "couldn't read expected float");
		return false;
	}

	if (token.type == TT_PUNCTUATION && !Str_Compare(token.string, "-")){
		if (!PS_ExpectTokenType(script, &token, TT_NUMBER, 0))
			return false;

		*value = -token.floatValue;

		return true;
	}

	if (token.type != TT_NUMBER){
		PS_ScriptError(script, "expected float value, found '%s' instead", token.string);
		return false;
	}

	*value = token.floatValue;

	return true;
}

/*
 ==================
 PS_ReadMatrix1D
 ==================
*/
bool PS_ReadMatrix1D (script_t *script, int x, float *matrix){

	token_t	token;
	int		i;

	if (!PS_ExpectTokenString(script, &token, "(", true))
		return false;

	for (i = 0; i < x; i++){
		if (!PS_ReadFloat(script, &matrix[i]))
			return false;
	}

	if (!PS_ExpectTokenString(script, &token, ")", true))
		return false;

	return true;
}

/*
 ==================
 PS_ReadMatrix2D
 ==================
*/
bool PS_ReadMatrix2D (script_t *script, int y, int x, float *matrix){

	token_t	token;
	int		i;

	if (!PS_ExpectTokenString(script, &token, "(", true))
		return false;

	for (i = 0; i < y; i++){
		if (!PS_ReadMatrix1D(script, x, matrix + i * x))
			return false;
	}

	if (!PS_ExpectTokenString(script, &token, ")", true))
		return false;

	return true;
}

/*
 ==================
 PS_ReadMatrix3D
 ==================
*/
bool PS_ReadMatrix3D (script_t *script, int z, int y, int x, float *matrix){

	token_t	token;
	int		i;

	if (!PS_ExpectTokenString(script, &token, "(", true))
		return false;

	for (i = 0; i < z; i++){
		if (!PS_ReadMatrix2D(script, y, x, matrix + i * x * y))
			return false;
	}

	if (!PS_ExpectTokenString(script, &token, ")", true))
		return false;

	return true;
}

/*
 ==================
 PS_ExpectTokenString
 ==================
*/
bool PS_ExpectTokenString (script_t *script, token_t *token, const char *string, bool caseSensitive){

	if (!PS_ReadToken(script, token)){
		PS_ScriptError(script, "couldn't read expected '%s'", string);
		return false;
	}

	if (caseSensitive){
		if (Str_Compare(token->string, string)){
			PS_ScriptError(script, "expected '%s', found '%s' instead", string, token->string);
			return false;
		}
	}
	else {
		if (Str_ICompare(token->string, string)){
			PS_ScriptError(script, "expected '%s', found '%s' instead", string, token->string);
			return false;
		}
	}

	return true;
}

/*
 ==================
 PS_ExpectTokenType
 ==================
*/
bool PS_ExpectTokenType (script_t *script, token_t *token, tokenType_t type, int subType){

	char	string[64];
	int		i;

	if (!PS_ReadToken(script, token)){
		PS_ScriptError(script, "couldn't read expected token");
		return false;
	}

	if (token->type != type){
		switch (type){
		case TT_PRIMITIVE:
			Str_Copy(string, "primitive", sizeof(string));
			break;
		case TT_STRING:
			Str_Copy(string, "string", sizeof(string));
			break;
		case TT_LITERAL:
			Str_Copy(string, "literal", sizeof(string));
			break;
		case TT_NUMBER:
			Str_Copy(string, "number", sizeof(string));
			break;
		case TT_NAME:
			Str_Copy(string, "name", sizeof(string));
			break;
		case TT_PUNCTUATION:
			Str_Copy(string, "punctuation", sizeof(string));
			break;
		default:
			Str_Copy(string, "unknown type", sizeof(string));
			break;
		}

		PS_ScriptError(script, "expected a %s, found '%s' instead", string, token->string);
		return false;
	}

	if (token->type == TT_NUMBER){
		if ((token->subType & subType) != subType){
			if (subType & NT_BINARY)
				Str_Copy(string, "binary", sizeof(string));
			if (subType & NT_OCTAL)
				Str_Copy(string, "octal", sizeof(string));
			if (subType & NT_DECIMAL)
				Str_Copy(string, "decimal", sizeof(string));
			if (subType & NT_HEXADECIMAL)
				Str_Copy(string, "hexadecimal", sizeof(string));

			if (subType & NT_UNSIGNED)
				Str_Append(string, " unsigned", sizeof(string));
			if (subType & NT_LONG)
				Str_Append(string, " long", sizeof(string));
			if (subType & NT_SINGLE)
				Str_Append(string, " single", sizeof(string));
			if (subType & NT_DOUBLE)
				Str_Append(string, " double", sizeof(string));
			if (subType & NT_EXTENDED)
				Str_Append(string, " extended", sizeof(string));
			if (subType & NT_INTEGER)
				Str_Append(string, " integer", sizeof(string));
			if (subType & NT_FLOAT)
				Str_Append(string, " float", sizeof(string));

			PS_ScriptError(script, "expected %s, found '%s' instead", string, token->string);
			return false;
		}
	}
	else if (token->type == TT_PUNCTUATION){
		if (token->subType != subType){
			for (i = 0; script->punctuations[i].name; i++){
				if (script->punctuations[i].type == subType){
					PS_ScriptError(script, "expected '%s', found '%s' instead", script->punctuations[i].name, token->string);
					return false;
				}
			}

			PS_ScriptError(script, "expected unknown punctuation, found '%s' instead", token->string);
			return false;
		}
	}

	return true;
}

/*
 ==================
 PS_CheckTokenString
 ==================
*/
bool PS_CheckTokenString (script_t *script, token_t *token, const char *string, bool caseSensitive){

	if (!PS_ReadToken(script, token))
		return false;

	if (caseSensitive){
		if (!Str_Compare(token->string, string))
			return true;
	}
	else {
		if (!Str_ICompare(token->string, string))
			return true;
	}

	script->text = script->lastText;
	script->line = script->lastLine;

	return false;
}

/*
 ==================
 PS_CheckTokenType
 ==================
*/
bool PS_CheckTokenType (script_t *script, token_t *token, tokenType_t type, int subType){

	if (!PS_ReadToken(script, token))
		return false;

	if (token->type == type){
		if (token->type == TT_NUMBER){
			if ((token->subType & subType) == subType)
				return true;
		}
		else if (token->type == TT_PUNCTUATION){
			if (token->subType == subType)
				return true;
		}
		else
			return true;
	}

	script->text = script->lastText;
	script->line = script->lastLine;

	return false;
}

/*
 ==================
 PS_PeekTokenString
 ==================
*/
bool PS_PeekTokenString (script_t *script, token_t *token, const char *string, bool caseSensitive){

	if (!PS_ReadToken(script, token))
		return false;

	script->text = script->lastText;
	script->line = script->lastLine;

	if (caseSensitive){
		if (!Str_Compare(token->string, string))
			return true;
	}
	else {
		if (!Str_ICompare(token->string, string))
			return true;
	}

	return false;
}

/*
 ==================
 PS_PeekTokenType
 ==================
*/
bool PS_PeekTokenType (script_t *script, token_t *token, tokenType_t type, int subType){

	if (!PS_ReadToken(script, token))
		return false;

	script->text = script->lastText;
	script->line = script->lastLine;

	if (token->type == type){
		if (token->type == TT_NUMBER){
			if ((token->subType & subType) == subType)
				return true;
		}
		else if (token->type == TT_PUNCTUATION){
			if (token->subType == subType)
				return true;
		}
		else
			return true;
	}

	return false;
}

/*
 ==================
 PS_TokenAvailable
 ==================
*/
bool PS_TokenAvailable (script_t *script, bool crossLines){

	token_t	token;

	if (!PS_ReadToken(script, &token))
		return false;

	script->text = script->lastText;
	script->line = script->lastLine;

	if (token.linesCrossed)
		return crossLines;

	return true;
}

/*
 ==================
 PS_SkipUntilString
 ==================
*/
bool PS_SkipUntilString (script_t *script, const char *string, bool caseSensitive){

	token_t	token;

	while (1){
		if (!PS_ReadToken(script, &token)){
			PS_ScriptError(script, "couldn't find expected '%s'", string);
			return false;
		}

		if (caseSensitive){
			if (!Str_Compare(token.string, string))
				break;
		}
		else {
			if (!Str_ICompare(token.string, string))
				break;
		}
	}

	return true;
}

/*
 ==================
 PS_SkipRestOfLine
 ==================
*/
bool PS_SkipRestOfLine (script_t *script){

	token_t	token;

	while (1){
		if (!PS_ReadToken(script, &token)){
			PS_ScriptError(script, "couldn't find newline");
			return false;
		}

		if (token.linesCrossed){
			script->text = script->lastText;
			script->line = script->lastLine;

			break;
		}
	}

	return true;
}

/*
 ==================
 PS_SkipBracedSection
 ==================
*/
bool PS_SkipBracedSection (script_t *script, int depth){

	token_t	token;

	while (1){
		if (!PS_ReadToken(script, &token)){
			if (depth)
				PS_ScriptError(script, "couldn't find expected '}'");
			else
				PS_ScriptError(script, "couldn't find expected '{'");

			return false;
		}

		if (!Str_Compare(token.string, "{"))
			depth++;
		else if (!Str_Compare(token.string, "}")){
			depth--;

			if (depth == 0)
				break;
			else if (depth < 0){
				PS_ScriptError(script, "unexpected '}'");
				return false;
			}
		}
	}

	return true;
}

/*
 ==================
 PS_ScriptWarning
 ==================
*/
void PS_ScriptWarning (script_t *script, const char *fmt, ...){

	char	message[MAX_PRINT_MESSAGE];
	va_list	argPtr;

	if (script->flags & SF_NOWARNINGS)
		return;

	va_start(argPtr, fmt);
	Str_VSPrintf(message, sizeof(message), fmt, argPtr);
	va_end(argPtr);

	Com_Printf(S_COLOR_YELLOW "WARNING: source '%s', line %i: %s\n", script->name, script->line, message);
}

/*
 ==================
 PS_ScriptError
 ==================
*/
void PS_ScriptError (script_t *script, const char *fmt, ...){

	char	message[MAX_PRINT_MESSAGE];
	va_list	argPtr;

	if (script->flags & SF_NOERRORS)
		return;

	va_start(argPtr, fmt);
	Str_VSPrintf(message, sizeof(message), fmt, argPtr);
	va_end(argPtr);

	if (!(script->flags & SF_NOFATALERRORS))
		Com_Error(ERR_DROP, "Script error: source '%s', line %i: %s", script->name, script->line, message);

	Com_Printf(S_COLOR_RED "ERROR: source '%s', line %i: %s\n", script->name, script->line, message);
}

/*
 ==================
 PS_ResetScript
 ==================
*/
void PS_ResetScript (script_t *script){

	script->text = script->buffer;
	script->lastText = script->buffer;

	script->line = script->start;
	script->lastLine = script->start;

	script->tokenAvailable = false;
	Mem_Fill(&script->token, 0, sizeof(token_t));
}

/*
 ==================
 PS_EndOfScript
 ==================
*/
bool PS_EndOfScript (script_t *script){

	if (script->text >= script->buffer + script->size)
		return true;

	return false;
}

/*
 ==================
 PS_SetScriptFlags
 ==================
*/
void PS_SetScriptFlags (script_t *script, int flags){

	script->flags = flags;
}

/*
 ==================
 PS_SetPunctuationsTable
 ==================
*/
void PS_SetPunctuationsTable (script_t *script, punctuation_t *punctuationsTable){

	if (punctuationsTable)
		script->punctuations = punctuationsTable;
	else
		script->punctuations = ps_punctuationsTable;
}

/*
 ==================
 PS_LoadScriptFile
 ==================
*/
script_t *PS_LoadScriptFile (const char *name){

	script_t	*script;
	char		*buffer;
	int			size;

	size = FS_ReadFile(name, (void **)&buffer);
	if (!buffer)
		return NULL;	// Let the caller handle this error

	// Allocate the script
	script = (script_t *)Mem_ClearedAlloc(sizeof(script_t), TAG_TEMPORARY);

	// Fill it in
	Str_Copy(script->name, name, sizeof(script->name));
	script->buffer = buffer;
	script->size = size;
	script->start = 1;
	script->allocated = true;
	script->text = buffer;
	script->lastText = buffer;
	script->line = 1;
	script->lastLine = 1;
	script->flags = 0;
	script->punctuations = ps_punctuationsTable;
	script->tokenAvailable = false;
	Mem_Fill(&script->token, 0, sizeof(token_t));

	return script;
}

/*
 ==================
 PS_LoadScriptMemory
 ==================
*/
script_t *PS_LoadScriptMemory (const char *name, const char *buffer, int size, int line){

	script_t	*script;

	if (!buffer || size < 0)
		return NULL;	// Let the caller handle this error

	// Allocate the script
	script = (script_t *)Mem_ClearedAlloc(sizeof(script_t), TAG_TEMPORARY);

	// Fill it in
	Str_Copy(script->name, name, sizeof(script->name));
	script->buffer = buffer;
	script->size = size;
	script->start = line;
	script->allocated = false;
	script->text = buffer;
	script->lastText = buffer;
	script->line = line;
	script->lastLine = line;
	script->flags = 0;
	script->punctuations = ps_punctuationsTable;
	script->tokenAvailable = false;
	Mem_Fill(&script->token, 0, sizeof(token_t));

	return script;
}

/*
 ==================
 PS_FreeScript
 ==================
*/
void PS_FreeScript (script_t *script){

	if (!script)
		Com_Error(ERR_FATAL, "PS_FreeScript: NULL script");

	if (script->allocated)
		FS_FreeFile(script->buffer);

	Mem_Free(script);
}