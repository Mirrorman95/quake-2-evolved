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
// cl_main.c - Primary client file
//

// TODO:
// - file downloading
// - cvar clean up


#include "client.h"


clientState_t				cl;
clientStatic_t				cls;

cvar_t *					cl_skipRendering;
cvar_t *					cl_hand;
cvar_t *					cl_zoomFov;
cvar_t *					cl_drawGun;
cvar_t *					cl_drawShells;
cvar_t *					cl_footSteps;
cvar_t *					cl_noSkins;
cvar_t *					cl_predict;
cvar_t *					cl_maxFPS;
cvar_t *					cl_freeLook;
cvar_t *					cl_lookSpring;
cvar_t *					cl_lookStrafe;
cvar_t *					cl_upSpeed;
cvar_t *					cl_forwardSpeed;
cvar_t *					cl_sideSpeed;
cvar_t *					cl_yawSpeed;
cvar_t *					cl_pitchSpeed;
cvar_t *					cl_angleSpeedKey;
cvar_t *					cl_run;
cvar_t *					cl_noDelta;
cvar_t *					cl_showNet;
cvar_t *					cl_showMiss;
cvar_t *					cl_showEvents;
cvar_t *					cl_showMaterial;
cvar_t *					cl_timeOut;
cvar_t *					cl_visibleWeapons;
cvar_t *					cl_thirdPerson;
cvar_t *					cl_thirdPersonRange;
cvar_t *					cl_thirdPersonAngle;
cvar_t *					cl_viewBlend;
cvar_t *					cl_particles;
cvar_t *					cl_particleLOD;
cvar_t *					cl_particleBounce;
cvar_t *					cl_particleFriction;
cvar_t *					cl_particleVertexLight;
cvar_t *					cl_markTime;
cvar_t *					cl_brassTime;
cvar_t *					cl_blood;
cvar_t *					cl_testModelPitch;
cvar_t *					cl_testModelYaw;
cvar_t *					cl_testModelRoll;
cvar_t *					cl_testModelAnimate;
cvar_t *					cl_testModelRotatePitch;
cvar_t *					cl_testModelRotateYaw;
cvar_t *					cl_testModelRotateRoll;
cvar_t *					cl_testGunX;
cvar_t *					cl_testGunY;
cvar_t *					cl_testGunZ;
cvar_t *					cl_drawCrosshair;
cvar_t *					cl_crosshairX;
cvar_t *					cl_crosshairY;
cvar_t *					cl_crosshairSize;
cvar_t *					cl_crosshairColor;
cvar_t *					cl_crosshairAlpha;
cvar_t *					cl_crosshairHealth;
cvar_t *					cl_crosshairNames;
cvar_t *					cl_viewSize;
cvar_t *					cl_centerTime;
cvar_t *					cl_drawGodModeShell;
cvar_t *					cl_drawCenterString;
cvar_t *					cl_drawPause;
cvar_t *					cl_drawLogo;
cvar_t *					cl_drawFPS;
cvar_t *					cl_drawLagometer;
cvar_t *					cl_drawDisconnected;
cvar_t *					cl_drawRecording;
cvar_t *					cl_draw2D;
cvar_t *					cl_drawIcons;
cvar_t *					cl_drawStatus;
cvar_t *					cl_drawInventory;
cvar_t *					cl_drawLayout;
cvar_t *					cl_allowDownload;
cvar_t *					cl_rconPassword;
cvar_t *					cl_rconAddress;
cvar_t *					cl_itemBob;


/*
 ==================
 CL_LoadAssets
 ==================
*/
static void CL_LoadAssets (){

	// Get information about the OpenGL / OpenAL subsystems
	cls.glConfig = R_GetGLConfig();
	cls.alConfig = S_GetALConfig();

	// TODO: aspect stuff?

	// Load the materials
	cls.media.whiteMaterial = R_RegisterMaterialNoMip("white");
	cls.media.consoleMaterial = R_RegisterMaterialNoMip("console");
	cls.media.charsetMaterial = R_RegisterMaterialNoMip("charset");

	// Precache UI files if requested
	UI_Precache();
}

/*
 ==================
 CL_FullRestart

 Called by CL_RestartVideo_f and CL_RestartSound_f to perform a full restart if
 needed (to reload the whole level)
 ==================
*/
static void CL_FullRestart (){

	// No longer fully initialized
	cls.fullyInitialized = false;

	// Shutdown UI system
	UI_Shutdown();

	// Shutdown sound system and renderer
	S_Shutdown(true);
	R_Shutdown(true);

	// Initialize renderer and sound system
	R_Init(true);
	S_Init(true);

	// Initialize UI system
	UI_Init();

	// Load local assets
	CL_LoadAssets();

	// Set menu visibility
	if (cls.state == CA_DISCONNECTED)
		UI_SetActiveMenu(UI_MAINMENU);
	else
		UI_SetActiveMenu(UI_CLOSEMENU);

	// Load all media
	CL_LoadingState();
	CL_LoadLevel();

	// Fully initialized
	cls.fullyInitialized = true;
}

/*
 ==================
 CL_InitAll

 Called before loading a level or after disconnecting from the server to
 initialize all the necessary subsystems
 ==================
*/
void CL_InitAll (){

	if (cls.state == CA_UNINITIALIZED)
		return;		// Nothing running on the client

	// Initialize renderer and sound system
	R_Init(false);
	S_Init(false);

	// Initialize UI system
	UI_Init();

	// Load local assets
	CL_LoadAssets();

	// Set menu visibility
	if (cls.state == CA_DISCONNECTED)
		UI_SetActiveMenu(UI_MAINMENU);
	else
		UI_SetActiveMenu(UI_CLOSEMENU);

	// Fully initialized
	cls.fullyInitialized = true;
}

/*
 ==================
 CL_ShutdownAll

 Called before loading a level or after disconnecting from the server to shut
 down all the necessary subsystems
 ==================
*/
void CL_ShutdownAll (){

	if (cls.state == CA_UNINITIALIZED)
		return;		// Nothing running on the client

	// No longer fully initialized
	cls.fullyInitialized = false;

	// Shutdown UI system
	UI_Shutdown();

	// Shutdown sound system and renderer
	S_Shutdown(false);
	R_Shutdown(false);
}

/*
 ==================
 CL_ClearState
 ==================
*/
void CL_ClearState (){

	// Clear all local effects
	CL_ClearTempEntities();
	CL_ClearLocalEntities();
	CL_ClearDynamicLights();
	CL_ClearParticles();
	CL_ClearLightStyles();

	// Wipe the entire clientState_t structure
	Mem_Fill(&cl, 0, sizeof(clientState_t));

	MSG_Clear(&cls.netChan.message);
}

/*
 ==================
 CL_FixCheatVariables
 ==================
*/
static void CL_FixCheatVariables (){

	// Allow cheats if disconnected
	if (cls.state == CA_DISCONNECTED){
		CVar_FixCheatVariables(true);
		return;
	}

	// Allow cheats if playing a cinematic, demo or singleplayer game
	if (cls.playingCinematic || cl.demoPlayback || !cl.multiPlayer){
		CVar_FixCheatVariables(true);
		return;
	}

	// Otherwise don't allow cheats at all
	CVar_FixCheatVariables(false);
}

/*
 ==================
 CL_PlayBackgroundTrack
 ==================
*/
void CL_PlayBackgroundTrack (){

	char	name[MAX_PATH_LENGTH];
	int		track;

	track = Str_ToInteger(cl.configStrings[CS_CDTRACK]);

	// Stop any playing track
	if (track == 0){
		S_StopMusic();
		return;
	}

	// If an OGG file exists play it, otherwise fall back to CD audio
	Str_SPrintf(name, sizeof(name), "music/track%02i.ogg", track);

	if (!FS_FileExists(name))
		return;

	S_PlayMusic(name, name, 0);
}

/*
 ==================
 CL_CanLaunchEditor
 ==================
*/
bool CL_CanLaunchEditor (const char *editor){

	if (cls.state != CA_ACTIVE || cls.demoFile){
		Com_Printf("You must be in a level to launch the %s editor\n", editor);
		return false;
	}

	if (!CVar_AllowCheats()){
		Com_Printf("You must enable cheats to launch the %s editor\n", editor);
		return false;
	}

	if (Sys_IsWindowFullscreen()){
		Com_Printf("You must be running the game in windowed mode to launch the %s editor\n", editor);
		return false;
	}

	if (!NET_IsLocalAddress(cls.netChan.remoteAddress)){
		Com_Printf("You must be running a local server to launch the %s editor\n", editor);
		return false;
	}

	return true;
}


/*
 ==============================================================================

 SERVER COMMAND

 ==============================================================================
*/


/*
 ==================
 CL_ForwardCommandToServer

 Adds the current command line as a CLC_STRINGCMD to the client message.
 Things like "god", "noclip", etc, are commands directed to the server, so when
 they are typed in at the console, they will need to be forwarded.
 ==================
*/
void CL_ForwardCommandToServer (){

	char	*cmd;

	cmd = Cmd_Argv(0);

	if (cls.state != CA_ACTIVE || *cmd == '-' || *cmd == '+'){
		Com_Printf("Unknown command \"%s\"\n", cmd);
		return;
	}

	MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print(&cls.netChan.message, cmd);

	if (Cmd_Argc() > 1){
		MSG_Print(&cls.netChan.message, " ");
		MSG_Print(&cls.netChan.message, Cmd_Args());
	}
}


/*
 ==============================================================================

 SERVER REQUEST MESSAGE

 ==============================================================================
*/


/*
 ==================
 CL_SendChallengePacket

 Send a connect message to the server
 ==================
*/
static void CL_SendChallengePacket (){

	NET_OutOfBandPrintf(NS_CLIENT, cls.serverAddress, "getchallenge\n");
}

/*
 ==================
 CL_SendConnectPacket

 We have gotten a challenge from the server, so try and connect
 ==================
*/
static void CL_SendConnectPacket (){

	CVar_ClearModifiedFlags(CVAR_USERINFO);

	NET_OutOfBandPrintf(NS_CLIENT, cls.serverAddress, "connect %i %i %i \"%s\"\n", PROTOCOL_VERSION, CVar_GetVariableInteger("net_qport"), cls.serverChallenge, CVar_InfoString(CVAR_USERINFO));
}

/*
 ==================
 CL_CheckForResend

 TODO: this might need some rewritting

 Resend a connection request message if the last one has timed out
 ==================
*/
static void CL_CheckForResend (){

	if (cls.state >= CA_CONNECTED)
		return;

	if (cls.state == CA_DISCONNECTED){
		// If the local server is running, then connect
		if (Com_ServerState()){
			Str_Copy(cls.serverName, "localhost", sizeof(cls.serverName));

			if (!NET_StringToAddress(cls.serverName, &cls.serverAddress)){
				Com_Printf("Bad server address\n");
				return;
			}

			if (cls.serverAddress.port == 0)
				cls.serverAddress.port = BigShort(PORT_SERVER);

			Str_Copy(cls.serverMessage, "", sizeof(cls.serverMessage));

			// We don't need a challenge on the localhost
			cls.state = CA_CHALLENGING;

			CL_LoadingState();

			CL_SendConnectPacket();
		}

		return;
	}

	// Draw the loading screen
	CL_LoadingState();

	// Resend if we haven't gotten a reply yet
	if (cls.realTime - cls.connectTime < 3000.0f)
		return;

	cls.connectTime = cls.realTime;
	cls.connectCount++;

	switch (cls.state){
	case CA_CONNECTING:
		// Request a challenge
		CL_SendChallengePacket();

		break;
	case CA_CHALLENGING:
		// Request a connection
		CL_SendConnectPacket();

		break;
	}
}


/*
 ==============================================================================

 CLIENT DISCONNECTION

 ==============================================================================
*/


/*
 ==================
 CL_Disconnect

 Called when a connection, demo, or cinematic is being terminated.
 Sends a disconnect message to the server and goes to the main menu
 unless shuttingDown is true.
 This is also called when an error is thrown, so it shouldn't cause any errors.
 ==================
*/
void CL_Disconnect (bool shuttingDown){

	byte	final[32];
	int		timeDemoMsec, timeDemoFrames;

	if (cls.state == CA_DISCONNECTED)
		return;

	if (com_timeDemo->integerValue){
		timeDemoMsec = Sys_Milliseconds() - cl.timeDemoStart;
		timeDemoFrames = cl.timeDemoFrames;
	}

	cls.state = CA_DISCONNECTED;
	cls.connectTime = 0;
	cls.connectCount = 0;
	cls.loading = false;

	// Stop demo recording
	if (cls.demoFile)
		CL_StopRecord_f();

	// Stop download
	if (cls.downloadFile){
		FS_CloseFile(cls.downloadFile);
		cls.downloadFile = 0;
	}

	// Stop cinematic
	CL_StopCinematic();

	// Send a disconnect message to the server (send it a few times in case one
	// is dropped)
	final[0] = CLC_STRINGCMD;
	Str_Copy((char *)final + 1, "disconnect", sizeof(final) - 1);
	NetChan_Transmit(&cls.netChan, final, Str_Length(final));
	NetChan_Transmit(&cls.netChan, final, Str_Length(final));
	NetChan_Transmit(&cls.netChan, final, Str_Length(final));

	// Clear the client state
	CL_ClearState();

	// Free current level
	CM_FreeMap();

	if (shuttingDown)
		return;

	// Restart the subsystems
	CL_ShutdownAll();
	CL_InitAll();

	if (com_timeDemo->integerValue){
		if (timeDemoMsec > 0)
			Com_Printf("%i frames, %3.1f seconds: %3.1f FPS\n", timeDemoFrames, timeDemoMsec / 1000.0, timeDemoFrames * 1000.0 / timeDemoMsec);
	}
}

/*
 ==================
 CL_Drop

 Called after a non-fatal error was thrown
 ==================
*/
void CL_Drop (){

	if (cls.state == CA_UNINITIALIZED)
		return;
	if (cls.state == CA_DISCONNECTED)
		return;

	CL_Disconnect(false);

	// If not fully initialized at this point, we can't display anything
	if (!cls.fullyInitialized)
		Com_Error(ERR_FATAL, "Client system not fully initialized");
}

/*
 ==================
 CL_MapLoading

 A local server is starting to load a map, so update the screen to let
 the user know about it
 ==================
*/
void CL_MapLoading (){

	if (cls.state == CA_UNINITIALIZED)
		return;

	// If we're already connected to the local server, stay connected
	if (cls.state >= CA_CONNECTED && NET_IsLocalAddress(cls.serverAddress)){
		cls.state = CA_CONNECTED;

		// Draw the loading screen
		CL_LoadingState();
		CL_UpdateScreen();

		return;
	}

	// Disconnect from server
	CL_Disconnect(false);

	// Connect to the local server
	Str_Copy(cls.serverName, "localhost", sizeof(cls.serverName));
	Str_Copy(cls.serverMessage, "", sizeof(cls.serverMessage));

	NET_StringToAddress(cls.serverName, &cls.serverAddress);

	// CL_CheckForResend will fire immediately
	cls.state = CA_CHALLENGING;		// We don't need a challenge on the localhost
	cls.connectTime = -99999;
	cls.connectCount = 0;

	// Draw the loading screen
	CL_LoadingState();
	CL_UpdateScreen();
}


/*
 ==============================================================================

 SERVER PACKET PARSING

 ==============================================================================
*/


/*
 ==================
 CL_ChallengePacket
 ==================
*/
static void CL_ChallengePacket (){

	if (cls.state != CA_CONNECTING){
		if (cls.state == CA_CHALLENGING)
			Com_Printf("Dup 'challenge' received. Ignored\n");
		else
			Com_Printf("Unwanted 'challenge' received. Ignored\n");

		return;
	}

	if (!NET_CompareAddress(net_from, cls.serverAddress)){
		Com_Printf("'challenge' received from invalid address. Ignored.\n");
		return;
	}

	Str_Copy(cls.serverMessage, "", sizeof(cls.serverMessage));

	cls.serverChallenge = Str_ToInteger(Cmd_Argv(1));

	// CL_CheckForResend will fire immediately
	cls.state = CA_CHALLENGING;

	cls.connectTime = -99999;
	cls.connectCount = 0;
}

/*
 ==================
 CL_ClientConnectPacket
 ==================
*/
static void CL_ClientConnectPacket (){

	if (cls.state != CA_CHALLENGING){
		if (cls.state == CA_CONNECTED)
			Com_Printf("Dup 'connect' received. Ignored\n");
		else
			Com_Printf("Unwanted 'connect' received. Ignored\n");

		return;
	}

	if (!NET_CompareAddress(net_from, cls.serverAddress)){
		Com_Printf("'connect' received from invalid address. Ignored.\n");
		return;
	}

	// Set up the network channel
	cls.state = CA_CONNECTED;

	NetChan_Setup(&cls.netChan, NS_CLIENT, net_from, CVar_GetVariableInteger("net_qport"));

	MSG_WriteChar(&cls.netChan.message, CLC_STRINGCMD);
	MSG_WriteString(&cls.netChan.message, "new");
}

/*
 ==================
 CL_CommandPacket
 ==================
*/
static void CL_CommandPacket (){

	char	*string;

	if (!NET_IsLocalAddress(net_from)){
		Com_Printf("Command packet from remote host. Ignored\n");
		return;
	}

	string = MSG_ReadString(&net_message);
	Cmd_AppendText(string);
	Cmd_AppendText("\n");
}

/*
 ==================
 CL_InfoPacket
 ==================
*/
static void CL_InfoPacket (){

	char	*string;

	string = MSG_ReadString(&net_message);
	UI_AddServerToList(net_from, string);

	Com_Printf("%s\n", string);
}

/*
 ==================
 CL_PrintPacket
 ==================
*/
static void CL_PrintPacket (){

	char	*string;

	string = MSG_ReadString(&net_message);
	Str_Copy(cls.serverMessage, string, sizeof(cls.serverMessage));

	Com_Printf("%s", string);
}

/*
 ==================
 CL_PingPacket
 ==================
*/
static void CL_PingPacket (){

	NET_OutOfBandPrintf(NS_CLIENT, net_from, "ack");
}

/*
 ==================
 CL_Echo

 Echo request from server
 ==================
*/
static void CL_EchoPacket (){

	NET_OutOfBandPrintf(NS_CLIENT, net_from, "%s", Cmd_Argv(1));
}

/*
 ==================
 CL_ConnectionlessPacket
 ==================
*/
static void CL_ConnectionlessPacket (){

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

	Com_DPrintf("Server out-of-band packet from %s: %s\n", NET_AddressToString(net_from), cmd);

	// Server responding to a 'getChallenge' command
	if (!Str_ICompare(cmd, "challenge")){
		CL_ChallengePacket();
		return;
	}

	// Server responding to a 'connect' command
	if (!Str_ICompare(cmd, "client_connect")){
		CL_ClientConnectPacket();
		return;
	}

	// Server responding to a 'rcon' command
	if (!Str_ICompare(cmd, "cmd")){
		CL_CommandPacket();
		return;
	}

	// Server responding to a 'status' command
	if (!Str_ICompare(cmd, "info")){
		CL_InfoPacket();
		return;
	}

	// TODO
	if (!Str_ICompare(cmd, "print")){
		CL_PrintPacket();
		return;
	}

	// Server responding to a 'ack' command
	if (!Str_ICompare(cmd, "ping")){
		CL_PingPacket();
		return;
	}

	// TODO
	if (!Str_ICompare(cmd, "echo")){
		CL_EchoPacket();
		return;
	}

	Com_DPrintf(S_COLOR_YELLOW "Bad server out-of-band packet from %s:\n" S_COLOR_YELLOW "%s\n", NET_AddressToString(net_from), string);
}

/*
 ==================
 CL_ParsePacket
 ==================
*/
static void CL_ParsePacket (){

	// Check for connectionless packet first
	if (*(int *)net_message.data == -1){
		CL_ConnectionlessPacket();
		return;
	}

	// Dump it if not connected
	if (cls.state < CA_CONNECTED)
		return;

	if (net_message.curSize < 8){
		Com_DPrintf(S_COLOR_YELLOW "Runt server packet from %s\n", NET_AddressToString(net_from));
		return;
	}

	if (!NET_CompareAddress(net_from, cls.netChan.remoteAddress)){
		Com_DPrintf(S_COLOR_YELLOW "Sequenced server packet without connection from %s\n", NET_AddressToString(net_from));
		return;
	}

	// Packet from server
	if (!NetChan_Process(&cls.netChan, &net_message))
		return;		// Out of order, duplicated, etc

	// Parse the message
	CL_ParseServerMessage();
}

/*
 ==================
 CL_ReadPackets
 ==================
*/
void CL_ReadPackets (){

	// Read incoming network packets
	while (NET_GetPacket(NS_CLIENT, &net_from, &net_message)){
		// Parse the packet
		CL_ParsePacket();
	}

	// Check time-out
	if (cls.state >= CA_CONNECTED && cls.realTime - cls.netChan.lastReceived > cl_timeOut->floatValue * 1000.0f){
		if (++cl.timeOutCount > 5){		// timeOutCount saves debugger
			Com_Printf("Server connection timed out\n");
			CL_Disconnect(false);
			return;
		}
	}
	else
		cl.timeOutCount = 0;
}


/*
 ==============================================================================

 FILE DOWNLOADING

 ==============================================================================
*/

#define PLAYER_MULT					5

#define ENV_CNT						(CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define TEX_CNT						(ENV_CNT + 6)

static const char *			cl_skySuffix[6] = {"rt", "lf", "bk", "ft", "up", "dn"};

static int					cl_precacheCheck;
static int					cl_precacheSpawnCount;
static byte *				cl_precacheModel;
static int					cl_precacheModelSkin;
static byte *				cl_precacheMap;
static int					cl_precacheTexture;


/*
 ==================
 
 Returns true if the file exists, otherwise it attempts to start a
 download from the server
 ==================
*/
static bool CL_CheckOrDownloadFile (const char *name){

	int		length;

	if (FS_FileExists(name))
		return true;	// It exists, no need to download

	if (Str_FindText(name, "..", false) || !Str_FindText(name, "/", false) || name[0] == '.' || name[0] == '/'){
		Com_Printf("Refusing to download %s\n", name);
		return true;
	}

	Str_Copy(cls.downloadName, name, sizeof(cls.downloadName));

	// Download to a temp name, and only rename to the real name when 
	// done, so if interrupted a runt file wont be left
	Com_StripExtension(cls.downloadName, cls.downloadTempName, sizeof(cls.downloadTempName));
	Com_DefaultExtension(cls.downloadTempName, sizeof(cls.downloadTempName), ".tmp");

	length = FS_OpenFile(cls.downloadTempName, FS_APPEND, &cls.downloadFile);
	if (!cls.downloadFile){
		Com_Printf("Failed to create %s\n", cls.downloadTempName);
		return true;
	}

	cls.downloadStart = cls.realTime;
	cls.downloadBytes = 0;
	cls.downloadPercent = 0;

	if (length){
		FS_Seek(cls.downloadFile, 0, FS_SEEK_END);

		Com_Printf("Resuming %s...\n", cls.downloadName);
		MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString(&cls.netChan.message, Str_VarArgs("download %s %i", cls.downloadName, length));
	} 
	else {
		Com_Printf("Downloading %s...\n", cls.downloadName);
		MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString(&cls.netChan.message, Str_VarArgs("download %s", cls.downloadName));
	}

	return false;
}

/*
 ==================
 
 TODO: extend to support TGA textures, MD3 models, etc... (PAK/PK3?)
 ==================
*/
void CL_RequestNextDownload (){

	char			name[MAX_PATH_LENGTH], *p;
	char			model[MAX_PATH_LENGTH], skin[MAX_PATH_LENGTH];
	md2Header_t		*md2;
	bspHeader_t		*header;
	bspTexInfo_t	*texInfo;
	int				numTexInfo;
	int				i, n;

	if (cls.state != CA_CONNECTED)
		return;

	if (!cl_allowDownload->integerValue)
		cl_precacheCheck = TEX_CNT + 1;

	// Check map
	if (cl_precacheCheck == CS_MODELS){
		cl_precacheCheck = CS_MODELS + 2;	// 0 isn't used

		if (!CL_CheckOrDownloadFile(cl.configStrings[CS_MODELS + 1]))
			return;		// Started a download
	}

	// Check models
	if (cl_precacheCheck >= CS_MODELS && cl_precacheCheck < CS_MODELS + MAX_MODELS){
		while (cl_precacheCheck < CS_MODELS + MAX_MODELS && cl.configStrings[cl_precacheCheck][0]){
			if (cl.configStrings[cl_precacheCheck][0] == '*' || cl.configStrings[cl_precacheCheck][0] == '#'){
				cl_precacheCheck++;
				continue;
			}

			if (cl_precacheModelSkin == -1){
				if (!CL_CheckOrDownloadFile(cl.configStrings[cl_precacheCheck])){
					cl_precacheModelSkin = 0;
					return;		// Started a download
				}

				cl_precacheModelSkin = 0;
			}

			// Check skins in the model
			if (!cl_precacheModel){
				FS_ReadFile(cl.configStrings[cl_precacheCheck], (void **)&cl_precacheModel);
				if (!cl_precacheModel){
					// Couldn't load it
					cl_precacheModelSkin = -1;

					cl_precacheCheck++;
					continue;
				}

				md2 = (md2Header_t *)cl_precacheModel;

				if (LittleLong(md2->id) != MD2_ID || LittleLong(md2->version) != MD2_VERSION){
					// Not an alias model or wrong version
					FS_FreeFile(cl_precacheModel);
					cl_precacheModel = NULL;
					cl_precacheModelSkin = -1;

					cl_precacheCheck++;
					continue;
				}
			}

			md2 = (md2Header_t *)cl_precacheModel;

			while (cl_precacheModelSkin < LittleLong(md2->numSkins)){
				Str_Copy(name, (char *)cl_precacheModel + LittleLong(md2->ofsSkins) + cl_precacheModelSkin * MAX_PATH_LENGTH, sizeof(name));
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheModelSkin++;
					return;		// Started a download
				}

				cl_precacheModelSkin++;
			}

			FS_FreeFile(cl_precacheModel);
			cl_precacheModel = NULL;
			cl_precacheModelSkin = -1;

			cl_precacheCheck++;
		}

		cl_precacheCheck = CS_SOUNDS;
	}

	// Check sounds
	if (cl_precacheCheck >= CS_SOUNDS && cl_precacheCheck < CS_SOUNDS+MAX_SOUNDS){
		if (cl_precacheCheck == CS_SOUNDS)
			cl_precacheCheck++;		// 0 isn't used

		while (cl_precacheCheck < CS_SOUNDS + MAX_SOUNDS && cl.configStrings[cl_precacheCheck][0]){
			if (cl.configStrings[cl_precacheCheck][0] == '*'){
				cl_precacheCheck++;
				continue;
			}

			Str_SPrintf(name, sizeof(name), "sound/%s", cl.configStrings[cl_precacheCheck++]);
			if (!CL_CheckOrDownloadFile(name))
				return;		// Started a download
		}

		cl_precacheCheck = CS_IMAGES;
	}

	// Check images
	if (cl_precacheCheck >= CS_IMAGES && cl_precacheCheck < CS_IMAGES+MAX_IMAGES){
		if (cl_precacheCheck == CS_IMAGES)
			cl_precacheCheck++;		// 0 isn't used

		while (cl_precacheCheck < CS_IMAGES + MAX_IMAGES && cl.configStrings[cl_precacheCheck][0]){
			Str_SPrintf(name, sizeof(name), "pics/%s.pcx", cl.configStrings[cl_precacheCheck++]);
			if (!CL_CheckOrDownloadFile(name))
				return;		// Started a download
		}

		cl_precacheCheck = CS_PLAYERSKINS;
	}

	// Check player skins
	if (cl_precacheCheck >= CS_PLAYERSKINS && cl_precacheCheck < CS_PLAYERSKINS+MAX_CLIENTS * PLAYER_MULT){
		while (cl_precacheCheck < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT){
			i = (cl_precacheCheck - CS_PLAYERSKINS) / PLAYER_MULT;
			n = (cl_precacheCheck - CS_PLAYERSKINS) % PLAYER_MULT;

			if (cl.configStrings[CS_PLAYERSKINS + i][0] == 0){
				cl_precacheCheck = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				continue;
			}

			p = Str_FindChar(cl.configStrings[CS_PLAYERSKINS+i], '\\');
			if (p)
				p++;
			else
				p = cl.configStrings[CS_PLAYERSKINS + i];

			Str_Copy(model, p, sizeof(model));
			p = Str_FindChar(model, '/');
			if (!p)
				p = Str_FindChar(model, '\\');
			if (p){
				*p++ = 0;
				Str_Copy(skin, p, sizeof(skin));
			}
			else
				skin[0] = 0;

			switch (n){
			case 0:		// Model
				Str_SPrintf(name, sizeof(name), "players/%s/tris.md2", model);
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 1;
					return;		// Started a download
				}
				n++;
				// Fall through

			case 1:		// Weapon model
				Str_SPrintf(name, sizeof(name), "players/%s/weapon.md2", model);
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 2;
					return;		// Started a download
				}
				n++;
				// Fall through

			case 2:		// Weapon skin
				Str_SPrintf(name, sizeof(name), "players/%s/weapon.pcx", model);
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 3;
					return;		// Started a download
				}
				n++;
				// Fall through

			case 3:		// Skin
				Str_SPrintf(name, sizeof(name), "players/%s/%s.pcx", model, skin);
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 4;
					return;		// Started a download
				}
				n++;
				// Fall through

			case 4:		// Icon
				Str_SPrintf(name, sizeof(name), "players/%s/%s_i.pcx", model, skin);
				if (!CL_CheckOrDownloadFile(name)){
					cl_precacheCheck = CS_PLAYERSKINS + i * PLAYER_MULT + 5;
					return;		// Started a download
				}

				// Move on to next model
				cl_precacheCheck = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
			}
		}

		cl_precacheCheck = ENV_CNT;
	}

	// Check skies
	if (cl_precacheCheck >= ENV_CNT && cl_precacheCheck < TEX_CNT){
		while (cl_precacheCheck < TEX_CNT){
			n = cl_precacheCheck++ - ENV_CNT;
			Str_SPrintf(name, sizeof(name), "env/%s%s.tga", cl.configStrings[CS_SKY], cl_skySuffix[n]);
			if (!CL_CheckOrDownloadFile(name))
				return;		// Started a download
		}

		cl_precacheCheck = TEX_CNT;
	}

	// Check textures
	if (cl_precacheCheck == TEX_CNT){
		while (cl_precacheCheck == TEX_CNT){
			if (!cl_precacheMap){
				FS_ReadFile(cl.configStrings[CS_MODELS + 1], (void **)&cl_precacheMap);
				if (!cl_precacheMap){
					// Couldn't load it
					cl_precacheCheck++;
					continue;
				}

				header = (bspHeader_t *)cl_precacheMap;

				if (LittleLong(header->id) != BSP_ID || LittleLong(header->version) != BSP_VERSION){
					// Not a map or wrong version
					FS_FreeFile(cl_precacheMap);
					cl_precacheMap = NULL;
					cl_precacheTexture = 0;

					cl_precacheCheck++;
					continue;
				}
			}

			header = (bspHeader_t *)cl_precacheMap;

			texInfo = (bspTexInfo_t *)(cl_precacheMap + LittleLong(header->lumps[LUMP_TEXINFO].offset));
			numTexInfo = LittleLong(header->lumps[LUMP_TEXINFO].length) / sizeof(bspTexInfo_t);

			while (cl_precacheTexture < numTexInfo){
				Str_SPrintf(name, sizeof(name), "textures/%s.wal", texInfo[cl_precacheTexture++].texture);
				if (!CL_CheckOrDownloadFile(name))
					return;		// Started a download
			}

			FS_FreeFile(cl_precacheMap);
			cl_precacheMap = NULL;
			cl_precacheTexture = 0;

			cl_precacheCheck++;
		}
	}

	// Load level
	CL_LoadLevel();

	MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
	MSG_WriteString(&cls.netChan.message, Str_VarArgs("begin %i\n", cl_precacheSpawnCount));
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 CL_Precache_f

 The server will send this command right before allowing the client into
 the server
 ==================
*/
static void CL_Precache_f (){

	if (cls.state != CA_CONNECTED)
		return;

	if (!cl.configStrings[CS_MODELS + 1][0])
		return;

	// Restart the subsystems
	CL_ShutdownAll();
	CL_InitAll();

	// HACK: to let old demos work with the old precache sequence
	if (Cmd_Argc() < 2){
		CL_LoadLevel();
		return;
	}

	cl_precacheCheck = CS_MODELS;
	cl_precacheSpawnCount = Str_ToInteger(Cmd_Argv(1));
	cl_precacheModel = NULL;
	cl_precacheModelSkin = -1;
	cl_precacheMap = NULL;
	cl_precacheTexture = 0;

	if (Com_ServerState() || !cl_allowDownload->integerValue)
		cl_precacheCheck = TEX_CNT + 1;

	CL_RequestNextDownload();
}

/*
 ==================
 CL_Changing_f

 The server will send this command as a hint that the client should draw
 the loading screen
 ==================
*/
static void CL_Changing_f (){

	Com_Printf("Changing map...\n");

	// Not active anymore, but not disconnected
	cls.state = CA_CONNECTED;

	CL_LoadingState();
}

/*
 ==================
 CL_ForwardToServer_f
 ==================
*/
static void CL_ForwardToServer_f (){

	if (cls.state < CA_CONNECTED){
		Com_Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	// Don't forward the first argument
	if (Cmd_Argc() > 1){
		MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
		MSG_Print(&cls.netChan.message, Cmd_Args());
	}
}

/*
 ==================
 CL_Connect_f
 ==================
*/
static void CL_Connect_f (){

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: connect <server>\n");
		return;	
	}

	Str_Copy(cls.serverName, Cmd_Argv(1), sizeof(cls.serverName));

	// If running a local server, kill it and reissue
	SV_Shutdown("Server quit\n", false);

	// Disconnect from server
	CL_Disconnect(false);

	// Resolve server address
	if (!NET_StringToAddress(cls.serverName, &cls.serverAddress)){
		Com_Printf("Couldn't resolve %s\n", cls.serverName);
		return;
	}

	if (cls.serverAddress.port == 0)
		cls.serverAddress.port = BigShort(PORT_SERVER);

	if (NET_IsLocalAddress(cls.serverAddress)){
		Com_Printf("Can't connect to localhost\n");
		return;
	}

	Com_Printf("%s resolved to %s\n", cls.serverName, NET_AddressToString(cls.serverAddress));

	Str_Copy(cls.serverMessage, "", sizeof(cls.serverMessage));

	// CL_CheckForResend will fire immediately
	cls.state = CA_CONNECTING;

	cls.connectTime = -99999;
	cls.connectCount = 0;

	Com_Printf("Connecting to %s...\n", cls.serverName);
}

/*
 ==================
 CL_Reconnect_f
 ==================
*/
static void CL_Reconnect_f (){

	if (!cls.serverName[0]){
		Com_Printf("No server to reconnect\n");
		return;
	}

	// If connected, the server is changing levels
	if (cls.state == CA_CONNECTED){
		Com_Printf("Reconnecting...\n");

		// Close download
		if (cls.downloadFile){
			FS_CloseFile(cls.downloadFile);
			cls.downloadFile = 0;
		}

		MSG_WriteChar(&cls.netChan.message, CLC_STRINGCMD);
		MSG_WriteString(&cls.netChan.message, "new");		
		return;
	}

	// If running a local server, kill it and reissue
	SV_Shutdown("Server quit\n", false);

	// Disconnect from server
	CL_Disconnect(false);

	// Resolve server address
	if (!NET_StringToAddress(cls.serverName, &cls.serverAddress)){
		Com_Printf("Couldn't resolve %s\n", cls.serverName);
		return;
	}

	if (cls.serverAddress.port == 0)
		cls.serverAddress.port = BigShort(PORT_SERVER);

	if (NET_IsLocalAddress(cls.serverAddress)){
		Com_Printf("Can't reconnect to localhost\n");
		return;
	}

	Com_Printf("%s resolved to %s\n", cls.serverName, NET_AddressToString(cls.serverAddress));

	Str_Copy(cls.serverMessage, "", sizeof(cls.serverMessage));

	// CL_CheckForResend will fire immediately
	cls.state = CA_CONNECTING;

	cls.connectTime = -99999;
	cls.connectCount = 0;

	Com_Printf("Reconnecting to %s...\n", cls.serverName);
}

/*
 ==================
 CL_Disconnect_f
 ==================
*/
static void CL_Disconnect_f (){

	if (cls.state == CA_DISCONNECTED){
		Com_Printf("Not connected to a server\n");
		return;
	}

	Com_Error(ERR_DROP, "Disconnected from server");
}

/*
 ==================
 CL_LocalServers_f
 ==================
*/
static void CL_LocalServers_f (){

	netAdr_t	adr;

	Com_Printf("Scanning for servers on the local network...\n");

	// Send a broadcast packet
	adr.type = NA_BROADCAST;
	adr.port = BigShort(PORT_SERVER);

	NET_OutOfBandPrintf(NS_CLIENT, adr, "info %i", PROTOCOL_VERSION);
}

/*
 ==================
 CL_PingServers_f
 ==================
*/
static void CL_PingServers_f (){

	int			i;
	netAdr_t	adr;
	const char	*adrString;

	Com_Printf("Pinging favorites...\n");

	// Send a packet to each address book entry
	for (i = 0; i < 16; i++){
		adrString = CVar_GetVariableString(Str_VarArgs("server%i", i + 1));
		if (!adrString[0])
			continue;

		Com_Printf("Pinging %s...\n", adrString);

		if (!NET_StringToAddress(adrString, &adr)){
			Com_Printf("Bad server address\n");
			continue;
		}

		if (adr.port == 0)
			adr.port = BigShort(PORT_SERVER);

		NET_OutOfBandPrintf(NS_CLIENT, adr, "info %i", PROTOCOL_VERSION);
	}
}

/*
 ==================
 CL_Ping_f
 ==================
*/
static void CL_Ping_f (){

	netAdr_t	adr;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: ping <server>\n");
		return;
	}

	// Resolve server address
	if (!NET_StringToAddress(Cmd_Argv(1), &adr)){
		Com_Printf("Couldn't resolve %s\n", Cmd_Argv(1));
		return;
	}

	if (adr.port == 0)
		adr.port = BigShort(PORT_SERVER);

	Com_Printf("%s resolved to %s\n", Cmd_Argv(1), NET_AddressToString(adr));

	// Send a packet to the specified address
	Com_Printf("Pinging %s...\n", Cmd_Argv(1));

	NET_OutOfBandPrintf(NS_CLIENT, adr, "info %i", PROTOCOL_VERSION);
}

/*
 ==================
 CL_Rcon_f

 Send the rest of the command line over as an unconnected command
 ==================
*/
static void CL_Rcon_f (){

	netAdr_t	to;
	char		message[1024];
	int			i;

	if (cls.state >= CA_CONNECTED)
		to = cls.netChan.remoteAddress;
	else {
		if (!cl_rconAddress->value[0]){
			Com_Printf("You must either be connected, or set the \"rconAddress\" cvar to issue rcon commands\n");
			return;
		}

		if (!NET_StringToAddress(cl_rconAddress->value, &to)){
			Com_Printf("Bad remote server address\n");
			return;
		}
		if (to.port == 0)
			to.port = BigShort(PORT_SERVER);
	}

	if (!cl_rconPassword->value[0]){
		Com_Printf("You must set the \"rconPassword\" cvar to issue rcon commands\n");
		return;
	}

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	Str_Append(message, "rcon ", sizeof(message));
	Str_Append(message, cl_rconPassword->value, sizeof(message));
	Str_Append(message, " ", sizeof(message));

	for (i = 1; i < Cmd_Argc(); i++){
		Str_Append(message, Cmd_Argv(i),sizeof(message));
		Str_Append(message, " ", sizeof(message));
	}

	NET_SendPacket(NS_CLIENT, to, message, Str_Length(message) + 1);
}

/*
 ==================
 CL_UserInfo_f
 ==================
*/
static void CL_UserInfo_f (){

	Com_Printf("User info settings:\n");
	Com_Printf("-------------------\n");
	Info_Print(CVar_InfoString(CVAR_USERINFO));
}

/*
 ==================
 CL_RestartVideo_f
 ==================
*/
static void CL_RestartVideo_f (){

	// If needed, perform a full restart so the level is reloaded
	if (cls.state >= CA_PRIMED){
		CL_FullRestart();
		return;
	}

	// No longer fully initialized
	cls.fullyInitialized = false;

	// Shutdown UI system
	UI_Shutdown();

	// Restart the renderer
	R_Shutdown(true);
	R_Init(true);

	// Initialize UI system
	UI_Init();

	// Load local assets
	CL_LoadAssets();

	// Set menu visibility
	if (cls.state == CA_DISCONNECTED)
		UI_SetActiveMenu(UI_MAINMENU);
	else
		UI_SetActiveMenu(UI_CLOSEMENU);

	// Fully initialized
	cls.fullyInitialized = true;
}

/*
 ==================
 CL_RestartSound_f
 ==================
*/
static void CL_RestartSound_f (){

	// If needed, perform a full restart so the level is reloaded
	if (cls.state >= CA_PRIMED){
		CL_FullRestart();
		return;
	}

	// No longer fully initialized
	cls.fullyInitialized = false;

	// Shutdown UI system
	UI_Shutdown();

	// Restart the sound system
	S_Shutdown(true);
	S_Init(true);

	// Initialize UI system
	UI_Init();

	// Load local assets
	CL_LoadAssets();

	// Set menu visibility
	if (cls.state == CA_DISCONNECTED)
		UI_SetActiveMenu(UI_MAINMENU);
	else
		UI_SetActiveMenu(UI_CLOSEMENU);

	// Fully initialized
	cls.fullyInitialized = true;
}

/*
 ==================
 CL_Skins_f

 List and load any custom player skins and models
 ==================
*/
static void CL_Skins_f (){

	int		i;

	if (cls.state < CA_CONNECTED){
		Com_Printf("Not connected to a server\n");
		return;
	}

	for (i = 0; i < MAX_CLIENTS; i++){
		if (!cl.configStrings[CS_PLAYERSKINS + i][0])
			continue;

		Com_Printf("Client %i: %s\n", i, cl.configStrings[CS_PLAYERSKINS + i]);

		CL_LoadClientInfo(&cl.clientInfo[i], cl.configStrings[CS_PLAYERSKINS + i]);
	}
}

/*
 ==================
 CL_ConfigStrings_f
 ==================
*/
static void CL_ConfigStrings_f (){

	int		i;

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: configStrings [index]\n");
		return;
	}

	if (cls.state < CA_CONNECTED){
		Com_Printf("Not connected to a server\n");
		return;
	}

	if (Cmd_Argc() == 1){
		for (i = 0; i < MAX_CONFIGSTRINGS; i++){
			if (!cl.configStrings[i][0])
				continue;

			Com_Printf("%4i: %s\n", i, cl.configStrings[i]);
		}

		return;
	}

	i = Str_ToInteger(Cmd_Argv(1));
	if (i < 0 || i >= MAX_CONFIGSTRINGS){
		Com_Printf("Bad configstring index %i\n", i);
		return;
	}

	Com_Printf("%4i: %s\n", i, cl.configStrings[i]);
}


/*
 ==============================================================================

 CONFIG REGISTRATION

 ==============================================================================
*/


/*
 ==================
 CL_Register
 ==================
*/
static void CL_Register (){

	cls.state = CA_DISCONNECTED;
	cls.realTime = Sys_Milliseconds();

	// Register variables
	CVar_Register("server1", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server2", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server3", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server4", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server5", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server6", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server7", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server8", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server9", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server10", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server11", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server12", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server13", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server14", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server15", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("server16", "", CVAR_STRING, CVAR_ARCHIVE, "Favorite server address", 0, 0);
	CVar_Register("name", "Player", CVAR_STRING, CVAR_USERINFO | CVAR_ARCHIVE, "Player name", 0, 0);
	CVar_Register("skin", "male/grunt", CVAR_STRING, CVAR_USERINFO | CVAR_ARCHIVE, NULL, 0, 0);
	CVar_Register("gender", "male", CVAR_STRING, CVAR_USERINFO | CVAR_ARCHIVE, NULL, 0, 0);
	CVar_Register("msg", "1", CVAR_INTEGER, CVAR_USERINFO | CVAR_ARCHIVE, NULL, 0, 10);
	CVar_Register("password", "", CVAR_STRING, CVAR_USERINFO, NULL, 0, 0);
	CVar_Register("spectator", "0", CVAR_BOOL, CVAR_USERINFO, NULL, 0, 0);
	CVar_Register("rate", "25000", CVAR_INTEGER, CVAR_USERINFO | CVAR_ARCHIVE, NULL, 0, 50000);
	CVar_Register("fov", "90.0f", CVAR_FLOAT, CVAR_USERINFO | CVAR_ARCHIVE, NULL, 0.0f, 200.0f);
	cl_skipRendering = CVar_Register("cl_skipRendering", "0", CVAR_BOOL, CVAR_CHEAT | CVAR_GAME, "Skip all game rendering", 0, 0);
	
	cl_hand = CVar_Register("hand", "0", CVAR_INTEGER, CVAR_USERINFO | CVAR_ARCHIVE, "Hand position", 0, 3);
	cl_zoomFov = CVar_Register("cl_zoomFov", "25", CVAR_FLOAT, CVAR_ARCHIVE, NULL, 0.0f, 90.0f);
	cl_drawGun = CVar_Register("cl_drawGun", "1", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_drawShells = CVar_Register("cl_drawShells", "1", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_footSteps = CVar_Register("cl_footSteps", "1", CVAR_BOOL, CVAR_CHEAT, "Use footstep sounds", 0, 0);
	cl_noSkins = CVar_Register("cl_noSkins", "0", CVAR_BOOL, 0, NULL, 0, 0);
	cl_predict = CVar_Register("cl_predict", "1", CVAR_BOOL, CVAR_ARCHIVE, "Predict player movement", 0, 0);
	cl_maxFPS = CVar_Register("cl_maxFPS", "90", CVAR_INTEGER, CVAR_ARCHIVE, NULL, 0, 500);
	cl_freeLook = CVar_Register("cl_freeLook", "1", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_lookSpring = CVar_Register("cl_lookSpring", "0", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_lookStrafe = CVar_Register("cl_lookStrafe", "0", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_upSpeed = CVar_Register("cl_upSpeed", "200", CVAR_FLOAT, CVAR_ARCHIVE, NULL, 0.0f, 500.0f);
	cl_forwardSpeed = CVar_Register("cl_forwardSpeed", "200", CVAR_FLOAT, CVAR_ARCHIVE, NULL, 0.0f, 500.0f);
	cl_sideSpeed = CVar_Register("cl_sideSpeed", "200", CVAR_FLOAT, CVAR_ARCHIVE, NULL, 0.0f, 500.0f);
	cl_yawSpeed = CVar_Register("cl_yawSpeed", "140", CVAR_FLOAT, CVAR_ARCHIVE, NULL, 0.0f, 500.0f);
	cl_pitchSpeed = CVar_Register("cl_pitchSpeed", "150", CVAR_FLOAT, CVAR_ARCHIVE, NULL, 0.0f, 500.0f);
	cl_angleSpeedKey = CVar_Register("cl_angleSpeedKey", "1.5", CVAR_FLOAT, CVAR_ARCHIVE, NULL, 0.0f, 100.0f);
	cl_run = CVar_Register("cl_run", "0", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_noDelta = CVar_Register("cl_noDelta", "0", CVAR_BOOL, 0, NULL, 0, 0);
	cl_showNet = CVar_Register("cl_showNet", "0", CVAR_BOOL, 0, NULL, 0, 0);
	cl_showMiss = CVar_Register("cl_showMiss", "0", CVAR_BOOL, 0, NULL, 0, 0);
	cl_showEvents = CVar_Register("cl_showEvents", "0", CVAR_BOOL, CVAR_CHEAT | CVAR_GAME, "Show entity events", 0, 0);
	cl_showMaterial = CVar_Register("cl_showMaterial", "0", CVAR_BOOL, CVAR_CHEAT, NULL, 0, 0);
	cl_timeOut = CVar_Register("cl_timeOut", "120", CVAR_FLOAT, 0, NULL, 0.0f, 500.0f);
	cl_visibleWeapons = CVar_Register("cl_visibleWeapons", "1", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_thirdPerson = CVar_Register("cl_thirdPerson", "0", CVAR_BOOL, CVAR_CHEAT, NULL, 0, 0);
	cl_thirdPersonRange = CVar_Register("cl_thirdPersonRange", "40", CVAR_FLOAT, CVAR_CHEAT, NULL, 0.0f, 100.0f);
	cl_thirdPersonAngle = CVar_Register("cl_thirdPersonAngle", "0", CVAR_FLOAT, CVAR_CHEAT, NULL, 0.0f, 100.0f);
	cl_viewBlend = CVar_Register("cl_viewBlend", "1", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_particles = CVar_Register("cl_particles", "1", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_particleLOD = CVar_Register("cl_particleLOD", "0", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_particleBounce = CVar_Register("cl_particleBounce", "1", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_particleFriction = CVar_Register("cl_particleFriction", "1", CVAR_BOOL, CVAR_ARCHIVE, NULL, 0, 0);
	cl_particleVertexLight = CVar_Register("cl_particleVertexLight", "1", CVAR_BOOL, CVAR_ARCHIVE, "Particle vertices effects by other light sources", 0, 0);
	cl_markTime = CVar_Register("cl_markTime", "15000", CVAR_INTEGER, CVAR_ARCHIVE, NULL, 0, 20000);
	cl_brassTime = CVar_Register("cl_brassTime", "2500", CVAR_INTEGER, CVAR_ARCHIVE, NULL, 0, 5000);
	cl_blood = CVar_Register("cl_blood", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw blood decals", 0, 0);
	cl_testModelPitch = CVar_Register("cl_testModelPitch", "0.0", CVAR_FLOAT, CVAR_CHEAT | CVAR_GAME, "Test model pitch", 0.0f, 360.0f);
	cl_testModelYaw = CVar_Register("cl_testModelYaw", "0.0", CVAR_FLOAT, CVAR_CHEAT | CVAR_GAME, "Test model yaw", 0.0f, 360.0f);
	cl_testModelRoll = CVar_Register("cl_testModelRoll", "0.0", CVAR_FLOAT, CVAR_CHEAT | CVAR_GAME, "Test model roll", 0.0f, 360.0f);
	cl_testModelAnimate = CVar_Register("cl_testModelAnimate", "0", CVAR_BOOL, CVAR_CHEAT, "Test model animation", 0, 0);
	cl_testModelRotatePitch = CVar_Register("cl_testModelRotatePitch", "0.0", CVAR_FLOAT, CVAR_CHEAT, "Test model rotation pitch", 0.0f, 0.0f);
	cl_testModelRotateYaw = CVar_Register("cl_testModelRotateYaw", "0.0", CVAR_FLOAT, CVAR_CHEAT, "Test model rotation yaw", 0.0f, 0.0f);
	cl_testModelRotateRoll = CVar_Register("cl_testModelRotateRoll", "0.0", CVAR_FLOAT, CVAR_CHEAT, "Test model rotation roll", 0.0f, 0.0f);	
	cl_testGunX = CVar_Register("cl_testGunX", "0", CVAR_FLOAT, CVAR_CHEAT, "Test gun X position", 0.0f, 100.0f);
	cl_testGunY = CVar_Register("cl_testGunY", "0", CVAR_FLOAT, CVAR_CHEAT, "Test gun Y position", 0.0f, 100.0f);
	cl_testGunZ = CVar_Register("cl_testGunZ", "0", CVAR_FLOAT, CVAR_CHEAT, "Test gun Z position", 0.0f, 100.0f);	
	cl_drawCrosshair = CVar_Register("cl_drawCrosshair", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw the crosshair", 0, 0);
	cl_crosshairX = CVar_Register("cl_crosshairX", "0", CVAR_INTEGER, CVAR_ARCHIVE, "Crosshair X position", 0, 100);
	cl_crosshairY = CVar_Register("cl_crosshairY", "0", CVAR_INTEGER, CVAR_ARCHIVE, "Crosshair Y position", 0, 100);
	cl_crosshairSize = CVar_Register("cl_crosshairSize", "24", CVAR_INTEGER, CVAR_ARCHIVE, "Crosshair size", 0, 63);
	cl_crosshairColor = CVar_Register("cl_crosshairColor", "7", CVAR_INTEGER, CVAR_ARCHIVE, "Crosshair color index", 0, 31);
	cl_crosshairAlpha = CVar_Register("cl_crosshairAlpha", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Crosshair alpha level", 0.0f, 1.0f);
	cl_crosshairHealth = CVar_Register("cl_crosshairHealth", "0", CVAR_BOOL, CVAR_ARCHIVE, "Draw crosshair color based on health", 0, 0);
	cl_crosshairNames = CVar_Register("cl_crosshairNames", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw player name under the crosshair", 0, 0);
	cl_viewSize = CVar_Register("cl_viewSize", "100", CVAR_INTEGER, CVAR_ARCHIVE, NULL, 0, 1000);
	cl_centerTime = CVar_Register("cl_centerTime", "2500", CVAR_INTEGER, CVAR_ARCHIVE, NULL, 0, 5000);
	cl_drawGodModeShell = CVar_Register("cl_drawGodModeShell", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw god mode shell on the view weapon", 0, 0);
	cl_drawCenterString = CVar_Register("cl_drawCenterString", "1",  CVAR_BOOL, CVAR_ARCHIVE, "Draw center strings", 0, 0);
	cl_drawPause = CVar_Register("cl_drawPause", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw the pause", 0, 0);
	cl_drawLogo = CVar_Register("cl_drawLogo", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw the promotion logo", 0, 0);
	cl_drawFPS = CVar_Register("cl_drawFPS", "0", CVAR_BOOL, CVAR_ARCHIVE, "Draw the frames per second counter", 0, 0);
	cl_drawLagometer = CVar_Register("cl_drawLagometer", "0", CVAR_BOOL, CVAR_ARCHIVE, "Draw the lagometer icon", 0, 0);
	cl_drawDisconnected = CVar_Register("cl_drawDisconnected", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw the disconnected icon", 0, 0);
	cl_drawRecording = CVar_Register("cl_drawRecording", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw recording", 0, 0);
	cl_draw2D = CVar_Register("cl_draw2D", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw 2D elements on screen", 0, 0);
	cl_drawIcons = CVar_Register("cl_drawIcons", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw the icons", 0, 0);
	cl_drawStatus = CVar_Register("cl_drawStatus", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw the status", 0, 0);
	cl_drawInventory = CVar_Register("cl_drawInventory", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw the inventory", 0, 0);
	cl_drawLayout = CVar_Register("cl_drawLayout", "1", CVAR_BOOL, CVAR_ARCHIVE, "Draw the layout", 0, 0);
	cl_allowDownload = CVar_Register("cl_allowDownload", "1", CVAR_BOOL, CVAR_ARCHIVE, "Allow file downloads from server", 0, 0);
	cl_rconPassword = CVar_Register("rconPassword", "", CVAR_STRING, 0, NULL, 0, 0);
	cl_rconAddress = CVar_Register("rconAddress", "", CVAR_STRING, 0, NULL, 0, 0);
	cl_itemBob = CVar_Register("cl_itemBob", "1", CVAR_BOOL, CVAR_ARCHIVE, "Make items bob up and down continuously", 0, 0);

	// Add commands
	CL_AddCommands();

	Cmd_AddCommand("precache", CL_Precache_f, "Precaches files for the current map", NULL);
	Cmd_AddCommand("changing", CL_Changing_f, "Change map", NULL);
	Cmd_AddCommand("cmd", CL_ForwardToServer_f, "Forward to server command", NULL);
	Cmd_AddCommand("connect", CL_Connect_f, "Connects to a server", NULL);
	Cmd_AddCommand("reconnect", CL_Reconnect_f, "Reconnects to the last server", NULL);
	Cmd_AddCommand("disconnect", CL_Disconnect_f, "Disconnects from server", NULL);
	Cmd_AddCommand("localservers", CL_LocalServers_f, "Pings local servers", NULL);
	Cmd_AddCommand("pingservers", CL_PingServers_f, "Ping favorite servers", NULL);
	Cmd_AddCommand("ping", CL_Ping_f, "Pings a server", NULL);
	Cmd_AddCommand("rcon", CL_Rcon_f, "Issues a remote console command", NULL);
	Cmd_AddCommand("userinfo", CL_UserInfo_f, "Shows user information", NULL);
	Cmd_AddCommand("restartVideo", CL_RestartVideo_f, "Restarts the video subsystem", NULL);
	Cmd_AddCommand("restartSound", CL_RestartSound_f, "Restarts the sound subsystem", NULL);	
	Cmd_AddCommand("skins", CL_Skins_f, "Shows player skins", NULL);
	Cmd_AddCommand("configstrings", CL_ConfigStrings_f, "Shows config strings", NULL);
	Cmd_AddCommand("record", CL_Record_f, "Records a demo", NULL);
	Cmd_AddCommand("stoprecord", CL_StopRecord_f, "Stops demo recording", NULL);

	Cmd_AddCommand("centerview", CL_CenterView_f, NULL, NULL);
	Cmd_AddCommand("+zoom", CL_ZoomDown_f, NULL, NULL);
	Cmd_AddCommand("-zoom", CL_ZoomUp_f, NULL, NULL);
	Cmd_AddCommand("+moveup", CL_UpDown_f, NULL, NULL);
	Cmd_AddCommand("-moveup", CL_UpUp_f, NULL, NULL);
	Cmd_AddCommand("+movedown", CL_DownDown_f, NULL, NULL);
	Cmd_AddCommand("-movedown", CL_DownUp_f, NULL, NULL);
	Cmd_AddCommand("+left", CL_LeftDown_f, NULL, NULL);
	Cmd_AddCommand("-left", CL_LeftUp_f, NULL, NULL);
	Cmd_AddCommand("+right", CL_RightDown_f, NULL, NULL);
	Cmd_AddCommand("-right", CL_RightUp_f, NULL, NULL);
	Cmd_AddCommand("+forward", CL_ForwardDown_f, NULL, NULL);
	Cmd_AddCommand("-forward", CL_ForwardUp_f, NULL, NULL);
	Cmd_AddCommand("+back", CL_BackDown_f, NULL, NULL);
	Cmd_AddCommand("-back", CL_BackUp_f, NULL, NULL);
	Cmd_AddCommand("+lookup", CL_LookUpDown_f, NULL, NULL);
	Cmd_AddCommand("-lookup", CL_LookUpUp_f, NULL, NULL);
	Cmd_AddCommand("+lookdown", CL_LookDownDown_f, NULL, NULL);
	Cmd_AddCommand("-lookdown", CL_LookDownUp_f, NULL, NULL);
	Cmd_AddCommand("+strafe", CL_StrafeDown_f, NULL, NULL);
	Cmd_AddCommand("-strafe", CL_StrafeUp_f, NULL, NULL);
	Cmd_AddCommand("+moveleft", CL_MoveLeftDown_f, NULL, NULL);
	Cmd_AddCommand("-moveleft", CL_MoveLeftUp_f, NULL, NULL);
	Cmd_AddCommand("+moveright", CL_MoveRightDown_f, NULL, NULL);
	Cmd_AddCommand("-moveright", CL_MoveRightUp_f, NULL, NULL);
	Cmd_AddCommand("+speed", CL_SpeedDown_f, NULL, NULL);
	Cmd_AddCommand("-speed", CL_SpeedUp_f, NULL, NULL);
	Cmd_AddCommand("+attack", CL_AttackDown_f, NULL, NULL);
	Cmd_AddCommand("-attack", CL_AttackUp_f, NULL, NULL);
	Cmd_AddCommand("+use", CL_UseDown_f, NULL, NULL);
	Cmd_AddCommand("-use", CL_UseUp_f, NULL, NULL);
	Cmd_AddCommand("+klook", CL_KLookDown_f, NULL, NULL);
	Cmd_AddCommand("-klook", CL_KLookUp_f, NULL, NULL);
	Cmd_AddCommand("impulse", CL_Impulse_f, NULL, NULL);

	// Forward to server commands.
	// The only thing this does is allow command completion to work. All
	// unknown commands are automatically forwarded to the server.
	Cmd_AddCommand("wave", NULL, NULL, NULL);
	Cmd_AddCommand("kill", NULL, NULL, NULL);
	Cmd_AddCommand("use", NULL, NULL, NULL);
	Cmd_AddCommand("drop", NULL, NULL, NULL);
	Cmd_AddCommand("say", NULL, NULL, NULL);
	Cmd_AddCommand("say_team", NULL, NULL, NULL);
	Cmd_AddCommand("info", NULL, NULL, NULL);
	Cmd_AddCommand("give", NULL, NULL, NULL);
	Cmd_AddCommand("god", NULL, NULL, NULL);
	Cmd_AddCommand("notarget", NULL, NULL, NULL);
	Cmd_AddCommand("noclip", NULL, NULL, NULL);
	Cmd_AddCommand("inven", NULL, NULL, NULL);
	Cmd_AddCommand("invuse", NULL, NULL, NULL);
	Cmd_AddCommand("invprev", NULL, NULL, NULL);
	Cmd_AddCommand("invnext", NULL, NULL, NULL);
	Cmd_AddCommand("invdrop", NULL, NULL, NULL);
	Cmd_AddCommand("invprevw", NULL, NULL, NULL);
	Cmd_AddCommand("invnextw", NULL, NULL, NULL);
	Cmd_AddCommand("invprevp", NULL, NULL, NULL);
	Cmd_AddCommand("invnextp", NULL, NULL, NULL);
	Cmd_AddCommand("weapnext", NULL, NULL, NULL);
	Cmd_AddCommand("weapprev", NULL, NULL, NULL);
	Cmd_AddCommand("weaplast", NULL, NULL, NULL);
	Cmd_AddCommand("score", NULL, NULL, NULL);
	Cmd_AddCommand("help", NULL, NULL, NULL);
	Cmd_AddCommand("putaway", NULL, NULL, NULL);
	Cmd_AddCommand("players", NULL, NULL, NULL);
	Cmd_AddCommand("playerlist", NULL, NULL, NULL);
}

/*
 ==================
 CL_Unregister
 ==================
*/
static void CL_Unregister (){

	Mem_Fill(&cls, 0, sizeof(cls));

	// Remove commands
	Cmd_RemoveCommand("precache");
	Cmd_RemoveCommand("changing");
	Cmd_RemoveCommand("cmd");
	Cmd_RemoveCommand("connect");
	Cmd_RemoveCommand("reconnect");
	Cmd_RemoveCommand("disconnect");
	Cmd_RemoveCommand("localservers");
	Cmd_RemoveCommand("pingservers");
	Cmd_RemoveCommand("ping");
	Cmd_RemoveCommand("rcon");
	Cmd_RemoveCommand("userinfo");
	Cmd_RemoveCommand("restartVideo");
	Cmd_RemoveCommand("restartSound");
	Cmd_RemoveCommand("skins");
	Cmd_RemoveCommand("configstrings");
	Cmd_RemoveCommand("record");
	Cmd_RemoveCommand("stoprecord");
	Cmd_RemoveCommand("centerview");
	Cmd_RemoveCommand("+zoom");
	Cmd_RemoveCommand("-zoom");
	Cmd_RemoveCommand("+moveup");
	Cmd_RemoveCommand("-moveup");
	Cmd_RemoveCommand("+movedown");
	Cmd_RemoveCommand("-movedown");
	Cmd_RemoveCommand("+left");
	Cmd_RemoveCommand("-left");
	Cmd_RemoveCommand("+right");
	Cmd_RemoveCommand("-right");
	Cmd_RemoveCommand("+forward");
	Cmd_RemoveCommand("-forward");
	Cmd_RemoveCommand("+back");
	Cmd_RemoveCommand("-back");
	Cmd_RemoveCommand("+lookup");
	Cmd_RemoveCommand("-lookup");
	Cmd_RemoveCommand("+lookdown");
	Cmd_RemoveCommand("-lookdown");
	Cmd_RemoveCommand("+strafe");
	Cmd_RemoveCommand("-strafe");
	Cmd_RemoveCommand("+moveleft");
	Cmd_RemoveCommand("-moveleft");
	Cmd_RemoveCommand("+moveright");
	Cmd_RemoveCommand("-moveright");
	Cmd_RemoveCommand("+speed");
	Cmd_RemoveCommand("-speed");
	Cmd_RemoveCommand("+attack");
	Cmd_RemoveCommand("-attack");
	Cmd_RemoveCommand("+use");
	Cmd_RemoveCommand("-use");
	Cmd_RemoveCommand("+klook");
	Cmd_RemoveCommand("-klook");
	Cmd_RemoveCommand("impulse");

	// Forward to server commands
	Cmd_RemoveCommand("wave");
	Cmd_RemoveCommand("kill");
	Cmd_RemoveCommand("use");
	Cmd_RemoveCommand("drop");
	Cmd_RemoveCommand("say");
	Cmd_RemoveCommand("say_team");
	Cmd_RemoveCommand("info");
	Cmd_RemoveCommand("give");
	Cmd_RemoveCommand("god");
	Cmd_RemoveCommand("notarget");
	Cmd_RemoveCommand("noclip");
	Cmd_RemoveCommand("inven");
	Cmd_RemoveCommand("invuse");
	Cmd_RemoveCommand("invprev");
	Cmd_RemoveCommand("invnext");
	Cmd_RemoveCommand("invdrop");
	Cmd_RemoveCommand("invprevw");
	Cmd_RemoveCommand("invnextw");
	Cmd_RemoveCommand("invprevp");
	Cmd_RemoveCommand("invnextp");
	Cmd_RemoveCommand("weapnext");
	Cmd_RemoveCommand("weapprev");
	Cmd_RemoveCommand("weaplast");
	Cmd_RemoveCommand("score");
	Cmd_RemoveCommand("help");
	Cmd_RemoveCommand("putaway");
	Cmd_RemoveCommand("players");
	Cmd_RemoveCommand("playerlist");
}


/*
 ==============================================================================

 FRAMEWORK

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
void CL_Frame (int msec){

	static int	extraTime;
	int			timeClient;

	if (cls.state == CA_UNINITIALIZED)
		return;

	if (com_speeds->integerValue)
		timeClient = Sys_Milliseconds();

	extraTime += msec;

	if (!com_timeDemo->integerValue){
		if (cl_maxFPS->integerValue > 0){
			if (extraTime < 1000 / cl_maxFPS->integerValue)
				return;		// Framerate is too high
		}
	}

	// Decide the simulation time
	cl.time += msec;
	cls.realTime = Sys_Milliseconds();

	cls.frameTime = MS2SEC(msec);
	if (cls.frameTime > 0.2f)
		cls.frameTime = 0.2f;

	// If in the debugger last frame, don't time-out
	if (msec > 5000)
		cls.netChan.lastReceived = Sys_Milliseconds();

	// Fix any cheating console variables
	CL_FixCheatVariables();

	// Read and parse incoming packets
	CL_ReadPackets();

	// Pump message loop
	Sys_ProcessEvents();

	// Let the mouse activate or deactivate
	IN_Frame();

	// Process console commands
	Cmd_ExecuteBuffer();

	// Build a user command and send it to the server
	CL_BuildUserCmd();

	// Resend a connection request if necessary
	CL_CheckForResend();

	// Predict all unacknowledged movements
	CL_PredictMovement();

	// Update the screen
	CL_UpdateScreen();

	// Update audio
	S_Update(cl.time);

	// Advance local effects for next frame
	CL_RunLightStyles();

	if (com_speeds->integerValue)
		com_timeClient += (Sys_Milliseconds() - timeClient);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 CL_Init
 ==================
*/
void CL_Init (){

	if (com_dedicated->integerValue)
		return;		// Nothing running on the client

	if (cls.state != CA_UNINITIALIZED)
		return;		// Already running

	Com_Printf("-------- Client Initialization --------\n");

	// Register commands and variables
	CL_Register();

	// Initialize console
	Con_Init();	

	// Initialize cinematic module
	CIN_Init();

	// Initialize renderer and sound system
	R_Init(true);
	S_Init(true);

	// Initialize input system
	IN_Init();

	// Initialize UI system
	UI_Init();

	// Load local assets
	CL_LoadAssets();

	// Fully initialized
	cls.fullyInitialized = true;

	// Make the main menu active
	UI_SetActiveMenu(UI_MAINMENU);

	// Set the com_clientRunning variable
	CVar_SetBool(com_clientRunning, true);

	Com_Printf("---------------------------------------\n");
}

/*
 ==================
 CL_Shutdown
 ==================
*/
void CL_Shutdown (){

	if (cls.state == CA_UNINITIALIZED)
		return;

	Com_Printf("-------- Client Shutdown --------\n");

	// No longer fully initialized
	cls.fullyInitialized = false;

	// Unregister commands
	CL_Unregister();

	// If connected to a server, disconnect
	CL_Disconnect(true);

	// Shutdown UI system
	UI_Shutdown();

	// Shutdown input system
	IN_Shutdown();

	// Shutdown sound system and renderer
	S_Shutdown(true);
	R_Shutdown(true);

	// Shutdown cinematic module
	CIN_Shutdown();

	// Shutdown console
	Con_Shutdown();

	// Wipe the entire clientStatic_t structure
	Mem_Fill(&cls, 0, sizeof(clientStatic_t));

	// Reset the com_clientRunning variable
	CVar_SetBool(com_clientRunning, false);

	Com_Printf("---------------------------------\n");
}