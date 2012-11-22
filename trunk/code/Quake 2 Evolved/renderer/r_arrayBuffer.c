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


static arrayBuffer_t *		r_vertexBufferList;
static arrayBuffer_t *		r_freeVertexBuffer;


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
arrayBuffer_t *R_AllocVertexBuffer (const char *name, bool dynamic, int vertexCount){

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
	vertexBuffer->size = vertexCount;
	vertexBuffer->frameUsed = 0;

	// Set array buffer usage
	if (vertexBuffer->dynamic)
		vertexBuffer->usage = GL_STREAM_DRAW;
	else
		vertexBuffer->usage = GL_STATIC_DRAW;

	// Clear the vertex pointer
	backEnd.vertexPointer = NULL;

	// Bind the array buffer
	GL_BindVertexBuffer(vertexBuffer);

	// Allocate the data store
	qglBufferData(GL_ARRAY_BUFFER, vertexBuffer->size, NULL, vertexBuffer->usage);

	return vertexBuffer;
}

/*
 ==================
 R_UpdateVertexBuffer
 ==================
*/
void R_UpdateVertexBuffer (arrayBuffer_t *vertexBuffer, const void *data, int size){

	// Clear the vertex pointer
	backEnd.vertexPointer = NULL;

	// Bind the array buffer
	GL_BindVertexBuffer(vertexBuffer);

	// Allocate the data store
	qglBufferData(GL_ARRAY_BUFFER_ARB, size, data, vertexBuffer->usage);
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


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
	Cmd_AddCommand("listVertexBuffers", R_ListVertexBuffers_f, "Lists vertex buffers", NULL);

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
	Cmd_RemoveCommand("listVertexBuffers");

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