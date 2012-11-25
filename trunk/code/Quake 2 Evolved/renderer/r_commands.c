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
// r_commands.c - Rendering commands execution
//


#include "r_local.h"


/*
 ==================
 
 ==================
*/
static void R_PerformanceCounters (){

	if (r_showCull->integerValue)
		Com_Printf("in: %i (b: %i, s: %i), out %i (b: %i, s: %i)", rg.pc.cullBoundsIn + rg.pc.cullSphereIn, rg.pc.cullBoundsIn, rg.pc.cullSphereIn, rg.pc.cullBoundsOut + rg.pc.cullSphereOut, rg.pc.cullBoundsOut, rg.pc.cullSphereOut);

	if (r_showScene->integerValue)
		Com_Printf("entities: %i, lights: %i, particles: %i, decals: %i\n", rg.pc.entities, rg.pc.lights, rg.pc.particles, rg.pc.decals);

	if (r_showDeforms->integerValue)
		Com_Printf("tris: %i verts: %i (expand: %i, move: %i, sprite: %i, tube: %i, beam: %i)\n", rg.pc.deformIndices / 3, rg.pc.deformVertices, rg.pc.deformExpand, rg.pc.deformMove, rg.pc.deformSprite, rg.pc.deformTube, rg.pc.deformBeam);

	if (r_showTextureUsage->integerValue)
		Com_Printf("textures: %i = %.2f MB\n", rg.pc.textures, rg.pc.textureBytes * (1.0f / 1048576.0f));

	// Clear for next frame
	Mem_Fill(&rg.pc, 0, sizeof(performanceCounters_t));
}

/*
 ==================
 R_GetCommandBuffer
 ==================
*/
static void *R_GetCommandBuffer (int size){

	commandBuffer_t	*commandBuffer = &backEnd.commandBuffer;

	// Always leave room for the end of list command
	if (commandBuffer->size + size + sizeof(renderCommand_t) > MAX_COMMAND_BUFFER_SIZE)
		Com_Error(ERR_DROP, "R_GetCommandBuffer: overflow");

	commandBuffer->size += size;

	return commandBuffer->data + commandBuffer->size - size;
}

/*
 ==================
 R_IssueRenderCommands
 ==================
*/
static void R_IssueRenderCommands (){

	commandBuffer_t	*commandBuffer = &backEnd.commandBuffer;

	if (!commandBuffer->size)
		return;

	// Add an end of list command
	*(renderCommand_t *)(commandBuffer->data + commandBuffer->size) = RC_END_OF_LIST;

	// Clear it out
	commandBuffer->size = 0;

	// Execute the commands
	RB_ExecuteRenderCommands(commandBuffer->data);

	// Look at the performance counters
	R_PerformanceCounters();
}


/*
 ==============================================================================

 RENDER COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_AddRenderViewCommand
 ==================
*/
void R_AddRenderViewCommand (){

	renderViewCommand_t		*cmd;

	// Add a render view command
	cmd = (renderViewCommand_t *)R_GetCommandBuffer(sizeof(renderViewCommand_t));

	cmd->commandId = RC_RENDER_VIEW;

	cmd->time = rg.renderView.time;

	// View parms
	cmd->viewParms.primaryView = rg.viewParms.primaryView;
	cmd->viewParms.viewType = rg.viewParms.viewType;

	cmd->viewParms.viewport = rg.viewParms.viewport;
	cmd->viewParms.scissor = rg.viewParms.scissor;

	VectorCopy(rg.renderView.origin, cmd->viewParms.origin);
	Matrix3_Copy(rg.renderView.axis, cmd->viewParms.axis);

	Matrix4_Copy(rg.viewParms.projectionMatrix, cmd->viewParms.projectionMatrix);
	Matrix4_Copy(rg.viewParms.modelviewMatrix, cmd->viewParms.modelviewMatrix);
	Matrix4_Copy(rg.viewParms.modelviewProjectionMatrix, cmd->viewParms.modelviewProjectionMatrix);
	Matrix4_Copy(rg.viewParms.skyBoxMatrix, cmd->viewParms.skyBoxMatrix);

	cmd->viewParms.numMeshes[0] = rg.viewParms.numMeshes[0];
	cmd->viewParms.numMeshes[1] = rg.viewParms.numMeshes[1];
	cmd->viewParms.numMeshes[2] = rg.viewParms.numMeshes[2];
	cmd->viewParms.numMeshes[3] = rg.viewParms.numMeshes[3];

	cmd->viewParms.meshes[0] = rg.viewParms.meshes[0];
	cmd->viewParms.meshes[1] = rg.viewParms.meshes[1];
	cmd->viewParms.meshes[2] = rg.viewParms.meshes[2];
	cmd->viewParms.meshes[3] = rg.viewParms.meshes[3];
}

/*
 ==================
 R_CaptureRenderToTexture
 ==================
*/
bool R_CaptureRenderToTexture (const char *name){

	captureRenderCommand_t	*cmd;
	texture_t				*texture;

	// Get the texture
	texture = R_GetTexture(name);
	if (!texture)
		return false;

	if (texture->type != TT_2D || !(texture->flags & TF_ALLOWCAPTURE))
		return false;

	// Add a capture render command
	cmd = (captureRenderCommand_t *)R_GetCommandBuffer(sizeof(captureRenderCommand_t));

	cmd->commandId = RC_CAPTURE_RENDER;

	cmd->texture = texture;

	return true;
}

/*
 ==================
 R_UpdateTextureImage
 ==================
*/
bool R_UpdateTextureImage (const char *name, const byte *image, int width, int height){

	updateTextureCommand_t	*cmd;
	texture_t				*texture;

	// Get the texture
	texture = R_GetTexture(name);
	if (!texture)
		return false;

	if (texture->type != TT_2D || !(texture->flags & TF_ALLOWUPDATE))
		return false;

	// Add an update texture command
	cmd = (updateTextureCommand_t *)R_GetCommandBuffer(sizeof(updateTextureCommand_t));

	cmd->commandId = RC_UPDATE_TEXTURE;

	cmd->texture = texture;

	cmd->image = image;
	cmd->width = width;
	cmd->height = height;

	return true;
}

/*
 ==================
 R_SetColor
 ==================
*/
void R_SetColor (const vec4_t rgba){

	setColorCommand_t	*cmd;

	// Add a set color command
	cmd = (setColorCommand_t *)R_GetCommandBuffer(sizeof(setColorCommand_t));

	cmd->commandId = RC_SET_COLOR;

	ColorPackVectorRGBA(cmd->color, rgba);
}

/*
 ==================
 R_SetColor1
 ==================
*/
void R_SetColor1 (float l){

	setColorCommand_t	*cmd;

	// Add a set color command
	cmd = (setColorCommand_t *)R_GetCommandBuffer(sizeof(setColorCommand_t));

	cmd->commandId = RC_SET_COLOR;

	ColorPackL(cmd->color, l);
}

/*
 ==================
 R_SetColor2
 ==================
*/
void R_SetColor2 (float l, float a){

	setColorCommand_t	*cmd;

	// Add a set color command
	cmd = (setColorCommand_t *)R_GetCommandBuffer(sizeof(setColorCommand_t));

	cmd->commandId = RC_SET_COLOR;

	ColorPackLA(cmd->color, l, a);
}

/*
 ==================
 R_SetColor3
 ==================
*/
void R_SetColor3 (float r, float g, float b){

	setColorCommand_t	*cmd;

	// Add a set color command
	cmd = (setColorCommand_t *)R_GetCommandBuffer(sizeof(setColorCommand_t));

	cmd->commandId = RC_SET_COLOR;

	ColorPackFloatRGB(cmd->color, r, g, b);
}

/*
 ==================
 R_SetColor4
 ==================
*/
void R_SetColor4 (float r, float g, float b, float a){

	setColorCommand_t	*cmd;
	byte				*color[4];

	// Add a set color command
	cmd = (setColorCommand_t *)R_GetCommandBuffer(sizeof(setColorCommand_t));

	cmd->commandId = RC_SET_COLOR;

	ColorPackFloatRGBA(cmd->color, r, g, b, a);
}

/*
 ==================
 R_SetParameter
 ==================
*/
void R_SetParameter (int index, float value){

	setParameterCommand_t	*cmd;

	if (index < 0 || index >= MAX_MATERIAL_PARMS)
		Com_Error(ERR_DROP, "R_SetParameter: index out of range");

	// Add a set parameter command
	cmd = (setParameterCommand_t *)R_GetCommandBuffer(sizeof(setParameterCommand_t));

	cmd->commandId = RC_SET_PARAMETER;

	cmd->index = index;
	cmd->value = value;
}

/*
 ==================
 R_SetParameters
 ==================
*/
void R_SetParameters (const float parms[MAX_MATERIAL_PARMS]){

	setParametersCommand_t	*cmd;

	// Add a set parameters command
	cmd = (setParametersCommand_t *)R_GetCommandBuffer(sizeof(setParametersCommand_t));

	cmd->commandId = RC_SET_PARAMETERS;

	Mem_Copy(cmd->parms, parms, MAX_MATERIAL_PARMS * sizeof(float));
}

/*
 ==================
 R_DrawStretchPic
 ==================
*/
void R_DrawStretchPic (float x, float y, float w, float h, float s1, float t1, float s2, float t2, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, material_t *material){

	drawPicStretchCommand_t	*cmd;
	float					x1, y1, x2, y2;

	// Scale the coordinates and correct the aspect ratio if needed
	R_AdjustHorzCoords(horzAdjust, horzPercent, x, w, &x, &w);
	R_AdjustVertCoords(vertAdjust, vertPercent, y, h, &y, &h);

	// Set up the coordinates
	x1 = x;
	y1 = y;
	x2 = x + w;
	y2 = y + h;

	// Add a draw stretch pic command
	cmd = (drawPicStretchCommand_t *)R_GetCommandBuffer(sizeof(drawPicStretchCommand_t));

	cmd->commandId = RC_DRAW_STRETCH_PIC;

	cmd->material = material;

	cmd->x1 = x1;
	cmd->y1 = y1;
	cmd->x2 = x2;
	cmd->y2 = y1;
	cmd->x3 = x2;
	cmd->y3 = y2;
	cmd->x4 = x1;
	cmd->y4 = y2;

	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

/*
 ==================
 R_DrawStretchPicEx
 ==================
*/
void R_DrawStretchPicEx (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float xShear, float yShear, float rotate, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, material_t *material){

	drawPicStretchCommand_t	*cmd;
	float					x1, y1, x2, y2;
	float					matrix[2][3];
	float					xCenter, yCenter;
	float					xRotate, yRotate;
	float					s, c;

	// Scale the coordinates and correct the aspect ratio if needed
	R_AdjustHorzCoords(horzAdjust, horzPercent, x, w, &x, &w);
	R_AdjustVertCoords(vertAdjust, vertPercent, y, h, &y, &h);

	// Set up the coordinates
	x1 = x;
	y1 = y;
	x2 = x + w;
	y2 = y + h;

	// Compute the transformation matrix
	SinCos(DEG2RAD(-rotate), s, c);

	xCenter = x + (w * 0.5f);
	yCenter = y + (h * 0.5f);

	xRotate = xCenter - xCenter * c + yCenter * s;
	yRotate = yCenter - yCenter * c - xCenter * s;

	matrix[0][0] = c + xShear * s;
	matrix[0][1] = -s + xShear * c;
	matrix[0][2] = xRotate + xShear * yRotate - xShear * yCenter;
	matrix[1][0] = c * yShear + s;
	matrix[1][1] = -s * yShear + c;
	matrix[1][2] = yRotate + yShear * xRotate - yShear * xCenter;

	// Add a draw stretch pic command
	cmd = (drawPicStretchCommand_t *)R_GetCommandBuffer(sizeof(drawPicStretchCommand_t));

	cmd->commandId = RC_DRAW_STRETCH_PIC;

	cmd->material = material;

	cmd->x1 = x1 * matrix[0][0] + y1 * matrix[0][1] + matrix[0][2];
	cmd->y1 = x1 * matrix[1][0] + y1 * matrix[1][1] + matrix[1][2];
	cmd->x2 = x2 * matrix[0][0] + y1 * matrix[0][1] + matrix[0][2];
	cmd->y2 = x2 * matrix[1][0] + y1 * matrix[1][1] + matrix[1][2];
	cmd->x3 = x2 * matrix[0][0] + y2 * matrix[0][1] + matrix[0][2];
	cmd->y3 = x2 * matrix[1][0] + y2 * matrix[1][1] + matrix[1][2];
	cmd->x4 = x1 * matrix[0][0] + y2 * matrix[0][1] + matrix[0][2];
	cmd->y4 = x1 * matrix[1][0] + y2 * matrix[1][1] + matrix[1][2];

	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

/*
 ==================
 R_DrawChar
 ==================
*/
void R_DrawChar (float x, float y, float w, float h, int c, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, material_t *material){

	float	col, row;

	c &= 255;

	if (c == ' ')
		return;

	col = (c & 15) * 0.0625f;
	row = (c >> 4) * 0.0625f;

	R_DrawStretchPic(x, y, w, h, col, row, col + 0.0625f, row + 0.0625f, horzAdjust, horzPercent, vertAdjust, vertPercent, material);
}

/*
 ==================
 R_DrawString
 ==================
*/
void R_DrawString (float x, float y, float w, float h, const char *string, const vec4_t color, bool forceColor, float xShadow, float yShadow, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, material_t *material){

	const char	*str;
	float		col, row;
	float		ofs;
	int			index;
	int			c;

	// Draw the shadow if desired
	if (xShadow || yShadow){
		str = string;
		ofs = 0.0f;

		R_SetColor2(0.0f, color[3]);

		while (*str){
			if (Str_IsColor(str)){
				str += 2;
				continue;
			}

			c = *(const byte *)str++;

			if (c != ' '){
				col = (c & 15) * 0.0625f;
				row = (c >> 4) * 0.0625f;

				R_DrawStretchPic(x + xShadow + ofs, y + yShadow, w, h, col, row, col + 0.0625f, row + 0.0625f, horzAdjust, horzPercent, vertAdjust, vertPercent, material);
			}

			ofs += w;
		}
	}

	// Draw the string
	str = string;
	ofs = 0.0f;

	R_SetColor(color);

	while (*str){
		if (Str_IsColor(str)){
			if (!forceColor){
				index = Str_ColorIndexForChar(str[1]);

				R_SetColor4(color_table[index][0], color_table[index][1], color_table[index][2], color[3]);
			}

			str += 2;
			continue;
		}

		c = *(const byte *)str++;

		if (c != ' '){
			col = (c & 15) * 0.0625f;
			row = (c >> 4) * 0.0625f;

			R_DrawStretchPic(x + ofs, y, w, h, col, row, col + 0.0625f, row + 0.0625f, horzAdjust, horzPercent, vertAdjust, vertPercent, material);
		}

		ofs += w;
	}
}

/*
 ==================
 R_DrawCinematic
 ==================
*/
void R_DrawCinematic (int x, int y, int w, int h, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, int handle, const byte *image, bool dirty, int width, int height, bool keepAspect){

	drawCinematicCommand_t	*cmd;
	int						x1, y1, x2, y2;
	int						size;

	if (handle <= 0 || handle > MAX_CINEMATICS)
		Com_Error(ERR_DROP, "R_DrawCinematic: handle out of range");

	// Scale the coordinates and correct the aspect ratio if needed
	R_AdjustHorzCoordsInt(horzAdjust, horzPercent, x, w, &x, &w);
	R_AdjustVertCoordsInt(vertAdjust, vertPercent, y, h, &y, &h);

	// Set up the coordinates
	x1 = x;
	y1 = y;
	x2 = x + w;
	y2 = y + h;

	// Add a draw cinematic command
	cmd = (drawCinematicCommand_t *)R_GetCommandBuffer(sizeof(drawCinematicCommand_t));

	cmd->commandId = RC_DRAW_CINEMATIC;

	cmd->texture = rg.cinematicTextures[handle - 1];

	cmd->image = image;
	cmd->dirty = dirty;
	cmd->width = width;
	cmd->height = height;

	cmd->vx1 = x1;
	cmd->vy1 = y1;
	cmd->vx2 = x2;
	cmd->vy2 = y2;

	cmd->wx1 = x1;
	cmd->wy1 = y1;
	cmd->wx2 = x2;
	cmd->wy2 = y2;

	// If desired, calculate coordinates preserving the original aspect ratio
	// of the cinematic, letterboxing or pillarboxing if needed
	if (keepAspect){
		if (w * height > h * width){
			size = h * width / height;

			x = x + ((w - size) >> 1);

			cmd->vx1 = x;
			cmd->vx2 = x + size;
		}
		else if (w * height < h * width){
			size = w * height / width;

			y = y + ((h - size) >> 1);

			cmd->vy1 = y;
			cmd->vy2 = y + size;
		}
	}
}

/*
 ==================
 R_ResetCropSize
 ==================
*/
static void R_ResetCropSize (){

	cropSizeCommand_t	*cmd;
	int					width, height;
	int					size;

	// Calculate the crop size
	if (rg.envShotRendering){
		width = rg.envShotSize;
		height = rg.envShotSize;
	}
	else {
		width = FloatToInt(glConfig.videoWidth * r_screenFraction->floatValue);
		height = FloatToInt(glConfig.videoHeight * r_screenFraction->floatValue);
	}

	// Set up the current render crop
	rg.currentRenderCrop = 0;

	rg.renderCrops[rg.currentRenderCrop].width = width;
	rg.renderCrops[rg.currentRenderCrop].height = height;

	rg.renderCrops[rg.currentRenderCrop].rect.x = 0;
	rg.renderCrops[rg.currentRenderCrop].rect.y = 0;
	rg.renderCrops[rg.currentRenderCrop].rect.width = width;
	rg.renderCrops[rg.currentRenderCrop].rect.height = height;

	rg.renderCrops[rg.currentRenderCrop].xScale = width * (1.0f / SCREEN_WIDTH);
	rg.renderCrops[rg.currentRenderCrop].yScale = height * (1.0f / SCREEN_HEIGHT);

	if (width * SCREEN_HEIGHT > height * SCREEN_WIDTH){
		size = height * SCREEN_WIDTH / SCREEN_HEIGHT;

		rg.renderCrops[rg.currentRenderCrop].aspectRatio = ASPECT_WIDE;
		rg.renderCrops[rg.currentRenderCrop].aspectScale = size * (1.0f / SCREEN_WIDTH);
		rg.renderCrops[rg.currentRenderCrop].aspectBias = (width - size) * 0.5f;
	}
	else if (width * SCREEN_HEIGHT < height * SCREEN_WIDTH){
		size = width * SCREEN_HEIGHT / SCREEN_WIDTH;

		rg.renderCrops[rg.currentRenderCrop].aspectRatio = ASPECT_HIGH;
		rg.renderCrops[rg.currentRenderCrop].aspectScale = size * (1.0f / SCREEN_HEIGHT);
		rg.renderCrops[rg.currentRenderCrop].aspectBias = (height - size) * 0.5f;
	}
	else {
		rg.renderCrops[rg.currentRenderCrop].aspectRatio = ASPECT_NORMAL;
		rg.renderCrops[rg.currentRenderCrop].aspectScale = width * (1.0f / SCREEN_WIDTH);
		rg.renderCrops[rg.currentRenderCrop].aspectBias = 0.0f;
	}

	// Add a crop size command
	cmd = (cropSizeCommand_t *)R_GetCommandBuffer(sizeof(cropSizeCommand_t));

	cmd->commandId = RC_CROP_SIZE;

	cmd->width = rg.renderCrops[rg.currentRenderCrop].width;
	cmd->height = rg.renderCrops[rg.currentRenderCrop].height;
}

/*
 ==================
 R_CropRenderSize
 ==================
*/
void R_CropRenderSize (int width, int height, bool forceDimensions){

	cropSizeCommand_t	*cmd;
	int					size;

	// Calculate the crop size
	if (rg.envShotRendering){
		width = rg.envShotSize;
		height = rg.envShotSize;
	}
	else {
		if (forceDimensions){
			if (width < 1 || height < 1 || width > glConfig.videoWidth || height > glConfig.videoHeight)
				Com_Error(ERR_DROP, "R_CropRenderSize: bad crop size (%i x %i)", width, height);

			width = FloatToInt(width * r_screenFraction->floatValue);
			height = FloatToInt(height * r_screenFraction->floatValue);
		}
		else {
			if (width < 1 || height < 1 || width > SCREEN_WIDTH || height > SCREEN_HEIGHT)
				Com_Error(ERR_DROP, "R_CropRenderSize: bad crop size (%i x %i)", width, height);

			width = FloatToInt(width * rg.renderCrops[rg.currentRenderCrop].xScale);
			height = FloatToInt(height * rg.renderCrops[rg.currentRenderCrop].yScale);
		}
	}

	// Set up the current render crop
	rg.currentRenderCrop++;

	if (rg.currentRenderCrop == MAX_RENDER_CROPS)
		Com_Error(ERR_DROP, "R_CropRenderSize: MAX_RENDER_CROPS hit");

	rg.renderCrops[rg.currentRenderCrop].width = width;
	rg.renderCrops[rg.currentRenderCrop].height = height;

	rg.renderCrops[rg.currentRenderCrop].rect.x = 0;
	rg.renderCrops[rg.currentRenderCrop].rect.y = 0;
	rg.renderCrops[rg.currentRenderCrop].rect.width = width;
	rg.renderCrops[rg.currentRenderCrop].rect.height = height;

	rg.renderCrops[rg.currentRenderCrop].xScale = width * (1.0f / SCREEN_WIDTH);
	rg.renderCrops[rg.currentRenderCrop].yScale = height * (1.0f / SCREEN_HEIGHT);

	if (width * SCREEN_HEIGHT > height * SCREEN_WIDTH){
		size = height * SCREEN_WIDTH / SCREEN_HEIGHT;

		rg.renderCrops[rg.currentRenderCrop].aspectRatio = ASPECT_WIDE;
		rg.renderCrops[rg.currentRenderCrop].aspectScale = size * (1.0f / SCREEN_WIDTH);
		rg.renderCrops[rg.currentRenderCrop].aspectBias = (width - size) * 0.5f;
	}
	else if (width * SCREEN_HEIGHT < height * SCREEN_WIDTH){
		size = width * SCREEN_HEIGHT / SCREEN_WIDTH;

		rg.renderCrops[rg.currentRenderCrop].aspectRatio = ASPECT_HIGH;
		rg.renderCrops[rg.currentRenderCrop].aspectScale = size * (1.0f / SCREEN_HEIGHT);
		rg.renderCrops[rg.currentRenderCrop].aspectBias = (height - size) * 0.5f;
	}
	else {
		rg.renderCrops[rg.currentRenderCrop].aspectRatio = ASPECT_NORMAL;
		rg.renderCrops[rg.currentRenderCrop].aspectScale = width * (1.0f / SCREEN_WIDTH);
		rg.renderCrops[rg.currentRenderCrop].aspectBias = 0.0f;
	}

	// Add a crop size command
	cmd = (cropSizeCommand_t *)R_GetCommandBuffer(sizeof(cropSizeCommand_t));

	cmd->commandId = RC_CROP_SIZE;

	cmd->width = rg.renderCrops[rg.currentRenderCrop].width;
	cmd->height = rg.renderCrops[rg.currentRenderCrop].height;
}

/*
 ==================
 R_UnCropRenderSize
 ==================
*/
void R_UnCropRenderSize (){

	cropSizeCommand_t	*cmd;

	// Set up the current render crop
	if (rg.currentRenderCrop < 1)
		Com_Error(ERR_DROP, "R_UnCropRenderSize: currentRenderCrop < 1");

	rg.currentRenderCrop--;

	// Add a crop size command
	cmd = (cropSizeCommand_t *)R_GetCommandBuffer(sizeof(cropSizeCommand_t));

	cmd->commandId = RC_CROP_SIZE;

	cmd->width = rg.renderCrops[rg.currentRenderCrop].width;
	cmd->height = rg.renderCrops[rg.currentRenderCrop].height;
}


// ============================================================================


/*
 ==================
 R_BeginFrame
 ==================
*/
void R_BeginFrame (int time){

	setupBuffersCommand_t	*cmd;

	// Log file
	if (r_logFile->modified){
		QGL_EnableLogging(r_logFile->integerValue);

		r_logFile->modified = false;
	}

	// Set the time
	rg.time = time;

	// Bump frame count
	rg.frameCount++;

	// Clear primary view
	rg.primaryViewAvailable = false;

	// Clear mesh and light lists
	R_ClearMeshes();
	R_ClearLights();

	// Clear scene render lists
	rg.scene.numEntities = rg.scene.firstEntity = 1;
	rg.scene.numLights = rg.scene.firstLight = 0;
	rg.scene.numParticles = rg.scene.firstParticle = 0;

	// Reset the crop size
	R_ResetCropSize();

	// Set default GL state
	GL_SetDefaultState();

	// Set gamma table and device gamma ramp if needed
	if (r_gamma->modified || r_contrast->modified || r_brightness->modified){
		R_SetGamma();

		r_gamma->modified = false;
		r_contrast->modified = false;
		r_brightness->modified = false;
	}

	// Change texture filtering if needed
	if (r_textureFilter->modified || r_textureLODBias->modified || r_textureAnisotropy->modified){
		R_ChangeTextureFilter();

		r_textureFilter->modified = false;
		r_textureLODBias->modified = false;
		r_textureAnisotropy->modified = false;
	}

	// Add a setup buffers command
	cmd = (setupBuffersCommand_t *)R_GetCommandBuffer(sizeof(setupBuffersCommand_t));

	cmd->commandId = RC_SETUP_BUFFERS;
}

/*
 ==================
 R_EndFrame
 ==================
*/
void R_EndFrame (){

	swapBuffersCommand_t	*cmd;

	// Add a swap buffers command
	cmd = (swapBuffersCommand_t *)R_GetCommandBuffer(sizeof(swapBuffersCommand_t));

	cmd->commandId = RC_SWAP_BUFFERS;

	// Issue all commands
	R_IssueRenderCommands();

	// Log file
	if (r_logFile->integerValue > 0)
		CVar_SetInteger(r_logFile, r_logFile->integerValue - 1);
}