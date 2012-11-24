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
// fileFormats.h - File formats used in the engine
//

// This file must be identical in the Quake and utils directories
// ZIP and JPEG files are not included here


#ifndef __FILEFORMATS_H__
#define __FILEFORMATS_H__


/*
 ==============================================================================

 PAK files are used for compressed zip files

 ==============================================================================
*/

#define PAK_ID						(('K' << 24) + ('C' << 16) + ('A' << 8) + 'P')

typedef struct {
	char					name[56];
	int						filePos;
	int						fileLen;
} pakFile_t;

typedef struct {
	int						id;
	int						dirOfs;
	int						dirLen;
} pakHeader_t;

/*
 ==============================================================================

 PCX files are used for uncompressed images

 ==============================================================================
*/

typedef struct {
    char					manufacturer;
    char					version;
    char					encoding;
    char					bitsPerPixel;
    ushort					xMin;
	ushort					yMin;
	ushort					xMax;
	ushort					yMax;
    ushort					hRes;
	ushort					vRes;
    uchar					palette[48];
    char					reserved;
    char					colorPlanes;
    ushort					bytesPerLine;
    ushort					paletteType;
    char					filler[58];
    uchar					data;			// Unbound
} pcxHeader_t;

/*
 ==============================================================================

 TGA files are used for uncompressed images

 ==============================================================================
*/

#define TGA_IMAGE_COLORMAP			1
#define TGA_IMAGE_TRUECOLOR			2
#define TGA_IMAGE_MONOCHROME		3

typedef struct {
	byte				bIdLength;
	byte				bColormapType;
	byte				bImageType;
	word				wColormapIndex;
	word				wColormapLength;
	byte				bColormapSize;
	word				wXOrigin;
	word				wYOrigin;
	word				wWidth;
	word				wHeight;
	byte				bPixelSize;
	byte				bAttributes;
} tgaHeader_t;

/*
 ==============================================================================

 DDS files are used for compressed images

 ==============================================================================
*/

#define DDS_MAGIC					(('D' << 0) + ('D' << 8) + ('S' << 16) + (' ' << 24))

#define DDS_FOURCC_DXT1				(('D' << 0) + ('X' << 8) + ('T' << 16) + ('1' << 24))
#define DDS_FOURCC_DXT2				(('D' << 0) + ('X' << 8) + ('T' << 16) + ('2' << 24))
#define DDS_FOURCC_DXT3				(('D' << 0) + ('X' << 8) + ('T' << 16) + ('3' << 24))
#define DDS_FOURCC_DXT4				(('D' << 0) + ('X' << 8) + ('T' << 16) + ('4' << 24))
#define DDS_FOURCC_DXT5				(('D' << 0) + ('X' << 8) + ('T' << 16) + ('5' << 24))
#define DDS_FOURCC_RXGB				(('R' << 0) + ('X' << 8) + ('G' << 16) + ('B' << 24))
#define DDS_FOURCC_ATI1				(('A' << 0) + ('T' << 8) + ('I' << 16) + ('1' << 24))
#define DDS_FOURCC_ATI2				(('A' << 0) + ('T' << 8) + ('I' << 16) + ('2' << 24))

#define DDS_CAPS					0x00000001
#define DDS_HEIGHT					0x00000002
#define DDS_WIDTH					0x00000004
#define DDS_PITCH					0x00000008
#define DDS_PIXELFORMAT				0x00001000
#define DDS_MIPMAPCOUNT				0x00020000
#define DDS_LINEARSIZE				0x00080000
#define DDS_DEPTH					0x00800000

#define DDS_ALPHAPIXELS				0x00000001
#define DDS_ALPHA					0x00000002
#define DDS_FOURCC					0x00000004
#define DDS_RGB						0x00000040
#define DDS_ALPHAPREMULT			0x00008000
#define DDS_LUMINANCE				0x00020000

#define DDS_COMPLEX					0x00000008
#define DDS_TEXTURE					0x00001000
#define DDS_MIPMAP					0x00400000

#define DDS_CUBEMAP					0x00000200
#define DDS_CUBEMAP_POSITIVEX		0x00000400
#define DDS_CUBEMAP_NEGATIVEX		0x00000800
#define DDS_CUBEMAP_POSITIVEY		0x00001000
#define DDS_CUBEMAP_NEGATIVEY		0x00002000
#define DDS_CUBEMAP_POSITIVEZ		0x00004000
#define DDS_CUBEMAP_NEGATIVEZ		0x00008000
#define DDS_VOLUME					0x00200000

typedef struct {
	dword					dwSize;
	dword					dwFlags;
	dword					dwFourCC;
	dword					dwRGBBitCount;
	dword					dwRBitMask;
	dword					dwGBitMask;
	dword					dwBBitMask;
	dword					dwABitMask;
} ddsPixelFormat_t;

typedef struct {
	dword					dwMagic;
	dword					dwSize;
	dword					dwFlags;
	dword					dwHeight;
	dword					dwWidth;
	dword					dwPitchOrLinearSize;
	dword					dwDepth;
	dword					dwMipMapCount;
	dword					dwReserved1[11];
	ddsPixelFormat_t		ddsPixelFormat;
	dword					dwCaps1;
	dword					dwCaps2;
	dword					dwReserved2[3];
} ddsHeader_t;

/*
 ==============================================================================

 WAL files are used for uncompressed images

 ==============================================================================
*/

#define	MIPLEVELS					4

typedef struct {
	char					name[32];
	uint					width;
	uint					height;
	uint					offsets[MIPLEVELS];	// Four mipmaps stored
	char					animName[32];		// Next frame in animation chain
	int						flags;
	int						contents;
	int						value;
} mipTex_t;

/*
 ==============================================================================

 WAV files are used for sounds

 ==============================================================================
*/

#define WAV_FORMAT_PCM				1

typedef struct {
	word					wFormat;
	word					wChannels;
	dword					dwSamplesPerSec;
	dword					dwAvgBytesPerSec;
	word					wBlockAlign;
	word					wBitsPerSample;
} wavFormat_t;

/*
 ==============================================================================

 ROQ files are used for cinematics and in-game videos

 ==============================================================================
*/

#define ROQ_ID						0x1084

#define ROQ_CHUNK_HEADER_SIZE		8
#define ROQ_CHUNK_MAX_SIZE			65536

#define ROQ_QUAD_INFO				0x1001
#define ROQ_QUAD_CODEBOOK			0x1002
#define ROQ_QUAD_VQ					0x1011

#define ROQ_SOUND_MONO				0x1020
#define ROQ_SOUND_STEREO			0x1021

#define ROQ_VQ_MOT					0x0000
#define ROQ_VQ_FCC					0x0001
#define ROQ_VQ_SLD					0x0002
#define ROQ_VQ_CCC					0x0003

typedef struct {
	word					id;
	dword					size;
	word					flags;
} roqChunk_t;

typedef struct {
	byte					y[4];
	byte					u;
	byte					v;
} roqQuadVector_t;

typedef struct {
	byte					index[4];
} roqQuadCell_t;

/*
 ==============================================================================

 SP2 files are used for sprite models

 ==============================================================================
*/

#define SP2_ID						(('2' << 24) + ('S' << 16) + ('D' << 8) + 'I')
#define SP2_VERSION					2

#define SP2_MAX_FRAMES				32

typedef struct {
	int						width;
	int						height;
	int						originX;			// Raster coordinates inside pic
	int						originY;			// Raster coordinates inside pic
	char					name[MAX_QPATH];	// Name of PCX file
} sp2Frame_t;

typedef struct {
	int						id;
	int						version;
	int						numFrames;
	sp2Frame_t				frames[1];			// Variable sized
} sp2Header_t;

/*
 ==============================================================================

 MD2 files are used for animated (dynamic) render and clip models

 ==============================================================================
*/

#define MD2_ID						(('2' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define MD2_VERSION					8

#define	MD2_MAX_TRIANGLES			4096
#define MD2_MAX_VERTS				2048
#define MD2_MAX_SKINS				32
#define MD2_MAX_FRAMES				512

typedef struct {
	short					s;
	short					t;
} md2St_t;

typedef struct {
	short					indexXyz[3];
	short					indexSt[3];
} md2Triangle_t;

typedef struct {
	byte					v[3];				// Scaled byte to fit in frame mins/maxs
	byte					lightNormalIndex;
} md2Vertex_t;

typedef struct {
	vec3_t					scale;				// Multiply byte verts by this
	vec3_t					translate;			// Then add this
	char					name[16];			// Frame name from grabbing
	md2Vertex_t				verts[1];			// Variable sized
} md2Frame_t;

typedef struct {
	int						id;
	int						version;

	int						skinWidth;
	int						skinHeight;
	int						frameSize;			// Byte size of each frame

	int						numSkins;
	int						numXyz;
	int						numSt;				// Greater than numXyz for seams
	int						numTris;
	int						numGLCmds;			// dwords in strip/fan command list
	int						numFrames;

	int						ofsSkins;			// Each skin is a MAX_QPATH string
	int						ofsSt;				// Byte offset from start for stverts
	int						ofsTris;			// Offset for triangles
	int						ofsFrames;			// Offset for first frame
	int						ofsGLCmds;	
	int						ofsEnd;				// End of file
} md2Header_t;

/*
 ==============================================================================

 MD3 files are used for animated (dynamic) render and clip models

 ==============================================================================
*/

#define MD3_ID						(('3' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define MD3_VERSION					15

#define MD3_MAX_TRIANGLES			8192		// Per surface
#define MD3_MAX_VERTS				4096		// Per surface
#define MD3_MAX_SHADERS				256			// Per surface
#define MD3_MAX_TAGS				16			// Per frame
#define MD3_MAX_FRAMES				1024		// Per model
#define MD3_MAX_SURFACES			32			// Per model

#define MD3_XYZ_SCALE				(1.0f/64)	// Vertex scales

typedef struct {
	vec3_t					bounds[2];
	vec3_t					localOrigin;
	float					radius;
	char					name[16];
} md3Frame_t;

typedef struct {
	char					name[MAX_QPATH];
	vec3_t					origin;
	vec3_t					axis[3];
} md3Tag_t;

typedef struct {
	int						id;

	char					name[MAX_QPATH];
	int						flags;

	int						numFrames;			// All surfaces in a model should have the same
	int						numShaders;			// All surfaces in a model should have the same
	int						numVerts;
	int						numTriangles;

	int						ofsTriangles;
	int						ofsShaders;			// Offset from start of md3Surface_t
	int						ofsSt;				// Texture coords are common for all frames
	int						ofsXyzNormals;		// numVerts * numFrames
	int						ofsEnd;				// Next surface follows
} md3Surface_t;

typedef struct {
	char					name[MAX_QPATH];
	int						materialIndex;		// For in-game use
} md3Material_t;

typedef struct {
	int						indexes[3];
} md3Triangle_t;

typedef struct {
	vec2_t					st;
} md3St_t;

typedef struct {
	short					xyz[3];
	short					normal;
} md3XyzNormal_t;

typedef struct {
	int						id;
	int						version;

	char					name[MAX_QPATH];	// Model name
	int						flags;

	int						numFrames;
	int						numTags;			
	int						numSurfaces;
	int						numSkins;

	int						ofsFrames;			// Offset for first frame
	int						ofsTags;			// numTags * numFrames
	int						ofsSurfaces;		// First surface, others follow
	int						ofsEnd;				// End of file
} md3Header_t;

/*
 ==============================================================================

 BSP files are used for maps

 ==============================================================================
*/

#define BSP_ID						(('P' << 24) + ('S' << 16) + ('B' << 8) + 'I')
#define BSP_VERSION					38

#define	MAX_STYLES					4

#define	LIGHTMAP_WIDTH				128
#define	LIGHTMAP_HEIGHT				128

#define	VIS_PVS						0
#define	VIS_PHS						1

// Upper design bounds
#define	MAX_MAP_ENTITIES			0x000800
#define	MAX_MAP_ENTSTRING			0x040000
#define	MAX_MAP_PLANES				0x010000
#define	MAX_MAP_VERTEXES			0x010000
#define	MAX_MAP_VISIBILITY			0x100000
#define	MAX_MAP_NODES				0x010000
#define	MAX_MAP_TEXINFO				0x002000
#define	MAX_MAP_FACES				0x010000
#define	MAX_MAP_LIGHTING			0x200000
#define	MAX_MAP_LEAFS				0x010000
#define	MAX_MAP_LEAFFACES			0x010000
#define	MAX_MAP_LEAFBRUSHES			0x010000
#define	MAX_MAP_EDGES				0x01F400
#define	MAX_MAP_SURFEDGES			0x03E800
#define	MAX_MAP_MODELS				0x000400
#define	MAX_MAP_BRUSHES				0x002000
#define	MAX_MAP_BRUSHSIDES			0x010000
#define	MAX_MAP_AREAS				0x000100
#define	MAX_MAP_AREAPORTALS			0x000400
#define	MAX_MAP_PORTALS				0x010000

typedef enum {
	LUMP_ENTITIES,
	LUMP_PLANES,
	LUMP_VERTICES,
	LUMP_VISIBILITY,
	LUMP_NODES,
	LUMP_TEXINFO,
	LUMP_FACES,
	LUMP_LIGHTING,
	LUMP_LEAFS,
	LUMP_LEAFFACES,
	LUMP_LEAFBRUSHES,
	LUMP_EDGES,
	LUMP_SURFEDGES,
	LUMP_INLINEMODELS,
	LUMP_BRUSHES,
	LUMP_BRUSHSIDES,
	LUMP_POP,
	LUMP_AREAS,
	LUMP_AREAPORTALS,
	NUM_HEADER_LUMPS
} headerLump_t;

typedef struct {
	int						offset;
	int						length;
} bspLump_t;

typedef struct {
	int						id;
	int						version;

	bspLump_t				lumps[NUM_HEADER_LUMPS];
} bspHeader_t;

typedef struct {
	vec3_t					mins;
	vec3_t					maxs;
	vec3_t					origin;					// For sounds or lights
	int						headNode;
	int						firstFace;				// Inline models just draw faces
	int						numFaces;				// without walking the BSP tree
} bspInlineModel_t;

typedef struct {
	vec3_t					point;
} bspVertex_t;

typedef struct {
	vec3_t					normal;
	float					dist;
	int						type;
} bspPlane_t;

typedef struct {
	int						planeNum;
	int						children[2];			// Negative numbers are -(leafs+1), not nodes
	
	short					mins[3];				// For frustum culling
	short					maxs[3];

	ushort					firstFace;
	ushort					numFaces;				// Counting both sides
} bspNode_t;

typedef struct {
	float					vecs[2][4];				// [st][xyz offset]
	int						flags;					// Miptex flags + overrides
	int						value;					// Light emission, etc...
	char					texture[32];			// Texture name (textures/*.wal)
	int						nextTexInfo;			// For animations, -1 = end of chain
} bspTexInfo_t;

// Note that edge 0 is never used, because negative edge nums are used 
// for counterclockwise use of the edge in a face
typedef struct {
	ushort					v[2];					// Vertex numbers
} bspEdge_t;

typedef struct {
	ushort					planeNum;
	short					side;

	int						firstEdge;				// We must support > 64k edges
	short					numEdges;	
	short					texInfo;

	// Lighting info
	byte					styles[MAX_STYLES];
	int						lightOfs;				// Start of samples
} bspFace_t;

typedef struct {
	int						contents;				// OR of all brushes (not needed?)

	short					cluster;
	short					area;

	short					mins[3];				// For frustum culling
	short					maxs[3];

	ushort					firstLeafFace;
	ushort					numLeafFaces;

	ushort					firstLeafBrush;
	ushort					numLeafBrushes;
} bspLeaf_t;

typedef struct {
	ushort					planeNum;				// Facing out of the leaf
	short					texInfo;
} bspBrushSide_t;

typedef struct {
	int						firstSide;
	int						numSides;
	int						contents;
} bspBrush_t;

// The visibility lump consists of a header with a count, then byte
// offsets for the PVS and PHS of each cluster, then the raw compressed
// bit vectors
typedef struct {
	int						numClusters;
	int						bitOfs[8][2];
} dvis_t;

// Each area has a list of portals that lead into other areas when 
// portals are closed, other areas may not be visible or hearable even 
// if the vis info says that it should be
typedef struct {
	int						portalNum;
	int						otherArea;
} bspAreaPortal_t;

typedef struct {
	int						numAreaPortals;
	int						firstAreaPortal;
} bspArea_t;


#endif	// __FILEFORMATS_H__