/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


// sv_main.c -- server main program


#include "server.h"


/*
 =======================================================================

 COM_PRINTF REDIRECTION

 =======================================================================
*/

char sv_outputBuf[OUTPUTBUF_LENGTH];


/*
 =================
 SV_FlushRedirect
 =================
*/
void SV_FlushRedirect (redirect_t redirect, char *outputBuf){

	if (redirect == RD_PACKET)
		NET_OutOfBandPrintf(NS_SERVER, net_from, "print\n%s", outputBuf);
	else if (redirect == RD_CLIENT){
		MSG_WriteByte(&sv_client->netChan.message, SVC_PRINT);
		MSG_WriteByte(&sv_client->netChan.message, PRINT_HIGH);
		MSG_WriteString(&sv_client->netChan.message, outputBuf);
	}
}


/*
 =======================================================================

 EVENT MESSAGES

 =======================================================================
*/


/*
 =================
 SV_ClientPrintf

 Sends text across to be displayed if the level passes
 =================
*/
void SV_ClientPrintf (client_t *cl, int level, const char *fmt, ...){

	char	string[1024];
	va_list	argPtr;

	if (level < cl->messageLevel)
		return;

	va_start(argPtr, fmt);
	vsnprintf(string, sizeof(string), fmt, argPtr);
	va_end(argPtr);

	MSG_WriteByte(&cl->netChan.message, SVC_PRINT);
	MSG_WriteByte(&cl->netChan.message, level);
	MSG_WriteString(&cl->netChan.message, string);
}

/*
 =================
 SV_BroadcastPrintf

 Sends text to all active clients
 =================
*/
void SV_BroadcastPrintf (int level, const char *fmt, ...){

	char		string[1024];
	va_list		argPtr;
	client_t	*cl;
	int			i;

	va_start(argPtr, fmt);
	vsnprintf(string, sizeof(string), fmt, argPtr);
	va_end(argPtr);

	// Echo to console
	if (com_dedicated->integerValue)
		Com_Printf("%s", string);

	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state != CS_SPAWNED)
			continue;

		if (level < cl->messageLevel)
			continue;

		MSG_WriteByte(&cl->netChan.message, SVC_PRINT);
		MSG_WriteByte(&cl->netChan.message, level);
		MSG_WriteString(&cl->netChan.message, string);
	}
}

/*
 =================
 SV_BroadcastCommand

 Sends text to all active clients
 =================
*/
void SV_BroadcastCommand (const char *fmt, ...){

	char	string[1024];
	va_list	argPtr;

	if (!sv.state)
		return;

	va_start(argPtr, fmt);
	vsnprintf(string, sizeof(string), fmt, argPtr);
	va_end(argPtr);

	MSG_WriteByte(&sv.multicast, SVC_STUFFTEXT);
	MSG_WriteString(&sv.multicast, string);
	SV_Multicast(NULL, MULTICAST_ALL_R);
}

/*
 =================
 SV_Multicast

 Sends the contents of sv.multicast to a subset of the clients, then 
 clears sv.multicast.

 MULTICAST_ALL = same as broadcast (origin can be NULL)
 MULTICAST_PVS = send to clients potentially visible from origin
 MULTICAST_PHS = send to clients potentially hearable from origin
 =================
*/
void SV_Multicast (vec3_t origin, multicast_t to){

	client_t	*cl;
	byte		*mask;
	int			leafNum, cluster, area1, area2;
	int			i;
	bool		reliable = false;

	if (to != MULTICAST_ALL_R && to != MULTICAST_ALL){
		leafNum = CM_PointInLeaf(origin, 0);
		area1 = CM_LeafArea(leafNum);
	}
	else {
		leafNum = 0;	// Just to avoid compiler warnings
		area1 = 0;
	}

	// If doing a serverrecord, store everything
	if (svs.demoFile)
		MSG_Write(&svs.demoMulticast, sv.multicast.data, sv.multicast.curSize);

	switch (to){
	case MULTICAST_ALL_R:
		reliable = true;	// Intentional fallthrough

	case MULTICAST_ALL:
		leafNum = 0;
		mask = NULL;

		break;
	case MULTICAST_PHS_R:
		reliable = true;	// Intentional fallthrough

	case MULTICAST_PHS:
		leafNum = CM_PointInLeaf(origin, 0);
		cluster = CM_LeafCluster(leafNum);
		mask = CM_ClusterPHS(cluster);

		break;
	case MULTICAST_PVS_R:
		reliable = true;	// Intentional fallthrough

	case MULTICAST_PVS:
		leafNum = CM_PointInLeaf(origin, 0);
		cluster = CM_LeafCluster(leafNum);
		mask = CM_ClusterPVS(cluster);

		break;
	default:
		Com_Error(ERR_DROP, "SV_Multicast: bad to");
	}

	// Send the data to all relevant clients
	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state == CS_FREE || cl->state == CS_ZOMBIE)
			continue;

		if (cl->state != CS_SPAWNED && !reliable)
			continue;

		if (mask){
			leafNum = CM_PointInLeaf(cl->edict->s.origin, 0);
			cluster = CM_LeafCluster(leafNum);
			area2 = CM_LeafArea(leafNum);

			if (!CM_AreasAreConnected(area1, area2))
				continue;

			if (!(mask[cluster>>3] & (1<<(cluster&7))))
				continue;
		}

		if (reliable)
			MSG_Write(&cl->netChan.message, sv.multicast.data, sv.multicast.curSize);
		else
			MSG_Write(&cl->datagram, sv.multicast.data, sv.multicast.curSize);
	}

	MSG_Clear(&sv.multicast);
}

/*  
 =================
 SV_StartSound

 Each entity can have eight independent sound sources, like voice,
 weapon, feet, etc...

 If channel & 8, the sound will be sent to everyone, not just things in 
 the PHS.

 FIXME: if entity isn't in PHS, they must be forced to be sent or have 
 the origin explicitly sent.

 Channel 0 is an auto-allocate channel, the others override anything
 already running on that entity/channel pair.

 An attenuation of 0 will play full volume everywhere in the level. 
 Larger attenuations will drop off (max 4 attenuation).

 TimeOfs can range from 0.0 to 0.255 to cause sounds to be started later 
 in the frame than they normally would.

 If origin is NULL, the origin is determined from the entity origin or 
 the mid point of the entity box for brush models.
 =================
*/  
void SV_StartSound (vec3_t origin, edict_t *entity, int channel, int sound, float volume, float attenuation, float timeOfs){

    int		flags;
	int		entNum, sendChannel;
	vec3_t	originVec;
	bool	usePHS;

	if (volume < 0 || volume > 1.0)
		Com_Error(ERR_DROP, "SV_StartSound: volume = %f", volume);

	if (attenuation < 0 || attenuation > 4)
		Com_Error(ERR_DROP, "SV_StartSound: attenuation = %f", attenuation);

	if (timeOfs < 0 || timeOfs > 0.255)
		Com_Error(ERR_DROP, "SV_StartSound: timeOfs = %f", timeOfs);

	entNum = NUM_FOR_EDICT(entity);

	if (channel & CHAN_NO_PHS_ADD){
		channel &= 7;
		usePHS = false;
	}
	else {
		// If the sound doesn't attenuate, send it to everyone (global
		// radio chatter, voice-overs, etc...)
		if (attenuation == ATTN_NONE)
			usePHS = false;
		else
			usePHS = true;
	}

	sendChannel = (entNum<<3) | (channel&7);

	// Always send the entity number for channel overrides
	flags = SND_ENT;

	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		flags |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		flags |= SND_ATTENUATION;
	if (timeOfs)
		flags |= SND_OFFSET;

	// The client doesn't know that brush models have weird origins.
	// The origin can also be explicitly set.
	if ((entity->svflags & SVF_NOCLIENT) || (entity->solid == SOLID_BSP) || origin)
		flags |= SND_POS;

	// Use the entity origin unless it is a brush model or explicitly
	// specified
	if (!origin){
		origin = originVec;
		if (entity->solid == SOLID_BSP){
			originVec[0] = entity->s.origin[0] + 0.5 * (entity->mins[0]+entity->maxs[0]);
			originVec[1] = entity->s.origin[1] + 0.5 * (entity->mins[1]+entity->maxs[1]);
			originVec[2] = entity->s.origin[2] + 0.5 * (entity->mins[2]+entity->maxs[2]);
		}
		else
			VectorCopy(entity->s.origin, originVec);
	}

	MSG_WriteByte(&sv.multicast, SVC_SOUND);
	MSG_WriteByte(&sv.multicast, flags);
	MSG_WriteByte(&sv.multicast, sound);

	if (flags & SND_VOLUME)
		MSG_WriteByte(&sv.multicast, volume*255);
	if (flags & SND_ATTENUATION)
		MSG_WriteByte(&sv.multicast, attenuation*64);
	if (flags & SND_OFFSET)
		MSG_WriteByte(&sv.multicast, timeOfs*1000);
	if (flags & SND_ENT)
		MSG_WriteShort(&sv.multicast, sendChannel);
	if (flags & SND_POS)
		MSG_WritePos(&sv.multicast, origin);

	if (channel & CHAN_RELIABLE){
		if (usePHS)
			SV_Multicast(origin, MULTICAST_PHS_R);
		else
			SV_Multicast(origin, MULTICAST_ALL_R);
	}
	else {
		if (usePHS)
			SV_Multicast(origin, MULTICAST_PHS);
		else
			SV_Multicast(origin, MULTICAST_ALL);
	}
}           


/*
 =======================================================================

 FRAME UPDATES

 =======================================================================
*/


/*
 =================
 SV_SendClientDatagram
 =================
*/
static void SV_SendClientDatagram (client_t *cl){

	byte	data[MAX_MSGLEN];
	msg_t	msg;

	MSG_Init(&msg, data, sizeof(data), true);

	// Send over all the relevant entity_state_t and the player_state_t
	SV_BuildClientFrame(cl);
	SV_WriteFrameToClient(cl, &msg);

	// Copy the accumulated multicast datagram for this client out to 
	// the message.
	// It is necessary for this to be after the SV_WriteFrameToClient so
	// that entity references will be current.
	if (cl->datagram.overflowed)
		Com_Printf(S_COLOR_YELLOW "WARNING: datagram overflowed for %s\n", cl->name);
	else
		MSG_Write(&msg, cl->datagram.data, cl->datagram.curSize);

	MSG_Clear(&cl->datagram);

	if (msg.overflowed){
		// Must have room left for the packet header
		Com_Printf(S_COLOR_YELLOW "WARNING: msg overflowed for %s\n", cl->name);
		MSG_Clear(&msg);
	}

	// Send the datagram
	NetChan_Transmit(&cl->netChan, msg.data, msg.curSize);

	// Record the size for rate estimation
	cl->messageSize[sv.frameNum % RATE_MESSAGES] = msg.curSize;
}

/*
 =================
 SV_DemoCompleted
 =================
*/
static void SV_DemoCompleted (void){

	if (sv.demoFile){
		FS_CloseFile(sv.demoFile);
		sv.demoFile = 0;
	}

	SV_NextServer();
}

/*
 =================
 SV_RateDrop

 Returns true if the client is over its current bandwidth estimation and 
 should not be sent another packet
 =================
*/
static bool SV_RateDrop (client_t *cl){

	int		i, total = 0;

	// Never drop over the loopback
	if (NET_IsLocalAddress(cl->netChan.remoteAddress))
		return false;

	for (i = 0; i < RATE_MESSAGES; i++)
		total += cl->messageSize[i];

	if (total > cl->rate){
		cl->suppressCount++;
		cl->messageSize[sv.frameNum % RATE_MESSAGES] = 0;
		return true;
	}

	return false;
}

/*
 =================
 SV_SendClientMessages
 =================
*/
void SV_SendClientMessages (void){

	client_t	*cl;
	byte		data[MAX_MSGLEN];
	int			i, r, length = 0;

	// Read the next demo message if needed
	if ((sv.state == SS_DEMO && sv.demoFile) && !com_paused->integerValue){
		r = FS_Read(sv.demoFile, &length, sizeof(length));
		if (r != 4){
			SV_DemoCompleted();
			return;
		}

		length = LittleLong(length);
		if (length == -1){
			SV_DemoCompleted();
			return;
		}

		if (length > MAX_MSGLEN)
			Com_Error(ERR_DROP, "SV_SendClientMessages: length > MAX_MSGLEN");

		r = FS_Read(sv.demoFile, data, length);
		if (r != length){
			SV_DemoCompleted();
			return;
		}
	}

	// Send a message to each connected client
	for (i = 0, cl = svs.clients; i < sv_maxClients->integerValue; i++, cl++){
		if (cl->state == CS_FREE)
			continue;

		// If the reliable message overflowed, drop the client
		if (cl->netChan.message.overflowed){
			MSG_Clear(&cl->netChan.message);
			MSG_Clear(&cl->datagram);
			SV_BroadcastPrintf(PRINT_HIGH, "%s overflowed\n", cl->name);
			SV_DropClient(cl);
		}

		if (sv.state == SS_DEMO || sv.state == SS_CINEMATIC || sv.state == SS_PIC)
			NetChan_Transmit(&cl->netChan, data, length);
		else if (cl->state == CS_SPAWNED){
			// Don't overrun bandwidth
			if (SV_RateDrop(cl))
				continue;

			SV_SendClientDatagram(cl);
		}
		else {
			// Just update reliable	if needed
			if (cl->netChan.message.curSize || Sys_Milliseconds() - cl->netChan.lastSent > 1000)
				NetChan_Transmit(&cl->netChan, NULL, 0);
		}
	}
}
