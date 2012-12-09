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
// common.c - Misc. functions used in client and server
//

// TODO
// - Com_AddEarlyCommands, Com_AddLateCommands, Com_Init


#include "common.h"
#include <setjmp.h>


#define	MAX_PRINTMSG				8192
#define MAX_ARGS					64

static int					com_argc;
static const char *			com_argv[MAX_ARGS];

static bool					com_editorActive;
static char					com_editorName[64];
static editorCallbacks_t	com_editorCallbacks;

static int					com_redirectTarget;
static char *				com_redirectBuffer;
static int					com_redirectSize;
static void					(*com_redirectFlush)(int target, const char *buffer);

static FILE *				com_logFileHandle;

static int					com_serverState;

static char					com_errorMessage[MAX_PRINT_MESSAGE];

static jmp_buf				com_abortFrame;

// TODO: remove/replace these
int							com_timeBefore, com_timeBetween, com_timeAfter;
int							com_timeBeforeGame, com_timeAfterGame;
int							com_timeBeforeRef, com_timeAfterRef;
// ------

int							com_timeAll = 0;
int							com_timeWaiting = 0;
int							com_timeServer = 0;
int							com_timeClient = 0;
int							com_timeFrontEnd = 0;
int							com_timeBackEnd = 0;
int							com_timeSound = 0;

bool						com_initialized = false;

cvar_t *					com_version;
cvar_t *					com_developer;
cvar_t *					dedicated;
cvar_t *					paused;
cvar_t *					com_timeDemo;
cvar_t *					fixedtime;
cvar_t *					timescale;
cvar_t *					com_aviDemo;
cvar_t *					com_forceAviDemo;
cvar_t *					com_speeds;
cvar_t *					com_debugMemory;
cvar_t *					com_zoneMegs;
cvar_t *					com_hunkMegs;
cvar_t *					com_logFile;


/*
 ==============================================================================

 CLIENT / SERVER INTERACTIONS

 ==============================================================================
*/


/*
 ==================
 Com_BeginRedirect
 ==================
*/
void Com_BeginRedirect (int target, char *buffer, int size, void (*flush)(int target, const char *buffer)){

	if (!target || !buffer || !size || !flush)
		return;

	com_redirectTarget = target;
	com_redirectBuffer = buffer;
	com_redirectSize = size;
	com_redirectFlush = flush;

	*com_redirectBuffer = 0;
}

/*
 ==================
 Com_EndRedirect
 ==================
*/
void Com_EndRedirect (){

	if (!com_redirectFlush)
		return;

	com_redirectFlush(com_redirectTarget, com_redirectBuffer);

	com_redirectTarget = 0;
	com_redirectBuffer = NULL;
	com_redirectSize = 0;
	com_redirectFlush = NULL;
}

/*
 ==================
 Com_Redirect
 ==================
*/
void Com_Redirect (const char *msg){

	if (!com_redirectTarget)
		return;

	if ((Str_Length(msg) + Str_Length(com_redirectBuffer)) > (com_redirectSize - 1)){
		com_redirectFlush(com_redirectTarget, com_redirectBuffer);
		*com_redirectBuffer = 0;
	}

	Str_Append(com_redirectBuffer, msg, com_redirectSize);
}

/*
 ==================
 Com_OpenLogFile
 ==================
*/
static void Com_OpenLogFile (){

	char		name[MAX_PATH_LENGTH];
	time_t		t;
	struct tm	*lt;

	if (com_logFileHandle)
		return;

	Str_SPrintf(name, sizeof(name), "%s/console.log", Sys_DefaultSaveDirectory());

	if (com_logFile->integerValue == 1 || com_logFile->integerValue == 2)
		com_logFileHandle = fopen(name, "wt");
	else if (com_logFile->integerValue == 3 || com_logFile->integerValue == 4)
		com_logFileHandle = fopen(name, "at");

	if (!com_logFileHandle)
		return;

	if (com_logFile->integerValue == 2 || com_logFile->integerValue == 4)
		setvbuf(com_logFileHandle, NULL, _IONBF, 0);

	time(&t);
	lt = localtime(&t);

	fprintf(com_logFileHandle, "%s %s (%s %s)", ENGINE_VERSION, BUILD_STRING, __DATE__, __TIME__);
	fprintf(com_logFileHandle, "\nLog file opened on %s\n\n", asctime(lt));
}

/*
 ==================
 Com_CloseLogFile
 ==================
*/
static void Com_CloseLogFile (){

	time_t		t;
	struct tm	*lt;

	if (!com_logFileHandle)
		return;

	time(&t);
	lt = localtime(&t);

	fprintf(com_logFileHandle, "\nLog file closed on %s\n\n", asctime(lt));

	fclose(com_logFileHandle);
	com_logFileHandle = NULL;
}

/*
 ==================
 Com_Print
 ==================
*/
static void Com_Print (const char *text){

	if (com_redirectTarget){
		Com_Redirect(text);
		return;
	}

	// Print to client console
	Con_Print(text);

	// Also echo to dedicated console
	Sys_Print(text);

	// Print to log file
	if (com_logFile){
		if (com_logFile->integerValue)
			Com_OpenLogFile();
		else
			Com_CloseLogFile();

		if (com_logFileHandle)
			fprintf(com_logFileHandle, "%s", text);
	}
}

/*
 ==================
 Com_Printf

 Both client and server can use this, and it will output to the 
 appropriate place
 ==================
*/
void Com_Printf (const char *fmt, ...){

	char	message[MAX_PRINT_MESSAGE];
	va_list	argPtr;

	va_start(argPtr, fmt);
	Str_VSPrintf(message, sizeof(message), fmt, argPtr);
	va_end(argPtr);

	Com_Print(message);
}

/*
 ==================
 Com_DPrintf

 A Com_Printf that only shows up if the "com_developer" cvar is set
 ==================
*/
void Com_DPrintf (const char *fmt, ...){

	char	message[MAX_PRINT_MESSAGE];
	va_list	argPtr;

	if (!com_developer || !com_developer->integerValue)
		return;		// Don't confuse non-developers with techie stuff

	va_start(argPtr, fmt);
	Str_VSPrintf(message, sizeof(message), fmt, argPtr);
	va_end(argPtr);

	Com_Print(message);
}

/*
 ==================
 Com_Error

 Both client and server can use this, and it will do the appropriate
 things
 ==================
*/
void Com_Error (int code, const char *fmt, ...){

	static bool	recursive;
	static int	count, lastTime;
	int			time;
	va_list		argPtr;

	if (recursive)
		Sys_Error("Recursive error after: %s", com_errorMessage);

	recursive = true;

	// Get the message
	va_start(argPtr, fmt);
	Str_VSPrintf(com_errorMessage, sizeof(com_errorMessage), fmt, argPtr);
	va_end(argPtr);

	// If we are getting a solid stream of errors, do a fatal error
	time = Sys_Milliseconds();

	if (time - lastTime >= 100)
		count = 0;
	else {
		if (++count > 3)
			code = ERR_FATAL;
	}

	lastTime = time;

	// Handle the error
	if (code == ERR_DISCONNECT){
		Com_Printf(S_COLOR_RED "****************************************\n");
		Com_Printf(S_COLOR_RED "ERROR: %s\n", com_errorMessage);
		Com_Printf(S_COLOR_RED "****************************************\n");

		CL_Drop();

		recursive = false;
		longjmp(com_abortFrame, -1);
	}
	else if (code == ERR_DROP){
		Com_Printf(S_COLOR_RED "****************************************\n");
		Com_Printf(S_COLOR_RED "ERROR: %s\n", com_errorMessage);
		Com_Printf(S_COLOR_RED "****************************************\n");

		SV_Shutdown(Str_VarArgs("Server crashed: %s\n", com_errorMessage), false);
		CL_Drop();

		recursive = false;
		longjmp(com_abortFrame, -1);
	}

	// ERR_FATAL
	SV_Shutdown(Str_VarArgs("Server fatal crashed: %s\n", com_errorMessage), false);

	Sys_Error("%s", com_errorMessage);
}


/*
 ==============================================================================

 INTEGRATED EDITORS INTERFACE

 ==============================================================================
*/


/*
 ==================
 Com_LaunchEditor
 ==================
*/
bool Com_LaunchEditor (const char *name, editorCallbacks_t *callbacks){

	void	*wndHandle;

	if (com_editorActive){
		if (!Str_ICompare(com_editorName, name)){
			Com_Printf("The %s editor is already active\n", com_editorName);
			return false;
		}

		Com_Printf("You must close the %s editor to launch the %s editor\n", com_editorName, name);

		return false;
	}

	if (!CL_CanLaunchEditor(name))
		return false;

	Com_Printf("Launching %s editor...\n", name);

	// Set the editor state
	com_editorActive = true;
	Str_Copy(com_editorName, name, sizeof(com_editorName));
	Mem_Copy(&com_editorCallbacks, callbacks, sizeof(com_editorCallbacks));

	// Create the editor window
	wndHandle = com_editorCallbacks.createWindow();
	if (!wndHandle){
		Com_CloseEditor();
		return false;
	}

	Sys_SetEditorWindow(wndHandle);

	return true;
}

/*
 ==================
 Com_CloseEditor
 ==================
*/
void Com_CloseEditor (){

	if (!com_editorActive)
		return;

	Com_Printf("Closing %s editor...\n", com_editorName);

	// Destroy the editor window
	Sys_SetEditorWindow(NULL);

	com_editorCallbacks.destroyWindow();

	// Clear the editor state
	com_editorActive = false;
}

/*
 ==================
 Com_IsEditorActive
 ==================
*/
bool Com_IsEditorActive (){

	return com_editorActive;
}

/*
 ==================
 Com_EditorEvent
 ==================
*/
bool Com_EditorEvent (){

	if (!com_editorActive)
		return false;

	// Perform a callback if possible
	if (!com_editorCallbacks.mouseEvent)
		return false;

	return com_editorCallbacks.mouseEvent();
}


// ============================================================================


/*
 ==================
 Com_ServerState
 ==================
*/
int Com_ServerState (){

	return com_serverState;
}

/*
 ==================
 Com_SetServerState
 ==================
*/
void Com_SetServerState (int state){

	com_serverState = state;
}

/*
 ==================
 Com_WriteConfigToFile
 ==================
*/
void Com_WriteConfigToFile (const char *name){

	fileHandle_t	f;

	// If not initialized, make sure we don't write out anything
	if (!com_initialized)
		return;

	// Write key bindings and archive variables to the given config file
	FS_OpenFile(name, FS_WRITE, &f);
	if (!f){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}

	FS_Printf(f, "// Generated by " ENGINE_NAME ", do not modify!" NEWLINE NEWLINE);

	Key_WriteBindings(f);
	CVar_WriteVariables(f);

	FS_CloseFile(f);
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 Com_Quit_f

 Both client and server can use this, and it will do the appropriate
 things
 ==================
*/
static void Com_Quit_f (){

	Sys_Quit();
}

/*
 ==================
 Com_ShowConsole_f
 ==================
*/
static void Com_ShowConsole_f (){

	Sys_ShowConsole(true);
}

/*
 ==================
 Com_Error_f

 Just trow a fatal or drop error to test error shutdown procedures
 ==================
*/
static void Com_Error_f (){

	if (!com_developer || !com_developer->integerValue){
		Com_Printf("This command can only be used in developer mode\n");
		return;
	}

	if (Cmd_Argc() > 1)
		Com_Error(ERR_DROP, "Testing drop error...");
	else
		Com_Error(ERR_FATAL, "Testing fatal error...");
}

/*
 ==================
 Com_Setenv_f
 ==================
*/
static void Com_Setenv_f (){

	char	buffer[1024], *env;
	int		i;

	if (Cmd_Argc() < 2){
		Com_Printf("Usage: setenv <variable> [value]\n");
		return;
	}

	if (Cmd_Argc() == 2){
		env = getenv(Cmd_Argv(1));
		if (env)
			Com_Printf("%s=%s\n", Cmd_Argv(1), env);
		else
			Com_Printf("%s undefined\n", Cmd_Argv(1), env);
	}
	else {
		Str_Copy(buffer, Cmd_Argv(1), sizeof(buffer));
		Str_Append(buffer, "=", sizeof(buffer));

		for (i = 2; i < Cmd_Argc(); i++){
			Str_Append(buffer, Cmd_Argv(i), sizeof(buffer));
			Str_Append(buffer, " ", sizeof(buffer));
		}

		putenv(buffer);
	}
}

/*
 ==================
 Com_WriteConfig_f
 ==================
*/
static void Com_WriteConfig_f (){

	char	name[MAX_PATH_LENGTH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: writeConfig <fileName>\n");
		return;
	}

	Str_Copy(name, Cmd_Argv(1), sizeof(name));
	Str_DefaultFileExtension(name, sizeof(name), ".cfg");

	Com_Printf("Writing %s\n", name);

	Com_WriteConfigToFile(name);
}

/*
 ==================
 Com_Pause_f
 ==================
*/
static void Com_Pause_f (){

	// Never pause in multiplayer
	if (!com_serverState || CVar_GetVariableInteger("maxclients") > 1){
		CVar_ForceSet("paused", "0");
		return;
	}

	CVar_SetVariableInteger("paused", !paused->integerValue, false);
}


/*
 ==============================================================================

 COMMAND LINE PARSING

 ==============================================================================
*/


/*
 ==================
 Com_ParseCommandLine

 Breaks the command line into multiple lines
 ==================
*/
static void Com_ParseCommandLine (const char *cmdLine){

	bool	inQuote = false;

	while (*cmdLine){
		if (*cmdLine == '"')
			inQuote = !inQuote;

		if (*cmdLine == '+' && !inQuote){
			if (com_argc == MAX_ARGS)
				break;

			com_argv[com_argc++] = cmdLine + 1;

			*(char *)cmdLine = 0;
		}

		cmdLine++;
	}
}

/*
 =================
 Com_AddEarlyCommands

 Adds command line parameters as script statements.
 Commands lead with a +, and continue until another +.

 Set commands are added early, so they are guaranteed to be set before
 the client and server initialize for the first time.

 Other commands are added late, after all initialization is complete.
 =================
*/
static void Com_AddEarlyCommands (bool clear){

	int		i;

	for (i = 1; i < com_argc; i++){
		if (Q_stricmp(com_argv[i], "+set"))
			continue;

		Cmd_AppendText(va("set %s %s\n", com_argv[i+1], com_argv[i+2]));

		if (clear){
			com_argv[i+0] = "";
			com_argv[i+1] = "";
			com_argv[i+2] = "";
		}

		i += 2;
	}
}

/*
 =================
 Com_AddLateCommands

 Adds command line parameters as script statements.
 Commands lead with a + and continue until another +.

 Returns true if any late commands were added, which will keep the 
 logo cinematic from immediately starting.
 =================
*/
static bool Com_AddLateCommands (void){

	int		i, j;
	char	text[1024];
	bool	ret = false;

	for (i = 1; i < com_argc; ){
		if (com_argv[i][0] != '+'){
			i++;
			continue;
		}

		for (j = 1; com_argv[i][j]; j++){
			if (com_argv[i][j] != '+')
				break;
		}

		Q_strncpyz(text, com_argv[i]+j, sizeof(text));
		Q_strncatz(text, " ", sizeof(text));
		i++;

		while (i < com_argc){
			if (com_argv[i][0] == '+')
				break;

			Q_strncatz(text, com_argv[i], sizeof(text));
			Q_strncatz(text, " ", sizeof(text));
			i++;
		}

		if (text[0]){
			ret = true;

			text[strlen(text)-1] = '\n';
			Cmd_AppendText(text);
		}
	}

	return ret;
}

/*
 ==================
 Com_SafeMode

 Checks for "safe" on the command line, which will skip executing config files
 ==================
*/
bool Com_SafeMode (){

	int		i;

	for (i = 0; i < com_argc; i++){
		if (!com_argv[i] || !com_argv[i][0])
			continue;

		if (!Str_ICompare(com_argv[i], "safe")){
			com_argv[i] = NULL;

			return true;
		}
	}

	return false;
}


// ============================================================================


/*
 ==================
 Com_PrintStats
 ==================
*/
static void Com_PrintStats (){

	int		all, sv, gm, cl, rf;

	if (!com_speeds->integerValue)
		return;

	all = com_timeAfter - com_timeBefore;
	sv = com_timeBetween - com_timeBefore;
	cl = com_timeAfter - com_timeBetween;
	gm = com_timeAfterGame - com_timeBeforeGame;
	rf = com_timeAfterRef - com_timeBeforeRef;
	sv -= gm;
	cl -= rf;

	Com_Printf("all:%3i sv:%3i, gm:%3i, cl:%3i, rf:%3i\n", all, sv, gm, cl, rf);
}


/*
 ==============================================================================

 FRAMEWORK

 ==============================================================================
*/


/*
 ==================
 Com_Frame
 ==================
*/
void Com_Frame (int msec){

	// If an error occurred, drop the entire frame
	if (setjmp(com_abortFrame))
		return;

	// Modify msec
	if (!com_timeDemo->integerValue){
		if (com_aviDemo->integerValue > 0)
			msec = 1000 / com_aviDemo->integerValue;
		else {
			if (fixedtime->integerValue)
				msec = fixedtime->integerValue;
			if (timescale->floatValue)
				msec *= timescale->floatValue;
		}
	}

	if (msec < 1)
		msec = 1;

	// Update the config file if needed
	if (CVar_GetModifiedFlags() & CVAR_ARCHIVE){
		CVar_ClearModifiedFlags(CVAR_ARCHIVE);

		Com_WriteConfigToFile(CONFIG_FILE);
	}

	// Process commands
	Cmd_ExecuteBuffer();

	if (com_speeds->integerValue)
		com_timeBefore = Sys_Milliseconds();

	// Run server frame
	SV_Frame(msec);

	if (com_speeds->integerValue)
		com_timeBetween = Sys_Milliseconds();

	// Run client frame
	CL_Frame(msec);

	if (com_speeds->integerValue)
		com_timeAfter = Sys_Milliseconds();

	// Print collision system statistics
	CM_PrintStats();

	// Print common statistics
	Com_PrintStats();
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
void Com_Init (const char *cmdLine){

	if (setjmp(com_abortFrame))
		Sys_Error("Error during initialization: %s", com_errorMessage);

	Com_Printf("%s %s (%s %s)\n", ENGINE_VERSION, BUILD_STRING, __DATE__, __TIME__);

	// Parse the command line
	Com_ParseCommandLine(cmdLine);

	// We need to call Com_InitMemory twice, because some strings and
	// structs need to be allocated during initialization, but we want
	// to get the amount of memory to allocate for the main zone and
	// hunk from the config files
	Mem_Init();

	// Initialize the rest of the subsystems
	Sys_Init();

	// Initialize command system
	Cmd_Init();

	// Initialize cvar system
	CVar_Init();

	// Initialize key binding/input
	Key_Init();

	// We need to add the early commands twice, because some file system
	// cvars need to be set before execing config files, but we want
	// other parms to override the settings of the config files
	Com_AddEarlyCommands(false);
	Cmd_ExecuteBuffer();

	// Initialize file system
	FS_Init();

	Com_AddEarlyCommands(true);
	Cmd_ExecuteBuffer();

	// Register variables
	com_version = CVar_Register("version", Str_VarArgs("%s (%s)", ENGINE_VERSION, __DATE__), CVAR_STRING, CVAR_SERVERINFO | CVAR_READONLY, "Game version", 0, 0);
	com_developer = CVar_Register("com_developer", "0", CVAR_BOOL, 0, "Developer mode", 0, 0);
	dedicated = CVar_Register("dedicated", "0", CVAR_BOOL, CVAR_INIT, "Dedicated server", 0, 0);
	paused = CVar_Register("paused", "0", CVAR_BOOL, CVAR_CHEAT, "Pauses the game", 0, 0);
	com_timeDemo = CVar_Register("com_timeDemo", "0", CVAR_BOOL, CVAR_CHEAT, "Timing a demo", 0, 0);
	fixedtime = CVar_Register("fixedtime", "0", CVAR_INTEGER, CVAR_CHEAT, "Fixed time", 0, 1000);
	timescale = CVar_Register("timescale", "1", CVAR_FLOAT, CVAR_CHEAT, "Time scale", 0.0f, 1000.0f);
	com_aviDemo = CVar_Register("com_aviDemo", "0", CVAR_INTEGER, CVAR_CHEAT, NULL, 0, 1000);
	com_forceAviDemo = CVar_Register("com_forceAviDemo", "0", CVAR_BOOL, CVAR_CHEAT, NULL, 0, 0);
	com_speeds = CVar_Register("com_speeds", "0", CVAR_BOOL, CVAR_CHEAT, "Show engine speeds", 0, 0);
	com_debugMemory = CVar_Register("com_debugMemory", "0", CVAR_BOOL, CVAR_CHEAT, NULL, 0, 0);
	com_zoneMegs = CVar_Register("com_zoneMegs", "16", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, NULL, 0, 1000);
	com_hunkMegs = CVar_Register("com_hunkMegs", "48", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, NULL, 0, 1000);
	com_logFile = CVar_Register("com_logFile", "0", CVAR_INTEGER, 0, "Log console messages (1 = write, 2 = unbuffered write, 3 = append, 4 = unbuffered append)", 0, 4);

	// Add commands
	Cmd_AddCommand("quit", Com_Quit_f, "Quits the game", NULL);
	Cmd_AddCommand("showConsole", Com_ShowConsole_f, "Shows the system console", NULL);
	Cmd_AddCommand("error", Com_Error_f, "Throws an error", NULL);
	Cmd_AddCommand("setenv", Com_Setenv_f, NULL, NULL);
	Cmd_AddCommand("writeConfig", Com_WriteConfig_f, "Writes a config file", NULL);
	Cmd_AddCommand("pause", Com_Pause_f, "Pauses the game", NULL);

	// Initialize main zone and hunk memory
	Mem_Init();

	// Initialize server and client
	SV_Init();
	CL_Init();

	// Initialize collision system
	CM_Init();

	// Initialize networking
	NET_Init();

	NetChan_Init();

	// Initialize lookup table manager
	LUT_Init();

	// Show or hide the system console
	if (dedicated->integerValue)
		Sys_ShowConsole(true);
	else
		Sys_ShowConsole(false);

	dedicated->modified = false;

	// Add commands from the command line
	if (!Com_AddLateCommands()){
		// If the user didn't give any commands, run default action
		if (!dedicated->integerValue)
			Cmd_AppendText(DEFAULT_ACTION);
	}

	// Initialized
	com_initialized = true;

	Com_Printf("======== " ENGINE_NAME " Initialization Complete ========\n");
}

/*
 ==================
 Com_Shutdown
 ==================
*/
void Com_Shutdown (){

	static bool	isDown;

	// Avoid recursive shutdown
	if (isDown)
		return;
	isDown = true;

	// No longer initialized
	com_initialized = false;

	// Remove commands
	Cmd_RemoveCommand("quit");
	Cmd_RemoveCommand("showConsole");
	Cmd_RemoveCommand("error");
	Cmd_RemoveCommand("setenv");
	Cmd_RemoveCommand("writeconfig");
	Cmd_RemoveCommand("pause");

	// Shutdown server and client
	SV_Shutdown("Server quit\n", false);
	CL_Shutdown();

	// Shutdown collision system
	CM_Shutdown();

	// Shutdown lookup table manager
	LUT_Shutdown();

	// Shutdown networking
	NET_Shutdown();

	// Shutdown file system
	FS_Shutdown();

	// Shutdown key binding/input
	Key_Shutdown();

	// Shutdown cvar system
	CVar_Shutdown();

	// Shutdown command system
	Cmd_Shutdown();

	// Shutdown system services
	Sys_Shutdown();

	// Shutdown memory manager
	Mem_Shutdown();

	// Close log file
	Com_CloseLogFile();
}