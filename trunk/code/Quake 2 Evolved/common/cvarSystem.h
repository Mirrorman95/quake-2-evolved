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
// cvarSystem.c - Dynamic variable tracking
//


#ifndef __CVARSYSTEM_H__
#define __CVARSYSTEM_H__


/*
 ==============================================================================

 Console Variables:

 CVars are used to hold boolean, numeric, or string variables that can be
 changed or displayed at the console as well as accessed directly in the code.

 CVars are mostly used to hold settings that can be changed from the console or
 saved to and loaded from config files, but are also occasionally used to
 communicate information between different modules of the program.

 The user can access CVars from the console in three ways:

    cvar                Prints the current value
    cvar X			    Sets the current value to X
    set cvar X		    Same as above, but creates the CVar if not present

 CVars may be declared multiple times using the same name. However, they will
 all reference the same value and changing the value of one CVar changes the
 value of all CVars with the same name.

 CVars are automatically range checked based on their type and any min/max
 specified.

 ==============================================================================
*/

extern cvar_t *			cvar_variables;

// Creates the variable if it doesn't exist, or returns the existing one.
// If the variable exists, the flags will be OR'ed in.
// The description parameter can be NULL if you don't want it.
cvar_t *		CVar_Register (const char *name, const char *value, cvarType_t type, int flags, const char *description, float min, float max);

// Sets the value of a variable
void			CVar_SetBool (cvar_t *cvar, bool value);
void			CVar_SetInteger (cvar_t *cvar, int value);
void			CVar_SetFloat (cvar_t *cvar, float value);
void			CVar_SetString (cvar_t *cvar, const char *value);

// Updates a latched variable
void			CVar_UpdateLatched (cvar_t *cvar);

// Finds a variable with the given name. Returns NULL if not found.
cvar_t *		CVar_FindVariable (const char *name);

// Called by the command system when argv[0] doesn't match a known command.
// Returns true if the command was a variable reference that was handled (print
// or change).
bool			CVar_Command ();

// Command completion using a callback for each valid string
void			CVar_CommandCompletion (void (*callback)(const char *string));

// Argument completion using a callback for each valid string
void			CVar_ArgumentCompletion (void (*callback)(const char *string));

// Gets the value of a variable by name. Returns false, zero, or an empty
// string if it doesn't exist.
bool			CVar_GetVariableBool (const char *name);
int				CVar_GetVariableInteger (const char *name);
float			CVar_GetVariableFloat (const char *name);
const char *	CVar_GetVariableString (const char *name);

// Sets the value of a variable by name. Creates the variable if it doesn't
// exist.
void			CVar_SetVariableBool (const char *name, bool value, bool force);
void			CVar_SetVariableInteger (const char *name, int value, bool force);
void			CVar_SetVariableFloat (const char *name, float value, bool force);
void			CVar_SetVariableString (const char *name, const char *value, bool force);

// Will set the variable even if CVAR_INIT, CVAR_LATCH or CVAR_ROM
void			CVar_ForceSet (const char *name, const char *value);

// Writes lines containing "seta <variable> <value>" to the given file for all
// the variables with the CVAR_ARCHIVE flag set.
// Used for config files.
void 			CVar_WriteVariables (fileHandle_t f);

// If allowCheats is false, all variables with the CVAR_CHEAT flag set will be
// forced to their default values. Otherwise the user will be able to change
// CVAR_CHEAT variables from the console or config files.
void			CVar_FixCheatVariables (bool allowCheats);

// Returns true if cheats are allowed
bool			CVar_AllowCheats ();

// Gets flags that tell what kind of variables have been modified
int				CVar_GetModifiedFlags ();

// Sets flags that tell what kind of variables have been modified
void			CVar_SetModifiedFlags (int flags);

// Clears flags to allow detecting another modification to variables
void			CVar_ClearModifiedFlags (int flags);

// Returns an info string containing all the variables with any of the given
// flags set
char *			CVar_InfoString (int flags);

// Initializes the cvar system
void			CVar_Init ();

// Shuts down the cvar system
void			CVar_Shutdown ();

// Stupid hack to workaround our changes to cvar_t
gamecvar_t *	CVar_GameUpdateOrCreate (cvar_t *cvar);
gamecvar_t *	CVar_GameRegister (char *name, char *value, int flags);
gamecvar_t *	CVar_GameSet (char *name, char *value);
gamecvar_t *	CVar_GameForceSet (char *name, char *value);


#endif	// __CVARSYSTEM_H__