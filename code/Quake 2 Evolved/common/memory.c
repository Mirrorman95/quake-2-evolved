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
// memory.c - Memory manager
//

// TODO:
// - there is an error with block link in the client (CL_LoadGameMedia -- Mem_TouchMemory)


#include "common.h"


#ifdef _DEBUG
#define DEBUG_MEMORY
#endif

#define MIN_FRAGMENT_SIZE			64

#define BLOCK_PADDING				0x0000000000000000
#define BLOCK_SENTINEL				0x65736F447265764F

#define SMALL_ZONE_SIZE				(4 << 20)
#define MEDIUM_ZONE_SIZE			(32 << 20)
#define LARGE_ZONE_SIZE				(512 << 20)

#define TINY_BLOCK_SIZE				80			// Upper limit for tiny block fill/copy/compare
#define IN_CACHE_BLOCK_SIZE			262144		// Upper limit for in-cache block fill/copy

typedef enum {
	ALLOC_SMALL_ZONE,
	ALLOC_MEDIUM_ZONE,
	ALLOC_LARGE_ZONE,
	ALLOC_HEAP
} memoryAllocator_t;

typedef struct memoryBlock_s {
	byte					allocator;
	byte					tag;				// Tag identifier
	short					overhead;			// Allocation overhead
	int						size;				// Block size (including this header, padding, and possibly tiny fragments)

	struct memoryBlock_s *	prev;
	struct memoryBlock_s *	next;
} memoryBlock_t;

typedef struct {
	const char *			name;
	memoryAllocator_t		allocator;

	void *					base;
	int						size;

	memoryBlock_t *			rover;

	memoryBlock_t			blockList;

	// Zone statistics
	int						allocBlocks;
	int						allocBytes;
	int						allocOverhead;
	int						allocPeak;
} memoryZone_t;

typedef struct {
	memoryAllocator_t		allocator;

	memoryBlock_t			blockList;

	// Heap statistics
	int						allocBlocks;
	int						allocBytes;
	int						allocOverhead;
	int						allocPeak;
} memoryHeap_t;

typedef struct {
	bool					initialized;

	memoryZone_t			smallZone;
	memoryZone_t			mediumZone;
	memoryZone_t			largeZone;

	memoryHeap_t			mainHeap;

	// Global statistics
	int						totalBlocks;
	int						totalBytes;
	int						totalOverhead;

	int						frameAllocBlocks;
	int						frameAllocBytes;
	int						frameFreeBlocks;
	int						frameFreeBytes;
} memory_t;

static memory_t				mem;

static const char *			mem_tagNames[TAG_MAX] = {"free", "client", "server", "common", "collision", "renderer", "sound", "GUI", "temporary"};

static cvar_t *				mem_checkIntegrity;
static cvar_t *				mem_showUsage;


/*
 ==============================================================================

 ZONE MEMORY ALLOCATOR

 ==============================================================================
*/


/*
 ==================
 Mem_ZoneCreate
 ==================
*/
static void Mem_ZoneCreate (memoryZone_t *zone, const char *name, memoryAllocator_t allocator, int size){

	memoryBlock_t	*block;

	// Allocate a big chunk of memory
	block = (memoryBlock_t *)malloc(size);
	if (!block)
		Com_Error(ERR_FATAL, "Failed to allocate %i MB for %s zone", size >> 20, name);

	// Set the entire zone to one free block
	zone->name = name;
	zone->allocator = allocator;
	zone->base = block;
	zone->size = size;
	zone->rover = block;
	zone->blockList.allocator = allocator;
	zone->blockList.tag = TAG_MAX;
	zone->blockList.overhead = 0;
	zone->blockList.size = 0;
	zone->blockList.prev = block;
	zone->blockList.next = block;
	zone->allocBlocks = 0;
	zone->allocBytes = 0;
	zone->allocOverhead = 0;
	zone->allocPeak = 0;

	block->allocator = zone->allocator;
	block->tag = TAG_FREE;
	block->overhead = 0;
	block->size = zone->size;
	block->prev = &zone->blockList;
	block->next = &zone->blockList;
}

/*
 ==================
 Mem_ZoneDestroy
 ==================
*/
static void Mem_ZoneDestroy (memoryZone_t *zone){

	if (!zone->base)
		return;

	// Free all the memory
	free(zone->base);
	zone->base = NULL;
}

/*
 ==================
 Mem_ZoneCheck
 ==================
*/
static void Mem_ZoneCheck (memoryZone_t *zone){

	memoryBlock_t	*block;

	for (block = zone->blockList.next; block != &zone->blockList; block = block->next){
		if (block->allocator != zone->allocator)
			Com_Error(ERR_FATAL, "Block has invalid allocator (%s zone)", zone->name);

		if (block->tag < TAG_FREE || block->tag >= TAG_MAX)
			Com_Error(ERR_FATAL, "Block has invalid tag (%s zone)", zone->name);

		if (block->prev->next != block || block->next->prev != block)
			Com_Error(ERR_FATAL, "Adjacent blocks do not have proper links (%s zone)", zone->name);

		if (block->next != &zone->blockList){
			if ((const byte *)block + block->size != (const byte *)block->next)
				Com_Error(ERR_FATAL, "Block size does not touch the next block (%s zone)", zone->name);

			if (block->tag == TAG_FREE && block->next->tag == TAG_FREE)
				Com_Error(ERR_FATAL, "Two contiguous free blocks (%s zone)", zone->name);
		}

#ifdef DEBUG_MEMORY

		if (block->tag != TAG_FREE){
			if (*(const qword *)((const byte *)block + sizeof(memoryBlock_t)) != BLOCK_SENTINEL)
				Com_Error(ERR_FATAL, "Block head sentinel trashed (%s zone)", zone->name);

			if (*(const qword *)((const byte *)block + block->size - sizeof(qword)) != BLOCK_SENTINEL)
				Com_Error(ERR_FATAL, "Block tail sentinel trashed (%s zone)", zone->name);
		}

#endif
	}
}

/*
 ==================
 Mem_ZoneAlloc
 ==================
*/
static memoryBlock_t *Mem_ZoneAlloc (memoryZone_t *zone, int size, memoryTag_t tag, bool align16){

	memoryBlock_t	*block, *fragment;
	memoryBlock_t	*start, *rover;
	int				allocSize, extraSize;

	// Account for header and padding, and align to 8-byte boundary
	if (align16)
		allocSize = ALIGN(size + 8 + sizeof(memoryBlock_t), 8);
	else
		allocSize = ALIGN(size + sizeof(memoryBlock_t), 8);

#ifdef DEBUG_MEMORY

	// Add space for head and tail sentinels
	allocSize += 16;

#endif

	// Scan through the block list looking for the first free block of
	// sufficient size
	block = rover = zone->rover;

	start = block->prev;

	while (1){
		if (rover == start){
			// Scanned all the way around the block list
			Com_DPrintf(S_COLOR_RED "Failed on allocation of %i bytes from the %s zone\n", allocSize, zone->name);

			return NULL;
		}

		if (rover->tag == TAG_FREE)
			rover = rover->next;
		else
			block = rover = rover->next;

		if (block->tag == TAG_FREE && block->size >= allocSize)
			break;
	}

	// Found a block big enough
	extraSize = block->size - allocSize;

	if (extraSize >= MIN_FRAGMENT_SIZE){
		// There will be a free fragment after the allocated block
		fragment = (memoryBlock_t *)((byte *)block + allocSize);

		fragment->allocator = zone->allocator;
		fragment->tag = TAG_FREE;
		fragment->overhead = 0;
		fragment->size = extraSize;
		fragment->prev = block;
		fragment->next = block->next;
		fragment->next->prev = fragment;

		block->size = allocSize;
		block->next = fragment;
	}

	// Next allocation will start looking here
	zone->rover = block->next;

	// Mark the block as allocated
	block->tag = tag;
	block->overhead = block->size - size;

	// Update zone statistics
	zone->allocBlocks++;
	zone->allocBytes += block->size;
	zone->allocOverhead += block->overhead;

	if (zone->allocBytes > zone->allocPeak)
		zone->allocPeak = zone->allocBytes;

	// Update global statistics
	mem.totalBlocks++;
	mem.totalBytes += block->size;
	mem.totalOverhead += block->overhead;

	mem.frameAllocBlocks++;
	mem.frameAllocBytes += block->size;

	// Development tool
	if (mem_checkIntegrity){
		if (mem_checkIntegrity->integerValue == 1 || mem_checkIntegrity->integerValue == 3)
			Mem_ZoneCheck(zone);
	}

	return block;
}

/*
 ==================
 Mem_ZoneFree
 ==================
*/
static void Mem_ZoneFree (memoryZone_t *zone, memoryBlock_t *block){

	memoryBlock_t	*other;

	// Update zone statistics
	zone->allocBlocks--;
	zone->allocBytes -= block->size;
	zone->allocOverhead -= block->overhead;

	// Update global statistics
	mem.totalBlocks--;
	mem.totalBytes -= block->size;
	mem.totalOverhead -= block->overhead;

	mem.frameFreeBlocks++;
	mem.frameFreeBytes += block->size;

	// Mark the block as freed
	block->tag = TAG_FREE;

	// If possible, merge with the previous free block
	if (block->prev->tag == TAG_FREE){
		other = block->prev;

		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;

		block = other;
	}

	// If possible, merge with the next free block
	if (block->next->tag == TAG_FREE){
		other = block->next;

		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
	}

	// Next allocation will start looking here
	zone->rover = block;

	// Development tool
	if (mem_checkIntegrity){
		if (mem_checkIntegrity->integerValue == 2 || mem_checkIntegrity->integerValue == 3)
			Mem_ZoneCheck(zone);
	}
}

/*
 ==================
 Mem_ZoneFreeAll
 ==================
*/
static int Mem_ZoneFreeAll (memoryZone_t *zone, memoryTag_t tag){

	int		bytes = 0;

	zone->rover = zone->blockList.next;

	while (1){
		if (zone->rover->tag == tag){
			bytes += zone->rover->size;

			Mem_ZoneFree(zone, zone->rover);

			continue;
		}

		zone->rover = zone->rover->next;

		if (zone->rover == &zone->blockList)
			break;
	}

	return bytes;
}

/*
 ==================
 Mem_ZoneStats
 ==================
*/
static void Mem_ZoneStats (memoryZone_t *zone, int tagBlocks[TAG_MAX], int tagBytes[TAG_MAX]){

	memoryBlock_t	*block;
	int				freeBlocks = 0, freeBytes = 0;
	int				maxAllocBytes = 0, maxFreeBytes = 0;

	for (block = zone->blockList.next; block != &zone->blockList; block = block->next){
		tagBlocks[block->tag]++;
		tagBytes[block->tag] += block->size;

		if (block->tag != TAG_FREE){
			if (block->size > maxAllocBytes)
				maxAllocBytes = block->size;

			continue;
		}

		freeBlocks++;
		freeBytes += block->size;

		if (block->size > maxFreeBytes)
			maxFreeBytes = block->size;
	}

	// Print zone statistics
	Com_Printf("\n");
	Com_Printf("%6.2f MB total %s zone:\n", zone->size * (1.0f / 1048576.0f), zone->name);
	Com_Printf("--------------------------------------------------------------\n");
	Com_Printf("    %9i bytes (%6.2f MB) in %5i allocated blocks\n", zone->allocBytes, zone->allocBytes * (1.0f / 1048576.0f), zone->allocBlocks);
	Com_Printf("    %9i bytes (%6.2f MB) in %5i free blocks\n", freeBytes, freeBytes * (1.0f / 1048576.0f), freeBlocks);

	if (zone->allocBlocks){
		Com_Printf("\n");
		Com_Printf("    %9i bytes (%6.2f MB) average allocated block\n", zone->allocBytes / zone->allocBlocks, (zone->allocBytes / zone->allocBlocks) * (1.0f / 1048576.0f));
		Com_Printf("    %9i bytes (%6.2f MB) largest allocated block\n", maxAllocBytes, maxAllocBytes * (1.0f / 1048576.0f));
	}

	if (freeBlocks){
		Com_Printf("\n");
		Com_Printf("    %9i bytes (%6.2f MB) average free block\n", freeBytes / freeBlocks, (freeBytes / freeBlocks) * (1.0f / 1048576.0f));
		Com_Printf("    %9i bytes (%6.2f MB) largest free block\n", maxFreeBytes, maxFreeBytes * (1.0f / 1048576.0f));
	}

	Com_Printf("\n");
	Com_Printf("    %9i %% fragmented\n", (freeBytes == 0) ? 0 : (freeBytes - maxFreeBytes) * 100 / freeBytes);
	Com_Printf("    %9i KB overhead\n", zone->allocOverhead >> 10);
	Com_Printf("    %9.2f MB peak usage\n", zone->allocPeak * (1.0f / 1048576.0f));
	Com_Printf("\n");
}

/*
 ==================
 Mem_ZoneDump
 ==================
*/
static void Mem_ZoneDump (memoryZone_t *zone, fileHandle_t f){

	memoryBlock_t	*block;

	FS_Printf(f, NEWLINE);

	for (block = zone->blockList.next; block != &zone->blockList; block = block->next)
		FS_Printf(f, "block: %8p, size: %9i, overhead: %2i, tag: %-10s" NEWLINE, block, block->size, block->overhead, mem_tagNames[block->tag]);

	FS_Printf(f, "---------------------------------------------------------------" NEWLINE);
	FS_Printf(f, "%.2f MB %s zone memory in %i blocks" NEWLINE, zone->allocBytes * (1.0f / 1048576.0f), zone->name, zone->allocBlocks);
	FS_Printf(f, "%i KB %s zone memory overhead" NEWLINE, zone->allocOverhead >> 10, zone->name);
	FS_Printf(f, NEWLINE);
}


/*
 ==============================================================================

 HEAP MEMORY ALLOCATOR

 ==============================================================================
*/


/*
 ==================
 Mem_HeapCreate
 ==================
*/
static void Mem_HeapCreate (memoryHeap_t *heap, memoryAllocator_t allocator){

	// Set up the heap
	heap->allocator = allocator;
	heap->blockList.allocator = allocator;
	heap->blockList.tag = TAG_MAX;
	heap->blockList.overhead = 0;
	heap->blockList.size = 0;
	heap->blockList.prev = &heap->blockList;
	heap->blockList.next = &heap->blockList;
	heap->allocBlocks = 0;
	heap->allocBytes = 0;
	heap->allocOverhead = 0;
	heap->allocPeak = 0;
}

/*
 ==================
 Mem_HeapDestroy
 ==================
*/
static void Mem_HeapDestroy (memoryHeap_t *heap){

	memoryBlock_t	*block, *next;

	if (!heap->blockList.prev || !heap->blockList.next)
		return;

	// Free all the memory
	for (block = heap->blockList.next; block != &heap->blockList; block = next){
		next = block->next;

		block->prev->next = block->next;
		block->next->prev = block->prev;

		free(block);
	}

	heap->blockList.prev = NULL;
	heap->blockList.next = NULL;
}

/*
 ==================
 Mem_HeapCheck
 ==================
*/
static void Mem_HeapCheck (memoryHeap_t *heap){

	memoryBlock_t	*block;

	for (block = heap->blockList.next; block != &heap->blockList; block = block->next){
		if (block->allocator != heap->allocator)
			Com_Error(ERR_FATAL, "Block has invalid allocator (heap)");

		if (block->tag <= TAG_FREE || block->tag >= TAG_MAX)
			Com_Error(ERR_FATAL, "Block has invalid tag (heap)");

		if (block->prev->next != block || block->next->prev != block)
			Com_Error(ERR_FATAL, "Adjacent blocks do not have proper links (heap)");

#ifdef DEBUG_MEMORY

		if (*(const qword *)((const byte *)block + sizeof(memoryBlock_t)) != BLOCK_SENTINEL)
			Com_Error(ERR_FATAL, "Block head sentinel trashed (heap)");

		if (*(const qword *)((const byte *)block + block->size - sizeof(qword)) != BLOCK_SENTINEL)
			Com_Error(ERR_FATAL, "Block tail sentinel trashed (heap)");

#endif
	}
}

/*
 ==================
 Mem_HeapAlloc
 ==================
*/
static memoryBlock_t *Mem_HeapAlloc (memoryHeap_t *heap, int size, memoryTag_t tag, bool align16){

	memoryBlock_t	*block;
	int				allocSize;

	// Account for header and padding, and align to 8-byte boundary
	if (align16)
		allocSize = ALIGN(size + 8 + sizeof(memoryBlock_t), 8);
	else
		allocSize = ALIGN(size + sizeof(memoryBlock_t), 8);

#ifdef DEBUG_MEMORY

	// Add space for head and tail sentinels
	allocSize += 16;

#endif

	// Allocate
	block = (memoryBlock_t *)malloc(allocSize);
	if (!block){
		Com_DPrintf(S_COLOR_RED "Failed on allocation of %i bytes from the heap\n", allocSize);

		return NULL;
	}

	// Fill in the block header
	block->allocator = heap->allocator;
	block->tag = tag;
	block->overhead = allocSize - size;
	block->size = allocSize;

	// Link it into the block chain
	block->prev = &heap->blockList;
	block->next = heap->blockList.next;

	heap->blockList.next->prev = block;
	heap->blockList.next = block;

	// Update heap statistics
	heap->allocBlocks++;
	heap->allocBytes += block->size;
	heap->allocOverhead += block->overhead;

	if (heap->allocBytes > heap->allocPeak)
		heap->allocPeak = heap->allocBytes;

	// Update global statistics
	mem.totalBlocks++;
	mem.totalBytes += block->size;
	mem.totalOverhead += block->overhead;

	mem.frameAllocBlocks++;
	mem.frameAllocBytes += block->size;

	// Development tool
	if (mem_checkIntegrity){
		if (mem_checkIntegrity->integerValue == 1 || mem_checkIntegrity->integerValue == 3)
			Mem_HeapCheck(heap);
	}

	return block;
}

/*
 ==================
 Mem_HeapFree
 ==================
*/
static void Mem_HeapFree (memoryHeap_t *heap, memoryBlock_t *block){

	// Update heap statistics
	heap->allocBlocks--;
	heap->allocBytes -= block->size;
	heap->allocOverhead -= block->overhead;

	// Update global statistics
	mem.totalBlocks--;
	mem.totalBytes -= block->size;
	mem.totalOverhead -= block->overhead;

	mem.frameFreeBlocks++;
	mem.frameFreeBytes += block->size;

	// Unlink it from the block chain
	block->prev->next = block->next;
	block->next->prev = block->prev;

	// Free
	free(block);

	// Development tool
	if (mem_checkIntegrity){
		if (mem_checkIntegrity->integerValue == 2 || mem_checkIntegrity->integerValue == 3)
			Mem_HeapCheck(heap);
	}
}

/*
 ==================
 Mem_HeapFreeAll
 ==================
*/
static int Mem_HeapFreeAll (memoryHeap_t *heap, memoryTag_t tag){

	memoryBlock_t	*block, *next;
	int				bytes = 0;

	for (block = heap->blockList.next; block != &heap->blockList; block = next){
		next = block->next;

		if (block->tag != tag)
			continue;

		bytes += block->size;

		Mem_HeapFree(heap, block);
	}

	return bytes;
}

/*
 ==================
 Mem_HeapStats
 ==================
*/
static void Mem_HeapStats (memoryHeap_t *heap, int tagBlocks[TAG_MAX], int tagBytes[TAG_MAX]){

	memoryBlock_t	*block;
	int				maxAllocBytes = 0;

	for (block = heap->blockList.next; block != &heap->blockList; block = block->next){
		tagBlocks[block->tag]++;
		tagBytes[block->tag] += block->size;

		if (block->size > maxAllocBytes)
			maxAllocBytes = block->size;
	}

	// Print heap statistics
	Com_Printf("\n");
	Com_Printf("Heap:\n");
	Com_Printf("--------------------------------------------------------------\n");
	Com_Printf("    %9i bytes (%6.2f MB) in %5i allocated blocks\n", heap->allocBytes, heap->allocBytes * (1.0f / 1048576.0f), heap->allocBlocks);

	if (heap->allocBlocks){
		Com_Printf("\n");
		Com_Printf("    %9i bytes (%6.2f MB) average block\n", heap->allocBytes / heap->allocBlocks, (heap->allocBytes / heap->allocBlocks) * (1.0f / 1048576.0f));
		Com_Printf("    %9i bytes (%6.2f MB) largest block\n", maxAllocBytes, maxAllocBytes * (1.0f / 1048576.0f));
	}

	Com_Printf("\n");
	Com_Printf("    %9i KB overhead\n", heap->allocOverhead >> 10);
	Com_Printf("    %9.2f MB peak usage\n", heap->allocPeak * (1.0f / 1048576.0f));
	Com_Printf("\n");
}

/*
 ==================
 Mem_HeapDump
 ==================
*/
static void Mem_HeapDump (memoryHeap_t *heap, fileHandle_t f){

	memoryBlock_t	*block;

	FS_Printf(f, NEWLINE);

	for (block = heap->blockList.next; block != &heap->blockList; block = block->next)
		FS_Printf(f, "block: %8p, size: %9i, overhead: %2i, tag: %-10s" NEWLINE, block, block->size, block->overhead, mem_tagNames[block->tag]);

	FS_Printf(f, "---------------------------------------------------------------" NEWLINE);
	FS_Printf(f, "%.2f MB heap memory in %i blocks" NEWLINE, heap->allocBytes * (1.0f / 1048576.0f), heap->allocBlocks);
	FS_Printf(f, "%i KB heap memory overhead" NEWLINE, heap->allocOverhead >> 10);
	FS_Printf(f, NEWLINE);
}


/*
 ==============================================================================

 MEMORY MANAGER INTERFACE

 ==============================================================================
*/


/*
 ==================
 Mem_Alloc
 ==================
*/
void *Mem_Alloc (int size, memoryTag_t tag){

	memoryBlock_t	*block;
	byte			*blockPtr;

	if (size < 0)
		Com_Error(ERR_FATAL, "Mem_Alloc: size < 0");

	if (tag <= TAG_FREE || tag >= TAG_MAX)
		Com_Error(ERR_FATAL, "Mem_Alloc: bad tag (%i)", tag);

	// If a temporary allocation, allocate it directly from the heap.
	// Otherwise try to allocate it from a zone selected depending on the
	// requested size.
	if (tag == TAG_TEMPORARY)
		block = Mem_HeapAlloc(&mem.mainHeap, size, tag, false);
	else {
		if (size < 256)
			block = Mem_ZoneAlloc(&mem.smallZone, size, tag, false);
		else if (size < 16384)
			block = Mem_ZoneAlloc(&mem.mediumZone, size, tag, false);
		else
			block = Mem_ZoneAlloc(&mem.largeZone, size, tag, false);

		// If the selected zone could not satisfy the allocation request,
		// allocate it directly from the heap
		if (!block)
			block = Mem_HeapAlloc(&mem.mainHeap, size, tag, false);
	}

	if (!block)
		Com_Error(ERR_FATAL, "Mem_Alloc: failed on allocation of %i bytes", size);

#ifdef DEBUG_MEMORY

	// Set the head sentinel
	*(qword *)((byte *)block + sizeof(memoryBlock_t)) = BLOCK_SENTINEL;

	// Set the tail sentinel
	*(qword *)((byte *)block + block->size - sizeof(qword)) = BLOCK_SENTINEL;

#endif

	// Get the pointer
	blockPtr = (byte *)block + sizeof(memoryBlock_t);

#ifdef DEBUG_MEMORY

	// Skip over the sentinel
	blockPtr += 8;

#endif

	return (void *)blockPtr;
}

/*
 ==================
 Mem_Alloc16
 ==================
*/
void *Mem_Alloc16 (int size, memoryTag_t tag){

	memoryBlock_t	*block;
	byte			*blockPtr;

	if (size < 0)
		Com_Error(ERR_FATAL, "Mem_Alloc16: size < 0");

	if (size & 15)
		Com_Error(ERR_FATAL, "Mem_Alloc16: size not multiple of 16");

	if (tag <= TAG_FREE || tag >= TAG_MAX)
		Com_Error(ERR_FATAL, "Mem_Alloc16: bad tag (%i)", tag);

	// If a temporary allocation, allocate it directly from the heap.
	// Otherwise try to allocate it from a zone selected depending on the
	// requested size.
	if (tag == TAG_TEMPORARY)
		block = Mem_HeapAlloc(&mem.mainHeap, size, tag, true);
	else {
		if (size < 256)
			block = Mem_ZoneAlloc(&mem.smallZone, size, tag, true);
		else if (size < 16384)
			block = Mem_ZoneAlloc(&mem.mediumZone, size, tag, true);
		else
			block = Mem_ZoneAlloc(&mem.largeZone, size, tag, true);

		// If the selected zone could not satisfy the allocation request,
		// allocate it directly from the heap
		if (!block)
			block = Mem_HeapAlloc(&mem.mainHeap, size, tag, true);
	}

	if (!block)
		Com_Error(ERR_FATAL, "Mem_Alloc16: failed on allocation of %i bytes", size);

#ifdef DEBUG_MEMORY

	// Set the head sentinel
	*(qword *)((byte *)block + sizeof(memoryBlock_t)) = BLOCK_SENTINEL;

	// Set the tail sentinel
	*(qword *)((byte *)block + block->size - sizeof(qword)) = BLOCK_SENTINEL;

#endif

	// Get the pointer
	blockPtr = (byte *)block + sizeof(memoryBlock_t);

#ifdef DEBUG_MEMORY

	// Skip over the sentinel
	blockPtr += 8;

#endif

	// Align on a 16-byte boundary if needed
	if (((dword)blockPtr) & 15){
		*(qword *)blockPtr = BLOCK_PADDING;

		// Skip over the padding
		blockPtr += 8;
	}

	return (void *)blockPtr;
}

/*
 ==================
 Mem_ClearedAlloc
 ==================
*/
void *Mem_ClearedAlloc (int size, memoryTag_t tag){

	memoryBlock_t	*block;
	byte			*blockPtr;

	if (size < 0)
		Com_Error(ERR_FATAL, "Mem_ClearedAlloc: size < 0");

	if (tag <= TAG_FREE || tag >= TAG_MAX)
		Com_Error(ERR_FATAL, "Mem_ClearedAlloc: bad tag (%i)", tag);

	// If a temporary allocation, allocate it directly from the heap.
	// Otherwise try to allocate it from a zone selected depending on the
	// requested size.
	if (tag == TAG_TEMPORARY)
		block = Mem_HeapAlloc(&mem.mainHeap, size, tag, false);
	else {
		if (size < 256)
			block = Mem_ZoneAlloc(&mem.smallZone, size, tag, false);
		else if (size < 16384)
			block = Mem_ZoneAlloc(&mem.mediumZone, size, tag, false);
		else
			block = Mem_ZoneAlloc(&mem.largeZone, size, tag, false);

		// If the selected zone could not satisfy the allocation request,
		// allocate it directly from the heap
		if (!block)
			block = Mem_HeapAlloc(&mem.mainHeap, size, tag, false);
	}

	if (!block)
		Com_Error(ERR_FATAL, "Mem_ClearedAlloc: failed on allocation of %i bytes", size);

#ifdef DEBUG_MEMORY

	// Set the head sentinel
	*(qword *)((byte *)block + sizeof(memoryBlock_t)) = BLOCK_SENTINEL;

	// Set the tail sentinel
	*(qword *)((byte *)block + block->size - sizeof(qword)) = BLOCK_SENTINEL;

#endif

	// Get the pointer
	blockPtr = (byte *)block + sizeof(memoryBlock_t);

#ifdef DEBUG_MEMORY

	// Skip over the sentinel
	blockPtr += 8;

#endif

	// Clear the block
	Mem_Fill(blockPtr, 0, size);

	return (void *)blockPtr;
}

/*
 ==================
 Mem_ClearedAlloc16
 ==================
*/
void *Mem_ClearedAlloc16 (int size, memoryTag_t tag){

	memoryBlock_t	*block;
	byte			*blockPtr;

	if (size < 0)
		Com_Error(ERR_FATAL, "Mem_ClearedAlloc16: size < 0");

	if (size & 15)
		Com_Error(ERR_FATAL, "Mem_ClearedAlloc16: size not multiple of 16");

	if (tag <= TAG_FREE || tag >= TAG_MAX)
		Com_Error(ERR_FATAL, "Mem_ClearedAlloc16: bad tag (%i)", tag);

	// If a temporary allocation, allocate it directly from the heap.
	// Otherwise try to allocate it from a zone selected depending on the
	// requested size.
	if (tag == TAG_TEMPORARY)
		block = Mem_HeapAlloc(&mem.mainHeap, size, tag, true);
	else {
		if (size < 256)
			block = Mem_ZoneAlloc(&mem.smallZone, size, tag, true);
		else if (size < 16384)
			block = Mem_ZoneAlloc(&mem.mediumZone, size, tag, true);
		else
			block = Mem_ZoneAlloc(&mem.largeZone, size, tag, true);

		// If the selected zone could not satisfy the allocation request,
		// allocate it directly from the heap
		if (!block)
			block = Mem_HeapAlloc(&mem.mainHeap, size, tag, true);
	}

	if (!block)
		Com_Error(ERR_FATAL, "Mem_ClearedAlloc16: failed on allocation of %i bytes", size);

#ifdef DEBUG_MEMORY

	// Set the head sentinel
	*(qword *)((byte *)block + sizeof(memoryBlock_t)) = BLOCK_SENTINEL;

	// Set the tail sentinel
	*(qword *)((byte *)block + block->size - sizeof(qword)) = BLOCK_SENTINEL;

#endif

	// Get the pointer
	blockPtr = (byte *)block + sizeof(memoryBlock_t);

#ifdef DEBUG_MEMORY

	// Skip over the sentinel
	blockPtr += 8;

#endif

	// Align on a 16-byte boundary if needed
	if (((dword)blockPtr) & 15){
		*(qword *)blockPtr = BLOCK_PADDING;

		// Skip over the padding
		blockPtr += 8;
	}

	// Clear the block
	Mem_Fill(blockPtr, 0, size);

	return (void *)blockPtr;
}

/*
 ==================
 Mem_Free
 ==================
*/
void Mem_Free (const void *ptr){

	memoryBlock_t	*block;
	const byte		*blockPtr;

	if (!ptr)
		Com_Error(ERR_FATAL, "Mem_Free: NULL pointer");

	blockPtr = (const byte *)ptr;

	// Back up over any padding
	if (*(const qword *)(blockPtr - 8) == BLOCK_PADDING)
		blockPtr -= 8;

#ifdef DEBUG_MEMORY

	// Back up over the sentinel
	blockPtr -= 8;

#endif

	// Get the block
	block = (memoryBlock_t *)(blockPtr - sizeof(memoryBlock_t));

	if (block->tag == TAG_FREE)
		Com_Error(ERR_FATAL, "Mem_Free: tried to free a freed block");

#ifdef DEBUG_MEMORY

	// Check the head sentinel
	if (*(const qword *)((const byte *)block + sizeof(memoryBlock_t)) != BLOCK_SENTINEL)
		Com_Error(ERR_FATAL, "Mem_Free: block head sentinel trashed");

	// Check the tail sentinel
	if (*(const qword *)((const byte *)block + block->size - sizeof(qword)) != BLOCK_SENTINEL)
		Com_Error(ERR_FATAL, "Mem_Free: block tail sentinel trashed");

#endif

	// Free
	switch (block->allocator){
	case ALLOC_SMALL_ZONE:
		Mem_ZoneFree(&mem.smallZone, block);
		break;
	case ALLOC_MEDIUM_ZONE:
		Mem_ZoneFree(&mem.mediumZone, block);
		break;
	case ALLOC_LARGE_ZONE:
		Mem_ZoneFree(&mem.largeZone, block);
		break;
	case ALLOC_HEAP:
		Mem_HeapFree(&mem.mainHeap, block);
		break;
	default:
		Com_Error(ERR_FATAL, "Mem_Free: block has invalid allocator");
	}
}

/*
 ==================
 Mem_FreeAll
 ==================
*/
void Mem_FreeAll (memoryTag_t tag, bool reportLeaks){

	int		totalBytes = 0;

	// Free all zone blocks with the given tag
	totalBytes += Mem_ZoneFreeAll(&mem.smallZone, tag);
	totalBytes += Mem_ZoneFreeAll(&mem.mediumZone, tag);
	totalBytes += Mem_ZoneFreeAll(&mem.largeZone, tag);

	// Free all heap blocks with the given tag
	totalBytes += Mem_HeapFreeAll(&mem.mainHeap, tag);

	// Report leaks if desired
	if (!reportLeaks || !totalBytes)
		return;

	Com_Printf("Freed %.2f MB of leaked %s data\n", totalBytes * (1.0f / 1048576.0f), mem_tagNames[tag]);
}

/*
 ==================
 Mem_Size
 ==================
*/
int Mem_Size (const void *ptr){

	memoryBlock_t	*block;
	const byte		*blockPtr;

	if (!ptr)
		Com_Error(ERR_FATAL, "Mem_Size: NULL pointer");

	blockPtr = (const byte *)ptr;

	// Back up over any padding
	if (*(const qword *)(blockPtr - 8) == BLOCK_PADDING)
		blockPtr -= 8;

#ifdef DEBUG_MEMORY

	// Back up over the sentinel
	blockPtr -= 8;

#endif

	// Get the block
	block = (memoryBlock_t *)(blockPtr - sizeof(memoryBlock_t));

	if (block->tag == TAG_FREE)
		Com_Error(ERR_FATAL, "Mem_Size: freed block");

	return block->size - block->overhead;
}

/*
 ==================
 Mem_Fill
 ==================
*/
void Mem_Fill (void *dst, int value, int size){

#if defined SIMD_X86

	__m128i	xmm[4];
	byte	*dstPtr;
	dword	fillVal;
	int		count;

	dstPtr = (byte *)dst;

	fillVal = value & 0xFF;
	fillVal = fillVal | (fillVal << 8);
	fillVal = fillVal | (fillVal << 16);

	// If a tiny block fill, don't bother using SIMD
	if (size < TINY_BLOCK_SIZE){
		count = size >> 2;
		size &= 3;

		while (count--){
			*(dword *)dstPtr = fillVal;

			dstPtr += 4;
		}

		while (size--)
			*dstPtr++ = fillVal;

		return;
	}

	// Perform in-cache fill or uncached fill depending on block size
	if (size < IN_CACHE_BLOCK_SIZE){
		// Fill the registers
		if (fillVal == 0){
			xmm[0] = _mm_setzero_si128();
			xmm[1] = _mm_setzero_si128();
			xmm[2] = _mm_setzero_si128();
			xmm[3] = _mm_setzero_si128();
		}
		else {
			xmm[0] = _mm_set1_epi32(fillVal);
			xmm[1] = _mm_load_si128(&xmm[0]);
			xmm[2] = _mm_load_si128(&xmm[0]);
			xmm[3] = _mm_load_si128(&xmm[0]);
		}

		// Align destination on a 16-byte boundary if needed
		count = ((dword)dstPtr) & 15;
		if (count){
			count = 16 - count;
			size -= count;

			while (count--)
				*dstPtr++ = fillVal;
		}

		// Fill 64 bytes (one cache line) per iteration
		count = size >> 6;
		size &= 63;

		while (count--){
			_mm_store_si128((__m128i *)(dstPtr +  0), xmm[0]);
			_mm_store_si128((__m128i *)(dstPtr + 16), xmm[1]);
			_mm_store_si128((__m128i *)(dstPtr + 32), xmm[2]);
			_mm_store_si128((__m128i *)(dstPtr + 48), xmm[3]);

			dstPtr += 64;
		}

		// Fill any remaining bytes
		count = size >> 2;
		size &= 3;

		while (count--){
			*(dword *)dstPtr = fillVal;

			dstPtr += 4;
		}

		while (size--)
			*dstPtr++ = fillVal;

		return;
	}

	// Fill the registers
	if (fillVal == 0){
		xmm[0] = _mm_setzero_si128();
		xmm[1] = _mm_setzero_si128();
		xmm[2] = _mm_setzero_si128();
		xmm[3] = _mm_setzero_si128();
	}
	else {
		xmm[0] = _mm_set1_epi32(fillVal);
		xmm[1] = _mm_load_si128(&xmm[0]);
		xmm[2] = _mm_load_si128(&xmm[0]);
		xmm[3] = _mm_load_si128(&xmm[0]);
	}

	// Align destination on a 16-byte boundary if needed
	count = ((dword)dstPtr) & 15;
	if (count){
		count = 16 - count;
		size -= count;

		while (count--)
			*dstPtr++ = fillVal;
	}

	// Fill 64 bytes (one cache line) per iteration
	count = size >> 6;
	size &= 63;

	while (count--){
		_mm_stream_si128((__m128i *)(dstPtr +  0), xmm[0]);
		_mm_stream_si128((__m128i *)(dstPtr + 16), xmm[1]);
		_mm_stream_si128((__m128i *)(dstPtr + 32), xmm[2]);
		_mm_stream_si128((__m128i *)(dstPtr + 48), xmm[3]);

		dstPtr += 64;
	}

	// Flush the write-combine buffers
	_mm_sfence();

	// Fill any remaining bytes
	count = size >> 2;
	size &= 3;

	while (count--){
		*(dword *)dstPtr = fillVal;

		dstPtr += 4;
	}

	while (size--)
		*dstPtr++ = fillVal;

#else

	memset(dst, value, size);

#endif
}

/*
 ==================
 Mem_Copy
 ==================
*/
void Mem_Copy (void *dst, const void *src, int size){

#if defined SIMD_X86

	__m128i	xmm[4];
	byte	*dstPtr, *srcPtr;
	int		count;

	dstPtr = (byte *)dst;
	srcPtr = (byte *)src;

	// If a tiny block copy, don't bother using SIMD
	if (size < TINY_BLOCK_SIZE){
		count = size >> 2;
		size &= 3;

		while (count--){
			*(dword *)dstPtr = *(const dword *)srcPtr;

			dstPtr += 4;
			srcPtr += 4;
		}

		while (size--)
			*dstPtr++ = *srcPtr++;

		return;
	}

	// Perform in-cache copy or uncached copy depending on block size
	if (size < IN_CACHE_BLOCK_SIZE){
		// Prefetch a few cache lines ahead
		_mm_prefetch((const char *)(srcPtr +   0), _MM_HINT_NTA);
		_mm_prefetch((const char *)(srcPtr +  64), _MM_HINT_NTA);
		_mm_prefetch((const char *)(srcPtr + 128), _MM_HINT_NTA);
		_mm_prefetch((const char *)(srcPtr + 192), _MM_HINT_NTA);
		_mm_prefetch((const char *)(srcPtr + 256), _MM_HINT_NTA);
		_mm_prefetch((const char *)(srcPtr + 320), _MM_HINT_NTA);
		_mm_prefetch((const char *)(srcPtr + 384), _MM_HINT_NTA);
		_mm_prefetch((const char *)(srcPtr + 448), _MM_HINT_NTA);

		// Align destination on a 16-byte boundary if needed
		count = ((dword)dstPtr) & 15;
		if (count){
			count = 16 - count;
			size -= count;

			while (count--)
				*dstPtr++ = *srcPtr++;
		}

		// Copy 64 bytes (one cache line) per iteration
		count = size >> 6;
		size &= 63;

		while (count--){
			_mm_prefetch((const char *)(srcPtr + 512), _MM_HINT_NTA);

			xmm[0] = _mm_loadu_si128((const __m128i *)(srcPtr +  0));
			xmm[1] = _mm_loadu_si128((const __m128i *)(srcPtr + 16));
			xmm[2] = _mm_loadu_si128((const __m128i *)(srcPtr + 32));
			xmm[3] = _mm_loadu_si128((const __m128i *)(srcPtr + 48));

			_mm_store_si128((__m128i *)(dstPtr +  0), xmm[0]);
			_mm_store_si128((__m128i *)(dstPtr + 16), xmm[1]);
			_mm_store_si128((__m128i *)(dstPtr + 32), xmm[2]);
			_mm_store_si128((__m128i *)(dstPtr + 48), xmm[3]);

			dstPtr += 64;
			srcPtr += 64;
		}

		// Copy any remaining bytes
		count = size >> 2;
		size &= 3;

		while (count--){
			*(dword *)dstPtr = *(const dword *)srcPtr;

			dstPtr += 4;
			srcPtr += 4;
		}

		while (size--)
			*dstPtr++ = *srcPtr++;

		return;
	}

	// Prefetch a few cache lines ahead
	_mm_prefetch((const char *)(srcPtr +   0), _MM_HINT_NTA);
	_mm_prefetch((const char *)(srcPtr +  64), _MM_HINT_NTA);
	_mm_prefetch((const char *)(srcPtr + 128), _MM_HINT_NTA);
	_mm_prefetch((const char *)(srcPtr + 192), _MM_HINT_NTA);
	_mm_prefetch((const char *)(srcPtr + 256), _MM_HINT_NTA);
	_mm_prefetch((const char *)(srcPtr + 320), _MM_HINT_NTA);
	_mm_prefetch((const char *)(srcPtr + 384), _MM_HINT_NTA);
	_mm_prefetch((const char *)(srcPtr + 448), _MM_HINT_NTA);

	// Align destination on a 16-byte boundary if needed
	count = ((dword)dstPtr) & 15;
	if (count){
		count = 16 - count;
		size -= count;

		while (count--)
			*dstPtr++ = *srcPtr++;
	}

	// Copy 64 bytes (one cache line) per iteration
	count = size >> 6;
	size &= 63;

	while (count--){
		_mm_prefetch((const char *)(srcPtr + 512), _MM_HINT_NTA);

		xmm[0] = _mm_loadu_si128((const __m128i *)(srcPtr +  0));
		xmm[1] = _mm_loadu_si128((const __m128i *)(srcPtr + 16));
		xmm[2] = _mm_loadu_si128((const __m128i *)(srcPtr + 32));
		xmm[3] = _mm_loadu_si128((const __m128i *)(srcPtr + 48));

		_mm_stream_si128((__m128i *)(dstPtr +  0), xmm[0]);
		_mm_stream_si128((__m128i *)(dstPtr + 16), xmm[1]);
		_mm_stream_si128((__m128i *)(dstPtr + 32), xmm[2]);
		_mm_stream_si128((__m128i *)(dstPtr + 48), xmm[3]);

		dstPtr += 64;
		srcPtr += 64;
	}

	// Flush the write-combine buffers
	_mm_sfence();

	// Copy any remaining bytes
	count = size >> 2;
	size &= 3;

	while (count--){
		*(dword *)dstPtr = *(const dword *)srcPtr;

		dstPtr += 4;
		srcPtr += 4;
	}

	while (size--)
		*dstPtr++ = *srcPtr++;

#else

	memcpy(dst, src, size);

#endif
}

/*
 ==================
 Mem_Compare
 ==================
*/
bool Mem_Compare (const void *buffer1, const void *buffer2, int size){

#if defined SIMD_X86

	__m128i			xmm[8];
	const byte		*ptr1, *ptr2;
	dword			diff;
	ALIGN_16(dword	diffVec[4]);
	int				count;

	ptr1 = (const byte *)buffer1;
	ptr2 = (const byte *)buffer2;

	diff = 0;

	// If a tiny block compare, don't bother using SIMD
	if (size < TINY_BLOCK_SIZE){
		count = size >> 2;
		size &= 3;

		while (count--){
			diff |= *(const dword *)ptr1 ^ *(const dword *)ptr2;

			ptr1 += 4;
			ptr2 += 4;
		}

		while (size--)
			diff |= *ptr1++ ^ *ptr2++;

		if (diff)
			return false;

		return true;
	}

	// Prefetch a few cache lines ahead
	_mm_prefetch((const char *)(ptr1 +   0), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr2 +   0), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr1 +  64), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr2 +  64), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr1 + 128), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr2 + 128), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr1 + 192), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr2 + 192), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr1 + 256), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr2 + 256), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr1 + 320), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr2 + 320), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr1 + 384), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr2 + 384), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr1 + 448), _MM_HINT_NTA);
	_mm_prefetch((const char *)(ptr2 + 448), _MM_HINT_NTA);

	// Compare 64 bytes (one cache line) per iteration
	count = size >> 6;
	size &= 63;

	while (count--){
		_mm_prefetch((const char *)(ptr1 + 512), _MM_HINT_NTA);
		_mm_prefetch((const char *)(ptr2 + 512), _MM_HINT_NTA);

		xmm[0] = _mm_loadu_si128((const __m128i *)(ptr1 +  0));
		xmm[1] = _mm_loadu_si128((const __m128i *)(ptr2 +  0));
		xmm[2] = _mm_loadu_si128((const __m128i *)(ptr1 + 16));
		xmm[3] = _mm_loadu_si128((const __m128i *)(ptr2 + 16));
		xmm[4] = _mm_loadu_si128((const __m128i *)(ptr1 + 32));
		xmm[5] = _mm_loadu_si128((const __m128i *)(ptr2 + 32));
		xmm[6] = _mm_loadu_si128((const __m128i *)(ptr1 + 48));
		xmm[7] = _mm_loadu_si128((const __m128i *)(ptr2 + 48));

		xmm[0] = _mm_xor_si128(xmm[0], xmm[1]);
		xmm[2] = _mm_xor_si128(xmm[2], xmm[3]);
		xmm[4] = _mm_xor_si128(xmm[4], xmm[5]);
		xmm[6] = _mm_xor_si128(xmm[6], xmm[7]);

		xmm[1] = _mm_or_si128(xmm[0], xmm[2]);
		xmm[5] = _mm_or_si128(xmm[4], xmm[6]);

		_mm_store_si128((__m128i *)diffVec, _mm_or_si128(xmm[1], xmm[5]));

		if (diffVec[0] || diffVec[1] || diffVec[2] || diffVec[3])
			return false;

		ptr1 += 64;
		ptr2 += 64;
	}

	// Compare any remaining bytes
	count = size >> 2;
	size &= 3;

	while (count--){
		diff |= *(const dword *)ptr1 ^ *(const dword *)ptr2;

		ptr1 += 4;
		ptr2 += 4;
	}

	while (size--)
		diff |= *ptr1++ ^ *ptr2++;

	if (diff)
		return false;

	return true;

#else

	if (memcmp(buffer1, buffer2, size))
		return false;

	return true;

#endif
}

/*
 ==================
 Mem_DupString
 ==================
*/
char *Mem_DupString (const char *string, memoryTag_t tag){

	char	*buffer;
	int		length;

	if (!string)
		Com_Error(ERR_FATAL, "Mem_DupString: NULL string");

	length = Str_Length(string) + 1;

	buffer = (char *)Mem_Alloc(length, tag);
	Str_Copy(buffer, string, length);

	return buffer;
}

/*
 ==================
 Mem_DupData
 ==================
*/
void *Mem_DupData (const void *data, int size, memoryTag_t tag){

	void	*buffer;

	if (!data)
		Com_Error(ERR_FATAL, "Mem_DupData: NULL data");

	buffer = (void *)Mem_Alloc(size, tag);
	Mem_Copy(buffer, data, size);

	return buffer;
}

/*
 ==================
 Mem_TouchMemory
 ==================
*/
void Mem_TouchMemory (){

	memoryBlock_t	*block;
	volatile int	sum = 0;
	int				time;
	int				i;

	time = Sys_Milliseconds();

	// Check integrity of the zones
	Mem_ZoneCheck(&mem.smallZone);
	Mem_ZoneCheck(&mem.mediumZone);
	Mem_ZoneCheck(&mem.largeZone);

	// Check integrity of the heap
	Mem_HeapCheck(&mem.mainHeap);

	// Touch the small zone
	for (block = mem.smallZone.blockList.next; block != &mem.smallZone.blockList; block = block->next){
		if (block->tag == TAG_FREE)
			continue;

		// Only need to touch each page
		for (i = 0; i < block->size; i += 1024)
			sum += ((const byte *)block)[i];
	}

	// Touch the medium zone
	for (block = mem.mediumZone.blockList.next; block != &mem.mediumZone.blockList; block = block->next){
		if (block->tag == TAG_FREE)
			continue;

		// Only need to touch each page
		for (i = 0; i < block->size; i += 1024)
			sum += ((const byte *)block)[i];
	}

	// Touch the large zone
	for (block = mem.largeZone.blockList.next; block != &mem.largeZone.blockList; block = block->next){
		if (block->tag == TAG_FREE)
			continue;

		// Only need to touch each page
		for (i = 0; i < block->size; i += 1024)
			sum += ((const byte *)block)[i];
	}

	// Touch the heap
	for (block = mem.mainHeap.blockList.next; block != &mem.mainHeap.blockList; block = block->next){
		// We only need to touch each page
		for (i = 0; i < block->size; i += 1024)
			sum += ((const byte *)block)[i];
	}

	Com_Printf("%.2f MB of memory touched in %i msec\n", mem.totalBytes * (1.0f / 1048576.0f), Sys_Milliseconds() - time);
}


// ============================================================================


/*
 ==================
 Mem_PrintStats
 ==================
*/
void Mem_PrintStats (){

	if (mem_showUsage->integerValue){
		Com_Printf("total allocated memory: %6i = %6.2f MB (overhead: %4i KB)\n", mem.totalBlocks, mem.totalBytes * (1.0f / 1048576.0f), mem.totalOverhead >> 10);
		Com_Printf("frame alloc: %4i = %4i KB, frame free: %4i = %4i KB\n", mem.frameAllocBlocks, mem.frameAllocBytes >> 10, mem.frameFreeBlocks, mem.frameFreeBytes >> 10);
	}

	// Clear for next frame
	mem.frameAllocBlocks = mem.frameAllocBytes = 0;
	mem.frameFreeBlocks = mem.frameFreeBytes = 0;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================


*/
/*
 ==================
 Mem_Stats_f
 ==================
*/
static void Mem_Stats_f (){

	int		tagBlocks[TAG_MAX];
	int		tagBytes[TAG_MAX];
	int		i;

	Mem_Fill(tagBlocks, 0, sizeof(tagBlocks));
	Mem_Fill(tagBytes, 0, sizeof(tagBytes));

	// Print zone statistics and accumulate per-tag information
	Mem_ZoneStats(&mem.smallZone, tagBlocks, tagBytes);
	Mem_ZoneStats(&mem.mediumZone, tagBlocks, tagBytes);
	Mem_ZoneStats(&mem.largeZone, tagBlocks, tagBytes);

	// Print heap statistics and accumulate per-tag information
	Mem_HeapStats(&mem.mainHeap, tagBlocks, tagBytes);

	// Print totals
	Com_Printf("\n");
	Com_Printf("Totals:\n");
	Com_Printf("--------------------------------------------------------------\n");

	for (i = TAG_FREE + 1; i < TAG_MAX; i++)
		Com_Printf("%9i bytes (%6.2f MB) in %5i blocks of %10s data\n", tagBytes[i], tagBytes[i] * (1.0f / 1048576.0f), tagBlocks[i], mem_tagNames[i]);

	Com_Printf("--------------------------------------------------------------\n");
	Com_Printf("%i total memory blocks allocated\n", mem.totalBlocks);
	Com_Printf("%.2f MB total memory allocated (%i KB total overhead)\n", mem.totalBytes * (1.0f / 1048576.0f), mem.totalOverhead >> 10);
	Com_Printf("\n");
}

/*
 ==================
 Mem_Dump_f
 ==================
*/
static void Mem_Dump_f (){

	fileHandle_t	f;
	char			name[MAX_PATH_LENGTH];

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: memDump <fileName>\n");
		return;
	}

	Str_Copy(name, Cmd_Argv(1), sizeof(name));
	Str_DefaultFileExtension(name, sizeof(name), ".txt");

	FS_OpenFile(name, FS_WRITE, &f);
	if (!f){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}

	// Write zone information
	Mem_ZoneDump(&mem.smallZone, f);
	Mem_ZoneDump(&mem.mediumZone, f);
	Mem_ZoneDump(&mem.largeZone, f);

	// Write heap information
	Mem_HeapDump(&mem.mainHeap, f);

	FS_CloseFile(f);

	Com_Printf("Dumped memory information to %s\n", name);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 Mem_Init
 ==================
*/
void Mem_Init (){

	// Create the heap
	Mem_HeapCreate(&mem.mainHeap, ALLOC_HEAP);

	// Create the zones
	Mem_ZoneCreate(&mem.smallZone, "small", ALLOC_SMALL_ZONE, SMALL_ZONE_SIZE);
	Mem_ZoneCreate(&mem.mediumZone, "medium", ALLOC_MEDIUM_ZONE, MEDIUM_ZONE_SIZE);
	Mem_ZoneCreate(&mem.largeZone, "large", ALLOC_LARGE_ZONE, LARGE_ZONE_SIZE);

	// Register variables
	mem_checkIntegrity = CVar_Register("mem_checkIntegrity", "0", CVAR_INTEGER, CVAR_CHEAT, "Check integrity of the heap (1 = on each alloc, 2 = on each free, 3 = on each alloc and free)", 0, 3);
	mem_showUsage = CVar_Register("mem_showUsage", "0", CVAR_BOOL, CVAR_CHEAT, "Show total and per-frame memory usage", 0, 0);

	// Add commands
	Cmd_AddCommand("memStats", Mem_Stats_f, "Shows memory statistics", NULL);
	Cmd_AddCommand("memDump", Mem_Dump_f, "Dumps memory information to a file", NULL);

	mem.initialized = true;

	Com_Printf("Memory Manager Initialized\n");
}

/*
 ==================
 Mem_Shutdown
 ==================
*/
void Mem_Shutdown (){

	if (!mem.initialized)
		return;

	// Remove commands
	Cmd_RemoveCommand("memStats");
	Cmd_RemoveCommand("memDump");

	// Destroy the zones
	Mem_ZoneDestroy(&mem.largeZone);
	Mem_ZoneDestroy(&mem.mediumZone);
	Mem_ZoneDestroy(&mem.smallZone);

	// Destroy the heap
	Mem_HeapDestroy(&mem.mainHeap);

	Mem_Fill(&mem, 0, sizeof(memory_t));
}