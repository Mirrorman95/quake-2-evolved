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
// protocol.h - Protocol for communication between the client and server
//


#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__


/*
 ==============================================================================

 Protocol:

 This is for basic communication between client and server in the engine.

 The game modules define their own protocol for communicating the initial game
 state, periodic state updates (snapshots), and other game related information.

 ==============================================================================
*/

#define	PROTOCOL_VERSION			34

#define	UPDATE_BACKUP				16
#define	UPDATE_MASK					(UPDATE_BACKUP-1)

// Server to client
enum {
	SVC_BAD,

	// These ops are known to the game library
	SVC_MUZZLEFLASH,
	SVC_MUZZLEFLASH2,
	SVC_TEMP_ENTITY,
	SVC_LAYOUT,
	SVC_INVENTORY,

	// The rest are private to the client and server
	SVC_NOP,
	SVC_DISCONNECT,
	SVC_RECONNECT,
	SVC_SOUND,					// <see code>
	SVC_PRINT,					// [byte] id [string] null terminated string
	SVC_STUFFTEXT,				// [string] stuffed into client's console buffer, should be \n terminated
	SVC_SERVERDATA,				// [long] protocol ...
	SVC_CONFIGSTRING,			// [short] [string]
	SVC_SPAWNBASELINE,		
	SVC_CENTERPRINT,			// [string] to put in center of the screen
	SVC_DOWNLOAD,				// [short] size [size bytes]
	SVC_PLAYERINFO,				// variable
	SVC_PACKETENTITIES,			// [...]
	SVC_DELTAPACKETENTITIES,	// [...]
	SVC_FRAME
} svcOps_t;

// Client to server
enum {
	CLC_BAD,
	CLC_NOP, 		
	CLC_MOVE,					// [usercmd_t]
	CLC_USERINFO,				// [user info string]
	CLC_STRINGCMD				// [string] message
} clcOps_t;

// player_state_t communication
#define	PS_M_TYPE					(1<<0)
#define	PS_M_ORIGIN					(1<<1)
#define	PS_M_VELOCITY				(1<<2)
#define	PS_M_TIME					(1<<3)
#define	PS_M_FLAGS					(1<<4)
#define	PS_M_GRAVITY				(1<<5)
#define	PS_M_DELTA_ANGLES			(1<<6)
#define	PS_VIEWOFFSET				(1<<7)
#define	PS_VIEWANGLES				(1<<8)
#define	PS_KICKANGLES				(1<<9)
#define	PS_BLEND					(1<<10)
#define	PS_FOV						(1<<11)
#define	PS_WEAPONINDEX				(1<<12)
#define	PS_WEAPONFRAME				(1<<13)
#define	PS_RDFLAGS					(1<<14)

// usercmd_t communication
#define	CM_ANGLE1 					(1<<0)
#define	CM_ANGLE2 					(1<<1)
#define	CM_ANGLE3 					(1<<2)
#define	CM_FORWARD					(1<<3)
#define	CM_SIDE						(1<<4)
#define	CM_UP						(1<<5)
#define	CM_BUTTONS					(1<<6)
#define	CM_IMPULSE					(1<<7)

// A sound without an ent or pos will be a local only sound
#define	SND_VOLUME					(1<<0)		// A byte
#define	SND_ATTENUATION				(1<<1)		// A byte
#define	SND_POS						(1<<2)		// Three coordinates
#define	SND_ENT						(1<<3)		// A short 0-2: channel, 3-12: entity
#define	SND_OFFSET					(1<<4)		// A byte, msec offset from frame start

#define DEFAULT_SOUND_PACKET_VOLUME			1.0f
#define DEFAULT_SOUND_PACKET_ATTENUATION	1.0f

// entity_state_t communication

// First byte
#define	U_ORIGIN1					(1<<0)
#define	U_ORIGIN2					(1<<1)
#define	U_ANGLE2					(1<<2)
#define	U_ANGLE3					(1<<3)
#define	U_FRAME8					(1<<4)		// Frame is a byte
#define	U_EVENT						(1<<5)
#define	U_REMOVE					(1<<6)		// REMOVE this entity, don't add it
#define	U_MOREBITS1					(1<<7)		// Read one additional byte

// Second byte
#define	U_NUMBER16					(1<<8)		// NUMBER8 is implicit if not set
#define	U_ORIGIN3					(1<<9)
#define	U_ANGLE1					(1<<10)
#define	U_MODEL						(1<<11)
#define U_RENDERFX8					(1<<12)		// Fullbright, etc...
#define	U_EFFECTS8					(1<<14)		// Autorotate, trails, etc...
#define	U_MOREBITS2					(1<<15)		// Read one additional byte

// Third byte
#define	U_SKIN8						(1<<16)
#define	U_FRAME16					(1<<17)		// Frame is a short
#define	U_RENDERFX16 				(1<<18)		// 8 + 16 = 32
#define	U_EFFECTS16					(1<<19)		// 8 + 16 = 32
#define	U_MODEL2					(1<<20)		// Weapons, flags, etc...
#define	U_MODEL3					(1<<21)
#define	U_MODEL4					(1<<22)
#define	U_MOREBITS3					(1<<23)		// Read one additional byte

// Fourth byte
#define	U_OLDORIGIN					(1<<24)		// FIXME: get rid of this
#define	U_SKIN16					(1<<25)
#define	U_SOUND						(1<<26)
#define	U_SOLID						(1<<27)


#endif	// __PROTOCOL_H__