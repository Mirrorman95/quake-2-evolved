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
// network.c - Network layer
//


#include "common.h"


/*
 ==============================================================================

 LOOPBACK BUFFERS FOR LOCAL PLAYER

 ==============================================================================
*/

#define	MAX_LOOPBACK				4

typedef struct {
	byte					data[MAX_MSGLEN];
	int						dataLength;
} loopMsg_t;

typedef struct {
	loopMsg_t				msgs[MAX_LOOPBACK];
	int						get;
	int						send;
} loopback_t;

static loopback_t			net_loopbacks[2];


/*
 ==================
 NET_GetLoopPacket
 ==================
*/
bool NET_GetLoopPacket (netSrc_t sock, netAdr_t *from, msg_t *msg){

	loopback_t	*loop;
	int			i;

	loop = &net_loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	i = loop->get & (MAX_LOOPBACK - 1);
	loop->get++;

	Mem_Copy(msg->data, loop->msgs[i].data, loop->msgs[i].dataLength);
	msg->curSize = loop->msgs[i].dataLength;

	Mem_Fill(from, 0, sizeof(netAdr_t));
	from->type = NA_LOOPBACK;

	return true;
}

/*
 ==================
 NET_SendLoopPacket
 ==================
*/
bool NET_SendLoopPacket (netSrc_t sock, const netAdr_t to, const void *data, int length){

	loopback_t	*loop;
	int			i;

	if (to.type != NA_LOOPBACK)
		return false;

	loop = &net_loopbacks[sock ^ 1];

	i = loop->send & (MAX_LOOPBACK - 1);
	loop->send++;

	Mem_Copy(loop->msgs[i].data, data, length);
	loop->msgs[i].dataLength = length;

	return true;
}


/*
 ==============================================================================

 OUT-OF-BAND DATAGRAMS

 ==============================================================================
*/


/*
 ==================
 NET_OutOfBandData
 ==================
*/
void NET_OutOfBandData (netSrc_t sock, const netAdr_t adr, const void *data, int length){

	byte	buffer[MAX_MSGLEN];
	msg_t	msg;

	// Write the packet header
	MSG_Init(&msg, buffer, sizeof(buffer), false);

	MSG_WriteLong(&msg, -1);		// -1 sequence means out of band
	MSG_Write(&msg, data, length);

	// Send the datagram
	NET_SendPacket(sock, adr, msg.data, msg.curSize);
}

/*
 ==================
 NET_OutOfBandPrintf
 ==================
*/
void NET_OutOfBandPrintf (netSrc_t sock, const netAdr_t adr, const char *fmt, ...){

	char	string[MAX_MSGLEN - 4];
	va_list	argPtr;

	// Write the string
	va_start(argPtr, fmt);
	Str_VSPrintf(string, sizeof(string), fmt, argPtr);
	va_end(argPtr);

	// Send the datagram
	NET_OutOfBandData(sock, adr, string, Str_Length(string));
}


/*
 ==============================================================================

 ADDRESS FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 NET_IsLocalAddress
 ==================
*/
bool NET_IsLocalAddress (const netAdr_t adr){

	if (adr.type == NA_LOOPBACK)
		return true;

	return false;
}

/*
 ==================
 NET_CompareAddress
 ==================
*/
bool NET_CompareAddress (const netAdr_t adr1, const netAdr_t adr2){

	if (adr1.type != adr2.type)
		return false;

	if (adr1.type == NA_LOOPBACK)
		return true;

	if (adr1.type == NA_IP){
		if (adr1.ip[0] == adr2.ip[0] && adr1.ip[1] == adr2.ip[1] && adr1.ip[2] == adr2.ip[2] && adr1.ip[3] == adr2.ip[3] && adr1.port == adr2.port)
			return true;

		return false;
	}

	return false;
}

/*
 ==================
 NET_CompareBaseAddress
 ==================
*/
bool NET_CompareBaseAddress (const netAdr_t adr1, const netAdr_t adr2){

	if (adr1.type != adr2.type)
		return false;

	if (adr1.type == NA_LOOPBACK)
		return true;

	if (adr1.type == NA_IP){
		if (adr1.ip[0] == adr2.ip[0] && adr1.ip[1] == adr2.ip[1] && adr1.ip[2] == adr2.ip[2] && adr1.ip[3] == adr2.ip[3])
			return true;

		return false;
	}

	return false;
}