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
// r_backEnd.c - Back-end renderer
//


#include "r_local.h"


backEnd_t					backEnd;


/*
 ==============================================================================

 TEXTURE TOOLS

 ==============================================================================
*/


/*
 ==================
 RB_TestTexture

 Displays a single texture over most of the screen
 ==================
*/
static void RB_TestTexture (){

	mat4_t	projectionMatrix = {2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f};
	float	w, h;
	int		max;

	if (!rg.testTexture)
		return;

	max = Max(rg.testTexture->width, rg.testTexture->height);

	w = 0.25f * rg.testTexture->width / max;
	h = 0.25f * rg.testTexture->height / max;

	w *= (float)backEnd.cropHeight / backEnd.cropWidth;

	// Set the GL state
	GL_LoadMatrix(GL_PROJECTION, projectionMatrix);
	GL_LoadIdentity(GL_MODELVIEW);

	GL_PolygonMode(GL_FILL);

	GL_Disable(GL_CULL_FACE);

	GL_Disable(GL_POLYGON_OFFSET_FILL);

	GL_Enable(GL_BLEND);
	GL_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_BlendEquation(GL_FUNC_ADD);

	GL_Disable(GL_ALPHA_TEST);
	GL_Disable(GL_DEPTH_TEST);
	GL_Disable(GL_STENCIL_TEST);

	GL_DepthRange(0.0f, 1.0f);

	GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GL_DepthMask(GL_FALSE);
	GL_StencilMask(0);

	// Set up the texture
	GL_EnableTexture(rg.testTexture->target);
	GL_BindTexture(rg.testTexture);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_REPLACE);

	// If a cube map texture
	if (rg.testTexture->type == TT_CUBE){
		w *= 0.333333f;
		h *= 0.333333f;

		// Draw it
		qglBegin(GL_QUADS);
		qglTexCoord3f(-1.0f,  1.0f, -1.0f);
		qglVertex2f(0.5f - w * 4.0f, h * 4.0f);
		qglTexCoord3f(-1.0f,  1.0f,  1.0f);
		qglVertex2f(0.5f - w * 2.0f, h * 4.0f);
		qglTexCoord3f(-1.0f, -1.0f,  1.0f);
		qglVertex2f(0.5f - w * 2.0f, h * 2.0f);
		qglTexCoord3f(-1.0f, -1.0f, -1.0f);
		qglVertex2f(0.5f - w * 4.0f, h * 2.0f);
		qglTexCoord3f(-1.0f,  1.0f,  1.0f);
		qglVertex2f(0.5f - w * 2.0f, h * 4.0f);
		qglTexCoord3f( 1.0f,  1.0f,  1.0f);
		qglVertex2f(0.5f, h * 4.0f);
		qglTexCoord3f( 1.0f, -1.0f,  1.0f);
		qglVertex2f(0.5f, h * 2.0f);
		qglTexCoord3f(-1.0f, -1.0f,  1.0f);
		qglVertex2f(0.5f - w * 2.0f, h * 2.0f);
		qglTexCoord3f( 1.0f,  1.0f,  1.0f);
		qglVertex2f(0.5f, h * 4.0f);
		qglTexCoord3f( 1.0f,  1.0f, -1.0f);
		qglVertex2f(0.5f + w * 2.0f, h * 4.0f);
		qglTexCoord3f( 1.0f, -1.0f, -1.0f);
		qglVertex2f(0.5f + w * 2.0f, h * 2.0f);
		qglTexCoord3f( 1.0f, -1.0f,  1.0f);
		qglVertex2f(0.5f, h * 2.0f);
		qglTexCoord3f( 1.0f,  1.0f, -1.0f);
		qglVertex2f(0.5f + w * 2.0f, h * 4.0f);
		qglTexCoord3f(-1.0f,  1.0f, -1.0f);
		qglVertex2f(0.5f + w * 4.0f, h * 4.0f);
		qglTexCoord3f(-1.0f, -1.0f, -1.0f);
		qglVertex2f(0.5f + w * 4.0f, h * 2.0f);
		qglTexCoord3f( 1.0f, -1.0f, -1.0f);
		qglVertex2f(0.5f + w * 2.0f, h * 2.0f);
		qglTexCoord3f(-1.0f,  1.0f, -1.0f);
		qglVertex2f(0.5f - w * 2.0f, h * 6.0f);
		qglTexCoord3f( 1.0f,  1.0f, -1.0f);
		qglVertex2f(0.5f, h * 6.0f);
		qglTexCoord3f( 1.0f,  1.0f,  1.0f);
		qglVertex2f(0.5f, h * 4.0f);
		qglTexCoord3f(-1.0f,  1.0f,  1.0f);
		qglVertex2f(0.5f - w * 2.0f, h * 4.0f);
		qglTexCoord3f(-1.0f, -1.0f,  1.0f);
		qglVertex2f(0.5f - w * 2.0f, h * 2.0f);
		qglTexCoord3f( 1.0f, -1.0f,  1.0f);
		qglVertex2f(0.5f, h * 2.0f);
		qglTexCoord3f( 1.0f, -1.0f, -1.0f);
		qglVertex2f(0.5f, 0.0f);
		qglTexCoord3f(-1.0f, -1.0f, -1.0f);
		qglVertex2f(0.5f - w * 2.0f, 0.0f);
		qglEnd();

		// Check for errors
		if (!r_ignoreGLErrors->integerValue)
			GL_CheckForErrors();

		return;
	}

	// Draw it
	qglBegin(GL_QUADS);
	qglTexCoord2f(0.0f, 0.0f);
	qglVertex2f(0.5f - w, h * 2.0f);
	qglTexCoord2f(1.0f, 0.0f);
	qglVertex2f(0.5f + w, h * 2.0f);
	qglTexCoord2f(1.0f, 1.0f);
	qglVertex2f(0.5f + w, 0.0f);
	qglTexCoord2f(0.0f, 1.0f);
	qglVertex2f(0.5f - w, 0.0f);
	qglEnd();

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();
}

/*
 ==================
 RB_ShowTextures

 Draws all the textures to the screen, on top of whatever was there.
 This is used to test for texture trashing.
 ==================
*/
static void RB_ShowTextures (){

	texture_t	*texture;
	float		x, y, w, h;
	int			time;
	int			i;

	if (!r_showTextures->integerValue)
		return;

	// Switch to 2D mode
	GL_Setup2D(rg.time);

	qglFinish();

	time = Sys_Milliseconds();

	// Draw all textures
	for (i = 0; i < MAX_TEXTURES; i++){
		texture = R_GetTextureByIndex(i);
		if (!texture)
			break;

		w = backEnd.cropWidth / 20;
		h = backEnd.cropHeight / 15;

		x = i % 20 * w;
		y = i / 20 * h;

		// Draw in proportional size if desired
		if (r_showTextures->integerValue == 2){
			w *= texture->width / 512.0f;
			h *= texture->height / 512.0f;
		}

		// Set up the texture
		GL_EnableTexture(texture->target);
		GL_BindTexture(texture);

		GL_LoadIdentity(GL_TEXTURE);
		GL_TexEnv(GL_REPLACE);

		// Draw it
		qglBegin(GL_QUADS);
		qglTexCoord2f(0.0f, 0.0f);
		qglVertex2f(x, y);
		qglTexCoord2f(1.0f, 0.0f);
		qglVertex2f(x + w, y);
		qglTexCoord2f(1.0f, 1.0f);
		qglVertex2f(x + w, y + h);
		qglTexCoord2f(0.0f, 1.0f);
		qglVertex2f(x, y + h);
		qglEnd();
	}

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	// Force a mode switch
	backEnd.projection2D = false;

	qglFinish();

	Com_Printf("%i msec to draw all textures\n", Sys_Milliseconds() - time);
}


/*
 ==============================================================================

 RENDER COMMANDS EXECUTION

 ==============================================================================
*/


/*
 ==================
 RB_RenderView
 ==================
*/
static const void *RB_RenderView (const void *data){

	const renderViewCommand_t	*cmd = (const renderViewCommand_t *)data;

	// Development tool
	if (r_skipRender->integerValue)
		return (const void *)(cmd + 1);

	// Finish 2D rendering if needed
	if (backEnd.projection2D)
		RB_RenderBatch();

	QGL_LogPrintf("---------- RB_RenderView ----------\n");

	rg.pc.views++;

	backEnd.viewParms = cmd->viewParms;

	// Development tool
	if (r_skipRenderContext->integerValue)
		GLImp_ActivateContext(false);

	// Switch to 3D projection
	GL_Setup3D(cmd->time);

	// Z-Fill pass
	RB_FillDepthBuffer(cmd->viewParms.numMeshes[0], cmd->viewParms.meshes[0]);

	// Shadow pass
	RB_RenderShadows(cmd->viewParms.numMeshes[0], cmd->viewParms.meshes[0]);

	// Ambient pass (opaque)
	RB_RenderMaterialPasses(cmd->viewParms.numMeshes[0], cmd->viewParms.meshes[0], AP_OPAQUE);

	// Ambient pass (translucent)
	RB_RenderMaterialPasses(cmd->viewParms.numMeshes[1], cmd->viewParms.meshes[1], AP_TRANSLUCENT);

	// Ambient pass (translucent)
	RB_RenderMaterialPasses(cmd->viewParms.numMeshes[2], cmd->viewParms.meshes[2], AP_TRANSLUCENT);

	// Ambient pass (post-process)
	RB_RenderMaterialPasses(cmd->viewParms.numMeshes[3], cmd->viewParms.meshes[3], AP_POST_PROCESS);

	// Debug tools visualization
	RB_RenderDebugTools();

	// Development tool
	if (r_skipRenderContext->integerValue){
		GLImp_ActivateContext(true);

		GL_SetDefaultState();
	}

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	QGL_LogPrintf("--------------------\n");

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_CaptureRender
 ==================
*/
static const void *RB_CaptureRender (const void *data){

	const captureRenderCommand_t	*cmd = (const captureRenderCommand_t *)data;

	// Finish 2D rendering if needed
	if (backEnd.projection2D)
		RB_RenderBatch();

	QGL_LogPrintf("---------- RB_CaptureRender ----------\n");

	// Update the texture
	R_CopyFramebufferToTexture(cmd->texture, 0, backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	QGL_LogPrintf("--------------------\n");

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_UpdateTexture
 ==================
*/
static const void *RB_UpdateTexture (const void *data){

	const updateTextureCommand_t	*cmd = (const updateTextureCommand_t *)data;

	// Finish 2D rendering if needed
	if (backEnd.projection2D)
		RB_RenderBatch();

	QGL_LogPrintf("---------- RB_UpdateTexture ----------\n");

	// Update the texture
	R_UploadTextureImage(cmd->texture, 0, cmd->image, cmd->width, cmd->height);

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	QGL_LogPrintf("--------------------\n");

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_SetColor
 ==================
*/
static const void *RB_SetColor (const void *data){

	const setColorCommand_t	*cmd = (const setColorCommand_t *)data;

	MakeRGBA(backEnd.color2D, cmd->color[0], cmd->color[1], cmd->color[2], cmd->color[3]);

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_SetParameter
 ==================
*/
static const void *RB_SetParameter (const void *data){

	const setParameterCommand_t		*cmd = (const setParameterCommand_t *)data;

	backEnd.parms2D[cmd->index] = cmd->value;

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_SetParameters
 ==================
*/
static const void *RB_SetParameters (const void *data){

	const setParametersCommand_t	*cmd = (const setParametersCommand_t *)data;

	Mem_Copy(backEnd.parms2D, cmd->parms, MAX_MATERIAL_PARMS * sizeof(float));

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_DrawStretchPic
 ==================
*/
static const void *RB_DrawStretchPic (const void *data){

	const drawPicStretchCommand_t	*cmd = (const drawPicStretchCommand_t *)data;
	glIndex_t						*indices;
	glVertex_t						*vertices;
	int								i;

	// Switch to 2D mode if needed
	if (!backEnd.projection2D){
		GL_Setup2D(rg.time);

		// Clear the batch state
		backEnd.entity = NULL;
		backEnd.material = NULL;
	}

	// Check if the state changed
	if (cmd->material != backEnd.material){
		// Draw the last batch
		RB_RenderBatch();

		// Evaluate registers
		RB_EvaluateRegisters(cmd->material, backEnd.floatTime, backEnd.parms2D);

		// Create a new batch
		RB_SetupBatch(NULL, cmd->material, RB_DrawMaterial2D);
	}

	// Skip if condition evaluated to false
	if (!cmd->material->expressionRegisters[cmd->material->conditionRegister])
		return (const void *)(cmd + 1);

	// Check for overflow
	RB_CheckMeshOverflow(6, 4);

	// Batch indices
	indices = backEnd.indices + backEnd.numIndices;

	for (i = 2; i < 4; i++){
		indices[0] = backEnd.numVertices + 0;
		indices[1] = backEnd.numVertices + i-1;
		indices[2] = backEnd.numVertices + i;

		indices += 3;
	}

	backEnd.numIndices += 6;

	// Batch vertices
	vertices = backEnd.vertices + backEnd.numVertices;

	vertices[0].xyz[0] = cmd->x1;
	vertices[0].xyz[1] = cmd->y1;
	vertices[0].xyz[2] = 0.0f;
	vertices[1].xyz[0] = cmd->x2;
	vertices[1].xyz[1] = cmd->y2;
	vertices[1].xyz[2] = 0.0f;
	vertices[2].xyz[0] = cmd->x3;
	vertices[2].xyz[1] = cmd->y3;
	vertices[2].xyz[2] = 0.0f;
	vertices[3].xyz[0] = cmd->x4;
	vertices[3].xyz[1] = cmd->y4;
	vertices[3].xyz[2] = 0.0f;

	vertices[0].st[0] = cmd->s1;
	vertices[0].st[1] = cmd->t1;
	vertices[1].st[0] = cmd->s2;
	vertices[1].st[1] = cmd->t1;
	vertices[2].st[0] = cmd->s2;
	vertices[2].st[1] = cmd->t2;
	vertices[3].st[0] = cmd->s1;
	vertices[3].st[1] = cmd->t2;

	for (i = 0; i < 4; i++){
		vertices->color[0] = backEnd.color2D[0];
		vertices->color[1] = backEnd.color2D[1];
		vertices->color[2] = backEnd.color2D[2];
		vertices->color[3] = backEnd.color2D[3];

		vertices++;
	}

	backEnd.numVertices += 4;

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_DrawCinematic
 ==================
*/
static const void *RB_DrawCinematic (const void *data){

	const drawCinematicCommand_t	*cmd = (const drawCinematicCommand_t *)data;

	// Finish 2D rendering if needed
	if (backEnd.projection2D)
		RB_RenderBatch();

	// Switch to 2D mode if needed
	if (!backEnd.projection2D){
		GL_Setup2D(rg.time);

		// Clear the batch state
		backEnd.entity = NULL;
		backEnd.material = NULL;
	}

	QGL_LogPrintf("---------- RB_DrawCinematic ----------\n");

	// TODO: vertex buffer stuff?

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

	// Draw vertical black bars if needed
	if (cmd->vx1 != cmd->wx1 || cmd->vx2 != cmd->wx2){
		GL_DisableTexture();

		qglColor3f(0.0f, 0.0f, 0.0f);

		qglBegin(GL_QUADS);
		qglVertex2i(cmd->vx1, cmd->wy1);
		qglVertex2i(cmd->vx1, cmd->wy2);
		qglVertex2i(cmd->wx1, cmd->wy2);
		qglVertex2i(cmd->wx1, cmd->wy1);
		qglVertex2i(cmd->wx2, cmd->wy1);
		qglVertex2i(cmd->wx2, cmd->wy2);
		qglVertex2i(cmd->vx2, cmd->wy2);
		qglVertex2i(cmd->vx2, cmd->wy1);
		qglEnd();
	}

	// Draw horizontal black bars if needed
	if (cmd->vy1 != cmd->wy1 || cmd->vy2 != cmd->wy2){
		GL_DisableTexture();

		qglColor3f(0.0f, 0.0f, 0.0f);

		qglBegin(GL_QUADS);
		qglVertex2i(cmd->wx1, cmd->wy1);
		qglVertex2i(cmd->wx2, cmd->wy1);
		qglVertex2i(cmd->wx2, cmd->vy1);
		qglVertex2i(cmd->wx1, cmd->vy1);
		qglVertex2i(cmd->wx1, cmd->vy2);
		qglVertex2i(cmd->wx2, cmd->vy2);
		qglVertex2i(cmd->wx2, cmd->wy2);
		qglVertex2i(cmd->wx1, cmd->wy2);
		qglEnd();
	}

	// Set up the texture
	GL_EnableTexture(cmd->texture->target);
	GL_BindTexture(cmd->texture);

	GL_LoadIdentity(GL_TEXTURE);
	GL_TexEnv(GL_REPLACE);

	// Update the texture if needed
	if (cmd->dirty)
		R_UploadTextureImage(cmd->texture, 0, cmd->image, cmd->width, cmd->height);

	// Draw it
	qglBegin(GL_QUADS);
	qglTexCoord2f(0.0f, 0.0f);
	qglVertex2i(cmd->vx1, cmd->vy1);
	qglTexCoord2f(1.0f, 0.0f);
	qglVertex2i(cmd->vx2, cmd->vy1);
	qglTexCoord2f(1.0f, 1.0f);
	qglVertex2i(cmd->vx2, cmd->vy2);
	qglTexCoord2f(0.0f, 1.0f);
	qglVertex2i(cmd->vx1, cmd->vy2);
	qglEnd();

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	QGL_LogPrintf("--------------------\n");

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_CropSize
 ==================
*/
static const void *RB_CropSize (const void *data){

	const cropSizeCommand_t	*cmd = (const cropSizeCommand_t *)data;

	// Finish 2D rendering if needed and force a mode switch
	if (backEnd.projection2D){
		RB_RenderBatch();

		backEnd.projection2D = false;
	}

	// Set the current crop size
	backEnd.cropWidth = cmd->width;
	backEnd.cropHeight = cmd->height;

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_SetupBuffers
 ==================
*/
static const void *RB_SetupBuffers (const void *data){

	const setupBuffersCommand_t	*cmd = (const setupBuffersCommand_t *)data;
	int							index;

	// Finish 2D rendering if needed and force a mode switch
	if (backEnd.projection2D){
		RB_RenderBatch();

		backEnd.projection2D = false;
	}

	QGL_LogPrintf("---------- RB_SetupBuffers ----------\n");

	// Set the draw and read buffers
	if (r_frontBuffer->integerValue){
		qglDrawBuffer(GL_FRONT);
		qglReadBuffer(GL_FRONT);
	}
	else {
		qglDrawBuffer(GL_BACK);
		qglReadBuffer(GL_BACK);
	}

	// Clear the color buffer if desired
	if (r_clear->integerValue){
		index = r_clearColor->integerValue & COLOR_MASK;

		GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		qglClearColor(color_table[index][0], color_table[index][1], color_table[index][2], color_table[index][3]);
		qglClear(GL_COLOR_BUFFER_BIT);
	}

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	QGL_LogPrintf("--------------------\n");

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_SwapBuffers
 ==================
*/
static const void *RB_SwapBuffers (const void *data){

	const swapBuffersCommand_t	*cmd = (const swapBuffersCommand_t *)data;

	// Finish 2D rendering if needed and force a mode switch
	if (backEnd.projection2D){
		RB_RenderBatch();

		backEnd.projection2D = false;
	}

	QGL_LogPrintf("---------- RB_SwapBuffers ----------\n");

	// Development tools
	RB_TestTexture();

	RB_ShowTextures();

	// Swap the buffers
	if (r_frontBuffer->integerValue || r_finish->integerValue)
		qglFinish();

	if (!r_frontBuffer->integerValue)
		GLImp_SwapBuffers();

	// Check for errors
	if (!r_ignoreGLErrors->integerValue)
		GL_CheckForErrors();

	QGL_LogPrintf("--------------------\n\n\n");

	return (const void *)(cmd + 1);
}

/*
 ==================
 RB_ExecuteRenderCommands
 ==================
*/
void RB_ExecuteRenderCommands (const void *data){

	renderCommand_t	commandId;
	int				timeBackEnd;

	if (r_skipBackEnd->integerValue)
		return;

	if (com_speeds->integerValue)
		timeBackEnd = Sys_Milliseconds();

	while (1){
		commandId = *(const renderCommand_t *)data;

		if (commandId == RC_END_OF_LIST){
			if (com_speeds->integerValue)
				com_timeBackEnd += (Sys_Milliseconds() - timeBackEnd);

			return;
		}

		switch (commandId){
		case RC_RENDER_VIEW:
			data = RB_RenderView(data);
			break;
		case RC_CAPTURE_RENDER:
			data = RB_CaptureRender(data);
			break;
		case RC_UPDATE_TEXTURE:
			data = RB_UpdateTexture(data);
			break;
		case RC_SET_COLOR:
			data = RB_SetColor(data);
			break;
		case RC_SET_PARAMETER:
			data = RB_SetParameter(data);
			break;
		case RC_SET_PARAMETERS:
			data = RB_SetParameters(data);
			break;
		case RC_DRAW_STRETCH_PIC:
			data = RB_DrawStretchPic(data);
			break;
		case RC_DRAW_CINEMATIC:
			data = RB_DrawCinematic(data);
			break;
		case RC_CROP_SIZE:
			data = RB_CropSize(data);
			break;
		case RC_SETUP_BUFFERS:
			data = RB_SetupBuffers(data);
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers(data);
			break;
		default:
			Com_Error(ERR_DROP, "RB_ExecuteRenderCommands: bad command id (%i)", commandId);
		}
	}
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 RB_InitBackEnd

 TODO: i do not like to alloc a list of memory that is not filled in because it can cause problems,
 find a way to clear the alloc and fix the "y u no draw" error
 ==================
*/
void RB_InitBackEnd (){

	// Allocate the render command buffer
	backEnd.commandBuffer.data = (byte *)Mem_Alloc(MAX_COMMAND_BUFFER_SIZE, TAG_RENDERER);

	// Allocate index and vertex arrays
	backEnd.indices = (glIndex_t *)Mem_Alloc(MAX_INDICES * sizeof(glIndex_t), TAG_RENDERER);
	backEnd.vertices = (glVertex_t *)Mem_Alloc16(MAX_VERTICES * sizeof(glVertex_t), TAG_RENDERER);

	// Allocate dynamic vertex buffer
	backEnd.vertexBuffer = R_AllocVertexBuffer("streamBuffer1", true, MAX_DYNAMIC_VERTICES * sizeof(arrayBuffer_t));
}

/*
 ==================
 RB_ShutdownBackEnd
 ==================
*/
void RB_ShutdownBackEnd (){

	// Clear the back-end structure
	Mem_Fill(&backEnd, 0, sizeof(backEnd_t));
}