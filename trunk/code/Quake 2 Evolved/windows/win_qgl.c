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
// win_qgl.c - Binding of GL to QGL function pointers
//


#include "../renderer/r_local.h"
#include "win_local.h"


typedef struct {
	HMODULE					hModule;

	bool					pointersCopied;

	FILE *					logFile;
	char					logFileName[MAX_PATH_LENGTH];
	int						logFileFrames;
	int						logFileNumber;
} qglState_t;

static qglState_t			qglState;


// ============================================================================


int							(WINAPI * qwglChoosePixelFormat)(HDC hDC, CONST PIXELFORMATDESCRIPTOR *pPFD);
BOOL						(WINAPI * qwglCopyContext)(HGLRC hGLRCSrc, HGLRC hGLRCDst, UINT mask);
HGLRC						(WINAPI * qwglCreateContext)(HDC hDC);
HGLRC						(WINAPI * qwglCreateLayerContext)(HDC hDC, int iLayerPlane);
BOOL						(WINAPI * qwglDeleteContext)(HGLRC hGLRC);
BOOL						(WINAPI * qwglDescribeLayerPlane)(HDC hDC, int iPixelFormat, int iLayerPlane, UINT nBytes, LPLAYERPLANEDESCRIPTOR pLPD);
int							(WINAPI * qwglDescribePixelFormat)(HDC hDC, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR pPFD);
HGLRC						(WINAPI * qwglGetCurrentContext)(void);
HDC							(WINAPI * qwglGetCurrentDC)(void);
int							(WINAPI * qwglGetLayerPaletteEntries)(HDC hDC, int iLayerPlane, int iStart, int cEntries, COLORREF *pCR);
int							(WINAPI * qwglGetPixelFormat)(HDC hDC);
PROC						(WINAPI * qwglGetProcAddress)(LPCSTR lpszProc);
BOOL						(WINAPI * qwglMakeCurrent)(HDC hDC, HGLRC hGLRC);
BOOL						(WINAPI * qwglRealizeLayerPalette)(HDC hDC, int iLayerPlane, BOOL bRealize);
int							(WINAPI * qwglSetLayerPaletteEntries)(HDC hDC, int iLayerPlane, int iStart, int cEntries, CONST COLORREF *pCR);
BOOL						(WINAPI * qwglSetPixelFormat)(HDC hDC, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *pPFD);
BOOL						(WINAPI * qwglShareLists)(HGLRC hGLRC1, HGLRC hGLRC2);
BOOL						(WINAPI * qwglSwapBuffers)(HDC hDC);
BOOL						(WINAPI * qwglSwapLayerBuffers)(HDC hDC, UINT fuPlanes);
BOOL						(WINAPI * qwglUseFontBitmaps)(HDC hDC, DWORD first, DWORD count, DWORD listBase);
BOOL						(WINAPI * qwglUseFontOutlines)(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpGMF);

BOOL						(WINAPI * qwglSwapIntervalEXT)(int interval);

GLvoid						(APIENTRY * qglAccum)(GLenum op, GLfloat value);
GLvoid						(APIENTRY * qglActiveStencilFaceEXT)(GLenum face);
GLvoid						(APIENTRY * qglActiveTexture)(GLenum texture);
GLvoid						(APIENTRY * qglAlphaFunc)(GLenum func, GLclampf ref);
GLboolean					(APIENTRY * qglAreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences);
GLvoid						(APIENTRY * qglArrayElement)(GLint i);
GLvoid						(APIENTRY * qglAttachShader)(GLuint program, GLuint shader);
GLvoid						(APIENTRY * qglBegin)(GLenum mode);
GLvoid						(APIENTRY * qglBeginConditionalRender)(GLuint id, GLenum mode);
GLvoid						(APIENTRY * qglBeginQuery)(GLenum target, GLuint id);
GLvoid						(APIENTRY * qglBeginTransformFeedback)(GLenum primitiveMode);
GLvoid						(APIENTRY * qglBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
GLvoid						(APIENTRY * qglBindBuffer)(GLenum target, GLuint buffer);
GLvoid						(APIENTRY * qglBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
GLvoid						(APIENTRY * qglBindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
GLvoid						(APIENTRY * qglBindFragDataLocation)(GLuint program, GLuint color, const GLchar *name);
GLvoid						(APIENTRY * qglBindFragDataLocationIndexed)(GLuint program, GLuint color, GLuint index, const GLchar *name);
GLvoid						(APIENTRY * qglBindFramebuffer)(GLenum target, GLuint framebuffer);
GLvoid						(APIENTRY * qglBindRenderbuffer)(GLenum target, GLuint renderbuffer);
GLvoid						(APIENTRY * qglBindSampler)(GLuint unit, GLuint sampler);
GLvoid						(APIENTRY * qglBindTexture)(GLenum target, GLuint texture);
GLvoid						(APIENTRY * qglBindVertexArray)(GLuint vertexArray);
GLvoid						(APIENTRY * qglBitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
GLvoid						(APIENTRY * qglBlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
GLvoid						(APIENTRY * qglBlendEquation)(GLenum mode);
GLvoid						(APIENTRY * qglBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
GLvoid						(APIENTRY * qglBlendFunc)(GLenum sfactor, GLenum dfactor);
GLvoid						(APIENTRY * qglBlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
GLvoid						(APIENTRY * qglBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
GLvoid						(APIENTRY * qglBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
GLvoid						(APIENTRY * qglBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
GLvoid						(APIENTRY * qglCallList)(GLuint list);
GLvoid						(APIENTRY * qglCallLists)(GLsizei n, GLenum type, const GLvoid *lists);
GLenum						(APIENTRY * qglCheckFramebufferStatus)(GLenum target);
GLvoid						(APIENTRY * qglClampColor)(GLenum target, GLenum clamp);
GLvoid						(APIENTRY * qglClear)(GLbitfield mask);
GLvoid						(APIENTRY * qglClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLvoid						(APIENTRY * qglClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
GLvoid						(APIENTRY * qglClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat *value);
GLvoid						(APIENTRY * qglClearBufferiv)(GLenum buffer, GLint drawbuffer, const GLint *value);
GLvoid						(APIENTRY * qglClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint *value);
GLvoid						(APIENTRY * qglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
GLvoid						(APIENTRY * qglClearDepth)(GLclampd depth);
GLvoid						(APIENTRY * qglClearIndex)(GLfloat c);
GLvoid						(APIENTRY * qglClearStencil)(GLint s);
GLvoid						(APIENTRY * qglClientActiveTexture)(GLenum texture);
GLenum						(APIENTRY * qglClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
GLvoid						(APIENTRY * qglClipPlane)(GLenum plane, const GLdouble *equation);
GLvoid						(APIENTRY * qglColor3b)(GLbyte red, GLbyte green, GLbyte blue);
GLvoid						(APIENTRY * qglColor3bv)(const GLbyte *v);
GLvoid						(APIENTRY * qglColor3d)(GLdouble red, GLdouble green, GLdouble blue);
GLvoid						(APIENTRY * qglColor3dv)(const GLdouble *v);
GLvoid						(APIENTRY * qglColor3f)(GLfloat red, GLfloat green, GLfloat blue);
GLvoid						(APIENTRY * qglColor3fv)(const GLfloat *v);
GLvoid						(APIENTRY * qglColor3i)(GLint red, GLint green, GLint blue);
GLvoid						(APIENTRY * qglColor3iv)(const GLint *v);
GLvoid						(APIENTRY * qglColor3s)(GLshort red, GLshort green, GLshort blue);
GLvoid						(APIENTRY * qglColor3sv)(const GLshort *v);
GLvoid						(APIENTRY * qglColor3ub)(GLubyte red, GLubyte green, GLubyte blue);
GLvoid						(APIENTRY * qglColor3ubv)(const GLubyte *v);
GLvoid						(APIENTRY * qglColor3ui)(GLuint red, GLuint green, GLuint blue);
GLvoid						(APIENTRY * qglColor3uiv)(const GLuint *v);
GLvoid						(APIENTRY * qglColor3us)(GLushort red, GLushort green, GLushort blue);
GLvoid						(APIENTRY * qglColor3usv)(const GLushort *v);
GLvoid						(APIENTRY * qglColor4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
GLvoid						(APIENTRY * qglColor4bv)(const GLbyte *v);
GLvoid						(APIENTRY * qglColor4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
GLvoid						(APIENTRY * qglColor4dv)(const GLdouble *v);
GLvoid						(APIENTRY * qglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GLvoid						(APIENTRY * qglColor4fv)(const GLfloat *v);
GLvoid						(APIENTRY * qglColor4i)(GLint red, GLint green, GLint blue, GLint alpha);
GLvoid						(APIENTRY * qglColor4iv)(const GLint *v);
GLvoid						(APIENTRY * qglColor4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha);
GLvoid						(APIENTRY * qglColor4sv)(const GLshort *v);
GLvoid						(APIENTRY * qglColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
GLvoid						(APIENTRY * qglColor4ubv)(const GLubyte *v);
GLvoid						(APIENTRY * qglColor4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
GLvoid						(APIENTRY * qglColor4uiv)(const GLuint *v);
GLvoid						(APIENTRY * qglColor4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha);
GLvoid						(APIENTRY * qglColor4usv)(const GLushort *v);
GLvoid						(APIENTRY * qglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
GLvoid						(APIENTRY * qglColorMaski)(GLuint index, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
GLvoid						(APIENTRY * qglColorMaterial)(GLenum face, GLenum mode);
GLvoid						(APIENTRY * qglColorP3ui)(GLenum type, GLuint v);
GLvoid						(APIENTRY * qglColorP3uiv)(GLenum type, const GLuint *v);
GLvoid						(APIENTRY * qglColorP4ui)(GLenum type, GLuint v);
GLvoid						(APIENTRY * qglColorP4uiv)(GLenum type, const GLuint *v);
GLvoid						(APIENTRY * qglColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid						(APIENTRY * qglCompileShader)(GLuint shader);
GLvoid						(APIENTRY * qglCompressedTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *pixels);
GLvoid						(APIENTRY * qglCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *pixels);
GLvoid						(APIENTRY * qglCompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *pixels);
GLvoid						(APIENTRY * qglCompressedTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *pixels);
GLvoid						(APIENTRY * qglCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *pixels);
GLvoid						(APIENTRY * qglCompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *pixels);
GLvoid						(APIENTRY * qglCopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
GLvoid						(APIENTRY * qglCopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
GLvoid						(APIENTRY * qglCopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
GLvoid						(APIENTRY * qglCopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
GLvoid						(APIENTRY * qglCopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
GLvoid						(APIENTRY * qglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GLvoid						(APIENTRY * qglCopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GLuint						(APIENTRY * qglCreateProgram)(GLvoid);
GLuint						(APIENTRY * qglCreateShader)(GLenum type);
GLvoid						(APIENTRY * qglCullFace)(GLenum mode);
GLvoid						(APIENTRY * qglDeleteBuffers)(GLsizei n, const GLuint *buffers);
GLvoid						(APIENTRY * qglDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
GLvoid						(APIENTRY * qglDeleteLists)(GLuint list, GLsizei range);
GLvoid						(APIENTRY * qglDeleteProgram)(GLuint program);
GLvoid						(APIENTRY * qglDeleteQueries)(GLsizei n, const GLuint *ids);
GLvoid						(APIENTRY * qglDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
GLvoid						(APIENTRY * qglDeleteSamplers)(GLsizei n, const GLuint *samplers);
GLvoid						(APIENTRY * qglDeleteShader)(GLuint shader);
GLvoid						(APIENTRY * qglDeleteSync)(GLsync sync);
GLvoid						(APIENTRY * qglDeleteTextures)(GLsizei n, const GLuint *textures);
GLvoid						(APIENTRY * qglDeleteVertexArrays)(GLsizei n, const GLuint *vertexArrays);
GLvoid						(APIENTRY * qglDepthBoundsEXT)(GLclampd zmin, GLclampd zmax);
GLvoid						(APIENTRY * qglDepthFunc)(GLenum func);
GLvoid						(APIENTRY * qglDepthMask)(GLboolean flag);
GLvoid						(APIENTRY * qglDepthRange)(GLclampd zNear, GLclampd zFar);
GLvoid						(APIENTRY * qglDetachShader)(GLuint program, GLuint shader);
GLvoid						(APIENTRY * qglDisable)(GLenum cap);
GLvoid					(APIENTRY * qglDisableClientState)(GLenum array);
GLvoid					(APIENTRY * qglDisableVertexAttribArray)(GLuint index);
GLvoid					(APIENTRY * qglDisablei)(GLenum target, GLuint index);
GLvoid					(APIENTRY * qglDrawArrays)(GLenum mode, GLint first, GLsizei count);
GLvoid					(APIENTRY * qglDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
GLvoid					(APIENTRY * qglDrawBuffer)(GLenum buffer);
GLvoid					(APIENTRY * qglDrawBuffers)(GLsizei n, const GLenum *buffers);
GLvoid					(APIENTRY * qglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
GLvoid					(APIENTRY * qglDrawElementsBaseVertex)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLint basevertex);
GLvoid					(APIENTRY * qglDrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount);
GLvoid					(APIENTRY * qglDrawElementsInstancedBaseVertex)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount, GLint basevertex);
GLvoid					(APIENTRY * qglDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid					(APIENTRY * qglDrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
GLvoid					(APIENTRY * qglDrawRangeElementsBaseVertex)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices, GLint basevertex);
GLvoid					(APIENTRY * qglEdgeFlag)(GLboolean flag);
GLvoid					(APIENTRY * qglEdgeFlagPointer)(GLsizei stride, const GLvoid *pointer);
GLvoid					(APIENTRY * qglEdgeFlagv)(const GLboolean *flag);
GLvoid					(APIENTRY * qglEnable)(GLenum cap);
GLvoid					(APIENTRY * qglEnableClientState)(GLenum array);
GLvoid					(APIENTRY * qglEnableVertexAttribArray)(GLuint index);
GLvoid					(APIENTRY * qglEnablei)(GLenum target, GLuint index);
GLvoid					(APIENTRY * qglEnd)(GLvoid);
GLvoid					(APIENTRY * qglEndConditionalRender)(GLvoid);
GLvoid					(APIENTRY * qglEndList)(GLvoid);
GLvoid					(APIENTRY * qglEndQuery)(GLenum target);
GLvoid					(APIENTRY * qglEndTransformFeedback)(GLvoid);
GLvoid					(APIENTRY * qglEvalCoord1d)(GLdouble u);
GLvoid					(APIENTRY * qglEvalCoord1dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglEvalCoord1f)(GLfloat u);
GLvoid					(APIENTRY * qglEvalCoord1fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglEvalCoord2d)(GLdouble u, GLdouble v);
GLvoid					(APIENTRY * qglEvalCoord2dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglEvalCoord2f)(GLfloat u, GLfloat v);
GLvoid					(APIENTRY * qglEvalCoord2fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglEvalMesh1)(GLenum mode, GLint i1, GLint i2);
GLvoid					(APIENTRY * qglEvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
GLvoid					(APIENTRY * qglEvalPoint1)(GLint i);
GLvoid					(APIENTRY * qglEvalPoint2)(GLint i, GLint j);
GLvoid					(APIENTRY * qglFeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer);
GLsync					(APIENTRY * qglFenceSync)(GLenum condition, GLbitfield flags);
GLvoid					(APIENTRY * qglFinish)(GLvoid);
GLvoid					(APIENTRY * qglFlush)(GLvoid);
GLvoid					(APIENTRY * qglFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length);
GLvoid					(APIENTRY * qglFogCoordPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid					(APIENTRY * qglFogCoordd)(GLdouble coord);
GLvoid					(APIENTRY * qglFogCoorddv)(const GLdouble *v);
GLvoid					(APIENTRY * qglFogCoordf)(GLfloat coord);
GLvoid					(APIENTRY * qglFogCoordfv)(const GLfloat *v);
GLvoid					(APIENTRY * qglFogf)(GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglFogfv)(GLenum pname, const GLfloat *params);
GLvoid					(APIENTRY * qglFogi)(GLenum pname, GLint param);
GLvoid					(APIENTRY * qglFogiv)(GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GLvoid					(APIENTRY * qglFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
GLvoid					(APIENTRY * qglFramebufferTexture1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLvoid					(APIENTRY * qglFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLvoid					(APIENTRY * qglFramebufferTexture3D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);
GLvoid					(APIENTRY * qglFramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
GLvoid					(APIENTRY * qglFrontFace)(GLenum mode);
GLvoid					(APIENTRY * qglFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLvoid					(APIENTRY * qglGenBuffers)(GLsizei n, GLuint *buffers);
GLvoid					(APIENTRY * qglGenFramebuffers)(GLsizei n, GLuint *framebuffers);
GLuint					(APIENTRY * qglGenLists)(GLsizei range);
GLvoid					(APIENTRY * qglGenQueries)(GLsizei n, GLuint *ids);
GLvoid					(APIENTRY * qglGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
GLvoid					(APIENTRY * qglGenSamplers)(GLsizei n, GLuint *samplers);
GLvoid					(APIENTRY * qglGenTextures)(GLsizei n, GLuint *textures);
GLvoid					(APIENTRY * qglGenVertexArrays)(GLsizei n, GLuint *vertexArrays);
GLvoid					(APIENTRY * qglGenerateMipmap)(GLenum target);
GLvoid					(APIENTRY * qglGetActiveAttrib)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
GLvoid					(APIENTRY * qglGetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
GLvoid					(APIENTRY * qglGetActiveUniformBlockName)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
GLvoid					(APIENTRY * qglGetActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetActiveUniformName)(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName);
GLvoid					(APIENTRY * qglGetActiveUniformsiv)(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetAttachedShaders)(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
GLint					(APIENTRY * qglGetAttribLocation)(GLuint program, const GLchar *name);
GLvoid					(APIENTRY * qglGetBooleani_v)(GLenum target, GLuint index, GLboolean *params);
GLvoid					(APIENTRY * qglGetBooleanv)(GLenum pname, GLboolean *params);
GLvoid					(APIENTRY * qglGetBufferParameteri64v)(GLenum target, GLenum pname, GLint64 *params);
GLvoid					(APIENTRY * qglGetBufferParameteriv)(GLenum target, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetBufferPointerv)(GLenum target, GLenum pname, GLvoid **params);
GLvoid					(APIENTRY * qglGetBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid *data);
GLvoid					(APIENTRY * qglGetClipPlane)(GLenum plane, GLdouble *equation);
GLvoid					(APIENTRY * qglGetCompressedTexImage)(GLenum target, GLint level, GLvoid *pixels);
GLvoid					(APIENTRY * qglGetDoublev)(GLenum pname, GLdouble *params);
GLenum					(APIENTRY * qglGetError)(GLvoid);
GLvoid					(APIENTRY * qglGetFloatv)(GLenum pname, GLfloat *params);
GLint					(APIENTRY * qglGetFragDataIndex)(GLuint program, const GLchar *name);
GLint					(APIENTRY * qglGetFragDataLocation)(GLuint program, const GLchar *name);
GLvoid					(APIENTRY * qglGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetInteger64i_v)(GLenum target, GLuint index, GLint64 *params);
GLvoid					(APIENTRY * qglGetInteger64v)(GLenum pname, GLint64 *params);
GLvoid					(APIENTRY * qglGetIntegeri_v)(GLenum target, GLuint index, GLint *params);
GLvoid					(APIENTRY * qglGetIntegerv)(GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetLightfv)(GLenum light, GLenum pname, GLfloat *params);
GLvoid					(APIENTRY * qglGetLightiv)(GLenum light, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetMapdv)(GLenum target, GLenum query, GLdouble *v);
GLvoid					(APIENTRY * qglGetMapfv)(GLenum target, GLenum query, GLfloat *v);
GLvoid					(APIENTRY * qglGetMapiv)(GLenum target, GLenum query, GLint *v);
GLvoid					(APIENTRY * qglGetMaterialfv)(GLenum face, GLenum pname, GLfloat *params);
GLvoid					(APIENTRY * qglGetMaterialiv)(GLenum face, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetMultisamplefv)(GLenum pname, GLuint index, GLfloat *params);
GLvoid					(APIENTRY * qglGetPixelMapfv)(GLenum map, GLfloat *values);
GLvoid					(APIENTRY * qglGetPixelMapuiv)(GLenum map, GLuint *values);
GLvoid					(APIENTRY * qglGetPixelMapusv)(GLenum map, GLushort *values);
GLvoid					(APIENTRY * qglGetPointerv)(GLenum pname, GLvoid **params);
GLvoid					(APIENTRY * qglGetPolygonStipple)(GLubyte *mask);
GLvoid					(APIENTRY * qglGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLvoid					(APIENTRY * qglGetProgramiv)(GLuint program, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetQueryObjecti64v)(GLuint id, GLenum pname, GLint64 *params);
GLvoid					(APIENTRY * qglGetQueryObjectiv)(GLuint id, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetQueryObjectui64v)(GLuint id, GLenum pname, GLuint64 *params);
GLvoid					(APIENTRY * qglGetQueryObjectuiv)(GLuint id, GLenum pname, GLuint *params);
GLvoid					(APIENTRY * qglGetQueryiv)(GLenum target, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetSamplerParameterIiv)(GLuint sampler, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetSamplerParameterIuiv)(GLuint sampler, GLenum pname, GLuint *params);
GLvoid					(APIENTRY * qglGetSamplerParameterfv)(GLuint sampler, GLenum pname, GLfloat *params);
GLvoid					(APIENTRY * qglGetSamplerParameteriv)(GLuint sampler, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
GLvoid					(APIENTRY * qglGetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
GLvoid					(APIENTRY * qglGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
const GLubyte *			(APIENTRY * qglGetString)(GLenum name);
const GLubyte *			(APIENTRY * qglGetStringi)(GLenum name, GLuint index);
GLvoid					(APIENTRY * qglGetSynciv)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
GLvoid					(APIENTRY * qglGetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params);
GLvoid					(APIENTRY * qglGetTexEnviv)(GLenum target, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetTexGendv)(GLenum coord, GLenum pname, GLdouble *params);
GLvoid					(APIENTRY * qglGetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params);
GLvoid					(APIENTRY * qglGetTexGeniv)(GLenum coord, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
GLvoid					(APIENTRY * qglGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
GLvoid					(APIENTRY * qglGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetTexParameterIiv)(GLenum target, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetTexParameterIuiv)(GLenum target, GLenum pname, GLuint *params);
GLvoid					(APIENTRY * qglGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
GLvoid					(APIENTRY * qglGetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetTransformFeedbackVarying)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
GLuint					(APIENTRY * qglGetUniformBlockIndex)(GLuint program, const GLchar *uniformBlockName);
GLvoid					(APIENTRY * qglGetUniformIndices)(GLuint program, GLsizei uniformCount, const GLchar **uniformNames, GLuint *uniformIndices);
GLint					(APIENTRY * qglGetUniformLocation)(GLuint program, const GLchar *name);
GLvoid					(APIENTRY * qglGetUniformfv)(GLuint program, GLint location, GLfloat *params);
GLvoid					(APIENTRY * qglGetUniformiv)(GLuint program, GLint location, GLint *params);
GLvoid					(APIENTRY * qglGetUniformuiv)(GLuint program, GLint location, GLuint *params);
GLvoid					(APIENTRY * qglGetVertexAttribIiv)(GLuint index, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglGetVertexAttribIuiv)(GLuint index, GLenum pname, GLuint *params);
GLvoid					(APIENTRY * qglGetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid **params);
GLvoid					(APIENTRY * qglGetVertexAttribdv)(GLuint index, GLenum pname, GLdouble *params);
GLvoid					(APIENTRY * qglGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat *params);
GLvoid					(APIENTRY * qglGetVertexAttribiv)(GLuint index, GLenum pname, GLint *params);
GLvoid					(APIENTRY * qglHint)(GLenum target, GLenum mode);
GLvoid					(APIENTRY * qglIndexMask)(GLuint mask);
GLvoid					(APIENTRY * qglIndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid					(APIENTRY * qglIndexd)(GLdouble c);
GLvoid					(APIENTRY * qglIndexdv)(const GLdouble *c);
GLvoid					(APIENTRY * qglIndexf)(GLfloat c);
GLvoid					(APIENTRY * qglIndexfv)(const GLfloat *c);
GLvoid					(APIENTRY * qglIndexi)(GLint c);
GLvoid					(APIENTRY * qglIndexiv)(const GLint *c);
GLvoid					(APIENTRY * qglIndexs)(GLshort c);
GLvoid					(APIENTRY * qglIndexsv)(const GLshort *c);
GLvoid					(APIENTRY * qglIndexub)(GLubyte c);
GLvoid					(APIENTRY * qglIndexubv)(const GLubyte *c);
GLvoid					(APIENTRY * qglInitNames)(GLvoid);
GLvoid					(APIENTRY * qglInterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean				(APIENTRY * qglIsBuffer)(GLuint buffer);
GLboolean				(APIENTRY * qglIsEnabled)(GLenum cap);
GLboolean				(APIENTRY * qglIsEnabledi)(GLenum target, GLuint index);
GLboolean				(APIENTRY * qglIsFramebuffer)(GLuint framebuffer);
GLboolean				(APIENTRY * qglIsList)(GLuint list);
GLboolean				(APIENTRY * qglIsProgram)(GLuint program);
GLboolean				(APIENTRY * qglIsQuery)(GLuint id);
GLboolean				(APIENTRY * qglIsRenderbuffer)(GLuint renderbuffer);
GLboolean				(APIENTRY * qglIsSampler)(GLuint sampler);
GLboolean				(APIENTRY * qglIsShader)(GLuint shader);
GLboolean				(APIENTRY * qglIsSync)(GLsync sync);
GLboolean				(APIENTRY * qglIsTexture)(GLuint texture);
GLboolean				(APIENTRY * qglIsVertexArray)(GLuint vertexArray);
GLvoid					(APIENTRY * qglLightModelf)(GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglLightModelfv)(GLenum pname, const GLfloat *params);
GLvoid					(APIENTRY * qglLightModeli)(GLenum pname, GLint param);
GLvoid					(APIENTRY * qglLightModeliv)(GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglLightf)(GLenum light, GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglLightfv)(GLenum light, GLenum pname, const GLfloat *params);
GLvoid					(APIENTRY * qglLighti)(GLenum light, GLenum pname, GLint param);
GLvoid					(APIENTRY * qglLightiv)(GLenum light, GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglLineStipple)(GLint factor, GLushort pattern);
GLvoid					(APIENTRY * qglLineWidth)(GLfloat width);
GLvoid					(APIENTRY * qglLinkProgram)(GLuint program);
GLvoid					(APIENTRY * qglListBase)(GLuint base);
GLvoid					(APIENTRY * qglLoadIdentity)(GLvoid);
GLvoid					(APIENTRY * qglLoadMatrixd)(const GLdouble *m);
GLvoid					(APIENTRY * qglLoadMatrixf)(const GLfloat *m);
GLvoid					(APIENTRY * qglLoadName)(GLuint name);
GLvoid					(APIENTRY * qglLoadTransposeMatrixd)(const GLdouble *m);
GLvoid					(APIENTRY * qglLoadTransposeMatrixf)(const GLfloat *m);
GLvoid					(APIENTRY * qglLogicOp)(GLenum opcode);
GLvoid					(APIENTRY * qglMap1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
GLvoid					(APIENTRY * qglMap1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
GLvoid					(APIENTRY * qglMap2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
GLvoid					(APIENTRY * qglMap2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
GLvoid *				(APIENTRY * qglMapBuffer)(GLenum target, GLenum access);
GLvoid *				(APIENTRY * qglMapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
GLvoid					(APIENTRY * qglMapGrid1d)(GLint un, GLdouble u1, GLdouble u2);
GLvoid					(APIENTRY * qglMapGrid1f)(GLint un, GLfloat u1, GLfloat u2);
GLvoid					(APIENTRY * qglMapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
GLvoid					(APIENTRY * qglMapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
GLvoid					(APIENTRY * qglMaterialf)(GLenum face, GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
GLvoid					(APIENTRY * qglMateriali)(GLenum face, GLenum pname, GLint param);
GLvoid					(APIENTRY * qglMaterialiv)(GLenum face, GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglMatrixMode)(GLenum mode);
GLvoid					(APIENTRY * qglMultMatrixd)(const GLdouble *m);
GLvoid					(APIENTRY * qglMultMatrixf)(const GLfloat *m);
GLvoid					(APIENTRY * qglMultTransposeMatrixd)(const GLdouble *m);
GLvoid					(APIENTRY * qglMultTransposeMatrixf)(const GLfloat *m);
GLvoid					(APIENTRY * qglMultiDrawArrays)(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
GLvoid					(APIENTRY * qglMultiDrawElements)(GLenum mode, const GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount);
GLvoid					(APIENTRY * qglMultiDrawElementsBaseVertex)(GLenum mode, const GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount, const GLint *basevertex);
GLvoid					(APIENTRY * qglMultiTexCoord1d)(GLenum target, GLdouble s);
GLvoid					(APIENTRY * qglMultiTexCoord1dv)(GLenum target, const GLdouble *v);
GLvoid					(APIENTRY * qglMultiTexCoord1f)(GLenum target, GLfloat s);
GLvoid					(APIENTRY * qglMultiTexCoord1fv)(GLenum target, const GLfloat *v);
GLvoid					(APIENTRY * qglMultiTexCoord1i)(GLenum target, GLint s);
GLvoid					(APIENTRY * qglMultiTexCoord1iv)(GLenum target, const GLint *v);
GLvoid					(APIENTRY * qglMultiTexCoord1s)(GLenum target, GLshort s);
GLvoid					(APIENTRY * qglMultiTexCoord1sv)(GLenum target, const GLshort *v);
GLvoid					(APIENTRY * qglMultiTexCoord2d)(GLenum target, GLdouble s, GLdouble t);
GLvoid					(APIENTRY * qglMultiTexCoord2dv)(GLenum target, const GLdouble *v);
GLvoid					(APIENTRY * qglMultiTexCoord2f)(GLenum target, GLfloat s, GLfloat t);
GLvoid					(APIENTRY * qglMultiTexCoord2fv)(GLenum target, const GLfloat *v);
GLvoid					(APIENTRY * qglMultiTexCoord2i)(GLenum target, GLint s, GLint t);
GLvoid					(APIENTRY * qglMultiTexCoord2iv)(GLenum target, const GLint *v);
GLvoid					(APIENTRY * qglMultiTexCoord2s)(GLenum target, GLshort s, GLshort t);
GLvoid					(APIENTRY * qglMultiTexCoord2sv)(GLenum target, const GLshort *v);
GLvoid					(APIENTRY * qglMultiTexCoord3d)(GLenum target, GLdouble s, GLdouble t, GLdouble r);
GLvoid					(APIENTRY * qglMultiTexCoord3dv)(GLenum target, const GLdouble *v);
GLvoid					(APIENTRY * qglMultiTexCoord3f)(GLenum target, GLfloat s, GLfloat t, GLfloat r);
GLvoid					(APIENTRY * qglMultiTexCoord3fv)(GLenum target, const GLfloat *v);
GLvoid					(APIENTRY * qglMultiTexCoord3i)(GLenum target, GLint s, GLint t, GLint r);
GLvoid					(APIENTRY * qglMultiTexCoord3iv)(GLenum target, const GLint *v);
GLvoid					(APIENTRY * qglMultiTexCoord3s)(GLenum target, GLshort s, GLshort t, GLshort r);
GLvoid					(APIENTRY * qglMultiTexCoord3sv)(GLenum target, const GLshort *v);
GLvoid					(APIENTRY * qglMultiTexCoord4d)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GLvoid					(APIENTRY * qglMultiTexCoord4dv)(GLenum target, const GLdouble *v);
GLvoid					(APIENTRY * qglMultiTexCoord4f)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GLvoid					(APIENTRY * qglMultiTexCoord4fv)(GLenum target, const GLfloat *v);
GLvoid					(APIENTRY * qglMultiTexCoord4i)(GLenum target, GLint s, GLint t, GLint r, GLint q);
GLvoid					(APIENTRY * qglMultiTexCoord4iv)(GLenum target, const GLint *v);
GLvoid					(APIENTRY * qglMultiTexCoord4s)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
GLvoid					(APIENTRY * qglMultiTexCoord4sv)(GLenum target, const GLshort *v);
GLvoid					(APIENTRY * qglMultiTexCoordP1ui)(GLenum target, GLenum type, GLuint v);
GLvoid					(APIENTRY * qglMultiTexCoordP1uiv)(GLenum target, GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglMultiTexCoordP2ui)(GLenum target, GLenum type, GLuint v);
GLvoid					(APIENTRY * qglMultiTexCoordP2uiv)(GLenum target, GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglMultiTexCoordP3ui)(GLenum target, GLenum type, GLuint v);
GLvoid					(APIENTRY * qglMultiTexCoordP3uiv)(GLenum target, GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglMultiTexCoordP4ui)(GLenum target, GLenum type, GLuint v);
GLvoid					(APIENTRY * qglMultiTexCoordP4uiv)(GLenum target, GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglNewList)(GLuint list, GLenum mode);
GLvoid					(APIENTRY * qglNormal3b)(GLbyte nx, GLbyte ny, GLbyte nz);
GLvoid					(APIENTRY * qglNormal3bv)(const GLbyte *v);
GLvoid					(APIENTRY * qglNormal3d)(GLdouble nx, GLdouble ny, GLdouble nz);
GLvoid					(APIENTRY * qglNormal3dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
GLvoid					(APIENTRY * qglNormal3fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglNormal3i)(GLint nx, GLint ny, GLint nz);
GLvoid					(APIENTRY * qglNormal3iv)(const GLint *v);
GLvoid					(APIENTRY * qglNormal3s)(GLshort nx, GLshort ny, GLshort nz);
GLvoid					(APIENTRY * qglNormal3sv)(const GLshort *v);
GLvoid					(APIENTRY * qglNormalP3ui)(GLenum type, GLuint v);
GLvoid					(APIENTRY * qglNormalP3uiv)(GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid					(APIENTRY * qglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLvoid					(APIENTRY * qglPassThrough)(GLfloat token);
GLvoid					(APIENTRY * qglPixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat *values);
GLvoid					(APIENTRY * qglPixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint *values);
GLvoid					(APIENTRY * qglPixelMapusv)(GLenum map, GLsizei mapsize, const GLushort *values);
GLvoid					(APIENTRY * qglPixelStoref)(GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglPixelStorei)(GLenum pname, GLint param);
GLvoid					(APIENTRY * qglPixelTransferf)(GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglPixelTransferi)(GLenum pname, GLint param);
GLvoid					(APIENTRY * qglPixelZoom)(GLfloat xfactor, GLfloat yfactor);
GLvoid					(APIENTRY * qglPointParameterf)(GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglPointParameterfv)(GLenum pname, const GLfloat *params);
GLvoid					(APIENTRY * qglPointParameteri)(GLenum pname, GLint param);
GLvoid					(APIENTRY * qglPointParameteriv)(GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglPointSize)(GLfloat size);
GLvoid					(APIENTRY * qglPolygonMode)(GLenum face, GLenum mode);
GLvoid					(APIENTRY * qglPolygonOffset)(GLfloat factor, GLfloat units);
GLvoid					(APIENTRY * qglPolygonStipple)(const GLubyte *mask);
GLvoid					(APIENTRY * qglPopAttrib)(GLvoid);
GLvoid					(APIENTRY * qglPopClientAttrib)(GLvoid);
GLvoid					(APIENTRY * qglPopMatrix)(GLvoid);
GLvoid					(APIENTRY * qglPopName)(GLvoid);
GLvoid					(APIENTRY * qglPrimitiveRestartIndex)(GLuint index);
GLvoid					(APIENTRY * qglPrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
GLvoid					(APIENTRY * qglProvokingVertex)(GLenum mode);
GLvoid					(APIENTRY * qglPushAttrib)(GLbitfield mask);
GLvoid					(APIENTRY * qglPushClientAttrib)(GLbitfield mask);
GLvoid					(APIENTRY * qglPushMatrix)(GLvoid);
GLvoid					(APIENTRY * qglPushName)(GLuint name);
GLvoid					(APIENTRY * qglQueryCounter)(GLuint id, GLenum target);
GLvoid					(APIENTRY * qglRasterPos2d)(GLdouble x, GLdouble y);
GLvoid					(APIENTRY * qglRasterPos2dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglRasterPos2f)(GLfloat x, GLfloat y);
GLvoid					(APIENTRY * qglRasterPos2fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglRasterPos2i)(GLint x, GLint y);
GLvoid					(APIENTRY * qglRasterPos2iv)(const GLint *v);
GLvoid					(APIENTRY * qglRasterPos2s)(GLshort x, GLshort y);
GLvoid					(APIENTRY * qglRasterPos2sv)(const GLshort *v);
GLvoid					(APIENTRY * qglRasterPos3d)(GLdouble x, GLdouble y, GLdouble z);
GLvoid					(APIENTRY * qglRasterPos3dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglRasterPos3f)(GLfloat x, GLfloat y, GLfloat z);
GLvoid					(APIENTRY * qglRasterPos3fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglRasterPos3i)(GLint x, GLint y, GLint z);
GLvoid					(APIENTRY * qglRasterPos3iv)(const GLint *v);
GLvoid					(APIENTRY * qglRasterPos3s)(GLshort x, GLshort y, GLshort z);
GLvoid					(APIENTRY * qglRasterPos3sv)(const GLshort *v);
GLvoid					(APIENTRY * qglRasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GLvoid					(APIENTRY * qglRasterPos4dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglRasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLvoid					(APIENTRY * qglRasterPos4fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglRasterPos4i)(GLint x, GLint y, GLint z, GLint w);
GLvoid					(APIENTRY * qglRasterPos4iv)(const GLint *v);
GLvoid					(APIENTRY * qglRasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w);
GLvoid					(APIENTRY * qglRasterPos4sv)(const GLshort *v);
GLvoid					(APIENTRY * qglReadBuffer)(GLenum buffer);
GLvoid					(APIENTRY * qglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
GLvoid					(APIENTRY * qglRectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
GLvoid					(APIENTRY * qglRectdv)(const GLdouble *v1, const GLdouble *v2);
GLvoid					(APIENTRY * qglRectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
GLvoid					(APIENTRY * qglRectfv)(const GLfloat *v1, const GLfloat *v2);
GLvoid					(APIENTRY * qglRecti)(GLint x1, GLint y1, GLint x2, GLint y2);
GLvoid					(APIENTRY * qglRectiv)(const GLint *v1, const GLint *v2);
GLvoid					(APIENTRY * qglRects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
GLvoid					(APIENTRY * qglRectsv)(const GLshort *v1, const GLshort *v2);
GLint					(APIENTRY * qglRenderMode)(GLenum mode);
GLvoid					(APIENTRY * qglRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GLvoid					(APIENTRY * qglRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
GLvoid					(APIENTRY * qglRotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
GLvoid					(APIENTRY * qglRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
GLvoid					(APIENTRY * qglSampleCoverage)(GLclampf value, GLboolean invert);
GLvoid					(APIENTRY * qglSampleMaski)(GLuint index, GLbitfield mask);
GLvoid					(APIENTRY * qglSamplerParameterIiv)(GLuint sampler, GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglSamplerParameterIuiv)(GLuint sampler, GLenum pname, const GLuint *params);
GLvoid					(APIENTRY * qglSamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglSamplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat *params);
GLvoid					(APIENTRY * qglSamplerParameteri)(GLuint sampler, GLenum pname, GLint param);
GLvoid					(APIENTRY * qglSamplerParameteriv)(GLuint sampler, GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglScaled)(GLdouble x, GLdouble y, GLdouble z);
GLvoid					(APIENTRY * qglScalef)(GLfloat x, GLfloat y, GLfloat z);
GLvoid					(APIENTRY * qglScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
GLvoid					(APIENTRY * qglSecondaryColor3b)(GLbyte red, GLbyte green, GLbyte blue);
GLvoid					(APIENTRY * qglSecondaryColor3bv)(const GLbyte *v);
GLvoid					(APIENTRY * qglSecondaryColor3d)(GLdouble red, GLdouble green, GLdouble blue);
GLvoid					(APIENTRY * qglSecondaryColor3dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglSecondaryColor3f)(GLfloat red, GLfloat green, GLfloat blue);
GLvoid					(APIENTRY * qglSecondaryColor3fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglSecondaryColor3i)(GLint red, GLint green, GLint blue);
GLvoid					(APIENTRY * qglSecondaryColor3iv)(const GLint *v);
GLvoid					(APIENTRY * qglSecondaryColor3s)(GLshort red, GLshort green, GLshort blue);
GLvoid					(APIENTRY * qglSecondaryColor3sv)(const GLshort *v);
GLvoid					(APIENTRY * qglSecondaryColor3ub)(GLubyte red, GLubyte green, GLubyte blue);
GLvoid					(APIENTRY * qglSecondaryColor3ubv)(const GLubyte *v);
GLvoid					(APIENTRY * qglSecondaryColor3ui)(GLuint red, GLuint green, GLuint blue);
GLvoid					(APIENTRY * qglSecondaryColor3uiv)(const GLuint *v);
GLvoid					(APIENTRY * qglSecondaryColor3us)(GLushort red, GLushort green, GLushort blue);
GLvoid					(APIENTRY * qglSecondaryColor3usv)(const GLushort *v);
GLvoid					(APIENTRY * qglSecondaryColorP3ui)(GLenum type, GLuint v);
GLvoid					(APIENTRY * qglSecondaryColorP3uiv)(GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglSecondaryColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid					(APIENTRY * qglSelectBuffer)(GLsizei size, GLuint *buffer);
GLvoid					(APIENTRY * qglShadeModel)(GLenum mode);
GLvoid					(APIENTRY * qglShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
GLvoid					(APIENTRY * qglStencilFunc)(GLenum func, GLint ref, GLuint mask);
GLvoid					(APIENTRY * qglStencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
GLvoid					(APIENTRY * qglStencilMask)(GLuint mask);
GLvoid					(APIENTRY * qglStencilMaskSeparate)(GLenum face, GLuint mask);
GLvoid					(APIENTRY * qglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
GLvoid					(APIENTRY * qglStencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
GLvoid					(APIENTRY * qglStencilOpSeparateATI)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
GLvoid					(APIENTRY * qglStencilFuncSeparateATI)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
GLvoid					(APIENTRY * qglTexBuffer)(GLenum target, GLenum internalformat, GLuint buffer);
GLvoid					(APIENTRY * qglTexCoord1d)(GLdouble s);
GLvoid					(APIENTRY * qglTexCoord1dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglTexCoord1f)(GLfloat s);
GLvoid					(APIENTRY * qglTexCoord1fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglTexCoord1i)(GLint s);
GLvoid					(APIENTRY * qglTexCoord1iv)(const GLint *v);
GLvoid					(APIENTRY * qglTexCoord1s)(GLshort s);
GLvoid					(APIENTRY * qglTexCoord1sv)(const GLshort *v);
GLvoid					(APIENTRY * qglTexCoord2d)(GLdouble s, GLdouble t);
GLvoid					(APIENTRY * qglTexCoord2dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglTexCoord2f)(GLfloat s, GLfloat t);
GLvoid					(APIENTRY * qglTexCoord2fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglTexCoord2i)(GLint s, GLint t);
GLvoid					(APIENTRY * qglTexCoord2iv)(const GLint *v);
GLvoid					(APIENTRY * qglTexCoord2s)(GLshort s, GLshort t);
GLvoid					(APIENTRY * qglTexCoord2sv)(const GLshort *v);
GLvoid					(APIENTRY * qglTexCoord3d)(GLdouble s, GLdouble t, GLdouble r);
GLvoid					(APIENTRY * qglTexCoord3dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglTexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
GLvoid					(APIENTRY * qglTexCoord3fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglTexCoord3i)(GLint s, GLint t, GLint r);
GLvoid					(APIENTRY * qglTexCoord3iv)(const GLint *v);
GLvoid					(APIENTRY * qglTexCoord3s)(GLshort s, GLshort t, GLshort r);
GLvoid					(APIENTRY * qglTexCoord3sv)(const GLshort *v);
GLvoid					(APIENTRY * qglTexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GLvoid					(APIENTRY * qglTexCoord4dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglTexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GLvoid					(APIENTRY * qglTexCoord4fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglTexCoord4i)(GLint s, GLint t, GLint r, GLint q);
GLvoid					(APIENTRY * qglTexCoord4iv)(const GLint *v);
GLvoid					(APIENTRY * qglTexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q);
GLvoid					(APIENTRY * qglTexCoord4sv)(const GLshort *v);
GLvoid					(APIENTRY * qglTexCoordP1ui)(GLenum type, GLuint v);
GLvoid					(APIENTRY * qglTexCoordP1uiv)(GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglTexCoordP2ui)(GLenum type, GLuint v);
GLvoid					(APIENTRY * qglTexCoordP2uiv)(GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglTexCoordP3ui)(GLenum type, GLuint v);
GLvoid					(APIENTRY * qglTexCoordP3uiv)(GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglTexCoordP4ui)(GLenum type, GLuint v);
GLvoid					(APIENTRY * qglTexCoordP4uiv)(GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid					(APIENTRY * qglTexEnvf)(GLenum target, GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
GLvoid					(APIENTRY * qglTexEnvi)(GLenum target, GLenum pname, GLint param);
GLvoid					(APIENTRY * qglTexEnviv)(GLenum target, GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglTexGend)(GLenum coord, GLenum pname, GLdouble param);
GLvoid					(APIENTRY * qglTexGendv)(GLenum coord, GLenum pname, const GLdouble *params);
GLvoid					(APIENTRY * qglTexGenf)(GLenum coord, GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);
GLvoid					(APIENTRY * qglTexGeni)(GLenum coord, GLenum pname, GLint param);
GLvoid					(APIENTRY * qglTexGeniv)(GLenum coord, GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid					(APIENTRY * qglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid					(APIENTRY * qglTexImage2DMultisample)(GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
GLvoid					(APIENTRY * qglTexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid					(APIENTRY * qglTexImage3DMultisample)(GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
GLvoid					(APIENTRY * qglTexParameterIiv)(GLenum target, GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglTexParameterIuiv)(GLenum target, GLenum pname, const GLuint *params);
GLvoid					(APIENTRY * qglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
GLvoid					(APIENTRY * qglTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
GLvoid					(APIENTRY * qglTexParameteri)(GLenum target, GLenum pname, GLint param);
GLvoid					(APIENTRY * qglTexParameteriv)(GLenum target, GLenum pname, const GLint *params);
GLvoid					(APIENTRY * qglTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid					(APIENTRY * qglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid					(APIENTRY * qglTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid					(APIENTRY * qglTransformFeedbackVaryings)(GLuint program, GLsizei count, const GLchar **varyings, GLenum bufferMode);
GLvoid					(APIENTRY * qglTranslated)(GLdouble x, GLdouble y, GLdouble z);
GLvoid					(APIENTRY * qglTranslatef)(GLfloat x, GLfloat y, GLfloat z);
GLvoid					(APIENTRY * qglUniform1f)(GLint location, GLfloat v0);
GLvoid					(APIENTRY * qglUniform1fv)(GLint location, GLsizei count, const GLfloat *v);
GLvoid					(APIENTRY * qglUniform1i)(GLint location, GLint v0);
GLvoid					(APIENTRY * qglUniform1iv)(GLint location, GLsizei count, const GLint *v);
GLvoid					(APIENTRY * qglUniform1ui)(GLint location, GLuint v0);
GLvoid					(APIENTRY * qglUniform1uiv)(GLint location, GLsizei count, const GLuint *v);
GLvoid					(APIENTRY * qglUniform2f)(GLint location, GLfloat v0, GLfloat v1);
GLvoid					(APIENTRY * qglUniform2fv)(GLint location, GLsizei count, const GLfloat *v);
GLvoid					(APIENTRY * qglUniform2i)(GLint location, GLint v0, GLint v1);
GLvoid					(APIENTRY * qglUniform2iv)(GLint location, GLsizei count, const GLint *v);
GLvoid					(APIENTRY * qglUniform2ui)(GLint location, GLuint v0, GLuint v1);
GLvoid					(APIENTRY * qglUniform2uiv)(GLint location, GLsizei count, const GLuint *v);
GLvoid					(APIENTRY * qglUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GLvoid					(APIENTRY * qglUniform3fv)(GLint location, GLsizei count, const GLfloat *v);
GLvoid					(APIENTRY * qglUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
GLvoid					(APIENTRY * qglUniform3iv)(GLint location, GLsizei count, const GLint *v);
GLvoid					(APIENTRY * qglUniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2);
GLvoid					(APIENTRY * qglUniform3uiv)(GLint location, GLsizei count, const GLuint *v);
GLvoid					(APIENTRY * qglUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLvoid					(APIENTRY * qglUniform4fv)(GLint location, GLsizei count, const GLfloat *v);
GLvoid					(APIENTRY * qglUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
GLvoid					(APIENTRY * qglUniform4iv)(GLint location, GLsizei count, const GLint *v);
GLvoid					(APIENTRY * qglUniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
GLvoid					(APIENTRY * qglUniform4uiv)(GLint location, GLsizei count, const GLuint *v);
GLvoid					(APIENTRY * qglUniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
GLvoid					(APIENTRY * qglUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
GLvoid					(APIENTRY * qglUniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
GLvoid					(APIENTRY * qglUniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
GLvoid					(APIENTRY * qglUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
GLvoid					(APIENTRY * qglUniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
GLvoid					(APIENTRY * qglUniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
GLvoid					(APIENTRY * qglUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
GLvoid					(APIENTRY * qglUniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
GLvoid					(APIENTRY * qglUniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
GLboolean				(APIENTRY * qglUnmapBuffer)(GLenum target);
GLvoid					(APIENTRY * qglUseProgram)(GLuint program);
GLvoid					(APIENTRY * qglValidateProgram)(GLuint program);
GLvoid					(APIENTRY * qglVertex2d)(GLdouble x, GLdouble y);
GLvoid					(APIENTRY * qglVertex2dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglVertex2f)(GLfloat x, GLfloat y);
GLvoid					(APIENTRY * qglVertex2fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglVertex2i)(GLint x, GLint y);
GLvoid					(APIENTRY * qglVertex2iv)(const GLint *v);
GLvoid					(APIENTRY * qglVertex2s)(GLshort x, GLshort y);
GLvoid					(APIENTRY * qglVertex2sv)(const GLshort *v);
GLvoid					(APIENTRY * qglVertex3d)(GLdouble x, GLdouble y, GLdouble z);
GLvoid					(APIENTRY * qglVertex3dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglVertex3f)(GLfloat x, GLfloat y, GLfloat z);
GLvoid					(APIENTRY * qglVertex3fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglVertex3i)(GLint x, GLint y, GLint z);
GLvoid					(APIENTRY * qglVertex3iv)(const GLint *v);
GLvoid					(APIENTRY * qglVertex3s)(GLshort x, GLshort y, GLshort z);
GLvoid					(APIENTRY * qglVertex3sv)(const GLshort *v);
GLvoid					(APIENTRY * qglVertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GLvoid					(APIENTRY * qglVertex4dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLvoid					(APIENTRY * qglVertex4fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglVertex4i)(GLint x, GLint y, GLint z, GLint w);
GLvoid					(APIENTRY * qglVertex4iv)(const GLint *v);
GLvoid					(APIENTRY * qglVertex4s)(GLshort x, GLshort y, GLshort z, GLshort w);
GLvoid					(APIENTRY * qglVertex4sv)(const GLshort *v);
GLvoid					(APIENTRY * qglVertexAttrib1d)(GLuint index, GLdouble x);
GLvoid					(APIENTRY * qglVertexAttrib1dv)(GLuint index, const GLdouble *v);
GLvoid					(APIENTRY * qglVertexAttrib1f)(GLuint index, GLfloat x);
GLvoid					(APIENTRY * qglVertexAttrib1fv)(GLuint index, const GLfloat *v);
GLvoid					(APIENTRY * qglVertexAttrib1s)(GLuint index, GLshort x);
GLvoid					(APIENTRY * qglVertexAttrib1sv)(GLuint index, const GLshort *v);
GLvoid					(APIENTRY * qglVertexAttrib2d)(GLuint index, GLdouble x, GLdouble y);
GLvoid					(APIENTRY * qglVertexAttrib2dv)(GLuint index, const GLdouble *v);
GLvoid					(APIENTRY * qglVertexAttrib2f)(GLuint index, GLfloat x, GLfloat y);
GLvoid					(APIENTRY * qglVertexAttrib2fv)(GLuint index, const GLfloat *v);
GLvoid					(APIENTRY * qglVertexAttrib2s)(GLuint index, GLshort x, GLshort y);
GLvoid					(APIENTRY * qglVertexAttrib2sv)(GLuint index, const GLshort *v);
GLvoid					(APIENTRY * qglVertexAttrib3d)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
GLvoid					(APIENTRY * qglVertexAttrib3dv)(GLuint index, const GLdouble *v);
GLvoid					(APIENTRY * qglVertexAttrib3f)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
GLvoid					(APIENTRY * qglVertexAttrib3fv)(GLuint index, const GLfloat *v);
GLvoid					(APIENTRY * qglVertexAttrib3s)(GLuint index, GLshort x, GLshort y, GLshort z);
GLvoid					(APIENTRY * qglVertexAttrib3sv)(GLuint index, const GLshort *v);
GLvoid					(APIENTRY * qglVertexAttrib4Nbv)(GLuint index, const GLbyte *v);
GLvoid					(APIENTRY * qglVertexAttrib4Niv)(GLuint index, const GLint *v);
GLvoid					(APIENTRY * qglVertexAttrib4Nsv)(GLuint index, const GLshort *v);
GLvoid					(APIENTRY * qglVertexAttrib4Nub)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
GLvoid					(APIENTRY * qglVertexAttrib4Nubv)(GLuint index, const GLubyte *v);
GLvoid					(APIENTRY * qglVertexAttrib4Nuiv)(GLuint index, const GLuint *v);
GLvoid					(APIENTRY * qglVertexAttrib4Nusv)(GLuint index, const GLushort *v);
GLvoid					(APIENTRY * qglVertexAttrib4bv)(GLuint index, const GLbyte *v);
GLvoid					(APIENTRY * qglVertexAttrib4d)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GLvoid					(APIENTRY * qglVertexAttrib4dv)(GLuint index, const GLdouble *v);
GLvoid					(APIENTRY * qglVertexAttrib4f)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GLvoid					(APIENTRY * qglVertexAttrib4fv)(GLuint index, const GLfloat *v);
GLvoid					(APIENTRY * qglVertexAttrib4iv)(GLuint index, const GLint *v);
GLvoid					(APIENTRY * qglVertexAttrib4s)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
GLvoid					(APIENTRY * qglVertexAttrib4sv)(GLuint index, const GLshort *v);
GLvoid					(APIENTRY * qglVertexAttrib4ubv)(GLuint index, const GLubyte *v);
GLvoid					(APIENTRY * qglVertexAttrib4uiv)(GLuint index, const GLuint *v);
GLvoid					(APIENTRY * qglVertexAttrib4usv)(GLuint index, const GLushort *v);
GLvoid					(APIENTRY * qglVertexAttribI1i)(GLuint index, GLint x);
GLvoid					(APIENTRY * qglVertexAttribI1iv)(GLuint index, const GLint *v);
GLvoid					(APIENTRY * qglVertexAttribI1ui)(GLuint index, GLuint x);
GLvoid					(APIENTRY * qglVertexAttribI1uiv)(GLuint index, const GLuint *v);
GLvoid					(APIENTRY * qglVertexAttribI2i)(GLuint index, GLint x, GLint y);
GLvoid					(APIENTRY * qglVertexAttribI2iv)(GLuint index, const GLint *v);
GLvoid					(APIENTRY * qglVertexAttribI2ui)(GLuint index, GLuint x, GLuint y);
GLvoid					(APIENTRY * qglVertexAttribI2uiv)(GLuint index, const GLuint *v);
GLvoid					(APIENTRY * qglVertexAttribI3i)(GLuint index, GLint x, GLint y, GLint z);
GLvoid					(APIENTRY * qglVertexAttribI3iv)(GLuint index, const GLint *v);
GLvoid					(APIENTRY * qglVertexAttribI3ui)(GLuint index, GLuint x, GLuint y, GLuint z);
GLvoid					(APIENTRY * qglVertexAttribI3uiv)(GLuint index, const GLuint *v);
GLvoid					(APIENTRY * qglVertexAttribI4bv)(GLuint index, const GLbyte *v);
GLvoid					(APIENTRY * qglVertexAttribI4i)(GLuint index, GLint x, GLint y, GLint z, GLint w);
GLvoid					(APIENTRY * qglVertexAttribI4iv)(GLuint index, const GLint *v);
GLvoid					(APIENTRY * qglVertexAttribI4sv)(GLuint index, const GLshort *v);
GLvoid					(APIENTRY * qglVertexAttribI4ubv)(GLuint index, const GLubyte *v);
GLvoid					(APIENTRY * qglVertexAttribI4ui)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
GLvoid					(APIENTRY * qglVertexAttribI4uiv)(GLuint index, const GLuint *v);
GLvoid					(APIENTRY * qglVertexAttribI4usv)(GLuint index, const GLushort *v);
GLvoid					(APIENTRY * qglVertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid					(APIENTRY * qglVertexAttribP1ui)(GLuint index, GLenum type, GLboolean normalized, GLuint v);
GLvoid					(APIENTRY * qglVertexAttribP1uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *v);
GLvoid					(APIENTRY * qglVertexAttribP2ui)(GLuint index, GLenum type, GLboolean normalized, GLuint v);
GLvoid					(APIENTRY * qglVertexAttribP2uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *v);
GLvoid					(APIENTRY * qglVertexAttribP3ui)(GLuint index, GLenum type, GLboolean normalized, GLuint v);
GLvoid					(APIENTRY * qglVertexAttribP3uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *v);
GLvoid					(APIENTRY * qglVertexAttribP4ui)(GLuint index, GLenum type, GLboolean normalized, GLuint v);
GLvoid					(APIENTRY * qglVertexAttribP4uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *v);
GLvoid					(APIENTRY * qglVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
GLvoid					(APIENTRY * qglVertexP2ui)(GLenum type, GLuint v);
GLvoid					(APIENTRY * qglVertexP2uiv)(GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglVertexP3ui)(GLenum type, GLuint v);
GLvoid					(APIENTRY * qglVertexP3uiv)(GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglVertexP4ui)(GLenum type, GLuint v);
GLvoid					(APIENTRY * qglVertexP4uiv)(GLenum type, const GLuint *v);
GLvoid					(APIENTRY * qglVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid					(APIENTRY * qglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
GLvoid					(APIENTRY * qglWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
GLvoid					(APIENTRY * qglWindowPos2d)(GLdouble x, GLdouble y);
GLvoid					(APIENTRY * qglWindowPos2dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglWindowPos2f)(GLfloat x, GLfloat y);
GLvoid					(APIENTRY * qglWindowPos2fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglWindowPos2i)(GLint x, GLint y);
GLvoid					(APIENTRY * qglWindowPos2iv)(const GLint *v);
GLvoid					(APIENTRY * qglWindowPos2s)(GLshort x, GLshort y);
GLvoid					(APIENTRY * qglWindowPos2sv)(const GLshort *v);
GLvoid					(APIENTRY * qglWindowPos3d)(GLdouble x, GLdouble y, GLdouble z);
GLvoid					(APIENTRY * qglWindowPos3dv)(const GLdouble *v);
GLvoid					(APIENTRY * qglWindowPos3f)(GLfloat x, GLfloat y, GLfloat z);
GLvoid					(APIENTRY * qglWindowPos3fv)(const GLfloat *v);
GLvoid					(APIENTRY * qglWindowPos3i)(GLint x, GLint y, GLint z);
GLvoid					(APIENTRY * qglWindowPos3iv)(const GLint *v);
GLvoid					(APIENTRY * qglWindowPos3s)(GLshort x, GLshort y, GLshort z);
GLvoid					(APIENTRY * qglWindowPos3sv)(const GLshort *v);


// ============================================================================


static GLvoid			(APIENTRY * dllAccum)(GLenum op, GLfloat value);
static GLvoid			(APIENTRY * dllActiveStencilFaceEXT)(GLenum face);
static GLvoid			(APIENTRY * dllActiveTexture)(GLenum texture);
static GLvoid			(APIENTRY * dllAlphaFunc)(GLenum func, GLclampf ref);
static GLboolean		(APIENTRY * dllAreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences);
static GLvoid			(APIENTRY * dllArrayElement)(GLint i);
static GLvoid			(APIENTRY * dllAttachShader)(GLuint program, GLuint shader);
static GLvoid			(APIENTRY * dllBegin)(GLenum mode);
static GLvoid			(APIENTRY * dllBeginConditionalRender)(GLuint id, GLenum mode);
static GLvoid			(APIENTRY * dllBeginQuery)(GLenum target, GLuint id);
static GLvoid			(APIENTRY * dllBeginTransformFeedback)(GLenum primitiveMode);
static GLvoid			(APIENTRY * dllBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
static GLvoid			(APIENTRY * dllBindBuffer)(GLenum target, GLuint buffer);
static GLvoid			(APIENTRY * dllBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
static GLvoid			(APIENTRY * dllBindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
static GLvoid			(APIENTRY * dllBindFragDataLocation)(GLuint program, GLuint color, const GLchar *name);
static GLvoid			(APIENTRY * dllBindFragDataLocationIndexed)(GLuint program, GLuint color, GLuint index, const GLchar *name);
static GLvoid			(APIENTRY * dllBindFramebuffer)(GLenum target, GLuint framebuffer);
static GLvoid			(APIENTRY * dllBindRenderbuffer)(GLenum target, GLuint renderbuffer);
static GLvoid			(APIENTRY * dllBindSampler)(GLuint unit, GLuint sampler);
static GLvoid			(APIENTRY * dllBindTexture)(GLenum target, GLuint texture);
static GLvoid			(APIENTRY * dllBindVertexArray)(GLuint vertexArray);
static GLvoid			(APIENTRY * dllBitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
static GLvoid			(APIENTRY * dllBlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static GLvoid			(APIENTRY * dllBlendEquation)(GLenum mode);
static GLvoid			(APIENTRY * dllBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
static GLvoid			(APIENTRY * dllBlendFunc)(GLenum sfactor, GLenum dfactor);
static GLvoid			(APIENTRY * dllBlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
static GLvoid			(APIENTRY * dllBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
static GLvoid			(APIENTRY * dllBufferData)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
static GLvoid			(APIENTRY * dllBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
static GLvoid			(APIENTRY * dllCallList)(GLuint list);
static GLvoid			(APIENTRY * dllCallLists)(GLsizei n, GLenum type, const GLvoid *lists);
static GLenum			(APIENTRY * dllCheckFramebufferStatus)(GLenum target);
static GLvoid			(APIENTRY * dllClampColor)(GLenum target, GLenum clamp);
static GLvoid			(APIENTRY * dllClear)(GLbitfield mask);
static GLvoid			(APIENTRY * dllClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static GLvoid			(APIENTRY * dllClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
static GLvoid			(APIENTRY * dllClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat *value);
static GLvoid			(APIENTRY * dllClearBufferiv)(GLenum buffer, GLint drawbuffer, const GLint *value);
static GLvoid			(APIENTRY * dllClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint *value);
static GLvoid			(APIENTRY * dllClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static GLvoid			(APIENTRY * dllClearDepth)(GLclampd depth);
static GLvoid			(APIENTRY * dllClearIndex)(GLfloat c);
static GLvoid			(APIENTRY * dllClearStencil)(GLint s);
static GLvoid			(APIENTRY * dllClientActiveTexture)(GLenum texture);
static GLenum			(APIENTRY * dllClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
static GLvoid			(APIENTRY * dllClipPlane)(GLenum plane, const GLdouble *equation);
static GLvoid			(APIENTRY * dllColor3b)(GLbyte red, GLbyte green, GLbyte blue);
static GLvoid			(APIENTRY * dllColor3bv)(const GLbyte *v);
static GLvoid			(APIENTRY * dllColor3d)(GLdouble red, GLdouble green, GLdouble blue);
static GLvoid			(APIENTRY * dllColor3dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllColor3f)(GLfloat red, GLfloat green, GLfloat blue);
static GLvoid			(APIENTRY * dllColor3fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllColor3i)(GLint red, GLint green, GLint blue);
static GLvoid			(APIENTRY * dllColor3iv)(const GLint *v);
static GLvoid			(APIENTRY * dllColor3s)(GLshort red, GLshort green, GLshort blue);
static GLvoid			(APIENTRY * dllColor3sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllColor3ub)(GLubyte red, GLubyte green, GLubyte blue);
static GLvoid			(APIENTRY * dllColor3ubv)(const GLubyte *v);
static GLvoid			(APIENTRY * dllColor3ui)(GLuint red, GLuint green, GLuint blue);
static GLvoid			(APIENTRY * dllColor3uiv)(const GLuint *v);
static GLvoid			(APIENTRY * dllColor3us)(GLushort red, GLushort green, GLushort blue);
static GLvoid			(APIENTRY * dllColor3usv)(const GLushort *v);
static GLvoid			(APIENTRY * dllColor4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
static GLvoid			(APIENTRY * dllColor4bv)(const GLbyte *v);
static GLvoid			(APIENTRY * dllColor4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
static GLvoid			(APIENTRY * dllColor4dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static GLvoid			(APIENTRY * dllColor4fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllColor4i)(GLint red, GLint green, GLint blue, GLint alpha);
static GLvoid			(APIENTRY * dllColor4iv)(const GLint *v);
static GLvoid			(APIENTRY * dllColor4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha);
static GLvoid			(APIENTRY * dllColor4sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
static GLvoid			(APIENTRY * dllColor4ubv)(const GLubyte *v);
static GLvoid			(APIENTRY * dllColor4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
static GLvoid			(APIENTRY * dllColor4uiv)(const GLuint *v);
static GLvoid			(APIENTRY * dllColor4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha);
static GLvoid			(APIENTRY * dllColor4usv)(const GLushort *v);
static GLvoid			(APIENTRY * dllColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static GLvoid			(APIENTRY * dllColorMaski)(GLuint index, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static GLvoid			(APIENTRY * dllColorMaterial)(GLenum face, GLenum mode);
static GLvoid			(APIENTRY * dllColorP3ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllColorP3uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllColorP4ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllColorP4uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllCompileShader)(GLuint shader);
static GLvoid			(APIENTRY * dllCompressedTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllCompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllCompressedTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllCompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllCopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
static GLvoid			(APIENTRY * dllCopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
static GLvoid			(APIENTRY * dllCopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
static GLvoid			(APIENTRY * dllCopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
static GLvoid			(APIENTRY * dllCopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
static GLvoid			(APIENTRY * dllCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
static GLvoid			(APIENTRY * dllCopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
static GLuint			(APIENTRY * dllCreateProgram)(GLvoid);
static GLuint			(APIENTRY * dllCreateShader)(GLenum type);
static GLvoid			(APIENTRY * dllCullFace)(GLenum mode);
static GLvoid			(APIENTRY * dllDeleteBuffers)(GLsizei n, const GLuint *buffers);
static GLvoid			(APIENTRY * dllDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
static GLvoid			(APIENTRY * dllDeleteLists)(GLuint list, GLsizei range);
static GLvoid			(APIENTRY * dllDeleteProgram)(GLuint program);
static GLvoid			(APIENTRY * dllDeleteQueries)(GLsizei n, const GLuint *ids);
static GLvoid			(APIENTRY * dllDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
static GLvoid			(APIENTRY * dllDeleteSamplers)(GLsizei n, const GLuint *samplers);
static GLvoid			(APIENTRY * dllDeleteShader)(GLuint shader);
static GLvoid			(APIENTRY * dllDeleteSync)(GLsync sync);
static GLvoid			(APIENTRY * dllDeleteTextures)(GLsizei n, const GLuint *textures);
static GLvoid			(APIENTRY * dllDeleteVertexArrays)(GLsizei n, const GLuint *vertexArrays);
static GLvoid			(APIENTRY * dllDepthBoundsEXT)(GLclampd zmin, GLclampd zmax);
static GLvoid			(APIENTRY * dllDepthFunc)(GLenum func);
static GLvoid			(APIENTRY * dllDepthMask)(GLboolean flag);
static GLvoid			(APIENTRY * dllDepthRange)(GLclampd zNear, GLclampd zFar);
static GLvoid			(APIENTRY * dllDetachShader)(GLuint program, GLuint shader);
static GLvoid			(APIENTRY * dllDisable)(GLenum cap);
static GLvoid			(APIENTRY * dllDisableClientState)(GLenum array);
static GLvoid			(APIENTRY * dllDisableVertexAttribArray)(GLuint index);
static GLvoid			(APIENTRY * dllDisablei)(GLenum target, GLuint index);
static GLvoid			(APIENTRY * dllDrawArrays)(GLenum mode, GLint first, GLsizei count);
static GLvoid			(APIENTRY * dllDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
static GLvoid			(APIENTRY * dllDrawBuffer)(GLenum buffer);
static GLvoid			(APIENTRY * dllDrawBuffers)(GLsizei n, const GLenum *buffers);
static GLvoid			(APIENTRY * dllDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
static GLvoid			(APIENTRY * dllDrawElementsBaseVertex)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLint basevertex);
static GLvoid			(APIENTRY * dllDrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount);
static GLvoid			(APIENTRY * dllDrawElementsInstancedBaseVertex)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount, GLint basevertex);
static GLvoid			(APIENTRY * dllDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllDrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
static GLvoid			(APIENTRY * dllDrawRangeElementsBaseVertex)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices, GLint basevertex);
static GLvoid			(APIENTRY * dllEdgeFlag)(GLboolean flag);
static GLvoid			(APIENTRY * dllEdgeFlagPointer)(GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllEdgeFlagv)(const GLboolean *flag);
static GLvoid			(APIENTRY * dllEnable)(GLenum cap);
static GLvoid			(APIENTRY * dllEnableClientState)(GLenum array);
static GLvoid			(APIENTRY * dllEnableVertexAttribArray)(GLuint index);
static GLvoid			(APIENTRY * dllEnablei)(GLenum target, GLuint index);
static GLvoid			(APIENTRY * dllEnd)(GLvoid);
static GLvoid			(APIENTRY * dllEndConditionalRender)(GLvoid);
static GLvoid			(APIENTRY * dllEndList)(GLvoid);
static GLvoid			(APIENTRY * dllEndQuery)(GLenum target);
static GLvoid			(APIENTRY * dllEndTransformFeedback)(GLvoid);
static GLvoid			(APIENTRY * dllEvalCoord1d)(GLdouble u);
static GLvoid			(APIENTRY * dllEvalCoord1dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllEvalCoord1f)(GLfloat u);
static GLvoid			(APIENTRY * dllEvalCoord1fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllEvalCoord2d)(GLdouble u, GLdouble v);
static GLvoid			(APIENTRY * dllEvalCoord2dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllEvalCoord2f)(GLfloat u, GLfloat v);
static GLvoid			(APIENTRY * dllEvalCoord2fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllEvalMesh1)(GLenum mode, GLint i1, GLint i2);
static GLvoid			(APIENTRY * dllEvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
static GLvoid			(APIENTRY * dllEvalPoint1)(GLint i);
static GLvoid			(APIENTRY * dllEvalPoint2)(GLint i, GLint j);
static GLvoid			(APIENTRY * dllFeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer);
static GLsync			(APIENTRY * dllFenceSync)(GLenum condition, GLbitfield flags);
static GLvoid			(APIENTRY * dllFinish)(GLvoid);
static GLvoid			(APIENTRY * dllFlush)(GLvoid);
static GLvoid			(APIENTRY * dllFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length);
static GLvoid			(APIENTRY * dllFogCoordPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllFogCoordd)(GLdouble coord);
static GLvoid			(APIENTRY * dllFogCoorddv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllFogCoordf)(GLfloat coord);
static GLvoid			(APIENTRY * dllFogCoordfv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllFogf)(GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllFogfv)(GLenum pname, const GLfloat *params);
static GLvoid			(APIENTRY * dllFogi)(GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllFogiv)(GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
static GLvoid			(APIENTRY * dllFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
static GLvoid			(APIENTRY * dllFramebufferTexture1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
static GLvoid			(APIENTRY * dllFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
static GLvoid			(APIENTRY * dllFramebufferTexture3D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);
static GLvoid			(APIENTRY * dllFramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
static GLvoid			(APIENTRY * dllFrontFace)(GLenum mode);
static GLvoid			(APIENTRY * dllFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static GLvoid			(APIENTRY * dllGenBuffers)(GLsizei n, GLuint *buffers);
static GLvoid			(APIENTRY * dllGenFramebuffers)(GLsizei n, GLuint *framebuffers);
static GLuint			(APIENTRY * dllGenLists)(GLsizei range);
static GLvoid			(APIENTRY * dllGenQueries)(GLsizei n, GLuint *ids);
static GLvoid			(APIENTRY * dllGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
static GLvoid			(APIENTRY * dllGenSamplers)(GLsizei n, GLuint *samplers);
static GLvoid			(APIENTRY * dllGenTextures)(GLsizei n, GLuint *textures);
static GLvoid			(APIENTRY * dllGenVertexArrays)(GLsizei n, GLuint *vertexArrays);
static GLvoid			(APIENTRY * dllGenerateMipmap)(GLenum target);
static GLvoid			(APIENTRY * dllGetActiveAttrib)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
static GLvoid			(APIENTRY * dllGetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
static GLvoid			(APIENTRY * dllGetActiveUniformBlockName)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
static GLvoid			(APIENTRY * dllGetActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetActiveUniformName)(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName);
static GLvoid			(APIENTRY * dllGetActiveUniformsiv)(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetAttachedShaders)(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
static GLint			(APIENTRY * dllGetAttribLocation)(GLuint program, const GLchar *name);
static GLvoid			(APIENTRY * dllGetBooleani_v)(GLenum target, GLuint index, GLboolean *params);
static GLvoid			(APIENTRY * dllGetBooleanv)(GLenum pname, GLboolean *params);
static GLvoid			(APIENTRY * dllGetBufferParameteri64v)(GLenum target, GLenum pname, GLint64 *params);
static GLvoid			(APIENTRY * dllGetBufferParameteriv)(GLenum target, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetBufferPointerv)(GLenum target, GLenum pname, GLvoid **params);
static GLvoid			(APIENTRY * dllGetBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid *data);
static GLvoid			(APIENTRY * dllGetClipPlane)(GLenum plane, GLdouble *equation);
static GLvoid			(APIENTRY * dllGetCompressedTexImage)(GLenum target, GLint level, GLvoid *pixels);
static GLvoid			(APIENTRY * dllGetDoublev)(GLenum pname, GLdouble *params);
static GLenum			(APIENTRY * dllGetError)(GLvoid);
static GLvoid			(APIENTRY * dllGetFloatv)(GLenum pname, GLfloat *params);
static GLint			(APIENTRY * dllGetFragDataIndex)(GLuint program, const GLchar *name);
static GLint			(APIENTRY * dllGetFragDataLocation)(GLuint program, const GLchar *name);
static GLvoid			(APIENTRY * dllGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetInteger64i_v)(GLenum target, GLuint index, GLint64 *params);
static GLvoid			(APIENTRY * dllGetInteger64v)(GLenum pname, GLint64 *params);
static GLvoid			(APIENTRY * dllGetIntegeri_v)(GLenum target, GLuint index, GLint *params);
static GLvoid			(APIENTRY * dllGetIntegerv)(GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetLightfv)(GLenum light, GLenum pname, GLfloat *params);
static GLvoid			(APIENTRY * dllGetLightiv)(GLenum light, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetMapdv)(GLenum target, GLenum query, GLdouble *v);
static GLvoid			(APIENTRY * dllGetMapfv)(GLenum target, GLenum query, GLfloat *v);
static GLvoid			(APIENTRY * dllGetMapiv)(GLenum target, GLenum query, GLint *v);
static GLvoid			(APIENTRY * dllGetMaterialfv)(GLenum face, GLenum pname, GLfloat *params);
static GLvoid			(APIENTRY * dllGetMaterialiv)(GLenum face, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetMultisamplefv)(GLenum pname, GLuint index, GLfloat *params);
static GLvoid			(APIENTRY * dllGetPixelMapfv)(GLenum map, GLfloat *values);
static GLvoid			(APIENTRY * dllGetPixelMapuiv)(GLenum map, GLuint *values);
static GLvoid			(APIENTRY * dllGetPixelMapusv)(GLenum map, GLushort *values);
static GLvoid			(APIENTRY * dllGetPointerv)(GLenum pname, GLvoid **params);
static GLvoid			(APIENTRY * dllGetPolygonStipple)(GLubyte *mask);
static GLvoid			(APIENTRY * dllGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
static GLvoid			(APIENTRY * dllGetProgramiv)(GLuint program, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetQueryObjecti64v)(GLuint id, GLenum pname, GLint64 *params);
static GLvoid			(APIENTRY * dllGetQueryObjectiv)(GLuint id, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetQueryObjectui64v)(GLuint id, GLenum pname, GLuint64 *params);
static GLvoid			(APIENTRY * dllGetQueryObjectuiv)(GLuint id, GLenum pname, GLuint *params);
static GLvoid			(APIENTRY * dllGetQueryiv)(GLenum target, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetSamplerParameterIiv)(GLuint sampler, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetSamplerParameterIuiv)(GLuint sampler, GLenum pname, GLuint *params);
static GLvoid			(APIENTRY * dllGetSamplerParameterfv)(GLuint sampler, GLenum pname, GLfloat *params);
static GLvoid			(APIENTRY * dllGetSamplerParameteriv)(GLuint sampler, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
static GLvoid			(APIENTRY * dllGetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
static GLvoid			(APIENTRY * dllGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
static const GLubyte *	(APIENTRY * dllGetString)(GLenum name);
static const GLubyte *	(APIENTRY * dllGetStringi)(GLenum name, GLuint index);
static GLvoid			(APIENTRY * dllGetSynciv)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
static GLvoid			(APIENTRY * dllGetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params);
static GLvoid			(APIENTRY * dllGetTexEnviv)(GLenum target, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetTexGendv)(GLenum coord, GLenum pname, GLdouble *params);
static GLvoid			(APIENTRY * dllGetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params);
static GLvoid			(APIENTRY * dllGetTexGeniv)(GLenum coord, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
static GLvoid			(APIENTRY * dllGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
static GLvoid			(APIENTRY * dllGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetTexParameterIiv)(GLenum target, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetTexParameterIuiv)(GLenum target, GLenum pname, GLuint *params);
static GLvoid			(APIENTRY * dllGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
static GLvoid			(APIENTRY * dllGetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetTransformFeedbackVarying)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
static GLuint			(APIENTRY * dllGetUniformBlockIndex)(GLuint program, const GLchar *uniformBlockName);
static GLvoid			(APIENTRY * dllGetUniformIndices)(GLuint program, GLsizei uniformCount, const GLchar **uniformNames, GLuint *uniformIndices);
static GLint			(APIENTRY * dllGetUniformLocation)(GLuint program, const GLchar *name);
static GLvoid			(APIENTRY * dllGetUniformfv)(GLuint program, GLint location, GLfloat *params);
static GLvoid			(APIENTRY * dllGetUniformiv)(GLuint program, GLint location, GLint *params);
static GLvoid			(APIENTRY * dllGetUniformuiv)(GLuint program, GLint location, GLuint *params);
static GLvoid			(APIENTRY * dllGetVertexAttribIiv)(GLuint index, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllGetVertexAttribIuiv)(GLuint index, GLenum pname, GLuint *params);
static GLvoid			(APIENTRY * dllGetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid **params);
static GLvoid			(APIENTRY * dllGetVertexAttribdv)(GLuint index, GLenum pname, GLdouble *params);
static GLvoid			(APIENTRY * dllGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat *params);
static GLvoid			(APIENTRY * dllGetVertexAttribiv)(GLuint index, GLenum pname, GLint *params);
static GLvoid			(APIENTRY * dllHint)(GLenum target, GLenum mode);
static GLvoid			(APIENTRY * dllIndexMask)(GLuint mask);
static GLvoid			(APIENTRY * dllIndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllIndexd)(GLdouble c);
static GLvoid			(APIENTRY * dllIndexdv)(const GLdouble *c);
static GLvoid			(APIENTRY * dllIndexf)(GLfloat c);
static GLvoid			(APIENTRY * dllIndexfv)(const GLfloat *c);
static GLvoid			(APIENTRY * dllIndexi)(GLint c);
static GLvoid			(APIENTRY * dllIndexiv)(const GLint *c);
static GLvoid			(APIENTRY * dllIndexs)(GLshort c);
static GLvoid			(APIENTRY * dllIndexsv)(const GLshort *c);
static GLvoid			(APIENTRY * dllIndexub)(GLubyte c);
static GLvoid			(APIENTRY * dllIndexubv)(const GLubyte *c);
static GLvoid			(APIENTRY * dllInitNames)(GLvoid);
static GLvoid			(APIENTRY * dllInterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer);
static GLboolean		(APIENTRY * dllIsBuffer)(GLuint buffer);
static GLboolean		(APIENTRY * dllIsEnabled)(GLenum cap);
static GLboolean		(APIENTRY * dllIsEnabledi)(GLenum target, GLuint index);
static GLboolean		(APIENTRY * dllIsFramebuffer)(GLuint framebuffer);
static GLboolean		(APIENTRY * dllIsList)(GLuint list);
static GLboolean		(APIENTRY * dllIsProgram)(GLuint program);
static GLboolean		(APIENTRY * dllIsQuery)(GLuint id);
static GLboolean		(APIENTRY * dllIsRenderbuffer)(GLuint renderbuffer);
static GLboolean		(APIENTRY * dllIsSampler)(GLuint sampler);
static GLboolean		(APIENTRY * dllIsShader)(GLuint shader);
static GLboolean		(APIENTRY * dllIsSync)(GLsync sync);
static GLboolean		(APIENTRY * dllIsTexture)(GLuint texture);
static GLboolean		(APIENTRY * dllIsVertexArray)(GLuint vertexArray);
static GLvoid			(APIENTRY * dllLightModelf)(GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllLightModelfv)(GLenum pname, const GLfloat *params);
static GLvoid			(APIENTRY * dllLightModeli)(GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllLightModeliv)(GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllLightf)(GLenum light, GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllLightfv)(GLenum light, GLenum pname, const GLfloat *params);
static GLvoid			(APIENTRY * dllLighti)(GLenum light, GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllLightiv)(GLenum light, GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllLineStipple)(GLint factor, GLushort pattern);
static GLvoid			(APIENTRY * dllLineWidth)(GLfloat width);
static GLvoid			(APIENTRY * dllLinkProgram)(GLuint program);
static GLvoid			(APIENTRY * dllListBase)(GLuint base);
static GLvoid			(APIENTRY * dllLoadIdentity)(GLvoid);
static GLvoid			(APIENTRY * dllLoadMatrixd)(const GLdouble *m);
static GLvoid			(APIENTRY * dllLoadMatrixf)(const GLfloat *m);
static GLvoid			(APIENTRY * dllLoadName)(GLuint name);
static GLvoid			(APIENTRY * dllLoadTransposeMatrixd)(const GLdouble *m);
static GLvoid			(APIENTRY * dllLoadTransposeMatrixf)(const GLfloat *m);
static GLvoid			(APIENTRY * dllLogicOp)(GLenum opcode);
static GLvoid			(APIENTRY * dllMap1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
static GLvoid			(APIENTRY * dllMap1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
static GLvoid			(APIENTRY * dllMap2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
static GLvoid			(APIENTRY * dllMap2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
static GLvoid *			(APIENTRY * dllMapBuffer)(GLenum target, GLenum access);
static GLvoid *			(APIENTRY * dllMapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
static GLvoid			(APIENTRY * dllMapGrid1d)(GLint un, GLdouble u1, GLdouble u2);
static GLvoid			(APIENTRY * dllMapGrid1f)(GLint un, GLfloat u1, GLfloat u2);
static GLvoid			(APIENTRY * dllMapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
static GLvoid			(APIENTRY * dllMapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
static GLvoid			(APIENTRY * dllMaterialf)(GLenum face, GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
static GLvoid			(APIENTRY * dllMateriali)(GLenum face, GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllMaterialiv)(GLenum face, GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllMatrixMode)(GLenum mode);
static GLvoid			(APIENTRY * dllMultMatrixd)(const GLdouble *m);
static GLvoid			(APIENTRY * dllMultMatrixf)(const GLfloat *m);
static GLvoid			(APIENTRY * dllMultTransposeMatrixd)(const GLdouble *m);
static GLvoid			(APIENTRY * dllMultTransposeMatrixf)(const GLfloat *m);
static GLvoid			(APIENTRY * dllMultiDrawArrays)(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
static GLvoid			(APIENTRY * dllMultiDrawElements)(GLenum mode, const GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount);
static GLvoid			(APIENTRY * dllMultiDrawElementsBaseVertex)(GLenum mode, const GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount, const GLint *basevertex);
static GLvoid			(APIENTRY * dllMultiTexCoord1d)(GLenum target, GLdouble s);
static GLvoid			(APIENTRY * dllMultiTexCoord1dv)(GLenum target, const GLdouble *v);
static GLvoid			(APIENTRY * dllMultiTexCoord1f)(GLenum target, GLfloat s);
static GLvoid			(APIENTRY * dllMultiTexCoord1fv)(GLenum target, const GLfloat *v);
static GLvoid			(APIENTRY * dllMultiTexCoord1i)(GLenum target, GLint s);
static GLvoid			(APIENTRY * dllMultiTexCoord1iv)(GLenum target, const GLint *v);
static GLvoid			(APIENTRY * dllMultiTexCoord1s)(GLenum target, GLshort s);
static GLvoid			(APIENTRY * dllMultiTexCoord1sv)(GLenum target, const GLshort *v);
static GLvoid			(APIENTRY * dllMultiTexCoord2d)(GLenum target, GLdouble s, GLdouble t);
static GLvoid			(APIENTRY * dllMultiTexCoord2dv)(GLenum target, const GLdouble *v);
static GLvoid			(APIENTRY * dllMultiTexCoord2f)(GLenum target, GLfloat s, GLfloat t);
static GLvoid			(APIENTRY * dllMultiTexCoord2fv)(GLenum target, const GLfloat *v);
static GLvoid			(APIENTRY * dllMultiTexCoord2i)(GLenum target, GLint s, GLint t);
static GLvoid			(APIENTRY * dllMultiTexCoord2iv)(GLenum target, const GLint *v);
static GLvoid			(APIENTRY * dllMultiTexCoord2s)(GLenum target, GLshort s, GLshort t);
static GLvoid			(APIENTRY * dllMultiTexCoord2sv)(GLenum target, const GLshort *v);
static GLvoid			(APIENTRY * dllMultiTexCoord3d)(GLenum target, GLdouble s, GLdouble t, GLdouble r);
static GLvoid			(APIENTRY * dllMultiTexCoord3dv)(GLenum target, const GLdouble *v);
static GLvoid			(APIENTRY * dllMultiTexCoord3f)(GLenum target, GLfloat s, GLfloat t, GLfloat r);
static GLvoid			(APIENTRY * dllMultiTexCoord3fv)(GLenum target, const GLfloat *v);
static GLvoid			(APIENTRY * dllMultiTexCoord3i)(GLenum target, GLint s, GLint t, GLint r);
static GLvoid			(APIENTRY * dllMultiTexCoord3iv)(GLenum target, const GLint *v);
static GLvoid			(APIENTRY * dllMultiTexCoord3s)(GLenum target, GLshort s, GLshort t, GLshort r);
static GLvoid			(APIENTRY * dllMultiTexCoord3sv)(GLenum target, const GLshort *v);
static GLvoid			(APIENTRY * dllMultiTexCoord4d)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
static GLvoid			(APIENTRY * dllMultiTexCoord4dv)(GLenum target, const GLdouble *v);
static GLvoid			(APIENTRY * dllMultiTexCoord4f)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
static GLvoid			(APIENTRY * dllMultiTexCoord4fv)(GLenum target, const GLfloat *v);
static GLvoid			(APIENTRY * dllMultiTexCoord4i)(GLenum target, GLint s, GLint t, GLint r, GLint q);
static GLvoid			(APIENTRY * dllMultiTexCoord4iv)(GLenum target, const GLint *v);
static GLvoid			(APIENTRY * dllMultiTexCoord4s)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
static GLvoid			(APIENTRY * dllMultiTexCoord4sv)(GLenum target, const GLshort *v);
static GLvoid			(APIENTRY * dllMultiTexCoordP1ui)(GLenum target, GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllMultiTexCoordP1uiv)(GLenum target, GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllMultiTexCoordP2ui)(GLenum target, GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllMultiTexCoordP2uiv)(GLenum target, GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllMultiTexCoordP3ui)(GLenum target, GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllMultiTexCoordP3uiv)(GLenum target, GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllMultiTexCoordP4ui)(GLenum target, GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllMultiTexCoordP4uiv)(GLenum target, GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllNewList)(GLuint list, GLenum mode);
static GLvoid			(APIENTRY * dllNormal3b)(GLbyte nx, GLbyte ny, GLbyte nz);
static GLvoid			(APIENTRY * dllNormal3bv)(const GLbyte *v);
static GLvoid			(APIENTRY * dllNormal3d)(GLdouble nx, GLdouble ny, GLdouble nz);
static GLvoid			(APIENTRY * dllNormal3dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
static GLvoid			(APIENTRY * dllNormal3fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllNormal3i)(GLint nx, GLint ny, GLint nz);
static GLvoid			(APIENTRY * dllNormal3iv)(const GLint *v);
static GLvoid			(APIENTRY * dllNormal3s)(GLshort nx, GLshort ny, GLshort nz);
static GLvoid			(APIENTRY * dllNormal3sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllNormalP3ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllNormalP3uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static GLvoid			(APIENTRY * dllPassThrough)(GLfloat token);
static GLvoid			(APIENTRY * dllPixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat *values);
static GLvoid			(APIENTRY * dllPixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint *values);
static GLvoid			(APIENTRY * dllPixelMapusv)(GLenum map, GLsizei mapsize, const GLushort *values);
static GLvoid			(APIENTRY * dllPixelStoref)(GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllPixelStorei)(GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllPixelTransferf)(GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllPixelTransferi)(GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllPixelZoom)(GLfloat xfactor, GLfloat yfactor);
static GLvoid			(APIENTRY * dllPointParameterf)(GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllPointParameterfv)(GLenum pname, const GLfloat *params);
static GLvoid			(APIENTRY * dllPointParameteri)(GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllPointParameteriv)(GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllPointSize)(GLfloat size);
static GLvoid			(APIENTRY * dllPolygonMode)(GLenum face, GLenum mode);
static GLvoid			(APIENTRY * dllPolygonOffset)(GLfloat factor, GLfloat units);
static GLvoid			(APIENTRY * dllPolygonStipple)(const GLubyte *mask);
static GLvoid			(APIENTRY * dllPopAttrib)(GLvoid);
static GLvoid			(APIENTRY * dllPopClientAttrib)(GLvoid);
static GLvoid			(APIENTRY * dllPopMatrix)(GLvoid);
static GLvoid			(APIENTRY * dllPopName)(GLvoid);
static GLvoid			(APIENTRY * dllPrimitiveRestartIndex)(GLuint index);
static GLvoid			(APIENTRY * dllPrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
static GLvoid			(APIENTRY * dllProvokingVertex)(GLenum mode);
static GLvoid			(APIENTRY * dllPushAttrib)(GLbitfield mask);
static GLvoid			(APIENTRY * dllPushClientAttrib)(GLbitfield mask);
static GLvoid			(APIENTRY * dllPushMatrix)(GLvoid);
static GLvoid			(APIENTRY * dllPushName)(GLuint name);
static GLvoid			(APIENTRY * dllQueryCounter)(GLuint id, GLenum target);
static GLvoid			(APIENTRY * dllRasterPos2d)(GLdouble x, GLdouble y);
static GLvoid			(APIENTRY * dllRasterPos2dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllRasterPos2f)(GLfloat x, GLfloat y);
static GLvoid			(APIENTRY * dllRasterPos2fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllRasterPos2i)(GLint x, GLint y);
static GLvoid			(APIENTRY * dllRasterPos2iv)(const GLint *v);
static GLvoid			(APIENTRY * dllRasterPos2s)(GLshort x, GLshort y);
static GLvoid			(APIENTRY * dllRasterPos2sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllRasterPos3d)(GLdouble x, GLdouble y, GLdouble z);
static GLvoid			(APIENTRY * dllRasterPos3dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllRasterPos3f)(GLfloat x, GLfloat y, GLfloat z);
static GLvoid			(APIENTRY * dllRasterPos3fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllRasterPos3i)(GLint x, GLint y, GLint z);
static GLvoid			(APIENTRY * dllRasterPos3iv)(const GLint *v);
static GLvoid			(APIENTRY * dllRasterPos3s)(GLshort x, GLshort y, GLshort z);
static GLvoid			(APIENTRY * dllRasterPos3sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllRasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static GLvoid			(APIENTRY * dllRasterPos4dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllRasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static GLvoid			(APIENTRY * dllRasterPos4fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllRasterPos4i)(GLint x, GLint y, GLint z, GLint w);
static GLvoid			(APIENTRY * dllRasterPos4iv)(const GLint *v);
static GLvoid			(APIENTRY * dllRasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w);
static GLvoid			(APIENTRY * dllRasterPos4sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllReadBuffer)(GLenum buffer);
static GLvoid			(APIENTRY * dllReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
static GLvoid			(APIENTRY * dllRectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
static GLvoid			(APIENTRY * dllRectdv)(const GLdouble *v1, const GLdouble *v2);
static GLvoid			(APIENTRY * dllRectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
static GLvoid			(APIENTRY * dllRectfv)(const GLfloat *v1, const GLfloat *v2);
static GLvoid			(APIENTRY * dllRecti)(GLint x1, GLint y1, GLint x2, GLint y2);
static GLvoid			(APIENTRY * dllRectiv)(const GLint *v1, const GLint *v2);
static GLvoid			(APIENTRY * dllRects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
static GLvoid			(APIENTRY * dllRectsv)(const GLshort *v1, const GLshort *v2);
static GLint			(APIENTRY * dllRenderMode)(GLenum mode);
static GLvoid			(APIENTRY * dllRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
static GLvoid			(APIENTRY * dllRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
static GLvoid			(APIENTRY * dllRotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
static GLvoid			(APIENTRY * dllRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
static GLvoid			(APIENTRY * dllSampleCoverage)(GLclampf value, GLboolean invert);
static GLvoid			(APIENTRY * dllSampleMaski)(GLuint index, GLbitfield mask);
static GLvoid			(APIENTRY * dllSamplerParameterIiv)(GLuint sampler, GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllSamplerParameterIuiv)(GLuint sampler, GLenum pname, const GLuint *params);
static GLvoid			(APIENTRY * dllSamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllSamplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat *params);
static GLvoid			(APIENTRY * dllSamplerParameteri)(GLuint sampler, GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllSamplerParameteriv)(GLuint sampler, GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllScaled)(GLdouble x, GLdouble y, GLdouble z);
static GLvoid			(APIENTRY * dllScalef)(GLfloat x, GLfloat y, GLfloat z);
static GLvoid			(APIENTRY * dllScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
static GLvoid			(APIENTRY * dllSecondaryColor3b)(GLbyte red, GLbyte green, GLbyte blue);
static GLvoid			(APIENTRY * dllSecondaryColor3bv)(const GLbyte *v);
static GLvoid			(APIENTRY * dllSecondaryColor3d)(GLdouble red, GLdouble green, GLdouble blue);
static GLvoid			(APIENTRY * dllSecondaryColor3dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllSecondaryColor3f)(GLfloat red, GLfloat green, GLfloat blue);
static GLvoid			(APIENTRY * dllSecondaryColor3fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllSecondaryColor3i)(GLint red, GLint green, GLint blue);
static GLvoid			(APIENTRY * dllSecondaryColor3iv)(const GLint *v);
static GLvoid			(APIENTRY * dllSecondaryColor3s)(GLshort red, GLshort green, GLshort blue);
static GLvoid			(APIENTRY * dllSecondaryColor3sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllSecondaryColor3ub)(GLubyte red, GLubyte green, GLubyte blue);
static GLvoid			(APIENTRY * dllSecondaryColor3ubv)(const GLubyte *v);
static GLvoid			(APIENTRY * dllSecondaryColor3ui)(GLuint red, GLuint green, GLuint blue);
static GLvoid			(APIENTRY * dllSecondaryColor3uiv)(const GLuint *v);
static GLvoid			(APIENTRY * dllSecondaryColor3us)(GLushort red, GLushort green, GLushort blue);
static GLvoid			(APIENTRY * dllSecondaryColor3usv)(const GLushort *v);
static GLvoid			(APIENTRY * dllSecondaryColorP3ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllSecondaryColorP3uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllSecondaryColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllSelectBuffer)(GLsizei size, GLuint *buffer);
static GLvoid			(APIENTRY * dllShadeModel)(GLenum mode);
static GLvoid			(APIENTRY * dllShaderSource)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
static GLvoid			(APIENTRY * dllStencilFunc)(GLenum func, GLint ref, GLuint mask);
static GLvoid			(APIENTRY * dllStencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
static GLvoid			(APIENTRY * dllStencilMask)(GLuint mask);
static GLvoid			(APIENTRY * dllStencilMaskSeparate)(GLenum face, GLuint mask);
static GLvoid			(APIENTRY * dllStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
static GLvoid			(APIENTRY * dllStencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
static GLvoid			(APIENTRY * dllStencilOpSeparateATI)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
static GLvoid			(APIENTRY * dllStencilFuncSeparateATI)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
static GLvoid			(APIENTRY * dllTexBuffer)(GLenum target, GLenum internalformat, GLuint buffer);
static GLvoid			(APIENTRY * dllTexCoord1d)(GLdouble s);
static GLvoid			(APIENTRY * dllTexCoord1dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllTexCoord1f)(GLfloat s);
static GLvoid			(APIENTRY * dllTexCoord1fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllTexCoord1i)(GLint s);
static GLvoid			(APIENTRY * dllTexCoord1iv)(const GLint *v);
static GLvoid			(APIENTRY * dllTexCoord1s)(GLshort s);
static GLvoid			(APIENTRY * dllTexCoord1sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllTexCoord2d)(GLdouble s, GLdouble t);
static GLvoid			(APIENTRY * dllTexCoord2dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllTexCoord2f)(GLfloat s, GLfloat t);
static GLvoid			(APIENTRY * dllTexCoord2fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllTexCoord2i)(GLint s, GLint t);
static GLvoid			(APIENTRY * dllTexCoord2iv)(const GLint *v);
static GLvoid			(APIENTRY * dllTexCoord2s)(GLshort s, GLshort t);
static GLvoid			(APIENTRY * dllTexCoord2sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllTexCoord3d)(GLdouble s, GLdouble t, GLdouble r);
static GLvoid			(APIENTRY * dllTexCoord3dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllTexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
static GLvoid			(APIENTRY * dllTexCoord3fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllTexCoord3i)(GLint s, GLint t, GLint r);
static GLvoid			(APIENTRY * dllTexCoord3iv)(const GLint *v);
static GLvoid			(APIENTRY * dllTexCoord3s)(GLshort s, GLshort t, GLshort r);
static GLvoid			(APIENTRY * dllTexCoord3sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllTexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
static GLvoid			(APIENTRY * dllTexCoord4dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllTexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
static GLvoid			(APIENTRY * dllTexCoord4fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllTexCoord4i)(GLint s, GLint t, GLint r, GLint q);
static GLvoid			(APIENTRY * dllTexCoord4iv)(const GLint *v);
static GLvoid			(APIENTRY * dllTexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q);
static GLvoid			(APIENTRY * dllTexCoord4sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllTexCoordP1ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllTexCoordP1uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllTexCoordP2ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllTexCoordP2uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllTexCoordP3ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllTexCoordP3uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllTexCoordP4ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllTexCoordP4uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllTexEnvf)(GLenum target, GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
static GLvoid			(APIENTRY * dllTexEnvi)(GLenum target, GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllTexEnviv)(GLenum target, GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllTexGend)(GLenum coord, GLenum pname, GLdouble param);
static GLvoid			(APIENTRY * dllTexGendv)(GLenum coord, GLenum pname, const GLdouble *params);
static GLvoid			(APIENTRY * dllTexGenf)(GLenum coord, GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);
static GLvoid			(APIENTRY * dllTexGeni)(GLenum coord, GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllTexGeniv)(GLenum coord, GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllTexImage2DMultisample)(GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
static GLvoid			(APIENTRY * dllTexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllTexImage3DMultisample)(GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
static GLvoid			(APIENTRY * dllTexParameterIiv)(GLenum target, GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllTexParameterIuiv)(GLenum target, GLenum pname, const GLuint *params);
static GLvoid			(APIENTRY * dllTexParameterf)(GLenum target, GLenum pname, GLfloat param);
static GLvoid			(APIENTRY * dllTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
static GLvoid			(APIENTRY * dllTexParameteri)(GLenum target, GLenum pname, GLint param);
static GLvoid			(APIENTRY * dllTexParameteriv)(GLenum target, GLenum pname, const GLint *params);
static GLvoid			(APIENTRY * dllTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
static GLvoid			(APIENTRY * dllTransformFeedbackVaryings)(GLuint program, GLsizei count, const GLchar **varyings, GLenum bufferMode);
static GLvoid			(APIENTRY * dllTranslated)(GLdouble x, GLdouble y, GLdouble z);
static GLvoid			(APIENTRY * dllTranslatef)(GLfloat x, GLfloat y, GLfloat z);
static GLvoid			(APIENTRY * dllUniform1f)(GLint location, GLfloat v0);
static GLvoid			(APIENTRY * dllUniform1fv)(GLint location, GLsizei count, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniform1i)(GLint location, GLint v0);
static GLvoid			(APIENTRY * dllUniform1iv)(GLint location, GLsizei count, const GLint *v);
static GLvoid			(APIENTRY * dllUniform1ui)(GLint location, GLuint v0);
static GLvoid			(APIENTRY * dllUniform1uiv)(GLint location, GLsizei count, const GLuint *v);
static GLvoid			(APIENTRY * dllUniform2f)(GLint location, GLfloat v0, GLfloat v1);
static GLvoid			(APIENTRY * dllUniform2fv)(GLint location, GLsizei count, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniform2i)(GLint location, GLint v0, GLint v1);
static GLvoid			(APIENTRY * dllUniform2iv)(GLint location, GLsizei count, const GLint *v);
static GLvoid			(APIENTRY * dllUniform2ui)(GLint location, GLuint v0, GLuint v1);
static GLvoid			(APIENTRY * dllUniform2uiv)(GLint location, GLsizei count, const GLuint *v);
static GLvoid			(APIENTRY * dllUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
static GLvoid			(APIENTRY * dllUniform3fv)(GLint location, GLsizei count, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
static GLvoid			(APIENTRY * dllUniform3iv)(GLint location, GLsizei count, const GLint *v);
static GLvoid			(APIENTRY * dllUniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2);
static GLvoid			(APIENTRY * dllUniform3uiv)(GLint location, GLsizei count, const GLuint *v);
static GLvoid			(APIENTRY * dllUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
static GLvoid			(APIENTRY * dllUniform4fv)(GLint location, GLsizei count, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
static GLvoid			(APIENTRY * dllUniform4iv)(GLint location, GLsizei count, const GLint *v);
static GLvoid			(APIENTRY * dllUniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
static GLvoid			(APIENTRY * dllUniform4uiv)(GLint location, GLsizei count, const GLuint *v);
static GLvoid			(APIENTRY * dllUniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
static GLvoid			(APIENTRY * dllUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
static GLvoid			(APIENTRY * dllUniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v);
static GLboolean		(APIENTRY * dllUnmapBuffer)(GLenum target);
static GLvoid			(APIENTRY * dllUseProgram)(GLuint program);
static GLvoid			(APIENTRY * dllValidateProgram)(GLuint program);
static GLvoid			(APIENTRY * dllVertex2d)(GLdouble x, GLdouble y);
static GLvoid			(APIENTRY * dllVertex2dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllVertex2f)(GLfloat x, GLfloat y);
static GLvoid			(APIENTRY * dllVertex2fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllVertex2i)(GLint x, GLint y);
static GLvoid			(APIENTRY * dllVertex2iv)(const GLint *v);
static GLvoid			(APIENTRY * dllVertex2s)(GLshort x, GLshort y);
static GLvoid			(APIENTRY * dllVertex2sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllVertex3d)(GLdouble x, GLdouble y, GLdouble z);
static GLvoid			(APIENTRY * dllVertex3dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllVertex3f)(GLfloat x, GLfloat y, GLfloat z);
static GLvoid			(APIENTRY * dllVertex3fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllVertex3i)(GLint x, GLint y, GLint z);
static GLvoid			(APIENTRY * dllVertex3iv)(const GLint *v);
static GLvoid			(APIENTRY * dllVertex3s)(GLshort x, GLshort y, GLshort z);
static GLvoid			(APIENTRY * dllVertex3sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllVertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static GLvoid			(APIENTRY * dllVertex4dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static GLvoid			(APIENTRY * dllVertex4fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllVertex4i)(GLint x, GLint y, GLint z, GLint w);
static GLvoid			(APIENTRY * dllVertex4iv)(const GLint *v);
static GLvoid			(APIENTRY * dllVertex4s)(GLshort x, GLshort y, GLshort z, GLshort w);
static GLvoid			(APIENTRY * dllVertex4sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllVertexAttrib1d)(GLuint index, GLdouble x);
static GLvoid			(APIENTRY * dllVertexAttrib1dv)(GLuint index, const GLdouble *v);
static GLvoid			(APIENTRY * dllVertexAttrib1f)(GLuint index, GLfloat x);
static GLvoid			(APIENTRY * dllVertexAttrib1fv)(GLuint index, const GLfloat *v);
static GLvoid			(APIENTRY * dllVertexAttrib1s)(GLuint index, GLshort x);
static GLvoid			(APIENTRY * dllVertexAttrib1sv)(GLuint index, const GLshort *v);
static GLvoid			(APIENTRY * dllVertexAttrib2d)(GLuint index, GLdouble x, GLdouble y);
static GLvoid			(APIENTRY * dllVertexAttrib2dv)(GLuint index, const GLdouble *v);
static GLvoid			(APIENTRY * dllVertexAttrib2f)(GLuint index, GLfloat x, GLfloat y);
static GLvoid			(APIENTRY * dllVertexAttrib2fv)(GLuint index, const GLfloat *v);
static GLvoid			(APIENTRY * dllVertexAttrib2s)(GLuint index, GLshort x, GLshort y);
static GLvoid			(APIENTRY * dllVertexAttrib2sv)(GLuint index, const GLshort *v);
static GLvoid			(APIENTRY * dllVertexAttrib3d)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
static GLvoid			(APIENTRY * dllVertexAttrib3dv)(GLuint index, const GLdouble *v);
static GLvoid			(APIENTRY * dllVertexAttrib3f)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
static GLvoid			(APIENTRY * dllVertexAttrib3fv)(GLuint index, const GLfloat *v);
static GLvoid			(APIENTRY * dllVertexAttrib3s)(GLuint index, GLshort x, GLshort y, GLshort z);
static GLvoid			(APIENTRY * dllVertexAttrib3sv)(GLuint index, const GLshort *v);
static GLvoid			(APIENTRY * dllVertexAttrib4Nbv)(GLuint index, const GLbyte *v);
static GLvoid			(APIENTRY * dllVertexAttrib4Niv)(GLuint index, const GLint *v);
static GLvoid			(APIENTRY * dllVertexAttrib4Nsv)(GLuint index, const GLshort *v);
static GLvoid			(APIENTRY * dllVertexAttrib4Nub)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
static GLvoid			(APIENTRY * dllVertexAttrib4Nubv)(GLuint index, const GLubyte *v);
static GLvoid			(APIENTRY * dllVertexAttrib4Nuiv)(GLuint index, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexAttrib4Nusv)(GLuint index, const GLushort *v);
static GLvoid			(APIENTRY * dllVertexAttrib4bv)(GLuint index, const GLbyte *v);
static GLvoid			(APIENTRY * dllVertexAttrib4d)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static GLvoid			(APIENTRY * dllVertexAttrib4dv)(GLuint index, const GLdouble *v);
static GLvoid			(APIENTRY * dllVertexAttrib4f)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static GLvoid			(APIENTRY * dllVertexAttrib4fv)(GLuint index, const GLfloat *v);
static GLvoid			(APIENTRY * dllVertexAttrib4iv)(GLuint index, const GLint *v);
static GLvoid			(APIENTRY * dllVertexAttrib4s)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
static GLvoid			(APIENTRY * dllVertexAttrib4sv)(GLuint index, const GLshort *v);
static GLvoid			(APIENTRY * dllVertexAttrib4ubv)(GLuint index, const GLubyte *v);
static GLvoid			(APIENTRY * dllVertexAttrib4uiv)(GLuint index, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexAttrib4usv)(GLuint index, const GLushort *v);
static GLvoid			(APIENTRY * dllVertexAttribI1i)(GLuint index, GLint x);
static GLvoid			(APIENTRY * dllVertexAttribI1iv)(GLuint index, const GLint *v);
static GLvoid			(APIENTRY * dllVertexAttribI1ui)(GLuint index, GLuint x);
static GLvoid			(APIENTRY * dllVertexAttribI1uiv)(GLuint index, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexAttribI2i)(GLuint index, GLint x, GLint y);
static GLvoid			(APIENTRY * dllVertexAttribI2iv)(GLuint index, const GLint *v);
static GLvoid			(APIENTRY * dllVertexAttribI2ui)(GLuint index, GLuint x, GLuint y);
static GLvoid			(APIENTRY * dllVertexAttribI2uiv)(GLuint index, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexAttribI3i)(GLuint index, GLint x, GLint y, GLint z);
static GLvoid			(APIENTRY * dllVertexAttribI3iv)(GLuint index, const GLint *v);
static GLvoid			(APIENTRY * dllVertexAttribI3ui)(GLuint index, GLuint x, GLuint y, GLuint z);
static GLvoid			(APIENTRY * dllVertexAttribI3uiv)(GLuint index, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexAttribI4bv)(GLuint index, const GLbyte *v);
static GLvoid			(APIENTRY * dllVertexAttribI4i)(GLuint index, GLint x, GLint y, GLint z, GLint w);
static GLvoid			(APIENTRY * dllVertexAttribI4iv)(GLuint index, const GLint *v);
static GLvoid			(APIENTRY * dllVertexAttribI4sv)(GLuint index, const GLshort *v);
static GLvoid			(APIENTRY * dllVertexAttribI4ubv)(GLuint index, const GLubyte *v);
static GLvoid			(APIENTRY * dllVertexAttribI4ui)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
static GLvoid			(APIENTRY * dllVertexAttribI4uiv)(GLuint index, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexAttribI4usv)(GLuint index, const GLushort *v);
static GLvoid			(APIENTRY * dllVertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllVertexAttribP1ui)(GLuint index, GLenum type, GLboolean normalized, GLuint v);
static GLvoid			(APIENTRY * dllVertexAttribP1uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexAttribP2ui)(GLuint index, GLenum type, GLboolean normalized, GLuint v);
static GLvoid			(APIENTRY * dllVertexAttribP2uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexAttribP3ui)(GLuint index, GLenum type, GLboolean normalized, GLuint v);
static GLvoid			(APIENTRY * dllVertexAttribP3uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexAttribP4ui)(GLuint index, GLenum type, GLboolean normalized, GLuint v);
static GLvoid			(APIENTRY * dllVertexAttribP4uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllVertexP2ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllVertexP2uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexP3ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllVertexP3uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexP4ui)(GLenum type, GLuint v);
static GLvoid			(APIENTRY * dllVertexP4uiv)(GLenum type, const GLuint *v);
static GLvoid			(APIENTRY * dllVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static GLvoid			(APIENTRY * dllViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
static GLvoid			(APIENTRY * dllWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
static GLvoid			(APIENTRY * dllWindowPos2d)(GLdouble x, GLdouble y);
static GLvoid			(APIENTRY * dllWindowPos2dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllWindowPos2f)(GLfloat x, GLfloat y);
static GLvoid			(APIENTRY * dllWindowPos2fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllWindowPos2i)(GLint x, GLint y);
static GLvoid			(APIENTRY * dllWindowPos2iv)(const GLint *v);
static GLvoid			(APIENTRY * dllWindowPos2s)(GLshort x, GLshort y);
static GLvoid			(APIENTRY * dllWindowPos2sv)(const GLshort *v);
static GLvoid			(APIENTRY * dllWindowPos3d)(GLdouble x, GLdouble y, GLdouble z);
static GLvoid			(APIENTRY * dllWindowPos3dv)(const GLdouble *v);
static GLvoid			(APIENTRY * dllWindowPos3f)(GLfloat x, GLfloat y, GLfloat z);
static GLvoid			(APIENTRY * dllWindowPos3fv)(const GLfloat *v);
static GLvoid			(APIENTRY * dllWindowPos3i)(GLint x, GLint y, GLint z);
static GLvoid			(APIENTRY * dllWindowPos3iv)(const GLint *v);
static GLvoid			(APIENTRY * dllWindowPos3s)(GLshort x, GLshort y, GLshort z);
static GLvoid			(APIENTRY * dllWindowPos3sv)(const GLshort *v);


// ============================================================================


static GLvoid APIENTRY logAccum (GLenum op, GLfloat value){

	fprintf(qglState.logFile, "glAccum( 0x%08X, %g )\n", op, value);
	dllAccum(op, value);
}

static GLvoid APIENTRY logActiveStencilFaceEXT (GLenum face){

	const char	*f;

	switch (face){
	case GL_FRONT:	f = "GL_FRONT";						break;
	case GL_BACK:	f = "GL_BACK";						break;
	default:		f = Str_VarArgs("0x%X", face);		break;
	}

	fprintf(qglState.logFile, "glActiveStencilFace( %s )\n", f);
	dllActiveStencilFaceEXT(face);
}

static GLvoid APIENTRY logActiveTexture (GLenum texture){

	const char	*t;

	if (texture >= GL_TEXTURE0 && texture <= GL_TEXTURE31)
		t = Str_VarArgs("GL_TEXTURE%i", texture - GL_TEXTURE0);
	else
		t = Str_VarArgs("0x%08X", texture);

	fprintf(qglState.logFile, "glActiveTexture( %s )\n", t);
	dllActiveTexture(texture);
}

static GLvoid APIENTRY logAlphaFunc (GLenum func, GLclampf ref){

	const char	*f;

	switch (func){
	case GL_NEVER:		f = "GL_NEVER";					break;
	case GL_LESS:		f = "GL_LESS";					break;
	case GL_LEQUAL:		f = "GL_LEQUAL";				break;
	case GL_EQUAL:		f = "GL_EQUAL";					break;
	case GL_NOTEQUAL:	f = "GL_NOTEQUAL";				break;
	case GL_GEQUAL:		f = "GL_GEQUAL";				break;
	case GL_GREATER:	f = "GL_GREATER";				break;
	case GL_ALWAYS:		f = "GL_ALWAYS";				break;
	default:			f = Str_VarArgs("0x%08X", func);	break;
	}

	fprintf(qglState.logFile, "glAlphaFunc( %s, %g )\n", f, ref);
	dllAlphaFunc(func, ref);
}

static GLboolean APIENTRY logAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences){

	fprintf(qglState.logFile, "glAreTexturesResident( %i, %p, %p )\n", n, textures, residences);
	return dllAreTexturesResident(n, textures, residences);
}

static GLvoid APIENTRY logArrayElement (GLint i){

	fprintf(qglState.logFile, "glArrayElement( %i )\n", i);
	dllArrayElement(i);
}

static GLvoid APIENTRY logAttachShader (GLuint program, GLuint shader){

	fprintf(qglState.logFile, "glAttachShader( %u, %u )\n", program, shader);
	dllAttachShader(program, shader);
}

static GLvoid APIENTRY logBegin (GLenum mode){

	const char	*m;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	fprintf(qglState.logFile, "glBegin( %s )\n", m, mode);
	dllBegin(mode);
}

static GLvoid APIENTRY logBeginConditionalRender (GLuint id, GLenum mode){

	fprintf(qglState.logFile, "glBeginConditionalRender( %u, 0x%08X )\n", id, mode);
	dllBeginConditionalRender(id, mode);
}

static GLvoid APIENTRY logBeginQuery (GLenum target, GLuint id){

	const char	*t;

	switch (target){
	case GL_SAMPLES_PASSED:		t = "GL_SAMPLES_PASSED";			break;
	case GL_ANY_SAMPLES_PASSED:	t = "GL_ANY_SAMPLES_PASSED";		break;
	case GL_TIME_ELAPSED:		t = "GL_TIME_ELAPSED";				break;
	default:					t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glBeginQuery( %s, %u )\n", t, id);
	dllBeginQuery(target, id);
}

static GLvoid APIENTRY logBeginTransformFeedback (GLenum primitiveMode){

	fprintf(qglState.logFile, "glBeginTransformFeedback( 0x%08X )\n", primitiveMode);
	dllBeginTransformFeedback(primitiveMode);
}

static GLvoid APIENTRY logBindAttribLocation (GLuint program, GLuint index, const GLchar *name){

	fprintf(qglState.logFile, "glBindAttribLocation( %u, %u, %s )\n", program, index, name);
	dllBindAttribLocation(program, index, name);
}

static GLvoid APIENTRY logBindBuffer (GLenum target, GLuint buffer){

	const char	*t;

	switch (target){
	case GL_ARRAY_BUFFER:				t = "GL_ARRAY_BUFFER";				break;
	case GL_ELEMENT_ARRAY_BUFFER:		t = "GL_ELEMENT_ARRAY_BUFFER";		break;
	case GL_PIXEL_PACK_BUFFER:			t = "GL_PIXEL_PACK_BUFFER";			break;
	case GL_PIXEL_UNPACK_BUFFER:		t = "GL_PIXEL_UNPACK_BUFFER";		break;
	case GL_COPY_READ_BUFFER:			t = "GL_COPY_READ_BUFFER";			break;
	case GL_COPY_WRITE_BUFFER:			t = "GL_COPY_WRITE_BUFFER";			break;
	case GL_TEXTURE_BUFFER:				t = "GL_TEXTURE_BUFFER";			break;
	case GL_UNIFORM_BUFFER:				t = "GL_UNIFORM_BUFFER";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glBindBuffer( %s, %u )\n", t, buffer);
	dllBindBuffer(target, buffer);
}

static GLvoid APIENTRY logBindBufferBase (GLenum target, GLuint index, GLuint buffer){

	const char	*t;

	switch (target){
	case GL_UNIFORM_BUFFER:	t = "GL_UNIFORM_BUFFER";			break;
	default:				t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glBindBufferBase( %s, %u, %u )\n", t, index, buffer);
	dllBindBufferBase(target, index, buffer);
}

static GLvoid APIENTRY logBindBufferRange (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size){

	const char	*t;

	switch (target){
	case GL_UNIFORM_BUFFER:	t = "GL_UNIFORM_BUFFER";			break;
	default:				t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glBindBufferRange( %s, %u, %u, %i, %i )\n", t, index, buffer, offset, size);
	dllBindBufferRange(target, index, buffer, offset, size);
}

static GLvoid APIENTRY logBindFragDataLocation (GLuint program, GLuint color, const GLchar *name){

	fprintf(qglState.logFile, "glBindFragDataLocation( %u, %u, %s )\n", program, color, name);
	dllBindFragDataLocation(program, color, name);
}

static GLvoid APIENTRY logBindFragDataLocationIndexed (GLuint program, GLuint color, GLuint index, const GLchar *name){

	fprintf(qglState.logFile, "glBindFragDataLocationIndexed( %u, %u, %u, %s )\n", program, color, index, name);
	dllBindFragDataLocationIndexed(program, color, index, name);
}

static GLvoid APIENTRY logBindFramebuffer (GLenum target, GLuint framebuffer){

	const char	*t;

	switch (target){
	case GL_FRAMEBUFFER:		t = "GL_FRAMEBUFFER";				break;
	case GL_DRAW_FRAMEBUFFER:	t = "GL_DRAW_FRAMEBUFFER";			break;
	case GL_READ_FRAMEBUFFER:	t = "GL_READ_FRAMEBUFFER";			break;
	default:					t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glBindFramebuffer( %s, %u )\n", t, framebuffer);
	dllBindFramebuffer(target, framebuffer);
}

static GLvoid APIENTRY logBindRenderbuffer (GLenum target, GLuint renderbuffer){

	const char	*t;

	switch (target){
	case GL_RENDERBUFFER:	t = "GL_RENDERBUFFER";				break;
	default:				t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glBindRenderbuffer( %s, %u )\n", t, renderbuffer);
	dllBindRenderbuffer(target, renderbuffer);
}

static GLvoid APIENTRY logBindSampler (GLuint unit, GLuint sampler){

	fprintf(qglState.logFile, "glBindSampler( %u, %u )\n", unit, sampler);
	dllBindSampler(unit, sampler);
}

static GLvoid APIENTRY logBindTexture (GLenum target, GLuint texture){

	const char	*t;

	switch (target){
	case GL_TEXTURE_1D:						t = "GL_TEXTURE_1D";					break;
	case GL_TEXTURE_1D_ARRAY:				t = "GL_TEXTURE_1D_ARRAY";				break;
	case GL_TEXTURE_2D:						t = "GL_TEXTURE_2D";					break;
	case GL_TEXTURE_2D_ARRAY:				t = "GL_TEXTURE_2D_ARRAY";				break;
	case GL_TEXTURE_2D_MULTISAMPLE:			t = "GL_TEXTURE_2D_MULTISAMPLE";		break;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:	t = "GL_TEXTURE_2D_MULTISAMPLE_ARRAY";	break;
	case GL_TEXTURE_3D:						t = "GL_TEXTURE_3D";					break;
	case GL_TEXTURE_BUFFER:					t = "GL_TEXTURE_BUFFER";				break;
	case GL_TEXTURE_CUBE_MAP:				t = "GL_TEXTURE_CUBE_MAP";				break;
	case GL_TEXTURE_RECTANGLE:				t = "GL_TEXTURE_RECTANGLE";				break;
	default:								t = Str_VarArgs("0x%08X", target);		break;
	}

	fprintf(qglState.logFile, "glBindTexture( %s, %u )\n", t, texture);
	dllBindTexture(target, texture);
}

static GLvoid APIENTRY logBindVertexArray (GLuint vertexArray){

	fprintf(qglState.logFile, "glBindVertexArray( %u )\n", vertexArray);
	dllBindVertexArray(vertexArray);
}

static GLvoid APIENTRY logBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap){

	fprintf(qglState.logFile, "glBitmap( %i, %i, %g, %g, %g, %g, %p )\n", width, height, xorig, yorig, xmove, ymove, bitmap);
	dllBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
}

static GLvoid APIENTRY logBlendColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){

	fprintf(qglState.logFile, "glBlendColor( %g, %g, %g, %g )\n", red, green, blue, alpha);
	dllBlendColor(red, green, blue, alpha);
}

static GLvoid APIENTRY logBlendEquation (GLenum mode){

	const char	*m;

	switch (mode){
	case GL_FUNC_ADD:				m = "GL_FUNC_ADD";				break;
	case GL_FUNC_SUBTRACT:			m = "GL_FUNC_SUBTRACT";			break;
	case GL_FUNC_REVERSE_SUBTRACT:	m = "GL_FUNC_REVERSE_SUBTRACT";	break;
	case GL_MIN:					m = "GL_MIN";					break;
	case GL_MAX:					m = "GL_MAX";					break;
	default:						m = Str_VarArgs("0x%08X", mode);	break;
	}

	fprintf(qglState.logFile, "glBlendEquation( %s )\n", m);
	dllBlendEquation(mode);
}

static GLvoid APIENTRY logBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha){

	const char	*mc, *ma;

	switch (modeRGB){
	case GL_FUNC_ADD:				mc = "GL_FUNC_ADD";						break;
	case GL_FUNC_SUBTRACT:			mc = "GL_FUNC_SUBTRACT";				break;
	case GL_FUNC_REVERSE_SUBTRACT:	mc = "GL_FUNC_REVERSE_SUBTRACT";		break;
	case GL_MIN:					mc = "GL_MIN";							break;
	case GL_MAX:					mc = "GL_MAX";							break;
	default:						mc = Str_VarArgs("0x%08X", modeRGB);		break;
	}

	switch (modeAlpha){
	case GL_FUNC_ADD:				ma = "GL_FUNC_ADD";						break;
	case GL_FUNC_SUBTRACT:			ma = "GL_FUNC_SUBTRACT";				break;
	case GL_FUNC_REVERSE_SUBTRACT:	ma = "GL_FUNC_REVERSE_SUBTRACT";		break;
	case GL_MIN:					ma = "GL_MIN";							break;
	case GL_MAX:					ma = "GL_MAX";							break;
	default:						ma = Str_VarArgs("0x%08X", modeAlpha);	break;
	}

	fprintf(qglState.logFile, "glBlendEquationSeparate( %s, %s )\n", mc, ma);
	dllBlendEquationSeparate(modeRGB, modeAlpha);
}

static GLvoid APIENTRY logBlendFunc (GLenum sfactor, GLenum dfactor){

	const char	*s, *d;

	switch (sfactor){
	case GL_ZERO:						s = "GL_ZERO";						break;
	case GL_ONE:						s = "GL_ONE";						break;
	case GL_SRC_COLOR:					s = "GL_SRC_COLOR";					break;
	case GL_ONE_MINUS_SRC_COLOR:		s = "GL_ONE_MINUS_SRC_COLOR";		break;
	case GL_SRC1_COLOR:					s = "GL_SRC1_COLOR";				break;
	case GL_ONE_MINUS_SRC1_COLOR:		s = "GL_ONE_MINUS_SRC1_COLOR";		break;
	case GL_DST_COLOR:					s = "GL_DST_COLOR";					break;
	case GL_ONE_MINUS_DST_COLOR:		s = "GL_ONE_MINUS_DST_COLOR";		break;
	case GL_CONSTANT_COLOR:				s = "GL_CONSTANT_COLOR";			break;
	case GL_ONE_MINUS_CONSTANT_COLOR:	s = "GL_ONE_MINUS_CONSTANT_COLOR";	break;
	case GL_SRC_ALPHA:					s = "GL_SRC_ALPHA";					break;
	case GL_ONE_MINUS_SRC_ALPHA:		s = "GL_ONE_MINUS_SRC_ALPHA";		break;
	case GL_SRC1_ALPHA:					s = "GL_SRC1_ALPHA";				break;
	case GL_ONE_MINUS_SRC1_ALPHA:		s = "GL_ONE_MINUS_SRC1_ALPHA";		break;
	case GL_DST_ALPHA:					s = "GL_DST_ALPHA";					break;
	case GL_ONE_MINUS_DST_ALPHA:		s = "GL_ONE_MINUS_DST_ALPHA";		break;
	case GL_CONSTANT_ALPHA:				s = "GL_CONSTANT_ALPHA";			break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:	s = "GL_ONE_MINUS_CONSTANT_ALPHA";	break;
	case GL_SRC_ALPHA_SATURATE:			s = "GL_SRC_ALPHA_SATURATE";		break;
	default:							s = Str_VarArgs("0x%08X", sfactor);	break;
	}

	switch (dfactor){
	case GL_ZERO:						d = "GL_ZERO";						break;
	case GL_ONE:						d = "GL_ONE";						break;
	case GL_SRC_COLOR:					d = "GL_SRC_COLOR";					break;
	case GL_ONE_MINUS_SRC_COLOR:		d = "GL_ONE_MINUS_SRC_COLOR";		break;
	case GL_SRC1_COLOR:					d = "GL_SRC1_COLOR";				break;
	case GL_ONE_MINUS_SRC1_COLOR:		d = "GL_ONE_MINUS_SRC1_COLOR";		break;
	case GL_DST_COLOR:					d = "GL_DST_COLOR";					break;
	case GL_ONE_MINUS_DST_COLOR:		d = "GL_ONE_MINUS_DST_COLOR";		break;
	case GL_CONSTANT_COLOR:				d = "GL_CONSTANT_COLOR";			break;
	case GL_ONE_MINUS_CONSTANT_COLOR:	d = "GL_ONE_MINUS_CONSTANT_COLOR";	break;
	case GL_SRC_ALPHA:					d = "GL_SRC_ALPHA";					break;
	case GL_ONE_MINUS_SRC_ALPHA:		d = "GL_ONE_MINUS_SRC_ALPHA";		break;
	case GL_SRC1_ALPHA:					d = "GL_SRC1_ALPHA";				break;
	case GL_ONE_MINUS_SRC1_ALPHA:		d = "GL_ONE_MINUS_SRC1_ALPHA";		break;
	case GL_DST_ALPHA:					d = "GL_DST_ALPHA";					break;
	case GL_ONE_MINUS_DST_ALPHA:		d = "GL_ONE_MINUS_DST_ALPHA";		break;
	case GL_CONSTANT_ALPHA:				d = "GL_CONSTANT_ALPHA";			break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:	d = "GL_ONE_MINUS_CONSTANT_ALPHA";	break;
	case GL_SRC_ALPHA_SATURATE:			d = "GL_SRC_ALPHA_SATURATE";		break;
	default:							d = Str_VarArgs("0x%08X", dfactor);	break;
	}

	fprintf(qglState.logFile, "glBlendFunc( %s, %s )\n", s, d);
	dllBlendFunc(sfactor, dfactor);
}

static GLvoid APIENTRY logBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha){

	const char	*sc, *dc, *sa, *da;

	switch (sfactorRGB){
	case GL_ZERO:						sc = "GL_ZERO";							break;
	case GL_ONE:						sc = "GL_ONE";							break;
	case GL_SRC_COLOR:					sc = "GL_SRC_COLOR";					break;
	case GL_ONE_MINUS_SRC_COLOR:		sc = "GL_ONE_MINUS_SRC_COLOR";			break;
	case GL_SRC1_COLOR:					sc = "GL_SRC1_COLOR";					break;
	case GL_ONE_MINUS_SRC1_COLOR:		sc = "GL_ONE_MINUS_SRC1_COLOR";			break;
	case GL_DST_COLOR:					sc = "GL_DST_COLOR";					break;
	case GL_ONE_MINUS_DST_COLOR:		sc = "GL_ONE_MINUS_DST_COLOR";			break;
	case GL_CONSTANT_COLOR:				sc = "GL_CONSTANT_COLOR";				break;
	case GL_ONE_MINUS_CONSTANT_COLOR:	sc = "GL_ONE_MINUS_CONSTANT_COLOR";		break;
	case GL_SRC_ALPHA:					sc = "GL_SRC_ALPHA";					break;
	case GL_ONE_MINUS_SRC_ALPHA:		sc = "GL_ONE_MINUS_SRC_ALPHA";			break;
	case GL_SRC1_ALPHA:					sc = "GL_SRC1_ALPHA";					break;
	case GL_ONE_MINUS_SRC1_ALPHA:		sc = "GL_ONE_MINUS_SRC1_ALPHA";			break;
	case GL_DST_ALPHA:					sc = "GL_DST_ALPHA";					break;
	case GL_ONE_MINUS_DST_ALPHA:		sc = "GL_ONE_MINUS_DST_ALPHA";			break;
	case GL_CONSTANT_ALPHA:				sc = "GL_CONSTANT_ALPHA";				break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:	sc = "GL_ONE_MINUS_CONSTANT_ALPHA";		break;
	case GL_SRC_ALPHA_SATURATE:			sc = "GL_SRC_ALPHA_SATURATE";			break;
	default:							sc = Str_VarArgs("0x%08X", sfactorRGB);	break;
	}

	switch (dfactorRGB){
	case GL_ZERO:						dc = "GL_ZERO";							break;
	case GL_ONE:						dc = "GL_ONE";							break;
	case GL_SRC_COLOR:					dc = "GL_SRC_COLOR";					break;
	case GL_ONE_MINUS_SRC_COLOR:		dc = "GL_ONE_MINUS_SRC_COLOR";			break;
	case GL_SRC1_COLOR:					dc = "GL_SRC1_COLOR";					break;
	case GL_ONE_MINUS_SRC1_COLOR:		dc = "GL_ONE_MINUS_SRC1_COLOR";			break;
	case GL_DST_COLOR:					dc = "GL_DST_COLOR";					break;
	case GL_ONE_MINUS_DST_COLOR:		dc = "GL_ONE_MINUS_DST_COLOR";			break;
	case GL_CONSTANT_COLOR:				dc = "GL_CONSTANT_COLOR";				break;
	case GL_ONE_MINUS_CONSTANT_COLOR:	dc = "GL_ONE_MINUS_CONSTANT_COLOR";		break;
	case GL_SRC_ALPHA:					dc = "GL_SRC_ALPHA";					break;
	case GL_ONE_MINUS_SRC_ALPHA:		dc = "GL_ONE_MINUS_SRC_ALPHA";			break;
	case GL_SRC1_ALPHA:					dc = "GL_SRC1_ALPHA";					break;
	case GL_ONE_MINUS_SRC1_ALPHA:		dc = "GL_ONE_MINUS_SRC1_ALPHA";			break;
	case GL_DST_ALPHA:					dc = "GL_DST_ALPHA";					break;
	case GL_ONE_MINUS_DST_ALPHA:		dc = "GL_ONE_MINUS_DST_ALPHA";			break;
	case GL_CONSTANT_ALPHA:				dc = "GL_CONSTANT_ALPHA";				break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:	dc = "GL_ONE_MINUS_CONSTANT_ALPHA";		break;
	case GL_SRC_ALPHA_SATURATE:			dc = "GL_SRC_ALPHA_SATURATE";			break;
	default:							dc = Str_VarArgs("0x%08X", dfactorRGB);	break;
	}

	switch (sfactorAlpha){
	case GL_ZERO:						sa = "GL_ZERO";							break;
	case GL_ONE:						sa = "GL_ONE";							break;
	case GL_SRC_COLOR:					sa = "GL_SRC_COLOR";					break;
	case GL_ONE_MINUS_SRC_COLOR:		sa = "GL_ONE_MINUS_SRC_COLOR";			break;
	case GL_SRC1_COLOR:					sa = "GL_SRC1_COLOR";					break;
	case GL_ONE_MINUS_SRC1_COLOR:		sa = "GL_ONE_MINUS_SRC1_COLOR";			break;
	case GL_DST_COLOR:					sa = "GL_DST_COLOR";					break;
	case GL_ONE_MINUS_DST_COLOR:		sa = "GL_ONE_MINUS_DST_COLOR";			break;
	case GL_CONSTANT_COLOR:				sa = "GL_CONSTANT_COLOR";				break;
	case GL_ONE_MINUS_CONSTANT_COLOR:	sa = "GL_ONE_MINUS_CONSTANT_COLOR";		break;
	case GL_SRC_ALPHA:					sa = "GL_SRC_ALPHA";					break;
	case GL_ONE_MINUS_SRC_ALPHA:		sa = "GL_ONE_MINUS_SRC_ALPHA";			break;
	case GL_SRC1_ALPHA:					sa = "GL_SRC1_ALPHA";					break;
	case GL_ONE_MINUS_SRC1_ALPHA:		sa = "GL_ONE_MINUS_SRC1_ALPHA";			break;
	case GL_DST_ALPHA:					sa = "GL_DST_ALPHA";					break;
	case GL_ONE_MINUS_DST_ALPHA:		sa = "GL_ONE_MINUS_DST_ALPHA";			break;
	case GL_CONSTANT_ALPHA:				sa = "GL_CONSTANT_ALPHA";				break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:	sa = "GL_ONE_MINUS_CONSTANT_ALPHA";		break;
	case GL_SRC_ALPHA_SATURATE:			sa = "GL_SRC_ALPHA_SATURATE";			break;
	default:							sa = Str_VarArgs("0x%08X", sfactorAlpha);	break;
	}

	switch (dfactorAlpha){
	case GL_ZERO:						da = "GL_ZERO";							break;
	case GL_ONE:						da = "GL_ONE";							break;
	case GL_SRC_COLOR:					da = "GL_SRC_COLOR";					break;
	case GL_ONE_MINUS_SRC_COLOR:		da = "GL_ONE_MINUS_SRC_COLOR";			break;
	case GL_SRC1_COLOR:					da = "GL_SRC1_COLOR";					break;
	case GL_ONE_MINUS_SRC1_COLOR:		da = "GL_ONE_MINUS_SRC1_COLOR";			break;
	case GL_DST_COLOR:					da = "GL_DST_COLOR";					break;
	case GL_ONE_MINUS_DST_COLOR:		da = "GL_ONE_MINUS_DST_COLOR";			break;
	case GL_CONSTANT_COLOR:				da = "GL_CONSTANT_COLOR";				break;
	case GL_ONE_MINUS_CONSTANT_COLOR:	da = "GL_ONE_MINUS_CONSTANT_COLOR";		break;
	case GL_SRC_ALPHA:					da = "GL_SRC_ALPHA";					break;
	case GL_ONE_MINUS_SRC_ALPHA:		da = "GL_ONE_MINUS_SRC_ALPHA";			break;
	case GL_SRC1_ALPHA:					da = "GL_SRC1_ALPHA";					break;
	case GL_ONE_MINUS_SRC1_ALPHA:		da = "GL_ONE_MINUS_SRC1_ALPHA";			break;
	case GL_DST_ALPHA:					da = "GL_DST_ALPHA";					break;
	case GL_ONE_MINUS_DST_ALPHA:		da = "GL_ONE_MINUS_DST_ALPHA";			break;
	case GL_CONSTANT_ALPHA:				da = "GL_CONSTANT_ALPHA";				break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:	da = "GL_ONE_MINUS_CONSTANT_ALPHA";		break;
	case GL_SRC_ALPHA_SATURATE:			da = "GL_SRC_ALPHA_SATURATE";			break;
	default:							da = Str_VarArgs("0x%08X", dfactorAlpha);	break;
	}

	fprintf(qglState.logFile, "glBlendFuncSeparate( %s, %s, %s, %s )\n", sc, dc, sa, da);
	dllBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}

static GLvoid APIENTRY logBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter){

	const char	*f;

	fprintf(qglState.logFile, "glBlitFramebuffer( %i, %i, %i, %i, %i, %i, %i, %i, %u", srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask);

	if (mask){
		fprintf(qglState.logFile, " = ( ");

		if (mask & GL_COLOR_BUFFER_BIT)
			fprintf(qglState.logFile, "GL_COLOR_BUFFER_BIT ");
		if (mask & GL_DEPTH_BUFFER_BIT)
			fprintf(qglState.logFile, "GL_DEPTH_BUFFER_BIT ");
		if (mask & GL_STENCIL_BUFFER_BIT)
			fprintf(qglState.logFile, "GL_STENCIL_BUFFER_BIT ");

		fprintf(qglState.logFile, ")");
	}

	switch (filter){
	case GL_NEAREST:	f = "GL_NEAREST";					break;
	case GL_LINEAR:		f = "GL_LINEAR";					break;
	default:			f = Str_VarArgs("0x%08X", filter);	break;
	}

	fprintf(qglState.logFile, ", %s )\n", f);
	dllBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

static GLvoid APIENTRY logBufferData (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage){

	const char	*t, *u;

	switch (target){
	case GL_ARRAY_BUFFER:				t = "GL_ARRAY_BUFFER";				break;
	case GL_ELEMENT_ARRAY_BUFFER:		t = "GL_ELEMENT_ARRAY_BUFFER";		break;
	case GL_PIXEL_PACK_BUFFER:			t = "GL_PIXEL_PACK_BUFFER";			break;
	case GL_PIXEL_UNPACK_BUFFER:		t = "GL_PIXEL_UNPACK_BUFFER";		break;
	case GL_COPY_READ_BUFFER:			t = "GL_COPY_READ_BUFFER";			break;
	case GL_COPY_WRITE_BUFFER:			t = "GL_COPY_WRITE_BUFFER";			break;
	case GL_TEXTURE_BUFFER:				t = "GL_TEXTURE_BUFFER";			break;
	case GL_UNIFORM_BUFFER:				t = "GL_UNIFORM_BUFFER";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (usage){
	case GL_STATIC_DRAW:				u = "GL_STATIC_DRAW";				break;
	case GL_STATIC_READ:				u = "GL_STATIC_READ";				break;
	case GL_STATIC_COPY:				u = "GL_STATIC_COPY";				break;
	case GL_DYNAMIC_DRAW:				u = "GL_DYNAMIC_DRAW";				break;
	case GL_DYNAMIC_READ:				u = "GL_DYNAMIC_READ";				break;
	case GL_DYNAMIC_COPY:				u = "GL_DYNAMIC_COPY";				break;
	case GL_STREAM_DRAW:				u = "GL_STREAM_DRAW";				break;
	case GL_STREAM_READ:				u = "GL_STREAM_READ";				break;
	case GL_STREAM_COPY:				u = "GL_STREAM_COPY";				break;
	default:							u = Str_VarArgs("0x%08X", usage);		break;
	}

	fprintf(qglState.logFile, "glBufferData( %s, %i, %p, %s )\n", t, size, data, u);
	dllBufferData(target, size, data, usage);
}

static GLvoid APIENTRY logBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data){

	const char	*t;

	switch (target){
	case GL_ARRAY_BUFFER:				t = "GL_ARRAY_BUFFER";				break;
	case GL_ELEMENT_ARRAY_BUFFER:		t = "GL_ELEMENT_ARRAY_BUFFER";		break;
	case GL_PIXEL_PACK_BUFFER:			t = "GL_PIXEL_PACK_BUFFER";			break;
	case GL_PIXEL_UNPACK_BUFFER:		t = "GL_PIXEL_UNPACK_BUFFER";		break;
	case GL_COPY_READ_BUFFER:			t = "GL_COPY_READ_BUFFER";			break;
	case GL_COPY_WRITE_BUFFER:			t = "GL_COPY_WRITE_BUFFER";			break;
	case GL_TEXTURE_BUFFER:				t = "GL_TEXTURE_BUFFER";			break;
	case GL_UNIFORM_BUFFER:				t = "GL_UNIFORM_BUFFER";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glBufferSubData( %s, %i, %i, %p )\n", t, offset, size, data);
	dllBufferSubData(target, offset, size, data);
}

static GLvoid APIENTRY logCallList (GLuint list){

	fprintf(qglState.logFile, "glCallList( %u )\n", list);
	dllCallList(list);
}

static GLvoid APIENTRY logCallLists (GLsizei n, GLenum type, const GLvoid *lists){

	fprintf(qglState.logFile, "glCallLists( %i, 0x%08X, %p )\n", n, type, lists);
	dllCallLists(n, type, lists);
}

static GLenum APIENTRY logCheckFramebufferStatus (GLenum target){

	const char	*t;

	switch (target){
	case GL_FRAMEBUFFER:		t = "GL_FRAMEBUFFER";				break;
	case GL_DRAW_FRAMEBUFFER:	t = "GL_DRAW_FRAMEBUFFER";			break;
	case GL_READ_FRAMEBUFFER:	t = "GL_READ_FRAMEBUFFER";			break;
	default:					t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glCheckFramebufferStatus( %s )\n", t);
	return dllCheckFramebufferStatus(target);
}

static GLvoid APIENTRY logClampColor (GLenum target, GLenum clamp){

	const char	*t, *c;

	switch (target){
	case GL_CLAMP_VERTEX_COLOR:		t = "GL_CLAMP_VERTEX_COLOR";		break;
	case GL_CLAMP_FRAGMENT_COLOR:	t = "GL_CLAMP_FRAGMENT_COLOR";		break;
	case GL_CLAMP_READ_COLOR:		t = "GL_CLAMP_READ_COLOR";			break;
	default:						t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (clamp){
	case GL_FALSE:					c = "GL_FALSE";						break;
	case GL_TRUE:					c = "GL_TRUE";						break;
	default:						c = Str_VarArgs("0x%08X", clamp);		break;
	}

	fprintf(qglState.logFile, "glClampColor( %s, %s )\n", t, c);
	dllClampColor(target, clamp);
}

static GLvoid APIENTRY logClear (GLbitfield mask){

	fprintf(qglState.logFile, "glClear( %u", mask);

	if (mask){
		fprintf(qglState.logFile, " = ( ");

		if (mask & GL_ACCUM_BUFFER_BIT)
			fprintf(qglState.logFile, "GL_ACCUM_BUFFER_BIT ");
		if (mask & GL_COLOR_BUFFER_BIT)
			fprintf(qglState.logFile, "GL_COLOR_BUFFER_BIT ");
		if (mask & GL_DEPTH_BUFFER_BIT)
			fprintf(qglState.logFile, "GL_DEPTH_BUFFER_BIT ");
		if (mask & GL_STENCIL_BUFFER_BIT)
			fprintf(qglState.logFile, "GL_STENCIL_BUFFER_BIT ");

		fprintf(qglState.logFile, ")");
	}

	fprintf(qglState.logFile, " )\n");
	dllClear(mask);
}

static GLvoid APIENTRY logClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){

	fprintf(qglState.logFile, "glClearAccum( %g, %g, %g, %g )\n", red, green, blue, alpha);
	dllClearAccum(red, green, blue, alpha);
}

static GLvoid APIENTRY logClearBufferfi (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil){

	const char	*b;

	switch (buffer){
	case GL_DEPTH_STENCIL:	b = "GL_DEPTH_STENCIL";				break;
	default:				b = Str_VarArgs("0x%08X", buffer);	break;
	}

	fprintf(qglState.logFile, "glClearBufferfi( %s, %i, %g, %i )\n", b, drawbuffer, depth, stencil);
	dllClearBufferfi(buffer, drawbuffer, depth, stencil);
}

static GLvoid APIENTRY logClearBufferfv (GLenum buffer, GLint drawbuffer, const GLfloat *value){

	const char	*b;

	switch (buffer){
	case GL_COLOR:			b = "GL_COLOR";						break;
	case GL_DEPTH:			b = "GL_DEPTH";						break;
	case GL_STENCIL:		b = "GL_STENCIL";					break;
	default:				b = Str_VarArgs("0x%08X", buffer);	break;
	}

	fprintf(qglState.logFile, "glClearBufferfv( %s, %i, %p )\n", b, drawbuffer, value);
	dllClearBufferfv(buffer, drawbuffer, value);
}

static GLvoid APIENTRY logClearBufferiv (GLenum buffer, GLint drawbuffer, const GLint *value){

	const char	*b;

	switch (buffer){
	case GL_COLOR:			b = "GL_COLOR";						break;
	case GL_DEPTH:			b = "GL_DEPTH";						break;
	case GL_STENCIL:		b = "GL_STENCIL";					break;
	default:				b = Str_VarArgs("0x%08X", buffer);	break;
	}

	fprintf(qglState.logFile, "glClearBufferiv( %s, %i, %p )\n", b, drawbuffer, value);
	dllClearBufferiv(buffer, drawbuffer, value);
}

static GLvoid APIENTRY logClearBufferuiv (GLenum buffer, GLint drawbuffer, const GLuint *value){

	const char	*b;

	switch (buffer){
	case GL_COLOR:			b = "GL_COLOR";						break;
	case GL_DEPTH:			b = "GL_DEPTH";						break;
	case GL_STENCIL:		b = "GL_STENCIL";					break;
	default:				b = Str_VarArgs("0x%08X", buffer);	break;
	}

	fprintf(qglState.logFile, "glClearBufferuiv( %s, %i, %p )\n", b, drawbuffer, value);
	dllClearBufferuiv(buffer, drawbuffer, value);
}

static GLvoid APIENTRY logClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){

	fprintf(qglState.logFile, "glClearColor( %g, %g, %g, %g )\n", red, green, blue, alpha);
	dllClearColor(red, green, blue, alpha);
}

static GLvoid APIENTRY logClearDepth (GLclampd depth){

	fprintf(qglState.logFile, "glClearDepth( %g )\n", depth);
	dllClearDepth(depth);
}

static GLvoid APIENTRY logClearIndex (GLfloat c){

	fprintf(qglState.logFile, "glClearIndex( %g )\n", c);
	dllClearIndex(c);
}

static GLvoid APIENTRY logClearStencil (GLint s){

	fprintf(qglState.logFile, "glClearStencil( %i )\n", s);
	dllClearStencil(s);
}

static GLvoid APIENTRY logClientActiveTexture (GLenum texture){

	const char	*t;

	if (texture >= GL_TEXTURE0 && texture <= GL_TEXTURE31)
		t = Str_VarArgs("GL_TEXTURE%i", texture - GL_TEXTURE0);
	else
		t = Str_VarArgs("0x%08X", texture);

	fprintf(qglState.logFile, "glClientActiveTexture( %s )\n", t);
	dllClientActiveTexture(texture);
}

static GLenum APIENTRY logClientWaitSync (GLsync sync, GLbitfield flags, GLuint64 timeout){

	fprintf(qglState.logFile, "glClientWaitSync( %p, %u", sync, flags);

	if (flags){
		fprintf(qglState.logFile, " = ( ");

		if (flags & GL_SYNC_FLUSH_COMMANDS_BIT)
			fprintf(qglState.logFile, "GL_SYNC_FLUSH_COMMANDS_BIT ");

		fprintf(qglState.logFile, ")");
	}

	fprintf(qglState.logFile, ", %I64u )\n", timeout);
	return dllClientWaitSync(sync, flags, timeout);
}

static GLvoid APIENTRY logClipPlane (GLenum plane, const GLdouble *equation){

	const char	*p;

	if (plane >= GL_CLIP_PLANE0 && plane <= GL_CLIP_PLANE5)
		p = Str_VarArgs("GL_CLIP_PLANE%i", plane - GL_CLIP_PLANE0);
	else
		p = Str_VarArgs("0x%08X", plane);

	fprintf(qglState.logFile, "glClipPlane( %s, %p )\n", p, equation);
	dllClipPlane(plane, equation);
}

static GLvoid APIENTRY logColor3b (GLbyte red, GLbyte green, GLbyte blue){

	fprintf(qglState.logFile, "glColor3b( %i, %i, %i )\n", red, green, blue);
	dllColor3b(red, green, blue);
}

static GLvoid APIENTRY logColor3bv (const GLbyte *v){

	fprintf(qglState.logFile, "glColor3bv( %p )\n", v);
	dllColor3bv(v);
}

static GLvoid APIENTRY logColor3d (GLdouble red, GLdouble green, GLdouble blue){

	fprintf(qglState.logFile, "glColor3d( %g, %g, %g )\n", red, green, blue);
	dllColor3d(red, green, blue);
}

static GLvoid APIENTRY logColor3dv (const GLdouble *v){

	fprintf(qglState.logFile, "glColor3dv( %p )\n", v);
	dllColor3dv(v);
}

static GLvoid APIENTRY logColor3f (GLfloat red, GLfloat green, GLfloat blue){

	fprintf(qglState.logFile, "glColor3f( %g, %g, %g )\n", red, green, blue);
	dllColor3f(red, green, blue);
}

static GLvoid APIENTRY logColor3fv (const GLfloat *v){

	fprintf(qglState.logFile, "glColor3fv( %p )\n", v);
	dllColor3fv(v);
}

static GLvoid APIENTRY logColor3i (GLint red, GLint green, GLint blue){

	fprintf(qglState.logFile, "glColor3i( %i, %i, %i )\n", red, green, blue);
	dllColor3i(red, green, blue);
}

static GLvoid APIENTRY logColor3iv (const GLint *v){

	fprintf(qglState.logFile, "glColor3iv( %p )\n", v);
	dllColor3iv(v);
}

static GLvoid APIENTRY logColor3s (GLshort red, GLshort green, GLshort blue){

	fprintf(qglState.logFile, "glColor3s( %i, %i, %i )\n", red, green, blue);
	dllColor3s(red, green, blue);
}

static GLvoid APIENTRY logColor3sv (const GLshort *v){

	fprintf(qglState.logFile, "glColor3sv( %p )\n", v);
	dllColor3sv(v);
}

static GLvoid APIENTRY logColor3ub (GLubyte red, GLubyte green, GLubyte blue){

	fprintf(qglState.logFile, "glColor3ub( %u, %u, %u )\n", red, green, blue);
	dllColor3ub(red, green, blue);
}

static GLvoid APIENTRY logColor3ubv (const GLubyte *v){

	fprintf(qglState.logFile, "glColor3ubv( %p )\n", v);
	dllColor3ubv(v);
}

static GLvoid APIENTRY logColor3ui (GLuint red, GLuint green, GLuint blue){

	fprintf(qglState.logFile, "glColor3ui( %u, %u, %u )\n", red, green, blue);
	dllColor3ui(red, green, blue);
}

static GLvoid APIENTRY logColor3uiv (const GLuint *v){

	fprintf(qglState.logFile, "glColor3uiv( %p )\n", v);
	dllColor3uiv(v);
}

static GLvoid APIENTRY logColor3us (GLushort red, GLushort green, GLushort blue){

	fprintf(qglState.logFile, "glColor3us( %u, %u, %u )\n", red, green, blue);
	dllColor3us(red, green, blue);
}

static GLvoid APIENTRY logColor3usv (const GLushort *v){

	fprintf(qglState.logFile, "glColor3usv( %p )\n", v);
	dllColor3usv(v);
}

static GLvoid APIENTRY logColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha){

	fprintf(qglState.logFile, "glColor4b( %i, %i, %i, %i )\n", red, green, blue, alpha);
	dllColor4b(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4bv (const GLbyte *v){

	fprintf(qglState.logFile, "glColor4bv( %p )\n", v);
	dllColor4bv(v);
}

static GLvoid APIENTRY logColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha){

	fprintf(qglState.logFile, "glColor4d( %g, %g, %g, %g )\n", red, green, blue, alpha);
	dllColor4d(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4dv (const GLdouble *v){

	fprintf(qglState.logFile, "glColor4dv( %p )\n", v);
	dllColor4dv(v);
}

static GLvoid APIENTRY logColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){

	fprintf(qglState.logFile, "glColor4f( %g, %g, %g, %g )\n", red, green, blue, alpha);
	dllColor4f(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4fv (const GLfloat *v){

	fprintf(qglState.logFile, "glColor4fv( %p )\n", v);
	dllColor4fv(v);
}

static GLvoid APIENTRY logColor4i (GLint red, GLint green, GLint blue, GLint alpha){

	fprintf(qglState.logFile, "glColor4i( %i, %i, %i, %i )\n", red, green, blue, alpha);
	dllColor4i(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4iv (const GLint *v){

	fprintf(qglState.logFile, "glColor4iv( %p )\n", v);
	dllColor4iv(v);
}

static GLvoid APIENTRY logColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha){

	fprintf(qglState.logFile, "glColor4s( %i, %i, %i, %i )\n", red, green, blue, alpha);
	dllColor4s(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4sv (const GLshort *v){

	fprintf(qglState.logFile, "glColor4sv( %p )\n", v);
	dllColor4sv(v);
}

static GLvoid APIENTRY logColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha){

	fprintf(qglState.logFile, "glColor4ub( %u, %u, %u, %u )\n", red, green, blue, alpha);
	dllColor4b(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4ubv (const GLubyte *v){

	fprintf(qglState.logFile, "glColor4ubv( %p )\n", v);
	dllColor4ubv(v);
}

static GLvoid APIENTRY logColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha){

	fprintf(qglState.logFile, "glColor4ui( %u, %u, %u, %u )\n", red, green, blue, alpha);
	dllColor4ui(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4uiv (const GLuint *v){

	fprintf(qglState.logFile, "glColor4uiv( %p )\n", v);
	dllColor4uiv(v);
}

static GLvoid APIENTRY logColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha){

	fprintf(qglState.logFile, "glColor4us( %u, %u, %u, %u )\n", red, green, blue, alpha);
	dllColor4us(red, green, blue, alpha);
}

static GLvoid APIENTRY logColor4usv (const GLushort *v){

	fprintf(qglState.logFile, "glColor4usv( %p )\n", v);
	dllColor4usv(v);
}

static GLvoid APIENTRY logColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){

	const char	*r, *g, *b, *a;

	switch (red){
	case GL_FALSE:	r = "GL_FALSE";					break;
	case GL_TRUE:	r = "GL_TRUE";					break;
	default:		r = Str_VarArgs("0x%08X", red);	break;
	}

	switch (green){
	case GL_FALSE:	g = "GL_FALSE";					break;
	case GL_TRUE:	g = "GL_TRUE";					break;
	default:		g = Str_VarArgs("0x%08X", green);	break;
	}

	switch (blue){
	case GL_FALSE:	b = "GL_FALSE";					break;
	case GL_TRUE:	b = "GL_TRUE";					break;
	default:		b = Str_VarArgs("0x%08X", blue);	break;
	}

	switch (alpha){
	case GL_FALSE:	a = "GL_FALSE";					break;
	case GL_TRUE:	a = "GL_TRUE";					break;
	default:		a = Str_VarArgs("0x%08X", alpha);	break;
	}

	fprintf(qglState.logFile, "glColorMask( %s, %s, %s, %s )\n", r, g, b, a);
	dllColorMask(red, green, blue, alpha);
}

static GLvoid APIENTRY logColorMaski (GLuint index, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){

	const char	*r, *g, *b, *a;

	switch (red){
	case GL_FALSE:	r = "GL_FALSE";					break;
	case GL_TRUE:	r = "GL_TRUE";					break;
	default:		r = Str_VarArgs("0x%08X", red);	break;
	}

	switch (green){
	case GL_FALSE:	g = "GL_FALSE";					break;
	case GL_TRUE:	g = "GL_TRUE";					break;
	default:		g = Str_VarArgs("0x%08X", green);	break;
	}

	switch (blue){
	case GL_FALSE:	b = "GL_FALSE";					break;
	case GL_TRUE:	b = "GL_TRUE";					break;
	default:		b = Str_VarArgs("0x%08X", blue);	break;
	}

	switch (alpha){
	case GL_FALSE:	a = "GL_FALSE";					break;
	case GL_TRUE:	a = "GL_TRUE";					break;
	default:		a = Str_VarArgs("0x%08X", alpha);	break;
	}

	fprintf(qglState.logFile, "glColorMaski( %u, %s, %s, %s, %s )\n", index, r, g, b, a);
	dllColorMaski(index, red, green, blue, alpha);
}

static GLvoid APIENTRY logColorMaterial (GLenum face, GLenum mode){

	fprintf(qglState.logFile, "glColorMaterial( 0x%08X, 0x%08X )\n", face, mode);
	dllColorMaterial(face, mode);
}

static GLvoid APIENTRY logColorP3ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glColorP3ui( 0x%08X, %u )\n", type, v);
	dllColorP3ui(type, v);
}

static GLvoid APIENTRY logColorP3uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glColorP3uiv( 0x%08X, %p )\n", type, v);
	dllColorP3uiv(type, v);
}

static GLvoid APIENTRY logColorP4ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glColorP4ui( 0x%08X, %u )\n", type, v);
	dllColorP4ui(type, v);
}

static GLvoid APIENTRY logColorP4uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glColorP4uiv( 0x%08X, %p )\n", type, v);
	dllColorP4uiv(type, v);
}

static GLvoid APIENTRY logColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){

	const char	*t;

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";					break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";			break;
	case GL_SHORT:			t = "GL_SHORT";					break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";		break;
	case GL_INT:			t = "GL_INT";					break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";			break;
	case GL_FLOAT:			t = "GL_FLOAT";					break;
	case GL_HALF_FLOAT:		t = "GL_HALF_FLOAT";			break;
	case GL_DOUBLE:			t = "GL_DOUBLE";				break;
	default:				t = Str_VarArgs("0x%08X", type);	break;
	}

	fprintf(qglState.logFile, "glColorPointer( %i, %s, %i, %p )\n", size, t, stride, pointer);
	dllColorPointer(size, type, stride, pointer);
}

static GLvoid APIENTRY logCompileShader (GLuint shader){

	fprintf(qglState.logFile, "glCompileShader( %u )\n", shader);
	dllCompileShader(shader);
}

static GLvoid APIENTRY logCompressedTexImage1D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *pixels){

	const char	*t;

	switch (target){
	case GL_TEXTURE_1D:	t = "GL_TEXTURE_1D";				break;
	default:			t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glCompressedTexImage1D( %s, %i, 0x%08X, %i, %i, %i, %p )\n", t, level, internalformat, width, border, imageSize, pixels);
	dllCompressedTexImage1D(target, level, internalformat, width, border, imageSize, pixels);
}

static GLvoid APIENTRY logCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *pixels){

	const char	*t, *i;

	switch (target){
	case GL_TEXTURE_1D_ARRAY:				t = "GL_TEXTURE_1D_ARRAY";					break;
	case GL_TEXTURE_2D:						t = "GL_TEXTURE_2D";						break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";		break;
	default:								t = Str_VarArgs("0x%08X", target);			break;
	}

	switch (internalformat){
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:	i = "GL_COMPRESSED_RGB_S3TC_DXT1";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT1";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT3";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT5";			break;
	default:								i = Str_VarArgs("0x%08X", internalformat);	break;
	}

	fprintf(qglState.logFile, "glCompressedTexImage2D( %s, %i, %s, %i, %i, %i, %i, %p )\n", t, level, i, width, height, border, imageSize, pixels);
	dllCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, pixels);
}

static GLvoid APIENTRY logCompressedTexImage3D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *pixels){

	const char	*t, *i;

	switch (target){
	case GL_TEXTURE_2D_ARRAY:				t = "GL_TEXTURE_2D_ARRAY";					break;
	case GL_TEXTURE_3D:						t = "GL_TEXTURE_3D";						break;
	default:								t = Str_VarArgs("0x%08X", target);			break;
	}

	switch (internalformat){
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:	i = "GL_COMPRESSED_RGB_S3TC_DXT1";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT1";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT3";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT5";			break;
	default:								i = Str_VarArgs("0x%08X", internalformat);	break;
	}

	fprintf(qglState.logFile, "glCompressedTexImage3D( %s, %i, %s, %i, %i, %i, %i, %i, %p )\n", t, level, i, width, height, depth, border, imageSize, pixels);
	dllCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, pixels);
}

static GLvoid APIENTRY logCompressedTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *pixels){

	const char	*t;

	switch (target){
	case GL_TEXTURE_1D:	t = "GL_TEXTURE_1D";				break;
	default:			t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glCompressedTexSubImage1D( %s, %i, %i, %i, 0x%08X, %i, %p )\n", t, level, xoffset, width, format, imageSize, pixels);
	dllCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, pixels);
}

static GLvoid APIENTRY logCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *pixels){

	const char	*t, *f;

	switch (target){
	case GL_TEXTURE_1D_ARRAY:				t = "GL_TEXTURE_1D_ARRAY";				break;
	case GL_TEXTURE_2D:						t = "GL_TEXTURE_2D";					break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";	break;
	default:								t = Str_VarArgs("0x%08X", target);		break;
	}

	switch (format){
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:	f = "GL_COMPRESSED_RGB_S3TC_DXT1";		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	f = "GL_COMPRESSED_RGBA_S3TC_DXT1";		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	f = "GL_COMPRESSED_RGBA_S3TC_DXT3";		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	f = "GL_COMPRESSED_RGBA_S3TC_DXT5";		break;
	default:								f = Str_VarArgs("0x%08X", format);		break;
	}

	fprintf(qglState.logFile, "glCompressedTexSubImage2D( %s, %i, %i, %i, %i, %i, %s, %i, %p )\n", t, level, xoffset, yoffset, width, height, f, imageSize, pixels);
	dllCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, pixels);
}

static GLvoid APIENTRY logCompressedTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *pixels){

	const char	*t, *f;

	switch (target){
	case GL_TEXTURE_2D_ARRAY:				t = "GL_TEXTURE_2D_ARRAY";				break;
	case GL_TEXTURE_3D:						t = "GL_TEXTURE_3D";					break;
	default:								t = Str_VarArgs("0x%08X", target);		break;
	}

	switch (format){
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:	f = "GL_COMPRESSED_RGB_S3TC_DXT1";		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	f = "GL_COMPRESSED_RGBA_S3TC_DXT1";		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	f = "GL_COMPRESSED_RGBA_S3TC_DXT3";		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	f = "GL_COMPRESSED_RGBA_S3TC_DXT5";		break;
	default:								f = Str_VarArgs("0x%08X", format);		break;
	}

	fprintf(qglState.logFile, "glCompressedTexSubImage3D( %s, %i, %i, %i, %i, %i, %i, %i, %s, %i, %p )\n", t, level, xoffset, yoffset, zoffset, width, height, depth, f, imageSize, pixels);
	dllCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, pixels);
}

static GLvoid APIENTRY logCopyBufferSubData (GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size){

	const char	*rt, *wt;

	switch (readTarget){
	case GL_ARRAY_BUFFER:				rt = "GL_ARRAY_BUFFER";					break;
	case GL_ELEMENT_ARRAY_BUFFER:		rt = "GL_ELEMENT_ARRAY_BUFFER";			break;
	case GL_PIXEL_PACK_BUFFER:			rt = "GL_PIXEL_PACK_BUFFER";			break;
	case GL_PIXEL_UNPACK_BUFFER:		rt = "GL_PIXEL_UNPACK_BUFFER";			break;
	case GL_COPY_READ_BUFFER:			rt = "GL_COPY_READ_BUFFER";				break;
	case GL_COPY_WRITE_BUFFER:			rt = "GL_COPY_WRITE_BUFFER";			break;
	case GL_TEXTURE_BUFFER:				rt = "GL_TEXTURE_BUFFER";				break;
	case GL_UNIFORM_BUFFER:				rt = "GL_UNIFORM_BUFFER";				break;
	default:							rt = Str_VarArgs("0x%08X", readTarget);	break;
	}

	switch (writeTarget){
	case GL_ARRAY_BUFFER:				wt = "GL_ARRAY_BUFFER";					break;
	case GL_ELEMENT_ARRAY_BUFFER:		wt = "GL_ELEMENT_ARRAY_BUFFER";			break;
	case GL_PIXEL_PACK_BUFFER:			wt = "GL_PIXEL_PACK_BUFFER";			break;
	case GL_PIXEL_UNPACK_BUFFER:		wt = "GL_PIXEL_UNPACK_BUFFER";			break;
	case GL_COPY_READ_BUFFER:			wt = "GL_COPY_READ_BUFFER";				break;
	case GL_COPY_WRITE_BUFFER:			wt = "GL_COPY_WRITE_BUFFER";			break;
	case GL_TEXTURE_BUFFER:				wt = "GL_TEXTURE_BUFFER";				break;
	case GL_UNIFORM_BUFFER:				wt = "GL_UNIFORM_BUFFER";				break;
	default:							wt = Str_VarArgs("0x%08X", writeTarget);	break;
	}

	fprintf(qglState.logFile, "glCopyBufferSubData( %s, %s, %i, %i, %i )\n", rt, wt, readOffset, writeOffset, size);
	dllCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
}

static GLvoid APIENTRY logCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type){

	const char	*t;

	switch (type){
	case GL_COLOR:			t = "GL_COLOR";					break;
	case GL_DEPTH:			t = "GL_DEPTH";					break;
	case GL_STENCIL:		t = "GL_STENCIL";				break;
	case GL_DEPTH_STENCIL:	t = "GL_DEPTH_STENCIL";			break;
	default:				t = Str_VarArgs("0x%08X", type);	break;
	}

	fprintf(qglState.logFile, "glCopyPixels( %i, %i, %i, %i, %s )\n", x, y, width, height, t);
	dllCopyPixels(x, y, width, height, type);
}

static GLvoid APIENTRY logCopyTexImage1D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border){

	const char	*t, *i;

	switch (target){
	case GL_TEXTURE_1D:						t = "GL_TEXTURE_1D";						break;
	default:								t = Str_VarArgs("0x%08X", target);			break;
	}

	switch (internalformat){
	case GL_ALPHA8:							i = "GL_ALPHA8";							break;
	case GL_INTENSITY8:						i = "GL_INTENSITY8";						break;
	case GL_LUMINANCE8:						i = "GL_LUMINANCE8";						break;
	case GL_LUMINANCE8_ALPHA8:				i = "GL_LUMINANCE8_ALPHA8";					break;
	case GL_R8:								i = "GL_R8";								break;
	case GL_R16F:							i = "GL_R16F";								break;
	case GL_R32F:							i = "GL_R32F";								break;
	case GL_RG8:							i = "GL_RG8";								break;
	case GL_RG16F:							i = "GL_RG16F";								break;
	case GL_RG32F:							i = "GL_RG32F";								break;
	case GL_RGB8:							i = "GL_RGB8";								break;
	case GL_RGB16F:							i = "GL_RGB16F";							break;
	case GL_RGB32F:							i = "GL_RGB32F";							break;
	case GL_RGBA8:							i = "GL_RGBA8";								break;
	case GL_RGBA16F:						i = "GL_RGBA16F";							break;
	case GL_RGBA32F:						i = "GL_RGBA32F";							break;
	case GL_DEPTH_COMPONENT16:				i = "GL_DEPTH_COMPONENT16";					break;
	case GL_DEPTH_COMPONENT24:				i = "GL_DEPTH_COMPONENT24";					break;
	case GL_DEPTH_COMPONENT32:				i = "GL_DEPTH_COMPONENT32";					break;
	case GL_DEPTH_COMPONENT32F:				i = "GL_DEPTH_COMPONENT32F";				break;
	case GL_DEPTH24_STENCIL8:				i = "GL_DEPTH24_STENCIL8";					break;
	case GL_DEPTH32F_STENCIL8:				i = "GL_DEPTH32F_STENCIL8";					break;
	case GL_COMPRESSED_ALPHA:				i = "GL_COMPRESSED_ALPHA";					break;
	case GL_COMPRESSED_INTENSITY:			i = "GL_COMPRESSED_INTENSITY";				break;
	case GL_COMPRESSED_LUMINANCE:			i = "GL_COMPRESSED_LUMINANCE";				break;
	case GL_COMPRESSED_LUMINANCE_ALPHA:		i = "GL_COMPRESSED_LUMINANCE_ALPHA";		break;
	case GL_COMPRESSED_RED:					i = "GL_COMPRESSED_RED";					break;
	case GL_COMPRESSED_RG:					i = "GL_COMPRESSED_RG";						break;
	case GL_COMPRESSED_RGB:					i = "GL_COMPRESSED_RGB";					break;
	case GL_COMPRESSED_RGBA:				i = "GL_COMPRESSED_RGBA";					break;
	default:								i = Str_VarArgs("0x%08X", internalformat);	break;
	}

	fprintf(qglState.logFile, "glCopyTexImage1D( %s, %i, %s, %i, %i, %i, %i )\n", t, level, i, x, y, width, border);
	dllCopyTexImage1D(target, level, internalformat, x, y, width, border);
}

static GLvoid APIENTRY logCopyTexImage2D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border){

	const char	*t, *i;

	switch (target){
	case GL_TEXTURE_1D_ARRAY:				t = "GL_TEXTURE_1D_ARRAY";					break;
	case GL_TEXTURE_2D:						t = "GL_TEXTURE_2D";						break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";		break;
	case GL_TEXTURE_RECTANGLE:				t = "GL_TEXTURE_RECTANGLE";					break;
	default:								t = Str_VarArgs("0x%08X", target);			break;
	}

	switch (internalformat){
	case GL_ALPHA8:							i = "GL_ALPHA8";							break;
	case GL_INTENSITY8:						i = "GL_INTENSITY8";						break;
	case GL_LUMINANCE8:						i = "GL_LUMINANCE8";						break;
	case GL_LUMINANCE8_ALPHA8:				i = "GL_LUMINANCE8_ALPHA8";					break;
	case GL_R8:								i = "GL_R8";								break;
	case GL_R16F:							i = "GL_R16F";								break;
	case GL_R32F:							i = "GL_R32F";								break;
	case GL_RG8:							i = "GL_RG8";								break;
	case GL_RG16F:							i = "GL_RG16F";								break;
	case GL_RG32F:							i = "GL_RG32F";								break;
	case GL_RGB8:							i = "GL_RGB8";								break;
	case GL_RGB16F:							i = "GL_RGB16F";							break;
	case GL_RGB32F:							i = "GL_RGB32F";							break;
	case GL_RGBA8:							i = "GL_RGBA8";								break;
	case GL_RGBA16F:						i = "GL_RGBA16F";							break;
	case GL_RGBA32F:						i = "GL_RGBA32F";							break;
	case GL_DEPTH_COMPONENT16:				i = "GL_DEPTH_COMPONENT16";					break;
	case GL_DEPTH_COMPONENT24:				i = "GL_DEPTH_COMPONENT24";					break;
	case GL_DEPTH_COMPONENT32:				i = "GL_DEPTH_COMPONENT32";					break;
	case GL_DEPTH_COMPONENT32F:				i = "GL_DEPTH_COMPONENT32F";				break;
	case GL_DEPTH24_STENCIL8:				i = "GL_DEPTH24_STENCIL8";					break;
	case GL_DEPTH32F_STENCIL8:				i = "GL_DEPTH32F_STENCIL8";					break;
	case GL_COMPRESSED_ALPHA:				i = "GL_COMPRESSED_ALPHA";					break;
	case GL_COMPRESSED_INTENSITY:			i = "GL_COMPRESSED_INTENSITY";				break;
	case GL_COMPRESSED_LUMINANCE:			i = "GL_COMPRESSED_LUMINANCE";				break;
	case GL_COMPRESSED_LUMINANCE_ALPHA:		i = "GL_COMPRESSED_LUMINANCE_ALPHA";		break;
	case GL_COMPRESSED_RED:					i = "GL_COMPRESSED_RED";					break;
	case GL_COMPRESSED_RG:					i = "GL_COMPRESSED_RG";						break;
	case GL_COMPRESSED_RGB:					i = "GL_COMPRESSED_RGB";					break;
	case GL_COMPRESSED_RGBA:				i = "GL_COMPRESSED_RGBA";					break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:	i = "GL_COMPRESSED_RGB_S3TC_DXT1";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT1";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT3";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT5";			break;
	default:								i = Str_VarArgs("0x%08X", internalformat);	break;
	}

	fprintf(qglState.logFile, "glCopyTexImage2D( %s, %i, %s, %i, %i, %i, %i, %i )\n", t, level, i, x, y, width, height, border);
	dllCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

static GLvoid APIENTRY logCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width){

	const char	*t;

	switch (target){
	case GL_TEXTURE_1D:	t = "GL_TEXTURE_1D";				break;
	default:			t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glCopyTexSubImage1D( %s, %i, %i, %i, %i, %i)\n", t, level, xoffset, x, y, width);
	dllCopyTexSubImage1D(target, level, xoffset, x, y, width);
}

static GLvoid APIENTRY logCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height){

	const char	*t;

	switch (target){
	case GL_TEXTURE_1D_ARRAY:				t = "GL_TEXTURE_1D_ARRAY";				break;
	case GL_TEXTURE_2D:						t = "GL_TEXTURE_2D";					break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";	break;
	case GL_TEXTURE_RECTANGLE:				t = "GL_TEXTURE_RECTANGLE";				break;
	default:								t = Str_VarArgs("0x%08X", target);		break;
	}

	fprintf(qglState.logFile, "glCopyTexSubImage2D( %s, %i, %i, %i, %i, %i, %i, %i )\n", t, level, xoffset, yoffset, x, y, width, height);
	dllCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

static GLvoid APIENTRY logCopyTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height){

	const char	*t;

	switch (target){
	case GL_TEXTURE_2D_ARRAY:	t = "GL_TEXTURE_2D_ARRAY";			break;
	case GL_TEXTURE_3D:			t = "GL_TEXTURE_3D";				break;
	default:					t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glCopyTexSubImage3D( %s, %i, %i, %i, %i, %i, %i, %i, %i )\n", t, level, xoffset, yoffset, zoffset, x, y, width, height);
	dllCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

static GLuint APIENTRY logCreateProgram (GLvoid){

	fprintf(qglState.logFile, "glCreateProgram()\n");
	return dllCreateProgram();
}

static GLuint APIENTRY logCreateShader (GLenum type){

	const char	*t;

	switch (type){
	case GL_VERTEX_SHADER:		t = "GL_VERTEX_SHADER";			break;
	case GL_GEOMETRY_SHADER:	t = "GL_GEOMETRY_SHADER";		break;
	case GL_FRAGMENT_SHADER:	t = "GL_FRAGMENT_SHADER";		break;
	default:					t = Str_VarArgs("0x%08X", type);	break;
	}

	fprintf(qglState.logFile, "glCreateShader( %s )\n", t);
	return dllCreateShader(type);
}

static GLvoid APIENTRY logCullFace (GLenum mode){

	const char	*m;

	switch (mode){
	case GL_FRONT:			m = "GL_FRONT";					break;
	case GL_BACK:			m = "GL_BACK";					break;
	case GL_FRONT_AND_BACK:	m = "GL_FRONT_AND_BACK";		break;
	default:				m = Str_VarArgs("0x%08X", mode);	break;
	}

	fprintf(qglState.logFile, "glCullFace( %s )\n", m);
	dllCullFace(mode);
}

static GLvoid APIENTRY logDeleteBuffers (GLsizei n, const GLuint *buffers){

	fprintf(qglState.logFile, "glDeleteBuffers( %i, %p )\n", n, buffers);
	dllDeleteBuffers(n, buffers);
}

static GLvoid APIENTRY logDeleteFramebuffers (GLsizei n, const GLuint *framebuffers){

	fprintf(qglState.logFile, "glDeleteFramebuffers( %i, %p )\n", n, framebuffers);
	dllDeleteFramebuffers(n, framebuffers);
}

static GLvoid APIENTRY logDeleteLists (GLuint list, GLsizei range){

	fprintf(qglState.logFile, "glDeleteLists( %u, %i )\n", list, range);
	dllDeleteLists(list, range);
}

static GLvoid APIENTRY logDeleteProgram (GLuint program){

	fprintf(qglState.logFile, "glDeleteProgram( %u )\n", program);
	dllDeleteProgram(program);
}

static GLvoid APIENTRY logDeleteQueries (GLsizei n, const GLuint *ids){

	fprintf(qglState.logFile, "glDeleteQueries( %i, %p )\n", n, ids);
	dllDeleteQueries(n, ids);
}

static GLvoid APIENTRY logDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers){

	fprintf(qglState.logFile, "glDeleteRenderbuffers( %i, %p )\n", n, renderbuffers);
	dllDeleteRenderbuffers(n, renderbuffers);
}

static GLvoid APIENTRY logDeleteSamplers (GLsizei n, const GLuint *samplers){

	fprintf(qglState.logFile, "glDeleteSamplers( %i, %p )\n", n, samplers);
	dllDeleteSamplers(n, samplers);
}

static GLvoid APIENTRY logDeleteShader (GLuint shader){

	fprintf(qglState.logFile, "glDeleteShader( %u )\n", shader);
	dllDeleteShader(shader);
}

static GLvoid APIENTRY logDeleteSync (GLsync sync){

	fprintf(qglState.logFile, "glDeleteSync( %p )\n", sync);
	dllDeleteSync(sync);
}

static GLvoid APIENTRY logDeleteTextures (GLsizei n, const GLuint *textures){

	fprintf(qglState.logFile, "glDeleteTextures( %i, %p )\n", n, textures);
	dllDeleteTextures(n, textures);
}

static GLvoid APIENTRY logDeleteVertexArrays (GLsizei n, const GLuint *vertexArrays){

	fprintf(qglState.logFile, "glDeleteVertexArrays( %i, %p )\n", n, vertexArrays);
	dllDeleteVertexArrays(n, vertexArrays);
}

static GLvoid APIENTRY logDepthBoundsEXT (GLclampd zmin, GLclampd zmax){

	fprintf(qglState.logFile, "glDepthBounds( %g, %g )\n", zmin, zmax);
	dllDepthBoundsEXT(zmin, zmax);
}

static GLvoid APIENTRY logDepthFunc (GLenum func){

	const char	*f;

	switch (func){
	case GL_NEVER:		f = "GL_NEVER";					break;
	case GL_LESS:		f = "GL_LESS";					break;
	case GL_LEQUAL:		f = "GL_LEQUAL";				break;
	case GL_EQUAL:		f = "GL_EQUAL";					break;
	case GL_NOTEQUAL:	f = "GL_NOTEQUAL";				break;
	case GL_GEQUAL:		f = "GL_GEQUAL";				break;
	case GL_GREATER:	f = "GL_GREATER";				break;
	case GL_ALWAYS:		f = "GL_ALWAYS";				break;
	default:			f = Str_VarArgs("0x%08X", func);	break;
	}

	fprintf(qglState.logFile, "glDepthFunc( %s )\n", f);
	dllDepthFunc(func);
}

static GLvoid APIENTRY logDepthMask (GLboolean flag){

	const char	*f;

	switch (flag){
	case GL_FALSE:	f = "GL_FALSE";					break;
	case GL_TRUE:	f = "GL_TRUE";					break;
	default:		f = Str_VarArgs("0x%08X", flag);	break;
	}

	fprintf(qglState.logFile, "glDepthMask( %s )\n", f);
	dllDepthMask(flag);
}

static GLvoid APIENTRY logDepthRange (GLclampd zNear, GLclampd zFar){

	fprintf(qglState.logFile, "glDepthRange( %g, %g )\n", zNear, zFar);
	dllDepthRange(zNear, zFar);
}

static GLvoid APIENTRY logDetachShader (GLuint program, GLuint shader){

	fprintf(qglState.logFile, "glDetachShader( %u, %u )\n", program, shader);
	dllDetachShader(program, shader);
}

static GLvoid APIENTRY logDisable (GLenum cap){

	const char	*c;

	switch (cap){
	case GL_ALPHA_TEST:					c = "GL_ALPHA_TEST";				break;
	case GL_BLEND:						c = "GL_BLEND";						break;
	case GL_COLOR_SUM:					c = "GL_COLOR_SUM";					break;
	case GL_CULL_FACE:					c = "GL_CULL_FACE";					break;
	case GL_DEPTH_CLAMP:				c = "GL_DEPTH_CLAMP";				break;
	case GL_DEPTH_TEST:					c = "GL_DEPTH_TEST";				break;
	case GL_FOG:						c = "GL_FOG";						break;
	case GL_MULTISAMPLE:				c = "GL_MULTISAMPLE";				break;
	case GL_POLYGON_OFFSET_FILL:		c = "GL_POLYGON_OFFSET_FILL";		break;
	case GL_POLYGON_OFFSET_LINE:		c = "GL_POLYGON_OFFSET_LINE";		break;
	case GL_POLYGON_OFFSET_POINT:		c = "GL_POLYGON_OFFSET_POINT";		break;
	case GL_SAMPLE_ALPHA_TO_COVERAGE:	c = "GL_SAMPLE_ALPHA_TO_COVERAGE";	break;
	case GL_SAMPLE_ALPHA_TO_ONE:		c = "GL_SAMPLE_ALPHA_TO_ONE";		break;
	case GL_SAMPLE_COVERAGE:			c = "GL_SAMPLE_COVERAGE";			break;
	case GL_SAMPLE_MASK:				c = "GL_SAMPLE_MASK";				break;
	case GL_STENCIL_TEST:				c = "GL_STENCIL_TEST";				break;
	case GL_SCISSOR_TEST:				c = "GL_SCISSOR_TEST";				break;
	case GL_TEXTURE_1D:					c = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_2D:					c = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_3D:					c = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP:			c = "GL_TEXTURE_CUBE_MAP";			break;
	case GL_TEXTURE_CUBE_MAP_SEAMLESS:	c = "GL_TEXTURE_CUBE_MAP_SEAMLESS";	break;
	case GL_TEXTURE_GEN_Q:				c = "GL_TEXTURE_GEN_Q";				break;
	case GL_TEXTURE_GEN_R:				c = "GL_TEXTURE_GEN_R";				break;
	case GL_TEXTURE_GEN_S:				c = "GL_TEXTURE_GEN_S";				break;
	case GL_TEXTURE_GEN_T:				c = "GL_TEXTURE_GEN_T";				break;
	case GL_TEXTURE_RECTANGLE:			c = "GL_TEXTURE_RECTANGLE";			break;
	default:							c = Str_VarArgs("0x%08X", cap);		break;
	}

	if (cap >= GL_CLIP_PLANE0 && cap <= GL_CLIP_PLANE5)
		c = Str_VarArgs("GL_CLIP_PLANE%i", cap - GL_CLIP_PLANE0);

	fprintf(qglState.logFile, "glDisable( %s )\n", c);
	dllDisable(cap);
}

static GLvoid APIENTRY logDisableClientState (GLenum array){

	const char	*a;

	switch (array){
	case GL_COLOR_ARRAY:			a = "GL_COLOR_ARRAY";			break;
	case GL_FOG_COORD_ARRAY:		a = "GL_FOG_COORD_ARRAY";		break;
	case GL_NORMAL_ARRAY:			a = "GL_NORMAL_ARRAY";			break;
	case GL_SECONDARY_COLOR_ARRAY:	a = "GL_SECONDARY_COLOR_ARRAY";	break;
	case GL_TEXTURE_COORD_ARRAY:	a = "GL_TEXTURE_COORD_ARRAY";	break;
	case GL_VERTEX_ARRAY:			a = "GL_VERTEX_ARRAY";			break;
	default:						a = Str_VarArgs("0x%08X", array);	break;
	}

	fprintf(qglState.logFile, "glDisableClientState( %s )\n", a);
	dllDisableClientState(array);
}

static GLvoid APIENTRY logDisableVertexAttribArray (GLuint index){

	fprintf(qglState.logFile, "glDisableVertexAttribArray( %u )\n", index);
	dllDisableVertexAttribArray(index);
}

static GLvoid APIENTRY logDisablei (GLenum target, GLuint index){

	const char	*t;

	switch (target){
	case GL_BLEND:	t = "GL_BLEND";						break;
	default:		t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glDisablei( %s, %u )\n", t, index);
	dllDisablei(target, index);
}

static GLvoid APIENTRY logDrawArrays (GLenum mode, GLint first, GLsizei count){

	const char	*m;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	fprintf(qglState.logFile, "glDrawArrays( %s, %i, %i )\n", m, first, count);
	dllDrawArrays(mode, first, count);
}

static GLvoid APIENTRY logDrawArraysInstanced (GLenum mode, GLint first, GLsizei count, GLsizei primcount){

	const char	*m;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	fprintf(qglState.logFile, "glDrawArraysInstanced( %s, %i, %i, %i )\n", m, first, count, primcount);
	dllDrawArraysInstanced(mode, first, count, primcount);
}

static GLvoid APIENTRY logDrawBuffer (GLenum buffer){

	const char	*b;

	switch (buffer){
	case GL_NONE:			b = "GL_NONE";						break;
	case GL_FRONT_LEFT:		b = "GL_FRONT_LEFT";				break;
	case GL_FRONT_RIGHT:	b = "GL_FRONT_RIGHT";				break;
	case GL_BACK_LEFT:		b = "GL_BACK_LEFT";					break;
	case GL_BACK_RIGHT:		b = "GL_BACK_RIGHT";				break;
	case GL_FRONT:			b = "GL_FRONT";						break;
	case GL_BACK:			b = "GL_BACK";						break;
	case GL_LEFT:			b = "GL_LEFT";						break;
	case GL_RIGHT:			b = "GL_RIGHT";						break;
	case GL_FRONT_AND_BACK:	b = "GL_FRONT_AND_BACK";			break;
	default:				b = Str_VarArgs("0x%08X", buffer);	break;
	}

	if (buffer >= GL_COLOR_ATTACHMENT0 && buffer <= GL_COLOR_ATTACHMENT15)
		b = Str_VarArgs("GL_COLOR_ATTACHMENT%i", buffer - GL_COLOR_ATTACHMENT0);

	fprintf(qglState.logFile, "glDrawBuffer( %s )\n", b);
	dllDrawBuffer(buffer);
}

static GLvoid APIENTRY logDrawBuffers (GLsizei n, const GLenum *buffers){

	fprintf(qglState.logFile, "glDrawBuffers( %i, %p )\n", n, buffers);
	dllDrawBuffers(n, buffers);
}

static GLvoid APIENTRY logDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices){

	const char	*m, *t;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	switch (type){
	case GL_UNSIGNED_BYTE:				t = "GL_UNSIGNED_BYTE";				break;
	case GL_UNSIGNED_SHORT:				t = "GL_UNSIGNED_SHORT";			break;
	case GL_UNSIGNED_INT:				t = "GL_UNSIGNED_INT";				break;
	default:							t = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glDrawElements( %s, %i, %s, %p )\n", m, count, t, indices);
	dllDrawElements(mode, count, type, indices);
}

static GLvoid APIENTRY logDrawElementsBaseVertex (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLint basevertex){

	const char	*m, *t;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	switch (type){
	case GL_UNSIGNED_BYTE:				t = "GL_UNSIGNED_BYTE";				break;
	case GL_UNSIGNED_SHORT:				t = "GL_UNSIGNED_SHORT";			break;
	case GL_UNSIGNED_INT:				t = "GL_UNSIGNED_INT";				break;
	default:							t = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glDrawElementsBaseVertex( %s, %i, %s, %p, %i )\n", m, count, t, indices, basevertex);
	dllDrawElementsBaseVertex(mode, count, type, indices, basevertex);
}

static GLvoid APIENTRY logDrawElementsInstanced (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount){

	const char	*m, *t;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	switch (type){
	case GL_UNSIGNED_BYTE:				t = "GL_UNSIGNED_BYTE";				break;
	case GL_UNSIGNED_SHORT:				t = "GL_UNSIGNED_SHORT";			break;
	case GL_UNSIGNED_INT:				t = "GL_UNSIGNED_INT";				break;
	default:							t = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glDrawElementsInstanced( %s, %i, %s, %p, %i )\n", m, count, t, indices, primcount);
	dllDrawElementsInstanced(mode, count, type, indices, primcount);
}

static GLvoid APIENTRY logDrawElementsInstancedBaseVertex (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount, GLint basevertex){

	const char	*m, *t;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	switch (type){
	case GL_UNSIGNED_BYTE:				t = "GL_UNSIGNED_BYTE";				break;
	case GL_UNSIGNED_SHORT:				t = "GL_UNSIGNED_SHORT";			break;
	case GL_UNSIGNED_INT:				t = "GL_UNSIGNED_INT";				break;
	default:							t = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glDrawElementsInstancedBaseVertex( %s, %i, %s, %p, %i, %i )\n", m, count, t, indices, primcount, basevertex);
	dllDrawElementsInstancedBaseVertex(mode, count, type, indices, primcount, basevertex);
}

static GLvoid APIENTRY logDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels){

	const char	*f, *t;

	switch (format){
	case GL_RGB:				f = "GL_RGB";						break;
	case GL_RGBA:				f = "GL_RGBA";						break;
	case GL_BGR:				f = "GL_BGR";						break;
	case GL_BGRA:				f = "GL_BGRA";						break;
	case GL_DEPTH_COMPONENT:	f = "GL_DEPTH_COMPONENT";			break;
	case GL_DEPTH_STENCIL:		f = "GL_DEPTH_STENCIL";				break;
	default:					f = Str_VarArgs("0x%08X", format);	break;
	}

	switch (type){
	case GL_BYTE:				t = "GL_BYTE";						break;
	case GL_UNSIGNED_BYTE:		t = "GL_UNSIGNED_BYTE";				break;
	case GL_SHORT:				t = "GL_SHORT";						break;
	case GL_UNSIGNED_SHORT:		t = "GL_UNSIGNED_SHORT";			break;
	case GL_INT:				t = "GL_INT";						break;
	case GL_UNSIGNED_INT:		t = "GL_UNSIGNED_INT";				break;
	case GL_UNSIGNED_INT_24_8:	t = "GL_UNSIGNED_INT_24_8";			break;
	case GL_FLOAT:				t = "GL_FLOAT";						break;
	case GL_HALF_FLOAT:			t = "GL_HALF_FLOAT";				break;
	case GL_BITMAP:				t = "GL_BITMAP";					break;
	default:					t = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glDrawPixels( %i, %i, %s, %s, %p )\n", width, height, f, t, pixels);
	dllDrawPixels(width, height, format, type, pixels);
}

static GLvoid APIENTRY logDrawRangeElements (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices){

	const char	*m, *t;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	switch (type){
	case GL_UNSIGNED_BYTE:				t = "GL_UNSIGNED_BYTE";				break;
	case GL_UNSIGNED_SHORT:				t = "GL_UNSIGNED_SHORT";			break;
	case GL_UNSIGNED_INT:				t = "GL_UNSIGNED_INT";				break;
	default:							t = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glDrawRangeElements( %s, %u, %u, %i, %s, %p )\n", m, start, end, count, t, indices);
	dllDrawRangeElements(mode, start, end, count, type, indices);
}

static GLvoid APIENTRY logDrawRangeElementsBaseVertex (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices, GLint basevertex){

	const char	*m, *t;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	switch (type){
	case GL_UNSIGNED_BYTE:				t = "GL_UNSIGNED_BYTE";				break;
	case GL_UNSIGNED_SHORT:				t = "GL_UNSIGNED_SHORT";			break;
	case GL_UNSIGNED_INT:				t = "GL_UNSIGNED_INT";				break;
	default:							t = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glDrawRangeElementsBaseVertex( %s, %u, %u, %i, %s, %p, %i )\n", m, start, end, count, t, indices, basevertex);
	dllDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);
}

static GLvoid APIENTRY logEdgeFlag (GLboolean flag){

	fprintf(qglState.logFile, "glEdgeFlag( %i )\n", flag);
	dllEdgeFlag(flag);
}

static GLvoid APIENTRY logEdgeFlagPointer (GLsizei stride, const GLvoid *pointer){

	fprintf(qglState.logFile, "glEdgeFlagPointer( %i, %p )\n", stride, pointer);
	dllEdgeFlagPointer(stride, pointer);
}

static GLvoid APIENTRY logEdgeFlagv (const GLboolean *flag){

	fprintf(qglState.logFile, "glEdgeFlagv( %p )\n", flag);
	dllEdgeFlagv(flag);
}

static GLvoid APIENTRY logEnable (GLenum cap){

	const char	*c;

	switch (cap){
	case GL_ALPHA_TEST:					c = "GL_ALPHA_TEST";				break;
	case GL_BLEND:						c = "GL_BLEND";						break;
	case GL_COLOR_SUM:					c = "GL_COLOR_SUM";					break;
	case GL_CULL_FACE:					c = "GL_CULL_FACE";					break;
	case GL_DEPTH_CLAMP:				c = "GL_DEPTH_CLAMP";				break;
	case GL_DEPTH_TEST:					c = "GL_DEPTH_TEST";				break;
	case GL_FOG:						c = "GL_FOG";						break;
	case GL_MULTISAMPLE:				c = "GL_MULTISAMPLE";				break;
	case GL_POLYGON_OFFSET_FILL:		c = "GL_POLYGON_OFFSET_FILL";		break;
	case GL_POLYGON_OFFSET_LINE:		c = "GL_POLYGON_OFFSET_LINE";		break;
	case GL_POLYGON_OFFSET_POINT:		c = "GL_POLYGON_OFFSET_POINT";		break;
	case GL_SAMPLE_ALPHA_TO_COVERAGE:	c = "GL_SAMPLE_ALPHA_TO_COVERAGE";	break;
	case GL_SAMPLE_ALPHA_TO_ONE:		c = "GL_SAMPLE_ALPHA_TO_ONE";		break;
	case GL_SAMPLE_COVERAGE:			c = "GL_SAMPLE_COVERAGE";			break;
	case GL_SAMPLE_MASK:				c = "GL_SAMPLE_MASK";				break;
	case GL_STENCIL_TEST:				c = "GL_STENCIL_TEST";				break;
	case GL_SCISSOR_TEST:				c = "GL_SCISSOR_TEST";				break;
	case GL_TEXTURE_1D:					c = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_2D:					c = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_3D:					c = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP:			c = "GL_TEXTURE_CUBE_MAP";			break;
	case GL_TEXTURE_CUBE_MAP_SEAMLESS:	c = "GL_TEXTURE_CUBE_MAP_SEAMLESS";	break;
	case GL_TEXTURE_GEN_Q:				c = "GL_TEXTURE_GEN_Q";				break;
	case GL_TEXTURE_GEN_R:				c = "GL_TEXTURE_GEN_R";				break;
	case GL_TEXTURE_GEN_S:				c = "GL_TEXTURE_GEN_S";				break;
	case GL_TEXTURE_GEN_T:				c = "GL_TEXTURE_GEN_T";				break;
	case GL_TEXTURE_RECTANGLE:			c = "GL_TEXTURE_RECTANGLE";			break;
	default:							c = Str_VarArgs("0x%08X", cap);		break;
	}

	if (cap >= GL_CLIP_PLANE0 && cap <= GL_CLIP_PLANE5)
		c = Str_VarArgs("GL_CLIP_PLANE%i", cap - GL_CLIP_PLANE0);

	fprintf(qglState.logFile, "glEnable( %s )\n", c);
	dllEnable(cap);
}

static GLvoid APIENTRY logEnableClientState (GLenum array){

	const char	*a;

	switch (array){
	case GL_COLOR_ARRAY:			a = "GL_COLOR_ARRAY";			break;
	case GL_FOG_COORD_ARRAY:		a = "GL_FOG_COORD_ARRAY";		break;
	case GL_NORMAL_ARRAY:			a = "GL_NORMAL_ARRAY";			break;
	case GL_SECONDARY_COLOR_ARRAY:	a = "GL_SECONDARY_COLOR_ARRAY";	break;
	case GL_TEXTURE_COORD_ARRAY:	a = "GL_TEXTURE_COORD_ARRAY";	break;
	case GL_VERTEX_ARRAY:			a = "GL_VERTEX_ARRAY";			break;
	default:						a = Str_VarArgs("0x%08X", array);	break;
	}

	fprintf(qglState.logFile, "glEnableClientState( %s )\n", a);
	dllEnableClientState(array);
}

static GLvoid APIENTRY logEnableVertexAttribArray (GLuint index){

	fprintf(qglState.logFile, "glEnableVertexAttribArray( %u )\n", index);
	dllEnableVertexAttribArray(index);
}

static GLvoid APIENTRY logEnablei (GLenum target, GLuint index){

	const char	*t;

	switch (target){
	case GL_BLEND:	t = "GL_BLEND";						break;
	default:		t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glEnablei( %s, %u )\n", t, index);
	dllEnablei(target, index);
}

static GLvoid APIENTRY logEnd (GLvoid){

	fprintf(qglState.logFile, "glEnd()\n");
	dllEnd();
}

static GLvoid APIENTRY logEndConditionalRender (GLvoid){

	fprintf(qglState.logFile, "glEndConditionalRender()\n");
	dllEndConditionalRender();
}

static GLvoid APIENTRY logEndList (GLvoid){

	fprintf(qglState.logFile, "glEndList()\n");
	dllEndList();
}

static GLvoid APIENTRY logEndQuery (GLenum target){

	const char	*t;

	switch (target){
	case GL_SAMPLES_PASSED:		t = "GL_SAMPLES_PASSED";			break;
	case GL_ANY_SAMPLES_PASSED:	t = "GL_ANY_SAMPLES_PASSED";		break;
	case GL_TIME_ELAPSED:		t = "GL_TIME_ELAPSED";				break;
	default:					t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glEndQuery( %s )\n", t);
	dllEndQuery(target);
}

static GLvoid APIENTRY logEndTransformFeedback (GLvoid){

	fprintf(qglState.logFile, "glEndTransformFeedback()\n");
	dllEndTransformFeedback();
}

static GLvoid APIENTRY logEvalCoord1d (GLdouble u){

	fprintf(qglState.logFile, "glEvalCoord1d( %g )\n", u);
	dllEvalCoord1d(u);
}

static GLvoid APIENTRY logEvalCoord1dv (const GLdouble *v){

	fprintf(qglState.logFile, "glEvalCoord1dv( %p )\n", v);
	dllEvalCoord1dv(v);
}

static GLvoid APIENTRY logEvalCoord1f (GLfloat u){

	fprintf(qglState.logFile, "glEvalCoord1f( %g )\n", u);
	dllEvalCoord1f(u);
}

static GLvoid APIENTRY logEvalCoord1fv (const GLfloat *v){

	fprintf(qglState.logFile, "glEvalCoord1fv( %p )\n", v);
	dllEvalCoord1fv(v);
}

static GLvoid APIENTRY logEvalCoord2d (GLdouble u, GLdouble v){

	fprintf(qglState.logFile, "glEvalCoord2d( %g, %g )\n", u, v);
	dllEvalCoord2d(u, v);
}

static GLvoid APIENTRY logEvalCoord2dv (const GLdouble *v){

	fprintf(qglState.logFile, "glEvalCoord2dv( %p )\n", v);
	dllEvalCoord2dv(v);
}

static GLvoid APIENTRY logEvalCoord2f (GLfloat u, GLfloat v){

	fprintf(qglState.logFile, "glEvalCoord2f( %g, %g )\n", u, v);
	dllEvalCoord2f(u, v);
}

static GLvoid APIENTRY logEvalCoord2fv (const GLfloat *v){

	fprintf(qglState.logFile, "glEvalCoord2fv( %p )\n", v);
	dllEvalCoord2fv(v);
}

static GLvoid APIENTRY logEvalMesh1 (GLenum mode, GLint i1, GLint i2){

	fprintf(qglState.logFile, "glEvalMesh1( 0x%08X, %i, %i )\n", mode, i1, i2);
	dllEvalMesh1(mode, i1, i2);
}

static GLvoid APIENTRY logEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2){

	fprintf(qglState.logFile, "glEvalMesh2( 0x%08X, %i, %i, %i, %i )\n", mode, i1, i2, j1, j2);
	dllEvalMesh2(mode, i1, i2, j1, j2);
}

static GLvoid APIENTRY logEvalPoint1 (GLint i){

	fprintf(qglState.logFile, "glEvalPoint1( %i )\n", i);
	dllEvalPoint1(i);
}

static GLvoid APIENTRY logEvalPoint2 (GLint i, GLint j){

	fprintf(qglState.logFile, "glEvalPoint2( %i, %i )\n", i, j);
	dllEvalPoint2(i, j);
}

static GLvoid APIENTRY logFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer){

	fprintf(qglState.logFile, "glFeedbackBuffer( %i, 0x%08X, %p )\n", size, type, buffer);
	dllFeedbackBuffer(size, type, buffer);
}

static GLsync APIENTRY logFenceSync (GLenum condition, GLbitfield flags){

	const char	*c;

	switch (condition){
	case GL_SYNC_GPU_COMMANDS_COMPLETE:	c = "GL_SYNC_GPU_COMMANDS_COMPLETE";	break;
	default:							c = Str_VarArgs("0x%08X", condition);		break;
	}

	fprintf(qglState.logFile, "glFenceSync( %s, %u )\n", c, flags);
	return dllFenceSync(condition, flags);
}

static GLvoid APIENTRY logFinish (GLvoid){

	fprintf(qglState.logFile, "glFinish()\n");
	dllFinish();
}

static GLvoid APIENTRY logFlush (GLvoid){

	fprintf(qglState.logFile, "glFlush()\n");
	dllFlush();
}

static GLvoid APIENTRY logFlushMappedBufferRange (GLenum target, GLintptr offset, GLsizeiptr length){

	const char	*t;

	switch (target){
	case GL_ARRAY_BUFFER:				t = "GL_ARRAY_BUFFER";				break;
	case GL_ELEMENT_ARRAY_BUFFER:		t = "GL_ELEMENT_ARRAY_BUFFER";		break;
	case GL_PIXEL_PACK_BUFFER:			t = "GL_PIXEL_PACK_BUFFER";			break;
	case GL_PIXEL_UNPACK_BUFFER:		t = "GL_PIXEL_UNPACK_BUFFER";		break;
	case GL_COPY_READ_BUFFER:			t = "GL_COPY_READ_BUFFER";			break;
	case GL_COPY_WRITE_BUFFER:			t = "GL_COPY_WRITE_BUFFER";			break;
	case GL_TEXTURE_BUFFER:				t = "GL_TEXTURE_BUFFER";			break;
	case GL_UNIFORM_BUFFER:				t = "GL_UNIFORM_BUFFER";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glFlushMappedBufferRange( %s, %i, %i )\n", t, offset, length);
	dllFlushMappedBufferRange(target, offset, length);
}

static GLvoid APIENTRY logFogCoordPointer (GLenum type, GLsizei stride, const GLvoid *pointer){

	const char	*t;

	switch (type){
	case GL_FLOAT:		t = "GL_FLOAT";					break;
	case GL_HALF_FLOAT:	t = "GL_HALF_FLOAT";			break;
	case GL_DOUBLE:		t = "GL_DOUBLE";				break;
	default:			t = Str_VarArgs("0x%08X", type);	break;
	}

	fprintf(qglState.logFile, "glFogCoordPointer( %s, %i, %p )\n", t, stride, pointer);
	dllFogCoordPointer(type, stride, pointer);
}

static GLvoid APIENTRY logFogCoordd (GLdouble coord){

	fprintf(qglState.logFile, "glFogCoordd( %g )\n", coord);
	dllFogCoordd(coord);
}

static GLvoid APIENTRY logFogCoorddv (const GLdouble *v){

	fprintf(qglState.logFile, "glFogCoorddv( %p )\n", v);
	dllFogCoorddv(v);
}

static GLvoid APIENTRY logFogCoordf (GLfloat coord){

	fprintf(qglState.logFile, "glFogCoordf( %g )\n", coord);
	dllFogCoordf(coord);
}

static GLvoid APIENTRY logFogCoordfv (const GLfloat *v){

	fprintf(qglState.logFile, "glFogCoordfv( %p )\n", v);
	dllFogCoordfv(v);
}

static GLvoid APIENTRY logFogf (GLenum pname, GLfloat param){

	const char	*n, *p;

	switch (pname){
	case GL_FOG_MODE:		n = "GL_FOG_MODE";				break;
	case GL_FOG_COORD_SRC:	n = "GL_FOG_COORD_SRC";			break;
	case GL_FOG_DENSITY:	n = "GL_FOG_DENSITY";			break;
	case GL_FOG_START:		n = "GL_FOG_START";				break;
	case GL_FOG_END:		n = "GL_FOG_END";				break;
	default:				n = Str_VarArgs("0x%08X", pname);	break;
	}

	switch ((int)param){
	case GL_EXP:			p = "GL_EXP";					break;
	case GL_EXP2:			p = "GL_EXP2";					break;
	case GL_LINEAR:			p = "GL_LINEAR";				break;
	case GL_FRAGMENT_DEPTH:	p = "GL_FRAGMENT_DEPTH";		break;
	case GL_FOG_COORD:		p = "GL_FOG_COORD";				break;
	default:				p = Str_VarArgs("0x%08X", param);	break;
	}

	if (pname == GL_FOG_DENSITY || pname == GL_FOG_START || pname == GL_FOG_END)
		p = Str_VarArgs("%g", param);

	fprintf(qglState.logFile, "glFogf( %s, %s )\n", n, p);
	dllFogf(pname, param);
}

static GLvoid APIENTRY logFogfv (GLenum pname, const GLfloat *params){

	const char	*n;

	switch (pname){
	case GL_FOG_MODE:		n = "GL_FOG_MODE";				break;
	case GL_FOG_COORD_SRC:	n = "GL_FOG_COORD_SRC";			break;
	case GL_FOG_DENSITY:	n = "GL_FOG_DENSITY";			break;
	case GL_FOG_START:		n = "GL_FOG_START";				break;
	case GL_FOG_END:		n = "GL_FOG_END";				break;
	default:				n = Str_VarArgs("0x%08X", pname);	break;
	}

	fprintf(qglState.logFile, "glFogfv( %s, %p )\n", n, params);
	dllFogfv(pname, params);
}

static GLvoid APIENTRY logFogi (GLenum pname, GLint param){

	const char	*n, *p;

	switch (pname){
	case GL_FOG_MODE:		n = "GL_FOG_MODE";				break;
	case GL_FOG_COORD_SRC:	n = "GL_FOG_COORD_SRC";			break;
	case GL_FOG_DENSITY:	n = "GL_FOG_DENSITY";			break;
	case GL_FOG_START:		n = "GL_FOG_START";				break;
	case GL_FOG_END:		n = "GL_FOG_END";				break;
	default:				n = Str_VarArgs("0x%08X", pname);	break;
	}

	switch (param){
	case GL_EXP:			p = "GL_EXP";					break;
	case GL_EXP2:			p = "GL_EXP2";					break;
	case GL_LINEAR:			p = "GL_LINEAR";				break;
	case GL_FRAGMENT_DEPTH:	p = "GL_FRAGMENT_DEPTH";		break;
	case GL_FOG_COORD:		p = "GL_FOG_COORD";				break;
	default:				p = Str_VarArgs("0x%08X", param);	break;
	}

	if (pname == GL_FOG_DENSITY || pname == GL_FOG_START || pname == GL_FOG_END)
		p = Str_VarArgs("%i", param);

	fprintf(qglState.logFile, "glFogi( %s, %s )\n", n, p);
	dllFogi(pname, param);
}

static GLvoid APIENTRY logFogiv (GLenum pname, const GLint *params){

	const char	*n;

	switch (pname){
	case GL_FOG_MODE:		n = "GL_FOG_MODE";				break;
	case GL_FOG_COORD_SRC:	n = "GL_FOG_COORD_SRC";			break;
	case GL_FOG_DENSITY:	n = "GL_FOG_DENSITY";			break;
	case GL_FOG_START:		n = "GL_FOG_START";				break;
	case GL_FOG_END:		n = "GL_FOG_END";				break;
	default:				n = Str_VarArgs("0x%08X", pname);	break;
	}

	fprintf(qglState.logFile, "glFogiv( %s, %p )\n", n, params);
	dllFogiv(pname, params);
}

static GLvoid APIENTRY logFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer){

	const char	*t, *a, *rt;

	switch (target){
	case GL_FRAMEBUFFER:				t = "GL_FRAMEBUFFER";							break;
	case GL_DRAW_FRAMEBUFFER:			t = "GL_DRAW_FRAMEBUFFER";						break;
	case GL_READ_FRAMEBUFFER:			t = "GL_READ_FRAMEBUFFER";						break;
	default:							t = Str_VarArgs("0x%08X", target);				break;
	}

	switch (attachment){
	case GL_COLOR_ATTACHMENT0:			a = "GL_COLOR_ATTACHMENT0";						break;
	case GL_COLOR_ATTACHMENT1:			a = "GL_COLOR_ATTACHMENT1";						break;
	case GL_COLOR_ATTACHMENT2:			a = "GL_COLOR_ATTACHMENT2";						break;
	case GL_COLOR_ATTACHMENT3:			a = "GL_COLOR_ATTACHMENT3";						break;
	case GL_COLOR_ATTACHMENT4:			a = "GL_COLOR_ATTACHMENT4";						break;
	case GL_COLOR_ATTACHMENT5:			a = "GL_COLOR_ATTACHMENT5";						break;
	case GL_COLOR_ATTACHMENT6:			a = "GL_COLOR_ATTACHMENT6";						break;
	case GL_COLOR_ATTACHMENT7:			a = "GL_COLOR_ATTACHMENT7";						break;
	case GL_COLOR_ATTACHMENT8:			a = "GL_COLOR_ATTACHMENT8";						break;
	case GL_COLOR_ATTACHMENT9:			a = "GL_COLOR_ATTACHMENT9";						break;
	case GL_COLOR_ATTACHMENT10:			a = "GL_COLOR_ATTACHMENT10";					break;
	case GL_COLOR_ATTACHMENT11:			a = "GL_COLOR_ATTACHMENT11";					break;
	case GL_COLOR_ATTACHMENT12:			a = "GL_COLOR_ATTACHMENT12";					break;
	case GL_COLOR_ATTACHMENT13:			a = "GL_COLOR_ATTACHMENT13";					break;
	case GL_COLOR_ATTACHMENT14:			a = "GL_COLOR_ATTACHMENT14";					break;
	case GL_COLOR_ATTACHMENT15:			a = "GL_COLOR_ATTACHMENT15";					break;
	case GL_DEPTH_ATTACHMENT:			a = "GL_DEPTH_ATTACHMENT";						break;
	case GL_STENCIL_ATTACHMENT:			a = "GL_STENCIL_ATTACHMENT";					break;
	case GL_DEPTH_STENCIL_ATTACHMENT:	a = "GL_DEPTH_STENCIL_ATTACHMENT";				break;
	default:							a = Str_VarArgs("0x%08X", attachment);			break;
	}

	switch (renderbuffertarget){
	case GL_RENDERBUFFER:				rt = "GL_RENDERBUFFER";							break;
	default:							rt = Str_VarArgs("0x%08X", renderbuffertarget);	break;
	}

	fprintf(qglState.logFile, "glFramebufferRenderbuffer( %s, %s, %s %u )\n", t, a, rt, renderbuffer);
	dllFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

static GLvoid APIENTRY logFramebufferTexture (GLenum target, GLenum attachment, GLuint texture, GLint level){

	const char	*t, *a;

	switch (target){
	case GL_FRAMEBUFFER:				t = "GL_FRAMEBUFFER";					break;
	case GL_DRAW_FRAMEBUFFER:			t = "GL_DRAW_FRAMEBUFFER";				break;
	case GL_READ_FRAMEBUFFER:			t = "GL_READ_FRAMEBUFFER";				break;
	default:							t = Str_VarArgs("0x%08X", target);		break;
	}

	switch (attachment){
	case GL_COLOR_ATTACHMENT0:			a = "GL_COLOR_ATTACHMENT0";				break;
	case GL_COLOR_ATTACHMENT1:			a = "GL_COLOR_ATTACHMENT1";				break;
	case GL_COLOR_ATTACHMENT2:			a = "GL_COLOR_ATTACHMENT2";				break;
	case GL_COLOR_ATTACHMENT3:			a = "GL_COLOR_ATTACHMENT3";				break;
	case GL_COLOR_ATTACHMENT4:			a = "GL_COLOR_ATTACHMENT4";				break;
	case GL_COLOR_ATTACHMENT5:			a = "GL_COLOR_ATTACHMENT5";				break;
	case GL_COLOR_ATTACHMENT6:			a = "GL_COLOR_ATTACHMENT6";				break;
	case GL_COLOR_ATTACHMENT7:			a = "GL_COLOR_ATTACHMENT7";				break;
	case GL_COLOR_ATTACHMENT8:			a = "GL_COLOR_ATTACHMENT8";				break;
	case GL_COLOR_ATTACHMENT9:			a = "GL_COLOR_ATTACHMENT9";				break;
	case GL_COLOR_ATTACHMENT10:			a = "GL_COLOR_ATTACHMENT10";			break;
	case GL_COLOR_ATTACHMENT11:			a = "GL_COLOR_ATTACHMENT11";			break;
	case GL_COLOR_ATTACHMENT12:			a = "GL_COLOR_ATTACHMENT12";			break;
	case GL_COLOR_ATTACHMENT13:			a = "GL_COLOR_ATTACHMENT13";			break;
	case GL_COLOR_ATTACHMENT14:			a = "GL_COLOR_ATTACHMENT14";			break;
	case GL_COLOR_ATTACHMENT15:			a = "GL_COLOR_ATTACHMENT15";			break;
	case GL_DEPTH_ATTACHMENT:			a = "GL_DEPTH_ATTACHMENT";				break;
	case GL_STENCIL_ATTACHMENT:			a = "GL_STENCIL_ATTACHMENT";			break;
	case GL_DEPTH_STENCIL_ATTACHMENT:	a = "GL_DEPTH_STENCIL_ATTACHMENT";		break;
	default:							a = Str_VarArgs("0x%08X", attachment);	break;
	}

	fprintf(qglState.logFile, "glFramebufferTexture( %s, %s, %u, %i )\n", t, a, texture, level);
	dllFramebufferTexture(target, attachment, texture, level);
}

static GLvoid APIENTRY logFramebufferTexture1D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level){

	const char	*t, *a, *tt;

	switch (target){
	case GL_FRAMEBUFFER:				t = "GL_FRAMEBUFFER";					break;
	case GL_DRAW_FRAMEBUFFER:			t = "GL_DRAW_FRAMEBUFFER";				break;
	case GL_READ_FRAMEBUFFER:			t = "GL_READ_FRAMEBUFFER";				break;
	default:							t = Str_VarArgs("0x%08X", target);		break;
	}

	switch (attachment){
	case GL_COLOR_ATTACHMENT0:			a = "GL_COLOR_ATTACHMENT0";				break;
	case GL_COLOR_ATTACHMENT1:			a = "GL_COLOR_ATTACHMENT1";				break;
	case GL_COLOR_ATTACHMENT2:			a = "GL_COLOR_ATTACHMENT2";				break;
	case GL_COLOR_ATTACHMENT3:			a = "GL_COLOR_ATTACHMENT3";				break;
	case GL_COLOR_ATTACHMENT4:			a = "GL_COLOR_ATTACHMENT4";				break;
	case GL_COLOR_ATTACHMENT5:			a = "GL_COLOR_ATTACHMENT5";				break;
	case GL_COLOR_ATTACHMENT6:			a = "GL_COLOR_ATTACHMENT6";				break;
	case GL_COLOR_ATTACHMENT7:			a = "GL_COLOR_ATTACHMENT7";				break;
	case GL_COLOR_ATTACHMENT8:			a = "GL_COLOR_ATTACHMENT8";				break;
	case GL_COLOR_ATTACHMENT9:			a = "GL_COLOR_ATTACHMENT9";				break;
	case GL_COLOR_ATTACHMENT10:			a = "GL_COLOR_ATTACHMENT10";			break;
	case GL_COLOR_ATTACHMENT11:			a = "GL_COLOR_ATTACHMENT11";			break;
	case GL_COLOR_ATTACHMENT12:			a = "GL_COLOR_ATTACHMENT12";			break;
	case GL_COLOR_ATTACHMENT13:			a = "GL_COLOR_ATTACHMENT13";			break;
	case GL_COLOR_ATTACHMENT14:			a = "GL_COLOR_ATTACHMENT14";			break;
	case GL_COLOR_ATTACHMENT15:			a = "GL_COLOR_ATTACHMENT15";			break;
	case GL_DEPTH_ATTACHMENT:			a = "GL_DEPTH_ATTACHMENT";				break;
	case GL_STENCIL_ATTACHMENT:			a = "GL_STENCIL_ATTACHMENT";			break;
	case GL_DEPTH_STENCIL_ATTACHMENT:	a = "GL_DEPTH_STENCIL_ATTACHMENT";		break;
	default:							a = Str_VarArgs("0x%08X", attachment);	break;
	}

	switch (textarget){
	case GL_TEXTURE_1D:					tt = "GL_TEXTURE_1D";					break;
	default:							tt = Str_VarArgs("0x%08X", textarget);	break;
	}

	fprintf(qglState.logFile, "glFramebufferTexture1D( %s, %s, %s, %u, %i )\n", t, a, tt, texture, level);
	dllFramebufferTexture1D(target, attachment, textarget, texture, level);
}

static GLvoid APIENTRY logFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level){

	const char	*t, *a, *tt;

	switch (target){
	case GL_FRAMEBUFFER:					t = "GL_FRAMEBUFFER";					break;
	case GL_DRAW_FRAMEBUFFER:				t = "GL_DRAW_FRAMEBUFFER";				break;
	case GL_READ_FRAMEBUFFER:				t = "GL_READ_FRAMEBUFFER";				break;
	default:								t = Str_VarArgs("0x%08X", target);		break;
	}

	switch (attachment){
	case GL_COLOR_ATTACHMENT0:				a = "GL_COLOR_ATTACHMENT0";				break;
	case GL_COLOR_ATTACHMENT1:				a = "GL_COLOR_ATTACHMENT1";				break;
	case GL_COLOR_ATTACHMENT2:				a = "GL_COLOR_ATTACHMENT2";				break;
	case GL_COLOR_ATTACHMENT3:				a = "GL_COLOR_ATTACHMENT3";				break;
	case GL_COLOR_ATTACHMENT4:				a = "GL_COLOR_ATTACHMENT4";				break;
	case GL_COLOR_ATTACHMENT5:				a = "GL_COLOR_ATTACHMENT5";				break;
	case GL_COLOR_ATTACHMENT6:				a = "GL_COLOR_ATTACHMENT6";				break;
	case GL_COLOR_ATTACHMENT7:				a = "GL_COLOR_ATTACHMENT7";				break;
	case GL_COLOR_ATTACHMENT8:				a = "GL_COLOR_ATTACHMENT8";				break;
	case GL_COLOR_ATTACHMENT9:				a = "GL_COLOR_ATTACHMENT9";				break;
	case GL_COLOR_ATTACHMENT10:				a = "GL_COLOR_ATTACHMENT10";			break;
	case GL_COLOR_ATTACHMENT11:				a = "GL_COLOR_ATTACHMENT11";			break;
	case GL_COLOR_ATTACHMENT12:				a = "GL_COLOR_ATTACHMENT12";			break;
	case GL_COLOR_ATTACHMENT13:				a = "GL_COLOR_ATTACHMENT13";			break;
	case GL_COLOR_ATTACHMENT14:				a = "GL_COLOR_ATTACHMENT14";			break;
	case GL_COLOR_ATTACHMENT15:				a = "GL_COLOR_ATTACHMENT15";			break;
	case GL_DEPTH_ATTACHMENT:				a = "GL_DEPTH_ATTACHMENT";				break;
	case GL_STENCIL_ATTACHMENT:				a = "GL_STENCIL_ATTACHMENT";			break;
	case GL_DEPTH_STENCIL_ATTACHMENT:		a = "GL_DEPTH_STENCIL_ATTACHMENT";		break;
	default:								a = Str_VarArgs("0x%08X", attachment);	break;
	}

	switch (textarget){
	case GL_TEXTURE_2D:						tt = "GL_TEXTURE_2D";					break;
	case GL_TEXTURE_2D_MULTISAMPLE:			tt = "GL_TEXTURE_2D_MULTISAMPLE";		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:	tt = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:	tt = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:	tt = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:	tt = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:	tt = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:	tt = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";	break;
	case GL_TEXTURE_RECTANGLE:				tt = "GL_TEXTURE_RECTANGLE";			break;
	default:								tt = Str_VarArgs("0x%08X", textarget);	break;
	}

	fprintf(qglState.logFile, "glFramebufferTexture2D( %s, %s, %s, %u, %i )\n", t, a, tt, texture, level);
	dllFramebufferTexture2D(target, attachment, textarget, texture, level);
}

static GLvoid APIENTRY logFramebufferTexture3D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer){

	const char	*t, *a, *tt;

	switch (target){
	case GL_FRAMEBUFFER:				t = "GL_FRAMEBUFFER";					break;
	case GL_DRAW_FRAMEBUFFER:			t = "GL_DRAW_FRAMEBUFFER";				break;
	case GL_READ_FRAMEBUFFER:			t = "GL_READ_FRAMEBUFFER";				break;
	default:							t = Str_VarArgs("0x%08X", target);		break;
	}

	switch (attachment){
	case GL_COLOR_ATTACHMENT0:			a = "GL_COLOR_ATTACHMENT0";				break;
	case GL_COLOR_ATTACHMENT1:			a = "GL_COLOR_ATTACHMENT1";				break;
	case GL_COLOR_ATTACHMENT2:			a = "GL_COLOR_ATTACHMENT2";				break;
	case GL_COLOR_ATTACHMENT3:			a = "GL_COLOR_ATTACHMENT3";				break;
	case GL_COLOR_ATTACHMENT4:			a = "GL_COLOR_ATTACHMENT4";				break;
	case GL_COLOR_ATTACHMENT5:			a = "GL_COLOR_ATTACHMENT5";				break;
	case GL_COLOR_ATTACHMENT6:			a = "GL_COLOR_ATTACHMENT6";				break;
	case GL_COLOR_ATTACHMENT7:			a = "GL_COLOR_ATTACHMENT7";				break;
	case GL_COLOR_ATTACHMENT8:			a = "GL_COLOR_ATTACHMENT8";				break;
	case GL_COLOR_ATTACHMENT9:			a = "GL_COLOR_ATTACHMENT9";				break;
	case GL_COLOR_ATTACHMENT10:			a = "GL_COLOR_ATTACHMENT10";			break;
	case GL_COLOR_ATTACHMENT11:			a = "GL_COLOR_ATTACHMENT11";			break;
	case GL_COLOR_ATTACHMENT12:			a = "GL_COLOR_ATTACHMENT12";			break;
	case GL_COLOR_ATTACHMENT13:			a = "GL_COLOR_ATTACHMENT13";			break;
	case GL_COLOR_ATTACHMENT14:			a = "GL_COLOR_ATTACHMENT14";			break;
	case GL_COLOR_ATTACHMENT15:			a = "GL_COLOR_ATTACHMENT15";			break;
	case GL_DEPTH_ATTACHMENT:			a = "GL_DEPTH_ATTACHMENT";				break;
	case GL_STENCIL_ATTACHMENT:			a = "GL_STENCIL_ATTACHMENT";			break;
	case GL_DEPTH_STENCIL_ATTACHMENT:	a = "GL_DEPTH_STENCIL_ATTACHMENT";		break;
	default:							a = Str_VarArgs("0x%08X", attachment);	break;
	}

	switch (textarget){
	case GL_TEXTURE_3D:					tt = "GL_TEXTURE_3D";					break;
	default:							tt = Str_VarArgs("0x%08X", textarget);	break;
	}

	fprintf(qglState.logFile, "glFramebufferTexture3D( %s, %s, %s, %u, %i, %i )\n", t, a, tt, texture, level, layer);
	dllFramebufferTexture3D(target, attachment, textarget, texture, level, layer);
}

static GLvoid APIENTRY logFramebufferTextureLayer (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer){

	const char	*t, *a;

	switch (target){
	case GL_FRAMEBUFFER:				t = "GL_FRAMEBUFFER";					break;
	case GL_DRAW_FRAMEBUFFER:			t = "GL_DRAW_FRAMEBUFFER";				break;
	case GL_READ_FRAMEBUFFER:			t = "GL_READ_FRAMEBUFFER";				break;
	default:							t = Str_VarArgs("0x%08X", target);		break;
	}

	switch (attachment){
	case GL_COLOR_ATTACHMENT0:			a = "GL_COLOR_ATTACHMENT0";				break;
	case GL_COLOR_ATTACHMENT1:			a = "GL_COLOR_ATTACHMENT1";				break;
	case GL_COLOR_ATTACHMENT2:			a = "GL_COLOR_ATTACHMENT2";				break;
	case GL_COLOR_ATTACHMENT3:			a = "GL_COLOR_ATTACHMENT3";				break;
	case GL_COLOR_ATTACHMENT4:			a = "GL_COLOR_ATTACHMENT4";				break;
	case GL_COLOR_ATTACHMENT5:			a = "GL_COLOR_ATTACHMENT5";				break;
	case GL_COLOR_ATTACHMENT6:			a = "GL_COLOR_ATTACHMENT6";				break;
	case GL_COLOR_ATTACHMENT7:			a = "GL_COLOR_ATTACHMENT7";				break;
	case GL_COLOR_ATTACHMENT8:			a = "GL_COLOR_ATTACHMENT8";				break;
	case GL_COLOR_ATTACHMENT9:			a = "GL_COLOR_ATTACHMENT9";				break;
	case GL_COLOR_ATTACHMENT10:			a = "GL_COLOR_ATTACHMENT10";			break;
	case GL_COLOR_ATTACHMENT11:			a = "GL_COLOR_ATTACHMENT11";			break;
	case GL_COLOR_ATTACHMENT12:			a = "GL_COLOR_ATTACHMENT12";			break;
	case GL_COLOR_ATTACHMENT13:			a = "GL_COLOR_ATTACHMENT13";			break;
	case GL_COLOR_ATTACHMENT14:			a = "GL_COLOR_ATTACHMENT14";			break;
	case GL_COLOR_ATTACHMENT15:			a = "GL_COLOR_ATTACHMENT15";			break;
	case GL_DEPTH_ATTACHMENT:			a = "GL_DEPTH_ATTACHMENT";				break;
	case GL_STENCIL_ATTACHMENT:			a = "GL_STENCIL_ATTACHMENT";			break;
	case GL_DEPTH_STENCIL_ATTACHMENT:	a = "GL_DEPTH_STENCIL_ATTACHMENT";		break;
	default:							a = Str_VarArgs("0x%08X", attachment);	break;
	}

	fprintf(qglState.logFile, "glFramebufferTextureLayer( %s, %s, %u, %i, %i )\n", t, a, texture, level, layer);
	dllFramebufferTextureLayer(target, attachment, texture, level, layer);
}

static GLvoid APIENTRY logFrontFace (GLenum mode){

	const char	*m;

	switch (mode){
	case GL_CW:		m = "GL_CW";					break;
	case GL_CCW:	m = "GL_CCW";					break;
	default:		m = Str_VarArgs("0x%08X", mode);	break;
	}

	fprintf(qglState.logFile, "glFrontFace( %s )\n", m);
	dllFrontFace(mode);
}

static GLvoid APIENTRY logFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar){

	fprintf(qglState.logFile, "glFrustum( %g, %g, %g, %g, %g, %g )\n", left, right, bottom, top, zNear, zFar);
	dllFrustum(left, right, bottom, top, zNear, zFar);
}

static GLvoid APIENTRY logGenBuffers (GLsizei n, GLuint *buffers){

	fprintf(qglState.logFile, "glGenBuffers( %i, %p )\n", n, buffers);
	dllGenBuffers(n, buffers);
}

static GLvoid APIENTRY logGenFramebuffers (GLsizei n, GLuint *framebuffers){

	fprintf(qglState.logFile, "glGenFramebuffers( %i, %p )\n", n, framebuffers);
	dllGenFramebuffers(n, framebuffers);
}

static GLuint APIENTRY logGenLists (GLsizei range){

	fprintf(qglState.logFile, "glGenLists( %i )\n", range);
	return dllGenLists(range);
}

static GLvoid APIENTRY logGenQueries (GLsizei n, GLuint *ids){

	fprintf(qglState.logFile, "glGenQueries( %i, %p )\n", n, ids);
	dllGenQueries(n, ids);
}

static GLvoid APIENTRY logGenRenderbuffers (GLsizei n, GLuint *renderbuffers){

	fprintf(qglState.logFile, "glGenRenderbuffers( %i, %p )\n", n, renderbuffers);
	dllGenRenderbuffers(n, renderbuffers);
}

static GLvoid APIENTRY logGenSamplers (GLsizei n, GLuint *samplers){

	fprintf(qglState.logFile, "glGenSamplers( %i, %p )\n", n, samplers);
	dllGenSamplers(n, samplers);
}

static GLvoid APIENTRY logGenTextures (GLsizei n, GLuint *textures){

	fprintf(qglState.logFile, "glGenTextures( %i, %p )\n", n, textures);
	dllGenTextures(n, textures);
}

static GLvoid APIENTRY logGenVertexArrays (GLsizei n, GLuint *vertexArrays){

	fprintf(qglState.logFile, "glGenVertexArrays( %i, %p )\n", n, vertexArrays);
	dllGenVertexArrays(n, vertexArrays);
}

static GLvoid APIENTRY logGenerateMipmap (GLenum target){

	const char	*t;

	switch (target){
	case GL_TEXTURE_1D:			t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_1D_ARRAY:	t = "GL_TEXTURE_1D_ARRAY";			break;
	case GL_TEXTURE_2D:			t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_2D_ARRAY:	t = "GL_TEXTURE_2D_ARRAY";			break;
	case GL_TEXTURE_3D:			t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP:	t = "GL_TEXTURE_CUBE_MAP";			break;
	default:					t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glGenerateMipmap( %s )\n", t);
	dllGenerateMipmap(target);
}

static GLvoid APIENTRY logGetActiveAttrib (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name){

	fprintf(qglState.logFile, "glGetActiveAttrib( %u, %u, %i, %p, %p, %p, %p )\n", program, index, bufSize, length, size, type, name);
	dllGetActiveAttrib(program, index, bufSize, length, size, type, name);
}

static GLvoid APIENTRY logGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name){

	fprintf(qglState.logFile, "glGetActiveUniform( %u, %u, %i, %p, %p, %p, %p )\n", program, index, bufSize, length, size, type, name);
	dllGetActiveUniform(program, index, bufSize, length, size, type, name);
}

static GLvoid APIENTRY logGetActiveUniformBlockName (GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName){

	fprintf(qglState.logFile, "glGetActiveUniformBlockName( %u, %u, %i, %p, %p )\n", program, uniformBlockIndex, bufSize, length, uniformBlockName);
	dllGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
}

static GLvoid APIENTRY logGetActiveUniformBlockiv (GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetActiveUniformBlockiv( %u, %u, 0x%08X, %p )\n", program, uniformBlockIndex, pname, params);
	dllGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
}

static GLvoid APIENTRY logGetActiveUniformName (GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName){

	fprintf(qglState.logFile, "glGetActiveUniformName( %u, %u, %i, %p, %p )\n", program, uniformIndex, bufSize, length, uniformName);
	dllGetActiveUniformName(program, uniformIndex, bufSize, length, uniformName);
}

static GLvoid APIENTRY logGetActiveUniformsiv (GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetActiveUniformsiv( %u, %i, %p, 0x%08X, %p )\n", program, uniformCount, uniformIndices, pname, params);
	dllGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
}

static GLvoid APIENTRY logGetAttachedShaders (GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders){

	fprintf(qglState.logFile, "glGetAttachedShaders( %u, %i, %p, %p )\n", program, maxCount, count, shaders);
	dllGetAttachedShaders(program, maxCount, count, shaders);
}

static GLint APIENTRY logGetAttribLocation (GLuint program, const GLchar *name){

	fprintf(qglState.logFile, "glGetAttribLocation( %u, %s )\n", program, name);
	return dllGetAttribLocation(program, name);
}

static GLvoid APIENTRY logGetBooleani_v (GLenum target, GLuint index, GLboolean *params){

	fprintf(qglState.logFile, "glGetBooleani_v( 0x%08X, %u, %p )\n", target, index, params);
	dllGetBooleani_v(target, index, params);
}

static GLvoid APIENTRY logGetBooleanv (GLenum pname, GLboolean *params){

	fprintf(qglState.logFile, "glGetBooleanv( 0x%08X, %p )\n", pname, params);
	dllGetBooleanv(pname, params);
}

static GLvoid APIENTRY logGetBufferParameteri64v (GLenum target, GLenum pname, GLint64 *params){

	fprintf(qglState.logFile, "glGetBufferParameteri64v( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetBufferParameteri64v(target, pname, params);
}

static GLvoid APIENTRY logGetBufferParameteriv (GLenum target, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetBufferParameteriv( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetBufferParameteriv(target, pname, params);
}

static GLvoid APIENTRY logGetBufferPointerv (GLenum target, GLenum pname, GLvoid **params){

	fprintf(qglState.logFile, "glGetBufferPointerv( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetBufferPointerv(target, pname, params);
}

static GLvoid APIENTRY logGetBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, GLvoid *data){

	fprintf(qglState.logFile, "glGetBufferSubData( 0x%08X, %i, %i, %p )\n", target, offset, size, data);
	dllGetBufferSubData(target, offset, size, data);
}

static GLvoid APIENTRY logGetClipPlane (GLenum plane, GLdouble *equation){

	fprintf(qglState.logFile, "glGetClipPlane( 0x%08X, %p )\n", plane, equation);
	dllGetClipPlane(plane, equation);
}

static GLvoid APIENTRY logGetCompressedTexImage (GLenum target, GLint level, GLvoid *pixels){

	fprintf(qglState.logFile, "glGetCompressedTexImage( 0x%08X, %i, %p )\n", target, level, pixels);
	dllGetCompressedTexImage(target, level, pixels);
}

static GLvoid APIENTRY logGetDoublev (GLenum pname, GLdouble *params){

	fprintf(qglState.logFile, "glGetDoublev( 0x%08X, %p )\n", pname, params);
	dllGetDoublev(pname, params);
}

static GLenum APIENTRY logGetError (GLvoid){

	fprintf(qglState.logFile, "glGetError()\n");
	return dllGetError();
}

static GLvoid APIENTRY logGetFloatv (GLenum pname, GLfloat *params){

	fprintf(qglState.logFile, "glGetFloatv( 0x%08X, %p )\n", pname, params);
	dllGetFloatv(pname, params);
}

static GLint APIENTRY logGetFragDataIndex (GLuint program, const GLchar *name){

	fprintf(qglState.logFile, "glGetFragDataIndex( %u, %s )\n", program, name);
	return dllGetFragDataIndex(program, name);
}

static GLint APIENTRY logGetFragDataLocation (GLuint program, const GLchar *name){

	fprintf(qglState.logFile, "glGetFragDataLocation( %u, %s )\n", program, name);
	return dllGetFragDataLocation(program, name);
}

static GLvoid APIENTRY logGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetFramebufferAttachmentParameteriv( 0x%08X, 0x%08X, 0x%08X, %p )\n", target, attachment, pname, params);
	dllGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

static GLvoid APIENTRY logGetInteger64i_v (GLenum target, GLuint index, GLint64 *params){

	fprintf(qglState.logFile, "glGetInteger64i_v( 0x%08X, %u, %p )\n", target, index, params);
	dllGetInteger64i_v(target, index, params);
}

static GLvoid APIENTRY logGetInteger64v (GLenum pname, GLint64 *params){

	fprintf(qglState.logFile, "glGetInteger64v( 0x%08X, %p )\n", pname, params);
	dllGetInteger64v(pname, params);
}

static GLvoid APIENTRY logGetIntegeri_v (GLenum target, GLuint index, GLint *params){

	fprintf(qglState.logFile, "glGetIntegeri_v( 0x%08X, %u, %p )\n", target, index, params);
	dllGetIntegeri_v(target, index, params);
}

static GLvoid APIENTRY logGetIntegerv (GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetIntegerv( 0x%08X, %p )\n", pname, params);
	dllGetIntegerv(pname, params);
}

static GLvoid APIENTRY logGetLightfv (GLenum light, GLenum pname, GLfloat *params){

	fprintf(qglState.logFile, "glGetLightfv( 0x%08X, 0x%08X, %p )\n", light, pname, params);
	dllGetLightfv(light, pname, params);
}

static GLvoid APIENTRY logGetLightiv (GLenum light, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetLightiv( 0x%08X, 0x%08X, %p )\n", light, pname, params);
	dllGetLightiv(light, pname, params);
}

static GLvoid APIENTRY logGetMapdv (GLenum target, GLenum query, GLdouble *v){

	fprintf(qglState.logFile, "glGetMapdv( 0x%08X, 0x%08X, %p )\n", target, query, v);
	dllGetMapdv(target, query, v);
}

static GLvoid APIENTRY logGetMapfv (GLenum target, GLenum query, GLfloat *v){

	fprintf(qglState.logFile, "glGetMapfv( 0x%08X, 0x%08X, %p )\n", target, query, v);
	dllGetMapfv(target, query, v);
}

static GLvoid APIENTRY logGetMapiv (GLenum target, GLenum query, GLint *v){

	fprintf(qglState.logFile, "glGetMapiv( 0x%08X, 0x%08X, %p )\n", target, query, v);
	dllGetMapiv(target, query, v);
}

static GLvoid APIENTRY logGetMaterialfv (GLenum face, GLenum pname, GLfloat *params){

	fprintf(qglState.logFile, "glGetMaterialfv( 0x%08X, 0x%08X, %p )\n", face, pname, params);
	dllGetMaterialfv(face, pname, params);
}

static GLvoid APIENTRY logGetMaterialiv (GLenum face, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetMaterialiv( 0x%08X, 0x%08X, %p )\n", face, pname, params);
	dllGetMaterialiv(face, pname, params);
}

static GLvoid APIENTRY logGetMultisamplefv (GLenum pname, GLuint index, GLfloat *params){

	fprintf(qglState.logFile, "glGetMultisamplefv( 0x%08X, %u, %p )\n", pname, index, params);
	dllGetMultisamplefv(pname, index, params);
}

static GLvoid APIENTRY logGetPixelMapfv (GLenum map, GLfloat *values){

	fprintf(qglState.logFile, "glGetPixelMapfv( 0x%08X, %p )\n", map, values);
	dllGetPixelMapfv(map, values);
}

static GLvoid APIENTRY logGetPixelMapuiv (GLenum map, GLuint *values){

	fprintf(qglState.logFile, "glGetPixelMapuiv( 0x%08X, %p )\n", map, values);
	dllGetPixelMapuiv(map, values);
}

static GLvoid APIENTRY logGetPixelMapusv (GLenum map, GLushort *values){

	fprintf(qglState.logFile, "glGetPixelMapusv( 0x%08X, %p )\n", map, values);
	dllGetPixelMapusv(map, values);
}

static GLvoid APIENTRY logGetPointerv (GLenum pname, GLvoid **params){

	fprintf(qglState.logFile, "glGetPointerv( 0x%08X, %p )\n", pname, params);
	dllGetPointerv(pname, params);
}

static GLvoid APIENTRY logGetPolygonStipple (GLubyte *mask){

	fprintf(qglState.logFile, "glGetPolygonStipple( %p )\n", mask);
	dllGetPolygonStipple(mask);
}

static GLvoid APIENTRY logGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog){

	fprintf(qglState.logFile, "glGetProgramInfoLog( %u, %i, %p, %p )\n", program, bufSize, length, infoLog);
	dllGetProgramInfoLog(program, bufSize, length, infoLog);
}

static GLvoid APIENTRY logGetProgramiv (GLuint program, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetProgramiv( %u, 0x%08X, %p )\n", program, pname, params);
	dllGetProgramiv(program, pname, params);
}

static GLvoid APIENTRY logGetQueryObjecti64v (GLuint id, GLenum pname, GLint64 *params){

	fprintf(qglState.logFile, "glGetQueryObjecti64v( %u, 0x%08X, %p )\n", id, pname, params);
	dllGetQueryObjecti64v(id, pname, params);
}

static GLvoid APIENTRY logGetQueryObjectiv (GLuint id, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetQueryObjectiv( %u, 0x%08X, %p )\n", id, pname, params);
	dllGetQueryObjectiv(id, pname, params);
}

static GLvoid APIENTRY logGetQueryObjectui64v (GLuint id, GLenum pname, GLuint64 *params){

	fprintf(qglState.logFile, "glGetQueryObjectui64v( %u, 0x%08X, %p )\n", id, pname, params);
	dllGetQueryObjectui64v(id, pname, params);
}

static GLvoid APIENTRY logGetQueryObjectuiv (GLuint id, GLenum pname, GLuint *params){

	fprintf(qglState.logFile, "glGetQueryObjectuiv( %u, 0x%08X, %p )\n", id, pname, params);
	dllGetQueryObjectuiv(id, pname, params);
}

static GLvoid APIENTRY logGetQueryiv (GLenum target, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetQueryiv( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetQueryiv(target, pname, params);
}

static GLvoid APIENTRY logGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetRenderbufferParameteriv( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetRenderbufferParameteriv(target, pname, params);
}

static GLvoid APIENTRY logGetSamplerParameterIiv (GLuint sampler, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetSamplerParameterIiv( %u, 0x%08X, %p )\n", sampler, pname, params);
	dllGetSamplerParameterIiv(sampler, pname, params);
}

static GLvoid APIENTRY logGetSamplerParameterIuiv (GLuint sampler, GLenum pname, GLuint *params){

	fprintf(qglState.logFile, "glGetSamplerParameterIuiv( %u, 0x%08X, %p )\n", sampler, pname, params);
	dllGetSamplerParameterIuiv(sampler, pname, params);
}

static GLvoid APIENTRY logGetSamplerParameterfv (GLuint sampler, GLenum pname, GLfloat *params){

	fprintf(qglState.logFile, "glGetSamplerParameterfv( %u, 0x%08X, %p )\n", sampler, pname, params);
	dllGetSamplerParameterfv(sampler, pname, params);
}

static GLvoid APIENTRY logGetSamplerParameteriv (GLuint sampler, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetSamplerParameteriv( %u, 0x%08X, %p )\n", sampler, pname, params);
	dllGetSamplerParameteriv(sampler, pname, params);
}

static GLvoid APIENTRY logGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog){

	fprintf(qglState.logFile, "glGetShaderInfoLog( %u, %i, %p, %p )\n", shader, bufSize, length, infoLog);
	dllGetShaderInfoLog(shader, bufSize, length, infoLog);
}

static GLvoid APIENTRY logGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source){

	fprintf(qglState.logFile, "glGetShaderSource( %u, %i, %p, %p )\n", shader, bufSize, length, source);
	dllGetShaderSource(shader, bufSize, length, source);
}

static GLvoid APIENTRY logGetShaderiv (GLuint shader, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetShaderiv( %u, 0x%08X, %p )\n", shader, pname, params);
	dllGetShaderiv(shader, pname, params);
}

static const GLubyte * APIENTRY logGetString (GLenum name){

	fprintf(qglState.logFile, "glGetString( 0x%08X )\n", name);
	return dllGetString(name);
}

static const GLubyte * APIENTRY logGetStringi (GLenum name, GLuint index){

	fprintf(qglState.logFile, "glGetStringi( 0x%08X, %u )\n", name, index);
	return dllGetStringi(name, index);
}

static GLvoid APIENTRY logGetSynciv (GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values){

	fprintf(qglState.logFile, "glGetSynciv( %p, 0x%08X, %i, %p, %p )\n", sync, pname, bufSize, length, values);
	dllGetSynciv(sync, pname, bufSize, length, values);
}

static GLvoid APIENTRY logGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params){

	fprintf(qglState.logFile, "glGetTexEnvfv( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetTexEnvfv(target, pname, params);
}

static GLvoid APIENTRY logGetTexEnviv (GLenum target, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetTexEnviv( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetTexEnviv(target, pname, params);
}

static GLvoid APIENTRY logGetTexGendv (GLenum coord, GLenum pname, GLdouble *params){

	fprintf(qglState.logFile, "glGetTexGendv( 0x%08X, 0x%08X, %p )\n", coord, pname, params);
	dllGetTexGendv(coord, pname, params);
}

static GLvoid APIENTRY logGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params){

	fprintf(qglState.logFile, "glGetTexGenfv( 0x%08X, 0x%08X, %p )\n", coord, pname, params);
	dllGetTexGenfv(coord, pname, params);
}

static GLvoid APIENTRY logGetTexGeniv (GLenum coord, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetTexGeniv( 0x%08X, 0x%08X, %p )\n", coord, pname, params);
	dllGetTexGeniv(coord, pname, params);
}

static GLvoid APIENTRY logGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels){

	fprintf(qglState.logFile, "glGetTexImage( 0x%08X, %i, 0x%08X, 0x%08X, %p )\n", target, level, format, type, pixels);
	dllGetTexImage(target, level, format, type, pixels);
}

static GLvoid APIENTRY logGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params){

	fprintf(qglState.logFile, "glGetTexLevelParameterfv( 0x%08X, %i, 0x%08X, %p )\n", target, level, pname, params);
	dllGetTexLevelParameterfv(target, level, pname, params);
}

static GLvoid APIENTRY logGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetTexLevelParameteriv( 0x%08X, %i, 0x%08X, %p )\n", target, level, pname, params);
	dllGetTexLevelParameteriv(target, level, pname, params);
}

static GLvoid APIENTRY logGetTexParameterIiv (GLenum target, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetTexParameterIiv( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetTexParameterIiv(target, pname, params);
}

static GLvoid APIENTRY logGetTexParameterIuiv (GLenum target, GLenum pname, GLuint *params){

	fprintf(qglState.logFile, "glGetTexParameterIuiv( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetTexParameterIuiv(target, pname, params);
}

static GLvoid APIENTRY logGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params){

	fprintf(qglState.logFile, "glGetTexParameterfv( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetTexParameterfv(target, pname, params);
}

static GLvoid APIENTRY logGetTexParameteriv (GLenum target, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetTexParameteriv( 0x%08X, 0x%08X, %p )\n", target, pname, params);
	dllGetTexParameteriv(target, pname, params);
}

static GLvoid APIENTRY logGetTransformFeedbackVarying (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name){

	fprintf(qglState.logFile, "glGetTransformFeedbackVarying( %u, %u, %i, %p, %p, %p, %p )\n", program, index, bufSize, length, size, type, name);
	dllGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
}

static GLuint APIENTRY logGetUniformBlockIndex (GLuint program, const GLchar *uniformBlockName){

	fprintf(qglState.logFile, "glGetUniformBlockIndex( %u, %s )\n", program, uniformBlockName);
	return dllGetUniformBlockIndex(program, uniformBlockName);
}

static GLvoid APIENTRY logGetUniformIndices (GLuint program, GLsizei uniformCount, const GLchar **uniformNames, GLuint *uniformIndices){

	fprintf(qglState.logFile, "glGetUniformIndices( %u, %i, %p, %p )\n", program, uniformCount, uniformNames, uniformIndices);
	dllGetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
}

static GLint APIENTRY logGetUniformLocation (GLuint program, const GLchar *name){

	fprintf(qglState.logFile, "glGetUniformLocation( %u, %s )\n", program, name);
	return dllGetUniformLocation(program, name);
}

static GLvoid APIENTRY logGetUniformfv (GLuint program, GLint location, GLfloat *params){

	fprintf(qglState.logFile, "glGetUniformfv( %u, %i, %p )\n", program, location, params);
	dllGetUniformfv(program, location, params);
}

static GLvoid APIENTRY logGetUniformiv (GLuint program, GLint location, GLint *params){

	fprintf(qglState.logFile, "glGetUniformiv( %u, %i, %p )\n", program, location, params);
	dllGetUniformiv(program, location, params);
}

static GLvoid APIENTRY logGetUniformuiv (GLuint program, GLint location, GLuint *params){

	fprintf(qglState.logFile, "glGetUniformiv( %u, %i, %p )\n", program, location, params);
	dllGetUniformuiv(program, location, params);
}

static GLvoid APIENTRY logGetVertexAttribIiv (GLuint index, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetVertexAttribIiv( %u, 0x%08X, %p )\n", index, pname, params);
	dllGetVertexAttribIiv(index, pname, params);
}

static GLvoid APIENTRY logGetVertexAttribIuiv (GLuint index, GLenum pname, GLuint *params){

	fprintf(qglState.logFile, "glGetVertexAttribIuiv( %u, 0x%08X, %p )\n", index, pname, params);
	dllGetVertexAttribIuiv(index, pname, params);
}

static GLvoid APIENTRY logGetVertexAttribPointerv (GLuint index, GLenum pname, GLvoid **params){

	fprintf(qglState.logFile, "glGetVertexAttribPointerv( %u, 0x%08X, %p )\n", index, pname, params);
	dllGetVertexAttribPointerv(index, pname, params);
}

static GLvoid APIENTRY logGetVertexAttribdv (GLuint index, GLenum pname, GLdouble *params){

	fprintf(qglState.logFile, "glGetVertexAttribdv( %u, 0x%08X, %p )\n", index, pname, params);
	dllGetVertexAttribdv(index, pname, params);
}

static GLvoid APIENTRY logGetVertexAttribfv (GLuint index, GLenum pname, GLfloat *params){

	fprintf(qglState.logFile, "glGetVertexAttribfv( %u, 0x%08X, %p )\n", index, pname, params);
	dllGetVertexAttribfv(index, pname, params);
}

static GLvoid APIENTRY logGetVertexAttribiv (GLuint index, GLenum pname, GLint *params){

	fprintf(qglState.logFile, "glGetVertexAttribiv( %u, 0x%08X, %p )\n", index, pname, params);
	dllGetVertexAttribiv(index, pname, params);
}

static GLvoid APIENTRY logHint (GLenum target, GLenum mode){

	fprintf(qglState.logFile, "glHint( 0x%08X, 0x%08X )\n", target, mode);
	dllHint(target, mode);
}

static GLvoid APIENTRY logIndexMask (GLuint mask){

	fprintf(qglState.logFile, "glIndexMask( %u )\n", mask);
	dllIndexMask(mask);
}

static GLvoid APIENTRY logIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer){

	const char	*t;

	switch (type){
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";			break;
	case GL_SHORT:			t = "GL_SHORT";					break;
	case GL_INT:			t = "GL_INT";					break;
	case GL_FLOAT:			t = "GL_FLOAT";					break;
	case GL_DOUBLE:			t = "GL_DOUBLE";				break;
	default:				t = Str_VarArgs("0x%08X", type);	break;
	}

	fprintf(qglState.logFile, "glIndexPointer( %s, %i, %p )\n", t, stride, pointer);
	dllIndexPointer(type, stride, pointer);
}

static GLvoid APIENTRY logIndexd (GLdouble c){

	fprintf(qglState.logFile, "glIndexd( %g )\n", c);
	dllIndexd(c);
}

static GLvoid APIENTRY logIndexdv (const GLdouble *c){

	fprintf(qglState.logFile, "glIndexdv( %p )\n", c);
	dllIndexdv(c);
}

static GLvoid APIENTRY logIndexf (GLfloat c){

	fprintf(qglState.logFile, "glIndexf( %g )\n", c);
	dllIndexf(c);
}

static GLvoid APIENTRY logIndexfv (const GLfloat *c){

	fprintf(qglState.logFile, "glIndexfv( %p )\n", c);
	dllIndexfv(c);
}

static GLvoid APIENTRY logIndexi (GLint c){

	fprintf(qglState.logFile, "glIndexi( %i )\n", c);
	dllIndexi(c);
}

static GLvoid APIENTRY logIndexiv (const GLint *c){

	fprintf(qglState.logFile, "glIndexiv( %p )\n", c);
	dllIndexiv(c);
}

static GLvoid APIENTRY logIndexs (GLshort c){

	fprintf(qglState.logFile, "glIndexs( %i )\n", c);
	dllIndexs(c);
}

static GLvoid APIENTRY logIndexsv (const GLshort *c){

	fprintf(qglState.logFile, "glIndexsv( %p )\n", c);
	dllIndexsv(c);
}

static GLvoid APIENTRY logIndexub (GLubyte c){

	fprintf(qglState.logFile, "glIndexub( %u )\n", c);
	dllIndexub(c);
}

static GLvoid APIENTRY logIndexubv (const GLubyte *c){

	fprintf(qglState.logFile, "glIndexubv( %p )\n", c);
	dllIndexubv(c);
}

static GLvoid APIENTRY logInitNames (GLvoid){

	fprintf(qglState.logFile, "glInitNames()\n");
	dllInitNames();
}

static GLvoid APIENTRY logInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer){

	fprintf(qglState.logFile, "glInterleavedArrays( 0x%08X, %i, %p )\n", format, stride, pointer);
	dllInterleavedArrays(format, stride, pointer);
}

static GLboolean APIENTRY logIsBuffer (GLuint buffer){

	fprintf(qglState.logFile, "glIsBuffer( %u )\n", buffer);
	return dllIsBuffer(buffer);
}

static GLboolean APIENTRY logIsEnabled (GLenum cap){

	fprintf(qglState.logFile, "glIsEnabled( 0x%08X )\n", cap);
	return dllIsEnabled(cap);
}

static GLboolean APIENTRY logIsEnabledi (GLenum target, GLuint index){

	fprintf(qglState.logFile, "glIsEnabledi( 0x%08X, %u )\n", target, index);
	return dllIsEnabledi(target, index);
}

static GLboolean APIENTRY logIsFramebuffer (GLuint framebuffer){

	fprintf(qglState.logFile, "glIsFramebuffer( %u )\n", framebuffer);
	return dllIsFramebuffer(framebuffer);
}

static GLboolean APIENTRY logIsList (GLuint list){

	fprintf(qglState.logFile, "glIsList( %u )\n", list);
	return dllIsList(list);
}

static GLboolean APIENTRY logIsProgram (GLuint program){

	fprintf(qglState.logFile, "glIsProgram( %u )\n", program);
	return dllIsProgram(program);
}

static GLboolean APIENTRY logIsQuery (GLuint id){

	fprintf(qglState.logFile, "glIsQuery( %u )\n", id);
	return dllIsQuery(id);
}

static GLboolean APIENTRY logIsRenderbuffer (GLuint renderbuffer){

	fprintf(qglState.logFile, "glIsRenderbuffer( %u )\n", renderbuffer);
	return dllIsRenderbuffer(renderbuffer);
}

static GLboolean APIENTRY logIsSampler (GLuint sampler){

	fprintf(qglState.logFile, "glIsSampler( %u )\n", sampler);
	return dllIsSampler(sampler);
}

static GLboolean APIENTRY logIsShader (GLuint shader){

	fprintf(qglState.logFile, "glIsShader( %u )\n", shader);
	return dllIsShader(shader);
}

static GLboolean APIENTRY logIsSync (GLsync sync){

	fprintf(qglState.logFile, "glIsSync( %p )\n", sync);
	return dllIsSync(sync);
}

static GLboolean APIENTRY logIsTexture (GLuint texture){

	fprintf(qglState.logFile, "glIsTexture( %u )\n", texture);
	return dllIsTexture(texture);
}

static GLboolean APIENTRY logIsVertexArray (GLuint vertexArray){

	fprintf(qglState.logFile, "glIsVertexArray( %u )\n", vertexArray);
	return dllIsVertexArray(vertexArray);
}

static GLvoid APIENTRY logLightModelf (GLenum pname, GLfloat param){

	fprintf(qglState.logFile, "glLightModelf( 0x%08X, %g )\n", pname, param);
	dllLightModelf(pname, param);
}

static GLvoid APIENTRY logLightModelfv (GLenum pname, const GLfloat *params){

	fprintf(qglState.logFile, "glLightModelfv( 0x%08X, %p )\n", pname, params);
	dllLightModelfv(pname, params);
}

static GLvoid APIENTRY logLightModeli (GLenum pname, GLint param){

	fprintf(qglState.logFile, "glLightModeli( 0x%08X, %i )\n", pname, param);
	dllLightModeli(pname, param);
}

static GLvoid APIENTRY logLightModeliv (GLenum pname, const GLint *params){

	fprintf(qglState.logFile, "glLightModeliv( 0x%08X, %p )\n", pname, params);
	dllLightModeliv(pname, params);
}

static GLvoid APIENTRY logLightf (GLenum light, GLenum pname, GLfloat param){

	fprintf(qglState.logFile, "glLightf( 0x%08X, 0x%08X, %g )\n", light, pname, param);
	dllLightf(light, pname, param);
}

static GLvoid APIENTRY logLightfv (GLenum light, GLenum pname, const GLfloat *params){

	fprintf(qglState.logFile, "glLightfv( 0x%08X, 0x%08X, %p )\n", light, pname, params);
	dllLightfv(light, pname, params);
}

static GLvoid APIENTRY logLighti (GLenum light, GLenum pname, GLint param){

	fprintf(qglState.logFile, "glLighti( 0x%08X, 0x%08X, %i )\n", light, pname, param);
	dllLighti(light, pname, param);
}

static GLvoid APIENTRY logLightiv (GLenum light, GLenum pname, const GLint *params){

	fprintf(qglState.logFile, "glLightiv( 0x%08X, 0x%08X, %p )\n", light, pname, params);
	dllLightiv(light, pname, params);
}

static GLvoid APIENTRY logLineStipple (GLint factor, GLushort pattern){

	fprintf(qglState.logFile, "glLineStipple( %i, %u )\n", factor, pattern);
	dllLineStipple(factor, pattern);
}

static GLvoid APIENTRY logLineWidth (GLfloat width){

	fprintf(qglState.logFile, "glLineWidth( %g )\n", width);
	dllLineWidth(width);
}

static GLvoid APIENTRY logLinkProgram (GLuint program){

	fprintf(qglState.logFile, "glLinkProgram( %u )\n", program);
	dllLinkProgram(program);
}

static GLvoid APIENTRY logListBase (GLuint base){

	fprintf(qglState.logFile, "glListBase( %u )\n", base);
	dllListBase(base);
}

static GLvoid APIENTRY logLoadIdentity (GLvoid){

	fprintf(qglState.logFile, "glLoadIdentity()\n");
	dllLoadIdentity();
}

static GLvoid APIENTRY logLoadMatrixd (const GLdouble *m){

	fprintf(qglState.logFile, "glLoadMatrixd( %p )\n", m);
	dllLoadMatrixd(m);
}

static GLvoid APIENTRY logLoadMatrixf (const GLfloat *m){

	fprintf(qglState.logFile, "glLoadMatrixf( %p )\n", m);
	dllLoadMatrixf(m);
}

static GLvoid APIENTRY logLoadName (GLuint name){

	fprintf(qglState.logFile, "glLoadName( %u )\n", name);
	dllLoadName(name);
}

static GLvoid APIENTRY logLoadTransposeMatrixd (const GLdouble *m){

	fprintf(qglState.logFile, "glLoadTransposeMatrixd( %p )\n", m);
	dllLoadTransposeMatrixd(m);
}

static GLvoid APIENTRY logLoadTransposeMatrixf (const GLfloat *m){

	fprintf(qglState.logFile, "glLoadTransposeMatrixf( %p )\n", m);
	dllLoadTransposeMatrixf(m);
}

static GLvoid APIENTRY logLogicOp (GLenum opcode){

	fprintf(qglState.logFile, "glLogicOp( 0x%08X )\n", opcode);
	dllLogicOp(opcode);
}

static GLvoid APIENTRY logMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points){

	fprintf(qglState.logFile, "glMap1d( 0x%08X, %g, %g, %i, %i, %p )\n", target, u1, u2, stride, order, points);
	dllMap1d(target, u1, u2, stride, order, points);
}

static GLvoid APIENTRY logMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points){

	fprintf(qglState.logFile, "glMap1f( 0x%08X, %g, %g, %i, %i, %p )\n", target, u1, u2, stride, order, points);
	dllMap1f(target, u1, u2, stride, order, points);
}

static GLvoid APIENTRY logMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points){

	fprintf(qglState.logFile, "glMap2d( 0x%08X, %g, %g, %i, %i, %g, %g, %i, %i, %p )\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
	dllMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

static GLvoid APIENTRY logMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points){

	fprintf(qglState.logFile, "glMap2f( 0x%08X, %g, %g, %i, %i, %g, %g, %i, %i, %p )\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
	dllMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

static GLvoid * APIENTRY logMapBuffer (GLenum target, GLenum access){

	const char	*t, *a;

	switch (target){
	case GL_ARRAY_BUFFER:				t = "GL_ARRAY_BUFFER";				break;
	case GL_ELEMENT_ARRAY_BUFFER:		t = "GL_ELEMENT_ARRAY_BUFFER";		break;
	case GL_PIXEL_PACK_BUFFER:			t = "GL_PIXEL_PACK_BUFFER";			break;
	case GL_PIXEL_UNPACK_BUFFER:		t = "GL_PIXEL_UNPACK_BUFFER";		break;
	case GL_COPY_READ_BUFFER:			t = "GL_COPY_READ_BUFFER";			break;
	case GL_COPY_WRITE_BUFFER:			t = "GL_COPY_WRITE_BUFFER";			break;
	case GL_TEXTURE_BUFFER:				t = "GL_TEXTURE_BUFFER";			break;
	case GL_UNIFORM_BUFFER:				t = "GL_UNIFORM_BUFFER";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (access){
	case GL_READ_ONLY:					a = "GL_READ_ONLY";					break;
	case GL_WRITE_ONLY:					a = "GL_WRITE_ONLY";				break;
	case GL_READ_WRITE:					a = "GL_READ_WRITE";				break;
	default:							a = Str_VarArgs("0x%08X", access);	break;
	}

	fprintf(qglState.logFile, "glMapBuffer( %s, %s )\n", t, a);
	return dllMapBuffer(target, access);
}

static GLvoid * APIENTRY logMapBufferRange (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access){

	const char	*t;

	switch (target){
	case GL_ARRAY_BUFFER:				t = "GL_ARRAY_BUFFER";				break;
	case GL_ELEMENT_ARRAY_BUFFER:		t = "GL_ELEMENT_ARRAY_BUFFER";		break;
	case GL_PIXEL_PACK_BUFFER:			t = "GL_PIXEL_PACK_BUFFER";			break;
	case GL_PIXEL_UNPACK_BUFFER:		t = "GL_PIXEL_UNPACK_BUFFER";		break;
	case GL_COPY_READ_BUFFER:			t = "GL_COPY_READ_BUFFER";			break;
	case GL_COPY_WRITE_BUFFER:			t = "GL_COPY_WRITE_BUFFER";			break;
	case GL_TEXTURE_BUFFER:				t = "GL_TEXTURE_BUFFER";			break;
	case GL_UNIFORM_BUFFER:				t = "GL_UNIFORM_BUFFER";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glMapBufferRange( %s, %i, %i, %u", t, offset, length, access);

	if (access){
		fprintf(qglState.logFile, " = ( ");

		if (access & GL_MAP_READ_BIT)
			fprintf(qglState.logFile, "GL_MAP_READ_BIT ");
		if (access & GL_MAP_WRITE_BIT)
			fprintf(qglState.logFile, "GL_MAP_WRITE_BIT ");
		if (access & GL_MAP_INVALIDATE_RANGE_BIT)
			fprintf(qglState.logFile, "GL_MAP_INVALIDATE_RANGE_BIT ");
		if (access & GL_MAP_INVALIDATE_BUFFER_BIT)
			fprintf(qglState.logFile, "GL_MAP_INVALIDATE_BUFFER_BIT ");
		if (access & GL_MAP_FLUSH_EXPLICIT_BIT)
			fprintf(qglState.logFile, "GL_MAP_FLUSH_EXPLICIT_BIT ");
		if (access & GL_MAP_UNSYNCHRONIZED_BIT)
			fprintf(qglState.logFile, "GL_MAP_UNSYNCHRONIZED_BIT ");

		fprintf(qglState.logFile, ")");
	}

	fprintf(qglState.logFile, " )\n");
	return dllMapBufferRange(target, offset, length, access);
}

static GLvoid APIENTRY logMapGrid1d (GLint un, GLdouble u1, GLdouble u2){

	fprintf(qglState.logFile, "glMapGrid1d( %i, %g, %g )\n", un, u1, u2);
	dllMapGrid1d(un, u1, u2);
}

static GLvoid APIENTRY logMapGrid1f (GLint un, GLfloat u1, GLfloat u2){

	fprintf(qglState.logFile, "glMapGrid1f( %i, %g, %g )\n", un, u1, u2);
	dllMapGrid1f(un, u1, u2);
}

static GLvoid APIENTRY logMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2){

	fprintf(qglState.logFile, "glMapGrid2d( %i, %g, %g, %i, %g, %g )\n", un, u1, u2, vn, v1, v2);
	dllMapGrid2d(un, u1, u2, vn, v1, v2);
}

static GLvoid APIENTRY logMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2){

	fprintf(qglState.logFile, "glMapGrid2f( %i, %g, %g, %i, %g, %g )\n", un, u1, u2, vn, v1, v2);
	dllMapGrid2f(un, u1, u2, vn, v1, v2);
}

static GLvoid APIENTRY logMaterialf (GLenum face, GLenum pname, GLfloat param){

	fprintf(qglState.logFile, "glMaterialf( 0x%08X, 0x%08X, %g )\n", face, pname, param);
	dllMaterialf(face, pname, param);
}

static GLvoid APIENTRY logMaterialfv (GLenum face, GLenum pname, const GLfloat *params){

	fprintf(qglState.logFile, "glMaterialfv( 0x%08X, 0x%08X, %p )\n", face, pname, params);
	dllMaterialfv(face, pname, params);
}

static GLvoid APIENTRY logMateriali (GLenum face, GLenum pname, GLint param){

	fprintf(qglState.logFile, "glMateriali( 0x%08X, 0x%08X, %i )\n", face, pname, param);
	dllMateriali(face, pname, param);
}

static GLvoid APIENTRY logMaterialiv (GLenum face, GLenum pname, const GLint *params){

	fprintf(qglState.logFile, "glMaterialiv( 0x%08X, 0x%08X, %p )\n", face, pname, params);
	dllMaterialiv(face, pname, params);
}

static GLvoid APIENTRY logMatrixMode (GLenum mode){

	const char	*m;

	switch (mode){
	case GL_PROJECTION:	m = "GL_PROJECTION";			break;
	case GL_MODELVIEW:	m = "GL_MODELVIEW";				break;
	case GL_TEXTURE:	m = "GL_TEXTURE";				break;
	default:			m = Str_VarArgs("0x%08X", mode);	break;
	}

	fprintf(qglState.logFile, "glMatrixMode( %s )\n", m);
	dllMatrixMode(mode);
}

static GLvoid APIENTRY logMultMatrixd (const GLdouble *m){

	fprintf(qglState.logFile, "glMultMatrixd( %p )\n", m);
	dllMultMatrixd(m);
}

static GLvoid APIENTRY logMultMatrixf (const GLfloat *m){

	fprintf(qglState.logFile, "glMultMatrixf( %p )\n", m);
	dllMultMatrixf(m);
}

static GLvoid APIENTRY logMultTransposeMatrixd (const GLdouble *m){

	fprintf(qglState.logFile, "glMultTransposeMatrixd( %p )\n", m);
	dllMultTransposeMatrixd(m);
}

static GLvoid APIENTRY logMultTransposeMatrixf (const GLfloat *m){

	fprintf(qglState.logFile, "glMultTransposeMatrixf( %p )\n", m);
	dllMultTransposeMatrixf(m);
}

static GLvoid APIENTRY logMultiDrawArrays (GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount){

	const char	*m;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	fprintf(qglState.logFile, "glMultiDrawArrays( %s, %p, %p, %i )\n", m, first, count, primcount);
	dllMultiDrawArrays(mode, first, count, primcount);
}

static GLvoid APIENTRY logMultiDrawElements (GLenum mode, const GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount){

	const char	*m, *t;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	switch (type){
	case GL_UNSIGNED_BYTE:				t = "GL_UNSIGNED_BYTE";				break;
	case GL_UNSIGNED_SHORT:				t = "GL_UNSIGNED_SHORT";			break;
	case GL_UNSIGNED_INT:				t = "GL_UNSIGNED_INT";				break;
	default:							t = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glMultiDrawElements( %s, %p, %s, %p, %i )\n", m, count, t, indices, primcount);
	dllMultiDrawElements(mode, count, type, indices, primcount);
}

static GLvoid APIENTRY logMultiDrawElementsBaseVertex (GLenum mode, const GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount, const GLint *basevertex){

	const char	*m, *t;

	switch (mode){
	case GL_POINTS:						m = "GL_POINTS";					break;
	case GL_LINES:						m = "GL_LINES";						break;
	case GL_LINES_ADJACENCY:			m = "GL_LINES_ADJACENCY";			break;
	case GL_LINE_STRIP:					m = "GL_LINE_STRIP";				break;
	case GL_LINE_STRIP_ADJACENCY:		m = "GL_LINE_STRIP_ADJACENCY";		break;
	case GL_LINE_LOOP:					m = "GL_LINE_LOOP";					break;
	case GL_TRIANGLES:					m = "GL_TRIANGLES";					break;
	case GL_TRIANGLES_ADJACENCY:		m = "GL_TRIANGLES_ADJACENCY";		break;
	case GL_TRIANGLE_STRIP:				m = "GL_TRIANGLE_STRIP";			break;
	case GL_TRIANGLE_STRIP_ADJACENCY:	m = "GL_TRIANGLE_STRIP_ADJACENCY";	break;
	case GL_TRIANGLE_FAN:				m = "GL_TRIANGLE_FAN";				break;
	case GL_QUADS:						m = "GL_QUADS";						break;
	case GL_QUAD_STRIP:					m = "GL_QUAD_STRIP";				break;
	case GL_POLYGON:					m = "GL_POLYGON";					break;
	default:							m = Str_VarArgs("0x%08X", mode);		break;
	}

	switch (type){
	case GL_UNSIGNED_BYTE:				t = "GL_UNSIGNED_BYTE";				break;
	case GL_UNSIGNED_SHORT:				t = "GL_UNSIGNED_SHORT";			break;
	case GL_UNSIGNED_INT:				t = "GL_UNSIGNED_INT";				break;
	default:							t = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glMultiDrawElementsBaseVertex( %s, %p, %s, %p, %i, %p )\n", m, count, t, indices, primcount, basevertex);
	dllMultiDrawElementsBaseVertex(mode, count, type, indices, primcount, basevertex);
}

static GLvoid APIENTRY logMultiTexCoord1d (GLenum target, GLdouble s){

	fprintf(qglState.logFile, "glMultiTexCoord1d( 0x%08X, %g )\n", target, s);
	dllMultiTexCoord1d(target, s);
}

static GLvoid APIENTRY logMultiTexCoord1dv (GLenum target, const GLdouble *v){

	fprintf(qglState.logFile, "glMultiTexCoord1dv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord1dv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord1f (GLenum target, GLfloat s){

	fprintf(qglState.logFile, "glMultiTexCoord1f( 0x%08X, %g )\n", target, s);
	dllMultiTexCoord1f(target, s);
}

static GLvoid APIENTRY logMultiTexCoord1fv (GLenum target, const GLfloat *v){

	fprintf(qglState.logFile, "glMultiTexCoord1fv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord1fv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord1i (GLenum target, GLint s){

	fprintf(qglState.logFile, "glMultiTexCoord1i( 0x%08X, %i )\n", target, s);
	dllMultiTexCoord1i(target, s);
}

static GLvoid APIENTRY logMultiTexCoord1iv (GLenum target, const GLint *v){

	fprintf(qglState.logFile, "glMultiTexCoord1iv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord1iv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord1s (GLenum target, GLshort s){

	fprintf(qglState.logFile, "glMultiTexCoord1s( 0x%08X, %i )\n", target, s);
	dllMultiTexCoord1s(target, s);
}

static GLvoid APIENTRY logMultiTexCoord1sv (GLenum target, const GLshort *v){

	fprintf(qglState.logFile, "glMultiTexCoord1sv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord1sv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord2d (GLenum target, GLdouble s, GLdouble t){

	fprintf(qglState.logFile, "glMultiTexCoord2d( 0x%08X, %g, %g )\n", target, s, t);
	dllMultiTexCoord2d(target, s, t);
}

static GLvoid APIENTRY logMultiTexCoord2dv (GLenum target, const GLdouble *v){

	fprintf(qglState.logFile, "glMultiTexCoord2dv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord2dv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord2f (GLenum target, GLfloat s, GLfloat t){

	fprintf(qglState.logFile, "glMultiTexCoord2f( 0x%08X, %g, %g )\n", target, s, t);
	dllMultiTexCoord2f(target, s, t);
}

static GLvoid APIENTRY logMultiTexCoord2fv (GLenum target, const GLfloat *v){

	fprintf(qglState.logFile, "glMultiTexCoord2fv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord2fv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord2i (GLenum target, GLint s, GLint t){

	fprintf(qglState.logFile, "glMultiTexCoord2i( 0x%08X, %i, %i )\n", target, s, t);
	dllMultiTexCoord2i(target, s, t);
}

static GLvoid APIENTRY logMultiTexCoord2iv (GLenum target, const GLint *v){

	fprintf(qglState.logFile, "glMultiTexCoord2iv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord2iv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord2s (GLenum target, GLshort s, GLshort t){

	fprintf(qglState.logFile, "glMultiTexCoord2s( 0x%08X, %i, %i )\n", target, s, t);
	dllMultiTexCoord2s(target, s, t);
}

static GLvoid APIENTRY logMultiTexCoord2sv (GLenum target, const GLshort *v){

	fprintf(qglState.logFile, "glMultiTexCoord2sv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord2sv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord3d (GLenum target, GLdouble s, GLdouble t, GLdouble r){

	fprintf(qglState.logFile, "glMultiTexCoord3d( 0x%08X, %g, %g, %g )\n", target, s, t, r);
	dllMultiTexCoord3d(target, s, t, r);
}

static GLvoid APIENTRY logMultiTexCoord3dv (GLenum target, const GLdouble *v){

	fprintf(qglState.logFile, "glMultiTexCoord3dv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord3dv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord3f (GLenum target, GLfloat s, GLfloat t, GLfloat r){

	fprintf(qglState.logFile, "glMultiTexCoord3f( 0x%08X, %g, %g, %g )\n", target, s, t, r);
	dllMultiTexCoord3f(target, s, t, r);
}

static GLvoid APIENTRY logMultiTexCoord3fv (GLenum target, const GLfloat *v){

	fprintf(qglState.logFile, "glMultiTexCoord3fv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord3fv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord3i (GLenum target, GLint s, GLint t, GLint r){

	fprintf(qglState.logFile, "glMultiTexCoord3i( 0x%08X, %i, %i, %i )\n", target, s, t, r);
	dllMultiTexCoord3i(target, s, t, r);
}

static GLvoid APIENTRY logMultiTexCoord3iv (GLenum target, const GLint *v){

	fprintf(qglState.logFile, "glMultiTexCoord3iv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord3iv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord3s (GLenum target, GLshort s, GLshort t, GLshort r){

	fprintf(qglState.logFile, "glMultiTexCoord3s( 0x%08X, %i, %i, %i )\n", target, s, t, r);
	dllMultiTexCoord3s(target, s, t, r);
}

static GLvoid APIENTRY logMultiTexCoord3sv (GLenum target, const GLshort *v){

	fprintf(qglState.logFile, "glMultiTexCoord3sv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord3sv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord4d (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q){

	fprintf(qglState.logFile, "glMultiTexCoord4d( 0x%08X, %g, %g, %g, %g )\n", target, s, t, r, q);
	dllMultiTexCoord4d(target, s, t, r, q);
}

static GLvoid APIENTRY logMultiTexCoord4dv (GLenum target, const GLdouble *v){

	fprintf(qglState.logFile, "glMultiTexCoord4dv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord4dv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord4f (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q){

	fprintf(qglState.logFile, "glMultiTexCoord4f( 0x%08X, %g, %g, %g, %g )\n", target, s, t, r, q);
	dllMultiTexCoord4f(target, s, t, r, q);
}

static GLvoid APIENTRY logMultiTexCoord4fv (GLenum target, const GLfloat *v){

	fprintf(qglState.logFile, "glMultiTexCoord4fv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord4fv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord4i (GLenum target, GLint s, GLint t, GLint r, GLint q){

	fprintf(qglState.logFile, "glMultiTexCoord4i( 0x%08X, %i, %i, %i, %i )\n", target, s, t, r, q);
	dllMultiTexCoord4i(target, s, t, r, q);
}

static GLvoid APIENTRY logMultiTexCoord4iv (GLenum target, const GLint *v){

	fprintf(qglState.logFile, "glMultiTexCoord4iv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord4iv(target, v);
}

static GLvoid APIENTRY logMultiTexCoord4s (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q){

	fprintf(qglState.logFile, "glMultiTexCoord4s( 0x%08X, %i, %i, %i, %i )\n", target, s, t, r, q);
	dllMultiTexCoord4s(target, s, t, r, q);
}

static GLvoid APIENTRY logMultiTexCoord4sv (GLenum target, const GLshort *v){

	fprintf(qglState.logFile, "glMultiTexCoord4sv( 0x%08X, %p )\n", target, v);
	dllMultiTexCoord4sv(target, v);
}

static GLvoid APIENTRY logMultiTexCoordP1ui (GLenum target, GLenum type, GLuint v){

	fprintf(qglState.logFile, "glMultiTexCoordP1ui( 0x%08X, 0x%08X, %u )\n", target, type, v);
	dllMultiTexCoordP1ui(target, type, v);
}

static GLvoid APIENTRY logMultiTexCoordP1uiv (GLenum target, GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glMultiTexCoordP1uiv( 0x%08X, 0x%08X, %p )\n", target, type, v);
	dllMultiTexCoordP1uiv(target, type, v);
}

static GLvoid APIENTRY logMultiTexCoordP2ui (GLenum target, GLenum type, GLuint v){

	fprintf(qglState.logFile, "glMultiTexCoordP2ui( 0x%08X, 0x%08X, %u )\n", target, type, v);
	dllMultiTexCoordP2ui(target, type, v);
}

static GLvoid APIENTRY logMultiTexCoordP2uiv (GLenum target, GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glMultiTexCoordP2uiv( 0x%08X, 0x%08X, %p )\n", target, type, v);
	dllMultiTexCoordP2uiv(target, type, v);
}

static GLvoid APIENTRY logMultiTexCoordP3ui (GLenum target, GLenum type, GLuint v){

	fprintf(qglState.logFile, "glMultiTexCoordP3ui( 0x%08X, 0x%08X, %u )\n", target, type, v);
	dllMultiTexCoordP3ui(target, type, v);
}

static GLvoid APIENTRY logMultiTexCoordP3uiv (GLenum target, GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glMultiTexCoordP3uiv( 0x%08X, 0x%08X, %p )\n", target, type, v);
	dllMultiTexCoordP3uiv(target, type, v);
}

static GLvoid APIENTRY logMultiTexCoordP4ui (GLenum target, GLenum type, GLuint v){

	fprintf(qglState.logFile, "glMultiTexCoordP4ui( 0x%08X, 0x%08X, %u )\n", target, type, v);
	dllMultiTexCoordP4ui(target, type, v);
}

static GLvoid APIENTRY logMultiTexCoordP4uiv (GLenum target, GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glMultiTexCoordP4uiv( 0x%08X, 0x%08X, %p )\n", target, type, v);
	dllMultiTexCoordP4uiv(target, type, v);
}

static GLvoid APIENTRY logNewList (GLuint list, GLenum mode){

	fprintf(qglState.logFile, "glNewList( %u, 0x%08X )\n", list, mode);
	dllNewList(list, mode);
}

static GLvoid APIENTRY logNormal3b (GLbyte nx, GLbyte ny, GLbyte nz){

	fprintf(qglState.logFile, "glNormal3b( %i, %i, %i )\n", nx, ny, nz);
	dllNormal3b(nx, ny, nz);
}

static GLvoid APIENTRY logNormal3bv (const GLbyte *v){

	fprintf(qglState.logFile, "glNormal3bv( %p )\n", v);
	dllNormal3bv(v);
}

static GLvoid APIENTRY logNormal3d (GLdouble nx, GLdouble ny, GLdouble nz){

	fprintf(qglState.logFile, "glNormal3d( %g, %g, %g )\n", nx, ny, nz);
	dllNormal3d(nx, ny, nz);
}

static GLvoid APIENTRY logNormal3dv (const GLdouble *v){

	fprintf(qglState.logFile, "glNormal3dv( %p )\n", v);
	dllNormal3dv(v);
}

static GLvoid APIENTRY logNormal3f (GLfloat nx, GLfloat ny, GLfloat nz){

	fprintf(qglState.logFile, "glNormal3f( %g, %g, %g )\n", nx, ny, nz);
	dllNormal3f(nx, ny, nz);
}

static GLvoid APIENTRY logNormal3fv (const GLfloat *v){

	fprintf(qglState.logFile, "glNormal3fv( %p )\n", v);
	dllNormal3fv(v);
}

static GLvoid APIENTRY logNormal3i (GLint nx, GLint ny, GLint nz){

	fprintf(qglState.logFile, "glNormal3i( %i, %i, %i )\n", nx, ny, nz);
	dllNormal3i(nx, ny, nz);
}

static GLvoid APIENTRY logNormal3iv (const GLint *v){

	fprintf(qglState.logFile, "glNormal3iv( %p )\n", v);
	dllNormal3iv(v);
}

static GLvoid APIENTRY logNormal3s (GLshort nx, GLshort ny, GLshort nz){

	fprintf(qglState.logFile, "glNormal3s( %i, %i, %i )\n", nx, ny, nz);
	dllNormal3s(nx, ny, nz);
}

static GLvoid APIENTRY logNormal3sv (const GLshort *v){

	fprintf(qglState.logFile, "glNormal3sv( %p )\n", v);
	dllNormal3sv(v);
}

static GLvoid APIENTRY logNormalP3ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glNormalP3ui( 0x%08X, %u )\n", type, v);
	dllNormalP3ui(type, v);
}

static GLvoid APIENTRY logNormalP3uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glNormalP3uiv( 0x%08X, %p )\n", type, v);
	dllNormalP3uiv(type, v);
}

static GLvoid APIENTRY logNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer){

	const char	*t;

	switch (type){
	case GL_BYTE:		t = "GL_BYTE";					break;
	case GL_SHORT:		t = "GL_SHORT";					break;
	case GL_INT:		t = "GL_INT";					break;
	case GL_FLOAT:		t = "GL_FLOAT";					break;
	case GL_HALF_FLOAT:	t = "GL_HALF_FLOAT";			break;
	case GL_DOUBLE:		t = "GL_DOUBLE";				break;
	default:			t = Str_VarArgs("0x%08X", type);	break;
	}

	fprintf(qglState.logFile, "glNormalPointer( %s, %i, %p )\n", t, stride, pointer);
	dllNormalPointer(type, stride, pointer);
}

static GLvoid APIENTRY logOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar){

	fprintf(qglState.logFile, "glOrtho( %g, %g, %g, %g, %g, %g )\n", left, right, bottom, top, zNear, zFar);
	dllOrtho(left, right, bottom, top, zNear, zFar);
}

static GLvoid APIENTRY logPassThrough (GLfloat token){

	fprintf(qglState.logFile, "glPassThrough( %g )\n", token);
	dllPassThrough(token);
}

static GLvoid APIENTRY logPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values){

	fprintf(qglState.logFile, "glPixelMapfv( 0x%08X, %i, %p )\n", map, mapsize, values);
	dllPixelMapfv(map, mapsize, values);
}

static GLvoid APIENTRY logPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values){

	fprintf(qglState.logFile, "glPixelMapuiv( 0x%08X, %i, %p )\n", map, mapsize, values);
	dllPixelMapuiv(map, mapsize, values);
}

static GLvoid APIENTRY logPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values){

	fprintf(qglState.logFile, "glPixelMapusv( 0x%08X, %i, %p )\n", map, mapsize, values);
	dllPixelMapusv(map, mapsize, values);
}

static GLvoid APIENTRY logPixelStoref (GLenum pname, GLfloat param){

	fprintf(qglState.logFile, "glPixelStoref( 0x%08X, %g )\n", pname, param);
	dllPixelStoref(pname, param);
}

static GLvoid APIENTRY logPixelStorei (GLenum pname, GLint param){

	fprintf(qglState.logFile, "glPixelStorei( 0x%08X, %i )\n", pname, param);
	dllPixelStorei(pname, param);
}

static GLvoid APIENTRY logPixelTransferf (GLenum pname, GLfloat param){

	fprintf(qglState.logFile, "glPixelTransferf( 0x%08X, %g )\n", pname, param);
	dllPixelTransferf(pname, param);
}

static GLvoid APIENTRY logPixelTransferi (GLenum pname, GLint param){

	fprintf(qglState.logFile, "glPixelTransferi( 0x%08X, %i )\n", pname, param);
	dllPixelTransferi(pname, param);
}

static GLvoid APIENTRY logPixelZoom (GLfloat xfactor, GLfloat yfactor){

	fprintf(qglState.logFile, "glPixelZoom( %g, %g )\n", xfactor, yfactor);
	dllPixelZoom(xfactor, yfactor);
}

static GLvoid APIENTRY logPointParameterf (GLenum pname, GLfloat param){

	fprintf(qglState.logFile, "glPointParameterf( 0x%08X, %g )\n", pname, param);
	dllPointParameterf(pname, param);
}

static GLvoid APIENTRY logPointParameterfv (GLenum pname, const GLfloat *params){

	fprintf(qglState.logFile, "glPointParameterfv( 0x%08X, %p )\n", pname, params);
	dllPointParameterfv(pname, params);
}

static GLvoid APIENTRY logPointParameteri (GLenum pname, GLint param){

	fprintf(qglState.logFile, "glPointParameteri( 0x%08X, %i )\n", pname, param);
	dllPointParameteri(pname, param);
}

static GLvoid APIENTRY logPointParameteriv (GLenum pname, const GLint *params){

	fprintf(qglState.logFile, "glPointParameteriv( 0x%08X, %p )\n", pname, params);
	dllPointParameteriv(pname, params);
}

static GLvoid APIENTRY logPointSize (GLfloat size){

	fprintf(qglState.logFile, "glPointSize( %g )\n", size);
	dllPointSize(size);
}

static GLvoid APIENTRY logPolygonMode (GLenum face, GLenum mode){

	const char	*f, *m;

	switch (face){
	case GL_FRONT:			f = "GL_FRONT";					break;
	case GL_BACK:			f = "GL_BACK";					break;
	case GL_FRONT_AND_BACK:	f = "GL_FRONT_AND_BACK";		break;
	default:				f = Str_VarArgs("0x%08X", face);	break;
	}

	switch (mode){
	case GL_POINT:			m = "GL_POINT";					break;
	case GL_LINE:			m = "GL_LINE";					break;
	case GL_FILL:			m = "GL_FILL";					break;
	default:				m = Str_VarArgs("0x%08X", mode);	break;
	}

	fprintf(qglState.logFile, "glPolygonMode( %s, %s )\n", f, m);
	dllPolygonMode(face, mode);
}

static GLvoid APIENTRY logPolygonOffset (GLfloat factor, GLfloat units){

	fprintf(qglState.logFile, "glPolygonOffset( %g, %g )\n", factor, units);
	dllPolygonOffset(factor, units);
}

static GLvoid APIENTRY logPolygonStipple (const GLubyte *mask){

	fprintf(qglState.logFile, "glPolygonStipple( %p )\n", mask);
	dllPolygonStipple(mask);
}

static GLvoid APIENTRY logPopAttrib (GLvoid){

	fprintf(qglState.logFile, "glPopAttrib()\n");
	dllPopAttrib();
}

static GLvoid APIENTRY logPopClientAttrib (GLvoid){

	fprintf(qglState.logFile, "glPopClientAttrib()\n");
	dllPopClientAttrib();
}

static GLvoid APIENTRY logPopMatrix (GLvoid){

	fprintf(qglState.logFile, "glPopMatrix()\n");
	dllPopMatrix();
}

static GLvoid APIENTRY logPopName (GLvoid){

	fprintf(qglState.logFile, "glPopName()\n");
	dllPopName();
}

static GLvoid APIENTRY logPrimitiveRestartIndex (GLuint index){

	fprintf(qglState.logFile, "glPrimitivRestartIndex( %u )\n", index);
	dllPrimitiveRestartIndex(index);
}

static GLvoid APIENTRY logPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities){

	fprintf(qglState.logFile, "glPrioritizeTextures( %i, %p, %p )\n", n, textures, priorities);
	dllPrioritizeTextures(n, textures, priorities);
}

static GLvoid APIENTRY logProvokingVertex (GLenum mode){

	fprintf(qglState.logFile, "glProvokingVertex( 0x%08X )\n", mode);
	dllProvokingVertex(mode);
}

static GLvoid APIENTRY logPushAttrib (GLbitfield mask){

	fprintf(qglState.logFile, "glPushAttrib( %u )\n", mask);
	dllPushAttrib(mask);
}

static GLvoid APIENTRY logPushClientAttrib (GLbitfield mask){

	fprintf(qglState.logFile, "glPushClientAttrib( %u )\n", mask);
	dllPushClientAttrib(mask);
}

static GLvoid APIENTRY logPushMatrix (GLvoid){

	fprintf(qglState.logFile, "glPushMatrix()\n");
	dllPushMatrix();
}

static GLvoid APIENTRY logPushName (GLuint name){

	fprintf(qglState.logFile, "glPushName( %u )\n", name);
	dllPushName(name);
}

static GLvoid APIENTRY logQueryCounter (GLuint id, GLenum target){

	const char	*t;

	switch (target){
	case GL_TIMESTAMP:	t = "GL_TIMESTAMP";					break;
	default:			t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glQueryCounter( %u, %s )\n", id, t);
	dllQueryCounter(id, target);
}

static GLvoid APIENTRY logRasterPos2d (GLdouble x, GLdouble y){

	fprintf(qglState.logFile, "glRasterPot2d( %g, %g )\n", x, y);
	dllRasterPos2d(x, y);
}

static GLvoid APIENTRY logRasterPos2dv (const GLdouble *v){

	fprintf(qglState.logFile, "glRasterPos2dv( %p )\n", v);
	dllRasterPos2dv(v);
}

static GLvoid APIENTRY logRasterPos2f (GLfloat x, GLfloat y){

	fprintf(qglState.logFile, "glRasterPos2f( %g, %g )\n", x, y);
	dllRasterPos2f(x, y);
}

static GLvoid APIENTRY logRasterPos2fv (const GLfloat *v){

	fprintf(qglState.logFile, "glRasterPos2dv( %p )\n", v);
	dllRasterPos2fv(v);
}

static GLvoid APIENTRY logRasterPos2i (GLint x, GLint y){

	fprintf(qglState.logFile, "glRasterPos2if( %i, %i )\n", x, y);
	dllRasterPos2i(x, y);
}

static GLvoid APIENTRY logRasterPos2iv (const GLint *v){

	fprintf(qglState.logFile, "glRasterPos2iv( %p )\n", v);
	dllRasterPos2iv(v);
}

static GLvoid APIENTRY logRasterPos2s (GLshort x, GLshort y){

	fprintf(qglState.logFile, "glRasterPos2s( %i, %i )\n", x, y);
	dllRasterPos2s(x, y);
}

static GLvoid APIENTRY logRasterPos2sv (const GLshort *v){

	fprintf(qglState.logFile, "glRasterPos2sv( %p )\n", v);
	dllRasterPos2sv(v);
}

static GLvoid APIENTRY logRasterPos3d (GLdouble x, GLdouble y, GLdouble z){

	fprintf(qglState.logFile, "glRasterPos3d( %g, %g, %g )\n", x, y, z);
	dllRasterPos3d(x, y, z);
}

static GLvoid APIENTRY logRasterPos3dv (const GLdouble *v){

	fprintf(qglState.logFile, "glRasterPos3dv( %p )\n", v);
	dllRasterPos3dv(v);
}

static GLvoid APIENTRY logRasterPos3f (GLfloat x, GLfloat y, GLfloat z){

	fprintf(qglState.logFile, "glRasterPos3f( %g, %g, %g )\n", x, y, z);
	dllRasterPos3f(x, y, z);
}

static GLvoid APIENTRY logRasterPos3fv (const GLfloat *v){

	fprintf(qglState.logFile, "glRasterPos3fv( %p )\n", v);
	dllRasterPos3fv(v);
}

static GLvoid APIENTRY logRasterPos3i (GLint x, GLint y, GLint z){

	fprintf(qglState.logFile, "glRasterPos3i( %i, %i, %i )\n", x, y, z);
	dllRasterPos3i(x, y, z);
}

static GLvoid APIENTRY logRasterPos3iv (const GLint *v){

	fprintf(qglState.logFile, "glRasterPos3iv( %p )\n", v);
	dllRasterPos3iv(v);
}

static GLvoid APIENTRY logRasterPos3s (GLshort x, GLshort y, GLshort z){

	fprintf(qglState.logFile, "glRasterPos3s( %i, %i, %i )\n", x, y, z);
	dllRasterPos3s(x, y, z);
}

static GLvoid APIENTRY logRasterPos3sv (const GLshort *v){

	fprintf(qglState.logFile, "glRasterPos3sv( %p )\n", v);
	dllRasterPos3sv(v);
}

static GLvoid APIENTRY logRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w){

	fprintf(qglState.logFile, "glRasterPos4d( %g, %g, %g, %g )\n", x, y, z, w);
	dllRasterPos4d(x, y, z, w);
}

static GLvoid APIENTRY logRasterPos4dv (const GLdouble *v){

	fprintf(qglState.logFile, "glRasterPos4dv( %p )\n", v);
	dllRasterPos4dv(v);
}

static GLvoid APIENTRY logRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w){

	fprintf(qglState.logFile, "glRasterPos4f( %g, %g, %g, %g )\n", x, y, z, w);
	dllRasterPos4f(x, y, z, w);
}

static GLvoid APIENTRY logRasterPos4fv (const GLfloat *v){

	fprintf(qglState.logFile, "glRasterPos4fv( %p )\n", v);
	dllRasterPos4fv(v);
}

static GLvoid APIENTRY logRasterPos4i (GLint x, GLint y, GLint z, GLint w){

	fprintf(qglState.logFile, "glRasterPos4i( %i, %i, %i, %i )\n", x, y, z, w);
	dllRasterPos4i(x, y, z, w);
}

static GLvoid APIENTRY logRasterPos4iv (const GLint *v){

	fprintf(qglState.logFile, "glRasterPos4iv( %p )\n", v);
	dllRasterPos4iv(v);
}

static GLvoid APIENTRY logRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w){

	fprintf(qglState.logFile, "glRasterPos4s( %i, %i, %i, %i )\n", x, y, z, w);
	dllRasterPos4s(x, y, z, w);
}

static GLvoid APIENTRY logRasterPos4sv (const GLshort *v){

	fprintf(qglState.logFile, "glRasterPos4sv( %p )\n", v);
	dllRasterPos4sv(v);
}

static GLvoid APIENTRY logReadBuffer (GLenum buffer){

	const char	*b;

	switch (buffer){
	case GL_FRONT_LEFT:		b = "GL_FRONT_LEFT";				break;
	case GL_FRONT_RIGHT:	b = "GL_FRONT_RIGHT";				break;
	case GL_BACK_LEFT:		b = "GL_BACK_LEFT";					break;
	case GL_BACK_RIGHT:		b = "GL_BACK_RIGHT";				break;
	case GL_FRONT:			b = "GL_FRONT";						break;
	case GL_BACK:			b = "GL_BACK";						break;
	case GL_LEFT:			b = "GL_LEFT";						break;
	case GL_RIGHT:			b = "GL_RIGHT";						break;
	default:				b = Str_VarArgs("0x%08X", buffer);	break;
	}

	if (buffer >= GL_COLOR_ATTACHMENT0 && buffer <= GL_COLOR_ATTACHMENT15)
		b = Str_VarArgs("GL_COLOR_ATTACHMENT%i", buffer - GL_COLOR_ATTACHMENT0);

	fprintf(qglState.logFile, "glReadBuffer( %s )\n", b);
	dllReadBuffer(buffer);
}

static GLvoid APIENTRY logReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels){

	const char	*f, *t;

	switch (format){
	case GL_RGB:				f = "GL_RGB";						break;
	case GL_RGBA:				f = "GL_RGBA";						break;
	case GL_BGR:				f = "GL_BGR";						break;
	case GL_BGRA:				f = "GL_BGRA";						break;
	case GL_DEPTH_COMPONENT:	f = "GL_DEPTH_COMPONENT";			break;
	case GL_DEPTH_STENCIL:		f = "GL_DEPTH_STENCIL";				break;
	default:					f = Str_VarArgs("0x%08X", format);	break;
	}

	switch (type){
	case GL_BYTE:				t = "GL_BYTE";						break;
	case GL_UNSIGNED_BYTE:		t = "GL_UNSIGNED_BYTE";				break;
	case GL_SHORT:				t = "GL_SHORT";						break;
	case GL_UNSIGNED_SHORT:		t = "GL_UNSIGNED_SHORT";			break;
	case GL_INT:				t = "GL_INT";						break;
	case GL_UNSIGNED_INT:		t = "GL_UNSIGNED_INT";				break;
	case GL_UNSIGNED_INT_24_8:	t = "GL_UNSIGNED_INT_24_8";			break;
	case GL_FLOAT:				t = "GL_FLOAT";						break;
	case GL_HALF_FLOAT:			t = "GL_HALF_FLOAT";				break;
	case GL_BITMAP:				t = "GL_BITMAP";					break;
	default:					t = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glReadPixels( %i, %i, %i, %i, %s, %s, %p )\n", x, y, width, height, f, t, pixels);
	dllReadPixels(x, y, width, height, format, type, pixels);
}

static GLvoid APIENTRY logRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2){

	fprintf(qglState.logFile, "glRectd( %g, %g, %g, %g )\n", x1, y1, x2, y2);
	dllRectd(x1, y1, x2, y2);
}

static GLvoid APIENTRY logRectdv (const GLdouble *v1, const GLdouble *v2){

	fprintf(qglState.logFile, "glRectdv( %p, %p )\n", v1, v2);
	dllRectdv(v1, v2);
}

static GLvoid APIENTRY logRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2){

	fprintf(qglState.logFile, "glRectf( %g, %g, %g, %g )\n", x1, y1, x2, y2);
	dllRectf(x1, y1, x2, y2);
}

static GLvoid APIENTRY logRectfv (const GLfloat *v1, const GLfloat *v2){

	fprintf(qglState.logFile, "glRectfv( %p, %p )\n", v1, v2);
	dllRectfv(v1, v2);
}

static GLvoid APIENTRY logRecti (GLint x1, GLint y1, GLint x2, GLint y2){

	fprintf(qglState.logFile, "glRecti( %i, %i, %i, %i )\n", x1, y1, x2, y2);
	dllRecti(x1, y1, x2, y2);
}

static GLvoid APIENTRY logRectiv (const GLint *v1, const GLint *v2){

	fprintf(qglState.logFile, "glRectiv( %p, %p )\n", v1, v2);
	dllRectiv(v1, v2);
}

static GLvoid APIENTRY logRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2){

	fprintf(qglState.logFile, "glRects( %i, %i, %i, %i )\n", x1, y1, x2, y2);
	dllRects(x1, y1, x2, y2);
}

static GLvoid APIENTRY logRectsv (const GLshort *v1, const GLshort *v2){

	fprintf(qglState.logFile, "glRectsv( %p, %p )\n", v1, v2);
	dllRectsv(v1, v2);
}

static GLint APIENTRY logRenderMode (GLenum mode){

	fprintf(qglState.logFile, "glRenderMode( 0x%08X )\n", mode);
	return dllRenderMode(mode);
}

static GLvoid APIENTRY logRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height){

	const char	*t, *i;

	switch (target){
	case GL_RENDERBUFFER:		t = "GL_RENDERBUFFER";						break;
	default:					t = Str_VarArgs("0x%08X", target);			break;
	}

	switch (internalformat){
	case GL_R8:					i = "GL_R8";								break;
	case GL_R16F:				i = "GL_R16F";								break;
	case GL_R32F:				i = "GL_R32F";								break;
	case GL_RG8:				i = "GL_RG8";								break;
	case GL_RG16F:				i = "GL_RG16F";								break;
	case GL_RG32F:				i = "GL_RG32F";								break;
	case GL_RGB8:				i = "GL_RGB8";								break;
	case GL_RGB16F:				i = "GL_RGB16F";							break;
	case GL_RGB32F:				i = "GL_RGB32F";							break;
	case GL_RGBA8:				i = "GL_RGBA8";								break;
	case GL_RGBA16F:			i = "GL_RGBA16F";							break;
	case GL_RGBA32F:			i = "GL_RGBA32F";							break;
	case GL_DEPTH_COMPONENT16:	i = "GL_DEPTH_COMPONENT16";					break;
	case GL_DEPTH_COMPONENT24:	i = "GL_DEPTH_COMPONENT24";					break;
	case GL_DEPTH_COMPONENT32:	i = "GL_DEPTH_COMPONENT32";					break;
	case GL_DEPTH_COMPONENT32F:	i = "GL_DEPTH_COMPONENT32F";				break;
	case GL_STENCIL_INDEX8:		i = "GL_STENCIL_INDEX8";					break;
	case GL_DEPTH24_STENCIL8:	i = "GL_DEPTH24_STENCIL8";					break;
	case GL_DEPTH32F_STENCIL8:	i = "GL_DEPTH32F_STENCIL8";					break;
	default:					i = Str_VarArgs("0x%08X", internalformat);	break;
	}

	fprintf(qglState.logFile, "glRenderbufferStorage( %s, %s, %i, %i )\n", t, i, width, height);
	dllRenderbufferStorage(target, internalformat, width, height);
}

static GLvoid APIENTRY logRenderbufferStorageMultisample (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height){

	const char	*t, *i;

	switch (target){
	case GL_RENDERBUFFER:		t = "GL_RENDERBUFFER";						break;
	default:					t = Str_VarArgs("0x%08X", target);			break;
	}

	switch (internalformat){
	case GL_R8:					i = "GL_R8";								break;
	case GL_R16F:				i = "GL_R16F";								break;
	case GL_R32F:				i = "GL_R32F";								break;
	case GL_RG8:				i = "GL_RG8";								break;
	case GL_RG16F:				i = "GL_RG16F";								break;
	case GL_RG32F:				i = "GL_RG32F";								break;
	case GL_RGB8:				i = "GL_RGB8";								break;
	case GL_RGB16F:				i = "GL_RGB16F";							break;
	case GL_RGB32F:				i = "GL_RGB32F";							break;
	case GL_RGBA8:				i = "GL_RGBA8";								break;
	case GL_RGBA16F:			i = "GL_RGBA16F";							break;
	case GL_RGBA32F:			i = "GL_RGBA32F";							break;
	case GL_DEPTH_COMPONENT16:	i = "GL_DEPTH_COMPONENT16";					break;
	case GL_DEPTH_COMPONENT24:	i = "GL_DEPTH_COMPONENT24";					break;
	case GL_DEPTH_COMPONENT32:	i = "GL_DEPTH_COMPONENT32";					break;
	case GL_DEPTH_COMPONENT32F:	i = "GL_DEPTH_COMPONENT32F";				break;
	case GL_STENCIL_INDEX8:		i = "GL_STENCIL_INDEX8";					break;
	case GL_DEPTH24_STENCIL8:	i = "GL_DEPTH24_STENCIL8";					break;
	case GL_DEPTH32F_STENCIL8:	i = "GL_DEPTH32F_STENCIL8";					break;
	default:					i = Str_VarArgs("0x%08X", internalformat);	break;
	}

	fprintf(qglState.logFile, "glRenderbufferStorageMultisample( %s, %i, %s, %i, %i )\n", t, samples, i, width, height);
	dllRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}

static GLvoid APIENTRY logRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z){

	fprintf(qglState.logFile, "glRotated( %g, %g, %g, %g )\n", angle, x, y, z);
	dllRotated(angle, x, y, z);
}

static GLvoid APIENTRY logRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z){

	fprintf(qglState.logFile, "glRotatef( %g, %g, %g, %g )\n", angle, x, y, z);
	dllRotatef(angle, x, y, z);
}

static GLvoid APIENTRY logSampleCoverage (GLclampf value, GLboolean invert){

	const char	*i;

	switch (invert){
	case GL_FALSE:	i = "GL_FALSE";						break;
	case GL_TRUE:	i = "GL_TRUE";						break;
	default:		i = Str_VarArgs("0x%08X", invert);	break;
	}

	fprintf(qglState.logFile, "glSampleCoverage( %g, %s )\n", value, i);
	dllSampleCoverage(value, invert);
}

static GLvoid APIENTRY logSampleMaski (GLuint index, GLbitfield mask){

	fprintf(qglState.logFile, "glSampleMaski( %u, %u )\n", index, mask);
	dllSampleMaski(index, mask);
}

static GLvoid APIENTRY logSamplerParameterIiv (GLuint sampler, GLenum pname, const GLint *params){

	const char	*n;

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	fprintf(qglState.logFile, "glSamplerParameterIiv( %u, %s, %p )\n", sampler, n, params);
	dllSamplerParameterIiv(sampler, pname, params);
}

static GLvoid APIENTRY logSamplerParameterIuiv (GLuint sampler, GLenum pname, const GLuint *params){

	const char	*n;

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	fprintf(qglState.logFile, "glSamplerParameterIuiv( %u, %s, %p )\n", sampler, n, params);
	dllSamplerParameterIuiv(sampler, pname, params);
}

static GLvoid APIENTRY logSamplerParameterf (GLuint sampler, GLenum pname, GLfloat param){

	const char	*n, *p;

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	switch ((int)param){
	case GL_NEAREST:					p = "GL_NEAREST";					break;
	case GL_LINEAR:						p = "GL_LINEAR";					break;
	case GL_NEAREST_MIPMAP_NEAREST:		p = "GL_NEAREST_MIPMAP_NEAREST";	break;
	case GL_LINEAR_MIPMAP_NEAREST:		p = "GL_LINEAR_MIPMAP_NEAREST";		break;
	case GL_NEAREST_MIPMAP_LINEAR:		p = "GL_NEAREST_MIPMAP_LINEAR";		break;
	case GL_LINEAR_MIPMAP_LINEAR:		p = "GL_LINEAR_MIPMAP_LINEAR";		break;
	case GL_REPEAT:						p = "GL_REPEAT";					break;
	case GL_MIRRORED_REPEAT:			p = "GL_MIRRORED_REPEAT";			break;
	case GL_CLAMP:						p = "GL_CLAMP";						break;
	case GL_CLAMP_TO_EDGE:				p = "GL_CLAMP_TO_EDGE";				break;
	case GL_CLAMP_TO_BORDER:			p = "GL_CLAMP_TO_BORDER";			break;
	case GL_RED:						p = "GL_RED";						break;
	case GL_GREEN:						p = "GL_GREEN";						break;
	case GL_BLUE:						p = "GL_BLUE";						break;
	case GL_ALPHA:						p = "GL_ALPHA";						break;
	case GL_ZERO:						p = "GL_ZERO";						break;
	case GL_ONE:						p = "GL_ONE";						break;
	case GL_COMPARE_REF_TO_TEXTURE:		p = "GL_COMPARE_REF_TO_TEXTURE";	break;
	case GL_NEVER:						p = "GL_NEVER";						break;
	case GL_LESS:						p = "GL_LESS";						break;
	case GL_LEQUAL:						p = "GL_LEQUAL";					break;
	case GL_EQUAL:						p = "GL_EQUAL";						break;
	case GL_NOTEQUAL:					p = "GL_NOTEQUAL";					break;
	case GL_GEQUAL:						p = "GL_GEQUAL";					break;
	case GL_GREATER:					p = "GL_GREATER";					break;
	case GL_ALWAYS:						p = "GL_ALWAYS";					break;
	case GL_INTENSITY:					p = "GL_INTENSITY";					break;
	case GL_LUMINANCE:					p = "GL_LUMINANCE";					break;
	default:							p = Str_VarArgs("0x%08X", param);		break;
	}

	if (pname == GL_GENERATE_MIPMAP){
		if ((int)param == GL_FALSE)
			p = "GL_FALSE";
		else
			p = "GL_TRUE";
	}

	if (pname == GL_TEXTURE_MIN_LOD || pname == GL_TEXTURE_MAX_LOD || pname == GL_TEXTURE_BASE_LEVEL || pname == GL_TEXTURE_MAX_LEVEL || pname == GL_TEXTURE_LOD_BIAS || pname == GL_TEXTURE_MAX_ANISOTROPY_EXT)
		p = Str_VarArgs("%g", param);

	if (pname == GL_TEXTURE_COMPARE_MODE && (int)param == GL_NONE)
		p = "GL_NONE";

	fprintf(qglState.logFile, "glSamplerParameterf( %u, %s, %s )\n", sampler, n, p);
	dllSamplerParameterf(sampler, pname, param);
}

static GLvoid APIENTRY logSamplerParameterfv (GLuint sampler, GLenum pname, const GLfloat *params){

	const char	*n;

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	fprintf(qglState.logFile, "glSamplerParameterfv( %u, %s, %p )\n", sampler, n, params);
	dllSamplerParameterfv(sampler, pname, params);
}

static GLvoid APIENTRY logSamplerParameteri (GLuint sampler, GLenum pname, GLint param){

	const char	*n, *p;

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	switch (param){
	case GL_NEAREST:					p = "GL_NEAREST";					break;
	case GL_LINEAR:						p = "GL_LINEAR";					break;
	case GL_NEAREST_MIPMAP_NEAREST:		p = "GL_NEAREST_MIPMAP_NEAREST";	break;
	case GL_LINEAR_MIPMAP_NEAREST:		p = "GL_LINEAR_MIPMAP_NEAREST";		break;
	case GL_NEAREST_MIPMAP_LINEAR:		p = "GL_NEAREST_MIPMAP_LINEAR";		break;
	case GL_LINEAR_MIPMAP_LINEAR:		p = "GL_LINEAR_MIPMAP_LINEAR";		break;
	case GL_REPEAT:						p = "GL_REPEAT";					break;
	case GL_MIRRORED_REPEAT:			p = "GL_MIRRORED_REPEAT";			break;
	case GL_CLAMP:						p = "GL_CLAMP";						break;
	case GL_CLAMP_TO_EDGE:				p = "GL_CLAMP_TO_EDGE";				break;
	case GL_CLAMP_TO_BORDER:			p = "GL_CLAMP_TO_BORDER";			break;
	case GL_RED:						p = "GL_RED";						break;
	case GL_GREEN:						p = "GL_GREEN";						break;
	case GL_BLUE:						p = "GL_BLUE";						break;
	case GL_ALPHA:						p = "GL_ALPHA";						break;
	case GL_ZERO:						p = "GL_ZERO";						break;
	case GL_ONE:						p = "GL_ONE";						break;
	case GL_COMPARE_REF_TO_TEXTURE:		p = "GL_COMPARE_REF_TO_TEXTURE";	break;
	case GL_NEVER:						p = "GL_NEVER";						break;
	case GL_LESS:						p = "GL_LESS";						break;
	case GL_LEQUAL:						p = "GL_LEQUAL";					break;
	case GL_EQUAL:						p = "GL_EQUAL";						break;
	case GL_NOTEQUAL:					p = "GL_NOTEQUAL";					break;
	case GL_GEQUAL:						p = "GL_GEQUAL";					break;
	case GL_GREATER:					p = "GL_GREATER";					break;
	case GL_ALWAYS:						p = "GL_ALWAYS";					break;
	case GL_INTENSITY:					p = "GL_INTENSITY";					break;
	case GL_LUMINANCE:					p = "GL_LUMINANCE";					break;
	default:							p = Str_VarArgs("0x%08X", param);		break;
	}

	if (pname == GL_GENERATE_MIPMAP){
		if (param == GL_FALSE)
			p = "GL_FALSE";
		else
			p = "GL_TRUE";
	}

	if (pname == GL_TEXTURE_MIN_LOD || pname == GL_TEXTURE_MAX_LOD || pname == GL_TEXTURE_BASE_LEVEL || pname == GL_TEXTURE_MAX_LEVEL || pname == GL_TEXTURE_LOD_BIAS || pname == GL_TEXTURE_MAX_ANISOTROPY_EXT)
		p = Str_VarArgs("%i", param);

	if (pname == GL_TEXTURE_COMPARE_MODE && param == GL_NONE)
		p = "GL_NONE";

	fprintf(qglState.logFile, "glSamplerParameteri( %u, %s, %s )\n", sampler, n, p);
	dllSamplerParameteri(sampler, pname, param);
}

static GLvoid APIENTRY logSamplerParameteriv (GLuint sampler, GLenum pname, const GLint *params){

	const char	*n;

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	fprintf(qglState.logFile, "glSamplerParameteriv( %u, %s, %p )\n", sampler, n, params);
	dllSamplerParameteriv(sampler, pname, params);
}

static GLvoid APIENTRY logScaled (GLdouble x, GLdouble y, GLdouble z){

	fprintf(qglState.logFile, "glScaled( %g, %g, %g )\n", x, y, z);
	dllScaled(x, y, z);
}

static GLvoid APIENTRY logScalef (GLfloat x, GLfloat y, GLfloat z){

	fprintf(qglState.logFile, "glScalef( %g, %g, %g )\n", x, y, z);
	dllScalef(x, y, z);
}

static GLvoid APIENTRY logScissor (GLint x, GLint y, GLsizei width, GLsizei height){

	fprintf(qglState.logFile, "glScissor( %i, %i, %i, %i )\n", x, y, width, height);
	dllScissor(x, y, width, height);
}

static GLvoid APIENTRY logSecondaryColor3b (GLbyte red, GLbyte green, GLbyte blue){

	fprintf(qglState.logFile, "glSecondaryColor3b( %i, %i, %i )\n", red, green, blue);
	dllSecondaryColor3b(red, green, blue);
}

static GLvoid APIENTRY logSecondaryColor3bv (const GLbyte *v){

	fprintf(qglState.logFile, "glSecondaryColor3bv( %p )\n", v);
	dllSecondaryColor3bv(v);
}

static GLvoid APIENTRY logSecondaryColor3d (GLdouble red, GLdouble green, GLdouble blue){

	fprintf(qglState.logFile, "glSecondaryColor3d( %g, %g, %g )\n", red, green, blue);
	dllSecondaryColor3d(red, green, blue);
}

static GLvoid APIENTRY logSecondaryColor3dv (const GLdouble *v){

	fprintf(qglState.logFile, "glSecondaryColor3dv( %p )\n", v);
	dllSecondaryColor3dv(v);
}

static GLvoid APIENTRY logSecondaryColor3f (GLfloat red, GLfloat green, GLfloat blue){

	fprintf(qglState.logFile, "glSecondaryColor3f( %g, %g, %g )\n", red, green, blue);
	dllSecondaryColor3f(red, green, blue);
}

static GLvoid APIENTRY logSecondaryColor3fv (const GLfloat *v){

	fprintf(qglState.logFile, "glSecondaryColor3fv( %p )\n", v);
	dllSecondaryColor3fv(v);
}

static GLvoid APIENTRY logSecondaryColor3i (GLint red, GLint green, GLint blue){

	fprintf(qglState.logFile, "glSecondaryColor3i( %i, %i, %i )\n", red, green, blue);
	dllSecondaryColor3i(red, green, blue);
}

static GLvoid APIENTRY logSecondaryColor3iv (const GLint *v){

	fprintf(qglState.logFile, "glSecondaryColor3iv( %p )\n", v);
	dllSecondaryColor3iv(v);
}

static GLvoid APIENTRY logSecondaryColor3s (GLshort red, GLshort green, GLshort blue){

	fprintf(qglState.logFile, "glSecondaryColor3s( %i, %i, %i )\n", red, green, blue);
	dllSecondaryColor3s(red, green, blue);
}

static GLvoid APIENTRY logSecondaryColor3sv (const GLshort *v){

	fprintf(qglState.logFile, "glSecondaryColor3sv( %p )\n", v);
	dllSecondaryColor3sv(v);
}

static GLvoid APIENTRY logSecondaryColor3ub (GLubyte red, GLubyte green, GLubyte blue){

	fprintf(qglState.logFile, "glSecondaryColor3ub( %u, %u, %u )\n", red, green, blue);
	dllSecondaryColor3ub(red, green, blue);
}

static GLvoid APIENTRY logSecondaryColor3ubv (const GLubyte *v){

	fprintf(qglState.logFile, "glSecondaryColor3ubv( %p )\n", v);
	dllSecondaryColor3ubv(v);
}

static GLvoid APIENTRY logSecondaryColor3ui (GLuint red, GLuint green, GLuint blue){

	fprintf(qglState.logFile, "glSecondaryColor3ui( %u, %u, %u )\n", red, green, blue);
	dllSecondaryColor3ui(red, green, blue);
}

static GLvoid APIENTRY logSecondaryColor3uiv (const GLuint *v){

	fprintf(qglState.logFile, "glSecondaryColor3uiv( %p )\n", v);
	dllSecondaryColor3uiv(v);
}

static GLvoid APIENTRY logSecondaryColor3us (GLushort red, GLushort green, GLushort blue){

	fprintf(qglState.logFile, "glSecondaryColor3us( %u, %u, %u )\n", red, green, blue);
	dllSecondaryColor3us(red, green, blue);
}

static GLvoid APIENTRY logSecondaryColor3usv (const GLushort *v){

	fprintf(qglState.logFile, "glSecondaryColor3usv( %p )\n", v);
	dllSecondaryColor3usv(v);
}

static GLvoid APIENTRY logSecondaryColorP3ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glSecondaryColorP3ui( 0x%08X, %u )\n", type, v);
	dllSecondaryColorP3ui(type, v);
}

static GLvoid APIENTRY logSecondaryColorP3uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glSecondaryColorP3uiv( 0x%08X, %p )\n", type, v);
	dllSecondaryColorP3uiv(type, v);
}

static GLvoid APIENTRY logSecondaryColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){

	const char	*t;

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";					break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";			break;
	case GL_SHORT:			t = "GL_SHORT";					break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";		break;
	case GL_INT:			t = "GL_INT";					break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";			break;
	case GL_FLOAT:			t = "GL_FLOAT";					break;
	case GL_HALF_FLOAT:		t = "GL_HALF_FLOAT";			break;
	case GL_DOUBLE:			t = "GL_DOUBLE";				break;
	default:				t = Str_VarArgs("0x%08X", type);	break;
	}

	fprintf(qglState.logFile, "glSecondaryColorPointer( %i, %s, %i, %p )\n", size, t, stride, pointer);
	dllSecondaryColorPointer(size, type, stride, pointer);
}

static GLvoid APIENTRY logSelectBuffer (GLsizei size, GLuint *buffer){

	fprintf(qglState.logFile, "glSelectBuffer( %i, %p )\n", size, buffer);
	dllSelectBuffer(size, buffer);
}

static GLvoid APIENTRY logShadeModel (GLenum mode){

	const char	*m;

	switch (mode){
	case GL_FLAT:	m = "GL_FLAT";					break;
	case GL_SMOOTH:	m = "GL_SMOOTH";				break;
	default:		m = Str_VarArgs("0x%08X", mode);	break;
	}

	fprintf(qglState.logFile, "glShadeModel( %s )\n", m);
	dllShadeModel(mode);
}

static GLvoid APIENTRY logShaderSource (GLuint shader, GLsizei count, const GLchar **string, const GLint *length){

	fprintf(qglState.logFile, "glShaderSource( %u, %i, %p, %p )\n", shader, count, string, length);
	dllShaderSource(shader, count, string, length);
}

static GLvoid APIENTRY logStencilFunc (GLenum func, GLint ref, GLuint mask){

	const char	*f;

	switch (func){
	case GL_NEVER:		f = "GL_NEVER";					break;
	case GL_LESS:		f = "GL_LESS";					break;
	case GL_LEQUAL:		f = "GL_LEQUAL";				break;
	case GL_EQUAL:		f = "GL_EQUAL";					break;
	case GL_NOTEQUAL:	f = "GL_NOTEQUAL";				break;
	case GL_GEQUAL:		f = "GL_GEQUAL";				break;
	case GL_GREATER:	f = "GL_GREATER";				break;
	case GL_ALWAYS:		f = "GL_ALWAYS";				break;
	default:			f = Str_VarArgs("0x%08X", func);	break;
	}

	fprintf(qglState.logFile, "glStencilFunc( %s, %i, %u )\n", f, ref, mask);
	dllStencilFunc(func, ref, mask);
}

static GLvoid APIENTRY logStencilFuncSeparate (GLenum face, GLenum func, GLint ref, GLuint mask){

	const char	*f, *sf;

	switch (face){
	case GL_FRONT:			f = "GL_FRONT";					break;
	case GL_BACK:			f = "GL_BACK";					break;
	case GL_FRONT_AND_BACK:	f = "GL_FRONT_AND_BACK";		break;
	default:				f = Str_VarArgs("0x%08X", face);	break;
	}

	switch (func){
	case GL_NEVER:			sf = "GL_NEVER";				break;
	case GL_LESS:			sf = "GL_LESS";					break;
	case GL_LEQUAL:			sf = "GL_LEQUAL";				break;
	case GL_EQUAL:			sf = "GL_EQUAL";				break;
	case GL_NOTEQUAL:		sf = "GL_NOTEQUAL";				break;
	case GL_GEQUAL:			sf = "GL_GEQUAL";				break;
	case GL_GREATER:		sf = "GL_GREATER";				break;
	case GL_ALWAYS:			sf = "GL_ALWAYS";				break;
	default:				sf = Str_VarArgs("0x%08X", func);	break;
	}

	fprintf(qglState.logFile, "glStencilFuncSeparate( %s, %s, %i, %u )\n", f, sf, ref, mask);
	dllStencilFuncSeparate(face, func, ref, mask);
}

static GLvoid APIENTRY logStencilMask (GLuint mask){

	fprintf(qglState.logFile, "glStencilMask( %u )\n", mask);
	dllStencilMask(mask);
}

static GLvoid APIENTRY logStencilMaskSeparate (GLenum face, GLuint mask){

	const char	*f;

	switch (face){
	case GL_FRONT:			f = "GL_FRONT";					break;
	case GL_BACK:			f = "GL_BACK";					break;
	case GL_FRONT_AND_BACK:	f = "GL_FRONT_AND_BACK";		break;
	default:				f = Str_VarArgs("0x%08X", face);	break;
	}

	fprintf(qglState.logFile, "glStencilMaskSeparate( %s, %u )\n", f, mask);
	dllStencilMaskSeparate(face, mask);
}

static GLvoid APIENTRY logStencilOp (GLenum fail, GLenum zfail, GLenum zpass){

	const char	*f, *zf, *zp;

	switch (fail){
	case GL_KEEP:		f = "GL_KEEP";						break;
	case GL_ZERO:		f = "GL_ZERO";						break;
	case GL_REPLACE:	f = "GL_REPLACE";					break;
	case GL_INCR:		f = "GL_INCR";						break;
	case GL_INCR_WRAP:	f = "GL_INCR_WRAP";					break;
	case GL_DECR:		f = "GL_DECR";						break;
	case GL_DECR_WRAP:	f = "GL_DECR_WRAP";					break;
	case GL_INVERT:		f = "GL_INVERT";					break;
	default:			f = Str_VarArgs("0x%08X", fail);		break;
	}

	switch (zfail){
	case GL_KEEP:		zf = "GL_KEEP";						break;
	case GL_ZERO:		zf = "GL_ZERO";						break;
	case GL_REPLACE:	zf = "GL_REPLACE";					break;
	case GL_INCR:		zf = "GL_INCR";						break;
	case GL_INCR_WRAP:	zf = "GL_INCR_WRAP";				break;
	case GL_DECR:		zf = "GL_DECR";						break;
	case GL_DECR_WRAP:	zf = "GL_DECR_WRAP";				break;
	case GL_INVERT:		zf = "GL_INVERT";					break;
	default:			zf = Str_VarArgs("0x%08X", zfail);	break;
	}

	switch (zpass){
	case GL_KEEP:		zp = "GL_KEEP";						break;
	case GL_ZERO:		zp = "GL_ZERO";						break;
	case GL_REPLACE:	zp = "GL_REPLACE";					break;
	case GL_INCR:		zp = "GL_INCR";						break;
	case GL_INCR_WRAP:	zp = "GL_INCR_WRAP";				break;
	case GL_DECR:		zp = "GL_DECR";						break;
	case GL_DECR_WRAP:	zp = "GL_DECR_WRAP";				break;
	case GL_INVERT:		zp = "GL_INVERT";					break;
	default:			zp = Str_VarArgs("0x%08X", zpass);	break;
	}

	fprintf(qglState.logFile, "glStencilOp( %s, %s, %s )\n", f, zf, zp);
	dllStencilOp(fail, zfail, zpass);
}

static GLvoid APIENTRY logStencilOpSeparate (GLenum face, GLenum fail, GLenum zfail, GLenum zpass){

	const char	*f, *sf, *zf, *zp;

	switch (face){
	case GL_FRONT:			f = "GL_FRONT";						break;
	case GL_BACK:			f = "GL_BACK";						break;
	case GL_FRONT_AND_BACK:	f = "GL_FRONT_AND_BACK";			break;
	default:				f = Str_VarArgs("0x%08X", face);		break;
	}

	switch (fail){
	case GL_KEEP:			sf = "GL_KEEP";						break;
	case GL_ZERO:			sf = "GL_ZERO";						break;
	case GL_REPLACE:		sf = "GL_REPLACE";					break;
	case GL_INCR:			sf = "GL_INCR";						break;
	case GL_INCR_WRAP:		sf = "GL_INCR_WRAP";				break;
	case GL_DECR:			sf = "GL_DECR";						break;
	case GL_DECR_WRAP:		sf = "GL_DECR_WRAP";				break;
	case GL_INVERT:			sf = "GL_INVERT";					break;
	default:				sf = Str_VarArgs("0x%08X", fail);		break;
	}

	switch (zfail){
	case GL_KEEP:			zf = "GL_KEEP";						break;
	case GL_ZERO:			zf = "GL_ZERO";						break;
	case GL_REPLACE:		zf = "GL_REPLACE";					break;
	case GL_INCR:			zf = "GL_INCR";						break;
	case GL_INCR_WRAP:		zf = "GL_INCR_WRAP";				break;
	case GL_DECR:			zf = "GL_DECR";						break;
	case GL_DECR_WRAP:		zf = "GL_DECR_WRAP";				break;
	case GL_INVERT:			zf = "GL_INVERT";					break;
	default:				zf = Str_VarArgs("0x%08X", zfail);	break;
	}

	switch (zpass){
	case GL_KEEP:			zp = "GL_KEEP";						break;
	case GL_ZERO:			zp = "GL_ZERO";						break;
	case GL_REPLACE:		zp = "GL_REPLACE";					break;
	case GL_INCR:			zp = "GL_INCR";						break;
	case GL_INCR_WRAP:		zp = "GL_INCR_WRAP";				break;
	case GL_DECR:			zp = "GL_DECR";						break;
	case GL_DECR_WRAP:		zp = "GL_DECR_WRAP";				break;
	case GL_INVERT:			zp = "GL_INVERT";					break;
	default:				zp = Str_VarArgs("0x%08X", zpass);	break;
	}

	fprintf(qglState.logFile, "glStencilOpSeparate( %s, %s, %s, %s )\n", f, sf, zf, zp);
	dllStencilOpSeparate(face, fail, zfail, zpass);
}

static GLvoid APIENTRY logStencilOpSeparateATI (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass){

	const char	*f, *sf, *dpf, *dpp;

	switch (face){
	case GL_FRONT:			f = "GL_FRONT";						break;
	case GL_BACK:			f = "GL_BACK";						break;
	case GL_FRONT_AND_BACK:	f = "GL_FRONT_AND_BACK";			break;
	default:				f = Str_VarArgs("0x%X", face);		break;
	}

	switch (sfail){
	case GL_KEEP:			sf = "GL_KEEP";						break;
	case GL_ZERO:			sf = "GL_ZERO";						break;
	case GL_REPLACE:		sf = "GL_REPLACE";					break;
	case GL_INCR:			sf = "GL_INCR";						break;
	case GL_INCR_WRAP_EXT:	sf = "GL_INCR_WRAP";				break;
	case GL_DECR:			sf = "GL_DECR";						break;
	case GL_DECR_WRAP_EXT:	sf = "GL_DECR_WRAP";				break;
	case GL_INVERT:			sf = "GL_INVERT";					break;
	default:				sf = Str_VarArgs("0x%X", sfail);	break;
	}

	switch (dpfail){
	case GL_KEEP:			dpf = "GL_KEEP";					break;
	case GL_ZERO:			dpf = "GL_ZERO";					break;
	case GL_REPLACE:		dpf = "GL_REPLACE";					break;
	case GL_INCR:			dpf = "GL_INCR";					break;
	case GL_INCR_WRAP_EXT:	dpf = "GL_INCR_WRAP";				break;
	case GL_DECR:			dpf = "GL_DECR";					break;
	case GL_DECR_WRAP_EXT:	dpf = "GL_DECR_WRAP";				break;
	case GL_INVERT:			dpf = "GL_INVERT";					break;
	default:				dpf = Str_VarArgs("0x%X", dpfail);	break;
	}

	switch (dppass){
	case GL_KEEP:			dpp = "GL_KEEP";					break;
	case GL_ZERO:			dpp = "GL_ZERO";					break;
	case GL_REPLACE:		dpp = "GL_REPLACE";					break;
	case GL_INCR:			dpp = "GL_INCR";					break;
	case GL_INCR_WRAP_EXT:	dpp = "GL_INCR_WRAP";				break;
	case GL_DECR:			dpp = "GL_DECR";					break;
	case GL_DECR_WRAP_EXT:	dpp = "GL_DECR_WRAP";				break;
	case GL_INVERT:			dpp = "GL_INVERT";					break;
	default:				dpp = Str_VarArgs("0x%X", dppass);	break;
	}

	fprintf(qglState.logFile, "glStencilOpSeparate( %s, %s, %s, %s )\n", f, sf, dpf, dpp);
	dllStencilOpSeparateATI(face, sfail, dpfail, dppass);
}

static GLvoid APIENTRY logStencilFuncSeparateATI (GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask){

	const char	*ff, *bf;

	switch (frontfunc){
	case GL_NEVER:		ff = "GL_NEVER";						break;
	case GL_LESS:		ff = "GL_LESS";							break;
	case GL_LEQUAL:		ff = "GL_LEQUAL";						break;
	case GL_GREATER:	ff = "GL_GREATER";						break;
	case GL_GEQUAL:		ff = "GL_GEQUAL";						break;
	case GL_EQUAL:		ff = "GL_EQUAL";						break;
	case GL_NOTEQUAL:	ff = "GL_NOTEQUAL";						break;
	case GL_ALWAYS:		ff = "GL_ALWAYS";						break;
	default:			ff = Str_VarArgs("0x%X", frontfunc);	break;
	}

	switch (backfunc){
	case GL_NEVER:		bf = "GL_NEVER";						break;
	case GL_LESS:		bf = "GL_LESS";							break;
	case GL_LEQUAL:		bf = "GL_LEQUAL";						break;
	case GL_GREATER:	bf = "GL_GREATER";						break;
	case GL_GEQUAL:		bf = "GL_GEQUAL";						break;
	case GL_EQUAL:		bf = "GL_EQUAL";						break;
	case GL_NOTEQUAL:	bf = "GL_NOTEQUAL";						break;
	case GL_ALWAYS:		bf = "GL_ALWAYS";						break;
	default:			bf = Str_VarArgs("0x%X", backfunc);		break;
	}

	fprintf(qglState.logFile, "glStencilFuncSeparate( %s, %s, %i, %u )\n", ff, bf, ref, mask);
	dllStencilFuncSeparateATI(frontfunc, backfunc, ref, mask);
}

static GLvoid APIENTRY logTexBuffer (GLenum target, GLenum internalformat, GLuint buffer){

	const char	*t, *i;

	switch (target){
	case GL_TEXTURE_BUFFER:	t = "GL_TEXTURE_BUFFER";					break;
	default:				t = Str_VarArgs("0x%08X", target);			break;
	}

	switch (internalformat){
	case GL_R8:				i = "GL_R8";								break;
	case GL_R16F:			i = "GL_R16F";								break;
	case GL_R32F:			i = "GL_R32F";								break;
	case GL_RG8:			i = "GL_RG8";								break;
	case GL_RG16F:			i = "GL_RG16F";								break;
	case GL_RG32F:			i = "GL_RG32F";								break;
	case GL_RGB8:			i = "GL_RGB8";								break;
	case GL_RGB16F:			i = "GL_RGB16F";							break;
	case GL_RGB32F:			i = "GL_RGB32F";							break;
	case GL_RGBA8:			i = "GL_RGBA8";								break;
	case GL_RGBA16F:		i = "GL_RGBA16F";							break;
	case GL_RGBA32F:		i = "GL_RGBA32F";							break;
	default:				i = Str_VarArgs("0x%08X", internalformat);	break;
	}

	fprintf(qglState.logFile, "glTextureBuffer( %s, %s, %u )\n", t, i, buffer);
	dllTexBuffer(target, internalformat, buffer);
}

static GLvoid APIENTRY logTexCoord1d (GLdouble s){

	fprintf(qglState.logFile, "glTexCoord1d( %g )\n", s);
	dllTexCoord1d(s);
}

static GLvoid APIENTRY logTexCoord1dv (const GLdouble *v){

	fprintf(qglState.logFile, "glTexCoord1dv( %p )\n", v);
	dllTexCoord1dv(v);
}

static GLvoid APIENTRY logTexCoord1f (GLfloat s){

	fprintf(qglState.logFile, "glTexCoord1f( %g )\n", s);
	dllTexCoord1f(s);
}

static GLvoid APIENTRY logTexCoord1fv (const GLfloat *v){

	fprintf(qglState.logFile, "glTexCoord1fv( %p )\n", v);
	dllTexCoord1fv(v);
}

static GLvoid APIENTRY logTexCoord1i (GLint s){

	fprintf(qglState.logFile, "glTexCoord1i( %i )\n", s);
	dllTexCoord1i(s);
}

static GLvoid APIENTRY logTexCoord1iv (const GLint *v){

	fprintf(qglState.logFile, "glTexCoord1iv( %p )\n", v);
	dllTexCoord1iv(v);
}

static GLvoid APIENTRY logTexCoord1s (GLshort s){

	fprintf(qglState.logFile, "glTexCoord1s( %i )\n", s);
	dllTexCoord1s(s);
}

static GLvoid APIENTRY logTexCoord1sv (const GLshort *v){

	fprintf(qglState.logFile, "glTexCoord1sv( %p )\n", v);
	dllTexCoord1sv(v);
}

static GLvoid APIENTRY logTexCoord2d (GLdouble s, GLdouble t){

	fprintf(qglState.logFile, "glTexCoord2d( %g, %g )\n", s, t);
	dllTexCoord2d(s, t);
}

static GLvoid APIENTRY logTexCoord2dv (const GLdouble *v){

	fprintf(qglState.logFile, "glTexCoord2dv( %p )\n", v);
	dllTexCoord2dv(v);
}

static GLvoid APIENTRY logTexCoord2f (GLfloat s, GLfloat t){

	fprintf(qglState.logFile, "glTexCoord2f( %g, %g )\n", s, t);
	dllTexCoord2f(s, t);
}

static GLvoid APIENTRY logTexCoord2fv (const GLfloat *v){

	fprintf(qglState.logFile, "glTexCoord2fv( %p )\n", v);
	dllTexCoord2fv(v);
}

static GLvoid APIENTRY logTexCoord2i (GLint s, GLint t){

	fprintf(qglState.logFile, "glTexCoord2i( %i, %i )\n", s, t);
	dllTexCoord2i(s, t);
}

static GLvoid APIENTRY logTexCoord2iv (const GLint *v){

	fprintf(qglState.logFile, "glTexCoord2iv( %p )\n", v);
	dllTexCoord2iv(v);
}

static GLvoid APIENTRY logTexCoord2s (GLshort s, GLshort t){

	fprintf(qglState.logFile, "glTexCoord2s( %i, %i )\n", s, t);
	dllTexCoord2s(s, t);
}

static GLvoid APIENTRY logTexCoord2sv (const GLshort *v){

	fprintf(qglState.logFile, "glTexCoord2sv( %p )\n", v);
	dllTexCoord2sv(v);
}

static GLvoid APIENTRY logTexCoord3d (GLdouble s, GLdouble t, GLdouble r){

	fprintf(qglState.logFile, "glTexCoord3d( %g, %g, %g )\n", s, t, r);
	dllTexCoord3d(s, t, r);
}

static GLvoid APIENTRY logTexCoord3dv (const GLdouble *v){

	fprintf(qglState.logFile, "glTexCoord3dv( %p )\n", v);
	dllTexCoord3dv(v);
}

static GLvoid APIENTRY logTexCoord3f (GLfloat s, GLfloat t, GLfloat r){

	fprintf(qglState.logFile, "glTexCoord3f( %g, %g, %g )\n", s, t, r);
	dllTexCoord3f(s, t, r);
}

static GLvoid APIENTRY logTexCoord3fv (const GLfloat *v){

	fprintf(qglState.logFile, "glTexCoord3fv( %p )\n", v);
	dllTexCoord3fv(v);
}

static GLvoid APIENTRY logTexCoord3i (GLint s, GLint t, GLint r){

	fprintf(qglState.logFile, "glTexCoord3i( %i, %i, %i )\n", s, t, r);
	dllTexCoord3i(s, t, r);
}

static GLvoid APIENTRY logTexCoord3iv (const GLint *v){

	fprintf(qglState.logFile, "glTexCoord3iv( %p )\n");
	dllTexCoord3iv(v);
}

static GLvoid APIENTRY logTexCoord3s (GLshort s, GLshort t, GLshort r){

	fprintf(qglState.logFile, "glTexCoord3s( %i, %i, %i )\n", s, t, r);
	dllTexCoord3s(s, t, r);
}

static GLvoid APIENTRY logTexCoord3sv (const GLshort *v){

	fprintf(qglState.logFile, "glTexCoord3sv( %p )\n", v);
	dllTexCoord3sv(v);
}

static GLvoid APIENTRY logTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q){

	fprintf(qglState.logFile, "glTexCoord4d( %g, %g, %g, %g )\n", s, t, r, q);
	dllTexCoord4d(s, t, r, q);
}

static GLvoid APIENTRY logTexCoord4dv (const GLdouble *v){

	fprintf(qglState.logFile, "glTexCoord4dv( %p )\n", v);
	dllTexCoord4dv(v);
}

static GLvoid APIENTRY logTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q){

	fprintf(qglState.logFile, "glTexCoord4f( %g, %g, %g, %g )\n", s, t, r, q);
	dllTexCoord4f(s, t, r, q);
}

static GLvoid APIENTRY logTexCoord4fv (const GLfloat *v){

	fprintf(qglState.logFile, "glTexCoord4fv( %p )\n", v);
	dllTexCoord4fv(v);
}

static GLvoid APIENTRY logTexCoord4i (GLint s, GLint t, GLint r, GLint q){

	fprintf(qglState.logFile, "glTexCoord4i( %i, %i, %i, %i )\n", s, t, r, q);
	dllTexCoord4i(s, t, r, q);
}

static GLvoid APIENTRY logTexCoord4iv (const GLint *v){

	fprintf(qglState.logFile, "glTexCoord4iv( %p )\n", v);
	dllTexCoord4iv(v);
}

static GLvoid APIENTRY logTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q){

	fprintf(qglState.logFile, "glTexCoord4s( %i, %i, %i, %i )\n", s, t, r, q);
	dllTexCoord4s(s, t, r, q);
}

static GLvoid APIENTRY logTexCoord4sv (const GLshort *v){

	fprintf(qglState.logFile, "glTexCoord4sv( %p )\n", v);
	dllTexCoord4sv(v);
}

static GLvoid APIENTRY logTexCoordP1ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glTexCoordP1ui( 0x%08X, %u )\n", type, v);
	dllTexCoordP1ui(type, v);
}

static GLvoid APIENTRY logTexCoordP1uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glTexCoordP1uiv( 0x%08X, %p )\n", type, v);
	dllTexCoordP1uiv(type, v);
}

static GLvoid APIENTRY logTexCoordP2ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glTexCoordP2ui( 0x%08X, %u )\n", type, v);
	dllTexCoordP2ui(type, v);
}

static GLvoid APIENTRY logTexCoordP2uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glTexCoordP2uiv( 0x%08X, %p )\n", type, v);
	dllTexCoordP2uiv(type, v);
}

static GLvoid APIENTRY logTexCoordP3ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glTexCoordP3ui( 0x%08X, %u )\n", type, v);
	dllTexCoordP3ui(type, v);
}

static GLvoid APIENTRY logTexCoordP3uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glTexCoordP3uiv( 0x%08X, %p )\n", type, v);
	dllTexCoordP3uiv(type, v);
}

static GLvoid APIENTRY logTexCoordP4ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glTexCoordP4ui( 0x%08X, %u )\n", type, v);
	dllTexCoordP4ui(type, v);
}

static GLvoid APIENTRY logTexCoordP4uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glTexCoordP4uiv( 0x%08X, %p )\n", type, v);
	dllTexCoordP4uiv(type, v);
}

static GLvoid APIENTRY logTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){

	const char	*t;

	switch (type){
	case GL_SHORT:		t = "GL_SHORT";					break;
	case GL_INT:		t = "GL_INT";					break;
	case GL_FLOAT:		t = "GL_FLOAT";					break;
	case GL_HALF_FLOAT:	t = "GL_HALF_FLOAT";			break;
	case GL_DOUBLE:		t = "GL_DOUBLE";				break;
	default:			t = Str_VarArgs("0x%08X", type);	break;
	}

	fprintf(qglState.logFile, "glTexCoordPointer( %i, %s, %i, %p )\n", size, t, stride, pointer);
	dllTexCoordPointer(size, type, stride, pointer);
}

static GLvoid APIENTRY logTexEnvf (GLenum target, GLenum pname, GLfloat param){

	const char	*t, *n, *p;

	switch (target){
	case GL_TEXTURE_ENV:			t = "GL_TEXTURE_ENV";				break;
	case GL_TEXTURE_FILTER_CONTROL:	t = "GL_TEXTURE_FILTER_CONTROL";	break;
	default:						t = Str_VarArgs("0x%08X", target);	break;
	}
	
	switch (pname){
	case GL_TEXTURE_ENV_MODE:		n = "GL_TEXTURE_ENV_MODE";			break;
	case GL_TEXTURE_LOD_BIAS:		n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_COMBINE_RGB:			n = "GL_COMBINE_RGB";				break;
	case GL_COMBINE_ALPHA:			n = "GL_COMBINE_ALPHA";				break;
	case GL_SRC0_RGB:				n = "GL_SRC0_RGB";					break;
	case GL_SRC1_RGB:				n = "GL_SRC1_RGB";					break;
	case GL_SRC2_RGB:				n = "GL_SRC2_RGB";					break;
	case GL_SRC0_ALPHA:				n = "GL_SRC0_ALPHA";				break;
	case GL_SRC1_ALPHA:				n = "GL_SRC1_ALPHA";				break;
	case GL_SRC2_ALPHA:				n = "GL_SRC2_ALPHA";				break;
	case GL_OPERAND0_RGB:			n = "GL_OPERAND0_RGB";				break;
	case GL_OPERAND1_RGB:			n = "GL_OPERAND1_RGB";				break;
	case GL_OPERAND2_RGB:			n = "GL_OPERAND2_RGB";				break;
	case GL_OPERAND0_ALPHA:			n = "GL_OPERAND0_ALPHA";			break;
	case GL_OPERAND1_ALPHA:			n = "GL_OPERAND1_ALPHA";			break;
	case GL_OPERAND2_ALPHA:			n = "GL_OPERAND2_ALPHA";			break;
	case GL_RGB_SCALE:				n = "GL_RGB_SCALE";					break;
	case GL_ALPHA_SCALE:			n = "GL_ALPHA_SCALE";				break;
	default:						n = Str_VarArgs("0x%08X", pname);		break;
	}

	switch ((int)param){
	case GL_REPLACE:				p = "GL_REPLACE";					break;
	case GL_MODULATE:				p = "GL_MODULATE";					break;
	case GL_DECAL:					p = "GL_DECAL";						break;
	case GL_BLEND:					p = "GL_BLEND";						break;
	case GL_ADD:					p = "GL_ADD";						break;
	case GL_ADD_SIGNED:				p = "GL_ADD_SIGNED";				break;
	case GL_SUBTRACT:				p = "GL_SUBTRACT";					break;
	case GL_INTERPOLATE:			p = "GL_INTERPOLATE";				break;
	case GL_DOT3_RGB:				p = "GL_DOT3_RGB";					break;
	case GL_DOT3_RGBA:				p = "GL_DOT3_RGBA";					break;
	case GL_COMBINE:				p = "GL_COMBINE";					break;
	case GL_TEXTURE:				p = "GL_TEXTURE";					break;
	case GL_PREVIOUS:				p = "GL_PREVIOUS";					break;
	case GL_CONSTANT:				p = "GL_CONSTANT";					break;
	case GL_PRIMARY_COLOR:			p = "GL_PRIMARY_COLOR";				break;
	case GL_SRC_COLOR:				p = "GL_SRC_COLOR";					break;
	case GL_ONE_MINUS_SRC_COLOR:	p = "GL_ONE_MINUS_SRC_COLOR";		break;
	case GL_SRC_ALPHA:				p = "GL_SRC_ALPHA";					break;
	case GL_ONE_MINUS_SRC_ALPHA:	p = "GL_ONE_MINUS_SRC_ALPHA";		break;
	default:						p = Str_VarArgs("0x%08X", param);		break;
	}

	if (pname == GL_TEXTURE_LOD_BIAS || pname == GL_RGB_SCALE || pname == GL_ALPHA_SCALE)
		p = Str_VarArgs("%g", param);

	if ((int)param >= GL_TEXTURE0 && (int)param <= GL_TEXTURE31)
		p = Str_VarArgs("GL_TEXTURE%i", (int)param - GL_TEXTURE0);

	fprintf(qglState.logFile, "glTexEnvf( %s, %s, %s )\n", t, n, p);
	dllTexEnvf(target, pname, param);
}

static GLvoid APIENTRY logTexEnvfv (GLenum target, GLenum pname, const GLfloat *params){

	const char	*t, *n;

	switch (target){
	case GL_TEXTURE_ENV:			t = "GL_TEXTURE_ENV";				break;
	case GL_TEXTURE_FILTER_CONTROL:	t = "GL_TEXTURE_FILTER_CONTROL";	break;
	default:						t = Str_VarArgs("0x%08X", target);	break;
	}
	
	switch (pname){
	case GL_TEXTURE_ENV_MODE:		n = "GL_TEXTURE_ENV_MODE";			break;
	case GL_TEXTURE_ENV_COLOR:		n = "GL_TEXTURE_ENV_COLOR";			break;
	case GL_TEXTURE_LOD_BIAS:		n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_COMBINE_RGB:			n = "GL_COMBINE_RGB";				break;
	case GL_COMBINE_ALPHA:			n = "GL_COMBINE_ALPHA";				break;
	case GL_SRC0_RGB:				n = "GL_SRC0_RGB";					break;
	case GL_SRC1_RGB:				n = "GL_SRC1_RGB";					break;
	case GL_SRC2_RGB:				n = "GL_SRC2_RGB";					break;
	case GL_SRC0_ALPHA:				n = "GL_SRC0_ALPHA";				break;
	case GL_SRC1_ALPHA:				n = "GL_SRC1_ALPHA";				break;
	case GL_SRC2_ALPHA:				n = "GL_SRC2_ALPHA";				break;
	case GL_OPERAND0_RGB:			n = "GL_OPERAND0_RGB";				break;
	case GL_OPERAND1_RGB:			n = "GL_OPERAND1_RGB";				break;
	case GL_OPERAND2_RGB:			n = "GL_OPERAND2_RGB";				break;
	case GL_OPERAND0_ALPHA:			n = "GL_OPERAND0_ALPHA";			break;
	case GL_OPERAND1_ALPHA:			n = "GL_OPERAND1_ALPHA";			break;
	case GL_OPERAND2_ALPHA:			n = "GL_OPERAND2_ALPHA";			break;
	case GL_RGB_SCALE:				n = "GL_RGB_SCALE";					break;
	case GL_ALPHA_SCALE:			n = "GL_ALPHA_SCALE";				break;
	default:						n = Str_VarArgs("0x%08X", pname);		break;
	}

	fprintf(qglState.logFile, "glTexEnvfv( %s, %s, %p )\n", t, n, params);
	dllTexEnvfv(target, pname, params);
}

static GLvoid APIENTRY logTexEnvi (GLenum target, GLenum pname, GLint param){

	const char	*t, *n, *p;

	switch (target){
	case GL_TEXTURE_ENV:			t = "GL_TEXTURE_ENV";				break;
	case GL_TEXTURE_FILTER_CONTROL:	t = "GL_TEXTURE_FILTER_CONTROL";	break;
	default:						t = Str_VarArgs("0x%08X", target);	break;
	}
	
	switch (pname){
	case GL_TEXTURE_ENV_MODE:		n = "GL_TEXTURE_ENV_MODE";			break;
	case GL_TEXTURE_LOD_BIAS:		n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_COMBINE_RGB:			n = "GL_COMBINE_RGB";				break;
	case GL_COMBINE_ALPHA:			n = "GL_COMBINE_ALPHA";				break;
	case GL_SRC0_RGB:				n = "GL_SRC0_RGB";					break;
	case GL_SRC1_RGB:				n = "GL_SRC1_RGB";					break;
	case GL_SRC2_RGB:				n = "GL_SRC2_RGB";					break;
	case GL_SRC0_ALPHA:				n = "GL_SRC0_ALPHA";				break;
	case GL_SRC1_ALPHA:				n = "GL_SRC1_ALPHA";				break;
	case GL_SRC2_ALPHA:				n = "GL_SRC2_ALPHA";				break;
	case GL_OPERAND0_RGB:			n = "GL_OPERAND0_RGB";				break;
	case GL_OPERAND1_RGB:			n = "GL_OPERAND1_RGB";				break;
	case GL_OPERAND2_RGB:			n = "GL_OPERAND2_RGB";				break;
	case GL_OPERAND0_ALPHA:			n = "GL_OPERAND0_ALPHA";			break;
	case GL_OPERAND1_ALPHA:			n = "GL_OPERAND1_ALPHA";			break;
	case GL_OPERAND2_ALPHA:			n = "GL_OPERAND2_ALPHA";			break;
	case GL_RGB_SCALE:				n = "GL_RGB_SCALE";					break;
	case GL_ALPHA_SCALE:			n = "GL_ALPHA_SCALE";				break;
	default:						n = Str_VarArgs("0x%08X", pname);		break;
	}

	switch (param){
	case GL_REPLACE:				p = "GL_REPLACE";					break;
	case GL_MODULATE:				p = "GL_MODULATE";					break;
	case GL_DECAL:					p = "GL_DECAL";						break;
	case GL_BLEND:					p = "GL_BLEND";						break;
	case GL_ADD:					p = "GL_ADD";						break;
	case GL_ADD_SIGNED:				p = "GL_ADD_SIGNED";				break;
	case GL_SUBTRACT:				p = "GL_SUBTRACT";					break;
	case GL_INTERPOLATE:			p = "GL_INTERPOLATE";				break;
	case GL_DOT3_RGB:				p = "GL_DOT3_RGB";					break;
	case GL_DOT3_RGBA:				p = "GL_DOT3_RGBA";					break;
	case GL_COMBINE:				p = "GL_COMBINE";					break;
	case GL_TEXTURE:				p = "GL_TEXTURE";					break;
	case GL_PREVIOUS:				p = "GL_PREVIOUS";					break;
	case GL_CONSTANT:				p = "GL_CONSTANT";					break;
	case GL_PRIMARY_COLOR:			p = "GL_PRIMARY_COLOR";				break;
	case GL_SRC_COLOR:				p = "GL_SRC_COLOR";					break;
	case GL_ONE_MINUS_SRC_COLOR:	p = "GL_ONE_MINUS_SRC_COLOR";		break;
	case GL_SRC_ALPHA:				p = "GL_SRC_ALPHA";					break;
	case GL_ONE_MINUS_SRC_ALPHA:	p = "GL_ONE_MINUS_SRC_ALPHA";		break;
	default:						p = Str_VarArgs("0x%08X", param);		break;
	}

	if (pname == GL_TEXTURE_LOD_BIAS || pname == GL_RGB_SCALE || pname == GL_ALPHA_SCALE)
		p = Str_VarArgs("%i", param);

	if (param >= GL_TEXTURE0 && param <= GL_TEXTURE31)
		p = Str_VarArgs("GL_TEXTURE%i", param - GL_TEXTURE0);

	fprintf(qglState.logFile, "glTexEnvi( %s, %s, %s )\n", t, n, p);
	dllTexEnvi(target, pname, param);
}

static GLvoid APIENTRY logTexEnviv (GLenum target, GLenum pname, const GLint *params){

	const char	*t, *n;

	switch (target){
	case GL_TEXTURE_ENV:			t = "GL_TEXTURE_ENV";				break;
	case GL_TEXTURE_FILTER_CONTROL:	t = "GL_TEXTURE_FILTER_CONTROL";	break;
	default:						t = Str_VarArgs("0x%08X", target);	break;
	}
	
	switch (pname){
	case GL_TEXTURE_ENV_MODE:		n = "GL_TEXTURE_ENV_MODE";			break;
	case GL_TEXTURE_ENV_COLOR:		n = "GL_TEXTURE_ENV_COLOR";			break;
	case GL_TEXTURE_LOD_BIAS:		n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_COMBINE_RGB:			n = "GL_COMBINE_RGB";				break;
	case GL_COMBINE_ALPHA:			n = "GL_COMBINE_ALPHA";				break;
	case GL_SRC0_RGB:				n = "GL_SRC0_RGB";					break;
	case GL_SRC1_RGB:				n = "GL_SRC1_RGB";					break;
	case GL_SRC2_RGB:				n = "GL_SRC2_RGB";					break;
	case GL_SRC0_ALPHA:				n = "GL_SRC0_ALPHA";				break;
	case GL_SRC1_ALPHA:				n = "GL_SRC1_ALPHA";				break;
	case GL_SRC2_ALPHA:				n = "GL_SRC2_ALPHA";				break;
	case GL_OPERAND0_RGB:			n = "GL_OPERAND0_RGB";				break;
	case GL_OPERAND1_RGB:			n = "GL_OPERAND1_RGB";				break;
	case GL_OPERAND2_RGB:			n = "GL_OPERAND2_RGB";				break;
	case GL_OPERAND0_ALPHA:			n = "GL_OPERAND0_ALPHA";			break;
	case GL_OPERAND1_ALPHA:			n = "GL_OPERAND1_ALPHA";			break;
	case GL_OPERAND2_ALPHA:			n = "GL_OPERAND2_ALPHA";			break;
	case GL_RGB_SCALE:				n = "GL_RGB_SCALE";					break;
	case GL_ALPHA_SCALE:			n = "GL_ALPHA_SCALE";				break;
	default:						n = Str_VarArgs("0x%08X", pname);		break;
	}

	fprintf(qglState.logFile, "glTexEnviv( %s, %s, %p )\n", t, n, params);
	dllTexEnviv(target, pname, params);
}

static GLvoid APIENTRY logTexGend (GLenum coord, GLenum pname, GLdouble param){

	const char	*c, *n, *p;

	switch (coord){
	case GL_S:					c = "GL_S";						break;
	case GL_T:					c = "GL_T";						break;
	case GL_R:					c = "GL_R";						break;
	case GL_Q:					c = "GL_Q";						break;
	default:					c = Str_VarArgs("0x%08X", coord);	break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";		break;
	default:					n = Str_VarArgs("0x%08X", pname);	break;
	}

	switch ((int)param){
	case GL_OBJECT_LINEAR:		p = "GL_OBJECT_LINEAR";			break;
	case GL_EYE_LINEAR:			p = "GL_EYE_LINEAR";			break;
	case GL_SPHERE_MAP:			p = "GL_SPHERE_MAP";			break;
	case GL_NORMAL_MAP:			p = "GL_NORMAL_MAP";			break;
	case GL_REFLECTION_MAP:		p = "GL_REFLECTION_MAP";		break;
	default:					p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qglState.logFile, "glTexGend( %s, %s, %s )\n", c, n, p);
	dllTexGend(coord, pname, param);
}

static GLvoid APIENTRY logTexGendv (GLenum coord, GLenum pname, const GLdouble *params){

	const char	*c, *n;

	switch (coord){
	case GL_S:					c = "GL_S";						break;
	case GL_T:					c = "GL_T";						break;
	case GL_R:					c = "GL_R";						break;
	case GL_Q:					c = "GL_Q";						break;
	default:					c = Str_VarArgs("0x%08X", coord);	break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";		break;
	case GL_OBJECT_PLANE:		n = "GL_OBJECT_PLANE";			break;
	case GL_EYE_PLANE:			n = "GL_EYE_PLANE";				break;
	default:					n = Str_VarArgs("0x%08X", pname);	break;
	}

	fprintf(qglState.logFile, "glTexGendv( %s, %s, %p )\n", c, n, params);
	dllTexGendv(coord, pname, params);
}

static GLvoid APIENTRY logTexGenf (GLenum coord, GLenum pname, GLfloat param){

	const char	*c, *n, *p;

	switch (coord){
	case GL_S:					c = "GL_S";						break;
	case GL_T:					c = "GL_T";						break;
	case GL_R:					c = "GL_R";						break;
	case GL_Q:					c = "GL_Q";						break;
	default:					c = Str_VarArgs("0x%08X", coord);	break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";		break;
	default:					n = Str_VarArgs("0x%08X", pname);	break;
	}

	switch ((int)param){
	case GL_OBJECT_LINEAR:		p = "GL_OBJECT_LINEAR";			break;
	case GL_EYE_LINEAR:			p = "GL_EYE_LINEAR";			break;
	case GL_SPHERE_MAP:			p = "GL_SPHERE_MAP";			break;
	case GL_NORMAL_MAP:			p = "GL_NORMAL_MAP";			break;
	case GL_REFLECTION_MAP:		p = "GL_REFLECTION_MAP";		break;
	default:					p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qglState.logFile, "glTexGenf( %s, %s, %s )\n", c, n, p);
	dllTexGenf(coord, pname, param);
}

static GLvoid APIENTRY logTexGenfv (GLenum coord, GLenum pname, const GLfloat *params){

	const char	*c, *n;

	switch (coord){
	case GL_S:					c = "GL_S";						break;
	case GL_T:					c = "GL_T";						break;
	case GL_R:					c = "GL_R";						break;
	case GL_Q:					c = "GL_Q";						break;
	default:					c = Str_VarArgs("0x%08X", coord);	break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";		break;
	case GL_OBJECT_PLANE:		n = "GL_OBJECT_PLANE";			break;
	case GL_EYE_PLANE:			n = "GL_EYE_PLANE";				break;
	default:					n = Str_VarArgs("0x%08X", pname);	break;
	}

	fprintf(qglState.logFile, "glTexGenfv( %s, %s, %p )\n", c, n, params);
	dllTexGenfv(coord, pname, params);
}

static GLvoid APIENTRY logTexGeni (GLenum coord, GLenum pname, GLint param){

	const char	*c, *n, *p;

	switch (coord){
	case GL_S:					c = "GL_S";						break;
	case GL_T:					c = "GL_T";						break;
	case GL_R:					c = "GL_R";						break;
	case GL_Q:					c = "GL_Q";						break;
	default:					c = Str_VarArgs("0x%08X", coord);	break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";		break;
	default:					n = Str_VarArgs("0x%08X", pname);	break;
	}

	switch (param){
	case GL_OBJECT_LINEAR:		p = "GL_OBJECT_LINEAR";			break;
	case GL_EYE_LINEAR:			p = "GL_EYE_LINEAR";			break;
	case GL_SPHERE_MAP:			p = "GL_SPHERE_MAP";			break;
	case GL_NORMAL_MAP:			p = "GL_NORMAL_MAP";			break;
	case GL_REFLECTION_MAP:		p = "GL_REFLECTION_MAP";		break;
	default:					p = Str_VarArgs("0x%08X", param);	break;
	}

	fprintf(qglState.logFile, "glTexGeni( %s, %s, %s )\n", c, n, p);
	dllTexGeni(coord, pname, param);
}

static GLvoid APIENTRY logTexGeniv (GLenum coord, GLenum pname, const GLint *params){

	const char	*c, *n;

	switch (coord){
	case GL_S:					c = "GL_S";						break;
	case GL_T:					c = "GL_T";						break;
	case GL_R:					c = "GL_R";						break;
	case GL_Q:					c = "GL_Q";						break;
	default:					c = Str_VarArgs("0x%08X", coord);	break;
	}

	switch (pname){
	case GL_TEXTURE_GEN_MODE:	n = "GL_TEXTURE_GEN_MODE";		break;
	case GL_OBJECT_PLANE:		n = "GL_OBJECT_PLANE";			break;
	case GL_EYE_PLANE:			n = "GL_EYE_PLANE";				break;
	default:					n = Str_VarArgs("0x%08X", pname);	break;
	}

	fprintf(qglState.logFile, "glTexGeniv( %s, %s, %p )\n", c, n, params);
	dllTexGeniv(coord, pname, params);
}

static GLvoid APIENTRY logTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels){

	const char	*t, *i, *f, *t2;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";						break;
	default:							t = Str_VarArgs("0x%08X", target);			break;
	}

	switch (internalformat){
	case GL_ALPHA8:						i = "GL_ALPHA8";							break;
	case GL_INTENSITY8:					i = "GL_INTENSITY8";						break;
	case GL_LUMINANCE8:					i = "GL_LUMINANCE8";						break;
	case GL_LUMINANCE8_ALPHA8:			i = "GL_LUMINANCE8_ALPHA8";					break;
	case GL_R8:							i = "GL_R8";								break;
	case GL_R16F:						i = "GL_R16F";								break;
	case GL_R32F:						i = "GL_R32F";								break;
	case GL_RG8:						i = "GL_RG8";								break;
	case GL_RG16F:						i = "GL_RG16F";								break;
	case GL_RG32F:						i = "GL_RG32F";								break;
	case GL_RGB8:						i = "GL_RGB8";								break;
	case GL_RGB16F:						i = "GL_RGB16F";							break;
	case GL_RGB32F:						i = "GL_RGB32F";							break;
	case GL_RGBA8:						i = "GL_RGBA8";								break;
	case GL_RGBA16F:					i = "GL_RGBA16F";							break;
	case GL_RGBA32F:					i = "GL_RGBA32F";							break;
	case GL_DEPTH_COMPONENT16:			i = "GL_DEPTH_COMPONENT16";					break;
	case GL_DEPTH_COMPONENT24:			i = "GL_DEPTH_COMPONENT24";					break;
	case GL_DEPTH_COMPONENT32:			i = "GL_DEPTH_COMPONENT32";					break;
	case GL_DEPTH_COMPONENT32F:			i = "GL_DEPTH_COMPONENT32F";				break;
	case GL_DEPTH24_STENCIL8:			i = "GL_DEPTH24_STENCIL8";					break;
	case GL_DEPTH32F_STENCIL8:			i = "GL_DEPTH32F_STENCIL8";					break;
	case GL_COMPRESSED_ALPHA:			i = "GL_COMPRESSED_ALPHA";					break;
	case GL_COMPRESSED_INTENSITY:		i = "GL_COMPRESSED_INTENSITY";				break;
	case GL_COMPRESSED_LUMINANCE:		i = "GL_COMPRESSED_LUMINANCE";				break;
	case GL_COMPRESSED_LUMINANCE_ALPHA:	i = "GL_COMPRESSED_LUMINANCE_ALPHA";		break;
	case GL_COMPRESSED_RED:				i = "GL_COMPRESSED_RED";					break;
	case GL_COMPRESSED_RG:				i = "GL_COMPRESSED_RG";						break;
	case GL_COMPRESSED_RGB:				i = "GL_COMPRESSED_RGB";					break;
	case GL_COMPRESSED_RGBA:			i = "GL_COMPRESSED_RGBA";					break;
	default:							i = Str_VarArgs("0x%08X", internalformat);	break;
	}

	switch (format){
	case GL_ALPHA:						f = "GL_ALPHA";								break;
	case GL_LUMINANCE:					f = "GL_LUMINANCE";							break;
	case GL_LUMINANCE_ALPHA:			f = "GL_LUMINANCE_ALPHA";					break;
	case GL_RGB:						f = "GL_RGB";								break;
	case GL_RGBA:						f = "GL_RGBA";								break;
	case GL_BGR:						f = "GL_BGR";								break;
	case GL_BGRA:						f = "GL_BGRA";								break;
	case GL_DEPTH_COMPONENT:			f = "GL_DEPTH_COMPONENT";					break;
	case GL_DEPTH_STENCIL:				f = "GL_DEPTH_STENCIL";						break;
	default:							f = Str_VarArgs("0x%08X", format);			break;
	}

	switch (type){
	case GL_BYTE:						t2 = "GL_BYTE";								break;
	case GL_UNSIGNED_BYTE:				t2 = "GL_UNSIGNED_BYTE";					break;
	case GL_SHORT:						t2 = "GL_SHORT";							break;
	case GL_UNSIGNED_SHORT:				t2 = "GL_UNSIGNED_SHORT";					break;
	case GL_INT:						t2 = "GL_INT";								break;
	case GL_UNSIGNED_INT:				t2 = "GL_UNSIGNED_INT";						break;
	case GL_UNSIGNED_INT_24_8:			t2 = "GL_UNSIGNED_INT_24_8";				break;
	case GL_FLOAT:						t2 = "GL_FLOAT";							break;
	case GL_HALF_FLOAT:					t2 = "GL_HALF_FLOAT";						break;
	case GL_BITMAP:						t2 = "GL_BITMAP";							break;
	default:							t2 = Str_VarArgs("0x%08X", type);				break;
	}

	fprintf(qglState.logFile, "glTexImage1D( %s, %i, %s, %i, %i, %s, %s, %p )\n", t, level, i, width, border, f, t2, pixels);
	dllTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}

static GLvoid APIENTRY logTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels){

	const char	*t, *i, *f, *t2;

	switch (target){
	case GL_TEXTURE_1D_ARRAY:				t = "GL_TEXTURE_1D_ARRAY";					break;
	case GL_TEXTURE_2D:						t = "GL_TEXTURE_2D";						break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";		break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";		break;
	case GL_TEXTURE_RECTANGLE:				t = "GL_TEXTURE_RECTANGLE";					break;
	default:								t = Str_VarArgs("0x%08X", target);			break;
	}

	switch (internalformat){
	case GL_ALPHA8:							i = "GL_ALPHA8";							break;
	case GL_INTENSITY8:						i = "GL_INTENSITY8";						break;
	case GL_LUMINANCE8:						i = "GL_LUMINANCE8";						break;
	case GL_LUMINANCE8_ALPHA8:				i = "GL_LUMINANCE8_ALPHA8";					break;
	case GL_R8:								i = "GL_R8";								break;
	case GL_R16F:							i = "GL_R16F";								break;
	case GL_R32F:							i = "GL_R32F";								break;
	case GL_RG8:							i = "GL_RG8";								break;
	case GL_RG16F:							i = "GL_RG16F";								break;
	case GL_RG32F:							i = "GL_RG32F";								break;
	case GL_RGB8:							i = "GL_RGB8";								break;
	case GL_RGB16F:							i = "GL_RGB16F";							break;
	case GL_RGB32F:							i = "GL_RGB32F";							break;
	case GL_RGBA8:							i = "GL_RGBA8";								break;
	case GL_RGBA16F:						i = "GL_RGBA16F";							break;
	case GL_RGBA32F:						i = "GL_RGBA32F";							break;
	case GL_DEPTH_COMPONENT16:				i = "GL_DEPTH_COMPONENT16";					break;
	case GL_DEPTH_COMPONENT24:				i = "GL_DEPTH_COMPONENT24";					break;
	case GL_DEPTH_COMPONENT32:				i = "GL_DEPTH_COMPONENT32";					break;
	case GL_DEPTH_COMPONENT32F:				i = "GL_DEPTH_COMPONENT32F";				break;
	case GL_DEPTH24_STENCIL8:				i = "GL_DEPTH24_STENCIL8";					break;
	case GL_DEPTH32F_STENCIL8:				i = "GL_DEPTH32F_STENCIL8";					break;
	case GL_COMPRESSED_ALPHA:				i = "GL_COMPRESSED_ALPHA";					break;
	case GL_COMPRESSED_INTENSITY:			i = "GL_COMPRESSED_INTENSITY";				break;
	case GL_COMPRESSED_LUMINANCE:			i = "GL_COMPRESSED_LUMINANCE";				break;
	case GL_COMPRESSED_LUMINANCE_ALPHA:		i = "GL_COMPRESSED_LUMINANCE_ALPHA";		break;
	case GL_COMPRESSED_RED:					i = "GL_COMPRESSED_RED";					break;
	case GL_COMPRESSED_RG:					i = "GL_COMPRESSED_RG";						break;
	case GL_COMPRESSED_RGB:					i = "GL_COMPRESSED_RGB";					break;
	case GL_COMPRESSED_RGBA:				i = "GL_COMPRESSED_RGBA";					break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:	i = "GL_COMPRESSED_RGB_S3TC_DXT1";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT1";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT3";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT5";			break;
	default:								i = Str_VarArgs("0x%08X", internalformat);	break;
	}

	switch (format){
	case GL_ALPHA:							f = "GL_ALPHA";								break;
	case GL_LUMINANCE:						f = "GL_LUMINANCE";							break;
	case GL_LUMINANCE_ALPHA:				f = "GL_LUMINANCE_ALPHA";					break;
	case GL_RGB:							f = "GL_RGB";								break;
	case GL_RGBA:							f = "GL_RGBA";								break;
	case GL_BGR:							f = "GL_BGR";								break;
	case GL_BGRA:							f = "GL_BGRA";								break;
	case GL_DEPTH_COMPONENT:				f = "GL_DEPTH_COMPONENT";					break;
	case GL_DEPTH_STENCIL:					f = "GL_DEPTH_STENCIL";						break;
	default:								f = Str_VarArgs("0x%08X", format);			break;
	}

	switch (type){
	case GL_BYTE:							t2 = "GL_BYTE";								break;
	case GL_UNSIGNED_BYTE:					t2 = "GL_UNSIGNED_BYTE";					break;
	case GL_SHORT:							t2 = "GL_SHORT";							break;
	case GL_UNSIGNED_SHORT:					t2 = "GL_UNSIGNED_SHORT";					break;
	case GL_INT:							t2 = "GL_INT";								break;
	case GL_UNSIGNED_INT:					t2 = "GL_UNSIGNED_INT";						break;
	case GL_UNSIGNED_INT_24_8:				t2 = "GL_UNSIGNED_INT_24_8";				break;
	case GL_FLOAT:							t2 = "GL_FLOAT";							break;
	case GL_HALF_FLOAT:						t2 = "GL_HALF_FLOAT";						break;
	case GL_BITMAP:							t2 = "GL_BITMAP";							break;
	default:								t2 = Str_VarArgs("0x%08X", type);				break;
	}

	fprintf(qglState.logFile, "glTexImage2D( %s, %i, %s, %i, %i, %i, %s, %s, %p )\n", t, level, i, width, height, border, f, t2, pixels);
	dllTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

static GLvoid APIENTRY logTexImage2DMultisample (GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations){

	const char	*t, *i, *f;

	switch (target){
	case GL_TEXTURE_2D_MULTISAMPLE:	t = "GL_TEXTURE_2D_MULTISAMPLE";				break;
	default:						t = Str_VarArgs("0x%08X", target);				break;
	}

	switch (internalformat){
	case GL_R8:						i = "GL_R8";									break;
	case GL_R16F:					i = "GL_R16F";									break;
	case GL_R32F:					i = "GL_R32F";									break;
	case GL_RG8:					i = "GL_RG8";									break;
	case GL_RG16F:					i = "GL_RG16F";									break;
	case GL_RG32F:					i = "GL_RG32F";									break;
	case GL_RGB8:					i = "GL_RGB8";									break;
	case GL_RGB16F:					i = "GL_RGB16F";								break;
	case GL_RGB32F:					i = "GL_RGB32F";								break;
	case GL_RGBA8:					i = "GL_RGBA8";									break;
	case GL_RGBA16F:				i = "GL_RGBA16F";								break;
	case GL_RGBA32F:				i = "GL_RGBA32F";								break;
	case GL_DEPTH_COMPONENT16:		i = "GL_DEPTH_COMPONENT16";						break;
	case GL_DEPTH_COMPONENT24:		i = "GL_DEPTH_COMPONENT24";						break;
	case GL_DEPTH_COMPONENT32:		i = "GL_DEPTH_COMPONENT32";						break;
	case GL_DEPTH_COMPONENT32F:		i = "GL_DEPTH_COMPONENT32F";					break;
	case GL_STENCIL_INDEX8:			i = "GL_STENCIL_INDEX8";						break;
	case GL_DEPTH24_STENCIL8:		i = "GL_DEPTH24_STENCIL8";						break;
	case GL_DEPTH32F_STENCIL8:		i = "GL_DEPTH32F_STENCIL8";						break;
	default:						i = Str_VarArgs("0x%08X", internalformat);		break;
	}

	switch (fixedsamplelocations){
	case GL_FALSE:					f = "GL_FALSE";									break;
	case GL_TRUE:					f = "GL_TRUE";									break;
	default:						f = Str_VarArgs("0x%08X", fixedsamplelocations);	break;
	}

	fprintf(qglState.logFile, "glTexImage2DMultisample( %s, %i, %s, %i, %i, %s )\n", t, samples, i, width, height, f);
	dllTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}

static GLvoid APIENTRY logTexImage3D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels){

	const char	*t, *i, *f, *t2;

	switch (target){
	case GL_TEXTURE_2D_ARRAY:				t = "GL_TEXTURE_2D_ARRAY";					break;
	case GL_TEXTURE_3D:						t = "GL_TEXTURE_3D";						break;
	default:								t = Str_VarArgs("0x%08X", target);			break;
	}

	switch (internalformat){
	case GL_ALPHA8:							i = "GL_ALPHA8";							break;
	case GL_INTENSITY8:						i = "GL_INTENSITY8";						break;
	case GL_LUMINANCE8:						i = "GL_LUMINANCE8";						break;
	case GL_LUMINANCE8_ALPHA8:				i = "GL_LUMINANCE8_ALPHA8";					break;
	case GL_R8:								i = "GL_R8";								break;
	case GL_R16F:							i = "GL_R16F";								break;
	case GL_R32F:							i = "GL_R32F";								break;
	case GL_RG8:							i = "GL_RG8";								break;
	case GL_RG16F:							i = "GL_RG16F";								break;
	case GL_RG32F:							i = "GL_RG32F";								break;
	case GL_RGB8:							i = "GL_RGB8";								break;
	case GL_RGB16F:							i = "GL_RGB16F";							break;
	case GL_RGB32F:							i = "GL_RGB32F";							break;
	case GL_RGBA8:							i = "GL_RGBA8";								break;
	case GL_RGBA16F:						i = "GL_RGBA16F";							break;
	case GL_RGBA32F:						i = "GL_RGBA32F";							break;
	case GL_DEPTH_COMPONENT16:				i = "GL_DEPTH_COMPONENT16";					break;
	case GL_DEPTH_COMPONENT24:				i = "GL_DEPTH_COMPONENT24";					break;
	case GL_DEPTH_COMPONENT32:				i = "GL_DEPTH_COMPONENT32";					break;
	case GL_DEPTH_COMPONENT32F:				i = "GL_DEPTH_COMPONENT32F";				break;
	case GL_DEPTH24_STENCIL8:				i = "GL_DEPTH24_STENCIL8";					break;
	case GL_DEPTH32F_STENCIL8:				i = "GL_DEPTH32F_STENCIL8";					break;
	case GL_COMPRESSED_ALPHA:				i = "GL_COMPRESSED_ALPHA";					break;
	case GL_COMPRESSED_INTENSITY:			i = "GL_COMPRESSED_INTENSITY";				break;
	case GL_COMPRESSED_LUMINANCE:			i = "GL_COMPRESSED_LUMINANCE";				break;
	case GL_COMPRESSED_LUMINANCE_ALPHA:		i = "GL_COMPRESSED_LUMINANCE_ALPHA";		break;
	case GL_COMPRESSED_RED:					i = "GL_COMPRESSED_RED";					break;
	case GL_COMPRESSED_RG:					i = "GL_COMPRESSED_RG";						break;
	case GL_COMPRESSED_RGB:					i = "GL_COMPRESSED_RGB";					break;
	case GL_COMPRESSED_RGBA:				i = "GL_COMPRESSED_RGBA";					break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:	i = "GL_COMPRESSED_RGB_S3TC_DXT1";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT1";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT3";			break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	i = "GL_COMPRESSED_RGBA_S3TC_DXT5";			break;
	default:								i = Str_VarArgs("0x%08X", internalformat);	break;
	}

	switch (format){
	case GL_ALPHA:							f = "GL_ALPHA";								break;
	case GL_LUMINANCE:						f = "GL_LUMINANCE";							break;
	case GL_LUMINANCE_ALPHA:				f = "GL_LUMINANCE_ALPHA";					break;
	case GL_RGB:							f = "GL_RGB";								break;
	case GL_RGBA:							f = "GL_RGBA";								break;
	case GL_BGR:							f = "GL_BGR";								break;
	case GL_BGRA:							f = "GL_BGRA";								break;
	case GL_DEPTH_COMPONENT:				f = "GL_DEPTH_COMPONENT";					break;
	case GL_DEPTH_STENCIL:					f = "GL_DEPTH_STENCIL";						break;
	default:								f = Str_VarArgs("0x%08X", format);			break;
	}

	switch (type){
	case GL_BYTE:							t2 = "GL_BYTE";								break;
	case GL_UNSIGNED_BYTE:					t2 = "GL_UNSIGNED_BYTE";					break;
	case GL_SHORT:							t2 = "GL_SHORT";							break;
	case GL_UNSIGNED_SHORT:					t2 = "GL_UNSIGNED_SHORT";					break;
	case GL_INT:							t2 = "GL_INT";								break;
	case GL_UNSIGNED_INT:					t2 = "GL_UNSIGNED_INT";						break;
	case GL_UNSIGNED_INT_24_8:				t2 = "GL_UNSIGNED_INT_24_8";				break;
	case GL_FLOAT:							t2 = "GL_FLOAT";							break;
	case GL_HALF_FLOAT:						t2 = "GL_HALF_FLOAT";						break;
	case GL_BITMAP:							t2 = "GL_BITMAP";							break;
	default:								t2 = Str_VarArgs("0x%08X", type);				break;
	}

	fprintf(qglState.logFile, "glTexImage3D( %s, %i, %s, %i, %i, %i, %i, %s, %s, %p )\n", t, level, i, width, height, depth, border, f, t2, pixels);
	dllTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

static GLvoid APIENTRY logTexImage3DMultisample (GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations){

	const char	*t, *i, *f;

	switch (target){
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:	t = "GL_TEXTURE_2D_MULTISAMPLE_ARRAY";			break;
	default:								t = Str_VarArgs("0x%08X", target);				break;
	}

	switch (internalformat){
	case GL_R8:								i = "GL_R8";									break;
	case GL_R16F:							i = "GL_R16F";									break;
	case GL_R32F:							i = "GL_R32F";									break;
	case GL_RG8:							i = "GL_RG8";									break;
	case GL_RG16F:							i = "GL_RG16F";									break;
	case GL_RG32F:							i = "GL_RG32F";									break;
	case GL_RGB8:							i = "GL_RGB8";									break;
	case GL_RGB16F:							i = "GL_RGB16F";								break;
	case GL_RGB32F:							i = "GL_RGB32F";								break;
	case GL_RGBA8:							i = "GL_RGBA8";									break;
	case GL_RGBA16F:						i = "GL_RGBA16F";								break;
	case GL_RGBA32F:						i = "GL_RGBA32F";								break;
	case GL_DEPTH_COMPONENT16:				i = "GL_DEPTH_COMPONENT16";						break;
	case GL_DEPTH_COMPONENT24:				i = "GL_DEPTH_COMPONENT24";						break;
	case GL_DEPTH_COMPONENT32:				i = "GL_DEPTH_COMPONENT32";						break;
	case GL_DEPTH_COMPONENT32F:				i = "GL_DEPTH_COMPONENT32F";					break;
	case GL_STENCIL_INDEX8:					i = "GL_STENCIL_INDEX8";						break;
	case GL_DEPTH24_STENCIL8:				i = "GL_DEPTH24_STENCIL8";						break;
	case GL_DEPTH32F_STENCIL8:				i = "GL_DEPTH32F_STENCIL8";						break;
	default:								i = Str_VarArgs("0x%08X", internalformat);		break;
	}

	switch (fixedsamplelocations){
	case GL_FALSE:							f = "GL_FALSE";									break;
	case GL_TRUE:							f = "GL_TRUE";									break;
	default:								f = Str_VarArgs("0x%08X", fixedsamplelocations);	break;
	}

	fprintf(qglState.logFile, "glTexImage3DMultisample( %s, %i, %s, %i, %i, %i, %s )\n", t, samples, i, width, height, depth, f);
	dllTexImage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}

static GLvoid APIENTRY logTexParameterIiv (GLenum target, GLenum pname, const GLint *params){

	const char	*t, *n;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_1D_ARRAY:			t = "GL_TEXTURE_1D_ARRAY";			break;
	case GL_TEXTURE_2D:					t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_2D_ARRAY:			t = "GL_TEXTURE_2D_ARRAY";			break;
	case GL_TEXTURE_3D:					t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP:			t = "GL_TEXTURE_CUBE_MAP";			break;
	case GL_TEXTURE_RECTANGLE:			t = "GL_TEXTURE_RECTANGLE";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_BORDER_COLOR:		n = "GL_TEXTURE_BORDER_COLOR";		break;
	case GL_TEXTURE_PRIORITY:			n = "GL_TEXTURE_PRIORITY";			break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	fprintf(qglState.logFile, "glTexParameterIiv( %s, %s, %p )\n", t, n, params);
	dllTexParameterIiv(target, pname, params);
}

static GLvoid APIENTRY logTexParameterIuiv (GLenum target, GLenum pname, const GLuint *params){

	const char	*t, *n;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_1D_ARRAY:			t = "GL_TEXTURE_1D_ARRAY";			break;
	case GL_TEXTURE_2D:					t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_2D_ARRAY:			t = "GL_TEXTURE_2D_ARRAY";			break;
	case GL_TEXTURE_3D:					t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP:			t = "GL_TEXTURE_CUBE_MAP";			break;
	case GL_TEXTURE_RECTANGLE:			t = "GL_TEXTURE_RECTANGLE";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_BORDER_COLOR:		n = "GL_TEXTURE_BORDER_COLOR";		break;
	case GL_TEXTURE_PRIORITY:			n = "GL_TEXTURE_PRIORITY";			break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	fprintf(qglState.logFile, "glTexParameterIuiv( %s, %s, %p )\n", t, n, params);
	dllTexParameterIuiv(target, pname, params);
}

static GLvoid APIENTRY logTexParameterf (GLenum target, GLenum pname, GLfloat param){

	const char	*t, *n, *p;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_1D_ARRAY:			t = "GL_TEXTURE_1D_ARRAY";			break;
	case GL_TEXTURE_2D:					t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_2D_ARRAY:			t = "GL_TEXTURE_2D_ARRAY";			break;
	case GL_TEXTURE_3D:					t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP:			t = "GL_TEXTURE_CUBE_MAP";			break;
	case GL_TEXTURE_RECTANGLE:			t = "GL_TEXTURE_RECTANGLE";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	switch ((int)param){
	case GL_NEAREST:					p = "GL_NEAREST";					break;
	case GL_LINEAR:						p = "GL_LINEAR";					break;
	case GL_NEAREST_MIPMAP_NEAREST:		p = "GL_NEAREST_MIPMAP_NEAREST";	break;
	case GL_LINEAR_MIPMAP_NEAREST:		p = "GL_LINEAR_MIPMAP_NEAREST";		break;
	case GL_NEAREST_MIPMAP_LINEAR:		p = "GL_NEAREST_MIPMAP_LINEAR";		break;
	case GL_LINEAR_MIPMAP_LINEAR:		p = "GL_LINEAR_MIPMAP_LINEAR";		break;
	case GL_REPEAT:						p = "GL_REPEAT";					break;
	case GL_MIRRORED_REPEAT:			p = "GL_MIRRORED_REPEAT";			break;
	case GL_CLAMP:						p = "GL_CLAMP";						break;
	case GL_CLAMP_TO_EDGE:				p = "GL_CLAMP_TO_EDGE";				break;
	case GL_CLAMP_TO_BORDER:			p = "GL_CLAMP_TO_BORDER";			break;
	case GL_RED:						p = "GL_RED";						break;
	case GL_GREEN:						p = "GL_GREEN";						break;
	case GL_BLUE:						p = "GL_BLUE";						break;
	case GL_ALPHA:						p = "GL_ALPHA";						break;
	case GL_ZERO:						p = "GL_ZERO";						break;
	case GL_ONE:						p = "GL_ONE";						break;
	case GL_COMPARE_REF_TO_TEXTURE:		p = "GL_COMPARE_REF_TO_TEXTURE";	break;
	case GL_NEVER:						p = "GL_NEVER";						break;
	case GL_LESS:						p = "GL_LESS";						break;
	case GL_LEQUAL:						p = "GL_LEQUAL";					break;
	case GL_EQUAL:						p = "GL_EQUAL";						break;
	case GL_NOTEQUAL:					p = "GL_NOTEQUAL";					break;
	case GL_GEQUAL:						p = "GL_GEQUAL";					break;
	case GL_GREATER:					p = "GL_GREATER";					break;
	case GL_ALWAYS:						p = "GL_ALWAYS";					break;
	case GL_INTENSITY:					p = "GL_INTENSITY";					break;
	case GL_LUMINANCE:					p = "GL_LUMINANCE";					break;
	default:							p = Str_VarArgs("0x%08X", param);		break;
	}

	if (pname == GL_GENERATE_MIPMAP){
		if ((int)param == GL_FALSE)
			p = "GL_FALSE";
		else
			p = "GL_TRUE";
	}

	if (pname == GL_TEXTURE_MIN_LOD || pname == GL_TEXTURE_MAX_LOD || pname == GL_TEXTURE_BASE_LEVEL || pname == GL_TEXTURE_MAX_LEVEL || pname == GL_TEXTURE_LOD_BIAS || pname == GL_TEXTURE_MAX_ANISOTROPY_EXT)
		p = Str_VarArgs("%g", param);

	if (pname == GL_TEXTURE_COMPARE_MODE && (int)param == GL_NONE)
		p = "GL_NONE";

	fprintf(qglState.logFile, "glTexParameterf( %s, %s, %s )\n", t, n, p);
	dllTexParameterf(target, pname, param);
}

static GLvoid APIENTRY logTexParameterfv (GLenum target, GLenum pname, const GLfloat *params){

	const char	*t, *n;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_1D_ARRAY:			t = "GL_TEXTURE_1D_ARRAY";			break;
	case GL_TEXTURE_2D:					t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_2D_ARRAY:			t = "GL_TEXTURE_2D_ARRAY";			break;
	case GL_TEXTURE_3D:					t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP:			t = "GL_TEXTURE_CUBE_MAP";			break;
	case GL_TEXTURE_RECTANGLE:			t = "GL_TEXTURE_RECTANGLE";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_BORDER_COLOR:		n = "GL_TEXTURE_BORDER_COLOR";		break;
	case GL_TEXTURE_PRIORITY:			n = "GL_TEXTURE_PRIORITY";			break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	fprintf(qglState.logFile, "glTexParameterfv( %s, %s, %p )\n", t, n, params);
	dllTexParameterfv(target, pname, params);
}

static GLvoid APIENTRY logTexParameteri (GLenum target, GLenum pname, GLint param){

	const char	*t, *n, *p;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_1D_ARRAY:			t = "GL_TEXTURE_1D_ARRAY";			break;
	case GL_TEXTURE_2D:					t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_2D_ARRAY:			t = "GL_TEXTURE_2D_ARRAY";			break;
	case GL_TEXTURE_3D:					t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP:			t = "GL_TEXTURE_CUBE_MAP";			break;
	case GL_TEXTURE_RECTANGLE:			t = "GL_TEXTURE_RECTANGLE";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	switch (param){
	case GL_NEAREST:					p = "GL_NEAREST";					break;
	case GL_LINEAR:						p = "GL_LINEAR";					break;
	case GL_NEAREST_MIPMAP_NEAREST:		p = "GL_NEAREST_MIPMAP_NEAREST";	break;
	case GL_LINEAR_MIPMAP_NEAREST:		p = "GL_LINEAR_MIPMAP_NEAREST";		break;
	case GL_NEAREST_MIPMAP_LINEAR:		p = "GL_NEAREST_MIPMAP_LINEAR";		break;
	case GL_LINEAR_MIPMAP_LINEAR:		p = "GL_LINEAR_MIPMAP_LINEAR";		break;
	case GL_REPEAT:						p = "GL_REPEAT";					break;
	case GL_MIRRORED_REPEAT:			p = "GL_MIRRORED_REPEAT";			break;
	case GL_CLAMP:						p = "GL_CLAMP";						break;
	case GL_CLAMP_TO_EDGE:				p = "GL_CLAMP_TO_EDGE";				break;
	case GL_CLAMP_TO_BORDER:			p = "GL_CLAMP_TO_BORDER";			break;
	case GL_RED:						p = "GL_RED";						break;
	case GL_GREEN:						p = "GL_GREEN";						break;
	case GL_BLUE:						p = "GL_BLUE";						break;
	case GL_ALPHA:						p = "GL_ALPHA";						break;
	case GL_ZERO:						p = "GL_ZERO";						break;
	case GL_ONE:						p = "GL_ONE";						break;
	case GL_COMPARE_REF_TO_TEXTURE:		p = "GL_COMPARE_REF_TO_TEXTURE";	break;
	case GL_NEVER:						p = "GL_NEVER";						break;
	case GL_LESS:						p = "GL_LESS";						break;
	case GL_LEQUAL:						p = "GL_LEQUAL";					break;
	case GL_EQUAL:						p = "GL_EQUAL";						break;
	case GL_NOTEQUAL:					p = "GL_NOTEQUAL";					break;
	case GL_GEQUAL:						p = "GL_GEQUAL";					break;
	case GL_GREATER:					p = "GL_GREATER";					break;
	case GL_ALWAYS:						p = "GL_ALWAYS";					break;
	case GL_INTENSITY:					p = "GL_INTENSITY";					break;
	case GL_LUMINANCE:					p = "GL_LUMINANCE";					break;
	default:							p = Str_VarArgs("0x%08X", param);		break;
	}

	if (pname == GL_GENERATE_MIPMAP){
		if (param == GL_FALSE)
			p = "GL_FALSE";
		else
			p = "GL_TRUE";
	}

	if (pname == GL_TEXTURE_MIN_LOD || pname == GL_TEXTURE_MAX_LOD || pname == GL_TEXTURE_BASE_LEVEL || pname == GL_TEXTURE_MAX_LEVEL || pname == GL_TEXTURE_LOD_BIAS || pname == GL_TEXTURE_MAX_ANISOTROPY_EXT)
		p = Str_VarArgs("%i", param);

	if (pname == GL_TEXTURE_COMPARE_MODE && param == GL_NONE)
		p = "GL_NONE";

	fprintf(qglState.logFile, "glTexParameteri( %s, %s, %s )\n", t, n, p);
	dllTexParameteri(target, pname, param);
}

static GLvoid APIENTRY logTexParameteriv (GLenum target, GLenum pname, const GLint *params){

	const char	*t, *n;

	switch (target){
	case GL_TEXTURE_1D:					t = "GL_TEXTURE_1D";				break;
	case GL_TEXTURE_1D_ARRAY:			t = "GL_TEXTURE_1D_ARRAY";			break;
	case GL_TEXTURE_2D:					t = "GL_TEXTURE_2D";				break;
	case GL_TEXTURE_2D_ARRAY:			t = "GL_TEXTURE_2D_ARRAY";			break;
	case GL_TEXTURE_3D:					t = "GL_TEXTURE_3D";				break;
	case GL_TEXTURE_CUBE_MAP:			t = "GL_TEXTURE_CUBE_MAP";			break;
	case GL_TEXTURE_RECTANGLE:			t = "GL_TEXTURE_RECTANGLE";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (pname){
	case GL_GENERATE_MIPMAP:			n = "GL_GENERATE_MIPMAP";			break;
	case GL_TEXTURE_MIN_FILTER:			n = "GL_TEXTURE_MIN_FILTER";		break;
	case GL_TEXTURE_MAG_FILTER:			n = "GL_TEXTURE_MAG_FILTER";		break;
	case GL_TEXTURE_BORDER_COLOR:		n = "GL_TEXTURE_BORDER_COLOR";		break;
	case GL_TEXTURE_PRIORITY:			n = "GL_TEXTURE_PRIORITY";			break;
	case GL_TEXTURE_MIN_LOD:			n = "GL_TEXTURE_MIN_LOD";			break;
	case GL_TEXTURE_MAX_LOD:			n = "GL_TEXTURE_MAX_LOD";			break;
	case GL_TEXTURE_BASE_LEVEL:			n = "GL_TEXTURE_BASE_LEVEL";		break;
	case GL_TEXTURE_MAX_LEVEL:			n = "GL_TEXTURE_MAX_LEVEL";			break;
	case GL_TEXTURE_LOD_BIAS:			n = "GL_TEXTURE_LOD_BIAS";			break;
	case GL_TEXTURE_WRAP_S:				n = "GL_TEXTURE_WRAP_S";			break;
	case GL_TEXTURE_WRAP_T:				n = "GL_TEXTURE_WRAP_T";			break;
	case GL_TEXTURE_WRAP_R:				n = "GL_TEXTURE_WRAP_R";			break;
	case GL_TEXTURE_SWIZZLE_R:			n = "GL_TEXTURE_SWIZZLE_R";			break;
	case GL_TEXTURE_SWIZZLE_G:			n = "GL_TEXTURE_SWIZZLE_G";			break;
	case GL_TEXTURE_SWIZZLE_B:			n = "GL_TEXTURE_SWIZZLE_B";			break;
	case GL_TEXTURE_SWIZZLE_A:			n = "GL_TEXTURE_SWIZZLE_A";			break;
	case GL_TEXTURE_SWIZZLE_RGBA:		n = "GL_TEXTURE_SWIZZLE_RGBA";		break;
	case GL_TEXTURE_COMPARE_MODE:		n = "GL_TEXTURE_COMPARE_MODE";		break;
	case GL_TEXTURE_COMPARE_FUNC:		n = "GL_TEXTURE_COMPARE_FUNC";		break;
	case GL_DEPTH_TEXTURE_MODE:			n = "GL_DEPTH_TEXTURE_MODE";		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:	n = "GL_TEXTURE_MAX_ANISOTROPY";	break;
	default:							n = Str_VarArgs("0x%08X", pname);		break;
	}

	fprintf(qglState.logFile, "glTexParameteriv( %s, %s, %p )\n", t, n, params);
	dllTexParameteriv(target, pname, params);
}

static GLvoid APIENTRY logTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels){

	const char	*t, *f, *t2;

	switch (target){
	case GL_TEXTURE_1D:			t = "GL_TEXTURE_1D";				break;
	default:					t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (format){
	case GL_ALPHA:				f = "GL_ALPHA";						break;
	case GL_LUMINANCE:			f = "GL_LUMINANCE";					break;
	case GL_LUMINANCE_ALPHA:	f = "GL_LUMINANCE_ALPHA";			break;
	case GL_RGB:				f = "GL_RGB";						break;
	case GL_RGBA:				f = "GL_RGBA";						break;
	case GL_BGR:				f = "GL_BGR";						break;
	case GL_BGRA:				f = "GL_BGRA";						break;
	case GL_DEPTH_COMPONENT:	f = "GL_DEPTH_COMPONENT";			break;
	case GL_DEPTH_STENCIL:		f = "GL_DEPTH_STENCIL";				break;
	default:					f = Str_VarArgs("0x%08X", format);	break;
	}

	switch (type){
	case GL_BYTE:				t2 = "GL_BYTE";						break;
	case GL_UNSIGNED_BYTE:		t2 = "GL_UNSIGNED_BYTE";			break;
	case GL_SHORT:				t2 = "GL_SHORT";					break;
	case GL_UNSIGNED_SHORT:		t2 = "GL_UNSIGNED_SHORT";			break;
	case GL_INT:				t2 = "GL_INT";						break;
	case GL_UNSIGNED_INT:		t2 = "GL_UNSIGNED_INT";				break;
	case GL_UNSIGNED_INT_24_8:	t2 = "GL_UNSIGNED_INT_24_8";		break;
	case GL_FLOAT:				t2 = "GL_FLOAT";					break;
	case GL_HALF_FLOAT:			t2 = "GL_HALF_FLOAT";				break;
	case GL_BITMAP:				t2 = "GL_BITMAP";					break;
	default:					t2 = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glTexSubImage1D( %s, %i, %i, %i, %s, %s, %p )\n", t, level, xoffset, width, f, t2, pixels);
	dllTexSubImage1D(target, level, xoffset, width, format, type, pixels);
}

static GLvoid APIENTRY logTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels){

	const char	*t, *f, *t2;

	switch (target){
	case GL_TEXTURE_1D_ARRAY:				t = "GL_TEXTURE_1D_ARRAY";				break;
	case GL_TEXTURE_2D:						t = "GL_TEXTURE_2D";					break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_POSITIVE_Z";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_X";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y";	break;
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:	t = "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z";	break;
	case GL_TEXTURE_RECTANGLE:				t = "GL_TEXTURE_RECTANGLE";				break;
	default:								t = Str_VarArgs("0x%08X", target);		break;
	}

	switch (format){
	case GL_ALPHA:							f = "GL_ALPHA";							break;
	case GL_LUMINANCE:						f = "GL_LUMINANCE";						break;
	case GL_LUMINANCE_ALPHA:				f = "GL_LUMINANCE_ALPHA";				break;
	case GL_RGB:							f = "GL_RGB";							break;
	case GL_RGBA:							f = "GL_RGBA";							break;
	case GL_BGR:							f = "GL_BGR";							break;
	case GL_BGRA:							f = "GL_BGRA";							break;
	case GL_DEPTH_COMPONENT:				f = "GL_DEPTH_COMPONENT";				break;
	case GL_DEPTH_STENCIL:					f = "GL_DEPTH_STENCIL";					break;
	default:								f = Str_VarArgs("0x%08X", format);		break;
	}

	switch (type){
	case GL_BYTE:							t2 = "GL_BYTE";							break;
	case GL_UNSIGNED_BYTE:					t2 = "GL_UNSIGNED_BYTE";				break;
	case GL_SHORT:							t2 = "GL_SHORT";						break;
	case GL_UNSIGNED_SHORT:					t2 = "GL_UNSIGNED_SHORT";				break;
	case GL_INT:							t2 = "GL_INT";							break;
	case GL_UNSIGNED_INT:					t2 = "GL_UNSIGNED_INT";					break;
	case GL_UNSIGNED_INT_24_8:				t2 = "GL_UNSIGNED_INT_24_8";			break;
	case GL_FLOAT:							t2 = "GL_FLOAT";						break;
	case GL_HALF_FLOAT:						t2 = "GL_HALF_FLOAT";					break;
	case GL_BITMAP:							t2 = "GL_BITMAP";						break;
	default:								t2 = Str_VarArgs("0x%08X", type);			break;
	}

	fprintf(qglState.logFile, "glTexSubImage2D( %s, %i, %i, %i, %i, %i, %s, %s, %p )\n", t, level, xoffset, yoffset, width, height, f, t2, pixels);
	dllTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

static GLvoid APIENTRY logTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels){

	const char	*t, *f, *t2;

	switch (target){
	case GL_TEXTURE_2D_ARRAY:	t = "GL_TEXTURE_2D_ARRAY";			break;
	case GL_TEXTURE_3D:			t = "GL_TEXTURE_3D";				break;
	default:					t = Str_VarArgs("0x%08X", target);	break;
	}

	switch (format){
	case GL_ALPHA:				f = "GL_ALPHA";						break;
	case GL_LUMINANCE:			f = "GL_LUMINANCE";					break;
	case GL_LUMINANCE_ALPHA:	f = "GL_LUMINANCE_ALPHA";			break;
	case GL_RGB:				f = "GL_RGB";						break;
	case GL_RGBA:				f = "GL_RGBA";						break;
	case GL_BGR:				f = "GL_BGR";						break;
	case GL_BGRA:				f = "GL_BGRA";						break;
	case GL_DEPTH_COMPONENT:	f = "GL_DEPTH_COMPONENT";			break;
	case GL_DEPTH_STENCIL:		f = "GL_DEPTH_STENCIL";				break;
	default:					f = Str_VarArgs("0x%08X", format);	break;
	}

	switch (type){
	case GL_BYTE:				t2 = "GL_BYTE";						break;
	case GL_UNSIGNED_BYTE:		t2 = "GL_UNSIGNED_BYTE";			break;
	case GL_SHORT:				t2 = "GL_SHORT";					break;
	case GL_UNSIGNED_SHORT:		t2 = "GL_UNSIGNED_SHORT";			break;
	case GL_INT:				t2 = "GL_INT";						break;
	case GL_UNSIGNED_INT:		t2 = "GL_UNSIGNED_INT";				break;
	case GL_UNSIGNED_INT_24_8:	t2 = "GL_UNSIGNED_INT_24_8";		break;
	case GL_FLOAT:				t2 = "GL_FLOAT";					break;
	case GL_HALF_FLOAT:			t2 = "GL_HALF_FLOAT";				break;
	case GL_BITMAP:				t2 = "GL_BITMAP";					break;
	default:					t2 = Str_VarArgs("0x%08X", type);		break;
	}

	fprintf(qglState.logFile, "glTexSubImage3D( %s, %i, %i, %i, %i, %i, %i, %i, %s, %s, %p )\n", t, level, xoffset, yoffset, zoffset, width, height, depth, f, t2, pixels);
	dllTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

static GLvoid APIENTRY logTransformFeedbackVaryings (GLuint program, GLsizei count, const GLchar **varyings, GLenum bufferMode){

	fprintf(qglState.logFile, "glTransformFeedbackVaryings( %u, %i, %p, 0x%08X )\n", program, count, varyings, bufferMode);
	dllTransformFeedbackVaryings(program, count, varyings, bufferMode);
}

static GLvoid APIENTRY logTranslated (GLdouble x, GLdouble y, GLdouble z){

	fprintf(qglState.logFile, "glTranslated( %g, %g, %g )\n", x, y, z);
	dllTranslated(x, y, z);
}

static GLvoid APIENTRY logTranslatef (GLfloat x, GLfloat y, GLfloat z){

	fprintf(qglState.logFile, "glTranslatef( %g, %g, %g )\n", x, y, z);
	dllTranslatef(x, y, z);
}

static GLvoid APIENTRY logUniform1f (GLint location, GLfloat v0){

	fprintf(qglState.logFile, "glUniform1f( %i, %g )\n", location, v0);
	dllUniform1f(location, v0);
}

static GLvoid APIENTRY logUniform1fv (GLint location, GLsizei count, const GLfloat *value){

	fprintf(qglState.logFile, "glUniform1fv( %i, %i, %p )\n", location, count, value);
	dllUniform1fv(location, count, value);
}

static GLvoid APIENTRY logUniform1i (GLint location, GLint v0){

	fprintf(qglState.logFile, "glUniform1i( %i, %i )\n", location, v0);
	dllUniform1i(location, v0);
}

static GLvoid APIENTRY logUniform1iv (GLint location, GLsizei count, const GLint *value){

	fprintf(qglState.logFile, "glUniform1iv( %i, %i, %p )\n", location, count, value);
	dllUniform1iv(location, count, value);
}

static GLvoid APIENTRY logUniform1ui (GLint location, GLuint v0){

	fprintf(qglState.logFile, "glUniform1ui( %i, %u )\n", location, v0);
	dllUniform1ui(location, v0);
}

static GLvoid APIENTRY logUniform1uiv (GLint location, GLsizei count, const GLuint *v){

	fprintf(qglState.logFile, "glUniform1uiv( %i, %i, %p )\n", location, count, v);
	dllUniform1uiv(location, count, v);
}

static GLvoid APIENTRY logUniform2f (GLint location, GLfloat v0, GLfloat v1){

	fprintf(qglState.logFile, "glUniform2f( %i, %g, %g )\n", location, v0, v1);
	dllUniform2f(location, v0, v1);
}

static GLvoid APIENTRY logUniform2fv (GLint location, GLsizei count, const GLfloat *value){

	fprintf(qglState.logFile, "glUniform2fv( %i, %i, %p )\n", location, count, value);
	dllUniform2fv(location, count, value);
}

static GLvoid APIENTRY logUniform2i (GLint location, GLint v0, GLint v1){

	fprintf(qglState.logFile, "glUniform2i( %i, %i, %i )\n", location, v0, v1);
	dllUniform2i(location, v0, v1);
}

static GLvoid APIENTRY logUniform2iv (GLint location, GLsizei count, const GLint *value){

	fprintf(qglState.logFile, "glUniform2iv( %i, %i, %p )\n", location, count, value);
	dllUniform2iv(location, count, value);
}

static GLvoid APIENTRY logUniform2ui (GLint location, GLuint v0, GLuint v1){

	fprintf(qglState.logFile, "glUniform2ui( %i, %u, %u )\n", location, v0, v1);
	dllUniform2ui(location, v0, v1);
}

static GLvoid APIENTRY logUniform2uiv (GLint location, GLsizei count, const GLuint *v){

	fprintf(qglState.logFile, "glUniform2uiv( %i, %i, %p )\n", location, count, v);
	dllUniform2uiv(location, count, v);
}

static GLvoid APIENTRY logUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2){

	fprintf(qglState.logFile, "glUniform3f( %i, %g, %g, %g )\n", location, v0, v1, v2);
	dllUniform3f(location, v0, v1, v2);
}

static GLvoid APIENTRY logUniform3fv (GLint location, GLsizei count, const GLfloat *value){

	fprintf(qglState.logFile, "glUniform3fv( %i, %i, %p )\n", location, count, value);
	dllUniform3fv(location, count, value);
}

static GLvoid APIENTRY logUniform3i (GLint location, GLint v0, GLint v1, GLint v2){

	fprintf(qglState.logFile, "glUniform3i( %i, %i, %i, %i )\n", location, v0, v1, v2);
	dllUniform3i(location, v0, v1, v2);
}

static GLvoid APIENTRY logUniform3iv (GLint location, GLsizei count, const GLint *value){

	fprintf(qglState.logFile, "glUniform3iv( %i, %i, %p )\n", location, count, value);
	dllUniform3iv(location, count, value);
}

static GLvoid APIENTRY logUniform3ui (GLint location, GLuint v0, GLuint v1, GLuint v2){

	fprintf(qglState.logFile, "glUniform3ui( %i, %u, %u, %u )\n", location, v0, v1, v2);
	dllUniform3ui(location, v0, v1, v2);
}

static GLvoid APIENTRY logUniform3uiv (GLint location, GLsizei count, const GLuint *v){

	fprintf(qglState.logFile, "glUniform3uiv( %i, %i, %p )\n", location, count, v);
	dllUniform3uiv(location, count, v);
}

static GLvoid APIENTRY logUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3){

	fprintf(qglState.logFile, "glUniform4f( %i, %g, %g, %g, %g )\n", location, v0, v1, v2, v3);
	dllUniform4f(location, v0, v1, v2, v3);
}

static GLvoid APIENTRY logUniform4fv (GLint location, GLsizei count, const GLfloat *value){

	fprintf(qglState.logFile, "glUniform4fv( %i, %i, %p )\n", location, count, value);
	dllUniform4fv(location, count, value);
}

static GLvoid APIENTRY logUniform4i (GLint location, GLint v0, GLint v1, GLint v2, GLint v3){

	fprintf(qglState.logFile, "glUniform4i( %i, %i, %i, %i, %i )\n", location, v0, v1, v2, v3);
	dllUniform4i(location, v0, v1, v2, v3);
}

static GLvoid APIENTRY logUniform4iv (GLint location, GLsizei count, const GLint *value){

	fprintf(qglState.logFile, "glUniform4iv( %i, %i, %p )\n", location, count, value);
	dllUniform4iv(location, count, value);
}

static GLvoid APIENTRY logUniform4ui (GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3){

	fprintf(qglState.logFile, "glUniform4ui( %i, %u, %u, %u, %u )\n", location, v0, v1, v2, v3);
	dllUniform4ui(location, v0, v1, v2, v3);
}

static GLvoid APIENTRY logUniform4uiv (GLint location, GLsizei count, const GLuint *v){

	fprintf(qglState.logFile, "glUniform4uiv( %i, %i, %p )\n", location, count, v);
	dllUniform4uiv(location, count, v);
}

static GLvoid APIENTRY logUniformBlockBinding (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding){

	fprintf(qglState.logFile, "glUniformBlockBinding( %u, %u, %u )\n", program, uniformBlockIndex, uniformBlockBinding);
	dllUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
}

static GLvoid APIENTRY logUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){

	const char	*t;

	switch (transpose){
	case GL_FALSE:	t = "GL_FALSE";						break;
	case GL_TRUE:	t = "GL_TRUE";						break;
	default:		t = Str_VarArgs("0x%08X", transpose);	break;
	}

	fprintf(qglState.logFile, "glUniformMatrix2fv( %i, %i, %s, %p )\n", location, count, t, value);
	dllUniformMatrix2fv(location, count, transpose, value);
}

static GLvoid APIENTRY logUniformMatrix2x3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){

	const char	*t;

	switch (transpose){
	case GL_FALSE:	t = "GL_FALSE";						break;
	case GL_TRUE:	t = "GL_TRUE";						break;
	default:		t = Str_VarArgs("0x%08X", transpose);	break;
	}

	fprintf(qglState.logFile, "glUniformMatrix2x3fv( %i, %i, %s, %p )\n", location, count, t, value);
	dllUniformMatrix2x3fv(location, count, transpose, value);
}

static GLvoid APIENTRY logUniformMatrix2x4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){

	const char	*t;

	switch (transpose){
	case GL_FALSE:	t = "GL_FALSE";						break;
	case GL_TRUE:	t = "GL_TRUE";						break;
	default:		t = Str_VarArgs("0x%08X", transpose);	break;
	}

	fprintf(qglState.logFile, "glUniformMatrix2x4fv( %i, %i, %s, %p )\n", location, count, t, value);
	dllUniformMatrix2x4fv(location, count, transpose, value);
}

static GLvoid APIENTRY logUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){

	const char	*t;

	switch (transpose){
	case GL_FALSE:	t = "GL_FALSE";						break;
	case GL_TRUE:	t = "GL_TRUE";						break;
	default:		t = Str_VarArgs("0x%08X", transpose);	break;
	}

	fprintf(qglState.logFile, "glUniformMatrix3fv( %i, %i, %s, %p )\n", location, count, t, value);
	dllUniformMatrix3fv(location, count, transpose, value);
}

static GLvoid APIENTRY logUniformMatrix3x2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){

	const char	*t;

	switch (transpose){
	case GL_FALSE:	t = "GL_FALSE";						break;
	case GL_TRUE:	t = "GL_TRUE";						break;
	default:		t = Str_VarArgs("0x%08X", transpose);	break;
	}

	fprintf(qglState.logFile, "glUniformMatrix3x2fv( %i, %i, %s, %p )\n", location, count, t, value);
	dllUniformMatrix3x2fv(location, count, transpose, value);
}

static GLvoid APIENTRY logUniformMatrix3x4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){

	const char	*t;

	switch (transpose){
	case GL_FALSE:	t = "GL_FALSE";						break;
	case GL_TRUE:	t = "GL_TRUE";						break;
	default:		t = Str_VarArgs("0x%08X", transpose);	break;
	}

	fprintf(qglState.logFile, "glUniformMatrix3x4fv( %i, %i, %s, %p )\n", location, count, t, value);
	dllUniformMatrix3x4fv(location, count, transpose, value);
}

static GLvoid APIENTRY logUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){

	const char	*t;

	switch (transpose){
	case GL_FALSE:	t = "GL_FALSE";						break;
	case GL_TRUE:	t = "GL_TRUE";						break;
	default:		t = Str_VarArgs("0x%08X", transpose);	break;
	}

	fprintf(qglState.logFile, "glUniformMatrix4fv( %i, %i, %s, %p )\n", location, count, t, value);
	dllUniformMatrix4fv(location, count, transpose, value);
}

static GLvoid APIENTRY logUniformMatrix4x2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){

	const char	*t;

	switch (transpose){
	case GL_FALSE:	t = "GL_FALSE";						break;
	case GL_TRUE:	t = "GL_TRUE";						break;
	default:		t = Str_VarArgs("0x%08X", transpose);	break;
	}

	fprintf(qglState.logFile, "glUniformMatrix4x2fv( %i, %i, %s, %p )\n", location, count, t, value);
	dllUniformMatrix4x2fv(location, count, transpose, value);
}

static GLvoid APIENTRY logUniformMatrix4x3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value){

	const char	*t;

	switch (transpose){
	case GL_FALSE:	t = "GL_FALSE";						break;
	case GL_TRUE:	t = "GL_TRUE";						break;
	default:		t = Str_VarArgs("0x%08X", transpose);	break;
	}

	fprintf(qglState.logFile, "glUniformMatrix4x3fv( %i, %i, %s, %p )\n", location, count, t, value);
	dllUniformMatrix4x3fv(location, count, transpose, value);
}

static GLboolean APIENTRY logUnmapBuffer (GLenum target){

	const char	*t;

	switch (target){
	case GL_ARRAY_BUFFER:				t = "GL_ARRAY_BUFFER";				break;
	case GL_ELEMENT_ARRAY_BUFFER:		t = "GL_ELEMENT_ARRAY_BUFFER";		break;
	case GL_PIXEL_PACK_BUFFER:			t = "GL_PIXEL_PACK_BUFFER";			break;
	case GL_PIXEL_UNPACK_BUFFER:		t = "GL_PIXEL_UNPACK_BUFFER";		break;
	case GL_COPY_READ_BUFFER:			t = "GL_COPY_READ_BUFFER";			break;
	case GL_COPY_WRITE_BUFFER:			t = "GL_COPY_WRITE_BUFFER";			break;
	case GL_TEXTURE_BUFFER:				t = "GL_TEXTURE_BUFFER";			break;
	case GL_UNIFORM_BUFFER:				t = "GL_UNIFORM_BUFFER";			break;
	default:							t = Str_VarArgs("0x%08X", target);	break;
	}

	fprintf(qglState.logFile, "glUnmapBuffer( %s )\n", t);
	return dllUnmapBuffer(target);
}

static GLvoid APIENTRY logUseProgram (GLuint program){

	fprintf(qglState.logFile, "glUseProgram( %u )\n", program);
	dllUseProgram(program);
}

static GLvoid APIENTRY logValidateProgram (GLuint program){

	fprintf(qglState.logFile, "glValidateProgram( %u )\n", program);
	dllValidateProgram(program);
}

static GLvoid APIENTRY logVertex2d (GLdouble x, GLdouble y){

	fprintf(qglState.logFile, "glVertex2d( %g, %g )\n", x, y);
	dllVertex2d(x, y);
}

static GLvoid APIENTRY logVertex2dv (const GLdouble *v){

	fprintf(qglState.logFile, "glVertex2dv( %p )\n", v);
	dllVertex2dv(v);
}

static GLvoid APIENTRY logVertex2f (GLfloat x, GLfloat y){

	fprintf(qglState.logFile, "glVertex2f( %g, %g )\n", x, y);
	dllVertex2f(x, y);
}

static GLvoid APIENTRY logVertex2fv (const GLfloat *v){

	fprintf(qglState.logFile, "glVertex2fv( %p )\n", v);
	dllVertex2fv(v);
}

static GLvoid APIENTRY logVertex2i (GLint x, GLint y){

	fprintf(qglState.logFile, "glVertex2i( %i, %i )\n", x, y);
	dllVertex2i(x, y);
}

static GLvoid APIENTRY logVertex2iv (const GLint *v){

	fprintf(qglState.logFile, "glVertex2iv( %p )\n", v);
	dllVertex2iv(v);
}

static GLvoid APIENTRY logVertex2s (GLshort x, GLshort y){

	fprintf(qglState.logFile, "glVertex2s( %i, %i )\n", x, y);
	dllVertex2s(x, y);
}

static GLvoid APIENTRY logVertex2sv (const GLshort *v){

	fprintf(qglState.logFile, "glVertex2sv( %p )\n", v);
	dllVertex2sv(v);
}

static GLvoid APIENTRY logVertex3d (GLdouble x, GLdouble y, GLdouble z){

	fprintf(qglState.logFile, "glVertex3d( %g, %g, %g )\n", x, y, z);
	dllVertex3d(x, y, z);
}

static GLvoid APIENTRY logVertex3dv (const GLdouble *v){

	fprintf(qglState.logFile, "glVertex3dv( %p )\n", v);
	dllVertex3dv(v);
}

static GLvoid APIENTRY logVertex3f (GLfloat x, GLfloat y, GLfloat z){

	fprintf(qglState.logFile, "glVertex3f( %g, %g, %g )\n", x, y, z);
	dllVertex3f(x, y, z);
}

static GLvoid APIENTRY logVertex3fv (const GLfloat *v){

	fprintf(qglState.logFile, "glVertex3fv( %p )\n", v);
	dllVertex3fv(v);
}

static GLvoid APIENTRY logVertex3i (GLint x, GLint y, GLint z){

	fprintf(qglState.logFile, "glVertex3i( %i, %i, %i )\n", x, y, z);
	dllVertex3i(x, y, z);
}

static GLvoid APIENTRY logVertex3iv (const GLint *v){

	fprintf(qglState.logFile, "glVertex3iv( %p )\n", v);
	dllVertex3iv(v);
}

static GLvoid APIENTRY logVertex3s (GLshort x, GLshort y, GLshort z){

	fprintf(qglState.logFile, "glVertex3s( %i, %i, %i )\n", x, y, z);
	dllVertex3s(x, y, z);
}

static GLvoid APIENTRY logVertex3sv (const GLshort *v){

	fprintf(qglState.logFile, "glVertex3sv( %p )\n", v);
	dllVertex3sv(v);
}

static GLvoid APIENTRY logVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w){

	fprintf(qglState.logFile, "glVertex4d( %g, %g, %g, %g )\n", x, y, z, w);
	dllVertex4d(x, y, z, w);
}

static GLvoid APIENTRY logVertex4dv (const GLdouble *v){

	fprintf(qglState.logFile, "glVertex4dv( %p )\n", v);
	dllVertex4dv(v);
}

static GLvoid APIENTRY logVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w){

	fprintf(qglState.logFile, "glVertex4f( %g, %g, %g, %g )\n", x, y, z, w);
	dllVertex4f(x, y, z, w);
}

static GLvoid APIENTRY logVertex4fv (const GLfloat *v){

	fprintf(qglState.logFile, "glVertex4fv( %p )\n", v);
	dllVertex4fv(v);
}

static GLvoid APIENTRY logVertex4i (GLint x, GLint y, GLint z, GLint w){

	fprintf(qglState.logFile, "glVertex4i( %i, %i, %i, %i )\n", x, y, z, w);
	dllVertex4i(x, y, z, w);
}

static GLvoid APIENTRY logVertex4iv (const GLint *v){

	fprintf(qglState.logFile, "glVertex4iv( %p )\n", v);
	dllVertex4iv(v);
}

static GLvoid APIENTRY logVertex4s (GLshort x, GLshort y, GLshort z, GLshort w){

	fprintf(qglState.logFile, "glVertex4s( %i, %i, %i, %i )\n", x, y, z, w);
	dllVertex4s(x, y, z, w);
}

static GLvoid APIENTRY logVertex4sv (const GLshort *v){

	fprintf(qglState.logFile, "glVertex4sv( %p )\n", v);
	dllVertex4sv(v);
}

static GLvoid APIENTRY logVertexAttrib1d (GLuint index, GLdouble x){

	fprintf(qglState.logFile, "glVertexAttrib1d( %u, %g )\n", index, x);
	dllVertexAttrib1d(index, x);
}

static GLvoid APIENTRY logVertexAttrib1dv (GLuint index, const GLdouble *v){

	fprintf(qglState.logFile, "glVertexAttrib1dv( %u, %p )\n", index, v);
	dllVertexAttrib1dv(index, v);
}

static GLvoid APIENTRY logVertexAttrib1f (GLuint index, GLfloat x){

	fprintf(qglState.logFile, "glVertexAttrib1f( %u, %g )\n", index, x);
	dllVertexAttrib1f(index, x);
}

static GLvoid APIENTRY logVertexAttrib1fv (GLuint index, const GLfloat *v){

	fprintf(qglState.logFile, "glVertexAttrib1fv( %u, %p )\n", index, v);
	dllVertexAttrib1fv(index, v);
}

static GLvoid APIENTRY logVertexAttrib1s (GLuint index, GLshort x){

	fprintf(qglState.logFile, "glVertexAttrib1s( %u, %i )\n", index, x);
	dllVertexAttrib1s(index, x);
}

static GLvoid APIENTRY logVertexAttrib1sv (GLuint index, const GLshort *v){

	fprintf(qglState.logFile, "glVertexAttrib1sv( %u, %p )\n", index, v);
	dllVertexAttrib1sv(index, v);
}

static GLvoid APIENTRY logVertexAttrib2d (GLuint index, GLdouble x, GLdouble y){

	fprintf(qglState.logFile, "glVertexAttrib2d( %u, %g, %g )\n", index, x, y);
	dllVertexAttrib2d(index, x, y);
}

static GLvoid APIENTRY logVertexAttrib2dv (GLuint index, const GLdouble *v){

	fprintf(qglState.logFile, "glVertexAttrib2dv( %u, %p )\n", index, v);
	dllVertexAttrib2dv(index, v);
}

static GLvoid APIENTRY logVertexAttrib2f (GLuint index, GLfloat x, GLfloat y){

	fprintf(qglState.logFile, "glVertexAttrib2f( %u, %g, %g )\n", index, x, y);
	dllVertexAttrib2f(index, x, y);
}

static GLvoid APIENTRY logVertexAttrib2fv (GLuint index, const GLfloat *v){

	fprintf(qglState.logFile, "glVertexAttrib2fv( %u, %p )\n", index, v);
	dllVertexAttrib2fv(index, v);
}

static GLvoid APIENTRY logVertexAttrib2s (GLuint index, GLshort x, GLshort y){

	fprintf(qglState.logFile, "glVertexAttrib2s( %u, %i, %i )\n", index, x, y);
	dllVertexAttrib2s(index, x, y);
}

static GLvoid APIENTRY logVertexAttrib2sv (GLuint index, const GLshort *v){

	fprintf(qglState.logFile, "glVertexAttrib2sv( %u, %p )\n", index, v);
	dllVertexAttrib2sv(index, v);
}

static GLvoid APIENTRY logVertexAttrib3d (GLuint index, GLdouble x, GLdouble y, GLdouble z){

	fprintf(qglState.logFile, "glVertexAttrib3d( %u, %g, %g, %g )\n", index, x, y, z);
	dllVertexAttrib3d(index, x, y, z);
}

static GLvoid APIENTRY logVertexAttrib3dv (GLuint index, const GLdouble *v){

	fprintf(qglState.logFile, "glVertexAttrib3dv( %u, %p )\n", index, v);
	dllVertexAttrib3dv(index, v);
}

static GLvoid APIENTRY logVertexAttrib3f (GLuint index, GLfloat x, GLfloat y, GLfloat z){

	fprintf(qglState.logFile, "glVertexAttrib3f( %u, %g, %g, %g )\n", index, x, y, z);
	dllVertexAttrib3f(index, x, y, z);
}

static GLvoid APIENTRY logVertexAttrib3fv (GLuint index, const GLfloat *v){

	fprintf(qglState.logFile, "glVertexAttrib3fv( %u, %p )\n", index, v);
	dllVertexAttrib3fv(index, v);
}

static GLvoid APIENTRY logVertexAttrib3s (GLuint index, GLshort x, GLshort y, GLshort z){

	fprintf(qglState.logFile, "glVertexAttrib3s( %u, %i, %i, %i )\n", index, x, y, z);
	dllVertexAttrib3s(index, x, y, z);
}

static GLvoid APIENTRY logVertexAttrib3sv (GLuint index, const GLshort *v){

	fprintf(qglState.logFile, "glVertexAttrib3sv( %u, %p )\n", index, v);
	dllVertexAttrib3sv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4Nbv (GLuint index, const GLbyte *v){

	fprintf(qglState.logFile, "glVertexAttrib4Nbv( %u, %p )\n", index, v);
	dllVertexAttrib4Nbv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4Niv (GLuint index, const GLint *v){

	fprintf(qglState.logFile, "glVertexAttrib4Niv( %u, %p )\n", index, v);
	dllVertexAttrib4Niv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4Nsv (GLuint index, const GLshort *v){

	fprintf(qglState.logFile, "glVertexAttrib4Nsv( %u, %p )\n", index, v);
	dllVertexAttrib4Nsv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4Nub (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w){

	fprintf(qglState.logFile, "glVertexAttrib4Nub( %u, %u, %u, %u, %u )\n", index, x, y, z, w);
	dllVertexAttrib4Nub(index, x, y, z, w);
}

static GLvoid APIENTRY logVertexAttrib4Nubv (GLuint index, const GLubyte *v){

	fprintf(qglState.logFile, "glVertexAttrib4Nubv( %u, %p )\n", index, v);
	dllVertexAttrib4Nubv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4Nuiv (GLuint index, const GLuint *v){

	fprintf(qglState.logFile, "glVertexAttrib4Nuiv( %u, %p )\n", index, v);
	dllVertexAttrib4Nuiv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4Nusv (GLuint index, const GLushort *v){

	fprintf(qglState.logFile, "glVertexAttrib4Nusv( %u, %p )\n", index, v);
	dllVertexAttrib4Nusv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4bv (GLuint index, const GLbyte *v){

	fprintf(qglState.logFile, "glVertexAttrib4bv( %u, %p )\n", index, v);
	dllVertexAttrib4bv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4d (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w){

	fprintf(qglState.logFile, "glVertexAttrib4d( %u, %g, %g, %g, %g )\n", index, x, y, z, w);
	dllVertexAttrib4d(index, x, y, z, w);
}

static GLvoid APIENTRY logVertexAttrib4dv (GLuint index, const GLdouble *v){

	fprintf(qglState.logFile, "glVertexAttrib4dv( %u, %p )\n", index, v);
	dllVertexAttrib4dv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4f (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w){

	fprintf(qglState.logFile, "glVertexAttrib4f( %u, %g, %g, %g, %g )\n", index, x, y, z, w);
	dllVertexAttrib4f(index, x, y, z, w);
}

static GLvoid APIENTRY logVertexAttrib4fv (GLuint index, const GLfloat *v){

	fprintf(qglState.logFile, "glVertexAttrib4fv( %u, %p )\n", index, v);
	dllVertexAttrib4fv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4iv (GLuint index, const GLint *v){

	fprintf(qglState.logFile, "glVertexAttrib4iv( %u, %p )\n", index, v);
	dllVertexAttrib4iv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4s (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w){

	fprintf(qglState.logFile, "glVertexAttrib4s( %u, %i, %i, %i, %i )\n", index, x, y, z, w);
	dllVertexAttrib4s(index, x, y, z, w);
}

static GLvoid APIENTRY logVertexAttrib4sv (GLuint index, const GLshort *v){

	fprintf(qglState.logFile, "glVertexAttrib4sv( %u, %p )\n", index, v);
	dllVertexAttrib4sv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4ubv (GLuint index, const GLubyte *v){

	fprintf(qglState.logFile, "glVertexAttrib4ubv( %u, %p )\n", index, v);
	dllVertexAttrib4ubv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4uiv (GLuint index, const GLuint *v){

	fprintf(qglState.logFile, "glVertexAttrib4uiv( %u, %p )\n", index, v);
	dllVertexAttrib4uiv(index, v);
}

static GLvoid APIENTRY logVertexAttrib4usv (GLuint index, const GLushort *v){

	fprintf(qglState.logFile, "glVertexAttrib4usv( %u, %p )\n", index, v);
	dllVertexAttrib4usv(index, v);
}

static GLvoid APIENTRY logVertexAttribI1i(GLuint index, GLint x){

	fprintf(qglState.logFile, "glVertexAttribI1i( %u, %i )\n", index, x);
	dllVertexAttribI1i(index, x);
}

static GLvoid APIENTRY logVertexAttribI1iv (GLuint index, const GLint *v){

	fprintf(qglState.logFile, "glVertexAttribI1iv( %u, %p )\n", index, v);
	dllVertexAttribI1iv(index, v);
}

static GLvoid APIENTRY logVertexAttribI1ui (GLuint index, GLuint x){

	fprintf(qglState.logFile, "glVertexAttribI1ui( %u, %u )\n", index, x);
	dllVertexAttribI1ui(index, x);
}

static GLvoid APIENTRY logVertexAttribI1uiv (GLuint index, const GLuint *v){

	fprintf(qglState.logFile, "glVertexAttribI1uiv( %u, %p )\n", index, v);
	dllVertexAttribI1uiv(index, v);
}

static GLvoid APIENTRY logVertexAttribI2i (GLuint index, GLint x, GLint y){

	fprintf(qglState.logFile, "glVertexAttribI2i( %u, %i, %i )\n", index, x, y);
	dllVertexAttribI2i(index, x, y);
}

static GLvoid APIENTRY logVertexAttribI2iv (GLuint index, const GLint *v){

	fprintf(qglState.logFile, "glVertexAttribI2iv( %u, %p )\n", index, v);
	dllVertexAttribI2iv(index, v);
}

static GLvoid APIENTRY logVertexAttribI2ui (GLuint index, GLuint x, GLuint y){

	fprintf(qglState.logFile, "glVertexAttribI2ui( %u, %u, %u )\n", index, x, y);
	dllVertexAttribI2ui(index, x, y);
}

static GLvoid APIENTRY logVertexAttribI2uiv (GLuint index, const GLuint *v){

	fprintf(qglState.logFile, "glVertexAttribI2uiv( %u, %p )\n", index, v);
	dllVertexAttribI2uiv(index, v);
}

static GLvoid APIENTRY logVertexAttribI3i (GLuint index, GLint x, GLint y, GLint z){

	fprintf(qglState.logFile, "glVertexAttribI3i( %u, %i, %i, %i )\n", index, x, y, z);
	dllVertexAttribI3i(index, x, y, z);
}

static GLvoid APIENTRY logVertexAttribI3iv (GLuint index, const GLint *v){

	fprintf(qglState.logFile, "glVertexAttribI3iv( %u, %p )\n", index, v);
	dllVertexAttribI3iv(index, v);
}

static GLvoid APIENTRY logVertexAttribI3ui (GLuint index, GLuint x, GLuint y, GLuint z){

	fprintf(qglState.logFile, "glVertexAttribI3ui( %u, %u, %u, %u )\n", index, x, y, z);
	dllVertexAttribI3ui(index, x, y, z);
}

static GLvoid APIENTRY logVertexAttribI3uiv (GLuint index, const GLuint *v){

	fprintf(qglState.logFile, "glVertexAttribI3uiv( %u, %p )\n", index, v);
	dllVertexAttribI3uiv(index, v);
}

static GLvoid APIENTRY logVertexAttribI4bv (GLuint index, const GLbyte *v){

	fprintf(qglState.logFile, "glVertexAttribI4bv( %u, %p )\n", index, v);
	dllVertexAttribI4bv(index, v);
}

static GLvoid APIENTRY logVertexAttribI4i (GLuint index, GLint x, GLint y, GLint z, GLint w){

	fprintf(qglState.logFile, "glVertexAttribI4i( %u, %i, %i, %i, %i )\n", index, x, y, z, w);
	dllVertexAttribI4i(index, x, y, z, w);
}

static GLvoid APIENTRY logVertexAttribI4iv (GLuint index, const GLint *v){

	fprintf(qglState.logFile, "glVertexAttribI4iv( %u, %p )\n", index, v);
	dllVertexAttribI4iv(index, v);
}

static GLvoid APIENTRY logVertexAttribI4sv (GLuint index, const GLshort *v){

	fprintf(qglState.logFile, "glVertexAttribI4sv( %u, %p )\n", index, v);
	dllVertexAttribI4sv(index, v);
}

static GLvoid APIENTRY logVertexAttribI4ubv (GLuint index, const GLubyte *v){

	fprintf(qglState.logFile, "glVertexAttribI4ubv( %u, %p )\n", index, v);
	dllVertexAttribI4ubv(index, v);
}

static GLvoid APIENTRY logVertexAttribI4ui (GLuint index, GLuint x, GLuint y, GLuint z, GLuint w){

	fprintf(qglState.logFile, "glVertexAttribI4ui( %u, %u, %u, %u, %u )\n", index, x, y, z, w);
	dllVertexAttribI4ui(index, x, y, z, w);
}

static GLvoid APIENTRY logVertexAttribI4uiv (GLuint index, const GLuint *v){

	fprintf(qglState.logFile, "glVertexAttribI4uiv( %u, %p )\n", index, v);
	dllVertexAttribI4uiv(index, v);
}

static GLvoid APIENTRY logVertexAttribI4usv (GLuint index, const GLushort *v){

	fprintf(qglState.logFile, "glVertexAttribI4usv( %u, %p )\n", index, v);
	dllVertexAttribI4usv(index, v);
}

static GLvoid APIENTRY logVertexAttribIPointer (GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){

	const char	*t;

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";							break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";					break;
	case GL_SHORT:			t = "GL_SHORT";							break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";				break;
	case GL_INT:			t = "GL_INT";							break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";					break;
	default:				t = Str_VarArgs("0x%08X", type);			break;
	}

	fprintf(qglState.logFile, "glVertexAttribIPointer( %u, %i, %s, %i, %p )\n", index, size, t, stride, pointer);
	dllVertexAttribIPointer(index, size, type, stride, pointer);
}

static GLvoid APIENTRY logVertexAttribP1ui (GLuint index, GLenum type, GLboolean normalized, GLuint v){

	fprintf(qglState.logFile, "glVertexAttribP1ui( %u, 0x%08X, %u, %u )\n", index, type, normalized, v);
	dllVertexAttribP1ui(index, type, normalized, v);
}

static GLvoid APIENTRY logVertexAttribP1uiv (GLuint index, GLenum type, GLboolean normalized, const GLuint *v){

	fprintf(qglState.logFile, "glVertexAttribP1uiv( %u, 0x%08X, %u, %p )\n", index, type, normalized, v);
	dllVertexAttribP1uiv(index, type, normalized, v);
}

static GLvoid APIENTRY logVertexAttribP2ui (GLuint index, GLenum type, GLboolean normalized, GLuint v){

	fprintf(qglState.logFile, "glVertexAttribP2ui( %u, 0x%08X, %u, %u )\n", index, type, normalized, v);
	dllVertexAttribP2ui(index, type, normalized, v);
}

static GLvoid APIENTRY logVertexAttribP2uiv (GLuint index, GLenum type, GLboolean normalized, const GLuint *v){

	fprintf(qglState.logFile, "glVertexAttribP2uiv( %u, 0x%08X, %u, %p )\n", index, type, normalized, v);
	dllVertexAttribP2uiv(index, type, normalized, v);
}

static GLvoid APIENTRY logVertexAttribP3ui (GLuint index, GLenum type, GLboolean normalized, GLuint v){

	fprintf(qglState.logFile, "glVertexAttribP3ui( %u, 0x%08X, %u, %u )\n", index, type, normalized, v);
	dllVertexAttribP3ui(index, type, normalized, v);
}

static GLvoid APIENTRY logVertexAttribP3uiv (GLuint index, GLenum type, GLboolean normalized, const GLuint *v){

	fprintf(qglState.logFile, "glVertexAttribP3uiv( %u, 0x%08X, %u, %p )\n", index, type, normalized, v);
	dllVertexAttribP3uiv(index, type, normalized, v);
}

static GLvoid APIENTRY logVertexAttribP4ui (GLuint index, GLenum type, GLboolean normalized, GLuint v){

	fprintf(qglState.logFile, "glVertexAttribP4ui( %u, 0x%08X, %u, %u )\n", index, type, normalized, v);
	dllVertexAttribP4ui(index, type, normalized, v);
}

static GLvoid APIENTRY logVertexAttribP4uiv (GLuint index, GLenum type, GLboolean normalized, const GLuint *v){

	fprintf(qglState.logFile, "glVertexAttribP4uiv( %u, 0x%08X, %u, %p )\n", index, type, normalized, v);
	dllVertexAttribP4uiv(index, type, normalized, v);
}

static GLvoid APIENTRY logVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer){

	const char	*t, *n;

	switch (type){
	case GL_BYTE:			t = "GL_BYTE";							break;
	case GL_UNSIGNED_BYTE:	t = "GL_UNSIGNED_BYTE";					break;
	case GL_SHORT:			t = "GL_SHORT";							break;
	case GL_UNSIGNED_SHORT:	t = "GL_UNSIGNED_SHORT";				break;
	case GL_INT:			t = "GL_INT";							break;
	case GL_UNSIGNED_INT:	t = "GL_UNSIGNED_INT";					break;
	case GL_FLOAT:			t = "GL_FLOAT";							break;
	case GL_HALF_FLOAT:		t = "GL_HALF_FLOAT";					break;
	case GL_DOUBLE:			t = "GL_DOUBLE";						break;
	default:				t = Str_VarArgs("0x%08X", type);			break;
	}

	switch (normalized){
	case GL_FALSE:			n = "GL_FALSE";							break;
	case GL_TRUE:			n = "GL_TRUE";							break;
	default:				n = Str_VarArgs("0x%08X", normalized);	break;
	}

	fprintf(qglState.logFile, "glVertexAttribPointer( %u, %i, %s, %s, %i, %p )\n", index, size, t, n, stride, pointer);
	dllVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

static GLvoid APIENTRY logVertexP2ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glVertexP2ui( 0x%08X, %u )\n", type, v);
	dllVertexP2ui(type, v);
}

static GLvoid APIENTRY logVertexP2uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glVertexP2uiv( 0x%08X, %p )\n", type, v);
	dllVertexP2uiv(type, v);
}

static GLvoid APIENTRY logVertexP3ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glVertexP3ui( 0x%08X, %u )\n", type, v);
	dllVertexP3ui(type, v);
}

static GLvoid APIENTRY logVertexP3uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glVertexP3uiv( 0x%08X, %p )\n", type, v);
	dllVertexP3uiv(type, v);
}

static GLvoid APIENTRY logVertexP4ui (GLenum type, GLuint v){

	fprintf(qglState.logFile, "glVertexP4ui( 0x%08X, %u )\n", type, v);
	dllVertexP4ui(type, v);
}

static GLvoid APIENTRY logVertexP4uiv (GLenum type, const GLuint *v){

	fprintf(qglState.logFile, "glVertexP4uiv( 0x%08X, %p )\n", type, v);
	dllVertexP4uiv(type, v);
}

static GLvoid APIENTRY logVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){

	const char	*t;

	switch (type){
	case GL_SHORT:		t = "GL_SHORT";					break;
	case GL_INT:		t = "GL_INT";					break;
	case GL_FLOAT:		t = "GL_FLOAT";					break;
	case GL_HALF_FLOAT:	t = "GL_HALF_FLOAT";			break;
	case GL_DOUBLE:		t = "GL_DOUBLE";				break;
	default:			t = Str_VarArgs("0x%08X", type);	break;
	}

	fprintf(qglState.logFile, "glVertexPointer( %i, %s, %i, %p )\n", size, t, stride, pointer);
	dllVertexPointer(size, type, stride, pointer);
}

static GLvoid APIENTRY logViewport (GLint x, GLint y, GLsizei width, GLsizei height){

	fprintf(qglState.logFile, "glViewport( %i, %i, %i, %i )\n", x, y, width, height);
	dllViewport(x, y, width, height);
}

static GLvoid APIENTRY logWaitSync (GLsync sync, GLbitfield flags, GLuint64 timeout){

	fprintf(qglState.logFile, "glWaitSync( %p, %u, %I64u )\n", sync, flags, timeout);
	dllWaitSync(sync, flags, timeout);
}

static GLvoid APIENTRY logWindowPos2d (GLdouble x, GLdouble y){

	fprintf(qglState.logFile, "glWindowPos2d( %g, %g )\n", x, y);
	dllWindowPos2d(x, y);
}

static GLvoid APIENTRY logWindowPos2dv (const GLdouble *v){

	fprintf(qglState.logFile, "glWindowPos2dv( %p )\n", v);
	dllWindowPos2dv(v);
}

static GLvoid APIENTRY logWindowPos2f (GLfloat x, GLfloat y){

	fprintf(qglState.logFile, "glWindowPos2f( %g, %g )\n", x, y);
	dllWindowPos2f(x, y);
}

static GLvoid APIENTRY logWindowPos2fv (const GLfloat *v){

	fprintf(qglState.logFile, "glWindowPos2fv( %p )\n", v);
	dllWindowPos2fv(v);
}

static GLvoid APIENTRY logWindowPos2i (GLint x, GLint y){

	fprintf(qglState.logFile, "glWindowPos2i( %i, %i )\n", x, y);
	dllWindowPos2i(x, y);
}

static GLvoid APIENTRY logWindowPos2iv (const GLint *v){

	fprintf(qglState.logFile, "glWindowPos2iv( %p )\n", v);
	dllWindowPos2iv(v);
}

static GLvoid APIENTRY logWindowPos2s (GLshort x, GLshort y){

	fprintf(qglState.logFile, "glWindowPos2s( %i, %i )\n", x, y);
	dllWindowPos2s(x, y);
}

static GLvoid APIENTRY logWindowPos2sv (const GLshort *v){

	fprintf(qglState.logFile, "glWindowPos2sv( %p )\n", v);
	dllWindowPos2sv(v);
}

static GLvoid APIENTRY logWindowPos3d (GLdouble x, GLdouble y, GLdouble z){

	fprintf(qglState.logFile, "glWindowPos3d( %g, %g, %g )\n", x, y, z);
	dllWindowPos3d(x, y, z);
}

static GLvoid APIENTRY logWindowPos3dv (const GLdouble *v){

	fprintf(qglState.logFile, "glWindowPos3dv( %p )\n", v);
	dllWindowPos3dv(v);
}

static GLvoid APIENTRY logWindowPos3f (GLfloat x, GLfloat y, GLfloat z){

	fprintf(qglState.logFile, "glWindowPos3f( %g, %g, %g )\n", x, y, z);
	dllWindowPos3f(x, y, z);
}

static GLvoid APIENTRY logWindowPos3fv (const GLfloat *v){

	fprintf(qglState.logFile, "glWindowPos3fv( %p )\n", v);
	dllWindowPos3fv(v);
}

static GLvoid APIENTRY logWindowPos3i (GLint x, GLint y, GLint z){

	fprintf(qglState.logFile, "glWindowPos3i( %i, %i, %i )\n", x, y, z);
	dllWindowPos3i(x, y, z);
}

static GLvoid APIENTRY logWindowPos3iv (const GLint *v){

	fprintf(qglState.logFile, "glWindowPos3iv( %p )\n", v);
	dllWindowPos3iv(v);
}

static GLvoid APIENTRY logWindowPos3s (GLshort x, GLshort y, GLshort z){

	fprintf(qglState.logFile, "glWindowPos3s( %i, %i, %i )\n", x, y, z);
	dllWindowPos3s(x, y, z);
}

static GLvoid APIENTRY logWindowPos3sv (const GLshort *v){

	fprintf(qglState.logFile, "glWindowPos3sv( %p )\n", v);
	dllWindowPos3sv(v);
}


/*
 ==============================================================================

 LOG FILE

 ==============================================================================
*/


/*
 ==================
 QGL_CopyPointers
 ==================
*/
static void QGL_CopyPointers (){

	dllAccum								= qglAccum;
	dllActiveStencilFaceEXT					= qglActiveStencilFaceEXT;
	dllActiveTexture						= qglActiveTexture;
	dllAlphaFunc							= qglAlphaFunc;
	dllAreTexturesResident					= qglAreTexturesResident;
	dllArrayElement							= qglArrayElement;
	dllAttachShader							= qglAttachShader;
	dllBegin								= qglBegin;
	dllBeginConditionalRender				= qglBeginConditionalRender;
	dllBeginQuery							= qglBeginQuery;
	dllBeginTransformFeedback				= qglBeginTransformFeedback;
	dllBindAttribLocation					= qglBindAttribLocation;
	dllBindBuffer							= qglBindBuffer;
	dllBindBufferBase						= qglBindBufferBase;
	dllBindBufferRange						= qglBindBufferRange;
	dllBindFragDataLocation					= qglBindFragDataLocation;
	dllBindFragDataLocationIndexed			= qglBindFragDataLocationIndexed;
	dllBindFramebuffer						= qglBindFramebuffer;
	dllBindRenderbuffer						= qglBindRenderbuffer;
	dllBindSampler							= qglBindSampler;
	dllBindTexture							= qglBindTexture;
	dllBindVertexArray						= qglBindVertexArray;
	dllBitmap								= qglBitmap;
	dllBlendColor							= qglBlendColor;
	dllBlendEquation						= qglBlendEquation;
	dllBlendEquationSeparate				= qglBlendEquationSeparate;
	dllBlendFunc							= qglBlendFunc;
	dllBlendFuncSeparate					= qglBlendFuncSeparate;
	dllBlitFramebuffer						= qglBlitFramebuffer;
	dllBufferData							= qglBufferData;
	dllBufferSubData						= qglBufferSubData;
	dllCallList								= qglCallList;
	dllCallLists							= qglCallLists;
	dllCheckFramebufferStatus				= qglCheckFramebufferStatus;
	dllClampColor							= qglClampColor;
	dllClear								= qglClear;
	dllClearAccum							= qglClearAccum;
	dllClearBufferfi						= qglClearBufferfi;
	dllClearBufferfv						= qglClearBufferfv;
	dllClearBufferiv						= qglClearBufferiv;
	dllClearBufferuiv						= qglClearBufferuiv;
	dllClearColor							= qglClearColor;
	dllClearDepth							= qglClearDepth;
	dllClearIndex							= qglClearIndex;
	dllClearStencil							= qglClearStencil;
	dllClientActiveTexture					= qglClientActiveTexture;
	dllClientWaitSync						= qglClientWaitSync;
	dllClipPlane							= qglClipPlane;
	dllColor3b								= qglColor3b;
	dllColor3bv								= qglColor3bv;
	dllColor3d								= qglColor3d;
	dllColor3dv								= qglColor3dv;
	dllColor3f								= qglColor3f;
	dllColor3fv								= qglColor3fv;
	dllColor3i								= qglColor3i;
	dllColor3iv								= qglColor3iv;
	dllColor3s								= qglColor3s;
	dllColor3sv								= qglColor3sv;
	dllColor3ub								= qglColor3ub;
	dllColor3ubv							= qglColor3ubv;
	dllColor3ui								= qglColor3ui;
	dllColor3uiv							= qglColor3uiv;
	dllColor3us								= qglColor3us;
	dllColor3usv							= qglColor3usv;
	dllColor4b								= qglColor4b;
	dllColor4bv								= qglColor4bv;
	dllColor4d								= qglColor4d;
	dllColor4dv								= qglColor4dv;
	dllColor4f								= qglColor4f;
	dllColor4fv								= qglColor4fv;
	dllColor4i								= qglColor4i;
	dllColor4iv								= qglColor4iv;
	dllColor4s								= qglColor4s;
	dllColor4sv								= qglColor4sv;
	dllColor4ub								= qglColor4ub;
	dllColor4ubv							= qglColor4ubv;
	dllColor4ui								= qglColor4ui;
	dllColor4uiv							= qglColor4uiv;
	dllColor4us								= qglColor4us;
	dllColor4usv							= qglColor4usv;
	dllColorMask							= qglColorMask;
	dllColorMaski							= qglColorMaski;
	dllColorMaterial						= qglColorMaterial;
	dllColorP3ui							= qglColorP3ui;
	dllColorP3uiv							= qglColorP3uiv;
	dllColorP4ui							= qglColorP4ui;
	dllColorP4uiv							= qglColorP4uiv;
	dllColorPointer							= qglColorPointer;
	dllCompileShader						= qglCompileShader;
	dllCompressedTexImage1D					= qglCompressedTexImage1D;
	dllCompressedTexImage2D					= qglCompressedTexImage2D;
	dllCompressedTexImage3D					= qglCompressedTexImage3D;
	dllCompressedTexSubImage1D				= qglCompressedTexSubImage1D;
	dllCompressedTexSubImage2D				= qglCompressedTexSubImage2D;
	dllCompressedTexSubImage3D				= qglCompressedTexSubImage3D;
	dllCopyBufferSubData					= qglCopyBufferSubData;
	dllCopyPixels							= qglCopyPixels;
	dllCopyTexImage1D						= qglCopyTexImage1D;
	dllCopyTexImage2D						= qglCopyTexImage2D;
	dllCopyTexSubImage1D					= qglCopyTexSubImage1D;
	dllCopyTexSubImage2D					= qglCopyTexSubImage2D;
	dllCopyTexSubImage3D					= qglCopyTexSubImage3D;
	dllCreateProgram						= qglCreateProgram;
	dllCreateShader							= qglCreateShader;
	dllCullFace								= qglCullFace;
	dllDeleteBuffers						= qglDeleteBuffers;
	dllDeleteFramebuffers					= qglDeleteFramebuffers;
	dllDeleteLists							= qglDeleteLists;
	dllDeleteProgram						= qglDeleteProgram;
	dllDeleteQueries						= qglDeleteQueries;
	dllDeleteRenderbuffers					= qglDeleteRenderbuffers;
	dllDeleteSamplers						= qglDeleteSamplers;
	dllDeleteShader							= qglDeleteShader;
	dllDeleteSync							= qglDeleteSync;
	dllDeleteTextures						= qglDeleteTextures;
	dllDeleteVertexArrays					= qglDeleteVertexArrays;
	dllDepthBoundsEXT						= qglDepthBoundsEXT;
	dllDepthFunc							= qglDepthFunc;
	dllDepthMask							= qglDepthMask;
	dllDepthRange							= qglDepthRange;
	dllDetachShader							= qglDetachShader;
	dllDisable								= qglDisable;
	dllDisableClientState					= qglDisableClientState;
	dllDisableVertexAttribArray				= qglDisableVertexAttribArray;
	dllDisablei								= qglDisablei;
	dllDrawArrays							= qglDrawArrays;
	dllDrawArraysInstanced					= qglDrawArraysInstanced;
	dllDrawBuffer							= qglDrawBuffer;
	dllDrawBuffers							= qglDrawBuffers;
	dllDrawElements							= qglDrawElements;
	dllDrawElementsBaseVertex				= qglDrawElementsBaseVertex;
	dllDrawElementsInstanced				= qglDrawElementsInstanced;
	dllDrawElementsInstancedBaseVertex		= qglDrawElementsInstancedBaseVertex;
	dllDrawPixels							= qglDrawPixels;
	dllDrawRangeElements					= qglDrawRangeElements;
	dllDrawRangeElementsBaseVertex			= qglDrawRangeElementsBaseVertex;
	dllEdgeFlag								= qglEdgeFlag;
	dllEdgeFlagPointer						= qglEdgeFlagPointer;
	dllEdgeFlagv							= qglEdgeFlagv;
	dllEnable								= qglEnable;
	dllEnableClientState					= qglEnableClientState;
	dllEnableVertexAttribArray				= qglEnableVertexAttribArray;
	dllEnablei								= qglEnablei;
	dllEnd									= qglEnd;
	dllEndConditionalRender					= qglEndConditionalRender;
	dllEndList								= qglEndList;
	dllEndQuery								= qglEndQuery;
	dllEndTransformFeedback					= qglEndTransformFeedback;
	dllEvalCoord1d							= qglEvalCoord1d;
	dllEvalCoord1dv							= qglEvalCoord1dv;
	dllEvalCoord1f							= qglEvalCoord1f;
	dllEvalCoord1fv							= qglEvalCoord1fv;
	dllEvalCoord2d							= qglEvalCoord2d;
	dllEvalCoord2dv							= qglEvalCoord2dv;
	dllEvalCoord2f							= qglEvalCoord2f;
	dllEvalCoord2fv							= qglEvalCoord2fv;
	dllEvalMesh1							= qglEvalMesh1;
	dllEvalMesh2							= qglEvalMesh2;
	dllEvalPoint1							= qglEvalPoint1;
	dllEvalPoint2							= qglEvalPoint2;
	dllFeedbackBuffer						= qglFeedbackBuffer;
	dllFenceSync							= qglFenceSync;
	dllFinish								= qglFinish;
	dllFlush								= qglFlush;
	dllFlushMappedBufferRange				= qglFlushMappedBufferRange;
	dllFogCoordPointer						= qglFogCoordPointer;
	dllFogCoordd							= qglFogCoordd;
	dllFogCoorddv							= qglFogCoorddv;
	dllFogCoordf							= qglFogCoordf;
	dllFogCoordfv							= qglFogCoordfv;
	dllFogf									= qglFogf;
	dllFogfv								= qglFogfv;
	dllFogi									= qglFogi;
	dllFogiv								= qglFogiv;
	dllFramebufferRenderbuffer				= qglFramebufferRenderbuffer;
	dllFramebufferTexture					= qglFramebufferTexture;
	dllFramebufferTexture1D					= qglFramebufferTexture1D;
	dllFramebufferTexture2D					= qglFramebufferTexture2D;
	dllFramebufferTexture3D					= qglFramebufferTexture3D;
	dllFramebufferTextureLayer				= qglFramebufferTextureLayer;
	dllFrontFace							= qglFrontFace;
	dllFrustum								= qglFrustum;
	dllGenBuffers							= qglGenBuffers;
	dllGenFramebuffers						= qglGenFramebuffers;
	dllGenLists								= qglGenLists;
	dllGenQueries							= qglGenQueries;
	dllGenRenderbuffers						= qglGenRenderbuffers;
	dllGenSamplers							= qglGenSamplers;
	dllGenTextures							= qglGenTextures;
	dllGenVertexArrays						= qglGenVertexArrays;
	dllGenerateMipmap						= qglGenerateMipmap;
	dllGetActiveAttrib						= qglGetActiveAttrib;
	dllGetActiveUniform						= qglGetActiveUniform;
	dllGetActiveUniformBlockName			= qglGetActiveUniformBlockName;
	dllGetActiveUniformBlockiv				= qglGetActiveUniformBlockiv;
	dllGetActiveUniformName					= qglGetActiveUniformName;
	dllGetActiveUniformsiv					= qglGetActiveUniformsiv;
	dllGetAttachedShaders					= qglGetAttachedShaders;
	dllGetAttribLocation					= qglGetAttribLocation;
	dllGetBooleani_v						= qglGetBooleani_v;
	dllGetBooleanv							= qglGetBooleanv;
	dllGetBufferParameteri64v				= qglGetBufferParameteri64v;
	dllGetBufferParameteriv					= qglGetBufferParameteriv;
	dllGetBufferPointerv					= qglGetBufferPointerv;
	dllGetBufferSubData						= qglGetBufferSubData;
	dllGetClipPlane							= qglGetClipPlane;
	dllGetCompressedTexImage				= qglGetCompressedTexImage;
	dllGetDoublev							= qglGetDoublev;
	dllGetError								= qglGetError;
	dllGetFloatv							= qglGetFloatv;
	dllGetFragDataIndex						= qglGetFragDataIndex;
	dllGetFragDataLocation					= qglGetFragDataLocation;
	dllGetFramebufferAttachmentParameteriv	= qglGetFramebufferAttachmentParameteriv;
	dllGetInteger64i_v						= qglGetInteger64i_v;
	dllGetInteger64v						= qglGetInteger64v;
	dllGetIntegeri_v						= qglGetIntegeri_v;
	dllGetIntegerv							= qglGetIntegerv;
	dllGetLightfv							= qglGetLightfv;
	dllGetLightiv							= qglGetLightiv;
	dllGetMapdv								= qglGetMapdv;
	dllGetMapfv								= qglGetMapfv;
	dllGetMapiv								= qglGetMapiv;
	dllGetMaterialfv						= qglGetMaterialfv;
	dllGetMaterialiv						= qglGetMaterialiv;
	dllGetMultisamplefv						= qglGetMultisamplefv;
	dllGetPixelMapfv						= qglGetPixelMapfv;
	dllGetPixelMapuiv						= qglGetPixelMapuiv;
	dllGetPixelMapusv						= qglGetPixelMapusv;
	dllGetPointerv							= qglGetPointerv;
	dllGetPolygonStipple					= qglGetPolygonStipple;
	dllGetProgramInfoLog					= qglGetProgramInfoLog;
	dllGetProgramiv							= qglGetProgramiv;
	dllGetQueryObjecti64v					= qglGetQueryObjecti64v;
	dllGetQueryObjectiv						= qglGetQueryObjectiv;
	dllGetQueryObjectui64v					= qglGetQueryObjectui64v;
	dllGetQueryObjectuiv					= qglGetQueryObjectuiv;
	dllGetQueryiv							= qglGetQueryiv;
	dllGetRenderbufferParameteriv			= qglGetRenderbufferParameteriv;
	dllGetSamplerParameterIiv				= qglGetSamplerParameterIiv;
	dllGetSamplerParameterIuiv				= qglGetSamplerParameterIuiv;
	dllGetSamplerParameterfv				= qglGetSamplerParameterfv;
	dllGetSamplerParameteriv				= qglGetSamplerParameteriv;
	dllGetShaderInfoLog						= qglGetShaderInfoLog;
	dllGetShaderSource						= qglGetShaderSource;
	dllGetShaderiv							= qglGetShaderiv;
	dllGetString							= qglGetString;
	dllGetStringi							= qglGetStringi;
	dllGetSynciv							= qglGetSynciv;
	dllGetTexEnvfv							= qglGetTexEnvfv;
	dllGetTexEnviv							= qglGetTexEnviv;
	dllGetTexGendv							= qglGetTexGendv;
	dllGetTexGenfv							= qglGetTexGenfv;
	dllGetTexGeniv							= qglGetTexGeniv;
	dllGetTexImage							= qglGetTexImage;
	dllGetTexLevelParameterfv				= qglGetTexLevelParameterfv;
	dllGetTexLevelParameteriv				= qglGetTexLevelParameteriv;
	dllGetTexParameterIiv					= qglGetTexParameterIiv;
	dllGetTexParameterIuiv					= qglGetTexParameterIuiv;
	dllGetTexParameterfv					= qglGetTexParameterfv;
	dllGetTexParameteriv					= qglGetTexParameteriv;
	dllGetTransformFeedbackVarying			= qglGetTransformFeedbackVarying;
	dllGetUniformBlockIndex					= qglGetUniformBlockIndex;
	dllGetUniformIndices					= qglGetUniformIndices;
	dllGetUniformLocation					= qglGetUniformLocation;
	dllGetUniformfv							= qglGetUniformfv;
	dllGetUniformiv							= qglGetUniformiv;
	dllGetUniformuiv						= qglGetUniformuiv;
	dllGetVertexAttribIiv					= qglGetVertexAttribIiv;
	dllGetVertexAttribIuiv					= qglGetVertexAttribIuiv;
	dllGetVertexAttribPointerv				= qglGetVertexAttribPointerv;
	dllGetVertexAttribdv					= qglGetVertexAttribdv;
	dllGetVertexAttribfv					= qglGetVertexAttribfv;
	dllGetVertexAttribiv					= qglGetVertexAttribiv;
	dllHint									= qglHint;
	dllIndexMask							= qglIndexMask;
	dllIndexPointer							= qglIndexPointer;
	dllIndexd								= qglIndexd;
	dllIndexdv								= qglIndexdv;
	dllIndexf								= qglIndexf;
	dllIndexfv								= qglIndexfv;
	dllIndexi								= qglIndexi;
	dllIndexiv								= qglIndexiv;
	dllIndexs								= qglIndexs;
	dllIndexsv								= qglIndexsv;
	dllIndexub								= qglIndexub;
	dllIndexubv								= qglIndexubv;
	dllInitNames							= qglInitNames;
	dllInterleavedArrays					= qglInterleavedArrays;
	dllIsBuffer								= qglIsBuffer;
	dllIsEnabled							= qglIsEnabled;
	dllIsEnabledi							= qglIsEnabledi;
	dllIsFramebuffer						= qglIsFramebuffer;
	dllIsList								= qglIsList;
	dllIsProgram							= qglIsProgram;
	dllIsQuery								= qglIsQuery;
	dllIsRenderbuffer						= qglIsRenderbuffer;
	dllIsSampler							= qglIsSampler;
	dllIsShader								= qglIsShader;
	dllIsSync								= qglIsSync;
	dllIsTexture							= qglIsTexture;
	dllIsVertexArray						= qglIsVertexArray;
	dllLightModelf							= qglLightModelf;
	dllLightModelfv							= qglLightModelfv;
	dllLightModeli							= qglLightModeli;
	dllLightModeliv							= qglLightModeliv;
	dllLightf								= qglLightf;
	dllLightfv								= qglLightfv;
	dllLighti								= qglLighti;
	dllLightiv								= qglLightiv;
	dllLineStipple							= qglLineStipple;
	dllLineWidth							= qglLineWidth;
	dllLinkProgram							= qglLinkProgram;
	dllListBase								= qglListBase;
	dllLoadIdentity							= qglLoadIdentity;
	dllLoadMatrixd							= qglLoadMatrixd;
	dllLoadMatrixf							= qglLoadMatrixf;
	dllLoadName								= qglLoadName;
	dllLoadTransposeMatrixd					= qglLoadTransposeMatrixd;
	dllLoadTransposeMatrixf					= qglLoadTransposeMatrixf;
	dllLogicOp								= qglLogicOp;
	dllMap1d								= qglMap1d;
	dllMap1f								= qglMap1f;
	dllMap2d								= qglMap2d;
	dllMap2f								= qglMap2f;
	dllMapBuffer							= qglMapBuffer;
	dllMapBufferRange						= qglMapBufferRange;
	dllMapGrid1d							= qglMapGrid1d;
	dllMapGrid1f							= qglMapGrid1f;
	dllMapGrid2d							= qglMapGrid2d;
	dllMapGrid2f							= qglMapGrid2f;
	dllMaterialf							= qglMaterialf;
	dllMaterialfv							= qglMaterialfv;
	dllMateriali							= qglMateriali;
	dllMaterialiv							= qglMaterialiv;
	dllMatrixMode							= qglMatrixMode;
	dllMultMatrixd							= qglMultMatrixd;
	dllMultMatrixf							= qglMultMatrixf;
	dllMultTransposeMatrixd					= qglMultTransposeMatrixd;
	dllMultTransposeMatrixf					= qglMultTransposeMatrixf;
	dllMultiDrawArrays						= qglMultiDrawArrays;
	dllMultiDrawElements					= qglMultiDrawElements;
	dllMultiDrawElementsBaseVertex			= qglMultiDrawElementsBaseVertex;
	dllMultiTexCoord1d						= qglMultiTexCoord1d;
	dllMultiTexCoord1dv						= qglMultiTexCoord1dv;
	dllMultiTexCoord1f						= qglMultiTexCoord1f;
	dllMultiTexCoord1fv						= qglMultiTexCoord1fv;
	dllMultiTexCoord1i						= qglMultiTexCoord1i;
	dllMultiTexCoord1iv						= qglMultiTexCoord1iv;
	dllMultiTexCoord1s						= qglMultiTexCoord1s;
	dllMultiTexCoord1sv						= qglMultiTexCoord1sv;
	dllMultiTexCoord2d						= qglMultiTexCoord2d;
	dllMultiTexCoord2dv						= qglMultiTexCoord2dv;
	dllMultiTexCoord2f						= qglMultiTexCoord2f;
	dllMultiTexCoord2fv						= qglMultiTexCoord2fv;
	dllMultiTexCoord2i						= qglMultiTexCoord2i;
	dllMultiTexCoord2iv						= qglMultiTexCoord2iv;
	dllMultiTexCoord2s						= qglMultiTexCoord2s;
	dllMultiTexCoord2sv						= qglMultiTexCoord2sv;
	dllMultiTexCoord3d						= qglMultiTexCoord3d;
	dllMultiTexCoord3dv						= qglMultiTexCoord3dv;
	dllMultiTexCoord3f						= qglMultiTexCoord3f;
	dllMultiTexCoord3fv						= qglMultiTexCoord3fv;
	dllMultiTexCoord3i						= qglMultiTexCoord3i;
	dllMultiTexCoord3iv						= qglMultiTexCoord3iv;
	dllMultiTexCoord3s						= qglMultiTexCoord3s;
	dllMultiTexCoord3sv						= qglMultiTexCoord3sv;
	dllMultiTexCoord4d						= qglMultiTexCoord4d;
	dllMultiTexCoord4dv						= qglMultiTexCoord4dv;
	dllMultiTexCoord4f						= qglMultiTexCoord4f;
	dllMultiTexCoord4fv						= qglMultiTexCoord4fv;
	dllMultiTexCoord4i						= qglMultiTexCoord4i;
	dllMultiTexCoord4iv						= qglMultiTexCoord4iv;
	dllMultiTexCoord4s						= qglMultiTexCoord4s;
	dllMultiTexCoord4sv						= qglMultiTexCoord4sv;
	dllMultiTexCoordP1ui					= qglMultiTexCoordP1ui;
	dllMultiTexCoordP1uiv					= qglMultiTexCoordP1uiv;
	dllMultiTexCoordP2ui					= qglMultiTexCoordP2ui;
	dllMultiTexCoordP2uiv					= qglMultiTexCoordP2uiv;
	dllMultiTexCoordP3ui					= qglMultiTexCoordP3ui;
	dllMultiTexCoordP3uiv					= qglMultiTexCoordP3uiv;
	dllMultiTexCoordP4ui					= qglMultiTexCoordP4ui;
	dllMultiTexCoordP4uiv					= qglMultiTexCoordP4uiv;
	dllNewList								= qglNewList;
	dllNormal3b								= qglNormal3b;
	dllNormal3bv							= qglNormal3bv;
	dllNormal3d								= qglNormal3d;
	dllNormal3dv							= qglNormal3dv;
	dllNormal3f								= qglNormal3f;
	dllNormal3fv							= qglNormal3fv;
	dllNormal3i								= qglNormal3i;
	dllNormal3iv							= qglNormal3iv;
	dllNormal3s								= qglNormal3s;
	dllNormal3sv							= qglNormal3sv;
	dllNormalP3ui							= qglNormalP3ui;
	dllNormalP3uiv							= qglNormalP3uiv;
	dllNormalPointer						= qglNormalPointer;
	dllOrtho								= qglOrtho;
	dllPassThrough							= qglPassThrough;
	dllPixelMapfv							= qglPixelMapfv;
	dllPixelMapuiv							= qglPixelMapuiv;
	dllPixelMapusv							= qglPixelMapusv;
	dllPixelStoref							= qglPixelStoref;
	dllPixelStorei							= qglPixelStorei;
	dllPixelTransferf						= qglPixelTransferf;
	dllPixelTransferi						= qglPixelTransferi;
	dllPixelZoom							= qglPixelZoom;
	dllPointParameterf						= qglPointParameterf;
	dllPointParameterfv						= qglPointParameterfv;
	dllPointParameteri						= qglPointParameteri;
	dllPointParameteriv						= qglPointParameteriv;
	dllPointSize							= qglPointSize;
	dllPolygonMode							= qglPolygonMode;
	dllPolygonOffset						= qglPolygonOffset;
	dllPolygonStipple						= qglPolygonStipple;
	dllPopAttrib							= qglPopAttrib;
	dllPopClientAttrib						= qglPopClientAttrib;
	dllPopMatrix							= qglPopMatrix;
	dllPopName								= qglPopName;
	dllPrimitiveRestartIndex				= qglPrimitiveRestartIndex;
	dllPrioritizeTextures					= qglPrioritizeTextures;
	dllProvokingVertex						= qglProvokingVertex;
	dllPushAttrib							= qglPushAttrib;
	dllPushClientAttrib						= qglPushClientAttrib;
	dllPushMatrix							= qglPushMatrix;
	dllPushName								= qglPushName;
	dllQueryCounter							= qglQueryCounter;
	dllRasterPos2d							= qglRasterPos2d;
	dllRasterPos2dv							= qglRasterPos2dv;
	dllRasterPos2f							= qglRasterPos2f;
	dllRasterPos2fv							= qglRasterPos2fv;
	dllRasterPos2i							= qglRasterPos2i;
	dllRasterPos2iv							= qglRasterPos2iv;
	dllRasterPos2s							= qglRasterPos2s;
	dllRasterPos2sv							= qglRasterPos2sv;
	dllRasterPos3d							= qglRasterPos3d;
	dllRasterPos3dv							= qglRasterPos3dv;
	dllRasterPos3f							= qglRasterPos3f;
	dllRasterPos3fv							= qglRasterPos3fv;
	dllRasterPos3i							= qglRasterPos3i;
	dllRasterPos3iv							= qglRasterPos3iv;
	dllRasterPos3s							= qglRasterPos3s;
	dllRasterPos3sv							= qglRasterPos3sv;
	dllRasterPos4d							= qglRasterPos4d;
	dllRasterPos4dv							= qglRasterPos4dv;
	dllRasterPos4f							= qglRasterPos4f;
	dllRasterPos4fv							= qglRasterPos4fv;
	dllRasterPos4i							= qglRasterPos4i;
	dllRasterPos4iv							= qglRasterPos4iv;
	dllRasterPos4s							= qglRasterPos4s;
	dllRasterPos4sv							= qglRasterPos4sv;
	dllReadBuffer							= qglReadBuffer;
	dllReadPixels							= qglReadPixels;
	dllRectd								= qglRectd;
	dllRectdv								= qglRectdv;
	dllRectf								= qglRectf;
	dllRectfv								= qglRectfv;
	dllRecti								= qglRecti;
	dllRectiv								= qglRectiv;
	dllRects								= qglRects;
	dllRectsv								= qglRectsv;
	dllRenderMode							= qglRenderMode;
	dllRenderbufferStorage					= qglRenderbufferStorage;
	dllRenderbufferStorageMultisample		= qglRenderbufferStorageMultisample;
	dllRotated								= qglRotated;
	dllRotatef								= qglRotatef;
	dllSampleCoverage						= qglSampleCoverage;
	dllSampleMaski							= qglSampleMaski;
	dllSamplerParameterIiv					= qglSamplerParameterIiv;
	dllSamplerParameterIuiv					= qglSamplerParameterIuiv;
	dllSamplerParameterf					= qglSamplerParameterf;
	dllSamplerParameterfv					= qglSamplerParameterfv;
	dllSamplerParameteri					= qglSamplerParameteri;
	dllSamplerParameteriv					= qglSamplerParameteriv;
	dllScaled								= qglScaled;
	dllScalef								= qglScalef;
	dllScissor								= qglScissor;
	dllSecondaryColor3b						= qglSecondaryColor3b;
	dllSecondaryColor3bv					= qglSecondaryColor3bv;
	dllSecondaryColor3d						= qglSecondaryColor3d;
	dllSecondaryColor3dv					= qglSecondaryColor3dv;
	dllSecondaryColor3f						= qglSecondaryColor3f;
	dllSecondaryColor3fv					= qglSecondaryColor3fv;
	dllSecondaryColor3i						= qglSecondaryColor3i;
	dllSecondaryColor3iv					= qglSecondaryColor3iv;
	dllSecondaryColor3s						= qglSecondaryColor3s;
	dllSecondaryColor3sv					= qglSecondaryColor3sv;
	dllSecondaryColor3ub					= qglSecondaryColor3ub;
	dllSecondaryColor3ubv					= qglSecondaryColor3ubv;
	dllSecondaryColor3ui					= qglSecondaryColor3ui;
	dllSecondaryColor3uiv					= qglSecondaryColor3uiv;
	dllSecondaryColor3us					= qglSecondaryColor3us;
	dllSecondaryColor3usv					= qglSecondaryColor3usv;
	dllSecondaryColorP3ui					= qglSecondaryColorP3ui;
	dllSecondaryColorP3uiv					= qglSecondaryColorP3uiv;
	dllSecondaryColorPointer				= qglSecondaryColorPointer;
	dllSelectBuffer							= qglSelectBuffer;
	dllShadeModel							= qglShadeModel;
	dllShaderSource							= qglShaderSource;
	dllStencilFunc							= qglStencilFunc;
	dllStencilFuncSeparate					= qglStencilFuncSeparate;
	dllStencilMask							= qglStencilMask;
	dllStencilMaskSeparate					= qglStencilMaskSeparate;
	dllStencilOp							= qglStencilOp;
	dllStencilOpSeparate					= qglStencilOpSeparate;
	dllStencilOpSeparateATI					= qglStencilOpSeparateATI;
	dllStencilFuncSeparateATI				= qglStencilFuncSeparateATI;
	dllTexBuffer							= qglTexBuffer;
	dllTexCoord1d							= qglTexCoord1d;
	dllTexCoord1dv							= qglTexCoord1dv;
	dllTexCoord1f							= qglTexCoord1f;
	dllTexCoord1fv							= qglTexCoord1fv;
	dllTexCoord1i							= qglTexCoord1i;
	dllTexCoord1iv							= qglTexCoord1iv;
	dllTexCoord1s							= qglTexCoord1s;
	dllTexCoord1sv							= qglTexCoord1sv;
	dllTexCoord2d							= qglTexCoord2d;
	dllTexCoord2dv							= qglTexCoord2dv;
	dllTexCoord2f							= qglTexCoord2f;
	dllTexCoord2fv							= qglTexCoord2fv;
	dllTexCoord2i							= qglTexCoord2i;
	dllTexCoord2iv							= qglTexCoord2iv;
	dllTexCoord2s							= qglTexCoord2s;
	dllTexCoord2sv							= qglTexCoord2sv;
	dllTexCoord3d							= qglTexCoord3d;
	dllTexCoord3dv							= qglTexCoord3dv;
	dllTexCoord3f							= qglTexCoord3f;
	dllTexCoord3fv							= qglTexCoord3fv;
	dllTexCoord3i							= qglTexCoord3i;
	dllTexCoord3iv							= qglTexCoord3iv;
	dllTexCoord3s							= qglTexCoord3s;
	dllTexCoord3sv							= qglTexCoord3sv;
	dllTexCoord4d							= qglTexCoord4d;
	dllTexCoord4dv							= qglTexCoord4dv;
	dllTexCoord4f							= qglTexCoord4f;
	dllTexCoord4fv							= qglTexCoord4fv;
	dllTexCoord4i							= qglTexCoord4i;
	dllTexCoord4iv							= qglTexCoord4iv;
	dllTexCoord4s							= qglTexCoord4s;
	dllTexCoord4sv							= qglTexCoord4sv;
	dllTexCoordP1ui							= qglTexCoordP1ui;
	dllTexCoordP1uiv						= qglTexCoordP1uiv;
	dllTexCoordP2ui							= qglTexCoordP2ui;
	dllTexCoordP2uiv						= qglTexCoordP2uiv;
	dllTexCoordP3ui							= qglTexCoordP3ui;
	dllTexCoordP3uiv						= qglTexCoordP3uiv;
	dllTexCoordP4ui							= qglTexCoordP4ui;
	dllTexCoordP4uiv						= qglTexCoordP4uiv;
	dllTexCoordPointer						= qglTexCoordPointer;
	dllTexEnvf								= qglTexEnvf;
	dllTexEnvfv								= qglTexEnvfv;
	dllTexEnvi								= qglTexEnvi;
	dllTexEnviv								= qglTexEnviv;
	dllTexGend								= qglTexGend;
	dllTexGendv								= qglTexGendv;
	dllTexGenf								= qglTexGenf;
	dllTexGenfv								= qglTexGenfv;
	dllTexGeni								= qglTexGeni;
	dllTexGeniv								= qglTexGeniv;
	dllTexImage1D							= qglTexImage1D;
	dllTexImage2D							= qglTexImage2D;
	dllTexImage2DMultisample				= qglTexImage2DMultisample;
	dllTexImage3D							= qglTexImage3D;
	dllTexImage3DMultisample				= qglTexImage3DMultisample;
	dllTexParameterIiv						= qglTexParameterIiv;
	dllTexParameterIuiv						= qglTexParameterIuiv;
	dllTexParameterf						= qglTexParameterf;
	dllTexParameterfv						= qglTexParameterfv;
	dllTexParameteri						= qglTexParameteri;
	dllTexParameteriv						= qglTexParameteriv;
	dllTexSubImage1D						= qglTexSubImage1D;
	dllTexSubImage2D						= qglTexSubImage2D;
	dllTexSubImage3D						= qglTexSubImage3D;
	dllTransformFeedbackVaryings			= qglTransformFeedbackVaryings;
	dllTranslated							= qglTranslated;
	dllTranslatef							= qglTranslatef;
	dllUniform1f							= qglUniform1f;
	dllUniform1fv							= qglUniform1fv;
	dllUniform1i							= qglUniform1i;
	dllUniform1iv							= qglUniform1iv;
	dllUniform1ui							= qglUniform1ui;
	dllUniform1uiv							= qglUniform1uiv;
	dllUniform2f							= qglUniform2f;
	dllUniform2fv							= qglUniform2fv;
	dllUniform2i							= qglUniform2i;
	dllUniform2iv							= qglUniform2iv;
	dllUniform2ui							= qglUniform2ui;
	dllUniform2uiv							= qglUniform2uiv;
	dllUniform3f							= qglUniform3f;
	dllUniform3fv							= qglUniform3fv;
	dllUniform3i							= qglUniform3i;
	dllUniform3iv							= qglUniform3iv;
	dllUniform3ui							= qglUniform3ui;
	dllUniform3uiv							= qglUniform3uiv;
	dllUniform4f							= qglUniform4f;
	dllUniform4fv							= qglUniform4fv;
	dllUniform4i							= qglUniform4i;
	dllUniform4iv							= qglUniform4iv;
	dllUniform4ui							= qglUniform4ui;
	dllUniform4uiv							= qglUniform4uiv;
	dllUniformBlockBinding					= qglUniformBlockBinding;
	dllUniformMatrix2fv						= qglUniformMatrix2fv;
	dllUniformMatrix2x3fv					= qglUniformMatrix2x3fv;
	dllUniformMatrix2x4fv					= qglUniformMatrix2x4fv;
	dllUniformMatrix3fv						= qglUniformMatrix3fv;
	dllUniformMatrix3x2fv					= qglUniformMatrix3x2fv;
	dllUniformMatrix3x4fv					= qglUniformMatrix3x4fv;
	dllUniformMatrix4fv						= qglUniformMatrix4fv;
	dllUniformMatrix4x2fv					= qglUniformMatrix4x2fv;
	dllUniformMatrix4x3fv					= qglUniformMatrix4x3fv;
	dllUnmapBuffer							= qglUnmapBuffer;
	dllUseProgram							= qglUseProgram;
	dllValidateProgram						= qglValidateProgram;
	dllVertex2d								= qglVertex2d;
	dllVertex2dv							= qglVertex2dv;
	dllVertex2f								= qglVertex2f;
	dllVertex2fv							= qglVertex2fv;
	dllVertex2i								= qglVertex2i;
	dllVertex2iv							= qglVertex2iv;
	dllVertex2s								= qglVertex2s;
	dllVertex2sv							= qglVertex2sv;
	dllVertex3d								= qglVertex3d;
	dllVertex3dv							= qglVertex3dv;
	dllVertex3f								= qglVertex3f;
	dllVertex3fv							= qglVertex3fv;
	dllVertex3i								= qglVertex3i;
	dllVertex3iv							= qglVertex3iv;
	dllVertex3s								= qglVertex3s;
	dllVertex3sv							= qglVertex3sv;
	dllVertex4d								= qglVertex4d;
	dllVertex4dv							= qglVertex4dv;
	dllVertex4f								= qglVertex4f;
	dllVertex4fv							= qglVertex4fv;
	dllVertex4i								= qglVertex4i;
	dllVertex4iv							= qglVertex4iv;
	dllVertex4s								= qglVertex4s;
	dllVertex4sv							= qglVertex4sv;
	dllVertexAttrib1d						= qglVertexAttrib1d;
	dllVertexAttrib1dv						= qglVertexAttrib1dv;
	dllVertexAttrib1f						= qglVertexAttrib1f;
	dllVertexAttrib1fv						= qglVertexAttrib1fv;
	dllVertexAttrib1s						= qglVertexAttrib1s;
	dllVertexAttrib1sv						= qglVertexAttrib1sv;
	dllVertexAttrib2d						= qglVertexAttrib2d;
	dllVertexAttrib2dv						= qglVertexAttrib2dv;
	dllVertexAttrib2f						= qglVertexAttrib2f;
	dllVertexAttrib2fv						= qglVertexAttrib2fv;
	dllVertexAttrib2s						= qglVertexAttrib2s;
	dllVertexAttrib2sv						= qglVertexAttrib2sv;
	dllVertexAttrib3d						= qglVertexAttrib3d;
	dllVertexAttrib3dv						= qglVertexAttrib3dv;
	dllVertexAttrib3f						= qglVertexAttrib3f;
	dllVertexAttrib3fv						= qglVertexAttrib3fv;
	dllVertexAttrib3s						= qglVertexAttrib3s;
	dllVertexAttrib3sv						= qglVertexAttrib3sv;
	dllVertexAttrib4Nbv						= qglVertexAttrib4Nbv;
	dllVertexAttrib4Niv						= qglVertexAttrib4Niv;
	dllVertexAttrib4Nsv						= qglVertexAttrib4Nsv;
	dllVertexAttrib4Nub						= qglVertexAttrib4Nub;
	dllVertexAttrib4Nubv					= qglVertexAttrib4Nubv;
	dllVertexAttrib4Nuiv					= qglVertexAttrib4Nuiv;
	dllVertexAttrib4Nusv					= qglVertexAttrib4Nusv;
	dllVertexAttrib4bv						= qglVertexAttrib4bv;
	dllVertexAttrib4d						= qglVertexAttrib4d;
	dllVertexAttrib4dv						= qglVertexAttrib4dv;
	dllVertexAttrib4f						= qglVertexAttrib4f;
	dllVertexAttrib4fv						= qglVertexAttrib4fv;
	dllVertexAttrib4iv						= qglVertexAttrib4iv;
	dllVertexAttrib4s						= qglVertexAttrib4s;
	dllVertexAttrib4sv						= qglVertexAttrib4sv;
	dllVertexAttrib4ubv						= qglVertexAttrib4ubv;
	dllVertexAttrib4uiv						= qglVertexAttrib4uiv;
	dllVertexAttrib4usv						= qglVertexAttrib4usv;
	dllVertexAttribI1i						= qglVertexAttribI1i;
	dllVertexAttribI1iv						= qglVertexAttribI1iv;
	dllVertexAttribI1ui						= qglVertexAttribI1ui;
	dllVertexAttribI1uiv					= qglVertexAttribI1uiv;
	dllVertexAttribI2i						= qglVertexAttribI2i;
	dllVertexAttribI2iv						= qglVertexAttribI2iv;
	dllVertexAttribI2ui						= qglVertexAttribI2ui;
	dllVertexAttribI2uiv					= qglVertexAttribI2uiv;
	dllVertexAttribI3i						= qglVertexAttribI3i;
	dllVertexAttribI3iv						= qglVertexAttribI3iv;
	dllVertexAttribI3ui						= qglVertexAttribI3ui;
	dllVertexAttribI3uiv					= qglVertexAttribI3uiv;
	dllVertexAttribI4bv						= qglVertexAttribI4bv;
	dllVertexAttribI4i						= qglVertexAttribI4i;
	dllVertexAttribI4iv						= qglVertexAttribI4iv;
	dllVertexAttribI4sv						= qglVertexAttribI4sv;
	dllVertexAttribI4ubv					= qglVertexAttribI4ubv;
	dllVertexAttribI4ui						= qglVertexAttribI4ui;
	dllVertexAttribI4uiv					= qglVertexAttribI4uiv;
	dllVertexAttribI4usv					= qglVertexAttribI4usv;
	dllVertexAttribIPointer					= qglVertexAttribIPointer;
	dllVertexAttribP1ui						= qglVertexAttribP1ui;
	dllVertexAttribP1uiv					= qglVertexAttribP1uiv;
	dllVertexAttribP2ui						= qglVertexAttribP2ui;
	dllVertexAttribP2uiv					= qglVertexAttribP2uiv;
	dllVertexAttribP3ui						= qglVertexAttribP3ui;
	dllVertexAttribP3uiv					= qglVertexAttribP3uiv;
	dllVertexAttribP4ui						= qglVertexAttribP4ui;
	dllVertexAttribP4uiv					= qglVertexAttribP4uiv;
	dllVertexAttribPointer					= qglVertexAttribPointer;
	dllVertexP2ui							= qglVertexP2ui;
	dllVertexP2uiv							= qglVertexP2uiv;
	dllVertexP3ui							= qglVertexP3ui;
	dllVertexP3uiv							= qglVertexP3uiv;
	dllVertexP4ui							= qglVertexP4ui;
	dllVertexP4uiv							= qglVertexP4uiv;
	dllVertexPointer						= qglVertexPointer;
	dllViewport								= qglViewport;
	dllWaitSync								= qglWaitSync;
	dllWindowPos2d							= qglWindowPos2d;
	dllWindowPos2dv							= qglWindowPos2dv;
	dllWindowPos2f							= qglWindowPos2f;
	dllWindowPos2fv							= qglWindowPos2fv;
	dllWindowPos2i							= qglWindowPos2i;
	dllWindowPos2iv							= qglWindowPos2iv;
	dllWindowPos2s							= qglWindowPos2s;
	dllWindowPos2sv							= qglWindowPos2sv;
	dllWindowPos3d							= qglWindowPos3d;
	dllWindowPos3dv							= qglWindowPos3dv;
	dllWindowPos3f							= qglWindowPos3f;
	dllWindowPos3fv							= qglWindowPos3fv;
	dllWindowPos3i							= qglWindowPos3i;
	dllWindowPos3iv							= qglWindowPos3iv;
	dllWindowPos3s							= qglWindowPos3s;
	dllWindowPos3sv							= qglWindowPos3sv;
}

/*
 ==================
 QGL_EnableLogging
 ==================
*/
void QGL_EnableLogging (bool enable){

	FILE		*f;
	time_t		t;
	struct tm	*lt;

	// Copy the function pointers if needed
	if (!qglState.pointersCopied){
		qglState.pointersCopied = true;

		QGL_CopyPointers();
	}

	// Enable or disable logging
	if (enable){
		if (qglState.logFile){
			qglState.logFileFrames++;
			return;
		}

		// Find a file name to save it to
		while (qglState.logFileNumber <= 9999){
			Str_SPrintf(qglState.logFileName, sizeof(qglState.logFileName), "%s/renderer_%04i.log", Sys_DefaultSaveDirectory(), qglState.logFileNumber);

			f = fopen(qglState.logFileName, "rt");
			if (!f)
				break;

			fclose(f);

			qglState.logFileNumber++;
		}

		if (qglState.logFileNumber == 10000)
			return;
		qglState.logFileNumber++;

		// Open the file
		qglState.logFile = fopen(qglState.logFileName, "wt");
		if (!qglState.logFile){
			Com_Printf("Couldn't open log file '%s'\n", qglState.logFileName);
			return;
		}

		Com_Printf("Opened log file '%s'\n", qglState.logFileName);

		qglState.logFileFrames = 1;

		time(&t);
		lt = localtime(&t);

		fprintf(qglState.logFile, "%s %s (%s %s)", ENGINE_VERSION, BUILD_STRING, __DATE__, __TIME__);
		fprintf(qglState.logFile, "\nLog file opened on %s\n\n", asctime(lt));

		// Change the function pointers
		qglAccum								= logAccum;
		qglActiveStencilFaceEXT					= logActiveStencilFaceEXT;
		qglActiveTexture						= logActiveTexture;
		qglAlphaFunc							= logAlphaFunc;
		qglAreTexturesResident					= logAreTexturesResident;
		qglArrayElement							= logArrayElement;
		qglAttachShader							= logAttachShader;
		qglBegin								= logBegin;
		qglBeginConditionalRender				= logBeginConditionalRender;
		qglBeginQuery							= logBeginQuery;
		qglBeginTransformFeedback				= logBeginTransformFeedback;
		qglBindAttribLocation					= logBindAttribLocation;
		qglBindBuffer							= logBindBuffer;
		qglBindBufferBase						= logBindBufferBase;
		qglBindBufferRange						= logBindBufferRange;
		qglBindFragDataLocation					= logBindFragDataLocation;
		qglBindFragDataLocationIndexed			= logBindFragDataLocationIndexed;
		qglBindFramebuffer						= logBindFramebuffer;
		qglBindRenderbuffer						= logBindRenderbuffer;
		qglBindSampler							= logBindSampler;
		qglBindTexture							= logBindTexture;
		qglBindVertexArray						= logBindVertexArray;
		qglBitmap								= logBitmap;
		qglBlendColor							= logBlendColor;
		qglBlendEquation						= logBlendEquation;
		qglBlendEquationSeparate				= logBlendEquationSeparate;
		qglBlendFunc							= logBlendFunc;
		qglBlendFuncSeparate					= logBlendFuncSeparate;
		qglBlitFramebuffer						= logBlitFramebuffer;
		qglBufferData							= logBufferData;
		qglBufferSubData						= logBufferSubData;
		qglCallList								= logCallList;
		qglCallLists							= logCallLists;
		qglCheckFramebufferStatus				= logCheckFramebufferStatus;
		qglClampColor							= logClampColor;
		qglClear								= logClear;
		qglClearAccum							= logClearAccum;
		qglClearBufferfi						= logClearBufferfi;
		qglClearBufferfv						= logClearBufferfv;
		qglClearBufferiv						= logClearBufferiv;
		qglClearBufferuiv						= logClearBufferuiv;
		qglClearColor							= logClearColor;
		qglClearDepth							= logClearDepth;
		qglClearIndex							= logClearIndex;
		qglClearStencil							= logClearStencil;
		qglClientActiveTexture					= logClientActiveTexture;
		qglClientWaitSync						= logClientWaitSync;
		qglClipPlane							= logClipPlane;
		qglColor3b								= logColor3b;
		qglColor3bv								= logColor3bv;
		qglColor3d								= logColor3d;
		qglColor3dv								= logColor3dv;
		qglColor3f								= logColor3f;
		qglColor3fv								= logColor3fv;
		qglColor3i								= logColor3i;
		qglColor3iv								= logColor3iv;
		qglColor3s								= logColor3s;
		qglColor3sv								= logColor3sv;
		qglColor3ub								= logColor3ub;
		qglColor3ubv							= logColor3ubv;
		qglColor3ui								= logColor3ui;
		qglColor3uiv							= logColor3uiv;
		qglColor3us								= logColor3us;
		qglColor3usv							= logColor3usv;
		qglColor4b								= logColor4b;
		qglColor4bv								= logColor4bv;
		qglColor4d								= logColor4d;
		qglColor4dv								= logColor4dv;
		qglColor4f								= logColor4f;
		qglColor4fv								= logColor4fv;
		qglColor4i								= logColor4i;
		qglColor4iv								= logColor4iv;
		qglColor4s								= logColor4s;
		qglColor4sv								= logColor4sv;
		qglColor4ub								= logColor4ub;
		qglColor4ubv							= logColor4ubv;
		qglColor4ui								= logColor4ui;
		qglColor4uiv							= logColor4uiv;
		qglColor4us								= logColor4us;
		qglColor4usv							= logColor4usv;
		qglColorMask							= logColorMask;
		qglColorMaski							= logColorMaski;
		qglColorMaterial						= logColorMaterial;
		qglColorP3ui							= logColorP3ui;
		qglColorP3uiv							= logColorP3uiv;
		qglColorP4ui							= logColorP4ui;
		qglColorP4uiv							= logColorP4uiv;
		qglColorPointer							= logColorPointer;
		qglCompileShader						= logCompileShader;
		qglCompressedTexImage1D					= logCompressedTexImage1D;
		qglCompressedTexImage2D					= logCompressedTexImage2D;
		qglCompressedTexImage3D					= logCompressedTexImage3D;
		qglCompressedTexSubImage1D				= logCompressedTexSubImage1D;
		qglCompressedTexSubImage2D				= logCompressedTexSubImage2D;
		qglCompressedTexSubImage3D				= logCompressedTexSubImage3D;
		qglCopyBufferSubData					= logCopyBufferSubData;
		qglCopyPixels							= logCopyPixels;
		qglCopyTexImage1D						= logCopyTexImage1D;
		qglCopyTexImage2D						= logCopyTexImage2D;
		qglCopyTexSubImage1D					= logCopyTexSubImage1D;
		qglCopyTexSubImage2D					= logCopyTexSubImage2D;
		qglCopyTexSubImage3D					= logCopyTexSubImage3D;
		qglCreateProgram						= logCreateProgram;
		qglCreateShader							= logCreateShader;
		qglCullFace								= logCullFace;
		qglDeleteBuffers						= logDeleteBuffers;
		qglDeleteFramebuffers					= logDeleteFramebuffers;
		qglDeleteLists							= logDeleteLists;
		qglDeleteProgram						= logDeleteProgram;
		qglDeleteQueries						= logDeleteQueries;
		qglDeleteRenderbuffers					= logDeleteRenderbuffers;
		qglDeleteSamplers						= logDeleteSamplers;
		qglDeleteShader							= logDeleteShader;
		qglDeleteSync							= logDeleteSync;
		qglDeleteTextures						= logDeleteTextures;
		qglDeleteVertexArrays					= logDeleteVertexArrays;
		qglDepthBoundsEXT						= logDepthBoundsEXT;
		qglDepthFunc							= logDepthFunc;
		qglDepthMask							= logDepthMask;
		qglDepthRange							= logDepthRange;
		qglDetachShader							= logDetachShader;
		qglDisable								= logDisable;
		qglDisableClientState					= logDisableClientState;
		qglDisableVertexAttribArray				= logDisableVertexAttribArray;
		qglDisablei								= logDisablei;
		qglDrawArrays							= logDrawArrays;
		qglDrawArraysInstanced					= logDrawArraysInstanced;
		qglDrawBuffer							= logDrawBuffer;
		qglDrawBuffers							= logDrawBuffers;
		qglDrawElements							= logDrawElements;
		qglDrawElementsBaseVertex				= logDrawElementsBaseVertex;
		qglDrawElementsInstanced				= logDrawElementsInstanced;
		qglDrawElementsInstancedBaseVertex		= logDrawElementsInstancedBaseVertex;
		qglDrawPixels							= logDrawPixels;
		qglDrawRangeElements					= logDrawRangeElements;
		qglDrawRangeElementsBaseVertex			= logDrawRangeElementsBaseVertex;
		qglEdgeFlag								= logEdgeFlag;
		qglEdgeFlagPointer						= logEdgeFlagPointer;
		qglEdgeFlagv							= logEdgeFlagv;
		qglEnable								= logEnable;
		qglEnableClientState					= logEnableClientState;
		qglEnableVertexAttribArray				= logEnableVertexAttribArray;
		qglEnablei								= logEnablei;
		qglEnd									= logEnd;
		qglEndConditionalRender					= logEndConditionalRender;
		qglEndList								= logEndList;
		qglEndQuery								= logEndQuery;
		qglEndTransformFeedback					= logEndTransformFeedback;
		qglEvalCoord1d							= logEvalCoord1d;
		qglEvalCoord1dv							= logEvalCoord1dv;
		qglEvalCoord1f							= logEvalCoord1f;
		qglEvalCoord1fv							= logEvalCoord1fv;
		qglEvalCoord2d							= logEvalCoord2d;
		qglEvalCoord2dv							= logEvalCoord2dv;
		qglEvalCoord2f							= logEvalCoord2f;
		qglEvalCoord2fv							= logEvalCoord2fv;
		qglEvalMesh1							= logEvalMesh1;
		qglEvalMesh2							= logEvalMesh2;
		qglEvalPoint1							= logEvalPoint1;
		qglEvalPoint2							= logEvalPoint2;
		qglFeedbackBuffer						= logFeedbackBuffer;
		qglFenceSync							= logFenceSync;
		qglFinish								= logFinish;
		qglFlush								= logFlush;
		qglFlushMappedBufferRange				= logFlushMappedBufferRange;
		qglFogCoordPointer						= logFogCoordPointer;
		qglFogCoordd							= logFogCoordd;
		qglFogCoorddv							= logFogCoorddv;
		qglFogCoordf							= logFogCoordf;
		qglFogCoordfv							= logFogCoordfv;
		qglFogf									= logFogf;
		qglFogfv								= logFogfv;
		qglFogi									= logFogi;
		qglFogiv								= logFogiv;
		qglFramebufferRenderbuffer				= logFramebufferRenderbuffer;
		qglFramebufferTexture					= logFramebufferTexture;
		qglFramebufferTexture1D					= logFramebufferTexture1D;
		qglFramebufferTexture2D					= logFramebufferTexture2D;
		qglFramebufferTexture3D					= logFramebufferTexture3D;
		qglFramebufferTextureLayer				= logFramebufferTextureLayer;
		qglFrontFace							= logFrontFace;
		qglFrustum								= logFrustum;
		qglGenBuffers							= logGenBuffers;
		qglGenFramebuffers						= logGenFramebuffers;
		qglGenLists								= logGenLists;
		qglGenQueries							= logGenQueries;
		qglGenRenderbuffers						= logGenRenderbuffers;
		qglGenSamplers							= logGenSamplers;
		qglGenTextures							= logGenTextures;
		qglGenVertexArrays						= logGenVertexArrays;
		qglGenerateMipmap						= logGenerateMipmap;
		qglGetActiveAttrib						= logGetActiveAttrib;
		qglGetActiveUniform						= logGetActiveUniform;
		qglGetActiveUniformBlockName			= logGetActiveUniformBlockName;
		qglGetActiveUniformBlockiv				= logGetActiveUniformBlockiv;
		qglGetActiveUniformName					= logGetActiveUniformName;
		qglGetActiveUniformsiv					= logGetActiveUniformsiv;
		qglGetAttachedShaders					= logGetAttachedShaders;
		qglGetAttribLocation					= logGetAttribLocation;
		qglGetBooleani_v						= logGetBooleani_v;
		qglGetBooleanv							= logGetBooleanv;
		qglGetBufferParameteri64v				= logGetBufferParameteri64v;
		qglGetBufferParameteriv					= logGetBufferParameteriv;
		qglGetBufferPointerv					= logGetBufferPointerv;
		qglGetBufferSubData						= logGetBufferSubData;
		qglGetClipPlane							= logGetClipPlane;
		qglGetCompressedTexImage				= logGetCompressedTexImage;
		qglGetDoublev							= logGetDoublev;
		qglGetError								= logGetError;
		qglGetFloatv							= logGetFloatv;
		qglGetFragDataIndex						= logGetFragDataIndex;
		qglGetFragDataLocation					= logGetFragDataLocation;
		qglGetFramebufferAttachmentParameteriv	= logGetFramebufferAttachmentParameteriv;
		qglGetInteger64i_v						= logGetInteger64i_v;
		qglGetInteger64v						= logGetInteger64v;
		qglGetIntegeri_v						= logGetIntegeri_v;
		qglGetIntegerv							= logGetIntegerv;
		qglGetLightfv							= logGetLightfv;
		qglGetLightiv							= logGetLightiv;
		qglGetMapdv								= logGetMapdv;
		qglGetMapfv								= logGetMapfv;
		qglGetMapiv								= logGetMapiv;
		qglGetMaterialfv						= logGetMaterialfv;
		qglGetMaterialiv						= logGetMaterialiv;
		qglGetMultisamplefv						= logGetMultisamplefv;
		qglGetPixelMapfv						= logGetPixelMapfv;
		qglGetPixelMapuiv						= logGetPixelMapuiv;
		qglGetPixelMapusv						= logGetPixelMapusv;
		qglGetPointerv							= logGetPointerv;
		qglGetPolygonStipple					= logGetPolygonStipple;
		qglGetProgramInfoLog					= logGetProgramInfoLog;
		qglGetProgramiv							= logGetProgramiv;
		qglGetQueryObjecti64v					= logGetQueryObjecti64v;
		qglGetQueryObjectiv						= logGetQueryObjectiv;
		qglGetQueryObjectui64v					= logGetQueryObjectui64v;
		qglGetQueryObjectuiv					= logGetQueryObjectuiv;
		qglGetQueryiv							= logGetQueryiv;
		qglGetRenderbufferParameteriv			= logGetRenderbufferParameteriv;
		qglGetSamplerParameterIiv				= logGetSamplerParameterIiv;
		qglGetSamplerParameterIuiv				= logGetSamplerParameterIuiv;
		qglGetSamplerParameterfv				= logGetSamplerParameterfv;
		qglGetSamplerParameteriv				= logGetSamplerParameteriv;
		qglGetShaderInfoLog						= logGetShaderInfoLog;
		qglGetShaderSource						= logGetShaderSource;
		qglGetShaderiv							= logGetShaderiv;
		qglGetString							= logGetString;
		qglGetStringi							= logGetStringi;
		qglGetSynciv							= logGetSynciv;
		qglGetTexEnvfv							= logGetTexEnvfv;
		qglGetTexEnviv							= logGetTexEnviv;
		qglGetTexGendv							= logGetTexGendv;
		qglGetTexGenfv							= logGetTexGenfv;
		qglGetTexGeniv							= logGetTexGeniv;
		qglGetTexImage							= logGetTexImage;
		qglGetTexLevelParameterfv				= logGetTexLevelParameterfv;
		qglGetTexLevelParameteriv				= logGetTexLevelParameteriv;
		qglGetTexParameterIiv					= logGetTexParameterIiv;
		qglGetTexParameterIuiv					= logGetTexParameterIuiv;
		qglGetTexParameterfv					= logGetTexParameterfv;
		qglGetTexParameteriv					= logGetTexParameteriv;
		qglGetTransformFeedbackVarying			= logGetTransformFeedbackVarying;
		qglGetUniformBlockIndex					= logGetUniformBlockIndex;
		qglGetUniformIndices					= logGetUniformIndices;
		qglGetUniformLocation					= logGetUniformLocation;
		qglGetUniformfv							= logGetUniformfv;
		qglGetUniformiv							= logGetUniformiv;
		qglGetUniformuiv						= logGetUniformuiv;
		qglGetVertexAttribIiv					= logGetVertexAttribIiv;
		qglGetVertexAttribIuiv					= logGetVertexAttribIuiv;
		qglGetVertexAttribPointerv				= logGetVertexAttribPointerv;
		qglGetVertexAttribdv					= logGetVertexAttribdv;
		qglGetVertexAttribfv					= logGetVertexAttribfv;
		qglGetVertexAttribiv					= logGetVertexAttribiv;
		qglHint									= logHint;
		qglIndexMask							= logIndexMask;
		qglIndexPointer							= logIndexPointer;
		qglIndexd								= logIndexd;
		qglIndexdv								= logIndexdv;
		qglIndexf								= logIndexf;
		qglIndexfv								= logIndexfv;
		qglIndexi								= logIndexi;
		qglIndexiv								= logIndexiv;
		qglIndexs								= logIndexs;
		qglIndexsv								= logIndexsv;
		qglIndexub								= logIndexub;
		qglIndexubv								= logIndexubv;
		qglInitNames							= logInitNames;
		qglInterleavedArrays					= logInterleavedArrays;
		qglIsBuffer								= logIsBuffer;
		qglIsEnabled							= logIsEnabled;
		qglIsEnabledi							= logIsEnabledi;
		qglIsFramebuffer						= logIsFramebuffer;
		qglIsList								= logIsList;
		qglIsProgram							= logIsProgram;
		qglIsQuery								= logIsQuery;
		qglIsRenderbuffer						= logIsRenderbuffer;
		qglIsSampler							= logIsSampler;
		qglIsShader								= logIsShader;
		qglIsSync								= logIsSync;
		qglIsTexture							= logIsTexture;
		qglIsVertexArray						= logIsVertexArray;
		qglLightModelf							= logLightModelf;
		qglLightModelfv							= logLightModelfv;
		qglLightModeli							= logLightModeli;
		qglLightModeliv							= logLightModeliv;
		qglLightf								= logLightf;
		qglLightfv								= logLightfv;
		qglLighti								= logLighti;
		qglLightiv								= logLightiv;
		qglLineStipple							= logLineStipple;
		qglLineWidth							= logLineWidth;
		qglLinkProgram							= logLinkProgram;
		qglListBase								= logListBase;
		qglLoadIdentity							= logLoadIdentity;
		qglLoadMatrixd							= logLoadMatrixd;
		qglLoadMatrixf							= logLoadMatrixf;
		qglLoadName								= logLoadName;
		qglLoadTransposeMatrixd					= logLoadTransposeMatrixd;
		qglLoadTransposeMatrixf					= logLoadTransposeMatrixf;
		qglLogicOp								= logLogicOp;
		qglMap1d								= logMap1d;
		qglMap1f								= logMap1f;
		qglMap2d								= logMap2d;
		qglMap2f								= logMap2f;
		qglMapBuffer							= logMapBuffer;
		qglMapBufferRange						= logMapBufferRange;
		qglMapGrid1d							= logMapGrid1d;
		qglMapGrid1f							= logMapGrid1f;
		qglMapGrid2d							= logMapGrid2d;
		qglMapGrid2f							= logMapGrid2f;
		qglMaterialf							= logMaterialf;
		qglMaterialfv							= logMaterialfv;
		qglMateriali							= logMateriali;
		qglMaterialiv							= logMaterialiv;
		qglMatrixMode							= logMatrixMode;
		qglMultMatrixd							= logMultMatrixd;
		qglMultMatrixf							= logMultMatrixf;
		qglMultTransposeMatrixd					= logMultTransposeMatrixd;
		qglMultTransposeMatrixf					= logMultTransposeMatrixf;
		qglMultiDrawArrays						= logMultiDrawArrays;
		qglMultiDrawElements					= logMultiDrawElements;
		qglMultiDrawElementsBaseVertex			= logMultiDrawElementsBaseVertex;
		qglMultiTexCoord1d						= logMultiTexCoord1d;
		qglMultiTexCoord1dv						= logMultiTexCoord1dv;
		qglMultiTexCoord1f						= logMultiTexCoord1f;
		qglMultiTexCoord1fv						= logMultiTexCoord1fv;
		qglMultiTexCoord1i						= logMultiTexCoord1i;
		qglMultiTexCoord1iv						= logMultiTexCoord1iv;
		qglMultiTexCoord1s						= logMultiTexCoord1s;
		qglMultiTexCoord1sv						= logMultiTexCoord1sv;
		qglMultiTexCoord2d						= logMultiTexCoord2d;
		qglMultiTexCoord2dv						= logMultiTexCoord2dv;
		qglMultiTexCoord2f						= logMultiTexCoord2f;
		qglMultiTexCoord2fv						= logMultiTexCoord2fv;
		qglMultiTexCoord2i						= logMultiTexCoord2i;
		qglMultiTexCoord2iv						= logMultiTexCoord2iv;
		qglMultiTexCoord2s						= logMultiTexCoord2s;
		qglMultiTexCoord2sv						= logMultiTexCoord2sv;
		qglMultiTexCoord3d						= logMultiTexCoord3d;
		qglMultiTexCoord3dv						= logMultiTexCoord3dv;
		qglMultiTexCoord3f						= logMultiTexCoord3f;
		qglMultiTexCoord3fv						= logMultiTexCoord3fv;
		qglMultiTexCoord3i						= logMultiTexCoord3i;
		qglMultiTexCoord3iv						= logMultiTexCoord3iv;
		qglMultiTexCoord3s						= logMultiTexCoord3s;
		qglMultiTexCoord3sv						= logMultiTexCoord3sv;
		qglMultiTexCoord4d						= logMultiTexCoord4d;
		qglMultiTexCoord4dv						= logMultiTexCoord4dv;
		qglMultiTexCoord4f						= logMultiTexCoord4f;
		qglMultiTexCoord4fv						= logMultiTexCoord4fv;
		qglMultiTexCoord4i						= logMultiTexCoord4i;
		qglMultiTexCoord4iv						= logMultiTexCoord4iv;
		qglMultiTexCoord4s						= logMultiTexCoord4s;
		qglMultiTexCoord4sv						= logMultiTexCoord4sv;
		qglMultiTexCoordP1ui					= logMultiTexCoordP1ui;
		qglMultiTexCoordP1uiv					= logMultiTexCoordP1uiv;
		qglMultiTexCoordP2ui					= logMultiTexCoordP2ui;
		qglMultiTexCoordP2uiv					= logMultiTexCoordP2uiv;
		qglMultiTexCoordP3ui					= logMultiTexCoordP3ui;
		qglMultiTexCoordP3uiv					= logMultiTexCoordP3uiv;
		qglMultiTexCoordP4ui					= logMultiTexCoordP4ui;
		qglMultiTexCoordP4uiv					= logMultiTexCoordP4uiv;
		qglNewList								= logNewList;
		qglNormal3b								= logNormal3b;
		qglNormal3bv							= logNormal3bv;
		qglNormal3d								= logNormal3d;
		qglNormal3dv							= logNormal3dv;
		qglNormal3f								= logNormal3f;
		qglNormal3fv							= logNormal3fv;
		qglNormal3i								= logNormal3i;
		qglNormal3iv							= logNormal3iv;
		qglNormal3s								= logNormal3s;
		qglNormal3sv							= logNormal3sv;
		qglNormalP3ui							= logNormalP3ui;
		qglNormalP3uiv							= logNormalP3uiv;
		qglNormalPointer						= logNormalPointer;
		qglOrtho								= logOrtho;
		qglPassThrough							= logPassThrough;
		qglPixelMapfv							= logPixelMapfv;
		qglPixelMapuiv							= logPixelMapuiv;
		qglPixelMapusv							= logPixelMapusv;
		qglPixelStoref							= logPixelStoref;
		qglPixelStorei							= logPixelStorei;
		qglPixelTransferf						= logPixelTransferf;
		qglPixelTransferi						= logPixelTransferi;
		qglPixelZoom							= logPixelZoom;
		qglPointParameterf						= logPointParameterf;
		qglPointParameterfv						= logPointParameterfv;
		qglPointParameteri						= logPointParameteri;
		qglPointParameteriv						= logPointParameteriv;
		qglPointSize							= logPointSize;
		qglPolygonMode							= logPolygonMode;
		qglPolygonOffset						= logPolygonOffset;
		qglPolygonStipple						= logPolygonStipple;
		qglPopAttrib							= logPopAttrib;
		qglPopClientAttrib						= logPopClientAttrib;
		qglPopMatrix							= logPopMatrix;
		qglPopName								= logPopName;
		qglPrimitiveRestartIndex				= logPrimitiveRestartIndex;
		qglPrioritizeTextures					= logPrioritizeTextures;
		qglProvokingVertex						= logProvokingVertex;
		qglPushAttrib							= logPushAttrib;
		qglPushClientAttrib						= logPushClientAttrib;
		qglPushMatrix							= logPushMatrix;
		qglPushName								= logPushName;
		qglQueryCounter							= logQueryCounter;
		qglRasterPos2d							= logRasterPos2d;
		qglRasterPos2dv							= logRasterPos2dv;
		qglRasterPos2f							= logRasterPos2f;
		qglRasterPos2fv							= logRasterPos2fv;
		qglRasterPos2i							= logRasterPos2i;
		qglRasterPos2iv							= logRasterPos2iv;
		qglRasterPos2s							= logRasterPos2s;
		qglRasterPos2sv							= logRasterPos2sv;
		qglRasterPos3d							= logRasterPos3d;
		qglRasterPos3dv							= logRasterPos3dv;
		qglRasterPos3f							= logRasterPos3f;
		qglRasterPos3fv							= logRasterPos3fv;
		qglRasterPos3i							= logRasterPos3i;
		qglRasterPos3iv							= logRasterPos3iv;
		qglRasterPos3s							= logRasterPos3s;
		qglRasterPos3sv							= logRasterPos3sv;
		qglRasterPos4d							= logRasterPos4d;
		qglRasterPos4dv							= logRasterPos4dv;
		qglRasterPos4f							= logRasterPos4f;
		qglRasterPos4fv							= logRasterPos4fv;
		qglRasterPos4i							= logRasterPos4i;
		qglRasterPos4iv							= logRasterPos4iv;
		qglRasterPos4s							= logRasterPos4s;
		qglRasterPos4sv							= logRasterPos4sv;
		qglReadBuffer							= logReadBuffer;
		qglReadPixels							= logReadPixels;
		qglRectd								= logRectd;
		qglRectdv								= logRectdv;
		qglRectf								= logRectf;
		qglRectfv								= logRectfv;
		qglRecti								= logRecti;
		qglRectiv								= logRectiv;
		qglRects								= logRects;
		qglRectsv								= logRectsv;
		qglRenderMode							= logRenderMode;
		qglRenderbufferStorage					= logRenderbufferStorage;
		qglRenderbufferStorageMultisample		= logRenderbufferStorageMultisample;
		qglRotated								= logRotated;
		qglRotatef								= logRotatef;
		qglSampleCoverage						= logSampleCoverage;
		qglSampleMaski							= logSampleMaski;
		qglSamplerParameterIiv					= logSamplerParameterIiv;
		qglSamplerParameterIuiv					= logSamplerParameterIuiv;
		qglSamplerParameterf					= logSamplerParameterf;
		qglSamplerParameterfv					= logSamplerParameterfv;
		qglSamplerParameteri					= logSamplerParameteri;
		qglSamplerParameteriv					= logSamplerParameteriv;
		qglScaled								= logScaled;
		qglScalef								= logScalef;
		qglScissor								= logScissor;
		qglSecondaryColor3b						= logSecondaryColor3b;
		qglSecondaryColor3bv					= logSecondaryColor3bv;
		qglSecondaryColor3d						= logSecondaryColor3d;
		qglSecondaryColor3dv					= logSecondaryColor3dv;
		qglSecondaryColor3f						= logSecondaryColor3f;
		qglSecondaryColor3fv					= logSecondaryColor3fv;
		qglSecondaryColor3i						= logSecondaryColor3i;
		qglSecondaryColor3iv					= logSecondaryColor3iv;
		qglSecondaryColor3s						= logSecondaryColor3s;
		qglSecondaryColor3sv					= logSecondaryColor3sv;
		qglSecondaryColor3ub					= logSecondaryColor3ub;
		qglSecondaryColor3ubv					= logSecondaryColor3ubv;
		qglSecondaryColor3ui					= logSecondaryColor3ui;
		qglSecondaryColor3uiv					= logSecondaryColor3uiv;
		qglSecondaryColor3us					= logSecondaryColor3us;
		qglSecondaryColor3usv					= logSecondaryColor3usv;
		qglSecondaryColorP3ui					= logSecondaryColorP3ui;
		qglSecondaryColorP3uiv					= logSecondaryColorP3uiv;
		qglSecondaryColorPointer				= logSecondaryColorPointer;
		qglSelectBuffer							= logSelectBuffer;
		qglShadeModel							= logShadeModel;
		qglShaderSource							= logShaderSource;
		qglStencilFunc							= logStencilFunc;
		qglStencilFuncSeparate					= logStencilFuncSeparate;
		qglStencilMask							= logStencilMask;
		qglStencilMaskSeparate					= logStencilMaskSeparate;
		qglStencilOp							= logStencilOp;
		qglStencilOpSeparate					= logStencilOpSeparate;
		qglStencilOpSeparateATI					= logStencilOpSeparateATI;
		qglStencilFuncSeparateATI				= logStencilFuncSeparateATI;
		qglTexBuffer							= logTexBuffer;
		qglTexCoord1d							= logTexCoord1d;
		qglTexCoord1dv							= logTexCoord1dv;
		qglTexCoord1f							= logTexCoord1f;
		qglTexCoord1fv							= logTexCoord1fv;
		qglTexCoord1i							= logTexCoord1i;
		qglTexCoord1iv							= logTexCoord1iv;
		qglTexCoord1s							= logTexCoord1s;
		qglTexCoord1sv							= logTexCoord1sv;
		qglTexCoord2d							= logTexCoord2d;
		qglTexCoord2dv							= logTexCoord2dv;
		qglTexCoord2f							= logTexCoord2f;
		qglTexCoord2fv							= logTexCoord2fv;
		qglTexCoord2i							= logTexCoord2i;
		qglTexCoord2iv							= logTexCoord2iv;
		qglTexCoord2s							= logTexCoord2s;
		qglTexCoord2sv							= logTexCoord2sv;
		qglTexCoord3d							= logTexCoord3d;
		qglTexCoord3dv							= logTexCoord3dv;
		qglTexCoord3f							= logTexCoord3f;
		qglTexCoord3fv							= logTexCoord3fv;
		qglTexCoord3i							= logTexCoord3i;
		qglTexCoord3iv							= logTexCoord3iv;
		qglTexCoord3s							= logTexCoord3s;
		qglTexCoord3sv							= logTexCoord3sv;
		qglTexCoord4d							= logTexCoord4d;
		qglTexCoord4dv							= logTexCoord4dv;
		qglTexCoord4f							= logTexCoord4f;
		qglTexCoord4fv							= logTexCoord4fv;
		qglTexCoord4i							= logTexCoord4i;
		qglTexCoord4iv							= logTexCoord4iv;
		qglTexCoord4s							= logTexCoord4s;
		qglTexCoord4sv							= logTexCoord4sv;
		qglTexCoordP1ui							= logTexCoordP1ui;
		qglTexCoordP1uiv						= logTexCoordP1uiv;
		qglTexCoordP2ui							= logTexCoordP2ui;
		qglTexCoordP2uiv						= logTexCoordP2uiv;
		qglTexCoordP3ui							= logTexCoordP3ui;
		qglTexCoordP3uiv						= logTexCoordP3uiv;
		qglTexCoordP4ui							= logTexCoordP4ui;
		qglTexCoordP4uiv						= logTexCoordP4uiv;
		qglTexCoordPointer						= logTexCoordPointer;
		qglTexEnvf								= logTexEnvf;
		qglTexEnvfv								= logTexEnvfv;
		qglTexEnvi								= logTexEnvi;
		qglTexEnviv								= logTexEnviv;
		qglTexGend								= logTexGend;
		qglTexGendv								= logTexGendv;
		qglTexGenf								= logTexGenf;
		qglTexGenfv								= logTexGenfv;
		qglTexGeni								= logTexGeni;
		qglTexGeniv								= logTexGeniv;
		qglTexImage1D							= logTexImage1D;
		qglTexImage2D							= logTexImage2D;
		qglTexImage2DMultisample				= logTexImage2DMultisample;
		qglTexImage3D							= logTexImage3D;
		qglTexImage3DMultisample				= logTexImage3DMultisample;
		qglTexParameterIiv						= logTexParameterIiv;
		qglTexParameterIuiv						= logTexParameterIuiv;
		qglTexParameterf						= logTexParameterf;
		qglTexParameterfv						= logTexParameterfv;
		qglTexParameteri						= logTexParameteri;
		qglTexParameteriv						= logTexParameteriv;
		qglTexSubImage1D						= logTexSubImage1D;
		qglTexSubImage2D						= logTexSubImage2D;
		qglTexSubImage3D						= logTexSubImage3D;
		qglTransformFeedbackVaryings			= logTransformFeedbackVaryings;
		qglTranslated							= logTranslated;
		qglTranslatef							= logTranslatef;
		qglUniform1f							= logUniform1f;
		qglUniform1fv							= logUniform1fv;
		qglUniform1i							= logUniform1i;
		qglUniform1iv							= logUniform1iv;
		qglUniform1ui							= logUniform1ui;
		qglUniform1uiv							= logUniform1uiv;
		qglUniform2f							= logUniform2f;
		qglUniform2fv							= logUniform2fv;
		qglUniform2i							= logUniform2i;
		qglUniform2iv							= logUniform2iv;
		qglUniform2ui							= logUniform2ui;
		qglUniform2uiv							= logUniform2uiv;
		qglUniform3f							= logUniform3f;
		qglUniform3fv							= logUniform3fv;
		qglUniform3i							= logUniform3i;
		qglUniform3iv							= logUniform3iv;
		qglUniform3ui							= logUniform3ui;
		qglUniform3uiv							= logUniform3uiv;
		qglUniform4f							= logUniform4f;
		qglUniform4fv							= logUniform4fv;
		qglUniform4i							= logUniform4i;
		qglUniform4iv							= logUniform4iv;
		qglUniform4ui							= logUniform4ui;
		qglUniform4uiv							= logUniform4uiv;
		qglUniformBlockBinding					= logUniformBlockBinding;
		qglUniformMatrix2fv						= logUniformMatrix2fv;
		qglUniformMatrix2x3fv					= logUniformMatrix2x3fv;
		qglUniformMatrix2x4fv					= logUniformMatrix2x4fv;
		qglUniformMatrix3fv						= logUniformMatrix3fv;
		qglUniformMatrix3x2fv					= logUniformMatrix3x2fv;
		qglUniformMatrix3x4fv					= logUniformMatrix3x4fv;
		qglUniformMatrix4fv						= logUniformMatrix4fv;
		qglUniformMatrix4x2fv					= logUniformMatrix4x2fv;
		qglUniformMatrix4x3fv					= logUniformMatrix4x3fv;
		qglUnmapBuffer							= logUnmapBuffer;
		qglUseProgram							= logUseProgram;
		qglValidateProgram						= logValidateProgram;
		qglVertex2d								= logVertex2d;
		qglVertex2dv							= logVertex2dv;
		qglVertex2f								= logVertex2f;
		qglVertex2fv							= logVertex2fv;
		qglVertex2i								= logVertex2i;
		qglVertex2iv							= logVertex2iv;
		qglVertex2s								= logVertex2s;
		qglVertex2sv							= logVertex2sv;
		qglVertex3d								= logVertex3d;
		qglVertex3dv							= logVertex3dv;
		qglVertex3f								= logVertex3f;
		qglVertex3fv							= logVertex3fv;
		qglVertex3i								= logVertex3i;
		qglVertex3iv							= logVertex3iv;
		qglVertex3s								= logVertex3s;
		qglVertex3sv							= logVertex3sv;
		qglVertex4d								= logVertex4d;
		qglVertex4dv							= logVertex4dv;
		qglVertex4f								= logVertex4f;
		qglVertex4fv							= logVertex4fv;
		qglVertex4i								= logVertex4i;
		qglVertex4iv							= logVertex4iv;
		qglVertex4s								= logVertex4s;
		qglVertex4sv							= logVertex4sv;
		qglVertexAttrib1d						= logVertexAttrib1d;
		qglVertexAttrib1dv						= logVertexAttrib1dv;
		qglVertexAttrib1f						= logVertexAttrib1f;
		qglVertexAttrib1fv						= logVertexAttrib1fv;
		qglVertexAttrib1s						= logVertexAttrib1s;
		qglVertexAttrib1sv						= logVertexAttrib1sv;
		qglVertexAttrib2d						= logVertexAttrib2d;
		qglVertexAttrib2dv						= logVertexAttrib2dv;
		qglVertexAttrib2f						= logVertexAttrib2f;
		qglVertexAttrib2fv						= logVertexAttrib2fv;
		qglVertexAttrib2s						= logVertexAttrib2s;
		qglVertexAttrib2sv						= logVertexAttrib2sv;
		qglVertexAttrib3d						= logVertexAttrib3d;
		qglVertexAttrib3dv						= logVertexAttrib3dv;
		qglVertexAttrib3f						= logVertexAttrib3f;
		qglVertexAttrib3fv						= logVertexAttrib3fv;
		qglVertexAttrib3s						= logVertexAttrib3s;
		qglVertexAttrib3sv						= logVertexAttrib3sv;
		qglVertexAttrib4Nbv						= logVertexAttrib4Nbv;
		qglVertexAttrib4Niv						= logVertexAttrib4Niv;
		qglVertexAttrib4Nsv						= logVertexAttrib4Nsv;
		qglVertexAttrib4Nub						= logVertexAttrib4Nub;
		qglVertexAttrib4Nubv					= logVertexAttrib4Nubv;
		qglVertexAttrib4Nuiv					= logVertexAttrib4Nuiv;
		qglVertexAttrib4Nusv					= logVertexAttrib4Nusv;
		qglVertexAttrib4bv						= logVertexAttrib4bv;
		qglVertexAttrib4d						= logVertexAttrib4d;
		qglVertexAttrib4dv						= logVertexAttrib4dv;
		qglVertexAttrib4f						= logVertexAttrib4f;
		qglVertexAttrib4fv						= logVertexAttrib4fv;
		qglVertexAttrib4iv						= logVertexAttrib4iv;
		qglVertexAttrib4s						= logVertexAttrib4s;
		qglVertexAttrib4sv						= logVertexAttrib4sv;
		qglVertexAttrib4ubv						= logVertexAttrib4ubv;
		qglVertexAttrib4uiv						= logVertexAttrib4uiv;
		qglVertexAttrib4usv						= logVertexAttrib4usv;
		qglVertexAttribI1i						= logVertexAttribI1i;
		qglVertexAttribI1iv						= logVertexAttribI1iv;
		qglVertexAttribI1ui						= logVertexAttribI1ui;
		qglVertexAttribI1uiv					= logVertexAttribI1uiv;
		qglVertexAttribI2i						= logVertexAttribI2i;
		qglVertexAttribI2iv						= logVertexAttribI2iv;
		qglVertexAttribI2ui						= logVertexAttribI2ui;
		qglVertexAttribI2uiv					= logVertexAttribI2uiv;
		qglVertexAttribI3i						= logVertexAttribI3i;
		qglVertexAttribI3iv						= logVertexAttribI3iv;
		qglVertexAttribI3ui						= logVertexAttribI3ui;
		qglVertexAttribI3uiv					= logVertexAttribI3uiv;
		qglVertexAttribI4bv						= logVertexAttribI4bv;
		qglVertexAttribI4i						= logVertexAttribI4i;
		qglVertexAttribI4iv						= logVertexAttribI4iv;
		qglVertexAttribI4sv						= logVertexAttribI4sv;
		qglVertexAttribI4ubv					= logVertexAttribI4ubv;
		qglVertexAttribI4ui						= logVertexAttribI4ui;
		qglVertexAttribI4uiv					= logVertexAttribI4uiv;
		qglVertexAttribI4usv					= logVertexAttribI4usv;
		qglVertexAttribIPointer					= logVertexAttribIPointer;
		qglVertexAttribP1ui						= logVertexAttribP1ui;
		qglVertexAttribP1uiv					= logVertexAttribP1uiv;
		qglVertexAttribP2ui						= logVertexAttribP2ui;
		qglVertexAttribP2uiv					= logVertexAttribP2uiv;
		qglVertexAttribP3ui						= logVertexAttribP3ui;
		qglVertexAttribP3uiv					= logVertexAttribP3uiv;
		qglVertexAttribP4ui						= logVertexAttribP4ui;
		qglVertexAttribP4uiv					= logVertexAttribP4uiv;
		qglVertexAttribPointer					= logVertexAttribPointer;
		qglVertexP2ui							= logVertexP2ui;
		qglVertexP2uiv							= logVertexP2uiv;
		qglVertexP3ui							= logVertexP3ui;
		qglVertexP3uiv							= logVertexP3uiv;
		qglVertexP4ui							= logVertexP4ui;
		qglVertexP4uiv							= logVertexP4uiv;
		qglVertexPointer						= logVertexPointer;
		qglViewport								= logViewport;
		qglWaitSync								= logWaitSync;
		qglWindowPos2d							= logWindowPos2d;
		qglWindowPos2dv							= logWindowPos2dv;
		qglWindowPos2f							= logWindowPos2f;
		qglWindowPos2fv							= logWindowPos2fv;
		qglWindowPos2i							= logWindowPos2i;
		qglWindowPos2iv							= logWindowPos2iv;
		qglWindowPos2s							= logWindowPos2s;
		qglWindowPos2sv							= logWindowPos2sv;
		qglWindowPos3d							= logWindowPos3d;
		qglWindowPos3dv							= logWindowPos3dv;
		qglWindowPos3f							= logWindowPos3f;
		qglWindowPos3fv							= logWindowPos3fv;
		qglWindowPos3i							= logWindowPos3i;
		qglWindowPos3iv							= logWindowPos3iv;
		qglWindowPos3s							= logWindowPos3s;
		qglWindowPos3sv							= logWindowPos3sv;
	}
	else {
		if (!qglState.logFile)
			return;

		// Close the file
		time(&t);
		lt = localtime(&t);

		fprintf(qglState.logFile, "\nLog file closed on %s\n\n", asctime(lt));

		fclose(qglState.logFile);
		qglState.logFile = NULL;

		Com_Printf("Closed log file '%s' after %i frames\n", qglState.logFileName, qglState.logFileFrames);

		// Reset the function pointers
		qglAccum								= dllAccum;
		qglActiveStencilFaceEXT					= dllActiveStencilFaceEXT;
		qglActiveTexture						= dllActiveTexture;
		qglAlphaFunc							= dllAlphaFunc;
		qglAreTexturesResident					= dllAreTexturesResident;
		qglArrayElement							= dllArrayElement;
		qglAttachShader							= dllAttachShader;
		qglBegin								= dllBegin;
		qglBeginConditionalRender				= dllBeginConditionalRender;
		qglBeginQuery							= dllBeginQuery;
		qglBeginTransformFeedback				= dllBeginTransformFeedback;
		qglBindAttribLocation					= dllBindAttribLocation;
		qglBindBuffer							= dllBindBuffer;
		qglBindBufferBase						= dllBindBufferBase;
		qglBindBufferRange						= dllBindBufferRange;
		qglBindFragDataLocation					= dllBindFragDataLocation;
		qglBindFragDataLocationIndexed			= dllBindFragDataLocationIndexed;
		qglBindFramebuffer						= dllBindFramebuffer;
		qglBindRenderbuffer						= dllBindRenderbuffer;
		qglBindSampler							= dllBindSampler;
		qglBindTexture							= dllBindTexture;
		qglBindVertexArray						= dllBindVertexArray;
		qglBitmap								= dllBitmap;
		qglBlendColor							= dllBlendColor;
		qglBlendEquation						= dllBlendEquation;
		qglBlendEquationSeparate				= dllBlendEquationSeparate;
		qglBlendFunc							= dllBlendFunc;
		qglBlendFuncSeparate					= dllBlendFuncSeparate;
		qglBlitFramebuffer						= dllBlitFramebuffer;
		qglBufferData							= dllBufferData;
		qglBufferSubData						= dllBufferSubData;
		qglCallList								= dllCallList;
		qglCallLists							= dllCallLists;
		qglCheckFramebufferStatus				= dllCheckFramebufferStatus;
		qglClampColor							= dllClampColor;
		qglClear								= dllClear;
		qglClearAccum							= dllClearAccum;
		qglClearBufferfi						= dllClearBufferfi;
		qglClearBufferfv						= dllClearBufferfv;
		qglClearBufferiv						= dllClearBufferiv;
		qglClearBufferuiv						= dllClearBufferuiv;
		qglClearColor							= dllClearColor;
		qglClearDepth							= dllClearDepth;
		qglClearIndex							= dllClearIndex;
		qglClearStencil							= dllClearStencil;
		qglClientActiveTexture					= dllClientActiveTexture;
		qglClientWaitSync						= dllClientWaitSync;
		qglClipPlane							= dllClipPlane;
		qglColor3b								= dllColor3b;
		qglColor3bv								= dllColor3bv;
		qglColor3d								= dllColor3d;
		qglColor3dv								= dllColor3dv;
		qglColor3f								= dllColor3f;
		qglColor3fv								= dllColor3fv;
		qglColor3i								= dllColor3i;
		qglColor3iv								= dllColor3iv;
		qglColor3s								= dllColor3s;
		qglColor3sv								= dllColor3sv;
		qglColor3ub								= dllColor3ub;
		qglColor3ubv							= dllColor3ubv;
		qglColor3ui								= dllColor3ui;
		qglColor3uiv							= dllColor3uiv;
		qglColor3us								= dllColor3us;
		qglColor3usv							= dllColor3usv;
		qglColor4b								= dllColor4b;
		qglColor4bv								= dllColor4bv;
		qglColor4d								= dllColor4d;
		qglColor4dv								= dllColor4dv;
		qglColor4f								= dllColor4f;
		qglColor4fv								= dllColor4fv;
		qglColor4i								= dllColor4i;
		qglColor4iv								= dllColor4iv;
		qglColor4s								= dllColor4s;
		qglColor4sv								= dllColor4sv;
		qglColor4ub								= dllColor4ub;
		qglColor4ubv							= dllColor4ubv;
		qglColor4ui								= dllColor4ui;
		qglColor4uiv							= dllColor4uiv;
		qglColor4us								= dllColor4us;
		qglColor4usv							= dllColor4usv;
		qglColorMask							= dllColorMask;
		qglColorMaski							= dllColorMaski;
		qglColorMaterial						= dllColorMaterial;
		qglColorP3ui							= dllColorP3ui;
		qglColorP3uiv							= dllColorP3uiv;
		qglColorP4ui							= dllColorP4ui;
		qglColorP4uiv							= dllColorP4uiv;
		qglColorPointer							= dllColorPointer;
		qglCompileShader						= dllCompileShader;
		qglCompressedTexImage1D					= dllCompressedTexImage1D;
		qglCompressedTexImage2D					= dllCompressedTexImage2D;
		qglCompressedTexImage3D					= dllCompressedTexImage3D;
		qglCompressedTexSubImage1D				= dllCompressedTexSubImage1D;
		qglCompressedTexSubImage2D				= dllCompressedTexSubImage2D;
		qglCompressedTexSubImage3D				= dllCompressedTexSubImage3D;
		qglCopyBufferSubData					= dllCopyBufferSubData;
		qglCopyPixels							= dllCopyPixels;
		qglCopyTexImage1D						= dllCopyTexImage1D;
		qglCopyTexImage2D						= dllCopyTexImage2D;
		qglCopyTexSubImage1D					= dllCopyTexSubImage1D;
		qglCopyTexSubImage2D					= dllCopyTexSubImage2D;
		qglCopyTexSubImage3D					= dllCopyTexSubImage3D;
		qglCreateProgram						= dllCreateProgram;
		qglCreateShader							= dllCreateShader;
		qglCullFace								= dllCullFace;
		qglDeleteBuffers						= dllDeleteBuffers;
		qglDeleteFramebuffers					= dllDeleteFramebuffers;
		qglDeleteLists							= dllDeleteLists;
		qglDeleteProgram						= dllDeleteProgram;
		qglDeleteQueries						= dllDeleteQueries;
		qglDeleteRenderbuffers					= dllDeleteRenderbuffers;
		qglDeleteSamplers						= dllDeleteSamplers;
		qglDeleteShader							= dllDeleteShader;
		qglDeleteSync							= dllDeleteSync;
		qglDeleteTextures						= dllDeleteTextures;
		qglDeleteVertexArrays					= dllDeleteVertexArrays;
		qglDepthBoundsEXT						= dllDepthBoundsEXT;
		qglDepthFunc							= dllDepthFunc;
		qglDepthMask							= dllDepthMask;
		qglDepthRange							= dllDepthRange;
		qglDetachShader							= dllDetachShader;
		qglDisable								= dllDisable;
		qglDisableClientState					= dllDisableClientState;
		qglDisableVertexAttribArray				= dllDisableVertexAttribArray;
		qglDisablei								= dllDisablei;
		qglDrawArrays							= dllDrawArrays;
		qglDrawArraysInstanced					= dllDrawArraysInstanced;
		qglDrawBuffer							= dllDrawBuffer;
		qglDrawBuffers							= dllDrawBuffers;
		qglDrawElements							= dllDrawElements;
		qglDrawElementsBaseVertex				= dllDrawElementsBaseVertex;
		qglDrawElementsInstanced				= dllDrawElementsInstanced;
		qglDrawElementsInstancedBaseVertex		= dllDrawElementsInstancedBaseVertex;
		qglDrawPixels							= dllDrawPixels;
		qglDrawRangeElements					= dllDrawRangeElements;
		qglDrawRangeElementsBaseVertex			= dllDrawRangeElementsBaseVertex;
		qglEdgeFlag								= dllEdgeFlag;
		qglEdgeFlagPointer						= dllEdgeFlagPointer;
		qglEdgeFlagv							= dllEdgeFlagv;
		qglEnable								= dllEnable;
		qglEnableClientState					= dllEnableClientState;
		qglEnableVertexAttribArray				= dllEnableVertexAttribArray;
		qglEnablei								= dllEnablei;
		qglEnd									= dllEnd;
		qglEndConditionalRender					= dllEndConditionalRender;
		qglEndList								= dllEndList;
		qglEndQuery								= dllEndQuery;
		qglEndTransformFeedback					= dllEndTransformFeedback;
		qglEvalCoord1d							= dllEvalCoord1d;
		qglEvalCoord1dv							= dllEvalCoord1dv;
		qglEvalCoord1f							= dllEvalCoord1f;
		qglEvalCoord1fv							= dllEvalCoord1fv;
		qglEvalCoord2d							= dllEvalCoord2d;
		qglEvalCoord2dv							= dllEvalCoord2dv;
		qglEvalCoord2f							= dllEvalCoord2f;
		qglEvalCoord2fv							= dllEvalCoord2fv;
		qglEvalMesh1							= dllEvalMesh1;
		qglEvalMesh2							= dllEvalMesh2;
		qglEvalPoint1							= dllEvalPoint1;
		qglEvalPoint2							= dllEvalPoint2;
		qglFeedbackBuffer						= dllFeedbackBuffer;
		qglFenceSync							= dllFenceSync;
		qglFinish								= dllFinish;
		qglFlush								= dllFlush;
		qglFlushMappedBufferRange				= dllFlushMappedBufferRange;
		qglFogCoordPointer						= dllFogCoordPointer;
		qglFogCoordd							= dllFogCoordd;
		qglFogCoorddv							= dllFogCoorddv;
		qglFogCoordf							= dllFogCoordf;
		qglFogCoordfv							= dllFogCoordfv;
		qglFogf									= dllFogf;
		qglFogfv								= dllFogfv;
		qglFogi									= dllFogi;
		qglFogiv								= dllFogiv;
		qglFramebufferRenderbuffer				= dllFramebufferRenderbuffer;
		qglFramebufferTexture					= dllFramebufferTexture;
		qglFramebufferTexture1D					= dllFramebufferTexture1D;
		qglFramebufferTexture2D					= dllFramebufferTexture2D;
		qglFramebufferTexture3D					= dllFramebufferTexture3D;
		qglFramebufferTextureLayer				= dllFramebufferTextureLayer;
		qglFrontFace							= dllFrontFace;
		qglFrustum								= dllFrustum;
		qglGenBuffers							= dllGenBuffers;
		qglGenFramebuffers						= dllGenFramebuffers;
		qglGenLists								= dllGenLists;
		qglGenQueries							= dllGenQueries;
		qglGenRenderbuffers						= dllGenRenderbuffers;
		qglGenSamplers							= dllGenSamplers;
		qglGenTextures							= dllGenTextures;
		qglGenVertexArrays						= dllGenVertexArrays;
		qglGenerateMipmap						= dllGenerateMipmap;
		qglGetActiveAttrib						= dllGetActiveAttrib;
		qglGetActiveUniform						= dllGetActiveUniform;
		qglGetActiveUniformBlockName			= dllGetActiveUniformBlockName;
		qglGetActiveUniformBlockiv				= dllGetActiveUniformBlockiv;
		qglGetActiveUniformName					= dllGetActiveUniformName;
		qglGetActiveUniformsiv					= dllGetActiveUniformsiv;
		qglGetAttachedShaders					= dllGetAttachedShaders;
		qglGetAttribLocation					= dllGetAttribLocation;
		qglGetBooleani_v						= dllGetBooleani_v;
		qglGetBooleanv							= dllGetBooleanv;
		qglGetBufferParameteri64v				= dllGetBufferParameteri64v;
		qglGetBufferParameteriv					= dllGetBufferParameteriv;
		qglGetBufferPointerv					= dllGetBufferPointerv;
		qglGetBufferSubData						= dllGetBufferSubData;
		qglGetClipPlane							= dllGetClipPlane;
		qglGetCompressedTexImage				= dllGetCompressedTexImage;
		qglGetDoublev							= dllGetDoublev;
		qglGetError								= dllGetError;
		qglGetFloatv							= dllGetFloatv;
		qglGetFragDataIndex						= dllGetFragDataIndex;
		qglGetFragDataLocation					= dllGetFragDataLocation;
		qglGetFramebufferAttachmentParameteriv	= dllGetFramebufferAttachmentParameteriv;
		qglGetInteger64i_v						= dllGetInteger64i_v;
		qglGetInteger64v						= dllGetInteger64v;
		qglGetIntegeri_v						= dllGetIntegeri_v;
		qglGetIntegerv							= dllGetIntegerv;
		qglGetLightfv							= dllGetLightfv;
		qglGetLightiv							= dllGetLightiv;
		qglGetMapdv								= dllGetMapdv;
		qglGetMapfv								= dllGetMapfv;
		qglGetMapiv								= dllGetMapiv;
		qglGetMaterialfv						= dllGetMaterialfv;
		qglGetMaterialiv						= dllGetMaterialiv;
		qglGetMultisamplefv						= dllGetMultisamplefv;
		qglGetPixelMapfv						= dllGetPixelMapfv;
		qglGetPixelMapuiv						= dllGetPixelMapuiv;
		qglGetPixelMapusv						= dllGetPixelMapusv;
		qglGetPointerv							= dllGetPointerv;
		qglGetPolygonStipple					= dllGetPolygonStipple;
		qglGetProgramInfoLog					= dllGetProgramInfoLog;
		qglGetProgramiv							= dllGetProgramiv;
		qglGetQueryObjecti64v					= dllGetQueryObjecti64v;
		qglGetQueryObjectiv						= dllGetQueryObjectiv;
		qglGetQueryObjectui64v					= dllGetQueryObjectui64v;
		qglGetQueryObjectuiv					= dllGetQueryObjectuiv;
		qglGetQueryiv							= dllGetQueryiv;
		qglGetRenderbufferParameteriv			= dllGetRenderbufferParameteriv;
		qglGetSamplerParameterIiv				= dllGetSamplerParameterIiv;
		qglGetSamplerParameterIuiv				= dllGetSamplerParameterIuiv;
		qglGetSamplerParameterfv				= dllGetSamplerParameterfv;
		qglGetSamplerParameteriv				= dllGetSamplerParameteriv;
		qglGetShaderInfoLog						= dllGetShaderInfoLog;
		qglGetShaderSource						= dllGetShaderSource;
		qglGetShaderiv							= dllGetShaderiv;
		qglGetString							= dllGetString;
		qglGetStringi							= dllGetStringi;
		qglGetSynciv							= dllGetSynciv;
		qglGetTexEnvfv							= dllGetTexEnvfv;
		qglGetTexEnviv							= dllGetTexEnviv;
		qglGetTexGendv							= dllGetTexGendv;
		qglGetTexGenfv							= dllGetTexGenfv;
		qglGetTexGeniv							= dllGetTexGeniv;
		qglGetTexImage							= dllGetTexImage;
		qglGetTexLevelParameterfv				= dllGetTexLevelParameterfv;
		qglGetTexLevelParameteriv				= dllGetTexLevelParameteriv;
		qglGetTexParameterIiv					= dllGetTexParameterIiv;
		qglGetTexParameterIuiv					= dllGetTexParameterIuiv;
		qglGetTexParameterfv					= dllGetTexParameterfv;
		qglGetTexParameteriv					= dllGetTexParameteriv;
		qglGetTransformFeedbackVarying			= dllGetTransformFeedbackVarying;
		qglGetUniformBlockIndex					= dllGetUniformBlockIndex;
		qglGetUniformIndices					= dllGetUniformIndices;
		qglGetUniformLocation					= dllGetUniformLocation;
		qglGetUniformfv							= dllGetUniformfv;
		qglGetUniformiv							= dllGetUniformiv;
		qglGetUniformuiv						= dllGetUniformuiv;
		qglGetVertexAttribIiv					= dllGetVertexAttribIiv;
		qglGetVertexAttribIuiv					= dllGetVertexAttribIuiv;
		qglGetVertexAttribPointerv				= dllGetVertexAttribPointerv;
		qglGetVertexAttribdv					= dllGetVertexAttribdv;
		qglGetVertexAttribfv					= dllGetVertexAttribfv;
		qglGetVertexAttribiv					= dllGetVertexAttribiv;
		qglHint									= dllHint;
		qglIndexMask							= dllIndexMask;
		qglIndexPointer							= dllIndexPointer;
		qglIndexd								= dllIndexd;
		qglIndexdv								= dllIndexdv;
		qglIndexf								= dllIndexf;
		qglIndexfv								= dllIndexfv;
		qglIndexi								= dllIndexi;
		qglIndexiv								= dllIndexiv;
		qglIndexs								= dllIndexs;
		qglIndexsv								= dllIndexsv;
		qglIndexub								= dllIndexub;
		qglIndexubv								= dllIndexubv;
		qglInitNames							= dllInitNames;
		qglInterleavedArrays					= dllInterleavedArrays;
		qglIsBuffer								= dllIsBuffer;
		qglIsEnabled							= dllIsEnabled;
		qglIsEnabledi							= dllIsEnabledi;
		qglIsFramebuffer						= dllIsFramebuffer;
		qglIsList								= dllIsList;
		qglIsProgram							= dllIsProgram;
		qglIsQuery								= dllIsQuery;
		qglIsRenderbuffer						= dllIsRenderbuffer;
		qglIsSampler							= dllIsSampler;
		qglIsShader								= dllIsShader;
		qglIsSync								= dllIsSync;
		qglIsTexture							= dllIsTexture;
		qglIsVertexArray						= dllIsVertexArray;
		qglLightModelf							= dllLightModelf;
		qglLightModelfv							= dllLightModelfv;
		qglLightModeli							= dllLightModeli;
		qglLightModeliv							= dllLightModeliv;
		qglLightf								= dllLightf;
		qglLightfv								= dllLightfv;
		qglLighti								= dllLighti;
		qglLightiv								= dllLightiv;
		qglLineStipple							= dllLineStipple;
		qglLineWidth							= dllLineWidth;
		qglLinkProgram							= dllLinkProgram;
		qglListBase								= dllListBase;
		qglLoadIdentity							= dllLoadIdentity;
		qglLoadMatrixd							= dllLoadMatrixd;
		qglLoadMatrixf							= dllLoadMatrixf;
		qglLoadName								= dllLoadName;
		qglLoadTransposeMatrixd					= dllLoadTransposeMatrixd;
		qglLoadTransposeMatrixf					= dllLoadTransposeMatrixf;
		qglLogicOp								= dllLogicOp;
		qglMap1d								= dllMap1d;
		qglMap1f								= dllMap1f;
		qglMap2d								= dllMap2d;
		qglMap2f								= dllMap2f;
		qglMapBuffer							= dllMapBuffer;
		qglMapBufferRange						= dllMapBufferRange;
		qglMapGrid1d							= dllMapGrid1d;
		qglMapGrid1f							= dllMapGrid1f;
		qglMapGrid2d							= dllMapGrid2d;
		qglMapGrid2f							= dllMapGrid2f;
		qglMaterialf							= dllMaterialf;
		qglMaterialfv							= dllMaterialfv;
		qglMateriali							= dllMateriali;
		qglMaterialiv							= dllMaterialiv;
		qglMatrixMode							= dllMatrixMode;
		qglMultMatrixd							= dllMultMatrixd;
		qglMultMatrixf							= dllMultMatrixf;
		qglMultTransposeMatrixd					= dllMultTransposeMatrixd;
		qglMultTransposeMatrixf					= dllMultTransposeMatrixf;
		qglMultiDrawArrays						= dllMultiDrawArrays;
		qglMultiDrawElements					= dllMultiDrawElements;
		qglMultiDrawElementsBaseVertex			= dllMultiDrawElementsBaseVertex;
		qglMultiTexCoord1d						= dllMultiTexCoord1d;
		qglMultiTexCoord1dv						= dllMultiTexCoord1dv;
		qglMultiTexCoord1f						= dllMultiTexCoord1f;
		qglMultiTexCoord1fv						= dllMultiTexCoord1fv;
		qglMultiTexCoord1i						= dllMultiTexCoord1i;
		qglMultiTexCoord1iv						= dllMultiTexCoord1iv;
		qglMultiTexCoord1s						= dllMultiTexCoord1s;
		qglMultiTexCoord1sv						= dllMultiTexCoord1sv;
		qglMultiTexCoord2d						= dllMultiTexCoord2d;
		qglMultiTexCoord2dv						= dllMultiTexCoord2dv;
		qglMultiTexCoord2f						= dllMultiTexCoord2f;
		qglMultiTexCoord2fv						= dllMultiTexCoord2fv;
		qglMultiTexCoord2i						= dllMultiTexCoord2i;
		qglMultiTexCoord2iv						= dllMultiTexCoord2iv;
		qglMultiTexCoord2s						= dllMultiTexCoord2s;
		qglMultiTexCoord2sv						= dllMultiTexCoord2sv;
		qglMultiTexCoord3d						= dllMultiTexCoord3d;
		qglMultiTexCoord3dv						= dllMultiTexCoord3dv;
		qglMultiTexCoord3f						= dllMultiTexCoord3f;
		qglMultiTexCoord3fv						= dllMultiTexCoord3fv;
		qglMultiTexCoord3i						= dllMultiTexCoord3i;
		qglMultiTexCoord3iv						= dllMultiTexCoord3iv;
		qglMultiTexCoord3s						= dllMultiTexCoord3s;
		qglMultiTexCoord3sv						= dllMultiTexCoord3sv;
		qglMultiTexCoord4d						= dllMultiTexCoord4d;
		qglMultiTexCoord4dv						= dllMultiTexCoord4dv;
		qglMultiTexCoord4f						= dllMultiTexCoord4f;
		qglMultiTexCoord4fv						= dllMultiTexCoord4fv;
		qglMultiTexCoord4i						= dllMultiTexCoord4i;
		qglMultiTexCoord4iv						= dllMultiTexCoord4iv;
		qglMultiTexCoord4s						= dllMultiTexCoord4s;
		qglMultiTexCoord4sv						= dllMultiTexCoord4sv;
		qglMultiTexCoordP1ui					= dllMultiTexCoordP1ui;
		qglMultiTexCoordP1uiv					= dllMultiTexCoordP1uiv;
		qglMultiTexCoordP2ui					= dllMultiTexCoordP2ui;
		qglMultiTexCoordP2uiv					= dllMultiTexCoordP2uiv;
		qglMultiTexCoordP3ui					= dllMultiTexCoordP3ui;
		qglMultiTexCoordP3uiv					= dllMultiTexCoordP3uiv;
		qglMultiTexCoordP4ui					= dllMultiTexCoordP4ui;
		qglMultiTexCoordP4uiv					= dllMultiTexCoordP4uiv;
		qglNewList								= dllNewList;
		qglNormal3b								= dllNormal3b;
		qglNormal3bv							= dllNormal3bv;
		qglNormal3d								= dllNormal3d;
		qglNormal3dv							= dllNormal3dv;
		qglNormal3f								= dllNormal3f;
		qglNormal3fv							= dllNormal3fv;
		qglNormal3i								= dllNormal3i;
		qglNormal3iv							= dllNormal3iv;
		qglNormal3s								= dllNormal3s;
		qglNormal3sv							= dllNormal3sv;
		qglNormalP3ui							= dllNormalP3ui;
		qglNormalP3uiv							= dllNormalP3uiv;
		qglNormalPointer						= dllNormalPointer;
		qglOrtho								= dllOrtho;
		qglPassThrough							= dllPassThrough;
		qglPixelMapfv							= dllPixelMapfv;
		qglPixelMapuiv							= dllPixelMapuiv;
		qglPixelMapusv							= dllPixelMapusv;
		qglPixelStoref							= dllPixelStoref;
		qglPixelStorei							= dllPixelStorei;
		qglPixelTransferf						= dllPixelTransferf;
		qglPixelTransferi						= dllPixelTransferi;
		qglPixelZoom							= dllPixelZoom;
		qglPointParameterf						= dllPointParameterf;
		qglPointParameterfv						= dllPointParameterfv;
		qglPointParameteri						= dllPointParameteri;
		qglPointParameteriv						= dllPointParameteriv;
		qglPointSize							= dllPointSize;
		qglPolygonMode							= dllPolygonMode;
		qglPolygonOffset						= dllPolygonOffset;
		qglPolygonStipple						= dllPolygonStipple;
		qglPopAttrib							= dllPopAttrib;
		qglPopClientAttrib						= dllPopClientAttrib;
		qglPopMatrix							= dllPopMatrix;
		qglPopName								= dllPopName;
		qglPrimitiveRestartIndex				= dllPrimitiveRestartIndex;
		qglPrioritizeTextures					= dllPrioritizeTextures;
		qglProvokingVertex						= dllProvokingVertex;
		qglPushAttrib							= dllPushAttrib;
		qglPushClientAttrib						= dllPushClientAttrib;
		qglPushMatrix							= dllPushMatrix;
		qglPushName								= dllPushName;
		qglQueryCounter							= dllQueryCounter;
		qglRasterPos2d							= dllRasterPos2d;
		qglRasterPos2dv							= dllRasterPos2dv;
		qglRasterPos2f							= dllRasterPos2f;
		qglRasterPos2fv							= dllRasterPos2fv;
		qglRasterPos2i							= dllRasterPos2i;
		qglRasterPos2iv							= dllRasterPos2iv;
		qglRasterPos2s							= dllRasterPos2s;
		qglRasterPos2sv							= dllRasterPos2sv;
		qglRasterPos3d							= dllRasterPos3d;
		qglRasterPos3dv							= dllRasterPos3dv;
		qglRasterPos3f							= dllRasterPos3f;
		qglRasterPos3fv							= dllRasterPos3fv;
		qglRasterPos3i							= dllRasterPos3i;
		qglRasterPos3iv							= dllRasterPos3iv;
		qglRasterPos3s							= dllRasterPos3s;
		qglRasterPos3sv							= dllRasterPos3sv;
		qglRasterPos4d							= dllRasterPos4d;
		qglRasterPos4dv							= dllRasterPos4dv;
		qglRasterPos4f							= dllRasterPos4f;
		qglRasterPos4fv							= dllRasterPos4fv;
		qglRasterPos4i							= dllRasterPos4i;
		qglRasterPos4iv							= dllRasterPos4iv;
		qglRasterPos4s							= dllRasterPos4s;
		qglRasterPos4sv							= dllRasterPos4sv;
		qglReadBuffer							= dllReadBuffer;
		qglReadPixels							= dllReadPixels;
		qglRectd								= dllRectd;
		qglRectdv								= dllRectdv;
		qglRectf								= dllRectf;
		qglRectfv								= dllRectfv;
		qglRecti								= dllRecti;
		qglRectiv								= dllRectiv;
		qglRects								= dllRects;
		qglRectsv								= dllRectsv;
		qglRenderMode							= dllRenderMode;
		qglRenderbufferStorage					= dllRenderbufferStorage;
		qglRenderbufferStorageMultisample		= dllRenderbufferStorageMultisample;
		qglRotated								= dllRotated;
		qglRotatef								= dllRotatef;
		qglSampleCoverage						= dllSampleCoverage;
		qglSampleMaski							= dllSampleMaski;
		qglSamplerParameterIiv					= dllSamplerParameterIiv;
		qglSamplerParameterIuiv					= dllSamplerParameterIuiv;
		qglSamplerParameterf					= dllSamplerParameterf;
		qglSamplerParameterfv					= dllSamplerParameterfv;
		qglSamplerParameteri					= dllSamplerParameteri;
		qglSamplerParameteriv					= dllSamplerParameteriv;
		qglScaled								= dllScaled;
		qglScalef								= dllScalef;
		qglScissor								= dllScissor;
		qglSecondaryColor3b						= dllSecondaryColor3b;
		qglSecondaryColor3bv					= dllSecondaryColor3bv;
		qglSecondaryColor3d						= dllSecondaryColor3d;
		qglSecondaryColor3dv					= dllSecondaryColor3dv;
		qglSecondaryColor3f						= dllSecondaryColor3f;
		qglSecondaryColor3fv					= dllSecondaryColor3fv;
		qglSecondaryColor3i						= dllSecondaryColor3i;
		qglSecondaryColor3iv					= dllSecondaryColor3iv;
		qglSecondaryColor3s						= dllSecondaryColor3s;
		qglSecondaryColor3sv					= dllSecondaryColor3sv;
		qglSecondaryColor3ub					= dllSecondaryColor3ub;
		qglSecondaryColor3ubv					= dllSecondaryColor3ubv;
		qglSecondaryColor3ui					= dllSecondaryColor3ui;
		qglSecondaryColor3uiv					= dllSecondaryColor3uiv;
		qglSecondaryColor3us					= dllSecondaryColor3us;
		qglSecondaryColor3usv					= dllSecondaryColor3usv;
		qglSecondaryColorP3ui					= dllSecondaryColorP3ui;
		qglSecondaryColorP3uiv					= dllSecondaryColorP3uiv;
		qglSecondaryColorPointer				= dllSecondaryColorPointer;
		qglSelectBuffer							= dllSelectBuffer;
		qglShadeModel							= dllShadeModel;
		qglShaderSource							= dllShaderSource;
		qglStencilFunc							= dllStencilFunc;
		qglStencilFuncSeparate					= dllStencilFuncSeparate;
		qglStencilMask							= dllStencilMask;
		qglStencilMaskSeparate					= dllStencilMaskSeparate;
		qglStencilOp							= dllStencilOp;
		qglStencilOpSeparate					= dllStencilOpSeparate;
		qglStencilOpSeparateATI					= dllStencilOpSeparateATI;
		qglStencilFuncSeparateATI				= dllStencilFuncSeparateATI;
		qglTexBuffer							= dllTexBuffer;
		qglTexCoord1d							= dllTexCoord1d;
		qglTexCoord1dv							= dllTexCoord1dv;
		qglTexCoord1f							= dllTexCoord1f;
		qglTexCoord1fv							= dllTexCoord1fv;
		qglTexCoord1i							= dllTexCoord1i;
		qglTexCoord1iv							= dllTexCoord1iv;
		qglTexCoord1s							= dllTexCoord1s;
		qglTexCoord1sv							= dllTexCoord1sv;
		qglTexCoord2d							= dllTexCoord2d;
		qglTexCoord2dv							= dllTexCoord2dv;
		qglTexCoord2f							= dllTexCoord2f;
		qglTexCoord2fv							= dllTexCoord2fv;
		qglTexCoord2i							= dllTexCoord2i;
		qglTexCoord2iv							= dllTexCoord2iv;
		qglTexCoord2s							= dllTexCoord2s;
		qglTexCoord2sv							= dllTexCoord2sv;
		qglTexCoord3d							= dllTexCoord3d;
		qglTexCoord3dv							= dllTexCoord3dv;
		qglTexCoord3f							= dllTexCoord3f;
		qglTexCoord3fv							= dllTexCoord3fv;
		qglTexCoord3i							= dllTexCoord3i;
		qglTexCoord3iv							= dllTexCoord3iv;
		qglTexCoord3s							= dllTexCoord3s;
		qglTexCoord3sv							= dllTexCoord3sv;
		qglTexCoord4d							= dllTexCoord4d;
		qglTexCoord4dv							= dllTexCoord4dv;
		qglTexCoord4f							= dllTexCoord4f;
		qglTexCoord4fv							= dllTexCoord4fv;
		qglTexCoord4i							= dllTexCoord4i;
		qglTexCoord4iv							= dllTexCoord4iv;
		qglTexCoord4s							= dllTexCoord4s;
		qglTexCoord4sv							= dllTexCoord4sv;
		qglTexCoordP1ui							= dllTexCoordP1ui;
		qglTexCoordP1uiv						= dllTexCoordP1uiv;
		qglTexCoordP2ui							= dllTexCoordP2ui;
		qglTexCoordP2uiv						= dllTexCoordP2uiv;
		qglTexCoordP3ui							= dllTexCoordP3ui;
		qglTexCoordP3uiv						= dllTexCoordP3uiv;
		qglTexCoordP4ui							= dllTexCoordP4ui;
		qglTexCoordP4uiv						= dllTexCoordP4uiv;
		qglTexCoordPointer						= dllTexCoordPointer;
		qglTexEnvf								= dllTexEnvf;
		qglTexEnvfv								= dllTexEnvfv;
		qglTexEnvi								= dllTexEnvi;
		qglTexEnviv								= dllTexEnviv;
		qglTexGend								= dllTexGend;
		qglTexGendv								= dllTexGendv;
		qglTexGenf								= dllTexGenf;
		qglTexGenfv								= dllTexGenfv;
		qglTexGeni								= dllTexGeni;
		qglTexGeniv								= dllTexGeniv;
		qglTexImage1D							= dllTexImage1D;
		qglTexImage2D							= dllTexImage2D;
		qglTexImage2DMultisample				= dllTexImage2DMultisample;
		qglTexImage3D							= dllTexImage3D;
		qglTexImage3DMultisample				= dllTexImage3DMultisample;
		qglTexParameterIiv						= dllTexParameterIiv;
		qglTexParameterIuiv						= dllTexParameterIuiv;
		qglTexParameterf						= dllTexParameterf;
		qglTexParameterfv						= dllTexParameterfv;
		qglTexParameteri						= dllTexParameteri;
		qglTexParameteriv						= dllTexParameteriv;
		qglTexSubImage1D						= dllTexSubImage1D;
		qglTexSubImage2D						= dllTexSubImage2D;
		qglTexSubImage3D						= dllTexSubImage3D;
		qglTransformFeedbackVaryings			= dllTransformFeedbackVaryings;
		qglTranslated							= dllTranslated;
		qglTranslatef							= dllTranslatef;
		qglUniform1f							= dllUniform1f;
		qglUniform1fv							= dllUniform1fv;
		qglUniform1i							= dllUniform1i;
		qglUniform1iv							= dllUniform1iv;
		qglUniform1ui							= dllUniform1ui;
		qglUniform1uiv							= dllUniform1uiv;
		qglUniform2f							= dllUniform2f;
		qglUniform2fv							= dllUniform2fv;
		qglUniform2i							= dllUniform2i;
		qglUniform2iv							= dllUniform2iv;
		qglUniform2ui							= dllUniform2ui;
		qglUniform2uiv							= dllUniform2uiv;
		qglUniform3f							= dllUniform3f;
		qglUniform3fv							= dllUniform3fv;
		qglUniform3i							= dllUniform3i;
		qglUniform3iv							= dllUniform3iv;
		qglUniform3ui							= dllUniform3ui;
		qglUniform3uiv							= dllUniform3uiv;
		qglUniform4f							= dllUniform4f;
		qglUniform4fv							= dllUniform4fv;
		qglUniform4i							= dllUniform4i;
		qglUniform4iv							= dllUniform4iv;
		qglUniform4ui							= dllUniform4ui;
		qglUniform4uiv							= dllUniform4uiv;
		qglUniformBlockBinding					= dllUniformBlockBinding;
		qglUniformMatrix2fv						= dllUniformMatrix2fv;
		qglUniformMatrix2x3fv					= dllUniformMatrix2x3fv;
		qglUniformMatrix2x4fv					= dllUniformMatrix2x4fv;
		qglUniformMatrix3fv						= dllUniformMatrix3fv;
		qglUniformMatrix3x2fv					= dllUniformMatrix3x2fv;
		qglUniformMatrix3x4fv					= dllUniformMatrix3x4fv;
		qglUniformMatrix4fv						= dllUniformMatrix4fv;
		qglUniformMatrix4x2fv					= dllUniformMatrix4x2fv;
		qglUniformMatrix4x3fv					= dllUniformMatrix4x3fv;
		qglUnmapBuffer							= dllUnmapBuffer;
		qglUseProgram							= dllUseProgram;
		qglValidateProgram						= dllValidateProgram;
		qglVertex2d								= dllVertex2d;
		qglVertex2dv							= dllVertex2dv;
		qglVertex2f								= dllVertex2f;
		qglVertex2fv							= dllVertex2fv;
		qglVertex2i								= dllVertex2i;
		qglVertex2iv							= dllVertex2iv;
		qglVertex2s								= dllVertex2s;
		qglVertex2sv							= dllVertex2sv;
		qglVertex3d								= dllVertex3d;
		qglVertex3dv							= dllVertex3dv;
		qglVertex3f								= dllVertex3f;
		qglVertex3fv							= dllVertex3fv;
		qglVertex3i								= dllVertex3i;
		qglVertex3iv							= dllVertex3iv;
		qglVertex3s								= dllVertex3s;
		qglVertex3sv							= dllVertex3sv;
		qglVertex4d								= dllVertex4d;
		qglVertex4dv							= dllVertex4dv;
		qglVertex4f								= dllVertex4f;
		qglVertex4fv							= dllVertex4fv;
		qglVertex4i								= dllVertex4i;
		qglVertex4iv							= dllVertex4iv;
		qglVertex4s								= dllVertex4s;
		qglVertex4sv							= dllVertex4sv;
		qglVertexAttrib1d						= dllVertexAttrib1d;
		qglVertexAttrib1dv						= dllVertexAttrib1dv;
		qglVertexAttrib1f						= dllVertexAttrib1f;
		qglVertexAttrib1fv						= dllVertexAttrib1fv;
		qglVertexAttrib1s						= dllVertexAttrib1s;
		qglVertexAttrib1sv						= dllVertexAttrib1sv;
		qglVertexAttrib2d						= dllVertexAttrib2d;
		qglVertexAttrib2dv						= dllVertexAttrib2dv;
		qglVertexAttrib2f						= dllVertexAttrib2f;
		qglVertexAttrib2fv						= dllVertexAttrib2fv;
		qglVertexAttrib2s						= dllVertexAttrib2s;
		qglVertexAttrib2sv						= dllVertexAttrib2sv;
		qglVertexAttrib3d						= dllVertexAttrib3d;
		qglVertexAttrib3dv						= dllVertexAttrib3dv;
		qglVertexAttrib3f						= dllVertexAttrib3f;
		qglVertexAttrib3fv						= dllVertexAttrib3fv;
		qglVertexAttrib3s						= dllVertexAttrib3s;
		qglVertexAttrib3sv						= dllVertexAttrib3sv;
		qglVertexAttrib4Nbv						= dllVertexAttrib4Nbv;
		qglVertexAttrib4Niv						= dllVertexAttrib4Niv;
		qglVertexAttrib4Nsv						= dllVertexAttrib4Nsv;
		qglVertexAttrib4Nub						= dllVertexAttrib4Nub;
		qglVertexAttrib4Nubv					= dllVertexAttrib4Nubv;
		qglVertexAttrib4Nuiv					= dllVertexAttrib4Nuiv;
		qglVertexAttrib4Nusv					= dllVertexAttrib4Nusv;
		qglVertexAttrib4bv						= dllVertexAttrib4bv;
		qglVertexAttrib4d						= dllVertexAttrib4d;
		qglVertexAttrib4dv						= dllVertexAttrib4dv;
		qglVertexAttrib4f						= dllVertexAttrib4f;
		qglVertexAttrib4fv						= dllVertexAttrib4fv;
		qglVertexAttrib4iv						= dllVertexAttrib4iv;
		qglVertexAttrib4s						= dllVertexAttrib4s;
		qglVertexAttrib4sv						= dllVertexAttrib4sv;
		qglVertexAttrib4ubv						= dllVertexAttrib4ubv;
		qglVertexAttrib4uiv						= dllVertexAttrib4uiv;
		qglVertexAttrib4usv						= dllVertexAttrib4usv;
		qglVertexAttribI1i						= dllVertexAttribI1i;
		qglVertexAttribI1iv						= dllVertexAttribI1iv;
		qglVertexAttribI1ui						= dllVertexAttribI1ui;
		qglVertexAttribI1uiv					= dllVertexAttribI1uiv;
		qglVertexAttribI2i						= dllVertexAttribI2i;
		qglVertexAttribI2iv						= dllVertexAttribI2iv;
		qglVertexAttribI2ui						= dllVertexAttribI2ui;
		qglVertexAttribI2uiv					= dllVertexAttribI2uiv;
		qglVertexAttribI3i						= dllVertexAttribI3i;
		qglVertexAttribI3iv						= dllVertexAttribI3iv;
		qglVertexAttribI3ui						= dllVertexAttribI3ui;
		qglVertexAttribI3uiv					= dllVertexAttribI3uiv;
		qglVertexAttribI4bv						= dllVertexAttribI4bv;
		qglVertexAttribI4i						= dllVertexAttribI4i;
		qglVertexAttribI4iv						= dllVertexAttribI4iv;
		qglVertexAttribI4sv						= dllVertexAttribI4sv;
		qglVertexAttribI4ubv					= dllVertexAttribI4ubv;
		qglVertexAttribI4ui						= dllVertexAttribI4ui;
		qglVertexAttribI4uiv					= dllVertexAttribI4uiv;
		qglVertexAttribI4usv					= dllVertexAttribI4usv;
		qglVertexAttribIPointer					= dllVertexAttribIPointer;
		qglVertexAttribP1ui						= dllVertexAttribP1ui;
		qglVertexAttribP1uiv					= dllVertexAttribP1uiv;
		qglVertexAttribP2ui						= dllVertexAttribP2ui;
		qglVertexAttribP2uiv					= dllVertexAttribP2uiv;
		qglVertexAttribP3ui						= dllVertexAttribP3ui;
		qglVertexAttribP3uiv					= dllVertexAttribP3uiv;
		qglVertexAttribP4ui						= dllVertexAttribP4ui;
		qglVertexAttribP4uiv					= dllVertexAttribP4uiv;
		qglVertexAttribPointer					= dllVertexAttribPointer;
		qglVertexP2ui							= dllVertexP2ui;
		qglVertexP2uiv							= dllVertexP2uiv;
		qglVertexP3ui							= dllVertexP3ui;
		qglVertexP3uiv							= dllVertexP3uiv;
		qglVertexP4ui							= dllVertexP4ui;
		qglVertexP4uiv							= dllVertexP4uiv;
		qglVertexPointer						= dllVertexPointer;
		qglViewport								= dllViewport;
		qglWaitSync								= dllWaitSync;
		qglWindowPos2d							= dllWindowPos2d;
		qglWindowPos2dv							= dllWindowPos2dv;
		qglWindowPos2f							= dllWindowPos2f;
		qglWindowPos2fv							= dllWindowPos2fv;
		qglWindowPos2i							= dllWindowPos2i;
		qglWindowPos2iv							= dllWindowPos2iv;
		qglWindowPos2s							= dllWindowPos2s;
		qglWindowPos2sv							= dllWindowPos2sv;
		qglWindowPos3d							= dllWindowPos3d;
		qglWindowPos3dv							= dllWindowPos3dv;
		qglWindowPos3f							= dllWindowPos3f;
		qglWindowPos3fv							= dllWindowPos3fv;
		qglWindowPos3i							= dllWindowPos3i;
		qglWindowPos3iv							= dllWindowPos3iv;
		qglWindowPos3s							= dllWindowPos3s;
		qglWindowPos3sv							= dllWindowPos3sv;
	}
}

/*
 ==================
 QGL_LogPrintf
 ==================
*/
void QGL_LogPrintf (const char *fmt, ...){

	va_list	argPtr;

	if (!qglState.logFile)
		return;

	va_start(argPtr, fmt);
	vfprintf(qglState.logFile, fmt, argPtr);
	va_end(argPtr);
}

/*
 ==================
 QGL_GetProcAddress
 ==================
*/
static void *QGL_GetProcAddress (const char *procName){

	void	*procAddress;

	procAddress = GetProcAddress(qglState.hModule, procName);
	if (!procAddress){
		FreeLibrary(qglState.hModule);
		qglState.hModule = NULL;

		Com_Error(ERR_FATAL, "QGL_GetProcAddress: GetProcAddress() failed for '%s'", procName);
	}

	return procAddress;
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 QGL_Init

 Loads the specified DLL then binds our QGL function pointers to the
 appropriate GL stuff
 ==================
*/
bool QGL_Init (const char *driver){

	char	name[MAX_PATH_LENGTH];

	Com_Printf("...initializing QGL\n");

	Str_Copy(name, driver, sizeof(name));
	Str_DefaultFileExtension(name, sizeof(name), LIBRARY_EXTENSION);

	Com_Printf("...calling LoadLibrary( '%s' ): ", name);

	if ((qglState.hModule = LoadLibrary(name)) == NULL){
		Com_Printf("failed\n");
		return false;
	}

	Com_Printf("succeeded\n");

	qwglChoosePixelFormat					= (WGLCHOOSEPIXELFORMAT)QGL_GetProcAddress("wglChoosePixelFormat");
	qwglCopyContext							= (WGLCOPYCONTEXT)QGL_GetProcAddress("wglCopyContext");
	qwglCreateContext						= (WGLCREATECONTEXT)QGL_GetProcAddress("wglCreateContext");
	qwglCreateLayerContext					= (WGLCREATELAYERCONTEXT)QGL_GetProcAddress("wglCreateLayerContext");
	qwglDeleteContext						= (WGLDELETECONTEXT)QGL_GetProcAddress("wglDeleteContext");
	qwglDescribeLayerPlane					= (WGLDESCRIBELAYERPLANE)QGL_GetProcAddress("wglDescribeLayerPlane");
	qwglDescribePixelFormat					= (WGLDESCRIBEPIXELFORMAT)QGL_GetProcAddress("wglDescribePixelFormat");
	qwglGetCurrentContext					= (WGLGETCURRENTCONTEXT)QGL_GetProcAddress("wglGetCurrentContext");
	qwglGetCurrentDC						= (WGLGETCURRENTDC)QGL_GetProcAddress("wglGetCurrentDC");
	qwglGetLayerPaletteEntries				= (WGLGETLAYERPALETTEENTRIES)QGL_GetProcAddress("wglGetLayerPaletteEntries");
	qwglGetPixelFormat						= (WGLGETPIXELFORMAT)QGL_GetProcAddress("wglGetPixelFormat");
	qwglGetProcAddress						= (WGLGETPROCADDRESS)QGL_GetProcAddress("wglGetProcAddress");
	qwglMakeCurrent							= (WGLMAKECURRENT)QGL_GetProcAddress("wglMakeCurrent");
	qwglRealizeLayerPalette					= (WGLREALIZELAYERPALETTE)QGL_GetProcAddress("wglRealizeLayerPalette");
	qwglSetLayerPaletteEntries				= (WGLSETLAYERPALETTEENTRIES)QGL_GetProcAddress("wglSetLayerPaletteEntries");
	qwglSetPixelFormat						= (WGLSETPIXELFORMAT)QGL_GetProcAddress("wglSetPixelFormat");
	qwglShareLists							= (WGLSHARELISTS)QGL_GetProcAddress("wglShareLists");
	qwglSwapBuffers							= (WGLSWAPBUFFERS)QGL_GetProcAddress("wglSwapBuffers");
	qwglSwapLayerBuffers					= (WGLSWAPLAYERBUFFERS)QGL_GetProcAddress("wglSwapLayerBuffers");
	qwglUseFontBitmaps						= (WGLUSEFONTBITMAPS)QGL_GetProcAddress("wglUseFontBitmapsA");
	qwglUseFontOutlines						= (WGLUSEFONTOUTLINES)QGL_GetProcAddress("wglUseFontOutlinesA");

	qwglSwapIntervalEXT						= NULL;

	qglAccum								= (GLACCUM)QGL_GetProcAddress("glAccum");
	qglActiveStencilFaceEXT					= NULL;
	qglActiveTexture						= NULL;
	qglAlphaFunc							= (GLALPHAFUNC)QGL_GetProcAddress("glAlphaFunc");
	qglAreTexturesResident					= (GLARETEXTURESRESIDENT)QGL_GetProcAddress("glAreTexturesResident");
	qglArrayElement							= (GLARRAYELEMENT)QGL_GetProcAddress("glArrayElement");
	qglAttachShader							= NULL;
	qglBegin								= (GLBEGIN)QGL_GetProcAddress("glBegin");
	qglBeginConditionalRender				= NULL;
	qglBeginQuery							= NULL;
	qglBeginTransformFeedback				= NULL;
	qglBindAttribLocation					= NULL;
	qglBindBuffer							= NULL;
	qglBindBufferBase						= NULL;
	qglBindBufferRange						= NULL;
	qglBindFragDataLocation					= NULL;
	qglBindFragDataLocationIndexed			= NULL;
	qglBindFramebuffer						= NULL;
	qglBindRenderbuffer						= NULL;
	qglBindSampler							= NULL;
	qglBindTexture							= (GLBINDTEXTURE)QGL_GetProcAddress("glBindTexture");
	qglBindVertexArray						= NULL;
	qglBitmap								= (GLBITMAP)QGL_GetProcAddress("glBitmap");
	qglBlendColor							= NULL;
	qglBlendEquation						= NULL;
	qglBlendEquationSeparate				= NULL;
	qglBlendFunc							= (GLBLENDFUNC)QGL_GetProcAddress("glBlendFunc");
	qglBlendFuncSeparate					= NULL;
	qglBlitFramebuffer						= NULL;
	qglBufferData							= NULL;
	qglBufferSubData						= NULL;
	qglCallList								= (GLCALLLIST)QGL_GetProcAddress("glCallList");
	qglCallLists							= (GLCALLLISTS)QGL_GetProcAddress("glCallLists");
	qglCheckFramebufferStatus				= NULL;
	qglClampColor							= NULL;
	qglClear								= (GLCLEAR)QGL_GetProcAddress("glClear");
	qglClearAccum							= (GLCLEARACCUM)QGL_GetProcAddress("glClearAccum");
	qglClearBufferfi						= NULL;
	qglClearBufferfv						= NULL;
	qglClearBufferiv						= NULL;
	qglClearBufferuiv						= NULL;
	qglClearColor							= (GLCLEARCOLOR)QGL_GetProcAddress("glClearColor");
	qglClearDepth							= (GLCLEARDEPTH)QGL_GetProcAddress("glClearDepth");
	qglClearIndex							= (GLCLEARINDEX)QGL_GetProcAddress("glClearIndex");
	qglClearStencil							= (GLCLEARSTENCIL)QGL_GetProcAddress("glClearStencil");
	qglClientActiveTexture					= NULL;
	qglClientWaitSync						= NULL;
	qglClipPlane							= (GLCLIPPLANE)QGL_GetProcAddress("glClipPlane");
	qglColor3b								= (GLCOLOR3B)QGL_GetProcAddress("glColor3b");
	qglColor3bv								= (GLCOLOR3BV)QGL_GetProcAddress("glColor3bv");
	qglColor3d								= (GLCOLOR3D)QGL_GetProcAddress("glColor3d");
	qglColor3dv								= (GLCOLOR3DV)QGL_GetProcAddress("glColor3dv");
	qglColor3f								= (GLCOLOR3F)QGL_GetProcAddress("glColor3f");
	qglColor3fv								= (GLCOLOR3FV)QGL_GetProcAddress("glColor3fv");
	qglColor3i								= (GLCOLOR3I)QGL_GetProcAddress("glColor3i");
	qglColor3iv								= (GLCOLOR3IV)QGL_GetProcAddress("glColor3iv");
	qglColor3s								= (GLCOLOR3S)QGL_GetProcAddress("glColor3s");
	qglColor3sv								= (GLCOLOR3SV)QGL_GetProcAddress("glColor3sv");
	qglColor3ub								= (GLCOLOR3UB)QGL_GetProcAddress("glColor3ub");
	qglColor3ubv							= (GLCOLOR3UBV)QGL_GetProcAddress("glColor3ubv");
	qglColor3ui								= (GLCOLOR3UI)QGL_GetProcAddress("glColor3ui");
	qglColor3uiv							= (GLCOLOR3UIV)QGL_GetProcAddress("glColor3uiv");
	qglColor3us								= (GLCOLOR3US)QGL_GetProcAddress("glColor3us");
	qglColor3usv							= (GLCOLOR3USV)QGL_GetProcAddress("glColor3usv");
	qglColor4b								= (GLCOLOR4B)QGL_GetProcAddress("glColor4b");
	qglColor4bv								= (GLCOLOR4BV)QGL_GetProcAddress("glColor4bv");
	qglColor4d								= (GLCOLOR4D)QGL_GetProcAddress("glColor4d");
	qglColor4dv								= (GLCOLOR4DV)QGL_GetProcAddress("glColor4dv");
	qglColor4f								= (GLCOLOR4F)QGL_GetProcAddress("glColor4f");
	qglColor4fv								= (GLCOLOR4FV)QGL_GetProcAddress("glColor4fv");
	qglColor4i								= (GLCOLOR4I)QGL_GetProcAddress("glColor4i");
	qglColor4iv								= (GLCOLOR4IV)QGL_GetProcAddress("glColor4iv");
	qglColor4s								= (GLCOLOR4S)QGL_GetProcAddress("glColor4s");
	qglColor4sv								= (GLCOLOR4SV)QGL_GetProcAddress("glColor4sv");
	qglColor4ub								= (GLCOLOR4UB)QGL_GetProcAddress("glColor4ub");
	qglColor4ubv							= (GLCOLOR4UBV)QGL_GetProcAddress("glColor4ubv");
	qglColor4ui								= (GLCOLOR4UI)QGL_GetProcAddress("glColor4ui");
	qglColor4uiv							= (GLCOLOR4UIV)QGL_GetProcAddress("glColor4uiv");
	qglColor4us								= (GLCOLOR4US)QGL_GetProcAddress("glColor4us");
	qglColor4usv							= (GLCOLOR4USV)QGL_GetProcAddress("glColor4usv");
	qglColorMask							= (GLCOLORMASK)QGL_GetProcAddress("glColorMask");
	qglColorMaski							= NULL;
	qglColorMaterial						= (GLCOLORMATERIAL)QGL_GetProcAddress("glColorMaterial");
	qglColorP3ui							= NULL;
	qglColorP3uiv							= NULL;
	qglColorP4ui							= NULL;
	qglColorP4uiv							= NULL;
	qglColorPointer							= (GLCOLORPOINTER)QGL_GetProcAddress("glColorPointer");
	qglCompileShader						= NULL;
	qglCompressedTexImage1D					= NULL;
	qglCompressedTexImage2D					= NULL;
	qglCompressedTexImage3D					= NULL;
	qglCompressedTexSubImage1D				= NULL;
	qglCompressedTexSubImage2D				= NULL;
	qglCompressedTexSubImage3D				= NULL;
	qglCopyBufferSubData					= NULL;
	qglCopyPixels							= (GLCOPYPIXELS)QGL_GetProcAddress("glCopyPixels");
	qglCopyTexImage1D						= (GLCOPYTEXIMAGE1D)QGL_GetProcAddress("glCopyTexImage1D");
	qglCopyTexImage2D						= (GLCOPYTEXIMAGE2D)QGL_GetProcAddress("glCopyTexImage2D");
	qglCopyTexSubImage1D					= (GLCOPYTEXSUBIMAGE1D)QGL_GetProcAddress("glCopyTexSubImage1D");
	qglCopyTexSubImage2D					= (GLCOPYTEXSUBIMAGE2D)QGL_GetProcAddress("glCopyTexSubImage2D");
	qglCopyTexSubImage3D					= NULL;
	qglCreateProgram						= NULL;
	qglCreateShader							= NULL;
	qglCullFace								= (GLCULLFACE)QGL_GetProcAddress("glCullFace");
	qglDeleteBuffers						= NULL;
	qglDeleteFramebuffers					= NULL;
	qglDeleteLists							= (GLDELETELISTS)QGL_GetProcAddress("glDeleteLists");
	qglDeleteProgram						= NULL;
	qglDeleteQueries						= NULL;
	qglDeleteRenderbuffers					= NULL;
	qglDeleteSamplers						= NULL;
	qglDeleteShader							= NULL;
	qglDeleteSync							= NULL;
	qglDeleteTextures						= (GLDELETETEXTURES)QGL_GetProcAddress("glDeleteTextures");
	qglDeleteVertexArrays					= NULL;
	qglDepthBoundsEXT						= NULL;
	qglDepthFunc							= (GLDEPTHFUNC)QGL_GetProcAddress("glDepthFunc");
	qglDepthMask							= (GLDEPTHMASK)QGL_GetProcAddress("glDepthMask");
	qglDepthRange							= (GLDEPTHRANGE)QGL_GetProcAddress("glDepthRange");
	qglDetachShader							= NULL;
	qglDisable								= (GLDISABLE)QGL_GetProcAddress("glDisable");
	qglDisableClientState					= (GLDISABLECLIENTSTATE)QGL_GetProcAddress("glDisableClientState");
	qglDisableVertexAttribArray				= NULL;
	qglDisablei								= NULL;
	qglDrawArrays							= (GLDRAWARRAYS)QGL_GetProcAddress("glDrawArrays");
	qglDrawArraysInstanced					= NULL;
	qglDrawBuffer							= (GLDRAWBUFFER)QGL_GetProcAddress("glDrawBuffer");
	qglDrawBuffers							= NULL;
	qglDrawElements							= (GLDRAWELEMENTS)QGL_GetProcAddress("glDrawElements");
	qglDrawElementsBaseVertex				= NULL;
	qglDrawElementsInstanced				= NULL;
	qglDrawElementsInstancedBaseVertex		= NULL;
	qglDrawPixels							= (GLDRAWPIXELS)QGL_GetProcAddress("glDrawPixels");
	qglDrawRangeElements					= NULL;
	qglDrawRangeElementsBaseVertex			= NULL;
	qglEdgeFlag								= (GLEDGEFLAG)QGL_GetProcAddress("glEdgeFlag");
	qglEdgeFlagPointer						= (GLEDGEFLAGPOINTER)QGL_GetProcAddress("glEdgeFlagPointer");
	qglEdgeFlagv							= (GLEDGEFLAGV)QGL_GetProcAddress("glEdgeFlagv");
	qglEnable								= (GLENABLE)QGL_GetProcAddress("glEnable");
	qglEnableClientState					= (GLENABLECLIENTSTATE)QGL_GetProcAddress("glEnableClientState");
	qglEnableVertexAttribArray				= NULL;
	qglEnablei								= NULL;
	qglEnd									= (GLEND)QGL_GetProcAddress("glEnd");
	qglEndConditionalRender					= NULL;
	qglEndList								= (GLENDLIST)QGL_GetProcAddress("glEndList");
	qglEndQuery								= NULL;
	qglEndTransformFeedback					= NULL;
	qglEvalCoord1d							= (GLEVALCOORD1D)QGL_GetProcAddress("glEvalCoord1d");
	qglEvalCoord1dv							= (GLEVALCOORD1DV)QGL_GetProcAddress("glEvalCoord1dv");
	qglEvalCoord1f							= (GLEVALCOORD1F)QGL_GetProcAddress("glEvalCoord1f");
	qglEvalCoord1fv							= (GLEVALCOORD1FV)QGL_GetProcAddress("glEvalCoord1fv");
	qglEvalCoord2d							= (GLEVALCOORD2D)QGL_GetProcAddress("glEvalCoord2d");
	qglEvalCoord2dv							= (GLEVALCOORD2DV)QGL_GetProcAddress("glEvalCoord2dv");
	qglEvalCoord2f							= (GLEVALCOORD2F)QGL_GetProcAddress("glEvalCoord2f");
	qglEvalCoord2fv							= (GLEVALCOORD2FV)QGL_GetProcAddress("glEvalCoord2fv");
	qglEvalMesh1							= (GLEVALMESH1)QGL_GetProcAddress("glEvalMesh1");
	qglEvalMesh2							= (GLEVALMESH2)QGL_GetProcAddress("glEvalMesh2");
	qglEvalPoint1							= (GLEVALPOINT1)QGL_GetProcAddress("glEvalPoint1");
	qglEvalPoint2							= (GLEVALPOINT2)QGL_GetProcAddress("glEvalPoint2");
	qglFeedbackBuffer						= (GLFEEDBACKBUFFER)QGL_GetProcAddress("glFeedbackBuffer");
	qglFenceSync							= NULL;
	qglFinish								= (GLFINISH)QGL_GetProcAddress("glFinish");
	qglFlush								= (GLFLUSH)QGL_GetProcAddress("glFlush");
	qglFlushMappedBufferRange				= NULL;
	qglFogCoordPointer						= NULL;
	qglFogCoordd							= NULL;
	qglFogCoorddv							= NULL;
	qglFogCoordf							= NULL;
	qglFogCoordfv							= NULL;
	qglFogf									= (GLFOGF)QGL_GetProcAddress("glFogf");
	qglFogfv								= (GLFOGFV)QGL_GetProcAddress("glFogfv");
	qglFogi									= (GLFOGI)QGL_GetProcAddress("glFogi");
	qglFogiv								= (GLFOGIV)QGL_GetProcAddress("glFogiv");
	qglFramebufferRenderbuffer				= NULL;
	qglFramebufferTexture					= NULL;
	qglFramebufferTexture1D					= NULL;
	qglFramebufferTexture2D					= NULL;
	qglFramebufferTexture3D					= NULL;
	qglFramebufferTextureLayer				= NULL;
	qglFrontFace							= (GLFRONTFACE)QGL_GetProcAddress("glFrontFace");
	qglFrustum								= (GLFRUSTUM)QGL_GetProcAddress("glFrustum");
	qglGenBuffers							= NULL;
	qglGenFramebuffers						= NULL;
	qglGenLists								= (GLGENLISTS)QGL_GetProcAddress("glGenLists");
	qglGenQueries							= NULL;
	qglGenRenderbuffers						= NULL;
	qglGenSamplers							= NULL;
	qglGenTextures							= (GLGENTEXTURES)QGL_GetProcAddress("glGenTextures");
	qglGenVertexArrays						= NULL;
	qglGenerateMipmap						= NULL;
	qglGetActiveAttrib						= NULL;
	qglGetActiveUniform						= NULL;
	qglGetActiveUniformBlockName			= NULL;
	qglGetActiveUniformBlockiv				= NULL;
	qglGetActiveUniformName					= NULL;
	qglGetActiveUniformsiv					= NULL;
	qglGetAttachedShaders					= NULL;
	qglGetAttribLocation					= NULL;
	qglGetBooleani_v						= NULL;
	qglGetBooleanv							= (GLGETBOOLEANV)QGL_GetProcAddress("glGetBooleanv");
	qglGetBufferParameteri64v				= NULL;
	qglGetBufferParameteriv					= NULL;
	qglGetBufferPointerv					= NULL;
	qglGetBufferSubData						= NULL;
	qglGetClipPlane							= (GLGETCLIPPLANE)QGL_GetProcAddress("glGetClipPlane");
	qglGetCompressedTexImage				= NULL;
	qglGetDoublev							= (GLGETDOUBLEV)QGL_GetProcAddress("glGetDoublev");
	qglGetError								= (GLGETERROR)QGL_GetProcAddress("glGetError");
	qglGetFloatv							= (GLGETFLOATV)QGL_GetProcAddress("glGetFloatv");
	qglGetFragDataIndex						= NULL;
	qglGetFragDataLocation					= NULL;
	qglGetFramebufferAttachmentParameteriv	= NULL;
	qglGetInteger64i_v						= NULL;
	qglGetInteger64v						= NULL;
	qglGetIntegeri_v						= NULL;
	qglGetIntegerv							= (GLGETINTEGERV)QGL_GetProcAddress("glGetIntegerv");
	qglGetLightfv							= (GLGETLIGHTFV)QGL_GetProcAddress("glGetLightfv");
	qglGetLightiv							= (GLGETLIGHTIV)QGL_GetProcAddress("glGetLightiv");
	qglGetMapdv								= (GLGETMAPDV)QGL_GetProcAddress("glGetMapdv");
	qglGetMapfv								= (GLGETMAPFV)QGL_GetProcAddress("glGetMapfv");
	qglGetMapiv								= (GLGETMAPIV)QGL_GetProcAddress("glGetMapiv");
	qglGetMaterialfv						= (GLGETMATERIALFV)QGL_GetProcAddress("glGetMaterialfv");
	qglGetMaterialiv						= (GLGETMATERIALIV)QGL_GetProcAddress("glGetMaterialiv");
	qglGetMultisamplefv						= NULL;
	qglGetPixelMapfv						= (GLGETPIXELMAPFV)QGL_GetProcAddress("glGetPixelMapfv");
	qglGetPixelMapuiv						= (GLGETPIXELMAPUIV)QGL_GetProcAddress("glGetPixelMapuiv");
	qglGetPixelMapusv						= (GLGETPIXELMAPUSV)QGL_GetProcAddress("glGetPixelMapusv");
	qglGetPointerv							= (GLGETPOINTERV)QGL_GetProcAddress("glGetPointerv");
	qglGetPolygonStipple					= (GLGETPOLYGONSTIPPLE)QGL_GetProcAddress("glGetPolygonStipple");
	qglGetProgramInfoLog					= NULL;
	qglGetProgramiv							= NULL;
	qglGetQueryObjecti64v					= NULL;
	qglGetQueryObjectiv						= NULL;
	qglGetQueryObjectui64v					= NULL;
	qglGetQueryObjectuiv					= NULL;
	qglGetQueryiv							= NULL;
	qglGetRenderbufferParameteriv			= NULL;
	qglGetSamplerParameterIiv				= NULL;
	qglGetSamplerParameterIuiv				= NULL;
	qglGetSamplerParameterfv				= NULL;
	qglGetSamplerParameteriv				= NULL;
	qglGetShaderInfoLog						= NULL;
	qglGetShaderSource						= NULL;
	qglGetShaderiv							= NULL;
	qglGetString							= (GLGETSTRING)QGL_GetProcAddress("glGetString");
	qglGetStringi							= NULL;
	qglGetSynciv							= NULL;
	qglGetTexEnvfv							= (GLGETTEXENVFV)QGL_GetProcAddress("glGetTexEnvfv");
	qglGetTexEnviv							= (GLGETTEXENVIV)QGL_GetProcAddress("glGetTexEnviv");
	qglGetTexGendv							= (GLGETTEXGENDV)QGL_GetProcAddress("glGetTexGendv");
	qglGetTexGenfv							= (GLGETTEXGENFV)QGL_GetProcAddress("glGetTexGenfv");
	qglGetTexGeniv							= (GLGETTEXGENIV)QGL_GetProcAddress("glGetTexGeniv");
	qglGetTexImage							= (GLGETTEXIMAGE)QGL_GetProcAddress("glGetTexImage");
	qglGetTexLevelParameterfv				= (GLGETTEXLEVELPARAMETERFV)QGL_GetProcAddress("glGetTexLevelParameterfv");
	qglGetTexLevelParameteriv				= (GLGETTEXLEVELPARAMETERIV)QGL_GetProcAddress("glGetTexLevelParameteriv");
	qglGetTexParameterIiv					= NULL;
	qglGetTexParameterIuiv					= NULL;
	qglGetTexParameterfv					= (GLGETTEXPARAMETERFV)QGL_GetProcAddress("glGetTexParameterfv");
	qglGetTexParameteriv					= (GLGETTEXPARAMETERIV)QGL_GetProcAddress("glGetTexParameteriv");
	qglGetTransformFeedbackVarying			= NULL;
	qglGetUniformBlockIndex					= NULL;
	qglGetUniformIndices					= NULL;
	qglGetUniformLocation					= NULL;
	qglGetUniformfv							= NULL;
	qglGetUniformiv							= NULL;
	qglGetUniformuiv						= NULL;
	qglGetVertexAttribIiv					= NULL;
	qglGetVertexAttribIuiv					= NULL;
	qglGetVertexAttribPointerv				= NULL;
	qglGetVertexAttribdv					= NULL;
	qglGetVertexAttribfv					= NULL;
	qglGetVertexAttribiv					= NULL;
	qglHint									= (GLHINT)QGL_GetProcAddress("glHint");
	qglIndexMask							= (GLINDEXMASK)QGL_GetProcAddress("glIndexMask");
	qglIndexPointer							= (GLINDEXPOINTER)QGL_GetProcAddress("glIndexPointer");
	qglIndexd								= (GLINDEXD)QGL_GetProcAddress("glIndexd");
	qglIndexdv								= (GLINDEXDV)QGL_GetProcAddress("glIndexdv");
	qglIndexf								= (GLINDEXF)QGL_GetProcAddress("glIndexf");
	qglIndexfv								= (GLINDEXFV)QGL_GetProcAddress("glIndexfv");
	qglIndexi								= (GLINDEXI)QGL_GetProcAddress("glIndexi");
	qglIndexiv								= (GLINDEXIV)QGL_GetProcAddress("glIndexiv");
	qglIndexs								= (GLINDEXS)QGL_GetProcAddress("glIndexs");
	qglIndexsv								= (GLINDEXSV)QGL_GetProcAddress("glIndexsv");
	qglIndexub								= (GLINDEXUB)QGL_GetProcAddress("glIndexub");
	qglIndexubv								= (GLINDEXUBV)QGL_GetProcAddress("glIndexubv");
	qglInitNames							= (GLINITNAMES)QGL_GetProcAddress("glInitNames");
	qglInterleavedArrays					= (GLINTERLEAVEDARRAYS)QGL_GetProcAddress("glInterleavedArrays");
	qglIsBuffer								= NULL;
	qglIsEnabled							= (GLISENABLED)QGL_GetProcAddress("glIsEnabled");
	qglIsEnabledi							= NULL;
	qglIsFramebuffer						= NULL;
	qglIsList								= (GLISLIST)QGL_GetProcAddress("glIsList");
	qglIsProgram							= NULL;
	qglIsQuery								= NULL;
	qglIsRenderbuffer						= NULL;
	qglIsSampler							= NULL;
	qglIsShader								= NULL;
	qglIsSync								= NULL;
	qglIsTexture							= (GLISTEXTURE)QGL_GetProcAddress("glIsTexture");
	qglIsVertexArray						= NULL;
	qglLightModelf							= (GLLIGHTMODELF)QGL_GetProcAddress("glLightModelf");
	qglLightModelfv							= (GLLIGHTMODELFV)QGL_GetProcAddress("glLightModelfv");
	qglLightModeli							= (GLLIGHTMODELI)QGL_GetProcAddress("glLightModeli");
	qglLightModeliv							= (GLLIGHTMODELIV)QGL_GetProcAddress("glLightModeliv");
	qglLightf								= (GLLIGHTF)QGL_GetProcAddress("glLightf");
	qglLightfv								= (GLLIGHTFV)QGL_GetProcAddress("glLightfv");
	qglLighti								= (GLLIGHTI)QGL_GetProcAddress("glLighti");
	qglLightiv								= (GLLIGHTIV)QGL_GetProcAddress("glLightiv");
	qglLineStipple							= (GLLINESTIPPLE)QGL_GetProcAddress("glLineStipple");
	qglLineWidth							= (GLLINEWIDTH)QGL_GetProcAddress("glLineWidth");
	qglLinkProgram							= NULL;
	qglListBase								= (GLLISTBASE)QGL_GetProcAddress("glListBase");
	qglLoadIdentity							= (GLLOADIDENTITY)QGL_GetProcAddress("glLoadIdentity");
	qglLoadMatrixd							= (GLLOADMATRIXD)QGL_GetProcAddress("glLoadMatrixd");
	qglLoadMatrixf							= (GLLOADMATRIXF)QGL_GetProcAddress("glLoadMatrixf");
	qglLoadName								= (GLLOADNAME)QGL_GetProcAddress("glLoadName");
	qglLoadTransposeMatrixd					= NULL;
	qglLoadTransposeMatrixf					= NULL;
	qglLogicOp								= (GLLOGICOP)QGL_GetProcAddress("glLogicOp");
	qglMap1d								= (GLMAP1D)QGL_GetProcAddress("glMap1d");
	qglMap1f								= (GLMAP1F)QGL_GetProcAddress("glMap1f");
	qglMap2d								= (GLMAP2D)QGL_GetProcAddress("glMap2d");
	qglMap2f								= (GLMAP2F)QGL_GetProcAddress("glMap2f");
	qglMapBuffer							= NULL;
	qglMapBufferRange						= NULL;
	qglMapGrid1d							= (GLMAPGRID1D)QGL_GetProcAddress("glMapGrid1d");
	qglMapGrid1f							= (GLMAPGRID1F)QGL_GetProcAddress("glMapGrid1f");
	qglMapGrid2d							= (GLMAPGRID2D)QGL_GetProcAddress("glMapGrid2d");
	qglMapGrid2f							= (GLMAPGRID2F)QGL_GetProcAddress("glMapGrid2f");
	qglMaterialf							= (GLMATERIALF)QGL_GetProcAddress("glMaterialf");
	qglMaterialfv							= (GLMATERIALFV)QGL_GetProcAddress("glMaterialfv");
	qglMateriali							= (GLMATERIALI)QGL_GetProcAddress("glMateriali");
	qglMaterialiv							= (GLMATERIALIV)QGL_GetProcAddress("glMaterialiv");
	qglMatrixMode							= (GLMATRIXMODE)QGL_GetProcAddress("glMatrixMode");
	qglMultMatrixd							= (GLMULTMATRIXD)QGL_GetProcAddress("glMultMatrixd");
	qglMultMatrixf							= (GLMULTMATRIXF)QGL_GetProcAddress("glMultMatrixf");
	qglMultTransposeMatrixd					= NULL;
	qglMultTransposeMatrixf					= NULL;
	qglMultiDrawArrays						= NULL;
	qglMultiDrawElements					= NULL;
	qglMultiDrawElementsBaseVertex			= NULL;
	qglMultiTexCoord1d						= NULL;
	qglMultiTexCoord1dv						= NULL;
	qglMultiTexCoord1f						= NULL;
	qglMultiTexCoord1fv						= NULL;
	qglMultiTexCoord1i						= NULL;
	qglMultiTexCoord1iv						= NULL;
	qglMultiTexCoord1s						= NULL;
	qglMultiTexCoord1sv						= NULL;
	qglMultiTexCoord2d						= NULL;
	qglMultiTexCoord2dv						= NULL;
	qglMultiTexCoord2f						= NULL;
	qglMultiTexCoord2fv						= NULL;
	qglMultiTexCoord2i						= NULL;
	qglMultiTexCoord2iv						= NULL;
	qglMultiTexCoord2s						= NULL;
	qglMultiTexCoord2sv						= NULL;
	qglMultiTexCoord3d						= NULL;
	qglMultiTexCoord3dv						= NULL;
	qglMultiTexCoord3f						= NULL;
	qglMultiTexCoord3fv						= NULL;
	qglMultiTexCoord3i						= NULL;
	qglMultiTexCoord3iv						= NULL;
	qglMultiTexCoord3s						= NULL;
	qglMultiTexCoord3sv						= NULL;
	qglMultiTexCoord4d						= NULL;
	qglMultiTexCoord4dv						= NULL;
	qglMultiTexCoord4f						= NULL;
	qglMultiTexCoord4fv						= NULL;
	qglMultiTexCoord4i						= NULL;
	qglMultiTexCoord4iv						= NULL;
	qglMultiTexCoord4s						= NULL;
	qglMultiTexCoord4sv						= NULL;
	qglMultiTexCoordP1ui					= NULL;
	qglMultiTexCoordP1uiv					= NULL;
	qglMultiTexCoordP2ui					= NULL;
	qglMultiTexCoordP2uiv					= NULL;
	qglMultiTexCoordP3ui					= NULL;
	qglMultiTexCoordP3uiv					= NULL;
	qglMultiTexCoordP4ui					= NULL;
	qglMultiTexCoordP4uiv					= NULL;
	qglNewList								= (GLNEWLIST)QGL_GetProcAddress("glNewList");
	qglNormal3b								= (GLNORMAL3B)QGL_GetProcAddress("glNormal3b");
	qglNormal3bv							= (GLNORMAL3BV)QGL_GetProcAddress("glNormal3bv");
	qglNormal3d								= (GLNORMAL3D)QGL_GetProcAddress("glNormal3d");
	qglNormal3dv							= (GLNORMAL3DV)QGL_GetProcAddress("glNormal3dv");
	qglNormal3f								= (GLNORMAL3F)QGL_GetProcAddress("glNormal3f");
	qglNormal3fv							= (GLNORMAL3FV)QGL_GetProcAddress("glNormal3fv");
	qglNormal3i								= (GLNORMAL3I)QGL_GetProcAddress("glNormal3i");
	qglNormal3iv							= (GLNORMAL3IV)QGL_GetProcAddress("glNormal3iv");
	qglNormal3s								= (GLNORMAL3S)QGL_GetProcAddress("glNormal3s");
	qglNormal3sv							= (GLNORMAL3SV)QGL_GetProcAddress("glNormal3sv");
	qglNormalP3ui							= NULL;
	qglNormalP3uiv							= NULL;
	qglNormalPointer						= (GLNORMALPOINTER)QGL_GetProcAddress("glNormalPointer");
	qglOrtho								= (GLORTHO)QGL_GetProcAddress("glOrtho");
	qglPassThrough							= (GLPASSTHROUGH)QGL_GetProcAddress("glPassThrough");
	qglPixelMapfv							= (GLPIXELMAPFV)QGL_GetProcAddress("glPixelMapfv");
	qglPixelMapuiv							= (GLPIXELMAPUIV)QGL_GetProcAddress("glPixelMapuiv");
	qglPixelMapusv							= (GLPIXELMAPUSV)QGL_GetProcAddress("glPixelMapusv");
	qglPixelStoref							= (GLPIXELSTOREF)QGL_GetProcAddress("glPixelStoref");
	qglPixelStorei							= (GLPIXELSTOREI)QGL_GetProcAddress("glPixelStorei");
	qglPixelTransferf						= (GLPIXELTRANSFERF)QGL_GetProcAddress("glPixelTransferf");
	qglPixelTransferi						= (GLPIXELTRANSFERI)QGL_GetProcAddress("glPixelTransferi");
	qglPixelZoom							= (GLPIXELZOOM)QGL_GetProcAddress("glPixelZoom");
	qglPointParameterf						= NULL;
	qglPointParameterfv						= NULL;
	qglPointParameteri						= NULL;
	qglPointParameteriv						= NULL;
	qglPointSize							= (GLPOINTSIZE)QGL_GetProcAddress("glPointSize");
	qglPolygonMode							= (GLPOLYGONMODE)QGL_GetProcAddress("glPolygonMode");
	qglPolygonOffset						= (GLPOLYGONOFFSET)QGL_GetProcAddress("glPolygonOffset");
	qglPolygonStipple						= (GLPOLYGONSTIPPLE)QGL_GetProcAddress("glPolygonStipple");
	qglPopAttrib							= (GLPOPATTRIB)QGL_GetProcAddress("glPopAttrib");
	qglPopClientAttrib						= (GLPOPCLIENTATTRIB)QGL_GetProcAddress("glPopClientAttrib");
	qglPopMatrix							= (GLPOPMATRIX)QGL_GetProcAddress("glPopMatrix");
	qglPopName								= (GLPOPNAME)QGL_GetProcAddress("glPopName");
	qglPrimitiveRestartIndex				= NULL;
	qglPrioritizeTextures					= (GLPRIORITIZETEXTURES)QGL_GetProcAddress("glPrioritizeTextures");
	qglProvokingVertex						= NULL;
	qglPushAttrib							= (GLPUSHATTRIB)QGL_GetProcAddress("glPushAttrib");
	qglPushClientAttrib						= (GLPUSHCLIENTATTRIB)QGL_GetProcAddress("glPushClientAttrib");
	qglPushMatrix							= (GLPUSHMATRIX)QGL_GetProcAddress("glPushMatrix");
	qglPushName								= (GLPUSHNAME)QGL_GetProcAddress("glPushName");
	qglQueryCounter							= NULL;
	qglRasterPos2d							= (GLRASTERPOS2D)QGL_GetProcAddress("glRasterPos2d");
	qglRasterPos2dv							= (GLRASTERPOS2DV)QGL_GetProcAddress("glRasterPos2dv");
	qglRasterPos2f							= (GLRASTERPOS2F)QGL_GetProcAddress("glRasterPos2f");
	qglRasterPos2fv							= (GLRASTERPOS2FV)QGL_GetProcAddress("glRasterPos2fv");
	qglRasterPos2i							= (GLRASTERPOS2I)QGL_GetProcAddress("glRasterPos2i");
	qglRasterPos2iv							= (GLRASTERPOS2IV)QGL_GetProcAddress("glRasterPos2iv");
	qglRasterPos2s							= (GLRASTERPOS2S)QGL_GetProcAddress("glRasterPos2s");
	qglRasterPos2sv							= (GLRASTERPOS2SV)QGL_GetProcAddress("glRasterPos2sv");
	qglRasterPos3d							= (GLRASTERPOS3D)QGL_GetProcAddress("glRasterPos3d");
	qglRasterPos3dv							= (GLRASTERPOS3DV)QGL_GetProcAddress("glRasterPos3dv");
	qglRasterPos3f							= (GLRASTERPOS3F)QGL_GetProcAddress("glRasterPos3f");
	qglRasterPos3fv							= (GLRASTERPOS3FV)QGL_GetProcAddress("glRasterPos3fv");
	qglRasterPos3i							= (GLRASTERPOS3I)QGL_GetProcAddress("glRasterPos3i");
	qglRasterPos3iv							= (GLRASTERPOS3IV)QGL_GetProcAddress("glRasterPos3iv");
	qglRasterPos3s							= (GLRASTERPOS3S)QGL_GetProcAddress("glRasterPos3s");
	qglRasterPos3sv							= (GLRASTERPOS3SV)QGL_GetProcAddress("glRasterPos3sv");
	qglRasterPos4d							= (GLRASTERPOS4D)QGL_GetProcAddress("glRasterPos4d");
	qglRasterPos4dv							= (GLRASTERPOS4DV)QGL_GetProcAddress("glRasterPos4dv");
	qglRasterPos4f							= (GLRASTERPOS4F)QGL_GetProcAddress("glRasterPos4f");
	qglRasterPos4fv							= (GLRASTERPOS4FV)QGL_GetProcAddress("glRasterPos4fv");
	qglRasterPos4i							= (GLRASTERPOS4I)QGL_GetProcAddress("glRasterPos4i");
	qglRasterPos4iv							= (GLRASTERPOS4IV)QGL_GetProcAddress("glRasterPos4iv");
	qglRasterPos4s							= (GLRASTERPOS4S)QGL_GetProcAddress("glRasterPos4s");
	qglRasterPos4sv							= (GLRASTERPOS4SV)QGL_GetProcAddress("glRasterPos4sv");
	qglReadBuffer							= (GLREADBUFFER)QGL_GetProcAddress("glReadBuffer");
	qglReadPixels							= (GLREADPIXELS)QGL_GetProcAddress("glReadPixels");
	qglRectd								= (GLRECTD)QGL_GetProcAddress("glRectd");
	qglRectdv								= (GLRECTDV)QGL_GetProcAddress("glRectdv");
	qglRectf								= (GLRECTF)QGL_GetProcAddress("glRectf");
	qglRectfv								= (GLRECTFV)QGL_GetProcAddress("glRectfv");
	qglRecti								= (GLRECTI)QGL_GetProcAddress("glRecti");
	qglRectiv								= (GLRECTIV)QGL_GetProcAddress("glRectiv");
	qglRects								= (GLRECTS)QGL_GetProcAddress("glRects");
	qglRectsv								= (GLRECTSV)QGL_GetProcAddress("glRectsv");
	qglRenderMode							= (GLRENDERMODE)QGL_GetProcAddress("glRenderMode");
	qglRenderbufferStorage					= NULL;
	qglRenderbufferStorageMultisample		= NULL;
	qglRotated								= (GLROTATED)QGL_GetProcAddress("glRotated");
	qglRotatef								= (GLROTATEF)QGL_GetProcAddress("glRotatef");
	qglSampleCoverage						= NULL;
	qglSampleMaski							= NULL;
	qglSamplerParameterIiv					= NULL;
	qglSamplerParameterIuiv					= NULL;
	qglSamplerParameterf					= NULL;
	qglSamplerParameterfv					= NULL;
	qglSamplerParameteri					= NULL;
	qglSamplerParameteriv					= NULL;
	qglScaled								= (GLSCALED)QGL_GetProcAddress("glScaled");
	qglScalef								= (GLSCALEF)QGL_GetProcAddress("glScalef");
	qglScissor								= (GLSCISSOR)QGL_GetProcAddress("glScissor");
	qglSecondaryColor3b						= NULL;
	qglSecondaryColor3bv					= NULL;
	qglSecondaryColor3d						= NULL;
	qglSecondaryColor3dv					= NULL;
	qglSecondaryColor3f						= NULL;
	qglSecondaryColor3fv					= NULL;
	qglSecondaryColor3i						= NULL;
	qglSecondaryColor3iv					= NULL;
	qglSecondaryColor3s						= NULL;
	qglSecondaryColor3sv					= NULL;
	qglSecondaryColor3ub					= NULL;
	qglSecondaryColor3ubv					= NULL;
	qglSecondaryColor3ui					= NULL;
	qglSecondaryColor3uiv					= NULL;
	qglSecondaryColor3us					= NULL;
	qglSecondaryColor3usv					= NULL;
	qglSecondaryColorP3ui					= NULL;
	qglSecondaryColorP3uiv					= NULL;
	qglSecondaryColorPointer				= NULL;
	qglSelectBuffer							= (GLSELECTBUFFER)QGL_GetProcAddress("glSelectBuffer");
	qglShadeModel							= (GLSHADEMODEL)QGL_GetProcAddress("glShadeModel");
	qglShaderSource							= NULL;
	qglStencilFunc							= (GLSTENCILFUNC)QGL_GetProcAddress("glStencilFunc");
	qglStencilFuncSeparate					= NULL;
	qglStencilMask							= (GLSTENCILMASK)QGL_GetProcAddress("glStencilMask");
	qglStencilMaskSeparate					= NULL;
	qglStencilOp							= (GLSTENCILOP)QGL_GetProcAddress("glStencilOp");
	qglStencilOpSeparate					= NULL;
	qglStencilOpSeparateATI					= NULL;
	qglStencilFuncSeparateATI				= NULL;
	qglTexBuffer							= NULL;
	qglTexCoord1d							= (GLTEXCOORD1D)QGL_GetProcAddress("glTexCoord1d");
	qglTexCoord1dv							= (GLTEXCOORD1DV)QGL_GetProcAddress("glTexCoord1dv");
	qglTexCoord1f							= (GLTEXCOORD1F)QGL_GetProcAddress("glTexCoord1f");
	qglTexCoord1fv							= (GLTEXCOORD1FV)QGL_GetProcAddress("glTexCoord1fv");
	qglTexCoord1i							= (GLTEXCOORD1I)QGL_GetProcAddress("glTexCoord1i");
	qglTexCoord1iv							= (GLTEXCOORD1IV)QGL_GetProcAddress("glTexCoord1iv");
	qglTexCoord1s							= (GLTEXCOORD1S)QGL_GetProcAddress("glTexCoord1s");
	qglTexCoord1sv							= (GLTEXCOORD1SV)QGL_GetProcAddress("glTexCoord1sv");
	qglTexCoord2d							= (GLTEXCOORD2D)QGL_GetProcAddress("glTexCoord2d");
	qglTexCoord2dv							= (GLTEXCOORD2DV)QGL_GetProcAddress("glTexCoord2dv");
	qglTexCoord2f							= (GLTEXCOORD2F)QGL_GetProcAddress("glTexCoord2f");
	qglTexCoord2fv							= (GLTEXCOORD2FV)QGL_GetProcAddress("glTexCoord2fv");
	qglTexCoord2i							= (GLTEXCOORD2I)QGL_GetProcAddress("glTexCoord2i");
	qglTexCoord2iv							= (GLTEXCOORD2IV)QGL_GetProcAddress("glTexCoord2iv");
	qglTexCoord2s							= (GLTEXCOORD2S)QGL_GetProcAddress("glTexCoord2s");
	qglTexCoord2sv							= (GLTEXCOORD2SV)QGL_GetProcAddress("glTexCoord2sv");
	qglTexCoord3d							= (GLTEXCOORD3D)QGL_GetProcAddress("glTexCoord3d");
	qglTexCoord3dv							= (GLTEXCOORD3DV)QGL_GetProcAddress("glTexCoord3dv");
	qglTexCoord3f							= (GLTEXCOORD3F)QGL_GetProcAddress("glTexCoord3f");
	qglTexCoord3fv							= (GLTEXCOORD3FV)QGL_GetProcAddress("glTexCoord3fv");
	qglTexCoord3i							= (GLTEXCOORD3I)QGL_GetProcAddress("glTexCoord3i");
	qglTexCoord3iv							= (GLTEXCOORD3IV)QGL_GetProcAddress("glTexCoord3iv");
	qglTexCoord3s							= (GLTEXCOORD3S)QGL_GetProcAddress("glTexCoord3s");
	qglTexCoord3sv							= (GLTEXCOORD3SV)QGL_GetProcAddress("glTexCoord3sv");
	qglTexCoord4d							= (GLTEXCOORD4D)QGL_GetProcAddress("glTexCoord4d");
	qglTexCoord4dv							= (GLTEXCOORD4DV)QGL_GetProcAddress("glTexCoord4dv");
	qglTexCoord4f							= (GLTEXCOORD4F)QGL_GetProcAddress("glTexCoord4f");
	qglTexCoord4fv							= (GLTEXCOORD4FV)QGL_GetProcAddress("glTexCoord4fv");
	qglTexCoord4i							= (GLTEXCOORD4I)QGL_GetProcAddress("glTexCoord4i");
	qglTexCoord4iv							= (GLTEXCOORD4IV)QGL_GetProcAddress("glTexCoord4iv");
	qglTexCoord4s							= (GLTEXCOORD4S)QGL_GetProcAddress("glTexCoord4s");
	qglTexCoord4sv							= (GLTEXCOORD4SV)QGL_GetProcAddress("glTexCoord4sv");
	qglTexCoordP1ui							= NULL;
	qglTexCoordP1uiv						= NULL;
	qglTexCoordP2ui							= NULL;
	qglTexCoordP2uiv						= NULL;
	qglTexCoordP3ui							= NULL;
	qglTexCoordP3uiv						= NULL;
	qglTexCoordP4ui							= NULL;
	qglTexCoordP4uiv						= NULL;
	qglTexCoordPointer						= (GLTEXCOORDPOINTER)QGL_GetProcAddress("glTexCoordPointer");
	qglTexEnvf								= (GLTEXENVF)QGL_GetProcAddress("glTexEnvf");
	qglTexEnvfv								= (GLTEXENVFV)QGL_GetProcAddress("glTexEnvfv");
	qglTexEnvi								= (GLTEXENVI)QGL_GetProcAddress("glTexEnvi");
	qglTexEnviv								= (GLTEXENVIV)QGL_GetProcAddress("glTexEnviv");
	qglTexGend								= (GLTEXGEND)QGL_GetProcAddress("glTexGend");
	qglTexGendv								= (GLTEXGENDV)QGL_GetProcAddress("glTexGendv");
	qglTexGenf								= (GLTEXGENF)QGL_GetProcAddress("glTexGenf");
	qglTexGenfv								= (GLTEXGENFV)QGL_GetProcAddress("glTexGenfv");
	qglTexGeni								= (GLTEXGENI)QGL_GetProcAddress("glTexGeni");
	qglTexGeniv								= (GLTEXGENIV)QGL_GetProcAddress("glTexGeniv");
	qglTexImage1D							= (GLTEXIMAGE1D)QGL_GetProcAddress("glTexImage1D");
	qglTexImage2D							= (GLTEXIMAGE2D)QGL_GetProcAddress("glTexImage2D");
	qglTexImage2DMultisample				= NULL;
	qglTexImage3D							= NULL;
	qglTexImage3DMultisample				= NULL;
	qglTexParameterIiv						= NULL;
	qglTexParameterIuiv						= NULL;
	qglTexParameterf						= (GLTEXPARAMETERF)QGL_GetProcAddress("glTexParameterf");
	qglTexParameterfv						= (GLTEXPARAMETERFV)QGL_GetProcAddress("glTexParameterfv");
	qglTexParameteri						= (GLTEXPARAMETERI)QGL_GetProcAddress("glTexParameteri");
	qglTexParameteriv						= (GLTEXPARAMETERIV)QGL_GetProcAddress("glTexParameteriv");
	qglTexSubImage1D						= (GLTEXSUBIMAGE1D)QGL_GetProcAddress("glTexSubImage1D");
	qglTexSubImage2D						= (GLTEXSUBIMAGE2D)QGL_GetProcAddress("glTexSubImage2D");
	qglTexSubImage3D						= NULL;
	qglTransformFeedbackVaryings			= NULL;
	qglTranslated							= (GLTRANSLATED)QGL_GetProcAddress("glTranslated");
	qglTranslatef							= (GLTRANSLATEF)QGL_GetProcAddress("glTranslatef");
	qglUniform1f							= NULL;
	qglUniform1fv							= NULL;
	qglUniform1i							= NULL;
	qglUniform1iv							= NULL;
	qglUniform1ui							= NULL;
	qglUniform1uiv							= NULL;
	qglUniform2f							= NULL;
	qglUniform2fv							= NULL;
	qglUniform2i							= NULL;
	qglUniform2iv							= NULL;
	qglUniform2ui							= NULL;
	qglUniform2uiv							= NULL;
	qglUniform3f							= NULL;
	qglUniform3fv							= NULL;
	qglUniform3i							= NULL;
	qglUniform3iv							= NULL;
	qglUniform3ui							= NULL;
	qglUniform3uiv							= NULL;
	qglUniform4f							= NULL;
	qglUniform4fv							= NULL;
	qglUniform4i							= NULL;
	qglUniform4iv							= NULL;
	qglUniform4ui							= NULL;
	qglUniform4uiv							= NULL;
	qglUniformBlockBinding					= NULL;
	qglUniformMatrix2fv						= NULL;
	qglUniformMatrix2x3fv					= NULL;
	qglUniformMatrix2x4fv					= NULL;
	qglUniformMatrix3fv						= NULL;
	qglUniformMatrix3x2fv					= NULL;
	qglUniformMatrix3x4fv					= NULL;
	qglUniformMatrix4fv						= NULL;
	qglUniformMatrix4x2fv					= NULL;
	qglUniformMatrix4x3fv					= NULL;
	qglUnmapBuffer							= NULL;
	qglUseProgram							= NULL;
	qglValidateProgram						= NULL;
	qglVertex2d								= (GLVERTEX2D)QGL_GetProcAddress("glVertex2d");
	qglVertex2dv							= (GLVERTEX2DV)QGL_GetProcAddress("glVertex2dv");
	qglVertex2f								= (GLVERTEX2F)QGL_GetProcAddress("glVertex2f");
	qglVertex2fv							= (GLVERTEX2FV)QGL_GetProcAddress("glVertex2fv");
	qglVertex2i								= (GLVERTEX2I)QGL_GetProcAddress("glVertex2i");
	qglVertex2iv							= (GLVERTEX2IV)QGL_GetProcAddress("glVertex2iv");
	qglVertex2s								= (GLVERTEX2S)QGL_GetProcAddress("glVertex2s");
	qglVertex2sv							= (GLVERTEX2SV)QGL_GetProcAddress("glVertex2sv");
	qglVertex3d								= (GLVERTEX3D)QGL_GetProcAddress("glVertex3d");
	qglVertex3dv							= (GLVERTEX3DV)QGL_GetProcAddress("glVertex3dv");
	qglVertex3f								= (GLVERTEX3F)QGL_GetProcAddress("glVertex3f");
	qglVertex3fv							= (GLVERTEX3FV)QGL_GetProcAddress("glVertex3fv");
	qglVertex3i								= (GLVERTEX3I)QGL_GetProcAddress("glVertex3i");
	qglVertex3iv							= (GLVERTEX3IV)QGL_GetProcAddress("glVertex3iv");
	qglVertex3s								= (GLVERTEX3S)QGL_GetProcAddress("glVertex3s");
	qglVertex3sv							= (GLVERTEX3SV)QGL_GetProcAddress("glVertex3sv");
	qglVertex4d								= (GLVERTEX4D)QGL_GetProcAddress("glVertex4d");
	qglVertex4dv							= (GLVERTEX4DV)QGL_GetProcAddress("glVertex4dv");
	qglVertex4f								= (GLVERTEX4F)QGL_GetProcAddress("glVertex4f");
	qglVertex4fv							= (GLVERTEX4FV)QGL_GetProcAddress("glVertex4fv");
	qglVertex4i								= (GLVERTEX4I)QGL_GetProcAddress("glVertex4i");
	qglVertex4iv							= (GLVERTEX4IV)QGL_GetProcAddress("glVertex4iv");
	qglVertex4s								= (GLVERTEX4S)QGL_GetProcAddress("glVertex4s");
	qglVertex4sv							= (GLVERTEX4SV)QGL_GetProcAddress("glVertex4sv");
	qglVertexAttrib1d						= NULL;
	qglVertexAttrib1dv						= NULL;
	qglVertexAttrib1f						= NULL;
	qglVertexAttrib1fv						= NULL;
	qglVertexAttrib1s						= NULL;
	qglVertexAttrib1sv						= NULL;
	qglVertexAttrib2d						= NULL;
	qglVertexAttrib2dv						= NULL;
	qglVertexAttrib2f						= NULL;
	qglVertexAttrib2fv						= NULL;
	qglVertexAttrib2s						= NULL;
	qglVertexAttrib2sv						= NULL;
	qglVertexAttrib3d						= NULL;
	qglVertexAttrib3dv						= NULL;
	qglVertexAttrib3f						= NULL;
	qglVertexAttrib3fv						= NULL;
	qglVertexAttrib3s						= NULL;
	qglVertexAttrib3sv						= NULL;
	qglVertexAttrib4Nbv						= NULL;
	qglVertexAttrib4Niv						= NULL;
	qglVertexAttrib4Nsv						= NULL;
	qglVertexAttrib4Nub						= NULL;
	qglVertexAttrib4Nubv					= NULL;
	qglVertexAttrib4Nuiv					= NULL;
	qglVertexAttrib4Nusv					= NULL;
	qglVertexAttrib4bv						= NULL;
	qglVertexAttrib4d						= NULL;
	qglVertexAttrib4dv						= NULL;
	qglVertexAttrib4f						= NULL;
	qglVertexAttrib4fv						= NULL;
	qglVertexAttrib4iv						= NULL;
	qglVertexAttrib4s						= NULL;
	qglVertexAttrib4sv						= NULL;
	qglVertexAttrib4ubv						= NULL;
	qglVertexAttrib4uiv						= NULL;
	qglVertexAttrib4usv						= NULL;
	qglVertexAttribI1i						= NULL;
	qglVertexAttribI1iv						= NULL;
	qglVertexAttribI1ui						= NULL;
	qglVertexAttribI1uiv					= NULL;
	qglVertexAttribI2i						= NULL;
	qglVertexAttribI2iv						= NULL;
	qglVertexAttribI2ui						= NULL;
	qglVertexAttribI2uiv					= NULL;
	qglVertexAttribI3i						= NULL;
	qglVertexAttribI3iv						= NULL;
	qglVertexAttribI3ui						= NULL;
	qglVertexAttribI3uiv					= NULL;
	qglVertexAttribI4bv						= NULL;
	qglVertexAttribI4i						= NULL;
	qglVertexAttribI4iv						= NULL;
	qglVertexAttribI4sv						= NULL;
	qglVertexAttribI4ubv					= NULL;
	qglVertexAttribI4ui						= NULL;
	qglVertexAttribI4uiv					= NULL;
	qglVertexAttribI4usv					= NULL;
	qglVertexAttribIPointer					= NULL;
	qglVertexAttribP1ui						= NULL;
	qglVertexAttribP1uiv					= NULL;
	qglVertexAttribP2ui						= NULL;
	qglVertexAttribP2uiv					= NULL;
	qglVertexAttribP3ui						= NULL;
	qglVertexAttribP3uiv					= NULL;
	qglVertexAttribP4ui						= NULL;
	qglVertexAttribP4uiv					= NULL;
	qglVertexAttribPointer					= NULL;
	qglVertexP2ui							= NULL;
	qglVertexP2uiv							= NULL;
	qglVertexP3ui							= NULL;
	qglVertexP3uiv							= NULL;
	qglVertexP4ui							= NULL;
	qglVertexP4uiv							= NULL;
	qglVertexPointer						= (GLVERTEXPOINTER)QGL_GetProcAddress("glVertexPointer");
	qglViewport								= (GLVIEWPORT)QGL_GetProcAddress("glViewport");
	qglWaitSync								= NULL;
	qglWindowPos2d							= NULL;
	qglWindowPos2dv							= NULL;
	qglWindowPos2f							= NULL;
	qglWindowPos2fv							= NULL;
	qglWindowPos2i							= NULL;
	qglWindowPos2iv							= NULL;
	qglWindowPos2s							= NULL;
	qglWindowPos2sv							= NULL;
	qglWindowPos3d							= NULL;
	qglWindowPos3dv							= NULL;
	qglWindowPos3f							= NULL;
	qglWindowPos3fv							= NULL;
	qglWindowPos3i							= NULL;
	qglWindowPos3iv							= NULL;
	qglWindowPos3s							= NULL;
	qglWindowPos3sv							= NULL;

	return true;
}

/*
 ==================
 QGL_Shutdown

 Unloads the specified DLL then nulls out all the QGL function pointers
 ==================
*/
void QGL_Shutdown (){

	Com_Printf("...shutting down QGL\n");

	if (qglState.logFile){
		fclose(qglState.logFile);
		qglState.logFile = NULL;
	}

	if (qglState.hModule){
		Com_Printf("...unloading OpenGL DLL\n");

		FreeLibrary(qglState.hModule);
		qglState.hModule = NULL;
	}

	Mem_Fill(&qglState, 0, sizeof(qglState_t));

	qwglChoosePixelFormat					= NULL;
	qwglCopyContext							= NULL;
	qwglCreateContext						= NULL;
	qwglCreateLayerContext					= NULL;
	qwglDeleteContext						= NULL;
	qwglDescribeLayerPlane					= NULL;
	qwglDescribePixelFormat					= NULL;
	qwglGetCurrentContext					= NULL;
	qwglGetCurrentDC						= NULL;
	qwglGetLayerPaletteEntries				= NULL;
	qwglGetPixelFormat						= NULL;
	qwglGetProcAddress						= NULL;
	qwglMakeCurrent							= NULL;
	qwglRealizeLayerPalette					= NULL;
	qwglSetLayerPaletteEntries				= NULL;
	qwglSetPixelFormat						= NULL;
	qwglShareLists							= NULL;
	qwglSwapBuffers							= NULL;
	qwglSwapLayerBuffers					= NULL;
	qwglUseFontBitmaps						= NULL;
	qwglUseFontOutlines						= NULL;

	qwglSwapIntervalEXT						= NULL;

	qglAccum								= NULL;
	qglActiveStencilFaceEXT					= NULL;
	qglActiveTexture						= NULL;
	qglAlphaFunc							= NULL;
	qglAreTexturesResident					= NULL;
	qglArrayElement							= NULL;
	qglAttachShader							= NULL;
	qglBegin								= NULL;
	qglBeginConditionalRender				= NULL;
	qglBeginQuery							= NULL;
	qglBeginTransformFeedback				= NULL;
	qglBindAttribLocation					= NULL;
	qglBindBuffer							= NULL;
	qglBindBufferBase						= NULL;
	qglBindBufferRange						= NULL;
	qglBindFragDataLocation					= NULL;
	qglBindFragDataLocationIndexed			= NULL;
	qglBindFramebuffer						= NULL;
	qglBindRenderbuffer						= NULL;
	qglBindSampler							= NULL;
	qglBindTexture							= NULL;
	qglBindVertexArray						= NULL;
	qglBitmap								= NULL;
	qglBlendColor							= NULL;
	qglBlendEquation						= NULL;
	qglBlendEquationSeparate				= NULL;
	qglBlendFunc							= NULL;
	qglBlendFuncSeparate					= NULL;
	qglBlitFramebuffer						= NULL;
	qglBufferData							= NULL;
	qglBufferSubData						= NULL;
	qglCallList								= NULL;
	qglCallLists							= NULL;
	qglCheckFramebufferStatus				= NULL;
	qglClampColor							= NULL;
	qglClear								= NULL;
	qglClearAccum							= NULL;
	qglClearBufferfi						= NULL;
	qglClearBufferfv						= NULL;
	qglClearBufferiv						= NULL;
	qglClearBufferuiv						= NULL;
	qglClearColor							= NULL;
	qglClearDepth							= NULL;
	qglClearIndex							= NULL;
	qglClearStencil							= NULL;
	qglClientActiveTexture					= NULL;
	qglClientWaitSync						= NULL;
	qglClipPlane							= NULL;
	qglColor3b								= NULL;
	qglColor3bv								= NULL;
	qglColor3d								= NULL;
	qglColor3dv								= NULL;
	qglColor3f								= NULL;
	qglColor3fv								= NULL;
	qglColor3i								= NULL;
	qglColor3iv								= NULL;
	qglColor3s								= NULL;
	qglColor3sv								= NULL;
	qglColor3ub								= NULL;
	qglColor3ubv							= NULL;
	qglColor3ui								= NULL;
	qglColor3uiv							= NULL;
	qglColor3us								= NULL;
	qglColor3usv							= NULL;
	qglColor4b								= NULL;
	qglColor4bv								= NULL;
	qglColor4d								= NULL;
	qglColor4dv								= NULL;
	qglColor4f								= NULL;
	qglColor4fv								= NULL;
	qglColor4i								= NULL;
	qglColor4iv								= NULL;
	qglColor4s								= NULL;
	qglColor4sv								= NULL;
	qglColor4ub								= NULL;
	qglColor4ubv							= NULL;
	qglColor4ui								= NULL;
	qglColor4uiv							= NULL;
	qglColor4us								= NULL;
	qglColor4usv							= NULL;
	qglColorMask							= NULL;
	qglColorMaski							= NULL;
	qglColorMaterial						= NULL;
	qglColorP3ui							= NULL;
	qglColorP3uiv							= NULL;
	qglColorP4ui							= NULL;
	qglColorP4uiv							= NULL;
	qglColorPointer							= NULL;
	qglCompileShader						= NULL;
	qglCompressedTexImage1D					= NULL;
	qglCompressedTexImage2D					= NULL;
	qglCompressedTexImage3D					= NULL;
	qglCompressedTexSubImage1D				= NULL;
	qglCompressedTexSubImage2D				= NULL;
	qglCompressedTexSubImage3D				= NULL;
	qglCopyBufferSubData					= NULL;
	qglCopyPixels							= NULL;
	qglCopyTexImage1D						= NULL;
	qglCopyTexImage2D						= NULL;
	qglCopyTexSubImage1D					= NULL;
	qglCopyTexSubImage2D					= NULL;
	qglCopyTexSubImage3D					= NULL;
	qglCreateProgram						= NULL;
	qglCreateShader							= NULL;
	qglCullFace								= NULL;
	qglDeleteBuffers						= NULL;
	qglDeleteFramebuffers					= NULL;
	qglDeleteLists							= NULL;
	qglDeleteProgram						= NULL;
	qglDeleteQueries						= NULL;
	qglDeleteRenderbuffers					= NULL;
	qglDeleteSamplers						= NULL;
	qglDeleteShader							= NULL;
	qglDeleteSync							= NULL;
	qglDeleteTextures						= NULL;
	qglDeleteVertexArrays					= NULL;
	qglDepthBoundsEXT						= NULL;
	qglDepthFunc							= NULL;
	qglDepthMask							= NULL;
	qglDepthRange							= NULL;
	qglDetachShader							= NULL;
	qglDisable								= NULL;
	qglDisableClientState					= NULL;
	qglDisableVertexAttribArray				= NULL;
	qglDisablei								= NULL;
	qglDrawArrays							= NULL;
	qglDrawArraysInstanced					= NULL;
	qglDrawBuffer							= NULL;
	qglDrawBuffers							= NULL;
	qglDrawElements							= NULL;
	qglDrawElementsBaseVertex				= NULL;
	qglDrawElementsInstanced				= NULL;
	qglDrawElementsInstancedBaseVertex		= NULL;
	qglDrawPixels							= NULL;
	qglDrawRangeElements					= NULL;
	qglDrawRangeElementsBaseVertex			= NULL;
	qglEdgeFlag								= NULL;
	qglEdgeFlagPointer						= NULL;
	qglEdgeFlagv							= NULL;
	qglEnable								= NULL;
	qglEnableClientState					= NULL;
	qglEnableVertexAttribArray				= NULL;
	qglEnablei								= NULL;
	qglEnd									= NULL;
	qglEndConditionalRender					= NULL;
	qglEndList								= NULL;
	qglEndQuery								= NULL;
	qglEndTransformFeedback					= NULL;
	qglEvalCoord1d							= NULL;
	qglEvalCoord1dv							= NULL;
	qglEvalCoord1f							= NULL;
	qglEvalCoord1fv							= NULL;
	qglEvalCoord2d							= NULL;
	qglEvalCoord2dv							= NULL;
	qglEvalCoord2f							= NULL;
	qglEvalCoord2fv							= NULL;
	qglEvalMesh1							= NULL;
	qglEvalMesh2							= NULL;
	qglEvalPoint1							= NULL;
	qglEvalPoint2							= NULL;
	qglFeedbackBuffer						= NULL;
	qglFenceSync							= NULL;
	qglFinish								= NULL;
	qglFlush								= NULL;
	qglFlushMappedBufferRange				= NULL;
	qglFogCoordPointer						= NULL;
	qglFogCoordd							= NULL;
	qglFogCoorddv							= NULL;
	qglFogCoordf							= NULL;
	qglFogCoordfv							= NULL;
	qglFogf									= NULL;
	qglFogfv								= NULL;
	qglFogi									= NULL;
	qglFogiv								= NULL;
	qglFramebufferRenderbuffer				= NULL;
	qglFramebufferTexture					= NULL;
	qglFramebufferTexture1D					= NULL;
	qglFramebufferTexture2D					= NULL;
	qglFramebufferTexture3D					= NULL;
	qglFramebufferTextureLayer				= NULL;
	qglFrontFace							= NULL;
	qglFrustum								= NULL;
	qglGenBuffers							= NULL;
	qglGenFramebuffers						= NULL;
	qglGenLists								= NULL;
	qglGenQueries							= NULL;
	qglGenRenderbuffers						= NULL;
	qglGenSamplers							= NULL;
	qglGenTextures							= NULL;
	qglGenVertexArrays						= NULL;
	qglGenerateMipmap						= NULL;
	qglGetActiveAttrib						= NULL;
	qglGetActiveUniform						= NULL;
	qglGetActiveUniformBlockName			= NULL;
	qglGetActiveUniformBlockiv				= NULL;
	qglGetActiveUniformName					= NULL;
	qglGetActiveUniformsiv					= NULL;
	qglGetAttachedShaders					= NULL;
	qglGetAttribLocation					= NULL;
	qglGetBooleani_v						= NULL;
	qglGetBooleanv							= NULL;
	qglGetBufferParameteri64v				= NULL;
	qglGetBufferParameteriv					= NULL;
	qglGetBufferPointerv					= NULL;
	qglGetBufferSubData						= NULL;
	qglGetClipPlane							= NULL;
	qglGetCompressedTexImage				= NULL;
	qglGetDoublev							= NULL;
	qglGetError								= NULL;
	qglGetFloatv							= NULL;
	qglGetFragDataIndex						= NULL;
	qglGetFragDataLocation					= NULL;
	qglGetFramebufferAttachmentParameteriv	= NULL;
	qglGetInteger64i_v						= NULL;
	qglGetInteger64v						= NULL;
	qglGetIntegeri_v						= NULL;
	qglGetIntegerv							= NULL;
	qglGetLightfv							= NULL;
	qglGetLightiv							= NULL;
	qglGetMapdv								= NULL;
	qglGetMapfv								= NULL;
	qglGetMapiv								= NULL;
	qglGetMaterialfv						= NULL;
	qglGetMaterialiv						= NULL;
	qglGetMultisamplefv						= NULL;
	qglGetPixelMapfv						= NULL;
	qglGetPixelMapuiv						= NULL;
	qglGetPixelMapusv						= NULL;
	qglGetPointerv							= NULL;
	qglGetPolygonStipple					= NULL;
	qglGetProgramInfoLog					= NULL;
	qglGetProgramiv							= NULL;
	qglGetQueryObjecti64v					= NULL;
	qglGetQueryObjectiv						= NULL;
	qglGetQueryObjectui64v					= NULL;
	qglGetQueryObjectuiv					= NULL;
	qglGetQueryiv							= NULL;
	qglGetRenderbufferParameteriv			= NULL;
	qglGetSamplerParameterIiv				= NULL;
	qglGetSamplerParameterIuiv				= NULL;
	qglGetSamplerParameterfv				= NULL;
	qglGetSamplerParameteriv				= NULL;
	qglGetShaderInfoLog						= NULL;
	qglGetShaderSource						= NULL;
	qglGetShaderiv							= NULL;
	qglGetString							= NULL;
	qglGetStringi							= NULL;
	qglGetSynciv							= NULL;
	qglGetTexEnvfv							= NULL;
	qglGetTexEnviv							= NULL;
	qglGetTexGendv							= NULL;
	qglGetTexGenfv							= NULL;
	qglGetTexGeniv							= NULL;
	qglGetTexImage							= NULL;
	qglGetTexLevelParameterfv				= NULL;
	qglGetTexLevelParameteriv				= NULL;
	qglGetTexParameterIiv					= NULL;
	qglGetTexParameterIuiv					= NULL;
	qglGetTexParameterfv					= NULL;
	qglGetTexParameteriv					= NULL;
	qglGetTransformFeedbackVarying			= NULL;
	qglGetUniformBlockIndex					= NULL;
	qglGetUniformIndices					= NULL;
	qglGetUniformLocation					= NULL;
	qglGetUniformfv							= NULL;
	qglGetUniformiv							= NULL;
	qglGetUniformuiv						= NULL;
	qglGetVertexAttribIiv					= NULL;
	qglGetVertexAttribIuiv					= NULL;
	qglGetVertexAttribPointerv				= NULL;
	qglGetVertexAttribdv					= NULL;
	qglGetVertexAttribfv					= NULL;
	qglGetVertexAttribiv					= NULL;
	qglHint									= NULL;
	qglIndexMask							= NULL;
	qglIndexPointer							= NULL;
	qglIndexd								= NULL;
	qglIndexdv								= NULL;
	qglIndexf								= NULL;
	qglIndexfv								= NULL;
	qglIndexi								= NULL;
	qglIndexiv								= NULL;
	qglIndexs								= NULL;
	qglIndexsv								= NULL;
	qglIndexub								= NULL;
	qglIndexubv								= NULL;
	qglInitNames							= NULL;
	qglInterleavedArrays					= NULL;
	qglIsBuffer								= NULL;
	qglIsEnabled							= NULL;
	qglIsEnabledi							= NULL;
	qglIsFramebuffer						= NULL;
	qglIsList								= NULL;
	qglIsProgram							= NULL;
	qglIsQuery								= NULL;
	qglIsRenderbuffer						= NULL;
	qglIsSampler							= NULL;
	qglIsShader								= NULL;
	qglIsSync								= NULL;
	qglIsTexture							= NULL;
	qglIsVertexArray						= NULL;
	qglLightModelf							= NULL;
	qglLightModelfv							= NULL;
	qglLightModeli							= NULL;
	qglLightModeliv							= NULL;
	qglLightf								= NULL;
	qglLightfv								= NULL;
	qglLighti								= NULL;
	qglLightiv								= NULL;
	qglLineStipple							= NULL;
	qglLineWidth							= NULL;
	qglLinkProgram							= NULL;
	qglListBase								= NULL;
	qglLoadIdentity							= NULL;
	qglLoadMatrixd							= NULL;
	qglLoadMatrixf							= NULL;
	qglLoadName								= NULL;
	qglLoadTransposeMatrixd					= NULL;
	qglLoadTransposeMatrixf					= NULL;
	qglLogicOp								= NULL;
	qglMap1d								= NULL;
	qglMap1f								= NULL;
	qglMap2d								= NULL;
	qglMap2f								= NULL;
	qglMapBuffer							= NULL;
	qglMapBufferRange						= NULL;
	qglMapGrid1d							= NULL;
	qglMapGrid1f							= NULL;
	qglMapGrid2d							= NULL;
	qglMapGrid2f							= NULL;
	qglMaterialf							= NULL;
	qglMaterialfv							= NULL;
	qglMateriali							= NULL;
	qglMaterialiv							= NULL;
	qglMatrixMode							= NULL;
	qglMultMatrixd							= NULL;
	qglMultMatrixf							= NULL;
	qglMultTransposeMatrixd					= NULL;
	qglMultTransposeMatrixf					= NULL;
	qglMultiDrawArrays						= NULL;
	qglMultiDrawElements					= NULL;
	qglMultiDrawElementsBaseVertex			= NULL;
	qglMultiTexCoord1d						= NULL;
	qglMultiTexCoord1dv						= NULL;
	qglMultiTexCoord1f						= NULL;
	qglMultiTexCoord1fv						= NULL;
	qglMultiTexCoord1i						= NULL;
	qglMultiTexCoord1iv						= NULL;
	qglMultiTexCoord1s						= NULL;
	qglMultiTexCoord1sv						= NULL;
	qglMultiTexCoord2d						= NULL;
	qglMultiTexCoord2dv						= NULL;
	qglMultiTexCoord2f						= NULL;
	qglMultiTexCoord2fv						= NULL;
	qglMultiTexCoord2i						= NULL;
	qglMultiTexCoord2iv						= NULL;
	qglMultiTexCoord2s						= NULL;
	qglMultiTexCoord2sv						= NULL;
	qglMultiTexCoord3d						= NULL;
	qglMultiTexCoord3dv						= NULL;
	qglMultiTexCoord3f						= NULL;
	qglMultiTexCoord3fv						= NULL;
	qglMultiTexCoord3i						= NULL;
	qglMultiTexCoord3iv						= NULL;
	qglMultiTexCoord3s						= NULL;
	qglMultiTexCoord3sv						= NULL;
	qglMultiTexCoord4d						= NULL;
	qglMultiTexCoord4dv						= NULL;
	qglMultiTexCoord4f						= NULL;
	qglMultiTexCoord4fv						= NULL;
	qglMultiTexCoord4i						= NULL;
	qglMultiTexCoord4iv						= NULL;
	qglMultiTexCoord4s						= NULL;
	qglMultiTexCoord4sv						= NULL;
	qglMultiTexCoordP1ui					= NULL;
	qglMultiTexCoordP1uiv					= NULL;
	qglMultiTexCoordP2ui					= NULL;
	qglMultiTexCoordP2uiv					= NULL;
	qglMultiTexCoordP3ui					= NULL;
	qglMultiTexCoordP3uiv					= NULL;
	qglMultiTexCoordP4ui					= NULL;
	qglMultiTexCoordP4uiv					= NULL;
	qglNewList								= NULL;
	qglNormal3b								= NULL;
	qglNormal3bv							= NULL;
	qglNormal3d								= NULL;
	qglNormal3dv							= NULL;
	qglNormal3f								= NULL;
	qglNormal3fv							= NULL;
	qglNormal3i								= NULL;
	qglNormal3iv							= NULL;
	qglNormal3s								= NULL;
	qglNormal3sv							= NULL;
	qglNormalP3ui							= NULL;
	qglNormalP3uiv							= NULL;
	qglNormalPointer						= NULL;
	qglOrtho								= NULL;
	qglPassThrough							= NULL;
	qglPixelMapfv							= NULL;
	qglPixelMapuiv							= NULL;
	qglPixelMapusv							= NULL;
	qglPixelStoref							= NULL;
	qglPixelStorei							= NULL;
	qglPixelTransferf						= NULL;
	qglPixelTransferi						= NULL;
	qglPixelZoom							= NULL;
	qglPointParameterf						= NULL;
	qglPointParameterfv						= NULL;
	qglPointParameteri						= NULL;
	qglPointParameteriv						= NULL;
	qglPointSize							= NULL;
	qglPolygonMode							= NULL;
	qglPolygonOffset						= NULL;
	qglPolygonStipple						= NULL;
	qglPopAttrib							= NULL;
	qglPopClientAttrib						= NULL;
	qglPopMatrix							= NULL;
	qglPopName								= NULL;
	qglPrimitiveRestartIndex				= NULL;
	qglPrioritizeTextures					= NULL;
	qglProvokingVertex						= NULL;
	qglPushAttrib							= NULL;
	qglPushClientAttrib						= NULL;
	qglPushMatrix							= NULL;
	qglPushName								= NULL;
	qglQueryCounter							= NULL;
	qglRasterPos2d							= NULL;
	qglRasterPos2dv							= NULL;
	qglRasterPos2f							= NULL;
	qglRasterPos2fv							= NULL;
	qglRasterPos2i							= NULL;
	qglRasterPos2iv							= NULL;
	qglRasterPos2s							= NULL;
	qglRasterPos2sv							= NULL;
	qglRasterPos3d							= NULL;
	qglRasterPos3dv							= NULL;
	qglRasterPos3f							= NULL;
	qglRasterPos3fv							= NULL;
	qglRasterPos3i							= NULL;
	qglRasterPos3iv							= NULL;
	qglRasterPos3s							= NULL;
	qglRasterPos3sv							= NULL;
	qglRasterPos4d							= NULL;
	qglRasterPos4dv							= NULL;
	qglRasterPos4f							= NULL;
	qglRasterPos4fv							= NULL;
	qglRasterPos4i							= NULL;
	qglRasterPos4iv							= NULL;
	qglRasterPos4s							= NULL;
	qglRasterPos4sv							= NULL;
	qglReadBuffer							= NULL;
	qglReadPixels							= NULL;
	qglRectd								= NULL;
	qglRectdv								= NULL;
	qglRectf								= NULL;
	qglRectfv								= NULL;
	qglRecti								= NULL;
	qglRectiv								= NULL;
	qglRects								= NULL;
	qglRectsv								= NULL;
	qglRenderMode							= NULL;
	qglRenderbufferStorage					= NULL;
	qglRenderbufferStorageMultisample		= NULL;
	qglRotated								= NULL;
	qglRotatef								= NULL;
	qglSampleCoverage						= NULL;
	qglSampleMaski							= NULL;
	qglSamplerParameterIiv					= NULL;
	qglSamplerParameterIuiv					= NULL;
	qglSamplerParameterf					= NULL;
	qglSamplerParameterfv					= NULL;
	qglSamplerParameteri					= NULL;
	qglSamplerParameteriv					= NULL;
	qglScaled								= NULL;
	qglScalef								= NULL;
	qglScissor								= NULL;
	qglSecondaryColor3b						= NULL;
	qglSecondaryColor3bv					= NULL;
	qglSecondaryColor3d						= NULL;
	qglSecondaryColor3dv					= NULL;
	qglSecondaryColor3f						= NULL;
	qglSecondaryColor3fv					= NULL;
	qglSecondaryColor3i						= NULL;
	qglSecondaryColor3iv					= NULL;
	qglSecondaryColor3s						= NULL;
	qglSecondaryColor3sv					= NULL;
	qglSecondaryColor3ub					= NULL;
	qglSecondaryColor3ubv					= NULL;
	qglSecondaryColor3ui					= NULL;
	qglSecondaryColor3uiv					= NULL;
	qglSecondaryColor3us					= NULL;
	qglSecondaryColor3usv					= NULL;
	qglSecondaryColorP3ui					= NULL;
	qglSecondaryColorP3uiv					= NULL;
	qglSecondaryColorPointer				= NULL;
	qglSelectBuffer							= NULL;
	qglShadeModel							= NULL;
	qglShaderSource							= NULL;
	qglStencilFunc							= NULL;
	qglStencilFuncSeparate					= NULL;
	qglStencilMask							= NULL;
	qglStencilMaskSeparate					= NULL;
	qglStencilOp							= NULL;
	qglStencilOpSeparate					= NULL;
	qglTexBuffer							= NULL;
	qglTexCoord1d							= NULL;
	qglTexCoord1dv							= NULL;
	qglTexCoord1f							= NULL;
	qglTexCoord1fv							= NULL;
	qglTexCoord1i							= NULL;
	qglTexCoord1iv							= NULL;
	qglTexCoord1s							= NULL;
	qglTexCoord1sv							= NULL;
	qglTexCoord2d							= NULL;
	qglTexCoord2dv							= NULL;
	qglTexCoord2f							= NULL;
	qglTexCoord2fv							= NULL;
	qglTexCoord2i							= NULL;
	qglTexCoord2iv							= NULL;
	qglTexCoord2s							= NULL;
	qglTexCoord2sv							= NULL;
	qglTexCoord3d							= NULL;
	qglTexCoord3dv							= NULL;
	qglTexCoord3f							= NULL;
	qglTexCoord3fv							= NULL;
	qglTexCoord3i							= NULL;
	qglTexCoord3iv							= NULL;
	qglTexCoord3s							= NULL;
	qglTexCoord3sv							= NULL;
	qglTexCoord4d							= NULL;
	qglTexCoord4dv							= NULL;
	qglTexCoord4f							= NULL;
	qglTexCoord4fv							= NULL;
	qglTexCoord4i							= NULL;
	qglTexCoord4iv							= NULL;
	qglTexCoord4s							= NULL;
	qglTexCoord4sv							= NULL;
	qglTexCoordP1ui							= NULL;
	qglTexCoordP1uiv						= NULL;
	qglTexCoordP2ui							= NULL;
	qglTexCoordP2uiv						= NULL;
	qglTexCoordP3ui							= NULL;
	qglTexCoordP3uiv						= NULL;
	qglTexCoordP4ui							= NULL;
	qglTexCoordP4uiv						= NULL;
	qglTexCoordPointer						= NULL;
	qglTexEnvf								= NULL;
	qglTexEnvfv								= NULL;
	qglTexEnvi								= NULL;
	qglTexEnviv								= NULL;
	qglTexGend								= NULL;
	qglTexGendv								= NULL;
	qglTexGenf								= NULL;
	qglTexGenfv								= NULL;
	qglTexGeni								= NULL;
	qglTexGeniv								= NULL;
	qglTexImage1D							= NULL;
	qglTexImage2D							= NULL;
	qglTexImage2DMultisample				= NULL;
	qglTexImage3D							= NULL;
	qglTexImage3DMultisample				= NULL;
	qglTexParameterIiv						= NULL;
	qglTexParameterIuiv						= NULL;
	qglTexParameterf						= NULL;
	qglTexParameterfv						= NULL;
	qglTexParameteri						= NULL;
	qglTexParameteriv						= NULL;
	qglTexSubImage1D						= NULL;
	qglTexSubImage2D						= NULL;
	qglTexSubImage3D						= NULL;
	qglTransformFeedbackVaryings			= NULL;
	qglTranslated							= NULL;
	qglTranslatef							= NULL;
	qglUniform1f							= NULL;
	qglUniform1fv							= NULL;
	qglUniform1i							= NULL;
	qglUniform1iv							= NULL;
	qglUniform1ui							= NULL;
	qglUniform1uiv							= NULL;
	qglUniform2f							= NULL;
	qglUniform2fv							= NULL;
	qglUniform2i							= NULL;
	qglUniform2iv							= NULL;
	qglUniform2ui							= NULL;
	qglUniform2uiv							= NULL;
	qglUniform3f							= NULL;
	qglUniform3fv							= NULL;
	qglUniform3i							= NULL;
	qglUniform3iv							= NULL;
	qglUniform3ui							= NULL;
	qglUniform3uiv							= NULL;
	qglUniform4f							= NULL;
	qglUniform4fv							= NULL;
	qglUniform4i							= NULL;
	qglUniform4iv							= NULL;
	qglUniform4ui							= NULL;
	qglUniform4uiv							= NULL;
	qglUniformBlockBinding					= NULL;
	qglUniformMatrix2fv						= NULL;
	qglUniformMatrix2x3fv					= NULL;
	qglUniformMatrix2x4fv					= NULL;
	qglUniformMatrix3fv						= NULL;
	qglUniformMatrix3x2fv					= NULL;
	qglUniformMatrix3x4fv					= NULL;
	qglUniformMatrix4fv						= NULL;
	qglUniformMatrix4x2fv					= NULL;
	qglUniformMatrix4x3fv					= NULL;
	qglUnmapBuffer							= NULL;
	qglUseProgram							= NULL;
	qglValidateProgram						= NULL;
	qglVertex2d								= NULL;
	qglVertex2dv							= NULL;
	qglVertex2f								= NULL;
	qglVertex2fv							= NULL;
	qglVertex2i								= NULL;
	qglVertex2iv							= NULL;
	qglVertex2s								= NULL;
	qglVertex2sv							= NULL;
	qglVertex3d								= NULL;
	qglVertex3dv							= NULL;
	qglVertex3f								= NULL;
	qglVertex3fv							= NULL;
	qglVertex3i								= NULL;
	qglVertex3iv							= NULL;
	qglVertex3s								= NULL;
	qglVertex3sv							= NULL;
	qglVertex4d								= NULL;
	qglVertex4dv							= NULL;
	qglVertex4f								= NULL;
	qglVertex4fv							= NULL;
	qglVertex4i								= NULL;
	qglVertex4iv							= NULL;
	qglVertex4s								= NULL;
	qglVertex4sv							= NULL;
	qglVertexAttrib1d						= NULL;
	qglVertexAttrib1dv						= NULL;
	qglVertexAttrib1f						= NULL;
	qglVertexAttrib1fv						= NULL;
	qglVertexAttrib1s						= NULL;
	qglVertexAttrib1sv						= NULL;
	qglVertexAttrib2d						= NULL;
	qglVertexAttrib2dv						= NULL;
	qglVertexAttrib2f						= NULL;
	qglVertexAttrib2fv						= NULL;
	qglVertexAttrib2s						= NULL;
	qglVertexAttrib2sv						= NULL;
	qglVertexAttrib3d						= NULL;
	qglVertexAttrib3dv						= NULL;
	qglVertexAttrib3f						= NULL;
	qglVertexAttrib3fv						= NULL;
	qglVertexAttrib3s						= NULL;
	qglVertexAttrib3sv						= NULL;
	qglVertexAttrib4Nbv						= NULL;
	qglVertexAttrib4Niv						= NULL;
	qglVertexAttrib4Nsv						= NULL;
	qglVertexAttrib4Nub						= NULL;
	qglVertexAttrib4Nubv					= NULL;
	qglVertexAttrib4Nuiv					= NULL;
	qglVertexAttrib4Nusv					= NULL;
	qglVertexAttrib4bv						= NULL;
	qglVertexAttrib4d						= NULL;
	qglVertexAttrib4dv						= NULL;
	qglVertexAttrib4f						= NULL;
	qglVertexAttrib4fv						= NULL;
	qglVertexAttrib4iv						= NULL;
	qglVertexAttrib4s						= NULL;
	qglVertexAttrib4sv						= NULL;
	qglVertexAttrib4ubv						= NULL;
	qglVertexAttrib4uiv						= NULL;
	qglVertexAttrib4usv						= NULL;
	qglVertexAttribI1i						= NULL;
	qglVertexAttribI1iv						= NULL;
	qglVertexAttribI1ui						= NULL;
	qglVertexAttribI1uiv					= NULL;
	qglVertexAttribI2i						= NULL;
	qglVertexAttribI2iv						= NULL;
	qglVertexAttribI2ui						= NULL;
	qglVertexAttribI2uiv					= NULL;
	qglVertexAttribI3i						= NULL;
	qglVertexAttribI3iv						= NULL;
	qglVertexAttribI3ui						= NULL;
	qglVertexAttribI3uiv					= NULL;
	qglVertexAttribI4bv						= NULL;
	qglVertexAttribI4i						= NULL;
	qglVertexAttribI4iv						= NULL;
	qglVertexAttribI4sv						= NULL;
	qglVertexAttribI4ubv					= NULL;
	qglVertexAttribI4ui						= NULL;
	qglVertexAttribI4uiv					= NULL;
	qglVertexAttribI4usv					= NULL;
	qglVertexAttribIPointer					= NULL;
	qglVertexAttribP1ui						= NULL;
	qglVertexAttribP1uiv					= NULL;
	qglVertexAttribP2ui						= NULL;
	qglVertexAttribP2uiv					= NULL;
	qglVertexAttribP3ui						= NULL;
	qglVertexAttribP3uiv					= NULL;
	qglVertexAttribP4ui						= NULL;
	qglVertexAttribP4uiv					= NULL;
	qglVertexAttribPointer					= NULL;
	qglVertexP2ui							= NULL;
	qglVertexP2uiv							= NULL;
	qglVertexP3ui							= NULL;
	qglVertexP3uiv							= NULL;
	qglVertexP4ui							= NULL;
	qglVertexP4uiv							= NULL;
	qglVertexPointer						= NULL;
	qglViewport								= NULL;
	qglWaitSync								= NULL;
	qglWindowPos2d							= NULL;
	qglWindowPos2dv							= NULL;
	qglWindowPos2f							= NULL;
	qglWindowPos2fv							= NULL;
	qglWindowPos2i							= NULL;
	qglWindowPos2iv							= NULL;
	qglWindowPos2s							= NULL;
	qglWindowPos2sv							= NULL;
	qglWindowPos3d							= NULL;
	qglWindowPos3dv							= NULL;
	qglWindowPos3f							= NULL;
	qglWindowPos3fv							= NULL;
	qglWindowPos3i							= NULL;
	qglWindowPos3iv							= NULL;
	qglWindowPos3s							= NULL;
	qglWindowPos3sv							= NULL;
}