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
// sv_main.c - Primary server file
//


#include "server.h"


#define MAX_MASTERS					6

static netAdr_t				sv_masterAdr[MAX_MASTERS];	// Address of group servers

server_t					sv;							// Local server
serverStatic_t				svs;						// Persistant server info

client_t *					sv_client;					// Current client
edict_t *					sv_player;					// Current player

cvar_t *					sv_maxClients;
cvar_t *					sv_hostName;
cvar_t *					sv_nextServer;
cvar_t *					sv_master1;
cvar_t *					sv_master2;
cvar_t *					sv_master3;
cvar_t *					sv_master4;
cvar_t *					sv_master5;
cvar_t *					sv_clientTimeOut;
cvar_t *					sv_zombieTimeOut;
cvar_t *					sv_reconnectLimit;
cvar_t *					sv_noReload;
cvar_t *					sv_airAccelerate;
cvar_t *					sv_enforceTime;
cvar_t *					sv_allowDownload;
cvar_t *					sv_publicServer;
cvar_t *					sv_rconPassword;
cvar_t *					sv_loadGame;


/*
 ==================
 
 ==================
*/
void SV_NextServer (){

	if (sv.state == SS_GAME)
		return;			// Can't nextserver while playing a normal game

	svs.spawnCount++;	// Make sure another doesn't sneak in

	if (!sv_nextServer->value[0])
		Cmd_AppendText("killserver\n");
	else {
		Cmd_AppendText(sv_nextServer->value);
		Cmd_AppendText("\n");

		CVar_SetVariableString("sv_nextServer", "", false);
	}
}

/*
 ==================
 SV_DropClient

 Called when the player is totally leaving the server, either willingly
 or unwillingly. This is NOT called if the entire server is quiting or 
 crashing.
 ==================
*/
void SV_DropClient (client_t *client){

	// Add the disconnect
	MSG_WriteByte(&client->netChan.message, SVC_DISCONNECT);

	// Call the game function for removing a client.
	// This will remove the body, among other things.
	if (client->state == CS_SPAWNED)
		ge->ClientDisconnect(client->edict);

	// Close file download
	if (client->downloadFile){
		FS_CloseFile(client->downloadFile);
		client->downloadFile = 0;
	}

	client->state = CS_ZOMBIE;		// Become free in a few seconds
	client->name[0] = 0;
}

/*
 ==================
 SV_UserInfoChanged

 Pull specific info from a newly changed user info string into a more C
 friendly form
 ==================
*/
void SV_UserInfoChanged (client_t *client){

	char	*value;

	// Call game code to allow overrides
	ge->ClientUserinfoChanged(client->edict, client->userInfo);

	// Name for C code
	Str_Copy(client->name, Info_ValueForKey(client->userInfo, "name"), sizeof(client->name));

	// Rate command
	value = Info_ValueForKey(client->userInfo, "rate");
	if (value[0])
		client->rate = Clamp(Str_ToInteger(value), 100, 25000);
	else
		client->rate = 5000;

	// Msg command
	value = Info_ValueForKey(client->userInfo, "msg");
	if (value[0])
		client->messageLevel = Str_ToInteger(value);
}

/*
 ==================
 SV_StatusString

 Builds the string that is sent as heartbeats and status replies
 ==================
*/
static char *SV_StatusString (){

	client_t	*client;
	static char	status[MAX_MSGLEN - 16];
	char		player[64];
	int			statusLength;
	int			playerLength;
	int			i;

	Str_SPrintf(status, sizeof(status), "%s\n", CVar_InfoString(CVAR_SERVERINFO));
	statusLength = Str_Length(status);

	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		if (client->state == CS_CONNECTED || client->state == CS_SPAWNED){
			Str_SPrintf(player, sizeof(player), "%i %i \"%s\"\n", client->edict->client->ps.stats[STAT_FRAGS], client->ping, client->name);
			playerLength = Str_Length(player);

			if (statusLength + playerLength >= sizeof(status))
				break;		// Can't hold any more

			Str_Append(status, player, sizeof(status));
			statusLength += playerLength;
		}
	}

	return status;
}

/*
 ==================
 SV_StatusPacket

 Responds with all the info that qplug or qspy can see
 ==================
*/
static void SV_StatusPacket (){

	// TODO: remove SV_StatusString and move it into here?

	// Send the status response
	NET_OutOfBandPrintf(NS_SERVER, net_from, "print\n%s", SV_StatusString());
}

/*
 ==================
 SV_AckPacket
 ==================
*/
static void SV_AckPacket (){

	// Send the acknowledge response
	Com_Printf("Ping acknowledge from %s\n", NET_AddressToString(net_from));
}

/*
 ==================
 SV_InfoPacket

 Responds with short info for broadcast scans.
 The second parameter should be the current protocol version number.
 ==================
*/
static void SV_InfoPacket (){

	char	string[64];
	int		protocol, count = 0;
	int		i;

	if (sv_maxClients->integerValue == 1)
		return;		// Ignore in singleplayer

	protocol = Str_ToInteger(Cmd_Argv(1));

	if (protocol != PROTOCOL_VERSION)
		Str_SPrintf(string, sizeof(string), "%s: wrong version\n", sv_hostName->value, sizeof(string));
	else {
		for (i = 0; i < sv_maxClients->integerValue; i++){
			if (svs.clients[i].state >= CS_CONNECTED)
				count++;
		}

		Str_SPrintf(string, sizeof(string), "%16s %8s %2i/%2i\n", sv_hostName->value, sv.name, count, sv_maxClients->integerValue);
	}

	// Send the info response
	NET_OutOfBandPrintf(NS_SERVER, net_from, "info\n%s", string);
}

/*
 ==================
 SV_PingPacket

 Just responds with an acknowledgement
 ==================
*/
static void SV_PingPacket (){

	// Send the ping response
	NET_OutOfBandPrintf(NS_SERVER, net_from, "ack");
}

/*
 ==================
 SV_GetChallengePacket

 TODO: this could be a bit cleaner

 Returns a challenge number that can be used in a subsequent 
 client_connect command.
 We do this to prevent denial of service attacks that flood the server 
 with invalid connection IPs. With a challenge, they must give a valid 
 IP address.
 ==================
*/
static void SV_GetChallengePacket (){

	int			oldest, oldestTime;
	int			i;

	oldest = 0;
	oldestTime = 0x7FFFFFFF;

	// See if we already have a challenge for this IP
	for (i = 0; i < MAX_CHALLENGES; i++){
		if (NET_CompareBaseAddress(net_from, svs.challenges[i].address))
			break;		// Reuse this challenge

		// Overwrite the oldest
		if (svs.challenges[i].time < oldestTime){
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES){
		// This is the first time this client has asked for a challenge
		svs.challenges[oldest].challenge = rand() & 0x7FFF;
		svs.challenges[oldest].address = net_from;
		svs.challenges[oldest].time = Sys_Milliseconds();
		i = oldest;
	}

	// Send the challenge response
	NET_OutOfBandPrintf(NS_SERVER, net_from, "challenge %i", svs.challenges[i].challenge);
}

/*
 ==================
 SV_ConnectPacket

 TODO: this could be a bit cleaner

 A connection request that did not come from the master
 ==================
*/
static void SV_ConnectPacket (){

	client_t	*client, *newClient = NULL;
	edict_t		*entity;
	char		userInfo[MAX_INFO_STRING];
	int			protocol, challenge, channelPort;
	int			i;

	protocol = Str_ToInteger(Cmd_Argv(1));
	if (protocol != PROTOCOL_VERSION){
		Com_DPrintf("Rejected connect from version %i\n", protocol);

		NET_OutOfBandPrintf(NS_SERVER, net_from, "print\nServer uses protocol version %i\n", PROTOCOL_VERSION);
		return;
	}

	// Parse the arguments
	channelPort = Str_ToInteger(Cmd_Argv(2));
	challenge = Str_ToInteger(Cmd_Argv(3));

	Str_Copy(userInfo, Cmd_Argv(4), sizeof(userInfo));

	// Force the IP key/value pair so the game can filter based on IP
	Info_SetValueForKey(userInfo, "ip", (char *)NET_AddressToString(net_from));

	// attractLoop servers are ONLY for local clients
	if (sv.attractLoop){
		if (!NET_IsLocalAddress(net_from)){
			Com_DPrintf("Rejected connect in attract loop\n");

			NET_OutOfBandPrintf(NS_SERVER, net_from, "print\nConnection refused\n");
			return;
		}
	}

	// See if the challenge is valid
	if (!NET_IsLocalAddress(net_from)){
		for (i = 0; i < MAX_CHALLENGES; i++){
			if (NET_CompareBaseAddress(net_from, svs.challenges[i].address)){
				if (challenge == svs.challenges[i].challenge)
					break;		// Good

				Com_DPrintf("%s: bad challenge\n", NET_AddressToString(net_from));

				NET_OutOfBandPrintf(NS_SERVER, net_from, "print\nBad challenge\n");
				return;
			}
		}

		if (i == MAX_CHALLENGES){
			Com_DPrintf("%s: no challenge\n", NET_AddressToString(net_from));

			NET_OutOfBandPrintf(NS_SERVER, net_from, "print\nNo challenge for address\n");
			return;
		}
	}

	// If there is already a slot for this IP, reuse it
	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		if (client->state == CS_FREE)
			continue;

		if (NET_CompareBaseAddress(net_from, client->netChan.remoteAddress) && (client->netChan.channelPort == channelPort || net_from.port == client->netChan.remoteAddress.port)){
			if (!NET_IsLocalAddress(net_from) && (svs.realTime - client->lastConnect) < (sv_reconnectLimit->floatValue * 1000)){
				Com_DPrintf("%s: reconnect rejected : too soon\n", NET_AddressToString(net_from));
				return;
			}

			Com_DPrintf("%s: reconnect\n", NET_AddressToString(net_from));

			newClient = client;
			break;
		}
	}

	// Find a client slot
	if (!newClient){
		for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
			if (client->state == CS_FREE){
				newClient = client;
				break;
			}
		}

		if (!newClient){
			Com_DPrintf("Rejected a connection\n");

			NET_OutOfBandPrintf(NS_SERVER, net_from, "print\nServer is full\n");
			return;
		}
	}

	// Build a new connection.
	// Accept the new client.
	// This is the only place a client_t is ever initialized.
	Mem_Fill(newClient, 0, sizeof(client_t));

	i = newClient - svs.clients;
	entity = EDICT_NUM(i+1);
	newClient->edict = entity;
	newClient->challenge = challenge;		// Save challenge for checksumming

	// Get the game a chance to reject this connection or modify the
	// user info
	if (!(ge->ClientConnect(entity, userInfo))){
		if (*Info_ValueForKey(userInfo, "rejmsg"))
			NET_OutOfBandPrintf(NS_SERVER, net_from, "print\n%s\n", Info_ValueForKey(userInfo, "rejmsg"));
		else
			NET_OutOfBandPrintf(NS_SERVER, net_from, "print\nConnection refused\n");

		Com_DPrintf("Game rejected a connection\n");
		return;
	}

	// Parse some info from the info strings
	Str_Copy(newClient->userInfo, userInfo, sizeof(newClient->userInfo));
	SV_UserInfoChanged(newClient);

	NetChan_Setup(&newClient->netChan, NS_SERVER, net_from, channelPort);

	MSG_Init(&newClient->datagram, newClient->datagramBuffer, sizeof(newClient->datagramBuffer), true);

	newClient->lastMessage = svs.realTime;	// Don't time-out
	newClient->lastConnect = svs.realTime;
	newClient->state = CS_CONNECTED;

	// Send the connect packet to the client
	NET_OutOfBandPrintf(NS_SERVER, net_from, "client_connect");
}

/*
 ==================
 SV_RconValidate
 ==================
*/
static bool SV_RconValidate (){

	if (!sv_rconPassword->value[0])
		return false;

	if (Str_Compare(sv_rconPassword->value, Cmd_Argv(1)))
		return false;

	return true;
}

/*
 ==================
 SV_RConPacket

 A client issued a rcon command.
 Shift down the remaining args.
 Redirect all printfs.
 ==================
*/
static void SV_RConPacket (){

	char	remaining[1024];
	int		i;

	if (!SV_RconValidate())
		Com_Printf("Bad rcon from %s:\n%s\n", NET_AddressToString(net_from), net_message.data+4);
	else
		Com_Printf("Rcon from %s:\n%s\n", NET_AddressToString(net_from), net_message.data+4);

	Com_BeginRedirect(RD_PACKET, sv_outputBuf, OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!SV_RconValidate())
		Com_Printf("Bad \"rconPassword\"\n");
	else {
		remaining[0] = 0;

		for (i = 2; i < Cmd_Argc(); i++){
			Str_Append(remaining, Cmd_Argv(i), sizeof(remaining));
			Str_Append(remaining, " ", sizeof(remaining));
		}

		Cmd_ExecuteText(CMD_EXEC_NOW, remaining);
	}

	Com_EndRedirect();
}

/*
 ==================
 SV_ConnectionlessPacket

 TODO: Clean this some more?

 A connectionless packet has four leading 0xff characters to distinguish 
 it from a game channel.
 Clients that are in the game can still send connectionless packets.
 ==================
*/
static void SV_ConnectionlessPacket (){

	char	*string, *cmd;

	// Read the message
	MSG_BeginReading(&net_message);

	MSG_ReadLong(&net_message);		// Skip the -1 marker
	string = MSG_ReadStringLine(&net_message);

	// Tokenize the string
	Cmd_TokenizeString(string);

	if (!Cmd_Argc())
		return;		// No tokens

	cmd = Cmd_Argv(0);

	Com_DPrintf("Client out-of-band packet from %s: %s\n", NET_AddressToString(net_from), cmd);

	if (!Str_ICompare(cmd, "status"))
		SV_StatusPacket();
	else if (!Str_ICompare(cmd, "ack"))
		SV_AckPacket();
	else if (!Str_ICompare(cmd, "info"))
		SV_InfoPacket();
	else if (!Str_ICompare(cmd, "ping"))
		SV_PingPacket();
	else if (!Str_ICompare(cmd, "getchallenge"))
		SV_GetChallengePacket();
	else if (!Str_ICompare(cmd, "connect"))
		SV_ConnectPacket();
	else if (!Str_ICompare(cmd, "rcon"))
		SV_RConPacket();
	else
		Com_DPrintf(S_COLOR_YELLOW "Bad client out-of-band packet from %s:\n" S_COLOR_YELLOW "%s\n", NET_AddressToString(net_from), string);
}

/*
 ==================
 SV_ParsePacket
 ==================
*/
static void SV_ParsePacket (){

	client_t	*client;
	int			channelPort;
	int			i;

	// Check for connectionless packet first
	if (*(int *)net_message.data == -1){
		SV_ConnectionlessPacket();
		return;
	}

	// Read the channel port from the message so we can fix up stupid address
	// translating routers
	MSG_BeginReading(&net_message);

	MSG_ReadLong(&net_message);		// Sequence number
	MSG_ReadLong(&net_message);		// Sequence number
	channelPort = MSG_ReadShort(&net_message) & 0xFFFF;

	// Check for packets from connected clients
	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		if (client->state == CS_FREE)
			continue;

		if (!NET_CompareBaseAddress(net_from, client->netChan.remoteAddress))
			continue;

		// It is possible to have multiple clients from a single IP address,
		// so they are differentiated by the channel port value
		if (client->netChan.channelPort != channelPort)
			continue;

		// The IP port can't be used to differentiate them, because some
		// address translating routers periodically change UDP port assignments
		if (client->netChan.remoteAddress.port != net_from.port){
			Com_DPrintf("Fixing up a translated port from %s\n", NET_AddressToString(net_from));

			client->netChan.remoteAddress.port = net_from.port;
		}

		// Packet from client
		if (!NetChan_Process(&client->netChan, &net_message))
			return;		// Out of order, duplicated, etc

		// Zombie clients still need to process the message to make sure they
		// don't need to retransmit the final reliable message, but they don't
		// do any other processing
		if (client->state == CS_ZOMBIE)
			return;

		// Don't time-out
		client->lastMessage = svs.realTime;

		// Parse the message
		SV_ParseClientMessage(client);

		return;
	}
}

/*
 ==================
 SV_ReadPackets
 ==================
*/
static void SV_ReadPackets (){

	// Read incoming network packets
	while (NET_GetPacket(NS_SERVER, &net_from, &net_message)){
		// Parse the packet
		SV_ParsePacket();
	}
}


// ============================================================================


/*
 ==================
 
 Updates the cl->ping variables
 ==================
*/
static void SV_UpdatePings (){

	client_t	*client;
	int			count, total;
	int			i, j;

	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		if (client->state != CS_SPAWNED)
			continue;

		count = 0;
		total = 0;

		for (j = 0; j < LATENCY_COUNTS; j++){
			if (client->frameLatency[j] > 0){
				count++;
				total += client->frameLatency[j];
			}
		}

		if (!count)
			client->ping = 0;
		else
			client->ping = total / count;

		// Let the game module know about the ping
		client->edict->client->ping = client->ping;
	}
}


// ============================================================================


/*
 ==================
 
 ==================
*/
static void SV_InitMaster (){

	cvar_t	*var;
	char	idMaster[32];
	int		i;

	if (!dedicated->integerValue)
		return;		// Only dedicated servers send heartbeats

	if (!sv_publicServer->integerValue)
		return;		// A private dedicated game

	for (i = 0; i < MAX_MASTERS; i++){
		Mem_Fill(&sv_masterAdr[i], 0, sizeof(sv_masterAdr[i]));

		if (i == 0){	// Slot 0 will always contain the id master
			Str_SPrintf(idMaster, sizeof(idMaster), "192.246.40.32:%i", PORT_MASTER);
			
			if (!NET_StringToAddress(idMaster, &sv_masterAdr[i])){
				Com_Printf("Bad master address: %s\n", idMaster);
				continue;
			}
		}
		else {
			var = CVar_Register(Str_VarArgs("sv_master%i", i), "", CVAR_STRING, 0, NULL, 0, 0);
			var->modified = false;

			if (!var->value[0])
				continue;	// Empty slot

			if (!NET_StringToAddress(var->value, &sv_masterAdr[i])){
				Com_Printf("Bad master address: %s\n", var->value);
				continue;
			}
		}

		if (sv_masterAdr[i].port == 0)
			sv_masterAdr[i].port = BigShort(PORT_MASTER);

		Com_Printf("Master server at %s\n", NET_AddressToString(sv_masterAdr[i]));
		NET_OutOfBandPrintf(NS_SERVER, sv_masterAdr[i], "ping");
	}

	svs.lastHeartbeat = -9999999;
}

/*
 ==================
 
 Send a message to the master every few minutes to let it know we are 
 alive, and log information
 ==================
*/
static void SV_MasterHeartbeat (){

	char	*string;
	int		i;

	if (!dedicated->integerValue)
		return;		// Only dedicated servers send heartbeats

	if (!sv_publicServer->integerValue)
		return;		// A private dedicated game

	// Check for time wrapping
	if (svs.lastHeartbeat > svs.realTime)
		svs.lastHeartbeat = svs.realTime;

	if (svs.realTime - svs.lastHeartbeat < 300000)
		return;		// Not time to send yet

	svs.lastHeartbeat = svs.realTime;

	// Send the same string that we would give for a status OOB command
	string = SV_StatusString();

	// Send to group master
	for (i = 0; i < MAX_MASTERS; i++){
		if (sv_masterAdr[i].port){
			Com_Printf("Sending heartbeat to %s...\n", NET_AddressToString(sv_masterAdr[i]));

			NET_OutOfBandPrintf(NS_SERVER, sv_masterAdr[i], "heartbeat\n%s", string);
		}
	}
}

/*
 ==================
 
 Informs all masters that this server is going down
 ==================
*/
static void SV_ShutdownMaster (){

	int		i;

	if (!dedicated->integerValue)
		return;		// Only dedicated servers send heartbeats

	if (!sv_publicServer->integerValue)
		return;		// A private dedicated game

	// Send to group master
	for (i = 0; i < MAX_MASTERS; i++){
		if (sv_masterAdr[i].port){
			Com_Printf("Sending heartbeat to %s...\n", NET_AddressToString(sv_masterAdr[i]));

			NET_OutOfBandPrintf(NS_SERVER, sv_masterAdr[i], "shutdown");
		}
	}
}


// ============================================================================


/*
 ==================
 SV_GiveMsec

 Every few frames, gives all clients an allotment of milliseconds for 
 their command moves. If they exceed it, assume cheating.
 ==================
*/
static void SV_GiveMsec (){

	client_t	*client;
	int			i;

	if (sv.frameNum & 15)
		return;

	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		if (client->state == CS_FREE)
			continue;

		client->commandMsec = 1800;		// 1600 + some slop
	}
}

/*
 ==================
 

 If a packet has not been received from a client for sv_clientTimeOut seconds,
 drop the connection. Server time is used instead of real time to avoid
 dropping the local client while debugging.

 When a client is normally dropped, the client goes into a zombie state for a
 few seconds to make sure any final reliable message gets resent if necessary.
 ==================
*/
static void SV_CheckTimeOuts (){

	client_t	*client;
	int			dropPoint, zombiePoint;
	int			i;

	dropPoint = svs.realTime - sv_clientTimeOut->floatValue * 1000;
	zombiePoint = svs.realTime - sv_zombieTimeOut->floatValue * 1000;

	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		// Message times may be wrong across a level change
		if (client->lastMessage > svs.realTime)
			client->lastMessage = svs.realTime;

		if (client->state == CS_ZOMBIE && client->lastMessage < zombiePoint){
			client->state = CS_FREE;	// Can now be reused
			continue;
		}

		if ((client->state == CS_CONNECTED || client->state == CS_SPAWNED) && client->lastMessage < dropPoint){
			SV_BroadcastPrintf(PRINT_HIGH, "%s timed out\n", client->name);
			SV_DropClient(client); 

			client->state = CS_FREE;	// Don't bother with zombie state
		}
	}
}

/*
 ==================
 SV_RunGameFrame
 ==================
*/
static void SV_RunGameFrame (){

	// Don't run if paused
	if (paused->integerValue && sv_maxClients->integerValue == 1)
		return;

	sv.frameNum++;
	sv.time = sv.frameNum * 100;

	if (com_speeds->integerValue)
		com_timeBeforeGame = Sys_Milliseconds();

	ge->RunFrame();

	if (com_speeds->integerValue)
		com_timeAfterGame = Sys_Milliseconds();

	// Never get more than one tic behind
	if (sv.time < svs.realTime)
		svs.realTime = sv.time;
}

/*
 ==================
 SV_ClearGameEvents
 ==================
*/
static void SV_ClearGameEvents (){

	edict_t	*entity;
	int		i;

	// Events only last for a single message
	for (i = 0; i < ge->num_edicts; i++, entity++){
		entity = EDICT_NUM(i);
		entity->s.event = 0;
	}
}

/*
 ==================
 SV_FinalMessage

 Used by SV_Shutdown to send a final message to all connected clients before
 the server goes down. The messages are sent immediately because the server is
 going to totally exit after returning from this function.
 ==================
*/
static void SV_FinalMessage (const char *message, bool reconnect){

	client_t	*client;
	msg_t		msg;
	char		buffer[MAX_MSGLEN];
	int			i;

	if (!svs.clients)
		return;

	MSG_Init(&msg, (byte *)buffer, sizeof(buffer), false);

	MSG_WriteByte(&msg, SVC_PRINT);
	MSG_WriteByte(&msg, PRINT_HIGH);
	MSG_WriteString(&msg, message);

	if (reconnect)
		MSG_WriteByte(&msg, SVC_RECONNECT);
	else
		MSG_WriteByte(&msg, SVC_DISCONNECT);

	// Send it twice in case one is dropped
	// Stagger the packets to crutch operating system limited buffers.
	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		if (client->state >= CS_CONNECTED)
			NetChan_Transmit(&client->netChan, msg.data, msg.curSize);
	}

	for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
		if (client->state >= CS_CONNECTED)
			NetChan_Transmit(&client->netChan, msg.data, msg.curSize);
	}
}


/*
 ==============================================================================

 FRAMEWORK

 ==============================================================================
*/


/*
 ==================
 SV_Frame
 ==================
*/
void SV_Frame (int msec){

	com_timeBeforeGame = com_timeAfterGame = 0;

	// If server is not active, do nothing
	if (!svs.initialized)
		return;

    svs.realTime += msec;

	// Keep the random time dependent
	rand();

	// Check if a client connection timed out
	SV_CheckTimeOuts();

	// Read and parse incoming packets
	SV_ReadPackets();

	// Move autonomous things around if enough time has passed
	if (!com_timeDemo->integerValue && svs.realTime < sv.time){
		// Never let the time get too far off
		if (sv.time - svs.realTime > 100)
			svs.realTime = sv.time - 100;

		return;
	}

	// Update pings for all connected clients
	SV_UpdatePings();

	// Give the clients some time slices
	SV_GiveMsec();

	// Run the game simulation
	SV_RunGameFrame();

	// Send messages back to the clients that had packets read this 
	// frame
	SV_SendClientMessages();

	// Save the entire world state if recording a serverdemo
	SV_RecordDemoMessage();

	// Clear game events for next frame
	SV_ClearGameEvents();

	// Send a heartbeat to the master server if needed
	if (dedicated->integerValue){
		if (sv_master1->modified || sv_master2->modified || sv_master3->modified || sv_master4->modified || sv_master5->modified)
			SV_InitMaster();

		SV_MasterHeartbeat();
	}
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 
 Only called from Com_Init, not for each game
 ==================
*/
void SV_Init (){

	// Register variables
	CVar_Register("cheats", "0", CVAR_BOOL, CVAR_SERVERINFO | CVAR_LATCH, NULL, 0, 0);
	CVar_Register("skill", "1", CVAR_INTEGER, CVAR_SERVERINFO | CVAR_LATCH, NULL, 0, 10);
	CVar_Register("coop", "0", CVAR_BOOL, CVAR_SERVERINFO | CVAR_LATCH, NULL, 0, 0);
	CVar_Register("deathmatch", "0", CVAR_BOOL, CVAR_SERVERINFO | CVAR_LATCH, NULL, 0, 0);
	CVar_Register("dmflags", Str_VarArgs("%i", DF_INSTANT_ITEMS), CVAR_INTEGER, CVAR_SERVERINFO, NULL, 0, 10000);
	CVar_Register("fraglimit", "0", CVAR_INTEGER, CVAR_SERVERINFO, NULL, 0, 10000);
	CVar_Register("timelimit", "0", CVAR_INTEGER, CVAR_SERVERINFO, NULL, 0, 10000);
	CVar_Register("sv_mapName", "", CVAR_STRING, CVAR_SERVERINFO | CVAR_READONLY, NULL, 0, 0);
	CVar_Register("sv_mapChecksum", "", CVAR_INTEGER, CVAR_READONLY, NULL, 0, 100000);
	CVar_Register("sv_protocol", Str_VarArgs("%i", PROTOCOL_VERSION), CVAR_STRING, CVAR_SERVERINFO | CVAR_READONLY, NULL, 0, 100000);
	sv_maxClients = CVar_Register("maxclients", "1", CVAR_INTEGER, CVAR_SERVERINFO | CVAR_LATCH, NULL, 0, 100000);
	sv_hostName = CVar_Register("sv_hostName", "noname", CVAR_STRING, CVAR_SERVERINFO | CVAR_ARCHIVE, NULL, 0, 0);
	sv_nextServer = CVar_Register("sv_nextServer", "", CVAR_STRING, 0, NULL, 0, 0);
	sv_master1 = CVar_Register("sv_master1", "", CVAR_STRING, CVAR_ARCHIVE, NULL, 0, 0);
	sv_master2 = CVar_Register("sv_master2", "", CVAR_STRING, CVAR_ARCHIVE, NULL, 0, 0);
	sv_master3 = CVar_Register("sv_master3", "", CVAR_STRING, CVAR_ARCHIVE, NULL, 0, 0);
	sv_master4 = CVar_Register("sv_master4", "", CVAR_STRING, CVAR_ARCHIVE, NULL, 0, 0);
	sv_master5 = CVar_Register("sv_master5", "", CVAR_STRING, CVAR_ARCHIVE, NULL, 0, 0);
	sv_noReload = CVar_Register("sv_noReload", "0", CVAR_BOOL, 0, NULL, 0, 0);
	sv_airAccelerate = CVar_Register("sv_airAccelerate", "0", CVAR_FLOAT, CVAR_LATCH, NULL, 0.0f, 100000.0f);
	sv_enforceTime = CVar_Register("sv_enforceTime", "0", CVAR_BOOL, 0, NULL, 0, 0);
	sv_allowDownload = CVar_Register("sv_allowDownload", "1", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	sv_publicServer = CVar_Register("sv_publicServer", "1", CVAR_BOOL, 0, NULL, 0, 0);

	sv_clientTimeOut = CVar_Register("sv_clientTimeOut", "120.0", CVAR_FLOAT, 0, "Client time-out time in seconds", 1.0f, 300.0f);
	sv_zombieTimeOut = CVar_Register("sv_zombieTimeOut", "2.0", CVAR_FLOAT, 0, "Zombie time-out time in seconds", 1.0f, 300.0f);
	sv_reconnectLimit = CVar_Register("sv_reconnectLimit", "3.0", CVAR_FLOAT, 0, "Time in seconds before a client is allowed to reconnect", 1.0f, 300.0f);
	sv_rconPassword = CVar_Register("rconPassword", "", CVAR_STRING, 0, "Remote console password", 0, 0);
	sv_loadGame = CVar_Register("sv_loadGame", "0", CVAR_INTEGER, 0, "Loads the desired game DLL (0 = extended, 1 = original)", 0, 1);

	// Add commands
	Cmd_AddCommand("loadgame", SV_LoadGame_f, NULL, NULL);
	Cmd_AddCommand("savegame", SV_SaveGame_f, NULL, NULL);
	Cmd_AddCommand("gamemap", SV_GameMap_f, NULL, NULL);
	Cmd_AddCommand("map", SV_Map_f, "Loads a map", Cmd_ArgCompletion_MapName);
	Cmd_AddCommand("demo", SV_Demo_f, NULL, NULL);
	Cmd_AddCommand("kick", SV_Kick_f, NULL, NULL);
	Cmd_AddCommand("status", SV_Status_f, "Shows server status", NULL);
	Cmd_AddCommand("heartbeat", SV_Heartbeat_f, "Sends a heartbeat to the master server", NULL);
	Cmd_AddCommand("serverinfo", SV_ServerInfo_f, "Shows server info", NULL);
	Cmd_AddCommand("dumpuser", SV_DumpUser_f, "Shows a client's user info", NULL);
	Cmd_AddCommand("serverrecord", SV_ServerRecord_f, NULL, NULL);
	Cmd_AddCommand("serverstoprecord", SV_ServerStopRecord_f, NULL, NULL);
	Cmd_AddCommand("killserver", SV_KillServer_f, "Kills the server", NULL);
	Cmd_AddCommand("sv", SV_ServerCommand_f, NULL, NULL);

	if (dedicated->integerValue)
		Cmd_AddCommand("say", SV_ConSay_f, NULL, NULL);
}

/*
 ==================
 SV_Shutdown

 Called from Com_Shutdown, but also when each game quits or an error is thrown
 ==================
*/
void SV_Shutdown (const char *message, bool reconnect){

	client_t	*client;
	int			i;

	if (!svs.initialized)
		return;		// Nothing running on the server

	Com_Printf("-------- Server Shutdown --------\n");

	// Send a final message
	SV_FinalMessage(message, reconnect);

	// Shutdown master server
	SV_ShutdownMaster();

	// Shutdown game module
	SG_Shutdown();

	// Close active server demo file
	if (sv.demoFile)
		FS_CloseFile(sv.demoFile);

	// Wipe the entire serverActive_t structure
	Mem_Fill(&sv, 0, sizeof(sv));

	// Close static server demo file
	if (svs.demoFile)
		FS_CloseFile(svs.demoFile);

	// Free clients
	if (svs.clients){
		for (i = 0, client = svs.clients; i < sv_maxClients->integerValue; i++, client++){
			// Close download file
			if (client->downloadFile)
				FS_CloseFile(client->downloadFile);
		}

		Mem_Free(svs.clients);
	}

	if (svs.clientEntities)
		Mem_Free(svs.clientEntities);

	// Wipe the entire serverStatic_t structure
	Mem_Fill(&svs, 0, sizeof(svs));

	// Free the clip map
	CM_FreeMap();

	// Set server state
	Com_SetServerState(sv.state);

	Com_Printf("---------------------------------\n");
}