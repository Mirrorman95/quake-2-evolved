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
// r_material.c - Material definition parsing and loading
//

// TODO:
// - R_ParseGlobalSort might need some more sort flags
// - R_ParseGlobalDecalInfo might be wrong
// - add Quake 3 sky effects stuff?


#include "r_local.h"


#define MATERIALS_HASH_SIZE			(MAX_MATERIALS >> 2)

typedef struct materialDef_s {
	char					name[MAX_PATH_LENGTH];
	char *					text;
	int						length;

	char					source[MAX_PATH_LENGTH];
	int						line;

	materialType_t			type;
	uint					surfaceParm;

	struct materialDef_s *	nextHash;
} materialDef_t;

static material_t			r_parseMaterial;
static stage_t				r_parseStages[MAX_STAGES];
static expOp_t				r_parseExpressionOps[MAX_EXPRESSION_OPS];
static float				r_parseExpressionRegisters[MAX_EXPRESSION_REGISTERS];

static materialDef_t *		r_materialDefsHashTable[MATERIALS_HASH_SIZE];

static material_t *			r_materialsHashTable[MATERIALS_HASH_SIZE];
static material_t *			r_materials[MAX_MATERIALS];
static int					r_numMaterials;


/*
 ==============================================================================

 MATERIAL EXPRESSION PARSING

 ==============================================================================
*/

#define MAX_EXPRESSION_VALUES		32
#define MAX_EXPRESSION_OPERATORS	32

#define MAX_EXPRESSION_DEPTH		32

typedef struct expValue_s {
	int						expressionRegister;

	int						brackets;
	int						parentheses;

	struct expValue_s *		prev;
	struct expValue_s *		next;
} expValue_t;

typedef struct expOperator_s {
	opType_t				type;
	int						priority;

	int						brackets;
	int						parentheses;

	struct expOperator_s *	prev;
	struct expOperator_s *	next;
} expOperator_t;

typedef struct {
	int						numValues;
	expValue_t				values[MAX_EXPRESSION_VALUES];
	expValue_t *			firstValue;
	expValue_t *			lastValue;

	int						numOperators;
	expOperator_t			operators[MAX_EXPRESSION_OPERATORS];
	expOperator_t *			firstOperator;
	expOperator_t *			lastOperator;

	int						brackets;
	int						parentheses[MAX_EXPRESSION_DEPTH];

	int						lastRegister;
} expression_t;

static bool	R_ParseExpressionValue (script_t *script, material_t *material, expression_t *expression);
static bool	R_ParseExpressionOperator (script_t *script, material_t *material, expression_t *expression);


/*
 ==================
 R_AddExpressionConstant
 ==================
*/
static bool R_AddExpressionConstant (material_t *material, expression_t *expression, float value){

	int		i;

	// Try to reuse an existing constant register
	if (value == 1.0f){
		expression->lastRegister = EXP_REGISTER_CONSTANT_ONE;
		return true;
	}

	if (value == 0.0f){
		expression->lastRegister = EXP_REGISTER_CONSTANT_ZERO;
		return true;
	}

	for (i = EXP_REGISTER_NUM_PREDEFINED; i < material->numRegisters; i++){
		if (material->expressionRegisters[i] == value){
			expression->lastRegister = i;
			return true;
		}
	}

	// Add a constant register
	if (material->numRegisters == MAX_EXPRESSION_REGISTERS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_REGISTERS hit in material '%s'\n", material->name);
		return false;
	}

	expression->lastRegister = material->numRegisters;

	material->expressionRegisters[material->numRegisters++] = value;

	return true;
}

/*
 ==================
 R_AddExpressionTemporary
 ==================
*/
static bool R_AddExpressionTemporary (material_t *material, expression_t *expression){

	// Add a temporary register
	if (material->numRegisters == MAX_EXPRESSION_REGISTERS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_REGISTERS hit in material '%s'\n", material->name);
		return false;
	}

	expression->lastRegister = material->numRegisters;

	material->expressionRegisters[material->numRegisters++] = 0.0f;

	return true;
}

/*
 ==================
 R_AddExpressionValue
 ==================
*/
static bool R_AddExpressionValue (material_t *material, expression_t *expression, int expressionRegister){

	expValue_t		*v;

	if (expression->numValues == MAX_EXPRESSION_VALUES){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_VALUES hit for expression in material '%s'\n", material->name);
		return false;
	}

	v = &expression->values[expression->numValues++];

	v->expressionRegister = expressionRegister;
	v->brackets = expression->brackets;
	v->parentheses = expression->parentheses[expression->brackets];
	v->next = NULL;
	v->prev = expression->lastValue;

	if (expression->lastValue)
		expression->lastValue->next = v;
	else
		expression->firstValue = v;

	expression->lastValue = v;

	return true;
}

/*
 ==================
 R_AddExpressionOperator
 ==================
*/
static bool R_AddExpressionOperator (material_t *material, expression_t *expression, opType_t type, int priority){

	expOperator_t	*o;

	if (expression->numOperators == MAX_EXPRESSION_OPERATORS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_OPERATORS hit for expression in material '%s'\n", material->name);
		return false;
	}

	o = &expression->operators[expression->numOperators++];

	o->type = type;
	o->priority = priority;
	o->brackets = expression->brackets;
	o->parentheses = expression->parentheses[expression->brackets];
	o->next = NULL;
	o->prev = expression->lastOperator;

	if (expression->lastOperator)
		expression->lastOperator->next = o;
	else
		expression->firstOperator = o;

	expression->lastOperator = o;

	return true;
}

/*
 ==================
 R_ParseExpressionValue
 ==================
*/
static bool R_ParseExpressionValue (script_t *script, material_t *material, expression_t *expression){

	token_t	token;
	int		table;

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: unexpected end of expression in material '%s'\n", material->name);
		return false;
	}

	switch (token.type){
	case TT_NUMBER:
		// It's a constant
		if (!R_AddExpressionConstant(material, expression, token.floatValue))
			return false;

		if (!R_AddExpressionValue(material, expression, expression->lastRegister))
			return false;

		break;
	case TT_NAME:
		// Check for a table
		table = LUT_FindTable(token.string);

		if (table != -1){
			// The next token should be an opening bracket
			if (!PS_ExpectTokenString(script, &token, "[", true)){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected '[', found '%s' instead for expression in material '%s'\n", token.string, material->name);
				return false;
			}

			if (expression->brackets == MAX_EXPRESSION_DEPTH){
				Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_DEPTH hit for expression in material '%s'\n", material->name);
				return false;
			}

			expression->brackets++;

			if (!R_AddExpressionValue(material, expression, table))
				return false;

			if (!R_AddExpressionOperator(material, expression, OP_TYPE_TABLE, 1))
				return false;

			// We still expect a value
			return R_ParseExpressionValue(script, material, expression);
		}

		// Check for a variable
		if (!Str_ICompare(token.string, "time"))
			expression->lastRegister = EXP_REGISTER_TIME;
		else if (!Str_ICompare(token.string, "parm0"))
			expression->lastRegister = EXP_REGISTER_PARM0;
		else if (!Str_ICompare(token.string, "parm1"))
			expression->lastRegister = EXP_REGISTER_PARM1;
		else if (!Str_ICompare(token.string, "parm2"))
			expression->lastRegister = EXP_REGISTER_PARM2;
		else if (!Str_ICompare(token.string, "parm3"))
			expression->lastRegister = EXP_REGISTER_PARM3;
		else if (!Str_ICompare(token.string, "parm4"))
			expression->lastRegister = EXP_REGISTER_PARM4;
		else if (!Str_ICompare(token.string, "parm5"))
			expression->lastRegister = EXP_REGISTER_PARM5;
		else if (!Str_ICompare(token.string, "parm6"))
			expression->lastRegister = EXP_REGISTER_PARM6;
		else if (!Str_ICompare(token.string, "parm7"))
			expression->lastRegister = EXP_REGISTER_PARM7;
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid value '%s' for expression in material '%s'\n", token.string, material->name);
			return false;
		}

		if (!R_AddExpressionValue(material, expression, expression->lastRegister))
			return false;

		break;
	case TT_PUNCTUATION:
		// Check for an opening parenthesis
		if (!Str_Compare(token.string, "(")){
			expression->parentheses[expression->brackets]++;

			// We still expect a value
			return R_ParseExpressionValue(script, material, expression);
		}

		// Check for a minus operator before a constant
		if (!Str_Compare(token.string, "-")){
			// The next token should be a constant
			if (!PS_ExpectTokenType(script, &token, TT_NUMBER, 0)){
				Com_Printf(S_COLOR_YELLOW "WARNING: invalid value '%s' for expression in material '%s'\n", token.string, material->name);
				return false;
			}

			if (!R_AddExpressionConstant(material, expression, -token.floatValue))
				return false;

			if (!R_AddExpressionValue(material, expression, expression->lastRegister))
				return false;
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid value '%s' for expression in material '%s'\n", token.string, material->name);
			return false;
		}

		break;
	default:
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value '%s' for expression in material '%s'\n", token.string, material->name);
		return false;
	}

	// We now expect an operator
	return R_ParseExpressionOperator(script, material, expression);
}

/*
 ==================
 R_ParseExpressionOperator
 ==================
*/
static bool R_ParseExpressionOperator (script_t *script, material_t *material, expression_t *expression){

	token_t	token;

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: unexpected end of expression in material '%s'\n", material->name);
		return false;
	}

	// A non-punctuation may be a material keyword, a comma separates
	// arguments, and a brace separates material definitions and stages
	if (token.type != TT_PUNCTUATION || !Str_Compare(token.string, ",") || !Str_Compare(token.string, "{") || !Str_Compare(token.string, "}")){
		if (expression->brackets){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching ']' for expression in material '%s'\n", material->name);
			return false;
		}

		if (expression->parentheses[expression->brackets]){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching ')' for expression in material '%s'\n", material->name);
			return false;
		}

		PS_UnreadToken(script, &token);
		return true;
	}

	// Check for a closing bracket
	if (!Str_Compare(token.string, "]")){
		if (expression->parentheses[expression->brackets]){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching ')' for expression in material '%s'\n", material->name);
			return false;
		}

		expression->brackets--;

		if (expression->brackets < 0){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching '[' for expression in material '%s'\n", material->name);
			return false;
		}

		// We still expect an operator
		return R_ParseExpressionOperator(script, material, expression);
	}

	// Check for a closing parenthesis
	if (!Str_Compare(token.string, ")")){
		expression->parentheses[expression->brackets]--;

		if (expression->parentheses[expression->brackets] < 0){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching '(' for expression in material '%s'\n", material->name);
			return false;
		}

		// We still expect an operator
		return R_ParseExpressionOperator(script, material, expression);
	}

	// Check for an operator
	if (!Str_ICompare(token.string, "*")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_MULTIPLY, 8))
			return false;
	}
	else if (!Str_ICompare(token.string, "/")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_DIVIDE, 8))
			return false;
	}
	else if (!Str_ICompare(token.string, "%")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_MOD, 7))
			return false;
	}
	else if (!Str_ICompare(token.string, "+")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_ADD, 6))
			return false;
	}
	else if (!Str_ICompare(token.string, "-")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_SUBTRACT, 6))
			return false;
	}
	else  if (!Str_ICompare(token.string, ">")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_GREATER, 5))
			return false;
	}
	else if (!Str_ICompare(token.string, "<")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_LESS, 5))
			return false;
	}
	else if (!Str_ICompare(token.string, ">=")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_GEQUAL, 5))
			return false;
	}
	else if (!Str_ICompare(token.string, "<=")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_LEQUAL, 5))
			return false;
	}
	else if (!Str_ICompare(token.string, "==")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_EQUAL, 4))
			return false;
	}
	else if (!Str_ICompare(token.string, "!=")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_NOTEQUAL, 4))
			return false;
	}
	else if (!Str_ICompare(token.string, "&&")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_AND, 3))
			return false;
	}
	else if (!Str_ICompare(token.string, "||")){
		if (!R_AddExpressionOperator(material, expression, OP_TYPE_OR, 2))
			return false;
	}
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid operator '%s' for expression in material '%s'\n", token.string, material->name);
		return false;
	}

	// We now expect a value
	return R_ParseExpressionValue(script, material, expression);
}

/*
 ==================
 R_ParseExpression
 ==================
*/
static bool R_ParseExpression (script_t *script, material_t *material, int *expressionRegister){

	expression_t	expression;
	expValue_t		*v;
	expOperator_t	*o;
	expOp_t			*op;

	Mem_Fill(&expression, 0, sizeof(expression_t));

	// Parse the expression, starting with a value
	PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS);

	if (!R_ParseExpressionValue(script, material, &expression)){
		PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);
		return false;
	}

	PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

	// Emit the expression ops
	while (expression.firstOperator){
		v = expression.firstValue;

		for (o = expression.firstOperator; o->next; o = o->next){
			// If the current operator is nested deeper in brackets than the
			// next operator
			if (o->brackets > o->next->brackets)
				break;

			// If the current and next operators are nested equally deep in
			// brackets
			if (o->brackets == o->next->brackets){
				// If the current operator is nested deeper in parentheses than
				// the next operator
				if (o->parentheses > o->next->parentheses)
					break;

				// If the current and next operators are nested equally deep in
				// parentheses
				if (o->parentheses == o->next->parentheses){
					// If the priority of the current operator is equal to or
					// higher than the priority of the next operator
					if (o->priority >= o->next->priority)
						break;
				}
			}

			v = v->next;
		}

		// Add a temporary register
		if (!R_AddExpressionTemporary(material, &expression))
			return false;

		// Emit the expression op
		if (material->numOps == MAX_EXPRESSION_OPS){
			Com_Printf(S_COLOR_YELLOW "WARNING: MAX_EXPRESSION_OPS hit in material '%s'\n", material->name);
			return false;
		}

		op = &material->expressionOps[material->numOps++];

		op->type = o->type;
		op->a = v->expressionRegister;
		op->b = v->next->expressionRegister;
		op->c = expression.lastRegister;

		// The current temporary register will be used as an operand for the
		// next operation
		v->expressionRegister = expression.lastRegister;

		// Remove the second value
		v = v->next;

		if (v->prev)
			v->prev->next = v->next;
		else
			expression.firstValue = v->next;

		if (v->next)
			v->next->prev = v->prev;
		else
			expression.lastValue = v->prev;

		// Remove the operator
		if (o->prev)
			o->prev->next = o->next;
		else
			expression.firstOperator = o->next;

		if (o->next)
			o->next->prev = o->prev;
		else
			expression.lastOperator = o->prev;
	}

	// The last temporary register will contain the result after evaluation
	*expressionRegister = expression.lastRegister;

	return true;
}


/*
 ==============================================================================

 MATERIAL IMAGE PROGRAM PARSING

 ==============================================================================
*/


/*
 ==================
 R_ParseImageProgram
 ==================
*/
static bool R_ParseImageProgram (script_t *script, material_t *material, char *imageProgram, int maxLength){

	token_t	token;
	int		depth = 1;

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing image program or texture name specification in material '%s'\n", material->name);
		return false;
	}

	Str_Copy(imageProgram, token.string, maxLength);
	Str_StripFileExtension(imageProgram);

	// Check for an opening parenthesis
	if (!PS_CheckTokenString(script, &token, "(", true))
		return true;

	Str_Append(imageProgram, "(", maxLength);

	// Read until a matching closing parenthesis is found
	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: no matching ')' for image program in material '%s'\n", material->name);
			return false;
		}

		Str_Append(imageProgram, token.string, maxLength);
		Str_StripFileExtension(imageProgram);

		if (token.type == TT_PUNCTUATION){
			if (!Str_Compare(token.string, "("))
				depth++;
			else if (!Str_Compare(token.string, ")")){
				depth--;

				if (depth == 0)
					break;
			}
		}
	}

	return true;
}


/*
 ==============================================================================

 MATERIAL PARSING

 ==============================================================================
*/


/*
 ==================
 R_ParseGlobalSurfaceParm
 ==================
*/
static bool R_ParseGlobalSurfaceParm (script_t *script, material_t *material){

	token_t token;

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'surfaceParm' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'surfaceParm' in material '%s'\n", material->name);
		return false;
	}

	if (!Str_ICompare(token.string, "lighting"))
		material->surfaceParm |= SURFACEPARM_LIGHTING;
	else if (!Str_ICompare(token.string, "sky"))
		material->surfaceParm |= SURFACEPARM_SKY;
	else if (!Str_ICompare(token.string, "warp"))
		material->surfaceParm |= SURFACEPARM_WARP;
	else if (!Str_ICompare(token.string, "trans33"))
		material->surfaceParm |= SURFACEPARM_TRANS33;
	else if (!Str_ICompare(token.string, "trans66"))
		material->surfaceParm |= SURFACEPARM_TRANS66;
	else if (!Str_ICompare(token.string, "flowing"))
		material->surfaceParm |= SURFACEPARM_FLOWING;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'surfaceParm' parameter '%s' in material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalIf
 ==================
*/
static bool R_ParseGlobalIf (script_t *script, material_t *material){

	if (!R_ParseExpression(script, material, &material->conditionRegister)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'if' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalTranslucent
 ==================
*/
static bool R_ParseGlobalTranslucent (script_t *script, material_t *material){

	material->coverage = MC_TRANSLUCENT;

	return true;
}

/*
 ==================
 R_ParseGlobalForceOverlays
 ==================
*/
static bool R_ParseGlobalForceOverlays (script_t *script, material_t *material){

	material->flags |= MF_FORCEOVERLAYS;

	return true;
}

/*
 ==================
 R_ParseGlobalNoOverlays
 ==================
*/
static bool R_ParseGlobalNoOverlays (script_t *script, material_t *material){

	material->flags |= MF_NOOVERLAYS;

	return true;
}

/*
 ==================
 R_ParseGlobalForceShadows
 ==================
*/
static bool R_ParseGlobalForceShadows (script_t *script, material_t *material){

	material->flags |= MF_FORCESHADOWS;

	return true;
}

/*
 ==================
 R_ParseGlobalNoShadows
 ==================
*/
static bool R_ParseGlobalNoShadows (script_t *script, material_t *material){

	material->flags |= MF_NOSHADOWS;

	return true;
}

/*
 ==================
 R_ParseGlobalNoAmbient
 ==================
*/
static bool R_ParseGlobalNoAmbient (script_t *script, material_t *material){

	material->flags |= MF_NOAMBIENT;

	return true;
}

/*
 ==================
 R_ParseGlobalNoBlend
 ==================
*/
static bool R_ParseGlobalNoBlend (script_t *script, material_t *material){

	material->flags |= MF_NOBLEND;

	return true;
}

/*
 ==================
 R_ParseGlobalNoFog
 ==================
*/
static bool R_ParseGlobalNoFog (script_t *script, material_t *material){

	material->flags |= MF_NOFOG;

	return true;
}

/*
 ==================
 R_ParseGlobalUpdateCurrentColor
 ==================
*/
static bool R_ParseGlobalUpdateCurrentColor (script_t *script, material_t *material){

	material->flags |= MF_UPDATECURRENTCOLOR;

	return true;
}

/*
 ==================
 R_ParseGlobalUpdateCurrentDepth
 ==================
*/
static bool R_ParseGlobalUpdateCurrentDepth (script_t *script, material_t *material){

	material->flags |= MF_UPDATECURRENTDEPTH;

	return true;
}

/*
 ==================
 R_ParseGlobalSort
 ==================
*/
static bool R_ParseGlobalSort (script_t *script, material_t *material){

	token_t	token;

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'sort' in material '%s'\n", material->name);
		return false;
	}

	if (!Str_ICompare(token.string, "opaque"))
		material->sort = SORT_OPAQUE;
	else if (!Str_ICompare(token.string, "decal"))
		material->sort = SORT_DECAL;
	else if (!Str_ICompare(token.string, "glare"))
		material->sort = SORT_GLARE;
	else if (!Str_ICompare(token.string, "refractable"))
		material->sort = SORT_REFRACTABLE;
	else if (!Str_ICompare(token.string, "refractive"))
		material->sort = SORT_REFRACTIVE;
	else if (!Str_ICompare(token.string, "farthest"))
		material->sort = SORT_FARTHEST;
	else if (!Str_ICompare(token.string, "almostFarthest"))
		material->sort = SORT_ALMOST_FARTHEST;
	else if (!Str_ICompare(token.string, "far"))
		material->sort = SORT_FAR;
	else if (!Str_ICompare(token.string, "medium"))
		material->sort = SORT_MEDIUM;
	else if (!Str_ICompare(token.string, "near"))
		material->sort = SORT_NEAR;
	else if (!Str_ICompare(token.string, "almostNearest"))
		material->sort = SORT_ALMOST_NEAREST;
	else if (!Str_ICompare(token.string, "nearest"))
		material->sort = SORT_NEAREST;
	else if (!Str_ICompare(token.string, "postProcess"))
		material->sort = SORT_POST_PROCESS;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'sort' parameter '%s' in material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalAmbientLight
 ==================
*/
static bool R_ParseGlobalAmbientLight (script_t *script, material_t *material){

	material->lightType = LT_AMBIENT;

	return true;
}

/*
 ==================
 R_ParseGlobalBlendLight
 ==================
*/
static bool R_ParseGlobalBlendLight (script_t *script, material_t *material){

	material->lightType = LT_BLEND;

	return true;
}

/*
 ==================
 R_ParseGlobalFogLight
 ==================
*/
static bool R_ParseGlobalFogLight (script_t *script, material_t *material){

	material->lightType = LT_FOG;

	return true;
}

/*
 ==================
 R_ParseGlobalLightFalloffImage
 ==================
*/
static bool R_ParseGlobalLightFalloffImage (script_t *script, material_t *material){

	token_t			token;
	char			name[MAX_PATH_LENGTH];
	int				flags = TF_NOPICMIP | TF_LIGHT;
	textureFilter_t	filter = TF_LINEAR;

	if (material->type != MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'lightFalloffImage' not allowed in material '%s'\n", material->name);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'lightFalloffImage' in material '%s'\n", material->name);
			return false;
		}

		if (!Str_ICompare(token.string, "uncompressed"))
			flags |= TF_UNCOMPRESSED;
		else if (!Str_ICompare(token.string, "nearest"))
			filter = TF_NEAREST;
		else if (!Str_ICompare(token.string, "linear"))
			filter = TF_LINEAR;
		else {
			PS_UnreadToken(script, &token);
			break;
		}
	}

	if (!R_ParseImageProgram(script, material, name, sizeof(name))){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'lightFalloffImage' in material '%s'\n", material->name);
		return false;
	}

	material->lightFalloffImage = R_FindTexture(name, flags, filter, TW_CLAMP);
	if (!material->lightFalloffImage){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalLightCubeImage
 ==================
*/
static bool R_ParseGlobalLightCubeImage (script_t *script, material_t *material){

	token_t			token;
	int				flags = TF_NOPICMIP | TF_LIGHT;
	textureFilter_t	filter = TF_DEFAULT;

	if (material->type != MT_LIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'lightCubeImage' not allowed in material '%s'\n", material->name);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'lightCubeImage' in material '%s'\n", material->name);
			return false;
		}

		if (!Str_ICompare(token.string, "uncompressed"))
			flags |= TF_UNCOMPRESSED;
		else if (!Str_ICompare(token.string, "nearest"))
			filter = TF_NEAREST;
		else if (!Str_ICompare(token.string, "linear"))
			filter = TF_LINEAR;
		else
			break;
	}

	material->lightCubeImage = R_FindCubeTexture(token.string, flags, filter, false);
	if (!material->lightCubeImage){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalSpectrum
 ==================
*/
static bool R_ParseGlobalSpectrum (script_t *script, material_t *material){

	if (!PS_ReadInteger(script, &material->spectrum)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'spectrum' in material '%s'\n", material->name);
		return false;
	}

	if (material->spectrum < 1){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %i for 'spectrum' in material '%s'\n", material->spectrum, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalBackSided
 ==================
*/
static bool R_ParseGlobalBackSided (script_t *script, material_t *material){

	material->cullType = CT_BACK_SIDED;

	return true;
}

/*
 ==================
 R_ParseGlobalTwoSided
 ==================
*/
static bool R_ParseGlobalTwoSided (script_t *script, material_t *material){

	material->cullType = CT_TWO_SIDED;

	return true;
}

/*
 ==================
 R_ParseGlobalPolygonOffset
 ==================
*/
static bool R_ParseGlobalPolygonOffset (script_t *script, material_t *material){

	token_t	token;
	int		i;

	material->flags |= MF_POLYGONOFFSET;

	material->polygonOffset = 1.0f;

	if (PS_CheckTokenType(script, &token, TT_NUMBER, 0))
		material->polygonOffset = token.floatValue;
	else if (PS_CheckTokenType(script, &token, TT_PUNCTUATION, PT_SUB)){
		if (!PS_ExpectTokenType(script, &token, TT_NUMBER, 0)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'polygonOffset' in material '%s'\n", material->name);
			return false;
		}

		material->polygonOffset = -token.floatValue;
	}

	if (!material->polygonOffset){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'polygonOffset' in material '%s'\n", material->polygonOffset, material->name);
		return false;
	}

	for (i = 0; i < MAX_STAGES; i++){
		material->stages[i].drawState |= DS_POLYGONOFFSET;
		material->stages[i].polygonOffset = material->polygonOffset;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalDecalMacro
 ==================
*/
static bool R_ParseGlobalDecalMacro (script_t *script, material_t *material){

	int		i;

	material->coverage = MC_TRANSLUCENT;

	material->sort = SORT_DECAL;

	material->flags |= MF_POLYGONOFFSET;
	material->polygonOffset = 1.0f;

	for (i = 0; i < MAX_STAGES; i++){
		material->stages[i].drawState |= DS_POLYGONOFFSET;
		material->stages[i].polygonOffset = material->polygonOffset;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalDecalInfo
 ==================
*/
static bool R_ParseGlobalDecalInfo (script_t *script, material_t *material){

	if (!PS_ReadFloat(script, &material->decalInfo.fadeInTime)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'decalInfo' in material '%s'\n", material->name);
		return false;
	}

	if (material->decalInfo.fadeInTime < 0.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'decalInfo' in material '%s'\n", material->decalInfo.fadeInTime, material->name);
		return false;
	}

	if (!PS_ReadFloat(script, &material->decalInfo.stayTime)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'decalInfo' in material '%s'\n", material->name);
		return false;
	}

	if (material->decalInfo.stayTime < 0.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'decalInfo' in material '%s'\n", material->decalInfo.stayTime, material->name);
		return false;
	}

	if (!PS_ReadFloat(script, &material->decalInfo.fadeOutTime)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'decalInfo' in material '%s'\n", material->name);
		return false;
	}

	if (material->decalInfo.fadeOutTime < 0.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'decalInfo' in material '%s'\n", material->decalInfo.fadeOutTime, material->name);
		return false;
	}

	if (!PS_ReadMatrix1D(script, 4, material->decalInfo.startRGBA)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'decalInfo' in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadMatrix1D(script, 4, material->decalInfo.stayRGBA)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'decalInfo' in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadMatrix1D(script, 4, material->decalInfo.endRGBA)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'decalInfo' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalDeform
 ==================
*/
static bool R_ParseGlobalDeform (script_t *script, material_t *material){

	token_t	token;
	int		i;

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'deform' in material '%s'\n", material->name);
		return false;
	}

	if (!Str_ICompare(token.string, "expand")){
		material->deform = DFRM_EXPAND;

		if (!R_ParseExpression(script, material, &material->deformRegisters[0])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'deform expand' in material '%s'\n", material->name);
			return false;
		}
	}
	else if (!Str_ICompare(token.string, "move")){
		material->deform = DFRM_MOVE;

		for (i = 0; i < 3; i++){
			if (!R_ParseExpression(script, material, &material->deformRegisters[i])){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'deform move' in material '%s'\n", material->name);
				return false;
			}

			if (i < 2){
				if (!PS_ExpectTokenString(script, &token, ",", true)){
					Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'deform move' in material '%s'\n", token.string, material->name);
					return false;
				}
			}
		}
	}
	else if (!Str_ICompare(token.string, "sprite"))
		material->deform = DFRM_SPRITE;
	else if (!Str_ICompare(token.string, "tube"))
		material->deform = DFRM_TUBE;
	else if (!Str_ICompare(token.string, "beam"))
		material->deform = DFRM_BEAM;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'deform' parameter '%s' in material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalBumpMap
 ==================
*/
static bool R_ParseGlobalBumpMap (script_t *script, material_t *material){

	stage_t	*stage;
	char	name[MAX_PATH_LENGTH];

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'bumpMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (material->numStages == MAX_STAGES){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_STAGES hit in material '%s'\n", material->name);
		return false;
	}
	stage = &material->stages[material->numStages++];

	stage->lighting = SL_BUMP;

	if (!R_ParseImageProgram(script, material, name, sizeof(name))){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'bumpMap' in material '%s'\n", material->name);
		return false;
	}

	stage->textureStage.texture = R_FindTexture(name, TF_BUMP, TF_DEFAULT, TW_REPEAT);
	if (!stage->textureStage.texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalDiffuseMap
 ==================
*/
static bool R_ParseGlobalDiffuseMap (script_t *script, material_t *material){

	stage_t	*stage;
	char	name[MAX_PATH_LENGTH];

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'diffuseMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (material->numStages == MAX_STAGES){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_STAGES hit in material '%s'\n", material->name);
		return false;
	}
	stage = &material->stages[material->numStages++];

	stage->lighting = SL_DIFFUSE;

	if (!R_ParseImageProgram(script, material, name, sizeof(name))){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'diffuseMap' in material '%s'\n", material->name);
		return false;
	}

	stage->textureStage.texture = R_FindTexture(name, TF_DIFFUSE, TF_DEFAULT, TW_REPEAT);
	if (!stage->textureStage.texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseGlobalSpecularMap
 ==================
*/
static bool R_ParseGlobalSpecularMap (script_t *script, material_t *material){

	stage_t	*stage;
	char	name[MAX_PATH_LENGTH];

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'specularMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (material->numStages == MAX_STAGES){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_STAGES hit in material '%s'\n", material->name);
		return false;
	}
	stage = &material->stages[material->numStages++];

	stage->lighting = SL_SPECULAR;

	if (!R_ParseImageProgram(script, material, name, sizeof(name))){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'specularMap' in material '%s'\n", material->name);
		return false;
	}

	stage->textureStage.texture = R_FindTexture(name, TF_SPECULAR, TF_DEFAULT, TW_REPEAT);
	if (!stage->textureStage.texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	return true;
}


// ============================================================================


/*
 ==================
 R_ParseStageShaderQuality
 ==================
*/
static bool R_ParseStageShaderQuality (script_t *script, material_t *material, stage_t *stage){

	Com_Printf(S_COLOR_YELLOW "WARNING: 'shaderQuality' must be the first stage keyword in material '%s'\n", material->name);

	return false;
}

/*
 ==================
 R_ParseStageIf
 ==================
*/
static bool R_ParseStageIf (script_t *script, material_t *material, stage_t *stage){

	if (!R_ParseExpression(script, material, &stage->conditionRegister)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'if' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageNoPicMip
 ==================
*/
static bool R_ParseStageNoPicMip (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'noPicMip' used after texture specification in material '%s'\n", material->name);
		return false;
	}

	textureStage->flags |= TF_NOPICMIP;

	return true;
}

/*
 ==================
 R_ParseStageUncompressed
 ==================
*/
static bool R_ParseStageUncompressed (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'uncompressed' used after texture specification in material '%s'\n", material->name);
		return false;
	}

	textureStage->flags |= TF_UNCOMPRESSED;

	return true;
}

/*
 ==================
 R_ParseStageHighQuality
 ==================
*/
static bool R_ParseStageHighQuality (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'highQuality' used after texture specification in material '%s'\n", material->name);
		return false;
	}

	textureStage->flags |= (TF_NOPICMIP | TF_UNCOMPRESSED);

	return true;
}

/*
 ==================
 R_ParseStageNearest
 ==================
*/
static bool R_ParseStageNearest (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'nearest' used after texture specification in material '%s'\n", material->name);
		return false;
	}

	textureStage->filter = TF_NEAREST;

	return true;
}

/*
 ==================
 R_ParseStageLinear
 ==================
*/
static bool R_ParseStageLinear (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'linear' used after texture specification in material '%s'\n", material->name);
		return false;
	}

	textureStage->filter = TF_LINEAR;

	return true;
}

/*
 ==================
 R_ParseStageMirroredRepeat
 ==================
*/
static bool R_ParseStageMirroredRepeat (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'mirroredRepeat' used after texture specification in material '%s'\n", material->name);
		return false;
	}

	textureStage->wrap = TW_REPEAT_MIRRORED;

	return true;
}

/*
 ==================
 R_ParseStageClamp
 ==================
*/
static bool R_ParseStageClamp (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'clamp' used after texture specification in material '%s'\n", material->name);
		return false;
	}

	textureStage->wrap = TW_CLAMP;

	return true;
}

/*
 ==================
 R_ParseStageZeroClamp
 ==================
*/
static bool R_ParseStageZeroClamp (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'zeroClamp' used after texture specification in material '%s'\n", material->name);
		return false;
	}

	textureStage->wrap = TW_CLAMP_TO_ZERO;

	return true;
}

/*
 ==================
 R_ParseStageAlphaZeroClamp
 ==================
*/
static bool R_ParseStageAlphaZeroClamp (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'alphaZeroClamp' used after texture specification in material '%s'\n", material->name);
		return false;
	}

	textureStage->wrap = TW_CLAMP_TO_ZERO_ALPHA;

	return true;
}

/*
 ==================
 R_ParseStageMap
 ==================
*/
static bool R_ParseStageMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	char			name[MAX_PATH_LENGTH];

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple textures for a stage in material '%s'\n", material->name);
		return false;
	}

	if (material->type == MT_LIGHT)
		textureStage->flags |= (TF_NOPICMIP | TF_LIGHT);

	if (material->type == MT_NOMIP){
		textureStage->flags |= TF_NOPICMIP;

		if (textureStage->filter == TF_DEFAULT)
			textureStage->filter = TF_LINEAR;
	}

	if (!R_ParseImageProgram(script, material, name, sizeof(name))){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'map' in material '%s'\n", material->name);
		return false;
	}

	textureStage->texture = R_FindTexture(name, textureStage->flags, textureStage->filter, textureStage->wrap);
	if (!textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageBumpMap
 ==================
*/
static bool R_ParseStageBumpMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	char			name[MAX_PATH_LENGTH];

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'bumpMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple textures for a stage in material '%s'\n", material->name);
		return false;
	}

	stage->lighting = SL_BUMP;

	if (!R_ParseImageProgram(script, material, name, sizeof(name))){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'bumpMap' in material '%s'\n", material->name);
		return false;
	}

	textureStage->texture = R_FindTexture(name, textureStage->flags | TF_BUMP, textureStage->filter, textureStage->wrap);
	if (!textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageDiffuseMap
 ==================
*/
static bool R_ParseStageDiffuseMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	char			name[MAX_PATH_LENGTH];

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'diffuseMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple textures for a stage in material '%s'\n", material->name);
		return false;
	}

	stage->lighting = SL_DIFFUSE;

	if (!R_ParseImageProgram(script, material, name, sizeof(name))){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'diffuseMap' in material '%s'\n", material->name);
		return false;
	}

	textureStage->texture = R_FindTexture(name, textureStage->flags | TF_DIFFUSE, textureStage->filter, textureStage->wrap);
	if (!textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageSpecularMap
 ==================
*/
static bool R_ParseStageSpecularMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	char			name[MAX_PATH_LENGTH];

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'specularMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple textures for a stage in material '%s'\n", material->name);
		return false;
	}

	stage->lighting = SL_SPECULAR;

	if (!R_ParseImageProgram(script, material, name, sizeof(name))){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'specularMap' in material '%s'\n", material->name);
		return false;
	}

	textureStage->texture = R_FindTexture(name, textureStage->flags | TF_SPECULAR, textureStage->filter, textureStage->wrap);
	if (!textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageCubeMap
 ==================
*/
static bool R_ParseStageCubeMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'cubeMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple textures for a stage in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'cubeMap' in material '%s'\n", material->name);
		return false;
	}

	textureStage->texture = R_FindCubeTexture(token.string, textureStage->flags, textureStage->filter, false);
	if (!textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageCameraCubeMap
 ==================
*/
static bool R_ParseStageCameraCubeMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'cameraCubeMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple textures for a stage in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'cameraCubeMap' in material '%s'\n", material->name);
		return false;
	}

	textureStage->texture = R_FindCubeTexture(token.string, textureStage->flags, textureStage->filter, true);
	if (!textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageVideoMap
 ==================
*/
static bool R_ParseStageVideoMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	char			name[MAX_PATH_LENGTH];

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple textures for a stage in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'videoMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->type == MT_NOMIP || r_inGameVideos->integerValue){
		textureStage->cinematicHandle = CIN_PlayCinematic(token.string, CIN_LOOPING | CIN_SILENT);
		if (!textureStage->cinematicHandle){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find video '%s' for material '%s'\n", token.string, material->name);
			return false;
		}

		textureStage->texture = rg.cinematicTextures[textureStage->cinematicHandle - 1];
	}
	else {
		Str_Copy(name, token.string, sizeof(name));
		Str_StripFileExtension(name);

		if (material->type == MT_LIGHT)
			textureStage->flags |= (TF_NOPICMIP | TF_LIGHT);

		textureStage->texture = R_FindTexture(name, textureStage->flags, textureStage->filter, TW_CLAMP_TO_ZERO);
		if (!textureStage->texture){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
			return false;
		}
	}
		
	return true;
}

/*
 ==================
 R_ParseStageSkyRenderMap
 ==================
*/
static bool R_ParseStageSkyRenderMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'skyRenderMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple textures for a stage in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewType != ST_NONE && material->subviewType != ST_SKY){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple subview types in material '%s'\n", material->name);
		return false;
	}

	material->subviewType = ST_SKY;
	material->subviewTexture = rg.skyTexture;

	if (!PS_ReadInteger(script, &material->subviewWidth)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skyRenderMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewWidth < 1 || material->subviewWidth > SCREEN_WIDTH){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid width value of %i for 'skyRenderMap' in material '%s'\n", material->subviewWidth, material->name);
		return false;
	}

	if (!PS_ReadInteger(script, &material->subviewHeight)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'skyRenderMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewHeight < 1 || material->subviewHeight > SCREEN_HEIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid height value of %i for 'skyRenderMap' in material '%s'\n", material->subviewHeight, material->name);
		return false;
	}

	textureStage->texture = rg.skyTexture;

	return true;
}

/*
 ==================
 R_ParseStageMirrorRenderMap
 ==================
*/
static bool R_ParseStageMirrorRenderMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'mirrorRenderMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple textures for a stage in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewType != ST_NONE && material->subviewType != ST_MIRROR){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple subview types in material '%s'\n", material->name);
		return false;
	}

	material->subviewType = ST_MIRROR;
	material->subviewTexture = rg.mirrorTexture;

	if (!PS_ReadInteger(script, &material->subviewWidth)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'mirrorRenderMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewWidth < 1 || material->subviewWidth > SCREEN_WIDTH){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid width value of %i for 'mirrorRenderMap' in material '%s'\n", material->subviewWidth, material->name);
		return false;
	}

	if (!PS_ReadInteger(script, &material->subviewHeight)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'mirrorRenderMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewHeight < 1 || material->subviewHeight > SCREEN_HEIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid height value of %i for 'mirrorRenderMap' in material '%s'\n", material->subviewHeight, material->name);
		return false;
	}

	textureStage->texture = rg.mirrorTexture;

	return true;
}

/*
 ==================
 R_ParseStageRemoteRenderMap
 ==================
*/
static bool R_ParseStageRemoteRenderMap (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (material->type != MT_GENERIC){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'remoteRenderMap' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (textureStage->texture){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple textures for a stage in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewType != ST_NONE && material->subviewType != ST_REMOTE){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple subview types in material '%s'\n", material->name);
		return false;
	}

	material->subviewType = ST_REMOTE;
	material->subviewTexture = rg.remoteTexture;

	if (!PS_ReadInteger(script, &material->subviewWidth)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'remoteRenderMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewWidth < 1 || material->subviewWidth > SCREEN_WIDTH){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid width value of %i for 'remoteRenderMap' in material '%s'\n", material->subviewWidth, material->name);
		return false;
	}

	if (!PS_ReadInteger(script, &material->subviewHeight)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'remoteRenderMap' in material '%s'\n", material->name);
		return false;
	}

	if (material->subviewHeight < 1 || material->subviewHeight > SCREEN_HEIGHT){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid height value of %i for 'remoteRenderMap' in material '%s'\n", material->subviewHeight, material->name);
		return false;
	}

	textureStage->texture = rg.remoteTexture;

	return true;
}

/*
 ==================
 R_ParseStageTexGen
 ==================
*/
static bool R_ParseStageTexGen (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'texGen' in material '%s'\n", material->name);
		return false;
	}

	if (!Str_ICompare(token.string, "vector")){
		textureStage->texGen = TG_VECTOR;

		for (i = 0; i < 2; i++){
			if (!PS_ReadMatrix1D(script, 4, textureStage->texGenVectors[i])){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'texGen vector' in material '%s'\n", material->name);
				return false;
			}
		}
	}
	else if (!Str_ICompare(token.string, "normal"))
		textureStage->texGen = TG_NORMAL;
	else if (!Str_ICompare(token.string, "reflect"))
		textureStage->texGen = TG_REFLECT;
	else if (!Str_ICompare(token.string, "skybox"))
		textureStage->texGen = TG_SKYBOX;
	else if (!Str_ICompare(token.string, "screen"))
		textureStage->texGen = TG_SCREEN;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'texGen' parameter '%s' in material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageTranslate
 ==================
*/
static bool R_ParseStageTranslate (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (textureStage->numTexMods == MAX_TEXMODS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXMODS hit in material '%s'\n", material->name);
		return false;
	}

	textureStage->texMods[textureStage->numTexMods] = TM_TRANSLATE;

	for (i = 0; i < 2; i++){
		if (!R_ParseExpression(script, material, &textureStage->texModsRegisters[textureStage->numTexMods][i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'translate' in material '%s'\n", material->name);
			return false;
		}

		if (i < 1){
			if (!PS_ExpectTokenString(script, &token, ",", true)){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'translate' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	textureStage->numTexMods++;

	return true;
}

/*
 ==================
 R_ParseStageScroll
 ==================
*/
static bool R_ParseStageScroll (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (textureStage->numTexMods == MAX_TEXMODS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXMODS hit in material '%s'\n", material->name);
		return false;
	}

	textureStage->texMods[textureStage->numTexMods] = TM_TRANSLATE;

	for (i = 0; i < 2; i++){
		if (!R_ParseExpression(script, material, &textureStage->texModsRegisters[textureStage->numTexMods][i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'scroll' in material '%s'\n", material->name);
			return false;
		}

		if (i < 1){
			if (!PS_ExpectTokenString(script, &token, ",", true)){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'scroll' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	textureStage->numTexMods++;

	return true;
}

/*
 ==================
 R_ParseStageScale
 ==================
*/
static bool R_ParseStageScale (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (textureStage->numTexMods == MAX_TEXMODS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXMODS hit in material '%s'\n", material->name);
		return false;
	}

	textureStage->texMods[textureStage->numTexMods] = TM_SCALE;

	for (i = 0; i < 2; i++){
		if (!R_ParseExpression(script, material, &textureStage->texModsRegisters[textureStage->numTexMods][i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'scale' in material '%s'\n", material->name);
			return false;
		}

		if (i < 1){
			if (!PS_ExpectTokenString(script, &token, ",", true)){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'scale' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	textureStage->numTexMods++;

	return true;
}

/*
 ==================
 R_ParseStageCenterScale
 ==================
*/
static bool R_ParseStageCenterScale (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (textureStage->numTexMods == MAX_TEXMODS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXMODS hit in material '%s'\n", material->name);
		return false;
	}

	textureStage->texMods[textureStage->numTexMods] = TM_CENTERSCALE;

	for (i = 0; i < 2; i++){
		if (!R_ParseExpression(script, material, &textureStage->texModsRegisters[textureStage->numTexMods][i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'centerScale' in material '%s'\n", material->name);
			return false;
		}

		if (i < 1){
			if (!PS_ExpectTokenString(script, &token, ",", true)){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'centerScale' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	textureStage->numTexMods++;

	return true;
}

/*
 ==================
 R_ParseStageShear
 ==================
*/
static bool R_ParseStageShear (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;
	token_t			token;
	int				i;

	if (textureStage->numTexMods == MAX_TEXMODS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXMODS hit in material '%s'\n", material->name);
		return false;
	}

	textureStage->texMods[textureStage->numTexMods] = TM_SHEAR;

	for (i = 0; i < 2; i++){
		if (!R_ParseExpression(script, material, &textureStage->texModsRegisters[textureStage->numTexMods][i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'shear' in material '%s'\n", material->name);
			return false;
		}

		if (i < 1){
			if (!PS_ExpectTokenString(script, &token, ",", true)){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'shear' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	textureStage->numTexMods++;

	return true;
}

/*
 ==================
 R_ParseStageRotate
 ==================
*/
static bool R_ParseStageRotate (script_t *script, material_t *material, stage_t *stage){

	textureStage_t	*textureStage = &stage->textureStage;

	if (textureStage->numTexMods == MAX_TEXMODS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_TEXMODS hit in material '%s'\n", material->name);
		return false;
	}

	textureStage->texMods[textureStage->numTexMods] = TM_ROTATE;

	if (!R_ParseExpression(script, material, &textureStage->texModsRegisters[textureStage->numTexMods][0])){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'rotate' in material '%s'\n", material->name);
		return false;
	}

	textureStage->numTexMods++;

	return true;
}

/*
 ==================
 R_ParseStageRed
 ==================
*/
static bool R_ParseStageRed (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	if (!R_ParseExpression(script, material, &colorStage->registers[0])){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'red' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageGreen
 ==================
*/
static bool R_ParseStageGreen (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	if (!R_ParseExpression(script, material, &colorStage->registers[1])){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'green' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageBlue
 ==================
*/
static bool R_ParseStageBlue (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	if (!R_ParseExpression(script, material, &colorStage->registers[2])){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'blue' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageAlpha
 ==================
*/
static bool R_ParseStageAlpha (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	if (!R_ParseExpression(script, material, &colorStage->registers[3])){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'alpha' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageRGB
 ==================
*/
static bool R_ParseStageRGB (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;
	int				expressionRegister;

	if (!R_ParseExpression(script, material, &expressionRegister)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'rgb' in material '%s'\n", material->name);
		return false;
	}

	colorStage->registers[0] = expressionRegister;
	colorStage->registers[1] = expressionRegister;
	colorStage->registers[2] = expressionRegister;

	return true;
}

/*
 ==================
 R_ParseStageRGBA
 ==================
*/
static bool R_ParseStageRGBA (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;
	int				expressionRegister;

	if (!R_ParseExpression(script, material, &expressionRegister)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'rgba' in material '%s'\n", material->name);
		return false;
	}

	colorStage->registers[0] = expressionRegister;
	colorStage->registers[1] = expressionRegister;
	colorStage->registers[2] = expressionRegister;
	colorStage->registers[3] = expressionRegister;

	return true;
}

/*
 ==================
 R_ParseStageColor
 ==================
*/
static bool R_ParseStageColor (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;
	token_t			token;
	int				i;

	for (i = 0; i < 4; i++){
		if (!R_ParseExpression(script, material, &colorStage->registers[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'color' in material '%s'\n", material->name);
			return false;
		}

		if (i < 3){
			if (!PS_ExpectTokenString(script, &token, ",", true)){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'color' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	return true;
}

/*
 ==================
 R_ParseStageColored
 ==================
*/
static bool R_ParseStageColored (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	colorStage->registers[0] = EXP_REGISTER_PARM0;
	colorStage->registers[1] = EXP_REGISTER_PARM1;
	colorStage->registers[2] = EXP_REGISTER_PARM2;
	colorStage->registers[3] = EXP_REGISTER_PARM3;

	return true;
}

/*
 ==================
 R_ParseStageVertexColor
 ==================
*/
static bool R_ParseStageVertexColor (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	colorStage->vertexColor = VC_MODULATE;

	return true;
}

/*
 ==================
 R_ParseStageInverseVertexColor
 ==================
*/
static bool R_ParseStageInverseVertexColor (script_t *script, material_t *material, stage_t *stage){

	colorStage_t	*colorStage = &stage->colorStage;

	colorStage->vertexColor = VC_INVERSE_MODULATE;

	return true;
}

/*
 ==================
 R_ParseStageVertexShader
 ==================
*/
static bool R_ParseStageVertexShader (script_t *script, material_t *material, stage_t *stage){

	shaderStage_t	*shaderStage = &stage->shaderStage;
	token_t			token;

	if (material->type != MT_GENERIC && material->type != MT_NOMIP){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'vertexShader' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (shaderStage->vertexShader){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple vertex shaders for a stage in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'vertexShader' in material '%s'\n", material->name);
		return false;
	}

	shaderStage->vertexShader = R_FindShader(token.string, GL_VERTEX_SHADER);
	if (!shaderStage->vertexShader){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find vertex shader '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	if (shaderStage->fragmentShader){
		if (Str_ICompare(token.string, shaderStage->fragmentShader->name))
			shaderStage->program = R_FindProgram(NULL, shaderStage->vertexShader, shaderStage->fragmentShader);
		else
			shaderStage->program = R_FindProgram(token.string, shaderStage->vertexShader, shaderStage->fragmentShader);

		if (!shaderStage->program){
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid program in material '%s'\n", material->name);
			return false;
		}
	}

	return true;
}

/*
 ==================
 R_ParseStageFragmentShader
 ==================
*/
static bool R_ParseStageFragmentShader (script_t *script, material_t *material, stage_t *stage){

	shaderStage_t	*shaderStage = &stage->shaderStage;
	token_t			token;

	if (material->type != MT_GENERIC && material->type != MT_NOMIP){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'fragmentShader' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (shaderStage->fragmentShader){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple fragment shaders for a stage in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'fragmentShader' in material '%s'\n", material->name);
		return false;
	}

	shaderStage->fragmentShader = R_FindShader(token.string, GL_FRAGMENT_SHADER);
	if (!shaderStage->fragmentShader){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find fragment shader '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	if (shaderStage->vertexShader){
		if (Str_ICompare(token.string, shaderStage->vertexShader->name))
			shaderStage->program = R_FindProgram(NULL, shaderStage->vertexShader, shaderStage->fragmentShader);
		else
			shaderStage->program = R_FindProgram(token.string, shaderStage->vertexShader, shaderStage->fragmentShader);

		if (!shaderStage->program){
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid program in material '%s'\n", material->name);
			return false;
		}
	}

	return true;
}

/*
 ==================
 R_ParseStageShader
 ==================
*/
static bool R_ParseStageShader (script_t *script, material_t *material, stage_t *stage){

	shaderStage_t	*shaderStage = &stage->shaderStage;
	token_t			token;

	if (material->type != MT_GENERIC && material->type != MT_NOMIP){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'shader' not allowed in material '%s'\n", material->name);
		return false;
	}

	if (shaderStage->vertexShader || shaderStage->fragmentShader){
		Com_Printf(S_COLOR_YELLOW "WARNING: multiple shaders for a stage in material '%s'\n", material->name);
		return false;
	}

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'shader' in material '%s'\n", material->name);
		return false;
	}

	shaderStage->vertexShader = R_FindShader(token.string, GL_VERTEX_SHADER);
	if (!shaderStage->vertexShader){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find vertex shader '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	shaderStage->fragmentShader = R_FindShader(token.string, GL_FRAGMENT_SHADER);
	if (!shaderStage->fragmentShader){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find fragment shader '%s' for material '%s'\n", token.string, material->name);
		return false;
	}

	shaderStage->program = R_FindProgram(token.string, shaderStage->vertexShader, shaderStage->fragmentShader);
	if (!shaderStage->program){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid program in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageShaderParm
 ==================
*/
static bool R_ParseStageShaderParm (script_t *script, material_t *material, stage_t *stage){

	shaderStage_t	*shaderStage = &stage->shaderStage;
	shaderParm_t	*shaderParm;
	token_t			token;
	int				i, parms;

	if (!shaderStage->program){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'shaderParm' not allowed without shaders in material '%s'\n", material->name);
		return false;
	}

	if (shaderStage->numShaderParms == MAX_SHADER_PARMS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_SHADER_PARMS hit in material '%s'\n", material->name);
		return false;
	}
	shaderParm = &shaderStage->shaderParms[shaderStage->numShaderParms++];

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'shaderParm' in material '%s'\n", material->name);
		return false;
	}

	shaderParm->uniform = R_GetProgramUniform(shaderStage->program, token.string);
	if (!shaderParm->uniform || shaderParm->uniform->type != UT_CUSTOM){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid uniform name for 'shaderParm' in material '%s'\n", material->name);
		return false;
	}

	if (shaderParm->uniform->size != 1){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid uniform size for 'shaderParm' in material '%s'\n", material->name);
		return false;
	}

	if (shaderParm->uniform->format == GL_FLOAT)
		parms = 1;
	else if (shaderParm->uniform->format == GL_FLOAT_VEC2)
		parms = 2;
	else if (shaderParm->uniform->format == GL_FLOAT_VEC3)
		parms = 3;
	else if (shaderParm->uniform->format == GL_FLOAT_VEC4)
		parms = 4;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid uniform format for 'shaderParm' in material '%s'\n", material->name);
		return false;
	}

	for (i = 0; i < parms; i++){
		if (!R_ParseExpression(script, material, &shaderParm->registers[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'shaderParm' in material '%s'\n", material->name);
			return false;
		}

		if (i < parms - 1){
			if (!PS_ExpectTokenString(script, &token, ",", true)){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'shaderParm' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	return true;
}

/*
 ==================
 R_ParseStageShaderMap
 ==================
*/
static bool R_ParseStageShaderMap (script_t *script, material_t *material, stage_t *stage){

	shaderStage_t	*shaderStage = &stage->shaderStage;
	shaderMap_t		*shaderMap;
	token_t			token;
	char			name[MAX_PATH_LENGTH];
	int				flags = 0;
	textureFilter_t	filter = TF_DEFAULT;
	textureWrap_t	wrap = TW_REPEAT;

	if (!shaderStage->program){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'shaderMap' not allowed without shaders in material '%s'\n", material->name);
		return false;
	}

	if (shaderStage->numShaderMaps == MAX_SHADER_MAPS){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_SHADER_MAPS hit in material '%s'\n", material->name);
		return false;
	}
	shaderMap = &shaderStage->shaderMaps[shaderStage->numShaderMaps++];

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'shaderMap' in material '%s'\n", material->name);
		return false;
	}

	shaderMap->uniform = R_GetProgramUniform(shaderStage->program, token.string);
	if (!shaderMap->uniform || shaderMap->uniform->type != UT_CUSTOM){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid uniform name for 'shaderMap' in material '%s'\n", material->name);
		return false;
	}

	if (shaderMap->uniform->size != 1){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid uniform size for 'shaderMap' in material '%s'\n", material->name);
		return false;
	}

	if (shaderMap->uniform->format == GL_SAMPLER_2D)
		R_SetProgramSampler(shaderStage->program, shaderMap->uniform, shaderStage->numShaderMaps - 1);
	else if (shaderMap->uniform->format == GL_SAMPLER_CUBE)
		R_SetProgramSampler(shaderStage->program, shaderMap->uniform, shaderStage->numShaderMaps - 1);
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid uniform format for 'shaderMap' in material '%s'\n", material->name);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'shaderMap' in material '%s'\n", material->name);
			return false;
		}

		if (!Str_ICompare(token.string, "noPicMip"))
			flags |= TF_NOPICMIP;
		else if (!Str_ICompare(token.string, "uncompressed"))
			flags |= TF_UNCOMPRESSED;
		else if (!Str_ICompare(token.string, "highQuality"))
			flags |= (TF_NOPICMIP | TF_UNCOMPRESSED);
		else if (!Str_ICompare(token.string, "nearest"))
			filter = TF_NEAREST;
		else if (!Str_ICompare(token.string, "linear"))
			filter = TF_LINEAR;
		else if (!Str_ICompare(token.string, "mirroredRepeat"))
			wrap = TW_REPEAT_MIRRORED;
		else if (!Str_ICompare(token.string, "clamp"))
			wrap = TW_CLAMP;
		else if (!Str_ICompare(token.string, "zeroClamp"))
			wrap = TW_CLAMP_TO_ZERO;
		else if (!Str_ICompare(token.string, "alphaZeroClamp"))
			wrap = TW_CLAMP_TO_ZERO_ALPHA;
		else
			break;
	}

	if (material->type == MT_NOMIP){
		flags |= TF_NOPICMIP;

		if (filter == TF_DEFAULT)
			filter = TF_LINEAR;
	}

	if (!Str_ICompare(token.string, "normalMap")){
		if (!R_ParseImageProgram(script, material, name, sizeof(name))){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'shaderMap' in material '%s'\n", material->name);
			return false;
		}

		if (shaderMap->uniform->format != GL_SAMPLER_2D){
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid texture '%s' for material '%s'\n", name, material->name);
			return false;
		}

		shaderMap->texture = R_FindTexture(name, flags | TF_BUMP, filter, wrap);
		if (!shaderMap->texture){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
			return false;
		}
	}
	else if (!Str_ICompare(token.string, "cubeMap")){
		if (material->type != MT_GENERIC){
			Com_Printf(S_COLOR_YELLOW "WARNING: 'cubeMap' not allowed for 'shaderMap' in material '%s'\n", material->name);
			return false;
		}

		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'shaderMap' in material '%s'\n", material->name);
			return false;
		}

		if (shaderMap->uniform->format != GL_SAMPLER_CUBE){
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid texture '%s' for material '%s'\n", token.string, material->name);
			return false;
		}

		shaderMap->texture = R_FindCubeTexture(token.string, flags, filter, false);
		if (!shaderMap->texture){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", token.string, material->name);
			return false;
		}
	}
	else if (!Str_ICompare(token.string, "cameraCubeMap")){
		if (material->type != MT_GENERIC){
			Com_Printf(S_COLOR_YELLOW "WARNING: 'cameraCubeMap' not allowed for 'shaderMap' in material '%s'\n", material->name);
			return false;
		}

		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'shaderMap' in material '%s'\n", material->name);
			return false;
		}

		if (shaderMap->uniform->format != GL_SAMPLER_CUBE){
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid texture '%s' for material '%s'\n", token.string, material->name);
			return false;
		}

		shaderMap->texture = R_FindCubeTexture(token.string, flags, filter, true);
		if (!shaderMap->texture){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", token.string, material->name);
			return false;
		}
	}
	else if (!Str_ICompare(token.string, "videoMap")){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'shaderMap' in material '%s'\n", material->name);
			return false;
		}

		if (material->type == MT_NOMIP || r_inGameVideos->integerValue){
			if (shaderMap->uniform->format != GL_SAMPLER_2D){
				Com_Printf(S_COLOR_YELLOW "WARNING: invalid video '%s' for material '%s'\n", token.string, material->name);
				return false;
			}

			shaderMap->cinematicHandle = CIN_PlayCinematic(token.string, CIN_LOOPING | CIN_SILENT);
			if (!shaderMap->cinematicHandle){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find video '%s' for material '%s'\n", token.string, material->name);
				return false;
			}

			shaderMap->texture = rg.cinematicTextures[shaderMap->cinematicHandle - 1];
		}
		else {
			Str_Copy(name, token.string, sizeof(name));
			Str_StripFileExtension(name);

			if (shaderMap->uniform->format != GL_SAMPLER_2D){
				Com_Printf(S_COLOR_YELLOW "WARNING: invalid texture '%s' for material '%s'\n", name, material->name);
				return false;
			}

			shaderMap->texture = R_FindTexture(name, flags, filter, TW_CLAMP_TO_ZERO);
			if (!shaderMap->texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
				return false;
			}
		}
	}
	else {
		PS_UnreadToken(script, &token);

		if (!R_ParseImageProgram(script, material, name, sizeof(name))){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'shaderMap' in material '%s'\n", material->name);
			return false;
		}

		if (shaderMap->uniform->format != GL_SAMPLER_2D){
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid texture '%s' for material '%s'\n", name, material->name);
			return false;
		}

		shaderMap->texture = R_FindTexture(name, flags, filter, wrap);
		if (!shaderMap->texture){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture '%s' for material '%s'\n", name, material->name);
			return false;
		}
	}

	return true;
}

/*
 ==================
 R_ParseStageSpecularParms
 ==================
*/
static bool R_ParseStageSpecularParms (script_t *script, material_t *material, stage_t *stage){

	token_t	token;
	int		i;

	for (i = 0; i < 2; i++){
		if (!PS_ReadFloat(script, &stage->parms[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'specularParms' in material '%s'\n", material->name);
			return false;
		}

		if (stage->parms[i] <= 0.0f){
			Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'specularParms' in material '%s'\n", stage->parms[i], material->name);
			return false;
		}

		if (i < 1){
			if (!PS_ExpectTokenString(script, &token, ",", true)){
				Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'specularParms' in material '%s'\n", token.string, material->name);
				return false;
			}
		}
	}

	return true;
}

/*
 ==================
 R_ParseStagePrivatePolygonOffset
 ==================
*/
static bool R_ParseStagePrivatePolygonOffset (script_t *script, material_t *material, stage_t *stage){

	token_t	token;

	stage->drawState |= DS_POLYGONOFFSET;

	stage->polygonOffset = 1.0f;

	if (PS_CheckTokenType(script, &token, TT_NUMBER, 0))
		stage->polygonOffset = token.floatValue;
	else if (PS_CheckTokenType(script, &token, TT_PUNCTUATION, PT_SUB)){
		if (!PS_ExpectTokenType(script, &token, TT_NUMBER, 0)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'privatePolygonOffset' in material '%s'\n", material->name);
			return false;
		}

		stage->polygonOffset = -token.floatValue;
	}

	if (!stage->polygonOffset){
		stage->drawState &= ~DS_POLYGONOFFSET;
		return true;
	}

	return true;
}

/*
 ==================
 R_ParseStageBlend
 ==================
*/
static bool R_ParseStageBlend (script_t *script, material_t *material, stage_t *stage){

	token_t	token;

	stage->drawState |= DS_BLEND;

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'blend' in material '%s'\n", material->name);
		return false;
	}

	if (!Str_ICompare(token.string, "none")){
		stage->blendSrc = GL_ZERO;
		stage->blendDst = GL_ONE;
	}
	else if (!Str_ICompare(token.string, "add")){
		stage->blendSrc = GL_ONE;
		stage->blendDst = GL_ONE;
	}
	else if (!Str_ICompare(token.string, "modulate") || !Str_ICompare(token.string, "filter")){
		stage->blendSrc = GL_DST_COLOR;
		stage->blendDst = GL_ZERO;
	}
	else if (!Str_ICompare(token.string, "blend") || !Str_ICompare(token.string, "alpha")){
		stage->blendSrc = GL_SRC_ALPHA;
		stage->blendDst = GL_ONE_MINUS_SRC_ALPHA;
	}
	else {
		if (!Str_ICompare(token.string, "GL_ZERO"))
			stage->blendSrc = GL_ZERO;
		else if (!Str_ICompare(token.string, "GL_ONE"))
			stage->blendSrc = GL_ONE;
		else if (!Str_ICompare(token.string, "GL_SRC_COLOR"))
			stage->blendSrc = GL_SRC_COLOR;
		else if (!Str_ICompare(token.string, "GL_ONE_MINUS_SRC_COLOR"))
			stage->blendSrc = GL_ONE_MINUS_SRC_COLOR;
		else if (!Str_ICompare(token.string, "GL_DST_COLOR"))
			stage->blendSrc = GL_DST_COLOR;
		else if (!Str_ICompare(token.string, "GL_ONE_MINUS_DST_COLOR"))
			stage->blendSrc = GL_ONE_MINUS_DST_COLOR;
		else if (!Str_ICompare(token.string, "GL_SRC_ALPHA"))
			stage->blendSrc = GL_SRC_ALPHA;
		else if (!Str_ICompare(token.string, "GL_ONE_MINUS_SRC_ALPHA"))
			stage->blendSrc = GL_ONE_MINUS_SRC_ALPHA;
		else if (!Str_ICompare(token.string, "GL_DST_ALPHA"))
			stage->blendSrc = GL_DST_ALPHA;
		else if (!Str_ICompare(token.string, "GL_ONE_MINUS_DST_ALPHA"))
			stage->blendSrc = GL_ONE_MINUS_DST_ALPHA;
		else if (!Str_ICompare(token.string, "GL_SRC_ALPHA_SATURATE"))
			stage->blendSrc = GL_SRC_ALPHA_SATURATE;
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'blend' parameter '%s' in material '%s'\n", token.string, material->name);
			return false;
		}

		if (!PS_ExpectTokenString(script, &token, ",", true)){
			Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for 'blend' in material '%s'\n", token.string, material->name);
			return false;
		}

		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'blend' in material '%s'\n", material->name);
			return false;
		}

		if (!Str_ICompare(token.string, "GL_ZERO"))
			stage->blendDst = GL_ZERO;
		else if (!Str_ICompare(token.string, "GL_ONE"))
			stage->blendDst = GL_ONE;
		else if (!Str_ICompare(token.string, "GL_SRC_COLOR"))
			stage->blendDst = GL_SRC_COLOR;
		else if (!Str_ICompare(token.string, "GL_ONE_MINUS_SRC_COLOR"))
			stage->blendDst = GL_ONE_MINUS_SRC_COLOR;
		else if (!Str_ICompare(token.string, "GL_DST_COLOR"))
			stage->blendDst = GL_DST_COLOR;
		else if (!Str_ICompare(token.string, "GL_ONE_MINUS_DST_COLOR"))
			stage->blendDst = GL_ONE_MINUS_DST_COLOR;
		else if (!Str_ICompare(token.string, "GL_SRC_ALPHA"))
			stage->blendDst = GL_SRC_ALPHA;
		else if (!Str_ICompare(token.string, "GL_ONE_MINUS_SRC_ALPHA"))
			stage->blendDst = GL_ONE_MINUS_SRC_ALPHA;
		else if (!Str_ICompare(token.string, "GL_DST_ALPHA"))
			stage->blendDst = GL_DST_ALPHA;
		else if (!Str_ICompare(token.string, "GL_ONE_MINUS_DST_ALPHA"))
			stage->blendDst = GL_ONE_MINUS_DST_ALPHA;
		else if (!Str_ICompare(token.string, "GL_SRC_ALPHA_SATURATE"))
			stage->blendDst = GL_SRC_ALPHA_SATURATE;
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'blend' parameter '%s' in material '%s'\n", token.string, material->name);
			return false;
		}
	}

	if (!stage->blendMode){
		stage->blendMode = GL_FUNC_ADD;
		return true;
	}

	return true;
}

/*
 ==================
 R_ParseStageBlendEquation
 ==================
*/
static bool R_ParseStageBlendEquation (script_t *script, material_t *material, stage_t *stage){

	token_t	token;

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'blendEquation' in material '%s'\n", material->name);
		return false;
	}

	if (!Str_ICompare(token.string, "GL_FUNC_ADD"))
		stage->blendMode = GL_FUNC_ADD;
	else if (!Str_ICompare(token.string, "GL_FUNC_SUBTRACT"))
		stage->blendMode = GL_FUNC_SUBTRACT;
	else if (!Str_ICompare(token.string, "GL_FUNC_REVERSE_SUBTRACT"))
		stage->blendMode = GL_FUNC_REVERSE_SUBTRACT;
	else if (!Str_ICompare(token.string, "GL_MIN"))
		stage->blendMode = GL_MIN;
	else if (!Str_ICompare(token.string, "GL_MAX"))
		stage->blendMode = GL_MAX;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'blendEquation' parameter '%s' in material '%s'\n", token.string, material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageAlphaTest
 ==================
*/
static bool R_ParseStageAlphaTest (script_t *script, material_t *material, stage_t *stage){

	stage->drawState |= DS_ALPHATEST;

	if (!R_ParseExpression(script, material, &stage->alphaTestRegister)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing expression parameters for 'alphaTest' in material '%s'\n", material->name);
		return false;
	}

	return true;
}

/*
 ==================
 R_ParseStageIgnoreAlphaTest
 ==================
*/
static bool R_ParseStageIgnoreAlphaTest (script_t *script, material_t *material, stage_t *stage){

	stage->drawState |= DS_IGNOREALPHATEST;

	return true;
}

/*
 ==================
 R_ParseStageMaskRed
 ==================
*/
static bool R_ParseStageMaskRed (script_t *script, material_t *material, stage_t *stage){

	stage->drawState |= DS_MASKRED;

	return true;
}

/*
 ==================
 R_ParseStageMaskGreen
 ==================
*/
static bool R_ParseStageMaskGreen (script_t *script, material_t *material, stage_t *stage){

	stage->drawState |= DS_MASKGREEN;

	return true;
}

/*
 ==================
 R_ParseStageMaskBlue
 ==================
*/
static bool R_ParseStageMaskBlue (script_t *script, material_t *material, stage_t *stage){

	stage->drawState |= DS_MASKBLUE;

	return true;
}

/*
 ==================
 R_ParseStageMaskColor
 ==================
*/
static bool R_ParseStageMaskColor (script_t *script, material_t *material, stage_t *stage){

	stage->drawState |= (DS_MASKRED | DS_MASKGREEN | DS_MASKBLUE);

	return true;
}

/*
 ==================
 R_ParseStageMaskAlpha
 ==================
*/
static bool R_ParseStageMaskAlpha (script_t *script, material_t *material, stage_t *stage){

	stage->drawState |= DS_MASKALPHA;

	return true;
}


// ============================================================================

typedef struct {
	const char *				name;
	bool						(*parse)(script_t *script, material_t *material);
} materialGlobalKeyword_t;

typedef struct {
	const char *				name;
	bool						(*parse)(script_t *script, material_t *material, stage_t *stage);
} materialStageKeyword_t;

static materialGlobalKeyword_t	r_materialGlobalKeywords[] = {
	{"surfaceParm",					R_ParseGlobalSurfaceParm},
	{"if",							R_ParseGlobalIf},
	{"translucent",					R_ParseGlobalTranslucent},
	{"forceOverlays",				R_ParseGlobalForceOverlays},
	{"noOverlays",					R_ParseGlobalNoOverlays},
	{"forceShadows",				R_ParseGlobalForceShadows},
	{"noShadows",					R_ParseGlobalNoShadows},
	{"noAmbient",					R_ParseGlobalNoAmbient},
	{"noBlend",						R_ParseGlobalNoBlend},
	{"noFog",						R_ParseGlobalNoFog},
	{"updateCurrentColor",			R_ParseGlobalUpdateCurrentColor},
	{"updateCurrentDepth",			R_ParseGlobalUpdateCurrentDepth},
	{"sort",						R_ParseGlobalSort},
	{"ambientLight",				R_ParseGlobalAmbientLight},
	{"blendLight",					R_ParseGlobalBlendLight},
	{"fogLight",					R_ParseGlobalFogLight},
	{"lightFalloffImage",			R_ParseGlobalLightFalloffImage},
	{"lightCubeImage",				R_ParseGlobalLightCubeImage},
	{"spectrum",					R_ParseGlobalSpectrum},
	{"backSided",					R_ParseGlobalBackSided},
	{"twoSided",					R_ParseGlobalTwoSided},
	{"polygonOffset",				R_ParseGlobalPolygonOffset},
	{"decalMacro",					R_ParseGlobalDecalMacro},
	{"decalInfo",					R_ParseGlobalDecalInfo},
	{"deform",						R_ParseGlobalDeform},
	{"bumpMap",						R_ParseGlobalBumpMap},
	{"diffuseMap",					R_ParseGlobalDiffuseMap},
	{"specularMap",					R_ParseGlobalSpecularMap},	
	{NULL,							NULL}
};

static materialStageKeyword_t	r_materialStageKeywords[] = {
	{"shaderQuality",				R_ParseStageShaderQuality},
	{"if",							R_ParseStageIf},
	{"noPicMip",					R_ParseStageNoPicMip},
	{"uncompressed",				R_ParseStageUncompressed},
	{"highQuality",					R_ParseStageHighQuality},
	{"nearest",						R_ParseStageNearest},
	{"linear",						R_ParseStageLinear},
	{"mirroredRepeat",				R_ParseStageMirroredRepeat},
	{"clamp",						R_ParseStageClamp},
	{"zeroClamp",					R_ParseStageZeroClamp},
	{"alphaZeroClamp",				R_ParseStageAlphaZeroClamp},
	{"map",							R_ParseStageMap},
	{"bumpMap",						R_ParseStageBumpMap},
	{"diffuseMap",					R_ParseStageDiffuseMap},
	{"specularMap",					R_ParseStageSpecularMap},	
	{"cubeMap",						R_ParseStageCubeMap},
	{"cameraCubeMap",				R_ParseStageCameraCubeMap},
	{"videoMap",					R_ParseStageVideoMap},
	{"skyRenderMap",				R_ParseStageSkyRenderMap},
	{"mirrorRenderMap",				R_ParseStageMirrorRenderMap},
	{"remoteRenderMap",				R_ParseStageRemoteRenderMap},
	{"texGen",						R_ParseStageTexGen},
	{"translate",					R_ParseStageTranslate},
	{"scroll",						R_ParseStageScroll},
	{"scale",						R_ParseStageScale},
	{"centerScale",					R_ParseStageCenterScale},
	{"shear",						R_ParseStageShear},
	{"rotate",						R_ParseStageRotate},
	{"red",							R_ParseStageRed},
	{"green",						R_ParseStageGreen},
	{"blue",						R_ParseStageBlue},
	{"alpha",						R_ParseStageAlpha},
	{"rgb",							R_ParseStageRGB},
	{"rgba",						R_ParseStageRGBA},
	{"color",						R_ParseStageColor},
	{"colored",						R_ParseStageColored},
	{"vertexColor",					R_ParseStageVertexColor},
	{"inverseVertexColor",			R_ParseStageInverseVertexColor},
	{"vertexShader",				R_ParseStageVertexShader},
	{"fragmentShader",				R_ParseStageFragmentShader},
	{"shader",						R_ParseStageShader},
	{"shaderParm",					R_ParseStageShaderParm},
	{"shaderMap",					R_ParseStageShaderMap},
	{"specularParms",				R_ParseStageSpecularParms},
	{"privatePolygonOffset",		R_ParseStagePrivatePolygonOffset},
	{"blend",						R_ParseStageBlend},
	{"blendEquation",				R_ParseStageBlendEquation},
	{"alphaTest",					R_ParseStageAlphaTest},
	{"ignoreAlphaTest",				R_ParseStageIgnoreAlphaTest},
	{"maskRed",						R_ParseStageMaskRed},
	{"maskGreen",					R_ParseStageMaskGreen},
	{"maskBlue",					R_ParseStageMaskBlue},
	{"maskColor",					R_ParseStageMaskColor},
	{"maskAlpha",					R_ParseStageMaskAlpha},
	{NULL,							NULL}
};


/*
 ==================
 R_ParseGlobalKeyword
 ==================
*/
static bool R_ParseGlobalKeyword (script_t *script, const char *keyword, material_t *material){

	materialGlobalKeyword_t	*k;

	for (k = r_materialGlobalKeywords; k->name; k++){
		if (!Str_ICompare(k->name, keyword))
			return k->parse(script, material);
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown global keyword '%s' in material '%s'\n", keyword, material->name);

	return false;
}

/*
 ==================
 R_ParseStageKeyword
 ==================
*/
static bool R_ParseStageKeyword (script_t *script, const char *keyword, material_t *material, stage_t *stage){

	materialStageKeyword_t	*k;

	for (k = r_materialStageKeywords; k->name; k++){
		if (!Str_ICompare(k->name, keyword))
			return k->parse(script, material, stage);
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown stage keyword '%s' in material '%s'\n", keyword, material->name);

	return false;
}

/*
 ==================
 R_CheckShaderQuality
 ==================
*/
static bool R_CheckShaderQuality (script_t *script, material_t *material){

	token_t	token;
	int		level;

	if (!PS_CheckTokenString(script, &token, "shaderQuality", false))
		return true;

	// Parse the operator
	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'shaderQuality' in material '%s', discarded stage\n", material->name);
		return false;
	}

	// Parse the shader quality level
	if (!PS_ReadInteger(script, &level)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'shaderQuality' in material '%s', discarded stage\n", material->name);
		return false;
	}

	// Compare the shader quality level
	if (!Str_ICompare(token.string, ">"))
		return (r_shaderQuality->integerValue > level);
	if (!Str_ICompare(token.string, "<"))
		return (r_shaderQuality->integerValue < level);
	if (!Str_ICompare(token.string, ">="))
		return (r_shaderQuality->integerValue >= level);
	if (!Str_ICompare(token.string, "<="))
		return (r_shaderQuality->integerValue <= level);
	if (!Str_ICompare(token.string, "=="))
		return (r_shaderQuality->integerValue == level);
	if (!Str_ICompare(token.string, "!="))
		return (r_shaderQuality->integerValue != level);

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'shaderQuality' parameter '%s' in material '%s', discarded stage\n", token.string, material->name);

	return false;
}

/*
 ==================
 R_ParseMaterial
 ==================
*/
static bool R_ParseMaterial (script_t *script, material_t *material){

	token_t	token;
	stage_t	*stage;

	// Parse the material
	if (!PS_ExpectTokenString(script, &token, "{", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in material '%s'\n", token.string, material->name);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in material '%s'\n", material->name);
			return false;	// End of script
		}

		if (!Str_Compare(token.string, "}"))
			break;			// End of material

		// Parse a stage
		if (!Str_Compare(token.string, "{")){
			// If the required shader quality level is not satisfied, discard
			// this stage
			if (!R_CheckShaderQuality(script, material)){
				PS_SkipBracedSection(script, 1);
				continue;
			}

			// Create a new stage
			if (material->numStages == MAX_STAGES){
				Com_Printf(S_COLOR_YELLOW "WARNING: MAX_STAGES hit in material '%s'\n", material->name);
				return false;
			}

			stage = &material->stages[material->numStages++];

			// Parse it
			while (1){
				if (!PS_ReadToken(script, &token)){
					Com_Printf(S_COLOR_YELLOW "WARNING: no matching '}' in material '%s'\n", material->name);
					return false;	// End of script
				}

				if (!Str_Compare(token.string, "}"))
					break;			// End of stage

				// Parse a stage keyword
				if (!R_ParseStageKeyword(script, token.string, material, stage))
					return false;
			}

			continue;
		}

		// Parse a global keyword
		if (!R_ParseGlobalKeyword(script, token.string, material))
			return false;
	}

	return true;
}

/*
 ==================
 R_ParseMaterialFile
 ==================
*/
static void R_ParseMaterialFile (script_t *script){

	token_t			token;
	materialDef_t	*materialDef;
	script_t		*scriptBlock;
	const char		*text;
	int				length;
	uint			hashKey;

	while (1){
		// Parse the name
		if (!PS_ReadToken(script, &token))
			break;		// End of script

		// Parse the script
		text = script->text;

		PS_SkipBracedSection(script, 0);

		length = script->text - text;

		// Check if there's a definition for this material already
		hashKey = Str_HashKey(token.string, MATERIALS_HASH_SIZE, false);

		for (materialDef = r_materialDefsHashTable[hashKey]; materialDef; materialDef = materialDef->nextHash){
			if (!Str_ICompare(materialDef->name, token.string))
				break;
		}

		// If it already exists, replace it
		if (materialDef){
			Mem_Free(materialDef->text);

			materialDef->text = (char *)Mem_ClearedAlloc(length + 1, TAG_RENDERER);
			materialDef->length = length;
			Str_Copy(materialDef->source, script->name, sizeof(materialDef->source));
			materialDef->line = token.line;

			Str_Copy(materialDef->text, text, length + 1);

//			materialDef->type = MT_NONE;
//			materialDef->surfaceParm = SURFACEPARM_NONE;

			continue;
		}

		// Store the material definition
		materialDef = (materialDef_t *)Mem_ClearedAlloc(sizeof(materialDef_t), TAG_RENDERER);

		Str_Copy(materialDef->name, token.string, sizeof(materialDef->name));
		materialDef->text = (char *)Mem_ClearedAlloc(length + 1, TAG_RENDERER);
		materialDef->length = length;
		Str_Copy(materialDef->source, script->name, sizeof(materialDef->source));
		materialDef->line = token.line;

		Str_Copy(materialDef->text, text, length + 1);

		materialDef->type = MT_NONE;
		materialDef->surfaceParm = SURFACEPARM_NONE;

		// Add to hash table
		materialDef->nextHash = r_materialDefsHashTable[hashKey];
		r_materialDefsHashTable[hashKey] = materialDef;

		// We must parse surfaceParm commands here, because
		// R_FindMaterial needs this for correct material loading.
		// Proper syntax checking will be done when the material is
		// actually loaded.
		scriptBlock = PS_LoadScriptMemory(materialDef->name, materialDef->source, materialDef->length, materialDef->line);
		if (scriptBlock){
			while (1){
				if (!PS_ReadToken(scriptBlock, &token))
					break;		// End of data

				if (!Str_ICompare(token.string, "surfaceParm")){
					if (!PS_ReadToken(scriptBlock, &token))
						continue;

					if (!Str_ICompare(token.string, "lighting"))
						materialDef->surfaceParm |= SURFACEPARM_LIGHTING;
					else if (!Str_ICompare(token.string, "sky"))
						materialDef->surfaceParm |= SURFACEPARM_SKY;
					else if (!Str_ICompare(token.string, "warp"))
						materialDef->surfaceParm |= SURFACEPARM_WARP;
					else if (!Str_ICompare(token.string, "trans33"))
						materialDef->surfaceParm |= SURFACEPARM_TRANS33;
					else if (!Str_ICompare(token.string, "trans66"))
						materialDef->surfaceParm |= SURFACEPARM_TRANS66;
					else if (!Str_ICompare(token.string, "flowing"))
						materialDef->surfaceParm |= SURFACEPARM_FLOWING;
					else
						continue;

					materialDef->type = MT_GENERIC;
				}
			}

			PS_FreeScript(scriptBlock);
		}
	}
}


/*
 ==============================================================================

 MATERIAL REGISTRATION

 ==============================================================================
*/


/*
 ==================
 R_NewMaterial
 ==================
*/
static material_t *R_NewMaterial (){

	material_t	*material;

	// Clear the material
	material = &r_parseMaterial;
	Mem_Fill(material, 0, sizeof(r_parseMaterial));

	// Clear the stages
	material->stages = r_parseStages;
	Mem_Fill(material->stages, 0, sizeof(r_parseStages));

	// Clear the expression ops
	material->expressionOps = r_parseExpressionOps;
	Mem_Fill(material->expressionOps, 0, sizeof(r_parseExpressionOps));

	// Clear the expression registers
	material->expressionRegisters = r_parseExpressionRegisters;
	Mem_Fill(material->expressionRegisters, 0, sizeof(r_parseExpressionRegisters));

	return material;
}


/*
 ==================
 R_CreateDefaultMaterial
 ==================
*/
static material_t *R_CreateDefaultMaterial (const char *name, materialType_t type, surfaceParm_t surfaceParm){

	material_t	*material;

	// Create a new material
	material = R_NewMaterial();

	// Fill it in
	Str_Copy(material->name, name, sizeof(material->name));
	material->index = r_numMaterials;
	material->type = type;
	material->flags = MF_DEFAULTED;
	material->surfaceParm = surfaceParm;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;

	if (type == MT_LIGHT)
		material->stages->textureStage.texture = rg.attenuationTexture;
	else
		material->stages->textureStage.texture = rg.defaultTexture;

	material->numStages++;

	return material;
}

/*
 ==================
 R_CreateMaterial
 ==================
*/
static material_t *R_CreateMaterial (const char *name, materialType_t type, surfaceParm_t surfaceParm, materialDef_t *materialDef){

	material_t	*material;
	script_t	*script;
	int			i;

	// Create a new material
	material = R_NewMaterial();

	// Fill it in
	Str_Copy(material->name, name, sizeof(material->name));
	material->index = r_numMaterials;
	material->type = type;
	material->flags = 0;
	material->surfaceParm = surfaceParm;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;

	// If we have a definition, create an explicit material
	if (materialDef){
		material->flags |= MF_EXPLICIT;

		// Load the script text
		script = PS_LoadScriptMemory(materialDef->source, materialDef->text, materialDef->length, materialDef->line);
		if (!script)
			return R_CreateDefaultMaterial(name, type, surfaceParm);

		PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

		// Parse it
		if (!R_ParseMaterial(script, material)){
			PS_FreeScript(script);

			return R_CreateDefaultMaterial(name, type, surfaceParm);
		}

		// Free the script text
		PS_FreeScript(script);

		return material;
	}

	// Otherwise create an implicit material
	switch (material->type){
	case MT_GENERIC:
		if (material->surfaceParm & SURFACEPARM_LIGHTING){
			material->stages[material->numStages].textureStage.texture = R_FindTexture(Str_VarArgs("%s_local", material->name), TF_BUMP, TF_DEFAULT, TW_REPEAT);
			if (!material->stages[material->numStages].textureStage.texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default\n", material->name);

				material->stages[material->numStages].textureStage.texture = rg.flatTexture;
			}
			material->stages[material->numStages++].lighting = SL_BUMP;

			material->stages[material->numStages].textureStage.texture = R_FindTexture(Str_VarArgs("%s_d", material->name), TF_DIFFUSE, TF_DEFAULT, TW_REPEAT);
			if (!material->stages[material->numStages].textureStage.texture){
				material->stages[material->numStages].textureStage.texture = R_FindTexture(Str_VarArgs("%s", material->name), TF_DIFFUSE, TF_DEFAULT, TW_REPEAT);
				if (!material->stages[material->numStages].textureStage.texture){
					Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default\n", material->name);

					material->stages[material->numStages].textureStage.texture = rg.whiteTexture;
				}
			}
			material->stages[material->numStages++].lighting = SL_DIFFUSE;

			material->stages[material->numStages].textureStage.texture = R_FindTexture(Str_VarArgs("%s_s", material->name), TF_SPECULAR, TF_DEFAULT, TW_REPEAT);
			if (!material->stages[material->numStages].textureStage.texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default\n", material->name);

				material->stages[material->numStages].textureStage.texture = rg.blackTexture;
			}
			material->stages[material->numStages++].lighting = SL_SPECULAR;

			// Add flowing if needed
			if (material->surfaceParm & SURFACEPARM_FLOWING){
				for (i = 0; i < material->numStages; i++){
					material->stages[i].textureStage.texMods[0] = TM_TRANSLATE;
					material->stages[i].textureStage.texModsRegisters[0][0] = EXP_REGISTER_PARM5;
					material->stages[i].textureStage.texModsRegisters[0][1] = EXP_REGISTER_CONSTANT_ZERO;
					material->stages[i].textureStage.numTexMods++;
				}
			}
		}
		else if (material->surfaceParm & SURFACEPARM_SKY){
			material->flags = MF_NOOVERLAYS | MF_NOSHADOWS;

			material->stages[material->numStages].textureStage.texture = R_FindCubeTexture(Str_VarArgs("%s", material->name), TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, false);
			if (!material->stages[material->numStages].textureStage.texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default\n", material->name);

				material->stages[material->numStages].textureStage.texture = rg.defaultTexture;
			}
			else
				material->stages[material->numStages].textureStage.texGen = TG_SKYBOX;

			material->numStages++;
		}
		else {
			material->stages[material->numStages].textureStage.texture = R_FindTexture(Str_VarArgs("%s", material->name), TF_NOPICMIP | TF_UNCOMPRESSED, TF_DEFAULT, TW_REPEAT);
			if (!material->stages[material->numStages].textureStage.texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default\n", material->name);

				material->stages[material->numStages].textureStage.texture = rg.defaultTexture;
			}

			// Make it translucent if needed
			if (material->surfaceParm & (SURFACEPARM_TRANS33 | SURFACEPARM_TRANS66)){
				material->flags = MF_NOOVERLAYS | MF_NOSHADOWS;

				material->stages[material->numStages].drawState = DS_BLEND | DS_IGNOREALPHATEST;
				material->stages[material->numStages].blendSrc = GL_SRC_ALPHA;
				material->stages[material->numStages].blendDst = GL_ONE_MINUS_SRC_ALPHA;
				material->stages[material->numStages].blendMode = GL_FUNC_ADD;
				material->stages[material->numStages].colorStage.vertexColor = VC_MODULATE;
			}

			// Make it warped if needed
			if (material->surfaceParm & SURFACEPARM_WARP){
				material->flags = MF_NOOVERLAYS | MF_NOSHADOWS;

				// Add flowing if needed
				if (material->surfaceParm & SURFACEPARM_FLOWING){
					material->stages[material->numStages].textureStage.texMods[0] = TM_TRANSLATE;
					material->stages[material->numStages].textureStage.texModsRegisters[0][0] = EXP_REGISTER_PARM4;
					material->stages[material->numStages].textureStage.texModsRegisters[0][1] = EXP_REGISTER_CONSTANT_ZERO;
					material->stages[material->numStages].textureStage.numTexMods++;
				}
			}
			else {
				// Add flowing if needed
				if (material->surfaceParm & SURFACEPARM_FLOWING){
					material->stages[material->numStages].textureStage.texMods[0] = TM_TRANSLATE;
					material->stages[material->numStages].textureStage.texModsRegisters[0][0] = EXP_REGISTER_PARM5;
					material->stages[material->numStages].textureStage.texModsRegisters[0][1] = EXP_REGISTER_CONSTANT_ZERO;
					material->stages[material->numStages].textureStage.numTexMods++;
				}
			}

			material->numStages++;
		}

		break;
	case MT_LIGHT:
		material->stages[material->numStages].textureStage.texture = R_FindTexture(material->name, TF_NOPICMIP | TF_LIGHT, TF_DEFAULT, TW_CLAMP_TO_ZERO);
		if (!material->stages[material->numStages].textureStage.texture){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default\n", material->name);

			material->stages[material->numStages].textureStage.texture = rg.attenuationTexture;
		}

		material->stages[material->numStages].colorStage.registers[0] = EXP_REGISTER_PARM0;
		material->stages[material->numStages].colorStage.registers[1] = EXP_REGISTER_PARM1;
		material->stages[material->numStages].colorStage.registers[2] = EXP_REGISTER_PARM2;
		material->stages[material->numStages].colorStage.registers[3] = EXP_REGISTER_PARM3;

		material->numStages++;

		break;
	case MT_NOMIP:
		material->stages[material->numStages].textureStage.texture = R_FindTexture(material->name, TF_NOPICMIP, TF_LINEAR, TW_CLAMP);
		if (!material->stages[material->numStages].textureStage.texture){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find texture for material '%s', using default\n", material->name);

			material->stages[material->numStages].textureStage.texture = rg.defaultTexture;
		}

		material->stages[material->numStages].drawState = DS_BLEND;
		material->stages[material->numStages].blendSrc = GL_SRC_ALPHA;
		material->stages[material->numStages].blendDst = GL_ONE_MINUS_SRC_ALPHA;
		material->stages[material->numStages].blendMode = GL_FUNC_ADD;
		material->stages[material->numStages].colorStage.vertexColor = VC_MODULATE;

		material->numStages++;

		break;
	}

	return material;
}

/*
 ==================
 R_FinishMaterial
 ==================
*/
static void R_FinishMaterial (material_t *material){

	stage_t	*stage;
	int		i, j;

	// Force the coverage to translucent for 2D materials
	if (material->type == MT_NOMIP)
		material->coverage = MC_TRANSLUCENT;

	// Force 'noOverlays' if it has 'deform'
	if (material->deform != DFRM_NONE)
		material->flags |= MF_NOOVERLAYS;

	// Force 'noShadows' if it has 'ambientLight', 'blendLight', or 'fogLight'
	if (material->lightType != LT_GENERIC)
		material->flags |= MF_NOSHADOWS;

	// Force 'updateCurrentColor' and 'updateCurrentDepth' for 2D materials
	if (material->type == MT_NOMIP)
		material->flags |= (MF_UPDATECURRENTCOLOR | MF_UPDATECURRENTDEPTH);

	// Make sure it has a 'lightFalloffImage'
	if (!material->lightFalloffImage){
		if (material->lightType == LT_FOG)
			material->lightFalloffImage = rg.fogEnterTexture;
		else if (material->lightType == LT_AMBIENT)
			material->lightFalloffImage = rg.noFalloffTexture;
		else
			material->lightFalloffImage = rg.falloffTexture;
	}

	// Make sure it has a 'lightCubeImage'
	if (!material->lightCubeImage)
		material->lightCubeImage = rg.cubicFilterTexture;

	// Check stages
	for (i = 0, stage = material->stages; i < material->numStages; i++, stage++){
		// Check whether it is a shader stage or a texture/color stage
		if (stage->shaderStage.program){
			// Make sure it is an ambient stage
			if (stage->lighting != SL_AMBIENT){
				Com_Printf(S_COLOR_YELLOW "WARNING: material '%s' has a non-ambient stage with shaders!\n", material->name);

				stage->lighting = SL_AMBIENT;
			}

			// Set the color scale and bias
			switch (stage->colorStage.vertexColor){
			case VC_IGNORE:
				stage->shaderStage.colorScale = 0.0f;
				stage->shaderStage.colorBias = 1.0f;

				break;
			case VC_MODULATE:
				stage->shaderStage.colorScale = 1.0f;
				stage->shaderStage.colorBias = 0.0f;

				break;
			case VC_INVERSE_MODULATE:
				stage->shaderStage.colorScale = -1.0f;
				stage->shaderStage.colorBias = 1.0f;

				break;
			}

			// Determine if it needs to capture the current color and/or depth
			for (j = 0; j < stage->shaderStage.numShaderMaps; j++){
				if (stage->shaderStage.shaderMaps[j].texture == rg.currentColorTexture)
					material->flags |= MF_NEEDCURRENTCOLOR;

				if (stage->shaderStage.shaderMaps[j].texture == rg.currentDepthTexture)
					material->flags |= MF_NEEDCURRENTDEPTH;
			}
		}
		else {
			// Make sure it has a texture
			if (!stage->textureStage.texture){
				Com_Printf(S_COLOR_YELLOW "WARNING: material '%s' has a stage with no texture!\n", material->name);

				if (material->type != MT_LIGHT)
					stage->textureStage.texture = rg.defaultTexture;
				else {
					if (material->lightType == LT_FOG)
						stage->textureStage.texture = rg.fogTexture;
					else if (material->lightType == LT_AMBIENT)
						stage->textureStage.texture = rg.noAttenuationTexture;
					else
						stage->textureStage.texture = rg.attenuationTexture;
				}
			}

			// Determine if the color is identity
			for (j = 0; j < 4; j++){
				if (stage->colorStage.registers[j] != EXP_REGISTER_CONSTANT_ONE)
					break;
			}

			if (j == 4)
				stage->colorStage.identity = true;
			else
				stage->colorStage.identity = false;

			// Set the color scale and bias
			switch (stage->colorStage.vertexColor){
			case VC_IGNORE:
				stage->colorStage.scale = 0.0f;
				stage->colorStage.bias = 1.0f;

				break;
			case VC_MODULATE:
				stage->colorStage.scale = 1.0f;
				stage->colorStage.bias = 0.0f;

				break;
			case VC_INVERSE_MODULATE:
				stage->colorStage.scale = -1.0f;
				stage->colorStage.bias = 1.0f;

				break;
			}

			// If a specular stage, make sure the parameters are valid
			if (stage->lighting == SL_SPECULAR){
				if (!stage->parms[0])
					stage->parms[0] = 16.0f;
				if (!stage->parms[1])
					stage->parms[1] = 2.0f;
			}

			// Determine if it needs to capture the current color and/or depth
			if (stage->textureStage.texture == rg.currentColorTexture)
				material->flags |= MF_NEEDCURRENTCOLOR;

			if (stage->textureStage.texture == rg.currentDepthTexture)
				material->flags |= MF_NEEDCURRENTDEPTH;

			// Determine if it may have alpha tested holes
			if (stage->lighting == SL_AMBIENT || stage->lighting == SL_DIFFUSE){
				if (stage->drawState & DS_ALPHATEST){
					// If not translucent, force the coverage to perforated
					if (material->coverage != MC_TRANSLUCENT)
						material->coverage = MC_PERFORATED;
				}
			}
		}

		// Count ambient stages
		if (stage->lighting == SL_AMBIENT)
			material->numAmbientStages++;
	}

	// Force 'noInteractions' and 'noAmbient' if it doesn't have non-ambient
	// stages
	if (material->numStages == material->numAmbientStages)
		material->flags |= (MF_NOINTERACTIONS | MF_NOAMBIENT);

	// Set 'sort' if unset
	if (material->sort == SORT_BAD){
		if (material->flags & MF_NEEDCURRENTCOLOR)
			material->sort = SORT_POST_PROCESS;
		else if (material->flags & MF_POLYGONOFFSET)
			material->sort = SORT_DECAL;
		else {
			if (!material->numStages || material->numStages != material->numAmbientStages || !(material->stages->drawState & DS_BLEND))
				material->sort = SORT_OPAQUE;
			else
				material->sort = SORT_MEDIUM;
		}
	}

	// If not 'sort opaque', force the coverage to translucent
	if (material->sort != SORT_OPAQUE)
		material->coverage = MC_TRANSLUCENT;

	// If a perforated material
	if (material->coverage == MC_PERFORATED){
		// Force 'noOverlays' unless it has 'forceOverlays'
		if (!(material->flags & MF_FORCEOVERLAYS))
			material->flags |= MF_NOOVERLAYS;
	}

	// If a translucent material
	if (material->coverage == MC_TRANSLUCENT){
		// Force 'noOverlays' unless it has 'forceOverlays'
		if (!(material->flags & MF_FORCEOVERLAYS))
			material->flags |= MF_NOOVERLAYS;

		// Force 'noShadows' unless it has 'forceShadows'
		if (!(material->flags & MF_FORCESHADOWS))
			material->flags |= MF_NOSHADOWS;

		// Force 'noBlend' and 'noFog'
		material->flags |= (MF_NOBLEND | MF_NOFOG);

		// Force 'ignoreAlphaTest' for all the stages
		for (i = 0, stage = material->stages; i < material->numStages; i++, stage++)
			stage->drawState |= DS_IGNOREALPHATEST;
	}
}

/*
 ==================
 R_PrecomputeMaterialRegisters
 ==================
*/
static void R_PrecomputeMaterialRegisters (material_t *material){

	float	*registers = material->expressionRegisters;
	expOp_t	*op;
	int		i;

	// Make sure the predefined registers are initialized
	registers[EXP_REGISTER_CONSTANT_ONE] = 1.0f;
	registers[EXP_REGISTER_CONSTANT_ZERO] = 0.0f;
	registers[EXP_REGISTER_TIME] = 0.0f;
	registers[EXP_REGISTER_PARM0] = 0.0f;
	registers[EXP_REGISTER_PARM1] = 0.0f;
	registers[EXP_REGISTER_PARM2] = 0.0f;
	registers[EXP_REGISTER_PARM3] = 0.0f;
	registers[EXP_REGISTER_PARM4] = 0.0f;
	registers[EXP_REGISTER_PARM5] = 0.0f;
	registers[EXP_REGISTER_PARM6] = 0.0f;
	registers[EXP_REGISTER_PARM7] = 0.0f;

	// Check for constant expressions
	for (i = 0, op = material->expressionOps; i < material->numOps; i++, op++){
		if (op->type == OP_TYPE_TABLE){
			if (op->b < EXP_REGISTER_NUM_PREDEFINED){
				if (op->b != EXP_REGISTER_CONSTANT_ONE && op->b != EXP_REGISTER_CONSTANT_ZERO)
					return;
			}
		}
		else {
			if (op->a < EXP_REGISTER_NUM_PREDEFINED){
				if (op->a != EXP_REGISTER_CONSTANT_ONE && op->a != EXP_REGISTER_CONSTANT_ZERO)
					return;
			}

			if (op->b < EXP_REGISTER_NUM_PREDEFINED){
				if (op->b != EXP_REGISTER_CONSTANT_ONE && op->b != EXP_REGISTER_CONSTANT_ZERO)
					return;
			}
		}
	}

	// Evaluate all the registers
	for (i = 0, op = material->expressionOps; i < material->numOps; i++, op++){
		switch (op->type){
		case OP_TYPE_MULTIPLY:
			registers[op->c] = registers[op->a] * registers[op->b];
			break;
		case OP_TYPE_DIVIDE:
			if (registers[op->b] == 0.0f){
				Com_Printf(S_COLOR_YELLOW "WARNING: division by zero in material '%s'\n", material->name);

				registers[op->c] = 0.0f;
				break;
			}

			registers[op->c] = registers[op->a] / registers[op->b];
			break;
		case OP_TYPE_MOD:
			if (registers[op->b] == 0.0f){
				Com_Printf(S_COLOR_YELLOW "WARNING: division by zero in material '%s'\n", material->name);

				registers[op->c] = 0.0f;
				break;
			}

			registers[op->c] = FloatToInt(FMod(registers[op->a], registers[op->b]));
			break;
		case OP_TYPE_ADD:
			registers[op->c] = registers[op->a] + registers[op->b];
			break;
		case OP_TYPE_SUBTRACT:
			registers[op->c] = registers[op->a] - registers[op->b];
			break;
		case OP_TYPE_GREATER:
			registers[op->c] = registers[op->a] > registers[op->b];
			break;
		case OP_TYPE_LESS:
			registers[op->c] = registers[op->a] < registers[op->b];
			break;
		case OP_TYPE_GEQUAL:
			registers[op->c] = registers[op->a] >= registers[op->b];
			break;
		case OP_TYPE_LEQUAL:
			registers[op->c] = registers[op->a] <= registers[op->b];
			break;
		case OP_TYPE_EQUAL:
			registers[op->c] = registers[op->a] == registers[op->b];
			break;
		case OP_TYPE_NOTEQUAL:
			registers[op->c] = registers[op->a] != registers[op->b];
			break;
		case OP_TYPE_AND:
			registers[op->c] = registers[op->a] && registers[op->b];
			break;
		case OP_TYPE_OR:
			registers[op->c] = registers[op->a] || registers[op->b];
			break;
		case OP_TYPE_TABLE:
			registers[op->c] = LUT_LookupTable(op->a, registers[op->b]);
			break;
		}
	}

	// We don't need to re-evaluate the registers during rendering, except for
	// development purposes
	material->constantExpressions = true;
}

/*
 ==================
 R_LoadMaterial
 ==================
*/
static material_t *R_LoadMaterial (material_t *newMaterial){

	material_t	*material;
	uint		hashKey;

	if (r_numMaterials == MAX_MATERIALS)
		Com_Error(ERR_DROP, "R_LoadMaterial: MAX_MATERIALS hit");

	r_materials[r_numMaterials++] = material = (material_t *)Mem_ClearedAlloc(sizeof(material_t), TAG_RENDERER);

	// Copy the material
	Mem_Copy(material, newMaterial, sizeof(material_t));

	// Copy the stages
	material->stages = (stage_t *)Mem_DupData(newMaterial->stages, material->numStages * sizeof(stage_t), TAG_RENDERER);

	// Copy the expression ops
	material->expressionOps = (expOp_t *)Mem_DupData(newMaterial->expressionOps, material->numOps * sizeof(expOp_t), TAG_RENDERER);

	// Copy the expression registers
	material->expressionRegisters = (float *)Mem_DupData(newMaterial->expressionRegisters, material->numRegisters * sizeof(float), TAG_RENDERER);

	// Make sure all the parameters are valid
	R_FinishMaterial(material);

	// Precompute registers if possible
	R_PrecomputeMaterialRegisters(material);

	// Add to hash table
	hashKey = Str_HashKey(material->name, MATERIALS_HASH_SIZE, false);

	material->nextHash = r_materialsHashTable[hashKey];
	r_materialsHashTable[hashKey] = material;

	return material;
}

/*
 ==================
 R_FindMaterial
 ==================
*/
material_t *R_FindMaterial (const char *name, materialType_t type, surfaceParm_t surfaceParm){

	material_t		*material;
	materialDef_t	*materialDef;
	uint			hashKey;

	// Performance evaluation option
	if (r_singleMaterial->integerValue && type == MT_GENERIC)
		return rg.defaultMaterial;

	// Check if already loaded
	hashKey = Str_HashKey(name, MATERIALS_HASH_SIZE, false);

	for (material = r_materialsHashTable[hashKey]; material; material = material->nextHash){
		if (material->type != type || material->surfaceParm != surfaceParm)
			continue;

		if (!Str_ICompare(material->name, name))
			return material;
	}

	// Check if there's a definition for this material
	for (materialDef = r_materialDefsHashTable[hashKey]; materialDef; materialDef = materialDef->nextHash){
		if (!Str_ICompare(materialDef->name, name)){
			if (materialDef->type == MT_NONE)
				break;

			if (materialDef->type == type && materialDef->surfaceParm == surfaceParm)
				break;
		}
	}

	// Create the material
	material = R_CreateMaterial(name, type, surfaceParm, materialDef);

	// Load it in
	return R_LoadMaterial(material);
}

/*
 ==================
 R_RegisterMaterial
 ==================
*/
material_t *R_RegisterMaterial (const char *name, bool lightingDefault){

	if (lightingDefault)
		return R_FindMaterial(name, MT_GENERIC, SURFACEPARM_LIGHTING);
	else
		return R_FindMaterial(name, MT_GENERIC, SURFACEPARM_NONE);
}

/*
 ==================
 R_RegisterMaterialLight
 ==================
*/
material_t *R_RegisterMaterialLight (const char *name){

	return R_FindMaterial(name, MT_LIGHT, SURFACEPARM_NONE);
}

/*
 ==================
 R_RegisterMaterialNoMip
 ==================
*/
material_t *R_RegisterMaterialNoMip (const char *name){

	return R_FindMaterial(name, MT_NOMIP, SURFACEPARM_NONE);
}


// ============================================================================


/*
 ==================
 R_EnumMaterialDefs
 ==================
*/
void R_EnumMaterialDefs (void (*callback)(const char *name)){

	materialDef_t	*materialDef;
	int				i;

	for (i = 0; i < MATERIALS_HASH_SIZE; i++){
		materialDef = r_materialDefsHashTable[i];

		while (materialDef){
			callback(materialDef->name);

			materialDef = materialDef->nextHash;
		}
	}
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_ListMaterials_f
 ==================
*/
static void R_ListMaterials_f (){

	material_t	*material;
	int			i;

	Com_Printf("\n");
	Com_Printf("      stg amb exp -ops -reg sort src -type-- -name-----------\n");

	for (i = 0; i < r_numMaterials; i++){
		material = r_materials[i];

		Com_Printf("%4i: ", i);

		Com_Printf("%3i %3i ", material->numStages, material->numAmbientStages);

		if (material->constantExpressions)
			Com_Printf(" C  ");
		else
			Com_Printf(" V  ");

		Com_Printf("%4i %4i ", material->numOps, material->numRegisters);

		Com_Printf("%4i ", material->sort);

		if (material->flags & MF_EXPLICIT)
			Com_Printf(" E  ");
		else
			Com_Printf(" I  ");

		switch (material->type){
		case MT_GENERIC:
			Com_Printf("GENERIC ");
			break;
		case MT_LIGHT:
			Com_Printf("LIGHT   ");
			break;
		case MT_NOMIP:
			Com_Printf("NOMIP   ");
			break;
		default:
			Com_Printf("??????? ");
			break;
		}

		Com_Printf("%s%s\n", material->name, (material->flags & MF_DEFAULTED) ? " (DEFAULTED)" : "");
	}

	Com_Printf("-------------------------------------------------------------\n");
	Com_Printf("%i total materials\n", r_numMaterials);
	Com_Printf("\n");
}

/*
 ==================
 R_ListMaterialDefs_f
 ==================
*/
static void R_ListMaterialDefs_f (){

	materialDef_t	*materialDef;
	int				total = 0, bytes = 0;
	int				i;

	Com_Printf("\n");
	Com_Printf("       -name-----------\n");

	for (i = 0; i < MATERIALS_HASH_SIZE; i++){
		materialDef = r_materialDefsHashTable[i];

		while (materialDef){
			total++;
			bytes += materialDef->length;

			Com_Printf("%5i: %s\n", total - 1, materialDef->name);

			materialDef = materialDef->nextHash;
		}
	}

	Com_Printf("-----------------------\n");
	Com_Printf("%i total material definitions\n", total);
	Com_Printf("%.2f MB of material definition text\n", bytes * (1.0f / 1048576.0f));
	Com_Printf("\n");
}

/*
 ==================
 R_PrintMaterialDef_f
 ==================
*/
static void R_PrintMaterialDef_f (){

	materialDef_t	*materialDef;
	const char		*name;
	uint			hashKey;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: printMaterialDef <name>\n");
		return;
	}

	name = Cmd_Argv(1);

	// Find the material definition
	hashKey = Str_HashKey(name, MATERIALS_HASH_SIZE, false);

	for (materialDef = r_materialDefsHashTable[hashKey]; materialDef; materialDef = materialDef->nextHash){
		if (!Str_ICompare(materialDef->name, name))
			break;
	}

	if (!materialDef){
		Com_Printf("Material definition '%s' not found\n", name);
		return;
	}

	// Print it
	Com_Printf("\n");
	Com_Printf("Source: %s (line: %i)\n", materialDef->source, materialDef->line);
	Com_Printf("\n");
	Com_Printf("%s%s\n", materialDef->name, materialDef->text);
	Com_Printf("\n");
}


/*
 ==============================================================================

 DEFAULT MATERIALS

 ==============================================================================
*/


/*
 ==================
 R_CreateDefaultMaterials
 ==================
*/
static void R_CreateDefaultMaterials (){

	material_t	*material;

	// Default material
	material = R_NewMaterial();

	Str_Copy(material->name, "_default", sizeof(material->name));
	material->index = r_numMaterials;
	material->type = MT_GENERIC;
	material->flags = MF_NOOVERLAYS | MF_NOSHADOWS | MF_NOBLEND | MF_NOFOG;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;
	material->stages->textureStage.texture = rg.defaultTexture;
	material->stages->colorStage.vertexColor = VC_MODULATE;
	material->numStages++;

	rg.defaultMaterial = R_LoadMaterial(material);

	// Default light material
	material = R_NewMaterial();

	Str_Copy(material->name, "_defaultLight", sizeof(material->name));
	material->index = r_numMaterials;
	material->type = MT_LIGHT;
	material->flags = 0;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;
	material->lightType = LT_GENERIC;
	material->lightFalloffImage = rg.falloffTexture;
	material->stages->textureStage.texture = rg.attenuationTexture;
	material->stages->colorStage.registers[0] = EXP_REGISTER_PARM0;
	material->stages->colorStage.registers[1] = EXP_REGISTER_PARM1;
	material->stages->colorStage.registers[2] = EXP_REGISTER_PARM2;
	material->stages->colorStage.registers[3] = EXP_REGISTER_PARM3;
	material->numStages++;

	rg.defaultLightMaterial = R_LoadMaterial(material);

	// Default projected light material
	material = R_NewMaterial();

	Str_Copy(material->name, "_defaultProjectedLight", sizeof(material->name));
	material->index = r_numMaterials;
	material->type = MT_LIGHT;
	material->flags = 0;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;
	material->lightType = LT_GENERIC;
	material->lightFalloffImage = rg.noFalloffTexture;
	material->stages->textureStage.texture = rg.attenuationTexture;
	material->stages->colorStage.registers[0] = EXP_REGISTER_PARM0;
	material->stages->colorStage.registers[1] = EXP_REGISTER_PARM1;
	material->stages->colorStage.registers[2] = EXP_REGISTER_PARM2;
	material->stages->colorStage.registers[3] = EXP_REGISTER_PARM3;
	material->numStages++;

	rg.defaultProjectedLightMaterial = R_LoadMaterial(material);

	// No-draw material
	material = R_NewMaterial();

	Str_Copy(material->name, "_noDraw", sizeof(material->name));
	material->index = r_numMaterials;
	material->type = MT_GENERIC;
	material->flags = MF_NOOVERLAYS | MF_NOSHADOWS;
	material->numRegisters = EXP_REGISTER_NUM_PREDEFINED;
	material->numStages++;

	rg.noDrawMaterial = R_LoadMaterial(material);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_InitMaterials
 ==================
*/
void R_InitMaterials (){

	script_t	*script;
	char		name[MAX_PATH_LENGTH];
	const char	**fileList;
	int			numFiles;
	int			i;

	Com_Printf("Initializing Materials\n");

	// Add commands
	Cmd_AddCommand("listMaterials", R_ListMaterials_f, "Lists loaded materials", NULL);
	Cmd_AddCommand("listMaterialDefs", R_ListMaterialDefs_f, "Lists material definitions", NULL);
	Cmd_AddCommand("printMaterialDef", R_PrintMaterialDef_f, "Prints a material definition", Cmd_ArgCompletion_MaterialName);

	// Load and parse .mtr files
	fileList = FS_ListFiles("materials", ".mtr", true, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Load the script file
		Str_SPrintf(name, sizeof(name), "materials/%s", fileList[i]);
		Com_Printf("...loading '%s'\n", name);

		script = PS_LoadScriptFile(name);
		if (!script){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't load '%s'\n", name);
			continue;
		}

		PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

		// Parse it
		R_ParseMaterialFile(script);

		// Free the script file
		PS_FreeScript(script);
	}

	FS_FreeFileList(fileList);

	// Create the default materials
	R_CreateDefaultMaterials();
}

/*
 ==================
 R_ShutdownMaterials
 ==================
*/
void R_ShutdownMaterials (){

	material_t	*material;
	stage_t		*stage;
	int			i, j, k;

	// Remove commands
	Cmd_RemoveCommand("listMaterials");
	Cmd_RemoveCommand("listMaterialDefs");
	Cmd_RemoveCommand("printMaterialDef");
#if 0
	// Stop all the cinematics
	for (i = 0; i < r_numMaterials; i++){
		material = r_materials[i];

		for (j = 0, stage = material->stages; j < material->numStages; j++, stage++){
			if (!stage->shaderStage.program){
				if (stage->textureStage.cinematicHandle)
					CIN_StopCinematic(stage->textureStage.cinematicHandle);

				continue;
			}

			for (k = 0; k < stage->shaderStage.numShaderMaps; k++){
				if (stage->shaderStage.shaderMaps[k].cinematicHandle)
					CIN_StopCinematic(stage->shaderStage.shaderMaps[k].cinematicHandle);
			}
		}
	}
#endif
	// Clear material definition and material lists
	Mem_Fill(r_materialDefsHashTable, 0, sizeof(r_materialDefsHashTable));

	Mem_Fill(r_materialsHashTable, 0, sizeof(r_materialsHashTable));
	Mem_Fill(r_materials, 0, sizeof(r_materials));

	r_numMaterials = 0;
}