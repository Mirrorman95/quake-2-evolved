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
// r_glState.c - GL state manager
//


#include "r_local.h"


static glState_t			glState;


/*
 ==============================================================================

 MATRIX FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 GL_LoadIdentity
 ==================
*/
void GL_LoadIdentity (uint mode){

	switch (mode){
	case GL_PROJECTION:
		if (glState.projectionMatrixIdentity)
			return;
		glState.projectionMatrixIdentity = true;

		break;
	case GL_MODELVIEW:
		if (glState.modelviewMatrixIdentity)
			return;
		glState.modelviewMatrixIdentity = true;

		break;
	case GL_TEXTURE:
		if (glState.textureMatrixIdentity[glState.texUnit])
			return;
		glState.textureMatrixIdentity[glState.texUnit] = true;

		break;
	}

	qglMatrixMode(mode);
	qglLoadIdentity();
}

/*
 ==================
 GL_LoadMatrix
 ==================
*/
void GL_LoadMatrix (uint mode, const mat4_t matrix){

	switch (mode){
	case GL_PROJECTION:
		glState.projectionMatrixIdentity = false;

		break;
	case GL_MODELVIEW:
		glState.modelviewMatrixIdentity = false;

		break;
	case GL_TEXTURE:
		glState.textureMatrixIdentity[glState.texUnit] = false;

		break;
	}

	qglMatrixMode(mode);
	qglLoadMatrixf(matrix);
}

/*
 ==================
 GL_LoadTransposeMatrix
 ==================
*/
void GL_LoadTransposeMatrix (uint mode, const mat4_t matrix){

	switch (mode){
	case GL_PROJECTION:
		glState.projectionMatrixIdentity = false;

		break;
	case GL_MODELVIEW:
		glState.modelviewMatrixIdentity = false;

		break;
	case GL_TEXTURE:
		glState.textureMatrixIdentity[glState.texUnit] = false;

		break;
	}

	qglMatrixMode(mode);
	qglLoadTransposeMatrixf(matrix);
}


/*
 ==============================================================================

 TEXTURE FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 GL_BindTexture
 ==================
*/
void GL_BindTexture (texture_t *texture){

	if (texture->frameUsed != rg.frameCount){
		texture->frameUsed = rg.frameCount;

		rg.pc.textures++;
		rg.pc.textureBytes += texture->size;
	}

	if (glState.texture[glState.texUnit] == texture)
		return;
	glState.texture[glState.texUnit] = texture;

	qglBindTexture(texture->target, texture->textureId);
}

/*
 ==================
 GL_BindMultitexture
 ==================
*/
void GL_BindMultitexture (texture_t *texture, int unit){

	if (texture->frameUsed != rg.frameCount){
		texture->frameUsed = rg.frameCount;

		rg.pc.textures++;
		rg.pc.textureBytes += texture->size;
	}

	if (glState.texture[unit] == texture)
		return;
	glState.texture[unit] = texture;

	if (glState.texUnit != unit){
		glState.texUnit = unit;

		qglActiveTexture(GL_TEXTURE0 + unit);
	}

	qglBindTexture(texture->target, texture->textureId);
}

/*
 ==================
 GL_SelectTexture
 ==================
*/
void GL_SelectTexture (int unit){

	if (glState.texUnit == unit)
		return;
	glState.texUnit = unit;

	qglActiveTexture(GL_TEXTURE0 + unit);
}

/*
 ==================
 GL_EnableTexture
 ==================
*/
void GL_EnableTexture (uint target){

	if (glState.texTarget[glState.texUnit] == target)
		return;

	if (glState.texTarget[glState.texUnit])
		qglDisable(glState.texTarget[glState.texUnit]);

	qglEnable(target);

	glState.texTarget[glState.texUnit] = target;
}

/*
 ==================
 GL_DisableTexture
 ==================
*/
void GL_DisableTexture (){

	if (!glState.texTarget[glState.texUnit])
		return;

	qglDisable(glState.texTarget[glState.texUnit]);

	glState.texTarget[glState.texUnit] = 0;
}

/*
 ==================
 GL_TexEnv
 ==================
*/
void GL_TexEnv (int texEnv){

	if (glState.texEnv[glState.texUnit] == texEnv)
		return;
	glState.texEnv[glState.texUnit] = texEnv;

	qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texEnv);
}

/*
 ==================
 GL_TexGen
 ==================
*/
void GL_TexGen (uint texCoord, int texGen){

	switch (texCoord){
	case GL_S:
		if (glState.texGen[glState.texUnit][0] == texGen)
			return;
		glState.texGen[glState.texUnit][0] = texGen;

		break;
	case GL_T:
		if (glState.texGen[glState.texUnit][1] == texGen)
			return;
		glState.texGen[glState.texUnit][1] = texGen;

		break;
	case GL_R:
		if (glState.texGen[glState.texUnit][2] == texGen)
			return;
		glState.texGen[glState.texUnit][2] = texGen;

		break;
	case GL_Q:
		if (glState.texGen[glState.texUnit][3] == texGen)
			return;
		glState.texGen[glState.texUnit][3] = texGen;

		break;
	}

	qglTexGeni(texCoord, GL_TEXTURE_GEN_MODE, texGen);
}


/*
 ==============================================================================

 PROGRAM FUNCTION

 ==============================================================================
*/


/*
 ==================
 GL_BindProgram
 ==================
*/
void GL_BindProgram (program_t *program){

	if (!program){
		if (!glState.program)
			return;
		glState.program = NULL;

		qglUseProgram(0);
		return;
	}

	if (glState.program == program)
		return;
	glState.program = program;

	qglUseProgram(program->programId);
}


/*
 ==============================================================================

 ARRAY BUFFER FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 GL_BindVertexBuffer
 ==================
*/
void GL_BindVertexBuffer (arrayBuffer_t *vertexBuffer){

	if (!vertexBuffer){
		if (!glState.vertexBuffer)
			return;
		glState.vertexBuffer = NULL;

		qglBindBuffer(GL_ARRAY_BUFFER, 0);
		return;
	}

	if (vertexBuffer->frameUsed != rg.frameCount){
		vertexBuffer->frameUsed = rg.frameCount;

		rg.pc.vertexBuffers[vertexBuffer->dynamic]++;
		rg.pc.vertexBufferBytes[vertexBuffer->dynamic] += vertexBuffer->size;
	}

	if (glState.vertexBuffer == vertexBuffer)
		return;
	glState.vertexBuffer = vertexBuffer;

	qglBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->bufferId);
}


/*
 ==============================================================================

 VIEWPORT FUNCTION

 ==============================================================================
*/


/*
 ==================
 GL_Viewport
 ==================
*/
void GL_Viewport (int x, int y, int width, int height){

	if (glState.viewportX == x && glState.viewportY == y && glState.viewportWidth == width && glState.viewportHeight == height)
		return;

	glState.viewportX = x;
	glState.viewportY = y;
	glState.viewportWidth = width;
	glState.viewportHeight = height;

	qglViewport(x, y, width, height);
}


/*
 ==============================================================================

 SCISSOR FUNCTION

 ==============================================================================
*/


/*
 ==================
 GL_Scissor

 TODO: use rec_t
 ==================
*/
void GL_Scissor (int x, int y, int width, int height){

	if (glState.scissorX == x && glState.scissorY == y && glState.scissorWidth == width && glState.scissorHeight == height)
		return;

	glState.scissorX = x;
	glState.scissorY = y;
	glState.scissorWidth = width;
	glState.scissorHeight = height;

	qglScissor(x, y, width, height);
}


/*
 ==============================================================================

 STATE FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 GL_Enable
 ==================
*/
void GL_Enable (uint cap){

	switch (cap){
	case GL_CULL_FACE:
		if (glState.cullFace)
			return;
		glState.cullFace = true;

		break;
	case GL_POLYGON_OFFSET_FILL:
		if (glState.polygonOffsetFill)
			return;
		glState.polygonOffsetFill = true;

		break;
	case GL_POLYGON_OFFSET_LINE:
		if (glState.polygonOffsetLine)
			return;
		glState.polygonOffsetLine = true;

		break;
	case GL_BLEND:
		if (glState.blend)
			return;
		glState.blend = true;

		break;
	case GL_ALPHA_TEST:
		if (glState.alphaTest)
			return;
		glState.alphaTest = true;

		break;
	case GL_DEPTH_TEST:
		if (glState.depthTest)
			return;
		glState.depthTest = true;

		break;
	case GL_STENCIL_TEST:
		if (glState.stencilTest)
			return;
		glState.stencilTest = true;

		break;
	case GL_TEXTURE_GEN_S:
		if (glState.textureGen[glState.texUnit][0])
			return;
		glState.textureGen[glState.texUnit][0] = true;

		break;
	case GL_TEXTURE_GEN_T:
		if (glState.textureGen[glState.texUnit][1])
			return;
		glState.textureGen[glState.texUnit][1] = true;

		break;
	case GL_TEXTURE_GEN_R:
		if (glState.textureGen[glState.texUnit][2])
			return;
		glState.textureGen[glState.texUnit][2] = true;

		break;
	case GL_TEXTURE_GEN_Q:
		if (glState.textureGen[glState.texUnit][3])
			return;
		glState.textureGen[glState.texUnit][3] = true;

		break;
	}

	qglEnable(cap);
}

/*
 ==================
 GL_Disable
 ==================
*/
void GL_Disable (uint cap){

	switch (cap){
	case GL_CULL_FACE:
		if (!glState.cullFace)
			return;
		glState.cullFace = false;

		break;
	case GL_POLYGON_OFFSET_FILL:
		if (!glState.polygonOffsetFill)
			return;
		glState.polygonOffsetFill = false;

		break;
	case GL_POLYGON_OFFSET_LINE:
		if (!glState.polygonOffsetLine)
			return;
		glState.polygonOffsetLine = false;

		break;
	case GL_BLEND:
		if (!glState.blend)
			return;
		glState.blend = false;

		break;
	case GL_ALPHA_TEST:
		if (!glState.alphaTest)
			return;
		glState.alphaTest = false;

		break;
	case GL_DEPTH_TEST:
		if (!glState.depthTest)
			return;
		glState.depthTest = false;

		break;
	case GL_STENCIL_TEST:
		if (!glState.stencilTest)
			return;
		glState.stencilTest = false;

		break;
	case GL_TEXTURE_GEN_S:
		if (!glState.textureGen[glState.texUnit][0])
			return;
		glState.textureGen[glState.texUnit][0] = false;

		break;
	case GL_TEXTURE_GEN_T:
		if (!glState.textureGen[glState.texUnit][1])
			return;
		glState.textureGen[glState.texUnit][1] = false;

		break;
	case GL_TEXTURE_GEN_R:
		if (!glState.textureGen[glState.texUnit][2])
			return;
		glState.textureGen[glState.texUnit][2] = false;

		break;
	case GL_TEXTURE_GEN_Q:
		if (!glState.textureGen[glState.texUnit][3])
			return;
		glState.textureGen[glState.texUnit][3] = false;

		break;
	}

	qglDisable(cap);
}

/*
 ==================
 GL_CullFace
 ==================
*/
void GL_CullFace (uint mode){

	if (glState.cullMode == mode)
		return;

	glState.cullMode = mode;

	qglCullFace(mode);
}

/*
 ==================
 GL_PolygonMode
 ==================
*/
void GL_PolygonMode (uint mode){

	if (glState.polygonMode == mode)
		return;

	glState.polygonMode = mode;

	qglPolygonMode(GL_FRONT_AND_BACK, mode);
}

/*
 ==================
 GL_PolygonOffset
 ==================
*/
void GL_PolygonOffset (float factor, float units){

	if (glState.polygonOffsetFactor == factor && glState.polygonOffsetUnits == units)
		return;

	glState.polygonOffsetFactor = factor;
	glState.polygonOffsetUnits = units;

	qglPolygonOffset(factor, units);
}

/*
 ==================
 GL_BlendFunc
 ==================
*/
void GL_BlendFunc (uint src, uint dst){

	if (glState.blendSrc == src && glState.blendDst == dst)
		return;

	glState.blendSrc = src;
	glState.blendDst = dst;

	qglBlendFunc(src, dst);
}

/*
 ==================
 GL_BlendEquation
 ==================
*/
void GL_BlendEquation (uint mode){

	if (glState.blendMode == mode)
		return;

	glState.blendMode = mode;

	qglBlendEquation(mode);
}

/*
 ==================
 GL_AlphaFunc
 ==================
*/
void GL_AlphaFunc (uint func, float ref){

	if (glState.alphaFunc == func && glState.alphaFuncRef == ref)
		return;

	glState.alphaFunc = func;
	glState.alphaFuncRef = ref;

	qglAlphaFunc(func, ref);
}

/*
 ==================
 GL_DepthFunc
 ==================
*/
void GL_DepthFunc (uint func){

	if (glState.depthFunc == func)
		return;

	glState.depthFunc = func;

	qglDepthFunc(func);
}

/*
 ==================
 GL_StencilFunc
 ==================
*/
void GL_StencilFunc (uint func, int ref, uint mask){

	if (glState.stencilFunc[0] == func && glState.stencilFunc[1] == func && glState.stencilFuncRef[0] == ref && glState.stencilFuncRef[1] == ref && glState.stencilFuncMask[0] == mask && glState.stencilFuncMask[1] == mask)
		return;

	glState.stencilFunc[0] = func;
	glState.stencilFunc[1] = func;
	glState.stencilFuncRef[0] = ref;
	glState.stencilFuncRef[1] = ref;
	glState.stencilFuncMask[0] = mask;
	glState.stencilFuncMask[1] = mask;

	qglStencilFunc(func, ref, mask);
}

/*
 ==================
 GL_StencilFuncSeparate
 ==================
*/
void GL_StencilFuncSeparate (uint funcFront, uint funcBack, int refFront, int refBack, uint maskFront, uint maskBack){

	if (glState.stencilFunc[0] == funcFront && glState.stencilFunc[1] == funcBack && glState.stencilFuncRef[0] == refFront && glState.stencilFuncRef[1] == refBack && glState.stencilFuncMask[0] == maskFront && glState.stencilFuncMask[1] == maskBack)
		return;

	glState.stencilFunc[0] = funcFront;
	glState.stencilFunc[1] = funcBack;
	glState.stencilFuncRef[0] = refFront;
	glState.stencilFuncRef[1] = refBack;
	glState.stencilFuncMask[0] = maskFront;
	glState.stencilFuncMask[1] = maskBack;

	qglStencilFuncSeparate(GL_FRONT, funcFront, refFront, maskFront);
	qglStencilFuncSeparate(GL_BACK, funcBack, refBack, maskBack);
}

/*
 ==================
 GL_StencilOp
 ==================
*/
void GL_StencilOp (uint fail, uint zFail, uint zPass){

	if (glState.stencilOpFail[0] == fail && glState.stencilOpFail[1] == fail && glState.stencilOpZFail[0] == zFail && glState.stencilOpZFail[1] == zFail && glState.stencilOpZPass[0] == zPass && glState.stencilOpZPass[1] == zPass)
		return;

	glState.stencilOpFail[0] = fail;
	glState.stencilOpFail[1] = fail;
	glState.stencilOpZFail[0] = zFail;
	glState.stencilOpZFail[1] = zFail;
	glState.stencilOpZPass[0] = zPass;
	glState.stencilOpZPass[1] = zPass;

	qglStencilOp(fail, zFail, zPass);
}

/*
 ==================
 GL_StencilOpSeparate
 ==================
*/
void GL_StencilOpSeparate (uint failFront, uint failBack, uint zFailFront, uint zFailBack, uint zPassFront, uint zPassBack){

	if (glState.stencilOpFail[0] == failFront && glState.stencilOpFail[1] == failBack && glState.stencilOpZFail[0] == zFailFront && glState.stencilOpZFail[1] == zFailBack && glState.stencilOpZPass[0] == zPassFront && glState.stencilOpZPass[1] == zPassBack)
		return;

	glState.stencilOpFail[0] = failFront;
	glState.stencilOpFail[1] = failBack;
	glState.stencilOpZFail[0] = zFailFront;
	glState.stencilOpZFail[1] = zFailBack;
	glState.stencilOpZPass[0] = zPassFront;
	glState.stencilOpZPass[1] = zPassBack;

	qglStencilOpSeparate(GL_FRONT, failFront, zFailFront, zPassFront);
	qglStencilOpSeparate(GL_BACK, failBack, zFailBack, zPassBack);
}

/*
 ==================
 GL_DepthRange
 ==================
*/
void GL_DepthRange (float min, float max){

	if (glState.depthMin == min && glState.depthMax == max)
		return;

	glState.depthMin = min;
	glState.depthMax = max;

	qglDepthRange(min, max);
}

/*
 ==================
 GL_ColorMask
 ==================
*/
void GL_ColorMask (bool red, bool green, bool blue, bool alpha){

	if (glState.colorMask[0] == red && glState.colorMask[1] == green && glState.colorMask[2] == blue && glState.colorMask[3] == alpha)
		return;

	glState.colorMask[0] = red;
	glState.colorMask[1] = green;
	glState.colorMask[2] = blue;
	glState.colorMask[3] = alpha;

	qglColorMask(red, green, blue, alpha);
}

/*
 ==================
 GL_DepthMask
 ==================
*/
void GL_DepthMask (bool mask){

	if (glState.depthMask == mask)
		return;

	glState.depthMask = mask;

	qglDepthMask(mask);
}

/*
 ==================
 GL_StencilMask
 ==================
*/
void GL_StencilMask (uint mask){

	if (glState.stencilMask[0] == mask && glState.stencilMask[1] == mask)
		return;

	glState.stencilMask[0] = mask;
	glState.stencilMask[1] = mask;

	qglStencilMask(mask);
}

/*
 ==================
 GL_StencilMaskSeparate
 ==================
*/
void GL_StencilMaskSeparate (uint maskFront, uint maskBack){

	if (glState.stencilMask[0] == maskFront && glState.stencilMask[1] == maskBack)
		return;

	glState.stencilMask[0] = maskFront;
	glState.stencilMask[1] = maskBack;

	qglStencilMaskSeparate(GL_FRONT, maskFront);
	qglStencilMaskSeparate(GL_BACK, maskBack);
}


/*
 ==============================================================================

 PROJECTION FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 GL_Setup3D

 TODO: time is messed up
 ==================
*/
void GL_Setup3D (int time){

	double	clipPlane[4];

	QGL_LogPrintf("---------- RB_Setup3D ----------\n");

	backEnd.projection2D = false;

	backEnd.time = time;
	backEnd.floatTime = MS2SEC(Sys_Milliseconds());

	backEnd.viewport.x = backEnd.viewParms.viewport.x;
	backEnd.viewport.y = backEnd.viewParms.viewport.y;
	backEnd.viewport.width = backEnd.viewParms.viewport.width;
	backEnd.viewport.height = backEnd.viewParms.viewport.height;

	backEnd.scissor.x = backEnd.viewParms.scissor.x;
	backEnd.scissor.y = backEnd.viewParms.scissor.y;
	backEnd.scissor.width = backEnd.viewParms.scissor.width;
	backEnd.scissor.height = backEnd.viewParms.scissor.height;

	backEnd.coordScale[0] = 1.0f / backEnd.viewport.width;
	backEnd.coordScale[1] = 1.0f / backEnd.viewport.height;

	backEnd.coordBias[0] = -backEnd.viewport.x * backEnd.coordScale[0];
	backEnd.coordBias[1] = -backEnd.viewport.y * backEnd.coordScale[1];

	backEnd.depthFilling = false;

	backEnd.currentColorCaptured = SORT_BAD;
	backEnd.currentDepthCaptured = false;

	// Set up the viewport
	GL_Viewport(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Set up the scissor
	GL_Scissor(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Set the projection matrix
	GL_LoadMatrix(GL_PROJECTION, backEnd.viewParms.projectionMatrix);

	// Set the modelview matrix
	GL_LoadIdentity(GL_MODELVIEW);

	// Set the GL state
	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_DepthRange(0.0f, 1.0f);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_TRUE);
	GL_StencilMask(255);

	// Enable the clip plane if needed
	if (backEnd.viewParms.viewType != VIEW_MIRROR)
		qglDisable(GL_CLIP_PLANE0);
	else {
		clipPlane[0] = -DotProduct(backEnd.viewParms.axis[1], backEnd.viewParms.clipPlane.normal);
		clipPlane[1] = DotProduct(backEnd.viewParms.axis[2], backEnd.viewParms.clipPlane.normal);
		clipPlane[2] = -DotProduct(backEnd.viewParms.axis[0], backEnd.viewParms.clipPlane.normal);
		clipPlane[3] = DotProduct(backEnd.viewParms.origin, backEnd.viewParms.clipPlane.normal) - backEnd.viewParms.clipPlane.dist;

		qglEnable(GL_CLIP_PLANE0);
		qglClipPlane(GL_CLIP_PLANE0, clipPlane);
	}

	// Enable multisampling if available
	if (glConfig.multiSamples > 1)
		qglEnable(GL_MULTISAMPLE);

	// Clear the buffers
	qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	qglClearDepth(1.0f);
	qglClearStencil(0);

	if (backEnd.viewParms.primaryView)
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	else
		qglClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	QGL_LogPrintf("--------------------\n");
}

/*
 ==================
 GL_Setup2D

 TODO: time is messed up
 ==================
*/
void GL_Setup2D (int time){

	mat4_t	projectionMatrix = {2.0f / backEnd.cropWidth, 0.0f, 0.0f, 0.0f, 0.0f, -2.0f / backEnd.cropHeight, 0.0f, 0.0f, 0.0f, 0.0f, -2.0f, 0.0f, -1.0f, 1.0f, -1.0f, 1.0f};

	QGL_LogPrintf("---------- RB_Setup2D ----------\n");

	backEnd.projection2D = true;

	backEnd.time = time;
	backEnd.floatTime = MS2SEC(Sys_Milliseconds());

	backEnd.viewport.x = 0;
	backEnd.viewport.y = 0;
	backEnd.viewport.width = backEnd.cropWidth;
	backEnd.viewport.height = backEnd.cropHeight;

	backEnd.scissor.x = 0;
	backEnd.scissor.y = 0;
	backEnd.scissor.width = backEnd.cropWidth;
	backEnd.scissor.height = backEnd.cropHeight;

	backEnd.coordScale[0] = 1.0f / backEnd.viewport.width;
	backEnd.coordScale[1] = 1.0f / backEnd.viewport.height;

	backEnd.coordBias[0] = -backEnd.viewport.x * backEnd.coordScale[0];
	backEnd.coordBias[1] = -backEnd.viewport.y * backEnd.coordScale[1];

	backEnd.depthFilling = false;

	backEnd.currentColorCaptured = SORT_BAD;
	backEnd.currentDepthCaptured = false;

	// Set up the viewport
	GL_Viewport(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Set up the scissor
	GL_Scissor(backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Set the projection matrix
	GL_LoadMatrix(GL_PROJECTION, projectionMatrix);

	// Set the modelview matrix
	GL_LoadIdentity(GL_MODELVIEW);

	// Set the GL state
	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);
	GL_Disable(GL_POLYGON_OFFSET_FILL);
	GL_Disable(GL_BLEND);
	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_DepthRange(0.0f, 1.0f);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Disable the clip plane
	qglDisable(GL_CLIP_PLANE0);

	// Disable multisampling if available
	if (glConfig.multiSamples > 1)
		qglDisable(GL_MULTISAMPLE);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	QGL_LogPrintf("--------------------\n");
}


/*
 ==============================================================================

 DEFAULT STATE

 ==============================================================================
*/


/*
 ==================
 GL_SetDefaultState
 ==================
*/
void GL_SetDefaultState (){

	int		i;

	QGL_LogPrintf("---------- GL_SetDefaultState ----------\n");

	// Reset the state manager
	glState.projectionMatrixIdentity = true;
	glState.modelviewMatrixIdentity = true;

	for (i = 0; i < MAX_TEXTURE_UNITS; i++)
		glState.textureMatrixIdentity[i] = true;

	for (i = 0; i < MAX_TEXTURE_UNITS; i++)
		glState.texture[i] = NULL;

	glState.program = NULL;
	glState.vertexBuffer = NULL;

	glState.viewportX = 0;
	glState.viewportY = 0;
	glState.viewportWidth = glConfig.videoWidth;
	glState.viewportHeight = glConfig.videoHeight;

	glState.scissorX = 0;
	glState.scissorY = 0;
	glState.scissorWidth = glConfig.videoWidth;
	glState.scissorHeight = glConfig.videoHeight;

	glState.texUnit = 0;

	for (i = 0; i < MAX_TEXTURE_UNITS; i++){
		glState.texTarget[i] = 0;

		glState.texEnv[i] = GL_MODULATE;

		glState.texGen[i][0] = GL_OBJECT_LINEAR;
		glState.texGen[i][1] = GL_OBJECT_LINEAR;
		glState.texGen[i][2] = GL_OBJECT_LINEAR;
		glState.texGen[i][3] = GL_OBJECT_LINEAR;
	}

	glState.cullFace = false;
	glState.polygonOffsetFill = false;
	glState.polygonOffsetLine = false;
	glState.blend = false;
	glState.alphaTest = false;
	glState.depthTest = false;
	glState.stencilTest = false;

	for (i = 0; i < MAX_TEXTURE_UNITS; i++){
		glState.textureGen[i][0] = false;
		glState.textureGen[i][1] = false;
		glState.textureGen[i][2] = false;
		glState.textureGen[i][3] = false;
	}

	glState.cullMode = GL_FRONT;
	glState.polygonMode = GL_FILL;
	glState.polygonOffsetFactor = 0.0f;
	glState.polygonOffsetUnits = 0.0f;
	glState.blendSrc = GL_ONE;
	glState.blendDst = GL_ZERO;
	glState.blendMode = GL_FUNC_ADD;
	glState.alphaFunc = GL_GREATER;
	glState.alphaFuncRef = 0.0f;
	glState.depthFunc = GL_LEQUAL;
	glState.stencilFunc[0] = GL_ALWAYS;
	glState.stencilFunc[1] = GL_ALWAYS;
	glState.stencilFuncRef[0] = 0;
	glState.stencilFuncRef[1] = 0;
	glState.stencilFuncMask[0] = 255;
	glState.stencilFuncMask[1] = 255;
	glState.stencilOpFail[0] = GL_KEEP;
	glState.stencilOpFail[1] = GL_KEEP;
	glState.stencilOpZFail[0] = GL_KEEP;
	glState.stencilOpZFail[1] = GL_KEEP;
	glState.stencilOpZPass[0] = GL_KEEP;
	glState.stencilOpZPass[1] = GL_KEEP;
	glState.depthMin = 0.0f;
	glState.depthMax = 1.0f;

	glState.colorMask[0] = GL_TRUE;
	glState.colorMask[1] = GL_TRUE;
	glState.colorMask[2] = GL_TRUE;
	glState.colorMask[3] = GL_TRUE;
	glState.depthMask = GL_TRUE;
	glState.stencilMask[0] = 255;
	glState.stencilMask[1] = 255;

	// Set default state
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	for (i = MAX_TEXTURE_UNITS - 1; i >= 0; i--){
		if (i >= glConfig.maxTextureImageUnits)
			continue;

		if (i >= glConfig.maxTextureUnits){
			qglActiveTexture(GL_TEXTURE0 + i);

			qglBindTexture(GL_TEXTURE_2D, 0);
			qglBindTexture(GL_TEXTURE_3D, 0);
			qglBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);

			continue;
		}

		qglActiveTexture(GL_TEXTURE0 + i);

		qglMatrixMode(GL_TEXTURE);
		qglLoadIdentity();

		qglDisable(GL_TEXTURE_2D);
		qglDisable(GL_TEXTURE_3D);
		qglDisable(GL_TEXTURE_CUBE_MAP);
		qglDisable(GL_TEXTURE_2D_ARRAY);

		qglBindTexture(GL_TEXTURE_2D, 0);
		qglBindTexture(GL_TEXTURE_3D, 0);
		qglBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		qglBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		qglDisable(GL_TEXTURE_GEN_S);
		qglDisable(GL_TEXTURE_GEN_T);
		qglDisable(GL_TEXTURE_GEN_R);
		qglDisable(GL_TEXTURE_GEN_Q);

		qglTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		qglTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		qglTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		qglTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	}

	qglDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	qglUseProgram(0);

	qglBindBuffer(GL_ARRAY_BUFFER, 0);

	qglViewport(0, 0, glConfig.videoWidth, glConfig.videoHeight);

	qglEnable(GL_SCISSOR_TEST);
	qglScissor(0, 0, glConfig.videoWidth, glConfig.videoHeight);

	qglFrontFace(GL_CCW);

	qglShadeModel(GL_SMOOTH);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	qglDisable(GL_CULL_FACE);
	qglCullFace(GL_FRONT);

	qglDisable(GL_POLYGON_OFFSET_FILL);
	qglDisable(GL_POLYGON_OFFSET_LINE);
	qglPolygonOffset(0.0f, 0.0f);

	qglDisable(GL_BLEND);
	qglBlendFunc(GL_ONE, GL_ZERO);
	qglBlendEquation(GL_FUNC_ADD);

	qglDisable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.0f);

	qglDisable(GL_DEPTH_TEST);
	qglDepthFunc(GL_LEQUAL);

	qglDisable(GL_STENCIL_TEST);
	qglStencilFunc(GL_ALWAYS, 0, 255);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	qglDepthRange(0.0f, 1.0f);

	qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	qglDepthMask(GL_TRUE);
	qglStencilMask(255);

	qglDisable(GL_DEPTH_CLAMP);

	qglDisable(GL_CLIP_PLANE0);

	if (glConfig.multiSamples > 1){
		qglDisable(GL_MULTISAMPLE);

		qglDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	}

	qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	qglClearDepth(1.0f);
	qglClearStencil(0);

	qglEnableClientState(GL_VERTEX_ARRAY);

	qglDisableVertexAttribArray(GL_ATTRIB_NORMAL);
	qglDisableVertexAttribArray(GL_ATTRIB_TANGENT1);
	qglDisableVertexAttribArray(GL_ATTRIB_TANGENT2);
	qglDisableVertexAttribArray(GL_ATTRIB_TEXCOORD);
	qglDisableVertexAttribArray(GL_ATTRIB_COLOR);

	QGL_LogPrintf("--------------------\n");
}

/*
 ==================
 GL_CheckForErrors
 ==================
*/
void GL_CheckForErrors (){

	const char	*string;
	int			error;

	error = qglGetError();

	if (error == GL_NO_ERROR)
		return;

	switch (error){
	case GL_INVALID_ENUM:
		string = "GL_INVALID_ENUM";
		break;
	case GL_INVALID_VALUE:
		string = "GL_INVALID_VALUE";
		break;
	case GL_INVALID_OPERATION:
		string = "GL_INVALID_OPERATION";
		break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		string = "GL_INVALID_FRAMEBUFFER_OPERATION";
		break;
	case GL_STACK_OVERFLOW:
		string = "GL_STACK_OVERFLOW";
		break;
	case GL_STACK_UNDERFLOW:
		string = "GL_STACK_UNDERFLOW";
		break;
	case GL_OUT_OF_MEMORY:
		string = "GL_OUT_OF_MEMORY";
		break;
	case GL_TABLE_TOO_LARGE:
		string = "GL_TABLE_TOO_LARGE";
		break;
	default:
		string = "UNKNOWN ERROR";
		break;
	}

	Com_Error(ERR_FATAL, "GL_CheckForErrors: %s", string);
}