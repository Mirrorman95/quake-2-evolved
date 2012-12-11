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
// r_arrayBuffer.c - Vertex array buffers
//


#include "r_local.h"


static arrayBuffer_t *		r_indexBufferList;
static arrayBuffer_t *		r_freeIndexBuffer;

static arrayBuffer_t *		r_vertexBufferList;
static arrayBuffer_t *		r_freeVertexBuffer;


/*
 ==============================================================================

 INDEX ARRAY BUFFERS

 ==============================================================================
*/


/*
 ==================
 R_AllocIndexBuffer
 ==================
*/
arrayBuffer_t *R_AllocIndexBuffer (const char *name, bool dynamic, int indexCount, const glIndex_t *indexData){

	arrayBuffer_t	*indexBuffer;

	if (dynamic){
		if (r_indexBuffers->integerValue != 2)
			return NULL;	// Not using dynamic index buffers
	}
	else {
		if (r_indexBuffers->integerValue < 1)
			return NULL;	// Not using static index buffers
	}

	if (!indexCount)
		return NULL;

	// Allocate an index buffer and remove it from the free list
	if (!r_freeIndexBuffer)
		return NULL;	// No free slots

	indexBuffer = r_freeIndexBuffer;
	r_freeIndexBuffer = indexBuffer->next;

	// Fill it in
	Str_Copy(indexBuffer->name, name, sizeof(indexBuffer->name));
	indexBuffer->dynamic = dynamic;
	indexBuffer->count = indexCount;
	indexBuffer->size = indexCount * sizeof(glIndex_t);
	indexBuffer->mapped = false;
	indexBuffer->frameUsed = 0;

	// Set array buffer usage
	if (indexBuffer->dynamic)
		indexBuffer->usage = GL_STREAM_DRAW;
	else
		indexBuffer->usage = GL_STATIC_DRAW;

	// Bind the array buffer
	GL_BindIndexBuffer(indexBuffer);

	// Allocate the data store
	qglBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->size, indexData, indexBuffer->usage);

	return indexBuffer;
}

/*
 ==================
 R_ReallocIndexBuffer
 ==================
*/
void R_ReallocIndexBuffer (arrayBuffer_t *indexBuffer, int indexCount, const glIndex_t *indexData){

	if (indexBuffer->mapped)
		Com_Error(ERR_DROP, "R_ReallocIndexBuffer: array buffer is currently mapped");

	// Fill it in
	indexBuffer->count = indexCount;
	indexBuffer->size = indexCount * sizeof(glIndex_t);
	indexBuffer->frameUsed = 0;

	// Bind the array buffer
	GL_BindIndexBuffer(indexBuffer);

	// Reallocate the data store
	qglBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->size, indexData, indexBuffer->usage);
}

/*
 ==================
 R_FreeIndexBuffer
 ==================
*/
void R_FreeIndexBuffer (arrayBuffer_t *indexBuffer){

	if (indexBuffer->mapped)
		Com_Error(ERR_DROP, "R_FreeIndexBuffer: array buffer is currently mapped");

	// Clear it
	indexBuffer->count = 0;
	indexBuffer->size = 0;
	indexBuffer->frameUsed = rg.frameCount;

	// Bind the array buffer
	GL_BindIndexBuffer(indexBuffer);

	// Free the data store
	qglBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, indexBuffer->usage);

	// Add the index buffer back to the free list
	indexBuffer->next = r_freeIndexBuffer;
	r_freeIndexBuffer = indexBuffer;
}

/*
 ==================
 R_UpdateIndexBuffer
 ==================
*/
bool R_UpdateIndexBuffer (arrayBuffer_t *indexBuffer, int indexOffset, int indexCount, const glIndex_t *indexData, bool discard, bool synchronize){

	glIndex_t	*indexPtr;

	if (indexBuffer->mapped)
		Com_Error(ERR_DROP, "R_UpdateIndexBuffer: array buffer is currently mapped");

	if (discard)
		indexBuffer->frameUsed = 0;

	// Bind the array buffer
	GL_BindIndexBuffer(indexBuffer);

	// Discard the entire buffer if desired
	if (discard)
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->size, NULL, indexBuffer->usage);

	// Update the specified range, synchronizing if desired
	if (synchronize)
		qglBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indexOffset * sizeof(glIndex_t), indexCount * sizeof(glIndex_t), indexData);
	else {
		indexPtr = (glIndex_t *)qglMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, indexOffset * sizeof(glIndex_t), indexCount * sizeof(glIndex_t), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		if (!indexPtr)
			return false;

		Mem_Copy(indexPtr, indexData, indexCount * sizeof(glIndex_t));

		if (!qglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER))
			return false;
	}

	return true;
}

/*
 ==================
 R_MapIndexBuffer
 ==================
*/
glIndex_t *R_MapIndexBuffer (arrayBuffer_t *indexBuffer, int indexOffset, int indexCount, bool discard, bool synchronize){

	glIndex_t	*indexPtr;

	if (indexBuffer->mapped)
		Com_Error(ERR_DROP, "R_MapIndexBuffer: array buffer is already mapped");

	if (discard)
		indexBuffer->frameUsed = 0;

	// Bind the array buffer
	GL_BindIndexBuffer(indexBuffer);

	// Discard the entire buffer if desired
	if (discard)
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->size, NULL, indexBuffer->usage);

	// Map the specified range, synchronizing if desired
	indexBuffer->mapped = true;

	if (synchronize)
		indexPtr = (glIndex_t *)qglMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, indexOffset * sizeof(glIndex_t), indexCount * sizeof(glIndex_t), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
	else
		indexPtr = (glIndex_t *)qglMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, indexOffset * sizeof(glIndex_t), indexCount * sizeof(glIndex_t), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

	return indexPtr;
}

/*
 ==================
 R_UnmapIndexBuffer
 ==================
*/
bool R_UnmapIndexBuffer (arrayBuffer_t *indexBuffer){

	if (!indexBuffer->mapped)
		Com_Error(ERR_DROP, "R_UnmapIndexBuffer: array buffer is already unmapped");

	// Bind the array buffer
	GL_BindIndexBuffer(indexBuffer);

	// Unmap it
	indexBuffer->mapped = false;

	if (!qglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER))
		return false;

	return true;
}


/*
 ==============================================================================

 VERTEX ARRAY BUFFERS

 ==============================================================================
*/


/*
 ==================
 R_AllocVertexBuffer
 ==================
*/
arrayBuffer_t *R_AllocVertexBuffer (const char *name, bool dynamic, int vertexCount, const glVertex_t *vertexData){

	arrayBuffer_t	*vertexBuffer;

	if (dynamic){
		if (r_vertexBuffers->integerValue != 2)
			return NULL;	// Not using dynamic vertex buffers
	}
	else {
		if (r_vertexBuffers->integerValue < 1)
			return NULL;	// Not using static vertex buffers
	}

	if (!vertexCount)
		return NULL;

	// Allocate a vertex buffer and remove it from the free list
	if (!r_freeVertexBuffer)
		return NULL;	// No free slots

	vertexBuffer = r_freeVertexBuffer;
	r_freeVertexBuffer = vertexBuffer->next;

	// Fill it in
	Str_Copy(vertexBuffer->name, name, sizeof(vertexBuffer->name));
	vertexBuffer->dynamic = dynamic;
	vertexBuffer->count = vertexCount;
	vertexBuffer->size = vertexCount * sizeof(glVertex_t);
	vertexBuffer->mapped = false;
	vertexBuffer->frameUsed = 0;

	// Set array buffer usage
	if (vertexBuffer->dynamic)
		vertexBuffer->usage = GL_STREAM_DRAW;
	else
		vertexBuffer->usage = GL_STATIC_DRAW;

	// Bind the array buffer
	GL_BindVertexBuffer(vertexBuffer);

	// Allocate the data store
	qglBufferData(GL_ARRAY_BUFFER, vertexBuffer->size, vertexData, vertexBuffer->usage);

	return vertexBuffer;
}

/*
 ==================
 R_ReallocVertexBuffer
 ==================
*/
void R_ReallocVertexBuffer (arrayBuffer_t *vertexBuffer, int vertexCount, const glVertex_t *vertexData){

	if (vertexBuffer->mapped)
		Com_Error(ERR_DROP, "R_ReallocVertexBuffer: array buffer is currently mapped");

	// Fill it in
	vertexBuffer->count = vertexCount;
	vertexBuffer->size = vertexCount * sizeof(glVertex_t);
	vertexBuffer->frameUsed = 0;

	// Bind the array buffer
	GL_BindVertexBuffer(vertexBuffer);

	// Reallocate the data store
	qglBufferData(GL_ARRAY_BUFFER, vertexBuffer->size, vertexData, vertexBuffer->usage);
}

/*
 ==================
 R_FreeVertexBuffer
 ==================
*/
void R_FreeVertexBuffer (arrayBuffer_t *vertexBuffer){

	if (vertexBuffer->mapped)
		Com_Error(ERR_DROP, "R_FreeVertexBuffer: array buffer is currently mapped");

	// Clear it
	vertexBuffer->count = 0;
	vertexBuffer->size = 0;
	vertexBuffer->frameUsed = rg.frameCount;

	// Bind the array buffer
	GL_BindVertexBuffer(vertexBuffer);

	// Free the data store
	qglBufferData(GL_ARRAY_BUFFER, 0, NULL, vertexBuffer->usage);

	// Add the vertex buffer back to the free list
	vertexBuffer->next = r_freeVertexBuffer;
	r_freeVertexBuffer = vertexBuffer;
}

/*
 ==================
 R_UpdateVertexBuffer
 ==================
*/
bool R_UpdateVertexBuffer (arrayBuffer_t *vertexBuffer, int vertexOffset, int vertexCount, const glVertex_t *vertexData, bool discard, bool synchronize){

	glVertex_t	*vertexPtr;

	if (vertexBuffer->mapped)
		Com_Error(ERR_DROP, "R_UpdateVertexBuffer: array buffer is currently mapped");

	if (discard)
		vertexBuffer->frameUsed = 0;

	// Bind the array buffer
	GL_BindVertexBuffer(vertexBuffer);

	// Discard the entire buffer if desired
	if (discard)
		qglBufferData(GL_ARRAY_BUFFER, vertexBuffer->size, NULL, vertexBuffer->usage);

	// Update the specified range, synchronizing if desired
	if (synchronize)
		qglBufferSubData(GL_ARRAY_BUFFER, vertexOffset * sizeof(glVertex_t), vertexCount * sizeof(glVertex_t), vertexData);
	else {
		vertexPtr = (glVertex_t *)qglMapBufferRange(GL_ARRAY_BUFFER, vertexOffset * sizeof(glVertex_t), vertexCount * sizeof(glVertex_t), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		if (!vertexPtr)
			return false;

		Mem_Copy(vertexPtr, vertexData, vertexCount * sizeof(glVertex_t));

		if (!qglUnmapBuffer(GL_ARRAY_BUFFER))
			return false;
	}

	return true;
}

/*
 ==================
 R_MapVertexBuffer
 ==================
*/
glVertex_t *R_MapVertexBuffer (arrayBuffer_t *vertexBuffer, int vertexOffset, int vertexCount, bool discard, bool synchronize){

	glVertex_t	*vertexPtr;

	if (vertexBuffer->mapped)
		Com_Error(ERR_DROP, "R_MapVertexBuffer: array buffer is already mapped");

	if (discard)
		vertexBuffer->frameUsed = 0;

	// Bind the array buffer
	GL_BindVertexBuffer(vertexBuffer);

	// Discard the entire buffer if desired
	if (discard)
		qglBufferData(GL_ARRAY_BUFFER, vertexBuffer->size, NULL, vertexBuffer->usage);

	// Map the specified range, synchronizing if desired
	vertexBuffer->mapped = true;

	if (synchronize)
		vertexPtr = (glVertex_t *)qglMapBufferRange(GL_ARRAY_BUFFER, vertexOffset * sizeof(glVertex_t), vertexCount * sizeof(glVertex_t), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
	else
		vertexPtr = (glVertex_t *)qglMapBufferRange(GL_ARRAY_BUFFER, vertexOffset * sizeof(glVertex_t), vertexCount * sizeof(glVertex_t), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

	return vertexPtr;
}

/*
 ==================
 R_UnmapVertexBuffer
 ==================
*/
bool R_UnmapVertexBuffer (arrayBuffer_t *vertexBuffer){

	if (!vertexBuffer->mapped)
		Com_Error(ERR_DROP, "R_UnmapVertexBuffer: array buffer is already unmapped");

	// Bind the array buffer
	GL_BindVertexBuffer(vertexBuffer);

	// Unmap it
	vertexBuffer->mapped = false;

	if (!qglUnmapBuffer(GL_ARRAY_BUFFER))
		return false;

	return true;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_ListIndexBuffers_f
 ==================
*/
static void R_ListIndexBuffers_f (){

	arrayBuffer_t	*indexBuffer;
	int				count = 0, bytes = 0;
	int				i;

	if (!r_indexBufferList){
		Com_Printf("Index buffers are disabled\n");
		return;
	}

	Com_Printf("      -type-- -size- -name-----------\n");

	for (i = 0, indexBuffer = r_indexBufferList; i < MAX_ARRAY_BUFFERS; i++, indexBuffer++){
		if (!indexBuffer->count)
			continue;

		count++;
		bytes += indexBuffer->size;

		Com_Printf("%4i: ", i);

		if (indexBuffer->dynamic)
			Com_Printf("DYNAMIC ");
		else
			Com_Printf("STATIC  ");

		Com_Printf("%5ik ", indexBuffer->size >> 10);

		Com_Printf("%s\n", indexBuffer->name);
	}

	Com_Printf("-------------------------------------\n");
	Com_Printf("%i total index buffers\n", count);
	Com_Printf("%.2f MB of index buffer data\n", bytes * (1.0f / 1048576.0f));
	Com_Printf("\n");
}

/*
 ==================
 R_ListVertexBuffers_f
 ==================
*/
static void R_ListVertexBuffers_f (){

	arrayBuffer_t	*vertexBuffer;
	int				count = 0, bytes = 0;
	int				i;

	if (!r_vertexBufferList){
		Com_Printf("Vertex buffers are disabled\n");
		return;
	}

	Com_Printf("      -type-- -size- -name-----------\n");

	for (i = 0, vertexBuffer = r_vertexBufferList; i < MAX_ARRAY_BUFFERS; i++, vertexBuffer++){
		if (!vertexBuffer->count)
			continue;

		count++;
		bytes += vertexBuffer->size;

		Com_Printf("%4i: ", i);

		if (vertexBuffer->dynamic)
			Com_Printf("DYNAMIC ");
		else
			Com_Printf("STATIC  ");

		Com_Printf("%5ik ", vertexBuffer->size >> 10);

		Com_Printf("%s\n", vertexBuffer->name);
	}

	Com_Printf("-------------------------------------\n");
	Com_Printf("%i total vertex buffers\n", count);
	Com_Printf("%.2f MB of vertex buffer data\n", bytes * (1.0f / 1048576.0f));
	Com_Printf("\n");
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_InitArrayBuffers
 ==================
*/
void R_InitArrayBuffers (){

	int		i;

	// Add commands
	Cmd_AddCommand("listIndexBuffers", R_ListIndexBuffers_f, "Lists index buffers", NULL);
	Cmd_AddCommand("listVertexBuffers", R_ListVertexBuffers_f, "Lists vertex buffers", NULL);

	// Allocate and set up the index buffer list
	if (r_indexBuffers->integerValue){
		r_indexBufferList = (arrayBuffer_t *)Mem_ClearedAlloc(MAX_ARRAY_BUFFERS * sizeof(arrayBuffer_t), TAG_RENDERER);

		for (i = 0; i < MAX_ARRAY_BUFFERS; i++){
			qglGenBuffers(1, &r_indexBufferList[i].bufferId);

			if (i < MAX_ARRAY_BUFFERS - 1)
				r_indexBufferList[i].next = &r_indexBufferList[i+1];
		}

		r_freeIndexBuffer = r_indexBufferList;
	}

	// Allocate and set up the vertex buffer list
	if (r_vertexBuffers->integerValue){
		r_vertexBufferList = (arrayBuffer_t *)Mem_ClearedAlloc(MAX_ARRAY_BUFFERS * sizeof(arrayBuffer_t), TAG_RENDERER);

		for (i = 0; i < MAX_ARRAY_BUFFERS; i++){
			qglGenBuffers(1, &r_vertexBufferList[i].bufferId);

			if (i < MAX_ARRAY_BUFFERS - 1)
				r_vertexBufferList[i].next = &r_vertexBufferList[i+1];
		}

		r_freeVertexBuffer = r_vertexBufferList;
	}
}

/*
 ==================
 R_ShutdownArrayBuffers
 ==================
*/
void R_ShutdownArrayBuffers (){

	int		i;

	// Remove commands
	Cmd_RemoveCommand("listIndexBuffers");
	Cmd_RemoveCommand("listVertexBuffers");

	// Delete all the index buffers
	if (r_indexBufferList){
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		for (i = 0; i < MAX_ARRAY_BUFFERS; i++)
			qglDeleteBuffers(1, &r_indexBufferList[i].bufferId);

		// Clear index buffer list
		r_indexBufferList = NULL;
		r_freeIndexBuffer = NULL;
	}

	// Delete all the vertex buffers
	if (r_vertexBufferList){
		qglBindBuffer(GL_ARRAY_BUFFER, 0);

		for (i = 0; i < MAX_ARRAY_BUFFERS; i++)
			qglDeleteBuffers(1, &r_vertexBufferList[i].bufferId);

		// Clear vertex buffer list
		r_vertexBufferList = NULL;
		r_freeVertexBuffer = NULL;
	}
}