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
// table.c - Lookup tables
//


#include "common.h"


#define MAX_TABLES					1024

#define MAX_TABLE_NAME				64
#define MAX_TABLE_SIZE				256

#define TABLES_HASH_SIZE			(MAX_TABLES >> 2)

typedef struct table_s {
	char					name[MAX_TABLE_NAME];
	int						index;

	bool					snap;
	bool					clamp;

	int						size;
	float *					values;

	struct table_s *		nextHash;
} table_t;

static table_t *			lut_tablesHashTable[TABLES_HASH_SIZE];
static table_t *			lut_tables[MAX_TABLES];
static int					lut_numTables;


/*
 ==============================================================================

 TABLE PARSING & LOADING

 ==============================================================================
*/


/*
 ==================
 LUT_LoadTable
 ==================
*/
static void LUT_LoadTable (const char *name, bool snap, bool clamp, int size, float *values){

	table_t	*table;
	uint	hashKey;

	// See if already loaded
	hashKey = Str_HashKey(name, TABLES_HASH_SIZE, false);

	for (table = lut_tablesHashTable[hashKey]; table; table = table->nextHash){
		if (!Str_ICompare(table->name, name))
			break;
	}

	// If the table already exists, replace it
	if (table){
		Mem_Free(table->values);

		table->snap = snap;
		table->clamp = clamp;
		table->size = size;
		table->values = (float *)Mem_DupData(values, size * sizeof(float), TAG_COMMON);

		return;
	}

	// Add a new table
	if (lut_numTables == MAX_TABLES)
		Com_Error(ERR_DROP, "LUT_LoadTable: MAX_TABLES hit");

	lut_tables[lut_numTables++] = table = (table_t *)Mem_ClearedAlloc(sizeof(table_t), TAG_COMMON);

	// Fill it in
	Str_Copy(table->name, name, sizeof(table->name));
	table->index = lut_numTables - 1;
	table->snap = snap;
	table->clamp = clamp;
	table->size = size;
	table->values = (float *)Mem_DupData(values, size * sizeof(float), TAG_COMMON);

	// Add to hash table
	table->nextHash = lut_tablesHashTable[hashKey];
	lut_tablesHashTable[hashKey] = table;
}

/*
 ==================
 LUT_ParseTable
 ==================
*/
static bool LUT_ParseTable (script_t *script, const char *name){

	token_t	token;
	float	values[MAX_TABLE_SIZE];
	bool	snap = false, clamp = false;
	int		size = 0;

	// Parse the table
	if (!PS_ExpectTokenString(script, &token, "{", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead for table '%s'\n", token.string, name);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' for table '%s'\n", name);
			return false;	// End of script
		}

		if (!Str_Compare(token.string, "}"))
			break;			// End of table

		// Parse the values
		if (!Str_Compare(token.string, "{")){
			// Check if already defined
			if (size){
				Com_Printf(S_COLOR_YELLOW "WARNING: values already defined for table '%s'\n", name);
				return false;
			}

			// Parse them
			while (1){
				if (size){
					if (!PS_ReadToken(script, &token)){
						Com_Printf(S_COLOR_YELLOW "WARNING: no matching '}' for table '%s'\n", name);
						return false;	// End of script
					}

					if (!Str_Compare(token.string, "}"))
						break;			// End of values

					// It must be a comma
					if (Str_Compare(token.string, ",")){
						Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead for table '%s'\n", token.string, name);
						return false;
					}
				}

				// Parse the value
				if (size == MAX_TABLE_SIZE){
					Com_Printf(S_COLOR_YELLOW "WARNING: too many values defined for table '%s'\n", name);
					return false;
				}

				if (!PS_ReadFloat(script, &values[size])){
					Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for table '%s'\n", name);
					return false;
				}

				size++;
			}

			continue;
		}

		// Parse the flags
		if (!Str_ICompare(token.string, "snap"))
			snap = true;
		else if (!Str_ICompare(token.string, "clamp"))
			clamp = true;
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' for table '%s'\n", token.string, name);
			return false;
		}
	}

	// Check if no values
	if (!size){
		Com_Printf(S_COLOR_YELLOW "WARNING: no values defined for table '%s'\n", name);
		return false;
	}

	// Load the table
	LUT_LoadTable(name, snap, clamp, size, values);

	return true;
}

/*
 ==================
 LUT_LoadAndParseTables
 ==================
*/
static void LUT_LoadAndParseTables (){

	script_t	*script;
	token_t		token;
	char		name[MAX_PATH_LENGTH];
	const char	**fileList;
	int			numFiles;
	int			i;

	Com_Printf("-------- Table Loading --------\n");

	// Load and parse .tbl files
	fileList = FS_ListFiles("tables", ".tbl", true, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Load the script file
		Str_SPrintf(name, sizeof(name), "tables/%s", fileList[i]);
		Com_Printf("...loading '%s'\n", name);

		script = PS_LoadScriptFile(name);
		if (!script){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't load '%s'\n", name);
			continue;
		}

		PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS);

		// Parse it
		while (1){
			// Parse the name
			if (!PS_ReadToken(script, &token))
				break;		// End of script

			// Parse the table
			if (!LUT_ParseTable(script, token.string))
				break;
		}

		// Free the script file
		PS_FreeScript(script);
	}

	FS_FreeFileList(fileList);

	Com_Printf("-------------------------------\n");
}


// ============================================================================


/*
 ==================
 LUT_FindTable
 ==================
*/
int LUT_FindTable (const char *name){

	table_t	*table;
	uint	hashKey;

	// See if already loaded
	hashKey = Str_HashKey(name, TABLES_HASH_SIZE, false);

	for (table = lut_tablesHashTable[hashKey]; table; table = table->nextHash){
		if (!Str_ICompare(table->name, name))
			return table->index;
	}

	// Not found
	return -1;
}

/*
 ==================
 LUT_LookupTable
 ==================
*/
float LUT_LookupTable (int tableIndex, float lookupIndex){

	table_t	*table;
	int		index, oldIndex, newIndex;

	if (tableIndex < 0 || tableIndex >= lut_numTables)
		Com_Error(ERR_DROP, "LUT_LookupTable: tableIndex out of range");

	table = lut_tables[tableIndex];

	// Lookup a single value if desired
	if (table->snap){
		lookupIndex *= table->size;

		if (table->clamp){
			if (lookupIndex < 0.0f)
				lookupIndex = 0.0f;

			index = FloatToInt(lookupIndex);
			index = min(index, table->size - 1);
		}
		else {
			if (lookupIndex < 0.0f)
				lookupIndex -= table->size * Floor(lookupIndex / table->size);

			index = FloatToInt(lookupIndex);
			index = index % table->size;
		}

		return table->values[index];
	}

	// Interpolate two values
	lookupIndex *= table->size;

	if (table->clamp){
		if (lookupIndex < 0.0f)
			lookupIndex = 0.0f;

		index = FloatToInt(lookupIndex);

		oldIndex = min(index, table->size - 1);
		newIndex = min(index + 1, table->size - 1);
	}
	else {
		if (lookupIndex < 0.0f)
			lookupIndex -= table->size * Floor(lookupIndex / table->size);

		index = FloatToInt(lookupIndex);

		oldIndex = index % table->size;
		newIndex = (index + 1) % table->size;
	}

	return LerpFast(table->values[oldIndex], table->values[newIndex], Frac(lookupIndex));
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 LUT_ListTables_f
 ==================
*/
static void LUT_ListTables_f (){

	table_t	*table;
	int		i;

	Com_Printf("\n");
	Com_Printf("      flags size -name-----------\n");

	for (i = 0; i < lut_numTables; i++){
		table = lut_tables[i];

		Com_Printf("%4i: ", i);

		if (table->snap)
			Com_Printf("sn ");
		else
			Com_Printf("   ");

		if (table->clamp)
			Com_Printf("cl ");
		else
			Com_Printf("   ");

		Com_Printf("%4i ", table->size);

		Com_Printf("%s\n", table->name);
	}

	Com_Printf("---------------------------------\n");
	Com_Printf("%i total tables\n", lut_numTables);
	Com_Printf("\n");
}

/*
 ==================
 LUT_ReloadTables_f
 ==================
*/
static void LUT_ReloadTables_f (){

	LUT_LoadAndParseTables();
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 LUT_Init
 ==================
*/
void LUT_Init (){

	// Add commands
	Cmd_AddCommand("listTables", LUT_ListTables_f, "Lists loaded tables", NULL);
	Cmd_AddCommand("reloadTables", LUT_ReloadTables_f, "Reloads all tables", NULL);

	// Load and parse all the tables
	LUT_LoadAndParseTables();
}

/*
 ==================
 LUT_Shutdown
 ==================
*/
void LUT_Shutdown (){

	// Remove commands
	Cmd_RemoveCommand("listTables");
	Cmd_RemoveCommand("reloadTables");

	// Clear table list
	Mem_Fill(lut_tablesHashTable, 0, sizeof(lut_tablesHashTable));
	Mem_Fill(lut_tables, 0, sizeof(lut_tables));

	lut_numTables = 0;
}