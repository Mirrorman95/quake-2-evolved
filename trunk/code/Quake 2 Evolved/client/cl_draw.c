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
// cl_draw.c - Drawing 2D elements
//


#include "client.h"


/*
 ==================
 CL_FadeColor
 ==================
*/
float *CL_FadeColor (const vec4_t color, int startTime, int totalTime, int fadeTime){

	static vec4_t	fadeColor;
	int				time;
	float			scale;

	time = cl.time - startTime;
	if (time >= totalTime)
		return NULL;

	if (totalTime - time < fadeTime && fadeTime != 0)
		scale = (float)(totalTime - time) * (1.0f / fadeTime);
	else
		scale = 1.0f;

	fadeColor[0] = color[0] * scale;
	fadeColor[1] = color[1] * scale;
	fadeColor[2] = color[2] * scale;
	fadeColor[3] = color[3];

	return fadeColor;
}

/*
 ==================
 CL_FadeAlpha
 ==================
*/
float *CL_FadeAlpha (const vec4_t color, int startTime, int totalTime, int fadeTime){

	static vec4_t	fadeColor;
	int				time;
	float			scale;

	time = cl.time - startTime;
	if (time >= totalTime)
		return NULL;

	if (totalTime - time < fadeTime && fadeTime != 0)
		scale = (float)(totalTime - time) * (1.0f / fadeTime);
	else
		scale = 1.0f;

	fadeColor[0] = color[0];
	fadeColor[1] = color[1];
	fadeColor[2] = color[2];
	fadeColor[3] = color[3] * scale;

	return fadeColor;
}

/*
 ==================
 CL_FadeColorAndAlpha
 ==================
*/
float *CL_FadeColorAndAlpha (const vec4_t color, int startTime, int totalTime, int fadeTime){

	static vec4_t	fadeColor;
	int				time;
	float			scale;

	time = cl.time - startTime;
	if (time >= totalTime)
		return NULL;

	if (totalTime - time < fadeTime && fadeTime != 0)
		scale = (float)(totalTime - time) * (1.0f / fadeTime);
	else
		scale = 1.0f;

	fadeColor[0] = color[0] * scale;
	fadeColor[1] = color[1] * scale;
	fadeColor[2] = color[2] * scale;
	fadeColor[3] = color[3] * scale;

	return fadeColor;
}

/*
 ==================
 CL_FillRect
 ==================
*/
void CL_FillRect (float x, float y, float w, float h, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent){

	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, horzAdjust, horzPercent, vertAdjust, vertPercent, cls.media.whiteMaterial);
}

/*
 ==================
 
 ==================
*/
void CL_DrawString (float x, float y, float w, float h, float width, const char *string, const color_t color, material_t *fontMaterial, int flags){

}

/*
 ==================
 
 ==================
*/
void CL_DrawStringSheared (float x, float y, float w, float h, float shearX, float shearY, float width, const char *string, const color_t color, material_t *fontMaterial, int flags){

}

/*
 ==================
 
 ==================
*/
void CL_DrawStringFixed (float x, float y, float w, float h, float width, const char *string, const color_t color, material_t *fontMaterial, int flags){

}

/*
 ==================
 
 ==================
*/
void CL_DrawStringShearedFixed (float x, float y, float w, float h, float shearX, float shearY, float width, const char *string, const color_t color, material_t *fontMaterial, int flags){

}

/*
 ==================
 CL_DrawPic
 ==================
*/
void CL_DrawPic (float x, float y, float w, float h, const vec4_t color, material_t *material){

	R_SetColor(color);
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_NONE, 1.0f, V_NONE, 1.0f, material);
}

/*
 ==================
 CL_DrawPicST
 ==================
*/
void CL_DrawPicST (float x, float y, float w, float h, float s1, float t1, float s2, float t2, const vec4_t color, material_t *material){

	R_SetColor(color);
	R_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, H_NONE, 1.0f, V_NONE, 1.0f, material);
}

/*
 ==================
 
 ==================
*/
void CL_DrawPicSheared (float x, float y, float w, float h, float shearX, float shearY, const color_t color, material_t *material){

}

/*
 ==================
 
 ==================
*/
void CL_DrawPicShearedST (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float shearX, float shearY, const color_t color, material_t *material){

}

/*
 ==================
 CL_DrawPicByName
 ==================
*/
void CL_DrawPicByName (float x, float y, float w, float h, const vec4_t color, const char *pic){

	material_t	*material;
	char		name[MAX_OSPATH];

	if (!Str_FindChar(pic, '/'))
		Str_SPrintf(name, sizeof(name), "pics/%s", pic);
	else {
		Str_Copy(name, pic, sizeof(name));
		Str_StripFileExtension(name);
	}

	material = R_RegisterMaterialNoMip(name);

	R_SetColor(color);
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_NONE, 1.0f, V_NONE, 1.0f, material);
}

/*
 ==================
 
 ==================
*/
void CL_DrawPicFixed (float x, float y, material_t *material){

}

/*
 ==================
 
 ==================
*/
void CL_DrawPicFixedByName (float x, float y, const char *pic){

}


// ============================================================================


/*
 ==================
 
 ==================
*/
void CL_DrawLoading (){

	char	string[MAX_STRING_LENGTH];
	float	speed;
	int		percent;

	if (!cls.loading)
		return;

	switch (cls.state){
	case CA_DISCONNECTED:
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, "ui/assets/title_screen/title_backg");
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, 160.0f, colorWhite, "ui/assets/title_screen/q2e_logo");

		break;
	case CA_CONNECTING:
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, "ui/assets/title_screen/title_backg");
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, 160.0f, colorWhite, "ui/assets/title_screen/q2e_logo");

		if (NET_IsLocalAddress(cls.serverAddress)){
			Str_SPrintf(string, sizeof(string), "Starting up...");
			// CL_DrawString
		}
		else {
//			if (cls.serverMessage[0])
				// CL_DrawString

			Str_SPrintf(string, sizeof(string), "Connecting to %s\nAwaiting connection... %i", cls.serverName, cls.connectCount);
			// CL_DrawString
		}

		break;
	case CA_CHALLENGING:
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, "ui/assets/title_screen/title_backg");
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, 160.0f, colorWhite, "ui/assets/title_screen/q2e_logo");

		if (NET_IsLocalAddress(cls.serverAddress)){
			Str_SPrintf(string, sizeof(string), "Starting up...");
			// CL_DrawString
		}
		else {
//			if (cls.serverMessage[0])
				// CL_DrawString

			Str_SPrintf(string, sizeof(string), "Connecting to %s\nAwaiting challenge... %i", cls.serverName, cls.connectCount);
			// CL_DrawString
		}

		break;
	case CA_CONNECTED:
		if (cls.downloadFile){
			CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, "ui/assets/title_screen/title_backg");
			CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, 160.0f, colorWhite, "ui/assets/title_screen/q2e_logo");

			if (cls.downloadStart != cls.realTime)
				speed = (float)(cls.downloadBytes / 1024) / ((cls.realTime - cls.downloadStart) / 1000);
			else
				speed = 0;

			if (Com_ServerState()){
				Str_SPrintf(string, sizeof(string), "Downloading %s... (%i%% @ %.2f KB/sec)", cls.downloadName, cls.downloadPercent, speed);
				// CL_DrawString
			}
			else {
				Str_SPrintf(string, sizeof(string), "Connecting to %s\nDownloading %s... (%i%% @ %.2f KB/sec)", cls.serverName, cls.downloadName, cls.downloadPercent, speed);
				// CL_DrawString
			}

			percent = Clamp(cls.downloadPercent - (cls.downloadPercent % 5), 5, 100);
			if (percent){
				CL_DrawPicByName(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, "ui/assets/loading/load_main2");
				CL_DrawPicByName(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, Str_VarArgs("ui/assets/loading/percent/load_%i", percent));
				CL_DrawPicByName(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, "ui/assets/loading/load_main");
			}

			break;
		}

		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, "ui/assets/title_screen/title_backg");
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, 160.0f, colorWhite, "ui/assets/title_screen/q2e_logo");

		if (NET_IsLocalAddress(cls.serverAddress)){
			Str_SPrintf(string, sizeof(string), "Starting up...");
			// CL_DrawString
		}
		else {
			Str_SPrintf(string, sizeof(string), "Connecting to %s\nAwaiting game state...", cls.serverName);
			// CL_DrawString
		}

		break;
	case CA_LOADING:
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshot);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshotDetail);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, 160.0f, colorWhite, cl.media.loadingLogo);

		if (NET_IsLocalAddress(cls.serverAddress)){
			Str_SPrintf(string, sizeof(string), "Loading %s\n\"%s\"\n\n\nLoading... %s\n", cls.loadingInfo.map, cls.loadingInfo.name, cls.loadingInfo.string);
			// CL_DrawString
		}
		else {
			Str_SPrintf(string, sizeof(string), "Loading %s\n\"%s\"\n\nConnecting to %s\nLoading... %s\n", cls.loadingInfo.map, cls.loadingInfo.name, cls.serverName, cls.loadingInfo.string);
			// CL_DrawString
		}

		percent = Clamp((cls.loadingInfo.percent / 5) - 1, 0, 19);
		if (percent){
			CL_DrawPic(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, cl.media.loadingDetail[0]);
			CL_DrawPic(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, cl.media.loadingPercent[percent]);
			CL_DrawPic(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, cl.media.loadingDetail[1]);
		}

		break;
	case CA_PRIMED:
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshot);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshotDetail);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, 160.0f, colorWhite, cl.media.loadingLogo);

		break;
	case CA_ACTIVE:
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshot);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshotDetail);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, 160.0f, colorWhite, cl.media.loadingLogo);

		Str_SPrintf(string, sizeof(string), "Awaiting frame...");
		// CL_DrawString

		break;
	}
}


// ============================================================================


/*
 ==================
 
 ==================
*/
static void CL_DrawStatus (){

	if (!cl_drawStatus->integerValue)
		return;
}

/*
 ==================
 
 ==================
*/
static void CL_DrawInventory (){

	if (!cl_drawInventory->integerValue)
		return;
}

/*
 ==================
 
 ==================
*/
static void CL_DrawLayout (){

	if (!cl_drawLayout->integerValue)
		return;
}

/*
 ==================
 CL_ScanForPlayerEntity
 ==================
*/
static void CL_ScanForPlayerEntity (){

	trace_t	trace;
	vec3_t	start, end;
	int		entNumber;

	VectorCopy(cl.renderView.origin, start);
	VectorMA(cl.renderView.origin, 8192.0f, cl.renderView.axis[0], end);

	trace = CL_Trace(start, vec3_origin, vec3_origin, end, cl.clientNum, MASK_PLAYERSOLID, false, &entNumber);
	if (trace.fraction == 0.0f || trace.fraction == 1.0f)
		return;

	if (entNumber < 1 || entNumber > MAX_CLIENTS)
		return;		// Not a valid entity

	if (cl.entities[entNumber].current.modelindex != 255)
		return;		// Not a player, or invisible

	cl.crosshairEntTime = cl.time;
	cl.crosshairEntNumber = entNumber;
}

/*
 ==================
 
 ==================
*/
static void CL_DrawCrosshair (){

	if (!cl_drawCrosshair->integerValue)
		return;
}

/*
 ==================
 
 TODO: fix position
 ==================
*/
static void CL_DrawCenterString (){

	float	*fadeColor;

	if (!cl_drawCenterString->integerValue)
		return;

	fadeColor = CL_FadeAlpha(colorWhite, cl.centerPrintTime, cl_centerTime->integerValue, cl_centerTime->integerValue / 4);
	if (!fadeColor)
		return;

	R_DrawString(0.0f, SCREEN_HEIGHT - 320.0f, 10.0f, 10.0f, cl.centerPrint, fadeColor, true, 1.0f, 2.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
}

/*
 ==================
 
 ==================
*/
static void CL_DrawLagometer (){

	if (!cl_drawLagometer->integerValue)
		return;
}

/*
 ==================
 CL_DrawDisconnected
 ==================
*/
static void CL_DrawDisconnected (){

	if (!cl_drawDisconnected->integerValue)
		return;

	// Don't draw if we're also the server
	if (Com_ServerState())
		return;

	if (cls.netChan.outgoingSequence - cls.netChan.incomingAcknowledged < CMD_BACKUP-1)
		return;

	// TODO: draw text?

	// Draw the icon if needed
	if ((cl.time >> 9) & 1)
		return;

	R_SetColor1(1.0f);
	R_DrawStretchPic(SCREEN_WIDTH - 64.0f, SCREEN_HEIGHT - 136.0f, 48.0f, 48.0f, 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_RIGHT, 1.0f, V_ALIGN_BOTTOM, 1.0f, cl.media.disconnectedMaterial);
}

/*
 ==================
 CL_DrawDemoRecording
 ==================
*/
static void CL_DrawDemoRecording (){

	char	string[MAX_STRING_LENGTH];
	int		length;

	if (!cl_drawRecording->integerValue)
		return;

	// Check if recording a demo
	if (!cls.demoFile)
		return;

	// Set the string
	length = Str_SPrintf(string, sizeof(string), "RECORDING: %s (%i KB)", cls.demoName, FS_Tell(cls.demoFile) / 1024);

	// Draw it
	R_DrawString((SCREEN_WIDTH - length * 8.0f) * 0.5f, 96.0f, 8.0f, 16.0f, string, colorWhite, true, 1.0f, 2.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
}

/*
 ==================
 CL_DrawFPS
 ==================
*/
static void CL_DrawFPS (){

	static int	previousTime;
	int			time, totalTime;
	int			fps;
	char		string[16];
	vec4_t		color;
	int			length;
	int			i;

	time = Sys_Milliseconds();

	cls.fpsTimes[cls.fpsIndex & (FPS_FRAMES - 1)] = time - previousTime;
	cls.fpsIndex++;

	previousTime = time;

	if (!cl_drawFPS->integerValue || cls.fpsIndex <= FPS_FRAMES)
		return;

	// Average multiple frames together to smooth changes out a bit
	totalTime = 0;

	for (i = 0; i < FPS_FRAMES; i++)
		totalTime += cls.fpsTimes[i];

	if (totalTime < 1)
		totalTime = 1;

	fps = 1000 * FPS_FRAMES / totalTime;
	if (fps > 1000)
		fps = 1000;

	// Set the string
	length = Str_SPrintf(string, sizeof(string), "%i FPS", fps);

	// Set the color
	if (fps >= 60)
		MakeRGBA(color, 1.0f, 1.0f, 1.0f, 1.0f);
	else {
		if (fps < 30)
			MakeRGBA(color, 1.0f, (float)fps / 30.0f, 0.0f, 1.0f);
		else
			MakeRGBA(color, 1.0f, 1.0f, (float)(fps - 30) / 30.0f, 1.0f);
	}

	// Draw it
	R_DrawString((SCREEN_WIDTH - length * 12.0f) - 6.0f, 6.0f, 12.0f, 12.0f, string, color, true, 2.0f, 2.0f, H_ALIGN_RIGHT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
}

/*
 ==================
 
 ==================
*/
static void CL_DrawPause (){

	if (!cl_drawPause->integerValue)
		return;
}

/*
 ==================
 
 ==================
*/
static void CL_DrawMaterial (){

	if (!cl_showMaterial->integerValue)
		return;
}


// ============================================================================


/*
 ==================
 CL_Draw2D
 ==================
*/
void CL_Draw2D (){

	if (!cl_draw2D->integerValue)
		return;

	// Draw status
	CL_DrawStatus();

	// Draw inventory
	CL_DrawInventory();

	// Draw hud layout
	CL_DrawLayout();

	// Draw the crosshair
	CL_DrawCrosshair();

	// Draw the center string
	CL_DrawCenterString();

	// Draw the lagometer graph
	CL_DrawLagometer();

	// Draw the disconnected icon
	CL_DrawDisconnected();

	// Draw the demo recording information
	CL_DrawDemoRecording();

	// Draw the pause frames-per-second counter
	CL_DrawFPS();

	// Draw the pause icon
	CL_DrawPause();

	// Draw the material under the crosshair
	CL_DrawMaterial();
}