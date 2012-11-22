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
// cl_demo.c - Demo recording and playback
//


#include "client.h"


#define DEMO_EOF					0xFFFFFFFF


/*
 ==============================================================================

 DEMO RECORDING

 ==============================================================================
*/


/*
 ==================
 CL_FindDemoName
 ==================
*/
static bool CL_FindDemoName (char *name, int maxLength){

	static int	demoNumber;

	while (demoNumber <= 9999){
		Str_SPrintf(name, maxLength, "demos/demo%04i.dm2", demoNumber);

		if (!FS_FileExists(name))
			break;

		demoNumber++;
	}

	if (demoNumber == 10000)
		return false;

	demoNumber++;

	return true;
}

/*
 ==================
 CL_WriteDemoMessage

 Dumps the current net message, prefixed by the length
 ==================
*/
void CL_WriteDemoMessage (){

	int		length, swLen;

	if (!cls.demoFile || cls.demoWaiting)
		return;

	// The first eight bytes are just packet sequencing stuff
	length = net_message.curSize - 8;
	swLen = LittleLong(length);

	if (!swLen)
		return;

	FS_Write(cls.demoFile, &swLen, sizeof(swLen));
	FS_Write(cls.demoFile, net_message.data + 8, length);
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 CL_Record_f
 ==================
*/
void CL_Record_f (){

	char			name[MAX_QPATH];
	char			data[MAX_MSGLEN];
	msg_t			msg;
	int				i, length;
	entity_state_t	*entity, nullState;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: record [name]\n");
		return;
	}

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic){
		Com_Printf("You must be in a level to record\n");
		return;
	}

	if (cls.demoFile){
		Com_Printf("Already recording a demo\n");
		return;
	}

	if (Cmd_Argc() != 1){
		Str_SPrintf(name, sizeof(name), "demos/%s", Cmd_Argv(1));
		Str_DefaultFileExtension(name, sizeof(name), ".dm2");
	}
	else {
		if (!CL_FindDemoName(name, sizeof(name))){
			Com_Printf("Demos directory is full!\n");
			return;
		}
	}

	// Open the demo file
	FS_OpenFile(name, FS_WRITE, &cls.demoFile);
	if (!cls.demoFile){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}

	Com_Printf("Recording to %s\n", name);

	Str_Copy(cls.demoName, name, sizeof(cls.demoName));

	// Don't start saving messages until a non-delta compressed message 
	// is received
	cls.demoWaiting = true;

	// Write out messages to hold the startup information
	MSG_Init(&msg, (byte *)data, sizeof(data), false);

	// Send the server data
	MSG_WriteByte(&msg, SVC_SERVERDATA);
	MSG_WriteLong(&msg, PROTOCOL_VERSION);
	MSG_WriteLong(&msg, 0x10000 + cl.serverCount);
	MSG_WriteByte(&msg, 1);			// Demos are always attract loops
	MSG_WriteString(&msg, cl.gameDir);
	MSG_WriteShort(&msg, cl.clientNum - 1);
	MSG_WriteString(&msg, cl.configStrings[CS_NAME]);

	// Configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS; i++){
		if (!cl.configStrings[i][0])
			continue;

		if (msg.curSize + Str_Length(cl.configStrings[i]) + 32 > msg.maxSize){
			// Write it out
			length = LittleLong(msg.curSize);
			FS_Write(cls.demoFile, &length, sizeof(length));
			FS_Write(cls.demoFile, msg.data, msg.curSize);

			msg.curSize = 0;
		}

		MSG_WriteByte(&msg, SVC_CONFIGSTRING);
		MSG_WriteShort(&msg, i);
		MSG_WriteString(&msg, cl.configStrings[i]);
	}

	// Baselines
	Mem_Fill(&nullState, 0, sizeof(nullState));

	for (i = 0; i < MAX_EDICTS; i++){
		entity = &cl.entities[i].baseline;
		if (!entity->modelindex)
			continue;

		if (msg.curSize + sizeof(entity_state_t) + 32 > msg.maxSize){
			// Write it out
			length = LittleLong(msg.curSize);
			FS_Write(cls.demoFile, &length, sizeof(length));
			FS_Write(cls.demoFile, msg.data, msg.curSize);

			msg.curSize = 0;
		}

		MSG_WriteByte(&msg, SVC_SPAWNBASELINE);
		MSG_WriteDeltaEntity(&msg, &nullState, entity, true, true);
	}

	MSG_WriteByte(&msg, SVC_STUFFTEXT);
	MSG_WriteString(&msg, "precache\n");

	// Write it to the demo file
	length = LittleLong(msg.curSize);
	FS_Write(cls.demoFile, &length, sizeof(length));
	FS_Write(cls.demoFile, msg.data, msg.curSize);

	// The rest of the demo file will be individual frames
}

/*
 ==================
 CL_StopRecord_f
 ==================
*/
void CL_StopRecord_f (){

	int		eof;

	if (!cls.demoFile){
		Com_Printf("Not recording a demo\n");
		return;
	}

	// Write the EOF
	eof = LittleLong(DEMO_EOF);

	FS_Write(cls.demoFile, &eof, sizeof(int));

	// Close the demo file
	FS_CloseFile(cls.demoFile);
	cls.demoFile = 0;

	Com_Printf("Stopped demo recording\n");
}