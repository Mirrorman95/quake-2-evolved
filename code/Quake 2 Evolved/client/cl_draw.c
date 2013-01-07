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
// cl_draw.c - Drawing functions
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
 CL_DrawTextWidth
 ==================
*/
float CL_DrawTextWidth (const char *text, float scale, fontInfo_t *fontInfo){

	glyphInfo_t	*glyphInfo;
	float		width = 0.0f;

	while (*text){
		if (Str_IsColor(text)){
			text += 2;
			continue;
		}

		glyphInfo = &fontInfo->glyphs[*(const byte *)text++];

		width += glyphInfo->xAdjust * scale;
	}

	return width;
}

/*
 ==================
 
 ==================
*/
void CL_DrawText (){

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
void CL_DrawStringSheared (float x, float y, float w, float h, float shearX, float shearY, float width, const char *string, const color_t color, material_t *fontMaterial, int flags){

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
void CL_DrawPic (float x, float y, float w, float h, const vec4_t color, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, material_t *material){

	R_SetColor(color);
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, horzAdjust, horzPercent, vertAdjust, vertPercent, material);
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
 CL_DrawPicSheared
 ==================
*/
void CL_DrawPicSheared (float x, float y, float w, float h, float xShear, float yShear, float rotate, const vec4_t color, material_t *material){

	R_SetColor(color);
	R_DrawStretchPicEx(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, xShear, yShear, rotate, H_NONE, 1.0f, V_NONE, 1.0f, material);
}

/*
 ==================
 CL_DrawPicShearedST
 ==================
*/
void CL_DrawPicShearedST (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float xShear, float yShear, float rotate, const vec4_t color, material_t *material){

	R_SetColor(color);
	R_DrawStretchPicEx(x, y, w, h, s1, t1, s2, t2, xShear, yShear, rotate, H_NONE, 1.0f, V_NONE, 1.0f, material);
}

/*
 ==================
 CL_DrawPicByName
 ==================
*/
void CL_DrawPicByName (float x, float y, float w, float h, const vec4_t color, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, const char *pic){

	material_t	*material;
	char		name[MAX_PATH_LENGTH];

	if (!Str_FindChar(pic, '/'))
		Str_SPrintf(name, sizeof(name), "pics/%s", pic);
	else {
		Str_Copy(name, pic, sizeof(name));
		Str_StripFileExtension(name);
	}

	material = R_RegisterMaterialNoMip(name);

	R_SetColor(color);
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, horzAdjust, horzPercent, vertAdjust, vertPercent, material);
}

/*
 ==================
 
 TODO: use h/v flags?
 ==================
*/
void CL_DrawPicFixed (float x, float y, material_t *material){

	float	w, h;

	R_GetPicSize(material, &w, &h);

	R_SetColor(colorWhite);
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_NONE, 1.0f, V_NONE, 1.0f, material);
}

/*
 ==================
 
 FIXME: does not show up on screen for some reason...
 ==================
*/
void CL_DrawPicFixedByName (float x, float y, const char *pic){

	material_t	*material;
	char		name[MAX_PATH_LENGTH];
	float		w, h;

	if (!Str_FindChar(pic, '/'))
		Str_SPrintf(name, sizeof(name), "pics/%s", pic);
	else {
		Str_Copy(name, pic, sizeof(name));
		Str_StripFileExtension(name);
	}

	material = R_RegisterMaterialNoMip(name);

	R_GetPicSize(material, &w, &h);

	R_SetColor(colorWhite);
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_NONE, 1.0f, V_NONE, 1.0f, material);
}


/*
 ==============================================================================

 LOADING SCREEN

 ==============================================================================
*/


/*
 ==================
 CL_DrawConnecting
 ==================
*/
static void CL_DrawConnecting (){

	char	string[512];
	float	ofs;
	int		length;

	// Awaiting connection
	CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, "ui/assets/title_screen/title_backg");
	CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, "ui/assets/title_screen/q2e_logo");

	if (NET_IsLocalAddress(cls.serverAddress)){
		ofs = SCREEN_HEIGHT - 56.0f;

		length = Str_SPrintf(string, sizeof(string), "Starting up...");
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
	}
	else {
		if (cls.serverMessage[0]){
			ofs = SCREEN_HEIGHT - 128.0f;

			length = Str_SPrintf(string, sizeof(string), "%s", cls.serverMessage);
			R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		}

		ofs = SCREEN_HEIGHT - 72.0f;

		length = Str_SPrintf(string, sizeof(string), "Connecting to %s", cls.serverName);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;

		length = Str_SPrintf(string, sizeof(string), "Awaiting challenge... %i", cls.connectCount);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;
	}
}

/*
 ==================
 CL_DrawChallenging
 ==================
*/
static void CL_DrawChallenging (){

	char	string[512];
	float	ofs;
	int		length;

	// Awaiting challenge
	CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, "ui/assets/title_screen/title_backg");
	CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, "ui/assets/title_screen/q2e_logo");

	if (NET_IsLocalAddress(cls.serverAddress)){
		ofs = SCREEN_HEIGHT - 56.0f;

		length = Str_SPrintf(string, sizeof(string), "Starting up...");
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
	}
	else {
		if (cls.serverMessage[0]){
			ofs = SCREEN_HEIGHT - 128.0f;

			length = Str_SPrintf(string, sizeof(string), "%s", cls.serverMessage);
			R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		}

		ofs = SCREEN_HEIGHT - 72.0f;

		length = Str_SPrintf(string, sizeof(string), "Connecting to %s", cls.serverName);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;

		length = Str_SPrintf(string, sizeof(string), "Awaiting challenge... %i", cls.connectCount);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;
	}
}

/*
 ==================
 CL_DrawConnected
 ==================
*/
static void CL_DrawConnected (){

	char	string[512];
	float	speed, ofs;
	int		length;
	int		percent;

	// Downloading file from server
	if (cls.downloadFile){
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, "ui/assets/title_screen/title_backg");
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, "ui/assets/title_screen/q2e_logo");

		if (cls.downloadStart != cls.realTime)
			speed = (float)(cls.downloadBytes / 1024) / ((cls.realTime - cls.downloadStart) / 1000);
		else
			speed = 0;

		if (Com_ServerState()){
			ofs = SCREEN_HEIGHT - 56.0f;

			length = Str_SPrintf(string, sizeof(string), "Downloading %s... (%i%% @ %.2f KB/sec)", cls.downloadName, cls.downloadPercent, speed);
			R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
			ofs += 16.0f;
		}
		else {
			ofs = SCREEN_HEIGHT - 72.0f;

			length = Str_SPrintf(string, sizeof(string), "Connecting to %s", cls.serverName);
			R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
			ofs += 16.0f;

			length = Str_SPrintf(string, sizeof(string), "Downloading %s... (%i%% @ %.2f KB/sec)", cls.downloadName, cls.downloadPercent, speed);
			R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
			ofs += 16.0f;
		}

		percent = ClampInt(cls.downloadPercent - (cls.downloadPercent % 5), 5, 100);
		if (percent){
			CL_DrawPicByName(SCREEN_WIDTH - 400.0f, SCREEN_HEIGHT - 320.0f, 160.0f, 160.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, "ui/assets/loading/load_main2");
			CL_DrawPicByName(SCREEN_WIDTH - 400.0f, SCREEN_HEIGHT - 320.0f, 160.0f, 160.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, Str_VarArgs("ui/assets/loading/percent/load_%i", percent));
			CL_DrawPicByName(SCREEN_WIDTH - 400.0f, SCREEN_HEIGHT - 320.0f, 160.0f, 160.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, "ui/assets/loading/load_main");
		}

		return;
	}

	// Awaiting game state
	CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, "ui/assets/title_screen/title_backg");
	CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, "ui/assets/title_screen/q2e_logo");

	if (NET_IsLocalAddress(cls.serverAddress)){
		ofs = SCREEN_HEIGHT - 56.0f;

		length = Str_SPrintf(string, sizeof(string), "Starting up...");
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
	}
	else {
		ofs = SCREEN_HEIGHT - 72.0f;

		length = Str_SPrintf(string, sizeof(string), "Connecting to %s", cls.serverName);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;

		length = Str_SPrintf(string, sizeof(string), "Awaiting game state...");
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;
	}
}

/*
 ==================
 CL_DrawLoadingData

 FIXME: the offets needs to be adjusted as the original
 ==================
*/
static void CL_DrawLoadingData (){

	char	string[512];
	float	ofs;
	int		length;
	int		percent;

	// Loading level data
	CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.levelshot);
	CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.levelshotDetail);
	CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.loadingLogo);

	ofs = SCREEN_HEIGHT - 120.0f;

	if (NET_IsLocalAddress(cls.serverAddress)){
		length = Str_SPrintf(string, sizeof(string), "Loading %s", cls.loadingInfo.map);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;

		length = Str_SPrintf(string, sizeof(string), "\"%s\"", cls.loadingInfo.name);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 48.0f;

		length = Str_SPrintf(string, sizeof(string), "Loading... %s", cls.loadingInfo.string);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;
	}
	else {
		length = Str_SPrintf(string, sizeof(string), "Loading %s", cls.loadingInfo.map);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;

		length = Str_SPrintf(string, sizeof(string), "\"%s\"", cls.loadingInfo.name);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 48.0f;

		length = Str_SPrintf(string, sizeof(string), "Connecting to %s", cls.serverName);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;

		length = Str_SPrintf(string, sizeof(string), "Loading... %s", cls.loadingInfo.string);
		R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
		ofs += 16.0f;
	}

	percent = ClampInt((cls.loadingInfo.percent / 5) - 1, 0, 19);
	if (percent){
		CL_DrawPic(SCREEN_WIDTH - 400.0f, SCREEN_HEIGHT - 320.0f, 160.0f, 160.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.loadingDetail[0]);
		CL_DrawPic(SCREEN_WIDTH - 400.0f, SCREEN_HEIGHT - 320.0f, 160.0f, 160.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.loadingPercent[percent]);
		CL_DrawPic(SCREEN_WIDTH - 400.0f, SCREEN_HEIGHT - 320.0f, 160.0f, 160.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.loadingDetail[1]);
	}
}

/*
 ==================
 CL_DrawPrimed
 ==================
*/
static void CL_DrawPrimed (){

	char	string[512];
	float	ofs;
	int		length;

	// Awaiting frame
	ofs = SCREEN_HEIGHT - 56.0f;

	CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.levelshot);
	CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.levelshotDetail);
	CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.loadingLogo);

	length = Str_SPrintf(string, sizeof(string), "Awaiting frame...");
	R_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, ofs, 16.0f, 16.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_BOTTOM, 1.0f, cls.media.charsetMaterial);
}

/*
 ==================
 CL_DrawLoading
 ==================
*/
void CL_DrawLoading (){

	switch (cls.state){
	case CA_CONNECTING:
		CL_DrawConnecting();
		break;
	case CA_CHALLENGING:
		CL_DrawChallenging();
		break;
	case CA_CONNECTED:
		CL_DrawConnected();
		break;
	case CA_LOADING:
		CL_DrawLoadingData();
		break;
	case CA_PRIMED:
		CL_DrawPrimed();
		break;
	}
}


/*
 ==============================================================================

 SCREEN BLENDS

 ==============================================================================
*/


/*
 ==================
 CL_DrawFireScreenBlend
 ==================
*/
static void CL_DrawFireScreenBlend (){

	vec4_t	color;
	float	alpha;
	int		time;

	if (cl.time < cl.fireScreenEndTime){
		// Calculate alpha
		time = cl.fireScreenEndTime - cl.time;

		if (time < 750)
			alpha = (float)time * (1.0f / 750);
		else
			alpha = 1.0f;

		// Draw it
		MakeRGBA(color, 1.0f, 1.0f, 1.0f, 0.5f * alpha);

		R_SetColor(color);
		R_DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.fireScreenMaterial);
	}
}

/*
 ==================
 
 ==================
*/
static void CL_DrawWaterBlurBlend (){

}

/*
 ==================
 
 ==================
*/
static void CL_DrawDoubleVisionBlend (){

}

/*
 ==================
 
 ==================
*/
static void CL_DrawUnderwaterVisionBlend (){

}

/*
 ==================
 CL_DrawIRGogglesBlend
 ==================
*/
static void CL_DrawIRGogglesBlend (){

	if (cl.playerState->rdflags & RDF_IRGOGGLES){
		R_SetColor1(1.0f);
		R_DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.media.irGogglesMaterial);
	}
}

/*
 ==================
 CL_DrawViewBlends
 ==================
*/
void CL_DrawViewBlends (){

	if (!cl_viewBlend->integerValue || cl_thirdPerson->integerValue)
		return;

	// This is just the old poly blend
	if (cl_viewBlend->integerValue == 1){
		if (!cl.playerState->blend[3])
			return;

		R_SetColor4(cl.playerState->blend[0], cl.playerState->blend[1], cl.playerState->blend[2], cl.playerState->blend[3]);
		CL_FillRect(cl.renderView.x, cl.renderView.y, cl.renderView.width, cl.renderView.height, H_SCALE, 1.0f, V_SCALE, 1.0f);

		return;
	}

	// Fire screen
	CL_DrawFireScreenBlend();

	// Underwater blur
	CL_DrawWaterBlurBlend();

	// Double vision
	CL_DrawDoubleVisionBlend();

	// Underwater vision
	CL_DrawUnderwaterVisionBlend();

	// Draw IR Goggles
	CL_DrawIRGogglesBlend();
}


/*
 ==============================================================================

 2D ELEMENTS

 ==============================================================================
*/

#define	DISPLAY_ITEMS		17

#define STAT_MINUS			10	// Num frame for '-' stats digit
#define	CHAR_WIDTH			16


/*
 ==================
 
 ==================
*/
static void CL_DrawInventory (){

	int		number;
	int		selectedItem, selectedNumber;
	int		index[MAX_ITEMS];
	int		top;
	int		x, y;
	int		i;

	if (!cl_drawInventory->integerValue)
		return;

	if (!(cl.playerState->stats[STAT_LAYOUTS] & 2))
		return;

	// Get the selected item
	selectedItem = cl.playerState->stats[STAT_SELECTED_ITEM];

	number = 0;
	selectedNumber = 0;

	for (i = 0; i < MAX_ITEMS; i++){
		if (i == selectedItem)
			selectedNumber = number;

		if (cl.inventory[i]){
			index[number] = i;
			number++;
		}
	}

	// Determine the scroll point
	top = selectedNumber - DISPLAY_ITEMS/2;
	if (number - top < DISPLAY_ITEMS)
		top = number - DISPLAY_ITEMS;
	if (top < 0)
		top = 0;

	x = (cls.glConfig.videoWidth-256)/2;
	y = (cls.glConfig.videoHeight-240)/2;

	CL_DrawPicFixedByName(x, y, "inventory");

	// TODO!!!
}

/*
 ==================
 CL_DrawLayoutFieldNumber
 ==================
*/
static void CL_DrawLayoutFieldNumber (int x, int y, int color, int width, int value){

	char	number[16];
	char	*ptr;
	int		length;
	int		frame;

	if (width < 1)
		return;

	if (width > 5)
		width = 5;

	// Set the string
	Str_SPrintf(number, sizeof(number), "%i", value);

	length = Str_Length(number);
	if (length > width)
		length = width;

	x += 2 + CHAR_WIDTH * (width - length);

	// Draw each character
	ptr = number;
	while (*ptr && length){
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr - '0';

		CL_DrawPicFixed(x, y, cl.media.hudNumberMaterials[color][frame]);

		x += CHAR_WIDTH;
		ptr++;
		length--;
	}
}

/*
 ==================
 
 ==================
*/
static void CL_DrawLayoutString (const char *string, int x, int y, int centerWidth, int xor){

}

/*
 ==================

 ==================
*/
static void CL_ExecuteLayoutString (char *string){

	script_t		*script;
	token_t			token;
	clientInfo_t	*clientInfo;
	char			block[80];
	int				index, value, color;
	int				score, ping, time;
	int				x, y, width, height;

	if (!string[0])
		return;

	script = PS_LoadScriptMemory("LayoutString", string, Str_Length(string), 1);
	if (!script)
		return;

	PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES | SF_PARSEPRIMITIVES);

	x = 0;
	y = 0;

	while (string){
		if (!PS_ReadToken(script, &token))
			break;

		if (!Str_ICompare(token.string, "xl")){
			PS_ReadToken(script, &token);

			x = Str_ToInteger(token.string);

			continue;
		}

		if (!Str_ICompare(token.string, "xr")){
			PS_ReadToken(script, &token);

			x = cls.glConfig.videoWidth + Str_ToInteger(token.string);

			continue;
		}

		if (!Str_ICompare(token.string, "xv")){
			PS_ReadToken(script, &token);

			x = cls.glConfig.videoWidth/2 - 160 + Str_ToInteger(token.string);

			continue;
		}

		if (!Str_ICompare(token.string, "yt")){
			PS_ReadToken(script, &token);

			y = Str_ToInteger(token.string);

			continue;
		}

		if (!Str_ICompare(token.string, "yb")){
			PS_ReadToken(script, &token);

			y = cls.glConfig.videoHeight + Str_ToInteger(token.string);

			continue;
		}

		if (!Str_ICompare(token.string, "yv")){
			PS_ReadToken(script, &token);

			y = cls.glConfig.videoHeight/2 - 120 + Str_ToInteger(token.string);

			continue;
		}

		// Draw a deathmatch client block
		if (!Str_ICompare(token.string, "client")){
			PS_ReadToken(script, &token);
			x = cls.glConfig.videoWidth/2 - 160 + Str_ToInteger(token.string);
			PS_ReadToken(script, &token);
			y = cls.glConfig.videoHeight/2 - 120 + Str_ToInteger(token.string);

			PS_ReadToken(script, &token);

			index = Str_ToInteger(token.string);
			if (index < 0 || index >= MAX_CLIENTS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad 'client' (index %i)", index);

			clientInfo = &cl.clientInfo[index];
			if (!clientInfo->valid)
				clientInfo = &cl.baseClientInfo;

			PS_ReadToken(script, &token);
			score = Str_ToInteger(token.string);
			PS_ReadToken(script, &token);
			ping = Str_ToInteger(token.string);
			PS_ReadToken(script, &token);
			time = Str_ToInteger(token.string);

			CL_DrawPicFixed(x, y, clientInfo->icon);

			// TODO: CL_DrawStringFixed

			continue;
		}

		// Draw a CTF client block
		if (!Str_ICompare(token.string, "ctf")){
			PS_ReadToken(script, &token);
			x = cls.glConfig.videoWidth/2 - 160 + Str_ToInteger(token.string);
			PS_ReadToken(script, &token);
			y = cls.glConfig.videoHeight/2 - 120 + Str_ToInteger(token.string);

			PS_ReadToken(script, &token);

			index = Str_ToInteger(token.string);
			if (index < 0 || index >= MAX_CLIENTS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad 'ctf' (index %i)", index);

			clientInfo = &cl.clientInfo[index];
			if (!clientInfo->valid)
				clientInfo = &cl.baseClientInfo;

			PS_ReadToken(script, &token);
			score = Str_ToInteger(token.string);
			PS_ReadToken(script, &token);
			ping = Str_ToInteger(token.string);

			if (ping > 999)
				ping = 999;

			Str_SPrintf(block, sizeof(block), "%3d %3d %-12.12s", score, ping, clientInfo->name);

			// TODO: CL_DrawStringFixed

			continue;
		}

		// Draw a pic from a stat number
		if (!Str_ICompare(token.string, "pic")){
			PS_ReadToken(script, &token);

			index = Str_ToInteger(token.string);
			if (index < 0 || index >= MAX_IMAGES)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad 'pic' (index %i)", index);

			if (!cl.media.gameMaterials[index])
				continue;

			if (cl_drawIcons->integerValue)
				CL_DrawPicFixed(x, y, cl.media.gameMaterials[index]);

			continue;
		}

		// Draw a pic from a name
		if (!Str_ICompare(token.string, "picn")){
			PS_ReadToken(script, &token);

			if (cl_drawIcons->integerValue)
				CL_DrawPicFixedByName(x, y, token.string);

			continue;
		}

		// Draw a number
		if (!Str_ICompare(token.string, "num")){
			PS_ReadToken(script, &token);
			width = Str_ToInteger(token.string);
			PS_ReadToken(script, &token);
			height = Str_ToInteger(token.string);

			index = Str_ToInteger(token.string);
			if (index < 0 || index >= MAX_STATS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad 'num' (index %i)", index);

			CL_DrawLayoutFieldNumber(x, y, 0, width, cl.playerState->stats[index]);

			continue;
		}

		// Health number
		if (!Str_ICompare(token.string, "hnum")){
			value = cl.playerState->stats[STAT_HEALTH];
			if (value > 25)
				color = 0;
			else if (value > 0)
				color = (cl.frame.serverFrame >> 2) & 1;
			else
				color = 1;

			if (cl.playerState->stats[STAT_FLASHES] & 1)
				CL_DrawPicFixedByName(x, y, "field_3");

			CL_DrawLayoutFieldNumber(x, y, color, 3, value);

			continue;
		}

		// Armor number
		if (!Str_ICompare(token.string, "rnum")){
			value = cl.playerState->stats[STAT_ARMOR];
			if (value < 1)
				continue;

			if (cl.playerState->stats[STAT_FLASHES] & 2)
				CL_DrawPicFixedByName(x, y, "field_3");

			CL_DrawLayoutFieldNumber(x, y, 0, 3, value);

			continue;
		}

		// Ammo number
		if (!Str_ICompare(token.string, "anum")){
			value = cl.playerState->stats[STAT_AMMO];
			if (value > 5)
				color = 0;
			else if (value >= 0)
				color = (cl.frame.serverFrame >> 2) & 1;
			else
				continue;

			if (cl.playerState->stats[STAT_FLASHES] & 4)
				CL_DrawPicFixedByName(x, y, "field_3");

			CL_DrawLayoutFieldNumber(x, y, color, 3, value);

			continue;
		}

		// Status string
		if (!Str_ICompare(token.string, "stat_string")){
			PS_ReadToken(script, &token);

			index = Str_ToInteger(token.string);
			if (index < 0 || index >= MAX_STATS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad 'stat_string' (index %i)", index);

			index = cl.playerState->stats[index];
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad 'stat_string' (index %i)", index);

			// TODO: CL_DrawStringFixed

			continue;
		}

		// String
		if (!Str_ICompare(token.string, "string")){
			PS_ReadToken(script, &token);

			// TODO: CL_DrawStringFixed

			continue;
		}

		// String 2
		if (!Str_ICompare(token.string, "string2")){
			PS_ReadToken(script, &token);

			// TODO: CL_DrawStringFixed

			continue;
		}

		// Center string
		if (!Str_ICompare(token.string, "cstring")){
			PS_ReadToken(script, &token);

			CL_DrawLayoutString(token.string, x, y, SCREEN_WIDTH/2, 0);

			continue;
		}

		// Center string 2
		if (!Str_ICompare(token.string, "cstring2")){
			PS_ReadToken(script, &token);

			CL_DrawLayoutString(token.string, x, y, SCREEN_WIDTH/2, 0x80);

			continue;
		}

		// If and endif
		if (!Str_ICompare(token.string, "if")){
			PS_ReadToken(script, &token);

			index = Str_ToInteger(token.string);
			if (index < 0 || index >= MAX_STATS)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad 'if' index (index %i)", index);

			if (!cl.playerState->stats[index]){
				// Skip to endif
				while (string && Str_ICompare(token.string, "endif"))
					PS_ReadToken(script, &token);
			}

			continue;
		}
	}

	PS_FreeScript(script);
}

/*
 ==================
 CL_DrawStatus
 ==================
*/
static void CL_DrawStatus (){

	if (!cl_drawStatus->integerValue)
		return;

//	if (!cl.gameMod)
//		return;

	CL_ExecuteLayoutString(cl.configStrings[CS_STATUSBAR]);
}

/*
 ==================
 CL_DrawLayout
 ==================
*/
static void CL_DrawLayout (){

	if (!cl_drawLayout->integerValue)
		return;

	if (!(cl.playerState->stats[STAT_LAYOUTS] & 1))
		return;

	CL_ExecuteLayoutString(cl.layout);
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

	clientInfo_t	*clientInfo;
	int				crosshair, health;
	float			color[4], *fadeColor;
	float			x, y, w, h;

	if (!cl_drawCrosshair->integerValue)
		return;

	if ((cl.playerState->rdflags & RDF_IRGOGGLES) || cl_thirdPerson->integerValue)
		return;

	// Select the crosshair
	crosshair = (cl_drawCrosshair->integerValue - 1) % NUM_CROSSHAIRS;
	if (crosshair < 0)
		return;

	// Set dimensions and position
	w = cl_crosshairSize->integerValue;
	h = cl_crosshairSize->integerValue;

	x = cl_crosshairX->integerValue + ((SCREEN_WIDTH - w) * 0.5f);
	y = cl_crosshairY->integerValue + ((SCREEN_HEIGHT- h) * 0.5f);

	// Set color and alpha
	if (cl_crosshairHealth->integerValue){
		health = cl.playerState->stats[STAT_HEALTH];

		// TODO!!!
	}
	else {
		color[0] = color_table[cl_crosshairColor->integerValue & COLOR_MASK][0];
		color[1] = color_table[cl_crosshairColor->integerValue & COLOR_MASK][1];
		color[2] = color_table[cl_crosshairColor->integerValue & COLOR_MASK][2];
		color[3] = 1.0f * ClampFloat(cl_crosshairAlpha->floatValue, 0.0f, 1.0f);
	}

	// Draw it
	R_SetColor(color);
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_CENTER, 1.0f, cl.media.crosshairMaterials[crosshair]);

	// Draw the target name
	if (cl_crosshairNames->integerValue){
		if (!cl.multiPlayer)
			return;		// Don't bother in singleplayer

		// Scan for a player entity
		CL_ScanForPlayerEntity();

		if (!cl.crosshairEntTime || !cl.crosshairEntNumber)
			return;

		clientInfo = &cl.clientInfo[cl.crosshairEntNumber - 1];
		if (!clientInfo->valid)
			return;

		// Set color and alpha
		if (cl_crosshairHealth->integerValue){
			color[0] = color_table[cl_crosshairColor->integerValue & COLOR_MASK][0];
			color[1] = color_table[cl_crosshairColor->integerValue & COLOR_MASK][1];
			color[2] = color_table[cl_crosshairColor->integerValue & COLOR_MASK][2];
		}

		fadeColor = CL_FadeAlpha(color, cl.crosshairEntTime, 1000, 250);
		if (!fadeColor){
			cl.crosshairEntTime = 0;
			cl.crosshairEntNumber = 0;
			return;
		}

		// Draw it
	}
}

/*
 ==================
 CL_DrawCenterString
 ==================
*/
static void CL_DrawCenterString (){

	float	*fadeColor;
	char	string[MAX_STRING_LENGTH];
	int		length;

	if (!cl_drawCenterString->integerValue)
		return;

	fadeColor = CL_FadeAlpha(colorWhite, cl.centerPrintTime, cl_centerTime->integerValue, cl_centerTime->integerValue / 4);
	if (!fadeColor)
		return;

	// Set the string
	length = Str_SPrintf(string, sizeof(string), "%s", cl.centerPrint);

	// Draw it
	R_DrawString((SCREEN_WIDTH - length * 10.0f) * 0.5f, SCREEN_HEIGHT - 320.0f, 10.0f, 10.0f, string, fadeColor, true, 1.0f, 2.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
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
 CL_DrawPause
 ==================
*/
static void CL_DrawPause (){

	if (!cl_drawPause->integerValue)
		return;

	if (!com_paused->integerValue || UI_IsVisible())
		return;

	// Draw it
	R_SetColor(colorWhite);
	R_DrawStretchPic(0.0f, SCREEN_HEIGHT - 260.0f, SCREEN_WIDTH, 40.0f, 0.0f, 0.0f, 1.0f, 1.0f, H_STRETCH_WIDTH, 1.0f, V_ALIGN_CENTER, 1.0f, cl.media.pauseMaterial);
}

/*
 ==================
 CL_DrawLogo
 ==================
*/
static void CL_DrawLogo (){

	if (!cl_drawLogo->integerValue)
		return;

	// Draw it
	R_SetColor(colorWhite);
	R_DrawStretchPic(2.0f, 412.0f, 264.0f, 66.0f, 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_BOTTOM, 1.0f, cl.media.logoMaterial);
}

/*
 ==================
 CL_DrawMaterial

 TODO: would be nice if we could trace for monsters, objects materials
 ==================
*/
static void CL_DrawMaterial (){

	trace_t	trace;
	vec3_t	start, end;
	char	string[512];
	float	ofs;

	if (!cl_showMaterial->integerValue)
		return;

	// Set up the trace
	VectorCopy(cl.renderView.origin, start);
	VectorMA(cl.renderView.origin, 8192.0f, cl.renderView.axis[0], end);

	if (cl_showMaterial->integerValue == 2)
		trace = CL_Trace(start, vec3_origin, vec3_origin, end, cl.clientNum, MASK_ALL, true, NULL);
	else
		trace = CL_Trace(start, vec3_origin, vec3_origin, end, cl.clientNum, MASK_SOLID | MASK_WATER, true, NULL);

	if (trace.fraction == 0.0f || trace.fraction == 1.0f)
		return;

	// Material
	ofs = 120.0f;

	R_DrawString(5.0f, ofs, 5.0f, 10.0f, "MATERIAL", colorGreen, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
	ofs += 10.0f;

	Str_SPrintf(string, sizeof(string), "textures/%s", trace.surface->name);
	R_DrawString(5.0f, ofs, 5.0f, 10.0f, string, colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);

	ofs += 10.0f;

	// Surface flags
	ofs += 5.0f;

	R_DrawString(5.0f, ofs, 5.0f, 10.0f, "SURFACE FLAGS", colorGreen, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
	ofs += 10.0f;

	if (trace.surface->flags){
		if (trace.surface->flags & SURF_LIGHT){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "light", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.surface->flags & SURF_SLICK){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "slick", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.surface->flags & SURF_SKY){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "sky", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}	 
		if (trace.surface->flags & SURF_WARP){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "warp", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.surface->flags & SURF_TRANS33){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "trans33", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.surface->flags & SURF_TRANS66){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "trans66", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.surface->flags & SURF_FLOWING){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "flowing", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.surface->flags & SURF_NODRAW){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "noDraw", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
	}
	else
		ofs += 10.0f;

	// Content flags
	ofs += 5.0f;

	R_DrawString(5.0f, ofs, 5.0f, 10.0f, "CONTENT FLAGS", colorGreen, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
	ofs += 10.0f;

	if (trace.contents){
		if (trace.contents & CONTENTS_SOLID){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "solid", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_WINDOW){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "window", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_AUX){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "aux", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_LAVA){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "lava", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_SLIME){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "slime", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_WATER){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "water", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_MIST){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "mist", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}

		if (trace.contents & CONTENTS_AREAPORTAL){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "areaPortal", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_PLAYERCLIP){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "playerClip", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_MONSTERCLIP){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "monsterClip", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}

		if (trace.contents & CONTENTS_CURRENT_0){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "current_0", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_CURRENT_90){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "current_90", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_CURRENT_180){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "current_180", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_CURRENT_270){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "current_270", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_CURRENT_UP){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "current_up", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_CURRENT_DOWN){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "current_down", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}

		if (trace.contents & CONTENTS_ORIGIN){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "origin", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_MONSTER){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "monster", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_DEADMONSTER){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "deadMonster", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_DETAIL){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "detail", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_TRANSLUCENT){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "translucent", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
		if (trace.contents & CONTENTS_LADDER){
			R_DrawString(5.0f, ofs, 5.0f, 10.0f, "ladder", colorWhite, true, 1.0f, 1.0f, H_ALIGN_LEFT, 1.0f, V_ALIGN_TOP, 1.0f, cls.media.charsetMaterial);
			ofs += 10.0f;
		}
	}
	else
		ofs += 10.0f;
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

	// Draw the promotion logo
	CL_DrawLogo();

	// Draw the material under the crosshair
	CL_DrawMaterial();
}