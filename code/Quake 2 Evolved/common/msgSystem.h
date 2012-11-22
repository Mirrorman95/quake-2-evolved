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
// msgSystem.h - Message I/O functions
//


#ifndef __MSGSYSTEM_H__
#define __MSGSYSTEM_H__


typedef struct {
	bool				allowOverflow;		// If false, do a Com_Error
	bool				overflowed;			// Set to true if the buffer size failed
	byte *				data;
	int					maxSize;
	int					curSize;
	int					readCount;
} msg_t;

void			MSG_Init (msg_t *msg, byte *data, int maxSize, bool allowOverflow);

void			MSG_Clear (msg_t *msg);
byte *			MSG_GetSpace (msg_t *msg, int length);
void			MSG_Write (msg_t *msg, const void *data, int length);
void			MSG_Print (msg_t *msg, const char *data);

// Writing functions
void			MSG_WriteChar (msg_t *msg, int value);
void			MSG_WriteByte (msg_t *msg, int value);
void			MSG_WriteShort (msg_t *msg, int value);
void			MSG_WriteLong (msg_t *msg, int value);
void			MSG_WriteFloat (msg_t *msg, float value);
void			MSG_WriteString (msg_t *msg, const char *string);
void			MSG_WriteCoord (msg_t *msg, float value);
void			MSG_WritePos (msg_t *msg, const vec3_t vec);
void			MSG_WriteAngle (msg_t *msg, float angle);
void			MSG_WriteAngle16 (msg_t *msg, float angle);
void			MSG_WriteDir (msg_t *msg, const vec3_t vec);
void			MSG_WriteDeltaUserCmd (msg_t *msg, const struct usercmd_s *from, const struct usercmd_s *to);
void			MSG_WriteDeltaEntity (msg_t *msg, const struct entity_state_s *from, const struct entity_state_s *to, bool force, bool newEntity);

// Reading functions
void			MSG_BeginReading (msg_t *msg);
int				MSG_ReadChar (msg_t *msg);
int				MSG_ReadByte (msg_t *msg);
int				MSG_ReadShort (msg_t *msg);
int				MSG_ReadLong (msg_t *msg);
float			MSG_ReadFloat (msg_t *msg);
char *			MSG_ReadString (msg_t *msg);
char *			MSG_ReadStringLine (msg_t *msg);
float			MSG_ReadCoord (msg_t *msg);
void			MSG_ReadPos (msg_t *msg, vec3_t pos);
float			MSG_ReadAngle (msg_t *msg);
float			MSG_ReadAngle16 (msg_t *msg);
void			MSG_ReadDir (msg_t *msg, vec3_t dir);
void			MSG_ReadDeltaUserCmd (msg_t *msg, const struct usercmd_s *from, struct usercmd_s *to);
void			MSG_ReadData (msg_t *msg, void *buffer, int size);


#endif	// __MSGSYSTEM_H__