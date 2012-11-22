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
// win_console.c - System console
//

// TODO:
// - background color
// - Cmd_Args(sys_console.cmdPartialArg + 1, -1, false) in Sys_CompleteCommand


#include "../common/common.h"
#include "win_local.h"


#define CONSOLE_WINDOW_NAME			ENGINE_NAME " Console"
#define CONSOLE_WINDOW_CLASS		ENGINE_NAME " Console"
#define CONSOLE_WINDOW_STYLE		(WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

#define CONSOLE_NOTIFY_MESSAGE		(WM_USER + 0)

#define MAX_OUTPUT_LENGTH			65536
#define MAX_INPUT_LENGTH			256

#define MAX_HISTORY_LINES			32

#define MAX_LIST_COMMANDS			8192

typedef struct {
	bool					initialized;

	int						outputLength;					// To keep track of output buffer length

	bool					onError;						// If true, we're on a fatal error
	bool					flashError;						// If true, flash error message

	// History buffer
	char					historyText[MAX_HISTORY_LINES][MAX_INPUT_LENGTH];
	int						historyCount;
	int						historyLine;

	// Command auto-completion
	char					cmdPartial[MAX_INPUT_LENGTH];
	int						cmdPartialLen;
	int						cmdPartialArg;

	char					cmdName[MAX_INPUT_LENGTH];
	char					cmdArgs[MAX_INPUT_LENGTH];
	const char *			cmdList[MAX_LIST_COMMANDS];
	int						cmdCount;
	int						cmdIndex;
	int						cmdCursor;

	// Window stuff
	HWND					hWnd;
	HWND					hWndError;
	HWND					hWndOutput;
	HWND					hWndInput;
	HWND					hWndCopy;
	HWND					hWndClear;
	HWND					hWndQuit;

	HFONT					hFont1;
	HFONT					hFont2;

	HBRUSH					hBrush1;
	HBRUSH					hBrush2;

	WNDPROC					defInputProc;

	NOTIFYICONDATA			notifyIcon;
} sysConsole_t;

static sysConsole_t			sys_console;


/*
 ==============================================================================

 COMMAND AUTO-COMPLETION

 ==============================================================================
*/


/*
 ==================
 Sys_ClearCommands
 ==================
*/
static void Sys_ClearCommands (){

	int		i;

	sys_console.cmdName[0] = 0;
	sys_console.cmdArgs[0] = 0;

	for (i = 0; i < sys_console.cmdCount; i++)
		Mem_Free(sys_console.cmdList[i]);

	sys_console.cmdCount = 0;
	sys_console.cmdIndex = -1;
	sys_console.cmdCursor = -1;
}

/*
 ==================
 Sys_SortCommandList
 ==================
*/
static int Sys_SortCommandList (const void *elem1, const void *elem2){

	const char	*cmd1 = *(const char **)elem1;
	const char	*cmd2 = *(const char **)elem2;

	return Str_Compare(cmd1, cmd2);
}

/*
 ==================
 Sys_CompleteCommandCallback
 ==================
*/
static void Sys_CompleteCommandCallback (const char *string){

	int		i;

	if (sys_console.cmdCount == MAX_LIST_COMMANDS)
		return;		// Too many commands

	if (Str_ICompareChars(string, sys_console.cmdPartial, sys_console.cmdPartialLen))
		return;		// No match

	// Ignore duplicates
	for (i = 0; i < sys_console.cmdCount; i++){
		if (!Str_ICompare(sys_console.cmdList[i], string))
			return;		// Already in list
	}

	// Add it
	sys_console.cmdList[sys_console.cmdCount++] = Mem_DupString(string, TAG_COMMON);
}

/*
 ==================
 Sys_CompleteCommand
 ==================
*/
static void Sys_CompleteCommand (){

	cvar_t	*cvar;
	char	text[MAX_INPUT_LENGTH], partial[MAX_INPUT_LENGTH];
	int		length;
	int		i;

	GetWindowText(sys_console.hWndInput, text, sizeof(text));

	// Clear auto-complete list
	Sys_ClearCommands();

	// Tokenize the string
	Cmd_TokenizeString(text);

	if (!Cmd_Argc())
		return;		// Nothing to search for

	// Find matching arguments if possible
	Str_Copy(sys_console.cmdPartial, Cmd_Argv(1), sizeof(sys_console.cmdPartial));
	sys_console.cmdPartialLen = Str_Length(sys_console.cmdPartial);
	sys_console.cmdPartialArg = 1;

	Cmd_ArgumentCompletion(Sys_CompleteCommandCallback);
	CVar_ArgumentCompletion(Sys_CompleteCommandCallback);

	// Otherwise find matching commands and variables
	if (!sys_console.cmdCount){
		Str_Copy(sys_console.cmdPartial, Cmd_Argv(0), sizeof(sys_console.cmdPartial));
		sys_console.cmdPartialLen = Str_Length(sys_console.cmdPartial);
		sys_console.cmdPartialArg = 0;

		Cmd_CommandCompletion(Sys_CompleteCommandCallback);
		CVar_CommandCompletion(Sys_CompleteCommandCallback);
	}

	if (!sys_console.cmdCount)
		return;		// Nothing was found

	// Copy the name and arguments if available
	if (sys_console.cmdPartialArg)
		Str_Copy(sys_console.cmdName, Cmd_Argv(0), sizeof(sys_console.cmdName));

	if (sys_console.cmdPartialArg + 1 != Cmd_Argc())
		Str_Copy(sys_console.cmdArgs, Cmd_Args(), sizeof(sys_console.cmdArgs));

	if (sys_console.cmdCount == 1){
		// Only one was found, so copy it to the input line
		if (sys_console.cmdName[0]){
			if (sys_console.cmdArgs[0])
				Str_SPrintf(text, sizeof(text), "%s %s %s", sys_console.cmdName, sys_console.cmdList[0], sys_console.cmdArgs);
			else
				Str_SPrintf(text, sizeof(text), "%s %s", sys_console.cmdName, sys_console.cmdList[0]);
		}
		else {
			if (sys_console.cmdArgs[0])
				Str_SPrintf(text, sizeof(text), "%s %s", sys_console.cmdList[0], sys_console.cmdArgs);
			else
				Str_SPrintf(text, sizeof(text), "%s ", sys_console.cmdList[0]);
		}

		SetWindowText(sys_console.hWndInput, text);

		SendMessage(sys_console.hWndInput, EM_SETSEL, 0, -1);
		SendMessage(sys_console.hWndInput, EM_SETSEL, -1, -1);
	}
	else {
		// Sort the commands
		qsort(sys_console.cmdList, sys_console.cmdCount, sizeof(char *), Sys_SortCommandList);

		// Print the commands
		if (sys_console.cmdName[0])
			Com_Printf("]%s %s\n", Cmd_Argv(0), Cmd_Argv(1));
		else
			Com_Printf("]%s\n", Cmd_Argv(0));

		for (i = 0; i < sys_console.cmdCount; i++){
			if (sys_console.cmdName[0])
				Com_Printf("    %s %s\n", sys_console.cmdName, sys_console.cmdList[i]);
			else {
				cvar = CVar_FindVariable(sys_console.cmdList[i]);
				if (cvar)
					Com_Printf("    %s = \"%s" S_COLOR_DEFAULT "\"\n", sys_console.cmdList[i], cvar->value);
				else
					Com_Printf("    %s\n", sys_console.cmdList[i]);
			}
		}

		if (sys_console.cmdCount == MAX_LIST_COMMANDS)
			Com_Printf("    ...\n");

		// Find the number of matching characters between the first and last
		// commands in the list and copy them
		length = 0;

		while (1){
			if (!sys_console.cmdList[0][length] || !sys_console.cmdList[sys_console.cmdCount-1][length])
				break;

			if (sys_console.cmdList[0][length] != sys_console.cmdList[sys_console.cmdCount-1][length])
				break;

			partial[length] = sys_console.cmdList[0][length];
			length++;
		}

		partial[length] = 0;

		if (sys_console.cmdName[0])
			length += Str_Length(sys_console.cmdName) + 1;

		// Save the cursor position
		sys_console.cmdCursor = length;

		// Copy the match to the input line
		if (sys_console.cmdName[0]){
			if (sys_console.cmdArgs[0])
				Str_SPrintf(text, sizeof(text), "%s %s %s", sys_console.cmdName, partial, sys_console.cmdArgs);
			else
				Str_SPrintf(text, sizeof(text), "%s %s", sys_console.cmdName, partial);
		}
		else {
			if (sys_console.cmdArgs[0])
				Str_SPrintf(text, sizeof(text), "%s %s", partial, sys_console.cmdArgs);
			else
				Str_SPrintf(text, sizeof(text), "%s", partial);
		}

		SetWindowText(sys_console.hWndInput, text);

		SendMessage(sys_console.hWndInput, EM_SETSEL, 0, length);
		SendMessage(sys_console.hWndInput, EM_SETSEL, -1, -1);
	}
}


/*
 ==============================================================================

 WINDOW PROCEDURES

 ==============================================================================
*/


/*
 ==================
 Sys_ConsoleWindowProc
 ==================
*/
static LRESULT CALLBACK Sys_ConsoleWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	switch (uMsg){
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_INACTIVE){
			SetFocus(sys_console.hWndInput);
			return 0;
		}

		break;
	case WM_CLOSE:
		if (sys_console.onError)
			PostQuitMessage(0);
		else
			ShowWindow(sys_console.hWnd, SW_HIDE);

		return 0;
	case WM_SYSCOMMAND:
		if (wParam == SC_KEYMENU)
			return 0;

		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED){
			if ((HWND)lParam == sys_console.hWndCopy){
				SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
				SendMessage(sys_console.hWndOutput, WM_COPY, 0, 0);

				break;
			}

			if ((HWND)lParam == sys_console.hWndClear){
				SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
				SendMessage(sys_console.hWndOutput, EM_REPLACESEL, FALSE, (LPARAM)"");

				break;
			}

			if ((HWND)lParam == sys_console.hWndQuit){
				PostQuitMessage(0);

				break;
			}
		}

		break;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == sys_console.hWndError){
			SetBkColor((HDC)wParam, RGB(192, 192, 192));

			if (sys_console.flashError)
				SetTextColor((HDC)wParam, RGB(255, 0, 0));
			else
				SetTextColor((HDC)wParam, RGB(0, 0, 0));

			return (LRESULT)sys_console.hBrush1;
		}

		if ((HWND)lParam == sys_console.hWndOutput){
			SetBkColor((HDC)wParam, RGB(28, 47, 54));
			SetTextColor((HDC)wParam, RGB(192, 192, 192));

			return (LRESULT)sys_console.hBrush2;
		}

		break;
	case WM_CTLCOLOREDIT:
		if ((HWND)lParam == sys_console.hWndInput){
			SetBkColor((HDC)wParam, RGB(28, 47, 54));
			SetTextColor((HDC)wParam, RGB(255, 255, 255));

			return (LRESULT)sys_console.hBrush2;
		}

		break;
	case WM_TIMER:
		sys_console.flashError = !sys_console.flashError;

		InvalidateRect(sys_console.hWndError, NULL, FALSE);

		break;
	case CONSOLE_NOTIFY_MESSAGE:
		if (lParam == WM_LBUTTONUP){
			ShowWindow(sys_console.hWnd, SW_RESTORE);
			UpdateWindow(sys_console.hWnd);
			SetForegroundWindow(sys_console.hWnd);
			SetFocus(sys_console.hWndInput);

			return 0;
		}

		break;
	}

	// Pass all unhandled messages to DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 ==================
 Sys_ConsoleInputProc
 ==================
*/
static LRESULT CALLBACK Sys_ConsoleInputProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	char	text[MAX_INPUT_LENGTH];
	int		cursor;

	switch (uMsg){
	case WM_KEYDOWN:
		// Auto-complete command
		if (wParam == VK_TAB){
			if (sys_console.cmdCount <= 1)
				Sys_CompleteCommand();
			else {
				if (GetKeyState(VK_SHIFT) & 0x8000){
					if (--sys_console.cmdIndex < 0)
						sys_console.cmdIndex = sys_console.cmdCount - 1;
				}
				else {
					if (++sys_console.cmdIndex == sys_console.cmdCount)
						sys_console.cmdIndex = 0;
				}

				if (sys_console.cmdName[0]){
					if (sys_console.cmdArgs[0])
						Str_SPrintf(text, sizeof(text), "%s %s %s", sys_console.cmdName, sys_console.cmdList[sys_console.cmdIndex], sys_console.cmdArgs);
					else
						Str_SPrintf(text, sizeof(text), "%s %s", sys_console.cmdName, sys_console.cmdList[sys_console.cmdIndex]);
				}
				else {
					if (sys_console.cmdArgs[0])
						Str_SPrintf(text, sizeof(text), "%s %s", sys_console.cmdList[sys_console.cmdIndex], sys_console.cmdArgs);
					else
						Str_SPrintf(text, sizeof(text), "%s", sys_console.cmdList[sys_console.cmdIndex]);
				}

				SetWindowText(sys_console.hWndInput, text);

				if (sys_console.cmdName[0])
					cursor = Str_Length(sys_console.cmdName) + 1 + Str_Length(sys_console.cmdList[sys_console.cmdIndex]);
				else
					cursor = Str_Length(sys_console.cmdList[sys_console.cmdIndex]);

				SendMessage(sys_console.hWndInput, EM_SETSEL, sys_console.cmdCursor, cursor);
			}

			return 0;
		}

		// Execute a command
		if (wParam == VK_RETURN){
			GetWindowText(sys_console.hWndInput, text, sizeof(text));

			if (!text[0])
				return 0;

			Com_Printf("]%s\n", text);

			// Add the command text
			Cmd_AppendText(text);
			Cmd_AppendText("\n");

			// Add to history
			Str_Copy(sys_console.historyText[sys_console.historyCount % MAX_HISTORY_LINES], text, sizeof(sys_console.historyText[sys_console.historyCount % MAX_HISTORY_LINES]));
			sys_console.historyCount++;
			sys_console.historyLine = sys_console.historyCount;

			// Clear input
			SetWindowText(sys_console.hWndInput, "");

			// Clear auto-complete list
			Sys_ClearCommands();

			return 0;
		}

		// Select previous history line
		if (wParam == VK_UP){
			if (sys_console.historyLine > 0 && sys_console.historyCount - sys_console.historyLine < MAX_HISTORY_LINES)
				sys_console.historyLine--;

			SetWindowText(sys_console.hWndInput, sys_console.historyText[sys_console.historyLine % MAX_HISTORY_LINES]);

			SendMessage(sys_console.hWndInput, EM_SETSEL, 0, -1);
			SendMessage(sys_console.hWndInput, EM_SETSEL, -1, -1);

			// Clear auto-complete list
			Sys_ClearCommands();

			return 0;
		}

		// Select next history line
		if (wParam == VK_DOWN){
			if (sys_console.historyLine == sys_console.historyCount)
				return 0;

			sys_console.historyLine++;

			if (sys_console.historyLine == sys_console.historyCount)
				SetWindowText(sys_console.hWndInput, "");
			else {
				SetWindowText(sys_console.hWndInput, sys_console.historyText[sys_console.historyLine % MAX_HISTORY_LINES]);

				SendMessage(sys_console.hWndInput, EM_SETSEL, 0, -1);
				SendMessage(sys_console.hWndInput, EM_SETSEL, -1, -1);
			}

			// Clear auto-complete list
			Sys_ClearCommands();

			return 0;
		}

		// Clear auto-complete list if needed
		if (wParam == VK_HOME || wParam == VK_END || wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_BACK || wParam == VK_DELETE || wParam == VK_INSERT){
			Sys_ClearCommands();
			break;
		}

		break;
	case WM_CHAR:
		// Clear auto-complete list if a clipboard operation
		if (wParam == 'x' - 'a' + 1 || wParam == 'c' - 'a' + 1 || wParam == 'v' - 'a' + 1){
			Sys_ClearCommands();
			break;
		}

		// Clear auto-complete list if printable
		if (Str_CharIsPrintable(wParam)){
			Sys_ClearCommands();
			break;
		}

		if (wParam == VK_TAB || wParam == VK_RETURN)
			return 0;		// Keep it from beeping

		break;
	}

	// Pass all unhandled messages to DefWindowProc
	return CallWindowProc(sys_console.defInputProc, hWnd, uMsg, wParam, lParam);
}


/*
 ==============================================================================

 CONSOLE CREATION AND DESTRUCTION

 ==============================================================================
*/


/*
 ==================
 Sys_CreateConsole
 ==================
*/
void Sys_CreateConsole (){

	WNDCLASSEX	wndClass;
	RECT		rect;
	HDC			hDC;
	int			screenWidth, screenHeight;
	int			x, y, w, h;
	int			size, tabStops = 16;

	// Calculate window position and dimensions
	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);
	
	rect.left = (screenWidth - 640) / 2;
	rect.top = (screenHeight - 480) / 2;
	rect.right = rect.left + 640;
	rect.bottom = rect.top + 480;

	AdjustWindowRectEx(&rect, CONSOLE_WINDOW_STYLE, FALSE, 0);
	
	x = rect.left;
	y = rect.top;
	w = rect.right - rect.left;
	h = rect.bottom - rect.top;

	// Register the window class
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = Sys_ConsoleWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = sys.hInstance;
	wndClass.hIcon = sys.hIcon;
	wndClass.hIconSm = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = CONSOLE_WINDOW_CLASS;

	if (!RegisterClassEx(&wndClass))
		Com_Error(ERR_FATAL, "Could not register system console window class");

	// Create the window
	sys_console.hWnd = CreateWindowEx(0, CONSOLE_WINDOW_CLASS, CONSOLE_WINDOW_NAME, CONSOLE_WINDOW_STYLE, x, y, w, h, NULL, NULL, sys.hInstance, NULL);
	if (!sys_console.hWnd){
		UnregisterClass(CONSOLE_WINDOW_CLASS, sys.hInstance);

		Com_Error(ERR_FATAL, "Could not create system console window");
	}

	// Create the controls
	sys_console.hWndError = CreateWindowEx(0, "STATIC", NULL, WS_CHILD | SS_SUNKEN | SS_LEFT | SS_NOPREFIX, 8, 8, 624, 32, sys_console.hWnd, NULL, sys.hInstance, NULL);
	sys_console.hWndOutput = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 8, 44, 624, 360, sys_console.hWnd, NULL, sys.hInstance, NULL);
	sys_console.hWndInput = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, 8, 408, 624, 22, sys_console.hWnd, NULL, sys.hInstance, NULL);
	sys_console.hWndCopy = CreateWindowEx(0, "BUTTON", "Copy", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 8, 449, 75, 23, sys_console.hWnd, NULL, sys.hInstance, NULL);
	sys_console.hWndClear = CreateWindowEx(0, "BUTTON", "Clear", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 87, 449, 75, 23, sys_console.hWnd, NULL, sys.hInstance, NULL);
	sys_console.hWndQuit = CreateWindowEx(0, "BUTTON", "Quit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 557, 449, 75, 23, sys_console.hWnd, NULL, sys.hInstance, NULL);

	// Create and set the fonts
	hDC = GetDC(sys_console.hWnd);
	size = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ReleaseDC(sys_console.hWnd, hDC);

	sys_console.hFont1 = CreateFont(size, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");
	sys_console.hFont2 = CreateFont(size, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Microsoft Sans Serif");

	SendMessage(sys_console.hWndError, WM_SETFONT, (WPARAM)sys_console.hFont1, FALSE);
	SendMessage(sys_console.hWndOutput, WM_SETFONT, (WPARAM)sys_console.hFont1, FALSE);
	SendMessage(sys_console.hWndInput, WM_SETFONT, (WPARAM)sys_console.hFont1, FALSE);
	SendMessage(sys_console.hWndCopy, WM_SETFONT, (WPARAM)sys_console.hFont2, FALSE);
	SendMessage(sys_console.hWndClear, WM_SETFONT, (WPARAM)sys_console.hFont2, FALSE);
	SendMessage(sys_console.hWndQuit, WM_SETFONT, (WPARAM)sys_console.hFont2, FALSE);

	// Create the brushes
	sys_console.hBrush1 = CreateSolidBrush(RGB(192, 192, 192));
	sys_console.hBrush2 = CreateSolidBrush(RGB(28, 47, 54));

	// Subclass input edit box
	sys_console.defInputProc = (WNDPROC)SetWindowLong(sys_console.hWndInput, GWL_WNDPROC, (LONG)Sys_ConsoleInputProc);

	// Set tab stops for the output edit box
	SendMessage(sys_console.hWndOutput, EM_SETTABSTOPS, (WPARAM)1, (LPARAM)&tabStops);

	// Set text limit for the edit boxes
	SendMessage(sys_console.hWndOutput, EM_SETLIMITTEXT, (WPARAM)(MAX_OUTPUT_LENGTH - 1), 0);
	SendMessage(sys_console.hWndInput, EM_SETLIMITTEXT, (WPARAM)(MAX_INPUT_LENGTH - 1), 0);

	// Create notify icon
	sys_console.notifyIcon.cbSize = sizeof(NOTIFYICONDATA);
	sys_console.notifyIcon.hWnd = sys_console.hWnd;
	sys_console.notifyIcon.uID = 0;
	sys_console.notifyIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	sys_console.notifyIcon.uCallbackMessage = CONSOLE_NOTIFY_MESSAGE;
	sys_console.notifyIcon.hIcon = sys.hIcon;
	Str_Copy(sys_console.notifyIcon.szTip, CONSOLE_WINDOW_NAME, sizeof(sys_console.notifyIcon.szTip));

	Shell_NotifyIcon(NIM_ADD, &sys_console.notifyIcon);

	sys_console.initialized = true;
}

/*
 ==================
 Sys_DestroyConsole
 ==================
*/
void Sys_DestroyConsole (){

	if (!sys_console.initialized)
		return;

	Shell_NotifyIcon(NIM_DELETE, &sys_console.notifyIcon);

	if (sys_console.onError)
		KillTimer(sys_console.hWnd, 1);

	if (sys_console.defInputProc)
		SetWindowLong(sys_console.hWndInput, GWL_WNDPROC, (LONG)sys_console.defInputProc);

	if (sys_console.hBrush1)
		DeleteObject(sys_console.hBrush1);
	if (sys_console.hBrush2)
		DeleteObject(sys_console.hBrush2);

	if (sys_console.hFont1)
		DeleteObject(sys_console.hFont1);
	if (sys_console.hFont2)
		DeleteObject(sys_console.hFont2);

	ShowWindow(sys_console.hWnd, SW_HIDE);
	DestroyWindow(sys_console.hWnd);

	UnregisterClass(CONSOLE_WINDOW_CLASS, sys.hInstance);

	Mem_Fill(&sys_console, 0, sizeof(sysConsole_t));
}


/*
 ==============================================================================

 MISCELLANEOUS FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 Sys_ConsolePrint
 ==================
*/
void Sys_ConsolePrint (const char *text){

	char	buffer[MAX_PRINT_MESSAGE];
	int		length = 0;

	if (!sys_console.initialized)
		return;

	// Copy into an intermediate buffer
	while (*text && (length < MAX_PRINT_MESSAGE - 2)){
		// Ignore color escape sequences
		if (Str_IsColor(text)){
			text += 2;
			continue;
		}

		// Copy \r\n unchanged
		if (text[0] == '\r' && text[1] == '\n'){
			buffer[length++] = '\r';
			buffer[length++] = '\n';

			text += 2;
			continue;
		}

		// Convert \r or \n to \r\n
		if (text[0] == '\r' || text[0] == '\n'){
			buffer[length++] = '\r';
			buffer[length++] = '\n';

			text += 1;
			continue;
		}

		// Copy the character
		buffer[length++] = *text++;
	}

	buffer[length] = 0;

	// Check for overflow
	sys_console.outputLength += length;
	if (sys_console.outputLength >= MAX_OUTPUT_LENGTH){
		sys_console.outputLength = length;

		SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
		SendMessage(sys_console.hWndOutput, EM_REPLACESEL, FALSE, (LPARAM)"");
	}

	// Scroll down
	SendMessage(sys_console.hWndOutput, EM_SETSEL, 0, -1);
	SendMessage(sys_console.hWndOutput, EM_SETSEL, -1, -1);

	SendMessage(sys_console.hWndOutput, EM_LINESCROLL, 0, 0xFFFF);
	SendMessage(sys_console.hWndOutput, EM_SCROLLCARET, 0, 0);

	// Add the text
	SendMessage(sys_console.hWndOutput, EM_REPLACESEL, FALSE, (LPARAM)buffer);

	// Update
	UpdateWindow(sys_console.hWnd);
}

/*
 ==================
 Sys_ConsoleError
 ==================
*/
void Sys_ConsoleError (const char *text){

	if (!sys_console.initialized){
		MessageBox(NULL, text, ENGINE_NAME, MB_OK | MB_ICONERROR | MB_TASKMODAL);

		PostQuitMessage(0);

		return;
	}

	sys_console.onError = true;

	SetTimer(sys_console.hWnd, 1, 1000, NULL);

	SetWindowText(sys_console.hWndError, text);

	ShowWindow(sys_console.hWndError, SW_SHOW);
	ShowWindow(sys_console.hWndInput, SW_HIDE);

	// Show the window
	ShowWindow(sys_console.hWnd, SW_RESTORE);
	UpdateWindow(sys_console.hWnd);
	SetForegroundWindow(sys_console.hWnd);
	SetFocus(sys_console.hWnd);
}

/*
 ==================
 Sys_ShowConsole
 ==================
*/
void Sys_ShowConsole (bool show){

	if (!sys_console.initialized)
		return;

	if (!show){
		// Hide the window
		ShowWindow(sys_console.hWnd, SW_HIDE);

		return;
	}

	// Show the window
	ShowWindow(sys_console.hWnd, SW_RESTORE);
	UpdateWindow(sys_console.hWnd);
	SetForegroundWindow(sys_console.hWnd);
	SetFocus(sys_console.hWndInput);
}