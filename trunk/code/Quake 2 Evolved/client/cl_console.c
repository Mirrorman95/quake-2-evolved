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
// cl_console.c - Client console
//

// TODO:
// - chat messages and console background drawing


#include "client.h"


#define CHAR_WIDTH					5
#define CHAR_HEIGHT					10

#define SPACE_SIZE					5
#define BORDER_SIZE					1

#define TOTAL_LINES					2048

#define	NOTIFY_TIMES				5

#define MAX_LINE_LENGTH				((SCREEN_WIDTH - (SPACE_SIZE * 6)) / CHAR_WIDTH)
#define MAX_INPUT_LENGTH			256

#define MAX_HISTORY_LINES			32

#define MAX_LIST_COMMANDS			8192

typedef struct {
	bool					initialized;

	float					opacity;			// Console opacity for drawing

	bool					overstrikeMode;

	// Text buffer
	short					text[TOTAL_LINES][MAX_LINE_LENGTH];
	int						pos;				// Offset in current line for next print
	int						color;				// Current color index

	bool					carriageReturn;

	int						currentLine;		// Line where next message will be printed
	int						displayLine;		// Bottom of console displays this line

	// Transparent notify lines
	int						notifyTimes[NOTIFY_TIMES];

	// Input buffer
	char					inputText[MAX_INPUT_LENGTH];
	int						inputLength;
	int						inputPos;
	int						inputSel;

	// History buffer
	char					historyText[MAX_HISTORY_LINES][MAX_INPUT_LENGTH];
	int						historyCount;
	int						historyLine;

	// Chat buffer
	bool					chatActive;
	bool					chatTeam;
	char					chatText[MAX_INPUT_LENGTH];
	int						chatPos;
	int						chatLength;

	// Command auto-completion
	char					cmdPartial[MAX_INPUT_LENGTH];
	int						cmdPartialLength;
	int						cmdPartialArg;

	char					cmdName[MAX_INPUT_LENGTH];
	char					cmdArgs[MAX_INPUT_LENGTH];
	const char *			cmdList[MAX_LIST_COMMANDS];
	int						cmdCount;
	int						cmdIndex;
	int						cmdCursor;
} console_t;

static console_t			con;

static cvar_t *				con_noPrint;
static cvar_t *				con_noNotify;
static cvar_t *				con_notifyTime;
static cvar_t *				con_speed;


/*
 ==================
 Con_ClearText
 ==================
*/
static void Con_ClearText (){

	Mem_Fill(con.text, 0, sizeof(con.text));

	con.pos = 0;

	con.currentLine = 0;
	con.displayLine = 0;
}

/*
 ==================
 Con_ClearNotify
 ==================
*/
void Con_ClearNotify (){

	Mem_Fill(con.notifyTimes, 0, sizeof(con.notifyTimes));
}

/*
 ==================
 Con_ClearInput
 ==================
*/
static void Con_ClearInput (){

	con.inputText[0] = 0;
	con.inputLength = 0;
	con.inputPos = 0;
	con.inputSel = -1;
}

/*
 ==================
 Con_ClearChat
 ==================
*/
static void Con_ClearChat (){

	con.chatActive = false;
	con.chatTeam = false;
	con.chatText[0] = 0;
	con.chatPos = 0;
	con.chatLength = 0;
}

/*
 ==================
 Con_ClearCommands
 ==================
*/
static void Con_ClearCommands (){

	int		i;

	con.cmdName[0] = 0;
	con.cmdArgs[0] = 0;

	for (i = 0; i < con.cmdCount; i++)
		Mem_Free(con.cmdList[i]);

	con.cmdCount = 0;
	con.cmdIndex = -1;
	con.cmdCursor = -1;
}

/*
 ==================
 Con_CarriageReturn
 ==================
*/
static void Con_CarriageReturn (){

	// Mark time for transparent overlay
	con.notifyTimes[con.currentLine % NOTIFY_TIMES] = cls.realTime;

	// Clear the line
	Mem_Fill(con.text[con.currentLine % TOTAL_LINES], 0, MAX_LINE_LENGTH * sizeof(short));

	con.pos = 0;
}

/*
 ==================
 Con_LineFeed
 ==================
*/
static void Con_LineFeed (){

	// Mark time for transparent overlay
	con.notifyTimes[con.currentLine % NOTIFY_TIMES] = cls.realTime;

	// Scroll down and advance
	if (con.displayLine == con.currentLine)
		con.displayLine++;

	con.currentLine++;

	if (con.displayLine < con.currentLine - TOTAL_LINES + 1)
		con.displayLine = con.currentLine - TOTAL_LINES + 1;

	// Clear the line
	Mem_Fill(con.text[con.currentLine % TOTAL_LINES], 0, MAX_LINE_LENGTH * sizeof(short));

	con.pos = 0;
}

/*
 ==================
 Con_Print

 Handles cursor positioning, line wrapping, colored text, etc
 ==================
*/
void Con_Print (const char *text){

	int		line, length;
	int		ch;

	if (!con.initialized)
		return;

	if (con_noPrint->integerValue)
		return;

	while (*text){
		if (Str_IsColor(text)){
			con.color = Str_ColorIndexForChar(text[1]);
			text += 2;
			continue;
		}

		ch = *(const byte *)text;

		line = con.currentLine % TOTAL_LINES;

		// If we are about to print a new word, check to see if we should wrap
		// to a new line
		if (ch > ' ' && (con.pos > 0 && (con.text[line][con.pos - 1] & 0xFF) <= ' ')){
			// Count word length
			for (length = 0; length < MAX_LINE_LENGTH; length++){
				if ((text[length] & 0xFF) <= ' ')
					break;
			}

			// Word wrap
			if (con.pos + length > MAX_LINE_LENGTH){
				Con_LineFeed();

				line = (line + 1) % TOTAL_LINES;
			}
		}

		// Handle carriage return
		if (con.carriageReturn){
			con.carriageReturn = false;

			if (ch != '\n')
				Con_CarriageReturn();
		}

		// Store character and advance
		switch (ch){
		case '\r':
			con.pos = 0;
			con.color = COLOR_DEFAULT;

			con.carriageReturn = true;

			break;
		case '\n':
			con.pos = 0;
			con.color = COLOR_DEFAULT;

			Con_LineFeed();

			break;
		case '\t':
			do {
				con.text[line][con.pos++] = (con.color << 8) | ' ';

				if (con.pos == MAX_LINE_LENGTH){
					Con_LineFeed();
					break;
				}
			} while (con.pos & 3);

			break;
		default:
			con.text[line][con.pos++] = (con.color << 8) | ch;

			if (con.pos == MAX_LINE_LENGTH)
				Con_LineFeed();

			break;
		}

		text++;
	}

	// Mark time for transparent overlay
	con.notifyTimes[con.currentLine % NOTIFY_TIMES] = cls.realTime;
}

/*
 ==================
 Con_Toggle

 TODO: Con_ClearChat(); ?
 ==================
*/
void Con_Toggle (){

	if (!con.initialized)
		return;

	Con_ClearNotify();
	Con_ClearInput();
	Con_ClearCommands();

	if (Key_GetKeyDest() == KEY_CONSOLE){
		if (UI_IsVisible())
			Key_SetKeyDest(KEY_MENU);
		else if (con.chatActive)
			Key_SetKeyDest(KEY_MESSAGE);
		else
			Key_SetKeyDest(KEY_GAME);
	}
	else
		Key_SetKeyDest(KEY_CONSOLE);
}

/*
 ==================
 Con_Close
 ==================
*/
void Con_Close (){

	if (!con.initialized)
		return;

	con.opacity = 0.0f;

	Con_ClearNotify();
	Con_ClearInput();
	Con_ClearChat();
	Con_ClearCommands();

	Key_SetKeyDest(KEY_GAME);
}


/*
 ==============================================================================

 CONSOLE INPUT

 ==============================================================================
*/


/*
 ==================
 Con_SortCommandList
 ==================
*/
static int Con_SortCommandList (const void *elem1, const void *elem2){

	const char	*cmd1 = *(const char **)elem1;
	const char	*cmd2 = *(const char **)elem2;

	return Str_Compare(cmd1, cmd2);
}

/*
 ==================
 Con_CompleteCommandCallback
 ==================
*/
static void Con_CompleteCommandCallback (const char *string){

	int		i;

	if (con.cmdCount == MAX_LIST_COMMANDS)
		return;		// Too many commands

	if (Str_ICompareChars(string, con.cmdPartial, con.cmdPartialLength))
		return;		// No match

	// Ignore duplicates
	for (i = 0; i < con.cmdCount; i++){
		if (!Str_ICompare(con.cmdList[i], string))
			return;		// Already in list
	}

	// Add it
	con.cmdList[con.cmdCount++] = Mem_DupString(string, TAG_COMMON);
}

/*
 ==================
 Con_CompleteCommand
 ==================
*/
static void Con_CompleteCommand (){

	cvar_t	*cvar;
	char	partial[MAX_INPUT_LENGTH];
	int		length;
	int		i;

	// Clear auto-complete list
	Con_ClearCommands();

	// Tokenize the string
	Cmd_TokenizeString(con.inputText);

	if (!Cmd_Argc())
		return;		// Nothing to search for

	// Find matching arguments if possible
	Str_Copy(con.cmdPartial, Cmd_Argv(1), sizeof(con.cmdPartial));
	con.cmdPartialLength = Str_Length(con.cmdPartial);
	con.cmdPartialArg = 1;

	Cmd_ArgumentCompletion(Con_CompleteCommandCallback);
	CVar_ArgumentCompletion(Con_CompleteCommandCallback);

	// Otherwise find matching commands and variables
	if (!con.cmdCount){
		Str_Copy(con.cmdPartial, Cmd_Argv(0), sizeof(con.cmdPartial));
		con.cmdPartialLength = Str_Length(con.cmdPartial);
		con.cmdPartialArg = 0;

		Cmd_CommandCompletion(Con_CompleteCommandCallback);
		CVar_CommandCompletion(Con_CompleteCommandCallback);
	}

	if (!con.cmdCount)
		return;		// Nothing was found

	// Copy the name and arguments if available
	if (con.cmdPartialArg)
		Str_Copy(con.cmdName, Cmd_Argv(0), sizeof(con.cmdName));

	if (con.cmdPartialArg + 1 != Cmd_Argc())
		Str_Copy(con.cmdArgs, Cmd_Args(), sizeof(con.cmdArgs));

	if (con.cmdCount == 1){
		// Only one was found, so copy it to the input line
		if (con.cmdName[0]){
			if (con.cmdArgs[0])
				Str_SPrintf(con.inputText, sizeof(con.inputText), "%s %s %s", con.cmdName, con.cmdList[0], con.cmdArgs);
			else
				Str_SPrintf(con.inputText, sizeof(con.inputText), "%s %s", con.cmdName, con.cmdList[0]);
		}
		else {
			if (con.cmdArgs[0])
				Str_SPrintf(con.inputText, sizeof(con.inputText), "%s %s", con.cmdList[0], con.cmdArgs);
			else
				Str_SPrintf(con.inputText, sizeof(con.inputText), "%s ", con.cmdList[0]);
		}

		con.inputLength = Str_Length(con.inputText);
		con.inputPos = con.inputLength;
		con.inputSel = -1;
	}
	else {
		// Sort the commands
		qsort(con.cmdList, con.cmdCount, sizeof(char *), Con_SortCommandList);

		// Print the commands
		if (con.cmdName[0])
			Com_Printf("]%s %s\n", Cmd_Argv(0), Cmd_Argv(1));
		else
			Com_Printf("]%s\n", Cmd_Argv(0));

		for (i = 0; i < con.cmdCount; i++){
			if (con.cmdName[0])
				Com_Printf("    %s %s\n", con.cmdName, con.cmdList[i]);
			else {
				cvar = CVar_FindVariable(con.cmdList[i]);
				if (cvar)
					Com_Printf("    %s = \"%s" S_COLOR_DEFAULT "\"\n", con.cmdList[i], cvar->value);
				else
					Com_Printf("    %s\n", con.cmdList[i]);
			}
		}

		if (con.cmdCount == MAX_LIST_COMMANDS)
			Com_Printf("    ...\n");

		// Find the number of matching characters between the first and last
		// commands in the list and copy them
		length = 0;

		while (1){
			if (!con.cmdList[0][length] || !con.cmdList[con.cmdCount-1][length])
				break;

			if (con.cmdList[0][length] != con.cmdList[con.cmdCount-1][length])
				break;

			partial[length] = con.cmdList[0][length];
			length++;
		}

		partial[length] = 0;

		if (con.cmdName[0])
			length += Str_Length(con.cmdName) + 1;

		// Save the cursor position
		con.cmdCursor = length;

		// Copy the match to the input line
		if (con.cmdName[0]){
			if (con.cmdArgs[0])
				Str_SPrintf(con.inputText, sizeof(con.inputText), "%s %s %s", con.cmdName, partial, con.cmdArgs);
			else
				Str_SPrintf(con.inputText, sizeof(con.inputText), "%s %s", con.cmdName, partial);
		}
		else {
			if (con.cmdArgs[0])
				Str_SPrintf(con.inputText, sizeof(con.inputText), "%s %s", partial, con.cmdArgs);
			else
				Str_SPrintf(con.inputText, sizeof(con.inputText), "%s", partial);
		}

		con.inputLength = Str_Length(con.inputText);
		con.inputPos = length;
		con.inputSel = -1;
	}
}

/*
 ==================
 Con_KeyEvent
 ==================
*/
void Con_KeyEvent (int key, bool down){

	const char	*text;
	int			length;
	int			i, j;

	if (!con.initialized)
		return;

	if (!down)
		return;		// Ignore key up events

	// Auto-complete command
	if (key == K_TAB){
		if (con.cmdCount <= 1)
			Con_CompleteCommand();
		else {
			// Cycle through the commands
			if (Key_IsDown(K_LSHIFT) || Key_IsDown(K_RSHIFT)){
				if (--con.cmdIndex < 0)
					con.cmdIndex = con.cmdCount - 1;
			}
			else {
				if (++con.cmdIndex == con.cmdCount)
					con.cmdIndex = 0;
			}

			if (con.cmdName[0]){
				if (con.cmdArgs[0])
					Str_SPrintf(con.inputText, sizeof(con.inputText), "%s %s %s", con.cmdName, con.cmdList[con.cmdIndex], con.cmdArgs);
				else
					Str_SPrintf(con.inputText, sizeof(con.inputText), "%s %s", con.cmdName, con.cmdList[con.cmdIndex]);
			}
			else {
				if (con.cmdArgs[0])
					Str_SPrintf(con.inputText, sizeof(con.inputText), "%s %s", con.cmdList[con.cmdIndex], con.cmdArgs);
				else
					Str_SPrintf(con.inputText, sizeof(con.inputText), "%s", con.cmdList[con.cmdIndex]);
			}

			con.inputLength = Str_Length(con.inputText);

			if (con.cmdName[0])
				con.inputPos = Str_Length(con.cmdName) + 1 + Str_Length(con.cmdList[con.cmdIndex]);
			else
				con.inputPos = Str_Length(con.cmdList[con.cmdIndex]);

			con.inputSel = con.cmdCursor;
		}

		return;
	}

	// Execute a command
	if (key == K_ENTER || key == K_KP_ENTER){
		if (!con.inputText[0])
			return;

		Com_Printf("]%s\n", con.inputText);

		// Add the command text
		Cmd_AppendText(con.inputText);

		// Add to history
		Str_Copy(con.historyText[con.historyCount % MAX_HISTORY_LINES], con.inputText, sizeof(con.historyText[con.historyCount % MAX_HISTORY_LINES]));
		con.historyCount++;
		con.historyLine = con.historyCount;

		// Clear input
		Con_ClearInput();

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Select previous history line
	if (key == K_UPARROW){
		if (con.historyLine > 0 && con.historyCount - con.historyLine < MAX_HISTORY_LINES)
			con.historyLine--;

		Str_Copy(con.inputText, con.historyText[con.historyLine % MAX_HISTORY_LINES], sizeof(con.inputText));
		con.inputLength = Str_Length(con.inputText);
		con.inputPos = con.inputLength;
		con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Select next history line
	if (key == K_DOWNARROW){
		if (con.historyLine == con.historyCount)
			return;

		con.historyLine++;

		if (con.historyLine == con.historyCount)
			Con_ClearInput();
		else {
			Str_Copy(con.inputText, con.historyText[con.historyLine % MAX_HISTORY_LINES], sizeof(con.inputText));
			con.inputLength = Str_Length(con.inputText);
			con.inputPos = con.inputLength;
			con.inputSel = -1;
		}

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Scroll up a few lines
	if (key == K_PAGEUP || key == K_MWHEELUP){
		if (con.pos == 0 && con.displayLine == con.currentLine)
			con.displayLine--;

		if (Key_IsDown(K_LCTRL) || Key_IsDown(K_RCTRL))
			con.displayLine -= 6;
		else
			con.displayLine -= 2;

		if (con.displayLine < 0)
			con.displayLine = 0;

		if (con.displayLine < con.currentLine - TOTAL_LINES + 1)
			con.displayLine = con.currentLine - TOTAL_LINES + 1;

		return;
	}

	// Scroll down a few lines
	if (key == K_PAGEDOWN || key == K_MWHEELDOWN){
		if (Key_IsDown(K_LCTRL) || Key_IsDown(K_RCTRL))
			con.displayLine += 6;
		else
			con.displayLine += 2;

		if (con.displayLine > con.currentLine)
			con.displayLine = con.currentLine;

		return;
	}

	// Scroll up to the top
	if (key == K_HOME && (Key_IsDown(K_LCTRL) || Key_IsDown(K_RCTRL))){
		con.displayLine = 0;

		if (con.displayLine < con.currentLine - TOTAL_LINES + 1)
			con.displayLine = con.currentLine - TOTAL_LINES + 1;

		return;
	}

	// Scroll down to the bottom
	if (key == K_END && (Key_IsDown(K_LCTRL) || Key_IsDown(K_RCTRL))){
		con.displayLine = con.currentLine;

		return;
	}

	// Select text from first character to selection cursor
	if (key == K_HOME && (Key_IsDown(K_LSHIFT) || Key_IsDown(K_RSHIFT))){
		if (con.inputPos && con.inputSel == -1)
			con.inputSel = con.inputPos;

		con.inputPos = 0;

		if (con.inputPos == con.inputSel)
			con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Select text from selection cursor to last character
	if (key == K_END && (Key_IsDown(K_LSHIFT) || Key_IsDown(K_RSHIFT))){
		if (con.inputPos != con.inputLength && con.inputSel == -1)
			con.inputSel = con.inputPos;

		con.inputPos = con.inputLength;

		if (con.inputPos == con.inputSel)
			con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Select text from previous word to selection cursor
	if (key == K_LEFTARROW && (Key_IsDown(K_LCTRL) || Key_IsDown(K_RCTRL)) && (Key_IsDown(K_LSHIFT) || Key_IsDown(K_RSHIFT))){
		if (con.inputPos && con.inputSel == -1)
			con.inputSel = con.inputPos;

		for (i = con.inputPos - 1; i >= 0; i--){
			if (con.inputText[i] != ' ')
				break;
		}

		for ( ; i >= 0; i--){
			if (con.inputText[i] == ' ')
				break;
		}

		con.inputPos = i + 1;

		if (con.inputPos == con.inputSel)
			con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Select text from selection cursor to next word
	if (key == K_RIGHTARROW && (Key_IsDown(K_LCTRL) || Key_IsDown(K_RCTRL)) && (Key_IsDown(K_LSHIFT) || Key_IsDown(K_RSHIFT))){
		if (con.inputPos != con.inputLength && con.inputSel == -1)
			con.inputSel = con.inputPos;

		for (i = con.inputPos; i < con.inputLength; i++){
			if (con.inputText[i] == ' ')
				break;
		}

		for ( ; i < con.inputLength; i++){
			if (con.inputText[i] != ' ')
				break;
		}

		con.inputPos = i;

		if (con.inputPos == con.inputSel)
			con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Select text from previous character to selection cursor
	if (key == K_LEFTARROW && (Key_IsDown(K_LSHIFT) || Key_IsDown(K_RSHIFT))){
		if (con.inputPos && con.inputSel == -1)
			con.inputSel = con.inputPos;

		if (con.inputPos)
			con.inputPos--;

		if (con.inputPos == con.inputSel)
			con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Select text from selection cursor to next character
	if (key == K_RIGHTARROW && (Key_IsDown(K_LSHIFT) || Key_IsDown(K_RSHIFT))){
		if (con.inputPos != con.inputLength && con.inputSel == -1)
			con.inputSel = con.inputPos;

		if (con.inputPos < con.inputLength)
			con.inputPos++;

		if (con.inputPos == con.inputSel)
			con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Undo selection
	if (key == K_ESCAPE){
		con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Move cursor to first character
	if (key == K_HOME){
		con.inputPos = 0;
		con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Move cursor to last character
	if (key == K_END){
		con.inputPos = con.inputLength;
		con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Move cursor to previous word
	if (key == K_LEFTARROW && (Key_IsDown(K_LCTRL) || Key_IsDown(K_RCTRL))){
		for (i = con.inputPos - 1; i >= 0; i--){
			if (con.inputText[i] != ' ')
				break;
		}

		for ( ; i >= 0; i--){
			if (con.inputText[i] == ' ')
				break;
		}

		con.inputPos = i + 1;
		con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Move cursor to next word
	if (key == K_RIGHTARROW && (Key_IsDown(K_LCTRL) || Key_IsDown(K_RCTRL))){
		for (i = con.inputPos; i < con.inputLength; i++){
			if (con.inputText[i] == ' ')
				break;
		}

		for ( ; i < con.inputLength; i++){
			if (con.inputText[i] != ' ')
				break;
		}

		con.inputPos = i;
		con.inputSel = -1;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Move cursor to previous character
	if (key == K_LEFTARROW){
		if (con.inputSel == -1){
			if (con.inputPos)
				con.inputPos--;
		}
		else {
			if (con.inputPos > con.inputSel)
				con.inputPos = con.inputSel;

			con.inputSel = -1;
		}

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Move cursor to next character
	if (key == K_RIGHTARROW){
		if (con.inputSel == -1){
			if (con.inputPos < con.inputLength)
				con.inputPos++;
		}
		else {
			if (con.inputPos < con.inputSel)
				con.inputPos = con.inputSel;

			con.inputSel = -1;
		}

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Delete previous character or selected text
	if (key == K_BACKSPACE){
		if (con.inputSel == -1){
			if (!con.inputPos)
				return;

			con.inputPos--;

			for (i = con.inputPos; i < con.inputLength; i++)
				con.inputText[i] = con.inputText[i+1];

			con.inputLength--;
		}
		else {
			if (con.inputPos < con.inputSel)
				length = con.inputSel - con.inputPos;
			else {
				length = con.inputPos - con.inputSel;

				con.inputPos = con.inputSel;
			}

			con.inputLength -= length;
			con.inputSel = -1;

			for (i = con.inputPos; i <= con.inputLength; i++)
				con.inputText[i] = con.inputText[i+length];
		}

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Delete next character or selected text
	if (key == K_DELETE){
		if (con.inputSel == -1){
			if (con.inputPos == con.inputLength)
				return;

			for (i = con.inputPos; i < con.inputLength; i++)
				con.inputText[i] = con.inputText[i+1];

			con.inputLength--;
		}
		else {
			if (con.inputPos < con.inputSel)
				length = con.inputSel - con.inputPos;
			else {
				length = con.inputPos - con.inputSel;

				con.inputPos = con.inputSel;
			}

			con.inputLength -= length;
			con.inputSel = -1;

			for (i = con.inputPos; i <= con.inputLength; i++)
				con.inputText[i] = con.inputText[i+length];
		}

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Delete selected text (if any) and paste text from clipboard
	if (key == K_INSERT && (Key_IsDown(K_LSHIFT) || Key_IsDown(K_RSHIFT))){
		text = Sys_GetClipboardText();
		if (!text)
			return;

		if (con.inputSel != -1){
			if (con.inputPos < con.inputSel)
				length = con.inputSel - con.inputPos;
			else {
				length = con.inputPos - con.inputSel;

				con.inputPos = con.inputSel;
			}

			con.inputLength -= length;
			con.inputSel = -1;

			for (i = con.inputPos; i <= con.inputLength; i++)
				con.inputText[i] = con.inputText[i+length];
		}
		else if (con.overstrikeMode){
			if (con.inputPos != con.inputLength){
				for (i = con.inputPos; i < con.inputLength; i++)
					con.inputText[i] = con.inputText[i+1];

				con.inputLength--;
			}
		}

		for (j = 0; text[j]; j++){
			if (!Str_CharIsPrintable(text[j]))
				continue;

			if (con.inputLength == MAX_INPUT_LENGTH - 1)
				break;

			for (i = con.inputLength; i > con.inputPos; i--)
				con.inputText[i] = con.inputText[i-1];

			con.inputLength++;

			con.inputText[con.inputPos++] = text[j];
			con.inputText[con.inputLength] = 0;
		}

		Mem_Free(text);

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Toggle overstrike mode
	if (key == K_INSERT){
		con.overstrikeMode = !con.overstrikeMode;

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}
}

/*
 ==================
 Con_CharEvent
 ==================
*/
void Con_CharEvent (int ch){

	char		string[MAX_INPUT_LENGTH];
	const char	*text;
	int			length;
	int			i, j;

	if (!con.initialized)
		return;

	// Clear text buffer
	if (ch == 'l' - 'a' + 1){
		Con_ClearText();
		return;
	}

	// Copy selected text to clipboard and delete it
	if (ch == 'x' - 'a' + 1){
		if (con.inputSel == -1)
			return;

		if (con.inputPos < con.inputSel){
			length = con.inputSel - con.inputPos;

			Str_Copy(string, &con.inputText[con.inputPos], length + 1);
		}
		else {
			length = con.inputPos - con.inputSel;

			Str_Copy(string, &con.inputText[con.inputSel], length + 1);

			con.inputPos = con.inputSel;
		}

		Sys_SetClipboardText(string);

		con.inputLength -= length;
		con.inputSel = -1;

		for (i = con.inputPos; i <= con.inputLength; i++)
			con.inputText[i] = con.inputText[i+length];

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Copy selected text to clipboard
	if (ch == 'c' - 'a' + 1){
		if (con.inputSel == -1)
			return;

		if (con.inputPos < con.inputSel){
			length = con.inputSel - con.inputPos;

			Str_Copy(string, &con.inputText[con.inputPos], length + 1);
		}
		else {
			length = con.inputPos - con.inputSel;

			Str_Copy(string, &con.inputText[con.inputSel], length + 1);
		}

		Sys_SetClipboardText(string);

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Delete selected text (if any) and paste text from clipboard
	if (ch == 'v' - 'a' + 1){
		text = Sys_GetClipboardText();
		if (!text)
			return;

		if (con.inputSel != -1){
			if (con.inputPos < con.inputSel)
				length = con.inputSel - con.inputPos;
			else {
				length = con.inputPos - con.inputSel;

				con.inputPos = con.inputSel;
			}

			con.inputLength -= length;
			con.inputSel = -1;

			for (i = con.inputPos; i <= con.inputLength; i++)
				con.inputText[i] = con.inputText[i+length];
		}
		else if (con.overstrikeMode){
			if (con.inputPos != con.inputLength){
				for (i = con.inputPos; i < con.inputLength; i++)
					con.inputText[i] = con.inputText[i+1];

				con.inputLength--;
			}
		}

		for (j = 0; text[j]; j++){
			if (!Str_CharIsPrintable(text[j]))
				continue;

			if (con.inputLength == MAX_INPUT_LENGTH - 1)
				break;

			for (i = con.inputLength; i > con.inputPos; i--)
				con.inputText[i] = con.inputText[i-1];

			con.inputLength++;

			con.inputText[con.inputPos++] = text[j];
			con.inputText[con.inputLength] = 0;
		}

		Mem_Free(text);

		// Clear auto-complete list
		Con_ClearCommands();

		return;
	}

	// Delete selected text (if any) and insert or append new character
	if (!Str_CharIsPrintable(ch))
		return;

	if (con.inputSel != -1){
		if (con.inputPos < con.inputSel)
			length = con.inputSel - con.inputPos;
		else {
			length = con.inputPos - con.inputSel;

			con.inputPos = con.inputSel;
		}

		con.inputLength -= length;
		con.inputSel = -1;

		for (i = con.inputPos; i <= con.inputLength; i++)
			con.inputText[i] = con.inputText[i+length];
	}
	else if (con.overstrikeMode){
		if (con.inputPos != con.inputLength){
			for (i = con.inputPos; i < con.inputLength; i++)
				con.inputText[i] = con.inputText[i+1];

			con.inputLength--;
		}
	}

	if (con.inputLength == MAX_INPUT_LENGTH - 1)
		return;

	for (i = con.inputLength; i > con.inputPos; i--)
		con.inputText[i] = con.inputText[i-1];

	con.inputLength++;

	con.inputText[con.inputPos++] = ch;
	con.inputText[con.inputLength] = 0;

	// Clear auto-complete list
	Con_ClearCommands();
}

/*
 ==================
 
 ==================
*/
void Con_KeyMessageEvent (int key, bool down){

	if (!con.initialized)
		return;

	if (!down)
		return;		// Ignore key up events
}

/*
 ==================
 
 ==================
*/
void Con_CharMessageEvent (int ch){

	if (!con.initialized)
		return;
}


/*
 ==============================================================================

 CONSOLE DRAWING

 ==============================================================================
*/

#define OUTPUT_TEXT_BOX_X			(SPACE_SIZE)
#define OUTPUT_TEXT_BOX_Y			(SPACE_SIZE)
#define OUTPUT_TEXT_BOX_W			(SCREEN_WIDTH - (SPACE_SIZE << 1))
#define OUTPUT_TEXT_BOX_H			((SCREEN_HEIGHT >> 1) - SPACE_SIZE - (CHAR_HEIGHT + (SPACE_SIZE << 1)) - (SPACE_SIZE << 1))

#define INPUT_TEXT_BOX_X			(SPACE_SIZE)
#define INPUT_TEXT_BOX_Y			((SCREEN_HEIGHT >> 1) - SPACE_SIZE - (CHAR_HEIGHT + (SPACE_SIZE << 1)))
#define INPUT_TEXT_BOX_W			(SCREEN_WIDTH - (SPACE_SIZE << 1))
#define INPUT_TEXT_BOX_H			(CHAR_HEIGHT + (SPACE_SIZE << 1))

#define SCROLL_BAR_X				(SCREEN_WIDTH - (SPACE_SIZE + (SPACE_SIZE << 1)))
#define SCROLL_BAR_Y				(SPACE_SIZE + (CHAR_HEIGHT + (SPACE_SIZE << 1)))
#define SCROLL_BAR_W				(SPACE_SIZE)
#define SCROLL_BAR_H				((SCREEN_HEIGHT >> 1) - ((CHAR_HEIGHT + (SPACE_SIZE << 1)) << 1) - (SPACE_SIZE << 2))

#define NOTIFY_TEXT_X				(SPACE_SIZE)
#define NOTIFY_TEXT_Y				(SPACE_SIZE)

#define OUTPUT_TEXT_X				(OUTPUT_TEXT_BOX_X + SPACE_SIZE)
#define OUTPUT_TEXT_Y				(OUTPUT_TEXT_BOX_Y + SPACE_SIZE + CHAR_HEIGHT + SPACE_SIZE)

#define INPUT_TEXT_X				(INPUT_TEXT_BOX_X + SPACE_SIZE)
#define INPUT_TEXT_Y				(INPUT_TEXT_BOX_Y + SPACE_SIZE)

#define VERSION_STRING_X			(OUTPUT_TEXT_BOX_X)
#define VERSION_STRING_Y			(OUTPUT_TEXT_BOX_Y + SPACE_SIZE)

#define OUTPUT_TEXT_LINES			(((SCREEN_HEIGHT >> 1) - ((CHAR_HEIGHT + (SPACE_SIZE << 1)) << 1) - (SPACE_SIZE << 2)) / CHAR_HEIGHT)

#define INPUT_TEXT_LENGTH			((SCREEN_WIDTH - (SPACE_SIZE << 2)) / CHAR_WIDTH)


/*
 ==================
 Con_DrawTextBox

 TODO: do something like the old console?
 ==================
*/
static void Con_DrawTextBox (int x, int y, int w, int h, bool input){

	// Draw the background
	R_SetColor4(0.10f, 0.18f, 0.21f, 0.75f * con.opacity);

	if (input)
		R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_STRETCH_WIDTH, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);
	else
		R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_STRETCH_WIDTH, 1.0f, V_STRETCH_TOP, 1.0f, cls.media.whiteMaterial);

	// Draw the border
	R_SetColor4(0.36f, 0.41f, 0.44f, 0.5f * con.opacity);

	if (input){
		R_DrawStretchPic(x, y, w, BORDER_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, H_STRETCH_WIDTH, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);
		R_DrawStretchPic(x, y + h - BORDER_SIZE, w, BORDER_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, H_STRETCH_WIDTH, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);
		R_DrawStretchPic(x, y + BORDER_SIZE, BORDER_SIZE, h - (BORDER_SIZE << 1), 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);
		R_DrawStretchPic(x + w - BORDER_SIZE, y + BORDER_SIZE, BORDER_SIZE, h - (BORDER_SIZE << 1), 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_RIGHT, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);
	}
	else {
		R_DrawStretchPic(x, y, w, BORDER_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, H_STRETCH_WIDTH, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.whiteMaterial);
		R_DrawStretchPic(x, y + h - BORDER_SIZE, w, BORDER_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, H_STRETCH_WIDTH, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);
		R_DrawStretchPic(x, y + BORDER_SIZE, BORDER_SIZE, h - (BORDER_SIZE << 1), 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_STRETCH_TOP, 1.0f, cls.media.whiteMaterial);
		R_DrawStretchPic(x + w - BORDER_SIZE, y + BORDER_SIZE, BORDER_SIZE, h - (BORDER_SIZE << 1), 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_RIGHT, 1.0f, V_STRETCH_TOP, 1.0f, cls.media.whiteMaterial);
	}
}

/*
 ==================
 Con_DrawScrollBar
 ==================
*/
static void Con_DrawScrollBar (int x, int y, int w, int h){

	float	fraction;
	int		size, offset;
	int		totalLines;

	// Draw the border
	R_SetColor4(0.36f, 0.41f, 0.44f, 0.5f * con.opacity);

	R_DrawStretchPic(x, y, w, BORDER_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_RIGHT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.whiteMaterial);
	R_DrawStretchPic(x, y + h - BORDER_SIZE, w, BORDER_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_RIGHT, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);
	R_DrawStretchPic(x, y + BORDER_SIZE, BORDER_SIZE, h - (BORDER_SIZE << 1), 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_RIGHT, 1.0f, V_STRETCH_TOP, 1.0f, cls.media.whiteMaterial);
	R_DrawStretchPic(x + w - BORDER_SIZE, y + BORDER_SIZE, BORDER_SIZE, h - (BORDER_SIZE << 1), 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_RIGHT, 1.0f, V_STRETCH_TOP, 1.0f, cls.media.whiteMaterial);

	// Calculate scroll bar size and offset
	if (con.currentLine == 0){
		size = h;

		offset = 0;
	}
	else {
		totalLines = con.currentLine;
		if (totalLines > TOTAL_LINES)
			totalLines = TOTAL_LINES;

		fraction = 1.0f - ((float)totalLines / TOTAL_LINES);
		size = FloatToInt(h * (0.05f + 0.95f * fraction));

		fraction = 1.0f - ((float)(con.currentLine - con.displayLine) / totalLines);
		offset = FloatToInt((h - size) * fraction);
	}

	// Draw the scroll bar
	R_SetColor4(colorDefault[0], colorDefault[1], colorDefault[2], con.opacity);

	R_DrawStretchPic(x + BORDER_SIZE, y + BORDER_SIZE + offset, w - (BORDER_SIZE << 1), size - (BORDER_SIZE << 1), 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_RIGHT, 1.0f, V_STRETCH_TOP, 1.0f, cls.media.whiteMaterial);
}

/*
 ==================
 Con_DrawNotify

 Draws the last few lines of output transparently on the screen
 ==================
*/
static void Con_DrawNotify (){

	const short	*text;
	int			color, oldColor;
	int			time;
	int			skip;
	int			c, x, y;
	int			i, j;

	if (con_noNotify->integerValue)
		return;

	y = NOTIFY_TEXT_Y;

	color = COLOR_DEFAULT;
	oldColor = COLOR_DEFAULT;

	R_SetColor(color_table[color]);

	for (i = con.currentLine - NOTIFY_TIMES + 1; i <= con.currentLine; i++){
		if (i < 0)
			continue;

		time = con.notifyTimes[i % NOTIFY_TIMES];
		if (time == 0)
			continue;

		if (cls.realTime - time > SEC2MS(con_notifyTime->floatValue))
			continue;

		// Draw the text
		text = con.text[i % TOTAL_LINES];

		for (j = 0, x = NOTIFY_TEXT_X; j < MAX_LINE_LENGTH; j++, x += CHAR_WIDTH){
			c = text[j] & 0xFF;
			if (c == 0)
				break;

			if (c == ' ')
				continue;

			color = (text[j] >> 8) & COLOR_MASK;

			if (color != oldColor){
				oldColor = color;

				R_SetColor(color_table[color]);
			}

			R_DrawChar(x, y, CHAR_WIDTH, CHAR_HEIGHT, c, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
		}

		y += CHAR_HEIGHT;
	}

	// Draw chat messages
	if (Key_GetKeyDest() == KEY_MESSAGE){
		if (con.chatTeam){
			R_DrawString(8.0f, y, CHAR_WIDTH, CHAR_HEIGHT, "say_team: ", colorWhite, false, 0.0f, 0.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			skip = 11;
		}
		else {
			R_DrawString(8.0f, y, CHAR_WIDTH, CHAR_HEIGHT, "say: ", colorWhite, false, 0.0f, 0.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			skip = 6;
		}

		// TODO: draw the text
	}
}

/*
 ==================
 Con_DrawInput

 The input line scrolls horizontally if typing goes beyond the right edge
 ==================
*/
static void Con_DrawInput (){

	const char	*text;
	int			color, oldColor;
	int			cursor, selection;
	int			length, scroll;
	int			i;

	// Find the cursor position
	cursor = con.inputPos;

	for (i = 0; i < con.inputPos; i++){
		if (!Str_IsColor(&con.inputText[i]))
			continue;

		if (i == con.inputPos - 1)
			cursor -= 1;
		else
			cursor -= 2;

		i++;
	}

	// Find the selection cursor position
	selection = con.inputSel;

	for (i = 0; i < con.inputSel; i++){
		if (!Str_IsColor(&con.inputText[i]))
			continue;

		if (i == con.inputSel - 1)
			selection -= 1;
		else
			selection -= 2;

		i++;
	}

	// Prestep if horizontally scrolling
	text = con.inputText;
	length = INPUT_TEXT_LENGTH - 2;

	color = COLOR_WHITE;

	if (cursor > length){
		scroll = cursor - length;

		cursor -= scroll;
		selection -= scroll;

		while (*text && scroll){
			if (Str_IsColor(text)){
				color = Str_ColorIndexForChar(text[1]);
				text += 2;
				continue;
			}

			text++;
			scroll--;
		}
	}

	if (con.inputPos != con.inputLength)
		length++;

	oldColor = color;

	// Draw the text box
	Con_DrawTextBox(INPUT_TEXT_BOX_X, INPUT_TEXT_BOX_Y, INPUT_TEXT_BOX_W, INPUT_TEXT_BOX_H, true);

	// Draw the prompt
	R_SetColor4(0.36f, 0.41f, 0.44f, con.opacity);
	R_DrawChar(INPUT_TEXT_X, INPUT_TEXT_Y, CHAR_WIDTH, CHAR_HEIGHT, ']', H_ALIGN_LEFT, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.charsetMaterial);

	// Draw the selection if needed
	if (con.inputSel != -1){
		if (selection > length)
			selection = length;

		if (cursor < selection)
			R_DrawStretchPic(INPUT_TEXT_X + (cursor + 1) * CHAR_WIDTH, INPUT_TEXT_Y, (selection - cursor) * CHAR_WIDTH, CHAR_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);
		else
			R_DrawStretchPic(INPUT_TEXT_X + (selection + 1) * CHAR_WIDTH, INPUT_TEXT_Y, (cursor - selection) * CHAR_WIDTH, CHAR_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);
	}

	// Draw the text
	i = 0;

	R_SetColor4(color_table[color][0], color_table[color][1], color_table[color][2], con.opacity);

	while (*text && i < length){
		if (Str_IsColor(text)){
			color = Str_ColorIndexForChar(text[1]);

			if (color != oldColor){
				oldColor = color;

				R_SetColor4(color_table[color][0], color_table[color][1], color_table[color][2], con.opacity);
			}

			text += 2;
			continue;
		}

		R_DrawChar(INPUT_TEXT_X + (i + 1) * CHAR_WIDTH, INPUT_TEXT_Y, CHAR_WIDTH, CHAR_HEIGHT, *text, H_ALIGN_LEFT, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.charsetMaterial);

		text++;
		i++;
	}

	// Draw the cursor if needed
	if ((cls.realTime >> 9) & 1)
		return;

	if (con.overstrikeMode && con.inputPos != con.inputLength && con.inputSel == -1){
		R_SetColor2(1.0f, 0.5f * con.opacity);
		R_DrawStretchPic(INPUT_TEXT_X + (cursor + 1) * CHAR_WIDTH, INPUT_TEXT_Y, CHAR_WIDTH, CHAR_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);

		return;
	}

	R_SetColor2(1.0f, con.opacity);
	R_DrawStretchPic(INPUT_TEXT_X + (cursor + 1) * CHAR_WIDTH, INPUT_TEXT_Y + CHAR_HEIGHT - BORDER_SIZE, CHAR_WIDTH, BORDER_SIZE, 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.whiteMaterial);
}

/*
 ==================
 Con_Redraw
 ==================
*/
void Con_Redraw (){

	vec4_t		textColor = {0.36f, 0.41f, 0.44f, con.opacity};
	const short	*text;
	float		opacity;
	int			color, oldColor;
	int			length, line;
	int			c, x, y;
	int			i, j;

	if (!con.initialized)
		return;

	// Decide on the opacity of the console
	if (Key_GetKeyDest() == KEY_CONSOLE)
		opacity = 1.0f;
	else
		opacity = 0.0f;

	if (opacity < con.opacity){
		con.opacity -= con_speed->floatValue * cls.frameTime;
		if (con.opacity < opacity)
			con.opacity = opacity;
	}
	else if (opacity > con.opacity){
		con.opacity += con_speed->floatValue * cls.frameTime;
		if (con.opacity > opacity)
			con.opacity = opacity;
	}

	// If not visible, draw notify lines
	if (!con.opacity){
		Con_DrawNotify();
		return;
	}

	// Draw the text box
	Con_DrawTextBox(OUTPUT_TEXT_BOX_X, OUTPUT_TEXT_BOX_Y, OUTPUT_TEXT_BOX_W, OUTPUT_TEXT_BOX_H, false);

	// Draw the version string
	length = Str_Length(com_version->value);

	x = VERSION_STRING_X + ((OUTPUT_TEXT_BOX_W - length * CHAR_WIDTH) >> 1);
	y = VERSION_STRING_Y;

	R_DrawString(x, y, CHAR_WIDTH, CHAR_HEIGHT, com_version->value, textColor, true, 0.0f, 0.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);

	// Draw the scroll bar
	Con_DrawScrollBar(SCROLL_BAR_X, SCROLL_BAR_Y, SCROLL_BAR_W, SCROLL_BAR_H);

	// Draw the text from the top down
	if (con.pos != 0 || con.displayLine != con.currentLine)
		line = con.displayLine;
	else
		line = con.displayLine - 1;

	color = COLOR_DEFAULT;
	oldColor = COLOR_DEFAULT;

	R_SetColor4(color_table[color][0], color_table[color][1], color_table[color][2], con.opacity);

	for (i = line - OUTPUT_TEXT_LINES + 1, y = OUTPUT_TEXT_Y; i <= line; i++, y += CHAR_HEIGHT){
		if (i < 0 || i < con.currentLine - TOTAL_LINES + 1)
			continue;

		// Draw the text
		text = con.text[i % TOTAL_LINES];

		for (j = 0, x = OUTPUT_TEXT_X; j < MAX_LINE_LENGTH; j++, x += CHAR_WIDTH){
			c = text[j] & 0xFF;
			if (c == 0)
				break;

			if (c == ' ')
				continue;

			color = (text[j] >> 8) & COLOR_MASK;

			if (color != oldColor){
				oldColor = color;

				R_SetColor4(color_table[color][0], color_table[color][1], color_table[color][2], con.opacity);
			}

			R_DrawChar(x, y, CHAR_WIDTH, CHAR_HEIGHT, c, H_ALIGN_LEFT, 1.0f, V_ALIGN_CENTER, 1.0f, cls.media.charsetMaterial);
		}
	}

	// Draw the input prompt, selection, text, and cursor if needed
	Con_DrawInput();
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 Con_Clear_f
 ==================
*/
static void Con_Clear_f (){

	Con_ClearText();
}

/*
 ==================
 Con_Dump_f
 ==================
*/
static void Con_Dump_f (){

	fileHandle_t	f;
	char			name[MAX_PATH_LENGTH];
	char			line[MAX_LINE_LENGTH + 1];
	short			*text;
	int				i, length;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: conDump <fileName>\n");
		return;
	}

	Str_Copy(name, Cmd_Argv(1), sizeof(name));
	Str_DefaultFileExtension(name, sizeof(name), ".txt");

	FS_OpenFile(name, FS_WRITE, &f);
	if (!f){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}

	// Skip empty lines
	i = con.currentLine - TOTAL_LINES + 1;
	if (i < 0)
		i = 0;

	for ( ; i <= con.currentLine; i++){
		if (con.text[i % TOTAL_LINES][0] & 0xFF)
			break;
	}

	// Write the remaining lines
	for ( ; i <= con.currentLine; i++){
		text = con.text[i % TOTAL_LINES];

		for (length = 0; length < MAX_LINE_LENGTH; length++){
			line[length] = text[length] & 0xFF;

			if (!line[length])
				break;
		}

		line[length] = 0;

		FS_Printf(f, "%s" NEWLINE, line);
	}

	FS_CloseFile(f);

	Com_Printf("Dumped console text to %s\n", name);
}

/*
 ==================
 Con_MessageMode_f
 ==================
*/
static void Con_MessageMode_f (){

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || cl.demoPlayback)
		return;

	con.chatActive = true;
	con.chatTeam = false;

	Key_SetKeyDest(KEY_MESSAGE);
}

/*
 ==================
 Con_MessageMode2_f
 ==================
*/
static void Con_MessageMode2_f (){

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || cl.demoPlayback)
		return;

	con.chatActive = true;
	con.chatTeam = true;

	Key_SetKeyDest(KEY_MESSAGE);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 Con_Init
 ==================
*/
void Con_Init (){

	// Register variables
	con_noPrint = CVar_Register("con_noPrint", "0", CVAR_BOOL, 0, "Don't print console messages", 0, 0);
	con_noNotify = CVar_Register("con_noNotify", "1", CVAR_BOOL, 0, "Don't print notify messages", 0, 0);
	con_notifyTime = CVar_Register("con_notifyTime", "3.0", CVAR_FLOAT, 0, "Time in seconds to display notify messages on-screen", 1.0f, 60.0f);
	con_speed = CVar_Register("con_speed", "3.0", CVAR_FLOAT, 0, "Speed at which the console is faded in/out", 0.1f, 10.0f);

	// Add commands
	Cmd_AddCommand("clear", Con_Clear_f, "Clears the console text", NULL);
	Cmd_AddCommand("conDump", Con_Dump_f, "Dumps the console text to a file", NULL);
	Cmd_AddCommand("messageMode", Con_MessageMode_f, "Type a chat message", NULL);
	Cmd_AddCommand("messageMode2", Con_MessageMode2_f, "Type a team chat message", NULL);

	// Set up the current color index
	con.color = COLOR_DEFAULT;

	// Clear text, notify lines, input, and auto-complete list
	Con_ClearText();
	Con_ClearNotify();
	Con_ClearInput();
	Con_ClearCommands();

	con.initialized = true;

	Com_Printf("Console Initialized\n");
}

/*
 ==================
 Con_Shutdown
 ==================
*/
void Con_Shutdown (){

	int		i;

	if (!con.initialized)
		return;

	// Remove commands
	Cmd_RemoveCommand("clear");
	Cmd_RemoveCommand("conDump");
	Cmd_RemoveCommand("messageMode");
	Cmd_RemoveCommand("messageMode2");

	// Free the auto-complete list
	for (i = 0; i < con.cmdCount; i++)
		Mem_Free(con.cmdList[i]);

	Mem_Fill(&con, 0, sizeof(console_t));
}