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
// server.h - ...
//


#ifndef __SERVER_H__
#define __SERVER_H__


#include "../common/common.h"
#include "../game/game.h"


#define	LATENCY_COUNTS				16
#define	RATE_MESSAGES				10

// MAX_CHALLENGES is made large to prevent a denial of service attack 
// that could cycle all of them out before legitimate users connected
#define	MAX_CHALLENGES				1024

typedef enum {
	SS_DEAD,		// No map loaded
	SS_LOADING,		// Spawning level edicts
	SS_GAME,		// Running game
	SS_DEMO,		// Running demo
	SS_CINEMATIC,	// Running cinematic
	SS_PIC			// Running static cinematic
} serverState_t;

typedef struct {
	serverState_t			state;				// Precache commands are only valid during load

	uint					time;				// Always sv.frameNum * 100 msec
	int						frameNum;

	char					name[MAX_PATH_LENGTH];	// Map name, demo, or cinematic name
	bool					attractLoop;		// Running cinematics and demos for the local system only
	bool					loadGame;			// Client begins should reuse existing entity

	struct cmodel_s *		models[MAX_MODELS];

	char					configStrings[MAX_CONFIGSTRINGS][MAX_PATH_LENGTH];
	entity_state_t			baselines[MAX_EDICTS];

	// The multicast buffer is used to send a message to a set of 
	// clients. It is only used to marshall data until SV_Multicast is
	// called.
	msg_t					multicast;
	byte					multicastBuffer[MAX_MSGLEN];

	// Demo server information
	fileHandle_t			demoFile;
} server_t;

typedef enum {
	CS_FREE,		// Can be reused for a new connection
	CS_ZOMBIE,		// Client has been disconnected, but don't reuse connection for a couple seconds
	CS_CONNECTED,	// Has been assigned to a client_t, but not in game yet
	CS_SPAWNED		// Client is fully in game
} clientState_t;

typedef struct {
	int						areaBytes;
	byte					areaBits[MAX_MAP_AREAS/8];	// portalarea visibility bits
	player_state_t			ps;
	int						numEntities;
	int						firstEntity;		// Into the circular sv_packet_entities[]
	int						sentTime;			// For ping calculations
} clientFrame_t;

// A client can leave the server in one of four ways:
//	- Dropping properly by quiting or disconnecting
//	- Timing out if no valid messages are received for time-out seconds
//	- Getting kicked off by the server operator
//	- A program error, like an overflowed reliable buffer

typedef struct client_s {
	clientState_t			state;

	char					userInfo[MAX_INFO_STRING];

	int						lastFrame;			// For delta compression
	usercmd_t				lastUserCmd;		// For filling in big drops

	int						commandMsec;		// Every seconds this is reset, if user
												// commands exhaust it, assume time cheating

	int						frameLatency[LATENCY_COUNTS];
	int						ping;

	int						messageSize[RATE_MESSAGES];	// Used to rate drop packets
	int						rate;
	int						suppressCount;		// Number of messages rate suppressed

	edict_t *				edict;				// EDICT_NUM(client number + 1)
	char					name[32];			// Extracted from user info, high bits masked
	int						messageLevel;		// For filtering printed messages

	// The datagram is written to by sound calls, prints, temp ents, etc...
	// It can be harmlessly overflowed
	msg_t					datagram;
	byte					datagramBuffer[MAX_MSGLEN];

	clientFrame_t			frames[UPDATE_BACKUP];	// Updates can be delta'ed from here

	int						lastMessage;		// sv.frameNum when packet was last received
	int						lastConnect;

	int						challenge;			// Challenge of this user, randomly generated

	netChan_t				netChan;

	// File transfer to client
	fileHandle_t			downloadFile;
	int						downloadSize;
	int						downloadOffset;
} client_t;

typedef struct {
	netAdr_t				address;

	int						challenge;
	int						time;
} challenge_t;

typedef struct {
	bool					initialized;				// sv_init has completed
	int						realTime;					// Always increasing, no clamping, etc...

	char					mapCmd[128];				// ie: *intro.cin+base1 

	int						spawnCount;					// Incremented each server start. Used to check late spawns

	client_t *				clients;					// [maxclients]
	int						nextClientEntities;			// Next client entity to use
	int						numClientEntities;			// maxclients*UPDATE_BACKUP*MAX_PACKET_ENTITIES
	entity_state_t *		clientEntities;				// [numClientEntities]

	int						lastHeartbeat;

	challenge_t				challenges[MAX_CHALLENGES];	// To prevent invalid IPs from connecting

	// Game module handle
	void *					gameLibHandle;

	// Server record values
	fileHandle_t			demoFile;
	msg_t					demoMulticast;
	byte					demoMulticastBuffer[MAX_MSGLEN];
} serverStatic_t;

// =====================================================================

#define EDICT_NUM(n)				((edict_t *)((byte *)ge->edicts + ge->edict_size*(n)))
#define NUM_FOR_EDICT(e)			(((byte *)(e)-(byte *)ge->edicts) / ge->edict_size)

#define	OUTPUTBUF_LENGTH			(MAX_MSGLEN - 16)

typedef enum {
	RD_NONE,
	RD_CLIENT,
	RD_PACKET
} redirect_t;

extern server_t				sv;					// Local server
extern serverStatic_t		svs;				// Persistant server info

extern client_t *			sv_client;
extern edict_t *			sv_player;

extern char					sv_outputBuf[OUTPUTBUF_LENGTH];

extern game_export_t *		ge;

extern cvar_t *				sv_maxClients;
extern cvar_t *				sv_hostName;
extern cvar_t *				sv_nextServer;
extern cvar_t *				sv_master1;
extern cvar_t *				sv_master2;
extern cvar_t *				sv_master3;
extern cvar_t *				sv_master4;
extern cvar_t *				sv_master5;
extern cvar_t *				sv_clientTimeOut;
extern cvar_t *				sv_zombieTimeOut;
extern cvar_t *				sv_reconnectLimit;
extern cvar_t *				sv_noReload;
extern cvar_t *				sv_airAccelerate;
extern cvar_t *				sv_enforceTime;
extern cvar_t *				sv_allowDownload;
extern cvar_t *				sv_publicServer;
extern cvar_t *				sv_rconPassword;
extern cvar_t *				sv_loadGame;

int				SV_ModelIndex (const char *name);
int				SV_SoundIndex (const char *name);
int				SV_ImageIndex (const char *name);

void			SV_DropClient (client_t *cl);
void			SV_UserInfoChanged (client_t *cl);

void			SV_LoadGame_f (void);
void			SV_SaveGame_f (void);
void			SV_GameMap_f (void);
void			SV_Map_f (void);
void			SV_Demo_f (void);
void			SV_Kick_f (void);
void			SV_Status_f (void);
void			SV_Heartbeat_f (void);
void			SV_ServerInfo_f (void);
void			SV_DumpUser_f (void);
void			SV_ServerRecord_f (void);
void			SV_ServerStopRecord_f (void);
void			SV_KillServer_f (void);
void			SV_ServerCommand_f (void);
void			SV_ConSay_f (void);

void			SV_InitGame (void);
void			SV_Map (const char *levelString, bool attractLoop, bool loadGame);

void			SV_FlushRedirect (redirect_t redirect, char *outputBuf);

void			SV_SendClientMessages (void);
void			SV_ParseClientMessage (client_t *cl);

void			SV_Multicast (vec3_t origin, multicast_t to);
void			SV_StartSound (vec3_t origin, edict_t *entity, int channel, int sound, float volume, float attenuation, float timeOfs);
void			SV_ClientPrintf (client_t *cl, int level, const char *fmt, ...);
void			SV_BroadcastPrintf (int level, const char *fmt, ...);
void			SV_BroadcastCommand (const char *fmt, ...);

void			SV_NextServer (void);

void			SV_ReadLevelFile (void);

void			SV_WriteFrameToClient (client_t *cl, msg_t *msg);
void			SV_RecordDemoMessage (void);
void			SV_BuildClientFrame (client_t *cl);

void			SG_Init ();
void			SG_Shutdown ();

// Called after the world model has been loaded, before linking any entities
void			SV_ClearWorld (void);

// Call before removing an entity, and before trying to move one, so it
// doesn't clip against itself
void			SV_UnlinkEdict (edict_t *ent);

// Needs to be called any time an entity changes origin, mins, maxs, or
// solid. Automatically unlinks if needed.
// Sets ent->v.absmin and ent->v.absmax.
// Sets ent->leafnums[] for PVS determination even if the entity is not
// not solid.
void			SV_LinkEdict (edict_t *ent);

// Fills in a table of edict pointers with edicts that have bounding 
// boxes that intersect the given area. It is possible for a non-axial 
// brush model to be returned that doesn't actually intersect the area
// on an exact test.
// Returns the number of pointers filled in.
// Does this always return the world?
int				SV_AreaEdicts (vec3_t mins, vec3_t maxs, edict_t **list, int maxCount, int areaType);

// mins and maxs are relative.
// If the entire move stays in a solid volume, trace.allsolid will be 
// set, trace.startsolid will be set, and trace.fraction will be 0.
//
// If the starting point is in a solid, it will be allowed to move out
// to an open area.
//
// passEdict is explicitly excluded from clipping checks (normally NULL)
trace_t			SV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passEdict, int contentMask);

// Returns the CONTENTS_* value from the world at the given point.
// Quake 2 extends this to also check entities, to allow moving liquids.
int				SV_PointContents (vec3_t p);


#endif	// __SERVER_H__