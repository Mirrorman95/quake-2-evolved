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
 
 ==================
*/
static void CL_ScanForPlayerEntity (){

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
 
 ==================
*/
static void CL_DrawCenterString (){

	if (!cl_drawCenterString->integerValue)
		return;
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
 
 ==================
*/
static void CL_DrawDisconnected (){

	if (!cl_drawDisconnected->integerValue)
		return;
}

/*
 ==================
 
 ==================
*/
static void CL_DrawRecording (){

	if (!cl_drawRecording->integerValue)
		return;
}

/*
 ==================
 
 ==================
*/
static void CL_DrawFPS (){

	if (!cl_drawFPS->integerValue)
		return;
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

	// Draw the recording information
	CL_DrawRecording();

	// Draw the pause frames-per-second counter
	CL_DrawFPS();

	// Draw the pause icon
	CL_DrawPause();

	// Draw the material under the crosshair
	CL_DrawMaterial();
}