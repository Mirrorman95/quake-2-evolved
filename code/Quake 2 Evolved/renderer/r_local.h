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
// r_local.h - Local header file to all renderer files
//


#ifndef __R_LOCAL_H__
#define __R_LOCAL_H__


#include "../client/client.h"
#include "../windows/qgl.h"


// This is defined in shared.h
#undef MAX_MODELS

#define GL_INDEX_TYPE				GL_UNSIGNED_INT

#define GL_VERTEX_XYZ(ptr)			((const byte *)(ptr) + 0)
#define GL_VERTEX_NORMAL(ptr)		((const byte *)(ptr) + 12)
#define GL_VERTEX_TANGENT1(ptr)		((const byte *)(ptr) + 24)
#define GL_VERTEX_TANGENT2(ptr)		((const byte *)(ptr) + 36)
#define GL_VERTEX_TEXCOORD(ptr)		((const byte *)(ptr) + 48)
#define GL_VERTEX_COLOR(ptr)		((const byte *)(ptr) + 56)

typedef unsigned int		glIndex_t;

typedef struct {
	vec3_t					xyz;
	vec3_t					normal;
	vec3_t					tangents[2];
	vec4_t					st;
	color_t					color;
} glVertex_t;

/*
 ==============================================================================

 TEXTURE MANAGER

 ==============================================================================
*/

#define MAX_TEXTURES				8192

typedef enum {
	TT_2D,
	TT_3D,
	TT_CUBE,
	TT_ARRAY
} textureType_t;

typedef enum {
	TF_INTERNAL						= BIT(0),
	TF_ALLOWCAPTURE					= BIT(1),
	TF_ALLOWUPDATE					= BIT(2),
	TF_NOPICMIP						= BIT(3),
	TF_UNCOMPRESSED					= BIT(4),
	TF_BUMP							= BIT(5),
	TF_DIFFUSE						= BIT(6),
	TF_SPECULAR						= BIT(7),
} textureFlags_t;

typedef enum {
	TF_LUMINANCE,
	TF_LUMINANCE_ALPHA,
	TF_RGB,
	TF_RGBA,
	TF_DEPTH_16,
	TF_DEPTH_24,
	TF_COMPRESSED_DXT1C,
	TF_COMPRESSED_DXT1A,
	TF_COMPRESSED_DXT3,
	TF_COMPRESSED_DXT5,
	TF_COMPRESSED_RXGB
} textureFormat_t;

typedef enum {
	TF_DEFAULT,
	TF_NEAREST,
	TF_LINEAR
} textureFilter_t;

typedef enum {
	TW_REPEAT,
	TW_REPEAT_MIRRORED,
	TW_CLAMP,
	TW_CLAMP_TO_ZERO,
	TW_CLAMP_TO_ZERO_ALPHA
} textureWrap_t;

typedef struct texture_s {
	char					name[MAX_PATH_LENGTH];

	textureType_t			type;
	int						flags;
	textureFormat_t			format;
	textureFilter_t			filter;
	textureWrap_t			wrap;

	int						width;
	int						height;
	int						depth;
	int						size;
	uint					target;
	uint					internalFormat;

	int						frameUsed;

	uint					textureId;

	struct texture_s *		nextHash;
} texture_t;

texture_t *		R_FindTexture (const char *name, int flags, textureFilter_t filter, textureWrap_t wrap);
texture_t *		R_FindCubeTexture (const char *name, int flags, textureFilter_t filter, bool cameraSpace);

texture_t *		R_GetTexture (const char *name);
texture_t *		R_GetTextureByIndex (int index);

void			R_UploadTextureImage (texture_t *texture, int unit, const byte *image, int width, int height);
void			R_CopyFramebufferToTexture (texture_t *texture, int unit, int x, int y, int width, int height);

void			R_SetTextureSize (texture_t *texture);

void			R_ChangeTextureFilter ();
void			R_ChangeShadowTextureFilter ();

void			R_InitTextures ();
void			R_ShutdownTextures ();

/*
 ==============================================================================

 SHADER MANAGER

 ==============================================================================
*/

#define MAX_SHADERS					512

typedef struct shader_s {
	char					name[MAX_PATH_LENGTH];

	uint					type;
	int						references;

	bool					compileStatus;

	uint					shaderId;

	struct shader_s *		nextHash;
} shader_t;

shader_t *		R_FindShader (const char *name, uint type);

void			R_InitShaders ();
void			R_ShutdownShaders ();

/*
 ==============================================================================

 PROGRAM MANAGER

 ==============================================================================
*/

#define MAX_PROGRAMS				256
#define MAX_PROGRAM_UNIFORMS		64

#define MAX_UNIFORM_NAME_LENGTH		64

typedef enum {
	VA_NORMAL						= BIT(0),
	VA_TANGENT1						= BIT(1),
	VA_TANGENT2						= BIT(2),
	VA_TEXCOORD						= BIT(3),
	VA_COLOR						= BIT(4)
} vertexAttrib_t;

typedef enum {
	UT_VIEW_ORIGIN,
	UT_VIEW_AXIS,
	UT_ENTITY_ORIGIN,
	UT_ENTITY_AXIS,
	UT_SUN_ORIGIN,
	UT_SUN_DIRECTION,
	UT_SUN_COLOR,
	UT_SCREEN_MATRIX,
	UT_COORD_SCALE_AND_BIAS,
	UT_COLOR_SCALE_AND_BIAS,
	UT_CUSTOM
} uniformType_t;

typedef struct {
	char					name[MAX_UNIFORM_NAME_LENGTH];

	uniformType_t			type;
	int						size;
	uint					format;
	int						location;

	int						unit;

	float					values[4];
} uniform_t;

typedef struct program_s {
	char					name[MAX_PATH_LENGTH];

	shader_t *				vertexShader;
	shader_t *				fragmentShader;

	int						vertexAttribs;

	int						numUniforms;
	uniform_t *				uniforms;

	bool					linkStatus;

	uint					programId;

	struct program_s *		nextHash;
} program_t;

program_t *		R_FindProgram (const char *name, shader_t *vertexShader, shader_t *fragmentShader);

uniform_t *		R_GetProgramUniform (program_t *program, const char *name);
uniform_t *		R_GetProgramUniformExplicit (program_t *program, const char *name, int size, uint format);

void			R_SetProgramSampler (program_t *program, uniform_t *uniform, int unit);
void			R_SetProgramSamplerExplicit (program_t *program, const char *name, int size, uint format, int unit);

void			R_UniformFloat (uniform_t *uniform, float v0);
void			R_UniformFloat2 (uniform_t *uniform, float v0, float v1);
void			R_UniformFloat3 (uniform_t *uniform, float v0, float v1, float v2);
void			R_UniformFloat4 (uniform_t *uniform, float v0, float v1, float v2, float v3);
void			R_UniformFloatArray (uniform_t *uniform, int count, const float *v);

void			R_UniformVector2 (uniform_t *uniform, const vec2_t v);
void			R_UniformVector2Array (uniform_t *uniform, int count, const vec2_t *v);

void			R_UniformVector3 (uniform_t *uniform, const vec3_t v);
void			R_UniformVector3Array (uniform_t *uniform, int count, const vec3_t *v);

void			R_UniformVector4 (uniform_t *uniform, const vec4_t v);
void			R_UniformVector4Array (uniform_t *uniform, int count, const vec4_t *v);

void			R_UniformMatrix3 (uniform_t *uniform, bool transpose, const mat3_t m);
void			R_UniformMatrix3Array (uniform_t *uniform, int count, bool transpose, const mat3_t *m);
void			R_UniformMatrix3Identity (uniform_t *uniform);

void			R_UniformMatrix4 (uniform_t *uniform, bool transpose, const mat4_t m);
void			R_UniformMatrix4Array (uniform_t *uniform, int count, bool transpose, const mat4_t *m);
void			R_UniformMatrix4Identity (uniform_t *uniform);

void			R_InitPrograms ();
void			R_ShutdownPrograms ();

/*
 ==============================================================================

 MATERIAL MANAGER

 ==============================================================================
*/

#define MAX_MATERIALS				4096
#define MAX_STAGES					64

#define MAX_EXPRESSION_OPS			1024
#define MAX_EXPRESSION_REGISTERS	1024

#define MAX_TEXMODS					4

#define MAX_SHADER_PARMS			32
#define MAX_SHADER_MAPS				16

typedef enum {
	SURFACEPARM_NONE				= BIT(0),
	SURFACEPARM_LIGHTING			= BIT(1),
	SURFACEPARM_SKY					= BIT(2),
	SURFACEPARM_WARP				= BIT(3),
	SURFACEPARM_TRANS33				= BIT(4),
	SURFACEPARM_TRANS66				= BIT(5),
	SURFACEPARM_FLOWING				= BIT(6)
} surfaceParm_t;

typedef enum {
	MT_NONE							= -1,
	MT_GENERIC,
	MT_NOMIP
} materialType_t;

typedef enum {
	MF_EXPLICIT						= BIT(0),
	MF_DEFAULTED					= BIT(1),
	MF_FORCEOVERLAYS				= BIT(2),
	MF_NOOVERLAYS					= BIT(3),
	MF_FORCESHADOWS					= BIT(4),
	MF_NOSHADOWS					= BIT(5),
	MF_UPDATECURRENTCOLOR			= BIT(6),
	MF_NEEDCURRENTCOLOR				= BIT(7),
	MF_UPDATECURRENTDEPTH			= BIT(8),
	MF_NEEDCURRENTDEPTH				= BIT(9),
	MF_POLYGONOFFSET				= BIT(10)
} materialFlags_t;

typedef enum {
	MC_OPAQUE,
	MC_PERFORATED,
	MC_TRANSLUCENT
} materialCoverage_t;

typedef enum {
	SORT_BAD,
	SORT_OPAQUE,
	SORT_DECAL,
	SORT_GLARE,
	SORT_REFRACTABLE,
	SORT_REFRACTIVE,
	SORT_FARTHEST,
	SORT_ALMOST_FARTHEST,
	SORT_FAR,
	SORT_MEDIUM,
	SORT_NEAR,
	SORT_ALMOST_NEAREST,
	SORT_NEAREST,
	SORT_POST_PROCESS
} sort_t;

typedef enum {
	ST_NONE,
	ST_SKY,
	ST_MIRROR,
	ST_PORTAL,
	ST_REMOTE
} subviewType_t;

typedef enum {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

typedef enum {
	DFRM_NONE,
	DFRM_EXPAND,
	DFRM_MOVE,
	DFRM_SPRITE,
	DFRM_TUBE,
	DFRM_BEAM
} deform_t;

typedef enum {
	SL_AMBIENT,
	SL_BUMP,
	SL_DIFFUSE,
	SL_SPECULAR
} stageLighting_t;

typedef enum {
	TG_EXPLICIT,
	TG_VECTOR,
	TG_NORMAL,
	TG_REFLECT,
	TG_SKYBOX,
	TG_SCREEN
} texGen_t;

typedef enum {
	TM_TRANSLATE,
	TM_SCALE,
	TM_CENTERSCALE,
	TM_SHEAR,
	TM_ROTATE
} texMod_t;

typedef enum {
	VC_IGNORE,
	VC_MODULATE,
	VC_INVERSE_MODULATE
} vertexColor_t;

typedef enum {
	DS_POLYGONOFFSET				= BIT(0),
	DS_BLEND						= BIT(1),
	DS_ALPHATEST					= BIT(2),
	DS_IGNOREALPHATEST				= BIT(3),
	DS_MASKRED						= BIT(4),
	DS_MASKGREEN					= BIT(5),
	DS_MASKBLUE						= BIT(6),
	DS_MASKALPHA					= BIT(7)
} drawState_t;

typedef enum {
	OP_TYPE_MULTIPLY,
	OP_TYPE_DIVIDE,
	OP_TYPE_MOD,
	OP_TYPE_ADD,
	OP_TYPE_SUBTRACT,
	OP_TYPE_GREATER,
	OP_TYPE_LESS,
	OP_TYPE_GEQUAL,
	OP_TYPE_LEQUAL,
	OP_TYPE_EQUAL,
	OP_TYPE_NOTEQUAL,
	OP_TYPE_AND,
	OP_TYPE_OR,
	OP_TYPE_TABLE
} opType_t;

typedef enum {
	EXP_REGISTER_CONSTANT_ONE,
	EXP_REGISTER_CONSTANT_ZERO,
	EXP_REGISTER_TIME,
	EXP_REGISTER_PARM0,
	EXP_REGISTER_PARM1,
	EXP_REGISTER_PARM2,
	EXP_REGISTER_PARM3,
	EXP_REGISTER_PARM4,
	EXP_REGISTER_PARM5,
	EXP_REGISTER_PARM6,
	EXP_REGISTER_PARM7,
	EXP_REGISTER_NUM_PREDEFINED
} expRegister_t;

typedef struct {
	opType_t				type;

	int						a;
	int						b;
	int						c;
} expOp_t;

typedef struct {
	float					fadeInTime;
	float					stayTime;
	float					fadeOutTime;

	vec4_t					startRGBA;
	vec4_t					stayRGBA;
	vec4_t					endRGBA;
} decalInfo_t;

typedef struct {
	uniform_t *				uniform;

	int						registers[4];
} shaderParm_t;

typedef struct {
	uniform_t *				uniform;

	texture_t *				texture;

	int						cinematicHandle;
} shaderMap_t;

typedef struct {
	int						flags;
	textureFilter_t			filter;
	textureWrap_t			wrap;

	texture_t *				texture;

	int						cinematicHandle;

	texGen_t				texGen;
	vec4_t					texGenVectors[2];
	int						texGenRegisters[3];

	int						numTexMods;
	texMod_t				texMods[MAX_TEXMODS];
	int						texModsRegisters[MAX_TEXMODS][2];
} textureStage_t;

typedef struct {
	bool					identity;

	float					scale;
	float					bias;

	int						registers[4];

	vertexColor_t			vertexColor;
} colorStage_t;

typedef struct {
	program_t *				program;

	shader_t *				vertexShader;
	shader_t *				fragmentShader;

	float					colorScale;
	float					colorBias;

	int						numShaderParms;
	shaderParm_t			shaderParms[MAX_SHADER_PARMS];

	int						numShaderMaps;
	shaderMap_t				shaderMaps[MAX_SHADER_MAPS];
} shaderStage_t;

typedef struct {
	stageLighting_t			lighting;

	int						conditionRegister;

	textureStage_t			textureStage;
	colorStage_t			colorStage;
	shaderStage_t			shaderStage;

	vec4_t					parms;

	int						drawState;

	float					polygonOffset;

	uint					blendSrc;
	uint					blendDst;
	uint					blendMode;

	int						alphaTestRegister;
} stage_t;

typedef struct material_s {
	char					name[MAX_PATH_LENGTH];
	int						index;

	materialType_t			type;
	int						flags;

	materialCoverage_t		coverage;

	uint					surfaceParm;

	int						conditionRegister;

	sort_t					sort;

	subviewType_t			subviewType;
	texture_t *				subviewTexture;
	int						subviewWidth;
	int						subviewHeight;

	cullType_t				cullType;

	float					polygonOffset;

	decalInfo_t				decalInfo;

	deform_t				deform;
	int						deformRegisters[3];

	int						numStages;
	int						numAmbientStages;
	stage_t *				stages;

	bool					constantExpressions;

	int						numOps;
	expOp_t *				expressionOps;

	int						numRegisters;
	float *					expressionRegisters;

	struct material_s *		nextHash;
} material_t;

material_t *	R_FindMaterial (const char *name, materialType_t type, surfaceParm_t surfaceParm);
material_t *	R_RegisterMaterial (const char *name, surfaceParm_t surfaceParm);
material_t *	R_RegisterMaterialNoMip (const char *name, surfaceParm_t surfaceParm);

void			R_InitMaterials ();
void			R_ShutdownMaterials ();

/*
 ==============================================================================

 FONT MANAGER

 ==============================================================================
*/

#define MAX_FONTS					16

typedef struct font_s {
	char					name[MAX_PATH_LENGTH];
	bool					defaulted;

	fontSet_t *				fontSet;

	struct font_s *			nextHash;
} font_t;

font_t *		R_FindFont (const char *name);

void			R_InitFonts ();
void			R_ShutdownFonts ();

/*
 ==============================================================================

 ARRAY BUFFER MANAGER

 ==============================================================================
*/

#define MAX_ARRAY_BUFFERS			2048

typedef struct arrayBuffer_s {
	char					name[MAX_QPATH];

	bool					dynamic;
	int						count;

	int						size;
	uint					usage;

	int						frameUsed;

	uint					bufferId;

	struct arrayBuffer_s *	next;
} arrayBuffer_t;

arrayBuffer_t *	R_AllocVertexBuffer (const char *name, bool dynamic, int vertexCount);

void			R_UpdateVertexBuffer (arrayBuffer_t *vertexBuffer, const void *data, int size);

void			R_InitArrayBuffers ();
void			R_ShutdownArrayBuffers ();

/*
 ==============================================================================

 MODEL MANAGER

 ==============================================================================
*/

#define MAX_MODELS			512

#define	SURF_PLANEBACK		1
#define SURF_WATERCAUSTICS	2
#define SURF_SLIMECAUSTICS	4
#define SURF_LAVACAUSTICS	8

typedef struct texInfo_s {
	int						flags;
	float					vecs[2][4];

	material_t *			material;
	int						width;
	int						height;
	bool					clamp;

	int						numFrames;
	struct texInfo_s *		next;		// Animation chain
} texInfo_t;

typedef struct {
	glIndex_t				index[3];
} surfTriangle_t;

typedef struct {
	vec3_t					xyz;
	vec2_t					st;
	vec2_t					lightmap;
	color_t					color;
} surfVertex_t;

typedef struct {
	int						flags;

	int						firstEdge;	// Look up in model->edges[]. Negative
	int						numEdges;	// numbers are backwards edges

	cplane_t *				plane;

	vec3_t					mins;
	vec3_t					maxs;

	short					textureMins[2];
	short					extents[2];

	vec3_t					tangent;
	vec3_t					binormal;
	vec3_t					normal;

	texInfo_t *				texInfo;

	int						numTriangles;
	surfTriangle_t			*triangles;

	int						numVertices;
	surfVertex_t			*vertices;

	// Frame counters
	int						viewCount;
	int						worldCount;
	int						fragmentCount;

	// Lighting info
	int						dlightFrame;
	int						dlightBits;

	int						lmWidth;
	int						lmHeight;
	int						lmS;
	int						lmT;
	int						lmNum;
	byte *					lmSamples;

	int						numStyles;
	byte					styles[MAX_STYLES];
	float					cachedLight[MAX_STYLES];	// Values currently used in lightmap
} surface_t;

typedef struct node_s {
	// Common with leaf
	int						contents;	// -1, to differentiate from leafs
	int						visCount;	// Node needs to be traversed if current

	vec3_t					mins;		// For bounding box culling
	vec3_t					maxs;		// For bounding box culling

	struct node_s *			parent;

	// Node specific
	cplane_t *				plane;
	struct node_s *			children[2];	

	ushort					firstSurface;
	ushort					numSurfaces;
} node_t;

typedef struct leaf_s {
	// Common with node
	int						contents;	// Will be a negative contents number
	int						visCount;	// Node needs to be traversed if current

	vec3_t					mins;		// For bounding box culling
	vec3_t					maxs;		// For bounding box culling

	struct node_s *			parent;

	// Leaf specific
	int						cluster;
	int						area;

	surface_t **			firstMarkSurface;
	int						numMarkSurfaces;
} leaf_t;

typedef struct {
	int						numClusters;
	int						bitOfs[8][2];
} vis_t;

typedef struct {
	vec3_t					point;
} vertex_t;

typedef struct {
	ushort					v[2];
} edge_t;

typedef struct {
	vec3_t					mins;
	vec3_t					maxs;
	vec3_t					origin;		// For sounds or lights
	float					radius;
	int						headNode;
	int						visLeafs;	// Not including the solid leaf 0
	int						firstFace;
	int						numFaces;
} inlineModel_t;

typedef struct {
	material_t *			material;

	float					rotate;
	vec3_t					axis;
} sky_t;

typedef struct {
	vec3_t					lightDir;
} lightGrid_t;

typedef struct {
	glIndex_t				index[3];
	int						neighbor[3];
} mdlTriangle_t;

typedef struct {
	vec3_t					xyz;
	vec3_t					normal;
	vec3_t					tangents[2];
} mdlXyzNormal_t;

typedef struct {
	vec2_t					st;
} mdlSt_t;

typedef struct {
	material_t *			material;
} mdlMaterial_t;

typedef struct {
	vec3_t					mins;
	vec3_t					maxs;
	vec3_t					scale;
	vec3_t					translate;
	float					radius;
} mdlFrame_t;

typedef struct {
	char					name[MAX_QPATH];
	vec3_t					origin;
	vec3_t					axis[3];
} mdlTag_t;

typedef struct {
	int						numTriangles;
	int						numVertices;
	int						numMaterials;

	mdlTriangle_t *			triangles;
	mdlXyzNormal_t *		xyzNormals;
	mdlSt_t *				st;
	mdlMaterial_t *			materials;
} mdlSurface_t;

typedef struct {
	int						numFrames;
	int						numTags;
	int						numSurfaces;

	mdlFrame_t *			frames;
	mdlTag_t *				tags;
	mdlSurface_t *			surfaces;
} mdl_t;

typedef struct {
	material_t *			material;
	float					radius;
} sprFrame_t;

typedef struct {
	int						numFrames;
	sprFrame_t *			frames;
} spr_t;

typedef enum {
	MODEL_STATIC,
	MODEL_INLINE,
	MODEL_MD3,
	MODEL_MD2,
	MODEL_SP2
} modelType_t;

typedef struct model_s {
	char					name[MAX_QPATH];
	bool					defaulted;

	modelType_t				type;
	void *					data;
	int						size;

	struct model_s *		nextHash;

	// Volume occupied by the model
	vec3_t					mins;
	vec3_t					maxs;
	float					radius;

	// Brush model
	int						numModelSurfaces;
	int						firstModelSurface;

	int						numInlineModels;
	inlineModel_t *			inlineModels;

	int						numVertices;
	vertex_t *				vertices;

	int						numSurfEdges;
	int *					surfEdges;

	int						numEdges;
	edge_t *				edges;

	int						numTexInfo;
	texInfo_t *				texInfo;

	int						numSurfaces;
	surface_t *				surfaces;

	int						numMarkSurfaces;
	surface_t **			markSurfaces;

	int						numPlanes;
	cplane_t *				planes;

	int						numNodes;
	int						firstNode;
	node_t *				nodes;

	int						numLeafs;
	leaf_t *				leafs;

	sky_t *					sky;

	vis_t *					vis;

	byte *					lightData;

	vec3_t					gridMins;
	vec3_t					gridSize;
	int						gridBounds[4];
	int						gridPoints;
	lightGrid_t *			lightGrid;

	// Alias model
	mdl_t *					alias;
	int						numAlias;

	// Sprite model
	spr_t *					sprite;
} model_t;

leaf_t *		R_PointInLeaf (const vec3_t p);
byte *			R_ClusterPVS (int cluster);

void			R_InitModels ();
void			R_ShutdownModels ();

/*
 ==============================================================================

 DECAL MANAGER

 ==============================================================================
*/

#define MAX_DECALS					2048
#define MAX_DECAL_VERTICES			10

typedef struct decal_s {
	surface_t				*parentSurface;

	struct decal_s *		prev;
	struct decal_s *		next;
} decal_t;

void			R_ClearDecals ();
void			R_AddDecals ();

/*
 ==============================================================================

 IMAGE LOADING

 ==============================================================================
*/

bool			R_LoadImage (const char *name, int flags, textureWrap_t wrap, byte **image, int *width, int *height, textureFormat_t *format, bool *uncompressed);
bool			R_LoadCubeImages (const char *name, int flags, bool cameraSpace, byte **images, int *width, int *height, textureFormat_t *format, bool *uncompressed);

void			R_InitImages ();
void			R_ShutdownImages ();

/*
 ==============================================================================

 GL STATE MANAGER

 ==============================================================================
*/

#define MAX_TEXTURE_UNITS			16

typedef enum {
	GL_ATTRIB_NORMAL				= 2,
	GL_ATTRIB_TANGENT1				= 8,
	GL_ATTRIB_TANGENT2				= 9,
	GL_ATTRIB_TEXCOORD				= 10,
	GL_ATTRIB_COLOR					= 11
} glVertexAttrib_t;

typedef struct {
	bool					projectionMatrixIdentity;
	bool					modelviewMatrixIdentity;
	bool					textureMatrixIdentity[MAX_TEXTURE_UNITS];

	texture_t *				texture[MAX_TEXTURE_UNITS];
	program_t *				program;
	arrayBuffer_t *			vertexBuffer;

	int						viewportX;
	int						viewportY;
	int						viewportWidth;
	int						viewportHeight;

	int						scissorX;
	int						scissorY;
	int						scissorWidth;
	int						scissorHeight;

	int						texUnit;
	uint					texTarget[MAX_TEXTURE_UNITS];

	int						texEnv[MAX_TEXTURE_UNITS];
	int						texGen[MAX_TEXTURE_UNITS][4];

	bool					cullFace;
	bool					polygonOffsetFill;
	bool					polygonOffsetLine;
	bool					blend;
	bool					alphaTest;
	bool					depthTest;
	bool					stencilTest;
	bool					textureGen[MAX_TEXTURE_UNITS][4];

	uint					cullMode;
	uint					polygonMode;
	float					polygonOffsetFactor;
	float					polygonOffsetUnits;
	uint					blendSrc;
	uint					blendDst;
	uint					blendMode;
	uint					alphaFunc;
	float					alphaFuncRef;
	uint					depthFunc;
	uint					stencilFunc[2];
	int						stencilFuncRef[2];
	uint					stencilFuncMask[2];
	uint					stencilOpFail[2];
	uint					stencilOpZFail[2];
	uint					stencilOpZPass[2];
	float					depthMin;
	float					depthMax;

	bool					colorMask[4];
	bool					depthMask;
	uint					stencilMask[2];
} glState_t;

void			GL_LoadIdentity (uint mode);
void			GL_LoadMatrix (uint mode, const mat4_t matrix);
void			GL_LoadTransposeMatrix (uint mode, const mat4_t matrix);

void			GL_BindTexture (texture_t *texture);
void			GL_BindMultitexture (texture_t *texture, int unit);
void			GL_SelectTexture (int unit);
void			GL_EnableTexture (uint target);
void			GL_DisableTexture ();
void			GL_TexEnv (int texEnv);
void			GL_TexGen (uint texCoord, int texGen);

void			GL_BindProgram (program_t *program);

void			GL_BindVertexBuffer (arrayBuffer_t *vertexBuffer);

void			GL_Viewport (int x, int y, int width, int height);

void			GL_Scissor (int x, int y, int width, int height);

void			GL_Enable (uint cap);
void			GL_Disable (uint cap);
void			GL_CullFace (uint mode);
void			GL_PolygonMode (uint mode);
void			GL_PolygonOffset (float factor, float units);
void			GL_BlendFunc (uint src, uint dst);
void			GL_BlendEquation (uint mode);
void			GL_AlphaFunc (uint func, float ref);
void			GL_DepthFunc (uint func);
void			GL_StencilFunc (uint func, int ref, uint mask);
void			GL_StencilFuncSeparate (uint funcFront, uint funcBack, int refFront, int refBack, uint maskFront, uint maskBack);
void			GL_StencilOp (uint fail, uint zFail, uint zPass);
void			GL_StencilOpSeparate (uint failFront, uint failBack, uint zFailFront, uint zFailBack, uint zPassFront, uint zPassBack);
void			GL_DepthRange (float min, float max);
void			GL_ColorMask (bool red, bool green, bool blue, bool alpha);
void			GL_DepthMask (bool mask);
void			GL_StencilMask (uint mask);
void			GL_StencilMaskSeparate (uint maskFront, uint maskBack);

void			GL_Setup3D (int time);
void			GL_Setup2D (int time);

void			GL_SetDefaultState ();

void			GL_CheckForErrors ();

/*
 ==============================================================================

 MESH BUFFER

 ==============================================================================
*/

#define MAX_MESHES					32768

#define MESH_SHIFT_SORT				28
#define MESH_SHIFT_ENTITY			16
#define MESH_SHIFT_MATERIAL			4

typedef void				meshData_t;

typedef enum {
	MESH_SURFACE,
	MESH_ALIASMODEL,
	MESH_SPRITE,
	MESH_BEAM,
	MESH_PARTICLE,
	MESH_DECAL
} meshType_t;

typedef struct {
	meshType_t				type;
	meshData_t *			data;

	renderEntity_t *		entity;
	material_t *			material;

	uint					sort;
} mesh_t;

void			R_AddMeshToList (meshType_t type,  meshData_t *data, renderEntity_t *entity, material_t *material);

void			R_SortMeshes (int numMeshes, mesh_t *meshes);

void			R_AllocMeshes ();
void			R_GenerateMeshes ();
void			R_ClearMeshLists ();

/*
 ==============================================================================

 BACK-END

 ==============================================================================
*/

#define MAX_COMMAND_BUFFER_SIZE		1048576

#define MAX_INDICES					16384 * 3
#define MAX_VERTICES				8192

#define MAX_DYNAMIC_VERTICES		(MAX_VERTICES << 3)

typedef enum {
	FRUSTUM_LEFT,
	FRUSTUM_RIGHT,
	FRUSTUM_TOP,
	FRUSTUM_BOTTOM,
	FRUSTUM_NEAR,
	FRUSTUM_USER,
	NUM_FRUSTUM_PLANES
} frustum_t;

typedef enum {
	AP_OPAQUE,
	AP_TRANSLUCENT,
	AP_POST_PROCESS
} ambientPass_t;

typedef enum {
	RC_RENDER_VIEW,
	RC_CAPTURE_RENDER,
	RC_UPDATE_TEXTURE,
	RC_SET_COLOR,
	RC_SET_PARAMETER,
	RC_SET_PARAMETERS,
	RC_DRAW_STRETCH_PIC,
	RC_DRAW_CINEMATIC,
	RC_CROP_SIZE,
	RC_SETUP_BUFFERS,
	RC_SWAP_BUFFERS,
	RC_END_OF_LIST
} renderCommand_t;

typedef struct {
	byte *					data;
	int						size;
} commandBuffer_t;

typedef struct {
	int						x;
	int						y;
	int						width;
	int						height;
} viewport_t;

typedef struct {
	int						x;
	int						y;
	int						width;
	int						height;
} scissor_t;

typedef struct {
	bool					primaryView;
	int						viewType;

	viewport_t				viewport;
	scissor_t				scissor;

	vec3_t					origin;
	vec3_t					axis[3];

	mat4_t					projectionMatrix;
	mat4_t					modelviewMatrix;
	mat4_t					modelviewProjectionMatrix;
	mat4_t					skyBoxMatrix;

	int						numMeshes[4];
	mesh_t *				meshes[4];
} renderViewParms_t;

typedef struct {
	vec3_t					viewOrigin;
	mat4_t					viewMatrix;
} renderLocalParms_t;

typedef struct {
	renderCommand_t			commandId;

	int						time;

	renderViewParms_t		viewParms;
} renderViewCommand_t;

typedef struct {
	renderCommand_t			commandId;

	texture_t *				texture;
} captureRenderCommand_t;

typedef struct {
	renderCommand_t			commandId;

	texture_t *				texture;

	const byte *			image;
	int						width;
	int						height;
} updateTextureCommand_t;

typedef struct {
	renderCommand_t			commandId;

	color_t					color;
} setColorCommand_t;

typedef struct {
	renderCommand_t			commandId;

	int						index;
	float					value;
} setParameterCommand_t;

typedef struct {
	renderCommand_t			commandId;

	float					parms[MAX_MATERIAL_PARMS];
} setParametersCommand_t;

typedef struct {
	renderCommand_t			commandId;

	material_t *			material;

	float					x1;
	float					y1;
	float					x2;
	float					y2;
	float					x3;
	float					y3;
	float					x4;
	float					y4;

	float					s1;
	float					t1;
	float					s2;
	float					t2;
} drawPicStretchCommand_t;

typedef struct {
	renderCommand_t			commandId;

	texture_t *				texture;

	const byte *			image;
	bool					dirty;
	int						width;
	int						height;

	int						vx1;
	int						vy1;
	int						vx2;
	int						vy2;

	int						wx1;
	int						wy1;
	int						wx2;
	int						wy2;
} drawCinematicCommand_t;

typedef struct {
	renderCommand_t			commandId;

	int						width;
	int						height;
} cropSizeCommand_t;

typedef struct {
	renderCommand_t			commandId;
} setupBuffersCommand_t;

typedef struct {
	renderCommand_t			commandId;
} swapBuffersCommand_t;

typedef struct {
	// General state
	bool					projection2D;
	color_t					color2D;
	float					parms2D[MAX_MATERIAL_PARMS];

	int						time;
	float					floatTime;

	viewport_t				viewport;

	vec2_t					coordScale;
	vec2_t					coordBias;

	bool					depthFilling;

	sort_t					currentColorCaptured;
	bool					currentDepthCaptured;

	// Crop size
	int						cropWidth;
	int						cropHeight;

	// Render commands
	commandBuffer_t			commandBuffer;

	// View parms
	renderViewParms_t		viewParms;

	// Local parms
	renderLocalParms_t		localParms;

	// Batch state
	material_t *			material;
	renderEntity_t *		entity;

	// Batch mesh data
	meshData_t *			meshData;

	arrayBuffer_t *			vertexBuffer;
	const void *			vertexPointer;

	// Draw function for current batch
	void					(*drawBatch)();

	// Index and vertex arrays
	int						numIndices;
	glIndex_t *				indices;

	int						numVertices;
	glVertex_t *			vertices;
} backEnd_t;

extern backEnd_t			backEnd;

// Geometry batching
void			RB_CheckMeshOverflow (int numIndices, int numVertices);
void			RB_SetupBatch (renderEntity_t *entity, material_t *material, void (*drawBatch)());
void			RB_RenderBatch ();

void			RB_RenderMeshes (mesh_t *meshes, int numMeshes);

void			RB_BatchGeometry (meshType_t type, meshData_t *data);

void			RB_BatchShadowGeometry (meshType_t type, meshData_t *data);

// Material deforms
void			RB_Deform (material_t *material);

// Material expression evaluation
void			RB_EvaluateRegisters (material_t *material, float time, const float *parms);

// Material rendering setup
void			RB_Cull (material_t *material);
void			RB_PolygonOffset (material_t *material);
void			RB_DrawState (material_t *material, stage_t *stage);

void			RB_BindTexture (material_t *material, texture_t *texture, int cinematicHandle);
void			RB_BindMultitexture (material_t *material, texture_t *texture, int cinematicHandle, int unit);

void			RB_ComputeTexturePlanes (material_t *material, textureStage_t *textureStage, vec4_t planes[4]);
void			RB_ComputeTextureMatrix (material_t *material, textureStage_t *textureStage, mat4_t matrix);

void			RB_SetupTextureStage (material_t *material, textureStage_t *textureStage);
void			RB_CleanupTextureStage (material_t *material, textureStage_t *textureStage);

void			RB_SetupColorStage (material_t *material, colorStage_t *colorStage);
void			RB_CleanupColorStage (material_t *material, colorStage_t *colorStage);

void			RB_SetupShaderStage (material_t *material, shaderStage_t *shaderStage);
void			RB_CleanupShaderStage (material_t *material, shaderStage_t *shaderStage);

// Rendering setup & utilities
void			RB_EntityState (renderEntity_t *entity);

void			RB_DrawElements ();

// Generic rendering
void			RB_FillDepthBuffer (int numMeshes, mesh_t *meshes);

void			RB_RenderMaterialPasses (int numMeshes, mesh_t *meshes, ambientPass_t pass);

void			RB_RenderShadows (int numMeshes, mesh_t *meshes);

void			RB_DrawMaterial2D ();

// Debug tools rendering
void			RB_RenderDebugTools ();

// Main back-end interface.
// These should be the only functions ever called by the front-end.
void			RB_ExecuteRenderCommands (const void *data);

void			RB_InitBackEnd ();
void			RB_ShutdownBackEnd ();

/*
 ==============================================================================

 FRONT-END

 ==============================================================================
*/

#define MAX_RENDER_CROPS			8

#define MAX_LIGHTMAPS				128

typedef enum {
	ASPECT_NORMAL,
	ASPECT_WIDE,
	ASPECT_HIGH
} aspectRatio_t;

typedef struct {
	bool					primaryView;
	int						viewType;

	viewport_t				viewport;
	scissor_t				scissor;

	float					fovX;
	float					fovY;
	float					fovScale;

	cplane_t				frustum[4];

	vec3_t					worldMins;
	vec3_t					worldMaxs;

	mat4_t					projectionMatrix;
	mat4_t					modelviewMatrix;
	mat4_t					modelviewProjectionMatrix;
	mat4_t					skyBoxMatrix;

	// Mesh data
	int						numMeshes[4];
	mesh_t *				meshes[4];

	// Scene render lists
	renderEntity_t *		renderEntities;
	int						numRenderEntities;

	renderLight_t *			renderLights;
	int						numRenderLights;

	renderParticle_t *		renderParticles;
	int						numRenderParticles;
} viewParms_t;

typedef struct {
	int 					x;
	int 					y;
	int 					width;
	int						height;
} cropRect_t;

typedef struct {
	int						width;
	int						height;

	cropRect_t				rect;

	float					xScale;
	float					yScale;

	aspectRatio_t			aspectRatio;
	float					aspectScale;
	float					aspectBias;
} renderCrop_t;

typedef struct {
	renderEntity_t			entities[MAX_RENDER_ENTITIES];
	int						numEntities;
	int						firstEntity;

	renderLight_t			lights[MAX_RENDER_LIGHTS];
	int						numLights;
	int						firstLight;

	renderParticle_t		particles[MAX_RENDER_PARTICLES];
	int						numParticles;
	int						firstParticle;
} scene_t;

typedef struct {
	int						numEntities;
	int						firstEntity;

	int						numLights;
	int						firstLight;

	int						numParticles;
	int						firstParticle;

	renderView_t			renderView;
} primaryView_t;

typedef struct {
	int						cullBoundsIn;
	int						cullBoundsOut;
	int						cullSphereIn;
	int						cullSphereOut;

	int						entities;
	int						lights;
	int						particles;
	int						decals;

	int						leafs;

	int						meshes;

	int						deformIndices;
	int						deformVertices;
	int						deformExpand;
	int						deformMove;
	int						deformSprite;
	int						deformTube;
	int						deformBeam;

	int						views;
	int						draws;
	int						totalIndices;
	int						totalVertices;

	int						vertexBuffers[2];
	int						vertexBufferBytes[2];

	int						textures;
	int						textureBytes;

	int						captureTextures;
	int						captureTexturePixels;
	int						updateTextures;
	int						updateTexturePixels;
} performanceCounters_t;

typedef struct {
	int						time;

	// Frame counters
	int						frameCount;
	int						viewCount;
	int						visCount;
	int						fragmentCount;

	// View cluster and area
	int						viewCluster, viewCluster2;
	int						oldViewCluster, oldViewCluster2;

	// Light styles
	lightStyle_t			lightStyles[MAX_LIGHTSTYLES];

	// Current view parms
	viewParms_t				viewParms;

	// Current render view
	renderView_t			renderView;

	// Meshes
	int						numMeshes[4];
	int						maxMeshes[4];
	int						firstMesh[4];
	mesh_t *				meshes[4];

	// Scene
	scene_t					scene;

	// Render crops
	renderCrop_t			renderCrops[MAX_RENDER_CROPS];
	int						currentRenderCrop;

	// World map
	model_t *				worldModel;
	renderEntity_t *		worldEntity;

	// Environment shot rendering
	bool					envShotRendering;
	int						envShotSize;

	// Copy of the primary view needed by some functions
	bool					primaryViewAvailable;
	primaryView_t			primaryView;

	// Development tools
	texture_t *				testTexture;

	// Performance counters
	performanceCounters_t	pc;

	// Internal assets
	texture_t *				defaultTexture;
	texture_t *				whiteTexture;
	texture_t *				blackTexture;
	texture_t *				flatTexture;
	texture_t *				lightmapTextures[MAX_LIGHTMAPS];
	texture_t *				cinematicTextures[MAX_CINEMATICS];
	texture_t *				skyTexture;
	texture_t *				mirrorTexture;
	texture_t *				remoteTexture;
	texture_t *				currentColorTexture;
	texture_t *				currentDepthTexture;

	material_t *			defaultMaterial;
	material_t *			defaultLightmapMaterial;
	material_t *			noDrawMaterial;
	material_t *			waterCausticsMaterial;
	material_t *			slimeCausticsMaterial;
	material_t *			lavaCausticsMaterial;

	font_t *				defaultFont;

	model_t *				defaultModel;

	// Tables
	byte					gammaTable[256];
} reGlobals_t;

extern reGlobals_t			rg;

extern glConfig_t			glConfig;

extern cvar_t *				r_logFile;
extern cvar_t *				r_ignoreGLErrors;
extern cvar_t *				r_clear;
extern cvar_t *				r_clearColor;
extern cvar_t *				r_frontBuffer;
extern cvar_t *				r_screenFraction;
extern cvar_t *				r_subviewOnly;
extern cvar_t *				r_lockVisibility;
extern cvar_t *				r_zNear;
extern cvar_t *				r_zFar;
extern cvar_t *				r_offsetFactor;
extern cvar_t *				r_offsetUnits;
extern cvar_t *				r_forceImagePrograms;
extern cvar_t *				r_writeImagePrograms;
extern cvar_t *				r_colorMipLevels;
extern cvar_t *				r_singleMaterial;
extern cvar_t *				r_singleEntity;
extern cvar_t *				r_singleLightmap;
extern cvar_t *				r_showCluster;
extern cvar_t *				r_showCull;
extern cvar_t *				r_showScene;
extern cvar_t *				r_showDeforms;
extern cvar_t *				r_showTextureUsage;
extern cvar_t *				r_showTextures;
extern cvar_t *				r_showDepth;
extern cvar_t *				r_showVertexColors;
extern cvar_t *				r_showTextureCoords;
extern cvar_t *				r_showTangentSpace;
extern cvar_t *				r_showTris;
extern cvar_t *				r_showNormals;
extern cvar_t *				r_showTextureVectors;
extern cvar_t *				r_showBatchSize;
extern cvar_t *				r_showModelBounds;
extern cvar_t *				r_skipLightning;
extern cvar_t *				r_skipVisibility;
extern cvar_t *				r_skipCulling;
extern cvar_t *				r_skipEntityCulling;
extern cvar_t *				r_skipScissors;
extern cvar_t *				r_skipSorting;
extern cvar_t *				r_skipEntities;
extern cvar_t *				r_skipParticles;
extern cvar_t *				r_skipDecals;
extern cvar_t *				r_skipExpressions;
extern cvar_t *				r_skipConstantExpressions;
extern cvar_t *				r_skipDeforms;
extern cvar_t *				r_skipAmbient;
extern cvar_t *				r_skipShadows;
extern cvar_t *				r_skipTranslucent;
extern cvar_t *				r_skipPostProcess;
extern cvar_t *				r_skipShaders;
extern cvar_t *				r_skipCopyToTextures;
extern cvar_t *				r_skipDynamicTextures;
extern cvar_t *				r_skipDrawElements;
extern cvar_t *				r_skipRender;
extern cvar_t *				r_skipRenderContext;
extern cvar_t *				r_skipFrontEnd;
extern cvar_t *				r_skipBackEnd;
extern cvar_t *				r_glDriver;
extern cvar_t *				r_mode;
extern cvar_t *				r_fullscreen;
extern cvar_t *				r_customWidth;
extern cvar_t *				r_customHeight;
extern cvar_t *				r_displayRefresh;
extern cvar_t *				r_multiSamples;
extern cvar_t *				r_alphaToCoverage;
extern cvar_t *				r_swapInterval;
extern cvar_t *				r_finish;
extern cvar_t *				r_gamma;
extern cvar_t *				r_contrast;
extern cvar_t *				r_brightness;
extern cvar_t *				r_vertexBuffers;
extern cvar_t *				r_shaderQuality;
extern cvar_t *				r_shadows;
extern cvar_t *				r_playerShadow;
extern cvar_t *				r_dynamicLights;
extern cvar_t *				r_caustics;
extern cvar_t *				r_depthClamp;
extern cvar_t *				r_seamlessCubeMaps;
extern cvar_t *				r_inGameVideos;
extern cvar_t *				r_precompressedImages;
extern cvar_t *				r_roundImagesDown;
extern cvar_t *				r_mipLevel;
extern cvar_t *				r_mipLevelBump;
extern cvar_t *				r_mipLevelDiffuse;
extern cvar_t *				r_mipLevelSpecular;
extern cvar_t *				r_maxTextureSize;
extern cvar_t *				r_compressTextures;
extern cvar_t *				r_compressNormalTextures;
extern cvar_t *				r_textureFilter;
extern cvar_t *				r_textureLODBias;
extern cvar_t *				r_textureAnisotropy;

void			R_AddAliasModel (renderEntity_t *entity);

void			R_AddEntityShadows ();

void			R_MarkLights ();
void			R_LightDirection (const vec3_t origin, vec3_t lightDir);
void			R_LightingAmbient ();
void			R_LightingDiffuse ();
void			R_BeginBuildingLightmaps ();
void			R_EndBuildingLightmaps ();
void			R_BuildSurfaceLightmap (surface_t *surface);
void			R_UpdateSurfaceLightmap (surface_t *surface);

bool			R_CullBox (const vec3_t mins, const vec3_t maxs, int clipFlags);
bool			R_CullSphere (const vec3_t origin, float radius, int clipFlags);

void			R_AdjustHorzCoords (horzAdjust_t adjust, float percent, float xIn, float wIn, float *xOut, float *wOut);
void			R_AdjustVertCoords (vertAdjust_t adjust, float percent, float yIn, float hIn, float *yOut, float *hOut);

void			R_AdjustHorzCoordsInt (horzAdjust_t adjust, float percent, int xIn, int wIn, int *xOut, int *wOut);
void			R_AdjustVertCoordsInt (vertAdjust_t adjust, float percent, int yIn, int hIn, int *yOut, int *hOut);

void			R_SetGamma ();

bool			R_GetVideoModeInfo (int mode, int *width, int *height);

bool			R_RenderSubview ();

void			R_RenderView (bool primaryView, int viewType);

void			R_AddRenderViewCommand ();

void			R_RenderShadows ();

void			R_AddInlineModel (renderEntity_t *entity);
void			R_AddWorldSurfaces ();

/*
 ==============================================================================

 IMPLEMENTATION SPECIFIC FUNCTIONS

 ==============================================================================
*/

#ifdef _WIN32

#define GL_DRIVER_OPENGL			"OpenGL32"

#define GLImp_SetDeviceGammaRamp	GLW_SetDeviceGammaRamp
#define GLImp_Activate				GLW_Activate
#define GLImp_ActivateContext		GLW_ActivateContext
#define GLImp_SwapBuffers			GLW_SwapBuffers
#define GLImp_Init					GLW_Init
#define GLImp_Shutdown				GLW_Shutdown

void			GLW_SetDeviceGammaRamp (const byte *gammaTable);
void			GLW_Activate (bool active);
void			GLW_ActivateContext (bool active);
void			GLW_SwapBuffers ();
void			GLW_Init ();
void			GLW_Shutdown ();

#else

#error "GLImp not available for this platform"

#endif


#endif	// __R_LOCAL_H__