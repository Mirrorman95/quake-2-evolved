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
// cvarSystem.c - Dynamic console variable tracking
//

// TODO:
// - Replace CVar_CommandCompletion
// - CVar_Register, CVar_Add_f, CVar_Sub_f, CVar_Reset_f
// - Replace the CVar_ForceSet with CVar_Set*


#include "common.h"


#define CVARS_HASH_SIZE				512

#define MAX_LIST_CVARS				2048

static cvar_t *			cvar_variablesHashTable[CVARS_HASH_SIZE];
cvar_t *				cvar_variables;

static bool				cvar_allowCheats;

static int				cvar_modifiedFlags;


/*
 ==================
 CVar_ValidateValue
 ==================
*/
static const char *CVar_ValidateValue (const char *value, cvarType_t type, float min, float max){

	float	val;

	switch (type){
	case CVAR_BOOL:
		val = Str_ToFloat(value);
		val = Clamp(val, 0.0f, 1.0f);

		return Str_FromFloat(val, 0);
	case CVAR_INTEGER:
		val = Str_ToFloat(value);
		if (min < max)
			val = Clamp(val, min, max);

		return Str_FromFloat(val, 0);
	case CVAR_FLOAT:
		val = Str_ToFloat(value);
		if (min < max)
			val = Clamp(val, min, max);

		return Str_FromFloat(val, -1);
	}

	return value;
}

/*
 ==================
 
 ==================
*/
cvar_t *CVar_Register (const char *name, const char *value, cvarType_t type, int flags, const char *description, float min, float max){

	cvar_t	*cvar;
	int		i;
	uint	hashKey;

	// Check if it already exists
	cvar = CVar_FindVariable(name);
	if (cvar){
		// OR in the specified flags
		cvar->flags |= flags;

		cvar->modified = true;

		// Set the modified flags
		cvar_modifiedFlags |= cvar->flags;

		if (description){
			if (cvar->description)
				Mem_Free(cvar->description);

			cvar->description = Mem_DupString(description, TAG_COMMON);
		}

		cvar->min = min;
		cvar->max = max;

		// TOOD: This needs some work?

		// Update latched cvars
		if (cvar->latchedValue){
			Mem_Free(cvar->value);

			cvar->value = cvar->latchedValue;
			cvar->latchedValue = NULL;
			cvar->integerValue = Str_ToInteger(cvar->value);
			cvar->floatValue = Str_ToFloat(cvar->value);
		}

		if (value){
			// Reset string should always use values set internally
			Mem_Free(cvar->resetValue);
			cvar->resetValue = Mem_DupString(value, TAG_COMMON);

			// Read only cvars should always use values set internally
			if (cvar->flags & CVAR_READONLY){
				Mem_Free(cvar->value);
				cvar->value = Mem_DupString(value, TAG_COMMON);
			}

		}

		return cvar;
	}

	if (!value)
		return NULL;

	// Check for invalid name
	for (i = 0; name[i]; i++){
		if (name[i] >= 'a' && name[i] <= 'z')
			continue;
		if (name[i] >= 'A' && name[i] <= 'Z')
			continue;
		if (name[i] >= '0' && name[i] <= '9')
			continue;
		if (name[i] == '_')
			continue;

		Com_Printf("Invalid cvar name '%s'\n", name);
		return NULL;
	}

	// Check for a command override
	if (Cmd_Exists(name)){
		Com_Printf("'%s' already defined as a command\n", name);
		return NULL;
	}

	// Check for invalid value
	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO)){
		if (Str_FindChar(value, '\\') || Str_FindChar(value, '\"') || Str_FindChar(value, ';')){
			Com_Printf("Invalid info cvar value\n");
			return NULL;
		}
	}
	
	// Validate the value
	value = CVar_ValidateValue(value, type, min, max);

	// Allocate a new variable
	cvar = (cvar_t *)Mem_ClearedAlloc(sizeof(cvar_t), TAG_COMMON);

	cvar->name = Mem_DupString(name, TAG_COMMON);
	cvar->value = Mem_DupString(value, TAG_COMMON);
	cvar->resetValue = Mem_DupString(value, TAG_COMMON);
	cvar->latchedValue = NULL;
	cvar->integerValue = Str_ToInteger(cvar->value);
	cvar->floatValue = Str_ToFloat(cvar->value);
	cvar->type = type;
	cvar->flags = flags;
	cvar->description = (description) ? Mem_DupString(description, TAG_COMMON) : NULL;
	cvar->min = min;
	cvar->max = max;
	cvar->modified = true;

	// Set the modified flags
	cvar_modifiedFlags |= cvar->flags;

	// Link the variable in
	cvar->next = cvar_variables;
	cvar_variables = cvar;

	// Add to hash table
	hashKey = Str_HashKey(name, CVARS_HASH_SIZE, false);

	cvar->nextHash = cvar_variablesHashTable[hashKey];
	cvar_variablesHashTable[hashKey] = cvar;

	return cvar;
}

/*
 ==================
 CVar_Set
 ==================
*/
static void CVar_Set (cvar_t *cvar, const char *value, bool force){

	Com_DPrintf("CVar_Set( \"%s\", \"%s" S_COLOR_DEFAULT "\", %i )\n", cvar->name, value, force);

	// Validate the value
	value = CVar_ValidateValue(value, cvar->type, cvar->min, cvar->max);

	// Check if the value didn't change at all
	if (!Str_Compare(cvar->value, value)){
		if (cvar->latchedValue){
			Mem_Free(cvar->latchedValue);
			cvar->latchedValue = NULL;
		}

		return;
	}

	if (cvar->flags & (CVAR_USERINFO | CVAR_SERVERINFO)){
		if (Str_FindChar(value, '\\') || Str_FindChar(value, '\"') || Str_FindChar(value, ';')){
			Com_Printf("Invalid info cvar value\n");
			return;
		}
	}

	// Set the new value
	if (force){
		if (cvar->latchedValue){
			Mem_Free(cvar->latchedValue);
			cvar->latchedValue = NULL;
		}
	}
	else {
		if (cvar->flags & CVAR_INIT){
			Com_Printf("\"%s\" is write protected\n", cvar->name);
			return;
		}

		if (cvar->flags & CVAR_READONLY){
			Com_Printf("\"%s\" is read only\n", cvar->name);
			return;
		}

		if (cvar->flags & CVAR_CHEAT && !cvar_allowCheats){
			Com_Printf("\"%s\" is cheat protected\n", cvar->name);
			return;
		}

		if (cvar->flags & CVAR_LATCH){
			if (cvar->latchedValue){
				if (!Str_ICompare(cvar->latchedValue, value))
					return;

				Mem_Free(cvar->latchedValue);
				cvar->latchedValue = NULL;
			}

			if (!Str_ICompare(cvar->value, value))
				return;

			cvar->latchedValue = Mem_DupString(value, TAG_COMMON);
			cvar->modified = true;

			// Set the modified flags
			cvar_modifiedFlags |= cvar->flags;

			Com_Printf("\"%s\" will be changed upon restarting\n", cvar->name);
			return;
		}
	}

	// Update the variable
	Mem_Free(cvar->value);

	cvar->value = Mem_DupString(value, TAG_COMMON);
	cvar->integerValue = Str_ToInteger(cvar->value);
	cvar->floatValue = Str_ToFloat(cvar->value);
	cvar->modified = true;

	// Set the modified flags
	cvar_modifiedFlags |= cvar->flags;

	// Stupid hack to workaround our changes to cvar_t
	if (cvar->flags & CVAR_GAME)
		CVar_GameUpdateOrCreate(cvar);
}

/*
 ==================
 CVar_SetBool
 ==================
*/
void CVar_SetBool (cvar_t *cvar, bool value){

	CVar_Set(cvar, Str_FromInteger(value), true);
}

/*
 ==================
 CVar_SetInteger
 ==================
*/
void CVar_SetInteger (cvar_t *cvar, int value){

	CVar_Set(cvar, Str_FromInteger(value), true);
}

/*
 ==================
 CVar_SetFloat
 ==================
*/
void CVar_SetFloat (cvar_t *cvar, float value){

	CVar_Set(cvar, Str_FromFloat(value, -1), true);
}

/*
 ==================
 CVar_SetString
 ==================
*/
void CVar_SetString (cvar_t *cvar, const char *value){

	CVar_Set(cvar, value, true);
}

/*
 ==================
 CVar_UpdateLatched
 ==================
*/
void CVar_UpdateLatched (cvar_t *cvar){

	if (!cvar->latchedValue)
		return;

	// Update the variable
	Mem_Free(cvar->value);

	cvar->value = cvar->latchedValue;
	cvar->latchedValue = NULL;
	cvar->integerValue = Str_ToInteger(cvar->value);
	cvar->floatValue = Str_ToFloat(cvar->value);
	cvar->modified = true;

	// Set the modified flags
	cvar_modifiedFlags |= cvar->flags;
}

/*
 ==================
 CVar_FindVariable
 ==================
*/
cvar_t *CVar_FindVariable (const char *name){

	cvar_t	*cvar;
	uint	hashKey;

	hashKey = Str_HashKey(name, CVARS_HASH_SIZE, false);

	for (cvar = cvar_variablesHashTable[hashKey]; cvar; cvar = cvar->nextHash){
		if (!Str_ICompare(cvar->name, name))
			return cvar;
	}

	return NULL;
}

/*
 ==================
 CVar_Command
 ==================
*/
bool CVar_Command (){

	cvar_t *cvar;

	// Check variables
	cvar = CVar_FindVariable(Cmd_Argv(0));
	if (!cvar)
		return false;
		
	// Perform a variable print or set
	if (Cmd_Argc() == 1){
		Com_Printf("\"%s\" is: \"%s" S_COLOR_DEFAULT "\" default: \"%s" S_COLOR_DEFAULT "\"", cvar->name, cvar->value, cvar->resetValue);

		if (cvar->latchedValue)
			Com_Printf(" latched: \"%s" S_COLOR_DEFAULT "\"\n", cvar->latchedValue);
		else
			Com_Printf("\n");

		if (cvar->description)
			Com_Printf("%s\n", cvar->description);

		return true;
	}

	CVar_Set(cvar, Cmd_Argv(1), false);

	return true;
}

/*
 ==================
 CVar_CommandCompletion
 ==================
*/
void CVar_CommandCompletion (void (*callback)(const char *string)){

	cvar_t	*cvar;

	for (cvar = cvar_variables; cvar; cvar = cvar->next)
		callback(cvar->name);
}

/*
 ==================
 CVar_ArgumentCompletion
 ==================
*/
void CVar_ArgumentCompletion (void (*callback)(const char *string)){

	cvar_t		*cvar;
	const char	*name;
	uint		hashKey;
	int			min, max;
	int			i;

	name = Cmd_Argv(0);

	hashKey = Str_HashKey(name, CVARS_HASH_SIZE, false);

	for (cvar = cvar_variablesHashTable[hashKey]; cvar; cvar = cvar->nextHash){
		if (!Str_ICompare(cvar->name, name)){
			if (cvar->type == CVAR_BOOL){
				callback("0");
				callback("1");
			}
			else if (cvar->type == CVAR_INTEGER){
				min = (int)cvar->min;
				max = (int)cvar->max;

				if (min < max && max - min < 10){
					for (i = min; i <= max; i++)
						callback(Str_FromInteger(i));
				}
			}

			return;
		}
	}
}

/*
 ==================
 CVar_GetVariableBool
 ==================
*/
bool CVar_GetVariableBool (const char *name){

	cvar_t	*cvar;

	cvar = CVar_FindVariable(name);
	if (!cvar)
		return false;

	return (cvar->integerValue != 0);
}

/*
 ==================
 CVar_GetVariableInteger
 ==================
*/
int CVar_GetVariableInteger (const char *name){

	cvar_t	*cvar;

	cvar = CVar_FindVariable(name);
	if (!cvar)
		return 0;

	return cvar->integerValue;
}

/*
 ==================
 CVar_GetVariableFloat
 ==================
*/
float CVar_GetVariableFloat (const char *name){

	cvar_t	*cvar;
	
	cvar = CVar_FindVariable(name);
	if (!cvar)
		return 0.0f;

	return cvar->floatValue;
}

/*
 ==================
 CVar_GetVariableString
 ==================
*/
const char *CVar_GetVariableString (const char *name){

	cvar_t	*cvar;
	
	cvar = CVar_FindVariable(name);
	if (!cvar)
		return "";

	return cvar->value;
}

/*
 ==================
 CVar_SetVariableBool
 ==================
*/
void CVar_SetVariableBool (const char *name, bool value, bool force){

	cvar_t	*cvar;

	cvar = CVar_FindVariable(name);
	if (!cvar){
		CVar_Register(name, Str_FromInteger(value), CVAR_BOOL, 0, NULL, 0, 0);
		return;
	}

	CVar_Set(cvar, Str_FromInteger(value), force);
}

/*
 ==================
 CVar_SetVariableInteger
 ==================
*/
void CVar_SetVariableInteger (const char *name, int value, bool force){

	cvar_t	*cvar;

	cvar = CVar_FindVariable(name);
	if (!cvar){
		CVar_Register(name, Str_FromInteger(value), CVAR_INTEGER, 0, NULL, 0, 0);
		return;
	}

	CVar_Set(cvar, Str_FromInteger(value), force);
}

/*
 ==================
 CVar_SetVariableFloat
 ==================
*/
void CVar_SetVariableFloat (const char *name, float value, bool force){

	cvar_t	*cvar;

	cvar = CVar_FindVariable(name);
	if (!cvar){
		CVar_Register(name, Str_FromFloat(value, -1), CVAR_FLOAT, 0, NULL, 0, 0);
		return;
	}

	CVar_Set(cvar, Str_FromFloat(value, -1), force);
}

/*
 ==================
 CVar_SetVariableString
 ==================
*/
void CVar_SetVariableString (const char *name, const char *value, bool force){

	cvar_t	*cvar;

	cvar = CVar_FindVariable(name);
	if (!cvar){
		CVar_Register(name, value, CVAR_STRING, 0, NULL, 0, 0);
		return;
	}

	CVar_Set(cvar, value, force);
}

/*
 ==================
 CVar_ForceSet
 ==================
*/
void CVar_ForceSet (const char *name, const char *value){

	cvar_t	*cvar;

	cvar = CVar_FindVariable(name);
	if (!cvar){
		CVar_Register(name, value, CVAR_STRING, 0, NULL, 0, 0);
		return;
	}

	CVar_Set(cvar, value, true);
}

/*
 ==================
 CVar_WriteVariables
 ==================
*/
void CVar_WriteVariables (fileHandle_t f){

	cvar_t	*cvar;

	for (cvar = cvar_variables; cvar; cvar = cvar->next){
		if (!(cvar->flags & CVAR_ARCHIVE))
			continue;

		if (!cvar->latchedValue)
			FS_Printf(f, "seta %s \"%s\"" NEWLINE, cvar->name, cvar->value);
		else
			FS_Printf(f, "seta %s \"%s\"" NEWLINE, cvar->name, cvar->latchedValue);
	}
}

/*
 ==================
 CVar_FixCheatVariables
 ==================
*/
void CVar_FixCheatVariables (bool allowCheats){

	cvar_t	*cvar;

	if (cvar_allowCheats == allowCheats)
		return;
	cvar_allowCheats = allowCheats;

	if (cvar_allowCheats)
		return;

	// Find cheat variables
	for (cvar = cvar_variables; cvar; cvar = cvar->next){
		if (!(cvar->flags & CVAR_CHEAT))
			continue;

		// Latched variables might escape the reset
		if (cvar->latchedValue){
			Mem_Free(cvar->latchedValue);
			cvar->latchedValue = NULL;
		}

		// Force it to its default value if necessary
		if (!Str_Compare(cvar->value, cvar->resetValue))
			continue;

		CVar_Set(cvar, cvar->resetValue, true);
	}
}

/*
 ==================
 CVar_AllowCheats
 ==================
*/
bool CVar_AllowCheats (){

	return cvar_allowCheats;
}

/*
 ==================
 CVar_GetModifiedFlags
 ==================
*/
int CVar_GetModifiedFlags (){

	return cvar_modifiedFlags;
}

/*
 ==================
 CVar_SetModifiedFlags
 ==================
*/
void CVar_SetModifiedFlags (int flags){

	cvar_modifiedFlags |= flags;
}

/*
 ==================
 CVar_ClearModifiedFlags
 ==================
*/
void CVar_ClearModifiedFlags (int flags){

	cvar_modifiedFlags &= ~flags;
}

/*
 ==================
 CVar_InfoString
 ==================
*/
char *CVar_InfoString (int flags){

	static char	infoString[MAX_INFO_STRING];
	char		value[MAX_INFO_VALUE];
	cvar_t		*cvar;

	infoString[0] = 0;

	for (cvar = cvar_variables; cvar; cvar = cvar->next){
		if (!(cvar->flags & flags))
			continue;

		if (Q_PrintStrlen(cvar->value) != Str_Length(cvar->value)){
			Str_SPrintf(value, sizeof(value), "%s%s", cvar->value, S_COLOR_WHITE);
			Info_SetValueForKey(infoString, cvar->name, value);
		}
		else
			Info_SetValueForKey(infoString, cvar->name, cvar->value);
	}

	return infoString;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 CVar_SortCVarList
 ==================
*/
static int CVar_SortCVarList (const void *elem1, const void *elem2){

	const cvar_t	*cvar1 = *(const cvar_t **)elem1;
	const cvar_t	*cvar2 = *(const cvar_t **)elem2;

	return Str_Compare(cvar1->name, cvar2->name);
}

/*
 =================
 Cvar_Get_f
 =================
*/
void Cvar_Get_f (void){

	cvar_t *var;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: get <variable>\n");
		return;
	}

	var = CVar_FindVariable(Cmd_Argv(1));
	if (!var){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	Com_Printf("\"%s\" is: \"%s" S_COLOR_WHITE "\" default: \"%s" S_COLOR_WHITE "\"", var->name, var->value, var->resetValue);
	if (var->latchedValue)
		Com_Printf(" latched: \"%s" S_COLOR_WHITE "\"", var->latchedValue);
	Com_Printf("\n");
}

/*
 ==================
 CVar_Set_f
 ==================
*/
static void CVar_Set_f (){

	cvar_t	*cvar;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: set <cvar> <value>\n");
		return;
	}

	cvar = CVar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		CVar_Register(Cmd_Argv(1), Cmd_Argv(2), CVAR_STRING, 0, NULL, 0, 0);
		return;
	}

	CVar_Set(cvar, Cmd_Argv(2), false);
}

/*
 ==================
 CVar_SetA_f
 ==================
*/
static void CVar_SetA_f (){

	cvar_t	*cvar;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: seta <cvar> <value>\n");
		return;
	}

	cvar = CVar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		CVar_Register(Cmd_Argv(1), Cmd_Argv(2), CVAR_STRING, CVAR_ARCHIVE, NULL, 0, 0);
		return;
	}

	CVar_Set(cvar, Cmd_Argv(2), false);

	// Make it CVAR_ARCHIVE if needed
	if (!(cvar->flags & CVAR_ARCHIVE)){
		cvar->flags |= CVAR_ARCHIVE;

		cvar->modified = true;

		// Set the modified flags
		cvar_modifiedFlags |= cvar->flags;
	}
}

/*
 ==================
 CVar_SetU_f
 ==================
*/
static void CVar_SetU_f (){

	cvar_t	*cvar;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: setu <cvar> <value>\n");
		return;
	}

	cvar = CVar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		CVar_Register(Cmd_Argv(1), Cmd_Argv(2), CVAR_STRING, CVAR_USERINFO, NULL, 0, 0);
		return;
	}

	CVar_Set(cvar, Cmd_Argv(2), false);

	// Make it CVAR_USERINFO if needed
	if (!(cvar->flags & CVAR_USERINFO)){
		cvar->flags |= CVAR_USERINFO;

		cvar->modified = true;

		// Set the modified flags
		cvar_modifiedFlags |= cvar->flags;
	}
}

/*
 ==================
 CVar_SetS_f
 ==================
*/
static void CVar_SetS_f (){

	cvar_t	*cvar;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: sets <cvar> <value>\n");
		return;
	}

	cvar = CVar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		CVar_Register(Cmd_Argv(1), Cmd_Argv(2), CVAR_STRING, CVAR_SERVERINFO, NULL, 0, 0);
		return;
	}

	CVar_Set(cvar, Cmd_Argv(2), false);

	// Make it CVAR_SERVERINFO if needed
	if (!(cvar->flags & CVAR_SERVERINFO)){
		cvar->flags |= CVAR_SERVERINFO;

		cvar->modified = true;

		// Set the modified flags
		cvar_modifiedFlags |= cvar->flags;
	}
}

/*
 ==================
 
 ==================
*/
static void CVar_Add_f (){

	cvar_t	*cvar;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: add <variable> <value>\n");
		return;
	}

	cvar = CVar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	CVar_Set(cvar, Str_VarArgs("%f", cvar->floatValue + Str_ToFloat(Cmd_Argv(2))), false);
}

/*
 ==================
 
 ==================
*/
static void CVar_Sub_f (){

	cvar_t	*cvar;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: sub <variable> <value>\n");
		return;
	}

	cvar = CVar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	CVar_Set(cvar, Str_VarArgs("%f", cvar->floatValue - Str_ToFloat(Cmd_Argv(2))), false);
}

/*
 ==================
 CVar_Inc_f
 ==================
*/
static void CVar_Inc_f (){

	cvar_t	*cvar;

	if (Cmd_Argc() < 2 || Cmd_Argc() > 3){
		Com_Printf("Usage: inc <cvar> [value]\n");
		return;
	}

	cvar = CVar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("\"%s\" is not a cvar\n", Cmd_Argv(1));
		return;
	}

	switch (cvar->type){
	case CVAR_INTEGER:
		if (Cmd_Argc() == 2)
			CVar_Set(cvar, Str_FromInteger(cvar->integerValue + 1), false);
		else
			CVar_Set(cvar, Str_FromInteger(cvar->integerValue + Str_ToInteger(Cmd_Argv(2))), false);

		break;
	case CVAR_FLOAT:
		if (Cmd_Argc() == 2)
			CVar_Set(cvar, Str_FromFloat(cvar->floatValue + 1.0f, -1), false);
		else
			CVar_Set(cvar, Str_FromFloat(cvar->floatValue + Str_ToFloat(Cmd_Argv(2)), -1), false);

		break;
	default:
		Com_Printf("\"%s\" is not a cvar of integer or float type\n", Cmd_Argv(1));

		break;
	}
}

/*
 ==================
 CVar_Dec_f
 ==================
*/
static void CVar_Dec_f (){

	cvar_t	*cvar;

	if (Cmd_Argc() < 2 || Cmd_Argc() > 3){
		Com_Printf("Usage: dec <cvar> [value]\n");
		return;
	}

	cvar = CVar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("\"%s\" is not a cvar\n", Cmd_Argv(1));
		return;
	}

	switch (cvar->type){
	case CVAR_INTEGER:
		if (Cmd_Argc() == 2)
			CVar_Set(cvar, Str_FromInteger(cvar->integerValue - 1), false);
		else
			CVar_Set(cvar, Str_FromInteger(cvar->integerValue - Str_ToInteger(Cmd_Argv(2))), false);

		break;
	case CVAR_FLOAT:
		if (Cmd_Argc() == 2)
			CVar_Set(cvar, Str_FromFloat(cvar->floatValue - 1.0f, -1), false);
		else
			CVar_Set(cvar, Str_FromFloat(cvar->floatValue - Str_ToFloat(Cmd_Argv(2)), -1), false);

		break;
	default:
		Com_Printf("\"%s\" is not a cvar of integer or float type\n", Cmd_Argv(1));

		break;
	}
}

/*
 ==================
 CVar_Toggle_f
 ==================
*/	
static void CVar_Toggle_f (){

	cvar_t	*cvar;
	int		min, max;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: toggle <cvar>\n");
		return;
	}

	cvar = CVar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("\"%s\" is not a cvar\n", Cmd_Argv(1));
		return;
	}

	switch (cvar->type){
	case CVAR_BOOL:
		CVar_Set(cvar, Str_FromInteger(!cvar->integerValue), false);

		break;
	case CVAR_INTEGER:
		min = (int)cvar->min;
		max = (int)cvar->max;

		if (min < max){
			if (cvar->integerValue >= max)
				CVar_Set(cvar, Str_FromInteger(min), false);
			else
				CVar_Set(cvar, Str_FromInteger(cvar->integerValue + 1), false);
		}

		break;
	default:
		Com_Printf("\"%s\" is not a cvar of bool or integer type\n", Cmd_Argv(1));

		break;
	}
}

/*
 ==================
 CVar_Reset_f
 ==================
*/
static void CVar_Reset_f (){

	cvar_t *cvar;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: reset <variable>\n");
		return;
	}

	cvar = CVar_FindVariable(Cmd_Argv(1));
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", Cmd_Argv(1));
		return;
	}

	CVar_Set(cvar, cvar->resetValue, false);
}

/*
 ==================
 CVar_RestartCVars_f
 ==================
*/
static void CVar_RestartCVars_f (){

	cvar_t	*cvar;

	// Set all variables to their default values
	for (cvar = cvar_variables; cvar; cvar = cvar->next){
		if (cvar->flags & (CVAR_INIT | CVAR_READONLY))
			continue;

		CVar_Set(cvar, cvar->resetValue, false);
	}
}

/*
 ==================
 CVar_ListCVars_f
 ==================
*/
static void CVar_ListCVars_f (void){

	cvar_t		*cvar, *list[MAX_LIST_CVARS];
	bool		type = false, flags = false, help = false;
	const char	*filter = NULL;
	int			found = 0, total = 0;
	int			i;

	if (Cmd_Argc() > 3){
		Com_Printf("Usage: listCVars [\"-type\" | \"-flags\" | \"-help\"] [filter]\n");
		return;
	}

	if (Cmd_Argc() == 2){
		if (!Str_ICompare("-type", Cmd_Argv(1)))
			type = true;
		else if (!Str_ICompare("-flags", Cmd_Argv(1)))
			flags = true;
		else if (!Str_ICompare("-help", Cmd_Argv(1)))
			help = true;
		else
			filter = Cmd_Argv(1);
	}
	else if (Cmd_Argc() == 3){
		if (!Str_ICompare("-type", Cmd_Argv(1)))
			type = true;
		else if (!Str_ICompare("-flags", Cmd_Argv(1)))
			flags = true;
		else if (!Str_ICompare("-help", Cmd_Argv(1)))
			help = true;
		else {
			Com_Printf("Usage: listCVars [\"-type\" | \"-flags\" | \"-help\"] [filter]\n");
			return;
		}

		filter = Cmd_Argv(2);
	}

	// Find matching variables
	for (cvar = cvar_variables; cvar; cvar = cvar->next){
		total++;

		if (found == MAX_LIST_CVARS)
			continue;

		if (filter){
			if (!Str_MatchFilter(cvar->name, filter, false))
				continue;
		}

		list[found++] = cvar;
	}

	// Sort the list
	qsort(list, found, sizeof(cvar_t *), CVar_SortCVarList);

	// Print the list
	for (i = 0; i < found; i++){
		cvar = list[i];

		Com_Printf("%-32s", cvar->name);

		if (type){
			switch (cvar->type){
			case CVAR_BOOL:
				Com_Printf(" BOOL");

				break;
			case CVAR_INTEGER:
				Com_Printf(" INTEGER");

				if (cvar->min < cvar->max)
					Com_Printf(" [%s - %s]", Str_FromFloat(cvar->min, 0), Str_FromFloat(cvar->max, 0));

				break;
			case CVAR_FLOAT:
				Com_Printf(" FLOAT");

				if (cvar->min < cvar->max)
					Com_Printf(" [%s - %s]", Str_FromFloat(cvar->min, -1), Str_FromFloat(cvar->max, -1));

				break;
			case CVAR_STRING:
				Com_Printf(" STRING");

				break;
			}

			Com_Printf("\n");

			continue;
		}

		if (flags){
			if (cvar->flags & CVAR_ARCHIVE)
				Com_Printf(" AR");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_USERINFO)
				Com_Printf(" UI");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_SERVERINFO)
				Com_Printf(" SI");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_INIT)
				Com_Printf(" IN");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_READONLY)
				Com_Printf(" RO");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_CHEAT)
				Com_Printf(" CH");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_LATCH)
				Com_Printf(" LA");
			else
				Com_Printf("   ");

			if (cvar->flags & CVAR_GAME)
				Com_Printf(" GA");
			else
				Com_Printf("   ");

			Com_Printf("\n");

			continue;
		}

		if (help){
			if (cvar->description)
				Com_Printf(" %s\n", cvar->description);
			else
				Com_Printf("\n");

			continue;
		}

		Com_Printf(" \"%s" S_COLOR_DEFAULT "\"\n", cvar->value);
	}

	if (found == MAX_LIST_CVARS)
		Com_Printf("...\n");

	Com_Printf("--------------------\n");
	Com_Printf("%i cvars listed (%i total cvars)\n", found, total);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 CVar_Init
 ==================
*/
void CVar_Init (){

	// Add commands
	Cmd_AddCommand("get", Cvar_Get_f, NULL, NULL);
	Cmd_AddCommand("set", CVar_Set_f, NULL, NULL);
	Cmd_AddCommand("seta", CVar_SetA_f, NULL, NULL);
	Cmd_AddCommand("setu", CVar_SetU_f, NULL, NULL);
	Cmd_AddCommand("sets", CVar_SetS_f, NULL, NULL);
	Cmd_AddCommand("add", CVar_Add_f, NULL, NULL);
	Cmd_AddCommand("sub", CVar_Sub_f, NULL, NULL);
	Cmd_AddCommand("inc", CVar_Inc_f, NULL, NULL);
	Cmd_AddCommand("dec", CVar_Dec_f, NULL, NULL);
	Cmd_AddCommand("toggle", CVar_Toggle_f, NULL, NULL);
	Cmd_AddCommand("reset", CVar_Reset_f, NULL, NULL);
	Cmd_AddCommand("restartCVars", CVar_RestartCVars_f, "Sets all cvars to their default values", NULL);
	Cmd_AddCommand("listCVars", CVar_ListCVars_f, "Lists cvars", NULL);

	// Allow cheats
	cvar_allowCheats = true;

	// Clear modified flags
	cvar_modifiedFlags = 0;
}

/*
 ==================
 CVar_Shutdown
 ==================
*/
void CVar_Shutdown (){

	// Remove commands
	Cmd_RemoveCommand("get");
	Cmd_RemoveCommand("set");
	Cmd_RemoveCommand("seta");
	Cmd_RemoveCommand("setu");
	Cmd_RemoveCommand("sets");
	Cmd_RemoveCommand("add");
	Cmd_RemoveCommand("sub");
	Cmd_RemoveCommand("inc");
	Cmd_RemoveCommand("dec");
	Cmd_RemoveCommand("toggle");
	Cmd_RemoveCommand("reset");
	Cmd_RemoveCommand("restartCVars");
	Cmd_RemoveCommand("listCVars");

	// Clear variable list
	Mem_Fill(cvar_variablesHashTable, 0, sizeof(cvar_variablesHashTable));

	cvar_variables = NULL;
}


/*
 ==============================================================================

 SERVER-GAME HACK

 ==============================================================================
*/

static gamecvar_t *		game_variables;


/*
 ==================
 CVar_GameFindVariable
 ==================
*/
gamecvar_t *CVar_GameFindVariable (char *name){

	gamecvar_t	*cvar;

	for (cvar = game_variables; cvar; cvar = cvar->next){
		if (!Str_ICompare(cvar->name, name))
			return cvar;
	}

	return NULL;
}

/*
 ==================
 CVar_GameUpdateOrCreate
 ==================
*/
gamecvar_t *CVar_GameUpdateOrCreate (cvar_t *cvar){

	gamecvar_t	*gameVar;

	gameVar = CVar_GameFindVariable(cvar->name);
	if (gameVar){
		if (gameVar->value)
			Mem_Free(gameVar->string);
		if (gameVar->latched_string)
			Mem_Free(gameVar->latched_string);

		gameVar->name = Mem_DupString(cvar->name, TAG_COMMON);
		gameVar->string = Mem_DupString(cvar->value, TAG_COMMON);
		gameVar->latched_string = (cvar->latchedValue) ? Mem_DupString(cvar->latchedValue, TAG_COMMON) : NULL;
		gameVar->flags = cvar->flags;
		gameVar->modified = cvar->modified;
		gameVar->value = cvar->floatValue;

		return gameVar;
	}

	gameVar = (gamecvar_t *)Mem_ClearedAlloc(sizeof(gamecvar_t), TAG_COMMON);

	gameVar->name = Mem_DupString(cvar->name, TAG_COMMON);
	gameVar->string = Mem_DupString(cvar->value, TAG_COMMON);
	gameVar->latched_string = (cvar->latchedValue) ? Mem_DupString(cvar->latchedValue, TAG_COMMON) : NULL;
	gameVar->flags = cvar->flags;
	gameVar->modified = cvar->modified;
	gameVar->value = cvar->floatValue;

	gameVar->next = game_variables;
	game_variables = gameVar;

	return gameVar;
}

/*
 ==================
 CVar_GameRegister
 ==================
*/
gamecvar_t *CVar_GameRegister (char *name, char *value, int flags){

	cvar_t	*cvar;

	// HACK: the game library may want to know the game dir
	if (!Str_ICompare(name, "game"))
		name = "fs_game";

	cvar = CVar_Register(name, value, CVAR_STRING, flags | CVAR_GAME, NULL, 0, 0);
	if (!cvar)
		return NULL;

	return CVar_GameUpdateOrCreate(cvar);
}

/*
 ==================
 CVar_GameSet
 ==================
*/
gamecvar_t *CVar_GameSet (char *name, char *value){

	cvar_t	*cvar;

	cvar = CVar_FindVariable(name);
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", name);
		return NULL;
	}

	cvar->flags |= CVAR_GAME;

	CVar_Set(cvar, value, true);

	return CVar_GameUpdateOrCreate(cvar);
}

/*
 ==================
 CVar_GameForceSet
 ==================
*/
gamecvar_t *CVar_GameForceSet (char *name, char *value){

	cvar_t	*cvar;

	cvar = CVar_FindVariable(name);
	if (!cvar){
		Com_Printf("'%s' is not a variable\n", name);
		return NULL;
	}

	cvar->flags |= CVAR_GAME;

	CVar_Set(cvar, value, true);

	return CVar_GameUpdateOrCreate(cvar);
}