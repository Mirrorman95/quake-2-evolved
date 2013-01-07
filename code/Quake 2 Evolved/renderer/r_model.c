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
// r_model.c - Model loading and caching
//

// TODO:
// - add .md5mesh and .md5anim support?
// - use FS_CacheFile for loading the BSP model?
// - something happend with lerping because the animation "laggs"


#include "r_local.h"


#define MODELS_HASH_SIZE			(MAX_MODELS >> 2)

static model_t				r_inlineModels[MAX_MODELS];

static model_t *			r_modelsHashTable[MODELS_HASH_SIZE];
static model_t *			r_models[MAX_MODELS];
static int					r_numModels;


/*
 ==================
 R_PointInLeaf
 ==================
*/
leaf_t *R_PointInLeaf (const vec3_t point){

	node_t		*node;
	int			side;

	if (!rg.worldModel || !rg.worldModel->nodes)
		Com_Error(ERR_DROP, "R_PointInLeaf: NULL world model");

	node = rg.worldModel->nodes;

	while (1){
		if (node->contents != -1)
			return (leaf_t *)node;

		side = PointOnPlaneSide(point, 0.0f, node->plane);

		if (side == PLANESIDE_FRONT)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	// Never reached
	return NULL;
}

/*
 ==================
 R_DecompressVis
 ==================
*/
static void R_DecompressVis (const byte *in, byte *out){

	byte	*pvs;
	int		c, row;

	row = (rg.worldModel->vis->numClusters + 7) >> 3;
	pvs = out;

	if (!in){
		// No vis info, so make all visible
		while (row){
			*pvs++ = 0xFF;
			row--;
		}

		return;
	}

	do {
		if (*in){
			*pvs++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c){
			*pvs++ = 0;
			c--;
		}
	} while (pvs - out < row);
}

/*
 ==================
 R_ClusterPVS
 ==================
*/
void R_ClusterPVS (int cluster, byte *pvs){

	if (cluster == -1 || !rg.worldModel || !rg.worldModel->vis){
		Mem_Fill(pvs, 0xFF, MAX_MAP_LEAFS/8);
		return;
	}

	R_DecompressVis((byte *)rg.worldModel->vis + rg.worldModel->vis->bitOfs[cluster][VIS_PVS], pvs);
}


/*
 ==============================================================================

 BSP LOADING

 ==============================================================================
*/


/*
 ==================
 R_LoadSky
 ==================
*/
static void R_LoadSky (const char *name, float rotate, const vec3_t axis){

	sky_t	*sky;

	rg.worldModel->sky = sky = (sky_t *)Mem_Alloc(sizeof(sky_t), TAG_RENDERER);
	rg.worldModel->size += sizeof(sky_t);

	sky->material = R_FindMaterial(name, MT_GENERIC, SURFACEPARM_SKY);

	sky->rotate = rotate;
	VectorCopy(axis, sky->axis);
}

/*
 ==================
 R_LoadVertices
 ==================
*/
static void R_LoadVertices (const byte *data, const bspLump_t *lump){

	bspVertex_t	*in;
	vertex_t	*out;
	int			i;

	in = (bspVertex_t *)(data + lump->offset);
	if (lump->length % sizeof(bspVertex_t))
		Com_Error(ERR_DROP, "R_LoadMap: funny lump size (%s)'", rg.worldModel->name);

	rg.worldModel->numVertices = lump->length / sizeof(bspVertex_t);
	rg.worldModel->vertices = out = (vertex_t *)Mem_Alloc(rg.worldModel->numVertices * sizeof(vertex_t), TAG_RENDERER);
	rg.worldModel->size += rg.worldModel->numVertices * sizeof(vertex_t);

	for (i = 0; i < rg.worldModel->numVertices; i++, in++, out++){
		out->point[0] = LittleFloat(in->point[0]);
		out->point[1] = LittleFloat(in->point[1]);
		out->point[2] = LittleFloat(in->point[2]);
	}
}

/*
 ==================
 R_LoadEdges
 ==================
*/
static void R_LoadEdges (const byte *data, const bspLump_t *lump){

	bspEdge_t	*in;
	edge_t		*out;
	int 		i;

	in = (bspEdge_t *)(data + lump->offset);
	if (lump->length % sizeof(bspEdge_t))
		Com_Error(ERR_DROP, "R_LoadMap: funny lump size (%s)", rg.worldModel->name);

	rg.worldModel->numEdges = lump->length / sizeof(bspEdge_t);
	rg.worldModel->edges = out = (edge_t *)Mem_Alloc(rg.worldModel->numEdges * sizeof(edge_t), TAG_RENDERER);
	rg.worldModel->size += rg.worldModel->numEdges * sizeof(edge_t);

	for (i = 0; i < rg.worldModel->numEdges; i++, in++, out++){
		out->v[0] = (ushort)LittleShort(in->v[0]);
		out->v[1] = (ushort)LittleShort(in->v[1]);
	}
}

/*
 ==================
 R_LoadSurfEdges
 ==================
*/
static void R_LoadSurfEdges (const byte *data, const bspLump_t *lump){

	int		*in, *out;
	int		i;
	
	in = (int *)(data + lump->offset);
	if (lump->length % sizeof(int))
		Com_Error(ERR_DROP, "R_LoadMap: funny lump size (%s)", rg.worldModel->name);

	rg.worldModel->numSurfEdges = lump->length / sizeof(int);
	rg.worldModel->surfEdges = out = (int *)Mem_Alloc(rg.worldModel->numSurfEdges * sizeof(int), TAG_RENDERER);
	rg.worldModel->size += rg.worldModel->numSurfEdges * sizeof(int);

	for (i = 0; i < rg.worldModel->numSurfEdges; i++)
		out[i] = LittleLong(in[i]);
}

/*
 ==================
 R_LoadPlanes
 ==================
*/
static void R_LoadPlanes (const byte *data, const bspLump_t *lump){

	bspPlane_t	*in;
	cplane_t	*out;
	int			i;
	
	in = (bspPlane_t *)(data + lump->offset);
	if (lump->length % sizeof(bspPlane_t))
		Com_Error(ERR_DROP, "R_LoadMap: funny lump size in '%s'", rg.worldModel->name);

	rg.worldModel->numPlanes = lump->length / sizeof(bspPlane_t);
	rg.worldModel->planes = out = (cplane_t *)Mem_Alloc(rg.worldModel->numPlanes * sizeof(cplane_t), TAG_RENDERER);
	rg.worldModel->size += rg.worldModel->numPlanes * sizeof(cplane_t);

	for (i = 0; i < rg.worldModel->numPlanes; i++, in++, out++){
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
 R_GetTexSize
 ==================
*/
static void R_GetTexSize (texInfo_t *texInfo){

	material_t		*material = texInfo->material;
	stage_t			*stage;
	char			name[MAX_PATH_LENGTH];
	mipTex_t		mt;
	fileHandle_t	f;
	int				i;

	// Look for a .wal texture first. This is so that retextures work.
	Str_SPrintf(name, sizeof(name), "%s.wal", material->name);
	FS_OpenFile(name, FS_READ, &f);
	if (f){
		// Found it, use its dimensions
		FS_Read(f, &mt, sizeof(mipTex_t));
		FS_CloseFile(f);

		texInfo->width = LittleLong(mt.width);
		texInfo->height = LittleLong(mt.height);
		return;
	}

	// No, so look for the first texture stage in the material
	for (i = 0, stage = material->stages; i < material->numStages; i++, stage++){
		if (stage->shaderStage.program)
			continue;

		if (stage->textureStage.texture->flags & TF_INTERNAL)
			continue;

		// Found it, use its dimensions
		texInfo->width = stage->textureStage.texture->width;
		texInfo->height = stage->textureStage.texture->height;
		return;
	}

	// Couldn't find shit, so just default to 64x64
	texInfo->width = 64;
	texInfo->height = 64;
}

/*
 ==================
 R_CheckTexClamp

 TODO: ignore other texture types?
 ==================
*/
static void R_CheckTexClamp (texInfo_t *texInfo){

	material_t	*material = texInfo->material;
	stage_t		*stage;
	int			i;

	// If at least one stage uses any form of texture clamping, then all
	// surfaces using this material will need their texture coordinates
	// to be clamped to the 0.0 - 1.0 range for things to work properly
	for (i = 0, stage = material->stages; i < material->numStages; i++, stage++){
		if (stage->shaderStage.program)
			continue;

		if (stage->textureStage.texture->type == TT_CUBE)
			continue;

		if (stage->textureStage.texture->wrap != TW_REPEAT){
			texInfo->clamp = true;
			return;
		}
	}

	// No need to clamp texture coordinates
	texInfo->clamp = false;
}

/*
 ==================
 R_LoadTexInfo
 ==================
*/
static void R_LoadTexInfo (const byte *data, const bspLump_t *lump){

	bspTexInfo_t	*in;
	texInfo_t		*out, *step;
	int				next;
	char			name[MAX_PATH_LENGTH];
	int 			i, j;
	uint			surfaceParm;

	in = (bspTexInfo_t *)(data + lump->offset);
	if (lump->length % sizeof(bspTexInfo_t))
		Com_Error(ERR_DROP, "R_LoadMap: funny lump size in '%s'", rg.worldModel->name);

	rg.worldModel->numTexInfo = lump->length / sizeof(bspTexInfo_t);
	rg.worldModel->texInfo = out = (texInfo_t *)Mem_Alloc(rg.worldModel->numTexInfo * sizeof(texInfo_t), TAG_RENDERER);
	rg.worldModel->size += rg.worldModel->numTexInfo * sizeof(texInfo_t);

	for (i = 0; i < rg.worldModel->numTexInfo; i++, in++, out++){
		out->flags = LittleLong(in->flags);

		for (j = 0; j < 4; j++){
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
		}

		next = LittleLong(in->nextTexInfo);
		if (next > 0)
			out->next = rg.worldModel->texInfo + next;
		else
			out->next = NULL;

		// Special case for sky surfaces
		if (out->flags & SURF_SKY){
			out->material = rg.worldModel->sky->material;
			out->width = 64;
			out->height = 64;
			out->clamp = false;

			continue;
		}

		// Special case for no-draw surfaces
		if (out->flags & SURF_NODRAW){
			out->material = rg.noDrawMaterial;
			out->width = 64;
			out->height = 64;
			out->clamp = false;

			continue;
		}

		// Get surfaceParm
		if (out->flags & (SURF_WARP | SURF_TRANS33 | SURF_TRANS66)){
			surfaceParm = 0;

			if (out->flags & SURF_WARP)
				surfaceParm |= SURFACEPARM_WARP;
			if (out->flags & SURF_TRANS33)
				surfaceParm |= SURFACEPARM_TRANS33;
			if (out->flags & SURF_TRANS66)
				surfaceParm |= SURFACEPARM_TRANS66;
			if (out->flags & SURF_FLOWING)
				surfaceParm |= SURFACEPARM_FLOWING;
		}
		else {
			surfaceParm = SURFACEPARM_LIGHTING;

			if (out->flags & SURF_FLOWING)
				surfaceParm |= SURFACEPARM_FLOWING;
		}

		// Load the material
		Str_SPrintf(name, sizeof(name), "textures/%s", in->texture);
		out->material = R_FindMaterial(name, MT_GENERIC, (surfaceParm_t)surfaceParm);

		// Find texture dimensions
		R_GetTexSize(out);

		// Check if surfaces will need clamping
		R_CheckTexClamp(out);
	}

	// Count animation frames
	for (i = 0, out = rg.worldModel->texInfo; i < rg.worldModel->numTexInfo; i++, out++){
		out->numFrames = 1;
		for (step = out->next; step && step != out; step = step->next)
			out->numFrames++;
	}
}

/*
 ==================
 R_FindSurfaceTriangleWithEdge
 ==================
*/
static int R_FindSurfaceTriangleWithEdge (int numTriangles, surfTriangle_t *triangles, glIndex_t start, glIndex_t end, int ignore){

	surfTriangle_t	*triangle;
	int				count, match;
	int				i;

	count = 0;
	match = -1;

	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		if ((triangle->index[0] == start && triangle->index[1] == end) || (triangle->index[1] == start && triangle->index[2] == end) || (triangle->index[2] == start && triangle->index[0] == end)){
			if (i != ignore)
				match = i;

			count++;
		}
		else if ((triangle->index[1] == start && triangle->index[0] == end) || (triangle->index[2] == start && triangle->index[1] == end) || (triangle->index[0] == start && triangle->index[2] == end))
			count++;
	}

	// Detect edges shared by three triangles and make them seams
	if (count > 2)
		match = -1;

	return match;
}

/*
 ==================
 R_BuildSurfaceTriangleNeighbors
 ==================
*/
static void R_BuildSurfaceTriangleNeighbors (int numTriangles, surfTriangle_t *triangles){

	surfTriangle_t	*triangle;
	int				i;

	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		triangle->neighbor[0] = R_FindSurfaceTriangleWithEdge(numTriangles, triangles, triangle->index[1], triangle->index[0], i);
		triangle->neighbor[1] = R_FindSurfaceTriangleWithEdge(numTriangles, triangles, triangle->index[2], triangle->index[1], i);
		triangle->neighbor[2] = R_FindSurfaceTriangleWithEdge(numTriangles, triangles, triangle->index[0], triangle->index[2], i);
	}
}

/*
 ==================
 R_CalcSurfaceBounds

 Fills in surface->mins and surface->maxs
 ==================
*/
static void R_CalcSurfaceBounds (surface_t *surface){

	vertex_t	*vertex;
	int			i, e;

	ClearBounds(surface->mins, surface->maxs);

	for (i = 0; i < surface->numEdges; i++){
		e = rg.worldModel->surfEdges[surface->firstEdge + i];
		if (e >= 0)
			vertex = &rg.worldModel->vertices[rg.worldModel->edges[e].v[0]];
		else
			vertex = &rg.worldModel->vertices[rg.worldModel->edges[-e].v[1]];

		AddPointToBounds(vertex->point, surface->mins, surface->maxs);
	}
}

/*
 ==================
 R_CalcSurfaceExtents

 Fills in surface->textureMins and surface->extents
 ==================
*/
static void R_CalcSurfaceExtents (surface_t *surface){

	vertex_t	*vertex;
	float		mins[2], maxs[2], value;
	int			bmins[2], bmaxs[2];
	int			i, j, e;

	mins[0] = mins[1] = 999999.0f;
	maxs[0] = maxs[1] = -999999.0f;

	for (i = 0; i < surface->numEdges; i++){
		e = rg.worldModel->surfEdges[surface->firstEdge + i];
		if (e >= 0)
			vertex = &rg.worldModel->vertices[rg.worldModel->edges[e].v[0]];
		else
			vertex = &rg.worldModel->vertices[rg.worldModel->edges[-e].v[1]];

		for (j = 0; j < 2; j++){
			value = DotProduct(vertex->point, surface->texInfo->vecs[j]) + surface->texInfo->vecs[j][3];
			if (value < mins[j])
				mins[j] = value;
			if (value > maxs[j])
				maxs[j] = value;
		}
	}

	for (i = 0; i < 2; i++){
		bmins[i] = floor(mins[i] / 16.0f);
		bmaxs[i] = ceil(maxs[i] / 16.0f);

		surface->textureMins[i] = bmins[i] * 16;
		surface->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}

/*
 ==================
 R_BuildSurfacePolygon
 ==================
*/
static void R_BuildSurfacePolygon (surface_t *surface){

	surfTriangle_t	*triangle;
	glVertex_t		*vertex;
	texInfo_t		*texInfo = surface->texInfo;
	vertex_t		*v;
	float			s, t;
	int				i, e;

	// Create triangles
	surface->numTriangles = (surface->numEdges - 2);
	surface->triangles = (surfTriangle_t *)Mem_Alloc(surface->numTriangles * sizeof(surfTriangle_t), TAG_RENDERER);

	rg.worldModel->size += surface->numTriangles * sizeof(surfTriangle_t);

	for (i = 2, triangle = surface->triangles; i < surface->numEdges; i++, triangle++){
		triangle->index[0] = 0;
		triangle->index[1] = i-1;
		triangle->index[2] = i;
	}

	// FIXME!!!
	surface->numIndices = surface->numTriangles;
	surface->indices = (glIndex_t *)Mem_Alloc(surface->numIndices * sizeof(glIndex_t), TAG_RENDERER);

	// Create vertices
	surface->numVertices = surface->numEdges;
	surface->vertices = (glVertex_t *)Mem_Alloc(surface->numVertices * sizeof(glVertex_t), TAG_RENDERER);

	rg.worldModel->size += surface->numVertices * sizeof(glVertex_t);

	for (i = 0, vertex = surface->vertices; i < surface->numEdges; i++, vertex++){
		// Vertex
		e = rg.worldModel->surfEdges[surface->firstEdge + i];
		if (e >= 0)
			v = &rg.worldModel->vertices[rg.worldModel->edges[e].v[0]];
		else
			v = &rg.worldModel->vertices[rg.worldModel->edges[-e].v[1]];

		VectorCopy(v->point, vertex->xyz);

		// Normal
		if (!(surface->flags & SURF_PLANEBACK))
			VectorCopy(surface->plane->normal, vertex->normal);
		else
			VectorNegate(surface->plane->normal, vertex->normal);

		// Tangents
		VectorNormalize2(texInfo->vecs[0], vertex->tangents[0]);
		VectorNormalize2(texInfo->vecs[1], vertex->tangents[1]);

		// Texture coordinates
		s = DotProduct(v->point, texInfo->vecs[0]) + texInfo->vecs[0][3];
		s /= texInfo->width;

		t = DotProduct(v->point, texInfo->vecs[1]) + texInfo->vecs[1][3];
		t /= texInfo->height;

		vertex->st[0] = s;
		vertex->st[1] = t;

		// Vertex color
		if (texInfo->flags & SURF_TRANS33)
			MakeRGBA(vertex->color, 255, 255, 255, 255 * 0.33f);
		else if (texInfo->flags & SURF_TRANS66)
			MakeRGBA(vertex->color, 255, 255, 255, 255 * 0.66f);
		else
			MakeRGBA(vertex->color, 255, 255, 255, 255);
	}

	// Build triangle neighbors
	R_BuildSurfaceTriangleNeighbors(surface->numTriangles, surface->triangles);
}

/*
 ==================
 R_FixSurfaceTextureCoords
 ==================
*/
static void R_FixSurfaceTextureCoords (surface_t *surface){

	glVertex_t		*vertex;
	vec2_t			bias = {999999.0f, 999999.0f};
	float			scale = 1.0f, max = 0.0f;
	int				i;

	if (!surface->texInfo->clamp)
		return;

	// Find the coordinate bias for each axis, which corresponds to the
	// minimum coordinate values
	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		if (vertex->st[0] < bias[0])
			bias[0] = vertex->st[0];
		if (vertex->st[1] < bias[1])
			bias[1] = vertex->st[1];
	}

	// Bias so that both axes end up with a minimum coordinate of zero,
	// and find the maximum coordinate value for transforming them to
	// the 0.0 - 1.0 range
	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		vertex->st[0] -= bias[0];
		vertex->st[1] -= bias[1];

		if (vertex->st[0] > max)
			max = vertex->st[0];
		if (vertex->st[1] > max)
			max = vertex->st[1];
	}

	// The scale factor is the inverse of the maximum value
	if (max)
		scale = 1.0f / max;

	// Finally scale so that both axes end up with a maximum coordinate
	// of one
	for (i = 0, vertex = surface->vertices; i < surface->numVertices; i++, vertex++){
		vertex->st[0] *= scale;
		vertex->st[1] *= scale;
	}
}

/*
 ==================
 R_LoadFaces
 ==================
*/
static void R_LoadFaces (const byte *data, const bspLump_t *lump){

	bspFace_t	*in;
	surface_t 	*out;
	int			i;

	in = (bspFace_t *)(data + lump->offset);
	if (lump->length % sizeof(bspFace_t))
		Com_Error(ERR_DROP, "R_LoadMap: funny lump size (%s)", rg.worldModel->name);

	rg.worldModel->numSurfaces = lump->length / sizeof(bspFace_t);
	rg.worldModel->surfaces = out = (surface_t *)Mem_Alloc(rg.worldModel->numSurfaces * sizeof(surface_t), TAG_RENDERER);
	rg.worldModel->size += rg.worldModel->numSurfaces * sizeof(surface_t);

	for (i = 0; i < rg.worldModel->numSurfaces; i++, in++, out++){
		out->flags = 0;
		out->firstEdge = LittleLong(in->firstEdge);
		out->numEdges = LittleShort(in->numEdges);

		if (LittleShort(in->side))
			out->flags |= SURF_PLANEBACK;

		out->plane = rg.worldModel->planes + LittleShort(in->planeNum);
		out->texInfo = rg.worldModel->texInfo + LittleShort(in->texInfo);

		// Clear index and vertex buffers
		out->indexBuffer = NULL;
		out->indexOffset = 0;

		out->vertexBuffer = NULL;
		out->vertexOffset = 0;

		// Clear counters
		out->viewCount = 0;
		out->worldCount = 0;
		out->fragmentCount = 0;

		// Find origin and bounds
		R_CalcSurfaceBounds(out);
		R_CalcSurfaceExtents(out);

		// Create the polygon
		R_BuildSurfacePolygon(out);

		// Fix texture coordinates for clamping
		R_FixSurfaceTextureCoords(out);
	}
}

/*
 ==================
 R_LoadMarkSurfaces
 ==================
*/
static void R_LoadMarkSurfaces (const byte *data, const bspLump_t *lump){

	short		*in;
	surface_t	**out;
	int			i;
	
	in = (short *)(data + lump->offset);
	if (lump->length % sizeof(short))
		Com_Error(ERR_DROP, "R_LoadMap: funny lump size (%s)", rg.worldModel->name);

	rg.worldModel->numMarkSurfaces = lump->length / sizeof(short);
	rg.worldModel->markSurfaces = out = (surface_t **)Mem_Alloc(rg.worldModel->numMarkSurfaces * sizeof(surface_t *), TAG_RENDERER);
	rg.worldModel->size += rg.worldModel->numMarkSurfaces * sizeof(surface_t *);

	for (i = 0; i < rg.worldModel->numMarkSurfaces; i++)
		out[i] = rg.worldModel->surfaces + LittleShort(in[i]);
}

/*
 ==================
 R_LoadVisibility
 ==================
*/
static void R_LoadVisibility (const byte *data, const bspLump_t *lump){

	int		i;

	if (!lump->length)
		return;

	rg.worldModel->vis = (vis_t *)Mem_Alloc(lump->length, TAG_RENDERER);
	rg.worldModel->size += lump->length;

	Mem_Copy(rg.worldModel->vis, data + lump->offset, lump->length);

	rg.worldModel->vis->numClusters = LittleLong(rg.worldModel->vis->numClusters);
	for (i = 0; i < rg.worldModel->vis->numClusters; i++){
		rg.worldModel->vis->bitOfs[i][0] = LittleLong(rg.worldModel->vis->bitOfs[i][0]);
		rg.worldModel->vis->bitOfs[i][1] = LittleLong(rg.worldModel->vis->bitOfs[i][1]);
	}
}

/*
 ==================
 R_LoadLeafs

 FIXME: changed back to Mem_ClearedAlloc due to node crashes in R_MarkLeaves
 ==================
*/
static void R_LoadLeafs (const byte *data, const bspLump_t *lump){

	bspLeaf_t	*in;
	leaf_t		*out;
	int			i, j;

	in = (bspLeaf_t *)(data + lump->offset);
	if (lump->length % sizeof(bspLeaf_t))
		Com_Error(ERR_DROP, "R_LoadMap: funny lump size (%s)", rg.worldModel->name);

	rg.worldModel->numLeafs = lump->length / sizeof(bspLeaf_t);
	rg.worldModel->leafs = out = (leaf_t *)Mem_ClearedAlloc(rg.worldModel->numLeafs * sizeof(leaf_t), TAG_RENDERER);
	rg.worldModel->size += rg.worldModel->numLeafs * sizeof(leaf_t);

	for (i = 0; i < rg.worldModel->numLeafs; i++, in++, out++){
		for (j = 0; j < 3; j++){
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}

		// Clear counters
		out->viewCount = 0;
		out->visCount = 0;

		// Leaf specific
		out->contents = LittleLong(in->contents);
		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		out->firstMarkSurface = rg.worldModel->markSurfaces + LittleShort(in->firstLeafFace);
		out->numMarkSurfaces = LittleShort(in->numLeafFaces);

		// Mark the surfaces for caustics
		if (out->contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
			for (j = 0; j < out->numMarkSurfaces; j++){
				if (out->firstMarkSurface[j]->texInfo->flags & SURF_WARP)
					continue;	// HACK: ignore warped surfaces

				if (out->contents & CONTENTS_WATER)
					out->firstMarkSurface[j]->flags |= SURF_WATERCAUSTICS;
				if (out->contents & CONTENTS_SLIME)
					out->firstMarkSurface[j]->flags |= SURF_SLIMECAUSTICS;
				if (out->contents & CONTENTS_LAVA)
					out->firstMarkSurface[j]->flags |= SURF_LAVACAUSTICS;
			}
		}
	}
}

/*
 ==================
 R_SetParent
 ==================
*/
static void R_SetParent (node_t *node, node_t *parent){

	node->parent = parent;
	if (node->contents != -1)
		return;

	R_SetParent(node->children[0], node);
	R_SetParent(node->children[1], node);
}

/*
 ==================
 R_LoadNodes

 FIXME: changed back to Mem_ClearedAlloc due to node crashes in R_MarkLeaves
 ==================
*/
static void R_LoadNodes (const byte *data, const bspLump_t *lump){

	bspNode_t	*in;
	node_t		*out;
	int			i, j, p;

	in = (bspNode_t *)(data + lump->offset);
	if (lump->length % sizeof(bspNode_t))
		Com_Error(ERR_DROP, "R_LoadMap: funny lump size (%s)", rg.worldModel->name);

	rg.worldModel->numNodes = lump->length / sizeof(bspNode_t);
	rg.worldModel->nodes = out = (node_t *)Mem_ClearedAlloc(rg.worldModel->numNodes * sizeof(node_t), TAG_RENDERER);
	rg.worldModel->size += rg.worldModel->numNodes * sizeof(node_t);

	for (i = 0; i < rg.worldModel->numNodes; i++, in++, out++){
		for (j = 0; j < 3; j++){
			out->mins[j] = LittleShort(in->mins[j]);
			out->maxs[j] = LittleShort(in->maxs[j]);
		}
	
		// Clear counters
		out->viewCount = 0;
		out->visCount = 0;

		// Node specific
		out->plane = rg.worldModel->planes + LittleLong(in->planeNum);

		out->contents = -1;
		out->firstSurface = LittleShort(in->firstFace);
		out->numSurfaces = LittleShort(in->numFaces);

		for (j = 0; j < 2; j++){
			p = LittleLong(in->children[j]);
			if (p >= 0)
				out->children[j] = rg.worldModel->nodes + p;
			else
				out->children[j] = (node_t *)(rg.worldModel->leafs + (-1 - p));
		}
	}

	// Set nodes and leafs
	R_SetParent(rg.worldModel->nodes, NULL);
}

/*
 ==================
 R_SetupInlineModels

 TODO: fix names
 ==================
*/
static void R_SetupInlineModels (){

	inlineModel_t	*inlineModel;
	model_t			*model;
	int				i;

	for (i = 0; i < rg.worldModel->numInlineModels; i++){
		inlineModel = &rg.worldModel->inlineModels[i];
		model = &r_inlineModels[i];

		*model = *rg.worldModel;

		model->numModelSurfaces = inlineModel->numFaces;
		model->firstModelSurface = inlineModel->firstFace;
		model->firstNode = inlineModel->headNode;
		VectorCopy(inlineModel->maxs, model->maxs);
		VectorCopy(inlineModel->mins, model->mins);
		model->radius = inlineModel->radius;

		if (i == 0){
			Str_Copy(model->name, "_worldMap", sizeof(model->name));
			*rg.worldModel = *model;
		}
//		else
//			Str_SPrintf(model->name, sizeof(model->name), "_inlineModel%i", i);

		model->numLeafs = inlineModel->visLeafs;
	}
}

/*
 ==================
 R_LoadInlineModels
 ==================
*/
static void R_LoadInlineModels (const byte *data, const bspLump_t *lump){

	bspInlineModel_t	*in;
	inlineModel_t		*out;
	int					i, j;

	in = (bspInlineModel_t *)(data + lump->offset);
	if (lump->length % sizeof(bspInlineModel_t))
		Com_Error(ERR_DROP, "R_LoadMap: funny lump size (%s)", rg.worldModel->name);

	rg.worldModel->numInlineModels = lump->length / sizeof(bspInlineModel_t);
	rg.worldModel->inlineModels = out = (inlineModel_t *)Mem_Alloc(rg.worldModel->numInlineModels * sizeof(inlineModel_t), TAG_RENDERER);
	rg.worldModel->size += rg.worldModel->numInlineModels * sizeof(inlineModel_t);

	for (i = 0; i < rg.worldModel->numInlineModels; i++, in++, out++){
		for (j = 0; j < 3; j++){
			// Spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;

			out->origin[j] = LittleFloat(in->origin[j]);
		}

		out->radius = RadiusFromBounds(out->mins, out->maxs);

		out->headNode = LittleLong(in->headNode);
		out->firstFace = LittleLong(in->firstFace);
		out->numFaces = LittleLong(in->numFaces);
	}

	// Set up the inline models
	R_SetupInlineModels();
}

/*
 ==================
 R_CacheGeometry
 ==================
*/
static void R_CacheGeometry (){

	char			name[MAX_PATH_LENGTH];
	surface_t		*surface;
	arrayBuffer_t	*indexBuffer, *vertexBuffer;
	int				indexCount[MAX_MATERIALS], vertexCount[MAX_MATERIALS];
	int				indexOffset, vertexOffset;
	int				count = 0;
	int				i, j;

	if (!r_indexBuffers->integerValue && !r_vertexBuffers->integerValue)
		return;

	// Count indices and vertices for each buffer
	Mem_Fill(indexCount, 0, sizeof(indexCount));
	Mem_Fill(vertexCount, 0, sizeof(vertexCount));

	for (i = 0, surface = rg.worldModel->surfaces; i < rg.worldModel->numSurfaces; i++, surface++){
		indexCount[surface->texInfo->material->index] += surface->numIndices;
		vertexCount[surface->texInfo->material->index] += surface->numVertices;
	}

	// Generate the index and vertex buffers
	for (i = 0; i < MAX_MATERIALS; i++){
		if (!indexCount[i] && !vertexCount[i])
			continue;

		// Allocate the index and vertex buffers
		Str_SPrintf(name, sizeof(name), "bspCache%i", count);

		indexBuffer = R_AllocIndexBuffer(name, false, indexCount[i], NULL);
		vertexBuffer = R_AllocVertexBuffer(name, false, vertexCount[i], NULL);

		if (!indexBuffer && !vertexBuffer)
			continue;

		// Cache all the surfaces
		indexOffset = 0;
		vertexOffset = 0;

		for (j = 0, surface = rg.worldModel->surfaces; j < rg.worldModel->numSurfaces; j++, surface++){
			if (surface->texInfo->material->index != i)
				continue;

			// Set up the index buffer
			if (indexBuffer){
				surface->indexBuffer = indexBuffer;
				surface->indexOffset = indexOffset;

				// Cache the surface indices
				R_UpdateIndexBuffer(surface->indexBuffer, surface->indexOffset, surface->numIndices, surface->indices, false, true);

				indexOffset += surface->numIndices;
			}

			// Set up the vertex buffer
			if (vertexBuffer){
				surface->vertexBuffer = vertexBuffer;
				surface->vertexOffset = vertexOffset;

				// Cache the surface vertices
				R_UpdateVertexBuffer(surface->vertexBuffer, surface->vertexOffset, surface->numVertices, surface->vertices, false, true);

				vertexOffset += surface->numVertices;
			}
		}

		count++;
	}
}

/*
 ==================
 R_LoadMap
 ==================
*/
void R_LoadMap (const char *name, const char *skyName, float skyRotate, const vec3_t skyAxis){

	bspHeader_t	*header;
	byte		*data;
	int			i;
	uint		hashKey;

	// Check if already loaded
	if (rg.worldModel){
		if (!Str_ICompare(rg.worldModel->name, name))
			return;

		Com_Error(ERR_DROP, "R_LoadMap: can't load '%s' because another world map is already loaded", name);
	}

	// Load the file
	FS_ReadFile(name, (void **)&data);
	if (!data)
		Com_Error(ERR_DROP, "R_LoadMap: '%s' not found", name);

	// Allocate the world
	r_models[r_numModels++] = rg.worldModel = (model_t *)Mem_Alloc(sizeof(model_t), TAG_RENDERER);

	// Fill it in
	Str_Copy(rg.worldModel->name, name, sizeof(rg.worldModel->name));
	rg.worldModel->type = MODEL_INLINE;
	rg.worldModel->size = 0;

	// Byte swap the header fields and sanity check
	header = (bspHeader_t *)data;

	for (i = 0; i < sizeof(bspHeader_t) / 4; i++)
		((int *)header)[i] = LittleLong(((int *)header)[i]);

	if (header->id != BSP_ID)
		Com_Error(ERR_DROP, "R_LoadMap: wrong file id (%s)", rg.worldModel->name);

	if (header->version != BSP_VERSION)
		Com_Error(ERR_DROP, "R_LoadMap: wrong version number (%i should be %i) (%s)", header->version, BSP_VERSION, rg.worldModel->name);

	// Load the lumps
	R_LoadSky(skyName, skyRotate, skyAxis);

	R_LoadVertices(data, &header->lumps[LUMP_VERTICES]);
	R_LoadEdges(data, &header->lumps[LUMP_EDGES]);
	R_LoadSurfEdges(data, &header->lumps[LUMP_SURFEDGES]);
	R_LoadPlanes(data, &header->lumps[LUMP_PLANES]);
	R_LoadTexInfo(data, &header->lumps[LUMP_TEXINFO]);
	R_LoadFaces(data, &header->lumps[LUMP_FACES]);
	R_LoadMarkSurfaces(data, &header->lumps[LUMP_LEAFFACES]);
	R_LoadVisibility(data, &header->lumps[LUMP_VISIBILITY]);
	R_LoadLeafs(data, &header->lumps[LUMP_LEAFS]);
	R_LoadNodes(data, &header->lumps[LUMP_NODES]);
	R_LoadInlineModels(data, &header->lumps[LUMP_INLINEMODELS]);

	FS_FreeFile(data);

	// Set the world model
	rg.worldEntity->model = rg.worldModel;

	// Load caustic materials
	rg.waterCausticsMaterial = R_FindMaterial("gfx/caustics/water", MT_GENERIC, SURFACEPARM_NONE);
	rg.slimeCausticsMaterial = R_FindMaterial("gfx/caustics/slime", MT_GENERIC, SURFACEPARM_NONE);
	rg.lavaCausticsMaterial = R_FindMaterial("gfx/caustics/lava", MT_GENERIC, SURFACEPARM_NONE);

	// Try to cache the geometry in static index and vertex buffers
	R_CacheGeometry();

	// Add to hash table
	hashKey = Str_HashKey(rg.worldModel->name, MODELS_HASH_SIZE, false);

	rg.worldModel->nextHash = r_modelsHashTable[hashKey];
	r_modelsHashTable[hashKey] = rg.worldModel;
}


// ============================================================================


/*
 ==================
 R_CalcTangentVectors
 ==================
*/
static void R_CalcTangentVectors (int numTriangles, mdlTriangle_t *triangles, int numVertices, mdlXyzNormal_t *xyzNormals, mdlSt_t *st, int frameNum){

	mdlTriangle_t	*triangle;
	mdlXyzNormal_t	*xyzNormal;
	float			*pXyz[3], *pSt[3];
	vec5_t			edge[2];
	vec3_t			normal, tangents[2], cross;
	float			d;
	int				i, j;

	xyzNormals += numVertices * frameNum;

	// Clear
	for (i = 0, xyzNormal = xyzNormals; i < numVertices; i++, xyzNormal++){
		VectorClear(xyzNormal->tangents[0]);
		VectorClear(xyzNormal->tangents[1]);
	}

	// Calculate normal and tangent vectors
	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		pXyz[0] = (float *)(xyzNormals[triangle->index[0]].xyz);
		pXyz[1] = (float *)(xyzNormals[triangle->index[1]].xyz);
		pXyz[2] = (float *)(xyzNormals[triangle->index[2]].xyz);

		pSt[0] = (float *)(st[triangle->index[0]].st);
		pSt[1] = (float *)(st[triangle->index[1]].st);
		pSt[2] = (float *)(st[triangle->index[2]].st);

		// Find edges
		edge[0][0] = pXyz[1][0] - pXyz[0][0];
		edge[0][1] = pXyz[1][1] - pXyz[0][1];
		edge[0][2] = pXyz[1][2] - pXyz[0][2];
		edge[0][3] = pSt[1][0] - pSt[0][0];
		edge[0][4] = pSt[1][1] - pSt[0][1];

		edge[1][0] = pXyz[2][0] - pXyz[0][0];
		edge[1][1] = pXyz[2][1] - pXyz[0][1];
		edge[1][2] = pXyz[2][2] - pXyz[0][2];
		edge[1][3] = pSt[2][0] - pSt[0][0];
		edge[1][4] = pSt[2][1] - pSt[0][1];

		// Compute normal vector
		normal[0] = edge[1][1] * edge[0][2] - edge[1][2] * edge[0][1];
		normal[1] = edge[1][2] * edge[0][0] - edge[1][0] * edge[0][2];
		normal[2] = edge[1][0] * edge[0][1] - edge[1][1] * edge[0][0];

		VectorNormalize(normal);

		// Compute first tangent vector
		tangents[0][0] = edge[1][4] * edge[0][0] - edge[1][0] * edge[0][4];
		tangents[0][1] = edge[1][4] * edge[0][1] - edge[1][1] * edge[0][4];
		tangents[0][2] = edge[1][4] * edge[0][2] - edge[1][2] * edge[0][4];

		d = DotProduct(tangents[0], normal);
		VectorMA(tangents[0], -d, normal, tangents[0]);
		VectorNormalize(tangents[0]);

		// Compute second tangent vector
		tangents[1][0] = edge[1][0] * edge[0][3] - edge[1][3] * edge[0][0];
		tangents[1][1] = edge[1][1] * edge[0][3] - edge[1][3] * edge[0][1];
		tangents[1][2] = edge[1][2] * edge[0][3] - edge[1][3] * edge[0][2];

		d = DotProduct(tangents[1], normal);
		VectorMA(tangents[1], -d, normal, tangents[1]);
		VectorNormalize(tangents[1]);

		// Inverse tangent vectors if needed
		CrossProduct(tangents[1], tangents[0], cross);
		if (DotProduct(cross, normal) < 0.0f){
			VectorInverse(tangents[0]);
			VectorInverse(tangents[1]);
		}

		// Add the vectors
		for (j = 0; j < 3; j++){
			VectorAdd(xyzNormals[triangle->index[j]].tangents[0], tangents[0], xyzNormals[triangle->index[j]].tangents[0]);
			VectorAdd(xyzNormals[triangle->index[j]].tangents[1], tangents[1], xyzNormals[triangle->index[j]].tangents[1]);
		}
	}

	// Renormalize the tangent vectors
	for (i = 0, xyzNormal = xyzNormals; i < numVertices; i++, xyzNormal++){
		VectorNormalize(xyzNormal->tangents[0]);
		VectorNormalize(xyzNormal->tangents[1]);
	}
}

/*
 ==================
 R_CalcFacePlanes
 ==================
*/
static void R_CalcFacePlanes (int numTriangles, mdlTriangle_t *triangles, mdlFacePlane_t *facePlanes, int numVertices, mdlXyzNormal_t *xyzNormals, int frameNum){

	mdlTriangle_t	*triangle;
	mdlFacePlane_t	*facePlane;
	float			*pXyz[3];
	vec3_t			edge[2];
	int				i;

	facePlanes += numTriangles * frameNum;
	xyzNormals += numVertices * frameNum;

	// Calculate face planes
	for (i = 0, triangle = triangles, facePlane = facePlanes; i < numTriangles; i++, triangle++, facePlane++){
		pXyz[0] = (float *)(xyzNormals[triangle->index[0]].xyz);
		pXyz[1] = (float *)(xyzNormals[triangle->index[1]].xyz);
		pXyz[2] = (float *)(xyzNormals[triangle->index[2]].xyz);

		// Find edges
		VectorSubtract(pXyz[1], pXyz[0], edge[0]);
		VectorSubtract(pXyz[2], pXyz[0], edge[1]);

		// Compute normal
		CrossProduct(edge[1], edge[0], facePlane->normal);
		VectorNormalize(facePlane->normal);

		// Compute distance
		facePlane->dist = DotProduct(pXyz[0], facePlane->normal);
	}
}

/*
 ==================
 R_FindTriangleWithEdge
 ==================
*/
static int R_FindTriangleWithEdge (int numTriangles, mdlTriangle_t *triangles, glIndex_t start, glIndex_t end, int ignore){

	mdlTriangle_t	*triangle;
	int				count, match;
	int				i;

	count = 0;
	match = -1;

	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		if ((triangle->index[0] == start && triangle->index[1] == end) || (triangle->index[1] == start && triangle->index[2] == end) || (triangle->index[2] == start && triangle->index[0] == end)){
			if (i != ignore)
				match = i;

			count++;
		}
		else if ((triangle->index[1] == start && triangle->index[0] == end) || (triangle->index[2] == start && triangle->index[1] == end) || (triangle->index[0] == start && triangle->index[2] == end))
			count++;
	}

	// Detect edges shared by three triangles and make them seams
	if (count > 2)
		match = -1;

	return match;
}

/*
 ==================
 R_BuildTriangleNeighbors
 ==================
*/
static void R_BuildTriangleNeighbors (int numTriangles, mdlTriangle_t *triangles){

	mdlTriangle_t	*triangle;
	int				i;

	for (i = 0, triangle = triangles; i < numTriangles; i++, triangle++){
		triangle->neighbor[0] = R_FindTriangleWithEdge(numTriangles, triangles, triangle->index[1], triangle->index[0], i);
		triangle->neighbor[1] = R_FindTriangleWithEdge(numTriangles, triangles, triangle->index[2], triangle->index[1], i);
		triangle->neighbor[2] = R_FindTriangleWithEdge(numTriangles, triangles, triangle->index[0], triangle->index[2], i);
	}
}

/*
 ==================
 R_CalcModelBounds
 ==================
*/
static void R_CalcModelBounds (model_t *model, mdl_t *outModel){

	int		i;

	// Calculate model bounds and radius
	ClearBounds(model->mins, model->maxs);

	for (i = 0; i < outModel->numFrames; i++){
		AddPointToBounds(outModel->frames[i].mins, model->mins, model->maxs);
		AddPointToBounds(outModel->frames[i].maxs, model->mins, model->maxs);
	}

	model->radius = RadiusFromBounds(model->mins, model->maxs);
}

/*
 ==================
 
 ==================
*/
static void R_CacheAliasModelGeometry (mdl_t *model, const char *modelName){

}


/*
 ==============================================================================

 MD3 LOADING

 ==============================================================================
*/


/*
 =================
 R_LoadMD3Model
 =================
*/
static bool R_LoadMD3Model (const char *name, mdl_t **model, int *size){

	byte			*data;
	md3Header_t		*inHeader;
	md3Frame_t		*inFrame;
	md3Tag_t		*inTag;
	md3Surface_t	*inSurface;
	md3Material_t	*inMaterial;
	md3Triangle_t	*inTriangle;
	md3St_t			*inSt;
	md3XyzNormal_t	*inXyzNormal;
	mdl_t			*outModel;
	mdlFrame_t		*outFrame;
	mdlTag_t		*outTag;
	mdlSurface_t	*outSurface;
	mdlMaterial_t	*outMaterial;
	mdlTriangle_t	*outTriangle;
	mdlSt_t			*outSt;
	mdlXyzNormal_t	*outXyzNormal;
	char			checkName[MAX_PATH_LENGTH];
	vec3_t			scale, translate;
	float			lat, lng;
	int				id, version;
	int				i, j, k;

	// Load the file
	FS_ReadFile(name, (void **)&data);
	if (!data)
		return false;

	*model = outModel = (mdl_t *)Mem_Alloc(sizeof(mdl_t), TAG_RENDERER);
	*size = sizeof(mdl_t);

	// Byte swap the header fields and sanity check
	inHeader = (md3Header_t *)data;

	id = LittleLong(inHeader->id);
	if (id != MD3_ID)
		Com_Error(ERR_DROP, "R_LoadMD3Model: wrong file id (%s)", name);

	version = LittleLong(inHeader->version);
	if (version != MD3_VERSION)
		Com_Error(ERR_DROP, "R_LoadMD3Model: wrong version number (%i should be %i) (%s)", version, MD3_VERSION, name);

	// Check the number of frames
	outModel->numFrames = LittleLong(inHeader->numFrames);
	if (outModel->numFrames > MD3_MAX_FRAMES || outModel->numFrames <= 0)
		Com_Error(ERR_DROP, "R_LoadMD3Model: invalid number of frames (%i) (%s)", outModel->numFrames, name);

	// Check the number of tags
	outModel->numTags = LittleLong(inHeader->numTags);
	if (outModel->numTags > MD3_MAX_TAGS || outModel->numTags < 0)
		Com_Error(ERR_DROP, "R_LoadMD3Model: invalid number of tags (%i) (%s)\n", outModel->numTags, name);

	// Check the number of surfaces
	outModel->numSurfaces = LittleLong(inHeader->numSurfaces);
	if (outModel->numSurfaces > MD3_MAX_SURFACES || outModel->numSurfaces <= 0)
		Com_Error(ERR_DROP, "R_LoadMD3Model: invalid number of surfaces (%i) (%s)\n", outModel->numSurfaces, name);

	// Load the frames
	inFrame = (md3Frame_t *)((byte *)inHeader + LittleLong(inHeader->ofsFrames));
	outModel->frames = outFrame = (mdlFrame_t *)Mem_Alloc(outModel->numFrames * sizeof(mdlFrame_t), TAG_RENDERER);

	*size += outModel->numFrames * sizeof(mdlFrame_t);

	for (i = 0; i < outModel->numFrames; i++, inFrame++, outFrame++){
		outFrame->mins[0] = LittleFloat(inFrame->bounds[0][0]);
		outFrame->mins[1] = LittleFloat(inFrame->bounds[0][1]);
		outFrame->mins[2] = LittleFloat(inFrame->bounds[0][2]);
		outFrame->maxs[0] = LittleFloat(inFrame->bounds[1][0]);
		outFrame->maxs[1] = LittleFloat(inFrame->bounds[1][1]);
		outFrame->maxs[2] = LittleFloat(inFrame->bounds[1][2]);

		outFrame->radius = LittleFloat(inFrame->radius);
	}

	// Load the tags
	inTag = (md3Tag_t *)((byte *)inHeader + LittleLong(inHeader->ofsTags));
	outModel->tags = outTag = (mdlTag_t *)Mem_Alloc(outModel->numFrames * outModel->numTags * sizeof(mdlTag_t), TAG_RENDERER);

	*size += outModel->numFrames * outModel->numTags * sizeof(mdlTag_t);

	for (i = 0; i < outModel->numFrames; i++){
		for (j = 0; j < outModel->numTags; j++, inTag++, outTag++){
			Str_Copy(outTag->name, inTag->name, sizeof(outTag->name));

			outTag->origin[0] = LittleFloat(inTag->origin[0]);
			outTag->origin[1] = LittleFloat(inTag->origin[1]);
			outTag->origin[2] = LittleFloat(inTag->origin[2]);

			outTag->axis[0][0] = LittleFloat(inTag->axis[0][0]);
			outTag->axis[0][1] = LittleFloat(inTag->axis[0][1]);
			outTag->axis[0][2] = LittleFloat(inTag->axis[0][2]);
			outTag->axis[1][0] = LittleFloat(inTag->axis[1][0]);
			outTag->axis[1][1] = LittleFloat(inTag->axis[1][1]);
			outTag->axis[1][2] = LittleFloat(inTag->axis[1][2]);
			outTag->axis[2][0] = LittleFloat(inTag->axis[2][0]);
			outTag->axis[2][1] = LittleFloat(inTag->axis[2][1]);
			outTag->axis[2][2] = LittleFloat(inTag->axis[2][2]);
		}
	}

	// Load the surfaces
	inSurface = (md3Surface_t *)((byte *)inHeader + LittleLong(inHeader->ofsSurfaces));
	outModel->surfaces = outSurface = (mdlSurface_t *)Mem_Alloc(outModel->numSurfaces * sizeof(mdlSurface_t), TAG_RENDERER);

	*size += outModel->numSurfaces * sizeof(mdlSurface_t);

	for (i = 0; i < outModel->numSurfaces; i++, outSurface++){
		// Byte swap the header fields and sanity check
		id = LittleLong(inSurface->id);
		if (id != MD3_ID)
			Com_Error(ERR_DROP, "R_LoadMD3Model: wrong file id in surface '%s' (%s)\n", inSurface->name, name);

		outSurface->numMaterials = LittleLong(inSurface->numShaders);
		if (outSurface->numMaterials > MD3_MAX_SHADERS || outSurface->numMaterials <= 0)
			Com_Error(ERR_DROP, "R_LoadMD3Model: invalid number of materials in surface '%s' (%i) (%s)\n", inSurface->name, outSurface->numMaterials, name);

		outSurface->numTriangles = LittleLong(inSurface->numTriangles);
		if (outSurface->numTriangles > MD3_MAX_TRIANGLES || outSurface->numTriangles <= 0)
			Com_Error(ERR_DROP, "R_LoadMD3Model: invalid number of triangles in surface '%s' (%i) (%s)\n", inSurface->name, outSurface->numTriangles, name);

		outSurface->numVertices = LittleLong(inSurface->numVerts);
		if (outSurface->numVertices > MD3_MAX_VERTS || outSurface->numVertices <= 0)
			Com_Error(ERR_DROP, "R_LoadMD3Model: invalid number of vertices in surface '%s' (%i) (%s)\n", inSurface->name, outSurface->numVertices, name);

		// Load the materials
		inMaterial = (md3Material_t *)((byte *)inSurface + LittleLong(inSurface->ofsShaders));
		outSurface->materials = outMaterial = (mdlMaterial_t *)Mem_Alloc(outSurface->numMaterials * sizeof(mdlMaterial_t), TAG_RENDERER);

		*size += outSurface->numMaterials * sizeof(mdlMaterial_t);

		for (j = 0; j < outSurface->numMaterials; j++, inMaterial++, outMaterial++){
			Str_Copy(checkName, inMaterial->name, sizeof(checkName));
			Str_StripFileExtension(checkName);

			outMaterial->material = R_FindMaterial(checkName, MT_GENERIC, SURFACEPARM_LIGHTING);
		}

		// Load the triangles
		inTriangle = (md3Triangle_t *)((byte *)inSurface + LittleLong(inSurface->ofsTriangles));
		outSurface->triangles = outTriangle = (mdlTriangle_t *)Mem_Alloc(outSurface->numTriangles * sizeof(mdlTriangle_t), TAG_RENDERER);

		*size += outSurface->numTriangles * sizeof(mdlTriangle_t);

		for (j = 0; j < outSurface->numTriangles; j++, inTriangle++, outTriangle++){
			outTriangle->index[0] = (glIndex_t)LittleLong(inTriangle->indexes[0]);
			outTriangle->index[1] = (glIndex_t)LittleLong(inTriangle->indexes[1]);
			outTriangle->index[2] = (glIndex_t)LittleLong(inTriangle->indexes[2]);
		}

		// Load the vertices
		inSt = (md3St_t *)((byte *)inSurface + LittleLong(inSurface->ofsSt));
		outSurface->st = outSt = (mdlSt_t *)Mem_Alloc(outSurface->numVertices * sizeof(mdlSt_t), TAG_RENDERER);

		*size += outSurface->numVertices * sizeof(mdlSt_t);

		for (j = 0; j < outSurface->numVertices; j++, inSt++, outSt++){
			outSt->st[0] = LittleFloat(inSt->st[0]);
			outSt->st[1] = LittleFloat(inSt->st[1]);
		}

		// Allocate space for face planes
		outSurface->facePlanes = (mdlFacePlane_t *)Mem_Alloc(outModel->numFrames * outSurface->numTriangles * sizeof(mdlFacePlane_t), TAG_RENDERER);
		*size += outModel->numFrames * outSurface->numTriangles * sizeof(mdlFacePlane_t);

		// Load XYZ vertices
		inXyzNormal = (md3XyzNormal_t *)((byte *)inSurface + LittleLong(inSurface->ofsXyzNormals));
		outSurface->xyzNormals = outXyzNormal = (mdlXyzNormal_t *)Mem_Alloc(outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t), TAG_RENDERER);

		*size += outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t);

		inFrame = (md3Frame_t *)((byte *)inHeader + LittleLong(inHeader->ofsFrames));

		for (j = 0; j < outModel->numFrames; j++, inFrame++){
			scale[0] = MD3_XYZ_SCALE;
			scale[1] = MD3_XYZ_SCALE;
			scale[2] = MD3_XYZ_SCALE;

			translate[0] = LittleFloat(inFrame->localOrigin[0]);
			translate[1] = LittleFloat(inFrame->localOrigin[1]);
			translate[2] = LittleFloat(inFrame->localOrigin[2]);

			for (k = 0; k < outSurface->numVertices; k++, inXyzNormal++, outXyzNormal++){
				outXyzNormal->xyz[0] = (float)LittleShort(inXyzNormal->xyz[0]) * scale[0] + translate[0];
				outXyzNormal->xyz[1] = (float)LittleShort(inXyzNormal->xyz[1]) * scale[1] + translate[1];
				outXyzNormal->xyz[2] = (float)LittleShort(inXyzNormal->xyz[2]) * scale[2] + translate[2];

				lat = (float)((inXyzNormal->normal >> 8) & 0xFF) * M_PI/128.0f;
				lng = (float)((inXyzNormal->normal >> 0) & 0xFF) * M_PI/128.0f;

				outXyzNormal->normal[0] = sin(lng) * cos(lat);
				outXyzNormal->normal[1] = sin(lng) * sin(lat);
				outXyzNormal->normal[2] = cos(lng);
			}

			// Calculate tangent vectors
			R_CalcTangentVectors(outSurface->numTriangles, outSurface->triangles, outSurface->numVertices, outSurface->xyzNormals, outSurface->st, j);

			// Calculate face planes
			R_CalcFacePlanes(outSurface->numTriangles, outSurface->triangles, outSurface->facePlanes, outSurface->numVertices, outSurface->xyzNormals, j);
		}

		// Build triangle neighbors
		R_BuildTriangleNeighbors(outSurface->numTriangles, outSurface->triangles);

		// Clear index and vertex buffers
		outSurface->indexBuffer = NULL;
		outSurface->indexOffset = 0;

		outSurface->vertexBuffer = NULL;
		outSurface->vertexOffset = 0;

		// Skip to next surface
		inSurface = (md3Surface_t *)((byte *)inSurface + LittleLong(inSurface->ofsEnd));
	}

	// Calculate bounds
	R_CalcModelBounds((model_t *)model, outModel);

	// Free file data
	FS_FreeFile(data);

	// Try to cache the geometry in static index and vertex buffers
	R_CacheAliasModelGeometry(outModel, name);

	return true;
}


/*
 ==============================================================================

 MD2 LOADING

 ==============================================================================
*/


/*
 ==================
 R_LoadMD2Model
 ==================
*/
static bool R_LoadMD2Model (const char *name, mdl_t **model, int *size){

	byte			*data;
	md2Header_t		*inHeader;
	md2Triangle_t	*inTriangle;
	md2St_t			*inSt;
	md2Frame_t		*inFrame;
	mdl_t			*outModel;
	mdlSurface_t	*outSurface;
	mdlTriangle_t	*outTriangle;
	mdlSt_t			*outSt;
	mdlFrame_t		*outFrame;
	mdlXyzNormal_t	*outXyzNormal;
	mdlMaterial_t	*outMaterial;
	char			checkName[MAX_PATH_LENGTH];
	vec3_t			scale, translate;
	glIndex_t		tmpXyzIndices[MD2_MAX_TRIANGLES*3], tmpStIndices[MD2_MAX_TRIANGLES*3];
	int				indexRemap[MD2_MAX_TRIANGLES*3];
	glIndex_t		indexTable[MD2_MAX_TRIANGLES*3];
	float			skinWidth, skinHeight;
	int				numIndices;
	int				id, version;
	int				i, j;

	// Load the file
	FS_ReadFile(name, (void **)&data);
	if (!data)
		return false;

	*model = outModel = (mdl_t *)Mem_Alloc(sizeof(mdl_t), TAG_RENDERER);
	*size = sizeof(mdl_t);

	// Byte swap the header fields and sanity check
	inHeader = (md2Header_t *)data;

	id = LittleLong(inHeader->id);
	if (id != MD2_ID)
		Com_Error(ERR_DROP, "R_LoadMD2Model: wrong file id (%s)", name);

	version = LittleLong(inHeader->version);
	if (version != MD2_VERSION)
		Com_Error(ERR_DROP, "R_LoadMD2Model: wrong version number (%i should be %i) (%s)", version, MD2_VERSION, name);

	// Check the number of frames
	outModel->numFrames = LittleLong(inHeader->numFrames);
	if (outModel->numFrames > MD2_MAX_FRAMES || outModel->numFrames <= 0)
		Com_Error(ERR_DROP, "R_LoadMD2Model: invalid number of frames (%i) (%s)", outModel->numFrames, name);

	// Clear tags and surfaces
	outModel->numTags = 0;
	outModel->numSurfaces = 1;

	// Check and get skin dimensions
	skinWidth = (float)LittleLong(inHeader->skinWidth);
	if (skinWidth <= 0.0f)
		Com_Error(ERR_DROP, "R_LoadMD2Model: invalid skin width (%i) (%s)", (int)skinWidth, name);

	skinHeight = (float)LittleLong(inHeader->skinHeight);
	if (skinHeight <= 0.0f)
		Com_Error(ERR_DROP, "R_LoadMD2Model: invalid skin height (%i) (%s)", (int)skinHeight, name);

	skinWidth = 1.0f / skinWidth;
	skinHeight = 1.0f / skinHeight;

	// Load the surfaces
	outModel->surfaces = outSurface = (mdlSurface_t *)Mem_Alloc(sizeof(mdlSurface_t), TAG_RENDERER);
	*size += sizeof(mdlSurface_t);

	// Check triangles
	outSurface->numTriangles = LittleLong(inHeader->numTris);
	if (outSurface->numTriangles > MD2_MAX_TRIANGLES || outSurface->numTriangles <= 0)
		Com_Error(ERR_DROP, "R_LoadMD2Model: invalid number of triangles (%i) (%s)", outSurface->numTriangles, name);

	// Check vertices
	outSurface->numVertices = LittleLong(inHeader->numXyz);
	if (outSurface->numVertices > MD2_MAX_VERTS || outSurface->numVertices <= 0)
		Com_Error(ERR_DROP, "R_LoadMD2Model: invalid number of vertices (%i) (%s)", outSurface->numVertices, name);

	// Check materials
	outSurface->numMaterials = LittleLong(inHeader->numSkins);
	if (outSurface->numMaterials > MD2_MAX_SKINS || outSurface->numMaterials < 0)
		Com_Error(ERR_DROP, "R_LoadMD2Model: invalid number of skins (%i) (%s)", outSurface->numMaterials, name);

	// Compute the triangle list
	inTriangle = (md2Triangle_t *)((byte *)inHeader + LittleLong(inHeader->ofsTris));

	for (i = 0; i < outSurface->numTriangles; i++){
		tmpXyzIndices[i*3+0] = (glIndex_t)LittleShort(inTriangle[i].indexXyz[0]);
		tmpXyzIndices[i*3+1] = (glIndex_t)LittleShort(inTriangle[i].indexXyz[1]);
		tmpXyzIndices[i*3+2] = (glIndex_t)LittleShort(inTriangle[i].indexXyz[2]);

		tmpStIndices[i*3+0] = (glIndex_t)LittleShort(inTriangle[i].indexSt[0]);
		tmpStIndices[i*3+1] = (glIndex_t)LittleShort(inTriangle[i].indexSt[1]);
		tmpStIndices[i*3+2] = (glIndex_t)LittleShort(inTriangle[i].indexSt[2]);
	}

	// Build list of unique vertices
	outSurface->numVertices = 0;

	numIndices = outSurface->numTriangles * 3;
	Mem_Fill(indexRemap, -1, MD2_MAX_TRIANGLES * 3 * sizeof(int));

	for (i = 0; i < numIndices; i++){
		if (indexRemap[i] != -1)
			continue;

		// Remap duplicates
		for (j = i + 1; j < numIndices; j++){
			if ((tmpXyzIndices[j] == tmpXyzIndices[i]) && (tmpStIndices[j] == tmpStIndices[i])){
				indexRemap[j] = i;
				indexTable[j] = outSurface->numVertices;
			}
		}

		// Add unique vertex
		indexRemap[i] = i;
		indexTable[i] = outSurface->numVertices++;
	}

	// Load the triangles
	outSurface->triangles = outTriangle = (mdlTriangle_t *)Mem_Alloc(outSurface->numTriangles * sizeof(mdlTriangle_t), TAG_RENDERER);
	*size += outSurface->numTriangles * sizeof(mdlTriangle_t);

	for (i = 0; i < numIndices; i += 3, outTriangle++){
		outTriangle->index[0] = indexTable[i+0];
		outTriangle->index[1] = indexTable[i+1];
		outTriangle->index[2] = indexTable[i+2];
	}

	// Load the vertices
	inSt = (md2St_t *)((byte *)inHeader + LittleLong(inHeader->ofsSt));
	outSurface->st = outSt = (mdlSt_t *)Mem_Alloc(outSurface->numVertices * sizeof(mdlSt_t), TAG_RENDERER);

	*size += outSurface->numVertices * sizeof(mdlSt_t);

	for (i = 0; i < numIndices; i++){
		if (indexRemap[i] != i)
			continue;

		outSt[indexTable[i]].st[0] = ((float)LittleShort(inSt[tmpStIndices[i]].s) + 0.5f) * skinWidth;
		outSt[indexTable[i]].st[1] = ((float)LittleShort(inSt[tmpStIndices[i]].t) + 0.5f) * skinHeight;
	}

	// Allocate space for face planes
	outSurface->facePlanes = (mdlFacePlane_t *)Mem_Alloc(outModel->numFrames * outSurface->numTriangles * sizeof(mdlFacePlane_t), TAG_RENDERER);
	*size += outModel->numFrames * outSurface->numTriangles * sizeof(mdlFacePlane_t);

	// Load the frames
	outModel->frames = outFrame = (mdlFrame_t *)Mem_Alloc(outModel->numFrames * sizeof(mdlFrame_t), TAG_RENDERER);
	*size += outModel->numFrames * sizeof(mdlFrame_t);

	outSurface->xyzNormals = outXyzNormal = (mdlXyzNormal_t *)Mem_Alloc(outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t), TAG_RENDERER);
	*size += outModel->numFrames * outSurface->numVertices * sizeof(mdlXyzNormal_t);

	for (i = 0; i < outModel->numFrames; i++, outFrame++, outXyzNormal += outSurface->numVertices){
		inFrame = (md2Frame_t *)((byte *)inHeader + LittleLong(inHeader->ofsFrames) + i*LittleLong(inHeader->frameSize));

		scale[0] = LittleFloat(inFrame->scale[0]);
		scale[1] = LittleFloat(inFrame->scale[1]);
		scale[2] = LittleFloat(inFrame->scale[2]);

		translate[0] = LittleFloat(inFrame->translate[0]);
		translate[1] = LittleFloat(inFrame->translate[1]);
		translate[2] = LittleFloat(inFrame->translate[2]);

		VectorCopy(translate, outFrame->mins);
		VectorMA(translate, 255.0f, scale, outFrame->maxs);

		outFrame->radius = RadiusFromBounds(outFrame->mins, outFrame->maxs);

		// Load XYZ vertices
		for (j = 0; j < numIndices; j++){
			if (indexRemap[j] != j)
				continue;

			outXyzNormal[indexTable[j]].xyz[0] = (float)inFrame->verts[tmpXyzIndices[j]].v[0] * scale[0] + translate[0];
			outXyzNormal[indexTable[j]].xyz[1] = (float)inFrame->verts[tmpXyzIndices[j]].v[1] * scale[1] + translate[1];
			outXyzNormal[indexTable[j]].xyz[2] = (float)inFrame->verts[tmpXyzIndices[j]].v[2] * scale[2] + translate[2];

			ByteToDir(inFrame->verts[tmpXyzIndices[j]].lightNormalIndex, outXyzNormal[indexTable[j]].normal);
		}

		// Calculate tangent vectors
		R_CalcTangentVectors(outSurface->numTriangles, outSurface->triangles, outSurface->numVertices, outSurface->xyzNormals, outSurface->st, i);

		// Calculate face planes
		R_CalcFacePlanes(outSurface->numTriangles, outSurface->triangles, outSurface->facePlanes, outSurface->numVertices, outSurface->xyzNormals, i);
	}

	// Build triangle neighbors
	R_BuildTriangleNeighbors(outSurface->numTriangles, outSurface->triangles);

	// FIXME: is this in the right place?

	// Clear index and vertex buffers
	outSurface->indexBuffer = NULL;
	outSurface->indexOffset = 0;

	outSurface->vertexBuffer = NULL;
	outSurface->vertexOffset = 0;

	// Load the skins
	outSurface->materials = outMaterial = (mdlMaterial_t *)Mem_Alloc(outSurface->numMaterials * sizeof(mdlMaterial_t), TAG_RENDERER);
	*size += outSurface->numMaterials * sizeof(mdlMaterial_t);

	for (i = 0; i < outSurface->numMaterials; i++, outMaterial++){
		Str_Copy(checkName, (char *)inHeader + LittleLong(inHeader->ofsSkins) + i*64, sizeof(checkName));
		Str_StripFileExtension(checkName);

		outMaterial->material = R_FindMaterial(checkName, MT_GENERIC, SURFACEPARM_LIGHTING);
	}

	// Calculate bounds
	R_CalcModelBounds((model_t *)model, outModel);

	// Free file data
	FS_FreeFile(data);

	// Try to cache the geometry in static index and vertex buffers
	R_CacheAliasModelGeometry(outModel, name);

	return true;
}


/*
 ==============================================================================

 SPRITE LOADING

 ==============================================================================
*/


/*
 ==================
 R_LoadSP2Model
 ==================
*/
static bool R_LoadSP2Model (const char *name, spr_t **model, int *size){

	byte		*data;
	sp2Header_t	*inModel;
	sp2Frame_t	*inFrame;
	spr_t		*outModel;
	sprFrame_t	*outFrame;
	char		checkName[MAX_PATH_LENGTH];
	int			id, version;
	int			width, height;
	int			i;

	// Load the file
	FS_ReadFile(name, (void **)&data);
	if (!data)
		return false;

	*model = outModel = (spr_t *)Mem_Alloc(sizeof(spr_t), TAG_RENDERER);
	*size = sizeof(spr_t);

	// Byte swap the header fields and sanity check
	inModel = (sp2Header_t *)data;

	id = LittleLong(inModel->id);
	if (id != SP2_ID)
		Com_Error(ERR_DROP, "R_LoadSP2Model: '%s' has wrong file id", name);

	version = LittleLong(inModel->version);
	if (version != SP2_VERSION)
		Com_Error(ERR_DROP, "R_LoadSP2Model: '%s' has wrong version number (%i should be %i)", name, version, SP2_VERSION);

	// Check the number of frames
	outModel->numFrames = LittleLong(inModel->numFrames);
	if (outModel->numFrames > SP2_MAX_FRAMES || outModel->numFrames <= 0)
		Com_Error(ERR_DROP, "R_LoadSP2Model: '%s' has invalid number of frames (%i)", name, outModel->numFrames);

	// Load the frames
	inFrame = inModel->frames;
	outModel->frames = outFrame = (sprFrame_t *)Mem_Alloc(outModel->numFrames * sizeof(sprFrame_t), TAG_RENDERER);

	*size += outModel->numFrames * sizeof(sprFrame_t);

	for (i = 0; i < outModel->numFrames; i++, inFrame++, outFrame++){
		// Byte swap everything
		width = LittleLong(inFrame->width);
		height = LittleLong(inFrame->height);

		// Load the shader
		Str_Copy(checkName, inFrame->name, sizeof(checkName));
		Str_StripFileExtension(checkName);

		outFrame->material = R_FindMaterial(checkName, MT_GENERIC, SURFACEPARM_NONE);

		// Calculate radius
		outFrame->radius = sqrt((float)width * (float)width + (float)height * (float)height);
	}

	// Free file data
	FS_FreeFile(data);

	return true;
}


/*
 ==============================================================================

 MODEL REGISTRATION

 ==============================================================================
*/


/*
 ==================
 R_LoadModel
 ==================
*/
static model_t *R_LoadModel (const char *name, bool defaulted, modelType_t type, void *data, int size){

	model_t	*model;
	uint	hashKey;

	if (r_numModels == MAX_MODELS)
		Com_Error(ERR_DROP, "R_LoadModel: MAX_MODELS hit");

	r_models[r_numModels++] = model = (model_t *)Mem_Alloc(sizeof(model_t), TAG_RENDERER);

	// Fill it in
	Str_Copy(model->name, name, sizeof(model->name));
	model->defaulted = defaulted;
	model->type = type;
	model->data = data;
	model->size = size;
	model->alias = (mdl_t *)data;

	// Add to hash table
	hashKey = Str_HashKey(model->name, MODELS_HASH_SIZE, false);

	model->nextHash = r_modelsHashTable[hashKey];
	r_modelsHashTable[hashKey] = model;

	return model;
}

/*
 ==================
 R_FindModel
 ==================
*/
model_t *R_FindModel (const char *name){

	model_t	*model;
	char	extension[MAX_PATH_LENGTH >> 4];
	void	*data;
	int		size;
	int		i;
	uint	hashKey;

	// Check if already loaded
	hashKey = Str_HashKey(name, MODELS_HASH_SIZE, false);

	for (model = r_modelsHashTable[hashKey]; model; model = model->nextHash){
		if (!Str_ICompare(model->name, name))
			return model;
	}

	// Used by the BSP loading code for inline models
	if (name[0] == '*'){
		i = Str_ToInteger(name+1);
		if (!rg.worldModel || (i < 1 || i >= rg.worldModel->numInlineModels))
			Com_Error(ERR_DROP, "R_FindModel: bad inline model number (%i)", i);

		return &r_inlineModels[i];
	}

	// Load it from disk
	Str_ExtractFileExtension(name, extension, sizeof(extension));

	if (!Str_ICompare(extension, ".md3")){
		if (R_LoadMD3Model(name, (mdl_t **)&data, &size))
			return R_LoadModel(name, false, MODEL_MD3, data, size);
	}

	if (!Str_ICompare(extension, ".md2")){
		if (R_LoadMD2Model(name, (mdl_t **)&data, &size))
			return R_LoadModel(name, false, MODEL_MD2, data, size);
	}

	if (!Str_ICompare(extension, ".sp2")){
		if (R_LoadSP2Model(name, (spr_t **)&data, &size))
			return R_LoadModel(name, false, MODEL_SP2, data, size);
	}

	// Not found
	return NULL;
}

/*
 ==================
 R_RegisterModel
 ==================
*/
model_t *R_RegisterModel (const char *name){

	model_t	*model;

	// TODO: remove this when the default model works
	if (Str_FindText(name, "maps", false))
		return NULL;

	model = R_FindModel(name);
	if (model)
		return model;

	// Register the name even if not found
	Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find model '%s', using default\n", name);

	return R_LoadModel(name, true, rg.defaultModel->type, rg.defaultModel->data, 0);
}


// ============================================================================


/*
 ==================
 R_ModelRadius
 ==================
*/
float R_ModelRadius (mdl_t *alias, renderEntity_t *entity){

	mdlFrame_t	*curFrame, *oldFrame;
	float		radius;

	if ((entity->frame < 0 || entity->frame >= alias->numFrames) || (entity->oldFrame < 0 || entity->oldFrame >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_ModelRadius: no such frame %i to %i (%s)\n", entity->frame, entity->oldFrame, entity->model->name);

		entity->frame = 0;
		entity->oldFrame = 0;
	}

	curFrame = alias->frames + entity->frame;
	oldFrame = alias->frames + entity->oldFrame;

	if (curFrame == oldFrame)
		radius = curFrame->radius;
	else {
		if (curFrame->radius > oldFrame->radius)
			radius = curFrame->radius;
		else
			radius = oldFrame->radius;
	}

	return radius;
}

/*
 ==================
 R_ModelMaterial
 ==================
*/
material_t *R_ModelMaterial (renderEntity_t *entity, mdlSurface_t *surface){

	material_t *material;

	if (entity->material)
		material = entity->material;
	else {
		if (surface->numMaterials){
			if (entity->skinIndex < 0 || entity->skinIndex >= surface->numMaterials){
				Com_DPrintf(S_COLOR_YELLOW "R_ModelMaterial: no such material %i (%s)\n", entity->skinIndex, entity->model->name);

				entity->skinIndex = 0;
			}

			material = surface->materials[entity->skinIndex].material;
		}
		else {
			Com_DPrintf(S_COLOR_YELLOW "R_ModelMaterial: no materials for surface (%s)\n", entity->model->name);

			material = rg.defaultMaterial;
		}
	}

	return material;
}

/*
 ==================
 R_ModelFrames
 ==================
*/
int R_ModelFrames (model_t *model){

	mdl_t	*alias = model->alias;

	if (model->type != MODEL_MD3 && model->type != MODEL_MD2)
		return 0;

	return alias->numFrames;
}

/*
 ==================
 R_ModelBounds
 ==================
*/
void R_ModelBounds (model_t *model, vec3_t mins, vec3_t maxs){

	VectorCopy(model->mins, mins);
	VectorCopy(model->maxs, maxs);
}

/*
 ==================
 R_LerpTag
 ==================
*/
bool R_LerpTag (tag_t *tag, model_t *model, int curFrame, int oldFrame, float backLerp, const char *tagName){

	mdl_t		*alias = model->alias;
	mdlTag_t	*curTag, *oldTag;
	int			i;

	if (model->type != MODEL_MD3)
		return false;

	// Find the tag
	for (i = 0; i < alias->numTags; i++){
		if (!Str_ICompare(alias->tags[i].name, tagName))
			break;
	}

	if (i == alias->numTags){
		Com_DPrintf(S_COLOR_YELLOW "R_LerpTag: no such tag %s (%s)\n", tagName, model->name);
		return false;
	}

	if ((curFrame < 0 || curFrame >= alias->numFrames) || (oldFrame < 0 || oldFrame >= alias->numFrames)){
		Com_DPrintf(S_COLOR_YELLOW "R_LerpTag: no such frame %i to %i (%s)\n", curFrame, oldFrame, model->name);
		return false;
	}

	curTag = alias->tags + alias->numTags * curFrame + i;
	oldTag = alias->tags + alias->numTags * oldFrame + i;

	// Interpolate origin
	VectorLerp(curTag->origin, oldTag->origin, backLerp, tag->origin);

	// Interpolate axes
	VectorLerp(curTag->axis[0], oldTag->axis[0], backLerp, tag->axis[0]);
	VectorLerp(curTag->axis[1], oldTag->axis[1], backLerp, tag->axis[1]);
	VectorLerp(curTag->axis[2], oldTag->axis[2], backLerp, tag->axis[2]);

	// Normalize axes
	VectorNormalize(tag->axis[0]);
	VectorNormalize(tag->axis[1]);
	VectorNormalize(tag->axis[2]);

	return true;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_WorldMapInfo_f
 ==================
*/
static void R_WorldMapInfo_f (){

	if (!rg.worldModel){
		Com_Printf("World map not loaded\n");
		return;
	}

	Com_Printf("\n");
	Com_Printf("%s\n", rg.worldModel->name);
	Com_Printf("----------------------------------------\n");
	Com_Printf("%5i KB in %8i model surfaces\n", (rg.worldModel->numModelSurfaces * sizeof(int)) >> 10, rg.worldModel->numModelSurfaces);
	Com_Printf("%5i KB in %8i inline models\n", (rg.worldModel->numInlineModels * sizeof(inlineModel_t)) >> 10, rg.worldModel->numInlineModels);
	Com_Printf("%5i KB in %8i vertices\n", (rg.worldModel->numVertices * sizeof(vertex_t)) >> 10, rg.worldModel->numVertices);
	Com_Printf("%5i KB in %8i surface edges\n", (rg.worldModel->numSurfEdges * sizeof(int)) >> 10, rg.worldModel->numSurfEdges);
	Com_Printf("%5i KB in %8i edges\n", (rg.worldModel->numEdges * sizeof(edge_t)) >> 10, rg.worldModel->numEdges);
	Com_Printf("%5i KB in %8i texture information\n", (rg.worldModel->numTexInfo * sizeof(texInfo_t)) >> 10, rg.worldModel->numTexInfo);
	Com_Printf("%5i KB in %8i surfaces\n", (rg.worldModel->numSurfaces * sizeof(surface_t)) >> 10, rg.worldModel->numSurfaces);
	Com_Printf("%5i KB in %8i marked surfaces\n", (rg.worldModel->numMarkSurfaces * sizeof(surface_t)) >> 10, rg.worldModel->numMarkSurfaces);
	Com_Printf("%5i KB in %8i planes\n", (rg.worldModel->numPlanes * sizeof(cplane_t)) >> 10, rg.worldModel->numPlanes);
	Com_Printf("%5i KB in %8i nodes\n", (rg.worldModel->numNodes * sizeof(node_t)) >> 10, rg.worldModel->numNodes);
	Com_Printf("%5i KB in %8i leafs\n", (rg.worldModel->numLeafs * sizeof(leaf_t)) >> 10, rg.worldModel->numLeafs);
	Com_Printf("%5i KB in %8i clusters\n", (rg.worldModel->vis->numClusters * sizeof(vis_t)) >> 10, rg.worldModel->vis->numClusters);
	Com_Printf("----------------------------------------\n");
	Com_Printf("%.2f MB of world map data\n", rg.worldModel->size * (1.0f / 1048576.0f));
	Com_Printf("\n");
}

/*
 ==================
 R_ListModels_f
 ==================
*/
static void R_ListModels_f (){

	model_t	*model;
	int		numTriangles, numVertices;
	int		bytes = 0;
	int		i, j;

	Com_Printf("\n");
	Com_Printf("      -srfs- -tris- -verts -size- -name-----------\n");

	for (i = 0; i < r_numModels; i++){
		model = r_models[i];

		bytes += model->size;

		Com_Printf("%4i: ", i);
		
		switch (model->type){
		case MODEL_INLINE:
			numTriangles = numVertices = 0;

			for (j = 0; j < model->numSurfaces; j++){
				numTriangles += model->surfaces[j].numTriangles;
				numVertices += model->surfaces[j].numVertices;
			}

			Com_Printf("%6i %6i %6i ", model->numSurfaces, numTriangles, numVertices);

			break;
		case MODEL_MD3:
		case MODEL_MD2:
			numTriangles = numVertices = 0;

			for (j = 0; j < model->alias->numSurfaces; j++){
				numTriangles += model->alias->surfaces[j].numTriangles;
				numVertices += model->alias->surfaces[j].numVertices;
			}

			Com_Printf("%6i %6i %6i ", model->alias->numSurfaces, numTriangles, numVertices);

			break;
		case MODEL_SP2:

			break;
		default:
			Com_Printf("?????? ?????? ?????? ");

			break;
		}

		Com_Printf("%5ik ", model->size >> 10);

		Com_Printf("%s%s\n", model->name, (model->defaulted) ? " (DEFAULTED)" : "");
	}

	Com_Printf("--------------------------------------------------\n");
	Com_Printf("%i total models\n", r_numModels);
	Com_Printf("%.2f MB of model data\n", bytes * (1.0f / 1048576.0f));
	Com_Printf("\n");
}

/*
 ==============================================================================

 DEFAULT MODEL

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void R_CreateDefaultModel (){

}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_InitModels
 ==================
*/
void R_InitModels (){

	// Add commands
	Cmd_AddCommand("worldMapInfo", R_WorldMapInfo_f, "Shows world map information", NULL);
	Cmd_AddCommand("listModels", R_ListModels_f, "Lists loaded models", NULL);

	// Create the default model
	R_CreateDefaultModel();

	// Set up the world entity
	rg.worldEntity = &rg.scene.entities[0];

	Mem_Fill(rg.worldEntity, 0, sizeof(renderEntity_t));

	rg.worldEntity->type = RE_MODEL;

	rg.worldEntity->index = 0;

	rg.worldEntity->model = rg.worldModel;

	VectorClear(rg.worldEntity->origin);
	Matrix3_Identity(rg.worldEntity->axis);

	rg.worldEntity->hasSubview = false;

	rg.worldEntity->depthHack = false;

	rg.worldEntity->allowInView = VIEW_ALL;
	rg.worldEntity->allowShadowInView = VIEW_ALL;

	rg.worldEntity->material = NULL;

	rg.worldEntity->materialParms[MATERIALPARM_RED] = 1.0f;
	rg.worldEntity->materialParms[MATERIALPARM_GREEN] = 1.0f;
	rg.worldEntity->materialParms[MATERIALPARM_BLUE] = 1.0f;
	rg.worldEntity->materialParms[MATERIALPARM_ALPHA] = 1.0f;
	rg.worldEntity->materialParms[MATERIALPARM_TIMEOFFSET] = 0.0f;
	rg.worldEntity->materialParms[MATERIALPARM_DIVERSITY] = 0.0f;
	rg.worldEntity->materialParms[MATERIALPARM_MISC] = 0.0f;
	rg.worldEntity->materialParms[MATERIALPARM_MODE] = 0.0f;
}

/*
 ==================
 R_ShutdownModels
 ==================
*/
void R_ShutdownModels (){

	// Remove commands
	Cmd_RemoveCommand("worldMapInfo");
	Cmd_RemoveCommand("listModels");

	// Clear world model and entity
	rg.worldModel = NULL;
	rg.worldEntity = NULL;

	// Clear model list
	Mem_Fill(r_modelsHashTable, 0, sizeof(r_modelsHashTable));
	Mem_Fill(r_models, 0, sizeof(r_models));

	r_numModels = 0;
}