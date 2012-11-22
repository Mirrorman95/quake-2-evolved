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
// r_texture.c - GL texture manager
//


#include "r_local.h"


#define TEXTURES_HASH_SIZE			(MAX_TEXTURES >> 2)

typedef struct {
	const char *			name;

	int						min;
	int						mag;
} filterTable_t;

static filterTable_t		r_filterTable[] = {
	{"GL_NEAREST"               , GL_NEAREST               , GL_NEAREST},
	{"GL_LINEAR"                , GL_LINEAR                , GL_LINEAR },
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST" , GL_LINEAR_MIPMAP_NEAREST , GL_LINEAR },
	{"GL_NEAREST_MIPMAP_LINEAR" , GL_NEAREST_MIPMAP_LINEAR , GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR"  , GL_LINEAR_MIPMAP_LINEAR  , GL_LINEAR },
	{NULL                       , 0                        , 0}
};

static int					r_mipBlendColors[16][4] = {
	{   0,   0,   0,   0},
	{ 255,   0,   0, 128},
	{   0, 255,   0, 128},
	{   0,   0, 255, 128},
	{ 255,   0,   0, 128},
	{   0, 255,   0, 128},
	{   0,   0, 255, 128},
	{ 255,   0,   0, 128},
	{   0, 255,   0, 128},
	{   0,   0, 255, 128},
	{ 255,   0,   0, 128},
	{   0, 255,   0, 128},
	{   0,   0, 255, 128},
	{ 255,   0,   0, 128},
	{   0, 255,   0, 128},
	{   0,   0, 255, 128}
};

static int					r_filterMin = GL_LINEAR_MIPMAP_LINEAR;
static int					r_filterMag = GL_LINEAR;

static texture_t *			r_texturesHashTable[TEXTURES_HASH_SIZE];
static texture_t *			r_textures[MAX_TEXTURES];
static int					r_numTextures;


/*
 ==============================================================================

 TEXTURE REGISTRATION

 ==============================================================================
*/


/*
 ==================
 R_MipMap

 Operates in place, quartering the size of the texture
 ==================
*/
static void R_MipMap (byte *in, int width, int height, bool isNormalMap){

	byte	*out = in;
	vec3_t	normal;
	int		row;
	int		x, y;

	row = width << 2;

	width >>= 1;
	height >>= 1;

	// Special case for normal maps
	if (isNormalMap){
		if (width == 0 || height == 0){
			width += height;

			for (x = 0; x < width; x++, in += 8, out += 4){
				normal[0] = (in[0] * (1.0f / 127.0f) - 1.0f) + (in[4] * (1.0f / 127.0f) - 1.0f);
				normal[1] = (in[1] * (1.0f / 127.0f) - 1.0f) + (in[5] * (1.0f / 127.0f) - 1.0f);
				normal[2] = (in[2] * (1.0f / 127.0f) - 1.0f) + (in[6] * (1.0f / 127.0f) - 1.0f);

				if (normal[2] <= 0.0f || !VectorNormalize(normal))
					VectorSet(normal, 0.0f, 0.0f, 1.0f);

				out[0] = 128 + FloatToInt(127.0f * normal[0]);
				out[1] = 128 + FloatToInt(127.0f * normal[1]);
				out[2] = 128 + FloatToInt(127.0f * normal[2]);
				out[3] = 255;
			}
		}
		else {
			for (y = 0; y < height; y++, in += row){
				for (x = 0; x < width; x++, in += 8, out += 4){
					normal[0] = (in[0] * (1.0f / 127.0f) - 1.0f) + (in[4] * (1.0f / 127.0f) - 1.0f) + (in[row+0] * (1.0f / 127.0f) - 1.0f) + (in[row+4] * (1.0f / 127.0f) - 1.0f);
					normal[1] = (in[1] * (1.0f / 127.0f) - 1.0f) + (in[5] * (1.0f / 127.0f) - 1.0f) + (in[row+1] * (1.0f / 127.0f) - 1.0f) + (in[row+5] * (1.0f / 127.0f) - 1.0f);
					normal[2] = (in[2] * (1.0f / 127.0f) - 1.0f) + (in[6] * (1.0f / 127.0f) - 1.0f) + (in[row+2] * (1.0f / 127.0f) - 1.0f) + (in[row+6] * (1.0f / 127.0f) - 1.0f);

					if (normal[2] <= 0.0f || !VectorNormalize(normal))
						VectorSet(normal, 0.0f, 0.0f, 1.0f);

					out[0] = 128 + FloatToInt(127.0f * normal[0]);
					out[1] = 128 + FloatToInt(127.0f * normal[1]);
					out[2] = 128 + FloatToInt(127.0f * normal[2]);
					out[3] = 255;
				}
			}
		}

		return;
	}

	// General case
	if (width == 0 || height == 0){
		width += height;

		for (x = 0; x < width; x++, in += 8, out += 4){
			out[0] = (in[0] + in[4]) >> 1;
			out[1] = (in[1] + in[5]) >> 1;
			out[2] = (in[2] + in[6]) >> 1;
			out[3] = (in[3] + in[7]) >> 1;
		}
	}
	else {
		for (y = 0; y < height; y++, in += row){
			for (x = 0; x < width; x++, in += 8, out += 4){
				out[0] = (in[0] + in[4] + in[row+0] + in[row+4]) >> 2;
				out[1] = (in[1] + in[5] + in[row+1] + in[row+5]) >> 2;
				out[2] = (in[2] + in[6] + in[row+2] + in[row+6]) >> 2;
				out[3] = (in[3] + in[7] + in[row+3] + in[row+7]) >> 2;
			}
		}
	}
}

/*
 ==================
 R_BlendOverTexture

 Applies a color blend over a set of pixels
 ==================
*/
static byte *R_BlendOverTexture (byte *pixels, int pixelCount, const int blend[4]){

	byte	*buffer, *ptr;
	int		inverseAlpha;
	int		premult[3];
	int		i;

	buffer = ptr = (byte *)Mem_ClearedAlloc(pixelCount * 4, TAG_TEMPORARY);

	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	inverseAlpha = 255 - blend[3];

	for (i = 0; i < pixelCount; i++, pixels += 4, ptr += 4){
		ptr[0] = (pixels[0] * inverseAlpha + premult[0]) >> 9;
		ptr[1] = (pixels[1] * inverseAlpha + premult[1]) >> 9;
		ptr[2] = (pixels[2] * inverseAlpha + premult[2]) >> 9;
		ptr[3] = pixels[3];
	}

	return buffer;
}

/*
 ==================
 R_SetTextureDimensions
 ==================
*/
static void R_SetTextureDimensions (texture_t *texture, int width, int height, int depth){

	int		mipLevel;
	int		maxTextureSize;

	// Select mip level and max texture size
	if (texture->flags & TF_NOPICMIP){
		mipLevel = 0;

		if (texture->type == TT_2D)
			maxTextureSize = glConfig.maxTextureSize;
		else if (texture->type == TT_3D)
			maxTextureSize = glConfig.max3DTextureSize;
		else if (texture->type == TT_CUBE)
			maxTextureSize = glConfig.maxCubeMapTextureSize;
		else
			maxTextureSize = glConfig.maxTextureSize;
	}
	else {
		if (texture->flags & TF_BUMP)
			mipLevel = r_mipLevelBump->integerValue;
		else if (texture->flags & TF_DIFFUSE)
			mipLevel = r_mipLevelDiffuse->integerValue;
		else if (texture->flags & TF_SPECULAR)
			mipLevel = r_mipLevelSpecular->integerValue;
		else
			mipLevel = r_mipLevel->integerValue;

		if (texture->type == TT_2D)
			maxTextureSize = Min(r_maxTextureSize->integerValue, glConfig.maxTextureSize);
		else if (texture->type == TT_3D)
			maxTextureSize = Min(r_maxTextureSize->integerValue, glConfig.max3DTextureSize);
		else if (texture->type == TT_CUBE)
			maxTextureSize = Min(r_maxTextureSize->integerValue, glConfig.maxCubeMapTextureSize);
		else
			maxTextureSize = Min(r_maxTextureSize->integerValue, glConfig.maxTextureSize);
	}

	// Downsample if desired and clamp to max texture size or hardware limits
	// if needed
	if (texture->type != TT_3D){
		width >>= mipLevel;
		height >>= mipLevel;

		while (width > maxTextureSize || height > maxTextureSize){
			width >>= 1;
			height >>= 1;
		}
	}
	else {
		width >>= mipLevel;
		height >>= mipLevel;
		depth >>= mipLevel;

		while (width > maxTextureSize || height > maxTextureSize || depth > maxTextureSize){
			width >>= 1;
			height >>= 1;
			depth >>= 1;
		}
	}

	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;
	if (depth < 1)
		depth = 1;

	// Set the texture dimensions
	texture->width = width;
	texture->height = height;
	texture->depth = depth;
}

/*
 ==================
 R_SetTextureInternalFormat
 ==================
*/
static void R_SetTextureInternalFormat (texture_t *texture, bool isCompressed){

	bool	useCompression;

	// Special case for compressed textures
	if (isCompressed){
		switch (texture->format){
		case TF_COMPRESSED_DXT1C:
			texture->internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			break;
		case TF_COMPRESSED_DXT1A:
			texture->internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			break;
		case TF_COMPRESSED_DXT3:
			texture->internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			break;
		case TF_COMPRESSED_DXT5:
			texture->internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
		case TF_COMPRESSED_RXGB:
			texture->internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
		default:
			Com_Error(ERR_DROP, "R_SetTextureInternalFormat: bad texture format (%i)", texture->format);
		}

		return;
	}

	// Determine if it should be compressed
	if (texture->type != TT_2D && texture->type != TT_CUBE)
		useCompression = false;
	else {
		if (!glConfig.textureCompressionS3TCAvailable || (texture->flags & TF_UNCOMPRESSED))
			useCompression = false;
		else {
			if (texture->flags & TF_BUMP)
				useCompression = r_compressNormalTextures->integerValue;
			else
				useCompression = r_compressTextures->integerValue;
		}
	}

	// Set the texture internal format
	if (useCompression){
		if (texture->format == TF_LUMINANCE || texture->format == TF_RGB)
			texture->internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		else {
			if (texture->flags & TF_DIFFUSE)
				texture->internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			else
				texture->internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}

		return;
	}

	switch (texture->format){
	case TF_LUMINANCE:
		texture->internalFormat = GL_LUMINANCE8;
		break;
	case TF_LUMINANCE_ALPHA:
		texture->internalFormat = GL_LUMINANCE8_ALPHA8;
		break;
	case TF_RGB:
		texture->internalFormat = GL_RGB8;
		break;
	case TF_RGBA:
		texture->internalFormat = GL_RGBA8;
		break;
	case TF_DEPTH_16:
		texture->internalFormat = GL_DEPTH_COMPONENT16;
		break;
	case TF_DEPTH_24:
		texture->internalFormat = GL_DEPTH_COMPONENT24;
		break;
	default:
		Com_Error(ERR_DROP, "R_SetTextureInternalFormat: bad texture format (%i)", texture->format);
	}
}

/*
 ==================
 R_SetTextureParameters
 ==================
*/
static void R_SetTextureParameters (texture_t *texture){

	vec4_t	zeroBorder = {0.0f, 0.0f, 0.0f, 1.0f}, zeroAlphaBorder = {1.0f, 1.0f, 1.0f, 0.0f};

	// Set texture swizzle
	switch (texture->format){
	case TF_LUMINANCE:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ONE);

		break;
	case TF_LUMINANCE_ALPHA:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

		break;
	case TF_RGB:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ONE);

		break;
	case TF_RGBA:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

		break;
	case TF_DEPTH_16:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_ONE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_ONE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ONE);

		break;
	case TF_DEPTH_24:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_ONE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_ONE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ONE);

		break;
	case TF_COMPRESSED_DXT1C:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ONE);

		break;
	case TF_COMPRESSED_DXT1A:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

		break;
	case TF_COMPRESSED_DXT3:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

		break;
	case TF_COMPRESSED_DXT5:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_RED);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ALPHA);

		break;
	case TF_COMPRESSED_RXGB:
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_G, GL_GREEN);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_B, GL_BLUE);
		qglTexParameteri(texture->target, GL_TEXTURE_SWIZZLE_A, GL_ONE);

		break;
	default:
		Com_Error(ERR_DROP, "R_SetTextureParameters: bad texture format (%i)", texture->format);
	}

	// Set texture filter
	switch (texture->filter){
	case TF_DEFAULT:
		qglTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, r_filterMin);
		qglTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, r_filterMag);

		// Also set texture LOD bias
		qglTexParameterf(texture->target, GL_TEXTURE_LOD_BIAS, r_textureLODBias->floatValue);

		// Also set texture anisotropy if available
		if (glConfig.textureFilterAnisotropicAvailable)
			qglTexParameterf(texture->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_textureAnisotropy->floatValue);

		break;
	case TF_NEAREST:
		qglTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		qglTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		break;
	case TF_LINEAR:
		qglTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		break;
	default:
		Com_Error(ERR_DROP, "R_SetTextureParameters: bad texture filter (%i)", texture->filter);
	}

	// Set texture wrap
	switch (texture->wrap){
	case TW_REPEAT:
		qglTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (texture->type == TT_3D)
			qglTexParameteri(texture->target, GL_TEXTURE_WRAP_R, GL_REPEAT);

		break;
	case TW_REPEAT_MIRRORED:
		qglTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		qglTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

		if (texture->type == TT_3D)
			qglTexParameteri(texture->target, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);

		break;
	case TW_CLAMP:
		qglTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		if (texture->type == TT_3D)
			qglTexParameteri(texture->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		break;
	case TW_CLAMP_TO_ZERO:
		qglTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		qglTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		if (texture->type == TT_3D)
			qglTexParameteri(texture->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

		qglTexParameterfv(texture->target, GL_TEXTURE_BORDER_COLOR, zeroBorder);

		break;
	case TW_CLAMP_TO_ZERO_ALPHA:
		qglTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		qglTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		if (texture->type == TT_3D)
			qglTexParameteri(texture->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

		qglTexParameterfv(texture->target, GL_TEXTURE_BORDER_COLOR, zeroAlphaBorder);

		break;
	default:
		Com_Error(ERR_DROP, "R_SetTextureParameters: bad texture wrap (%i)", texture->wrap);
	}
}

/*
 ==================
 R_UploadTexture
 ==================
*/
static void R_UploadTexture (texture_t *texture, int numImages, byte **images, int width, int height){

	byte	*data, *blend;
	int		mipLevel, mipWidth, mipHeight;
	int		i;

	// Set the texture dimensions
	R_SetTextureDimensions(texture, width, height, 1);

	// Set the texture internal format
	R_SetTextureInternalFormat(texture, false);

	// Bind the texture
	qglGenTextures(1, &texture->textureId);

	GL_BindTexture(texture);

	// Upload the images
	for (i = 0; i < numImages; i++){
		data = images[i];

		// Downsample if desired
		if (width > texture->width || height > texture->height){
			mipWidth = width;
			mipHeight = height;

			while (mipWidth > texture->width || mipHeight > texture->height){
				R_MipMap(data, mipWidth, mipHeight, (texture->flags & TF_BUMP));

				mipWidth >>= 1;
				if (mipWidth < 1)
					mipWidth = 1;

				mipHeight >>= 1;
				if (mipHeight < 1)
					mipHeight = 1;
			}
		}

		// Upload the base texture
		if (texture->type == TT_2D){
			if (texture->format != TF_DEPTH_16 && texture->format != TF_DEPTH_24)
				qglTexImage2D(GL_TEXTURE_2D, 0, texture->internalFormat, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			else
				qglTexImage2D(GL_TEXTURE_2D, 0, texture->internalFormat, texture->width, texture->height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, data);
		}
		else {
			if (texture->format != TF_DEPTH_16 && texture->format != TF_DEPTH_24)
				qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, texture->internalFormat, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			else
				qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, texture->internalFormat, texture->width, texture->height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, data);
		}

		// Generate mipmaps if desired
		if (texture->filter == TF_DEFAULT){
			mipLevel = 0;
			mipWidth = texture->width;
			mipHeight = texture->height;

			while (mipWidth > 1 || mipHeight > 1){
				// Build the mipmap
				R_MipMap(data, mipWidth, mipHeight, (texture->flags & TF_BUMP));

				mipLevel++;

				mipWidth >>= 1;
				if (mipWidth < 1)
					mipWidth = 1;

				mipHeight >>= 1;
				if (mipHeight < 1)
					mipHeight = 1;

				// Development tool
				if (r_colorMipLevels->integerValue && !(texture->flags & (TF_INTERNAL | TF_BUMP))){
					if (texture->format == TF_RGB || texture->format == TF_RGBA){
						blend = R_BlendOverTexture(data, mipWidth * mipHeight, r_mipBlendColors[mipLevel]);

						// Upload the mipmap texture
						if (texture->type == TT_2D)
							qglTexImage2D(GL_TEXTURE_2D, mipLevel, texture->internalFormat, mipWidth, mipHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, blend);
						else
							qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipLevel, texture->internalFormat, mipWidth, mipHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, blend);

						Mem_Free(blend);

						continue;
					}
				}

				// Upload the mipmap texture
				if (texture->type == TT_2D){
					if (texture->format != TF_DEPTH_16 && texture->format != TF_DEPTH_24)
						qglTexImage2D(GL_TEXTURE_2D, mipLevel, texture->internalFormat, mipWidth, mipHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
					else
						qglTexImage2D(GL_TEXTURE_2D, mipLevel, texture->internalFormat, mipWidth, mipHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, data);
				}
				else {
					if (texture->format != TF_DEPTH_16 && texture->format != TF_DEPTH_24)
						qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipLevel, texture->internalFormat, mipWidth, mipHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
					else
						qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipLevel, texture->internalFormat, mipWidth, mipHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, data);
				}
			}
		}
	}

	// Set texture swizzle, filter, wrap, etc
	R_SetTextureParameters(texture);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 ==================
 R_UploadCompressedTexture
 ==================
*/
static void R_UploadCompressedTexture (texture_t *texture, int numImages, byte **images, int width, int height){

	byte	*data;
	int		size;
	int		mipLevel, mipWidth, mipHeight;
	int		i;

	// Set the texture dimensions
	R_SetTextureDimensions(texture, width, height, 1);

	// Set the texture internal format
	R_SetTextureInternalFormat(texture, true);

	// Bind the texture
	qglGenTextures(1, &texture->textureId);

	GL_BindTexture(texture);

	// Upload the images
	for (i = 0; i < numImages; i++){
		data = images[i];

		// Downsample if desired
		if (width > texture->width || height > texture->height){
			mipWidth = width;
			mipHeight = height;

			while (mipWidth > texture->width || mipHeight > texture->height){
				if (texture->format == TF_COMPRESSED_DXT1C || texture->format == TF_COMPRESSED_DXT1A)
					data += ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2) * 8;
				else
					data += ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2) * 16;

				mipWidth >>= 1;
				if (mipWidth < 1)
					mipWidth = 1;

				mipHeight >>= 1;
				if (mipHeight < 1)
					mipHeight = 1;
			}
		}

		// Upload the base texture
		if (texture->format == TF_COMPRESSED_DXT1C || texture->format == TF_COMPRESSED_DXT1A)
			size = ((texture->width + 3) >> 2) * ((texture->height + 3) >> 2) * 8;
		else
			size = ((texture->width + 3) >> 2) * ((texture->height + 3) >> 2) * 16;

		if (texture->type == TT_2D)
			qglCompressedTexImage2D(GL_TEXTURE_2D, 0, texture->internalFormat, texture->width, texture->height, 0, size, data);
		else
			qglCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipLevel, texture->internalFormat, texture->width, texture->height, 0, size, data);

		// Generate mipmaps if desired
		if (texture->filter == TF_DEFAULT){
			mipLevel = 0;
			mipWidth = texture->width;
			mipHeight = texture->height;

			while (mipWidth > 1 || mipHeight > 1){
				// Skip to next mipmap
				data += size;

				mipLevel++;

				mipWidth >>= 1;
				if (mipWidth < 1)
					mipWidth = 1;

				mipHeight >>= 1;
				if (mipHeight < 1)
					mipHeight = 1;

				// Upload the mipmap texture
				if (texture->format == TF_COMPRESSED_DXT1C || texture->format == TF_COMPRESSED_DXT1A)
					size = ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2) * 8;
				else
					size = ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2) * 16;

				if (texture->type == TT_2D)
					qglCompressedTexImage2D(GL_TEXTURE_2D, mipLevel, texture->internalFormat, mipWidth, mipHeight, 0, size, data);
				else
					qglCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipLevel, texture->internalFormat, mipWidth, mipHeight, 0, size, data);
			}
		}
	}

	// Set texture swizzle, filter, wrap, etc
	R_SetTextureParameters(texture);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 ==================
 R_UploadVolumeTexture
 ==================
*/
static void R_UploadVolumeTexture (texture_t *texture, byte *image, int width, int height, int depth){

	// Set the texture dimensions
	R_SetTextureDimensions(texture, width, height, depth);

	// Set the texture internal format
	R_SetTextureInternalFormat(texture, false);

	// Bind the texture
	qglGenTextures(1, &texture->textureId);

	GL_BindTexture(texture);

	// Upload the base texture
	qglTexImage3D(texture->target, 0, texture->internalFormat, texture->width, texture->height, texture->depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	// Set texture swizzle, filter, wrap, etc
	R_SetTextureParameters(texture);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 ==================
 R_UploadArrayTexture
 ==================
*/
static void R_UploadArrayTexture (texture_t *texture, byte *image, int width, int height, int layers){

	// Set the texture dimensions
	R_SetTextureDimensions(texture, width, height, layers);

	// Set the texture internal format
	R_SetTextureInternalFormat(texture, false);

	// Bind the texture
	qglGenTextures(1, &texture->textureId);

	GL_BindTexture(texture);

	// Upload the base texture
	if (texture->format != TF_DEPTH_16 && texture->format != TF_DEPTH_24)
		qglTexImage3D(texture->target, 0, texture->internalFormat, texture->width, texture->height, texture->depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	else
		qglTexImage3D(texture->target, 0, texture->internalFormat, texture->width, texture->height, texture->depth, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, image);

	// Set texture swizzle, filter, wrap, etc
	R_SetTextureParameters(texture);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 ==================
 R_LoadTexture
 ==================
*/
static texture_t *R_LoadTexture (const char *name, byte *image, int width, int height, int flags, textureFormat_t format, textureFilter_t filter, textureWrap_t wrap, bool uncompressed){

	texture_t	*texture;
	uint		hashKey;

	if (r_numTextures == MAX_TEXTURES)
		Com_Error(ERR_DROP, "R_LoadTexture: MAX_TEXTURES hit");

	r_textures[r_numTextures++] = texture = (texture_t *)Mem_ClearedAlloc(sizeof(texture_t), TAG_RENDERER);

	// Fill it in
	Str_Copy(texture->name, name, sizeof(texture->name));
	texture->type = TT_2D;
	texture->flags = flags;
	texture->format = format;
	texture->filter = filter;
	texture->wrap = wrap;
	texture->size = 0;
	texture->target = GL_TEXTURE_2D;
	texture->frameUsed = 0;

	if (uncompressed)
		R_UploadTexture(texture, 1, &image, width, height);
	else
		R_UploadCompressedTexture(texture, 1, &image, width, height);

	R_SetTextureSize(texture);

	// Add to hash table
	hashKey = Str_HashKey(texture->name, TEXTURES_HASH_SIZE, false);

	texture->nextHash = r_texturesHashTable[hashKey];
	r_texturesHashTable[hashKey] = texture;

	return texture;
}

/*
 ==================
 R_LoadCubeTexture
 ==================
*/
static texture_t *R_LoadCubeTexture (const char *name, byte **images, int width, int height, int flags, textureFormat_t format, textureFilter_t filter, bool uncompressed){

	texture_t	*texture;
	uint		hashKey;

	if (r_numTextures == MAX_TEXTURES)
		Com_Error(ERR_DROP, "R_LoadCubeTexture: MAX_TEXTURES hit");

	r_textures[r_numTextures++] = texture = (texture_t *)Mem_ClearedAlloc(sizeof(texture_t), TAG_RENDERER);

	// Fill it in
	Str_Copy(texture->name, name, sizeof(texture->name));
	texture->type = TT_CUBE;
	texture->flags = flags;
	texture->format = format;
	texture->filter = filter;
	texture->wrap = TW_CLAMP;
	texture->size = 0;
	texture->target = GL_TEXTURE_CUBE_MAP;
	texture->frameUsed = 0;

	if (uncompressed)
		R_UploadTexture(texture, 6, images, width, height);
	else
		R_UploadCompressedTexture(texture, 6, images, width, height);

	R_SetTextureSize(texture);

	// Add to hash table
	hashKey = Str_HashKey(texture->name, TEXTURES_HASH_SIZE, false);

	texture->nextHash = r_texturesHashTable[hashKey];
	r_texturesHashTable[hashKey] = texture;

	return texture;
}

/*
 ==================
 R_LoadVolumeTexture
 ==================
*/
static texture_t *R_LoadVolumeTexture (const char *name, byte *image, int width, int height, int depth, int flags, textureFormat_t format, textureFilter_t filter, textureWrap_t wrap){

	texture_t	*texture;
	uint		hashKey;

	if (r_numTextures == MAX_TEXTURES)
		Com_Error(ERR_DROP, "R_LoadVolumeTexture: MAX_TEXTURES hit");

	r_textures[r_numTextures++] = texture = (texture_t *)Mem_ClearedAlloc(sizeof(texture_t), TAG_RENDERER);

	// Fill it in
	Str_Copy(texture->name, name, sizeof(texture->name));
	texture->type = TT_3D;
	texture->flags = flags;
	texture->format = format;
	texture->filter = filter;
	texture->wrap = wrap;
	texture->size = 0;
	texture->target = GL_TEXTURE_3D;
	texture->frameUsed = 0;

	R_UploadVolumeTexture(texture, image, width, height, depth);

	R_SetTextureSize(texture);

	// Add to hash table
	hashKey = Str_HashKey(texture->name, TEXTURES_HASH_SIZE, false);

	texture->nextHash = r_texturesHashTable[hashKey];
	r_texturesHashTable[hashKey] = texture;

	return texture;
}

/*
 ==================
 R_LoadArrayTexture
 ==================
*/
static texture_t *R_LoadArrayTexture (const char *name, byte *image, int width, int height, int layers, int flags, textureFormat_t format, textureFilter_t filter, textureWrap_t wrap){

	texture_t	*texture;
	uint		hashKey;

	if (r_numTextures == MAX_TEXTURES)
		Com_Error(ERR_DROP, "R_LoadArrayTexture: MAX_TEXTURES hit");

	r_textures[r_numTextures++] = texture = (texture_t *)Mem_ClearedAlloc(sizeof(texture_t), TAG_RENDERER);

	// Fill it in
	Str_Copy(texture->name, name, sizeof(texture->name));
	texture->type = TT_ARRAY;
	texture->flags = flags;
	texture->format = format;
	texture->filter = filter;
	texture->wrap = wrap;
	texture->size = 0;
	texture->target = GL_TEXTURE_2D_ARRAY;
	texture->frameUsed = 0;

	R_UploadArrayTexture(texture, image, width, height, layers);

	R_SetTextureSize(texture);

	// Add to hash table
	hashKey = Str_HashKey(texture->name, TEXTURES_HASH_SIZE, false);

	texture->nextHash = r_texturesHashTable[hashKey];
	r_texturesHashTable[hashKey] = texture;

	return texture;
}

/*
 ==================
 R_FindTexture
 ==================
*/
texture_t *R_FindTexture (const char *name, int flags, textureFilter_t filter, textureWrap_t wrap){

	texture_t		*texture;
	textureFormat_t	format;
	byte			*image, emptyImage[4*4*4];
	int				width, height;
	bool			uncompressed;
	uint			hashKey;

	// Check if already loaded
	hashKey = Str_HashKey(name, TEXTURES_HASH_SIZE, false);

	for (texture = r_texturesHashTable[hashKey]; texture; texture = texture->nextHash){
		if (texture->type != TT_2D)
			continue;

		if (!Str_ICompare(texture->name, name)){
			if (texture->flags & TF_INTERNAL)
				return texture;

			if (texture->flags != flags)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed flags parameter\n", name);
			if (texture->filter != filter)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed filter parameter\n", name);
			if (texture->wrap != wrap)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed wrap parameter\n", name);

			return texture;
		}
	}

	// If name begins with an underline, create an empty internal texture that
	// can be dynamically modified
	if (name[0] == '_'){
		Mem_Fill(emptyImage, 0xFF, sizeof(emptyImage));

		if (filter == TF_NEAREST)
			texture = R_LoadTexture(name, emptyImage, 4, 4, TF_INTERNAL | TF_ALLOWCAPTURE | TF_ALLOWUPDATE | TF_NOPICMIP | TF_UNCOMPRESSED | flags, TF_RGBA, TF_NEAREST, wrap, true);
		else
			texture = R_LoadTexture(name, emptyImage, 4, 4, TF_INTERNAL | TF_ALLOWCAPTURE | TF_ALLOWUPDATE | TF_NOPICMIP | TF_UNCOMPRESSED | flags, TF_RGBA, TF_LINEAR, wrap, true);

		return texture;
	}

	// Load it from disk
	if (!R_LoadImage(name, flags, wrap, &image, &width, &height, &format, &uncompressed))
		return NULL;

	// Load the texture
	texture = R_LoadTexture(name, image, width, height, flags, format, filter, wrap, uncompressed);

	Mem_Free(image);

	return texture;
}

/*
 ==================
 R_FindCubeTexture
 ==================
*/
texture_t *R_FindCubeTexture (const char *name, int flags, textureFilter_t filter, bool cameraSpace){

	texture_t		*texture;
	textureFormat_t	format;
	byte			*images[6];
	int				width, height;
	bool			uncompressed;
	uint			hashKey;
	int				i;

	// Check if already loaded
	hashKey = Str_HashKey(name, TEXTURES_HASH_SIZE, false);

	for (texture = r_texturesHashTable[hashKey]; texture; texture = texture->nextHash){
		if (texture->type != TT_CUBE)
			continue;

		if (!Str_ICompare(texture->name, name)){
			if (texture->flags & TF_INTERNAL)
				return texture;

			if (texture->flags != flags)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed flags parameter\n", name);
			if (texture->filter != filter)
				Com_Printf(S_COLOR_YELLOW "WARNING: reused texture '%s' with mixed filter parameter\n", name);

			return texture;
		}
	}

	// Load it from disk
	if (!R_LoadCubeImages(name, flags, cameraSpace, images, &width, &height, &format, &uncompressed))
		return NULL;

	// Load the texture
	texture = R_LoadCubeTexture(name, images, width, height, flags, format, filter, uncompressed);

	for (i = 0; i < 6; i++)
		Mem_Free(images[i]);

	return texture;
}


// ============================================================================


/*
 ==================
 R_GetTexture
 ==================
*/
texture_t *R_GetTexture (const char *name){

	texture_t	*texture;
	uint		hashKey;

	hashKey = Str_HashKey(name, TEXTURES_HASH_SIZE, false);

	for (texture = r_texturesHashTable[hashKey]; texture; texture = texture->nextHash){
		if (!Str_ICompare(texture->name, name))
			return texture;
	}

	return NULL;
}

/*
 ==================
 R_GetTextureByIndex
 ==================
*/
texture_t *R_GetTextureByIndex (int index){

	if (index < 0 || index >= r_numTextures)
		return NULL;

	return r_textures[index];
}

/*
 ==================
 R_UploadTextureImage
 ==================
*/
void R_UploadTextureImage (texture_t *texture, int unit, const byte *image, int width, int height){

	if (texture->type != TT_2D)
		return;

	if (r_skipDynamicTextures->integerValue)
		return;

	rg.pc.updateTextures++;
	rg.pc.updateTexturePixels += width * height;

	// Update the texture
	if (width == texture->width && height == texture->height){
		GL_SelectTexture(unit);
		GL_BindTexture(texture);

		qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);
	}
	else {
		// Check the dimensions
		if (width > glConfig.maxTextureSize || height > glConfig.maxTextureSize)
			Com_Error(ERR_DROP, "R_UploadTextureImage: size exceeds hardware limits (%i > %i or %i > %i)", width, glConfig.maxTextureSize, height, glConfig.maxTextureSize);

		// Reallocate the texture
		texture->width = width;
		texture->height = height;
		texture->frameUsed = 0;

		R_SetTextureSize(texture);

		GL_SelectTexture(unit);
		GL_BindTexture(texture);

		qglTexImage2D(GL_TEXTURE_2D, 0, texture->internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	}
}

/*
 ==================
 R_CopyFramebufferToTexture
 ==================
*/
void R_CopyFramebufferToTexture (texture_t *texture, int unit, int x, int y, int width, int height){

	if (texture->type != TT_2D)
		return;

	if (r_skipCopyToTextures->integerValue)
		return;

	rg.pc.captureTextures++;
	rg.pc.captureTexturePixels += width * height;

	// Update the texture
	if (width == texture->width && height == texture->height){
		GL_SelectTexture(unit);
		GL_BindTexture(texture);

		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, x, y, width, height);
	}
	else {
		// Check the dimensions
		if (width > glConfig.maxTextureSize || height > glConfig.maxTextureSize)
			Com_Error(ERR_DROP, "R_CopyFramebufferToTexture: size exceeds hardware limits (%i > %i or %i > %i)", width, glConfig.maxTextureSize, height, glConfig.maxTextureSize);

		// Reallocate the texture
		texture->width = width;
		texture->height = height;
		texture->frameUsed = 0;

		R_SetTextureSize(texture);

		GL_SelectTexture(unit);
		GL_BindTexture(texture);

		qglCopyTexImage2D(GL_TEXTURE_2D, 0, texture->internalFormat, x, y, width, height, 0);
	}
}

/*
 ==================
 R_SetTextureSize
 ==================
*/
void R_SetTextureSize (texture_t *texture){

	int		width, height, depth;
	int		size;

	switch (texture->internalFormat){
	case GL_LUMINANCE8:
		size = texture->width * texture->height * texture->depth;
		break;
	case GL_LUMINANCE8_ALPHA8:
		size = texture->width * texture->height * texture->depth * 2;
		break;
	case GL_RGB8:
		size = texture->width * texture->height * texture->depth * 4;
		break;
	case GL_RGBA8:
		size = texture->width * texture->height * texture->depth * 4;
		break;
	case GL_DEPTH_COMPONENT16:
		size = texture->width * texture->height * texture->depth * 2;
		break;
	case GL_DEPTH_COMPONENT24:
		size = texture->width * texture->height * texture->depth * 4;
		break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		size = ((texture->width + 3) >> 2) * ((texture->height + 3) >> 2) * 8;
		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		size = ((texture->width + 3) >> 2) * ((texture->height + 3) >> 2) * 8;
		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		size = ((texture->width + 3) >> 2) * ((texture->height + 3) >> 2) * 16;
		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		size = ((texture->width + 3) >> 2) * ((texture->height + 3) >> 2) * 16;
		break;
	default:
		Com_Error(ERR_DROP, "R_SetTextureSize: bad texture internal format (%u)", texture->internalFormat);
	}

	if (texture->filter == TF_DEFAULT){
		width = texture->width;
		height = texture->height;
		depth = texture->depth;

		while (width > 1 || height > 1 || depth > 1){
			width >>= 1;
			if (width < 1)
				width = 1;

			height >>= 1;
			if (height < 1)
				height = 1;

			depth >>= 1;
			if (depth < 1)
				depth = 1;

			switch (texture->internalFormat){
			case GL_LUMINANCE8:
				size += width * height * depth;
				break;
			case GL_LUMINANCE8_ALPHA8:
				size += width * height * depth * 2;
				break;
			case GL_RGB8:
				size += width * height * depth * 4;
				break;
			case GL_RGBA8:
				size += width * height * depth * 4;
				break;
			case GL_DEPTH_COMPONENT16:
				size += width * height * depth * 2;
				break;
			case GL_DEPTH_COMPONENT24:
				size += width * height * depth * 4;
				break;
			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
				size += ((width + 3) >> 2) * ((height + 3) >> 2) * 8;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				size += ((width + 3) >> 2) * ((height + 3) >> 2) * 8;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
				size += ((width + 3) >> 2) * ((height + 3) >> 2) * 16;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				size += ((width + 3) >> 2) * ((height + 3) >> 2) * 16;
				break;
			default:
				Com_Error(ERR_DROP, "R_SetTextureSize: bad texture internal format (%u)", texture->internalFormat);
			}
		}
	}

	if (texture->type != TT_CUBE)
		texture->size = size;
	else
		texture->size = size * 6;
}

/*
 ==================
 R_ChangeTextureFilter
 ==================
*/
void R_ChangeTextureFilter (){

	filterTable_t	*filterTable;
	texture_t		*texture;
	int				i;

	// Validate texture filter
	for (filterTable = r_filterTable; filterTable->name; filterTable++){
		if (!Str_ICompare(filterTable->name, r_textureFilter->value))
			break;
	}

	if (filterTable->name){
		r_filterMin = filterTable->min;
		r_filterMag = filterTable->mag;
	}
	else {
		Com_Printf(S_COLOR_RED "Bad r_textureFilter '%s'. Defaulting to GL_LINEAR_MIPMAP_LINEAR.\n", r_textureFilter->value);

		CVar_SetString(r_textureFilter, "GL_LINEAR_MIPMAP_LINEAR");

		r_filterMin = GL_LINEAR_MIPMAP_LINEAR;
		r_filterMag = GL_LINEAR;
	}

	// Validate texture LOD bias
	if (r_textureLODBias->floatValue < -glConfig.maxTextureLODBias)
		CVar_SetFloat(r_textureLODBias, -glConfig.maxTextureLODBias);
	else if (r_textureLODBias->floatValue > glConfig.maxTextureLODBias)
		CVar_SetFloat(r_textureLODBias, glConfig.maxTextureLODBias);

	// Validate texture anisotropy if available
	if (glConfig.textureFilterAnisotropicAvailable){
		if (r_textureAnisotropy->floatValue < 1.0f)
			CVar_SetFloat(r_textureAnisotropy, 1.0f);
		else if (r_textureAnisotropy->floatValue > glConfig.maxTextureMaxAnisotropy)
			CVar_SetFloat(r_textureAnisotropy, glConfig.maxTextureMaxAnisotropy);
	}

	// Change all the existing mipmapped texture objects
	for (i = 0; i < r_numTextures; i++){
		texture = r_textures[i];

		if (texture->filter != TF_DEFAULT)
			continue;

		GL_BindTexture(texture);

		// Set texture filter
		qglTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, r_filterMin);
		qglTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, r_filterMag);

		// Set texture LOD bias
		qglTexParameterf(texture->target, GL_TEXTURE_LOD_BIAS, r_textureLODBias->floatValue);

		// Set texture anisotropy if available
		if (glConfig.textureFilterAnisotropicAvailable)
			qglTexParameterf(texture->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_textureAnisotropy->floatValue);
	}
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_TestTexture_f
 ==================
*/
static void R_TestTexture_f (){

	texture_t	*texture;
	char		name[MAX_PATH_LENGTH];

	if (Cmd_Argc() > 3){
		Com_Printf("Usage: testTexture [name]\n");
		return;
	}

	// Clear the old texture, if any
	rg.testTexture = NULL;

	if (Cmd_Argc() < 2)
		return;

	Str_Copy(name, Cmd_Argv(1), sizeof(name));
	Str_StripFileExtension(name);

	// Test the specified texture
	texture = R_GetTexture(name);

	if (texture){
		if (texture->type != TT_2D)
			return;

		rg.testTexture = texture;
		return;
	}

	// If not already loaded, try to load it
	rg.testTexture = R_FindTexture(name, TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, TW_REPEAT);
}

/*
 ==================
 R_TestCubeTexture_f
 ==================
*/
static void R_TestCubeTexture_f (){

	texture_t	*texture;
	char		name[MAX_PATH_LENGTH];

	if (Cmd_Argc() > 3){
		Com_Printf("Usage: testCubeTexture [name]\n");
		return;
	}

	// Clear the old texture, if any
	rg.testTexture = NULL;

	if (Cmd_Argc() < 2)
		return;

	Str_Copy(name, Cmd_Argv(1), sizeof(name));
	Str_StripFileExtension(name);

	// Test the specified texture
	texture = R_GetTexture(name);

	if (texture){
		if (texture->type != TT_CUBE)
			return;

		rg.testTexture = texture;
		return;
	}

	// If not already loaded, try to load it
	rg.testTexture = R_FindCubeTexture(name, TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, false);

	if (rg.testTexture)
		return;

	rg.testTexture = R_FindCubeTexture(name, TF_NOPICMIP | TF_UNCOMPRESSED, TF_LINEAR, true);
}

/*
 ==================
 R_ListTextures_f
 ==================
*/
static void R_ListTextures_f (){

	texture_t	*texture;
	int			bytes = 0;
	int			i;

	Com_Printf("\n");
	Com_Printf("      -w-- -h-- -d-- -size- -fmt--- type filt wrap -name-----------\n");

	for (i = 0; i < r_numTextures; i++){
		texture = r_textures[i];

		bytes += texture->size;

		Com_Printf("%4i: ", i);

		Com_Printf("%4i %4i %4i ", texture->width, texture->height, texture->depth);

		Com_Printf("%5ik ", texture->size >> 10);

		switch (texture->internalFormat){
		case GL_LUMINANCE8:
			Com_Printf("L8      ");
			break;
		case GL_LUMINANCE8_ALPHA8:
			Com_Printf("L8A8    ");
			break;
		case GL_RGB8:
			Com_Printf("RGB8    ");
			break;
		case GL_RGBA8:
			Com_Printf("RGBA8   ");
			break;
		case GL_DEPTH_COMPONENT16:
			Com_Printf("D16     ");
			break;
		case GL_DEPTH_COMPONENT24:
			Com_Printf("D24     ");
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			Com_Printf("DXT1C   ");
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			Com_Printf("DXT1A   ");
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			Com_Printf("DXT3    ");
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			Com_Printf("DXT5    ");
			break;
		default:
			Com_Printf("??????? ");
			break;
		}

		switch (texture->type){
		case TT_2D:
			Com_Printf(" 2D  ");
			break;
		case TT_3D:
			Com_Printf(" 3D  ");
			break;
		case TT_CUBE:
			Com_Printf("CUBE ");
			break;
		case TT_ARRAY:
			Com_Printf("LYRD ");
			break;
		default:
			Com_Printf("???? ");
			break;
		}

		switch (texture->filter){
		case TF_DEFAULT:
			Com_Printf("dflt ");
			break;
		case TF_NEAREST:
			Com_Printf("nrst ");
			break;
		case TF_LINEAR:
			Com_Printf("linr ");
			break;
		default:
			Com_Printf("???? ");
			break;
		}

		switch (texture->wrap){
		case TW_REPEAT:
			Com_Printf("rept ");
			break;
		case TW_REPEAT_MIRRORED:
			Com_Printf("mirr ");
			break;
		case TW_CLAMP:
			Com_Printf("clmp ");
			break;
		case TW_CLAMP_TO_ZERO:
			Com_Printf("zero ");
			break;
		case TW_CLAMP_TO_ZERO_ALPHA:
			Com_Printf("azro ");
			break;
		default:
			Com_Printf("???? ");
			break;
		}

		Com_Printf("%s\n", texture->name);
	}

	Com_Printf("-------------------------------------------------------------------\n");
	Com_Printf("%i total textures\n", r_numTextures);
	Com_Printf("%.2f MB of texture data\n", bytes * (1.0f / 1048576.0f));
	Com_Printf("\n");
}


/*
 ==============================================================================

 INTERNAL TEXTURES

 ==============================================================================
*/


/*
 ==================
 R_CreateInternalTextures

 TODO: lightmapTextures
 ==================
*/
static void R_CreateInternalTextures (){

	byte		image[256*256*4];
	byte		emptyImage[4*4*4], *emptyCubeImages[6];
	int			x, y;
	int			i;

	Mem_Fill(emptyImage, 0xFF, sizeof(emptyImage));

	for (i = 0; i < 6; i++)
		emptyCubeImages[i] = emptyImage;

	// Default texture
	for (y = 0; y < 16; y++){
		for (x = 0; x < 16; x++){
			if (x == 0 || x == 15 || y == 0 || y == 15)
				((dword *)image)[y * 16 + x] = LittleLong(0xFFFFFFFF);
			else
				((dword *)image)[y * 16 + x] = LittleLong(0xFF000000);
		}
	}

	rg.defaultTexture = R_LoadTexture("_default", image, 16, 16, TF_INTERNAL | TF_NOPICMIP, TF_LUMINANCE, TF_DEFAULT, TW_REPEAT, true);

	// White texture
	for (i = 0; i < 16; i++)
		((dword *)image)[i] = LittleLong(0xFFFFFFFF);

	rg.whiteTexture = R_LoadTexture("_white", image, 4, 4, TF_INTERNAL | TF_NOPICMIP, TF_LUMINANCE, TF_DEFAULT, TW_REPEAT, true);

	// Black texture
	for (i = 0; i < 16; i++)
		((dword *)image)[i] = LittleLong(0xFF000000);

	rg.blackTexture = R_LoadTexture("_black", image, 4, 4, TF_INTERNAL | TF_NOPICMIP, TF_LUMINANCE, TF_DEFAULT, TW_REPEAT, true);

	// Flat texture
	for (i = 0; i < 16; i++)
		((dword *)image)[i] = LittleLong(0xFFFF8080);

	rg.flatTexture = R_LoadTexture("_flat", image, 4, 4, TF_INTERNAL | TF_NOPICMIP | TF_BUMP, TF_RGB, TF_DEFAULT, TW_REPEAT, true);

	// TODO: rg.lightmapTextures

	// Cinematic textures
	for (i = 0; i < MAX_CINEMATICS; i++)
		rg.cinematicTextures[i] = R_LoadTexture(Str_VarArgs("_cinematic%i", i+1), emptyImage, 4, 4, TF_INTERNAL | TF_NOPICMIP | TF_UNCOMPRESSED, TF_RGB, TF_LINEAR, TW_CLAMP_TO_ZERO, true);

	// Sky texture
	rg.skyTexture = R_LoadTexture("_sky", emptyImage, 4, 4, TF_INTERNAL | TF_ALLOWCAPTURE | TF_NOPICMIP | TF_UNCOMPRESSED, TF_RGB, TF_LINEAR, TW_CLAMP, true);

	// Mirror texture
	rg.mirrorTexture = R_LoadTexture("_mirror", emptyImage, 4, 4, TF_INTERNAL | TF_ALLOWCAPTURE | TF_NOPICMIP | TF_UNCOMPRESSED, TF_RGB, TF_LINEAR, TW_CLAMP, true);

	// Remote texture
	rg.remoteTexture = R_LoadTexture("_remote", emptyImage, 4, 4, TF_INTERNAL | TF_ALLOWCAPTURE | TF_NOPICMIP | TF_UNCOMPRESSED, TF_RGB, TF_LINEAR, TW_REPEAT, true);

	// Current color texture
	rg.currentColorTexture = R_LoadTexture("_currentColor", emptyImage, 4, 4, TF_INTERNAL | TF_ALLOWCAPTURE | TF_NOPICMIP | TF_UNCOMPRESSED, TF_RGBA, TF_LINEAR, TW_CLAMP, true);

	// Current depth texture
	rg.currentDepthTexture = R_LoadTexture("_currentDepth", emptyImage, 4, 4, TF_INTERNAL | TF_ALLOWCAPTURE | TF_NOPICMIP | TF_UNCOMPRESSED, TF_DEPTH_24, TF_NEAREST, TW_CLAMP, true);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_InitTextures
 ==================
*/
void R_InitTextures (){

	// Add commands
	Cmd_AddCommand("testTexture", R_TestTexture_f, "Tests a texture", Cmd_ArgCompletion_TextureName);
	Cmd_AddCommand("testCubeTexture", R_TestCubeTexture_f, "Tests a cube texture", Cmd_ArgCompletion_TextureName);
	Cmd_AddCommand("listTextures", R_ListTextures_f, "Lists loaded textures", NULL);

	// Change texture filtering
	R_ChangeTextureFilter();

	// Create internal textures
	R_CreateInternalTextures();
}

/*
 ==================
 R_ShutdownTextures
 ==================
*/
void R_ShutdownTextures (){

	texture_t	*texture;
	int			i;

	// Remove commands
	Cmd_RemoveCommand("testTexture");
	Cmd_RemoveCommand("testCubeTexture");
	Cmd_RemoveCommand("listTextures");

	// Delete all the textures
	for (i = MAX_TEXTURE_UNITS - 1; i >= 0; i--){
		if (i >= glConfig.maxTextureImageUnits)
			continue;

		qglActiveTexture(GL_TEXTURE0 + i);

		qglBindTexture(GL_TEXTURE_2D, 0);
		qglBindTexture(GL_TEXTURE_3D, 0);
		qglBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	for (i = 0; i < r_numTextures; i++){
		texture = r_textures[i];

		qglDeleteTextures(1, &texture->textureId);
	}

	// Clear texture list
	Mem_Fill(r_texturesHashTable, 0, sizeof(r_texturesHashTable));
	Mem_Fill(r_textures, 0, sizeof(r_textures));

	r_numTextures = 0;
}