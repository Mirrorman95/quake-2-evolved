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
// cmdSystem.c - Script command processing
//


#include "common.h"


#define ALIAS_HASH_SIZE				32

#define	MAX_ALIAS_COUNT				16

typedef struct cmdAlias_s {
	char *				name;
	char *				command;
	struct cmdAlias_s *	next;
	struct cmdAlias_s *	nextHash;
} cmdAlias_t;

static cmdAlias_t *		cmd_aliasHash[ALIAS_HASH_SIZE];
static cmdAlias_t *		cmd_alias;

#define CMDS_HASH_SIZE				512

#define MAX_LIST_CMDS				2048

typedef struct cmd_s {
	const char *		name;
	cmdFunction_t		function;
	const char *		description;
	argCompletion_t		argCompletion;

	struct cmd_s *		next;
	struct cmd_s *		nextHash;
} cmd_t;

static cmd_t *			cmd_functionsHashTable[CMDS_HASH_SIZE];
static cmd_t *			cmd_functions;

static int				cmd_aliasCount;		// For detecting runaway loops

static int				cmd_wait;


/*
 =======================================================================

 COMMAND REGISTRATION

 =======================================================================
*/


/*
 ==================
 Cmd_AddCommand
 ==================
*/
void Cmd_AddCommand (const char *name, cmdFunction_t function, const char *description, argCompletion_t argCompletion){

	cmd_t	*cmd;
	uint	hashKey;

	// Fail if the command is a variable name
	if (CVar_FindVariable(name)){
		Com_DPrintf("Cmd_AddCommand: '%s' already defined as a cvar\n", name);
		return;
	}
	
	// Fail if the command already exists
	hashKey = Str_HashKey(name, CMDS_HASH_SIZE, false);

	for (cmd = cmd_functionsHashTable[hashKey]; cmd; cmd = cmd->nextHash){
		if (!Str_ICompare(cmd->name, name)){
			if (function != NULL)
				Com_Printf(S_COLOR_YELLOW "Cmd_AddCommand: \"%s\" already defined\n", name);

			return;
		}
	}

	// Allocate a new command
	cmd = (cmd_t *)Mem_ClearedAlloc(sizeof(cmd_t), TAG_COMMON);

	cmd->name = Mem_DupString(name, TAG_COMMON);
	cmd->function = function;
	cmd->description = (description) ? Mem_DupString(description, TAG_COMMON) : NULL;
	cmd->argCompletion = argCompletion;

	// Link the command in
	cmd->next = cmd_functions;
	cmd_functions = cmd;

	// Add to hash table
	cmd->nextHash = cmd_functionsHashTable[hashKey];
	cmd_functionsHashTable[hashKey] = cmd;
}

/*
 ==================
 Cmd_RemoveCommand
 ==================
*/
void Cmd_RemoveCommand (const char *name){

	cmd_t	*cmd, **prevCmd;
	cmd_t	*hash, **prevHash;
	uint	hashKey;

	// Find the command
	prevCmd = &cmd_functions;

	while (1){
		cmd = *prevCmd;
		if (!cmd)
			break;		// Not found

		if (!Str_ICompare(cmd->name, name)){
			// Remove from hash table
			hashKey = Str_HashKey(cmd->name, CMDS_HASH_SIZE, false);

			prevHash = &cmd_functionsHashTable[hashKey];

			while (1){
				hash = *prevHash;
				if (!hash)
					break;		// Not found

				if (!Str_ICompare(hash->name, cmd->name)){
					*prevHash = hash->nextHash;
					break;
				}

				prevHash = &hash->nextHash;
			}

			// Remove the command
			*prevCmd = cmd->next;

			if (cmd->name)
				Mem_Free(cmd->name);
			if (cmd->description)
				Mem_Free(cmd->description);

			Mem_Free(cmd);

			return;
		}

		prevCmd = &cmd->next;
	}
}

/*
 ==================
 Cmd_Exists
 ==================
*/
bool Cmd_Exists (const char *name){

	cmd_t	*cmd;
	uint	hashKey;

	hashKey = Str_HashKey(name, CMDS_HASH_SIZE, false);

	for (cmd = cmd_functionsHashTable[hashKey]; cmd; cmd = cmd->nextHash){
		if (!Str_ICompare(cmd->name, name))
			return true;
	}

	return false;
}

/*
 ==================
 Cmd_CommandCompletion

 TODO: aliases ?
 ==================
*/
void Cmd_CommandCompletion (void (*callback)(const char *string)){

	cmd_t	*cmd;

	for (cmd = cmd_functions; cmd; cmd = cmd->next)
		callback(cmd->name);
}

/*
 ==================
 Cmd_ArgumentCompletion
 ==================
*/
void Cmd_ArgumentCompletion (void (*callback)(const char *string)){

	cmd_t		*cmd;
	const char	*name;
	uint		hashKey;

	name = Cmd_Argv(0);

	hashKey = Str_HashKey(name, CMDS_HASH_SIZE, false);

	for (cmd = cmd_functionsHashTable[hashKey]; cmd; cmd = cmd->nextHash){
		if (!cmd->argCompletion)
			continue;

		if (!Str_ICompare(cmd->name, name)){
			cmd->argCompletion(callback);
			return;
		}
	}
}

/*
 ==================
 Cmd_ArgCompletion_PathExtension
 ==================
*/
void Cmd_ArgCompletion_PathExtension (void (*callback)(const char *string), const char *path, const char *extension, bool stripPath){

	char		string[MAX_PATH_LENGTH];
	char		argPath[MAX_PATH_LENGTH];
	bool		cmdOnly;
	const char	**fileList;
	int			numFiles;
	int			i, skip;

	if (Cmd_Argc() == 1)
		cmdOnly = true;
	else {
		cmdOnly = false;

		if (stripPath)
			Str_SPrintf(string, sizeof(string), "%s/%s", path, Cmd_Argv(1));
		else
			Str_SPrintf(string, sizeof(string), "/%s", Cmd_Argv(1));

		Str_StripFileName(string);

		if (string[0] != '/')
			Str_Copy(argPath, string, sizeof(argPath));
		else
			Str_Copy(argPath, string + 1, sizeof(argPath));
	}

	if (!stripPath || !path[0])
		skip = 0;
	else
		skip = Str_Length(path) + 1;

	// List directories
	if (cmdOnly){
		fileList = FS_ListFiles(path, "/", false, &numFiles);

		if (path[0]){
			for (i = 0; i < numFiles; i++){
				Str_SPrintf(string, sizeof(string), "%s/%s/", path, fileList[i]);

				callback(string + skip);
			}
		}
		else {
			for (i = 0; i < numFiles; i++){
				Str_SPrintf(string, sizeof(string), "%s/", fileList[i]);

				callback(string + skip);
			}
		}

		FS_FreeFileList(fileList);
	}
	else {
		fileList = FS_ListFiles(argPath, "/", false, &numFiles);

		if (argPath[0]){
			for (i = 0; i < numFiles; i++){
				Str_SPrintf(string, sizeof(string), "%s/%s/", argPath, fileList[i]);
				
				callback(string + skip);
			}
		}
		else {
			for (i = 0; i < numFiles; i++){
				Str_SPrintf(string, sizeof(string), "%s/", fileList[i]);

				callback(string + skip);
			}
		}

		FS_FreeFileList(fileList);
	}

	// List files
	if (cmdOnly){
		fileList = FS_ListFiles(path, extension, false, &numFiles);

		if (path[0]){
			for (i = 0; i < numFiles; i++){
				Str_SPrintf(string, sizeof(string), "%s/%s", path, fileList[i]);

				callback(string + skip);
			}
		}
		else {
			for (i = 0; i < numFiles; i++){
				Str_SPrintf(string, sizeof(string), "%s", fileList[i]);

				callback(string + skip);
			}
		}

		FS_FreeFileList(fileList);
	}
	else {
		fileList = FS_ListFiles(argPath, extension, false, &numFiles);

		if (argPath[0]){
			for (i = 0; i < numFiles; i++){
				Str_SPrintf(string, sizeof(string), "%s/%s", argPath, fileList[i]);

				callback(string + skip);
			}
		}
		else {
			for (i = 0; i < numFiles; i++){
				Str_SPrintf(string, sizeof(string), "%s", fileList[i]);

				callback(string + skip);
			}
		}

		FS_FreeFileList(fileList);
	}
}

/*
 ==================
 Cmd_ArgCompletion_FileName
 ==================
*/
void Cmd_ArgCompletion_FileName (void (*callback)(const char *string)){

	Cmd_ArgCompletion_PathExtension(callback, "", NULL, false);
}

/*
 ==================
 Cmd_ArgCompletion_ConfigName
 ==================
*/
void Cmd_ArgCompletion_ConfigName (void (*callback)(const char *string)){

	Cmd_ArgCompletion_PathExtension(callback, "", ".cfg", false);
}

/*
 ==================
 Cmd_ArgCompletion_DemoName
 ==================
*/
void Cmd_ArgCompletion_DemoName (void (*callback)(const char *string)){

	Cmd_ArgCompletion_PathExtension(callback, "demos", ".dm2", true);
}

/*
 ==================
 Cmd_ArgCompletion_MapName
 ==================
*/
void Cmd_ArgCompletion_MapName (void (*callback)(const char *string)){

	Cmd_ArgCompletion_PathExtension(callback, "maps", ".bsp", true);
}

/*
 ==================
 Cmd_ArgCompletion_ModelName
 ==================
*/
void Cmd_ArgCompletion_ModelName (void (*callback)(const char *string)){

	Cmd_ArgCompletion_PathExtension(callback, "models", ".md2", false);
	Cmd_ArgCompletion_PathExtension(callback, "models", ".md3", false);
}

/*
 ==================
 Cmd_ArgCompletion_TextureName
 ==================
*/
void Cmd_ArgCompletion_TextureName (void (*callback)(const char *string)){

	Cmd_ArgCompletion_PathExtension(callback, "env", ".tga", false);

	Cmd_ArgCompletion_PathExtension(callback, "gfx", ".tga", false);

	Cmd_ArgCompletion_PathExtension(callback, "models", ".tga", false);

	Cmd_ArgCompletion_PathExtension(callback, "textures", ".tga", false);

	Cmd_ArgCompletion_PathExtension(callback, "videos", ".tga", false);
}

/*
 ==================
 Cmd_ArgCompletion_SoundName
 ==================
*/
void Cmd_ArgCompletion_SoundName (void (*callback)(const char *string)){

	Cmd_ArgCompletion_PathExtension(callback, "sounds", ".wav", false);
}

/*
 ==================
 Cmd_ArgCompletion_MusicName
 ==================
*/
void Cmd_ArgCompletion_MusicName (void (*callback)(const char *string)){

	Cmd_ArgCompletion_PathExtension(callback, "music", ".ogg", false);
}

/*
 ==================
 Cmd_ArgCompletion_VideoName
 ==================
*/
void Cmd_ArgCompletion_VideoName (void (*callback)(const char *string)){

	Cmd_ArgCompletion_PathExtension(callback, "videos", ".RoQ", false);
}

/*
 ==================
 Cmd_ArgCompletion_MaterialName
 ==================
*/
void Cmd_ArgCompletion_MaterialName (void (*callback)(const char *string)){

	R_EnumMaterialDefs(callback);
}


/*
 ==============================================================================

 COMMAND ARGUMENTS

 ==============================================================================
*/

static int				cmd_argc;
static char *			cmd_argv[MAX_STRING_TOKENS];
static char				cmd_args[MAX_STRING_CHARS];


/*
 ==================
 Cmd_Argc
 ==================
*/
int	Cmd_Argc (){

	return cmd_argc;
}

/*
 ==================
 Cmd_Argv
 ==================
*/
char *Cmd_Argv (int arg){

	if (arg < 0 || arg >= cmd_argc)
		return "";

	return cmd_argv[arg];	
}

/*
 ==================
 Cmd_Args
 ==================
*/
char *Cmd_Args (){

	return cmd_args;
}

/*
 ==================
 Cmd_MacroExpandString

 $Cvars will be expanded unless they are in a quoted token
 ==================
*/
static char *Cmd_MacroExpandString (const char *text){

	static char	expanded[MAX_STRING_CHARS];
	char		temporary[MAX_STRING_CHARS];
	char		*scan, *start, *token;
	int			i, length, count;
	bool		inQuote = false;

	length = Str_Length(text);
	if (length >= MAX_STRING_CHARS){
		Com_Printf("Line exceeded %i chars, discarded\n", MAX_STRING_CHARS);
		return NULL;
	}

	scan = (char *)text;
	count = 0;

	for (i = 0; i < length; i++){
		if (scan[i] == '"')
			inQuote ^= 1;
		if (inQuote)
			continue;	// Don't expand inside quotes
		if (scan[i] != '$')
			continue;

		// Scan out the complete macro
		start = scan + (i + 1);
		token = Com_Parse(&start);
		if (!start)
			continue;

		token = (char *)CVar_GetVariableString(token);

		length += Str_Length(token);
		if (length >= MAX_STRING_CHARS){
			Com_Printf("Expanded line exceeded %i chars, discarded\n", MAX_STRING_CHARS);
			return NULL;
		}

		Str_Copy(temporary, scan, i + 1);
		Str_Append(temporary, token, sizeof(temporary));
		Str_Append(temporary, start, sizeof(temporary));

		Str_Copy(expanded, temporary, sizeof(expanded));
		scan = expanded;
		i--;

		if (++count == 100){
			Com_Printf("Macro expansion loop, discarded\n");
			return NULL;
		}
	}

	if (inQuote){
		Com_Printf("Line has unmatched quote, discarded\n");
		return NULL;
	}

	return scan;
}

/*
 ==================
 Cmd_TokenizeString

 Parses the given string into command line tokens
 ==================
*/
void Cmd_TokenizeString (const char *text){

	int		i;
	char	*txt, *token;

	// Clear the args from the last string
	for (i = 0; i < cmd_argc; i++)
		Mem_Free(cmd_argv[i]);

	cmd_argc = 0;
	cmd_args[0] = 0;

	if (!text)
		return;

	txt = (char *)text;
	while (1){
		// Skip whitespace up to a \n
		while (*txt && *txt <= ' ' && *txt != '\n')
			txt++;

		// A newline separates commands in the buffer
		if (*txt == '\n'){
			txt++;
			break;
		}

		if (!*txt)
			break;

		token = Com_Parse(&txt);
		if (!txt)
			break;

		if (cmd_argc < MAX_STRING_TOKENS){
			cmd_argv[cmd_argc] = Mem_DupString(token, TAG_COMMON);
			cmd_argc++;
		}
	}

	// Set cmd_args to everything after the first arg
	for (i = 1; i < cmd_argc; i++){
		Str_Append(cmd_args, cmd_argv[i], sizeof(cmd_args));
		if (i != cmd_argc - 1)
			Str_Append(cmd_args, " ", sizeof(cmd_args));
	}
}


/*
 ==============================================================================

 COMMAND EXECUTION

 ==============================================================================
*/


/*
 ==================
 Cmd_ExecuteString

 A complete command line has been parsed, so try to execute it
 ==================
*/
void Cmd_ExecuteString (const char *text){

	cmd_t		*cmd;
	cmdAlias_t	*alias;
	const char	*name;
	uint		hashKey;

	// Macro expand the text
	text = Cmd_MacroExpandString(text);

	// Tokenize
	Cmd_TokenizeString(text);

	// Execute the command line
	if (!Cmd_Argc())
		return;		// No tokens

	name = Cmd_Argv(0);

	// Check commands
	hashKey = Str_HashKey(name, CMDS_HASH_SIZE, false);

	for (cmd = cmd_functionsHashTable[hashKey]; cmd; cmd = cmd->nextHash){
		if (!Str_ICompare(cmd->name, name)){
			if (cmd->function)
				cmd->function();
			else
				Cmd_ExecuteString(Str_VarArgs("cmd %s", text));	// Forward to server command

			return;
		}
	}

	// Check alias commands
	hashKey = Str_HashKey(name, ALIAS_HASH_SIZE, false);

	for (alias = cmd_aliasHash[hashKey]; alias; alias = alias->nextHash){
		if (!Str_ICompare(alias->name, name)){
			if (++cmd_aliasCount == MAX_ALIAS_COUNT){
				Com_Printf("MAX_ALIAS_COUNT hit\n");
				return;
			}

			Cmd_InsertText(alias->command);
			return;
		}
	}

	// Check variables
	if (CVar_Command())
		return;

	// Send it as a server command if we are connected
	CL_ForwardCommandToServer();
}


/*
 ==============================================================================

 COMMAND TEXT BUFFERING

 ==============================================================================
*/

#define MAX_COMMAND_BUFFER			65536

typedef struct {
	char				text[MAX_COMMAND_BUFFER];
	int					size;
} cmdBuffer_t;

static cmdBuffer_t		cmd_buffer;
static char				cmd_deferTextBuffer[MAX_COMMAND_BUFFER];


/*
 ==================
 Cmd_AppendText
 ==================
*/
void Cmd_AppendText (const char *text){

	int		length;

	length = Str_Length(text);
	if (length == 0)
		return;

	if (cmd_buffer.size + length > MAX_COMMAND_BUFFER){
		Com_Printf(S_COLOR_RED "Cmd_AppendText: overflow\n");
		return;
	}

	// Append the entire text
	Mem_Copy(cmd_buffer.text + cmd_buffer.size, text, length);
	cmd_buffer.size += length;
}

/*
 ==================
 Cmd_InsertText
 ==================
*/
void Cmd_InsertText (const char *text){

	int		length;
	int		i;

	length = Str_Length(text) + 1;
	if (length == 1)
		return;

	if (cmd_buffer.size + length > MAX_COMMAND_BUFFER){
		Com_Printf(S_COLOR_RED "Cmd_InsertText: overflow\n");
		return;
	}

	// Move any commands still remaining in the buffer
	for (i = cmd_buffer.size - 1; i >= 0; i--)
		cmd_buffer.text[i + length] = cmd_buffer.text[i];

	// Insert the entire text
	Mem_Copy(cmd_buffer.text, text, length - 1);
	cmd_buffer.size += length;

	// Add a \n
	cmd_buffer.text[length - 1] = '\n';
}

/*
 ==================
 Cmd_ExecuteText
 ==================
*/
void Cmd_ExecuteText (cmdExec_t exec, const char *text){

	switch (exec){
	case CMD_EXEC_APPEND:
		Cmd_AppendText(text);
		break;
	case CMD_EXEC_INSERT:
		Cmd_InsertText(text);
		break;
	case CMD_EXEC_NOW:
		Cmd_ExecuteString(text);
		break;
	default:
		Com_Error(ERR_FATAL, "Cmd_ExecuteText: bad exec");
	}
}

/*
 ==================
 Cmd_ExecuteBuffer
 ==================
*/
void Cmd_ExecuteBuffer (){

	char	line[MAX_STRING_LENGTH];
	char	*text;
	bool	inQuote;
	int		i, length;

	cmd_aliasCount = 0;		// Don't allow infinite alias loops

	while (cmd_buffer.size){
		if (cmd_wait){
			// Skip out while text still remains in the buffer, leaving it for
			// next frame
			cmd_wait--;
			break;
		}

		// Find a \n or ; line break
		text = cmd_buffer.text;
		inQuote = false;

		for (length = 0; length < cmd_buffer.size; length++){
			if (text[length] == '"')
				inQuote = !inQuote;
			if (text[length] == ';' && !inQuote)
				break;
			if (text[length] == '\n')
				break;
		}

		if (length > MAX_STRING_LENGTH - 1)
			length = MAX_STRING_LENGTH - 1;

		Mem_Copy(line, text, length);
		line[length] = 0;

		// Delete the text from the command buffer and move remaining commands
		// down. This is necessary because some commands (exec, vstr, etc) can
		// insert data at the beginning of the buffer.
		if (length == cmd_buffer.size)
			cmd_buffer.size = 0;
		else {
			length++;
			cmd_buffer.size -= length;

			for (i = 0; i < cmd_buffer.size; i++)
				cmd_buffer.text[i] = cmd_buffer.text[i + length];
		}

		// Execute the command line
		Cmd_ExecuteString(line);
	}
}

/*
 ==================
 Cmd_ClearBuffer
 ==================
*/
static void Cmd_ClearBuffer (){

	cmd_buffer.size = 0;
}

/*
 ==================
 Cmd_CopyToDefer
 ==================
*/
void Cmd_CopyToDefer (){

	Mem_Copy(cmd_deferTextBuffer, cmd_buffer.text, cmd_buffer.size);
	cmd_deferTextBuffer[cmd_buffer.size] = 0;
	cmd_buffer.size = 0;
}

/*
 ==================
 Cmd_InsertFromDefer
 ==================
*/
void Cmd_InsertFromDefer (){

	Cmd_InsertText(cmd_deferTextBuffer);
	cmd_deferTextBuffer[0] = 0;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 Cmd_SortCmdList
 ==================
*/
static int Cmd_SortCmdList (const void *elem1, const void *elem2){

	const cmd_t		*cmd1 = *(const cmd_t **)elem1;
	const cmd_t		*cmd2 = *(const cmd_t **)elem2;

	return Str_Compare(cmd1->name, cmd2->name);
}

/*
 ==================
 Cmd_Wait_f

 Causes execution of the remainder of the command buffer to be delayed 
 during the given number of frames. This allows commands like:
 bind g "+attack ; wait ; -attack"
 ==================
*/
static void Cmd_Wait_f (){

	int		frames;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: wait [frames]\n");
		return;
	}

	if (Cmd_Argc() == 1){
		cmd_wait++;
		return;
	}

	frames = Str_ToInteger(Cmd_Argv(1));
	if (frames <= 0)
		return;

	cmd_wait += frames;
}

/*
 ==================
 Cmd_Exec_f
 ==================
*/
static void Cmd_Exec_f (){

	char	name[MAX_PATH_LENGTH];
	char	*text;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: exec <fileName>\n");
		return;
	}

	Str_Copy(name, Cmd_Argv(1), sizeof(name));
	Str_DefaultFileExtension(name, sizeof(name), ".cfg");

	// Load the file
	FS_ReadFile(name, (void **)&text);
	if (!text){
		Com_Printf("Couldn't exec %s\n", name);
		return;
	}

	Com_Printf("Execing %s\n", name);

	Cmd_InsertText(text);

	FS_FreeFile(text);
}

/*
 ==================
 Cmd_Echo_f
 ==================
*/
static void Cmd_Echo_f (){

	int		i;
	
	for (i = 1; i < Cmd_Argc(); i++)
		Com_Printf("%s ", Cmd_Argv(i));
	
	Com_Printf("\n");
}

/*
 ==================
 Cmd_Alias_f
 ==================
*/
static void Cmd_Alias_f (){

	cmdAlias_t	*alias;
	char		*name, command[MAX_STRING_CHARS];
	int			i;
	uint		hashKey;

	if (Cmd_Argc() == 1){
		Com_Printf("Current alias commands:\n");

		for (alias = cmd_alias; alias; alias = alias->next)
			Com_Printf("%-32s : %s", alias->name, alias->command);

		return;
	}

	name = Cmd_Argv(1);

	// If the alias already exists, reuse it
	hashKey = Str_HashKey(name, ALIAS_HASH_SIZE, false);

	for (alias = cmd_aliasHash[hashKey]; alias; alias = alias->next){
		if (!Str_ICompare(alias->name, name)){
			Mem_Free(alias->command);
			break;
		}
	}

	if (!alias){
		// Allocate a new alias
		alias = (cmdAlias_t *)Mem_ClearedAlloc(sizeof(cmdAlias_t), TAG_COMMON);

		alias->name = Mem_DupString(name, TAG_COMMON);

		// Link the alias in
		alias->next = cmd_alias;
		cmd_alias = alias;

		// Add to hash table
		alias->nextHash = cmd_aliasHash[hashKey];
		cmd_aliasHash[hashKey] = alias;
	}

	// Copy the rest of the command line
	command[0] = 0;
	for (i = 2; i < Cmd_Argc(); i++){
		Str_Append(command, Cmd_Argv(i), sizeof(command));
		if (i != Cmd_Argc() - 1)
			Str_Append(command, " ", sizeof(command));
	}
	Str_Append(command, "\n", sizeof(command));

	alias->command = Mem_DupString(command, TAG_COMMON);
}

/*
 ==================
 Cmd_ListCmds_f
 ==================
*/
static void Cmd_ListCmds_f (){

	cmd_t		*cmd, *list[MAX_LIST_CMDS];
	const char	*filter = NULL;
	int			found = 0, total = 0;
	int			i;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: listCmds [filter]\n");
		return;
	}

	if (Cmd_Argc() == 2)
		filter = Cmd_Argv(1);

	// Find matching commands
	for (cmd = cmd_functions; cmd; cmd = cmd->next){
		total++;

		if (found == MAX_LIST_CMDS)
			continue;

		if (filter){
			if (!Str_MatchFilter(cmd->name, filter, false))
				continue;
		}

		list[found++] = cmd;
	}

	// Sort the list
	qsort(list, found, sizeof(cmd_t *), Cmd_SortCmdList);

	// Print the list
	for (i = 0; i < found; i++){
		cmd = list[i];

		Com_Printf("%-32s", cmd->name);

		if (cmd->description)
			Com_Printf(" %s\n", cmd->description);
		else
			Com_Printf("\n");
	}

	if (found == MAX_LIST_CMDS)
		Com_Printf("...\n");

	Com_Printf("--------------------\n");
	Com_Printf("%i cmds listed (%i total cmds)\n", found, total);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 Cmd_Init
 ==================
*/
void Cmd_Init (){

	// Add commands
	Cmd_AddCommand("wait", Cmd_Wait_f, "Delays remaining commands a given number of frames", NULL);
	Cmd_AddCommand("exec", Cmd_Exec_f, "Executes a config file", NULL);
	Cmd_AddCommand("echo", Cmd_Echo_f, "Echoes text to the console", NULL);
	Cmd_AddCommand("alias", Cmd_Alias_f, "Creates a new command that executes a command string (possibly ; separated)", NULL);
	Cmd_AddCommand("listCmds", Cmd_ListCmds_f, "Lists commands", NULL);

	// Clear wait counter
	cmd_wait = 0;

	// Clear command buffer
	Cmd_ClearBuffer();
}

/*
 ==================
 Cmd_Shutdown
 ==================
*/
void Cmd_Shutdown (){

	// Remove commands
	Cmd_RemoveCommand("wait");
	Cmd_RemoveCommand("exec");
	Cmd_RemoveCommand("echo");
	Cmd_RemoveCommand("alias");
	Cmd_RemoveCommand("listCmds");

	// Clear command list
	Mem_Fill(cmd_functionsHashTable, 0, sizeof(cmd_functionsHashTable));

	cmd_functions = NULL;
}