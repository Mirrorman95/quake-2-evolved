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
// common.h - Common definitions between client and server
//


#ifndef __COMMON_H__
#define __COMMON_H__


#include "../shared/q_shared.h"

#include "fileFormats.h"

#include "memory.h"
#include "fileSystem.h"
#include "cmdSystem.h"
#include "cvarSystem.h"
#include "msgSystem.h"
#include "protocol.h"
#include "network.h"
#include "netChan.h"
#include "table.h"
#include "parser.h"
#include "system.h"

#include "../collision/cm_public.h"


#define ENGINE_NAME					"Quake 2 Evolved"
#define ENGINE_VERSION				"Quake 2 Evolved 0.42 BETA"

#define CONFIG_FILE					"Quake2Evolved.cfg"

// These directories will be ignored by file and directory scanning functions
#define CODE_DIRECTORY				"code"
#define DOCS_DIRECTORY				"docs"
#define SVN_DIRECTORY				".svn"

// The default action will be executed immediately after initialization
#define DEFAULT_ACTION				"playCinematic idlog.cin\n"

#define	MAX_PRINT_MESSAGE			8192

typedef struct {
	// Called by LaunchEditor to create the window for the active editor.
	// Should return the handle of the created window, or NULL if an error
	// occurred.
	void *					(*createWindow)();

	// Called by CloseEditor to destroy the window for the active editor
	void					(*destroyWindow)();

	// Called by the client system for left mouse button down events to let the
	// active editor select an entity for editing.
	// Should return true if the client system should skip generating game
	// commands for said event.
	// This function can be NULL if the active editor doesn't need to select
	// entities.
	bool					(*mouseEvent)();
} editorCallbacks_t;

extern int					com_frameTime;
extern int					com_frameMsec;
extern int					com_frameCount;

extern int					com_timeAll;
extern int					com_timeWaiting;
extern int					com_timeServer;
extern int					com_timeClient;
extern int					com_timeFrontEnd;
extern int					com_timeBackEnd;
extern int					com_timeSound;

extern cvar_t *				com_version;
extern cvar_t *				com_dedicated;
extern cvar_t *				com_paused;
extern cvar_t *				com_developer;
extern cvar_t *				com_logFile;
extern cvar_t *				com_fixedTime;
extern cvar_t *				com_timeScale;
extern cvar_t *				com_speeds;
extern cvar_t *				com_clientRunning;
extern cvar_t *				com_serverRunning;
extern cvar_t *				com_timeDemo;
extern cvar_t *				com_maxFPS;

ulong		MD4_BlockChecksum (const void *data, int length);

byte		Com_BlockSequenceCRCByte (const byte *buffer, int length, int sequence);

// Begins redirection of console output to the given buffer
void		Com_BeginRedirect (int target, char *buffer, int size, void (*flush)(int target, const char *buffer));

// Ends redirection of console output
void		Com_EndRedirect ();

// Prints a message to the console, and possibly to the log file
void 		Com_Printf (const char *fmt, ...);

// Like Com_Printf, but only prints the message in developer mode
void 		Com_DPrintf (const char *fmt, ...);

// Throws an error and aborts the current frame. If fatal is true, it will quit
// to the system console and wait for the user to terminate the program.
// If disconnection or drop is true, it will exit the user from the game
// to the main menu and print the message.
void 		Com_Error (int code, const char *fmt, ...);

// Launches an integrated editor.
// Note that only one editor can be active at a time.
bool		Com_LaunchEditor (const char *name, editorCallbacks_t *callbacks);

// Closes the currently active integrated editor
void		Com_CloseEditor ();

// Returns true if an integrated editor is currently active
bool		Com_IsEditorActive ();

// Called by the client system for left mouse button down events.
// Will perform a callback to let the active editor select an entity for
// editing. The callback function should return true if it wants the client
// system to skip generating game commands for said event.
bool		Com_EditorEvent ();

// Returns the server state
int			Com_ServerState ();

// Sets the server state
void		Com_SetServerState (int state);

// Searches for command line parameters that are "set" commands.
// If match is not NULL, only that variable will be looked for.
// If once is true, the command will be removed. This is to keep it from being
// executed multiple times.
void		Com_StartupVariable (const char *match, bool once);

// Checks for "safe" on the command line, which will skip executing config
// files
bool		Com_SafeMode ();

// Runs a complete frame.
// Called from the program's main loop.
void		Com_Frame ();

// Initializes all of the engine subsystems
void		Com_Init (const char *cmdLine);

// Shuts down all of the engine subsystems
void		Com_Shutdown ();

/*
 ==============================================================================

 PLAYER MOVEMENT CODE

 Common between server and client so prediction matches
 ==============================================================================
*/

extern float				pm_airAccelerate;

void			PMove (pmove_t *pmove);

/*
 ==============================================================================

 CLIENT / SERVER SYSTEMS

 ==============================================================================
*/

void			R_DebugLine (const vec4_t color, const vec3_t start, const vec3_t end, bool depthTest, int allowInView);
void			R_DebugBounds (const vec4_t color, const vec3_t mins, const vec3_t maxs, bool depthTest, int allowInView);
void			R_DebugBox (const vec4_t color, const vec3_t origin, const vec3_t axis[3], const vec3_t mins, const vec3_t maxs, bool depthTest, int allowInView);
void			R_DebugPolygon (const vec4_t color, int numPoints, const vec3_t *points, bool fill, bool depthTest, int allowInView);
void			R_DebugText (const vec4_t color, bool forceColor, const vec3_t origin, float cw, float ch, const char *text, bool depthTest, int allowInView);

void			R_EnumMaterialDefs (void (*callback)(const char *string));

void			Con_Print (const char *text);

void			Key_WriteBindings (fileHandle_t f);
void			Key_Init ();
void			Key_Shutdown ();

void			CL_UpdateScreen ();
void			CL_ForwardCommandToServer ();
bool			CL_CanLaunchEditor (const char *editor);
void			CL_Drop ();
void			CL_MapLoading ();

void			CL_Frame (int msec);
void			CL_Init ();
void			CL_Shutdown ();

void			SV_Frame (int msec);
void			SV_Init ();
void			SV_Shutdown (const char *message, bool reconnect);


#endif	// __COMMON_H__