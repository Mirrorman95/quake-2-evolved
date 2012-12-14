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
// cl_key.c - Key event management
//


#include "client.h"


typedef struct {
	const char *			name;
	int						keyNum;
} keyName_t;

typedef struct {
	bool					down;					// True if currently down
	int						repeats;				// If > 1, it is auto-repeating

	const char *			binding;				// Bound command text
} key_t;

static keyName_t	key_names[] = {
	{"TAB",					K_TAB},
	{"ENTER",				K_ENTER},
	{"ESCAPE",				K_ESCAPE},
	{"SPACE",				K_SPACE},
	{"BACKSPACE",			K_BACKSPACE},

	{"UPARROW",				K_UPARROW},
	{"DOWNARROW",			K_DOWNARROW},
	{"LEFTARROW",			K_LEFTARROW},
	{"RIGHTARROW",			K_RIGHTARROW},

	{"INSERT",				K_INSERT},
	{"DELETE",				K_DELETE},
	{"HOME",				K_HOME},
	{"END",					K_END},
	{"PAGEUP",				K_PAGEUP},
	{"PAGEDOWN",			K_PAGEDOWN},

	{"LALT",				K_LALT},
	{"RALT",				K_RALT},
	{"LCTRL",				K_LCTRL},
	{"RCTRL",				K_RCTRL},
	{"LSHIFT",				K_LSHIFT},
	{"RSHIFT",				K_RSHIFT},

	{"LWIN",				K_LWIN},
	{"RWIN",				K_RWIN},
	{"MENU",				K_MENU},

	{"F1",					K_F1},
	{"F2",					K_F2},
	{"F3",					K_F3},
	{"F4",					K_F4},
	{"F5",					K_F5},
	{"F6",					K_F6},
	{"F7",					K_F7},
	{"F8",					K_F8},
	{"F9",					K_F9},
	{"F10",					K_F10},
	{"F11",					K_F11},
	{"F12",					K_F12},

	{"PAUSE",				K_PAUSE},
	{"CAPSLOCK",			K_CAPSLOCK},
	{"SCROLLLOCK",			K_SCROLLLOCK},
	{"PRINTSCREEN",			K_PRINTSCREEN},

	{"KP_HOME",				K_KP_HOME},
	{"KP_UPARROW",			K_KP_UPARROW},
	{"KP_PAGEUP",			K_KP_PAGEUP},
	{"KP_LEFTARROW",		K_KP_LEFTARROW},
	{"KP_5",				K_KP_5},
	{"KP_RIGHTARROW",		K_KP_RIGHTARROW},
	{"KP_END",				K_KP_END},
	{"KP_DOWNARROW",		K_KP_DOWNARROW},
	{"KP_PAGEDOWN",			K_KP_PAGEDOWN},
	{"KP_INSERT",			K_KP_INSERT},
	{"KP_DELETE",			K_KP_DELETE},
	{"KP_SLASH",			K_KP_SLASH},
	{"KP_STAR",				K_KP_STAR},
	{"KP_MINUS",			K_KP_MINUS},
	{"KP_PLUS",				K_KP_PLUS},
	{"KP_ENTER",			K_KP_ENTER},
	{"KP_NUMLOCK",			K_KP_NUMLOCK},

	{"MOUSE1",				K_MOUSE1},
	{"MOUSE2",				K_MOUSE2},
	{"MOUSE3",				K_MOUSE3},
	{"MOUSE4",				K_MOUSE4},
	{"MOUSE5",				K_MOUSE5},
	{"MOUSE6",				K_MOUSE6},
	{"MOUSE7",				K_MOUSE7},
	{"MOUSE8",				K_MOUSE8},

	{"MWHEELUP",			K_MWHEELUP},
	{"MWHEELDOWN",			K_MWHEELDOWN},

	{"CTRL_POV1",			K_CTRL_POV1},
	{"CTRL_POV2",			K_CTRL_POV2},
	{"CTRL_POV3",			K_CTRL_POV3},
	{"CTRL_POV4",			K_CTRL_POV4},
	{"CTRL_POV5",			K_CTRL_POV5},
	{"CTRL_POV6",			K_CTRL_POV6},
	{"CTRL_POV7",			K_CTRL_POV7},
	{"CTRL_POV8",			K_CTRL_POV8},
	{"CTRL_POV9",			K_CTRL_POV9},
	{"CTRL_POV10",			K_CTRL_POV10},
	{"CTRL_POV11",			K_CTRL_POV11},
	{"CTRL_POV12",			K_CTRL_POV12},
	{"CTRL_POV13",			K_CTRL_POV13},
	{"CTRL_POV14",			K_CTRL_POV14},
	{"CTRL_POV15",			K_CTRL_POV15},
	{"CTRL_POV16",			K_CTRL_POV16},

	{"CTRL_BUTTON1",		K_CTRL_BUTTON1},
	{"CTRL_BUTTON2",		K_CTRL_BUTTON2},
	{"CTRL_BUTTON3",		K_CTRL_BUTTON3},
	{"CTRL_BUTTON4",		K_CTRL_BUTTON4},
	{"CTRL_BUTTON5",		K_CTRL_BUTTON5},
	{"CTRL_BUTTON6",		K_CTRL_BUTTON6},
	{"CTRL_BUTTON7",		K_CTRL_BUTTON7},
	{"CTRL_BUTTON8",		K_CTRL_BUTTON8},
	{"CTRL_BUTTON9",		K_CTRL_BUTTON9},
	{"CTRL_BUTTON10",		K_CTRL_BUTTON10},
	{"CTRL_BUTTON11",		K_CTRL_BUTTON11},
	{"CTRL_BUTTON12",		K_CTRL_BUTTON12},
	{"CTRL_BUTTON13",		K_CTRL_BUTTON13},
	{"CTRL_BUTTON14",		K_CTRL_BUTTON14},
	{"CTRL_BUTTON15",		K_CTRL_BUTTON15},
	{"CTRL_BUTTON16",		K_CTRL_BUTTON16},
	{"CTRL_BUTTON17",		K_CTRL_BUTTON17},
	{"CTRL_BUTTON18",		K_CTRL_BUTTON18},
	{"CTRL_BUTTON19",		K_CTRL_BUTTON19},
	{"CTRL_BUTTON20",		K_CTRL_BUTTON20},
	{"CTRL_BUTTON21",		K_CTRL_BUTTON21},
	{"CTRL_BUTTON22",		K_CTRL_BUTTON22},
	{"CTRL_BUTTON23",		K_CTRL_BUTTON23},
	{"CTRL_BUTTON24",		K_CTRL_BUTTON24},
	{"CTRL_BUTTON25",		K_CTRL_BUTTON25},
	{"CTRL_BUTTON26",		K_CTRL_BUTTON26},
	{"CTRL_BUTTON27",		K_CTRL_BUTTON27},
	{"CTRL_BUTTON28",		K_CTRL_BUTTON28},
	{"CTRL_BUTTON29",		K_CTRL_BUTTON29},
	{"CTRL_BUTTON30",		K_CTRL_BUTTON30},
	{"CTRL_BUTTON31",		K_CTRL_BUTTON31},
	{"CTRL_BUTTON32",		K_CTRL_BUTTON32},

	{"X360_DPAD_UP",		K_X360_DPAD_UP},
	{"X360_DPAD_DOWN",		K_X360_DPAD_DOWN},
	{"X360_DPAD_LEFT",		K_X360_DPAD_LEFT},
	{"X360_DPAD_RIGHT",		K_X360_DPAD_RIGHT},
	{"X360_START",			K_X360_START},
	{"X360_BACK",			K_X360_BACK},
	{"X360_LEFT_THUMB",		K_X360_LEFT_THUMB},
	{"X360_RIGHT_THUMB",	K_X360_RIGHT_THUMB},
	{"X360_LEFT_SHOULDER",	K_X360_LEFT_SHOULDER},
	{"X360_RIGHT_SHOULDER",	K_X360_RIGHT_SHOULDER},
	{"X360_LEFT_TRIGGER",	K_X360_LEFT_TRIGGER},
	{"X360_RIGHT_TRIGGER",	K_X360_RIGHT_TRIGGER},
	{"X360_A",				K_X360_A},
	{"X360_B",				K_X360_B},
	{"X360_X",				K_X360_X},
	{"X360_Y",				K_X360_Y},

	{"SEMICOLON",			';'},			// A raw semicolon separates commands
	{"APOSTROPHE",			'\''},			// A raw apostrophe separates literals

	{NULL,					0}
};

static key_t				key_keys[MAX_KEYS];

static keyDest_t			key_dest;

static int					key_anyDown;


/*
 ==================
 Key_StringToKeyNum
 ==================
*/
int Key_StringToKeyNum (const char *string){

	keyName_t	*kn;

	if (!string || !string[0])
		return -1;

	if (!string[1])
		return string[0];

	// Scan for a key name match
	for (kn = key_names; kn->name; kn++){
		if (!Str_ICompare(kn->name, string))
			return kn->keyNum;
	}

	return -1;
}

/*
 ==================
 Key_KeyNumToString
 ==================
*/
const char *Key_KeyNumToString (int keyNum){

	static char	string[2];
	keyName_t	*kn;

	if (keyNum == -1)
		return "<KEY NOT FOUND>";

	if (keyNum < 0 || keyNum >= MAX_KEYS)
		return "<KEY OUT OF RANGE>";

	// Check for printable ASCII
	if (keyNum > 32 && keyNum < 127 && keyNum != ';' && keyNum != '\''){
		string[0] = keyNum;
		string[1] = 0;

		return string;
	}

	// Scan for a key number match
	for (kn = key_names; kn->name; kn++){
		if (kn->keyNum == keyNum)
			return kn->name;
	}

	return "<UNKNOWN KEY>";
}

/*
 ==================
 Key_GetBinding
 ==================
*/
const char *Key_GetBinding (int keyNum){

	key_t	*k;

	if (keyNum < 0 || keyNum >= MAX_KEYS)
		return "";

	k = &key_keys[keyNum];

	if (!k->binding)
		return "";

	return k->binding;
}

/*
 ==================
 Key_SetBinding
 ==================
*/
void Key_SetBinding (int keyNum, const char *binding){

	key_t	*k;

	if (keyNum < 0 || keyNum >= MAX_KEYS)
		return;

	k = &key_keys[keyNum];

	// Consider this like modifying an archive variable, so the config file
	// will be written the next frame
	CVar_SetModifiedFlags(CVAR_ARCHIVE);

	// Free old binding
	if (k->binding){
		Mem_Free(k->binding);
		k->binding = NULL;
	}

	// Allocate memory for new binding
	if (!binding || !binding[0])
		return;

	k->binding = Mem_DupString(binding, TAG_COMMON);
}

/*
 ==================
 Key_WriteBindings
 ==================
*/
void Key_WriteBindings (fileHandle_t f){

	key_t	*k;
	int		i;

	FS_Printf(f, "unbindAll" NEWLINE);

	for (i = 0, k = key_keys; i < MAX_KEYS; i++, k++){
		if (!k->binding)
			continue;

		FS_Printf(f, "bind %s \"%s\"" NEWLINE, Key_KeyNumToString(i), k->binding);
	}
}


// ============================================================================


/*
 ==================
 Key_GetKeyDest
 ==================
*/
keyDest_t Key_GetKeyDest (){

	return key_dest;
}

/*
 ==================
 Key_SetKeyDest
 ==================
*/
void Key_SetKeyDest (keyDest_t dest){

	key_dest = dest;
}

/*
 ==================
 Key_IsDown
 ==================
*/
bool Key_IsDown (int key){

	if (key < 0 || key >= MAX_KEYS)
		return false;

	return key_keys[key].down;
}

/*
 ==================
 Key_IsAnyDown
 ==================
*/
bool Key_IsAnyDown (){

	return (key_anyDown != 0);
}

/*
 ==================
 Key_ClearStates
 ==================
*/
void Key_ClearStates (){

	key_t	*k;
	int		i;

	key_anyDown = 0;

	for (i = 0, k = key_keys; i < MAX_KEYS; i++, k++){
		if (k->down)
			Key_Event(i, 0, false);

		k->down = false;
		k->repeats = 0;
	}
}

/*
 ==================
 Key_Event
 ==================
*/
void Key_Event (int key, int time, bool down){

	key_t		*k;
	const char	*kb;
	char		cmd[1024];

	if (key < 0 || key >= MAX_KEYS)
		return;

	k = &key_keys[key];

	// Update key status
	if (down){
		k->down = true;
		k->repeats++;

		if (k->repeats == 1)
			key_anyDown++;

		if (key_dest == KEY_GAME){
			if (k->repeats == 100 && !k->binding)
				Com_Printf("\"%s\" is unbound, use controls menu to set\n", Key_KeyNumToString(key));

			if (k->repeats > 1)
				return;		// Ignore most auto-repeats
		}
	}
	else {
		k->down = false;
		k->repeats = 0;

		key_anyDown--;
		if (key_anyDown < 0)
			key_anyDown = 0;
	}

	// Console key is hardcoded, so the user can never unbind it
	if (key == '`' || key == '~'){
		if (!down)
			return;

		Con_Toggle();
		return;
	}

	// Any key pressed during cinematic playback will finish it and
	// advance to the next server
	if ((cls.state == CA_ACTIVE && cls.playingCinematic) && (key_dest != KEY_CONSOLE && (key < K_F1 || key > K_F12))){
		if (!down)
			return;

		CL_StopCinematic();
		CL_FinishCinematic();
		return;
	}

	// Any key pressed during demo playback will disconnect
	if ((cls.state == CA_ACTIVE && cl.demoPlayback) && (key_dest != KEY_CONSOLE && (key < K_F1 || key > K_F12))){
		if (!down)
			return;

		Cmd_AppendText("disconnect\n");
		return;
	}

	// Escape key is hardcoded, so the user can never unbind it
	if (key == K_ESCAPE){
		// We only care about key down events here
		if (!down)
			return;

		// If playing a cinematic, stop and bring up the main menu
		if (cls.playingCinematic && key_dest != KEY_CONSOLE){
			CL_StopCinematic();
			CL_FinishCinematic();
			return;
		}


		// If connecting or loading, disconnect
		if ((cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE) && key_dest != KEY_CONSOLE){
			Cmd_AppendText("disconnect\n");
			return;
		}

		// If the help computer / inventory is on, put away
		if (cl.frame.playerState.stats[STAT_LAYOUTS] && key_dest == KEY_GAME){
			Cmd_AppendText("cmd putaway\n");
			return;
		}

		// Send the key event to the appropriate handler
		switch (key_dest){
		case KEY_GAME:
			if (cls.state == CA_ACTIVE)
				UI_SetActiveMenu(UI_INGAMEMENU);
			else
				UI_SetActiveMenu(UI_MAINMENU);

			break;
		case KEY_CONSOLE:
			Con_KeyEvent(key, down);
			break;
		case KEY_MESSAGE:
			Con_KeyMessageEvent(key, down);
			break;
		case KEY_MENU:
			UI_KeyEvent(key, down);
			break;
		default:
			Com_Error(ERR_FATAL, "Key_Event: bad key_dest");
		}

		return;
	}

	// Key up events only generate game commands if the key binding is a button
	// command (leading + sign). These will be processed even if any key catcher is active,
	// to keep the player from continuing an action started before a mode switch.
	// Button commands include the key number as a parameter, so
	// multiple downs can be matched with ups.
	if (!down){
		kb = k->binding;
		if (kb && kb[0] == '+'){
			Str_SPrintf(cmd, sizeof(cmd), "-%s %i %i\n", kb+1, key, time);
			Cmd_AppendText(cmd);
		}

		return;
	}

	// Send the key event to the appropriate handler
	switch (key_dest){
	case KEY_GAME:
		// Send mouse events to the editor if active
		if (key == K_MOUSE1 && Com_IsEditorActive()){
			if (Com_EditorEvent())
				return;		// Don't generate a game command
		}

		// Generate a game command
		kb = k->binding;
		if (kb){
			if (kb[0] == '+'){
				Str_SPrintf(cmd, sizeof(cmd), "%s %i %i\n", kb, key, time);
				Cmd_AppendText(cmd);
			}
			else {
				Str_SPrintf(cmd, sizeof(cmd), "%s\n", kb);
				Cmd_AppendText(cmd);
			}
		}

		break;
	case KEY_CONSOLE:
		Con_KeyEvent(key, down);
		break;
	case KEY_MESSAGE:
		Con_KeyMessageEvent(key, down);
		break;
	case KEY_MENU:
		UI_KeyEvent(key, down);
		break;
	default:
		Com_Error(ERR_FATAL, "Key_Event: bad key_dest");
	}
}

/*
 ==================
 Key_CharEvent
 ==================
*/
void Key_CharEvent (int ch){

	if (ch < 0 || ch > 255)
		return;

	// Send the char event to the appropriate handler
	switch (key_dest){
	case KEY_GAME:
		break;
	case KEY_CONSOLE:
		Con_CharEvent(ch);
		break;
	case KEY_MESSAGE:
		Con_CharMessageEvent(ch);
		break;
	case KEY_MENU:
		UI_CharEvent(ch);
		break;
	default:
		Com_Error(ERR_FATAL, "Key_CharEvent: bad key_dest");
	}
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 Key_ArgCompletion_KeyName
 ==================
*/
static void Key_ArgCompletion_KeyName (void (*callback)(const char *string)){

	keyName_t	*kn;
	char		string[2];
	int			i;

	callback("-");
	callback("=");
	callback("[");
	callback("]");
	callback(",");
	callback(".");
	callback("/");
	callback("\\");

	for (i = '0'; i <= '9'; i++){
		string[0] = i;
		string[1] = 0;

		callback(string);
	}

	for (i = 'a'; i <= 'z'; i++){
		string[0] = i;
		string[1] = 0;

		callback(string);
	}

	for (kn = key_names; kn->name; kn++)
		callback(kn->name);
}

/*
 ==================
 Key_Bind_f
 ==================
*/
static void Key_Bind_f (){

	int		keyNum;
	char	cmd[1024];
	int		i;

	if (Cmd_Argc() < 2){
		Com_Printf("Usage: bind <key> [command]\n");
		return;
	}

	keyNum = Key_StringToKeyNum(Cmd_Argv(1));
	if (keyNum == -1){
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (Cmd_Argc() == 2){
		if (key_keys[keyNum].binding)
			Com_Printf("\"%s\" = \"%s" S_COLOR_DEFAULT "\"\n", Cmd_Argv(1), key_keys[keyNum].binding);
		else
			Com_Printf("\"%s\" is not bound\n", Cmd_Argv(1));

		return;
	}

	cmd[0] = 0;
	for (i = 2; i < Cmd_Argc(); i++){
		Str_Append(cmd, Cmd_Argv(i), sizeof(cmd));
		if (i != Cmd_Argc() - 1)
			Str_Append(cmd, " ", sizeof(cmd));
	}

	Key_SetBinding(keyNum, cmd);
}

/*
 ==================
 Key_Unbind_f
 ==================
*/
static void Key_Unbind_f (){

	int		keyNum;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: unbind <key>\n");
		return;
	}

	keyNum = Key_StringToKeyNum(Cmd_Argv(1));
	if (keyNum == -1){
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (!key_keys[keyNum].binding){
		Com_Printf("\"%s\" is not bound\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding(keyNum, NULL);
}

/*
 ==================
 Key_UnbindAll_f
 ==================
*/
static void Key_UnbindAll_f (){

	key_t	*k;
	int		i;

	for (i = 0, k = key_keys; i < MAX_KEYS; i++, k++){
		if (!k->binding)
			continue;

		Key_SetBinding(i, NULL);
	}
}

/*
 ==================
 Key_ListBinds_f
 ==================
*/
static void Key_ListBinds_f (){

	key_t	*k;
	int		i;

	for (i = 0, k = key_keys; i < MAX_KEYS; i++, k++){
		if (!k->binding)
			continue;

		Com_Printf("%-32s \"%s" S_COLOR_DEFAULT "\"\n", Key_KeyNumToString(i), k->binding);
	}
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 Key_Init
 ==================
*/
void Key_Init (){

	// Add commands
	Cmd_AddCommand("bind", Key_Bind_f, "Binds a command to a key", Key_ArgCompletion_KeyName);
	Cmd_AddCommand("unbind", Key_Unbind_f, "Unbinds any command from a key", Key_ArgCompletion_KeyName);
	Cmd_AddCommand("unbindAll", Key_UnbindAll_f, "Unbinds any commands from all keys", NULL);
	Cmd_AddCommand("listBinds", Key_ListBinds_f, "Lists key bindings", NULL);
}

/*
 ==================
 Key_Shutdown
 ==================
*/
void Key_Shutdown (){

	key_t	*k;
	int		i;

	// Remove commands
	Cmd_RemoveCommand("bind");
	Cmd_RemoveCommand("unbind");
	Cmd_RemoveCommand("unbindAll");
	Cmd_RemoveCommand("listBinds");

	// Remove all key destinations
	key_dest = KEY_NONE;

	// No keys down
	key_anyDown = 0;

	// Free bindings
	for (i = 0, k = key_keys; i < MAX_KEYS; i++, k++){
		if (k->binding)
			Mem_Free(k->binding);
	}

	Mem_Fill(key_keys, 0, sizeof(key_keys));
}