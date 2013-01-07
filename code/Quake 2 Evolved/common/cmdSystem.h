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
// cmdSystem.h - Script command processing
//


#ifndef __CMDSYSTEM_H__
#define __CMDSYSTEM_H__


/*
 ==============================================================================

 Console command execution and command text buffering:

 Any number of commands can be added in a frame from several different sources.
 Most commands come from either key bindings or console line input, but entire
 text files can be executed.
 The + command line arguments are also added to the command buffer.

 Command execution takes a NULL terminated string, breaks it into tokens, then
 searches for a command or variable that matches the first token.

 ==============================================================================
*/

typedef void				(*cmdFunction_t)(void);
typedef void				(*argCompletion_t)(void (*callback)(const char *string));

typedef enum {
	CMD_EXEC_APPEND,							// Append at the end of the command buffer
	CMD_EXEC_INSERT,							// Insert at the beginning of the command buffer
	CMD_EXEC_NOW								// Don't return until completed
} cmdExec_t;

// Adds a command and the function to call for it.
// If function is NULL, the command will be forwarded to the server as a CLC_STRINGCMD instead of
// executed locally.
// The description parameter can be NULL if you don't want it.
// The argCompletion parameter can be NULL, one of the default argument
// completion functions, or a custom argument completion function.
void			Cmd_AddCommand (const char *name, cmdFunction_t function, const char *description, argCompletion_t argCompletion);

// Removes a command
void			Cmd_RemoveCommand (const char *name);

// Used by the console variable code to check for cvar / command name overlap
bool			Cmd_Exists (const char *name);

// Command completion using a callback for each valid string
void			Cmd_CommandCompletion (void (*callback)(const char *string));

// Argument completion using a callback for each valid string
void			Cmd_ArgumentCompletion (void (*callback)(const char *string));

// Base for path/file argument completion
void			Cmd_ArgCompletion_PathExtension (void (*callback)(const char *string), const char *path, const char *extension, bool stripPath);

// Default argument completion functions
void			Cmd_ArgCompletion_FileName (void (*callback)(const char *string));
void			Cmd_ArgCompletion_ConfigName (void (*callback)(const char *string));
void			Cmd_ArgCompletion_DemoName (void (*callback)(const char *string));
void			Cmd_ArgCompletion_MapName (void (*callback)(const char *string));
void			Cmd_ArgCompletion_ModelName (void (*callback)(const char *string));
void			Cmd_ArgCompletion_TextureName (void (*callback)(const char *string));
void			Cmd_ArgCompletion_SoundName (void (*callback)(const char *string));
void			Cmd_ArgCompletion_MusicName (void (*callback)(const char *string));
void			Cmd_ArgCompletion_VideoName (void (*callback)(const char *string));
void			Cmd_ArgCompletion_MaterialName (void (*callback)(const char *string));
void			Cmd_ArgCompletion_SoundShaderName (void (*callback)(const char *string));

// The functions that execute commands get their parameters with these
// functions. Cmd_Argv() will return an empty string, not a NULL if
// arg > argc, so string operations are always safe.
int				Cmd_Argc ();
char *			Cmd_Argv (int arg);
char *			Cmd_Args ();

// Parses the given string into command line tokens. Does not need to be \n
// terminated.
// The text is copied to a separate buffer and 0 characters are inserted in the
// appropriate place. The argv array will point into this temporary buffer.
// $CVars will be expanded unless they are in a quoted string.
void			Cmd_TokenizeString (const char *text);

// As new commands are generated from the console or key bindings, the
// text is added to the end of the command buffer
void			Cmd_AppendText (const char *text);

// When a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining 
// unexecuted commands
void			Cmd_InsertText (const char *text);

// This can be used in place of either Cmd_AppendText or Cmd_InsertText
void			Cmd_ExecuteText (cmdExec_t exec, const char *text);

// Pulls off \n or ; terminated lines of text from the command buffer and
// executes the commands. Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!
void			Cmd_ExecuteBuffer ();

// These two functions are used to defer any pending commands while a 
// map is being loaded
void			Cmd_CopyToDefer ();
void			Cmd_InsertFromDefer ();

// Initializes the command system
void			Cmd_Init ();

// Shuts down the command system
void			Cmd_Shutdown ();


#endif	// __CMDSYSTEM_H__