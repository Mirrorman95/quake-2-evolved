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
// sv_parse.c - Client message parsing
//


#include "server.h"


#define	MAX_STRING_CMDS				8


/*
 ==============================================================================

 CLIENT COMMAND EXECUTION

 ==============================================================================
*/


/*
 ==================
 SV_NewCommand

 Sends the first message from the server to a connected client. 
 This will be sent on the initial connection and upon each server load.
 ==================
*/
static void SV_NewCommand (){

	edict_t		*entity;
	char		name[MAX_PATH_LENGTH];
	int			playerNum;

	Com_DPrintf("SV_New_f() from %s\n", sv_client->name);

	if (sv_client->state != CS_CONNECTED){
		Com_DPrintf("SV_New_f() not valid. Already spawned\n");
		return;
	}

	// Demo servers just dump the file message
	if (sv.state == SS_DEMO){
		Str_SPrintf(name, sizeof(name), "demos/%s", sv.name);
		FS_OpenFile(name, FS_READ, &sv.demoFile);
		if (!sv.demoFile)
			Com_Error(ERR_DROP, "Couldn't open demo %s", name);

		return;
	}

	// SVC_SERVERDATA needs to go over for all types of servers to make
	// sure the protocol is right, and to set the game dir
	if (sv.state == SS_CINEMATIC || sv.state == SS_PIC)
		playerNum = -1;
	else
		playerNum = sv_client - svs.clients;

	// Send the server data
	MSG_WriteByte(&sv_client->netChan.message, SVC_SERVERDATA);
	MSG_WriteLong(&sv_client->netChan.message, PROTOCOL_VERSION);
	MSG_WriteLong(&sv_client->netChan.message, svs.spawnCount);
	MSG_WriteByte(&sv_client->netChan.message, sv.attractLoop);
	MSG_WriteString(&sv_client->netChan.message, CVar_GetVariableString("fs_game"));
	MSG_WriteShort(&sv_client->netChan.message, playerNum);
	MSG_WriteString(&sv_client->netChan.message, sv.configStrings[CS_NAME]);

	// Game server
	if (sv.state == SS_GAME){
		// Set up the entity for the client
		entity = EDICT_NUM(playerNum+1);
		entity->s.number = playerNum+1;
		sv_client->edict = entity;
		Mem_Fill(&sv_client->lastUserCmd, 0, sizeof(sv_client->lastUserCmd));

		// Begin fetching configstrings
		MSG_WriteByte(&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString(&sv_client->netChan.message, Str_VarArgs("cmd configstrings %i 0\n", svs.spawnCount));
	}
}

/*
 ==================
 SV_ConfigStringsCommand
 ==================
*/
static void SV_ConfigStringsCommand (){

	int		start;

	Com_DPrintf("SV_ConfigStrings_f() from %s\n", sv_client->name);

	if (sv_client->state != CS_CONNECTED){
		Com_DPrintf("SV_ConfigStrings_f() not valid. Already spawned\n");
		return;
	}

	// Handle the case of a level changing while a client was connecting
	if (Str_ToInteger(Cmd_Argv(1)) != svs.spawnCount){
		Com_DPrintf("SV_ConfigStrings_f() from different level\n");
		SV_NewCommand();
		return;
	}
	
	start = Str_ToInteger(Cmd_Argv(2));
	if (start < 0)
		start = 0;
	else if (start > MAX_CONFIGSTRINGS)
		start = MAX_CONFIGSTRINGS;

	// Write a packet full of data
	while (sv_client->netChan.message.curSize < MAX_MSGLEN/2 && start < MAX_CONFIGSTRINGS){
		if (sv.configStrings[start][0]){
			MSG_WriteByte(&sv_client->netChan.message, SVC_CONFIGSTRING);
			MSG_WriteShort(&sv_client->netChan.message, start);
			MSG_WriteString(&sv_client->netChan.message, sv.configStrings[start]);
		}
		start++;
	}

	// Send next command
	if (start == MAX_CONFIGSTRINGS){
		MSG_WriteByte(&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString(&sv_client->netChan.message, Str_VarArgs("cmd baselines %i 0\n", svs.spawnCount));
	}
	else {
		MSG_WriteByte(&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString(&sv_client->netChan.message, Str_VarArgs("cmd configstrings %i %i\n", svs.spawnCount, start));
	}
}

/*
 ==================
 SV_BaseLinesCommand
 ==================
*/
static void SV_BaseLinesCommand (){

	entity_state_t	*base, nullState;
	int				start;

	Com_DPrintf("SV_BaseLines_f() from %s\n", sv_client->name);

	if (sv_client->state != CS_CONNECTED){
		Com_DPrintf("SV_BaseLines_f() not valid. Already spawned\n");
		return;
	}
	
	// Handle the case of a level changing while a client was connecting
	if (Str_ToInteger(Cmd_Argv(1)) != svs.spawnCount){
		Com_DPrintf("SV_BaseLines_f() from different level\n");
		SV_NewCommand();
		return;
	}
	
	start = Str_ToInteger(Cmd_Argv(2));
	if (start < 0)
		start = 0;
	else if (start > MAX_EDICTS)
		start = MAX_EDICTS;

	Mem_Fill(&nullState, 0, sizeof(nullState));

	// Write a packet full of data
	while (sv_client->netChan.message.curSize < MAX_MSGLEN/2 && start < MAX_EDICTS){
		base = &sv.baselines[start];
		if (base->modelindex || base->sound || base->effects){
			MSG_WriteByte(&sv_client->netChan.message, SVC_SPAWNBASELINE);
			MSG_WriteDeltaEntity(&sv_client->netChan.message, &nullState, base, true, true);
		}
		start++;
	}

	// Send next command
	if (start == MAX_EDICTS){
		MSG_WriteByte(&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString(&sv_client->netChan.message, Str_VarArgs("precache %i\n", svs.spawnCount));
	}
	else {
		MSG_WriteByte(&sv_client->netChan.message, SVC_STUFFTEXT);
		MSG_WriteString(&sv_client->netChan.message, Str_VarArgs("cmd baselines %i %i\n", svs.spawnCount, start));
	}
}

/*
 ==================
 SV_BeginCommand
 ==================
*/
static void SV_BeginCommand (){

	Com_DPrintf("SV_Begin_f() from %s\n", sv_client->name);

	// Handle the case of a level changing while a client was connecting
	if (Str_ToInteger(Cmd_Argv(1)) != svs.spawnCount){
		Com_DPrintf("SV_Begin_f() from different level\n");
		SV_NewCommand();
		return;
	}

	sv_client->state = CS_SPAWNED;

	// Call the game begin function
	ge->ClientBegin(sv_player);

	Cmd_InsertFromDefer();
}

/*
 ==================
 SV_NextServerCommand

 A cinematic has completed or been aborted by a client, so move to the 
 next server
 ==================
*/
static void SV_NextServerCommand (){

	Com_DPrintf("SV_Nextserver_f() from %s\n", sv_client->name);

	if (Str_ToInteger(Cmd_Argv(1)) != svs.spawnCount){
		Com_DPrintf("SV_Nextserver_f() from different level\n");
		return;		// Leftover from last server
	}

	SV_NextServer();
}

/*
 ==================
 SV_DisconnectCommand

 The client is going to disconnect, so remove the connection immediately
 ==================
*/
static void SV_DisconnectCommand (){

	SV_DropClient(sv_client);	
}

/*
 ==================
 SV_NextDownloadCommand
 ==================
*/
static void SV_NextDownloadCommand (){

	byte	data[1024];
	int		length, percent;

	if (!sv_client->downloadFile)
		return;

	length = sv_client->downloadSize - sv_client->downloadOffset;
	if (length > sizeof(data))
		length = sizeof(data);

	length = FS_Read(sv_client->downloadFile, data, length);

	sv_client->downloadOffset += length;
	percent = sv_client->downloadOffset * 100/sv_client->downloadSize;

	MSG_WriteByte(&sv_client->netChan.message, SVC_DOWNLOAD);
	MSG_WriteShort(&sv_client->netChan.message, length);
	MSG_WriteByte(&sv_client->netChan.message, percent);
	MSG_Write(&sv_client->netChan.message, data, length);

	if (sv_client->downloadOffset == sv_client->downloadSize){
		FS_CloseFile(sv_client->downloadFile);
		sv_client->downloadFile = 0;
	}
}

/*
 ==================
 SV_DownloadCommand
 ==================
*/
static void SV_DownloadCommand (){

	char	*name;
	int		offset = 0;

	name = Cmd_Argv(1);

	if (Cmd_Argc() > 2)
		offset = Str_ToInteger(Cmd_Argv(2));	// Downloaded offset

	if (Str_FindChar(name, '..') || !Str_FindChar(name, '/') || name[0] == '.' || name[0] == '/' || !sv_allowDownload->integerValue){
		MSG_WriteByte(&sv_client->netChan.message, SVC_DOWNLOAD);
		MSG_WriteShort(&sv_client->netChan.message, -1);
		MSG_WriteByte(&sv_client->netChan.message, 0);
		return;
	}

	if (sv_client->downloadFile){
		FS_CloseFile(sv_client->downloadFile);
		sv_client->downloadFile = 0;
	}

	sv_client->downloadSize = FS_OpenFile(name, FS_READ, &sv_client->downloadFile);
	
	if (!sv_client->downloadFile || !sv_client->downloadSize){
		Com_DPrintf("Couldn't download %s to %s\n", name, sv_client->name);

		if (sv_client->downloadFile){
			FS_CloseFile(sv_client->downloadFile);
			sv_client->downloadFile = 0;
		}

		MSG_WriteByte(&sv_client->netChan.message, SVC_DOWNLOAD);
		MSG_WriteShort(&sv_client->netChan.message, -1);
		MSG_WriteByte(&sv_client->netChan.message, 0);
		return;
	}

	sv_client->downloadOffset = offset;
	if (sv_client->downloadOffset > sv_client->downloadSize)
		sv_client->downloadOffset = sv_client->downloadSize;

	SV_NextDownloadCommand();

	Com_DPrintf("Downloading %s to %s...\n", name, sv_client->name);
}

/*
 ==================
 SV_InfoCommand

 Dumps the server info string
 ==================
*/
static void SV_InfoCommand (){

	Com_Printf("Server info settings:\n");
	Com_Printf("---------------------\n");

	Info_Print(CVar_InfoString(CVAR_SERVERINFO));
}

/*
 ==================
 SV_ExecuteClientCommand

 TODO: grab sv_client here and pass it down?
 ==================
*/
static void SV_ExecuteClientCommand (const char *command){

	const char	*cmd;

	Com_DPrintf("Client command from %s" S_COLOR_DEFAULT ": %s\n", sv_client->name, command);

	// Execute the client command
	Cmd_TokenizeString(command);

	if (!Cmd_Argc())
		return;	// No tokens

	cmd = Cmd_Argv(0);

	// Check for a server level command
	if (!Str_ICompare(cmd, "new")){
		SV_NewCommand();
		return;
	}

	if (!Str_ICompare(cmd, "configstrings")){
		SV_ConfigStringsCommand();
		return;
	}

	if (!Str_ICompare(cmd, "baselines")){
		SV_BaseLinesCommand();
		return;
	}

	if (!Str_ICompare(cmd, "begin")){
		SV_BeginCommand();
		return;
	}

	if (!Str_ICompare(cmd, "nextserver")){
		SV_NextServerCommand();
		return;
	}

	if (!Str_ICompare(cmd, "disconnect")){
		SV_DisconnectCommand();
		return;
	}

	if (!Str_ICompare(cmd, "nextdl")){
		SV_NextDownloadCommand();
		return;
	}

	if (!Str_ICompare(cmd, "download")){
		SV_DownloadCommand();
		return;
	}

	if (!Str_ICompare(cmd, "info")){
		SV_InfoCommand();
		return;
	}

	// Pass unknown client commands to the game module if allowed
	if (sv.state == SS_GAME)
		ge->ClientCommand(sv_player);

	Com_DPrintf("Client command from %s" S_COLOR_DEFAULT " ignored: %s\n", sv_client->name, cmd);
}


// ============================================================================


/*
 ==================
 SV_ClientThink

 TODO: move to sv_main?
 ==================
*/
static void SV_ClientThink (client_t *client, usercmd_t *userCmd){

	client->commandMsec -= userCmd->msec;

	if (client->commandMsec < 0 && sv_enforceTime->integerValue){
		Com_DPrintf("SV_ClientThink: commandMsec underflow from %s\n", client->name);
		return;
	}

	ge->ClientThink(client->edict, userCmd);
}


/*
 ==============================================================================

 CLIENT MESSAGE PARSING

 ==============================================================================
*/


/*
 ==================
 SV_ParseUserInfo
 ==================
*/
static void SV_ParseUserInfo (client_t *client){

	char *string;

	// Parse the string
	string = MSG_ReadString(&net_message);
	Str_Copy(client->userInfo, string, sizeof(client->userInfo));

	SV_UserInfoChanged(client);
}

/*
 ==================
 SV_ParseMove
 ==================
*/
static void SV_ParseMove (client_t *client, bool moveIssued){

	usercmd_t	nullUserCmd, oldestUserCmd, oldUserCmd, newUserCmd;
	int			checksum, calculatedChecksum, checksumIndex;
	int			dropped, lastFrame;

	if (moveIssued)
		return;		// Someone is trying to cheat...
	moveIssued = true;

	checksumIndex = net_message.readCount;
	checksum = MSG_ReadByte(&net_message);
	lastFrame = MSG_ReadLong(&net_message);
	if (lastFrame != client->lastFrame){
		client->lastFrame = lastFrame;
		if (client->lastFrame > 0)
			client->frameLatency[client->lastFrame & (LATENCY_COUNTS-1)] = svs.realTime - client->frames[client->lastFrame & UPDATE_MASK].sentTime;
	}

	// Parse all the user commands
	Mem_Fill(&nullUserCmd, 0, sizeof(nullUserCmd));

	MSG_ReadDeltaUserCmd(&net_message, &nullUserCmd, &oldestUserCmd);
	MSG_ReadDeltaUserCmd(&net_message, &oldestUserCmd, &oldUserCmd);
	MSG_ReadDeltaUserCmd(&net_message, &oldUserCmd, &newUserCmd);

	if (client->state != CS_SPAWNED){
		client->lastFrame = -1;
		return;
	}

	// If the checksum fails, ignore the rest of the packet
	calculatedChecksum = Com_BlockSequenceCRCByte(net_message.data + checksumIndex + 1, net_message.readCount - checksumIndex - 1, client->netChan.incomingSequence);
	if (calculatedChecksum != checksum){
		Com_DPrintf(S_COLOR_RED "Failed command checksum for %s (%i != %i)\n", client->name, calculatedChecksum, checksum);
		return;
	}

	if (!com_paused->integerValue){
		dropped = client->netChan.dropped;
		if (dropped < 20){
			while (dropped > 2){
				SV_ClientThink(client, &client->lastUserCmd);
				dropped--;
			}

			if (dropped > 1)
				SV_ClientThink(client, &oldestUserCmd);

			if (dropped > 0)
				SV_ClientThink(client, &oldUserCmd);

		}

		SV_ClientThink(client, &newUserCmd);
	}

	client->lastUserCmd = newUserCmd;
}

/*
 ==================
 SV_ParseStringCmd
 ==================
*/
static void SV_ParseStringCmd (client_t *client, int stringCmdCount){

	char	*string;

	// Parse the string
	string = MSG_ReadString(&net_message);

	// Malicious users may try using too many string commands
	if (++stringCmdCount < MAX_STRING_CMDS){
		SV_ExecuteClientCommand(string);
	}

	if (client->state == CS_ZOMBIE)
		return;		// Disconnect command
}

/*
 ==================
 SV_ParseClientMessage

 The current net_message is parsed for the given client
 ==================
*/
void SV_ParseClientMessage (client_t *client){

	bool		moveIssued;
	int			stringCmdCount;
	int			cmd;

	sv_client = client;
	sv_player = sv_client->edict;

	// Only allow one move command
	moveIssued = false;
	stringCmdCount = 0;

	// Parse the message
	while (1){
		if (net_message.readCount > net_message.curSize){
			Com_Printf(S_COLOR_RED "SV_ParseClientMessage: bad client message\n");
			SV_DropClient(client);
			return;
		}	

		// Parse the client op
		cmd = MSG_ReadByte(&net_message);

		if (cmd == -1)
			break;

		switch (cmd){
		case CLC_NOP:

			break;
		case CLC_USERINFO:
			SV_ParseUserInfo(client);
			break;
		case CLC_MOVE:
			SV_ParseMove(client, moveIssued);
			break;
		case CLC_STRINGCMD:	
			SV_ParseStringCmd(client, stringCmdCount);
			break;
		default:
			Com_Printf(S_COLOR_RED "SV_ParseClientMessage: illegible client message\n");
			SV_DropClient(client);
			return;
		}
	}
}