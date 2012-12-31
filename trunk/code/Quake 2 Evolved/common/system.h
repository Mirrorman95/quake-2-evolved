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
// system.h - User system-specific code
//


#ifndef __SYSTEM_H__
#define __SYSTEM_H__


/*
 ==============================================================================

 System Interface:

 Portable interface to non-portable system services.

 ==============================================================================
*/

// Shows or hides the system console
void			Sys_ShowConsole (bool show);

// Returns a list of files and subdirectories that match the given extension
// (which must include a leading '.' and must not contain wildcards).
// If extension is NULL, all the files will be returned and all the
// subdirectories ignored.
// If extension is "/", all the subdirectories will be returned and all the
// files ignored.
// The returned list can optionally be sorted.
const char **	Sys_ListFiles (const char *directory, const char *extension, bool sort, int *numFiles);

// Returns a list of files and subdirectories that match the given filter.
// The returned list can optionally be sorted.
const char **	Sys_ListFilteredFiles (const char *directory, const char *filter, bool sort, int *numFiles);

// Frees the memory allocated by Sys_ListFiles and Sys_ListFilteredFiles
void			Sys_FreeFileList (const char **fileList);

// Creates the given directory
void			Sys_CreateDirectory (const char *directory);

// Removes the given directory, including all the subdirectories and files
// contained inside
void			Sys_RemoveDirectory (const char *directory);

// Returns the current directory
const char *	Sys_CurrentDirectory ();

// Returns the default base directory
const char *	Sys_DefaultBaseDirectory ();

// Returns the default save directory
const char *	Sys_DefaultSaveDirectory ();

// Returns the amount of free space (in megabytes) available on the disk
// containing the given directory
int				Sys_DiskFreeSpace (const char *directory);

// Loads the given dynamic library
void *			Sys_LoadLibrary (const char *libPath);

// Frees the given dynamic library
void			Sys_FreeLibrary (void *libHandle);

// Gets the address of the given procedure from the given dynamic library
void *			Sys_GetProcAddress (void *libHandle, const char *procName);

// Returns information about the CPU
int				Sys_GetProcessorId ();
int				Sys_GetProcessorCount ();
const char *	Sys_GetProcessorString ();

// Prints text to the system console and possibly other output windows (like
// debugger output)
void			Sys_Print (const char *text);

// Shuts down all the subsystems, shows the given error message, then waits for
// the user to quit.
// Used by Com_Error for fatal errors.
void			Sys_Error (const char *fmt, ...);

// Causes a breakpoint exception
void			Sys_DebugBreak ();

// Yields CPU time for the given number of milliseconds
void			Sys_Sleep (int msec);

// Processes events like user input and other system specific events.
// Returns the current time in milliseconds.
int				Sys_ProcessEvents ();

// Sys_Milliseconds should only be used for profiling purposes, any game
// related timing information should come from event timestamps
int				Sys_Milliseconds ();

// For accurate performance testing
longlong		Sys_ClockTicks ();
longlong		Sys_ClockTicksPerSecond ();

// Returns true if the main window is active
bool			Sys_IsWindowActive ();

// Returns true if the main window is fullscreen
bool			Sys_IsWindowFullscreen ();

// Sets the window handle of the currently active integrated editor
void			Sys_SetEditorWindow (void *wndHandle);

// Returns the handle to the application instance
void *			Sys_GetInstanceHandle ();

// Returns the handle to the application icon
void *			Sys_GetIconHandle ();

// Grabs text from the clipboard if available.
// The returned pointer must be freed by the caller.
const char *	Sys_GetClipboardText (void);

// Sends text to the clipboard
void			Sys_SetClipboardText (const char *text);

// Starts the given process and optionally quits the program immediately
void			Sys_StartProcess (const char *exePath, bool quit);

// Opens the given URL and optionally quits the program immediately
void			Sys_OpenURL (const char *url, bool quit);

// Initializes system services
void			Sys_Init ();

// Shuts down system services
void			Sys_Shutdown ();

// Shuts down all the subsystems and quits the program immediately
void			Sys_Quit ();


#endif	// __SYSTEM_H__