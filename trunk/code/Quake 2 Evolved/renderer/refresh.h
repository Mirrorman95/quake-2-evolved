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
// refresh.h - Public renderer file
//


#ifndef __REFRESH_H__
#define __REFRESH_H__


// Virtual screen dimensions
#define SCREEN_WIDTH				640
#define SCREEN_HEIGHT				480

// Font support
#define FONTS_PER_SET				3
#define GLYPHS_PER_FONT				256

#define FONT_SMALL					0
#define FONT_MEDIUM					1
#define FONT_LARGE					2

// Scene limits
#define	MAX_RENDER_ENTITIES			1024
#define	MAX_RENDER_LIGHTS			32
#define MAX_RENDER_PARTICLES		8192

// Material parms
#define MAX_MATERIAL_PARMS			8

#define MATERIALPARM_RED			0			// Used by "colored" materials
#define MATERIALPARM_GREEN			1			// Used by "colored" materials
#define MATERIALPARM_BLUE			2			// Used by "colored" materials
#define MATERIALPARM_ALPHA			3			// Used by "colored" materials
#define MATERIALPARM_TIMEOFFSET		4			// Render time offset
#define MATERIALPARM_DIVERSITY		5			// Random value for some effects
#define MATERIALPARM_MISC			6			// Miscellaneous value for some effects
#define MATERIALPARM_MODE			7			// For selecting which material stages to enable

// Render entity types
typedef enum {
	RE_MODEL,
	RE_SPRITE,
	RE_BEAM
} reType_t;

// Render light types
typedef enum {
	RL_POINT,
	RL_CUBIC,
	RL_PROJECTED,
	RL_DIRECTIONAL
} rlType_t;

// View types for per-view allow/suppress
typedef enum {
	VIEW_NONE						= 0,
	VIEW_MAIN						= BIT(0),
	VIEW_SKY						= BIT(1),
	VIEW_MIRROR						= BIT(2),
	VIEW_REMOTE						= BIT(3),
	VIEW_ALL						= BIT(4) - 1
} viewType_t;

// Horizontal adjustment for 2D drawing
typedef enum {
	H_NONE,
	H_SCALE,
	H_ALIGN_LEFT,
	H_ALIGN_CENTER,
	H_ALIGN_RIGHT,
	H_STRETCH_LEFT,
	H_STRETCH_WIDTH,
	H_STRETCH_RIGHT
} horzAdjust_t;

// Vertical adjustment for 2D drawing
typedef enum {
	V_NONE,
	V_SCALE,
	V_ALIGN_TOP,
	V_ALIGN_CENTER,
	V_ALIGN_BOTTOM,
	V_STRETCH_TOP,
	V_STRETCH_HEIGHT,
	V_STRETCH_BOTTOM
} vertAdjust_t;

typedef enum {
	GLHW_GENERIC,
	GLHW_ATI,
	GLHW_NVIDIA
} glHardwareType_t;

typedef struct sound_s				sound_t;
typedef struct model_s				model_t;
typedef struct material_s			material_t;

typedef struct {
	int						image;				// Image index

	int						xAdjust;			// X adjustment
	int						yAdjust;			// Y adjustment

	int						width;				// Width of glyph
	int						height;				// Height of glyph

	float					s1;					// X offset in image where glyph starts
	float					t1;					// Y offset in image where glyph starts
	float					s2;					// X offset in image where glyph ends
	float					t2;					// Y offset in image where glyph ends

	material_t *			material;			// The material for the glyph
} glyphInfo_t;

typedef struct {
	int						maxWidth;
	int						maxHeight;

	int						glyphImages;
	float					glyphScale;

	glyphInfo_t				glyphs[GLYPHS_PER_FONT];
} fontInfo_t;

typedef struct {
	fontInfo_t				fontInfo[FONTS_PER_SET];
} fontSet_t;

typedef struct {
	reType_t				type;

	int						renderFX;

	int						index;				// Set by the renderer

	model_t *				model;

	int						viewCount;			// Set by the renderer

	// Transformation matrix
	vec3_t					origin;
	vec3_t					axis[3];

	// Previous data for lerping
	vec3_t					oldOrigin;
	int						oldFrame;

	float					backLerp;

	// Sprite specific
	bool					spriteOriented;
	float					spriteRadius;
	float					spriteRotation;

	// Beam specific
	vec3_t					beamEnd;
	float					beamWidth;
	float					beamLength;

	// Entity attributes
	bool					depthHack;			// Hack the depth range to avoid poking into geometry (implies noShadows)

	int						allowInView;		// For per-view allow/suppress
	int						allowShadowInView;	// For per-view allow/suppress

	int						skinIndex;

	int						frame;

	// Material
	material_t *			material;
	float					materialParms[MAX_MATERIAL_PARMS];
} renderEntity_t;

typedef struct {
	rlType_t				type;

	int						index;				// Set by the renderer

	int						lightNum;

	// Transformation matrix
	vec3_t					origin;
	vec3_t					center;				// Offset relative to origin (also gives the direction to the light for directional lights)
	vec3_t					axis[3];

	// Bounding volume for point, cubic, and directional lights
	vec3_t					radius;

	// Frustum definition for projected lights
	float					xMin;
	float					xMax;

	float					yMin;
	float					yMax;

	float					zNear;
	float					zFar;

	// Shadowing parameters
	bool					noShadows;			// Used to override material settings

	// Fogging parameters
	float					fogDistance;
	float					fogHeight;

	// Light attributes
	int						detailLevel;
	int						style;

	int						allowInView;		// For per-view allow/suppress

	// Material
	material_t *			material;
	float					materialParms[MAX_MATERIAL_PARMS];
} renderLight_t;

typedef struct {
	material_t *			material;
	vec3_t					origin;
	vec3_t					oldOrigin;
	float					radius;
	float					length;
	float					rotation;
	color_t					modulate;
} renderParticle_t;

typedef struct {
	vec3_t					rgb;				// 0.0 - 2.0
	float					white;				// Highest of RGB
} lightStyle_t;

typedef struct {
	vec3_t					origin;
	vec3_t					axis[3];
} tag_t;

typedef struct {
	// Viewport
	int						x;
	int						y;
	int						width;
	int						height;

	// Horizontal adjustment
	horzAdjust_t			horzAdjust;
	float					horzPercent;

	// Vertical adjustment
	vertAdjust_t			vertAdjust;
	float					vertPercent;

	// Transformation matrix
	vec3_t					origin;
	vec3_t					axis[3];

	// Field of view
	float					fovX;
	float					fovY;
	float					fovScale;

	// Time in milliseconds for time dependent effects
	float					time;

	// Material parms
	float					materialParms[MAX_MATERIAL_PARMS];

	// If not NULL, only areas with set bits will be drawn
	byte *					areaBits;
} renderView_t;

typedef struct {
	bool					initialized;

	glHardwareType_t		hardwareType;

	const char *			vendorString;
	const char *			rendererString;
	const char *			versionString;
	const char *			extensionsString;
	const char *			wglExtensionsString;

	const char *			shadingLanguageVersionString;

	bool					textureCompressionS3TCAvailable;
	bool					textureFilterAnisotropicAvailable;
	bool					swapControlAvailable;
	bool					swapControlTearAvailable;
	bool					stencilWrapAvailable;
	bool					stencilTwoSideAvailable;
	bool					atiSeparateStencilAvailable;

	int						maxTextureSize;
	int						max3DTextureSize;
	int						maxCubeMapTextureSize;
	int						maxArrayTextureLayers;
	int						maxTextureUnits;
	int						maxTextureCoords;
	int						maxTextureImageUnits;
	int						maxVertexTextureImageUnits;
	int						maxCombinedTextureImageUnits;
	int						maxVertexAttribs;
	int						maxVaryingComponents;
	int						maxVertexUniformComponents;
	int						maxFragmentUniformComponents;
	int						maxColorAttachments;
	int						maxRenderbufferSize;
	float					maxTextureLODBias;
	float					maxTextureMaxAnisotropy;

	int						videoWidth;
	int						videoHeight;

	bool					isFullscreen;
	int						displayFrequency;

	int						colorBits;
	int						alphaBits;
	int						depthBits;
	int						stencilBits;

	int						multiSamples;
} glConfig_t;

// Loads and prepares the given map for rendering
void			R_LoadMap (const char *name, const char *skyName, float skyRotate, const vec3_t skyAxis);

// Loads and prepares lights for the given map
void			R_LoadLights (const char *name);

// Loads and registers the given model
model_t *		R_RegisterModel (const char *name);

// Loads and registers the given material
material_t *	R_RegisterMaterial (const char *name, bool lightingDefault);
material_t *	R_RegisterMaterialLight (const char *name);
material_t *	R_RegisterMaterialNoMip (const char *name);

// Loads and registers the given font
fontSet_t *		R_RegisterFont (const char *name);

// Adds entities to the scene for rendering
void			R_AddEntityToScene (const renderEntity_t *renderEntity);

// Adds lights to the scene for rendering
void			R_AddLightToScene (const renderLight_t *renderLight);

// Adds particles to the scene for rendering
void			R_AddParticleToScene (const renderParticle_t *renderParticle);

// Generates lists of static shadows and interactions and precaches the given
// lights.
// If this isn't called after loading a map, all the world shadows and
// interactions will be dynamically generated.
void			R_PrecacheLights (int numRenderLights, const renderLight_t *renderLights);

// Renders all scene objects.
// If primaryView is false, the world map will be ignored completely.
void			R_RenderScene (const renderView_t *renderView, bool primaryView);

// Clear all scene objects
void			R_ClearScene ();

// Projects a decal onto the world
void			R_ProjectDecalOntoWorld (const vec3_t origin, const vec3_t direction, float rotation, float radius, int startTime, material_t *material);

// Captures the render output to the given texture.
// Returns false if the texture wasn't found or couldn't be dynamically
// modified.
bool			R_CaptureRenderToTexture (const char *name);

// Updates the given texture's image. The image data must be in RGBA format.
// Does not allocate memory, so the image pointer must remain valid for the
// rest of the frame.
// Returns false if the texture wasn't found or couldn't be dynamically
// modified.
bool			R_UpdateTextureImage (const char *name, const byte *image, int width, int height);

// 2D drawing just involves color and material parameters setting and axial
// image subsections.
// The horizontal / vertical adjustment parameters are used to control the
// automatic coordinate scaling and aspect ratio correction on each axis
// separately.
void			R_SetColor (const vec4_t rgba);
void			R_SetColor1 (float l);
void			R_SetColor2 (float l, float a);
void			R_SetColor3 (float r, float g, float b);
void			R_SetColor4 (float r, float g, float b, float a);

void			R_SetParameter (int index, float value);
void			R_SetParameters (const float parms[MAX_MATERIAL_PARMS]);

void			R_DrawStretchPic (float x, float y, float w, float h, float s1, float t1, float s2, float t2, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, material_t *material);
void			R_DrawStretchPicEx (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float xShear, float yShear, float rotate, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, material_t *material);

void			R_DrawChar (float x, float y, float w, float h, int c, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, material_t *material);
void			R_DrawString (float x, float y, float w, float h, const char *string, const vec4_t color, bool forceColor, float xShadow, float yShadow, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, material_t *material);

// Cinematics can preserve their original aspect ratio, automatically
// letterboxing or pillarboxing if needed.
// The horizontal / vertical adjustment parameters are used to control the
// automatic coordinate scaling and aspect ratio correction on each axis
// separately.
// Does not allocate memory, so the image pointer must remain valid for the
// rest of the frame.
void			R_DrawCinematic (int x, int y, int w, int h, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, int handle, const byte *image, bool dirty, int width, int height, bool keepAspect);

// The render output can be cropped down to a subset of the real screen.
// To render to a texture, first set the crop size, then perform all desired
// rendering, then capture to a texture.
// The specified dimensions are in virtual SCREEN_WIDTH x SCREEN_HEIGHT
// coordinates unless forceDimensions is true.
// Users of the renderer will not know the actual pixel size of the area they
// are rendering to.
void			R_CropRenderSize (int width, int height, bool forceDimensions);
void			R_UnCropRenderSize ();

// A frame can consist of 2D drawing and potentially multiple 3D scenes
void			R_BeginFrame (int time);
void			R_EndFrame ();

// Returns the number of frames that the model has
int				R_ModelFrames (model_t *model);

// Returns the model bounds
void			R_ModelBounds (model_t *model, vec3_t mins, vec3_t maxs);

// Interpolates the model by tag
bool			R_LerpTag (tag_t *tag, model_t *model, int curFrame, int oldFrame, float backLerp, const char *tagName);

// Enumerates material definitions using a callback
void			R_EnumMaterialDefs (void (*callback)(const char *name));

// Sets light style parameters
void			R_SetLightStyle (int style, float r, float g, float b);

// Returns the proper texture size
void			R_GetPicSize (material_t *material, float *w, float *h);

// Returns information about the OpenGL subsystem
glConfig_t		R_GetGLConfig ();

// This is called by the system when the main window gains or loses focus
void			R_Activate (bool active);

// Initializes the renderer subsystem.
// If all is false, the OpenGL subsystem will not be initialized, which is
// useful for flushing all data and resetting the state.
void			R_Init (bool all);

// Shuts down the renderer subsystem.
// If all is false, the OpenGL subsystem will not be shut down, which is useful
// for flushing all data and resetting the state.
void			R_Shutdown (bool all);


#endif // __REFRESH_H__