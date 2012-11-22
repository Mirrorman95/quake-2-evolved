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
// memory.h - Memory manager
//


#ifndef __MEMORY_H__
#define __MEMORY_H__


/*
 ==============================================================================

 Memory Manager:

 Dynamic memory may be allocated from one of three internal zones or directly
 from the heap.

 The memory manager will select the appropriate allocator depending on the
 requested size.

 None of the functions that allocate memory will ever return a NULL pointer.

 If an internal zone can't satisfy an allocation request (due to lack of free
 space or excessive fragmentation), memory will be allocated from the heap.
 In case the heap is also unable to allocate memory, the program will terminate
 immediately with an error message.

 All memory allocations are tagged to allow the memory manager to free all the
 memory blocks with the same tag through a single function call.

 Freeing a pointer may not actually release the memory. If it was allocated
 from one of the internal zones, the block will just be marked as reusable for
 any future allocations and possibly merged with contiguous free blocks to
 reduce fragmentation.

 Temporary allocations should only be used for file loading and any other
 operations that are temporary in nature.

 Memory blocks that are meant to be used for permanent or long-term storage
 should never be tagged as temporary because they will be freed automatically
 after loading a level or recovering from a non-fatal error.

 Temporary memory is always allocated from the heap to avoid fragmenting the
 internal zones during level loading.

 All memory allocations are always guaranteed to be 8-byte aligned unless
 16-byte alignment is explicitly requested.

 ==============================================================================
*/

typedef enum {
	TAG_FREE,
	TAG_CLIENT,
	TAG_SERVER,
	TAG_COMMON,
	TAG_COLLISION,
	TAG_RENDERER,
	TAG_SOUND,
	TAG_GUI,
	TAG_TEMPORARY,
	TAG_MAX
} memoryTag_t;

// Allocates a block of memory of the given size
void *			Mem_Alloc (int size, memoryTag_t tag);

// Allocates a block of memory of the given size.
// The returned pointer is aligned on a 16-byte boundary.
void *			Mem_Alloc16 (int size, memoryTag_t tag);

// Allocates a block of memory of the given size and clears it to zero
void *			Mem_ClearedAlloc (int size, memoryTag_t tag);

// Allocates a block of memory of the given size and clears it to zero.
// The returned pointer is aligned on a 16-byte boundary.
void *			Mem_ClearedAlloc16 (int size, memoryTag_t tag);

// Frees a block of memory
void			Mem_Free (const void *ptr);

// Frees all blocks of memory with the given tag
void			Mem_FreeAll (memoryTag_t tag, bool reportLeaks);

// Returns the size of the given block of memory
int				Mem_Size (const void *ptr);

// Fills a block of memory of the given size with the given value
void			Mem_Fill (void *dst, int value, int size);

// Copies a block of memory of the given size
void			Mem_Copy (void *dst, const void *src, int size);

// Compares two blocks of memory of the given size
bool			Mem_Compare (const void *buffer1, const void *buffer2, int size);

// Allocates a block of memory and copies the given string
char *			Mem_DupString (const char *string, memoryTag_t tag);

// Allocates a block of memory and copies the given data
void *			Mem_DupData (const void *data, int size, memoryTag_t tag);

// After loading a level, all the allocated memory must be touched to make sure
// it is paged in
void			Mem_TouchMemory ();

// Statistics for debugging and optimization
void			Mem_PrintStats ();

// Initializes the memory manager
void			Mem_Init ();

// Shuts down the memory manager
void			Mem_Shutdown ();


#endif	// __MEMORY_H__