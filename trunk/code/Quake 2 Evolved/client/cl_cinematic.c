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

// TODO!!!
// - use handle based indices for drawing?
// - move huffman into it's own file?


#include "client.h"


typedef struct {
	char					name[MAX_OSPATH];
	int						flags;

	fileHandle_t			file;
	int						size;
	int						offset;
	int						header;

	bool					isRoQ;
	
	int						vidWidth;
	int						vidHeight;
	byte *					vidBuffer;

	int						rawWidth;
	int						rawHeight;
	byte *					rawBuffer;

	int						sndRate;
	int						sndWidth;
	int						sndChannels;

	int						frameRate;
	int						startTime;
	int						currentFrame;

	bool					playing;

	// PCX stuff
	byte *					pcxBuffer;

	// Huffman stuff
	byte *					hBuffer;

	uint					hPalette[256];

	int *					hNodes;
	int						hNumNodes[256];

	int						hUsed[512];
	int						hCount[512];

	// RoQ stuff
	byte *					roqCache;
	byte *					roqBuffers[2];

	roqChunk_t				roqChunk;
	roqQuadVector_t			roqCells[256];
	roqQuadCell_t			roqQuadCells[256];
} cinematic_t;

static short				cin_v2rTable[256];
static short				cin_u2gTable[256];
static short				cin_v2gTable[256];
static short				cin_u2bTable[256];

static short				cin_sqrTable[256];

static short				cin_soundSamples[ROQ_CHUNK_MAX_SIZE >> 1];

static cinematic_t			cin;


/*
 ==============================================================================

 HUFFMAN TABLES

 ==============================================================================
*/

/*
 ==================
 
 ==================
*/
static void CIN_SetupFile (){

}

/*
 ==================
 
 ==================
*/
static int CIN_SmallestNode (int numNodes){

}

/*
 ==================
 
 ==================
*/
static void CIN_SetupHuffTables (){

}

/*
 ==================
 
 ==================
*/
static void CIN_ReadPalette (){

}


/*
 ==============================================================================

 CINEMATIC DATA DECODING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void CIN_DecodeBlock (byte *dst0, byte *dst1, int y1, int y2, int y3, int y4, int u, int v){

}

/*
 ==================
 
 ==================
*/
static void CIN_ApplyVector2x2 (int x, int y, const roqQuadVector_t *cell){

}

/*
 ==================
 
 ==================
*/
static void CIN_ApplyVector4x4 (int x, int y, const roqQuadVector_t *cell){

}

/*
 ==================
 
 ==================
*/
static void CIN_ApplyMotion4x4 (int x, int y, byte mv){

}

/*
 ==================
 
 ==================
*/
static void CIN_ApplyMotion8x8 (int x, int y, byte mv){

}

/*
 ==================
 
 ==================
*/
static void CIN_ResampleVideoFrame (){

}

/*
 ==================
 
 ==================
*/
static void CIN_DecodeInfo (){

}

/*
 ==================
 
 ==================
*/
static void CIN_DecodeCodebook (){

}

/*
 ==================
 
 ==================
*/
static void CIN_DecodeVideo (const byte *data){

	roqChunk_t	*chunk = &cin.roqChunk;
}

/*
 ==================
 CIN_DecodeSoundMono
 ==================
*/
static void CIN_DecodeSoundMono (const byte *data){

	roqChunk_t	*chunk = &cin.roqChunk;
	int			prev;
	int			i;

	if (cin.flags & CIN_SILENT)
		return;

	prev = chunk->flags;

	for (i = 0; i < chunk->size; i++){
		prev = (short)(prev + cin_sqrTable[data[i]]);

		cin_soundSamples[i] = (short)prev;
	}

	// Submit the sound samples
	S_RawSamples(cin_soundSamples, chunk->size, 22050, false, 1.0f);
}

/*
 ==================
 CIN_DecodeSoundStereo
 ==================
*/
static void CIN_DecodeSoundStereo (const byte *data){

	roqChunk_t	*chunk = &cin.roqChunk;
	int			prevL, prevR;
	int			i;

	if (cin.flags & CIN_SILENT)
		return;

	prevL = (chunk->flags & 0xFF00) << 0;
	prevR = (chunk->flags & 0x00FF) << 8;

	for (i = 0; i < chunk->size; i += 2){
		prevL = (short)(prevL + cin_sqrTable[data[i+0]]);
		prevR = (short)(prevR + cin_sqrTable[data[i+1]]);

		cin_soundSamples[i+0] = (short)prevL;
		cin_soundSamples[i+1] = (short)prevR;
	}

	// Submit the sound samples
	S_RawSamples(cin_soundSamples, chunk->size >> 1, 22050, true, 1.0f);
}

/*
 ==================
 
 ==================
*/
static void CIN_DecodeSound (){

	byte	data[0x40000];
	int		start, end;
	int		samples;
	int		length;

	if (cin.flags & CIN_SILENT)
		return;

	start = cin.currentFrame * cin.sndRate/14;
	end = (cin.currentFrame+1) * cin.sndRate/14;
	samples = end - start;
	length = samples * cin.sndWidth * cin.sndChannels;

	FS_Read(cin.file, data, length);
	cin.offset += length;

	// Submit the sound samples
}

/*
 ==================
 CIN_DecodeRoQ
 ==================
*/
static bool CIN_DecodeRoQ (){

	roqChunk_t	*chunk = &cin.roqChunk;
	byte		buffer[ROQ_CHUNK_HEADER_SIZE + ROQ_CHUNK_MAX_SIZE];
	byte		*data;

	if (cin.offset >= cin.size)
		return false;	// Finished

	data = buffer;

	// Read and decode the first chunk header if needed
	if (cin.offset == ROQ_CHUNK_HEADER_SIZE){
		cin.offset += FS_Read(cin.file, buffer, ROQ_CHUNK_HEADER_SIZE);

		chunk->id = data[0] | (data[1] << 8);
		chunk->size = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
		chunk->flags = data[6] | (data[7] << 8);
	}

	// Read the chunk data and the next chunk header
	if (chunk->size > ROQ_CHUNK_MAX_SIZE)
		Com_Error(ERR_DROP, "CIN_DecodeChunk: bad chunk size (%u)", chunk->size);

	if (cin.offset + chunk->size >= cin.size)
		cin.offset += FS_Read(cin.file, buffer, chunk->size);
	else
		cin.offset += FS_Read(cin.file, buffer, chunk->size + ROQ_CHUNK_HEADER_SIZE);

	// Decode the chunk data
	switch (chunk->id){
	case ROQ_QUAD_INFO:
		CIN_DecodeInfo();
		break;
	case ROQ_QUAD_CODEBOOK:
		CIN_DecodeCodebook();
		break;
	case ROQ_QUAD_VQ:
		CIN_DecodeVideo(data);
		break;
	case ROQ_SOUND_MONO:
		CIN_DecodeSoundMono(data);
		break;
	case ROQ_SOUND_STEREO:
		CIN_DecodeSoundStereo(data);
		break;
	default:
		Com_Error(ERR_DROP, "CIN_DecodeChunk: bad chunk id (%u)", chunk->id);
	}

	data += chunk->size;

	// Decode the next chunk header if needed
	if (cin.offset >= cin.size)
		return true;

	chunk->id = data[0] | (data[1] << 8);
	chunk->size = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
	chunk->flags = data[6] | (data[7] << 8);

	return true;
}

/*
 ==================
 
 ==================
*/
static bool CIN_DecodeStaticCinematic (){

	return false;	// TODO: should be true
}


/*
 ==================
 CIN_DecodeChunk
 ==================
*/
static bool CIN_DecodeChunk (){

	if (cin.isRoQ){
		if (!CIN_DecodeRoQ())
			return false;
	}
	else {
		if (!CIN_DecodeStaticCinematic())
			return false;
	}

	return true;
}


/*
 ==============================================================================

 STATIC PCX CINEMATIC LOADING

 ==============================================================================
*/


/*
 ==================
 CIN_StaticCinematic
 ==================
*/
static bool CIN_StaticCinematic (const char *name, int flags){

	pcxHeader_t	*header;
	byte		*data, *image;
	byte		*in, *out;
	byte		palette[768];
	char		loadName[MAX_OSPATH];
	int			x, y, length;
	int			dataByte, runLength;

	Str_SPrintf(loadName, sizeof(loadName), "%s.pcx", name);

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
		Com_Error(ERR_DROP, "CIN_StaticCinematic: invalid PCX header (%s)\n", name);

	if (header->bitsPerPixel != 8 || header->colorPlanes != 1)
		Com_Error(ERR_DROP, "CIN_StaticCinematic: only 8 bit PCX images supported (%s)\n", name);
		
	if (header->xMax <= 0 || header->yMax <= 0 || header->xMax >= 640 || header->yMax >= 480)
		Com_Error(ERR_DROP, "CIN_StaticCinematic: bad image size (%i x %i) (%s)\n", header->xMax, header->yMax, name);

	Mem_Copy(palette, data + length - 768, 768);

	image = out = (byte *)Mem_ClearedAlloc((header->xMax+1) * (header->yMax+1) * 4, TAG_TEMPORARY);

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
		Com_DPrintf(S_COLOR_YELLOW "R_LoadPCX: PCX file was malformed (%s)\n", name);

		FS_FreeFile(data);
		Mem_Free(image);

		return false;
	}

	// Free the file data
	FS_FreeFile(data);

	// Setup the cinematic file
	CIN_SetupFile();

	// Resample if needed
	CIN_ResampleVideoFrame();

	return true;
}


// ============================================================================


/*
 ==================
 CIN_UpdateCinematic
 ==================
*/
bool CIN_UpdateCinematic (){

	int		frame;

	// Check if playing a cinematic
	if (!cin.playing)
		return false;

	// Static image
	if (cin.currentFrame == -1)
		return true;

	// If we don't have a frame yet, set the start time
	if (cin.currentFrame == 0)
		cin.startTime = cls.realTime;

	// Check if a new frame is needed
	frame = (cls.realTime - cin.startTime) * cin.frameRate / 1000;
	if (frame < 1)
		frame = 1;

	// Never drop too many frames in a row because it stalls
	if (frame > cin.currentFrame + 100 / cin.frameRate){
		cin.startTime = cls.realTime - cin.currentFrame * 1000 / cin.frameRate;

		frame = (cls.realTime - cin.startTime) * cin.frameRate / 1000;
		if (frame < 1)
			frame = 1;
	}

	// Get the desired frame
	while (frame > cin.currentFrame){
		// Read the next frame
		if (CIN_DecodeChunk())
			continue;

		// Make sure we don't get stuck into an infinite loop
		if (cin.currentFrame == 0)
			return false;

		// Restart the cinematic if needed
		if (cin.flags & CIN_LOOPING){
			FS_Seek(cin.file, cin.header, FS_SEEK_SET);
			cin.offset = cin.header;

			cin.startTime = cls.realTime;
			cin.currentFrame = 0;

			frame = 1;
			continue;
		}

		// Finished
		return false;
	}

	return true;
}

/*
 ==================
 
 ==================
*/
void CIN_DrawCinematic (){

	byte	*buffer;

	// Check if playing a cinematic
	if (!cin.playing)
		return;

	// Select a source buffer
	if (cin.rawWidth != cin.vidWidth || cin.rawHeight != cin.vidHeight)
		buffer = cin.rawBuffer;
	else
		buffer = cin.vidBuffer;

	// Update the cinematic texture

	// Draw it
}

/*
 ==================
 CIN_PlayCinematic
 ==================
*/
bool CIN_PlayCinematic (const char *name, int flags){

	fileHandle_t	file;
	byte			buffer[ROQ_CHUNK_HEADER_SIZE];
	word			id, fps;
	bool			isRoQ;
	char			checkName[MAX_OSPATH], loadName[MAX_OSPATH];
	int				size;

	if (!name || !name[0])
		Com_Error(ERR_DROP, "CIN_PlayCinematic: NULL cinematic name");

	if (Str_Length(name) >= MAX_OSPATH)
		Com_Error(ERR_DROP, "CIN_PlayCinematic: cinematic name exceeds MAX_OSPATH");

	// Check if playing a cinematic
	if (cin.playing)
		CIN_StopCinematic();

	if (flags & CIN_SYSTEM){
		// Make sure sounds aren't playing
		S_StopAllSounds();

		// Force menu and console off
		UI_SetActiveMenu(UI_CLOSEMENU);
		Con_Close();
	}

	// Strip extension
	Str_Copy(checkName, name, sizeof(checkName));
	Str_StripFileExtension(checkName);

	// Check for a static PCX image
	Str_SPrintf(loadName, sizeof(loadName), "%s.pcx", checkName);

	if (FS_FileExists(loadName)){
		if (!CIN_StaticCinematic(checkName, flags))
			return false;

		return true;
	}

	// Try to open a RoQ or CUN file
	Str_SPrintf(loadName, sizeof(loadName), "%s.RoQ", checkName);

	size = FS_OpenFile(loadName, FS_READ, &file);
	if (file)
		isRoQ = true;
	else {
		Str_SPrintf(loadName, sizeof(loadName), "%s.cin", checkName);

		size = FS_OpenFile(loadName, FS_READ, &file);
		if (file)
			isRoQ = false;
		else
			return false;
	}

	// Read the file header
	if (isRoQ){
		FS_Read(file, buffer, ROQ_CHUNK_HEADER_SIZE);

		id = buffer[0] | (buffer[1] << 8);
		fps = buffer[6] | (buffer[7] << 8);

		if (id != ROQ_ID){
			Com_DPrintf("Cinematic %s is not a RoQ file\n", name);

			FS_CloseFile(file);

			return false;
		}

		// Fill it in
		Str_Copy(cin.name, checkName, sizeof(cin.name));
		cin.flags = flags;

		cin.file = file;
		cin.size = size;
		cin.offset = ROQ_CHUNK_HEADER_SIZE;
		cin.header = ROQ_CHUNK_HEADER_SIZE;

		cin.isRoQ = true;

		cin.vidWidth = 0;
		cin.vidHeight = 0;
		cin.vidBuffer = NULL;

		cin.rawWidth = 0;
		cin.rawHeight = 0;
		cin.rawBuffer = NULL;

		// TODO: these are not needed for RoQ since we set them directly
		cin.sndRate = 22050;
		cin.sndWidth = 2;
		cin.sndChannels = 0;

		cin.frameRate = (fps) ? fps : 30;

		cin.startTime = 0;
		cin.currentFrame = 0;

		cin.playing = true;

		cin.roqCache = NULL;
	}
	else {
		FS_Read(file, &cin.vidWidth, sizeof(cin.vidWidth));
		FS_Read(file, &cin.vidHeight, sizeof(cin.vidHeight));
		FS_Read(file, &cin.sndRate, sizeof(cin.sndRate));
		FS_Read(file, &cin.sndWidth, sizeof(cin.sndWidth));
		FS_Read(file, &cin.sndChannels, sizeof(cin.sndChannels));

		// Fill it in
		Str_Copy(cin.name, checkName, sizeof(cin.name));
		cin.flags = flags;

		cin.file = file;
		cin.size = size;
		cin.offset = 20;
		cin.header = 20;

		cin.isRoQ = false;

		cin.vidWidth = LittleLong(cin.vidWidth);
		cin.vidHeight = LittleLong(cin.vidHeight);
		cin.vidBuffer = NULL;

		// TODO: Q2e 0.68 uses hardware limits here instaid of a fixed size, maybe it's better

		cin.rawWidth = 256;
		cin.rawHeight = 256;

		if (cin.rawWidth != cin.vidWidth || cin.rawHeight != cin.vidHeight)
			cin.rawBuffer = (byte *)Mem_ClearedAlloc(cin.rawWidth * cin.rawHeight * 4, TAG_TEMPORARY);

		cin.sndRate = LittleLong(cin.sndRate);
		cin.sndWidth = LittleLong(cin.sndWidth);
		cin.sndChannels = LittleLong(cin.sndChannels);

		cin.frameRate = 14;
		cin.startTime = 0;
		cin.currentFrame = 0;

		cin.playing = true;

		cin.hBuffer =(byte *) Mem_ClearedAlloc(cin.vidWidth * cin.vidHeight * 4, TAG_TEMPORARY);

		CIN_SetupHuffTables();
	}

	// TODO: start streaming if not CIN_SILIENT

	return true;
}

/*
 ==================
 CIN_StopCinematic
 ==================
*/
void CIN_StopCinematic (){
	
	// Check if playing a cinematic
	if (!cin.playing)
		return;

	// Stop the cinematic
	if (!(cin.flags & CIN_SILENT))
		S_StopAllSounds();

	// Free the frame buffers
	if (cin.rawBuffer)
		Mem_Free(cin.rawBuffer);

	if (cin.pcxBuffer)
		Mem_Free(cin.pcxBuffer);

	if (cin.hBuffer)
		Mem_Free(cin.hBuffer);
	if (cin.hNodes)
		Mem_Free(cin.hNodes);

	// Free RoQ cache
	if (cin.roqCache)
		Mem_Free(cin.roqCache);

	// Close the file
	if (cin.file)
		FS_CloseFile(cin.file);

	Mem_Fill(&cin, 0, sizeof(cinematic_t));
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

	// If running a local server, kill it and reissue
	SV_Shutdown("Server quit\n", false);

	// Disconnect from server
	CL_Disconnect(false);

	// Play the cinematic
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

	// Remove commands
	Cmd_RemoveCommand("playCinematic");
	Cmd_RemoveCommand("listCinematics");

	// Stop all the cinematics

	// Clear cinematic list
	Mem_Fill(&cin, 0, sizeof(cinematic_t));
}