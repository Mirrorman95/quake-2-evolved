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
// netChan.c - Network channel
//


#include "common.h"


netAdr_t				net_from;
msg_t					net_message;
byte					net_messageBuffer[MAX_MSGLEN];

static const char *		net_sourceString[2] = {"client", "server"};

static cvar_t *			net_qport;
static cvar_t *			net_showPackets;
static cvar_t *			net_showDrop;


/*
 ==============================================================================

 INITIALIZATION AND SETUP

 ==============================================================================
*/


/*
 ==================
 NetChan_Init
 ==================
*/
void NetChan_Init (){

	// Register variables
	net_qport = CVar_Register("net_qport", Str_FromInteger(Sys_Milliseconds() & 0xFFFF), CVAR_INTEGER, CVAR_READONLY, "Network channel port", 0, 65535);
	net_showPackets = CVar_Register("net_showPackets", "0", CVAR_BOOL, 0, "Show incoming/outgoing packets", 0, 0);
	net_showDrop = CVar_Register("net_showDrop", "0", CVAR_BOOL, 0, "Show packet dropping", 0, 0);

	// Initialize the message buffer
	MSG_Init(&net_message, net_messageBuffer, sizeof(net_messageBuffer), false);
}

/*
 ==================
 NetChan_Setup
 ==================
*/
void NetChan_Setup (netChan_t *netChan, netSrc_t sock, const netAdr_t adr, int port){

	Mem_Fill(netChan, 0, sizeof(netChan_t));

	netChan->sock = sock;
	netChan->remoteAddress = adr;
	netChan->channelPort = port;
	netChan->lastReceived = Sys_Milliseconds();
	netChan->incomingSequence = 0;
	netChan->outgoingSequence = 1;

	MSG_Init(&netChan->message, netChan->messageBuffer, sizeof(netChan->messageBuffer), true);
}

/*
 ==================
 NetChan_Transmit
 ==================
*/
void NetChan_Transmit (netChan_t *netChan, const void *data, int length){

	byte	buffer[MAX_MSGLEN];
	msg_t	msg;
	bool	sendReliable = false;
	uint	w1, w2;

	// Check for message overflow
	if (netChan->message.overflowed){
		Com_Printf(S_COLOR_YELLOW "%s: outgoing message overflow\n", NET_AddressToString(netChan->remoteAddress));
		return;
	}

	// If the remote side dropped the last reliable message, resend it
	if (netChan->incomingAcknowledged > netChan->lastReliableSequence && netChan->incomingReliableAcknowledged != netChan->reliableSequence)
		sendReliable = true;

	// If the reliable transmit buffer is empty, copy the current 
	// message out
	if (!netChan->reliableLength && netChan->message.curSize)
		sendReliable = true;

	if (!netChan->reliableLength && netChan->message.curSize){
		Mem_Copy(netChan->reliableBuffer, netChan->messageBuffer, netChan->message.curSize);
		netChan->reliableLength = netChan->message.curSize;
		netChan->message.curSize = 0;
		netChan->reliableSequence ^= 1;
	}

	// Write the packet header
	MSG_Init(&msg, buffer, sizeof(buffer), false);

	w1 = (netChan->outgoingSequence & ~(1 << 31)) | (sendReliable<<31);
	w2 = (netChan->incomingSequence & ~(1 << 31)) | (netChan->incomingReliableSequence<<31);

	netChan->outgoingSequence++;
	netChan->lastSent = Sys_Milliseconds();

	MSG_WriteLong(&msg, w1);
	MSG_WriteLong(&msg, w2);

	// Send the qport if we are a client
	if (netChan->sock == NS_CLIENT)
		MSG_WriteShort(&msg, net_qport->integerValue);

	// Copy the reliable message to the packet first
	if (sendReliable){
		MSG_Write(&msg, netChan->reliableBuffer, netChan->reliableLength);
		netChan->lastReliableSequence = netChan->outgoingSequence;
	}

	// Add the unreliable part if space is available
	if (msg.maxSize - msg.curSize >= length)
		MSG_Write(&msg, data, length);
	else
		Com_Printf(S_COLOR_YELLOW "%s: dumped unreliable\n", NET_AddressToString(netChan->remoteAddress));

	// Send the datagram
	NET_SendPacket(netChan->sock, netChan->remoteAddress, msg.data, msg.curSize);

	if (net_showPackets->integerValue){
		if (sendReliable)
			Com_Printf("%s send %4i: seq=%i reliable=%i ack=%i rack=%i\n", net_sourceString[netChan->sock], msg.curSize, netChan->outgoingSequence - 1, netChan->reliableSequence, netChan->incomingSequence, netChan->incomingReliableSequence);
		else
			Com_Printf("%s send %4i: seq=%i ack=%i rack=%i\n", net_sourceString[netChan->sock], msg.curSize, netChan->outgoingSequence - 1, netChan->incomingSequence, netChan->incomingReliableSequence);
	}
}

/*
 ==================
 NetChan_Process
 ==================
*/
bool NetChan_Process (netChan_t *netChan, msg_t *msg){

	uint	sequence, sequenceAck;
	uint	reliableAck, reliableMessage;
	int		port;

	MSG_BeginReading(msg);

	// Get sequence numbers
	sequence = MSG_ReadLong(msg);
	sequenceAck = MSG_ReadLong(msg);

	// Read the qport if we are a server
	if (netChan->sock == NS_SERVER)
		port = MSG_ReadShort(msg);

	reliableMessage = sequence >> 31;
	reliableAck = sequenceAck >> 31;

	sequence &= ~(1 << 31);
	sequenceAck &= ~(1 << 31);	

	if (net_showPackets->integerValue){
		if (reliableMessage)
			Com_Printf("%s recv %4i: seq=%i reliable=%i ack=%i rack=%i\n", net_sourceString[netChan->sock], msg->curSize, sequence, netChan->incomingReliableSequence ^ 1, sequenceAck, reliableAck);
		else
			Com_Printf("%s recv %4i: seq=%i ack=%i rack=%i\n", net_sourceString[netChan->sock], msg->curSize, sequence, sequenceAck, reliableAck);
	}

	// Discard stale or duplicated packets
	if (sequence <= netChan->incomingSequence){
		if (net_showDrop->integerValue)
			Com_Printf("%s: out of order packet %i at %i\n", NET_AddressToString(netChan->remoteAddress), sequence, netChan->incomingSequence);

		return false;
	}

	// Dropped packets don't keep the message from being used
	netChan->dropped = sequence - (netChan->incomingSequence + 1);
	if (netChan->dropped > 0){
		if (net_showDrop->integerValue)
			Com_Printf("%s: dropped %i packets at %i\n", NET_AddressToString(netChan->remoteAddress), netChan->dropped, sequence);
	}

	// If the current outgoing reliable message has been acknowledged
	// clear the buffer to make way for the next
	if (reliableAck == netChan->reliableSequence)
		netChan->reliableLength = 0;	// It has been received
	
	// If this message contains a reliable message, bump 
	// incomingReliableSequence 
	netChan->incomingSequence = sequence;
	netChan->incomingAcknowledged = sequenceAck;
	netChan->incomingReliableAcknowledged = reliableAck;

	if (reliableMessage)
		netChan->incomingReliableSequence ^= 1;

	// The message can now be read from the current message pointer
	netChan->lastReceived = Sys_Milliseconds();

	return true;
}