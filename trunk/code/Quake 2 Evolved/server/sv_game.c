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
// sv_game.c - Interface to the game module
//


#include "server.h"


static game_import_t		gi;

game_export_t *				ge;


/*
 ==================
 SG_Unicast

 Sends the contents of the multicast buffer to a single client
 ==================
*/
static void SG_Unicast (edict_t *edict, bool reliable){

	int			e;
	client_t	*client;

	if (!edict)
		return;

	e = NUM_FOR_EDICT(edict);
	if (e < 1 || e > sv_maxClients->integerValue)
		return;

	client = svs.clients + (e-1);

	if (reliable)
		MSG_Write(&client->netChan.message, sv.multicast.data, sv.multicast.curSize);
	else
		MSG_Write(&client->datagram, sv.multicast.data, sv.multicast.curSize);

	MSG_Clear(&sv.multicast);
}

/*
 ==================
 SG_DPrintf

 Debug print to server console
 ==================
*/
static void SG_DPrintf (char *fmt, ...){

	char	message[1024];
	va_list	argPtr;

	va_start(argPtr, fmt);
	Str_VSPrintf(message, sizeof(message), fmt, argPtr);
	va_end(argPtr);

	Com_Printf("%s", message);
}

/*
 ==================
 SG_ClientPrintf

 Print to a single client
 ==================
*/
static void SG_ClientPrintf (edict_t *edict, int level, char *fmt, ...){

	char	message[1024];
	va_list	argPtr;
	int		e;

	if (edict){
		e = NUM_FOR_EDICT(edict);
		if (e < 1 || e > sv_maxClients->integerValue)
			Com_Error(ERR_DROP, "SVG_ClientPrintf: client = %i", e);
	}

	va_start(argPtr, fmt);
	Str_VSPrintf(message, sizeof(message), fmt, argPtr);
	va_end(argPtr);

	if (edict)
		SV_ClientPrintf(svs.clients + (e-1), level, "%s", message);
	else
		Com_Printf("%s", message);
}

/*
 ==================
 SG_CenterPrintf

 Center print to a single client
 ==================
*/
static void SG_CenterPrintf (edict_t *edict, char *fmt, ...){

	char	message[1024];
	va_list	argPtr;
	int		e;

	e = NUM_FOR_EDICT(edict);
	if (e < 1 || e > sv_maxClients->integerValue)
		return;

	va_start(argPtr, fmt);
	Str_VSPrintf(message, sizeof(message), fmt, argPtr);
	va_end(argPtr);

	MSG_WriteByte(&sv.multicast, SVC_CENTERPRINT);
	MSG_WriteString(&sv.multicast, message);
	SG_Unicast(edict, true);
}

/*
 ==================
 SG_Error

 Aborts the server with a game error
 ==================
*/
static void SG_Error (char *fmt, ...){

	char	message[1024];
	va_list	argPtr;

	va_start(argPtr, fmt);
	Str_VSPrintf(message, sizeof(message), fmt, argPtr);
	va_end(argPtr);

	Com_Error(ERR_DROP, "Game Error: %s", message);
}

/*
 ==================
 SG_SetModel

 Also sets mins and maxs for inline models
 ==================
*/
static void SG_SetModel (edict_t *edict, char *name){

	clipInlineModel_t	*model;

	if (!edict || !name)
		Com_Error(ERR_DROP, "SVG_SetModel: NULL parameter");

	edict->s.modelindex = SV_ModelIndex(name);

	// If it is an inline model, get the size information for it
	if (name[0] == '*'){
		model = CM_InlineModel(name);
		VectorCopy(model->mins, edict->mins);
		VectorCopy(model->maxs, edict->maxs);
		SV_LinkEdict(edict);
	}
}

/*
 ==================
 SG_ConfigString
 ==================
*/
static void SG_ConfigString (int index, char *string){

	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		Com_Error(ERR_DROP, "SVG_ConfigString: index = %i", index);

	if (!string)
		string = "";

	// Change the string in sv
	strcpy(sv.configStrings[index], string);

	if (sv.state != SS_LOADING){
		// Send the update to everyone
		MSG_Clear(&sv.multicast);
		MSG_WriteChar(&sv.multicast, SVC_CONFIGSTRING);
		MSG_WriteShort(&sv.multicast, index);
		MSG_WriteString(&sv.multicast, string);
		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	}
}

/*
 ==================
 SG_WriteChar
 ==================
*/
static void SG_WriteChar (int value){

	MSG_WriteChar(&sv.multicast, value);
}

/*
 ==================
 SG_WriteByte
 ==================
*/
static void SG_WriteByte (int value){

	MSG_WriteByte(&sv.multicast, value);
}

/*
 ==================
 SG_WriteShort
 ==================
*/
static void SG_WriteShort (int value){

	MSG_WriteShort(&sv.multicast, value);
}

/*
 ==================
 SG_WriteLong
 ==================
*/
static void SG_WriteLong (int value){

	MSG_WriteLong(&sv.multicast, value);
}

/*
 ==================
 SG_WriteFloat
 ==================
*/
static void SG_WriteFloat (float value){

	MSG_WriteFloat(&sv.multicast, value);
}

/*
 ==================
 SG_WriteString
 ==================
*/
static void SG_WriteString (char *string){

	MSG_WriteString(&sv.multicast, string);
}

/*
 ==================
 SG_WritePos
 ==================
*/
static void SG_WritePos (vec3_t vec){

	MSG_WritePos(&sv.multicast, vec);
}

/*
 ==================
 SG_WriteDir
 ==================
*/
static void SG_WriteDir (vec3_t vec){

	MSG_WriteDir(&sv.multicast, vec);
}

/*
 ==================
 SG_WriteAngle
 ==================
*/
static void SG_WriteAngle (float angle){

	MSG_WriteAngle(&sv.multicast, angle);
}

/*
 ==================
 SG_InPVS

 Also checks portalareas so that doors block sight
 ==================
*/
static bool SG_InPVS (vec3_t p1, vec3_t p2){

	int		leafNum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafNum = CM_PointLeafNum(p1);
	cluster = CM_LeafCluster(leafNum);
	area1 = CM_LeafArea(leafNum);
	mask = CM_ClusterPVS(cluster);

	leafNum = CM_PointLeafNum(p2);
	cluster = CM_LeafCluster(leafNum);
	area2 = CM_LeafArea(leafNum);

	if (!(mask[cluster >> 3] & (1 << (cluster & 7))))
		return false;

	if (!CM_AreasAreConnected(area1, area2))
		return false;		// A door blocks sight

	return true;
}

/*
 ==================
 SG_InPHS

 Also checks portalareas so that doors block sound
 ==================
*/
static bool SG_InPHS (vec3_t p1, vec3_t p2){

	int		leafNum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafNum = CM_PointLeafNum(p1);
	cluster = CM_LeafCluster(leafNum);
	area1 = CM_LeafArea(leafNum);
	mask = CM_ClusterPHS(cluster);

	leafNum = CM_PointLeafNum(p2);
	cluster = CM_LeafCluster(leafNum);
	area2 = CM_LeafArea(leafNum);

	if (!(mask[cluster >> 3] & (1 << (cluster & 7))))
		return false;

	if (!CM_AreasAreConnected(area1, area2))
		return false;		// A door blocks hearing

	return true;
}

/*
 ==================
 SG_StartSound
 ==================
*/
static void SG_StartSound (edict_t *edict, int channel, int sound, float volume, float attenuation, float timeOfs){

	if (!edict)
		return;

	SV_StartSound(NULL, edict, channel, sound, volume, attenuation, timeOfs);
}

/*
 ==================
 SG_DebugGraph

 Does nothing. Just a dummy function for the game server.
 ==================
*/
static void SG_DebugGraph (float value, int color){

}

/*
 ==================
 SG_ClearedAlloc

 HACK: stupid workaround because the game module uses different tag
 numbers
 ==================
*/
static void *SG_ClearedAlloc (int size, int tag){

	if (tag == 765 || tag == 766)
		tag = TAG_SERVER;

	Mem_ClearedAlloc(size, (memoryTag_t)tag);
}

/*
 ==================
 SG_Free

 HACK: stupid workaround
 ==================
*/
static void SG_Free (void *ptr){

	Mem_Free((const void *)ptr);
}

/*
 ==================
 SG_FreeAll

 HACK: stupid workaround
 ==================
*/
static void SG_FreeAll (int tag){

	Mem_FreeAll((memoryTag_t)tag, true);
}


// ============================================================================


/*
 ==================
 SG_SetupFramework

 TODO: Fix the errors and clean the pointer loading order?
 ==================
*/
static void SG_SetupFramework (){

	// Unload anything we have now
	if (ge)
		SG_Shutdown();

	// Load the pointers
	gi.unicast = SG_Unicast;
	gi.dprintf = SG_DPrintf;
	gi.cprintf = SG_ClientPrintf;
	gi.centerprintf = SG_CenterPrintf;
	gi.error = SG_Error;
	gi.setmodel = SG_SetModel;
	gi.configstring = SG_ConfigString;
	gi.WriteChar = SG_WriteChar;
	gi.WriteByte = SG_WriteByte;
	gi.WriteShort = SG_WriteShort;
	gi.WriteLong = SG_WriteLong;
	gi.WriteFloat = SG_WriteFloat;
	gi.WriteString = SG_WriteString;
	gi.WritePosition = SG_WritePos;
	gi.WriteDir = SG_WriteDir;
	gi.WriteAngle = SG_WriteAngle;
	gi.inPVS = SG_InPVS;
	gi.inPHS = SG_InPHS;
	gi.sound = SG_StartSound;
	gi.DebugGraph = SG_DebugGraph;

	gi.multicast = SV_Multicast;
	gi.bprintf = SV_BroadcastPrintf;
	gi.positioned_sound = SV_StartSound;
	gi.linkentity = SV_LinkEdict;
	gi.unlinkentity = SV_UnlinkEdict;
	gi.BoxEdicts = SV_AreaEdicts;
	gi.trace = SV_Trace;
	gi.pointcontents = SV_PointContents;
	gi.Pmove = PMove;

	gi.modelindex = SV_ModelIndex;
	gi.soundindex = SV_SoundIndex;
	gi.imageindex = SV_ImageIndex;

	gi.TagMalloc = SG_ClearedAlloc;
	gi.TagFree = SG_Free;
	gi.FreeTags = SG_FreeAll;

	gi.cvar = CVar_GameRegister;
	gi.cvar_set = CVar_GameSet;
	gi.cvar_forceset = CVar_GameForceSet;

	gi.argc = Cmd_Argc;
	gi.argv = Cmd_Argv;
	gi.args = Cmd_Args;
	gi.AddCommandString = Cmd_AppendText;

	gi.SetAreaPortalState = CM_SetAreaPortalState;
	gi.AreasConnected = CM_AreasAreConnected;
}

/*
 ==================
 SG_Init
 ==================
*/
void SG_Init (){

	char	path[MAX_QPATH];
	void	*(*GetSGameAPI) (void *);

	// Set up the framework
	SG_SetupFramework();

	// Extract the game module from a pack file
	if (sv_loadGame->integerValue){
		if (!FS_ExtractLibrary("game", path))
			Com_Error(ERR_FATAL, "Could not extract server game module");
	}
	else {
		if (!FS_ExtractLibrary("q2e_game", path))
			Com_Error(ERR_FATAL, "Could not extract server game module");
	}

	// Load the game module
	Com_Printf("Loading server game module\n");

	svs.gameLibHandle = Sys_LoadLibrary(path);
	if (!svs.gameLibHandle)
		Com_Error(ERR_FATAL, "Could not load server game module");

	// Get the game API
	GetSGameAPI = (void *(*)(void *))Sys_GetProcAddress(svs.gameLibHandle, "GetGameAPI");
	if (!GetSGameAPI)
		Com_Error(ERR_FATAL, "Could not get server game interface");

	ge = (game_export_t *)GetSGameAPI(&gi);

	if (ge->apiversion != GAME_API_VERSION)
		Com_Error(ERR_FATAL, "Server game interface version is %i, expected version %i", ge->apiversion, GAME_API_VERSION);

	// Initialize the game
	ge->Init();
}

/*
 ==================
 SG_Shutdown
 ==================
*/
void SG_Shutdown (){

	if (!svs.gameLibHandle)
		return;

	// Shutdown the game
	if (ge){
		ge->Shutdown();
		ge = NULL;
	}

	// Free all game allocations
	Mem_FreeAll(TAG_SERVER, true);

	// Free the game module
	Com_Printf("Unloading server game module\n");

	Sys_FreeLibrary(svs.gameLibHandle);
	svs.gameLibHandle = 0;
}