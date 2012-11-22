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
// table.h - Lookup tables
//


#ifndef __TABLE_H__
#define __TABLE_H__


/*
 ==============================================================================

 Lookup Tables:

 Tables are used to map a floating point input value to a floating point output
 value, with optional wrap / clamp and interpolation.

 ==============================================================================
*/

// Finds a table with the given name. Returns -1 if not found.
int				LUT_FindTable (const char *name);

// Performs a lookup in the given table with the given index
float			LUT_LookupTable (int tableIndex, float lookupIndex);

// Initializes the lookup table manager
void			LUT_Init ();

// Shuts down the lookup table manager
void			LUT_Shutdown ();


#endif	// __TABLE_H__