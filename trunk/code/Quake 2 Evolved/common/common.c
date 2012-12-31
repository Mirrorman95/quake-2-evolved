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

int							com_frameTime;
int							com_frameMsec;
int							com_frameCount;

int							com_timeAll = 0;
int							com_timeWaiting = 0;
int							com_timeServer = 0;
int							com_timeClient = 0;
int							com_timeFrontEnd = 0;
int							com_timeBackEnd = 0;
int							com_timeSound = 0;

bool						com_initialized = false;

cvar_t *					com_version;
cvar_t *					com_dedicated;
cvar_t *					com_paused;
cvar_t *					com_developer;
cvar_t *					com_logFile;
cvar_t *					com_fixedTime;
cvar_t *					com_timeScale;
cvar_t *					com_speeds;
cvar_t *					com_clientRunning;
cvar_t *					com_serverRunning;
cvar_t *					com_timeDemo;
cvar_t *					com_maxFPS;


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
 Com_ShowConsole_f
 ==================
*/
static void Com_ShowConsole_f (){

	Sys_ShowConsole(true);
}

/*
 ==================
 Com_Error_f

 Just throw a drop error to test error shutdown procedures
 ==================
*/
static void Com_Error_f (){

	if (!com_developer || !com_developer->integerValue){
		Com_Printf("This command can only be used in developer mode\n");
		return;
	}

	Com_Error(ERR_DROP, "Testing drop error");
}

/*
 ==================
 Com_FatalError_f

 Just throw a fatal error to test error shutdown procedures
 ==================
*/
static void Com_FatalError_f (){

	if (!com_developer || !com_developer->integerValue){
		Com_Printf("This command can only be used in developer mode\n");
		return;
	}

	Com_Error(ERR_FATAL, "Testing fatal error");
}

/*
 ==================
 Com_Freeze_f

 Just freeze in place for a given number of milliseconds to test error recovery
 ==================
*/
static void Com_Freeze_f (){

	int		msec;

	if (!com_developer || !com_developer->integerValue){
		Com_Printf("This command can only be used in developer mode\n");
		return;
	}

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: freeze [msec]\n");
		return;
	}

	if (Cmd_Argc() == 1)
		msec = 1000;
	else
		msec = Str_ToInteger(Cmd_Argv(1));

	Sys_Sleep(msec);
}

/*
 ==================
 Com_Crash_f

 A way to force a bus error for development reasons
 ==================
*/
static void Com_Crash_f (){

	if (!com_developer || !com_developer->integerValue){
		Com_Printf("This command can only be used in developer mode\n");
		return;
	}

	*(int *)0 = 0x12345678;
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

	CVar_SetVariableInteger("paused", !com_paused->integerValue, false);
}

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
 ==================
 Com_AddStartupCommands

 Adds command line parameters as script statements.
 Commands are separated by + signs.

 Returns true if any commands were added, which will keep the default action
 from running.
 ==================
*/
static bool Com_AddStartupCommands (){

	bool	added = false;
	int		i;

	for (i = 0; i < com_argc; i++){
		if (!com_argv[i] || !com_argv[i][0])
			continue;

		if (!Str_ICompare(com_argv[i], "safe"))
			continue;

		if (Str_ICompareChars(com_argv[i], "set", 3))
			added = true;

		Cmd_AppendText(Str_VarArgs("%s\n", com_argv[i]));
	}

	return added;
}

/*
 ==================
 Com_StartupVariable

 Searches for command line parameters that are "set" commands.
 If match is not NULL, only that variable will be looked for.
 This is necessary because some variables need to be set before executing
 config files, but we want other parameters to override the settings from the
 config files.
 If once is true, the command will be removed. This is to keep it from being
 executed multiple times.
 ==================
*/
void Com_StartupVariable (const char *match, bool once){

	int		i;

	for (i = 0; i < com_argc; i++){
		if (!com_argv[i] || !com_argv[i][0])
			continue;

		if (!Str_ICompare(com_argv[i], "safe"))
			continue;

		if (Str_ICompareChars(com_argv[i], "set", 3))
			continue;

		Cmd_TokenizeString(com_argv[i]);

		if (!match || !Str_ICompare(Cmd_Argv(1), match)){
			CVar_SetVariableString(Cmd_Argv(1), Cmd_Argv(2), false);

			if (once)
				com_argv[i] = NULL;
		}
	}
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

	if (com_speeds->integerValue){
		com_timeClient -= (com_timeFrontEnd + com_timeBackEnd + com_timeSound);

		Com_Printf("frame: %i, all: %3i (w: %3i), sv: %3i, cl: %3i, rf: %3i, rb: %3i, snd: %3i\n", com_frameCount, com_timeAll, com_timeWaiting, com_timeServer, com_timeClient, com_timeFrontEnd, com_timeBackEnd, com_timeSound);
	}

	// Clear for next frame
	com_timeAll = 0;
	com_timeWaiting = 0;
	com_timeServer = 0;
	com_timeClient = 0;
	com_timeFrontEnd = 0;
	com_timeBackEnd = 0;
	com_timeSound = 0;
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
void Com_Frame (){

	int		timeAll, timeWaiting;
	int		lastFrameTime;
	int		msec, minMsec;

	// If an error occurred, drop the entire frame
	if (setjmp(com_abortFrame))
		return;

	if (com_speeds->integerValue)
		timeAll = Sys_Milliseconds();

	// Update the config file if needed
	if (CVar_GetModifiedFlags() & CVAR_ARCHIVE){
		CVar_ClearModifiedFlags(CVAR_ARCHIVE);

		Com_WriteConfigToFile(CONFIG_FILE);
	}

	// We may want to spin here if things are going too fast
	if ((com_dedicated->integerValue && com_serverRunning->integerValue) || com_timeDemo->integerValue)
		minMsec = 1;
	else
		minMsec = 1000 / com_maxFPS->integerValue;

	// Main event loop
	lastFrameTime = com_frameTime;

	while (1){
		com_frameTime = Sys_ProcessEvents();
		if (lastFrameTime > com_frameTime)
			lastFrameTime = com_frameTime;		// Time wrapped?

		msec = com_frameTime - lastFrameTime;
		if (msec >= minMsec)
			break;

		// If not running a dedicated server, sleep until time enough for a
		// frame has gone by
		if (!com_dedicated->integerValue || !com_serverRunning->integerValue){
			if (com_speeds->integerValue)
				timeWaiting = Sys_Milliseconds();

			Sys_Sleep(minMsec - msec);

			if (com_speeds->integerValue)
				com_timeWaiting += (Sys_Milliseconds() - timeWaiting);
		}
	}

	// Modify msec
	com_frameMsec = msec;

	if (!com_timeDemo->integerValue){
		if (com_fixedTime->integerValue)
			msec = com_fixedTime->integerValue;
		if (com_timeScale->floatValue != 1.0f)
			msec *= com_timeScale->floatValue;
	}

	if (msec < 1)
		msec = 1;

	// Bump frame count
	com_frameCount++;

	// Process commands
	Cmd_ExecuteBuffer();

	// Run server frame
	SV_Frame(msec);

	// Run client frame
	CL_Frame(msec);

	if (com_speeds->integerValue)
		com_timeAll += (Sys_Milliseconds() - timeAll);

	// Print collision system statistics
	CM_PrintStats();

	// Print memory manager statistics
	Mem_PrintStats();

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
 Com_Init
 ==================
*/
void Com_Init (const char *cmdLine){

	if (setjmp(com_abortFrame))
		Sys_Error("Error during initialization: %s", com_errorMessage);

	Com_Printf("%s %s (%s %s)\n", ENGINE_VERSION, BUILD_STRING, __DATE__, __TIME__);

	// Parse the command line
	Com_ParseCommandLine(cmdLine);

	// Initialize memory manager
	Mem_Init();

	// Initialize the rest of the subsystems
	Sys_Init();

	// Initialize command system
	Cmd_Init();

	// Initialize cvar system
	CVar_Init();

	// Initialize key binding/input
	Key_Init();

	// Initialize file system
	FS_Init();

	// Override the settings from the config files with command line parameters
	Com_StartupVariable(NULL, true);

	// Register variables
	com_version = CVar_Register("version", Str_VarArgs("%s (%s)", ENGINE_VERSION, __DATE__), CVAR_STRING, CVAR_SERVERINFO | CVAR_READONLY, "Game version", 0, 0);
	com_dedicated = CVar_Register("dedicated", "0", CVAR_BOOL, CVAR_INIT, "Dedicated server", 0, 0);
	com_paused = CVar_Register("paused", "0", CVAR_BOOL, CVAR_CHEAT, "Pauses the game", 0, 0);
	com_developer = CVar_Register("com_developer", "0", CVAR_BOOL, 0, "Developer mode", 0, 0);
	com_logFile = CVar_Register("com_logFile", "0", CVAR_INTEGER, 0, "Log console messages (1 = write, 2 = unbuffered write, 3 = append, 4 = unbuffered append)", 0, 4);
	com_fixedTime = CVar_Register("com_fixedTime", "0", CVAR_INTEGER, CVAR_CHEAT, "Fixed time", 0, 1000);
	com_timeScale = CVar_Register("com_timeScale", "1.0", CVAR_FLOAT, CVAR_CHEAT, "Time scale", 0.0f, 1000.0f);
	com_speeds = CVar_Register("com_speeds", "0", CVAR_BOOL, CVAR_CHEAT, "Show engine speeds", 0, 0);
	com_clientRunning = CVar_Register("com_clientRunning", "0", CVAR_BOOL, CVAR_READONLY, "Client is running", 0, 0);
	com_serverRunning = CVar_Register("com_serverRunning", "0", CVAR_BOOL, CVAR_READONLY, "Server is running", 0, 0);	
	com_timeDemo = CVar_Register("com_timeDemo", "0", CVAR_BOOL, CVAR_CHEAT, "Timing a demo", 0, 0);
	com_maxFPS = CVar_Register("com_maxFPS", "60", CVAR_INTEGER, CVAR_ARCHIVE, "Maximum framerate", 1, 125);

	// Add commands
	Cmd_AddCommand("writeConfig", Com_WriteConfig_f, "Writes a config file", NULL);
	Cmd_AddCommand("showConsole", Com_ShowConsole_f, "Shows the system console", NULL);
	Cmd_AddCommand("error", Com_Error_f, "Throws an error", NULL);
	Cmd_AddCommand("fatalError", Com_FatalError_f, "Throws a fatal error", NULL);
	Cmd_AddCommand("freeze", Com_Freeze_f, "Freezes the game", NULL);
	Cmd_AddCommand("crash", Com_Crash_f, "Crashes the game", NULL);
	Cmd_AddCommand("pause", Com_Pause_f, "Pauses the game", NULL);
	Cmd_AddCommand("quit", Com_Quit_f, "Quits the game", NULL);

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
	if (com_dedicated->integerValue)
		Sys_ShowConsole(true);
	else
		Sys_ShowConsole(false);

	com_dedicated->modified = false;

	// Add commands from the command line
	if (!Com_AddStartupCommands()){
		// If the user didn't give any commands, run default action
		if (!com_dedicated->integerValue)
			Cmd_AppendText(DEFAULT_ACTION);
	}

	// Set initial frame time, msec, and count
	com_frameTime = Sys_Milliseconds();
	com_frameMsec = 1;
	com_frameCount = 0;

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
	Cmd_RemoveCommand("writeconfig");
	Cmd_RemoveCommand("showConsole");
	Cmd_RemoveCommand("error");
	Cmd_RemoveCommand("fatalError");
	Cmd_RemoveCommand("freeze");
	Cmd_RemoveCommand("crash");
	Cmd_RemoveCommand("pause");
	Cmd_RemoveCommand("quit");

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