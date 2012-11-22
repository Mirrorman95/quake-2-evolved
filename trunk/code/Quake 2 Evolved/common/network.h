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
// network.h - Network layer
//


#ifndef __NETWORK_H__
#define __NETWORK_H__


/*
 ==============================================================================

 Network Layer:

 Handles all the incoming/outgoing traffic through UDP sockets and provides a
 portable interface to non-portable networking services.

 ==============================================================================
*/

#define MAX_ADDRESS_LENGTH			64			// Max length of a network address

#define	PORT_MASTER					27900
#define	PORT_CLIENT					27901
#define	PORT_SERVER					27910
#define	PORT_ANY					-1

#define	MAX_MSGLEN					1400		// Max length of a message

typedef enum {
	NS_CLIENT, 
	NS_SERVER
} netSrc_t;

typedef enum {
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP
} netAdrType_t;

typedef struct {
	netAdrType_t		type;

	byte				ip[4];
	ushort				port;
} netAdr_t;

// Checks for and reads an incoming loopback packet.
// Returns true if a packet was available.
bool			NET_GetLoopPacket (netSrc_t sock, netAdr_t *from, msg_t *msg);

// Sends a loopback packet
bool			NET_SendLoopPacket (netSrc_t sock, const netAdr_t to, const void *data, int length);

// Checks for and reads an incoming network packet.
// Returns true if a packet was received and valid.
bool			NET_GetPacket (netSrc_t sock, netAdr_t *from, msg_t *message);

// Sends a packet to the given network address
void			NET_SendPacket (netSrc_t sock, const netAdr_t to, const void *data, int length);

// Sends a data message in an out-of-band datagram
void			NET_OutOfBandData (netSrc_t sock, const netAdr_t adr, const void *data, int length);

// Sends a text message in an out-of-band datagram
void			NET_OutOfBandPrintf (netSrc_t sock, const netAdr_t adr, const char *fmt, ...);

// Converts the given network address to a string
const char *	NET_AddressToString (const netAdr_t adr);

// Converts the given string to a network address
bool			NET_StringToAddress (const char *string, netAdr_t *adr);

// Returns true if the given network address is a local address
bool			NET_IsLocalAddress (const netAdr_t adr);

// Returns true if the given network addresses are equal
bool			NET_CompareAddress (const netAdr_t adr1, const netAdr_t adr2);

// Returns true if the given network addresses (minus port) are equal
bool			NET_CompareBaseAddress (const netAdr_t adr1, const netAdr_t adr2);

// Initializes the network subsystem
void			NET_Init ();

// Shuts down the network subsystem
void			NET_Shutdown ();


#endif	// __NETWORK_H__