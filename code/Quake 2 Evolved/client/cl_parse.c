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
// cl_parse.c - Server message parsing
//


#include "client.h"


char *						svc_strings[256] = {
	"SVC_BAD",
	"SVC_MUZZLEFLASH",
	"SVC_MUZZLEFLASH2",
	"SVC_TEMP_ENTITY",
	"SVC_LAYOUT",
	"SVC_INVENTORY",
	"SVC_NOP",
	"SVC_DISCONNECT",
	"SVC_RECONNECT",
	"SVC_SOUND",
	"SVC_PRINT",
	"SVC_STUFFTEXT",
	"SVC_SERVERDATA",
	"SVC_CONFIGSTRING",
	"SVC_SPAWNBASELINE",	
	"SVC_CENTERPRINT",
	"SVC_DOWNLOAD",
	"SVC_PLAYERINFO",
	"SVC_PACKETENTITIES",
	"SVC_DELTAPACKETENTITIES",
	"SVC_FRAME"
};


/*
 ==============================================================================

 SERVER MESSAGE PARSING

 ==============================================================================
*/


/*
 ==================
 CL_ParseDisconnect
 ==================
*/
static void CL_ParseDisconnect (){

	Com_Error(ERR_DISCONNECT, "Server disconnected");
}

/*
 ==================
 CL_ParseReconnect
 ==================
*/
static void CL_ParseReconnect (){

	// Close download
	if (cls.downloadFile){
		FS_CloseFile(cls.downloadFile);
		cls.downloadFile = 0;
	}

	Str_Copy(cls.serverMessage, "", sizeof(cls.serverMessage));

	// CL_CheckForResend will fire immediately
	cls.state = CA_CONNECTING;

	cls.connectTime = -99999;
	cls.connectCount = 0;

	Com_Printf("Server disconnected, reconnecting...\n");
}

/*
 ==================
 CL_ParseStuffText
 ==================
*/
static void CL_ParseStuffText (){

	const char	*string;

	string = MSG_ReadString(&net_message);
	Cmd_AppendText(string);

	Com_DPrintf("StuffText: %s\n", string);
}

/*
 ==================
 CL_ParsePrint
 ==================
*/
static void CL_ParsePrint (){

	const char	*string;
	char		*ch, name[64];
	int			i;

	// Parse the message
	i = MSG_ReadByte(&net_message);
	string = MSG_ReadString(&net_message);

	if (i == PRINT_CHAT){
		S_PlayLocalSound(S_RegisterSound("misc/talk.wav", 0));

		ch = Str_FindChar(string, ':');
		if (ch){
			Str_Copy(name, string, sizeof(name));
			name[ch - string] = 0;
			Com_Printf("%s:" S_COLOR_GREEN "%s", name, ch + 1);
		}
		else
			Com_Printf(S_COLOR_GREEN "%s", string);
	}
	else
		Com_Printf("%s", string);
}

/*
 ==================
 CL_ParseCenterPrint

 Called for important messages that should stay in the center of the 
 screen for a few moments
 ==================
*/
static void CL_ParseCenterPrint (){

	const char	*string;
	char		line[64];
	int			length;
	int			i, j;

	// Parse the message
	string = MSG_ReadString(&net_message);

	Str_Copy(cl.centerPrint, string, sizeof(cl.centerPrint));
	cl.centerPrintTime = cl.time;

	// Echo it to the console
	Com_Printf("\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

	do {
		// Scan the width of the line
		for (length = 0; length < 40; length++){
			if (string[length] == '\n' || !string[length])
				break;
		}

		for (i = 0; i < (40 - length) / 2; i++)
			line[i] = ' ';

		for (j = 0; j < length; j++)
			line[i++] = string[j];
		
		line[i] = '\n';
		line[i + 1] = 0;

		Com_Printf("%s", line);

		while (*string && *string != '\n')
			string++;

		if (!*string)
			break;

		string++;	// Skip the \n
	} while (1);

	Com_Printf("\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

	Con_ClearNotify();
}

/*
 ==================
 CL_ParseDownload

 A download message has been received from the server
 ==================
*/
static void CL_ParseDownload (){

	int		size, percent;

	// Parse the data
	size = MSG_ReadShort(&net_message);
	percent = MSG_ReadByte(&net_message);

	if (!cls.downloadFile){
		Com_Printf("Unwanted download received. Ignored\n");

		net_message.readCount += size;
		return;
	}

	if (size == -1){
		Com_Printf("Server does not have this file\n");

		if (cls.downloadFile){
			// If here, we tried to resume a file but the server said no
			FS_CloseFile(cls.downloadFile);
			cls.downloadFile = 0;
		}

		CL_RequestNextDownload();
		return;
	}

	cls.downloadBytes += size;
	cls.downloadPercent = percent;

	// Write the data
	FS_Write(cls.downloadFile, net_message.data + net_message.readCount, size);
	net_message.readCount += size;

	if (percent == 100){
		// Finished download
		FS_CloseFile(cls.downloadFile);
		cls.downloadFile = 0;

		// Rename the temp file to its final name
		FS_RenameFile(cls.downloadTempName, cls.downloadName);

		// Get another file if needed
		CL_RequestNextDownload();
		return;
	}

	// Request next block
	MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print(&cls.netChan.message, "nextdl");
}

/*
 ==================
 CL_ParseServerData
 ==================
*/
static void CL_ParseServerData (){

	char	*string;

	Com_DPrintf("Server data packet received\n");

	// Make sure any stuffed commands are done
	Cmd_ExecuteBuffer();

	// If playing a cinematic, stop it
	CL_StopCinematic();

	// Clear the client state
	CL_ClearState();

	// Parse protocol version number
	cls.serverProtocol = MSG_ReadLong(&net_message);

	// BIG HACK to let old demos continue to work
	if (Com_ServerState() && PROTOCOL_VERSION == 34)
		;
	else if (cls.serverProtocol != PROTOCOL_VERSION)
		Com_Error(ERR_DROP, "Server returned version %i, not %i", cls.serverProtocol, PROTOCOL_VERSION);

	cl.serverCount = MSG_ReadLong(&net_message);
	cl.demoPlayback = MSG_ReadByte(&net_message);

	// Parse game directory
	string = MSG_ReadString(&net_message);
	if (string[0]){
		if (!Str_ICompare(string, "baseq2") || !Str_ICompare(string, "ctf") || !Str_ICompare(string, "rogue") || !Str_ICompare(string, "xatrix"))
			cl.gameMod = false;
		else
			cl.gameMod = true;

		Str_Copy(cl.gameDir, string, sizeof(cl.gameDir));
	}
	else{
		cl.gameMod = false;
		Str_Copy(cl.gameDir, "baseq2", sizeof(cl.gameDir));
	}

	// Parse client entity number
	cl.clientNum = MSG_ReadShort(&net_message) + 1;

	// Parse the full level name
	string = MSG_ReadString(&net_message);

	// Change the game directory if needed
	if (Str_ICompare(cl.gameDir, CVar_GetVariableString("fs_game"))){
		CVar_ForceSet("fs_game", cl.gameDir);

		// Restart file system
		FS_Restart();
	}

	if (!cl.clientNum){
		// Restart the subsystems
		CL_ShutdownAll();
		CL_InitAll();

		// Playing a cinematic or showing a pic, not a level
		cls.state = CA_ACTIVE;
		cls.loading = false;

		CL_PlayCinematic(string);
	}
}

/*
 ==================
 CL_ParseConfigString
 ==================
*/
static void CL_ParseConfigString (){

	int			index;
	const char	*string;
	char		name[MAX_PATH_LENGTH];

	// Parse the string
	index = MSG_ReadShort(&net_message);
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		Com_Error(ERR_DROP, "CL_ParseConfigstring: index = %i", index);

	string = MSG_ReadString(&net_message);
	strcpy(cl.configStrings[index], string);

	// Do something appropriate 
	if (index == CS_MAXCLIENTS)
		cl.multiPlayer = (cl.configStrings[index][0] && Str_ICompare(cl.configStrings[index], "1"));
	if (index >= CS_LIGHTS && index < CS_LIGHTS + MAX_LIGHTSTYLES)
		CL_SetLightStyle(index - CS_LIGHTS);
	else if (index == CS_CDTRACK){
		if (cls.state <= CA_LOADING)
			return;

		CL_PlayBackgroundTrack();
	}
	else if (index >= CS_MODELS && index < CS_MODELS + MAX_MODELS){
		if (cls.state <= CA_LOADING)
			return;

		cl.media.gameModels[index - CS_MODELS] = R_RegisterModel(cl.configStrings[index]);

		if (cl.configStrings[index][0] == '*')
			cl.media.gameCModels[index - CS_MODELS] = CM_LoadInlineModel(cl.configStrings[index]);
		else
			cl.media.gameCModels[index - CS_MODELS] = NULL;
	}
	else if (index >= CS_SOUNDS && index < CS_SOUNDS + MAX_SOUNDS){
		if (cls.state <= CA_LOADING)
			return;

		cl.media.gameSounds[index - CS_SOUNDS] = S_RegisterSound(cl.configStrings[index], 0);
	}
	else if (index >= CS_IMAGES && index < CS_IMAGES + MAX_IMAGES){
		if (cls.state <= CA_LOADING)
			return;

		if (!Str_FindChar(cl.configStrings[index], '/'))
			Str_SPrintf(name, sizeof(name), "pics/%s", cl.configStrings[index]);
		else
			Com_StripExtension(cl.configStrings[index], name, sizeof(name));

		cl.media.gameMaterials[index - CS_IMAGES] = R_RegisterMaterialNoMip(name);
	}
	else if (index >= CS_PLAYERSKINS && index < CS_PLAYERSKINS + MAX_CLIENTS){
		if (cls.state <= CA_LOADING)
			return;

		CL_LoadClientInfo(&cl.clientInfo[index - CS_PLAYERSKINS], cl.configStrings[index]);
	}
}

/*
 ==================
 CL_ParseLayout
 ==================
*/
static void CL_ParseLayout (){

	const char	*string;

	// Parse the string
	string = MSG_ReadString(&net_message);
	Str_Copy(cl.layout, string, sizeof(cl.layout));
}

/*
 ==================
 CL_ParseInventory
 ==================
*/
static void CL_ParseInventory (){

	int		i;

	for (i = 0; i < MAX_ITEMS; i++)
		cl.inventory[i] = MSG_ReadShort(&net_message);
}

/*
 ==================
 CL_ParseStartSound
 ==================
*/
static void CL_ParseStartSound (){

    vec3_t  originVec;
	float	*origin;
	int		flags, sound;
	int		entNum, entChannel, timeOfs;
    float 	volume, attenuation;

	flags = MSG_ReadByte(&net_message);
	sound = MSG_ReadByte(&net_message);

    if (flags & SND_VOLUME)
		volume = MSG_ReadByte(&net_message) / 255.0f;
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
    if (flags & SND_ATTENUATION)
		attenuation = MSG_ReadByte(&net_message) / 64.0f;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;	

    if (flags & SND_OFFSET)
		timeOfs = MSG_ReadByte(&net_message);
	else
		timeOfs = 0;

	if (flags & SND_ENT){
		// Entity relative
		entChannel = MSG_ReadShort(&net_message); 
		entNum = entChannel >> 3;
		if (entNum < 0 || entNum >= MAX_EDICTS)
			Com_Error(ERR_DROP, "CL_ParseStartSound: entNum = %i", entNum);

		entChannel &= 7;
	}
	else {
		entNum = 0;
		entChannel = 0;
	}

	if (flags & SND_POS){
		// Positioned in space
		MSG_ReadPos(&net_message, originVec);
 		origin = originVec;
	}
	else	// Use entity number
		origin = NULL;

	if (!cl.media.gameSounds[sound])
		return;

	S_PlaySound(origin, entNum, entChannel, cl.media.gameSounds[sound], volume, attenuation, timeOfs);
}

/*
 ==================
 CL_UpdateLagometer

 A new packet was just parsed
 ==================
*/
static void CL_UpdateLagometer (){

	int		current;

	if (cls.state != CA_ACTIVE)
		return;

	current = cl.lagometer.current & (LAG_SAMPLES - 1);

	cl.lagometer.dropped[current] = cls.netChan.dropped;
	cl.lagometer.suppressed[current] = cl.suppressCount;
	cl.lagometer.ping[current] = cls.realTime - cl.cmdTime[cls.netChan.incomingAcknowledged & CMD_MASK];
	cl.lagometer.current++;
}

/*
 ==================
 CL_ShowNet
 ==================
*/
void CL_ShowNet (int level, const char *fmt, ...){

	char	message[1024];
	va_list	argPtr;

	if (cl_showNet->integerValue < level)
		return;

	va_start(argPtr, fmt);
	Str_VSPrintf(message, sizeof(message), fmt, argPtr);
	va_end(argPtr);

	Com_Printf("%s\n", message);
}

/*
 ==================
 CL_ParseServerMessage
 ==================
*/
void CL_ParseServerMessage (){

	int		cmd;

	CL_ShowNet(2, "------------------\n");
	CL_ShowNet(1, "%i", net_message.curSize);

	// Parse the message
	while (1){
		if (net_message.readCount > net_message.curSize)
			Com_Error(ERR_DROP, "CL_ParseServerMessage: bad server message");

		// Parse the server op
		cmd = MSG_ReadByte(&net_message);
		if (cmd == -1){
			CL_ShowNet(2, "END OF MESSAGE");
			break;
		}

		if (!svc_strings[cmd])
			CL_ShowNet(2, "%3i: BAD CMD %i", net_message.readCount-1, cmd);
		else
			CL_ShowNet(2, "%3i: %s", net_message.readCount-1, svc_strings[cmd]);

		// Other commands
		switch (cmd){
		case SVC_NOP:

			break;
		case SVC_DISCONNECT:
			CL_ParseDisconnect();
			break;
		case SVC_RECONNECT:
			CL_ParseReconnect();
			break;
		case SVC_STUFFTEXT:
			CL_ParseStuffText();
			break;
		case SVC_PRINT:
			CL_ParsePrint();
			break;
		case SVC_CENTERPRINT:
			CL_ParseCenterPrint();
			break;
		case SVC_DOWNLOAD:
			CL_ParseDownload();
			break;
		case SVC_SERVERDATA:
			CL_ParseServerData();
			break;
		case SVC_CONFIGSTRING:
			CL_ParseConfigString();
			break;
		case SVC_LAYOUT:
			CL_ParseLayout();
			break;
		case SVC_INVENTORY:
			CL_ParseInventory();
			break;
		case SVC_SOUND:
			CL_ParseStartSound();
			break;
		case SVC_MUZZLEFLASH:
			CL_ParsePlayerMuzzleFlash();
			break;
		case SVC_MUZZLEFLASH2:
			CL_ParseMonsterMuzzleFlash();
			break;
		case SVC_TEMP_ENTITY:
			CL_ParseTempEntity();
			break;
		case SVC_SPAWNBASELINE:
			CL_ParseBaseLine();
			break;
		case SVC_FRAME:
			CL_ParseFrame();
			break;
		case SVC_PLAYERINFO:
		case SVC_PACKETENTITIES:
		case SVC_DELTAPACKETENTITIES:
			Com_Error(ERR_DROP, "CL_ParseServerMessage: out of place frame data");
			break;
		default:
			Com_Error(ERR_DROP, "CL_ParseServerMessage: illegible server message");
			break;
		}
	}

	// Update lagometer info
	CL_UpdateLagometer();

	// We don't know if it is ok to save a demo message until after we
	// have parsed the frame
	CL_WriteDemoMessage();
}