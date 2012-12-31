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
// netChan.h - Network channel
//


#ifndef __NETCHAN_H__
#define __NETCHAN_H__


/*
 ==============================================================================

 Network Channels:

 TODO

 Packet header:
 --------------
 31	sequence
 1		Does this message contain a reliable payload
 31		Acknowledge sequence
 1		Acknowledge receipt of even/odd message
 16		qport

 The remote connection never knows if it missed a reliable message, the
 local side detects that it has been dropped by seeing a sequence
 acknowledge higher than the last reliable sequence, but without the
 correct even/odd bit for the reliable set.

 If the sender notices that a reliable message has been dropped, it will
 be retransmitted. It will not be retransmitted again until a message
 after the retransmit has been acknowledged and the reliable still
 failed to get there.

 If the sequence number is -1, the packet should be handled without a
 net connection.

 The reliable message can be added to at any time by doing
 MSG_Write* (&netChan->message, <data>).

 If the message buffer is overflowed, either by a single message, or by
 multiple frames worth piling up while the last reliable transmit goes
 unacknowledged, the netChan signals a fatal error.

 Reliable messages are always placed first in a packet, then the
 unreliable message is included if there is sufficient room.

 To the receiver, there is no distinction between the reliable and
 unreliable parts of the message, they are just processed out as a
 single larger message.

 Illogical packet sequence numbers cause the packet to be dropped, but
 do not kill the connection. This, combined with the tight window of
 valid reliable acknowledgement numbers provides protection against
 malicious address spoofing.

 The qport field is a workaround for bad address translating routers
 that sometimes remap the client's source port on a packet during
 gameplay.

 If the base part of the net address matches and the qport matches, then
 the channel matches even if the IP port differs. The IP port should be
 updated to the new value before sending out any replies.

 If there is no information that needs to be transfered on a given
 frame, such as during the connection stage while waiting for the client
 to load, then a packet only needs to be delivered if there is something
 in the unacknowledged reliable.

 ==============================================================================
*/

typedef struct {
	netSrc_t				sock;

	int						dropped;						// Between last packet and previous

	int						lastReceived;					// For time-outs
	int						lastSent;						// For retransmits

	netAdr_t				remoteAddress;
	int						channelPort;					// Channel port value to write when transmitting

	// Sequencing variables
	int						incomingSequence;
	int						incomingAcknowledged;
	int						incomingReliableSequence;		// Single bit, maintained local
	int						incomingReliableAcknowledged;	// Single bit

	int						outgoingSequence;
	int						reliableSequence;				// Single bit
	int						lastReliableSequence;			// Sequence number of last send

	// Reliable staging and holding areas
	msg_t					message;						// Writing buffer to send to server
	byte					messageBuffer[MAX_MSGLEN-16];	// Leave space for header

	// Message is copied to this buffer when it is first transfered
	int						reliableLength;
	byte					reliableBuffer[MAX_MSGLEN-16];	// Unacked reliable message
} netChan_t;

extern netAdr_t				net_from;
extern msg_t				net_message;

// Initializes the network channel subsystem
void			NetChan_Init ();

// Opens a channel to a remote system
void			NetChan_Setup (netChan_t *netChan, netSrc_t sock, const netAdr_t adr, int port);

// Tries to send an unreliable message to a connection, and handles the
// transmition / retransmition of the reliable messages.
// A 0 length will still generate a packet and deal with the reliable 
// messages.
void			NetChan_Transmit (netChan_t *netChan, const void *data, int length);

// Called when the current net_message is from remoteAddress. Modifies 
// net_message so that it points to the packet payload.
bool			NetChan_Process (netChan_t *netChan, msg_t *msg);


#endif	// __NETCHAN_H__