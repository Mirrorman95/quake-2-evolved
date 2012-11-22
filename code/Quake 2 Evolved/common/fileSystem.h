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
// fileSystem.h - Handle based file system
//


#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__


/*
 ==============================================================================

 File System:

 All of Quake's data access is through a hierchal file system, but the 
 contents of the file system can be transparently merged from several 
 sources.

 The "base directory" is the path to the directory holding the 
 q2e.exe and all game directories. This can be overridden with the 
 "fs_homePath" variable to allow code debugging in a different
 directory. The base directory is only used during filesystem 
 initialization.

 The "game directory" is the first tree on the search path and directory 
 that all generated files (savegames, screenshots, demos, config files) 
 will be saved to. This can be overridden with the "fs_game" variable.
 The game directory can never be changed while Quake is executing. This 
 is a precacution against having a malicious server instruct clients to 
 write files over areas they shouldn't.

 ==============================================================================
*/

typedef int fileHandle_t;

typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND
} fsMode_t;

typedef enum {
	FS_SEEK_SET,
	FS_SEEK_CUR,
	FS_SEEK_END
} fsOrigin_t;

typedef enum {
	FIND_NO,
	FIND_PACK,
	FIND_DIRECTORY
} findFile_t;

typedef struct {
	const char *		directory;
	const char *		description;
} modList_t;

// Reads data from a file, properly handling partial reads.
// Returns the number of bytes read.
int				FS_Read (fileHandle_t f, void *buffer, int size);

// Writes data to a file, properly handling partial writes.
// Returns the number of bytes written.
int				FS_Write (fileHandle_t f, const void *buffer, int size);

// Writes formatted text to a file.
// Returns the number of characters written.
int				FS_Printf (fileHandle_t f, const char *fmt, ...);
int				FS_VPrintf (fileHandle_t f, const char *fmt, va_list argPtr);

// Returns the length of a file
int				FS_Length (fileHandle_t f);

// Returns the offset in a file
int				FS_Tell (fileHandle_t f);

// Seeks on a file
void			FS_Seek (fileHandle_t f, int offset, fsOrigin_t origin);

// Causes any buffered data to be written to a file
void			FS_Flush (fileHandle_t f);

// Forces flush on a file being written to
void			FS_ForceFlush (fileHandle_t f);

// Opens the given file for reading, writing, or appending. Will create any
// needed subdirectories.
// Returns file size, or -1 if the file wasn't found or an error occurred.
int				FS_OpenFile (const char *name, fsMode_t mode, fileHandle_t *f);

// Closes the given file
void			FS_CloseFile (fileHandle_t f);

// Copies the given file
bool			FS_CopyFile (const char *srcName, const char *dstName);

// Renames the given file
bool			FS_RenameFile (const char *oldName, const char *newName);

// Removes the given file
bool			FS_RemoveFile (const char *name);

// Returns true if the given file exists in the current game directory.
// This is to determine if writing a file will cause any overwrites.
bool			FS_FileExists (const char *name);

// Looks for a file in the search paths.
// Returns file availability and optionally sets the source path if found.
findFile_t		FS_FindFile (const char *name, char sourcePath[MAX_PATH_LENGTH]);

// Reads a complete file.
// Returns file size, or -1 if the file wasn't found or an error occurred.
// A NULL buffer will just return the file size without loading as a quick
// check for existance (-1 == not present).
// Appends a trailing 0, so string operations are always safe.
int				FS_ReadFile (const char *name, void **buffer);

// Frees the memory allocated by FS_ReadFile
void			FS_FreeFile (const void *buffer);

// Writes a complete file. Will create any needed subdirectories.
// Returns true if the file was written.
bool			FS_WriteFile (const char *name, const void *buffer, int size);

// Extracts the given dynamic library from a pack file and sets the fully
// qualified path to the extracted file (appending suffix and extension
// appropriate for the underlying platform).
// Returns false if the dynamic library could not be found or extracted.
bool			FS_ExtractLibrary (const char *baseName, char libPath[MAX_PATH_LENGTH]);

// Returns a list of files and subdirectories that match the given extension
// (which must include a leading '.' and must not contain wildcards).
// If extension is NULL, all the files will be returned and all the
// subdirectories ignored.
// If extension is "/", all the subdirectories will be returned and all the
// files ignored.
// The returned list can optionally be sorted.
const char **	FS_ListFiles (const char *path, const char *extension, bool sort, int *numFiles);

// Returns a list of files and subdirectories that match the given filter.
// The returned list can optionally be sorted.
const char **	FS_ListFilteredFiles (const char *filter, bool sort, int *numFiles);

// Frees the memory allocated by FS_ListFiles and FS_ListFilteredFiles
void			FS_FreeFileList (const char **fileList);

// Returns a list of game mods with all the data needed by the GUI.
// The returned list can optionally be sorted.
modList_t **	FS_ListMods (bool sort, int *numMods);

// Fress the memory allocated by FS_ListMods
void			FS_FreeModList (modList_t **modList);

// Adds to the current read count
void			FS_AddToReadCount (int count);

// Returns the current read count
int				FS_GetReadCount ();

// Resets the bytes read counter
void			FS_ResetReadCount ();

// Adds to the current write count
void			FS_AddToWriteCount (int count);

// Returns the current write count
int				FS_GetWriteCount ();

// Resets the bytes written counter
void			FS_ResetWriteCount ();

// Restarts the file system.
// Make sure all file I/O operations are terminated before calling this
// function!
void			FS_Restart ();

// Initializes the file system
void			FS_Init ();

// Shuts down the file system
void			FS_Shutdown ();


#endif	// __FILESYSTEM_H__