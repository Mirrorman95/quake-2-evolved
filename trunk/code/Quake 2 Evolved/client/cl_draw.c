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
 CL_DrawString
 ==================
*/
void CL_DrawString (float x, float y, float w, float h, const char *string, const vec4_t color, bool forceColor, float xShadow, float yShadow, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent, material_t *material){

	R_DrawString(x, y, w, h, string, color, forceColor, xShadow, yShadow, horzAdjust, horzPercent, vertAdjust, vertPercent, material);
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

 TODO: why does this not scale to screen dimensions?
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
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_SCALE, 1.0f, V_SCALE, 1.0f, material);
}

/*
 ==================
 
 FIXME: x and y coords are way off
 ==================
*/
void CL_DrawPicFixed (float x, float y, material_t *material){

	float	w, h;

	R_GetPicSize(material, &w, &h);

	R_SetColor(colorWhite);
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_SCALE, 1.0f, V_SCALE, 1.0f, material);
}

/*
 ==================
 
 FIXME: does not show up on screen for some reason...
 ==================
*/
void CL_DrawPicFixedByName (float x, float y, const char *pic){

	material_t	*material;
	char		name[MAX_OSPATH];
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
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_SCALE, 1.0f, V_SCALE, 1.0f, material);
}


// ============================================================================


/*
 ==================
 
 TODO: some strings uses \n and \r which messes up the string
 TODO: fix position
 ==================
*/
void CL_DrawLoading (){

	char	string[MAX_STRING_LENGTH];
	float	speed;
	int		percent;
	int		length;

	if (!cls.loading)
		return;

	switch (cls.state){
	case CA_DISCONNECTED:
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, "ui/assets/title_screen/title_backg");
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, "ui/assets/title_screen/q2e_logo");

		break;
	case CA_CONNECTING:
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, "ui/assets/title_screen/title_backg");
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, "ui/assets/title_screen/q2e_logo");

		if (NET_IsLocalAddress(cls.serverAddress)){
			length = Str_SPrintf(string, sizeof(string), "Starting up...");
			CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 56.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
		}
		else {
			if (cls.serverMessage[0]){
				length = Str_SPrintf(string, sizeof(string), "%s", cls.serverMessage[0]);
				CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 120.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
			}

			length = Str_SPrintf(string, sizeof(string), "Connecting to %s\nAwaiting connection... %i", cls.serverName, cls.connectCount);
			CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 72.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
		}

		break;
	case CA_CHALLENGING:
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, "ui/assets/title_screen/title_backg");
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, "ui/assets/title_screen/q2e_logo");

		if (NET_IsLocalAddress(cls.serverAddress)){
			length = Str_SPrintf(string, sizeof(string), "Starting up...");
			CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 56.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
		}
		else {
			if (cls.serverMessage[0]){
				length = Str_SPrintf(string, sizeof(string), "%s", cls.serverMessage[0]);
				CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 120.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
			}

			length = Str_SPrintf(string, sizeof(string), "Connecting to %s\nAwaiting challenge... %i", cls.serverName, cls.connectCount);
			CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 72.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
		}

		break;
	case CA_CONNECTED:
		if (cls.downloadFile){
			CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, "ui/assets/title_screen/title_backg");
			CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, "ui/assets/title_screen/q2e_logo");

			if (cls.downloadStart != cls.realTime)
				speed = (float)(cls.downloadBytes / 1024) / ((cls.realTime - cls.downloadStart) / 1000);
			else
				speed = 0;

			if (Com_ServerState()){
				length = Str_SPrintf(string, sizeof(string), "Downloading %s... (%i%% @ %.2f KB/sec)", cls.downloadName, cls.downloadPercent, speed);
				CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 56.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
			}
			else {
				length = Str_SPrintf(string, sizeof(string), "Connecting to %s\nDownloading %s... (%i%% @ %.2f KB/sec)", cls.serverName, cls.downloadName, cls.downloadPercent, speed);
				CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 72.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
			}

			percent = ClampInt(cls.downloadPercent - (cls.downloadPercent % 5), 5, 100);
			if (percent){
				CL_DrawPicByName(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, "ui/assets/loading/load_main2");
				CL_DrawPicByName(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, Str_VarArgs("ui/assets/loading/percent/load_%i", percent));
				CL_DrawPicByName(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, "ui/assets/loading/load_main");
			}

			break;
		}

		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, "ui/assets/title_screen/title_backg");
		CL_DrawPicByName(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, "ui/assets/title_screen/q2e_logo");

		if (NET_IsLocalAddress(cls.serverAddress)){
			length = Str_SPrintf(string, sizeof(string), "Starting up...");
			CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 56.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
		}
		else {
			length = Str_SPrintf(string, sizeof(string), "Connecting to %s\nAwaiting game state...", cls.serverName);
			CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 72.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
		}

		break;
	case CA_LOADING:
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshot);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshotDetail);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, cl.media.loadingLogo);

		if (NET_IsLocalAddress(cls.serverAddress)){
			length = Str_SPrintf(string, sizeof(string), "Loading %s\n\"%s\"\n\n\nLoading... %s\n", cls.loadingInfo.map, cls.loadingInfo.name, cls.loadingInfo.string);
			CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 180.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
		}
		else {
			length = Str_SPrintf(string, sizeof(string), "Loading %s\n\"%s\"\n\nConnecting to %s\nLoading... %s\n", cls.loadingInfo.map, cls.loadingInfo.name, cls.serverName, cls.loadingInfo.string);
			CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 180.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);
		}

		percent = ClampInt((cls.loadingInfo.percent / 5) - 1, 0, 19);
		if (percent){
			CL_DrawPic(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, cl.media.loadingDetail[0]);
			CL_DrawPic(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, cl.media.loadingPercent[percent]);
			CL_DrawPic(240.0f, 160.0f, 160.0f, 160.0f, colorWhite, cl.media.loadingDetail[1]);
		}

		break;
	case CA_PRIMED:
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshot);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshotDetail);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, cl.media.loadingLogo);

		break;
	case CA_ACTIVE:
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshot);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, colorWhite, cl.media.levelshotDetail);
		CL_DrawPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT - 320.0f, colorWhite, cl.media.loadingLogo);

		length = Str_SPrintf(string, sizeof(string), "Awaiting frame...");
		CL_DrawString((SCREEN_WIDTH - length * 16.0f) * 0.5f, SCREEN_HEIGHT - 56.0f, 16.0f, 16.0f, string, colorWhite, false, 1.0f, 2.0f, H_NONE, 1.0f, V_NONE, 1.0f, cls.media.charsetMaterial);

		break;
	}
}


// ============================================================================

#define STAT_MINUS			10	// Num frame for '-' stats digit
#define	CHAR_WIDTH			16


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
static void CL_DrawLayoutString (){

}

/*
 ==================
 
 ==================
*/
static void CL_ExecuteLayoutString (char *string){

	script_t	*script;
	token_t		token;
	int			width;
	int			index;
	int			x, y;

	if (!string[0])
		return;

	script = PS_LoadScriptMemory("LayoutString", string, Str_Length(string), 1);
	if (!script)
		return;

	PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

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

			continue;
		}

		// Draw a CTF client block
		if (!Str_ICompare(token.string, "ctf")){
			PS_ReadToken(script, &token);

			continue;
		}

		// Draw a pic from a stat number
		if (!Str_ICompare(token.string, "pic")){
			PS_ReadToken(script, &token);

			index = Str_ToInteger(token.string);
			if (index < 0 || index >= MAX_IMAGES)
				Com_Error(ERR_DROP, "CL_ExecuteLayoutString: bad pic index %i", index);

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

			continue;
		}

		// Health number
		if (!Str_ICompare(token.string, "hnum")){
			PS_ReadToken(script, &token);

			continue;
		}

		// Armor number
		if (!Str_ICompare(token.string, "rnum")){
			PS_ReadToken(script, &token);

			continue;
		}

		// Ammo number
		if (!Str_ICompare(token.string, "anum")){
			PS_ReadToken(script, &token);

			continue;
		}

		if (!Str_ICompare(token.string, "stat_string")){
			PS_ReadToken(script, &token);

			continue;
		}

		if (!Str_ICompare(token.string, "string")){
			PS_ReadToken(script, &token);

			continue;
		}

		if (!Str_ICompare(token.string, "string2")){
			PS_ReadToken(script, &token);

			continue;
		}

		if (!Str_ICompare(token.string, "cstring")){
			PS_ReadToken(script, &token);

			continue;
		}

		if (!Str_ICompare(token.string, "cstring2")){
			PS_ReadToken(script, &token);

			continue;
		}

		if (!Str_ICompare(token.string, "if")){
			PS_ReadToken(script, &token);

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
 
 ==================
*/
static void CL_DrawPause (){

	if (!cl_drawPause->integerValue)
		return;

	// Draw it
	R_SetColor(colorWhite);
	R_DrawStretchPic(0.0f, SCREEN_HEIGHT - 260.0f, 0.0f, 40.0f, 0.0f, 0.0f, 1.0f, 1.0f, H_ALIGN_CENTER, 1.0f, V_ALIGN_CENTER, 1.0f, cl.media.pauseMaterial);
}

/*
 ==================
 CL_DrawMaterial
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

	// Draw the material under the crosshair
	CL_DrawMaterial();
}