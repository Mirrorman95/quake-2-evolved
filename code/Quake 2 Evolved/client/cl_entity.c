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
// cl_entity.c - Entity parsing and management
//


#include "client.h"


static const char *	cl_eventNames[256] = {
	"EV_NONE",
	"EV_ITEM_RESPAWN",
	"EV_FOOTSTEP",
	"EV_FALLSHORT",
	"EV_FALL",
	"EV_FALLFAR",
	"EV_PLAYER_TELEPORT",
	"EV_OTHER_TELEPORT"
};


/*
 ==============================================================================

 FRAME PARSING

 ==============================================================================
*/


/*
 ==================
 CL_ParseEntityBits

 Returns the entity number and the header bits
 ==================
*/
static int CL_ParseEntityBits (uint *bits){

    uint	b, total;
    int		number;

    total = MSG_ReadByte(&net_message);

    if (total & U_MOREBITS1){
        b = MSG_ReadByte(&net_message);
        total |= b << 8;
    }
    if (total & U_MOREBITS2){
        b = MSG_ReadByte(&net_message);
        total |= b << 16;
    }
    if (total & U_MOREBITS3){
        b = MSG_ReadByte(&net_message);
        total |= b << 24;
    }

	if (total & U_NUMBER16)
		number = MSG_ReadShort(&net_message);
    else
		number = MSG_ReadByte(&net_message);

    *bits = total;

    return number;
}

/*
 ==================
 CL_ParseDelta

 Can go from either a baseline or a previous packet entity
 ==================
*/
static void CL_ParseDelta (entity_state_t *from, entity_state_t *to, int number, int bits){

    // Set everything to the state we are delta'ing from
    *to = *from;

    VectorCopy(from->origin, to->old_origin);
    to->number = number;

    if (bits & U_MODEL)
        to->modelindex = MSG_ReadByte(&net_message);
    if (bits & U_MODEL2)
        to->modelindex2 = MSG_ReadByte(&net_message);
    if (bits & U_MODEL3)
        to->modelindex3 = MSG_ReadByte(&net_message);
    if (bits & U_MODEL4)
        to->modelindex4 = MSG_ReadByte(&net_message);

    if (bits & U_FRAME8)
        to->frame = MSG_ReadByte(&net_message);
    if (bits & U_FRAME16)
        to->frame = MSG_ReadShort(&net_message);

    if ((bits & U_SKIN8) && (bits & U_SKIN16))
        to->skinnum = MSG_ReadLong(&net_message);
    else if (bits & U_SKIN8)
        to->skinnum = MSG_ReadByte(&net_message);
    else if (bits & U_SKIN16)
        to->skinnum = MSG_ReadShort(&net_message);

    if ((bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16))
        to->effects = MSG_ReadLong(&net_message);
    else if (bits & U_EFFECTS8)
        to->effects = MSG_ReadByte(&net_message);
    else if (bits & U_EFFECTS16)
        to->effects = MSG_ReadShort(&net_message);

    if ((bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16))
        to->renderfx = MSG_ReadLong(&net_message);
    else if (bits & U_RENDERFX8)
        to->renderfx = MSG_ReadByte(&net_message);
    else if (bits & U_RENDERFX16)
        to->renderfx = MSG_ReadShort(&net_message);

    if (bits & U_ORIGIN1)
        to->origin[0] = MSG_ReadCoord(&net_message);
    if (bits & U_ORIGIN2)
        to->origin[1] = MSG_ReadCoord(&net_message);
    if (bits & U_ORIGIN3)
        to->origin[2] = MSG_ReadCoord(&net_message);

    if (bits & U_ANGLE1)
        to->angles[0] = MSG_ReadAngle(&net_message);
    if (bits & U_ANGLE2)
        to->angles[1] = MSG_ReadAngle(&net_message);
    if (bits & U_ANGLE3)
        to->angles[2] = MSG_ReadAngle(&net_message);

    if (bits & U_OLDORIGIN)
        MSG_ReadPos(&net_message, to->old_origin);

    if (bits & U_SOUND)
        to->sound = MSG_ReadByte(&net_message);

    if (bits & U_EVENT)
        to->event = MSG_ReadByte(&net_message);
    else
        to->event = 0;

    if (bits & U_SOLID)
        to->solid = MSG_ReadShort(&net_message);
}

/*
 ==================

 Parses deltas from the given base and adds the resulting entity to the
 current frame 
 ==================
*/
static void CL_DeltaEntity (frame_t *frame, entity_state_t *from, int number, int bits){

    centity_t		*centity;
    entity_state_t	*to;

	frame->numEntities++;

	centity = &cl.entities[number];

    to = &cl.parseEntities[cl.parseEntitiesIndex & (MAX_PARSE_ENTITIES - 1)];
    cl.parseEntitiesIndex++;

    CL_ParseDelta(from, to, number, bits);

    // Some data changes will force no lerping
    if (to->modelindex != centity->current.modelindex || to->modelindex2 != centity->current.modelindex2 || to->modelindex3 != centity->current.modelindex3 || to->modelindex4 != centity->current.modelindex4)
		centity->serverFrame = -99;
	if (fabs(to->origin[0] - centity->current.origin[0]) > 512.0f || fabs(to->origin[1] - centity->current.origin[1]) > 512.0f || fabs(to->origin[2] - centity->current.origin[2]) > 512.0f)
		centity->serverFrame = -99;
	if (to->event == EV_PLAYER_TELEPORT || to->event == EV_OTHER_TELEPORT)
        centity->serverFrame = -99;

    if (centity->serverFrame != frame->serverFrame - 1){
		// Wasn't in last update, so initialize some things.
        // Duplicate the current state so lerping doesn't hurt anything.
        centity->prev = *to;

        if (to->event == EV_OTHER_TELEPORT){
            VectorCopy(to->origin, centity->prev.origin);
            VectorCopy(to->origin, centity->lerpOrigin);
        }
        else {
            VectorCopy(to->old_origin, centity->prev.origin);
            VectorCopy(to->old_origin, centity->lerpOrigin);
        }
    }
    else
		// Shuffle the last state to previous
        centity->prev = centity->current;

    centity->serverFrame = frame->serverFrame;
    centity->current = *to;
}

/*
 ==================
 CL_ParsePacketEntities

 A SVC_PACKETENTITIES has just been parsed, deal with the rest of the
 data stream
 ==================
*/
static void CL_ParsePacketEntities (frame_t *oldFrame, frame_t *newFrame){

    entity_state_t	*oldState;
    int				newNum, oldNum;
    int				bits, oldIndex;

    newFrame->parseEntitiesIndex = cl.parseEntitiesIndex;
    newFrame->numEntities = 0;

    // Delta from the entities present in oldFrame
    oldIndex = 0;
    if (!oldFrame)
        oldNum = 99999;
    else {
        if (oldIndex >= oldFrame->numEntities)
            oldNum = 99999;
        else {
            oldState = &cl.parseEntities[(oldFrame->parseEntitiesIndex+oldIndex) & (MAX_PARSE_ENTITIES - 1)];
            oldNum = oldState->number;
        }
    }

    while (1){
        newNum = CL_ParseEntityBits(&bits);
        if (newNum >= MAX_EDICTS)
            Com_Error(ERR_DROP, "CL_ParsePacketEntities: newNum = %i", newNum);

        if (net_message.readCount > net_message.curSize)
            Com_Error(ERR_DROP, "CL_ParsePacketEntities: end of message");

        if (!newNum)
            break;

        while (oldNum < newNum){
			// One or more entities from the old packet are unchanged
			CL_ShowNet(3, "Unchanged: %i", oldNum);

			CL_DeltaEntity(newFrame, oldState, oldNum, 0);

            oldIndex++;

            if (oldIndex >= oldFrame->numEntities)
                oldNum = 99999;
            else {
                oldState = &cl.parseEntities[(oldFrame->parseEntitiesIndex+oldIndex) & (MAX_PARSE_ENTITIES - 1)];
                oldNum = oldState->number;
            }
        }

        if (bits & U_REMOVE){
			// The entity present in oldFrame is not in the current 
			// frame
			CL_ShowNet(3, "Remove: %i", newNum);

            if (oldNum != newNum)
                Com_DPrintf(S_COLOR_YELLOW "CL_ParsePacketEntities: oldNum != newNum\n");

            oldIndex++;

            if (oldIndex >= oldFrame->numEntities)
                oldNum = 99999;
            else {
                oldState = &cl.parseEntities[(oldFrame->parseEntitiesIndex+oldIndex) & (MAX_PARSE_ENTITIES - 1)];
                oldNum = oldState->number;
            }

            continue;
        }

        if (oldNum == newNum){
			// Delta from previous state
			CL_ShowNet(3, "Delta: %i", newNum);

            CL_DeltaEntity(newFrame, oldState, newNum, bits);

            oldIndex++;

            if (oldIndex >= oldFrame->numEntities)
                oldNum = 99999;
            else {
                oldState = &cl.parseEntities[(oldFrame->parseEntitiesIndex+oldIndex) & (MAX_PARSE_ENTITIES - 1)];
                oldNum = oldState->number;
            }

            continue;
        }

        if (oldNum > newNum){
			// Delta from baseline
			CL_ShowNet(3, "Baseline: %i", newNum);

            CL_DeltaEntity(newFrame, &cl.entities[newNum].baseline, newNum, bits);
            continue;
        }
    }

    // Any remaining entities in the old frame are copied over
    while (oldNum != 99999){
		// One or more entities from the old packet are unchanged
		CL_ShowNet(3, "Unchanged: %i", oldNum);

        CL_DeltaEntity(newFrame, oldState, oldNum, 0);

        oldIndex++;

        if (oldIndex >= oldFrame->numEntities)
            oldNum = 99999;
        else {
            oldState = &cl.parseEntities[(oldFrame->parseEntitiesIndex+oldIndex) & (MAX_PARSE_ENTITIES - 1)];
            oldNum = oldState->number;
        }
    }
}

/*
 ==================
 CL_ParsePlayerState
 ==================
*/
static void CL_ParsePlayerState (frame_t *oldFrame, frame_t *newFrame){

    player_state_t  *state;
    int				i, flags, statBits;

    state = &newFrame->playerState;

    // Clear to old value before delta parsing
    if (oldFrame)
        *state = oldFrame->playerState;
    else
        Mem_Fill(state, 0, sizeof(*state));

    flags = MSG_ReadShort(&net_message);

    // Parse the pmove_state_t
    if (flags & PS_M_TYPE)
        state->pmove.pm_type = MSG_ReadByte(&net_message);

    if (flags & PS_M_ORIGIN){
        state->pmove.origin[0] = MSG_ReadShort(&net_message);
        state->pmove.origin[1] = MSG_ReadShort(&net_message);
        state->pmove.origin[2] = MSG_ReadShort(&net_message);
    }

    if (flags & PS_M_VELOCITY){
        state->pmove.velocity[0] = MSG_ReadShort(&net_message);
        state->pmove.velocity[1] = MSG_ReadShort(&net_message);
        state->pmove.velocity[2] = MSG_ReadShort(&net_message);
    }

    if (flags & PS_M_TIME)
        state->pmove.pm_time = MSG_ReadByte(&net_message);

    if (flags & PS_M_FLAGS)
        state->pmove.pm_flags = MSG_ReadByte(&net_message);

    if (flags & PS_M_GRAVITY)
        state->pmove.gravity = MSG_ReadShort(&net_message);

    if (flags & PS_M_DELTA_ANGLES){
        state->pmove.delta_angles[0] = MSG_ReadShort(&net_message);
        state->pmove.delta_angles[1] = MSG_ReadShort(&net_message);
        state->pmove.delta_angles[2] = MSG_ReadShort(&net_message);
    }

    if (cl.demoPlayback)
        state->pmove.pm_type = PM_FREEZE;       // Demo playback

    // Parse the rest of the player_state_t
    if (flags & PS_VIEWOFFSET){
        state->viewoffset[0] = MSG_ReadChar(&net_message) * 0.25f;
        state->viewoffset[1] = MSG_ReadChar(&net_message) * 0.25f;
        state->viewoffset[2] = MSG_ReadChar(&net_message) * 0.25f;
    }

    if (flags & PS_VIEWANGLES){
        state->viewangles[0] = MSG_ReadAngle16(&net_message);
        state->viewangles[1] = MSG_ReadAngle16(&net_message);
        state->viewangles[2] = MSG_ReadAngle16(&net_message);
    }

    if (flags & PS_KICKANGLES){
        state->kick_angles[0] = MSG_ReadChar(&net_message) * 0.25f;
        state->kick_angles[1] = MSG_ReadChar(&net_message) * 0.25f;
        state->kick_angles[2] = MSG_ReadChar(&net_message) * 0.25f;
    }

    if (flags & PS_WEAPONINDEX)
        state->gunindex = MSG_ReadByte(&net_message);

    if (flags & PS_WEAPONFRAME){
        state->gunframe = MSG_ReadByte(&net_message);
        state->gunoffset[0] = MSG_ReadChar(&net_message) * 0.25f;
        state->gunoffset[1] = MSG_ReadChar(&net_message) * 0.25f;
        state->gunoffset[2] = MSG_ReadChar(&net_message) * 0.25f;
        state->gunangles[0] = MSG_ReadChar(&net_message) * 0.25f;
        state->gunangles[1] = MSG_ReadChar(&net_message) * 0.25f;
        state->gunangles[2] = MSG_ReadChar(&net_message) * 0.25f;
    }

    if (flags & PS_BLEND){
        state->blend[0] = MSG_ReadByte(&net_message) / 255.0f;
        state->blend[1] = MSG_ReadByte(&net_message) / 255.0f;
        state->blend[2] = MSG_ReadByte(&net_message) / 255.0f;
        state->blend[3] = MSG_ReadByte(&net_message) / 255.0f;
    }

    if (flags & PS_FOV)
        state->fov = MSG_ReadByte(&net_message);

    if (flags & PS_RDFLAGS)
        state->rdflags = MSG_ReadByte(&net_message);

    // Parse stats
    statBits = MSG_ReadLong(&net_message);

    for (i = 0; i < MAX_STATS; i++){
        if (statBits & (1 << i))
            state->stats[i] = MSG_ReadShort(&net_message);
	}
}

/*
 ==================
 CL_FootStepEffect
 ==================
*/
static void CL_FootStepEffect (entity_state_t *state){

	trace_t trace;
	vec3_t	end, dir;
	int		contents;

	VectorSet(end, state->origin[0], state->origin[1], state->origin[2] - 24.0f);

	// Not touching water
	contents = CL_PointContents(end, -1);
	if (!(contents & (CONTENTS_WATER)))
		return;

	// Find the surface
	trace = CL_Trace(state->origin, vec3_origin, vec3_origin, end, cl.clientNum, CONTENTS_WATER, true, NULL);
	if (trace.fraction == 1.0f)
		return;

	// Make the effect
	VectorSet(dir, 0.0f, 0.0f, 1.0f);
	CL_WaterSplash(trace.endpos, dir);
}

/*
 ==================
 CL_EntityEvent
 ==================
*/
static void CL_EntityEvent (){

	entity_state_t  *state;
    int				i;

	for (i = 0; i < cl.frame.numEntities; i++){
        state = &cl.parseEntities[(cl.frame.parseEntitiesIndex+i) & (MAX_PARSE_ENTITIES - 1)];

		// EF_TELEPORTER acts like an event, but is not cleared each
		// frame
		if (state->effects & EF_TELEPORTER)
			CL_TeleporterParticles(state->origin);

		if (!state->event)
			continue;

		if (cl_showEvents->integerValue){
			if (!cl_eventNames[state->event])
				Com_Printf("%4i: BAD EVENT %i\n", state->number, state->event);
			else
				Com_Printf("%4i: %s\n", state->number, cl_eventNames[state->event]);
		}

		switch (state->event){
		case EV_NONE:

			break;
		case EV_ITEM_RESPAWN:
//			S_PlaySound(NULL, state->number, CHAN_WEAPON, S_RegisterSound("items/respawn1.wav", 0), 1.0f, ATTN_IDLE, 0.0f);
			CL_ItemRespawnParticles(state->origin);

			break;
		case EV_FOOTSTEP:
//			if (cl_footSteps->integerValue)
//				S_PlayLocalSound(cl.media.footStepSoundShaders[rand() & 3]);

			CL_FootStepEffect(state);

			break;
		case EV_FALLSHORT:
//			S_PlaySound(NULL, state->number, CHAN_AUTO, S_RegisterSound("player/land1.wav", 0), 1.0f, ATTN_NORM, 0.0f);

			break;
		case EV_FALL:
//			S_PlaySound(NULL, state->number, CHAN_AUTO, S_RegisterSound("*fall2.wav", 0), 1.0f, ATTN_NORM, 0.0f);

			break;
		case EV_FALLFAR:
//			S_PlaySound(NULL, state->number, CHAN_AUTO, S_RegisterSound("*fall1.wav", 0), 1.0f, ATTN_NORM, 0.0f);

			break;
		case EV_PLAYER_TELEPORT:
//			S_PlaySound(NULL, state->number, CHAN_WEAPON, S_RegisterSound("misc/tele1.wav", 0), 1.0f, ATTN_IDLE, 0.0f);
			CL_TeleportParticles(state->origin);

			break;
		case EV_OTHER_TELEPORT:

			break;
		default:
			Com_Error(ERR_DROP, "CL_EntityEvent: bad event type (%i)", state->event);
		}
	}
}

/*
 ==================
 CL_ParseBaseLine
 ==================
*/
void CL_ParseBaseLine (){

	entity_state_t	*state, nullState;
	int				bits, number;

	Mem_Fill(&nullState, 0, sizeof(nullState));

	number = CL_ParseEntityBits(&bits);
	state = &cl.entities[number].baseline;

	CL_ParseDelta(&nullState, state, number, bits);
}
  
/*
 ==================
 CL_ParseFrame
 ==================
*/
void CL_ParseFrame (){

	frame_t	*oldFrame;
	int     cmd, length;
	short	delta[3];

	Mem_Fill(&cl.frame, 0, sizeof(cl.frame));

	cl.frame.serverFrame = MSG_ReadLong(&net_message);
	cl.frame.deltaFrame = MSG_ReadLong(&net_message);
	cl.frame.serverTime = cl.frame.serverFrame * 100;

	// BIG HACK to let old demos continue to work
	if (cls.serverProtocol != 26)
		cl.suppressCount = MSG_ReadByte(&net_message);

	CL_ShowNet(3, "Frame: %i   Delta: %i", cl.frame.serverFrame, cl.frame.deltaFrame);

	// If the frame is delta compressed from data that we no longer have
	// available, we must suck up the rest of the frame, but not use it,
	// then ask for a non-compressed message 
	if (cl.frame.deltaFrame <= 0){
		oldFrame = NULL;
		cl.frame.valid = true;		// Uncompressed frame
		cls.demoWaiting = false;	// We can start recording now
	}
	else {
		oldFrame = &cl.frames[cl.frame.deltaFrame & UPDATE_MASK];
		if (!oldFrame->valid)
			// Should never happen
			Com_DPrintf(S_COLOR_YELLOW "Delta from invalid frame (not supposed to happen!)\n");

		if (cl.frame.deltaFrame != oldFrame->serverFrame)
			Com_DPrintf(S_COLOR_YELLOW "Delta frame too old\n");
		else if (cl.parseEntitiesIndex - oldFrame->parseEntitiesIndex > MAX_PARSE_ENTITIES - 128)
			Com_DPrintf(S_COLOR_YELLOW "Delta parseEntitiesIndex too old\n");
		else
			cl.frame.valid = true;	// Valid delta parse
	}

	// Clamp time
	if (cl.time > cl.frame.serverTime)
		cl.time = cl.frame.serverTime;
	else if (cl.time < cl.frame.serverTime - 100)
		cl.time = cl.frame.serverTime - 100;

	// Read areabits
	length = MSG_ReadByte(&net_message);
	MSG_ReadData(&net_message, &cl.frame.areaBits, length);

	// Read player state
	cmd = MSG_ReadByte(&net_message);
	if (cmd != SVC_PLAYERINFO)
		Com_Error(ERR_DROP, "CL_ParseFrame: not player state");

	CL_ShowNet(2, "%3i: %s", net_message.readCount - 1, svc_strings[cmd]);

	CL_ParsePlayerState(oldFrame, &cl.frame);

	// Read packet entities
	cmd = MSG_ReadByte(&net_message);
	if (cmd != SVC_PACKETENTITIES)
		Com_Error(ERR_DROP, "CL_ParseFrame: not packet entities");

	CL_ShowNet(2, "%3i: %s", net_message.readCount - 1, svc_strings[cmd]);

	CL_ParsePacketEntities(oldFrame, &cl.frame);

	// Save the frame off in the backup array for later delta 
	// comparisons
	cl.frames[cl.frame.serverFrame & UPDATE_MASK] = cl.frame;

    // Find the previous frame to interpolate from
    oldFrame = &cl.frames[(cl.frame.serverFrame - 1) & UPDATE_MASK];
    if ((oldFrame->serverFrame != cl.frame.serverFrame -1) || !oldFrame->valid)
        oldFrame = &cl.frame;		// Previous frame was dropped or invalid

	// Find the previous player state to interpolate from
	cl.playerState = &cl.frame.playerState;
	cl.oldPlayerState = &oldFrame->playerState;

	// See if the player respawned this frame
	if (cl.playerState->stats[STAT_HEALTH] > 0 && cl.oldPlayerState->stats[STAT_HEALTH] <= 0){
		// Clear a few things
		cl.doubleVisionEndTime = 0;
		cl.underwaterVisionEndTime = 0;
		cl.fireScreenEndTime = 0;

		cl.crosshairEntTime = 0;
		cl.crosshairEntNumber = 0;
	}

    // See if the player entity was teleported this frame
	delta[0] = cl.oldPlayerState->pmove.origin[0] - cl.playerState->pmove.origin[0];
	delta[1] = cl.oldPlayerState->pmove.origin[1] - cl.playerState->pmove.origin[1];
	delta[2] = cl.oldPlayerState->pmove.origin[2] - cl.playerState->pmove.origin[2];

    if (abs(delta[0]) > 2048 || abs(delta[1]) > 2048 || abs(delta[2]) > 2048)
		cl.oldPlayerState = (player_state_t *)&cl.playerState;	// Don't interpolate

	if (!cl.frame.valid)
		return;

	// Getting a valid frame message ends the connection/loading process
	if (cls.state == CA_PRIMED){
		cls.state = CA_ACTIVE;
		cls.loading = false;

		cl.predictedOrigin[0] = cl.frame.playerState.pmove.origin[0] * 0.125f;
		cl.predictedOrigin[1] = cl.frame.playerState.pmove.origin[1] * 0.125f;
		cl.predictedOrigin[2] = cl.frame.playerState.pmove.origin[2] * 0.125f;

		VectorCopy(cl.frame.playerState.viewangles, cl.predictedAngles);
	}

	if (cls.state != CA_ACTIVE)
		return;

	CL_BuildSolidList();

	CL_EntityEvent();

	CL_CheckPredictionError();
}


/*
 ==============================================================================

 INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS

 ==============================================================================
*/


/*
 ==================
 CL_FlyEffect
 ==================
*/
static void CL_FlyEffect (centity_t *centity, const vec3_t origin){

	int		n, count, start;

	if (centity->flyStopTime < cl.time){
		start = cl.time;
		centity->flyStopTime = cl.time + 60000;
	}
	else
		start = centity->flyStopTime - 60000;

	n = cl.time - start;
	if (n < 20000)
		count = n * 162 / 20000.0f;
	else {
		n = centity->flyStopTime - cl.time;
		if (n < 20000)
			count = n * 162 / 20000.0f;
		else
			count = 162;
	}

	if (count > NUM_VERTEX_NORMALS)
		count = NUM_VERTEX_NORMALS;

	CL_FlyParticles(origin, count);
}

/*
 ==================
 CL_AddShellEntity
 ==================
*/
static void CL_AddShellEntity (renderEntity_t *entity, uint effects, bool light){

	vec3_t	rgb;

	if (!cl_drawShells->integerValue)
		return;

	if (!(effects & (EF_PENT|EF_QUAD|EF_DOUBLE|EF_HALF_DAMAGE|EF_COLOR_SHELL)))
		return;

	if (effects & EF_PENT){
		entity->material = cl.media.invulnerabilityShellMaterial;
		MakeRGBA(entity->materialParms, 1.0f, 0.0f, 0.0f, 0.3f);

		R_AddEntityToScene(entity);

		if (light)
			CL_DynamicLight(entity->origin, 200.0f + (rand() & 31), 1.0f, 0.0f, 0.0f, false, 0);
	}

	if (effects & EF_QUAD){
		entity->material = cl.media.quadDamageShellMaterial;
		MakeRGBA(entity->materialParms, 0.0f, 0.0f, 1.0f, 0.3f);

		R_AddEntityToScene(entity);

		if (light)
			CL_DynamicLight(entity->origin, 200.0f + (rand() & 31), 0.0f, 0.0f, 1.0f, false, 0);
	}

	if (effects & EF_DOUBLE){
		entity->material = cl.media.doubleDamageShellMaterial;
		MakeRGBA(entity->materialParms, 0.9f, 0.7f, 0.0f, 0.3f);

		R_AddEntityToScene(entity);

		if (light)
			CL_DynamicLight(entity->origin, 200.0f + (rand() & 31), 0.9f, 0.7f, 0.0f, false ,0);
	}

	if (effects & EF_HALF_DAMAGE){
		entity->material = cl.media.halfDamageShellMaterial;
		MakeRGBA(entity->materialParms, 0.55f, 0.6f, 0.45f, 0.3f);

		R_AddEntityToScene(entity);

		if (light)
			CL_DynamicLight(entity->origin, 200.0f + (rand() & 31), 0.56f, 0.59f, 0.45f, false ,0);
	}

	if (effects & EF_COLOR_SHELL){
		entity->material = cl.media.genericShellMaterial;
		MakeRGBA(entity->materialParms, 0.0f, 0.0f, 0.0f, 0.3f);

		VectorClear(rgb);

		if (entity->renderFX & RF_SHELL_RED){
			entity->materialParms[MATERIALPARM_RED] = 1.0f;
			rgb[0] = 1.0f;
		}
		if (entity->renderFX & RF_SHELL_GREEN){
			entity->materialParms[MATERIALPARM_GREEN] = 1.0f;
			rgb[1] = 1.0f;
		}
		if (entity->renderFX & RF_SHELL_BLUE){
			entity->materialParms[MATERIALPARM_BLUE] = 1.0f;
			rgb[2] = 1.0f;
		}

		R_AddEntityToScene(entity);

		if (light)
			CL_DynamicLight(entity->origin, 200.0f + (rand() & 31), rgb[0], rgb[1], rgb[2], false, 0);
	}

	// Make sure these get reset
	entity->material = NULL;
	MakeRGBA(entity->materialParms, 1.0f, 1.0f, 1.0f, 1.0f);
}

/*
 ==================
 CL_AddEntityTrails
 ==================
*/
static void CL_AddEntityTrails (centity_t *centity, renderEntity_t *entity, uint effects, bool viewer){

	if (effects & EF_ROCKET){
		CL_RocketTrail(centity->lerpOrigin, entity->origin);
		CL_DynamicLight(entity->origin, 200.0f, 1.0f, 1.0f, 0.0f, 0, false);
	}
	else if (effects & EF_BLASTER){
		if (effects & EF_TRACKER){
			CL_BlasterTrail(centity->lerpOrigin, entity->origin, 0.00f, 1.00f, 0.00f);
			CL_DynamicLight(entity->origin, 200.0f, 0.0f, 1.0f, 0.0f, false ,0);
		}
		else {
			CL_BlasterTrail(centity->lerpOrigin, entity->origin, 0.97f, 0.46f, 0.14f);
			CL_DynamicLight(entity->origin, 200.0f, 1.0f, 1.0f, 0.0f, false, 0);
		}
	}
	else if (effects & EF_HYPERBLASTER){
		if (effects & EF_TRACKER)
			CL_DynamicLight(entity->origin, 200.0f, 0.0f, 1.0f, 0.0f, false, 0);
		else
			CL_DynamicLight(entity->origin, 200.0f, 1.0f, 1.0f, 0.0f, false, 0);
	}
	else if (effects & EF_GIB)
		CL_BloodTrail(centity->lerpOrigin, entity->origin, false);
	else if (effects & EF_GREENGIB)
		CL_BloodTrail(centity->lerpOrigin, entity->origin, true);
	else if (effects & EF_GRENADE)
		CL_GrenadeTrail(centity->lerpOrigin, entity->origin);
	else if (effects & EF_FLIES)
		CL_FlyEffect(centity, entity->origin);
	else if (effects & EF_TRAP){
		CL_TrapParticles(entity->origin);
		CL_DynamicLight(entity->origin, 100.0f + (rand() % 100), 1.0f, 0.8f, 0.1f, false, 0);
	}
	else if (effects & EF_FLAG1){
		if (!viewer)
			CL_FlagTrail(centity->lerpOrigin, entity->origin, 1.0f, 0.1f, 0.1f);

		CL_DynamicLight(entity->origin, 225.0f, 1.0f, 0.1f, 0.1f, false, 0);
	}
	else if (effects & EF_FLAG2){
		if (!viewer)
			CL_FlagTrail(centity->lerpOrigin, entity->origin, 0.1f, 0.1f, 1.0f);

		CL_DynamicLight(entity->origin, 225.0f, 0.1f, 0.1f, 1.0f, false, 0);
	}
	else if (effects & EF_TAGTRAIL){
		if (!viewer)
			CL_TagTrail(centity->lerpOrigin, entity->origin);

		CL_DynamicLight(entity->origin, 225.0f, 1.0f, 1.0f, 0.0f, false, 0);
	}
	else if (effects & EF_TRACKERTRAIL){
		if (viewer)
			CL_DynamicLight(entity->origin, 255.0f, -1.0f, -1.0f, -1.0f, false, 0);
		else {
			if (effects & EF_TRACKER)
				CL_DynamicLight(entity->origin, 50.0f + (500.0f * (sin(cl.time / 500.0f) + 1.0f)), -1.0f, -1.0f, -1.0f, false, 0);
			else {
				CL_TrackerShellParticles(centity->lerpOrigin);
				CL_DynamicLight(entity->origin, 155.0f, -1.0f, -1.0f, -1.0f, false, 0);
			}
		}
	}
	else if (effects & EF_TRACKER){
		CL_TrackerTrail(centity->lerpOrigin, entity->origin);
		CL_DynamicLight(entity->origin, 200.0f, -1.0f, -1.0f, -1.0f, false, 0);
	}
	else if (effects & EF_IONRIPPER)
		CL_DynamicLight(entity->origin, 100.0f, 1.0f, 0.5f, 0.5f, false, 0);
	else if (effects & EF_BLUEHYPERBLASTER)
		CL_DynamicLight(entity->origin, 200.0f, 0.0f, 0.0f, 1.0f, false, 0);
}

/*
 ==================
 CL_AddPacketEntities
 ==================
*/
void CL_AddPacketEntities (){

	renderEntity_t	entity;
	entity_state_t	*state;
	centity_t		*centity;
	clientInfo_t    *clientInfo;
	vec3_t			origin, angles, autoRotateAxis[3];
	int				animAll, animAllFast, anim01, anim23;
	bool			isClient;
	uint			delta;
	float			scale;
	int				weapon;
	int				i;

	// Some items auto-rotate
	VectorSet(angles, 0.0f, AngleMod(cl.time * 0.1f), 0.0f);
	AnglesToMat3(angles, autoRotateAxis);

	// Brush models can auto-animate their frames
	animAll = 2 * cl.time / 1000;
	animAllFast = cl.time / 100;
	anim01 = (animAll & 1);
	anim23 = (animAll & 1) + 2;

	Mem_Fill(&entity, 0, sizeof(renderEntity_t));

	for (i = 0; i < cl.frame.numEntities; i++){
		state = &cl.parseEntities[(cl.frame.parseEntitiesIndex + i) & (MAX_PARSE_ENTITIES - 1)];

		centity = &cl.entities[state->number];

		isClient = state->number == cl.clientNum;

		// Add laser beams
		if (state->renderfx & RF_BEAM){
			CL_LaserBeam(centity->current.origin, centity->current.old_origin, state->frame, state->skinnum, 75, 1, cl.media.laserBeamMaterial);

			VectorCopy(centity->current.origin, centity->lerpOrigin);
			continue;
		}

		// Interpolate origin
		if (isClient && !cl_thirdPerson->integerValue && cl_predict->integerValue && !(cl.playerState->pmove.pm_flags & PMF_NO_PREDICTION)){
			// Use predicted values
			VectorMA(cl.predictedOrigin, -(1.0 - cl.lerpFrac), cl.predictedError, entity.origin);

	        // Smooth out stair climbing
			delta = cls.realTime - cl.predictedStepTime;
			if (delta < 100)
				entity.origin[2] -= cl.predictedStep * (100 - delta) * 0.01f;
		}
		else {
			if (state->renderfx & RF_FRAMELERP)
				VectorLerp(centity->current.old_origin, centity->current.origin, cl.lerpFrac, entity.origin);
			else
				VectorLerp(centity->prev.origin, centity->current.origin, cl.lerpFrac, entity.origin);
		}

		// BFG and Phalanx effects are just sprites
		if (state->effects & EF_BFG){
			if (state->effects & EF_ANIM_ALLFAST){
				CL_Sprite(entity.origin, 40.0f, cl.media.bfgBallMaterial);
				CL_BFGTrail(centity->lerpOrigin, entity.origin);
				CL_DynamicLight(entity.origin, 200.0f, 0.0f, 1.0f, 0.0f, false, 0);
			}

			VectorCopy(entity.origin, centity->lerpOrigin);
			continue;
		}
		if (state->effects & EF_PLASMA){
			if (state->effects & EF_ANIM_ALLFAST){
				CL_Sprite(entity.origin, 25.0f, cl.media.plasmaBallMaterial);
				CL_BlasterTrail(centity->lerpOrigin, entity.origin, 0.97f, 0.46f, 0.14f);
				CL_DynamicLight(entity.origin, 130.0f, 1.0f, 0.5f, 0.5f, false, 0);
			}

			VectorCopy(entity.origin, centity->lerpOrigin);
			continue;
		}

		// Calculate angles
		if (state->effects & EF_ROTATE){
			Matrix3_Copy(autoRotateAxis, entity.axis);

			if (cl_itemBob->integerValue){
				scale = 0.005f + centity->current.number * 0.00001f;
				entity.origin[2] += 4 + cos((cl.time + 1000) *  scale) * 4;
			}
		}
		else if (state->effects & EF_SPINNINGLIGHTS){
			VectorSet(angles, 0.0f, AngleMod(cl.time * 0.5f) + state->angles[1], 180.0f);
			AnglesToMat3(angles, entity.axis);

			VectorMA(entity.origin, 64.0f, entity.axis[0], origin);
			CL_DynamicLight(origin, 100.0f, 1.0f, 0.0f, 0.0f, false, 0);
		}
		else {
			// Interpolate angles
			LerpAngles(centity->prev.angles, centity->current.angles, cl.lerpFrac, angles);
			AnglesToMat3(angles, entity.axis);
		}

		// If set to invisible, skip
		if (!state->modelindex){
			VectorCopy(entity.origin, centity->lerpOrigin);
			continue;
		}

		// Create a new entity
		entity.type = RE_MODEL;

		// Set model and skin
		if (state->modelindex == 255){
			// Use custom player skin
			clientInfo = &cl.clientInfo[state->skinnum & 0xFF];
			if (!clientInfo->valid)
				clientInfo = &cl.baseClientInfo;

			entity.skinIndex = 0;
			entity.model = clientInfo->model;
			entity.material = clientInfo->skin;

			if (state->renderfx & RF_USE_DISGUISE){
				if (!Str_ICompareChars(clientInfo->info, "male", 4))
					entity.material = R_RegisterMaterial("players/male/disguise", true);
				else if (!Str_ICompareChars(clientInfo->info, "female", 6))
					entity.material = R_RegisterMaterial("players/female/disguise", true);
				else if (!Str_ICompareChars(clientInfo->info, "cyborg", 6))
					entity.material = R_RegisterMaterial("players/cyborg/disguise", true);
			}
		}
		else {
			entity.skinIndex = state->skinnum;
			entity.model = cl.media.gameModels[state->modelindex];
			entity.material = NULL;
		}

		// Set frame
		if (state->effects & EF_ANIM_ALL)
			entity.frame = animAll;
		else if (state->effects & EF_ANIM_ALLFAST)
			entity.frame = animAllFast;
		else if (state->effects & EF_ANIM01)
			entity.frame = anim01;
		else if (state->effects & EF_ANIM23)
			entity.frame = anim23;
		else
			entity.frame = state->frame;

		entity.oldFrame = centity->prev.frame;
		entity.backLerp = 1.0f - cl.lerpFrac;

		entity.materialParms[MATERIALPARM_RED] = 1.0f;
		entity.materialParms[MATERIALPARM_GREEN] = 1.0f;
		entity.materialParms[MATERIALPARM_BLUE] = 1.0f;
		entity.materialParms[MATERIALPARM_ALPHA] = 1.0f;
		entity.materialParms[MATERIALPARM_TIMEOFFSET] = -MS2SEC(centity->flashStartTime);
		entity.materialParms[MATERIALPARM_DIVERSITY] = centity->flashRotation;
		entity.materialParms[MATERIALPARM_MISC] = entity.frame;
		entity.materialParms[MATERIALPARM_MODE] = 0.0f;

		// Only used for black hole model
		if (state->renderfx == RF_TRANSLUCENT)
			entity.materialParms[MATERIALPARM_ALPHA] = 0.7f;

		if (state->effects & EF_SPHERETRANS){
			if (state->effects & EF_TRACKERTRAIL)
				entity.materialParms[MATERIALPARM_ALPHA] = 0.6f;
			else
				entity.materialParms[MATERIALPARM_ALPHA] = 0.3f;
		}

		// Render effects
		entity.renderFX = state->renderfx;

		if (isClient && !cl_thirdPerson->integerValue)
			entity.renderFX |= RF_VIEWERMODEL;		// Only draw from mirrors

		// Add to refresh list
		R_AddEntityToScene(&entity);

		// Color shells generate a separate entity for the main model
		CL_AddShellEntity(&entity, state->effects, !(entity.renderFX & RF_VIEWERMODEL));

		// Make sure these get reset
		entity.skinIndex = 0;
		entity.material = NULL;

		MakeRGBA(entity.materialParms, 1.0f, 1.0f, 1.0f, 1.0f);

		// Duplicate for linked models
		if (state->modelindex2){
			if (state->modelindex2 == 255){
				// Use custom weapon
				clientInfo = &cl.clientInfo[state->skinnum & 0xFF];
				if (!clientInfo->valid)
					clientInfo = &cl.baseClientInfo;

				weapon = state->skinnum >> 8;
				if ((weapon < 0 || weapon >= MAX_CLIENTWEAPONMODELS) || !cl_visibleWeapons->integerValue)
					weapon = 0;

				if (clientInfo->weaponModel[weapon])
					entity.model = clientInfo->weaponModel[weapon];
				else
					entity.model = clientInfo->weaponModel[0];
			}
			else {
				entity.model = cl.media.gameModels[state->modelindex2];

				// HACK: check for the defender sphere shell. Make it
				// translucent.
				if (!Str_ICompare(cl.configStrings[CS_MODELS + state->modelindex2], "models/items/shell/tris.md2"))
					entity.materialParms[MATERIALPARM_ALPHA] = 0.3f;
			}

			R_AddEntityToScene(&entity);

			// Color shells generate a separate entity for the main model
			CL_AddShellEntity(&entity, state->effects, false);

			// Make sure this gets reset
            entity.materialParms[MATERIALPARM_ALPHA] = 1.0f;
		}
        if (state->modelindex3){
			entity.model = cl.media.gameModels[state->modelindex3];
            R_AddEntityToScene(&entity);

			// Color shells generate a separate entity for the main model
			CL_AddShellEntity(&entity, state->effects, false);
		}
        if (state->modelindex4){
			entity.model = cl.media.gameModels[state->modelindex4];
            R_AddEntityToScene(&entity);

			// Color shells generate a separate entity for the main model
			CL_AddShellEntity(&entity, state->effects, false);
		}

		// Power screen
        if (state->effects & EF_POWERSCREEN){
			entity.model = cl.media.powerScreenShellModel;
            entity.frame = 0;
            entity.oldFrame = 0;
			entity.material = cl.media.powerScreenShellMaterial;
			MakeRGBA(entity.materialParms, 0.0f, 1.0f, 0.0f, 0.3f);

            R_AddEntityToScene(&entity);

			CL_DynamicLight(entity.origin, 250.0f, 0.0f, 1.0f, 0.0f, false, 0);
		}

        // Add automatic trails
        if (state->effects & ~EF_ROTATE)
			CL_AddEntityTrails(centity, &entity, state->effects, (entity.renderFX & RF_VIEWERMODEL));

		VectorCopy(entity.origin, centity->lerpOrigin);
    }
}

/*
 ==================
 CL_AddViewWeapon
 ==================
*/
void CL_AddViewWeapon (){

    renderEntity_t	gun;
	centity_t		*centity;
	entity_state_t	*state;
	vec3_t			angles;

	// Don't add if in third person view
	if (cl_thirdPerson->integerValue)
		return;

    // Allow the gun to be completely removed
    if (!cl_drawGun->integerValue || cl_hand->integerValue == 2)
        return;

	// Don't add if in wide angle view
	if (cl.playerState->fov > DEFAULT_FOV)
		return;

	// Don't add if testing a gun model
	if (cl.testModel.isGun)
		return;

    if (!cl.media.gameModels[cl.playerState->gunindex])
        return;

	state = &cl.entities[cl.clientNum].current;

	centity = &cl.entities[cl.clientNum];

	Mem_Fill(&gun, 0, sizeof(renderEntity_t));

	gun.type = RE_MODEL;
	gun.renderFX = RF_WEAPONMODEL;
	gun.depthHack = true;
	gun.model = cl.media.gameModels[cl.playerState->gunindex];
	gun.backLerp = 1.0f - cl.lerpFrac;

	gun.materialParms[MATERIALPARM_RED] = 1.0f;
	gun.materialParms[MATERIALPARM_GREEN] = 1.0f;
	gun.materialParms[MATERIALPARM_BLUE] = 1.0f;
	gun.materialParms[MATERIALPARM_ALPHA] = 1.0f;
	gun.materialParms[MATERIALPARM_TIMEOFFSET] = -MS2SEC(centity->flashStartTime);
	gun.materialParms[MATERIALPARM_DIVERSITY] = centity->flashRotation;
	gun.materialParms[MATERIALPARM_MISC] = cl.playerState->stats[STAT_AMMO];
	gun.materialParms[MATERIALPARM_MODE] = 0.0f;

    // Set up gun position and angles
	VectorLerp(cl.oldPlayerState->gunoffset, cl.playerState->gunoffset, cl.lerpFrac, gun.origin);
	VectorAdd(gun.origin, cl.renderView.origin, gun.origin);

	LerpAngles(cl.oldPlayerState->gunangles, cl.playerState->gunangles, cl.lerpFrac, angles);
	VectorAdd(angles, cl.renderViewAngles, angles);

	AnglesToMat3(angles, gun.axis);

	// Set up gun frames
	gun.frame = cl.playerState->gunframe;
	gun.oldFrame = cl.oldPlayerState->gunframe;

	if (gun.frame == 0)
		gun.oldFrame = 0;	// Just changed weapons, don't lerp from old

    R_AddEntityToScene(&gun);

	// Color shells generate a separate entity for the main model
	if (cl_drawShells->integerValue){
		gun.renderFX |= state->renderfx;

		if (cl_drawGodModeShell->integerValue){
			if (state->effects & EF_COLOR_SHELL){
				// Remove godmode for weapon
				if ((gun.renderFX & RF_SHELL_RED) && (gun.renderFX & RF_SHELL_GREEN) && (gun.renderFX & RF_SHELL_BLUE)){
					CL_AddShellEntity(&gun, (state->effects & ~EF_COLOR_SHELL), true);
					return;
				}
			}
		}

		CL_AddShellEntity(&gun, state->effects, true);
	}
}

/*
 ==================
 CL_GetEntitySoundSpatialization

 Called by the sound system to get the sound spatialization origin and
 velocity for the given entity
 ==================
*/
void CL_GetEntitySoundSpatialization (int entity, vec3_t origin, vec3_t velocity){

	centity_t			*centity;
	clipInlineModel_t	*model;
	vec3_t				midPoint;

	if (entity < 0 || entity >= MAX_EDICTS)
		Com_Error(ERR_DROP, "CL_GetEntitySoundSpatialization: entity = %i", entity);

	centity = &cl.entities[entity];

	if (centity->current.renderfx & (RF_FRAMELERP|RF_BEAM)){
		// Calculate origin
		origin[0] = centity->current.old_origin[0] + (centity->current.origin[0] - centity->current.old_origin[0]) * cl.lerpFrac;
		origin[1] = centity->current.old_origin[1] + (centity->current.origin[1] - centity->current.old_origin[1]) * cl.lerpFrac;
		origin[2] = centity->current.old_origin[2] + (centity->current.origin[2] - centity->current.old_origin[2]) * cl.lerpFrac;

		// Calculate velocity
		VectorSubtract(centity->current.origin, centity->current.old_origin, velocity);
		VectorScale(velocity, 10.0f, velocity);
	}
	else {
		// Calculate origin
		origin[0] = centity->prev.origin[0] + (centity->current.origin[0] - centity->prev.origin[0]) * cl.lerpFrac;
		origin[1] = centity->prev.origin[1] + (centity->current.origin[1] - centity->prev.origin[1]) * cl.lerpFrac;
		origin[2] = centity->prev.origin[2] + (centity->current.origin[2] - centity->prev.origin[2]) * cl.lerpFrac;

		// Calculate velocity
		VectorSubtract(centity->current.origin, centity->prev.origin, velocity);
		VectorScale(velocity, 10.0f, velocity);
	}

	// If a brush model, offset the origin
	if (centity->current.solid == 31){
		model = cl.media.gameCModels[centity->current.modelindex];
		if (!model)
			return;

		VectorAverage(model->mins, model->maxs, midPoint);
		VectorAdd(origin, midPoint, origin);
	}
}