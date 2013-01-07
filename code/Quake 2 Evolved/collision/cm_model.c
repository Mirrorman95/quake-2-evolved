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
// cm_model.c - BSP model collision
//


#include "cm_local.h"


static clipBrush_t *		cm_boxBrush;
static clipLeaf_t *			cm_boxLeaf;
static cplane_t *			cm_boxPlanes;
int							cm_boxHeadNode;


/*
 ==================
 CM_LoadPlanes
 ==================
*/
static void CM_LoadPlanes (const byte *data, const bspLump_t *lump){

	bspPlane_t	*in;
	cplane_t	*out;
	int			i;

	in = (bspPlane_t *)(data + lump->offset);
	if (lump->length % sizeof(bspPlane_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size (%s)", cm.name);

	cm.numPlanes = lump->length / sizeof(bspPlane_t);
	if (cm.numPlanes < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: no planes (%s)", cm.name);
	if (cm.numPlanes > MAX_MAP_PLANES)
		Com_Error(ERR_DROP, "CM_LoadMap: too many planes (%s)", cm.name);

	// Extra for box hull
	cm.planes = out = (cplane_t *)Mem_Alloc((cm.numPlanes + 12) * sizeof(cplane_t), TAG_COLLISION);
	cm.size += cm.numPlanes * sizeof(cplane_t);

	for (i = 0; i < cm.numPlanes; i++, in++, out++){
		out->normal[0] = LittleFloat(in->normal[0]);
		out->normal[1] = LittleFloat(in->normal[1]);
		out->normal[2] = LittleFloat(in->normal[2]);

		out->dist = LittleFloat(in->dist);
		out->type = PlaneTypeForNormal(out->normal);
		SetPlaneSignbits(out);
	}
}

/*
 ==================
 CM_LoadSurfaces
 ==================
*/
static void CM_LoadSurfaces (const byte *data, const bspLump_t *lump){

	bspTexInfo_t	*in;
	clipSurface_t	*out;
	int				i;

	in = (bspTexInfo_t *)(data + lump->offset);
	if (lump->length % sizeof(bspTexInfo_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size (%s)", cm.name);

	cm.numSurfaces = lump->length / sizeof(bspTexInfo_t);
	if (cm.numSurfaces < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: no surfaces (%s)", cm.name);
	if (cm.numSurfaces > MAX_MAP_TEXINFO)
		Com_Error(ERR_DROP, "CM_LoadMap: too many surfaces (%s)", cm.name);

	cm.surfaces = out = (clipSurface_t *)Mem_Alloc(cm.numSurfaces * sizeof(clipSurface_t), TAG_COLLISION);
	cm.size += cm.numPlanes * sizeof(clipSurface_t);

	for (i = 0; i < cm.numSurfaces; i++, in++, out++){
		Str_Copy(out->name, in->texture, sizeof(out->name));
		Str_Copy(out->c.name, in->texture, sizeof(out->c.name));
		out->c.flags = LittleLong(in->flags);
		out->c.value = LittleLong(in->value);
	}
}

/*
 ==================
 CM_LoadVisibility
 ==================
*/
static void CM_LoadVisibility (const byte *data, const bspLump_t *lump){

	int		i;
	
	cm.numVisibility = lump->length;
	if (cm.numVisibility < 1)
		return;
	if (cm.numVisibility > MAX_MAP_VISIBILITY)
		Com_Error(ERR_DROP, "CM_LoadMap: too large visibility lump (%s)", cm.name);

	cm.visibility = (clipVis_t *)Mem_Alloc(cm.numVisibility, TAG_COLLISION);
	Mem_Copy(cm.visibility, data + lump->offset, cm.numVisibility);
	cm.size += cm.numVisibility * sizeof(clipVis_t);

	cm.visibility->numClusters = LittleLong(cm.visibility->numClusters);
	for (i = 0; i < cm.visibility->numClusters; i++){
		cm.visibility->bitOfs[i][0] = LittleLong(cm.visibility->bitOfs[i][0]);
		cm.visibility->bitOfs[i][1] = LittleLong(cm.visibility->bitOfs[i][1]);
	}
}

/*
 ==================
 CM_LoadLeafs
 ==================
*/
static void CM_LoadLeafs (const byte *data, const bspLump_t *lump){

	bspLeaf_t	*in;
	clipLeaf_t	*out;
	int			i;
	
	in = (bspLeaf_t *)(data + lump->offset);
	if (lump->length % sizeof(bspLeaf_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size (%s)", cm.name);
	
	cm.numLeafs = lump->length / sizeof(bspLeaf_t);
	if (cm.numLeafs < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: no leafs (%s)", cm.name);
	if (cm.numLeafs > MAX_MAP_LEAFS)
		Com_Error(ERR_DROP, "CM_LoadMap: too many leafs (%s)", cm.name);

	// Extra for box hull
	cm.leafs = out = (clipLeaf_t *)Mem_Alloc((cm.numLeafs + 1) * sizeof(clipLeaf_t), TAG_COLLISION);
	cm.size += cm.numLeafs * sizeof(clipLeaf_t);

	cm.numClusters = 0;
	for (i = 0; i < cm.numLeafs; i++, in++, out++){
		out->contents = LittleLong(in->contents);
		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);
		out->firstLeafBrush = LittleShort(in->firstLeafBrush);
		out->numLeafBrushes = LittleShort(in->numLeafBrushes);

		if (out->cluster >= cm.numClusters)
			cm.numClusters = out->cluster + 1;
	}

	if (cm.leafs[0].contents != CONTENTS_SOLID)
		Com_Error(ERR_DROP, "CM_LoadMap: leaf 0 is not CONTENTS_SOLID in '%s'", cm.name);
}

/*
 ==================
 CM_LoadLeafBrushes
 ==================
*/
static void CM_LoadLeafBrushes (const byte *data, const bspLump_t *lump){

	ushort 	*in;
	ushort	*out;
	int		i;
	
	in = (ushort *)(data + lump->offset);
	if (lump->length % sizeof(ushort))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size (%s)", cm.name);

	cm.numLeafBrushes = lump->length / sizeof(ushort);
	if (cm.numLeafBrushes < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: no leaf brushes (%s)", cm.name);
	if (cm.numLeafBrushes > MAX_MAP_LEAFBRUSHES)
		Com_Error(ERR_DROP, "CM_LoadMap: too many leaf brushes (%s)", cm.name);

	// Extra for box hull
	cm.leafBrushes = out = (ushort *)Mem_Alloc((cm.numLeafBrushes + 1) * sizeof(ushort), TAG_COLLISION);
	cm.size += cm.numLeafBrushes * sizeof(ushort);

	for (i = 0; i < cm.numLeafBrushes; i++, in++, out++)
		*out = LittleShort(*in);
}

/*
 ==================
 CM_LoadBrushes
 ==================
*/
static void CM_LoadBrushes (const byte *data, const bspLump_t *lump){

	bspBrush_t	*in;
	clipBrush_t	*out;
	int			i;
	
	in = (bspBrush_t *)(data + lump->offset);
	if (lump->length % sizeof(bspBrush_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size (%s)", cm.name);
	
	cm.numBrushes = lump->length / sizeof(bspBrush_t);
	if (cm.numBrushes < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: no brushes (%s)", cm.name);
	if (cm.numBrushes > MAX_MAP_BRUSHES)
		Com_Error(ERR_DROP, "CM_LoadMap: too many brushes (%s)", cm.name);

	// Extra for box hull
	cm.brushes = out = (clipBrush_t *)Mem_Alloc((cm.numBrushes + 1) * sizeof(clipBrush_t), TAG_COLLISION);
	cm.size += cm.numBrushes * sizeof(clipBrush_t);

	for (i = 0; i < cm.numBrushes; i++, out++, in++){
		out->firstBrushSide = LittleLong(in->firstSide);
		out->numSides = LittleLong(in->numSides);
		out->contents = LittleLong(in->contents);
	}
}

/*
 ==================
 CM_LoadBrushSides
 ==================
*/
static void CM_LoadBrushSides (const byte *data, const bspLump_t *lump){

	bspBrushSide_t	*in;
	clipBrushSide_t	*out;
	int				i;

	in = (bspBrushSide_t *)(data + lump->offset);
	if (lump->length % sizeof(bspBrushSide_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size (%s)", cm.name);

	cm.numBrushSides = lump->length / sizeof(bspBrushSide_t);
	if (cm.numBrushSides < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: no brush sides (%s)", cm.name);
	if (cm.numBrushSides > MAX_MAP_BRUSHSIDES)
		Com_Error(ERR_DROP, "CM_LoadMap: too many brush sides (%s)", cm.name);

	// Extra for box hull
	cm.brushSides = out = (clipBrushSide_t *)Mem_Alloc((cm.numBrushSides + 6) * sizeof(clipBrushSide_t), TAG_COLLISION);
	cm.size += cm.numBrushSides * sizeof(clipBrushSide_t);

	for (i = 0; i < cm.numBrushSides; i++, in++, out++){
		out->plane = cm.planes + LittleShort(in->planeNum);
		out->surface = cm.surfaces + LittleShort(in->texInfo);
	}
}

/*
 ==================
 CM_LoadNodes
 ==================
*/
static void CM_LoadNodes (const byte *data, const bspLump_t *lump){

	bspNode_t	*in;
	clipNode_t	*out;
	int			i;
	
	in = (bspNode_t *)(data + lump->offset);
	if (lump->length % sizeof(bspNode_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size (%s)", cm.name);
	
	cm.numNodes = lump->length / sizeof(bspNode_t);
	if (cm.numNodes < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: no nodes (%s)", cm.name);
	if (cm.numNodes > MAX_MAP_NODES)
		Com_Error(ERR_DROP, "CM_LoadMap: too many nodes (%s)", cm.name);

	// Extra for box hull
	cm.nodes = out = (clipNode_t *)Mem_Alloc((cm.numNodes + 6) * sizeof(clipNode_t), TAG_COLLISION);
	cm.size += cm.numNodes * sizeof(clipNode_t);

	for (i = 0; i < cm.numNodes; i++, out++, in++){
		out->plane = cm.planes + LittleLong(in->planeNum);

		out->children[0] = LittleLong(in->children[0]);
		out->children[1] = LittleLong(in->children[1]);
	}
}

/*
 ==================
 CM_LoadInlineModels
 ==================
*/
static void CM_LoadInlineModels (const byte *data, const bspLump_t *lump){

	bspInlineModel_t	*in;
	clipInlineModel_t	*out;
	int					i, j;

	in = (bspInlineModel_t *)(data + lump->offset);
	if (lump->length % sizeof(bspInlineModel_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size (%s)", cm.name);

	cm.numModels = lump->length / sizeof(bspInlineModel_t);
	if (cm.numModels < 1)
		Com_Error(ERR_DROP, "CM_LoadMap: no inline models (%s)", cm.name);
	if (cm.numModels > MAX_MAP_MODELS)
		Com_Error(ERR_DROP, "CM_LoadMap: too many inline models (%s)", cm.name);

	cm.models = out = (clipInlineModel_t *)Mem_Alloc(cm.numModels * sizeof(clipInlineModel_t), TAG_COLLISION);
	cm.size += cm.numModels * sizeof(clipInlineModel_t);

	for (i = 0; i < cm.numModels; i++, in++, out++){
		for (j = 0; j < 3; j++){
			// Spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;

			out->origin[j] = LittleFloat(in->origin[j]);
		}

		out->headNode = LittleLong(in->headNode);
	}
}

/*
 ==================
 CM_LoadAreas
 ==================
*/
static void CM_LoadAreas (const byte *data, const bspLump_t *lump){

	bspArea_t	*in;
	clipArea_t	*out;
	int			i;

	in = (bspArea_t *)(data + lump->offset);
	if (lump->length % sizeof(bspArea_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size (%s)", cm.name);

	cm.numAreas = lump->length / sizeof(bspArea_t);
	if (cm.numAreas < 1)
		return;
	if (cm.numAreas > BSP_MAX_AREAS)
		Com_Error(ERR_DROP, "CM_LoadMap: too many areas (%s)", cm.name);

	cm.areas = out = (clipArea_t *)Mem_Alloc(cm.numAreas * sizeof(clipArea_t), TAG_COLLISION);
	cm.size += cm.numAreas * sizeof(clipArea_t);

	for (i = 0; i < cm.numAreas; i++, in++, out++){
		out->numAreaPortals = LittleLong(in->numAreaPortals);
		out->firstAreaPortal = LittleLong(in->firstAreaPortal);
		out->floodValid = 0;
		out->floodNum = 0;
	}
}

/*
 ==================
 CM_LoadAreaPortals
 ==================
*/
static void CM_LoadAreaPortals (const byte *data, const bspLump_t *lump){

	bspAreaPortal_t		*in;
	clipAreaPortal_t	*out;
	int					i;

	in = (bspAreaPortal_t *)(data + lump->offset);
	if (lump->length % sizeof(bspAreaPortal_t))
		Com_Error(ERR_DROP, "CM_LoadMap: funny lump size (%s)", cm.name);

	cm.numAreaPortals = lump->length / sizeof(bspAreaPortal_t);
	if (cm.numAreaPortals < 1)
		return;
	if (cm.numAreaPortals > MAX_MAP_AREAPORTALS)
		Com_Error(ERR_DROP, "CM_LoadMap: too many area portals (%s)", cm.name);

	cm.areaPortals = out = (clipAreaPortal_t *)Mem_Alloc(cm.numAreaPortals * sizeof(clipAreaPortal_t), TAG_COLLISION);
	cm.size += cm.numAreaPortals * sizeof(clipAreaPortal_t);

	for (i = 0; i < cm.numAreaPortals; i++, in++, out++){
		out->portalNum = LittleLong(in->portalNum);
		out->otherArea = LittleLong(in->otherArea);
	}
}

/*
 ==================
 CM_LoadEntityString
 ==================
*/
static void CM_LoadEntityString (const byte *data, const bspLump_t *lump){

	cm.numEntityChars = lump->length;
	if (cm.numEntityChars < 1)
		return;
	if (cm.numEntityChars > MAX_MAP_ENTSTRING)
		Com_Error(ERR_DROP, "CM_LoadMap: too large entity lump (%s)", cm.name);

	cm.entityString = (char *)Mem_Alloc(cm.numEntityChars + 1, TAG_COLLISION);
	Mem_Copy(cm.entityString, data + lump->offset, cm.numEntityChars);
	cm.size += cm.numEntityChars * sizeof(char);
}

/*
 ==================
 CM_LoadMap
 ==================
*/
clipInlineModel_t *CM_LoadMap (const char *name, bool clientLoad, uint *checkCount){

	bspHeader_t	*header;
	byte		*data;
	int			i, length;

	// Cinematic servers won't have anything at all
	if (!name){
		CM_FreeMap();

		*checkCount = 0;
		return &cm.nullModel;
	}

	Com_DPrintf("CM_LoadMap( %s, %i )\n", name, clientLoad);

	if (!Str_ICompare(cm.name, name) && clientLoad && Com_ServerState()){
		// Still have the right version
		*checkCount = cm.checkCount;
		return &cm.models[0];
	}

	// Free old stuff
	CM_FreeMap();

	// Load the file
	length = FS_ReadFile(name, (void **)&data);
	if (!data)
		Com_Error(ERR_DROP, "CM_LoadMap: '%s' not found", name);

	// Fill it in
	cm.loaded = true;

	Str_Copy(cm.name, name, sizeof(cm.name));
	cm.size = 0;

	cm.checkCount = LittleLong(MD4_BlockChecksum(data, length));

	// Byte swap the header fields and sanity check
	header = (bspHeader_t *)data;

	for (i = 0; i < sizeof(bspHeader_t) / 4; i++)
		((int *)header)[i] = LittleLong(((int *)header)[i]);

	if (header->id != BSP_ID)
		Com_Error(ERR_DROP, "CM_LoadMap: wrong file id (%s)", cm.name);

	if (header->version != BSP_VERSION)
		Com_Error(ERR_DROP, "CM_LoadMap: wrong version number (%i should be %i) (%s)", header->version, BSP_VERSION, cm.name);

	// Load the lumps
	CM_LoadPlanes(data, &header->lumps[LUMP_PLANES]);
	CM_LoadSurfaces(data, &header->lumps[LUMP_TEXINFO]);
	CM_LoadVisibility(data, &header->lumps[LUMP_VISIBILITY]);
	CM_LoadLeafs(data, &header->lumps[LUMP_LEAFS]);
	CM_LoadLeafBrushes(data, &header->lumps[LUMP_LEAFBRUSHES]);
	CM_LoadBrushes(data, &header->lumps[LUMP_BRUSHES]);
	CM_LoadBrushSides(data, &header->lumps[LUMP_BRUSHSIDES]);
	CM_LoadNodes(data, &header->lumps[LUMP_NODES]);
	CM_LoadInlineModels(data, &header->lumps[LUMP_INLINEMODELS]);
	CM_LoadAreas(data, &header->lumps[LUMP_AREAS]);
	CM_LoadAreaPortals(data, &header->lumps[LUMP_AREAPORTALS]);
	CM_LoadEntityString(data, &header->lumps[LUMP_ENTITIES]);

	// Free file data
	FS_FreeFile(data);

	// Create the box model
	CM_CreateBoxModel();

	// All floods are initially invalid
	CM_FloodAreaConnections(true);

	// Clear counter
	*checkCount = cm.checkCount;

	return &cm.models[0];
}

/*
 ==================
 
 ==================
*/
void CM_FreeMap (){

	cm.numPlanes = 0;
	cm.numSurfaces = 0;
	cm.numVisibility = 0;
	cm.numLeafs = 1;
	cm.numClusters = 1;
	cm.numLeafBrushes = 0;
	cm.numBrushes = 0;
	cm.numBrushSides = 0;
	cm.numNodes = 0;
	cm.numModels = 0;
	cm.numAreas = 1;
	cm.numAreaPortals = 0;
	cm.numEntityChars = 0;

	cm.name[0] = 0;
	cm.loaded = false;
	cm.checkCount = 0;
}


// ============================================================================


/*
 ==================
 CM_LoadInlineModel
 ==================
*/
clipInlineModel_t *CM_LoadInlineModel (const char *name){

	int		index;

	if (!cm.loaded)	
		Com_Error(ERR_DROP, "CM_LoadInlineModel: map not loaded");

	if (!name || name[0] != '*')
		Com_Error(ERR_DROP, "CM_LoadInlineModel: NULL inline model name");

	index = Str_ToInteger(name+1);
	if (index < 1 || index >= cm.numModels)
		Com_Error(ERR_DROP, "CM_LoadInlineModel: bad index");

	return &cm.models[index];
}

/*
 ==================
 CM_NumInlineModels
 ==================
*/
int	CM_NumInlineModels (){

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_NumInlineModels: map not loaded");

	return cm.numModels;
}

/*
 ==================
 CM_GetEntityString
 ==================
*/
char *CM_GetEntityString (){

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_GetEntityString: map not loaded");

	if (!cm.numEntityChars)
		return "";

	return cm.entityString;
}

/*
 ==================
 
 ==================
*/
void CM_CreateBoxModel (){

	int				side;
	clipBrushSide_t	*s;
	clipNode_t		*n;
	cplane_t		*p;
	int				i;

	// Create the planes
	cm_boxPlanes = &cm.planes[cm.numPlanes];
	cm_boxHeadNode = cm.numNodes;

	// Create the brush
	cm_boxBrush = &cm.brushes[cm.numBrushes];
	cm_boxBrush->numSides = 6;
	cm_boxBrush->firstBrushSide = cm.numBrushSides;
	cm_boxBrush->contents = CONTENTS_MONSTER;

	cm_boxLeaf = &cm.leafs[cm.numLeafs];
	cm_boxLeaf->numLeafBrushes = 1;
	cm_boxLeaf->firstLeafBrush = cm.numLeafBrushes;
	cm_boxLeaf->contents = CONTENTS_MONSTER;

	cm.leafBrushes[cm.numLeafBrushes] = cm.numBrushes;

	for (i = 0; i < 6; i++){
		side = i & 1;

		// Brush sides
		s = &cm.brushSides[cm.numBrushSides+i];
		s->plane = &cm.planes[cm.numPlanes+i*2+side];
		s->surface = &cm.nullSurface;

		// Nodes
		n = &cm.nodes[cm.numNodes+i];
		n->plane = &cm.planes[cm.numPlanes+i*2];
		n->children[side] = -1 - cm.numLeafs;
		if (i != 5)
			n->children[side^1] = cm_boxHeadNode+i + 1;
		else
			n->children[side^1] = -1 - cm.numLeafs;

		// Planes
		p = &cm_boxPlanes[i*2+0];
		VectorClear(p->normal);
		p->normal[i>>1] = 1;
		p->type = i>>1;
		p->signbits = 0;

		p = &cm_boxPlanes[i*2+1];
		VectorClear(p->normal);
		p->normal[i>>1] = -1;
		p->type = 3;
		p->signbits = 0;
	}
}

/*
 ==================
 
 ==================
*/
int	CM_SetupBoxModel (const vec3_t mins, const vec3_t maxs){

	if (!cm.loaded)
		Com_Error(ERR_DROP, "CM_SetupBoxModel: map not loaded");

	// Set up the planes
	cm_boxPlanes[ 0].dist = maxs[0];
	cm_boxPlanes[ 1].dist = -maxs[0];
	cm_boxPlanes[ 2].dist = mins[0];
	cm_boxPlanes[ 3].dist = -mins[0];
	cm_boxPlanes[ 4].dist = maxs[1];
	cm_boxPlanes[ 5].dist = -maxs[1];
	cm_boxPlanes[ 6].dist = mins[1];
	cm_boxPlanes[ 7].dist = -mins[1];
	cm_boxPlanes[ 8].dist = maxs[2];
	cm_boxPlanes[ 9].dist = -maxs[2];
	cm_boxPlanes[10].dist = mins[2];
	cm_boxPlanes[11].dist = -mins[2];

	return cm_boxHeadNode;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void CM_ClipMapInfo_f (){

	if (!cm.loaded){
		Com_Printf("Clip map not loaded\n");
		return;
	}

	Com_Printf("\n");
	Com_Printf("%s\n", cm.name);
	Com_Printf("----------------------------------------\n");

	Com_Printf("----------------------------------------\n");
	Com_Printf("%.2f MB of clip map data\n", cm.size * (1.0f / 1048576.0f));
	Com_Printf("\n");
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 CM_InitModels
 ==================
*/
void CM_InitModels (){

	// Add commands
	Cmd_AddCommand("clipMapInfo", CM_ClipMapInfo_f, "Shows clip map information", NULL);
}

/*
 ==================
 CM_ShutdownModels
 ==================
*/
void CM_ShutdownModels (){

	// Remove commands
	Cmd_RemoveCommand("clipMapInfo");
}