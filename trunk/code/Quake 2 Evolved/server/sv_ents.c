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


#include "server.h"


/*
 =======================================================================

 ENCODE A CLIENT FRAME ONTO THE NETWORK CHANNEL

 =======================================================================
*/


/*
 =================
 SV_EmitPacketEntities

 Writes a delta update of an entity_state_t list to the message
 =================
*/
static void SV_EmitPacketEntities (clientFrame_t *from, clientFrame_t *to, msg_t *msg){

	entity_state_t	*oldState, *newState;
	int				oldIndex, newIndex;
	int				oldNum, newNum;
	int				fromNumEntities;
	int				bits;

	MSG_WriteByte(msg, SVC_PACKETENTITIES);

	if (!from)
		fromNumEntities = 0;
	else
		fromNumEntities = from->numEntities;

	newIndex = 0;
	oldIndex = 0;
	while (newIndex < to->numEntities || oldIndex < fromNumEntities){
		if (newIndex >= to->numEntities)
			newNum = 9999;
		else {
			newState = &svs.clientEntities[(to->firstEntity+newIndex)%svs.numClientEntities];
			newNum = newState->number;
		}

		if (oldIndex >= fromNumEntities)
			oldNum = 9999;
		else {
			oldState = &svs.clientEntities[(from->firstEntity+oldIndex)%svs.numClientEntities];
			oldNum = oldState->number;
		}

		if (newNum == oldNum){
			// Delta update from old position.
			// Because the force parm is false, this will not result in
			// any bytes being emited if the entity has not changed at 
			// all.
			// Note that players are always 'newentities', this updates 
			// their oldorigin always and prevents warping.
			MSG_WriteDeltaEntity(msg, oldState, newState, false, newState->number <= sv_maxClients->integerValue);

			oldIndex++;
			newIndex++;
			continue;
		}

		if (newNum < oldNum){
			// This is a new entity, send it from the baseline
			MSG_WriteDeltaEntity(msg, &sv.baselines[newNum], newState, true, true);

			newIndex++;
			continue;
		}

		if (newNum > oldNum){
			// The old entity isn't present in the new message
			bits = U_REMOVE;
			if (oldNum >= 256)
				bits |= U_NUMBER16 | U_MOREBITS1;

			MSG_WriteByte(msg, bits&255);
			if (bits & 0x0000ff00)
				MSG_WriteByte(msg, (bits>>8)&255);

			if (bits & U_NUMBER16)
				MSG_WriteShort(msg, oldNum);
			else
				MSG_WriteByte(msg, oldNum);

			oldIndex++;
			continue;
		}
	}

	MSG_WriteShort(msg, 0);	// End of packet entities
}

/*
 =================
 SV_WritePlayerStateToClient
 =================
*/
static void SV_WritePlayerStateToClient (clientFrame_t *from, clientFrame_t *to, msg_t *msg){

	player_state_t	*newPS, *oldPS, dummy;
	int				i, flags, statBits;

	newPS = &to->ps;
	if (!from){
		Mem_Fill(&dummy, 0, sizeof(dummy));
		oldPS = &dummy;
	}
	else
		oldPS = &from->ps;

	// Determine what needs to be sent
	flags = 0;

	if (newPS->pmove.pm_type != oldPS->pmove.pm_type)
		flags |= PS_M_TYPE;

	if (newPS->pmove.origin[0] != oldPS->pmove.origin[0] || newPS->pmove.origin[1] != oldPS->pmove.origin[1] || newPS->pmove.origin[2] != oldPS->pmove.origin[2])
		flags |= PS_M_ORIGIN;

	if (newPS->pmove.velocity[0] != oldPS->pmove.velocity[0] || newPS->pmove.velocity[1] != oldPS->pmove.velocity[1] || newPS->pmove.velocity[2] != oldPS->pmove.velocity[2])
		flags |= PS_M_VELOCITY;

	if (newPS->pmove.pm_time != oldPS->pmove.pm_time)
		flags |= PS_M_TIME;

	if (newPS->pmove.pm_flags != oldPS->pmove.pm_flags)
		flags |= PS_M_FLAGS;

	if (newPS->pmove.gravity != oldPS->pmove.gravity)
		flags |= PS_M_GRAVITY;

	if (newPS->pmove.delta_angles[0] != oldPS->pmove.delta_angles[0] || newPS->pmove.delta_angles[1] != oldPS->pmove.delta_angles[1] || newPS->pmove.delta_angles[2] != oldPS->pmove.delta_angles[2])
		flags |= PS_M_DELTA_ANGLES;

	if (newPS->viewoffset[0] != oldPS->viewoffset[0] || newPS->viewoffset[1] != oldPS->viewoffset[1] || newPS->viewoffset[2] != oldPS->viewoffset[2])
		flags |= PS_VIEWOFFSET;

	if (newPS->viewangles[0] != oldPS->viewangles[0] || newPS->viewangles[1] != oldPS->viewangles[1] || newPS->viewangles[2] != oldPS->viewangles[2])
		flags |= PS_VIEWANGLES;

	if (newPS->kick_angles[0] != oldPS->kick_angles[0] || newPS->kick_angles[1] != oldPS->kick_angles[1] || newPS->kick_angles[2] != oldPS->kick_angles[2])
		flags |= PS_KICKANGLES;

	if (newPS->blend[0] != oldPS->blend[0] || newPS->blend[1] != oldPS->blend[1] || newPS->blend[2] != oldPS->blend[2] || newPS->blend[3] != oldPS->blend[3])
		flags |= PS_BLEND;

	if (newPS->fov != oldPS->fov)
		flags |= PS_FOV;

	if (newPS->rdflags != oldPS->rdflags)
		flags |= PS_RDFLAGS;

	if (newPS->gunframe != oldPS->gunframe)
		flags |= PS_WEAPONFRAME;

	flags |= PS_WEAPONINDEX;

	// Write it
	MSG_WriteByte(msg, SVC_PLAYERINFO);
	MSG_WriteShort(msg, flags);

	// Write the pmove_state_t
	if (flags & PS_M_TYPE)
		MSG_WriteByte(msg, newPS->pmove.pm_type);

	if (flags & PS_M_ORIGIN){
		MSG_WriteShort(msg, newPS->pmove.origin[0]);
		MSG_WriteShort(msg, newPS->pmove.origin[1]);
		MSG_WriteShort(msg, newPS->pmove.origin[2]);
	}

	if (flags & PS_M_VELOCITY){
		MSG_WriteShort(msg, newPS->pmove.velocity[0]);
		MSG_WriteShort(msg, newPS->pmove.velocity[1]);
		MSG_WriteShort(msg, newPS->pmove.velocity[2]);
	}

	if (flags & PS_M_TIME)
		MSG_WriteByte(msg, newPS->pmove.pm_time);

	if (flags & PS_M_FLAGS)
		MSG_WriteByte(msg, newPS->pmove.pm_flags);

	if (flags & PS_M_GRAVITY)
		MSG_WriteShort(msg, newPS->pmove.gravity);

	if (flags & PS_M_DELTA_ANGLES){
		MSG_WriteShort(msg, newPS->pmove.delta_angles[0]);
		MSG_WriteShort(msg, newPS->pmove.delta_angles[1]);
		MSG_WriteShort(msg, newPS->pmove.delta_angles[2]);
	}

	// Write the rest of the player_state_t
	if (flags & PS_VIEWOFFSET){
		MSG_WriteChar(msg, newPS->viewoffset[0]*4);
		MSG_WriteChar(msg, newPS->viewoffset[1]*4);
		MSG_WriteChar(msg, newPS->viewoffset[2]*4);
	}

	if (flags & PS_VIEWANGLES){
		MSG_WriteAngle16(msg, newPS->viewangles[0]);
		MSG_WriteAngle16(msg, newPS->viewangles[1]);
		MSG_WriteAngle16(msg, newPS->viewangles[2]);
	}

	if (flags & PS_KICKANGLES){
		MSG_WriteChar(msg, newPS->kick_angles[0]*4);
		MSG_WriteChar(msg, newPS->kick_angles[1]*4);
		MSG_WriteChar(msg, newPS->kick_angles[2]*4);
	}

	if (flags & PS_WEAPONINDEX)
		MSG_WriteByte(msg, newPS->gunindex);

	if (flags & PS_WEAPONFRAME){
		MSG_WriteByte(msg, newPS->gunframe);
		MSG_WriteChar(msg, newPS->gunoffset[0]*4);
		MSG_WriteChar(msg, newPS->gunoffset[1]*4);
		MSG_WriteChar(msg, newPS->gunoffset[2]*4);
		MSG_WriteChar(msg, newPS->gunangles[0]*4);
		MSG_WriteChar(msg, newPS->gunangles[1]*4);
		MSG_WriteChar(msg, newPS->gunangles[2]*4);
	}

	if (flags & PS_BLEND){
		MSG_WriteByte(msg, newPS->blend[0]*255);
		MSG_WriteByte(msg, newPS->blend[1]*255);
		MSG_WriteByte(msg, newPS->blend[2]*255);
		MSG_WriteByte(msg, newPS->blend[3]*255);
	}

	if (flags & PS_FOV)
		MSG_WriteByte(msg, newPS->fov);

	if (flags & PS_RDFLAGS)
		MSG_WriteByte(msg, newPS->rdflags);
	
	// Send stats
	statBits = 0;
	for (i = 0; i < MAX_STATS; i++){
		if (newPS->stats[i] != oldPS->stats[i])
			statBits |= 1<<i;
	}

	MSG_WriteLong(msg, statBits);

	for (i = 0; i < MAX_STATS; i++){
		if (statBits & (1<<i))
			MSG_WriteShort(msg, newPS->stats[i]);
	}
}

/*
 =================
 SV_WriteFrameToClient
 =================
*/
void SV_WriteFrameToClient (client_t *cl, msg_t *msg){

	clientFrame_t	*frame, *oldFrame;
	int				lastFrame;

	// This is the frame we are creating
	frame = &cl->frames[sv.frameNum & UPDATE_MASK];

	if (cl->lastFrame <= 0){
		// Client is asking for a retransmit
		oldFrame = NULL;
		lastFrame = -1;
	}
	else if (sv.frameNum - cl->lastFrame >= (UPDATE_BACKUP - 3)){
		// Client hasn't gotten a good message through in a long time
		oldFrame = NULL;
		lastFrame = -1;
	}
	else {
		// We have a valid message to delta from
		oldFrame = &cl->frames[cl->lastFrame & UPDATE_MASK];
		lastFrame = cl->lastFrame;
	}

	MSG_WriteByte(msg, SVC_FRAME);
	MSG_WriteLong(msg, sv.frameNum);
	MSG_WriteLong(msg, lastFrame);			// What we are delta'ing from
	MSG_WriteByte(msg, cl->suppressCount);	// Rate dropped packets
	cl->suppressCount = 0;

	// Send over the areabits
	MSG_WriteByte(msg, frame->areaBytes);
	MSG_Write(msg, frame->areaBits, frame->areaBytes);

	// Delta encode the player state
	SV_WritePlayerStateToClient(oldFrame, frame, msg);

	// Delta encode the entities
	SV_EmitPacketEntities(oldFrame, frame, msg);
}


/*
 =======================================================================

 BUILD A CLIENT FRAME STRUCTURE

 =======================================================================
*/

static byte		sv_fatPVS[65536/8];		// 32767 is MAX_MAP_LEAFS


/*
 =================
 SV_FatPVS

 The client will interpolate the view position, so we can't use a single 
 PVS point
 =================
*/
static void SV_FatPVS (vec3_t org){

	int		leafs[64];
	int		i, j, count, longs;
	byte	*src;
	vec3_t	mins, maxs;

	for (i = 0; i < 3; i++){
		mins[i] = org[i] - 8;
		maxs[i] = org[i] + 8;
	}

	count = CM_BoxLeafNums(mins, maxs, leafs, 64, NULL);
	if (count < 1)
		Com_Error(ERR_DROP, "SV_FatPVS: count < 1");

	longs = (CM_NumClusters()+31)>>5;

	// Convert leafs to clusters
	for (i = 0; i < count; i++)
		leafs[i] = CM_LeafCluster(leafs[i]);

	Mem_Copy(sv_fatPVS, CM_ClusterPVS(leafs[0]), longs<<2);

	// Or in all the other leaf bits
	for (i = 1; i < count; i++){
		for (j = 0; j < i; j++){
			if (leafs[i] == leafs[j])
				break;
		}

		if (j != i)
			continue;		// Already have the cluster we want

		src = CM_ClusterPVS(leafs[i]);
		for (j = 0; j < longs; j++)
			((long *)sv_fatPVS)[j] |= ((long *)src)[j];
	}
}

/*
 =================
 SV_BuildClientFrame

 Decides which entities are going to be visible to the client
 =================
*/
void SV_BuildClientFrame (client_t *cl){

	int				i, e;
	vec3_t			org, delta;
	edict_t			*edict, *clEdict;
	clientFrame_t	*frame;
	entity_state_t	*state;
	int				l, leafNum, area, cluster;
	byte			*phs, *bitVector;

	clEdict = cl->edict;
	if (!clEdict->client)
		return;		// Not in game yet

	// This is the frame we are creating
	frame = &cl->frames[sv.frameNum & UPDATE_MASK];

	frame->sentTime = svs.realTime; // Save it for ping calc later

	// Find the client's PVS
	org[0] = clEdict->client->ps.pmove.origin[0] * 0.125 + clEdict->client->ps.viewoffset[0];
	org[1] = clEdict->client->ps.pmove.origin[1] * 0.125 + clEdict->client->ps.viewoffset[1];
	org[2] = clEdict->client->ps.pmove.origin[2] * 0.125 + clEdict->client->ps.viewoffset[2];

	leafNum = CM_PointInLeaf(org, 0);
	area = CM_LeafArea(leafNum);
	cluster = CM_LeafCluster(leafNum);

	// Calculate the visible areas
	frame->areaBytes = CM_WriteAreaBits(frame->areaBits, area);

	// Grab the current player_state_t
	frame->ps = clEdict->client->ps;

	SV_FatPVS(org);
	phs = CM_ClusterPHS(cluster);

	// Build up the list of visible entities
	frame->numEntities = 0;
	frame->firstEntity = svs.nextClientEntities;

	for (e = 1; e < ge->num_edicts; e++){
		edict = EDICT_NUM(e);

		// Ignore ents without visible models
		if (edict->svflags & SVF_NOCLIENT)
			continue;

		// Ignore ents without visible models unless they have an effect
		if (!edict->s.modelindex && !edict->s.effects && !edict->s.sound && !edict->s.event)
			continue;

		// Ignore if not touching a PV leaf
		if (edict != clEdict){
			// Check area
			if (!CM_AreasAreConnected(area, edict->areanum)){
				// Doors can legally straddle two areas, so we may need 
				// to check another one
				if (!edict->areanum2 || !CM_AreasAreConnected(area, edict->areanum2))
					continue;		// Blocked by a door
			}

			// Beams just check one point for PHS
			if (edict->s.renderfx & RF_BEAM){
				l = edict->clusternums[0];
				if (!(phs[l >> 3] & (1 << (l&7))))
					continue;
			}
			else {
				// FIXME: if an entity has a model and a sound, but
				// isn't in the PVS, only the PHS, clear the model
				bitVector = sv_fatPVS;

				if (edict->num_clusters == -1){
					// Too many leafs for individual check, go by headnode
					if (!CM_HeadNodeVisible(edict->headnode, bitVector))
						continue;
				}
				else {
					// Check individual leafs
					for (i = 0; i < edict->num_clusters; i++){
						l = edict->clusternums[i];
						if (bitVector[l >> 3] & (1 << (l&7)))
							break;
					}

					if (i == edict->num_clusters)
						continue;		// Not visible
				}

				if (!edict->s.modelindex){
					// Don't send sounds if they will be attenuated away
					VectorSubtract(org, edict->s.origin, delta);
					if (VectorLength(delta) > 400)
						continue;
				}
			}
		}

		// Add it to the circular clientEntities array
		if (edict->s.number != e){
			Com_DPrintf(S_COLOR_YELLOW "FIXING EDICT->S.NUMBER != E!!!\n");
			edict->s.number = e;
		}

		state = &svs.clientEntities[svs.nextClientEntities%svs.numClientEntities];
		*state = edict->s;

		// Don't mark players missiles as solid
		if (edict->owner == cl->edict)
			state->solid = 0;

		svs.nextClientEntities++;
		frame->numEntities++;
	}
}

/*
 =================
 SV_RecordDemoMessage

 Save everything in the world out without deltas. 
 Used for recording footage for merged or assembled demos.
 =================
*/
void SV_RecordDemoMessage (void){

	byte			data[32768];
	msg_t			msg;
	edict_t			*edict;
	entity_state_t	nullState;
	int				e, length;

	if (!svs.demoFile)
		return;

	Mem_Fill(&nullState, 0, sizeof(nullState));

	MSG_Init(&msg, data, sizeof(data), false);

	// Write a frame message that doesn't contain a player_state_t
	MSG_WriteByte(&msg, SVC_FRAME);
	MSG_WriteLong(&msg, sv.frameNum);

	MSG_WriteByte(&msg, SVC_PACKETENTITIES);

	e = 1;
	edict = EDICT_NUM(e);
	while (e < ge->num_edicts){
		// Ignore ents without visible models unless they have an effect
		if (edict->inuse && edict->s.number && (edict->s.modelindex || edict->s.effects || edict->s.sound || edict->s.event) && !(edict->svflags & SVF_NOCLIENT))
			MSG_WriteDeltaEntity(&msg, &nullState, &edict->s, false, true);

		e++;
		edict = EDICT_NUM(e);
	}

	MSG_WriteShort(&msg, 0);		// End of packet entities

	// Now add the accumulated multicast information
	MSG_Write(&msg, svs.demoMulticast.data, svs.demoMulticast.curSize);
	MSG_Clear(&svs.demoMulticast);

	// Now write the entire message to the file, prefixed by the length
	length = LittleLong(msg.curSize);
	FS_Write(svs.demoFile, &length, sizeof(length));
	FS_Write(svs.demoFile, msg.data, msg.curSize);
}
