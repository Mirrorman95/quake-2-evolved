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
// r_image.c - Image loading
//


#include "r_local.h"


#define MAX_IMAGE_SIZE				4096

typedef struct {
	const char *			suffix;

	bool					flipX;
	bool					flipY;
	bool					rotate;
} cubeMapFace_t;

typedef struct {
	const char *			suffix;

	mat3_t					axis;

	bool					flipX;
	bool					flipY;
	bool					rotate;
} envShotFace_t;

static cubeMapFace_t		r_cubeMapFaces[2][6] = {
	{
		{"px"     , false, false, false},
		{"nx"     , false, false, false},
		{"py"     , false, false, false},
		{"ny"     , false, false, false},
		{"pz"     , false, false, false},
		{"nz"     , false, false, false}
	},
	{
		{"rt"	  , false, false, true },
		{"lf"	  , true , true	, true },
		{"bk"	  , false, true	, false},
		{"ft"	  , true , false, false},
		{"up"	  , false, false, true },
		{"dn"	  , false, false, true }
	}
};

static envShotFace_t	r_envShotFaces[6] = {
	{"px", { 1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f}, true , true , true },
	{"nx", {-1.0f,  0.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f}, false, false, true },
	{"py", { 0.0f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f}, false, true , false},
	{"ny", { 0.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f}, true , false, false},
	{"pz", { 0.0f,  0.0f,  1.0f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f}, false, false, true },
	{"nz", { 0.0f,  0.0f, -1.0f,  0.0f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f}, false, false, true }
};

static float				r_luminanceTable[256][3];

static uint					r_palette[256];


/*
 ==============================================================================

 PCX LOADING

 ==============================================================================
*/


/*
 ==================
 R_LoadPCX
 ==================
*/
static bool R_LoadPCX (const char *name, byte **image, byte **palette, int *width, int *height){

	pcxHeader_t	*header;
	byte		*data;
	byte		*in, *out;
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
		Com_Error(ERR_DROP, "R_LoadPCX: invalid PCX header (%s)\n", name);

	if (header->bitsPerPixel != 8 || header->colorPlanes != 1)
		Com_Error(ERR_DROP, "R_LoadPCX: only 8 bit PCX images supported (%s)\n", name);
		
	if (header->xMax <= 0 || header->yMax <= 0 || header->xMax >= 640 || header->yMax >= 480)
		Com_Error(ERR_DROP, "R_LoadPCX: bad image size (%i x %i) (%s)\n", header->xMax, header->yMax, name);

	if (palette){
		*palette = (byte *)Mem_Alloc(768, TAG_TEMPORARY);
		Mem_Copy(*palette, (byte *)data + length - 768, 768);
	}

	if (!image){
		FS_FreeFile(data);
		return true;	// Because only the palette was requested
	}

	*width = header->xMax+1;
	*height = header->yMax+1;

	*image = out = (byte *)Mem_Alloc((header->xMax+1) * (header->yMax+1) * 4, TAG_TEMPORARY);

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
				*(uint *)out = r_palette[dataByte];

				out += 4;
				x++;
			}
		}
	}

	if (in - data > length){
		Com_DPrintf(S_COLOR_YELLOW "R_LoadPCX: PCX file was malformed (%s)\n", name);

		FS_FreeFile(data);
		Mem_Free(*image);
		*image = NULL;

		return false;
	}

	// Free the file data
	FS_FreeFile(data);

	return true;
}


/*
 ==============================================================================

 WAL LOADING

 ==============================================================================
*/


/*
 ==================
 R_LoadWAL
 ==================
*/
static bool R_LoadWAL (const char *name, byte **image, int *width, int *height){
	
	mipTex_t	*header;
	byte		*data;
	byte		*in, *out;
	int			i, c;

	// Load the file
	FS_ReadFile(name, (void **)&data);
	if (!data)
		return false;

	// Parse the WAL header
	header = (mipTex_t *)data;

	header->width = LittleLong(header->width);
	header->height = LittleLong(header->height);
	header->offsets[0] = LittleLong(header->offsets[0]);

	if (header->width == 0 || header->height == 0)
		Com_Error(ERR_DROP, "R_LoadWAL: bad image size (%i x %i) (%s)\n", header->width, header->height, name);

	// Read the image pixels
	in = data + header->offsets[0];

	*image = out = (byte *)Mem_Alloc(header->width * header->height * 4, TAG_TEMPORARY);

	*width = header->width;
	*height = header->height;

	c = header->width * header->height;

	for (i = 0; i < c; i++, in++, out += 4)
		*(uint *)out = r_palette[*in];

	// Free file data
	FS_FreeFile(data);

	return true;
}


/*
 ==============================================================================

 TGA LOADING

 ==============================================================================
*/


/*
 ==================
 R_LoadTGA
 ==================
*/
static bool R_LoadTGA (const char *name, byte **image, int *width, int *height){

	tgaHeader_t	header;
	byte		*data;
	byte		*in, *out;
	int			w, h, stride = 0;
	byte		r, g, b, a;
	byte		packetHeader, packetSize, i;

	// Load the file
	FS_ReadFile(name, (void **)&data);
	if (!data)
		return false;

	// Parse the TGA header
	header.bIdLength = data[0];
	header.bColormapType = data[1];
	header.bImageType = data[2];
	header.wColormapIndex = data[3] | (data[4] << 8);
	header.wColormapLength = data[5] | (data[6] << 8);
	header.bColormapSize = data[7];
	header.wXOrigin = data[8] | (data[9] << 8);
	header.wYOrigin = data[10] | (data[11] << 8);
	header.wWidth = data[12] | (data[13] << 8);
	header.wHeight = data[14] | (data[15] << 8);
	header.bPixelSize = data[16];
	header.bAttributes = data[17];

	if (header.bImageType != TGA_IMAGE_TRUECOLOR && header.bImageType != TGA_IMAGE_MONOCHROME && header.bImageType != 10)
		Com_Error(ERR_DROP, "R_LoadTGA: only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported (%s)\n", name);

	if (header.bColormapType != 0)
		Com_Error(ERR_DROP, "R_LoadTGA: colormaps not supported (%s)\n", name);
		
	if (header.bPixelSize != 8 && header.bPixelSize != 24 && header.bPixelSize != 32)
		Com_Error(ERR_DROP, "R_LoadTGA: only 8 (gray), 24 (RGB), and 32 (RGBA) bit images supported (%s)", name);

	if (header.wWidth <= 0 || header.wHeight <= 0)
		Com_Error(ERR_DROP, "R_LoadTGA: bad image size (%i x %i) (%s)\n", header.wWidth, header.wHeight, name);

	// Read the image pixels
	in = data + 18 + header.bIdLength;

	*image = out = (byte *)Mem_Alloc(header.wWidth * header.wHeight * 4, TAG_TEMPORARY);

	*width = header.wWidth;
	*height = header.wHeight;

	if (header.bAttributes & BIT(5))
		stride = 0;
	else {
		// Flipped image
		stride = -header.wWidth * 4 * 2;

		out += (header.wHeight - 1) * header.wWidth * 4;
	}
	
	if (header.bImageType == TGA_IMAGE_TRUECOLOR || header.bImageType == TGA_IMAGE_MONOCHROME){
		// Uncompressed RGB or grayscale image
		for (h = 0; h < header.wHeight; h++){
			for (w = 0; w < header.wWidth; w++){
				switch (header.bPixelSize){
				case 8:
					out[0] = in[0];
					out[1] = in[0];
					out[2] = in[0];
					out[3] = 255;

					in += 1;
					break;
				case 24:
					out[0] = in[2];
					out[1] = in[1];
					out[2] = in[0];
					out[3] = 255;

					in += 3;
					break;
				case 32:
					out[0] = in[2];
					out[1] = in[1];
					out[2] = in[0];
					out[3] = in[3];

					in += 4;
					break;
				}

				out += 4;
			}

			out += stride;
		}
	}
	else if (header.bImageType == 10){   
		// Run-Length encoded RGB image
		for (h = 0; h < header.wHeight; h++){
			for (w = 0; w < header.wWidth; ){
				packetHeader = *in++;
				packetSize = 1 + (packetHeader & 0x7F);
				
				if (packetHeader & 0x80){        // Run-Length packet
					switch (header.bPixelSize){
					case 24:
						r = in[2];
						g = in[1];
						b = in[0];
						a = 255;

						in += 3;
						break;
					case 32:
						r = in[2];
						g = in[1];
						b = in[0];
						a = in[3];

						in += 4;
						break;
					}
	
					for (i = 0; i < packetSize; i++){
						out[0] = r;
						out[1] = g;
						out[2] = b;
						out[3] = a;

						out += 4;
						
						w++;
						if (w == header.wWidth){	// Run spans across rows
							w = 0;
							if (h < header.wHeight-1)
								h++;
							else
								goto breakOut;

							out += stride;
						}
					}
				}
				else {							// Non Run-Length packet
					for (i = 0; i < packetSize; i++){
						switch (header.bPixelSize){
						case 24:
							out[0] = in[2];
							out[1] = in[1];
							out[2] = in[0];
							out[3] = 255;

							in += 3;
							break;
						case 32:
							out[0] = in[2];
							out[1] = in[1];
							out[2] = in[0];
							out[3] = in[3];

							in += 4;
							break;
						}

						out += 4;
		
						w++;
						if (w == header.wWidth){	// Run spans across rows
							w = 0;
							if (h < header.wHeight-1)
								h++;
							else
								goto breakOut;

							out += stride;
						}						
					}
				}
			}
			
			out += stride;

breakOut:
			;
		}
	}

	// Free file data
	FS_FreeFile(data);

	return true;
}

/*
 ==============================================================================

 DDS LOADING

 ==============================================================================
*/


/*
 ==================
 R_LoadDDS
 ==================
*/
static bool R_LoadDDS (const char *name, byte **image, int *width, int *height, uint *fourCC, bool *alphaPixels){

	ddsHeader_t	header;
	byte		*data;
	byte		*in, *out;
	int			mipCount, mipWidth, mipHeight;
	int			size;

	// Load the file
	FS_ReadFile(name, (void **)&data);
	if (!data)
		return false;

	// Parse the DDS header
	header.dwMagic = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	header.dwSize = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);
	header.dwFlags = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);
	header.dwHeight = data[12] | (data[13] << 8) | (data[14] << 16) | (data[15] << 24);
	header.dwWidth = data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24);
	header.dwPitchOrLinearSize = data[20] | (data[21] << 8) | (data[22] << 16) | (data[23] << 24);
	header.dwDepth = data[24] | (data[25] << 8) | (data[26] << 16) | (data[27] << 24);
	header.dwMipMapCount = data[28] | (data[29] << 8) | (data[30] << 16) | (data[31] << 24);
	header.dwReserved1[0] = data[32] | (data[33] << 8) | (data[34] << 16) | (data[35] << 24);
	header.dwReserved1[1] = data[36] | (data[37] << 8) | (data[38] << 16) | (data[39] << 24);
	header.dwReserved1[2] = data[40] | (data[41] << 8) | (data[42] << 16) | (data[43] << 24);
	header.dwReserved1[3] = data[44] | (data[45] << 8) | (data[46] << 16) | (data[47] << 24);
	header.dwReserved1[4] = data[48] | (data[49] << 8) | (data[50] << 16) | (data[51] << 24);
	header.dwReserved1[5] = data[52] | (data[53] << 8) | (data[54] << 16) | (data[55] << 24);
	header.dwReserved1[6] = data[56] | (data[57] << 8) | (data[58] << 16) | (data[59] << 24);
	header.dwReserved1[7] = data[60] | (data[61] << 8) | (data[62] << 16) | (data[63] << 24);
	header.dwReserved1[8] = data[64] | (data[65] << 8) | (data[66] << 16) | (data[67] << 24);
	header.dwReserved1[9] = data[68] | (data[69] << 8) | (data[70] << 16) | (data[71] << 24);
	header.dwReserved1[10] = data[72] | (data[73] << 8) | (data[74] << 16) | (data[75] << 24);
	header.ddsPixelFormat.dwSize = data[76] | (data[77] << 8) | (data[78] << 16) | (data[79] << 24);
	header.ddsPixelFormat.dwFlags = data[80] | (data[81] << 8) | (data[82] << 16) | (data[83] << 24);
	header.ddsPixelFormat.dwFourCC = data[84] | (data[85] << 8) | (data[86] << 16) | (data[87] << 24);
	header.ddsPixelFormat.dwRGBBitCount = data[88] | (data[89] << 8) | (data[90] << 16) | (data[91] << 24);
	header.ddsPixelFormat.dwRBitMask = data[92] | (data[93] << 8) | (data[94] << 16) | (data[95] << 24);
	header.ddsPixelFormat.dwGBitMask = data[96] | (data[97] << 8) | (data[98] << 16) | (data[99] << 24);
	header.ddsPixelFormat.dwBBitMask = data[100] | (data[101] << 8) | (data[102] << 16) | (data[103] << 24);
	header.ddsPixelFormat.dwABitMask = data[104] | (data[105] << 8) | (data[106] << 16) | (data[107] << 24);
	header.dwCaps1 = data[108] | (data[109] << 8) | (data[110] << 16) | (data[111] << 24);
	header.dwCaps2 = data[112] | (data[113] << 8) | (data[114] << 16) | (data[115] << 24);
	header.dwReserved2[0] = data[116] | (data[117] << 8) | (data[118] << 16) | (data[119] << 24);
	header.dwReserved2[1] = data[120] | (data[121] << 8) | (data[122] << 16) | (data[123] << 24);
	header.dwReserved2[2] = data[124] | (data[125] << 8) | (data[126] << 16) | (data[127] << 24);

	if (header.dwMagic != DDS_MAGIC)
		Com_Error(ERR_DROP, "R_LoadDDS: bad magic (%s)", name);

	if ((header.dwFlags & (DDS_WIDTH | DDS_HEIGHT | DDS_MIPMAPCOUNT | DDS_PIXELFORMAT | DDS_CAPS)) != (DDS_WIDTH | DDS_HEIGHT | DDS_MIPMAPCOUNT | DDS_PIXELFORMAT | DDS_CAPS))
		Com_Error(ERR_DROP, "R_LoadDDS: bad image flags (%i) (%s)", header.dwFlags, name);

	if (!(header.ddsPixelFormat.dwFlags & DDS_FOURCC))
		Com_Error(ERR_DROP, "R_LoadDDS: only FourCC images supported (%s)", name);

	if (header.ddsPixelFormat.dwFourCC != DDS_FOURCC_DXT1 && header.ddsPixelFormat.dwFourCC != DDS_FOURCC_DXT3 && header.ddsPixelFormat.dwFourCC != DDS_FOURCC_DXT5 && header.ddsPixelFormat.dwFourCC != DDS_FOURCC_RXGB)
		Com_Error(ERR_DROP, "R_LoadDDS: only DXT1, DXT3, DXT5, and RxGB images supported (%s)", name);

	if ((header.dwCaps1 & (DDS_COMPLEX | DDS_TEXTURE | DDS_MIPMAP)) != (DDS_COMPLEX | DDS_TEXTURE | DDS_MIPMAP))
		Com_Error(ERR_DROP, "R_LoadDDS: bad image caps (%i) (%s)", header.dwCaps1, name);

	if (header.dwCaps2 & (DDS_CUBEMAP | DDS_VOLUME))
		Com_Error(ERR_DROP, "R_LoadDDS: only 2D images supported (%s)", name);

	if (header.dwWidth == 0 || header.dwHeight == 0 || header.dwWidth > MAX_IMAGE_SIZE || header.dwHeight > MAX_IMAGE_SIZE || !IsPowerOfTwo(header.dwWidth) || !IsPowerOfTwo(header.dwHeight))
		Com_Error(ERR_DROP, "R_LoadDDS: bad image size (%i x %i) (%s)", header.dwWidth, header.dwHeight, name);

	// Calculate mipmap count and size
	mipCount = 1;
	mipWidth = header.dwWidth;
	mipHeight = header.dwHeight;

	if (header.ddsPixelFormat.dwFourCC == DDS_FOURCC_DXT1)
		size = ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2) * 8;
	else
		size = ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2) * 16;

	while (mipWidth > 1 || mipHeight > 1){
		mipCount++;

		mipWidth >>= 1;
		if (mipWidth < 1)
			mipWidth = 1;

		mipHeight >>= 1;
		if (mipHeight < 1)
			mipHeight = 1;

		if (header.ddsPixelFormat.dwFourCC == DDS_FOURCC_DXT1)
			size += ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2) * 8;
		else
			size += ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2) * 16;
	}

	if (header.dwMipMapCount != mipCount)
		Com_Error(ERR_DROP, "R_LoadDDS: bad image mipmap count (%i != %i) (%s)", header.dwMipMapCount, mipCount, name);

	// Read the image pixels
	in = data + 128;

	*image = out = (byte *)Mem_Alloc(size, TAG_TEMPORARY);

	*width = header.dwWidth;
	*height = header.dwHeight;

	*fourCC = header.ddsPixelFormat.dwFourCC;
	*alphaPixels = (header.ddsPixelFormat.dwFlags & DDS_ALPHAPIXELS);

	Mem_Copy(out, in, size);

	// Free file data
	FS_FreeFile(data);

	return true;
}


/*
 ==============================================================================

 IMAGE PROGRAM PARSING AND PROCESSING

 ==============================================================================
*/

static bool				R_ParseImageProgram (script_t *script, byte **image, int *width, int *height, bool wrapClamp);


/*
 ==================
 R_AddImages
 ==================
*/
static byte *R_AddImages (const byte *in1, const byte *in2, int width, int height){

	byte	*image, *out;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, in1 += 4, in2 += 4, out += 4){
			out[0] = Min(in1[0] + in2[0], 255);
			out[1] = Min(in1[1] + in2[1], 255);
			out[2] = Min(in1[2] + in2[2], 255);
			out[3] = Min(in1[3] + in2[3], 255);
		}
	}

	return image;
}

/*
 ==================
 R_SubtractImages
 ==================
*/
static byte *R_SubtractImages (const byte *in1, const byte *in2, int width, int height){

	byte	*image, *out;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, in1 += 4, in2 += 4, out += 4){
			out[0] = Max(in1[0] - in2[0], 0);
			out[1] = Max(in1[1] - in2[1], 0);
			out[2] = Max(in1[2] - in2[2], 0);
			out[3] = Max(in1[3] - in2[3], 0);
		}
	}

	return image;
}

/*
 ==================
 R_ModulateImages
 ==================
*/
static byte *R_ModulateImages (const byte *in1, const byte *in2, int width, int height){

	byte	*image, *out;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, in1 += 4, in2 += 4, out += 4){
			out[0] = FloatToByte(in1[0] * (in2[0] * (1.0f / 255.0f)));
			out[1] = FloatToByte(in1[1] * (in2[1] * (1.0f / 255.0f)));
			out[2] = FloatToByte(in1[2] * (in2[2] * (1.0f / 255.0f)));
			out[3] = FloatToByte(in1[3] * (in2[3] * (1.0f / 255.0f)));
		}
	}

	return image;
}

/*
 ==================
 R_BiasImage
 ==================
*/
static byte *R_BiasImage (const byte *in, int width, int height, const vec4_t bias){

	byte	*image, *out;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, in += 4, out += 4){
			out[0] = FloatToByte(in[0] + bias[0]);
			out[1] = FloatToByte(in[1] + bias[1]);
			out[2] = FloatToByte(in[2] + bias[2]);
			out[3] = FloatToByte(in[3] + bias[3]);
		}
	}

	return image;
}

/*
 ==================
 R_ScaleImage
 ==================
*/
static byte *R_ScaleImage (const byte *in, int width, int height, const vec4_t scale){

	byte	*image, *out;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, in += 4, out += 4){
			out[0] = FloatToByte(in[0] * scale[0]);
			out[1] = FloatToByte(in[1] * scale[1]);
			out[2] = FloatToByte(in[2] * scale[2]);
			out[3] = FloatToByte(in[3] * scale[3]);
		}
	}

	return image;
}

/*
 ==================
 R_InvertImageColor
 ==================
*/
static byte *R_InvertImageColor (const byte *in, int width, int height){

	byte	*image, *out;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, in += 4, out += 4){
			out[0] = 255 - in[0];
			out[1] = 255 - in[1];
			out[2] = 255 - in[2];
			out[3] = in[3];
		}
	}

	return image;
}

/*
 ==================
 R_InvertImageAlpha
 ==================
*/
static byte *R_InvertImageAlpha (const byte *in, int width, int height){

	byte	*image, *out;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, in += 4, out += 4){
			out[0] = in[0];
			out[1] = in[1];
			out[2] = in[2];
			out[3] = 255 - in[3];
		}
	}

	return image;
}

/*
 ==================
 R_MakeImageIntensity
 ==================
*/
static byte *R_MakeImageIntensity (const byte *in, int width, int height){

	byte	*image, *out;
	float	r, g, b;
	int		intensity;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, in += 4, out += 4){
			r = r_luminanceTable[in[0]][0];
			g = r_luminanceTable[in[1]][1];
			b = r_luminanceTable[in[2]][2];

			intensity = FloatToInt(r + g + b);

			out[0] = intensity;
			out[1] = intensity;
			out[2] = intensity;
			out[3] = intensity;
		}
	}

	return image;
}

/*
 ==================
 R_MakeImageAlpha
 ==================
*/
static byte *R_MakeImageAlpha (const byte *in, int width, int height){

	byte	*image, *out;
	float	r, g, b;
	int		alpha;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, in += 4, out += 4){
			r = r_luminanceTable[in[0]][0];
			g = r_luminanceTable[in[1]][1];
			b = r_luminanceTable[in[2]][2];

			alpha = FloatToInt(r + g + b);

			out[0] = 255;
			out[1] = 255;
			out[2] = 255;
			out[3] = alpha;
		}
	}

	return image;
}

/*
 ==================
 R_HeightMapToNormalMap
 ==================
*/
static byte *R_HeightMapToNormalMap (const byte *in, int width, int height, float scale, bool wrapClamp){

	byte	*image, *out;
	float	r, g, b;
	float	n, nx, ny;
	vec3_t	normal;
	int		index;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, out += 4){
			index = (y * width + x) * 4;

			r = r_luminanceTable[in[index+0]][0];
			g = r_luminanceTable[in[index+1]][1];
			b = r_luminanceTable[in[index+2]][2];

			n = (r + g + b) * (1.0f / 255.0f);

			if (wrapClamp && (x == width - 1))
				index = (y * width + x) * 4;
			else
				index = (y * width + ((x+1) % width)) * 4;

			r = r_luminanceTable[in[index+0]][0];
			g = r_luminanceTable[in[index+1]][1];
			b = r_luminanceTable[in[index+2]][2];

			nx = (r + g + b) * (1.0f / 255.0f);

			if (wrapClamp && (y == height - 1))
				index = (y * width + x) * 4;
			else
				index = (((y+1) % height) * width + x) * 4;

			r = r_luminanceTable[in[index+0]][0];
			g = r_luminanceTable[in[index+1]][1];
			b = r_luminanceTable[in[index+2]][2];

			ny = (r + g + b) * (1.0f / 255.0f);

			normal[0] = (n - nx) * scale;
			normal[1] = (n - ny) * scale;
			normal[2] = 1.0f;

			VectorNormalize(normal);

			out[0] = 128 + FloatToInt(127.0f * normal[0]);
			out[1] = 128 + FloatToInt(127.0f * normal[1]);
			out[2] = 128 + FloatToInt(127.0f * normal[2]);
			out[3] = 255;
		}
	}

	return image;
}

/*
 ==================
 R_AddNormalMaps
 ==================
*/
static byte *R_AddNormalMaps (const byte *in1, const byte *in2, int width, int height){

	byte	*image, *out;
	vec3_t	normal1, normal2;
	vec3_t	normal;
	int		x, y;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, in1 += 4, in2 += 4, out += 4){
			normal1[0] = in1[0] * (1.0f / 127.0f) - 1.0f;
			normal1[1] = in1[1] * (1.0f / 127.0f) - 1.0f;
			normal1[2] = in1[2] * (1.0f / 127.0f) - 1.0f;

			normal2[0] = in2[0] * (1.0f / 127.0f) - 1.0f;
			normal2[1] = in2[1] * (1.0f / 127.0f) - 1.0f;
			normal2[2] = in2[2] * (1.0f / 127.0f) - 1.0f;

			if (normal1[2] <= 0.0f || normal2[2] <= 0.0f)
				VectorSet(normal, 0.0f, 0.0f, 1.0f);
			else {
				normal[0] = (normal1[0] / normal1[2]) + (normal2[0] / normal2[2]);
				normal[1] = (normal1[1] / normal1[2]) + (normal2[1] / normal2[2]);
				normal[2] = 1.0f;

				VectorNormalize(normal);
			}

			out[0] = 128 + FloatToInt(127.0f * normal[0]);
			out[1] = 128 + FloatToInt(127.0f * normal[1]);
			out[2] = 128 + FloatToInt(127.0f * normal[2]);
			out[3] = 255;
		}
	}

	return image;
}

/*
 ==================
 R_SmoothNormalMap
 ==================
*/
static byte *R_SmoothNormalMap (const byte *in, int width, int height, bool wrapClamp){

	byte	*image, *out;
	int		xOfs, xOffsets[8] = {-1, -1, -1, 1, 1, 1, 0, 0};
	int		yOfs, yOffsets[8] = {-1, 0, 1, -1, 0, 1, -1, 1};
	vec3_t	normal;
	int		index;
	int		x, y;
	int		i;

	image = out = (byte *)Mem_Alloc(width * height * 4, TAG_TEMPORARY);

	for (y = 0; y < height; y++){
		for (x = 0; x < width; x++, out += 4){
			index = (y * width + x) * 4;

			normal[0] = in[index+0] * (1.0f / 127.0f) - 1.0f;
			normal[1] = in[index+1] * (1.0f / 127.0f) - 1.0f;
			normal[2] = in[index+2] * (1.0f / 127.0f) - 1.0f;

			for (i = 0; i < 8; i++){
				xOfs = x + xOffsets[i];
				yOfs = y + yOffsets[i];

				if (xOfs < 0 || xOfs > width - 1){
					if (wrapClamp)
						continue;

					xOfs = (xOfs < 0) ? width - 1 : 0;
				}

				if (yOfs < 0 || yOfs > height - 1){
					if (wrapClamp)
						continue;

					yOfs = (yOfs < 0) ? height - 1 : 0;
				}

				index = (yOfs * width + xOfs) * 4;

				normal[0] += in[index+0] * (1.0f / 127.0f) - 1.0f;
				normal[1] += in[index+1] * (1.0f / 127.0f) - 1.0f;
				normal[2] += in[index+2] * (1.0f / 127.0f) - 1.0f;
			}

			if (normal[2] <= 0.0f || !VectorNormalize(normal))
				VectorSet(normal, 0.0f, 0.0f, 1.0f);

			out[0] = 128 + FloatToInt(127.0f * normal[0]);
			out[1] = 128 + FloatToInt(127.0f * normal[1]);
			out[2] = 128 + FloatToInt(127.0f * normal[2]);
			out[3] = 255;
		}
	}

	return image;
}

/*
 ==================
 R_ParseAdd
 ==================
*/
static bool R_ParseAdd (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	byte	*imgData1, *imgData2;
	int		imgWidth1, imgHeight1, imgWidth2, imgHeight2;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData1, &imgWidth1, &imgHeight1, wrapClamp))
		return false;

	if (!PS_ExpectTokenString(script, &token, ",", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData1);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData2, &imgWidth2, &imgHeight2, wrapClamp)){
		Mem_Free(imgData1);
		return false;
	}

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData1);
		Mem_Free(imgData2);
		return false;
	}

	if (imgWidth1 != imgWidth2 || imgHeight1 != imgHeight2){
		Com_Printf(S_COLOR_YELLOW "WARNING: images with mismatched dimensions (%i x %i != %i x %i) in image program\n", imgWidth1, imgHeight1, imgWidth2, imgHeight2);

		Mem_Free(imgData1);
		Mem_Free(imgData2);
		return false;
	}

	*image = R_AddImages(imgData1, imgData2, imgWidth1, imgHeight1);

	*width = imgWidth1;
	*height = imgHeight1;

	Mem_Free(imgData1);
	Mem_Free(imgData2);

	return true;
}

/*
 ==================
 R_ParseSubtract
 ==================
*/
static bool R_ParseSubtract (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	byte	*imgData1, *imgData2;
	int		imgWidth1, imgHeight1, imgWidth2, imgHeight2;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData1, &imgWidth1, &imgHeight1, wrapClamp))
		return false;

	if (!PS_ExpectTokenString(script, &token, ",", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData1);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData2, &imgWidth2, &imgHeight2, wrapClamp)){
		Mem_Free(imgData1);
		return false;
	}

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData1);
		Mem_Free(imgData2);
		return false;
	}

	if (imgWidth1 != imgWidth2 || imgHeight1 != imgHeight2){
		Com_Printf(S_COLOR_YELLOW "WARNING: images with mismatched dimensions (%i x %i != %i x %i) in image program\n", imgWidth1, imgHeight1, imgWidth2, imgHeight2);

		Mem_Free(imgData1);
		Mem_Free(imgData2);
		return false;
	}

	*image = R_SubtractImages(imgData1, imgData2, imgWidth1, imgHeight1);

	*width = imgWidth1;
	*height = imgHeight1;

	Mem_Free(imgData1);
	Mem_Free(imgData2);

	return true;
}

/*
 ==================
 R_ParseModulate
 ==================
*/
static bool R_ParseModulate (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	byte	*imgData1, *imgData2;
	int		imgWidth1, imgHeight1, imgWidth2, imgHeight2;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData1, &imgWidth1, &imgHeight1, wrapClamp))
		return false;

	if (!PS_ExpectTokenString(script, &token, ",", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData1);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData2, &imgWidth2, &imgHeight2, wrapClamp)){
		Mem_Free(imgData1);
		return false;
	}

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData1);
		Mem_Free(imgData2);
		return false;
	}

	if (imgWidth1 != imgWidth2 || imgHeight1 != imgHeight2){
		Com_Printf(S_COLOR_YELLOW "WARNING: images with mismatched dimensions (%i x %i != %i x %i) in image program\n", imgWidth1, imgHeight1, imgWidth2, imgHeight2);

		Mem_Free(imgData1);
		Mem_Free(imgData2);
		return false;
	}

	*image = R_ModulateImages(imgData1, imgData2, imgWidth1, imgHeight1);

	*width = imgWidth1;
	*height = imgHeight1;

	Mem_Free(imgData1);
	Mem_Free(imgData2);

	return true;
}

/*
 ==================
 R_ParseBias
 ==================
*/
static bool R_ParseBias (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	vec4_t	bias;
	byte	*imgData;
	int		imgWidth, imgHeight;
	int		i;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData, &imgWidth, &imgHeight, wrapClamp))
		return false;

	for (i = 0; i < 4; i++){
		if (!PS_ExpectTokenString(script, &token, ",", true)){
			Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in image program\n", token.string);

			Mem_Free(imgData);
			return false;
		}

		if (!PS_ReadFloat(script, &bias[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for image program\n");

			Mem_Free(imgData);
			return false;
		}
	}

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData);
		return false;
	}

	*image = R_BiasImage(imgData, imgWidth, imgHeight, bias);

	*width = imgWidth;
	*height = imgHeight;

	Mem_Free(imgData);

	return true;
}

/*
 ==================
 R_ParseScale
 ==================
*/
static bool R_ParseScale (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	vec4_t	scale;
	byte	*imgData;
	int		imgWidth, imgHeight;
	int		i;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData, &imgWidth, &imgHeight, wrapClamp))
		return false;

	for (i = 0; i < 4; i++){
		if (!PS_ExpectTokenString(script, &token, ",", true)){
			Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in image program\n", token.string);

			Mem_Free(imgData);
			return false;
		}

		if (!PS_ReadFloat(script, &scale[i])){
			Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for image program\n");

			Mem_Free(imgData);
			return false;
		}
	}

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData);
		return false;
	}

	*image = R_ScaleImage(imgData, imgWidth, imgHeight, scale);

	*width = imgWidth;
	*height = imgHeight;

	Mem_Free(imgData);

	return true;
}

/*
 ==================
 R_ParseInvertColor
 ==================
*/
static bool R_ParseInvertColor (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	byte	*imgData;
	int		imgWidth, imgHeight;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData, &imgWidth, &imgHeight, wrapClamp))
		return false;

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData);
		return false;
	}

	*image = R_InvertImageColor(imgData, imgWidth, imgHeight);

	*width = imgWidth;
	*height = imgHeight;

	Mem_Free(imgData);

	return true;
}

/*
 ==================
 R_ParseInvertAlpha
 ==================
*/
static bool R_ParseInvertAlpha (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	byte	*imgData;
	int		imgWidth, imgHeight;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData, &imgWidth, &imgHeight, wrapClamp))
		return false;

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData);
		return false;
	}

	*image = R_InvertImageAlpha(imgData, imgWidth, imgHeight);

	*width = imgWidth;
	*height = imgHeight;

	Mem_Free(imgData);

	return true;
}

/*
 ==================
 R_ParseMakeAlpha
 ==================
*/
static bool R_ParseMakeAlpha (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	byte	*imgData;
	int		imgWidth, imgHeight;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData, &imgWidth, &imgHeight, wrapClamp))
		return false;

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData);
		return false;
	}

	*image = R_MakeImageAlpha(imgData, imgWidth, imgHeight);

	*width = imgWidth;
	*height = imgHeight;

	Mem_Free(imgData);

	return true;
}

/*
 ==================
 R_ParseMakeIntensity
 ==================
*/
static bool R_ParseMakeIntensity (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	byte	*imgData;
	int		imgWidth, imgHeight;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData, &imgWidth, &imgHeight, wrapClamp))
		return false;

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData);
		return false;
	}

	*image = R_MakeImageIntensity(imgData, imgWidth, imgHeight);

	*width = imgWidth;
	*height = imgHeight;

	Mem_Free(imgData);

	return true;
}

/*
 ==================
 R_ParseHeightMap
 ==================
*/
static bool R_ParseHeightMap (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	float	scale;
	byte	*imgData;
	int		imgWidth, imgHeight;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData, &imgWidth, &imgHeight, wrapClamp))
		return false;

	if (!PS_ExpectTokenString(script, &token, ",", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData);
		return false;
	}

	if (!PS_ReadFloat(script, &scale)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for image program\n");

		Mem_Free(imgData);
		return false;
	}

	if (scale <= 0.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g in image program\n", scale);

		Mem_Free(imgData);
		return false;
	}

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData);
		return false;
	}

	*image = R_HeightMapToNormalMap(imgData, imgWidth, imgHeight, scale, wrapClamp);

	*width = imgWidth;
	*height = imgHeight;

	Mem_Free(imgData);

	return true;
}

/*
 ==================
 R_ParseAddNormals
 ==================
*/
static bool R_ParseAddNormals (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	byte	*imgData1, *imgData2;
	int		imgWidth1, imgHeight1, imgWidth2, imgHeight2;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData1, &imgWidth1, &imgHeight1, wrapClamp))
		return false;

	if (!PS_ExpectTokenString(script, &token, ",", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ',', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData1);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData2, &imgWidth2, &imgHeight2, wrapClamp)){
		Mem_Free(imgData1);
		return false;
	}

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData1);
		Mem_Free(imgData2);
		return false;
	}

	if (imgWidth1 != imgWidth2 || imgHeight1 != imgHeight2){
		Com_Printf(S_COLOR_YELLOW "WARNING: images with mismatched dimensions (%i x %i != %i x %i) in image program\n", imgWidth1, imgHeight1, imgWidth2, imgHeight2);

		Mem_Free(imgData1);
		Mem_Free(imgData2);
		return false;
	}

	*image = R_AddNormalMaps(imgData1, imgData2, imgWidth1, imgHeight1);

	*width = imgWidth1;
	*height = imgHeight1;

	Mem_Free(imgData1);
	Mem_Free(imgData2);

	return true;
}

/*
 ==================
 R_ParseSmoothNormals
 ==================
*/
static bool R_ParseSmoothNormals (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	byte	*imgData;
	int		imgWidth, imgHeight;

	if (!PS_ExpectTokenString(script, &token, "(", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '(', found '%s' instead in image program\n", token.string);
		return false;
	}

	if (!R_ParseImageProgram(script, &imgData, &imgWidth, &imgHeight, wrapClamp))
		return false;

	if (!PS_ExpectTokenString(script, &token, ")", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected ')', found '%s' instead in image program\n", token.string);

		Mem_Free(imgData);
		return false;
	}

	*image = R_SmoothNormalMap(imgData, imgWidth, imgHeight, wrapClamp);

	*width = imgWidth;
	*height = imgHeight;

	Mem_Free(imgData);

	return true;
}

/*
 ==================
 R_ParseImageProgram

 TODO: R_LoadPCX and R_LoadWAL
 ==================
*/
static bool R_ParseImageProgram (script_t *script, byte **image, int *width, int *height, bool wrapClamp){

	token_t	token;
	char	name[MAX_PATH_LENGTH];

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for image program\n");
		return false;
	}

	if (!Str_ICompare(token.string, "add"))
		return R_ParseAdd(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "subtract"))
		return R_ParseSubtract(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "modulate"))
		return R_ParseModulate(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "bias"))
		return R_ParseBias(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "scale"))
		return R_ParseScale(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "invertColor"))
		return R_ParseInvertColor(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "invertAlpha"))
		return R_ParseInvertAlpha(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "makeAlpha"))
		return R_ParseMakeAlpha(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "makeIntensity"))
		return R_ParseMakeIntensity(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "heightMap"))
		return R_ParseHeightMap(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "addNormals"))
		return R_ParseAddNormals(script, image, width, height, wrapClamp);
	else if (!Str_ICompare(token.string, "smoothNormals"))
		return R_ParseSmoothNormals(script, image, width, height, wrapClamp);
	else {
		Str_SPrintf(name, sizeof(name), "%s.tga", token.string);

		if (R_LoadTGA(name, image, width, height))
			return true;

		return false;
	}
}

/*
 ==================
 R_LoadImageProgram
 ==================
*/
static bool R_LoadImageProgram (const char *imageProgram, byte **image, int *width, int *height, bool wrapClamp){

	script_t	*script;

	script = PS_LoadScriptMemory("ImageProgram", imageProgram, Str_Length(imageProgram), 1);
	if (!script)
		return false;

	PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

	if (!R_ParseImageProgram(script, image, width, height, wrapClamp)){
		PS_FreeScript(script);
		return false;
	}

	PS_FreeScript(script);

	return true;
}

/*
 ==================
 R_WriteImageProgram
 ==================
*/
static void R_WriteImageProgram (const char *imageProgram, const byte *image, int width, int height){

	byte	*buffer, *ptr;
	bool	hasColor = false, hasAlpha = false;
	int		samples, size;
	int		x, y;
	int		i;

	// Determine the number of samples actually needed
	size = width * height * 4;

	for (i = 0; i < size; i += 4){
		if (image[i+0] != image[i+1] || image[i+1] != image[i+2])
			hasColor = true;
		if (image[i+3] != 255)
			hasAlpha = true;

		if (hasColor && hasAlpha)
			break;
	}

	if (hasAlpha)
		samples = 4;
	else {
		if (hasColor)
			samples = 3;
		else
			samples = 1;
	}

	// Allocate the buffer
	size = 18 + (width * height * samples);
	buffer = (byte *)Mem_Alloc(size, TAG_TEMPORARY);

	// Set up the header
	Mem_Fill(buffer, 0, 18);

	buffer[2] = TGA_IMAGE_TRUECOLOR;
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = samples << 3;
	buffer[17] = BIT(5);

	// Write the image pixels
	ptr = buffer + 18;

	switch (samples){
	case 1:
		for (y = 0; y < height; y++){
			for (x = 0; x < width; x++, image += 4, ptr += 1)
				ptr[0] = image[0];
		}

		break;
	case 3:
		for (y = 0; y < height; y++){
			for (x = 0; x < width; x++, image += 4, ptr += 3){
				ptr[0] = image[2];
				ptr[1] = image[1];
				ptr[2] = image[0];
			}
		}

		break;
	case 4:
		for (y = 0; y < height; y++){
			for (x = 0; x < width; x++, image += 4, ptr += 4){
				ptr[0] = image[2];
				ptr[1] = image[1];
				ptr[2] = image[0];
				ptr[3] = image[3];
			}
		}

		break;
	}

	// Write the image
	if (!FS_WriteFile(imageProgram, buffer, size)){
		Com_Printf("Couldn't write %s\n", imageProgram);

		Mem_Free(buffer);
		return;
	}

	Com_Printf("Wrote %s\n", imageProgram);

	Mem_Free(buffer);
}

/*
 ==================
 R_ImageProgramToFileName
 ==================
*/
static bool R_ImageProgramToFileName (const char *imageProgram, char *realName, char *compressedName, int maxLength){

	char	name[MAX_PATH_LENGTH];
	int		length = 0;
	int		i;

	// If not an image program just add an extension
	if (!Str_FindChar(imageProgram, '(') || !Str_FindChar(imageProgram, ')')){
		Str_SPrintf(realName, maxLength, "%s.tga", imageProgram);
		Str_SPrintf(compressedName, maxLength, "%s.dds", imageProgram);

		return false;
	}

	// Convert punctuations to underlines and make sure it is lower-case
	for (i = 0; imageProgram[i]; i++){
		if (length == MAX_PATH_LENGTH - 1)
			break;

		if (Str_CharIsPunctuation(imageProgram[i])){
			if (name[length-1] != '_')
				name[length++] = '_';

			continue;
		}

		name[length++] = Str_CharToLower(imageProgram[i]);
	}

	// Remove any trailing underlines
	for ( ; length > 0; length--){
		if (name[length-1] != '_')
			break;
	}

	// Terminate the string
	name[length] = 0;

	// Set the file names
	Str_SPrintf(realName, maxLength, "imgprog/%s.tga", name);
	Str_SPrintf(compressedName, maxLength, "imgprog/%s.dds", name);

	return true;
}


/*
 ==============================================================================

 IMAGE REGISTRATION

 ==============================================================================
*/


/*
 ==================
 R_ResampleImage
 ==================
*/
static byte *R_ResampleImage (const byte *image, int inWidth, int inHeight, int outWidth, int outHeight, bool isNormalMap){

	byte		*buffer, *ptr;
	const dword	*row1, *row2;
	const byte	*pix1, *pix2, *pix3, *pix4;
	uint		p1[MAX_IMAGE_SIZE], p2[MAX_IMAGE_SIZE];
	uint		frac, fracStep;
	vec3_t		normal;
	int			x, y;
	int			i;

	buffer = ptr = (byte *)Mem_Alloc(outWidth * outHeight * 4, TAG_TEMPORARY);

	fracStep = inWidth * 0x10000 / outWidth;

	frac = fracStep >> 2;
	for (i = 0; i < outWidth; i++){
		p1[i] = 4 * (frac >> 16);
		frac += fracStep;
	}

	frac = (fracStep >> 2) * 3;
	for (i = 0; i < outWidth; i++){
		p2[i] = 4 * (frac >> 16);
		frac += fracStep;
	}

	// Special case for normal maps
	if (isNormalMap){
		for (y = 0; y < outHeight; y++){
			row1 = (const dword *)image + inWidth * (int)(((float)y + 0.25f) * inHeight/outHeight);
			row2 = (const dword *)image + inWidth * (int)(((float)y + 0.75f) * inHeight/outHeight);

			for (x = 0; x < outWidth; x++, ptr += 4){
				pix1 = (const byte *)row1 + p1[x];
				pix2 = (const byte *)row1 + p2[x];
				pix3 = (const byte *)row2 + p1[x];
				pix4 = (const byte *)row2 + p2[x];

				normal[0] = (pix1[0] * (1.0f / 127.0f) - 1.0f) + (pix2[0] * (1.0f / 127.0f) - 1.0f) + (pix3[0] * (1.0f / 127.0f) - 1.0f) + (pix4[0] * (1.0f / 127.0f) - 1.0f);
				normal[1] = (pix1[1] * (1.0f / 127.0f) - 1.0f) + (pix2[1] * (1.0f / 127.0f) - 1.0f) + (pix3[1] * (1.0f / 127.0f) - 1.0f) + (pix4[1] * (1.0f / 127.0f) - 1.0f);
				normal[2] = (pix1[2] * (1.0f / 127.0f) - 1.0f) + (pix2[2] * (1.0f / 127.0f) - 1.0f) + (pix3[2] * (1.0f / 127.0f) - 1.0f) + (pix4[2] * (1.0f / 127.0f) - 1.0f);

				if (normal[2] <= 0.0f || !VectorNormalize(normal))
					VectorSet(normal, 0.0f, 0.0f, 1.0f);

				ptr[0] = 128 + FloatToInt(127.0f * normal[0]);
				ptr[1] = 128 + FloatToInt(127.0f * normal[1]);
				ptr[2] = 128 + FloatToInt(127.0f * normal[2]);
				ptr[3] = 255;
			}
		}

		Mem_Free(image);

		return buffer;
	}

	// General case
	for (y = 0; y < outHeight; y++){
		row1 = (const dword *)image + inWidth * (int)(((float)y + 0.25f) * inHeight/outHeight);
		row2 = (const dword *)image + inWidth * (int)(((float)y + 0.75f) * inHeight/outHeight);

		for (x = 0; x < outWidth; x++, ptr += 4){
			pix1 = (const byte *)row1 + p1[x];
			pix2 = (const byte *)row1 + p2[x];
			pix3 = (const byte *)row2 + p1[x];
			pix4 = (const byte *)row2 + p2[x];

			ptr[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
			ptr[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
			ptr[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
			ptr[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
		}
	}

	Mem_Free(image);

	return buffer;
}

/*
 ==================
 R_FlipAndRotateImage
 ==================
*/
static byte *R_FlipAndRotateImage (const byte *image, int width, int height, bool noAlpha, bool flipX, bool flipY, bool rotate){

	byte		*buffer, *ptr;
	const byte	*line, *l;
	int			xStride, yStride;
	int			xOffset, yOffset;
	int			x, y;
	int			samples;

	if (noAlpha)
		samples = 3;
	else
		samples = 4;

	buffer = ptr = (byte *)Mem_Alloc(width * height * samples, TAG_TEMPORARY);

	if (flipX){
		xStride = -samples;
		xOffset = (width - 1) * samples;
	}
	else {
		xStride = samples;
		xOffset = 0;
	}

	if (flipY){
		yStride = -samples * width;
		yOffset = (height - 1) * samples * width;
	}
	else {
		yStride = samples * width;
		yOffset = 0;
	}

	if (rotate){
		if (noAlpha){
			for (x = 0, line = image + xOffset; x < width; x++, line += xStride){
				for (y = 0, l = line + yOffset; y < height; y++, l += yStride, ptr += samples){
					ptr[0] = l[0];
					ptr[1] = l[1];
					ptr[2] = l[2];
				}
			}
		}
		else {
			for (x = 0, line = image + xOffset; x < width; x++, line += xStride){
				for (y = 0, l = line + yOffset; y < height; y++, l += yStride, ptr += samples){
					ptr[0] = l[0];
					ptr[1] = l[1];
					ptr[2] = l[2];
					ptr[3] = l[3];
				}
			}
		}
	}
	else {
		if (noAlpha){
			for (y = 0, line = image + yOffset; y < height; y++, line += yStride){
				for (x = 0, l = line + xOffset; x < width; x++, l += xStride, ptr += samples){
					ptr[0] = l[0];
					ptr[1] = l[1];
					ptr[2] = l[2];
				}
			}
		}
		else {
			for (y = 0, line = image + yOffset; y < height; y++, line += yStride){
				for (x = 0, l = line + xOffset; x < width; x++, l += xStride, ptr += samples){
					ptr[0] = l[0];
					ptr[1] = l[1];
					ptr[2] = l[2];
					ptr[3] = l[3];
				}
			}
		}
	}

	Mem_Free(image);

	return buffer;
}

/*
 ==================
 R_SelectImageFormat
 ==================
*/
static textureFormat_t R_SelectImageFormat (int numImages, byte **images, int width, int height, uint fourCC, bool alphaPixels, bool isCompressed, bool isNormalMap){

	textureFormat_t	format;
	bool			hasColor, hasAlpha;
	byte			*scan;
	int				size;
	int				i, j;

	// Special case for compressed images
	if (isCompressed){
		if (fourCC == DDS_FOURCC_DXT1){
			if (alphaPixels)
				format = TF_COMPRESSED_DXT1A;
			else
				format = TF_COMPRESSED_DXT1C;
		}
		else if (fourCC == DDS_FOURCC_DXT3)
			format = TF_COMPRESSED_DXT3;
		else if (fourCC == DDS_FOURCC_DXT5)
			format = TF_COMPRESSED_DXT5;
		else if (fourCC == DDS_FOURCC_RXGB)
			format = TF_COMPRESSED_RXGB;
		else
			Com_Error(ERR_DROP, "R_SelectImageFormat: bad compressed image format (%u)", fourCC);

		return format;
	}

	// Special case for normal maps
	if (isNormalMap)
		return TF_RGB;

	// Scan all the images
	hasColor = false;
	hasAlpha = false;

	size = width * height;

	for (i = 0; i < numImages; i++){
		scan = images[i];

		for (j = 0; j < size; j++){
			if (scan[0] != scan[1] || scan[1] != scan[2])
				hasColor = true;
			if (scan[3] != 255)
				hasAlpha = true;

			if (hasColor && hasAlpha)
				break;

			scan += 4;
		}

		if (hasColor && hasAlpha)
			break;
	}

	// Set format accordingly
	if (hasColor){
		if (hasAlpha)
			format = TF_RGBA;
		else
			format = TF_RGB;
	}
	else {
		if (hasAlpha)
			format = TF_LUMINANCE_ALPHA;
		else
			format = TF_LUMINANCE;
	}

	return format;
}

/*
 ==================
 R_LoadImageFormat
 ==================
*/
static bool R_LoadImageFormat (const char *name, const char *realName, textureWrap_t wrap, byte **image, int *width, int *height, bool isImageProgram){

	char	loadName[MAX_PATH_LENGTH];
	bool	failed = true;
	byte	*imgData;
	int		imgWidth, imgHeight;

	Str_SPrintf(loadName, sizeof(loadName), "%s.tga", name);
	if (R_LoadTGA(loadName, &imgData, &imgWidth, &imgHeight)){
		failed = false;

		*image = imgData;
		*width = imgWidth;
		*height = imgHeight;

		return true;
	}

	Str_SPrintf(loadName, sizeof(loadName), "%s.pcx", name);
	if (R_LoadPCX(loadName, &imgData, NULL, &imgWidth, &imgHeight)){
		failed = false;

		*image = imgData;
		*width = imgWidth;
		*height = imgHeight;

		return true;
	}

	Str_SPrintf(loadName, sizeof(loadName), "%s.wal", name);
	if (R_LoadWAL(loadName, &imgData, &imgWidth, &imgHeight)){
		failed = false;

		*image = imgData;
		*width = imgWidth;
		*height = imgHeight;

		return true;
	}

	if (failed){
		if (!isImageProgram)
			return false;

		if (!R_LoadImageProgram(name, &imgData, &imgWidth, &imgHeight, (wrap != TW_REPEAT && wrap != TW_REPEAT_MIRRORED)))
			return false;

		// Development tool
		if (r_writeImagePrograms->integerValue)
			R_WriteImageProgram(realName, imgData, imgWidth, imgHeight);

		*image = imgData;
		*width = imgWidth;
		*height = imgHeight;

		return true;
	}

	return false;
}

/*
 ==================
 R_LoadImage
 ==================
*/
bool R_LoadImage (const char *name, int flags, textureWrap_t wrap, byte **image, int *width, int *height, textureFormat_t *format, bool *uncompressed){

	char	realName[MAX_PATH_LENGTH], compressedName[MAX_PATH_LENGTH];
	bool	allowCompressed, isCompressed, isImageProgram;
	byte	*imgData;
	int		imgWidth, imgHeight;
	uint	imgFourCC;
	bool	imgAlphaPixels;
	int		resampleWidth, resampleHeight;

	// Determine if we should allow compressed images to be loaded
	if (!glConfig.textureCompressionS3TCAvailable || (flags & TF_UNCOMPRESSED))
		allowCompressed = false;
	else {
		if (flags & TF_BUMP)
			allowCompressed = r_compressNormalTextures->integerValue;
		else
			allowCompressed = r_compressTextures->integerValue;
	}

	// Load the image
	isImageProgram = R_ImageProgramToFileName(name, realName, compressedName, MAX_PATH_LENGTH);

	if (isImageProgram && r_forceImagePrograms->integerValue){
		isCompressed = false;

		if (!R_LoadImageProgram(name, &imgData, &imgWidth, &imgHeight, (wrap != TW_REPEAT && wrap != TW_REPEAT_MIRRORED)))
			return false;

		// Development tool
		if (r_writeImagePrograms->integerValue)
			R_WriteImageProgram(realName, imgData, imgWidth, imgHeight);
	}
	else {
		if (!allowCompressed || !r_precompressedImages->integerValue)
			isCompressed = false;
		else
			isCompressed = R_LoadDDS(compressedName, &imgData, &imgWidth, &imgHeight, &imgFourCC, &imgAlphaPixels);

		if (!isCompressed){
			if (!R_LoadImageFormat(name, realName, wrap, &imgData, &imgWidth, &imgHeight, isImageProgram))
				return false;
		}
	}

	// If not compressed
	if (!isCompressed){
		// Find nearest power of two
		if (r_roundImagesDown->integerValue){
			resampleWidth = FloorPowerOfTwo(imgWidth);
			resampleHeight = FloorPowerOfTwo(imgHeight);
		}
		else {
			resampleWidth = CeilPowerOfTwo(imgWidth);
			resampleHeight = CeilPowerOfTwo(imgHeight);
		}

		// Resample the image if needed
		if (resampleWidth != imgWidth || resampleHeight != imgHeight){
			imgData = R_ResampleImage(imgData, imgWidth, imgHeight, resampleWidth, resampleHeight, (flags & TF_BUMP));

			imgWidth = resampleWidth;
			imgHeight = resampleHeight;
		}
	}

	// Set parameters
	*image = imgData;

	*width = imgWidth;
	*height = imgHeight;

	*uncompressed = !isCompressed;

	// Select appropriate format
	*format = R_SelectImageFormat(1, image, imgWidth, imgHeight, imgFourCC, imgAlphaPixels, isCompressed, (flags & TF_BUMP));

	return true;
}

/*
 ==================
 R_LoadCubeImages
 ==================
*/
bool R_LoadCubeImages (const char *name, int flags, bool cameraSpace, byte **images, int *width, int *height, textureFormat_t *format, bool *uncompressed){

	char	realName[MAX_PATH_LENGTH], compressedName[MAX_PATH_LENGTH];
	bool	allowCompressed, isCompressed;
	byte	*imgData;
	int		imgWidth, imgHeight;
	uint	imgFourCC;
	bool	imgAlphaPixels;
	int		cubeWidth, cubeHeight;
	uint	cubeFourCC;
	bool	cubeAlphaPixels;
	int		resampleWidth, resampleHeight;
	int		i, j;

	// Determine if we should allow compressed images to be loaded
	if (!glConfig.textureCompressionS3TCAvailable || (flags & TF_UNCOMPRESSED))
		allowCompressed = false;
	else {
		if (flags & TF_BUMP)
			allowCompressed = r_compressNormalTextures->integerValue;
		else
			allowCompressed = r_compressTextures->integerValue;
	}

	// Load all the images
	for (i = 0; i < 6; i++){
		if (i == 0){
			if (!allowCompressed || !r_precompressedImages->integerValue)
				isCompressed = false;
			else {
				Str_SPrintf(compressedName, sizeof(compressedName), "%s_%s.dds", name, r_cubeMapFaces[cameraSpace][i].suffix);

				isCompressed = R_LoadDDS(compressedName, &imgData, &imgWidth, &imgHeight, &imgFourCC, &imgAlphaPixels);
			}

			if (!isCompressed){
				Str_SPrintf(realName, sizeof(realName), "%s_%s.tga", name, r_cubeMapFaces[cameraSpace][i].suffix);

				if (!R_LoadTGA(realName, &imgData, &imgWidth, &imgHeight))
					break;

				// Find nearest power of two
				if (r_roundImagesDown->integerValue){
					resampleWidth = FloorPowerOfTwo(imgWidth);
					resampleHeight = FloorPowerOfTwo(imgHeight);
				}
				else {
					resampleWidth = CeilPowerOfTwo(imgWidth);
					resampleHeight = CeilPowerOfTwo(imgHeight);
				}
			}

			// Check dimensions
			if (imgWidth != imgHeight){
				Com_Printf(S_COLOR_YELLOW "WARNING: cube map face '%s_%s' is not square (%i != %i)\n", name, r_cubeMapFaces[cameraSpace][i].suffix, imgWidth, imgHeight);

				Mem_Free(imgData);
				break;
			}

			// Save dimensions and compressed format for checking the rest of
			// the faces
			cubeWidth = imgWidth;
			cubeHeight = imgHeight;

			cubeFourCC = imgFourCC;
			cubeAlphaPixels = imgAlphaPixels;
		}
		else {
			if (isCompressed){
				Str_SPrintf(compressedName, sizeof(compressedName), "%s_%s.dds", name, r_cubeMapFaces[cameraSpace][i].suffix);

				if (!R_LoadDDS(compressedName, &imgData, &imgWidth, &imgHeight, &imgFourCC, &imgAlphaPixels))
					break;
			}
			else {
				Str_SPrintf(realName, sizeof(realName), "%s_%s.tga", name, r_cubeMapFaces[cameraSpace][i].suffix);

				if (!R_LoadTGA(realName, &imgData, &imgWidth, &imgHeight))
					break;
			}

			// Check dimensions
			if (imgWidth != cubeWidth || imgHeight != cubeHeight){
				Com_Printf(S_COLOR_YELLOW "WARNING: cube map face '%s_%s' has mismatched dimensions (%i x %i != %i x %i)\n", name, r_cubeMapFaces[cameraSpace][i].suffix, imgWidth, imgHeight, cubeWidth, cubeHeight);

				Mem_Free(imgData);
				break;
			}

			// If compressed, check format
			if (isCompressed){
				if (imgFourCC != cubeFourCC || imgAlphaPixels != cubeAlphaPixels){
					Com_Printf(S_COLOR_YELLOW "WARNING: cube map face '%s_%s' has mismatched compressed format (%u != u or %i != %i)\n", name, r_cubeMapFaces[cameraSpace][i].suffix, imgFourCC, cubeFourCC, imgAlphaPixels, cubeAlphaPixels);

					Mem_Free(imgData);
					break;
				}
			}
		}

		// If not compressed
		if (!isCompressed){
			// Resample the image if needed
			if (resampleWidth != imgWidth || resampleHeight != imgHeight){
				imgData = R_ResampleImage(imgData, imgWidth, imgHeight, resampleWidth, resampleHeight, (flags & TF_BUMP));

				imgWidth = resampleWidth;
				imgHeight = resampleHeight;
			}

			// Flip and rotate the image if needed
			if (r_cubeMapFaces[cameraSpace][i].flipX || r_cubeMapFaces[cameraSpace][i].flipY || r_cubeMapFaces[cameraSpace][i].rotate)
				imgData = R_FlipAndRotateImage(imgData, imgWidth, imgHeight, false, r_cubeMapFaces[cameraSpace][i].flipX, r_cubeMapFaces[cameraSpace][i].flipY, r_cubeMapFaces[cameraSpace][i].rotate);
		}

		// Set parameters
		images[i] = imgData;

		*width = imgWidth;
		*height = imgHeight;

		*uncompressed = !isCompressed;
	}

	if (i != 6){
		for (j = 0; j < i; j++)
			Mem_Free(images[j]);

		return false;
	}

	// Select appropriate format
	*format = R_SelectImageFormat(6, images, imgWidth, imgHeight, imgFourCC, imgAlphaPixels, isCompressed, (flags & TF_BUMP));

	return true;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_FindScreenshotName
 ==================
*/
static bool R_FindScreenshotName (char *name, int maxLength){

	static int	shotNumber;

	while (shotNumber <= 9999){
		Str_SPrintf(name, maxLength, "screenshots/shot_%04i.tga", shotNumber);

		if (!FS_FileExists(name))
			break;

		shotNumber++;
	}

	if (shotNumber == 10000)
		return false;

	shotNumber++;

	return true;
}

/*
 ==================
 R_TakeScreenshot
 ==================
*/
bool R_TakeScreenshot (const char *name, bool gammaCorrect){

	byte	*buffer;
	int		i, size;

	// Allocate the buffer
	size = 18 + (glConfig.videoWidth * glConfig.videoHeight * 3);
	buffer = (byte *)Mem_Alloc(size, TAG_TEMPORARY);

	// Set up the header
	Mem_Fill(buffer, 0, 18);

	buffer[2] = TGA_IMAGE_TRUECOLOR;
	buffer[12] = glConfig.videoWidth & 255;
	buffer[13] = glConfig.videoWidth >> 8;
	buffer[14] = glConfig.videoHeight & 255;
	buffer[15] = glConfig.videoHeight >> 8;
	buffer[16] = 24;

	// Read the pixels
	qglPixelStorei(GL_PACK_ALIGNMENT, 1);

	qglReadBuffer(GL_FRONT);
	qglReadPixels(0, 0, glConfig.videoWidth, glConfig.videoHeight, GL_BGR, GL_UNSIGNED_BYTE, buffer + 18);

	qglPixelStorei(GL_PACK_ALIGNMENT, 4);

	// Apply gamma correction if desired
	if (gammaCorrect){
		for (i = 18; i < size; i += 3){
			buffer[i+0] = rg.gammaTable[buffer[i+0]];
			buffer[i+1] = rg.gammaTable[buffer[i+1]];
			buffer[i+2] = rg.gammaTable[buffer[i+2]];
		}
	}

	// Write the image
	if (!FS_WriteFile(name, buffer, size)){
		Mem_Free(buffer);
		return false;
	}

	Mem_Free(buffer);

	return true;
}

/*
 ==================
 R_Screenshot_f
 ==================
*/
static void R_Screenshot_f (){

	char	name[MAX_PATH_LENGTH];

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: screenshot [name]\n");
		return;
	}

	// Find a file name to save it to
	if (Cmd_Argc() != 1){
		Str_SPrintf(name, sizeof(name), "screenshots/%s", Cmd_Argv(1));
		Str_DefaultFileExtension(name, sizeof(name), ".tga");
	}
	else {
		if (!R_FindScreenshotName(name, sizeof(name))){
			Com_Printf("Screenshots directory is full!\n");
			return;
		}
	}

	// Take the screenshot
	if (!R_TakeScreenshot(name, true)){
		Com_Printf("Couldn't write %s\n", name);
		return;
	}

	Com_Printf("Wrote %s\n", name);
}

/*
 ==================
 R_EnvShot_f
 ==================
*/
static void R_EnvShot_f (){

	primaryView_t	oldPrimaryView;
	char			baseName[MAX_PATH_LENGTH], realName[MAX_PATH_LENGTH];
	byte			*pixels;
	byte			*buffer;
	int				i, size;

	if (Cmd_Argc() < 2 || Cmd_Argc() > 3){
		Com_Printf("Usage: envShot <baseName> [size]\n");
		return;
	}

	if (!rg.primaryViewAvailable){
		Com_Printf("No primaryView available\n");
		return;
	}

	Str_Copy(baseName, Cmd_Argv(1), sizeof(baseName));

	if (Cmd_Argc() == 2)
		size = 256;
	else {
		size = Str_ToInteger(Cmd_Argv(2));

		if (size < 32)
			size = 32;
		else
			size = FloorPowerOfTwo(size);
	}

	// Make sure the specified size is valid
	if (size > glConfig.videoWidth || size > glConfig.videoHeight){
		Com_Printf("Specified size is greater than current resolution\n");
		return;
	}

	// Allocate the buffers
	pixels = (byte *)Mem_Alloc(size * size * 3, TAG_TEMPORARY);
	buffer = (byte *)Mem_Alloc(18 + (size * size * 3), TAG_TEMPORARY);

	// Set up the header
	Mem_Fill(buffer, 0, 18);

	buffer[2] = TGA_IMAGE_TRUECOLOR;
	buffer[12] = size & 255;
	buffer[13] = size >> 8;
	buffer[14] = size & 255;
	buffer[15] = size >> 8;
	buffer[16] = 24;

	// Backup primary view
	Mem_Copy(&oldPrimaryView, &rg.primaryView, sizeof(primaryView_t));

	// Render a frame for each face of the cube
	rg.envShotRendering = true;
	rg.envShotSize = size;

	for (i = 0; i < 6; i++){
		// Set up the view
		AnglesToMat3(r_envShotFaces[i].axis, rg.primaryView.renderView.axis);

		// Render the scene
		R_BeginFrame(rg.time);

		rg.scene.numEntities = rg.primaryView.numEntities;
		rg.scene.firstEntity = rg.primaryView.firstEntity;

		rg.scene.numLights = rg.primaryView.numLights;
		rg.scene.firstLight = rg.primaryView.firstLight;

		rg.scene.numParticles = rg.primaryView.numParticles;
		rg.scene.firstParticle = rg.primaryView.firstParticle;

		R_RenderScene(&rg.primaryView.renderView, true);

		R_EndFrame();

		// Read the pixels
		qglPixelStorei(GL_PACK_ALIGNMENT, 1);

		qglReadBuffer(GL_FRONT);
		qglReadPixels(0, 0, size, size, GL_BGR, GL_UNSIGNED_BYTE, pixels);

		qglPixelStorei(GL_PACK_ALIGNMENT, 4);

		// Flip and rotate accordingly
		pixels = R_FlipAndRotateImage(pixels, size, size, true, r_envShotFaces[i].flipX, r_envShotFaces[i].flipY, r_envShotFaces[i].rotate);

		// Copy the pixels
		Mem_Copy(buffer + 18, pixels, size * size * 3);

		// Write the image
		Str_SPrintf(realName, sizeof(realName), "env/%s_%s.tga", baseName, r_envShotFaces[i].suffix);

		if (!FS_WriteFile(realName, buffer, 18 + (size * size * 3))){
			Com_Printf("Couldn't write env/%s_*.tga\n", baseName);

			rg.envShotRendering = false;
			rg.envShotSize = 0;

			// Restore primary view
			Mem_Copy(&rg.primaryView.renderView, &oldPrimaryView, sizeof(primaryView_t));

			// Free the buffers
			Mem_Free(pixels);
			Mem_Free(buffer);

			return;
		}
	}

	rg.envShotRendering = false;
	rg.envShotSize = 0;

	// Restore primary view
	Mem_Copy(&rg.primaryView.renderView, &oldPrimaryView, sizeof(primaryView_t));

	// Free the buffers
	Mem_Free(pixels);
	Mem_Free(buffer);

	Com_Printf("Wrote env/%s_*.tga\n", baseName);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_InitImages
 ==================
*/
void R_InitImages (){

	int		i;
	int		r, g, b;
	byte	*palette;
	byte	defaultPalette[] = {
#include "palette.h"
	};

	// Add commands
	Cmd_AddCommand("screenshot", R_Screenshot_f, "Takes a screenshot", NULL);
	Cmd_AddCommand("envShot", R_EnvShot_f, "Takes an environment shot", NULL);

	// Build luminance table
	for (i = 0; i < 256; i++){
		r_luminanceTable[i][0] = i * 0.2125f;
		r_luminanceTable[i][1] = i * 0.7154f;
		r_luminanceTable[i][2] = i * 0.0721f;
	}

	// Load the palette
	if (!R_LoadPCX("pics/colormap.pcx", NULL, &palette, NULL, NULL))
		palette = defaultPalette;

	for (i = 0; i < 256; i++){
		r = palette[i*3+0];
		g = palette[i*3+1];
		b = palette[i*3+2];

		r_palette[i] = (r << 0) + (g << 8) + (b << 16) + (255 << 24);
		r_palette[i] = LittleLong(r_palette[i]);
	}
	r_palette[255] &= LittleLong(0x00FFFFFF);	// 255 is transparent

	if (palette != defaultPalette)
		Mem_Free(palette);
}

/*
 ==================
 R_ShutdownImages
 ==================
*/
void R_ShutdownImages (){

	// Remove commands
	Cmd_RemoveCommand("screenshot");
	Cmd_RemoveCommand("envShot");
}