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
// cl_cinematic.c - Cinematic playback
//

// TODO:
// - figure out how to handle framebuffers and then continue the work


#include "client.h"


typedef struct {
	bool					playing;

	bool					isRoQ;

	char					name[MAX_PATH_LENGTH];
	int						flags;

	fileHandle_t			file;
	int						size;
	int						offset;

	int						startTime;

	int						frameRate;

	int						frameCount;

	roqChunk_t				chunk;

	roqQuadVector_t			quadVectors[256];
	roqQuadCell_t			quadCells[256];
} cinematic_t;

static short				cin_v2rTable[256];
static short				cin_u2gTable[256];
static short				cin_v2gTable[256];
static short				cin_u2bTable[256];

static short				cin_sqrTable[256];

static short				cin_soundSamples[ROQ_CHUNK_MAX_SIZE >> 1];

static cinematic_t			cin_cinematics[MAX_CINEMATICS];


/*
 ==================
 
 ==================
*/
static void CIN_ResampleVideo (cinematic_t *cin){

}


/*
 ==============================================================================

 ROQ DECODING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void CIN_QuadVector4x4 (cinematic_t *cin, int x, int y, const byte *indices){

}

/*
 ==================
 
 ==================
*/
static void CIN_QuadVector8x8 (cinematic_t *cin, int x, int y, const byte *indices){

}

/*
 ==================
 
 ==================
*/
static void CIN_MotionBlock4x4 (cinematic_t *cin, int x, int y, int xMean, int yMean, int xyMotion){

}

/*
 ==================
 
 ==================
*/
static void CIN_MotionBlock8x8 (cinematic_t *cin, int x, int y, int xMean, int yMean, int xyMotion){

}

/*
 ==================
 
 ==================
*/
static void CIN_DecodeInfo (cinematic_t *cin, const byte *data){

}

/*
 ==================
 
 ==================
*/
static void CIN_DecodeCodebook (cinematic_t *cin, const byte *data){

}

/*
 ==================
 
 ==================
*/
static void CIN_DecodeVideo (cinematic_t *cin, const byte *data){

}

/*
 ==================
 CIN_DecodeSoundMono
 ==================
*/
static void CIN_DecodeSoundMono (cinematic_t *cin, const byte *data){

	int		prev;
	int		i;

	if (cin->flags & CIN_SILENT)
		return;

	prev = cin->chunk.flags;

	for (i = 0; i < cin->chunk.size; i++){
		prev = (short)(prev + cin_sqrTable[data[i]]);

		cin_soundSamples[i] = (short)prev;
	}

	// Submit the sound samples
	S_RawSamples(cin_soundSamples, cin->chunk.size, 22050, false, 1.0f);
}

/*
 ==================
 CIN_DecodeSoundStereo
 ==================
*/
static void CIN_DecodeSoundStereo (cinematic_t *cin, const byte *data){

	int		prevL, prevR;
	int		i;

	if (cin->flags & CIN_SILENT)
		return;

	prevL = (cin->chunk.flags & 0xFF00) << 0;
	prevR = (cin->chunk.flags & 0x00FF) << 8;

	for (i = 0; i < cin->chunk.size; i += 2){
		prevL = (short)(prevL + cin_sqrTable[data[i+0]]);
		prevR = (short)(prevR + cin_sqrTable[data[i+1]]);

		cin_soundSamples[i+0] = (short)prevL;
		cin_soundSamples[i+1] = (short)prevR;
	}

	// Submit the sound samples
	S_RawSamples(cin_soundSamples, cin->chunk.size >> 1, 22050, true, 1.0f);
}

/*
 ==================
 CIN_DecodeRoQChunk
 ==================
*/
static bool CIN_DecodeRoQChunk (cinematic_t *cin){

	byte	buffer[ROQ_CHUNK_HEADER_SIZE + ROQ_CHUNK_MAX_SIZE];
	byte	*data;

	if (cin->offset >= cin->size)
		return false;	// Finished

	data = buffer;

	// Read and decode the first chunk header if needed
	if (cin->offset == ROQ_CHUNK_HEADER_SIZE){
		cin->offset += FS_Read(cin->file, buffer, ROQ_CHUNK_HEADER_SIZE);

		cin->chunk.id = data[0] | (data[1] << 8);
		cin->chunk.size = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
		cin->chunk.flags = data[6] | (data[7] << 8);
	}

	// Read the chunk data and the next chunk header
	if (cin->chunk.size > ROQ_CHUNK_MAX_SIZE)
		Com_Error(ERR_DROP, "CIN_DecodeChunk: bad chunk size (%u)", cin->chunk.size);

	if (cin->offset + cin->chunk.size >= cin->size)
		cin->offset += FS_Read(cin->file, buffer, cin->chunk.size);
	else
		cin->offset += FS_Read(cin->file, buffer, cin->chunk.size + ROQ_CHUNK_HEADER_SIZE);

	// Decode the chunk data
	switch (cin->chunk.id){
	case ROQ_QUAD_INFO:
		CIN_DecodeInfo(cin, data);
		break;
	case ROQ_QUAD_CODEBOOK:
		CIN_DecodeCodebook(cin, data);
		break;
	case ROQ_QUAD_VQ:
		CIN_DecodeVideo(cin, data);
		break;
	case ROQ_SOUND_MONO:
		CIN_DecodeSoundMono(cin, data);
		break;
	case ROQ_SOUND_STEREO:
		CIN_DecodeSoundStereo(cin, data);
		break;
	default:
		Com_Error(ERR_DROP, "CIN_DecodeChunk: bad chunk id (%u)", cin->chunk.id);
	}

	data += cin->chunk.size;

	// Decode the next chunk header if needed
	if (cin->offset >= cin->size)
		return true;

	cin->chunk.id = data[0] | (data[1] << 8);
	cin->chunk.size = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
	cin->chunk.flags = data[6] | (data[7] << 8);

	return true;
}


/*
 ==============================================================================

 CIN DECODING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void CIN_DecodePalette (cinematic_t *cin){

}

/*
 ==================
 
 ==================
*/
static void CIN_DecodeCinVideo (cinematic_t *cin){

}

/*
 ==================
 
 ==================
*/
static void CIN_DecodeCinSound (cinematic_t *cin){

	byte	data[0x40000];
	int		start, end;
	int		samples;
	int		length;

	if (cin->flags & CIN_SILENT)
		return;
}

/*
 ==================
 CIN_DecodeCinChunk
 ==================
*/
static bool CIN_DecodeCinChunk (cinematic_t *cin){

	int		command;

	// Read the command
	FS_Read(cin->file, &command, sizeof(command));
	cin->offset += sizeof(command);

	command = LittleLong(command);
	if (command == 2)
		return false;	// Reached the end

	if (cin->offset >= cin->size)
		return false;	// Frame goes past the end

	// Decode the palette
	if (command == 1)
		CIN_DecodePalette(cin);

	if (cin->offset >= cin->size)
		return false;	// Invalid frame

	// Decode the video
	CIN_DecodeCinVideo(cin);

	if (cin->offset >= cin->size)
		return false;	// Invalid frame

	// Decode the sound
	CIN_DecodeCinSound(cin);

	cin->frameCount++;

	return true;
}


/*
 ==============================================================================

 PCX LOADING

 ==============================================================================
*/


/*
 ==================
 CIN_LoadPCX

 TODO: fill in cinematic_t
 ==================
*/
static bool CIN_LoadPCX (cinematic_t *cin, const char *name, int flags){

	pcxHeader_t	*header;
	byte		*data, *pcxData;
	byte		*in, *out;
	byte		palette[768];
	int			x, y, length;
	int			dataByte, runLength;

	// Load the file
	length = FS_ReadFile(name, (void **)&data);
	if (!data)
		return false;

	// Parse the PCX header
	header = (pcxHeader_t *)data;

    header->xMin = LittleShort(header->xMin);
    header->yMin = LittleShort(header->yMin);
    header->xMax = LittleShort(header->xMax);
    header->yMax = LittleShort(header->yMax);
    header->hRes = LittleShort(header->hRes);
    header->vRes = LittleShort(header->vRes);
    header->bytesPerLine = LittleShort(header->bytesPerLine);
    header->paletteType = LittleShort(header->paletteType);

	in = &header->data;

	if (header->manufacturer != 0x0A || header->version != 5 || header->encoding != 1)
		Com_Error(ERR_DROP, "CIN_LoadPCX: invalid PCX header (%s)\n", name);

	if (header->bitsPerPixel != 8 || header->colorPlanes != 1)
		Com_Error(ERR_DROP, "CIN_LoadPCX: only 8 bit PCX images supported (%s)\n", name);
		
	if (header->xMax <= 0 || header->yMax <= 0 || header->xMax >= 640 || header->yMax >= 480)
		Com_Error(ERR_DROP, "CIN_LoadPCX: bad image size (%i x %i) (%s)\n", header->xMax, header->yMax, name);

	Mem_Copy(palette, (byte *)data + length - 768, 768);

	pcxData = out = (byte *)Mem_Alloc((header->xMax+1) * (header->yMax+1) * 4, TAG_TEMPORARY);

	for (y = 0; y <= header->yMax; y++){
		for (x = 0; x <= header->xMax; ){
			dataByte = *in++;

			if ((dataByte & 0xC0) == 0xC0){
				runLength = dataByte & 0x3F;
				dataByte = *in++;
			}
			else
				runLength = 1;

			while (runLength-- > 0){
				out[0] = palette[dataByte*3+0];
				out[1] = palette[dataByte*3+1];
				out[2] = palette[dataByte*3+2];
				out[3] = 255;

				out += 4;
				x++;
			}
		}
	}

	if (in - data > length){
		Com_DPrintf(S_COLOR_YELLOW "CIN_LoadPCX: PCX file was malformed (%s)\n", name);

		FS_FreeFile(data);
		Mem_Free(pcxData);
		pcxData = NULL;

		return false;
	}

	// Free the file data
	FS_FreeFile(data);

	// Fill it in
	cin->playing = true;

	cin->isRoQ = false;

	Str_Copy(cin->name, name, sizeof(cin->name));
	cin->flags = flags;

	cin->file = 0;
	cin->size = 0;
	cin->offset = 0;

	cin->startTime = 0;

	cin->frameRate = 0;

	cin->frameCount = -1;

	// Resample video if needed
	CIN_ResampleVideo(cin);

	return true;
}


// ============================================================================


/*
 ==================
 CIN_HandleForCinematic
 ==================
*/
static cinematic_t *CIN_HandleForCinematic (int *handle){

	cinematic_t	*cin;
	int			i;

	for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++){
		if (!cin->playing)
			break;
	}

	if (i == MAX_CINEMATICS)
		Com_Error(ERR_DROP, "CIN_HandleForCinematic: none free");

	*handle = i + 1;

	return cin;
}

/*
 ==================
 CIN_GetCinematicByHandle
 ==================
*/
static cinematic_t *CIN_GetCinematicByHandle (int handle){

	cinematic_t	*cin;

	if (handle <= 0 || handle > MAX_CINEMATICS)
		Com_Error(ERR_DROP, "CIN_GetCinematicByHandle: handle out of range");

	cin = &cin_cinematics[handle - 1];

	if (!cin->playing)
		Com_Error(ERR_DROP, "CIN_GetCinematicByHandle: invalid handle");

	return cin;
}

/*
 ==================
 CIN_PlayCinematic

 TODO: read the .cin file header
 TODO: fill in cinematic_t
 ==================
*/
int	CIN_PlayCinematic (const char *name, int flags){

	cinematic_t		*cin;
	fileHandle_t	file;
	byte			buffer[ROQ_CHUNK_HEADER_SIZE];
	word			id, fps;
	char			checkName[MAX_PATH_LENGTH], loadName[MAX_PATH_LENGTH];
	bool			isRoQ;
	int				handle;
	int				size;
	int				i;

	// Check if already playing
	for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++){
		if (!cin->playing)
			continue;

		if (!Str_ICompare(cin->name, name)){
			if (cin->flags != flags)
				continue;

			return i + 1;
		}
	}

	// Get a free handle
	cin = CIN_HandleForCinematic(&handle);

	if (flags & CIN_SYSTEM){
		cls.playingCinematic = true;

		S_StopAllSounds();

		Com_Printf("Playing cinematic %s\n", name);
	}

	// Strip file extension
	Str_Copy(checkName, name, sizeof(checkName));
	Str_StripFileExtension(checkName);

	// Check for a static PCX image
	Str_SPrintf(loadName, sizeof(loadName), "%s.pcx", checkName);

	if (FS_FileExists(loadName)){
		if (!CIN_LoadPCX(cin, checkName, flags))
			return 0;

		return handle;
	}

	// Open the file
	Str_SPrintf(loadName, sizeof(loadName), "%s.RoQ", checkName);

	size = FS_OpenFile(loadName, FS_READ, &file);
	if (file)
		isRoQ = true;
	else {
		Str_SPrintf(loadName, sizeof(loadName), "%s.cin", checkName);

		size = FS_OpenFile(loadName, FS_READ, &file);
		if (file)
			isRoQ = false;
		else {
			if (flags & CIN_SYSTEM){
				cls.playingCinematic = false;

				Com_Printf("Cinematic %s not found\n", name);
			}

			return 0;
		}
	}

	if (isRoQ){
		// Read the file header
		FS_Read(file, buffer, ROQ_CHUNK_HEADER_SIZE);

		id = buffer[0] | (buffer[1] << 8);
		fps = buffer[6] | (buffer[7] << 8);

		if (id != ROQ_ID){
			FS_CloseFile(file);

			if (flags & CIN_SYSTEM){
				cls.playingCinematic = false;

				Com_Printf("Cinematic %s is not a RoQ file\n", name);
			}

			return 0;
		}

		// Fill it in
		cin->playing = true;

		cin->isRoQ = isRoQ;

		Str_Copy(cin->name, name, sizeof(cin->name));
		cin->flags = flags;

		cin->file = file;
		cin->size = size;
		cin->offset = ROQ_CHUNK_HEADER_SIZE;

		cin->startTime = 0;

		cin->frameRate = (fps) ? fps : 30;

		cin->frameCount = 0;
	}
	else {
		// Read the file header

		// Fill it in
		cin->playing = true;

		cin->isRoQ = isRoQ;

		Str_Copy(cin->name, name, sizeof(cin->name));
		cin->flags = flags;

		cin->file = file;
		cin->size = size;
		cin->offset = 20;

		cin->startTime = 0;

		cin->frameRate = 14;

		cin->frameCount = 0;
	}

	return handle;
}

/*
 ==================
 
 ==================
*/
cinData_t CIN_UpdateCinematic (int handle, int time){

	cinematic_t	*cin;
	cinData_t	data;
	int			frame;

	cin = CIN_GetCinematicByHandle(handle);

	// TODO: check if cin->frameCount == -1 (static image)

	// If we don't have a frame yet, set the start time
	if (!cin->frameCount)
		cin->startTime = time;

	// Check if a new frame is needed
	frame = (time - cin->startTime) * cin->frameRate / 1000;
	if (frame < 1)
		frame = 1;

	// TODO: check if dropping frames

	// Get the desired frame
	while (frame > cin->frameCount){
		// Decode a chunk
		if (cin->isRoQ){
			if (CIN_DecodeRoQChunk(cin))
				continue;
		}
		else {
			if (CIN_DecodeCinChunk(cin))
				continue;
		}

		// TODO!!!
	}

	// TODO!!!

	return data;
}

/*
 ==================
 
 ==================
*/
void CIN_StopCinematic (int handle){

	cinematic_t	*cin;

	cin = CIN_GetCinematicByHandle(handle);

	// Stop the cinematic
	if (cin->flags & CIN_SYSTEM){
		cls.playingCinematic = false;

		// Make sure sounds aren't playing
		S_StopAllSounds();
	}

	// Close the file
	if (cin->file)
		FS_CloseFile(cin->file);

	Mem_Fill(cin, 0, sizeof(cinematic_t));
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 CIN_PlayCinematic_f
 ==================
*/
static void CIN_PlayCinematic_f (){

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: playCinematic <name>\n");
		return;
	}

	// If running a local server, kill it
	SV_Shutdown("Server quit\n", false);

	// If connected to a server, disconnect
	CL_Disconnect(true);

	CL_PlayCinematic(Cmd_Argv(1));
}

/*
 ==================
 
 ==================
*/
static void CIN_ListCinematics_f (){

}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 CIN_Init
 ==================
*/
void CIN_Init (){

	float	f;
	short	s;
	int		i;

	// Add commands
	Cmd_AddCommand("playCinematic", CIN_PlayCinematic_f, "Plays a cinematic", Cmd_ArgCompletion_VideoName);
	Cmd_AddCommand("listCinematics", CIN_ListCinematics_f, "Lists playing cinematics", NULL);

	// Build YUV-to-RGB tables
	for (i = 0; i < 256; i++){
		f = (float)(i - 128);

		cin_v2rTable[i] = (short)(f *  1.40200f);
		cin_u2gTable[i] = (short)(f * -0.34414f);
		cin_v2gTable[i] = (short)(f * -0.71414f);
		cin_u2bTable[i] = (short)(f *  1.77200f);
	}

	// Build square table
	for (i = 0; i < 128; i++){
		s = (short)Square(i);

		cin_sqrTable[i] = s;
		cin_sqrTable[i+128] = -s;
	}
}

/*
 ==================
 
 ==================
*/
void CIN_Shutdown (){

	cinematic_t	*cin;
	int			i;

	// Remove commands
	Cmd_RemoveCommand("playCinematic");
	Cmd_RemoveCommand("listCinematics");

	// Stop all the cinematics
	cls.playingCinematic = false;

	for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++){
		if (!cin->playing)
			continue;

		// Close the file
		if (cin->file)
			FS_CloseFile(cin->file);
	}

	// Clear cinematic list
	Mem_Fill(cin_cinematics, 0, sizeof(cin_cinematics));
}