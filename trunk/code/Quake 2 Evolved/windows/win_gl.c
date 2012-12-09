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
// win_gl.c - Windows-specific OpenGL code
//


#include "../renderer/r_local.h"
#include "win_local.h"


#define WINDOW_NAME					ENGINE_NAME
#define	WINDOW_CLASS_MAIN			ENGINE_NAME " Main"
#define	WINDOW_CLASS_FAKE			ENGINE_NAME " Fake"
#define WINDOW_STYLE_FULLSCREEN		(WS_POPUP)
#define WINDOW_STYLE_WINDOWED		(WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

typedef struct {
	DEVMODE					devMode;

	HWND					hWnd;
	HDC						hDC;
	HGLRC					hGLRC;

	HWND					hWndFake;
	HDC						hDCFake;
	HGLRC					hGLRCFake;

	int						desktopWidth;
	int						desktopHeight;
	int						desktopBitDepth;

	BOOL					supportsGamma;

	WORD					oldGammaRamp[3][256];
	WORD					newGammaRamp[3][256];
} glwState_t;

static glwState_t			glwState;

static const char *			(WINAPI * qwglGetExtensionsStringARB)(HDC hDC);

static BOOL					(WINAPI * qwglGetPixelFormatAttribivARB)(HDC hDC, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
static BOOL					(WINAPI * qwglGetPixelFormatAttribfvARB)(HDC hDC, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
static BOOL					(WINAPI * qwglChoosePixelFormatARB)(HDC hDC, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);


/*
 ==============================================================================

 EXTENSIONS

 ==============================================================================
*/


/*
 ==================
 GLW_IsExtensionPresent
 ==================
*/
static bool GLW_IsExtensionPresent (const char *extString, const char *extName){

	int		length;

	length = Str_Length(extName);

	while (*extString){
		if (extString[length] == '\0' || Str_CharIsSpace(extString[length])){
			if (!Str_ICompareChars(extString, extName, length))
				return true;
		}

		while (*extString){
			if (Str_CharIsSpace(*extString))
				break;

			extString++;
		}

		while (*extString){
			if (!Str_CharIsSpace(*extString))
				break;

			extString++;
		}
	}

	return false;
}

/*
 ==================
 GLW_GetProcAddress
 ==================
*/
static void *GLW_GetProcAddress (const char *procName){

	void	*procAddress;

	procAddress = qwglGetProcAddress(procName);
	if (!procAddress){
		GLW_Shutdown();

		Com_Error(ERR_FATAL, "GLW_GetProcAddress: wglGetProcAddress() failed for '%s'", procName);
	}

	return procAddress;
}

/*
 ==================
 GLW_InitExtensions
 ==================
*/
static void GLW_InitExtensions (void){

	Com_Printf("Initializing OpenGL extensions\n");

	// We must treat these as extensions, because on Windows we're still forced
	// to deal with the old OpenGL 1.1 core
	qglActiveTexture						= (GLACTIVETEXTURE)GLW_GetProcAddress("glActiveTexture");
	qglAttachShader							= (GLATTACHSHADER)GLW_GetProcAddress("glAttachShader");
	qglBeginConditionalRender				= (GLBEGINCONDITIONALRENDER)GLW_GetProcAddress("glBeginConditionalRender");
	qglBeginQuery							= (GLBEGINQUERY)GLW_GetProcAddress("glBeginQuery");
	qglBeginTransformFeedback				= (GLBEGINTRANSFORMFEEDBACK)GLW_GetProcAddress("glBeginTransformFeedback");
	qglBindAttribLocation					= (GLBINDATTRIBLOCATION)GLW_GetProcAddress("glBindAttribLocation");
	qglBindBuffer							= (GLBINDBUFFER)GLW_GetProcAddress("glBindBuffer");
	qglBindBufferBase						= (GLBINDBUFFERBASE)GLW_GetProcAddress("glBindBufferBase");
	qglBindBufferRange						= (GLBINDBUFFERRANGE)GLW_GetProcAddress("glBindBufferRange");
	qglBindFragDataLocation					= (GLBINDFRAGDATALOCATION)GLW_GetProcAddress("glBindFragDataLocation");
	qglBindFragDataLocationIndexed			= (GLBINDFRAGDATALOCATIONINDEXED)GLW_GetProcAddress("glBindFragDataLocationIndexed");
	qglBindFramebuffer						= (GLBINDFRAMEBUFFER)GLW_GetProcAddress("glBindFramebuffer");
	qglBindRenderbuffer						= (GLBINDRENDERBUFFER)GLW_GetProcAddress("glBindRenderbuffer");
	qglBindSampler							= (GLBINDSAMPLER)GLW_GetProcAddress("glBindSampler");
	qglBindVertexArray						= (GLBINDVERTEXARRAY)GLW_GetProcAddress("glBindVertexArray");
	qglBlendColor							= (GLBLENDCOLOR)GLW_GetProcAddress("glBlendColor");
	qglBlendEquation						= (GLBLENDEQUATION)GLW_GetProcAddress("glBlendEquation");
	qglBlendEquationSeparate				= (GLBLENDEQUATIONSEPARATE)GLW_GetProcAddress("glBlendEquationSeparate");
	qglBlendFuncSeparate					= (GLBLENDFUNCSEPARATE)GLW_GetProcAddress("glBlendFuncSeparate");
	qglBlitFramebuffer						= (GLBLITFRAMEBUFFER)GLW_GetProcAddress("glBlitFramebuffer");
	qglBufferData							= (GLBUFFERDATA)GLW_GetProcAddress("glBufferData");
	qglBufferSubData						= (GLBUFFERSUBDATA)GLW_GetProcAddress("glBufferSubData");
	qglCheckFramebufferStatus				= (GLCHECKFRAMEBUFFERSTATUS)GLW_GetProcAddress("glCheckFramebufferStatus");
	qglClampColor							= (GLCLAMPCOLOR)GLW_GetProcAddress("glClampColor");
	qglClearBufferfi						= (GLCLEARBUFFERFI)GLW_GetProcAddress("glClearBufferfi");
	qglClearBufferfv						= (GLCLEARBUFFERFV)GLW_GetProcAddress("glClearBufferfv");
	qglClearBufferiv						= (GLCLEARBUFFERIV)GLW_GetProcAddress("glClearBufferiv");
	qglClearBufferuiv						= (GLCLEARBUFFERUIV)GLW_GetProcAddress("glClearBufferuiv");
	qglClientActiveTexture					= (GLCLIENTACTIVETEXTURE)GLW_GetProcAddress("glClientActiveTexture");
	qglClientWaitSync						= (GLCLIENTWAITSYNC)GLW_GetProcAddress("glClientWaitSync");
	qglColorMaski							= (GLCOLORMASKI)GLW_GetProcAddress("glColorMaski");
	qglColorP3ui							= (GLCOLORP3UI)GLW_GetProcAddress("glColorP3ui");
	qglColorP3uiv							= (GLCOLORP3UIV)GLW_GetProcAddress("glColorP3uiv");
	qglColorP4ui							= (GLCOLORP4UI)GLW_GetProcAddress("glColorP4ui");
	qglColorP4uiv							= (GLCOLORP4UIV)GLW_GetProcAddress("glColorP4uiv");
	qglCompileShader						= (GLCOMPILESHADER)GLW_GetProcAddress("glCompileShader");
	qglCompressedTexImage1D					= (GLCOMPRESSEDTEXIMAGE1D)GLW_GetProcAddress("glCompressedTexImage1D");
	qglCompressedTexImage2D					= (GLCOMPRESSEDTEXIMAGE2D)GLW_GetProcAddress("glCompressedTexImage2D");
	qglCompressedTexImage3D					= (GLCOMPRESSEDTEXIMAGE3D)GLW_GetProcAddress("glCompressedTexImage3D");
	qglCompressedTexSubImage1D				= (GLCOMPRESSEDTEXSUBIMAGE1D)GLW_GetProcAddress("glCompressedTexSubImage1D");
	qglCompressedTexSubImage2D				= (GLCOMPRESSEDTEXSUBIMAGE2D)GLW_GetProcAddress("glCompressedTexSubImage2D");
	qglCompressedTexSubImage3D				= (GLCOMPRESSEDTEXSUBIMAGE3D)GLW_GetProcAddress("glCompressedTexSubImage3D");
	qglCopyBufferSubData					= (GLCOPYBUFFERSUBDATA)GLW_GetProcAddress("glCopyBufferSubData");
	qglCopyTexSubImage3D					= (GLCOPYTEXSUBIMAGE3D)GLW_GetProcAddress("glCopyTexSubImage3D");
	qglCreateProgram						= (GLCREATEPROGRAM)GLW_GetProcAddress("glCreateProgram");
	qglCreateShader							= (GLCREATESHADER)GLW_GetProcAddress("glCreateShader");
	qglDeleteBuffers						= (GLDELETEBUFFERS)GLW_GetProcAddress("glDeleteBuffers");
	qglDeleteFramebuffers					= (GLDELETEFRAMEBUFFERS)GLW_GetProcAddress("glDeleteFramebuffers");
	qglDeleteProgram						= (GLDELETEPROGRAM)GLW_GetProcAddress("glDeleteProgram");
	qglDeleteQueries						= (GLDELETEQUERIES)GLW_GetProcAddress("glDeleteQueries");
	qglDeleteRenderbuffers					= (GLDELETERENDERBUFFERS)GLW_GetProcAddress("glDeleteRenderbuffers");
	qglDeleteSamplers						= (GLDELETESAMPLERS)GLW_GetProcAddress("glDeleteSamplers");
	qglDeleteShader							= (GLDELETESHADER)GLW_GetProcAddress("glDeleteShader");
	qglDeleteSync							= (GLDELETESYNC)GLW_GetProcAddress("glDeleteSync");
	qglDeleteVertexArrays					= (GLDELETEVERTEXARRAYS)GLW_GetProcAddress("glDeleteVertexArrays");
	qglDetachShader							= (GLDETACHSHADER)GLW_GetProcAddress("glDetachShader");
	qglDisableVertexAttribArray				= (GLDISABLEVERTEXATTRIBARRAY)GLW_GetProcAddress("glDisableVertexAttribArray");
	qglDisablei								= (GLDISABLEI)GLW_GetProcAddress("glDisablei");
	qglDrawArraysInstanced					= (GLDRAWARRAYSINSTANCED)GLW_GetProcAddress("glDrawArraysInstanced");
	qglDrawBuffers							= (GLDRAWBUFFERS)GLW_GetProcAddress("glDrawBuffers");
	qglDrawElementsBaseVertex				= (GLDRAWELEMENTSBASEVERTEX)GLW_GetProcAddress("glDrawElementsBaseVertex");
	qglDrawElementsInstanced				= (GLDRAWELEMENTSINSTANCED)GLW_GetProcAddress("glDrawElementsInstanced");
	qglDrawElementsInstancedBaseVertex		= (GLDRAWELEMENTSINSTANCEDBASEVERTEX)GLW_GetProcAddress("glDrawElementsInstancedBaseVertex");
	qglDrawRangeElements					= (GLDRAWRANGEELEMENTS)GLW_GetProcAddress("glDrawRangeElements");
	qglDrawRangeElementsBaseVertex			= (GLDRAWRANGEELEMENTSBASEVERTEX)GLW_GetProcAddress("glDrawRangeElementsBaseVertex");
	qglEnableVertexAttribArray				= (GLENABLEVERTEXATTRIBARRAY)GLW_GetProcAddress("glEnableVertexAttribArray");
	qglEnablei								= (GLENABLEI)GLW_GetProcAddress("glEnablei");
	qglEndConditionalRender					= (GLENDCONDITIONALRENDER)GLW_GetProcAddress("glEndConditionalRender");
	qglEndQuery								= (GLENDQUERY)GLW_GetProcAddress("glEndQuery");
	qglEndTransformFeedback					= (GLENDTRANSFORMFEEDBACK)GLW_GetProcAddress("glEndTransformFeedback");
	qglFenceSync							= (GLFENCESYNC)GLW_GetProcAddress("glFenceSync");
	qglFlushMappedBufferRange				= (GLFLUSHMAPPEDBUFFERRANGE)GLW_GetProcAddress("glFlushMappedBufferRange");
	qglFogCoordPointer						= (GLFOGCOORDPOINTER)GLW_GetProcAddress("glFogCoordPointer");
	qglFogCoordd							= (GLFOGCOORDD)GLW_GetProcAddress("glFogCoordd");
	qglFogCoorddv							= (GLFOGCOORDDV)GLW_GetProcAddress("glFogCoorddv");
	qglFogCoordf							= (GLFOGCOORDF)GLW_GetProcAddress("glFogCoordf");
	qglFogCoordfv							= (GLFOGCOORDFV)GLW_GetProcAddress("glFogCoordfv");
	qglFramebufferRenderbuffer				= (GLFRAMEBUFFERRENDERBUFFER)GLW_GetProcAddress("glFramebufferRenderbuffer");
	qglFramebufferTexture					= (GLFRAMEBUFFERTEXTURE)GLW_GetProcAddress("glFramebufferTexture");
	qglFramebufferTexture1D					= (GLFRAMEBUFFERTEXTURE1D)GLW_GetProcAddress("glFramebufferTexture1D");
	qglFramebufferTexture2D					= (GLFRAMEBUFFERTEXTURE2D)GLW_GetProcAddress("glFramebufferTexture2D");
	qglFramebufferTexture3D					= (GLFRAMEBUFFERTEXTURE3D)GLW_GetProcAddress("glFramebufferTexture3D");
	qglFramebufferTextureLayer				= (GLFRAMEBUFFERTEXTURELAYER)GLW_GetProcAddress("glFramebufferTextureLayer");
	qglGenBuffers							= (GLGENBUFFERS)GLW_GetProcAddress("glGenBuffers");
	qglGenFramebuffers						= (GLGENFRAMEBUFFERS)GLW_GetProcAddress("glGenFramebuffers");
	qglGenQueries							= (GLGENQUERIES)GLW_GetProcAddress("glGenQueries");
	qglGenRenderbuffers						= (GLGENRENDERBUFFERS)GLW_GetProcAddress("glGenRenderbuffers");
	qglGenSamplers							= (GLGENSAMPLERS)GLW_GetProcAddress("glGenSamplers");
	qglGenVertexArrays						= (GLGENVERTEXARRAYS)GLW_GetProcAddress("glGenVertexArrays");
	qglGenerateMipmap						= (GLGENERATEMIPMAP)GLW_GetProcAddress("glGenerateMipmap");
	qglGetActiveAttrib						= (GLGETACTIVEATTRIB)GLW_GetProcAddress("glGetActiveAttrib");
	qglGetActiveUniform						= (GLGETACTIVEUNIFORM)GLW_GetProcAddress("glGetActiveUniform");
	qglGetActiveUniformBlockName			= (GLGETACTIVEUNIFORMBLOCKNAME)GLW_GetProcAddress("glGetActiveUniformBlockName");
	qglGetActiveUniformBlockiv				= (GLGETACTIVEUNIFORMBLOCKIV)GLW_GetProcAddress("glGetActiveUniformBlockiv");
	qglGetActiveUniformName					= (GLGETACTIVEUNIFORMNAME)GLW_GetProcAddress("glGetActiveUniformName");
	qglGetActiveUniformsiv					= (GLGETACTIVEUNIFORMSIV)GLW_GetProcAddress("glGetActiveUniformsiv");
	qglGetAttachedShaders					= (GLGETATTACHEDSHADERS)GLW_GetProcAddress("glGetAttachedShaders");
	qglGetAttribLocation					= (GLGETATTRIBLOCATION)GLW_GetProcAddress("glGetAttribLocation");
	qglGetBooleani_v						= (GLGETBOOLEANI_V)GLW_GetProcAddress("glGetBooleani_v");
	qglGetBufferParameteri64v				= (GLGETBUFFERPARAMETERI64V)GLW_GetProcAddress("glGetBufferParameteri64v");
	qglGetBufferParameteriv					= (GLGETBUFFERPARAMETERIV)GLW_GetProcAddress("glGetBufferParameteriv");
	qglGetBufferPointerv					= (GLGETBUFFERPOINTERV)GLW_GetProcAddress("glGetBufferPointerv");
	qglGetBufferSubData						= (GLGETBUFFERSUBDATA)GLW_GetProcAddress("glGetBufferSubData");
	qglGetCompressedTexImage				= (GLGETCOMPRESSEDTEXIMAGE)GLW_GetProcAddress("glGetCompressedTexImage");
	qglGetFragDataIndex						= (GLGETFRAGDATAINDEX)GLW_GetProcAddress("glGetFragDataIndex");
	qglGetFragDataLocation					= (GLGETFRAGDATALOCATION)GLW_GetProcAddress("glGetFragDataLocation");
	qglGetFramebufferAttachmentParameteriv	= (GLGETFRAMEBUFFERATTACHMENTPARAMETERIV)GLW_GetProcAddress("glGetFramebufferAttachmentParameteriv");
	qglGetInteger64i_v						= (GLGETINTEGER64I_V)GLW_GetProcAddress("glGetInteger64i_v");
	qglGetInteger64v						= (GLGETINTEGER64V)GLW_GetProcAddress("glGetInteger64v");
	qglGetIntegeri_v						= (GLGETINTEGERI_V)GLW_GetProcAddress("glGetIntegeri_v");
	qglGetMultisamplefv						= (GLGETMULTISAMPLEFV)GLW_GetProcAddress("glGetMultisamplefv");
	qglGetProgramInfoLog					= (GLGETPROGRAMINFOLOG)GLW_GetProcAddress("glGetProgramInfoLog");
	qglGetProgramiv							= (GLGETPROGRAMIV)GLW_GetProcAddress("glGetProgramiv");
	qglGetQueryObjecti64v					= (GLGETQUERYOBJECTI64V)GLW_GetProcAddress("glGetQueryObjecti64v");
	qglGetQueryObjectiv						= (GLGETQUERYOBJECTIV)GLW_GetProcAddress("glGetQueryObjectiv");
	qglGetQueryObjectui64v					= (GLGETQUERYOBJECTUI64V)GLW_GetProcAddress("glGetQueryObjectui64v");
	qglGetQueryObjectuiv					= (GLGETQUERYOBJECTUIV)GLW_GetProcAddress("glGetQueryObjectuiv");
	qglGetQueryiv							= (GLGETQUERYIV)GLW_GetProcAddress("glGetQueryiv");
	qglGetRenderbufferParameteriv			= (GLGETRENDERBUFFERPARAMETERIV)GLW_GetProcAddress("glGetRenderbufferParameteriv");
	qglGetSamplerParameterIiv				= (GLGETSAMPLERPARAMETERIIV)GLW_GetProcAddress("glGetSamplerParameterIiv");
	qglGetSamplerParameterIuiv				= (GLGETSAMPLERPARAMETERIUIV)GLW_GetProcAddress("glGetSamplerParameterIuiv");
	qglGetSamplerParameterfv				= (GLGETSAMPLERPARAMETERFV)GLW_GetProcAddress("glGetSamplerParameterfv");
	qglGetSamplerParameteriv				= (GLGETSAMPLERPARAMETERIV)GLW_GetProcAddress("glGetSamplerParameteriv");
	qglGetShaderInfoLog						= (GLGETSHADERINFOLOG)GLW_GetProcAddress("glGetShaderInfoLog");
	qglGetShaderSource						= (GLGETSHADERSOURCE)GLW_GetProcAddress("glGetShaderSource");
	qglGetShaderiv							= (GLGETSHADERIV)GLW_GetProcAddress("glGetShaderiv");
	qglGetStringi							= (GLGETSTRINGI)GLW_GetProcAddress("glGetStringi");
	qglGetSynciv							= (GLGETSYNCIV)GLW_GetProcAddress("glGetSynciv");
	qglGetTexParameterIiv					= (GLGETTEXPARAMETERIIV)GLW_GetProcAddress("glGetTexParameterIiv");
	qglGetTexParameterIuiv					= (GLGETTEXPARAMETERIUIV)GLW_GetProcAddress("glGetTexParameterIuiv");
	qglGetTransformFeedbackVarying			= (GLGETTRANSFORMFEEDBACKVARYING)GLW_GetProcAddress("glGetTransformFeedbackVarying");
	qglGetUniformBlockIndex					= (GLGETUNIFORMBLOCKINDEX)GLW_GetProcAddress("glGetUniformBlockIndex");
	qglGetUniformIndices					= (GLGETUNIFORMINDICES)GLW_GetProcAddress("glGetUniformIndices");
	qglGetUniformLocation					= (GLGETUNIFORMLOCATION)GLW_GetProcAddress("glGetUniformLocation");
	qglGetUniformfv							= (GLGETUNIFORMFV)GLW_GetProcAddress("glGetUniformfv");
	qglGetUniformiv							= (GLGETUNIFORMIV)GLW_GetProcAddress("glGetUniformiv");
	qglGetUniformuiv						= (GLGETUNIFORMUIV)GLW_GetProcAddress("glGetUniformuiv");
	qglGetVertexAttribIiv					= (GLGETVERTEXATTRIBIIV)GLW_GetProcAddress("glGetVertexAttribIiv");
	qglGetVertexAttribIuiv					= (GLGETVERTEXATTRIBIUIV)GLW_GetProcAddress("glGetVertexAttribIuiv");
	qglGetVertexAttribPointerv				= (GLGETVERTEXATTRIBPOINTERV)GLW_GetProcAddress("glGetVertexAttribPointerv");
	qglGetVertexAttribdv					= (GLGETVERTEXATTRIBDV)GLW_GetProcAddress("glGetVertexAttribdv");
	qglGetVertexAttribfv					= (GLGETVERTEXATTRIBFV)GLW_GetProcAddress("glGetVertexAttribfv");
	qglGetVertexAttribiv					= (GLGETVERTEXATTRIBIV)GLW_GetProcAddress("glGetVertexAttribiv");
	qglIsBuffer								= (GLISBUFFER)GLW_GetProcAddress("glIsBuffer");
	qglIsEnabledi							= (GLISENABLEDI)GLW_GetProcAddress("glIsEnabledi");
	qglIsFramebuffer						= (GLISFRAMEBUFFER)GLW_GetProcAddress("glIsFramebuffer");
	qglIsProgram							= (GLISPROGRAM)GLW_GetProcAddress("glIsProgram");
	qglIsQuery								= (GLISQUERY)GLW_GetProcAddress("glIsQuery");
	qglIsRenderbuffer						= (GLISRENDERBUFFER)GLW_GetProcAddress("glIsRenderbuffer");
	qglIsSampler							= (GLISSAMPLER)GLW_GetProcAddress("glIsSampler");
	qglIsShader								= (GLISSHADER)GLW_GetProcAddress("glIsShader");
	qglIsSync								= (GLISSYNC)GLW_GetProcAddress("glIsSync");
	qglIsVertexArray						= (GLISVERTEXARRAY)GLW_GetProcAddress("glIsVertexArray");
	qglLinkProgram							= (GLLINKPROGRAM)GLW_GetProcAddress("glLinkProgram");
	qglLoadTransposeMatrixd					= (GLLOADTRANSPOSEMATRIXD)GLW_GetProcAddress("glLoadTransposeMatrixd");
	qglLoadTransposeMatrixf					= (GLLOADTRANSPOSEMATRIXF)GLW_GetProcAddress("glLoadTransposeMatrixf");
	qglMapBuffer							= (GLMAPBUFFER)GLW_GetProcAddress("glMapBuffer");
	qglMapBufferRange						= (GLMAPBUFFERRANGE)GLW_GetProcAddress("glMapBufferRange");
	qglMultTransposeMatrixd					= (GLMULTTRANSPOSEMATRIXD)GLW_GetProcAddress("glMultTransposeMatrixd");
	qglMultTransposeMatrixf					= (GLMULTTRANSPOSEMATRIXF)GLW_GetProcAddress("glMultTransposeMatrixf");
	qglMultiDrawArrays						= (GLMULTIDRAWARRAYS)GLW_GetProcAddress("glMultiDrawArrays");
	qglMultiDrawElements					= (GLMULTIDRAWELEMENTS)GLW_GetProcAddress("glMultiDrawElements");
	qglMultiDrawElementsBaseVertex			= (GLMULTIDRAWELEMENTSBASEVERTEX)GLW_GetProcAddress("glMultiDrawElementsBaseVertex");
	qglMultiTexCoord1d						= (GLMULTITEXCOORD1D)GLW_GetProcAddress("glMultiTexCoord1d");
	qglMultiTexCoord1dv						= (GLMULTITEXCOORD1DV)GLW_GetProcAddress("glMultiTexCoord1dv");
	qglMultiTexCoord1f						= (GLMULTITEXCOORD1F)GLW_GetProcAddress("glMultiTexCoord1f");
	qglMultiTexCoord1fv						= (GLMULTITEXCOORD1FV)GLW_GetProcAddress("glMultiTexCoord1fv");
	qglMultiTexCoord1i						= (GLMULTITEXCOORD1I)GLW_GetProcAddress("glMultiTexCoord1i");
	qglMultiTexCoord1iv						= (GLMULTITEXCOORD1IV)GLW_GetProcAddress("glMultiTexCoord1iv");
	qglMultiTexCoord1s						= (GLMULTITEXCOORD1S)GLW_GetProcAddress("glMultiTexCoord1s");
	qglMultiTexCoord1sv						= (GLMULTITEXCOORD1SV)GLW_GetProcAddress("glMultiTexCoord1sv");
	qglMultiTexCoord2d						= (GLMULTITEXCOORD2D)GLW_GetProcAddress("glMultiTexCoord2d");
	qglMultiTexCoord2dv						= (GLMULTITEXCOORD2DV)GLW_GetProcAddress("glMultiTexCoord2dv");
	qglMultiTexCoord2f						= (GLMULTITEXCOORD2F)GLW_GetProcAddress("glMultiTexCoord2f");
	qglMultiTexCoord2fv						= (GLMULTITEXCOORD2FV)GLW_GetProcAddress("glMultiTexCoord2fv");
	qglMultiTexCoord2i						= (GLMULTITEXCOORD2I)GLW_GetProcAddress("glMultiTexCoord2i");
	qglMultiTexCoord2iv						= (GLMULTITEXCOORD2IV)GLW_GetProcAddress("glMultiTexCoord2iv");
	qglMultiTexCoord2s						= (GLMULTITEXCOORD2S)GLW_GetProcAddress("glMultiTexCoord2s");
	qglMultiTexCoord2sv						= (GLMULTITEXCOORD2SV)GLW_GetProcAddress("glMultiTexCoord2sv");
	qglMultiTexCoord3d						= (GLMULTITEXCOORD3D)GLW_GetProcAddress("glMultiTexCoord3d");
	qglMultiTexCoord3dv						= (GLMULTITEXCOORD3DV)GLW_GetProcAddress("glMultiTexCoord3dv");
	qglMultiTexCoord3f						= (GLMULTITEXCOORD3F)GLW_GetProcAddress("glMultiTexCoord3f");
	qglMultiTexCoord3fv						= (GLMULTITEXCOORD3FV)GLW_GetProcAddress("glMultiTexCoord3fv");
	qglMultiTexCoord3i						= (GLMULTITEXCOORD3I)GLW_GetProcAddress("glMultiTexCoord3i");
	qglMultiTexCoord3iv						= (GLMULTITEXCOORD3IV)GLW_GetProcAddress("glMultiTexCoord3iv");
	qglMultiTexCoord3s						= (GLMULTITEXCOORD3S)GLW_GetProcAddress("glMultiTexCoord3s");
	qglMultiTexCoord3sv						= (GLMULTITEXCOORD3SV)GLW_GetProcAddress("glMultiTexCoord3sv");
	qglMultiTexCoord4d						= (GLMULTITEXCOORD4D)GLW_GetProcAddress("glMultiTexCoord4d");
	qglMultiTexCoord4dv						= (GLMULTITEXCOORD4DV)GLW_GetProcAddress("glMultiTexCoord4dv");
	qglMultiTexCoord4f						= (GLMULTITEXCOORD4F)GLW_GetProcAddress("glMultiTexCoord4f");
	qglMultiTexCoord4fv						= (GLMULTITEXCOORD4FV)GLW_GetProcAddress("glMultiTexCoord4fv");
	qglMultiTexCoord4i						= (GLMULTITEXCOORD4I)GLW_GetProcAddress("glMultiTexCoord4i");
	qglMultiTexCoord4iv						= (GLMULTITEXCOORD4IV)GLW_GetProcAddress("glMultiTexCoord4iv");
	qglMultiTexCoord4s						= (GLMULTITEXCOORD4S)GLW_GetProcAddress("glMultiTexCoord4s");
	qglMultiTexCoord4sv						= (GLMULTITEXCOORD4SV)GLW_GetProcAddress("glMultiTexCoord4sv");
	qglMultiTexCoordP1ui					= (GLMULTITEXCOORDP1UI)GLW_GetProcAddress("glMultiTexCoordP1ui");
	qglMultiTexCoordP1uiv					= (GLMULTITEXCOORDP1UIV)GLW_GetProcAddress("glMultiTexCoordP1uiv");
	qglMultiTexCoordP2ui					= (GLMULTITEXCOORDP2UI)GLW_GetProcAddress("glMultiTexCoordP2ui");
	qglMultiTexCoordP2uiv					= (GLMULTITEXCOORDP2UIV)GLW_GetProcAddress("glMultiTexCoordP2uiv");
	qglMultiTexCoordP3ui					= (GLMULTITEXCOORDP3UI)GLW_GetProcAddress("glMultiTexCoordP3ui");
	qglMultiTexCoordP3uiv					= (GLMULTITEXCOORDP3UIV)GLW_GetProcAddress("glMultiTexCoordP3uiv");
	qglMultiTexCoordP4ui					= (GLMULTITEXCOORDP4UI)GLW_GetProcAddress("glMultiTexCoordP4ui");
	qglMultiTexCoordP4uiv					= (GLMULTITEXCOORDP4UIV)GLW_GetProcAddress("glMultiTexCoordP4uiv");
	qglNormalP3ui							= (GLNORMALP3UI)GLW_GetProcAddress("glNormalP3ui");
	qglNormalP3uiv							= (GLNORMALP3UIV)GLW_GetProcAddress("glNormalP3uiv");
	qglPointParameterf						= (GLPOINTPARAMETERF)GLW_GetProcAddress("glPointParameterf");
	qglPointParameterfv						= (GLPOINTPARAMETERFV)GLW_GetProcAddress("glPointParameterfv");
	qglPointParameteri						= (GLPOINTPARAMETERI)GLW_GetProcAddress("glPointParameteri");
	qglPointParameteriv						= (GLPOINTPARAMETERIV)GLW_GetProcAddress("glPointParameteriv");
	qglPrimitiveRestartIndex				= (GLPRIMITIVERESTARTINDEX)GLW_GetProcAddress("glPrimitiveRestartIndex");
	qglProvokingVertex						= (GLPROVOKINGVERTEX)GLW_GetProcAddress("glProvokingVertex");
	qglQueryCounter							= (GLQUERYCOUNTER)GLW_GetProcAddress("glQueryCounter");
	qglRenderbufferStorage					= (GLRENDERBUFFERSTORAGE)GLW_GetProcAddress("glRenderbufferStorage");
	qglRenderbufferStorageMultisample		= (GLRENDERBUFFERSTORAGEMULTISAMPLE)GLW_GetProcAddress("glRenderbufferStorageMultisample");
	qglSampleCoverage						= (GLSAMPLECOVERAGE)GLW_GetProcAddress("glSampleCoverage");
	qglSampleMaski							= (GLSAMPLEMASKI)GLW_GetProcAddress("glSampleMaski");
	qglSamplerParameterIiv					= (GLSAMPLERPARAMETERIIV)GLW_GetProcAddress("glSamplerParameterIiv");
	qglSamplerParameterIuiv					= (GLSAMPLERPARAMETERIUIV)GLW_GetProcAddress("glSamplerParameterIuiv");
	qglSamplerParameterf					= (GLSAMPLERPARAMETERF)GLW_GetProcAddress("glSamplerParameterf");
	qglSamplerParameterfv					= (GLSAMPLERPARAMETERFV)GLW_GetProcAddress("glSamplerParameterfv");
	qglSamplerParameteri					= (GLSAMPLERPARAMETERI)GLW_GetProcAddress("glSamplerParameteri");
	qglSamplerParameteriv					= (GLSAMPLERPARAMETERIV)GLW_GetProcAddress("glSamplerParameteriv");
	qglSecondaryColor3b						= (GLSECONDARYCOLOR3B)GLW_GetProcAddress("glSecondaryColor3b");
	qglSecondaryColor3bv					= (GLSECONDARYCOLOR3BV)GLW_GetProcAddress("glSecondaryColor3bv");
	qglSecondaryColor3d						= (GLSECONDARYCOLOR3D)GLW_GetProcAddress("glSecondaryColor3d");
	qglSecondaryColor3dv					= (GLSECONDARYCOLOR3DV)GLW_GetProcAddress("glSecondaryColor3dv");
	qglSecondaryColor3f						= (GLSECONDARYCOLOR3F)GLW_GetProcAddress("glSecondaryColor3f");
	qglSecondaryColor3fv					= (GLSECONDARYCOLOR3FV)GLW_GetProcAddress("glSecondaryColor3fv");
	qglSecondaryColor3i						= (GLSECONDARYCOLOR3I)GLW_GetProcAddress("glSecondaryColor3i");
	qglSecondaryColor3iv					= (GLSECONDARYCOLOR3IV)GLW_GetProcAddress("glSecondaryColor3iv");
	qglSecondaryColor3s						= (GLSECONDARYCOLOR3S)GLW_GetProcAddress("glSecondaryColor3s");
	qglSecondaryColor3sv					= (GLSECONDARYCOLOR3SV)GLW_GetProcAddress("glSecondaryColor3sv");
	qglSecondaryColor3ub					= (GLSECONDARYCOLOR3UB)GLW_GetProcAddress("glSecondaryColor3ub");
	qglSecondaryColor3ubv					= (GLSECONDARYCOLOR3UBV)GLW_GetProcAddress("glSecondaryColor3ubv");
	qglSecondaryColor3ui					= (GLSECONDARYCOLOR3UI)GLW_GetProcAddress("glSecondaryColor3ui");
	qglSecondaryColor3uiv					= (GLSECONDARYCOLOR3UIV)GLW_GetProcAddress("glSecondaryColor3uiv");
	qglSecondaryColor3us					= (GLSECONDARYCOLOR3US)GLW_GetProcAddress("glSecondaryColor3us");
	qglSecondaryColor3usv					= (GLSECONDARYCOLOR3USV)GLW_GetProcAddress("glSecondaryColor3usv");
	qglSecondaryColorP3ui					= (GLSECONDARYCOLORP3UI)GLW_GetProcAddress("glSecondaryColorP3ui");
	qglSecondaryColorP3uiv					= (GLSECONDARYCOLORP3UIV)GLW_GetProcAddress("glSecondaryColorP3uiv");
	qglSecondaryColorPointer				= (GLSECONDARYCOLORPOINTER)GLW_GetProcAddress("glSecondaryColorPointer");
	qglShaderSource							= (GLSHADERSOURCE)GLW_GetProcAddress("glShaderSource");
	qglStencilFuncSeparate					= (GLSTENCILFUNCSEPARATE)GLW_GetProcAddress("glStencilFuncSeparate");
	qglStencilMaskSeparate					= (GLSTENCILMASKSEPARATE)GLW_GetProcAddress("glStencilMaskSeparate");
	qglStencilOpSeparate					= (GLSTENCILOPSEPARATE)GLW_GetProcAddress("glStencilOpSeparate");
	qglTexBuffer							= (GLTEXBUFFER)GLW_GetProcAddress("glTexBuffer");
	qglTexCoordP1ui							= (GLTEXCOORDP1UI)GLW_GetProcAddress("glTexCoordP1ui");
	qglTexCoordP1uiv						= (GLTEXCOORDP1UIV)GLW_GetProcAddress("glTexCoordP1uiv");
	qglTexCoordP2ui							= (GLTEXCOORDP2UI)GLW_GetProcAddress("glTexCoordP2ui");
	qglTexCoordP2uiv						= (GLTEXCOORDP2UIV)GLW_GetProcAddress("glTexCoordP2uiv");
	qglTexCoordP3ui							= (GLTEXCOORDP3UI)GLW_GetProcAddress("glTexCoordP3ui");
	qglTexCoordP3uiv						= (GLTEXCOORDP3UIV)GLW_GetProcAddress("glTexCoordP3uiv");
	qglTexCoordP4ui							= (GLTEXCOORDP4UI)GLW_GetProcAddress("glTexCoordP4ui");
	qglTexCoordP4uiv						= (GLTEXCOORDP4UIV)GLW_GetProcAddress("glTexCoordP4uiv");
	qglTexImage2DMultisample				= (GLTEXIMAGE2DMULTISAMPLE)GLW_GetProcAddress("glTexImage2DMultisample");
	qglTexImage3D							= (GLTEXIMAGE3D)GLW_GetProcAddress("glTexImage3D");
	qglTexImage3DMultisample				= (GLTEXIMAGE3DMULTISAMPLE)GLW_GetProcAddress("glTexImage3DMultisample");
	qglTexParameterIiv						= (GLTEXPARAMETERIIV)GLW_GetProcAddress("glTexParameterIiv");
	qglTexParameterIuiv						= (GLTEXPARAMETERIUIV)GLW_GetProcAddress("glTexParameterIuiv");
	qglTexSubImage3D						= (GLTEXSUBIMAGE3D)GLW_GetProcAddress("glTexSubImage3D");
	qglTransformFeedbackVaryings			= (GLTRANSFORMFEEDBACKVARYINGS)GLW_GetProcAddress("glTransformFeedbackVaryings");
	qglUniform1f							= (GLUNIFORM1F)GLW_GetProcAddress("glUniform1f");
	qglUniform1fv							= (GLUNIFORM1FV)GLW_GetProcAddress("glUniform1fv");
	qglUniform1i							= (GLUNIFORM1I)GLW_GetProcAddress("glUniform1i");
	qglUniform1iv							= (GLUNIFORM1IV)GLW_GetProcAddress("glUniform1iv");
	qglUniform1ui							= (GLUNIFORM1UI)GLW_GetProcAddress("glUniform1ui");
	qglUniform1uiv							= (GLUNIFORM1UIV)GLW_GetProcAddress("glUniform1uiv");
	qglUniform2f							= (GLUNIFORM2F)GLW_GetProcAddress("glUniform2f");
	qglUniform2fv							= (GLUNIFORM2FV)GLW_GetProcAddress("glUniform2fv");
	qglUniform2i							= (GLUNIFORM2I)GLW_GetProcAddress("glUniform2i");
	qglUniform2iv							= (GLUNIFORM2IV)GLW_GetProcAddress("glUniform2iv");
	qglUniform2ui							= (GLUNIFORM2UI)GLW_GetProcAddress("glUniform2ui");
	qglUniform2uiv							= (GLUNIFORM2UIV)GLW_GetProcAddress("glUniform2uiv");
	qglUniform3f							= (GLUNIFORM3F)GLW_GetProcAddress("glUniform3f");
	qglUniform3fv							= (GLUNIFORM3FV)GLW_GetProcAddress("glUniform3fv");
	qglUniform3i							= (GLUNIFORM3I)GLW_GetProcAddress("glUniform3i");
	qglUniform3iv							= (GLUNIFORM3IV)GLW_GetProcAddress("glUniform3iv");
	qglUniform3ui							= (GLUNIFORM3UI)GLW_GetProcAddress("glUniform3ui");
	qglUniform3uiv							= (GLUNIFORM3UIV)GLW_GetProcAddress("glUniform3uiv");
	qglUniform4f							= (GLUNIFORM4F)GLW_GetProcAddress("glUniform4f");
	qglUniform4fv							= (GLUNIFORM4FV)GLW_GetProcAddress("glUniform4fv");
	qglUniform4i							= (GLUNIFORM4I)GLW_GetProcAddress("glUniform4i");
	qglUniform4iv							= (GLUNIFORM4IV)GLW_GetProcAddress("glUniform4iv");
	qglUniform4ui							= (GLUNIFORM4UI)GLW_GetProcAddress("glUniform4ui");
	qglUniform4uiv							= (GLUNIFORM4UIV)GLW_GetProcAddress("glUniform4uiv");
	qglUniformBlockBinding					= (GLUNIFORMBLOCKBINDING)GLW_GetProcAddress("glUniformBlockBinding");
	qglUniformMatrix2fv						= (GLUNIFORMMATRIX2FV)GLW_GetProcAddress("glUniformMatrix2fv");
	qglUniformMatrix2x3fv					= (GLUNIFORMMATRIX2X3FV)GLW_GetProcAddress("glUniformMatrix2x3fv");
	qglUniformMatrix2x4fv					= (GLUNIFORMMATRIX2X4FV)GLW_GetProcAddress("glUniformMatrix2x4fv");
	qglUniformMatrix3fv						= (GLUNIFORMMATRIX3FV)GLW_GetProcAddress("glUniformMatrix3fv");
	qglUniformMatrix3x2fv					= (GLUNIFORMMATRIX3X2FV)GLW_GetProcAddress("glUniformMatrix3x2fv");
	qglUniformMatrix3x4fv					= (GLUNIFORMMATRIX3X4FV)GLW_GetProcAddress("glUniformMatrix3x4fv");
	qglUniformMatrix4fv						= (GLUNIFORMMATRIX4FV)GLW_GetProcAddress("glUniformMatrix4fv");
	qglUniformMatrix4x2fv					= (GLUNIFORMMATRIX4X2FV)GLW_GetProcAddress("glUniformMatrix4x2fv");
	qglUniformMatrix4x3fv					= (GLUNIFORMMATRIX4X3FV)GLW_GetProcAddress("glUniformMatrix4x3fv");
	qglUnmapBuffer							= (GLUNMAPBUFFER)GLW_GetProcAddress("glUnmapBuffer");
	qglUseProgram							= (GLUSEPROGRAM)GLW_GetProcAddress("glUseProgram");
	qglValidateProgram						= (GLVALIDATEPROGRAM)GLW_GetProcAddress("glValidateProgram");
	qglVertexAttrib1d						= (GLVERTEXATTRIB1D)GLW_GetProcAddress("glVertexAttrib1d");
	qglVertexAttrib1dv						= (GLVERTEXATTRIB1DV)GLW_GetProcAddress("glVertexAttrib1dv");
	qglVertexAttrib1f						= (GLVERTEXATTRIB1F)GLW_GetProcAddress("glVertexAttrib1f");
	qglVertexAttrib1fv						= (GLVERTEXATTRIB1FV)GLW_GetProcAddress("glVertexAttrib1fv");
	qglVertexAttrib1s						= (GLVERTEXATTRIB1S)GLW_GetProcAddress("glVertexAttrib1s");
	qglVertexAttrib1sv						= (GLVERTEXATTRIB1SV)GLW_GetProcAddress("glVertexAttrib1sv");
	qglVertexAttrib2d						= (GLVERTEXATTRIB2D)GLW_GetProcAddress("glVertexAttrib2d");
	qglVertexAttrib2dv						= (GLVERTEXATTRIB2DV)GLW_GetProcAddress("glVertexAttrib2dv");
	qglVertexAttrib2f						= (GLVERTEXATTRIB2F)GLW_GetProcAddress("glVertexAttrib2f");
	qglVertexAttrib2fv						= (GLVERTEXATTRIB2FV)GLW_GetProcAddress("glVertexAttrib2fv");
	qglVertexAttrib2s						= (GLVERTEXATTRIB2S)GLW_GetProcAddress("glVertexAttrib2s");
	qglVertexAttrib2sv						= (GLVERTEXATTRIB2SV)GLW_GetProcAddress("glVertexAttrib2sv");
	qglVertexAttrib3d						= (GLVERTEXATTRIB3D)GLW_GetProcAddress("glVertexAttrib3d");
	qglVertexAttrib3dv						= (GLVERTEXATTRIB3DV)GLW_GetProcAddress("glVertexAttrib3dv");
	qglVertexAttrib3f						= (GLVERTEXATTRIB3F)GLW_GetProcAddress("glVertexAttrib3f");
	qglVertexAttrib3fv						= (GLVERTEXATTRIB3FV)GLW_GetProcAddress("glVertexAttrib3fv");
	qglVertexAttrib3s						= (GLVERTEXATTRIB3S)GLW_GetProcAddress("glVertexAttrib3s");
	qglVertexAttrib3sv						= (GLVERTEXATTRIB3SV)GLW_GetProcAddress("glVertexAttrib3sv");
	qglVertexAttrib4Nbv						= (GLVERTEXATTRIB4NBV)GLW_GetProcAddress("glVertexAttrib4Nbv");
	qglVertexAttrib4Niv						= (GLVERTEXATTRIB4NIV)GLW_GetProcAddress("glVertexAttrib4Niv");
	qglVertexAttrib4Nsv						= (GLVERTEXATTRIB4NSV)GLW_GetProcAddress("glVertexAttrib4Nsv");
	qglVertexAttrib4Nub						= (GLVERTEXATTRIB4NUB)GLW_GetProcAddress("glVertexAttrib4Nub");
	qglVertexAttrib4Nubv					= (GLVERTEXATTRIB4NUBV)GLW_GetProcAddress("glVertexAttrib4Nubv");
	qglVertexAttrib4Nuiv					= (GLVERTEXATTRIB4NUIV)GLW_GetProcAddress("glVertexAttrib4Nuiv");
	qglVertexAttrib4Nusv					= (GLVERTEXATTRIB4NUSV)GLW_GetProcAddress("glVertexAttrib4Nusv");
	qglVertexAttrib4bv						= (GLVERTEXATTRIB4BV)GLW_GetProcAddress("glVertexAttrib4bv");
	qglVertexAttrib4d						= (GLVERTEXATTRIB4D)GLW_GetProcAddress("glVertexAttrib4d");
	qglVertexAttrib4dv						= (GLVERTEXATTRIB4DV)GLW_GetProcAddress("glVertexAttrib4dv");
	qglVertexAttrib4f						= (GLVERTEXATTRIB4F)GLW_GetProcAddress("glVertexAttrib4f");
	qglVertexAttrib4fv						= (GLVERTEXATTRIB4FV)GLW_GetProcAddress("glVertexAttrib4fv");
	qglVertexAttrib4iv						= (GLVERTEXATTRIB4IV)GLW_GetProcAddress("glVertexAttrib4iv");
	qglVertexAttrib4s						= (GLVERTEXATTRIB4S)GLW_GetProcAddress("glVertexAttrib4s");
	qglVertexAttrib4sv						= (GLVERTEXATTRIB4SV)GLW_GetProcAddress("glVertexAttrib4sv");
	qglVertexAttrib4ubv						= (GLVERTEXATTRIB4UBV)GLW_GetProcAddress("glVertexAttrib4ubv");
	qglVertexAttrib4uiv						= (GLVERTEXATTRIB4UIV)GLW_GetProcAddress("glVertexAttrib4uiv");
	qglVertexAttrib4usv						= (GLVERTEXATTRIB4USV)GLW_GetProcAddress("glVertexAttrib4usv");
	qglVertexAttribI1i						= (GLVERTEXATTRIBI1I)GLW_GetProcAddress("glVertexAttribI1i");
	qglVertexAttribI1iv						= (GLVERTEXATTRIBI1IV)GLW_GetProcAddress("glVertexAttribI1iv");
	qglVertexAttribI1ui						= (GLVERTEXATTRIBI1UI)GLW_GetProcAddress("glVertexAttribI1ui");
	qglVertexAttribI1uiv					= (GLVERTEXATTRIBI1UIV)GLW_GetProcAddress("glVertexAttribI1uiv");
	qglVertexAttribI2i						= (GLVERTEXATTRIBI2I)GLW_GetProcAddress("glVertexAttribI2i");
	qglVertexAttribI2iv						= (GLVERTEXATTRIBI2IV)GLW_GetProcAddress("glVertexAttribI2iv");
	qglVertexAttribI2ui						= (GLVERTEXATTRIBI2UI)GLW_GetProcAddress("glVertexAttribI2ui");
	qglVertexAttribI2uiv					= (GLVERTEXATTRIBI2UIV)GLW_GetProcAddress("glVertexAttribI2uiv");
	qglVertexAttribI3i						= (GLVERTEXATTRIBI3I)GLW_GetProcAddress("glVertexAttribI3i");
	qglVertexAttribI3iv						= (GLVERTEXATTRIBI3IV)GLW_GetProcAddress("glVertexAttribI3iv");
	qglVertexAttribI3ui						= (GLVERTEXATTRIBI3UI)GLW_GetProcAddress("glVertexAttribI3ui");
	qglVertexAttribI3uiv					= (GLVERTEXATTRIBI3UIV)GLW_GetProcAddress("glVertexAttribI3uiv");
	qglVertexAttribI4bv						= (GLVERTEXATTRIBI4BV)GLW_GetProcAddress("glVertexAttribI4bv");
	qglVertexAttribI4i						= (GLVERTEXATTRIBI4I)GLW_GetProcAddress("glVertexAttribI4i");
	qglVertexAttribI4iv						= (GLVERTEXATTRIBI4IV)GLW_GetProcAddress("glVertexAttribI4iv");
	qglVertexAttribI4sv						= (GLVERTEXATTRIBI4SV)GLW_GetProcAddress("glVertexAttribI4sv");
	qglVertexAttribI4ubv					= (GLVERTEXATTRIBI4UBV)GLW_GetProcAddress("glVertexAttribI4ubv");
	qglVertexAttribI4ui						= (GLVERTEXATTRIBI4UI)GLW_GetProcAddress("glVertexAttribI4ui");
	qglVertexAttribI4uiv					= (GLVERTEXATTRIBI4UIV)GLW_GetProcAddress("glVertexAttribI4uiv");
	qglVertexAttribI4usv					= (GLVERTEXATTRIBI4USV)GLW_GetProcAddress("glVertexAttribI4usv");
	qglVertexAttribIPointer					= (GLVERTEXATTRIBIPOINTER)GLW_GetProcAddress("glVertexAttribIPointer");
	qglVertexAttribP1ui						= (GLVERTEXATTRIBP1UI)GLW_GetProcAddress("glVertexAttribP1ui");
	qglVertexAttribP1uiv					= (GLVERTEXATTRIBP1UIV)GLW_GetProcAddress("glVertexAttribP1uiv");
	qglVertexAttribP2ui						= (GLVERTEXATTRIBP2UI)GLW_GetProcAddress("glVertexAttribP2ui");
	qglVertexAttribP2uiv					= (GLVERTEXATTRIBP2UIV)GLW_GetProcAddress("glVertexAttribP2uiv");
	qglVertexAttribP3ui						= (GLVERTEXATTRIBP3UI)GLW_GetProcAddress("glVertexAttribP3ui");
	qglVertexAttribP3uiv					= (GLVERTEXATTRIBP3UIV)GLW_GetProcAddress("glVertexAttribP3uiv");
	qglVertexAttribP4ui						= (GLVERTEXATTRIBP4UI)GLW_GetProcAddress("glVertexAttribP4ui");
	qglVertexAttribP4uiv					= (GLVERTEXATTRIBP4UIV)GLW_GetProcAddress("glVertexAttribP4uiv");
	qglVertexAttribPointer					= (GLVERTEXATTRIBPOINTER)GLW_GetProcAddress("glVertexAttribPointer");
	qglVertexP2ui							= (GLVERTEXP2UI)GLW_GetProcAddress("glVertexP2ui");
	qglVertexP2uiv							= (GLVERTEXP2UIV)GLW_GetProcAddress("glVertexP2uiv");
	qglVertexP3ui							= (GLVERTEXP3UI)GLW_GetProcAddress("glVertexP3ui");
	qglVertexP3uiv							= (GLVERTEXP3UIV)GLW_GetProcAddress("glVertexP3uiv");
	qglVertexP4ui							= (GLVERTEXP4UI)GLW_GetProcAddress("glVertexP4ui");
	qglVertexP4uiv							= (GLVERTEXP4UIV)GLW_GetProcAddress("glVertexP4uiv");
	qglWaitSync								= (GLWAITSYNC)GLW_GetProcAddress("glWaitSync");
	qglWindowPos2d							= (GLWINDOWPOS2D)GLW_GetProcAddress("glWindowPos2d");
	qglWindowPos2dv							= (GLWINDOWPOS2DV)GLW_GetProcAddress("glWindowPos2dv");
	qglWindowPos2f							= (GLWINDOWPOS2F)GLW_GetProcAddress("glWindowPos2f");
	qglWindowPos2fv							= (GLWINDOWPOS2FV)GLW_GetProcAddress("glWindowPos2fv");
	qglWindowPos2i							= (GLWINDOWPOS2I)GLW_GetProcAddress("glWindowPos2i");
	qglWindowPos2iv							= (GLWINDOWPOS2IV)GLW_GetProcAddress("glWindowPos2iv");
	qglWindowPos2s							= (GLWINDOWPOS2S)GLW_GetProcAddress("glWindowPos2s");
	qglWindowPos2sv							= (GLWINDOWPOS2SV)GLW_GetProcAddress("glWindowPos2sv");
	qglWindowPos3d							= (GLWINDOWPOS3D)GLW_GetProcAddress("glWindowPos3d");
	qglWindowPos3dv							= (GLWINDOWPOS3DV)GLW_GetProcAddress("glWindowPos3dv");
	qglWindowPos3f							= (GLWINDOWPOS3F)GLW_GetProcAddress("glWindowPos3f");
	qglWindowPos3fv							= (GLWINDOWPOS3FV)GLW_GetProcAddress("glWindowPos3fv");
	qglWindowPos3i							= (GLWINDOWPOS3I)GLW_GetProcAddress("glWindowPos3i");
	qglWindowPos3iv							= (GLWINDOWPOS3IV)GLW_GetProcAddress("glWindowPos3iv");
	qglWindowPos3s							= (GLWINDOWPOS3S)GLW_GetProcAddress("glWindowPos3s");
	qglWindowPos3sv							= (GLWINDOWPOS3SV)GLW_GetProcAddress("glWindowPos3sv");

	// Initialize the extensions
	if (GLW_IsExtensionPresent(glConfig.extensionsString, "GL_EXT_texture_compression_s3tc")){
		glConfig.textureCompressionS3TCAvailable = true;

		Com_Printf("...using GL_EXT_texture_compression_s3tc\n");
	}
	else
		Com_Printf("...GL_EXT_texture_compression_s3tc not found\n");

	if (GLW_IsExtensionPresent(glConfig.extensionsString, "GL_EXT_texture_filter_anisotropic")){
		glConfig.textureFilterAnisotropicAvailable = true;

		qglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureMaxAnisotropy);

		Com_Printf("...using GL_EXT_texture_filter_anisotropic\n");
	}
	else
		Com_Printf("...GL_EXT_texture_filter_anisotropic not found\n");

	if (GLW_IsExtensionPresent(glConfig.extensionsString, "WGL_EXT_swap_control")){
		glConfig.swapControlAvailable = true;

		qwglSwapIntervalEXT						= (WGLSWAPINTERVALEXT)GLW_GetProcAddress("wglSwapIntervalEXT");

		Com_Printf("...using WGL_EXT_swap_control\n");
	}
	else
		Com_Printf("...WGL_EXT_swap_control not found\n");

	if (GLW_IsExtensionPresent(glConfig.wglExtensionsString, "WGL_EXT_swap_control_tear")){
		glConfig.swapControlTearAvailable = true;

		Com_Printf("...using WGL_EXT_swap_control_tear\n");
	}
	else
		Com_Printf("...WGL_EXT_swap_control_tear not found\n");

	if (GLW_IsExtensionPresent(glConfig.extensionsString, "GL_EXT_stencil_wrap")){
		glConfig.stencilWrapAvailable = true;

		Com_Printf("...using GL_EXT_stencil_wrap\n");
	}
	else
		Com_Printf("...GL_EXT_stencil_wrap not found\n");

	if (GLW_IsExtensionPresent(glConfig.extensionsString, "GL_EXT_stencil_two_side")){
		glConfig.stencilTwoSideAvailable = true;

		qglActiveStencilFaceEXT					= GLW_GetProcAddress("glActiveStencilFaceEXT");

		Com_Printf("...using GL_EXT_stencil_two_side\n");
	}
	else
		Com_Printf("...GL_EXT_stencil_two_side not found\n");

	if (GLW_IsExtensionPresent(glConfig.extensionsString, "GL_ATI_separate_stencil")){
		glConfig.atiSeparateStencilAvailable = true;

		qglStencilOpSeparateATI					= GLW_GetProcAddress("glStencilOpSeparateATI");
		qglStencilFuncSeparateATI				= GLW_GetProcAddress("glStencilFuncSeparateATI");

		Com_Printf("...using GL_ATI_separate_stencil\n");
	}
	else
		Com_Printf("...GL_ATI_separate_stencil not found\n");
}


/*
 ==============================================================================

 DRIVER INITIALIZATION

 ==============================================================================
*/


/*
 ==================
 GLW_InitFakeOpenGL

 Fake OpenGL stuff to work around the crappy WGL limitations.
 Do this silently.
 ==================
*/
static void GLW_InitFakeOpenGL (){

	WNDCLASSEX				wndClass;
	PIXELFORMATDESCRIPTOR	pfd;
	int						pixelFormat;

	// Register the window class
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = WIN_FakeWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = sys.hInstance;
	wndClass.hIcon = sys.hIcon;
	wndClass.hIconSm = NULL;
	wndClass.hCursor = NULL;
	wndClass.hbrBackground = NULL;
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = WINDOW_CLASS_FAKE;

	if (!RegisterClassEx(&wndClass))
		return;

	// Create the fake window
	if ((glwState.hWndFake = CreateWindowEx(0, WINDOW_CLASS_FAKE, WINDOW_NAME, WINDOW_STYLE_FULLSCREEN, 0, 0, 100, 100, NULL, NULL, sys.hInstance, NULL)) == NULL){
		UnregisterClass(WINDOW_CLASS_FAKE, sys.hInstance);
		return;
	}

	// Get a DC for the fake window
	if ((glwState.hDCFake = GetDC(glwState.hWndFake)) == NULL)
		return;

	// Choose a pixel format
	Mem_Fill(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;

	pixelFormat = ChoosePixelFormat(glwState.hDCFake, &pfd);
	if (!pixelFormat)
		return;

	// Set the pixel format
	if (!SetPixelFormat(glwState.hDCFake, pixelFormat, &pfd))
		return;

	// Create the fake GL context
	if ((glwState.hGLRCFake = qwglCreateContext(glwState.hDCFake)) == NULL)
		return;

	// Make the fake GL context current
	if (!qwglMakeCurrent(glwState.hDCFake, glwState.hGLRCFake))
		return;

	// We only need these function pointers if available
	qwglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)qwglGetProcAddress("wglGetPixelFormatAttribivARB");
	qwglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)qwglGetProcAddress("wglGetPixelFormatAttribfvARB");
	qwglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)qwglGetProcAddress("wglChoosePixelFormatARB");
}

/*
 ==================
 GLW_ShutdownFakeOpenGL

 Fake OpenGL stuff to work around the crappy WGL limitations.
 Do this silently.
 ==================
*/
static void GLW_ShutdownFakeOpenGL (){

	if (glwState.hGLRCFake){
		qwglMakeCurrent(NULL, NULL);

		qwglDeleteContext(glwState.hGLRCFake);
		glwState.hGLRCFake = NULL;
	}

	if (glwState.hDCFake){
		ReleaseDC(glwState.hWndFake, glwState.hDCFake);
		glwState.hDCFake = NULL;
	}

	if (glwState.hWndFake){
		DestroyWindow(glwState.hWndFake);
		glwState.hWndFake = NULL;

		UnregisterClass(WINDOW_CLASS_FAKE, sys.hInstance);
	}

	qwglGetPixelFormatAttribivARB = NULL;
	qwglGetPixelFormatAttribfvARB = NULL;
	qwglChoosePixelFormatARB = NULL;
}

/*
 ==================
 GLW_ChoosePixelFormat
 ==================
*/
static int GLW_ChoosePixelFormat (PIXELFORMATDESCRIPTOR *pfd){

	int		attribs[24];
	int		samples = 0;
	int		pixelFormat = 0;
	int		numPixelFormats;

	// Check if multisampling is desired
	if (r_multiSamples->integerValue > 1)
		Com_Printf("...attempting to use %ix multisampling\n", r_multiSamples->integerValue);

	// Initialize the fake OpenGL stuff so that we can use the extended pixel
	// format functionality
	GLW_InitFakeOpenGL();

	// Choose a pixel format
	if (qwglChoosePixelFormatARB == NULL || r_multiSamples->integerValue <= 1){
		Mem_Fill(pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

		pfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd->nVersion = 1;
		pfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd->iPixelType = PFD_TYPE_RGBA;
		pfd->iLayerType = PFD_MAIN_PLANE;
		pfd->cColorBits = 32;
		pfd->cAlphaBits = 8;
		pfd->cDepthBits = 24;
		pfd->cStencilBits = 8;

		pixelFormat = ChoosePixelFormat(glwState.hDC, pfd);
	}
	else {
		attribs[0] = WGL_ACCELERATION_ARB;
		attribs[1] = WGL_FULL_ACCELERATION_ARB;
		attribs[2] = WGL_DRAW_TO_WINDOW_ARB;
		attribs[3] = TRUE;
		attribs[4] = WGL_SUPPORT_OPENGL_ARB;
		attribs[5] = TRUE;
		attribs[6] = WGL_DOUBLE_BUFFER_ARB;
		attribs[7] = TRUE;
		attribs[8] = WGL_PIXEL_TYPE_ARB;
		attribs[9] = WGL_TYPE_RGBA_ARB;
		attribs[10] = WGL_COLOR_BITS_ARB;
		attribs[11] = 32;
		attribs[12] = WGL_ALPHA_BITS_ARB;
		attribs[13] = 8;
		attribs[14] = WGL_DEPTH_BITS_ARB;
		attribs[15] = 24;
		attribs[16] = WGL_STENCIL_BITS_ARB;
		attribs[17] = 8;
		attribs[18] = WGL_SAMPLE_BUFFERS_ARB;
		attribs[19] = 1;
		attribs[20] = WGL_SAMPLES_ARB;
		attribs[21] = r_multiSamples->integerValue;
		attribs[22] = 0;
		attribs[23] = 0;

		qwglChoosePixelFormatARB(glwState.hDC, attribs, NULL, 1, &pixelFormat, (UINT *)&numPixelFormats);

		if (pixelFormat){
			samples = WGL_SAMPLES_ARB;

			qwglGetPixelFormatAttribivARB(glwState.hDC, pixelFormat, 0, 1, &samples, &samples);
		}
	}

	// Shutdown the fake OpenGL stuff since we no longer need it
	GLW_ShutdownFakeOpenGL();

	// Make sure we have a valid pixel format
	if (!pixelFormat)
		return 0;

	// Describe the pixel format
	DescribePixelFormat(glwState.hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), pfd);

	if (pfd->dwFlags & (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED)){
		Com_Printf("...no hardware acceleration found\n");
		return 0;
	}

	Com_Printf("...hardware acceleration found\n");

	if (pfd->cColorBits < 32 || pfd->cAlphaBits < 8 || pfd->cDepthBits < 24 || pfd->cStencilBits < 8){
		Com_Printf("...insufficient pixel format\n");
		return 0;
	}

	// Report if multisampling is desired but unavailable
	if (r_multiSamples->integerValue > 1 && samples <= 1)
		Com_Printf("...failed to select a multisample pixel format\n");

	// Get the pixel format information
	glConfig.colorBits = pfd->cColorBits;
	glConfig.alphaBits = pfd->cAlphaBits;
	glConfig.depthBits = pfd->cDepthBits;
	glConfig.stencilBits = pfd->cStencilBits;

	return pixelFormat;
}

/*
 ==================
 GLW_InitDriver
 ==================
*/
static bool GLW_InitDriver (){

	PIXELFORMATDESCRIPTOR	pfd;
	int						pixelFormat;

	Com_Printf("Initializing OpenGL driver\n");

	// Get a DC for the current window
	Com_Printf("...getting DC: ");

	if ((glwState.hDC = GetDC(glwState.hWnd)) == NULL){
		Com_Printf("failed\n" );
		return false;
	}

	Com_Printf("succeeded\n");

	// Get the device gamma ramp
	Com_Printf("...getting gamma ramp: ");

	if ((glwState.supportsGamma = GetDeviceGammaRamp(glwState.hDC, glwState.oldGammaRamp)) == FALSE)
		Com_Printf("failed\n");
	else
		Com_Printf("succeeded\n");

	// Choose a pixel format
	if ((pixelFormat = GLW_ChoosePixelFormat(&pfd)) == 0){
		Com_Printf("...failed to find an appropriate pixel format\n");

		ReleaseDC(glwState.hWnd, glwState.hDC);
		glwState.hDC = NULL;

		return false;
	}

	Com_Printf("...pixel format %i selected\n", pixelFormat);

	// Set the pixel format
	if (!SetPixelFormat(glwState.hDC, pixelFormat, &pfd)){
		Com_Printf("...failed to set pixel format\n");

		ReleaseDC(glwState.hWnd, glwState.hDC);
		glwState.hDC = NULL;

		return false;
	}

	// Create the GL context
	Com_Printf("...creating GL context: ");

	if ((glwState.hGLRC = qwglCreateContext(glwState.hDC)) == NULL){
		Com_Printf("failed\n");

		ReleaseDC(glwState.hWnd, glwState.hDC);
		glwState.hDC = NULL;

		return false;
	}

	Com_Printf("succeeded\n");

	// Make the GL context current
	Com_Printf("...making context current: ");

	if (!qwglMakeCurrent(glwState.hDC, glwState.hGLRC)){
		Com_Printf("failed\n");

		qwglDeleteContext(glwState.hGLRC);
		glwState.hGLRC = NULL;

		ReleaseDC(glwState.hWnd, glwState.hDC);
		glwState.hDC = NULL;

		return false;
	}

	Com_Printf("succeeded\n");

	return true;
}


/*
 ==============================================================================

 WINDOW CREATION

 ==============================================================================
*/


/*
 ==================
 GLW_CreateWindow
 ==================
*/
static bool GLW_CreateWindow (){

	cvar_t		*xPos, *yPos;
	WNDCLASSEX	wndClass;
	RECT		rect;
	int			style, exStyle;
	int			x, y, w, h;

	// Get the window position and dimensions
	xPos = CVar_Register("win_xPos", "10", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Horizontal window position", 0, 65535);
	yPos = CVar_Register("win_yPos", "30", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Vertical window position", 0, 65535);

	if (glConfig.isFullscreen){
		style = WINDOW_STYLE_FULLSCREEN;
		exStyle = WS_EX_TOPMOST;

		x = 0;
		y = 0;
		w = glConfig.videoWidth;
		h = glConfig.videoHeight;
	}
	else {
		style = WINDOW_STYLE_WINDOWED;
		exStyle = 0;

		rect.left = 0;
		rect.top = 0;
		rect.right = glConfig.videoWidth;
		rect.bottom = glConfig.videoHeight;

		AdjustWindowRectEx(&rect, style, FALSE, exStyle);

		x = xPos->integerValue;
		y = yPos->integerValue;
		w = rect.right - rect.left;
		h = rect.bottom - rect.top;

		// Adjust window coordinates if necessary so that the window is
		// completely on screen
		if (w > glwState.desktopWidth)
			x = 0;
		else if (x + w > glwState.desktopWidth)
			x = glwState.desktopWidth - w;

		if (h > glwState.desktopHeight)
			y = 0;
		else if (y + h > glwState.desktopHeight)
			y = glwState.desktopHeight - h;
	}

	// Let the system code know if we're fullscreen
	sys.isFullscreen = glConfig.isFullscreen;

	// Register the window class
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = WIN_MainWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = sys.hInstance;
	wndClass.hIcon = sys.hIcon;
	wndClass.hIconSm = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = NULL;
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = WINDOW_CLASS_MAIN;

	if (!RegisterClassEx(&wndClass)){
		Com_Printf("...failed to register window class\n");
		return false;
	}

	Com_Printf("...registered window class\n");

	// Create the window
	if ((glwState.hWnd = CreateWindowEx(exStyle, WINDOW_CLASS_MAIN, WINDOW_NAME, style, x, y, w, h, NULL, NULL, sys.hInstance, NULL)) == NULL){
		Com_Printf("...failed to create window\n");

		UnregisterClass(WINDOW_CLASS_MAIN, sys.hInstance);

		return false;
	}

	Com_Printf("...created window @ %i,%i (%ix%i)\n", x, y, w, h);

	// Show the window
	ShowWindow(glwState.hWnd, SW_SHOW);
	UpdateWindow(glwState.hWnd);
	SetForegroundWindow(glwState.hWnd);
	SetFocus(glwState.hWnd);

	// Create the context
	if (!GLW_InitDriver()){
		Com_Printf("...destroying window\n");

		ShowWindow(glwState.hWnd, SW_HIDE);
		DestroyWindow(glwState.hWnd);
		glwState.hWnd = NULL;

		UnregisterClass(WINDOW_CLASS_MAIN, sys.hInstance);

		return false;
	}

	return true;
}


/*
 ==============================================================================

 DISPLAY MODE

 ==============================================================================
*/


/*
 ==================
 GLW_SetDisplayMode
 ==================
*/
static bool GLW_SetDisplayMode (){

	HDC		hDC;
	int		error;

	Com_Printf("...setting mode %i: ", r_mode->integerValue);

	if (!R_GetVideoModeInfo(r_mode->integerValue, &glConfig.videoWidth, &glConfig.videoHeight)){
		Com_Printf("invalid\n");
		return false;
	}

	Com_Printf("%ix%i ", glConfig.videoWidth, glConfig.videoHeight);

	if (r_fullscreen->integerValue){
		if (r_displayRefresh->integerValue)
			Com_Printf("fullscreen (%i Hz)\n", r_displayRefresh->integerValue);
		else
			Com_Printf("fullscreen\n");
	}
	else
		Com_Printf("windowed\n");

	// Get desktop attributes
	hDC = GetDC(GetDesktopWindow());

	glwState.desktopWidth = GetDeviceCaps(hDC, HORZRES);
	glwState.desktopHeight = GetDeviceCaps(hDC, VERTRES);
	glwState.desktopBitDepth = GetDeviceCaps(hDC, BITSPIXEL);

	ReleaseDC(GetDesktopWindow(), hDC);

	// Change display settings if needed
	if (r_fullscreen->integerValue){
		Mem_Fill(&glwState.devMode, 0, sizeof(DEVMODE));

		glwState.devMode.dmSize = sizeof(DEVMODE);
		glwState.devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
		glwState.devMode.dmPelsWidth = glConfig.videoWidth;
		glwState.devMode.dmPelsHeight = glConfig.videoHeight;
		glwState.devMode.dmBitsPerPel = 32;

		if (r_displayRefresh->integerValue){
			glwState.devMode.dmFields |= DM_DISPLAYFREQUENCY;
			glwState.devMode.dmDisplayFrequency = r_displayRefresh->integerValue;
		}

		Com_Printf("...changing display settings: ");

		if ((error = ChangeDisplaySettings(&glwState.devMode, CDS_FULLSCREEN)) == DISP_CHANGE_SUCCESSFUL){
			Com_Printf("ok\n");

			glConfig.isFullscreen = true;
		}
		else {
			switch (error){
			case DISP_CHANGE_BADMODE:
				Com_Printf("bad mode\n");
				break;
			case DISP_CHANGE_BADFLAGS:
				Com_Printf("bad flags\n");
				break;
			case DISP_CHANGE_BADPARAM:
				Com_Printf("bad param\n");
				break;
			case DISP_CHANGE_BADDUALVIEW:
				Com_Printf("bad DualView\n");
				break;
			case DISP_CHANGE_FAILED:
				Com_Printf("failed\n");
				break;
			case DISP_CHANGE_NOTUPDATED:
				Com_Printf("not updated\n");
				break;
			case DISP_CHANGE_RESTART:
				Com_Printf("restart required\n");
				break;
			default:
				Com_Printf("unknown error (%i)\n", error);
				break;
			}

			// Try safe mode, unless we just tried it and failed
			if (glConfig.videoWidth != glwState.desktopWidth || glConfig.videoHeight != glwState.desktopHeight){
				Com_Printf("...trying safe mode: %ix%i fullscreen\n", glwState.desktopWidth, glwState.desktopHeight);

				Mem_Fill(&glwState.devMode, 0, sizeof(DEVMODE));

				glwState.devMode.dmSize = sizeof(DEVMODE);
				glwState.devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
				glwState.devMode.dmPelsWidth = glwState.desktopWidth;
				glwState.devMode.dmPelsHeight = glwState.desktopHeight;
				glwState.devMode.dmBitsPerPel = 32;

				Com_Printf("...changing display settings: ");

				if ((error = ChangeDisplaySettings(&glwState.devMode, CDS_FULLSCREEN)) == DISP_CHANGE_SUCCESSFUL){
					Com_Printf("ok\n");

					glConfig.isFullscreen = true;

					glConfig.videoWidth = glwState.desktopWidth;
					glConfig.videoHeight = glwState.desktopHeight;
				}
				else {
					switch (error){
					case DISP_CHANGE_BADMODE:
						Com_Printf("bad mode\n");
						break;
					case DISP_CHANGE_BADFLAGS:
						Com_Printf("bad flags\n");
						break;
					case DISP_CHANGE_BADPARAM:
						Com_Printf("bad param\n");
						break;
					case DISP_CHANGE_BADDUALVIEW:
						Com_Printf("bad DualView\n");
						break;
					case DISP_CHANGE_FAILED:
						Com_Printf("failed\n");
						break;
					case DISP_CHANGE_NOTUPDATED:
						Com_Printf("not updated\n");
						break;
					case DISP_CHANGE_RESTART:
						Com_Printf("restart required\n");
						break;
					default:
						Com_Printf("unknown error (%i)\n", error);
						break;
					}
				}
			}
		}
	}

	// Create the window
	if (!glConfig.isFullscreen){
		if (r_fullscreen->integerValue)
			Com_Printf("...falling back to windowed mode\n");
		else
			Com_Printf("...setting windowed mode\n");

		if (glwState.desktopBitDepth != 32){
			Com_Printf("...windowed mode requires 32 bits desktop depth\n");
			return false;
		}
	}

	if (!GLW_CreateWindow()){
		if (!glConfig.isFullscreen)
			return false;

		Com_Printf("...restoring display settings\n");

		ChangeDisplaySettings(NULL, CDS_FULLSCREEN);
		glConfig.isFullscreen = false;

		return false;
	}

	// Get the display frequency
	hDC = GetDC(GetDesktopWindow());

	glConfig.displayFrequency = GetDeviceCaps(hDC, VREFRESH);

	ReleaseDC(GetDesktopWindow(), hDC);

	return true;
}


// ============================================================================


/*
 ==================
 GLW_CheckVersion
 ==================
*/
static bool GLW_CheckVersion (int major, int minor){

	int		majorVersion, minorVersion;

	qglGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	qglGetIntegerv(GL_MINOR_VERSION, &minorVersion);

	if (majorVersion < major || (majorVersion == major && minorVersion < minor))
		return false;

	return true;
}

/*
 ==================
 GLW_SetDeviceGammaRamp
 ==================
*/
void GLW_SetDeviceGammaRamp (const byte *gammaTable){

	WORD	v;
	int		i;

	if (!glwState.hDC || !glwState.supportsGamma)
		return;

	for (i = 0; i < 256; i++){
		v = (((WORD)gammaTable[i]) << 8) | gammaTable[i];

		glwState.newGammaRamp[0][i] = v;
		glwState.newGammaRamp[1][i] = v;
		glwState.newGammaRamp[2][i] = v;
	}

	if (!SetDeviceGammaRamp(glwState.hDC, glwState.newGammaRamp))
		Com_DPrintf(S_COLOR_YELLOW "WARNING: SetDeviceGammaRamp() failed\n");
}

/*
 ==================
 GLW_Activate
 ==================
*/
void GLW_Activate (bool active){

	if (!glwState.hWnd || !glwState.hDC)
		return;

	if (active){
		if (glConfig.isFullscreen){
			if (ChangeDisplaySettings(&glwState.devMode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
				Com_Error(ERR_FATAL, "GLW_Activate: ChangeDisplaySettings() failed");
		}

		ShowWindow(glwState.hWnd, SW_RESTORE);
		UpdateWindow(glwState.hWnd);
		SetForegroundWindow(glwState.hWnd);
		SetFocus(glwState.hWnd);

		if (glwState.supportsGamma){
			if (!SetDeviceGammaRamp(glwState.hDC, glwState.newGammaRamp))
				Com_DPrintf(S_COLOR_YELLOW "WARNING: SetDeviceGammaRamp() failed\n");
		}
	}
	else {
		if (glConfig.isFullscreen){
			if (ChangeDisplaySettings(NULL, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
				Com_Error(ERR_FATAL, "GLW_Activate: ChangeDisplaySettings() failed");

			ShowWindow(glwState.hWnd, SW_MINIMIZE);
		}

		if (glwState.supportsGamma){
			if (!SetDeviceGammaRamp(glwState.hDC, glwState.oldGammaRamp))
				Com_DPrintf(S_COLOR_YELLOW "WARNING: SetDeviceGammaRamp() failed\n");
		}
	}
}

/*
 ==================
 GLW_ActivateContext
 ==================
*/
void GLW_ActivateContext (bool active){

	if (!glwState.hDC || !glwState.hGLRC)
		return;

	if (active){
		if (!qwglMakeCurrent(glwState.hDC, glwState.hGLRC))
			Com_Error(ERR_FATAL, "GLW_ActivateContext: wglMakeCurrent() failed");
	}
	else {
		if (!qwglMakeCurrent(NULL, NULL))
			Com_Error(ERR_FATAL, "GLW_ActivateContext: wglMakeCurrent() failed");
	}
}

/*
 ==================
 GLW_SwapBuffers
 ==================
*/
void GLW_SwapBuffers (){

	int		interval;

	if (!glwState.hDC)
		return;

	if (r_swapInterval->modified){
		if (glConfig.swapControlAvailable){
			interval = 0;

			if (r_swapInterval->integerValue == 1)
				interval = 1;
			else if (r_swapInterval->integerValue == 2)
				interval = (glConfig.swapControlTearAvailable) ? -1 : 1;

			qwglSwapIntervalEXT(interval);
		}

		r_swapInterval->modified = false;
	}

	if (!SwapBuffers(glwState.hDC))
		Com_Error(ERR_FATAL, "GLW_SwapBuffers: SwapBuffers() failed");
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 GLW_Init
 ==================
*/
void GLW_Init (){

	const char	*driver;

	Com_Printf("Initializing OpenGL subsystem\n");

	// Initialize our QGL dynamic bindings
	if (r_glDriver->value[0])
		driver = r_glDriver->value;
	else
		driver = GL_DRIVER_OPENGL;

	if (!QGL_Init(driver))
		Com_Error(ERR_FATAL, "GLW_Init: could not load OpenGL subsystem");

	// Initialize the display, window, context, etc
	if (!GLW_SetDisplayMode()){
		QGL_Shutdown();

		Com_Error(ERR_FATAL, "GLW_Init: could not load OpenGL subsystem");
	}

	// Check for OpenGL 3.3 support
	if (!GLW_CheckVersion(3, 3)){
		GLW_Shutdown();

		Com_Error(ERR_FATAL, "The current video card / driver combination does not support OpenGL 3.3");
	}

	// Get GL strings
	glConfig.vendorString = (const char *)qglGetString(GL_VENDOR);
	glConfig.rendererString = (const char *)qglGetString(GL_RENDERER);
	glConfig.versionString = (const char *)qglGetString(GL_VERSION);
	glConfig.extensionsString = (const char *)qglGetString(GL_EXTENSIONS);

	// Get WGL strings if available
	qwglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)qwglGetProcAddress("wglGetExtensionsStringARB");

	if (qwglGetExtensionsStringARB)
		glConfig.wglExtensionsString = qwglGetExtensionsStringARB(glwState.hDC);
	else
		glConfig.wglExtensionsString = "";

	// Get GLSL version string
	glConfig.shadingLanguageVersionString = (const char *)qglGetString(GL_SHADING_LANGUAGE_VERSION);

	// Get GL implementation limits
	qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize);
	qglGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &glConfig.max3DTextureSize);
	qglGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &glConfig.maxCubeMapTextureSize);
	qglGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &glConfig.maxArrayTextureLayers);
	qglGetIntegerv(GL_MAX_TEXTURE_UNITS, &glConfig.maxTextureUnits);
	qglGetIntegerv(GL_MAX_TEXTURE_COORDS, &glConfig.maxTextureCoords);
	qglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &glConfig.maxTextureImageUnits);
	qglGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &glConfig.maxVertexTextureImageUnits);
	qglGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &glConfig.maxCombinedTextureImageUnits);
	qglGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &glConfig.maxVertexAttribs);
	qglGetIntegerv(GL_MAX_VARYING_COMPONENTS, &glConfig.maxVaryingComponents);
	qglGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &glConfig.maxVertexUniformComponents);
	qglGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &glConfig.maxFragmentUniformComponents);
	qglGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &glConfig.maxColorAttachments);
	qglGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &glConfig.maxRenderbufferSize);
	qglGetFloatv(GL_MAX_TEXTURE_LOD_BIAS, &glConfig.maxTextureLODBias);

	// Get number of samples for multisample antialiasing
	qglGetIntegerv(GL_SAMPLES, &glConfig.multiSamples);

	// Determine GL hardware type
	if (Str_FindText(glConfig.vendorString, "ATI Technologies", false))
		glConfig.hardwareType = GLHW_ATI;
	else if (Str_FindText(glConfig.vendorString, "NVIDIA Corporation", false))
		glConfig.hardwareType = GLHW_NVIDIA;
	else
		glConfig.hardwareType = GLHW_GENERIC;

	// Initialize extensions
	GLW_InitExtensions();

	// Enable logging if requested
	QGL_EnableLogging(r_logFile->integerValue);

	// OpenGL fully initialized
	glConfig.initialized = true;
}

/*
 ==================
 GLW_Shutdown
 ==================
*/
void GLW_Shutdown (){

	Com_Printf("Shutting down OpenGL subsystem\n");

	if (glwState.hGLRC){
		Com_Printf("...wglMakeCurrent( NULL, NULL ): ");

		if (!qwglMakeCurrent(NULL, NULL))
			Com_Printf("failed\n");
		else
			Com_Printf("succeeded\n");
		
		Com_Printf("...deleting GL context: ");

		if (!qwglDeleteContext(glwState.hGLRC))
			Com_Printf("failed\n");
		else
			Com_Printf("succeeded\n");

		glwState.hGLRC = NULL;
	}

	if (glwState.hDC){
		if (glwState.supportsGamma){
			Com_Printf("...restoring gamma ramp: ");

			if (!SetDeviceGammaRamp(glwState.hDC, glwState.oldGammaRamp))
				Com_Printf("failed\n");
			else
				Com_Printf("succeeded\n");
		}

		Com_Printf("...releasing DC: ");

		if (!ReleaseDC(glwState.hWnd, glwState.hDC))
			Com_Printf("failed\n");
		else
			Com_Printf("succeeded\n");

		glwState.hDC = NULL;
	}

	if (glwState.hWnd){
		Com_Printf("...destroying window\n");

		ShowWindow(glwState.hWnd, SW_HIDE);
		DestroyWindow(glwState.hWnd);
		glwState.hWnd = NULL;

		UnregisterClass(WINDOW_CLASS_MAIN, sys.hInstance);
	}

	if (glConfig.isFullscreen){
		Com_Printf("...restoring display settings\n");

		ChangeDisplaySettings(NULL, CDS_FULLSCREEN);
		glConfig.isFullscreen = false;
	}

	QGL_Shutdown();

	Mem_Fill(&glConfig, 0, sizeof(glConfig_t));
	Mem_Fill(&glwState, 0, sizeof(glwState_t));
}