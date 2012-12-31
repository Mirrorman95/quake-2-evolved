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
// sv_commands.c - Operator console commands
//


#include "server.h"


/*
 ==============================================================================

 SAVEGAME FILES

 ==============================================================================
*/


/*
 ==================
 SV_WipeSaveGame

 Delete save/<XXX>/
 ==================
*/
static void SV_WipeSaveGame (const char *saveName){

	char		name[MAX_PATH_LENGTH];
	const char	**fileList;
	int			numFiles;
	int			i;

	Com_DPrintf("SV_WipeSaveGame( %s )\n", saveName);

	// Delete the savegame
	Str_SPrintf(name, sizeof(name), "save/%s/server.ssv", saveName);
	FS_RemoveFile(name);

	Str_SPrintf(name, sizeof(name), "save/%s/game.ssv", saveName);
	FS_RemoveFile(name);

	// Find .sav files
	Str_SPrintf(name, sizeof(name), "save/%s", saveName);

	fileList = FS_ListFiles(name, ".sav", false, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Delete .sav
		Str_SPrintf(name, sizeof(name), "save/%s/%s", saveName, fileList[i]);
		FS_RemoveFile(name);
	}

	FS_FreeFileList(fileList);

	// Find .sv2 files
	Str_SPrintf(name, sizeof(name), "save/%s", saveName);

	fileList = FS_ListFiles(name, ".sv2", false, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Delete .sv2
		Str_SPrintf(name, sizeof(name), "save/%s/%s", saveName, fileList[i]);
		FS_RemoveFile(name);
	}

	FS_FreeFileList(fileList);
}

/*
 ==================
 SV_CopySaveGame
 ==================
*/
static void SV_CopySaveGame (const char *src, const char *dst){

	char		path[MAX_PATH_LENGTH], nameSrc[MAX_PATH_LENGTH], nameDst[MAX_PATH_LENGTH];
	char		name[MAX_PATH_LENGTH];
	const char	**fileList;
	int			numFiles;
	int			i;

	SV_WipeSaveGame(dst);

	Com_DPrintf("SV_CopySaveGame( %s, %s )\n", src, dst);

	// Copy the savegame over
	Str_SPrintf(nameSrc, sizeof(nameSrc), "save/%s/server.ssv", src);
	Str_SPrintf(nameDst, sizeof(nameDst), "save/%s/server.ssv", dst);

	FS_CopyFile(nameSrc, nameDst);

	Str_SPrintf(nameSrc, sizeof(nameSrc), "save/%s/game.ssv", src);
	Str_SPrintf(nameDst, sizeof(nameDst), "save/%s/game.ssv", dst);

	FS_CopyFile(nameSrc, nameDst);

	// Find .sav files
	Str_SPrintf(path, sizeof(path), "save/%s", src);

	fileList = FS_ListFiles(name, ".sav", false, &numFiles);

	for (i = 0; i < numFiles; i++){
		Str_SPrintf(nameSrc, sizeof(nameSrc), "save/%s/%s", src, fileList[i]);
		Str_SPrintf(nameDst, sizeof(nameDst), "save/%s/%s", dst, fileList[i]);

		FS_CopyFile(nameSrc, nameDst);

		// Change .sav to .sv2
		nameSrc[Str_Length(nameSrc)-3] = 0;
		Str_Append(nameSrc, "sv2", sizeof(nameSrc));
		nameDst[Str_Length(nameDst)-3] = 0;
		Str_Append(nameDst, "sv2", sizeof(nameDst));

		FS_CopyFile(nameSrc, nameDst);
	}

	FS_FreeFileList(fileList);
}

/*
 ==================
 SV_WriteServerFile
 ==================
*/
static void SV_WriteServerFile (bool autoSave){

	fileHandle_t	f;
	cvar_t			*cvar;
	char			comment[32];
	char			name[128], string[128];
	time_t			t;
	struct tm		*lt;

	Com_DPrintf("SV_WriteServerFile( %s )\n", autoSave ? "true" : "false");

	Str_SPrintf(name, sizeof(name), "save/current/server.ssv");
	FS_OpenFile(name, FS_WRITE, &f);
	if (!f){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}
	
	// Write the comment field
	if (!autoSave){
		time(&t);
		lt = localtime(&t);
		Str_SPrintf(comment, sizeof(comment), "%2i:%i%i %2i/%2i  ", lt->tm_hour, lt->tm_min/10, lt->tm_min%10, lt->tm_mon+1, lt->tm_mday);
		Str_Append(comment, sv.configStrings[CS_NAME], sizeof(comment));
	}
	else
		// Autosaved
		Str_SPrintf(comment, sizeof(comment), "ENTERING %s", sv.configStrings[CS_NAME]);
	
	FS_Write(f, comment, sizeof(comment));

	// Write the map cmd
	FS_Write(f, svs.mapCmd, sizeof(svs.mapCmd));

	// Write all CVAR_LATCH cvars
	// These will be things like coop, skill, deathmatch, etc...
	for (cvar = cvar_variables; cvar; cvar = cvar->next){
		if (!(cvar->flags & CVAR_SERVERINFO))
			continue;
		if (!(cvar->flags & CVAR_LATCH))
			continue;

		if (Str_Length(cvar->name) >= sizeof(name) || Str_Length(cvar->value) >= sizeof(string)){
			Com_DPrintf("Variable is too long: %s = %s\n", cvar->name, cvar->value);
			continue;
		}

		Str_Copy(name, cvar->name, sizeof(name));
		Str_Copy(string, cvar->value, sizeof(string));
		FS_Write(f, name, sizeof(name));
		FS_Write(f, string, sizeof(string));
	}

	FS_CloseFile(f);

	// Write the game state
	Str_SPrintf(name, sizeof(name), "%s/%s/save/current/game.ssv", CVar_GetVariableString("fs_savePath"), CVar_GetVariableString("fs_game"));
	ge->WriteGame(name, autoSave);
}

/*
 ==================
 SV_ReadServerFile
 ==================
*/
static void SV_ReadServerFile (){

	fileHandle_t	f;
	int				size;
	char			comment[32], mapCmd[128];
	char			name[128], string[128];

	Com_DPrintf("SV_ReadServerFile()\n");

	Str_SPrintf(name, sizeof(name), "save/current/server.ssv");
	size = FS_OpenFile(name, FS_READ, &f);
	if (!f){
		Com_Printf("Couldn't read %s\n", name);
		return;
	}

	// Read the comment field
	FS_Read(f, comment, sizeof(comment));

	// Read the map cmd
	FS_Read(f, mapCmd, sizeof(mapCmd));

	// Read all CVAR_LATCH cvars
	// These will be things like coop, skill, deathmatch, etc...
	while (FS_Tell(f) < size){
		if (!FS_Read(f, name, sizeof(name)))
			break;

		FS_Read(f, string, sizeof(string));
		CVar_SetVariableString(name, string, false);
	}

	FS_CloseFile(f);

	// Start a new game fresh with new cvars
	SV_InitGame();

	Str_Copy(svs.mapCmd, mapCmd, sizeof(svs.mapCmd));

	// Read the game state
	Str_SPrintf(name, sizeof(name), "%s/%s/save/current/game.ssv", CVar_GetVariableString("fs_savePath"), CVar_GetVariableString("fs_game"));
	ge->ReadGame(name);
}

/*
 ==================
 SV_WriteLevelFile
 ==================
*/
void SV_WriteLevelFile (){

	fileHandle_t	f;
	char			name[MAX_PATH_LENGTH];

	Com_DPrintf("SV_WriteLevelFile()\n");

	Str_SPrintf(name, sizeof(name), "save/current/%s.sv2", sv.name);
	FS_OpenFile(name, FS_WRITE, &f);
	if (!f){
		Com_Printf("Failed to open %s\n", name);
		return;
	}

	FS_Write(f, sv.configStrings, sizeof(sv.configStrings));
	CM_WritePortalState(f);
	FS_CloseFile(f);

	Str_SPrintf(name, sizeof(name), "%s/%s/save/current/%s.sav", CVar_GetVariableString("fs_savePath"), CVar_GetVariableString("fs_game"), sv.name);
	ge->WriteLevel(name);
}

/*
 ==================
 SV_ReadLevelFile
 ==================
*/
void SV_ReadLevelFile (){

	fileHandle_t	f;
	char			name[MAX_PATH_LENGTH];

	Com_DPrintf("SV_ReadLevelFile()\n");

	Str_SPrintf(name, sizeof(name), "save/current/%s.sv2", sv.name);
	FS_OpenFile(name, FS_READ, &f);
	if (!f){
		Com_Printf("Failed to open %s\n", name);
		return;
	}

	FS_Read(f, sv.configStrings, sizeof(sv.configStrings));
	CM_ReadPortalState(f);
	FS_CloseFile(f);

	Str_SPrintf(name, sizeof(name), "%s/%s/save/current/%s.sav", CVar_GetVariableString("fs_savePath"), CVar_GetVariableString("fs_game"), sv.name);
	ge->ReadLevel(name);
}

/*
 ==================
 SV_LoadGame_f
 ==================
*/
void SV_LoadGame_f (){

	char	name[MAX_PATH_LENGTH];
	char	*dir;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: loadGame <directory>\n");
		return;
	}

	dir = Cmd_Argv(1);
	if (Str_FindChar(dir, '..') || Str_FindChar(dir, '/') || Str_FindChar(dir, '\\')){
		Com_Printf("Bad save directory\n");
		return;
	}

	if (!Str_ICompare(dir, "current")){
		Com_Printf("Can't load from 'current'\n");
		return;
	}

	// Make sure the server.ssv file exists
	Str_SPrintf(name, sizeof(name), "save/%s/server.ssv", dir);
	if (!FS_FileExists(name)){
		Com_Printf("No such savegame '%s'\n", name);
		return;
	}

	Com_Printf("Loading game...\n");

	SV_CopySaveGame(dir, "current");

	SV_ReadServerFile();

	// Don't save current level when changing
	sv.state = SS_DEAD;
	Com_SetServerState(sv.state);

	// Go to the map
	SV_Map(svs.mapCmd, false, true);
}

/*
 ==================
 SV_SaveGame_f
 ==================
*/
void SV_SaveGame_f (){

	char	*dir;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: saveGame <directory>\n");
		return;
	}

	dir = Cmd_Argv(1);
	if (Str_FindChar(dir, '..') || Str_FindChar(dir, '/') || Str_FindChar(dir, '\\')){
		Com_Printf("Bad save directory\n");
		return;
	}

	if (sv.state != SS_GAME){
		Com_Printf("You must be in a game to save\n");
		return;
	}

	if (CVar_GetVariableInteger("deathmatch")){
		Com_Printf("Can't save in a deathmatch game\n");
		return;
	}

	if (sv_maxClients->integerValue == 1 && svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0){
		Com_Printf("Can't save while dead!\n");
		return;
	}

	if (!Str_ICompare(dir, "current")){
		Com_Printf("Can't save to 'current'\n");
		return;
	}

	Com_Printf("Saving game...\n");

	// Archive current level, including all client edicts.
	// When the level is reloaded, they will be shells awaiting a
	// connecting client.
	SV_WriteLevelFile();

	// Save server state
	SV_WriteServerFile(false);

	// Copy it off
	SV_CopySaveGame("current", dir);

	Com_Printf("Done\n");
}


/*
 ==============================================================================

 MAP, DEMO, AND CINEMATIC COMMANDS

 ==============================================================================
*/


/*
 ==================
 SV_GameMap

 Saves the state of the map just being exited and goes to a new map.

 If the initial character of the map string is '*', the next map is in a
 new unit, so the current savegame directory is cleared of map files.

 Example:

 *ntro.cin+base1

 Clears the archived maps, plays the ntro.cin cinematic, then goes to 
 map base1.bsp.
 ==================
*/
static void SV_GameMap (const char *map){

	client_t	*client;
	bool		savedInuse[MAX_CLIENTS];
	int			i;

	Com_DPrintf("SV_GameMap( %s )\n", map);

	// Check for clearing the current savegame
	if (map[0] == '*')
		// Wipe all the *.sav files
			SV_WipeSaveGame("current");
	else {
		// Save the map just exited
		if (sv.state == SS_GAME){
			// Clear all the client inuse flags before saving so that
			// when the level is re-entered, the clients will spawn at
			// spawn points instead of occupying body shells
			for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
				savedInuse[i] = client->edict->inuse;
				client->edict->inuse = false;
			}

			SV_WriteLevelFile();

			// We must restore these for clients to transfer over 
			// correctly
			for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++)
				client->edict->inuse = savedInuse[i];
		}
	}

	// Start up the next map
	SV_Map(map, false, false);

	// Archive server state
	Str_Copy(svs.mapCmd, Cmd_Argv(1), sizeof(svs.mapCmd));

	// Copy off the level to the autosave slot
	if (!com_dedicated->integerValue){
		SV_WriteServerFile(true);
		SV_CopySaveGame("current", "save0");
	}
}

/*
 ==================
 SV_GameMap_f

 Can be map, demo or cinematic
 ==================
*/
void SV_GameMap_f (){

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: gameMap <map>\n");
		return;
	}

	SV_GameMap(Cmd_Argv(1));
}

/*
 ==================
 SV_Map_f

 Goes directly to a given map without any savegame archiving.
 For development work.
 ==================
*/
void SV_Map_f (){

	char	map[MAX_PATH_LENGTH];
	char	checkName[MAX_PATH_LENGTH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: map <name>\n");
		return;
	}

	Str_Copy(map, Cmd_Argv(1), sizeof(map));

	// If not a demo or cinematic, check to make sure the level exists
	Str_SPrintf(checkName, sizeof(checkName), "maps/%s.bsp", map);
	if (!FS_FileExists(checkName)){
		Com_Printf("Can't find %s\n", checkName);
		return;
	}

	// Don't save current level when changing
	sv.state = SS_DEAD;
	Com_SetServerState(sv.state);

	SV_WipeSaveGame("current");

	SV_GameMap(map);
}

/*
 ==================
 SV_Demo_f

 Runs a demo
 ==================
*/
void SV_Demo_f (){

	char	map[MAX_PATH_LENGTH];
	char	checkName[MAX_PATH_LENGTH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: demo <demoName>\n");
		return;
	}

	Str_Copy(map, Cmd_Argv(1), sizeof(map));
	Str_DefaultFileExtension(map, sizeof(map), ".dm2");

	// Make sure it exists
	Str_SPrintf(checkName, sizeof(checkName), "demos/%s", map);
	if (!FS_FileExists(checkName)){
		Com_Printf("Can't find %s\n", checkName);
		return;
	}

	SV_Map(map, true, false);
}


/*
 ==============================================================================

 ...

 ==============================================================================
*/


/*
 ==================
 SV_SetPlayer

 TODO: split this function up into two?, one for getting the ID and the other for name

 Sets sv_client and sv_player to the player with userId
 ==================
*/
static bool SV_SetPlayer (const char *userId){

	client_t	*client;
	char		name[32];
	int			num;
	int			i;

	// Check for a name match
	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		if (client->state == CS_FREE)
			continue;

		// Compare without color sequences
		Str_Copy(name, client->name, sizeof(name));

		if (!Str_ICompareWithoutColors(client->name, userId) || !Str_ICompareWithoutColors(name, userId)){
			sv_client = client;
			sv_player = sv_client->edict;
			return true;
		}
	}

	// If a numeric value, check slots
	for (i = 0; userId[i]; i++){
		if (userId[i] < '0' || userId[i] > '9'){
			Com_Printf("Client '%s' is not on the server\n", userId);
			return false;
		}
	}

	num = Str_ToInteger(userId);
	if (num >= sv_maxClients->integerValue){
		Com_Printf("Bad client slot %i\n", num);
		return false;
	}

	if (svs.clients[num].state == CS_FREE){
		Com_Printf("Client %i is not active\n", num);
		return false;
	}

	sv_client = &svs.clients[num];
	sv_player = sv_client->edict;

	return true;
}

/*
 ==================
 SV_Kick_f

 Kick a user off the server
 ==================
*/
void SV_Kick_f (){

	if (!svs.initialized){
		Com_Printf("Server is not running\n");
		return;
	}

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: kick <number>\n");
		return;
	}

	if (!SV_SetPlayer(Cmd_Argv(1)))
		return;

	SV_BroadcastPrintf(PRINT_HIGH, "%s was kicked\n", sv_client->name);

	// Print directly, because the dropped client won't get the
	// SV_BroadcastPrintf message
	SV_ClientPrintf(sv_client, PRINT_HIGH, "You were kicked from the game\n");
	SV_DropClient(sv_client);

	sv_client->lastMessage = svs.realTime;		// In case there is a funny zombie
}

/*
 ==================
 SV_Status_f

 TODO: this could be improved some more
 ==================
*/
void SV_Status_f (){

	client_t	*client;
	const char	*string;
	int			d, h, m, s;
	int			time;
	int			ping, length;
	int			i, j;

	if (!svs.initialized){
		Com_Printf("Server is not running\n");
		return;
	}

	time = svs.realTime;
	d = time / 86400000;

	time %= 86400000;
	h = time / 3600000;

	time %= 3600000;
	m = time / 60000;

	time %= 60000;
	s = time / 1000;

	Com_Printf("\n");
	Com_Printf("Map: %s\n", sv.name);
	Com_Printf("Up time: %02i:%02i:%02i:%02i\n", d, h, m, s);

	Com_Printf("\n");
	Com_Printf("num score ping lastmsg address               cport  rate  name\n");
	Com_Printf("--- ----- ---- ------- --------------------- ------ ----- ---------------\n");
	
	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		if (client->state == CS_FREE)
			continue;

		Com_Printf("%3i ", i);
		Com_Printf("%5i ", client->edict->client->ps.stats[STAT_FRAGS]);

		switch (client->state){
		case CS_ZOMBIE:
			Com_Printf("ZMBI ");
			break;
		case CS_CONNECTED:
			Com_Printf("CNCT ");
			break;
		case CS_SPAWNED:
			Com_Printf("SPWD ");
			break;
		default:
			ping = Clamp(client->ping, 0, 9999);
			Com_Printf("%4i ", ping);
			break;
		}

		Com_Printf("%7i ", svs.realTime - client->lastMessage);

		string = NET_AddressToString(client->netChan.remoteAddress);
		Com_Printf("%s", string);
		length = 22 - Str_Length(string);
		for (j = 0; j < length; j++)
			Com_Printf(" ");
		
		Com_Printf("%6i ", client->netChan.channelPort);

		Com_Printf("%5i ", client->rate);

		Com_Printf("%s\n", client->name);
	}

	Com_Printf("\n");
}

/*
 ==================
 SV_Heartbeat_f
 ==================
*/
void SV_Heartbeat_f (){

	if (!svs.initialized){
		Com_Printf("Server is not running\n");
		return;
	}

	// SV_MasterHeartbeat will fire immediately
	svs.lastHeartbeat = -9999999;
}

/*
 ==================
 SV_ServerInfo_f

 Examine the server info string
 ==================
*/
void SV_ServerInfo_f (){

	Com_Printf("Server info settings:\n");
	Com_Printf("---------------------\n");

	Info_Print(CVar_InfoString(CVAR_SERVERINFO));
}

/*
 ==================
 SV_DumpUser_f

 Examine a user's info string
 ==================
*/
void SV_DumpUser_f (){

	if (!svs.initialized){
		Com_Printf("Server is not running\n");
		return;
	}

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: dumpUserInfo <number>\n");
		return;
	}

	if (!SV_SetPlayer(Cmd_Argv(1)))
		return;

	Com_Printf("User info settings:\n");
	Com_Printf("-------------------\n");

	Info_Print(sv_client->userInfo);
}


/*
 ==================
 SV_ServerRecord_f

 Begins server demo recording. Every entity and every message will be
 recorded, but no playerinfo will be stored. Primarily for demo merging.
 ==================
*/
void SV_ServerRecord_f (){

	msg_t	msg;
	char	name[MAX_PATH_LENGTH];
	char	data[32768];
	int		length;
	int		i;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: serverRecord [demoName]\n");
		return;
	}

	if (svs.demoFile){
		Com_Printf("Already recording\n");
		return;
	}

	if (sv.state != SS_GAME){
		Com_Printf("You must be in a level to record\n");
		return;
	}

	if (Cmd_Argc() == 1){
		// Find a file name to record to
		for (i = 0; i <= 9999; i++){
			Str_SPrintf(name, sizeof(name), "demos/demo%04i.dm2", i);
			if (!FS_FileExists(name))
				break;	// File doesn't exist
		}

		if (i == 10000){
			Com_Printf("Demos directory is full!\n");
			return;
		}
	}
	else {
		Str_SPrintf(name, sizeof(name), "demos/%s", Cmd_Argv(1));
		Str_DefaultFileExtension(name, sizeof(name), ".dm2");
	}

	// Open the demo file
	FS_OpenFile(name, FS_WRITE, &svs.demoFile);
	if (!svs.demoFile){
		Com_Printf("Couldn't open %s\n", name);
		return;
	}

	Com_Printf("Recording to %s\n", name);

	// Setup a buffer to catch all multicasts
	MSG_Init(&svs.demoMulticast, svs.demoMulticastBuffer, sizeof(svs.demoMulticastBuffer), false);

	// Write a single giant fake message with all the startup info
	MSG_Init(&msg, data, sizeof(data), false);

	// Send the server data
	MSG_WriteByte(&msg, SVC_SERVERDATA);
	MSG_WriteLong(&msg, PROTOCOL_VERSION);
	MSG_WriteLong(&msg, svs.spawnCount);
	MSG_WriteByte(&msg, 2);		// Demos are always attract loops (2 means server demo)
	MSG_WriteString(&msg, CVar_GetVariableString("fs_game"));
	MSG_WriteShort(&msg, -1);
	MSG_WriteString(&msg, sv.configStrings[CS_NAME]);

	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
		if (sv.configStrings[i][0]){
			MSG_WriteByte(&msg, SVC_CONFIGSTRING);
			MSG_WriteShort(&msg, i);
			MSG_WriteString(&msg, sv.configStrings[i]);
		}

	// Write it to the demo file
	Com_DPrintf("Signon message length: %i\n", msg.curSize);

	length = LittleLong(msg.curSize);
	FS_Write(svs.demoFile, &length, sizeof(length));
	FS_Write(svs.demoFile, msg.data, msg.curSize);

	// The rest of the demo file will be individual frames
}

/*
 ==================
 SV_ServerStopRecord_f

 Ends server demo recording
 ==================
*/
void SV_ServerStopRecord_f (){

	if (!svs.demoFile){
		Com_Printf("Not recording a demo\n");
		return;
	}

	FS_CloseFile(svs.demoFile);
	svs.demoFile = 0;

	Com_Printf("Stopped recording\n");
}

/*
 ==================
 SV_KillServer_f

 Kick everyone off, possibly in preparation for a new game
 ==================
*/
void SV_KillServer_f (){

	SV_Shutdown("Server was killed\n", false);
}

/*
 ==================
 SV_ServerCommand_f

 Let the game library handle a command
 ==================
*/
void SV_ServerCommand_f (){

	if (!ge){
		Com_Printf("No game loaded\n");
		return;
	}

	ge->ServerCommand();
}

/*
 ==================
 SV_ConSay_f
 ==================
*/
void SV_ConSay_f (){

	client_t	*client;
	char		text[1024], *p;
	int			i;

	if (!svs.initialized){
		Com_Printf("Server is not running\n");
		return;
	}

	if (Cmd_Argc () < 2)
		return;

	Str_Copy(text, "Console: ", sizeof(text));

	p = Cmd_Args();
	if (*p == '"'){
		p++;

		if (p[Str_Length(p)-1] == '"')
			p[Str_Length(p)-1] = 0;
	}

	Str_Append(text, p, sizeof(text));

	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		if (client->state != CS_SPAWNED)
			continue;
		
		SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}