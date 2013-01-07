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
// sv_spawn.c - server spawning code
//

// TODO:
// - some variables are forced here but i had to use the "other" force functions,
// later on we should get the variable from SV_Init and change the force
// functions into "CVar_Set..."


#include "server.h"


/*
 ==============================================================================

 INDEX FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 SV_FindIndex
 ==================
*/
static int SV_FindIndex (const char *name, int start, int max){

	int		i;
	
	if (!name || !name[0])
		return 0;

	for (i = 1; i < max && sv.configStrings[start+i][0]; i++){
		if (!Str_Compare(sv.configStrings[start+i], name))
			return i;
	}

	if (i == max)
		Com_Error(ERR_DROP, "SV_FindIndex: overflow");

	Str_Copy(sv.configStrings[start+i], name, sizeof(sv.configStrings[start+i]));

	if (sv.state != SS_LOADING){
		// Send the update to everyone
		MSG_Clear(&sv.multicast);
		MSG_WriteChar(&sv.multicast, SVC_CONFIGSTRING);
		MSG_WriteShort(&sv.multicast, start+i);
		MSG_WriteString(&sv.multicast, name);
		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	}

	return i;
}

/*
 ==================
 SV_ModelIndex
 ==================
*/
int SV_ModelIndex (char *name){

	return SV_FindIndex(name, CS_MODELS, MAX_MODELS);
}

/*
 ==================
 SV_SoundIndex
 ==================
*/
int SV_SoundIndex (char *name){

	return SV_FindIndex(name, CS_SOUNDS, MAX_SOUNDS);
}

/*
 ==================
 SV_ImageIndex
 ==================
*/
int SV_ImageIndex (char *name){

	return SV_FindIndex(name, CS_IMAGES, MAX_IMAGES);
}


/*
 ==============================================================================

 SERVER SPAWNING

 ==============================================================================
*/


/*
 ==================
 SV_CreateBaseLine

 Entity baselines are used to compress the update messages to the 
 clients. Only the fields that differ from the baseline will be 
 transmitted.
 ==================
*/
static void SV_CreateBaseLine (){

	edict_t	*edict;
	int		e;	

	for (e = 1; e < ge->num_edicts; e++){
		edict = EDICT_NUM(e);
		if (!edict->inuse)
			continue;
		if (!edict->s.modelindex && !edict->s.sound && !edict->s.effects)
			continue;

		edict->s.number = e;

		// Take current state as baseline
		VectorCopy(edict->s.origin, edict->s.old_origin);
		sv.baselines[e] = edict->s;
	}
}

/*
 ==================
 SV_CheckForSaveGame
 ==================
*/
static void SV_CheckForSaveGame (){

	serverState_t	previousState;
	char			name[MAX_PATH_LENGTH];
	int				i;

	if (sv_noReload->integerValue)
		return;

	if (CVar_GetVariableBool("deathmatch"))
		return;

	Str_SPrintf(name, sizeof(name), "save/current/%s.sav", sv.name);
	if (!FS_FileExists(name))
		return;		// No savegame

	SV_ClearWorld();

	// Get configstrings and areaportals
	SV_ReadLevelFile();

	if (!sv.loadGame){
		// Coming back to a level after being in a different level, so 
		// run it for ten seconds
		previousState = sv.state;

		sv.state = SS_LOADING;			
		for (i = 0; i < 100; i++)
			ge->RunFrame();

		sv.state = previousState;		
	}
}

/*
 ==================
 SV_SpawnServer

 Change the server to a new level, taking all connected clients along with it
 ==================
*/
static void SV_SpawnServer (const char *levelName, const char *spawnPoint, bool attractLoop, bool loadGame, serverState_t serverState){

	uint	checksum;
	int		i;

	Com_Printf("-------- Server Initialization --------\n");
	Com_Printf("Map: %s\n", levelName);
	Com_Printf("Cheats are %s\n", (CVar_GetVariableBool("cheats")) ? "enabled" : "disabled");

	// Make sure we're not paused
	CVar_ForceSet("paused", "0");

	// Load client assets
	CL_MapLoading();

	// Initialize the game for the first time
	if (sv.state == SS_DEAD && !loadGame)
		SV_InitGame();

	// Broadcast that the server has changed
	SV_BroadcastCommand("changing\n");
	if (serverState == SS_GAME)
		SV_SendClientMessages();

	svs.spawnCount++;		// Any partially connected client will be restarted
	svs.realTime = 0;

	// Update server state
	sv.state = SS_DEAD;
	Com_SetServerState(sv.state);

	if (sv.demoFile)
		FS_CloseFile(sv.demoFile);

	// Wipe the entire serverActive_t structure
	Mem_Fill(&sv, 0, sizeof(sv));

	sv.attractLoop = attractLoop;
	sv.loadGame = loadGame;
	Str_Copy(sv.name, levelName, sizeof(sv.name));
	Str_Copy(sv.configStrings[CS_NAME], levelName, sizeof(sv.configStrings[CS_NAME]));
	sv.time = 1000;

	if (CVar_GetVariableBool("deathmatch")){
		Str_SPrintf(sv.configStrings[CS_AIRACCEL], sizeof(sv.configStrings[CS_AIRACCEL]), "%g", sv_airAccelerate->floatValue);
		pm_airAccelerate = sv_airAccelerate->floatValue;
	}
	else {
		Str_SPrintf(sv.configStrings[CS_AIRACCEL], sizeof(sv.configStrings[CS_AIRACCEL]), "0");
		pm_airAccelerate = 0;
	}

	MSG_Init(&sv.multicast, sv.multicastBuffer, sizeof(sv.multicastBuffer), false);

	// Leave slots at start for clients only
	for (i = 0; i < sv_maxClients->integerValue; i++){
		// Needs to reconnect
		if (svs.clients[i].state > CS_CONNECTED)
			svs.clients[i].state = CS_CONNECTED;

		svs.clients[i].lastFrame = -1;
	}

	if (serverState == SS_GAME){
		Str_SPrintf(sv.configStrings[CS_MODELS + 1], sizeof(sv.configStrings[CS_MODELS + 1]), "maps/%s.bsp", levelName);	// FIXME!!!
		sv.models[1] = CM_LoadMap(sv.configStrings[CS_MODELS + 1], false, &checksum);
	}
	else {
		Str_SPrintf(sv.configStrings[CS_MODELS + 1], sizeof(sv.configStrings[CS_MODELS + 1]), "%s", levelName);
		sv.models[1] = CM_LoadMap(NULL, false, &checksum);	// No real map
	}
	Str_SPrintf(sv.configStrings[CS_MAPCHECKSUM], sizeof(sv.configStrings[CS_MAPCHECKSUM]), "%i", checksum);

	// Clear physics interaction links
	SV_ClearWorld();

	for (i = 1; i < CM_NumInlineModels(); i++){
		Str_SPrintf(sv.configStrings[CS_MODELS + 1 + i], sizeof(sv.configStrings[CS_MODELS + 1 + i]), "*%i", i);
		sv.models[i+1] = CM_LoadInlineModel(sv.configStrings[CS_MODELS + 1 + i]);
	}

	// Spawn the rest of the entities on the map

	// Precache and static commands can be issued during map 
	// initialization
	sv.state = SS_LOADING;
	Com_SetServerState(sv.state);

	// Load and spawn all other entities
	ge->SpawnEntities(sv.name, CM_GetEntityString(), (char *)spawnPoint);

	// Run two frames to allow everything to settle
	ge->RunFrame();
	ge->RunFrame();

	// All precaches are complete
	sv.state = serverState;
	Com_SetServerState(sv.state);

	// Create a baseline for more efficient communications
	SV_CreateBaseLine();

	// Check for a saved game
	SV_CheckForSaveGame();

	if (serverState == SS_GAME)
		Cmd_CopyToDefer();

	CVar_SetVariableString("sv_mapName", levelName, true);
	CVar_SetVariableInteger("sv_mapChecksum", checksum, true);

	Com_Printf("---------------------------------------\n");
}

/*
 ==================
 SV_GetLatchedVariables

 TODO: change these to CVar_UpdateLatched when we can grab the
 variables
 ==================
*/
static void SV_GetLatchedVariables (){

	CVar_GetVariableBool("cheats");
	CVar_GetVariableInteger("skill");
	CVar_GetVariableBool("coop");
	CVar_GetVariableBool("deathmatch");
	CVar_GetVariableInteger("maxclients");
	CVar_GetVariableFloat("sv_airAccelerate");
}

/*
 ==================
 SV_InitGame

 A brand new game has been started
 ==================
*/
void SV_InitGame (){

	int		e;
	edict_t	*edict;

	// If the server was running, cause any connected clients to
	// reconnect
	if (svs.initialized)
		SV_Shutdown("Server restarted\n", true);
	else
		CL_Drop();

	// Get any latched variable changes
	SV_GetLatchedVariables();

	// Restart file system
	FS_Restart();

	if (CVar_GetVariableBool("coop") && CVar_GetVariableInteger("deathmatch")){
		Com_Printf("'deathmatch' and 'coop' are both set, disabling 'coop'\n");
		CVar_SetVariableBool("coop", false, true);
	}

	// Dedicated servers can't be singleplayer and are usually DM so
	// unless they explicity set coop, force it to deathmatch
	if (com_dedicated->integerValue){
		if (!CVar_GetVariableBool("coop"))
			CVar_SetVariableInteger("deathmatch", 1, true);
	}

	// Clamp max clients to the appropriate game mode
	if (CVar_GetVariableBool("deathmatch")){
		if (sv_maxClients->integerValue <= 1)
			CVar_SetVariableInteger("maxclients", 8, true);
		else if (sv_maxClients->integerValue > MAX_CLIENTS)
			CVar_SetVariableInteger("maxclients", MAX_CLIENTS, true);
	}
	else if (CVar_GetVariableBool("coop")){
		if (sv_maxClients->integerValue <= 1)
			CVar_SetVariableInteger("maxclients", 4, true);
		else if (sv_maxClients->integerValue > 8)
			CVar_SetVariableInteger("maxclients", 8, true);
	}
	else	// Single player
		CVar_SetVariableInteger("maxclients", 1, true);

	// Now active
	svs.initialized = true;

	svs.spawnCount = rand();

	// Allocate the client slots
	svs.clients = (client_t *)Mem_ClearedAlloc(sv_maxClients->integerValue * sizeof(client_t), TAG_COMMON);

	// Allocate the client entity slots
	svs.numClientEntities = sv_maxClients->integerValue * UPDATE_BACKUP * 64;
	svs.clientEntities = (entity_state_t*)Mem_ClearedAlloc(svs.numClientEntities * sizeof(entity_state_t), TAG_COMMON);

	// Send a heartbeat immediately
	svs.lastHeartbeat = -9999999;

	// Initialize game module
	SG_Init();

	// Set up the client entity indices
	for (e = 0; e < sv_maxClients->integerValue; e++){
		edict = EDICT_NUM(e+1);
		edict->s.number = e+1;
		svs.clients[e].edict = edict;
	}

	// Set the com_serverRunning variable
	CVar_SetBool(com_serverRunning, true);
}

/*
 ==================
 SV_Map

 The full syntax is:

 map [*]<map><$startspot><+nextserver>

 Command from the console or progs.
 Map can also be a .cin, .pcx, or .dm2 file.
 Nextserver is used to allow a cinematic to play, then proceed to 
 another level:

 map ntro.cin+base1
 ==================
*/
void SV_Map (const char *levelString, bool attractLoop, bool loadGame){

	const char *gameDir;
	char		server[MAX_PATH_LENGTH], spawnPoint[MAX_PATH_LENGTH];
	char		extension[8], *ch;

	Str_Copy(server, levelString, sizeof(server));

	// If there is a + in the map, set sv_nextServer to the remainder
	ch = Str_FindChar(server, '+');
	if (ch){
		*ch = 0;
		CVar_SetVariableString("sv_nextServer", Str_VarArgs("gamemap \"%s\"", ch+1), false);
	}
	else
		CVar_SetVariableString("sv_nextServer", "", false);

	// Special hack for end game screen in coop mode
	if (CVar_GetVariableInteger("coop") && !Str_ICompare(server, "victory.pcx")){
		gameDir = CVar_GetVariableString("fs_game");

		if (!Str_ICompare(gameDir, "baseq2"))
			CVar_SetVariableString("sv_nextServer", "gamemap \"*base1\"", false);
		else if (!Str_ICompare(gameDir, "xatrix"))
			CVar_SetVariableString("sv_nextServer", "gamemap \"*xswamp\"", false);
		else if (!Str_ICompare(gameDir, "rogue"))
			CVar_SetVariableString("sv_nextServer", "gamemap \"*rmine1\"", false);
	}

	// If there is a $, use the remainder as a spawn point
	ch = Str_FindChar(server, '$');
	if (ch){
		*ch = 0;
		Str_Copy(spawnPoint, ch+1, sizeof(spawnPoint));
	}
	else
		spawnPoint[0] = 0;

	// Skip the end-of-unit flag if necessary
	if (server[0] == '*')
		Str_Copy(server, server+1, sizeof(server));

	Com_FileExtension(server, extension, sizeof(extension));

	if (!Str_ICompare(extension, "pcx"))
		SV_SpawnServer(server, spawnPoint, attractLoop, loadGame, SS_PIC);
	else if (!Str_ICompare(extension, "cin"))
		SV_SpawnServer(server, spawnPoint, attractLoop, loadGame, SS_CINEMATIC);
	else if (!Str_ICompare(extension, "dm2"))
		SV_SpawnServer(server, spawnPoint, attractLoop, loadGame, SS_DEMO);
	else
		SV_SpawnServer(server, spawnPoint, attractLoop, loadGame, SS_GAME);

	SV_BroadcastCommand("reconnect\n");
}