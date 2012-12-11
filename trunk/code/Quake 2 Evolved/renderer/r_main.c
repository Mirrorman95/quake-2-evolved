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
// r_main.c - Primary renderer file
//


#include "r_local.h"


#define NUM_VIDEO_MODES				(sizeof(r_videoModes) / sizeof(videoMode_t))

typedef struct {
	const char *			description;

	int						width;
	int						height;
} videoMode_t;

static videoMode_t			r_videoModes[] = {
	{"Mode  0: 640x480 (4:3)",		640,	480},
	{"Mode  1: 800x600 (4:3)",		800,	600},
	{"Mode  2: 960x720 (4:3)",		960,	720},
	{"Mode  3: 1024x768 (4:3)",		1024,	768},
	{"Mode  4: 1152x864 (4:3)",		1152,	864},
	{"Mode  5: 1280x960 (4:3)",		1280,	960},
	{"Mode  6: 1400x1050 (4:3)",	1400,	1050},
	{"Mode  7: 1600x1200 (4:3)",	1600,	1200},
	{"Mode  8: 2048x1536 (4:3)",	2048,	1536},
	{"Mode  9: 720x576 (5:4)",		720,	576},
	{"Mode 10: 1280x1024 (5:4)",	1280,	1024},
	{"Mode 11: 640x360 (16:9)",		640,	360},
	{"Mode 12: 800x450 (16:9)",		800,	450},
	{"Mode 13: 960x540 (16:9)",		960,	540},
	{"Mode 14: 1024x576 (16:9)",	1024,	576},
	{"Mode 15: 1088x612 (16:9)",	1088,	612},
	{"Mode 16: 1280x720 (16:9)",	1280,	720},
	{"Mode 17: 1600x900 (16:9)",	1600,	900},
	{"Mode 18: 1920x1080 (16:9)",	1920,	1080},
	{"Mode 19: 2048x1152 (16:9)",	2048,	1152},
	{"Mode 20: 640x400 (16:10)",	640,	400},
	{"Mode 21: 800x500 (16:10)",	800,	500},
	{"Mode 22: 960x600 (16:10)",	960,	600},
	{"Mode 23: 1024x640 (16:10)",	1024,	640},
	{"Mode 24: 1152x720 (16:10)",	1152,	720},
	{"Mode 25: 1280x800 (16:10)",	1280,	800},
	{"Mode 26: 1440x900 (16:10)",	1440,	900},
	{"Mode 27: 1680x1050 (16:10)",	1680,	1050},
	{"Mode 28: 1920x1200 (16:10)",	1920,	1200}
};

reGlobals_t					rg;

glConfig_t					glConfig;

cvar_t *					r_logFile;
cvar_t *					r_ignoreGLErrors;
cvar_t *					r_clear;
cvar_t *					r_clearColor;
cvar_t *					r_frontBuffer;
cvar_t *					r_screenFraction;
cvar_t *					r_subviewOnly;
cvar_t *					r_lockVisibility;
cvar_t *					r_zNear;
cvar_t *					r_zFar;
cvar_t *					r_offsetFactor;
cvar_t *					r_offsetUnits;
cvar_t *					r_forceImagePrograms;
cvar_t *					r_writeImagePrograms;
cvar_t *					r_colorMipLevels;
cvar_t *					r_maxDebugPolygons;
cvar_t *					r_maxDebugLines;
cvar_t *					r_maxDebugText;
cvar_t *					r_singleMaterial;
cvar_t *					r_singleEntity;
cvar_t *					r_singleLight;
cvar_t *					r_showCluster;
cvar_t *					r_showFarClip;
cvar_t *					r_showCull;
cvar_t *					r_showScene;
cvar_t *					r_showSurfaces;
cvar_t *					r_showDeforms;
cvar_t *					r_showTextureUsage;
cvar_t *					r_showTextures;
cvar_t *					r_showDepth;
cvar_t *					r_showLightCount;
cvar_t *					r_showLightVolumes;
cvar_t *					r_showLightScissors;
cvar_t *					r_showShadowTris;
cvar_t *					r_showShadowVolumes;
cvar_t *					r_showShadowSilhouettes;
cvar_t *					r_showVertexColors;
cvar_t *					r_showTextureCoords;
cvar_t *					r_showTangentSpace;
cvar_t *					r_showTris;
cvar_t *					r_showNormals;
cvar_t *					r_showTextureVectors;
cvar_t *					r_showBatchSize;
cvar_t *					r_showModelBounds;
cvar_t *					r_skipVisibility;
cvar_t *					r_skipSuppress;
cvar_t *					r_skipCulling;
cvar_t *					r_skipFaceCulling;
cvar_t *					r_skipEntityCulling;
cvar_t *					r_skipLightCulling;
cvar_t *					r_skipScissors;
cvar_t *					r_skipLightScissors;
cvar_t *					r_skipSorting;
cvar_t *					r_skipEntities;
cvar_t *					r_skipLights;
cvar_t *					r_skipParticles;
cvar_t *					r_skipDecals;
cvar_t *					r_skipExpressions;
cvar_t *					r_skipConstantExpressions;
cvar_t *					r_skipLightCache;
cvar_t *					r_skipDeforms;
cvar_t *					r_skipAmbient;
cvar_t *					r_skipBump;
cvar_t *					r_skipDiffuse;
cvar_t *					r_skipSpecular;
cvar_t *					r_skipShadows;
cvar_t *					r_skipInteractions;
cvar_t *					r_skipAmbientLights;
cvar_t *					r_skipBlendLights;
cvar_t *					r_skipFogLights;
cvar_t *					r_skipTranslucent;
cvar_t *					r_skipPostProcess;
cvar_t *					r_skipShaders;
cvar_t *					r_skipCopyToTextures;
cvar_t *					r_skipDynamicTextures;
cvar_t *					r_skipDrawElements;
cvar_t *					r_skipRender;
cvar_t *					r_skipRenderContext;
cvar_t *					r_skipFrontEnd;
cvar_t *					r_skipBackEnd;
cvar_t *					r_glDriver;
cvar_t *					r_mode;
cvar_t *					r_fullscreen;
cvar_t *					r_customWidth;
cvar_t *					r_customHeight;
cvar_t *					r_displayRefresh;
cvar_t *					r_multiSamples;
cvar_t *					r_alphaToCoverage;
cvar_t *					r_swapInterval;
cvar_t *					r_finish;
cvar_t *					r_fullbright;
cvar_t *					r_gamma;
cvar_t *					r_contrast;
cvar_t *					r_brightness;
cvar_t *					r_indexBuffers;
cvar_t *					r_vertexBuffers;
cvar_t *					r_shaderQuality;
cvar_t *					r_lightScale;
cvar_t *					r_lightDetailLevel;
cvar_t *					r_shadows;
cvar_t *					r_playerShadow;
cvar_t *					r_dynamicLights;
cvar_t *					r_modulate;
cvar_t *					r_caustics;
cvar_t *					r_seamlessCubeMaps;
cvar_t *					r_inGameVideos;
cvar_t *					r_precompressedImages;
cvar_t *					r_roundImagesDown;
cvar_t *					r_mipLevel;
cvar_t *					r_mipLevelBump;
cvar_t *					r_mipLevelDiffuse;
cvar_t *					r_mipLevelSpecular;
cvar_t *					r_maxTextureSize;
cvar_t *					r_compressTextures;
cvar_t *					r_compressNormalTextures;
cvar_t *					r_textureFilter;
cvar_t *					r_textureLODBias;
cvar_t *					r_textureAnisotropy;


/*
 ==================
 R_SetGamma
 ==================
*/
void R_SetGamma (){

	float	f;
	int		i;

	// Build gamma table
	for (i = 0; i < 256; i++){
		f = i * (1.0f / 255.0f);

		// Adjust gamma
		f = Pow(f, 1.0f / r_gamma->floatValue);

		// Adjust contrast
		f = (f - 0.5f) * r_contrast->floatValue + 0.5f;

		// Adjust brightness
		if (r_brightness->floatValue < 1.0f)
			f = f + (r_brightness->floatValue - 1.0f);
		else
			f = f + (r_brightness->floatValue - 1.0f) * 0.5f;

		// Clamp the final value
		rg.gammaTable[i] = FloatToByte(f * 255.0f);
	}

	// Set device gamma ramp
	GLImp_SetDeviceGammaRamp(rg.gammaTable);
}

/*
 ==================
 R_SetLightStyle
 ==================
*/
void R_SetLightStyle (int style, float r, float g, float b){

	lightStyle_t	*ls;

	if (style < 0 || style >= MAX_LIGHTSTYLES)
		Com_Error(ERR_DROP, "R_SetLightStyle: out of range");

	ls = &rg.lightStyles[style];

	ls->white = r + g + b;
	VectorSet(ls->rgb, r, g, b);
}

/*
 ==================
 R_GetPicSize
 ==================
*/
void R_GetPicSize (material_t *material, float *w, float *h){

	*w = (float)material->stages->textureStage.texture->width;
	*h = (float)material->stages->textureStage.texture->height;
}

/*
 ==================
 R_GetVideoModeInfo
 ==================
*/
bool R_GetVideoModeInfo (int mode, int *width, int *height){

	if (mode == -1){
		*width = r_customWidth->integerValue;
		*height = r_customHeight->integerValue;

		return true;
	}

	if (mode < 0 || mode >= NUM_VIDEO_MODES)
		return false;

	*width = r_videoModes[mode].width;
	*height = r_videoModes[mode].height;

	return true;
}

/*
 ==================
 R_GetGLConfig
 ==================
*/
glConfig_t R_GetGLConfig (){

	return glConfig;
}

/*
 ==================
 R_Activate

 Called when the main window gains or loses focus.
 The window may have been destroyed and recreated between a deactivate and an
 activate.
 ==================
*/
void R_Activate (bool active){

	if (!glConfig.initialized)
		return;

	GLImp_Activate(active);
}


// ============================================================================


/*
 ==================
 R_ClearRender
 ==================
*/
static void R_ClearRender (){

	int		i;

	// TODO: Mem_Fill renderGlobals_t ?

	// TODO: clear counters?

	// Clear view clusters
	rg.viewCluster = rg.oldViewCluster = -1;
	rg.viewCluster2 = rg.oldViewCluster2 = -1;

	// Clear light styles
	for (i = 0; i < MAX_LIGHTSTYLES; i++){
		rg.lightStyles[i].rgb[0] = 1.0f;
		rg.lightStyles[i].rgb[1] = 1.0f;
		rg.lightStyles[i].rgb[2] = 1.0f;
	}

	// Clear decals
	R_ClearDecals();
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_ListVideoModes_f
 ==================
*/
static void R_ListVideoModes_f (){

	int		i;

	Com_Printf("Mode -1: %ix%i (custom)\n", r_customWidth->integerValue, r_customHeight->integerValue);

	for (i = 0; i < NUM_VIDEO_MODES; i++)
		Com_Printf("%s\n", r_videoModes[i].description);
}

/*
 ==================
 R_GfxInfo_f
 ==================
*/
static void R_GfxInfo_f (){

	Com_Printf("\n");
	Com_Printf("GL_VENDOR: %s\n", glConfig.vendorString);
	Com_Printf("GL_RENDERER: %s\n", glConfig.rendererString);
	Com_Printf("GL_VERSION: %s\n", glConfig.versionString);
	Com_Printf("GL_EXTENSIONS: %s\n", glConfig.extensionsString);

#ifdef _WIN32

	Com_Printf("WGL_EXTENSIONS: %s\n", glConfig.wglExtensionsString);

#endif

	Com_Printf("\n");
	Com_Printf("GL_SHADING_LANGUAGE_VERSION: %s\n", glConfig.shadingLanguageVersionString);
	Com_Printf("\n");
	Com_Printf("GL_MAX_TEXTURE_SIZE: %i\n", glConfig.maxTextureSize);
	Com_Printf("GL_MAX_3D_TEXTURE_SIZE: %i\n", glConfig.max3DTextureSize);
	Com_Printf("GL_MAX_CUBE_MAP_TEXTURE_SIZE: %i\n", glConfig.maxCubeMapTextureSize);
	Com_Printf("GL_MAX_ARRAY_TEXTURE_LAYERS: %i\n", glConfig.maxArrayTextureLayers);
	Com_Printf("GL_MAX_TEXTURE_UNITS: %i\n", glConfig.maxTextureUnits);
	Com_Printf("GL_MAX_TEXTURE_COORDS: %i\n", glConfig.maxTextureCoords);
	Com_Printf("GL_MAX_TEXTURE_IMAGE_UNITS: %i\n", glConfig.maxTextureImageUnits);
	Com_Printf("GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS: %i\n", glConfig.maxVertexTextureImageUnits);
	Com_Printf("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: %i\n", glConfig.maxCombinedTextureImageUnits);
	Com_Printf("GL_MAX_VERTEX_ATTRIBS: %i\n", glConfig.maxVertexAttribs);
	Com_Printf("GL_MAX_VARYING_COMPONENTS: %i\n", glConfig.maxVaryingComponents);
	Com_Printf("GL_MAX_VERTEX_UNIFORM_COMPONENTS: %i\n", glConfig.maxVertexUniformComponents);
	Com_Printf("GL_MAX_FRAGMENT_UNIFORM_COMPONENTS: %i\n", glConfig.maxFragmentUniformComponents);
	Com_Printf("GL_MAX_COLOR_ATTACHMENTS: %i\n", glConfig.maxColorAttachments);
	Com_Printf("GL_MAX_RENDERBUFFER_SIZE: %i\n", glConfig.maxRenderbufferSize);
	Com_Printf("GL_MAX_TEXTURE_LOD_BIAS: %g\n", glConfig.maxTextureLODBias);
	Com_Printf("GL_MAX_TEXTURE_MAX_ANISOTROPY: %g\n", glConfig.maxTextureMaxAnisotropy);
	Com_Printf("\n");
	Com_Printf("MODE: %i, %ix%i %s%s\n", r_mode->integerValue, glConfig.videoWidth, glConfig.videoHeight, (glConfig.isFullscreen) ? "fullscreen" : "windowed", (glConfig.displayFrequency) ? Str_VarArgs(" (%i Hz)", glConfig.displayFrequency) : "");
	Com_Printf("PIXELFORMAT: color(%i-bits) alpha(%i-bits) depth(%i-bits) stencil(%i-bits)\n", glConfig.colorBits, glConfig.alphaBits, glConfig.depthBits, glConfig.stencilBits);
	Com_Printf("SAMPLES: %s\n", (glConfig.multiSamples > 1) ? Str_VarArgs("%ix", glConfig.multiSamples) : "none");
	Com_Printf("CPU: %s\n", Sys_GetProcessorString());
	Com_Printf("\n");

	if (r_vertexBuffers->integerValue == 2)
		Com_Printf("Using static and dynamic vertex buffers\n");
	else if (r_vertexBuffers->integerValue == 1)
		Com_Printf("Using static vertex buffers\n");
	else
		Com_Printf("Vertex buffers are disabled\n");

	if (r_shaderQuality->integerValue == 2)
		Com_Printf("Using high quality shaders\n");
	else if (r_shaderQuality->integerValue == 1)
		Com_Printf("Using medium quality shaders\n");
	else
		Com_Printf("Using low quality shaders\n");

	if (glConfig.textureCompressionS3TCAvailable){
		if (r_compressTextures->integerValue)
			Com_Printf("Using DXTC texture compression\n");
		else
			Com_Printf("DXTC texture compression is disabled\n");
	}
	else
		Com_Printf("Texture compression not available\n");

	if (glConfig.textureFilterAnisotropicAvailable)
		Com_Printf("Using %s texture filtering w/ %g anisotropy\n", r_textureFilter->value, r_textureAnisotropy->floatValue);
	else
		Com_Printf("Using %s texture filtering\n", r_textureFilter->value);

	if (glConfig.swapControlAvailable){
		if (r_swapInterval->integerValue)
			Com_Printf("Forcing swap interval %i\n", r_swapInterval->integerValue);
		else
			Com_Printf("Swap interval not forced\n");
	}
	else
		Com_Printf("Swap interval not available\n");

	if (r_finish->integerValue)
		Com_Printf("Forcing glFinish\n");
	else
		Com_Printf("glFinish not forced\n");

	Com_Printf("\n");
}


/*
 ==============================================================================

 CONFIG REGISTRATION

 ==============================================================================
*/


/*
 ==================
 R_Register
 ==================
*/
static void R_Register (){

	// Register variables
	r_logFile = CVar_Register("r_logFile", "0", CVAR_INTEGER, CVAR_CHEAT, "Number of frames to log GL calls", 0, 0);
	r_ignoreGLErrors = CVar_Register("r_ignoreGLErrors", "1", CVAR_BOOL, CVAR_CHEAT, "Ignore GL errors", 0, 0);
	r_clear = CVar_Register("r_clear", "0", CVAR_BOOL, CVAR_CHEAT, "Clear the color buffer", 0, 0);
	r_clearColor = CVar_Register("r_clearColor", Str_FromInteger(COLOR_BLACK), CVAR_INTEGER, CVAR_CHEAT, "Clear color index", 0, COLOR_MASK);
	r_frontBuffer = CVar_Register("r_frontBuffer", "0", CVAR_BOOL, CVAR_CHEAT, "Render to the front buffer for debugging", 0, 0);	
	r_screenFraction = CVar_Register("r_screenFraction", "1.0", CVAR_FLOAT, CVAR_CHEAT, "Render to a fraction of the screen for testing fillrate", 0.1f, 1.0f);
	r_subviewOnly = CVar_Register("r_subviewOnly", "0", CVAR_BOOL, CVAR_CHEAT, "Only render subviews for debugging", 0, 0);
	r_lockVisibility = CVar_Register("r_lockVisibility", "0", CVAR_BOOL, CVAR_CHEAT, "Don't update visibility when the camera moves", 0, 0);
	r_zNear = CVar_Register("r_zNear", "3.0", CVAR_FLOAT, CVAR_CHEAT, "Near clip plane distance", 0.1f, 10.0f);
	r_zFar = CVar_Register("r_zFar", "0.0", CVAR_FLOAT, CVAR_CHEAT, "Far clip plane distance (0 = dynamic)", 0.0f, 0.0f);	
	r_offsetFactor = CVar_Register("r_offsetFactor", "-1.0", CVAR_FLOAT, CVAR_CHEAT, "Polygon offset factor", 0.0f, 0.0f);
	r_offsetUnits = CVar_Register("r_offsetUnits", "-2.0", CVAR_FLOAT, CVAR_CHEAT, "Polygon offset units", 0.0f, 0.0f);
	r_forceImagePrograms = CVar_Register("r_forceImagePrograms", "0", CVAR_BOOL, CVAR_CHEAT, "Force processing of image programs", 0, 0);
	r_writeImagePrograms = CVar_Register("r_writeImagePrograms", "0", CVAR_BOOL, CVAR_CHEAT, "Write final images to disk after processing image programs", 0, 0);	
	r_colorMipLevels = CVar_Register("r_colorMipLevels", "0", CVAR_BOOL, CVAR_CHEAT | CVAR_LATCH, "Color mip levels for testing mipmap usage", 0, 0);
	r_maxDebugPolygons = CVar_Register("r_maxDebugPolygons", "8192", CVAR_INTEGER, CVAR_CHEAT, "Maximum number of debug polygons", 0, 0);
	r_maxDebugLines = CVar_Register("r_maxDebugLines", "16384", CVAR_INTEGER, CVAR_CHEAT, "Maximum number of debug lines", 0, 0);
	r_maxDebugText = CVar_Register("r_maxDebugText", "512", CVAR_INTEGER, CVAR_CHEAT, "Maximum number of debug text strings", 0, 0);	
	r_singleMaterial = CVar_Register("r_singleMaterial", "0", CVAR_BOOL, CVAR_CHEAT | CVAR_LATCH, "Use a single default material on every surface", 0, 0);
	r_singleEntity = CVar_Register("r_singleEntity", "-1", CVAR_INTEGER, CVAR_CHEAT, "Only draw the specified entity", -1, MAX_RENDER_ENTITIES - 1);
	r_singleLight = CVar_Register("r_singleLight", "-1", CVAR_INTEGER, CVAR_CHEAT, "Only draw the specified light", -1, MAX_RENDER_LIGHTS - 1);
	r_showCluster = CVar_Register("r_showCluster", "0", CVAR_BOOL, CVAR_CHEAT, "Show the current view cluster", 0, 0);
	r_showFarClip = CVar_Register("r_showFarClip", "0", CVAR_BOOL, CVAR_CHEAT, "Show the calculated far clip plane distance", 0, 0);
	r_showCull = CVar_Register("r_showCull", "0", CVAR_BOOL, CVAR_CHEAT, "Show culling statistics", 0, 0);
	r_showScene = CVar_Register("r_showScene", "0", CVAR_BOOL, CVAR_CHEAT, "Show number of entities, lights, particles, and decals in view", 0, 0);
	r_showSurfaces = CVar_Register("r_showSurfaces", "0", CVAR_BOOL, CVAR_CHEAT, "Show number of surfaces in view", 0, 0);
	r_showDeforms = CVar_Register("r_showDeforms", "0", CVAR_BOOL, CVAR_CHEAT, "Show material deform statistics", 0, 0);
	r_showTextureUsage = CVar_Register("r_showTextureUsage", "0", CVAR_BOOL, CVAR_CHEAT, "Show texture memory usage", 0, 0);
	r_showTextures = CVar_Register("r_showTextures", "0", CVAR_INTEGER, CVAR_CHEAT, "Draw all textures instead of rendering (2 = draw in proportional size)", 0, 2);
	r_showDepth = CVar_Register("r_showDepth", "0", CVAR_BOOL, CVAR_CHEAT, "Draw the contents of the depth buffer", 0, 0);
	r_showLightCount = CVar_Register("r_showLightCount", "0", CVAR_INTEGER, CVAR_CHEAT, "Draw triangles colored by light count (1 = count visible ones, 2 = count everything through walls)", 0, 2);
	r_showLightVolumes = CVar_Register("r_showLightVolumes", "0", CVAR_INTEGER, CVAR_CHEAT, "Draw light volumes (1 = draw planes, 2 = draw edges, 3 = draw planes and edges)", 0, 3);
	r_showLightScissors = CVar_Register("r_showLightScissors", "0", CVAR_BOOL, CVAR_CHEAT, "Draw light scissor rectangles", 0, 0);
	r_showShadowTris = CVar_Register("r_showShadowTris", "0", CVAR_INTEGER, CVAR_CHEAT, "Draw shadows in wireframe mode (1 = draw visible ones, 2 = draw everything through walls)", 0, 2);
	r_showShadowVolumes = CVar_Register("r_showShadowVolumes", "0", CVAR_BOOL, CVAR_CHEAT, "Draw shadow planes", 0, 0);
	r_showShadowSilhouettes = CVar_Register("r_showShadowSilhouettes", "0", CVAR_BOOL, CVAR_CHEAT, "Draw shadow silhouettes", 0, 0);	
	r_showVertexColors = CVar_Register("r_showVertexColors", "0", CVAR_BOOL, CVAR_CHEAT, "Draw triangles colored by vertex colors", 0, 0);
	r_showTextureCoords = CVar_Register("r_showTextureCoords", "0", CVAR_BOOL, CVAR_CHEAT, "Draw triangles colored by texture coords", 0, 0);
	r_showTangentSpace = CVar_Register("r_showTangentSpace", "0", CVAR_INTEGER, CVAR_CHEAT, "Draw triangles colored by tangent space (1 = 1st tangent, 2 = 2nd tangent, 3 = normal)", 0, 3);
	r_showTris = CVar_Register("r_showTris", "0", CVAR_INTEGER, CVAR_CHEAT, "Draw triangle outlines (1 = draw visible ones, 2 = draw everything through walls)", 0, 2);
	r_showNormals = CVar_Register("r_showNormals", "0.0", CVAR_FLOAT, CVAR_CHEAT, "Draw vertex normals", 0.0f, 10.0f);
	r_showTextureVectors = CVar_Register("r_showTextureVectors", "0.0", CVAR_FLOAT, CVAR_CHEAT, "Draw texture (tangent) vectors", 0.0f, 10.0f);
	r_showBatchSize = CVar_Register("r_showBatchSize", "0", CVAR_INTEGER, CVAR_CHEAT, "Draw triangles colored by batch size (1 = draw visible ones, 2 = draw everything through walls)", 0, 2);
	r_showModelBounds = CVar_Register("r_showModelBounds", "0", CVAR_BOOL, CVAR_CHEAT, "Draw model bounds", 0, 0);
	r_skipVisibility = CVar_Register("r_skipVisibility", "0", CVAR_BOOL, CVAR_CHEAT, "Skip visibility determination tests", 0, 0);
	r_skipSuppress = CVar_Register("r_skipSuppress", "0", CVAR_BOOL, CVAR_CHEAT, "Skip per-view suppressions", 0, 0);
	r_skipCulling = CVar_Register("r_skipCulling", "0", CVAR_BOOL, CVAR_CHEAT, "Skip culling", 0, 0);
	r_skipFaceCulling = CVar_Register("r_skipFaceCulling", "0", CVAR_BOOL, CVAR_CHEAT, "Skip face culling", 0, 0);
	r_skipEntityCulling = CVar_Register("r_skipEntityCulling", "0", CVAR_BOOL, CVAR_CHEAT, "Skip entity culling", 0, 0);
	r_skipLightCulling = CVar_Register("r_skipLightCulling", "0", CVAR_BOOL, CVAR_CHEAT, "Skip light culling", 0, 0);
	r_skipScissors = CVar_Register("r_skipScissors", "0", CVAR_BOOL, CVAR_CHEAT, "Skip scissor testing", 0, 0);
	r_skipLightScissors = CVar_Register("r_skipLightScissors", "0", CVAR_BOOL, CVAR_CHEAT, "Skip scissor testing for lights", 0, 0);
	r_skipSorting = CVar_Register("r_skipSorting", "0", CVAR_BOOL, CVAR_CHEAT, "Skip surface sorting", 0, 0);
	r_skipEntities = CVar_Register("r_skipEntities", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering entities", 0, 0);
	r_skipLights = CVar_Register("r_skipLights", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering lights", 0, 0);
	r_skipParticles = CVar_Register("r_skipParticles", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering particles", 0, 0);
	r_skipDecals = CVar_Register("r_skipDecals", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering decals", 0, 0);
	r_skipExpressions = CVar_Register("r_skipExpressions", "0", CVAR_BOOL, CVAR_CHEAT, "Skip expression evaluation in materials, making everything static", 0, 0);
	r_skipConstantExpressions = CVar_Register("r_skipConstantExpressions", "0", CVAR_BOOL, CVAR_CHEAT, "Skip constant expressions in materials, re-evaluating everything each frame", 0, 0);	
	r_skipLightCache = CVar_Register("r_skipLightCache", "0", CVAR_BOOL, CVAR_CHEAT, "Skip precached shadow and interaction lists and generate them dynamically", 0, 0);
	r_skipDeforms = CVar_Register("r_skipDeforms", "0", CVAR_BOOL, CVAR_CHEAT, "Skip material deforms", 0, 0);
	r_skipAmbient = CVar_Register("r_skipAmbient", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering ambient stages", 0, 0);
	r_skipBump = CVar_Register("r_skipBump", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering bump stages", 0, 0);
	r_skipDiffuse = CVar_Register("r_skipDiffuse", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering diffuse stages", 0, 0);
	r_skipSpecular = CVar_Register("r_skipSpecular", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering specular stages", 0, 0);	
	r_skipShadows = CVar_Register("r_skipShadows", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering shadows", 0, 0);
	r_skipInteractions = CVar_Register("r_skipInteractions", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering light-surface interactions", 0, 0);
	r_skipAmbientLights = CVar_Register("r_skipAmbientLights", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering ambient lights", 0, 0);
	r_skipBlendLights = CVar_Register("r_skipBlendLights", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering blend lights", 0, 0);
	r_skipFogLights = CVar_Register("r_skipFogLights", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering fog lights", 0, 0);	
	r_skipTranslucent = CVar_Register("r_skipTranslucent", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering translucent materials", 0, 0);
	r_skipPostProcess = CVar_Register("r_skipPostProcess", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering post-process materials", 0, 0);	
	r_skipShaders = CVar_Register("r_skipShaders", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering custom shaders", 0, 0);
	r_skipCopyToTextures = CVar_Register("r_skipCopyToTextures", "0", CVAR_BOOL, CVAR_CHEAT, "Skip copying the framebuffer to textures", 0, 0);
	r_skipDynamicTextures = CVar_Register("r_skipDynamicTextures", "0", CVAR_BOOL, CVAR_CHEAT, "Skip updating dynamically generated textures", 0, 0);	
	r_skipDrawElements = CVar_Register("r_skipDrawElements", "0", CVAR_BOOL, CVAR_CHEAT, "Skip drawing primitives", 0, 0);
	r_skipRender = CVar_Register("r_skipRender", "0", CVAR_BOOL, CVAR_CHEAT, "Skip rendering 3D views", 0, 0);
	r_skipRenderContext = CVar_Register("r_skipRenderContext", "0", CVAR_BOOL, CVAR_CHEAT, "Skip all GL calls for testing CPU performance", 0, 0);	
	r_skipFrontEnd = CVar_Register("r_skipFrontEnd", "0", CVAR_BOOL, CVAR_CHEAT, "Skip all front-end work", 0, 0);
	r_skipBackEnd = CVar_Register("r_skipBackEnd", "0", CVAR_BOOL, CVAR_CHEAT, "Skip all back-end work", 0, 0);
	r_glDriver = CVar_Register("r_glDriver", "", CVAR_STRING, CVAR_ARCHIVE | CVAR_LATCH, "GL driver", 0, 0);
	r_mode = CVar_Register("r_mode", "0", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Video mode index (-1 = custom)", -1, NUM_VIDEO_MODES - 1);
	r_fullscreen = CVar_Register("r_fullscreen", "1", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Enable fullscreen video mode", 0, 0);
	r_customWidth = CVar_Register("r_customWidth", "720", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Custom screen width", 256, 65536);
	r_customHeight = CVar_Register("r_customHeight", "480", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Custom screen height", 256, 65536);
	r_displayRefresh = CVar_Register("r_displayRefresh", "0", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Display refresh rate (0 = default)", 0, 1000);
	r_multiSamples = CVar_Register("r_multiSamples", "0", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Number of samples for multisample antialiasing", 0, 0);	
	r_alphaToCoverage = CVar_Register("r_alphaToCoverage", "0", CVAR_BOOL, CVAR_ARCHIVE, "Sample alpha to coverage when multisampling", 0, 0);
	r_swapInterval = CVar_Register("r_swapInterval", "0", CVAR_INTEGER, CVAR_ARCHIVE, "Synchronize buffer swaps with the display's refresh (1 = v-sync, 2 = tear when needed)", 0, 2);
	r_finish = CVar_Register("r_finish", "0", CVAR_BOOL, CVAR_ARCHIVE, "Synchronize CPU and GPU every frame", 0, 0);
	r_gamma = CVar_Register("r_gamma", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Adjust display gamma", 0.5f, 3.0f);
	r_contrast = CVar_Register("r_contrast", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Adjust display contrast", 0.5f, 2.0f);
	r_brightness = CVar_Register("r_brightness", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Adjust display brightness", 0.5f, 2.0f);
	r_indexBuffers = CVar_Register("r_indexBuffers", "2", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Store geometry in index buffers (1 = static geometry, 2 = also dynamic geometry)", 0, 2);
	r_vertexBuffers = CVar_Register("r_vertexBuffers", "2", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Store geometry in vertex buffers (1 = static geometry, 2 = also dynamic geometry)", 0, 2);
	r_shaderQuality = CVar_Register("r_shaderQuality", "1", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Shader quality (0 = low, 1 = medium, 2 = high)", 0, 2);
	r_lightScale = CVar_Register("r_lightScale", "2.0", CVAR_FLOAT, CVAR_ARCHIVE, "Light intensity scale factor", 0.5f, 5.0f);
	r_lightDetailLevel = CVar_Register("r_lightDetailLevel", "1", CVAR_INTEGER, CVAR_ARCHIVE, "Light detail level (0 = low, 1 = medium, 2 = high)", 0, 2);
	r_shadows = CVar_Register("r_shadows", "1", CVAR_BOOL, CVAR_ARCHIVE, "Render stencil shadows", 0, 0);
	r_playerShadow = CVar_Register("r_playerShadow", "0", CVAR_BOOL, CVAR_ARCHIVE, "Render stencil shadows for the player", 0, 0);
	r_dynamicLights = CVar_Register("r_dynamicLights", "1", CVAR_BOOL, CVAR_ARCHIVE, "Render dynamic lights", 0, 0);	
	r_modulate = CVar_Register("r_modulate", "1.0", CVAR_FLOAT, CVAR_ARCHIVE | CVAR_LATCH, "Modulates lightmap colors", 0.0f, 1.0f);
	r_caustics = CVar_Register("r_caustics", "1", CVAR_BOOL, CVAR_ARCHIVE, "Render underwater caustics", 0, 0);
	r_seamlessCubeMaps = CVar_Register("r_seamlessCubeMaps", "0", CVAR_BOOL, CVAR_ARCHIVE, "Sample multiple faces from cube map textures", 0, 0);
	r_inGameVideos = CVar_Register("r_inGameVideos", "1", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Play in-game videos", 0, 0);
	r_precompressedImages = CVar_Register("r_precompressedImages", "1", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Always load precompressed images if present", 0, 0);
	r_roundImagesDown = CVar_Register("r_roundImagesDown", "1", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Round images down to nearest power of two", 0, 0);
	r_mipLevel = CVar_Register("r_mipLevel", "1", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Mip level for downsampled textures", 0, 4);
	r_mipLevelBump = CVar_Register("r_mipLevelBump", "1", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Mip level for downsampled bump map textures", 0, 4);
	r_mipLevelDiffuse = CVar_Register("r_mipLevelDiffuse", "1", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Mip level for downsampled diffuse map textures", 0, 4);
	r_mipLevelSpecular = CVar_Register("r_mipLevelSpecular", "1", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Mip level for downsampled specular map textures", 0, 4);	
	r_maxTextureSize = CVar_Register("r_maxTextureSize", "1024", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Maximum texture size", 256, 4096);
	r_compressTextures = CVar_Register("r_compressTextures", "1", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Compress textures", 0, 0);
	r_compressNormalTextures = CVar_Register("r_compressNormalTextures", "1", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Compress normal map textures", 0, 0);	
	r_textureFilter = CVar_Register("r_textureFilter", "GL_LINEAR_MIPMAP_LINEAR", CVAR_STRING, CVAR_ARCHIVE, "Filtering mode for mipmapped textures", 0, 0);
	r_textureLODBias = CVar_Register("r_textureLODBias", "0.0", CVAR_FLOAT, CVAR_ARCHIVE, "LOD bias for mipmapped textures", 0.0f, 0.0f);
	r_textureAnisotropy = CVar_Register("r_textureAnisotropy", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Anisotropic filtering level for mipmapped textures", 0.0f, 0.0f);
	
	// Add commands
	Cmd_AddCommand("listVideoModes", R_ListVideoModes_f, "Lists video modes", NULL);
	Cmd_AddCommand("gfxInfo", R_GfxInfo_f, "Shows graphics information", NULL);
}

/*
 ==================
 R_Unregister
 ==================
*/
static void R_Unregister (){

	// Remove commands
	Cmd_RemoveCommand("listVideoModes");
	Cmd_RemoveCommand("gfxInfo");
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_Init
 ==================
*/
void R_Init (bool all){

	Com_Printf("-------- Renderer Initialization --------\n");

	if (all){
		// Register commands and variables
		R_Register();

		// Initialize OpenGL subsystem
		GLImp_Init();
	}
	
	// Clear renderer variables
	R_ClearRender();

	// Set default GL state
	GL_SetDefaultState();

	// Build gamma table and set device gamma ramp
	R_SetGamma();

	// Allocate light mesh interactions
	R_AllocLightMeshes();

	// Allocate mesh and light lists
	R_AllocMeshes();
	R_AllocLights();

	// Initialize all the renderer modules
	R_InitImages();
	R_InitTextures();
	R_InitShaders();
	R_InitPrograms();
	R_InitMaterials();
	R_InitFonts();
	R_InitArrayBuffers();
	R_InitModels();
	R_InitLightEditor();

	RB_InitBackEnd();

	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	Com_Printf("-----------------------------------------\n");
}

/*
 ==================
 R_Shutdown
 ==================
*/
void R_Shutdown (bool all){

	if (!glConfig.initialized)
		return;

	// Shutdown all the renderer modules
	RB_ShutdownBackEnd();

	R_ShutdownLightEditor();
	R_ShutdownModels();
	R_ShutdownArrayBuffers();
	R_ShutdownFonts();
	R_ShutdownMaterials();
	R_ShutdownPrograms();
	R_ShutdownShaders();
	R_ShutdownTextures();
	R_ShutdownImages();

	if (all){
		// Unregister commands
		R_Unregister();

		// Shutdown OpenGL subsystem
		GLImp_Shutdown();
	}

	// Free all renderer allocations
	Mem_FreeAll(TAG_RENDERER, false);

	Mem_Fill(&rg, 0, sizeof(reGlobals_t));
}