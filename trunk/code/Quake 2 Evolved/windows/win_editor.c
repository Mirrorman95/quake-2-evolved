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
// win_editor.c - Editors used by varius modules
//


#include "../renderer/r_local.h"
#include "../common/editor.h"
#include "win_local.h"


/*
 ==============================================================================

 INTEGRATED POST-PROCESSING EDITOR

 ==============================================================================
*/

#ifdef _WIN32

#define POST_PROCESS_EDITOR_WINDOW_NAME		ENGINE_NAME " Post-Process Editor"
#define POST_PROCESS_EDITOR_WINDOW_CLASS	ENGINE_NAME " Post-Process Editor"
#define POST_PROCESS_EDITOR_WINDOW_STYLE	(WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

static postProcessParms_t	win_postProcessParms = {1.0f, 0.5f, 1.0f, 1.0f, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0}, "_default"};

typedef struct {
	bool					initialized;
	bool					enabled;

	int						editIndex;
	postProcessParms_t		editParms;

	// Window stuff
	HINSTANCE				hInstance;

	HWND					hWnd;
	HWND					hWndArea;
	HWND					hWndAreaValue;
	HWND					hWndApplyToAll;
	HWND					hWndBloom;
	HWND					hWndBloomContrast;
	HWND					hWndBloomContrastValue;
	HWND					hWndBloomContrastSpin;
	HWND					hWndBloomThreshold;
	HWND					hWndBloomThresholdValue;
	HWND					hWndBloomThresholdSpin;
	HWND					hWndBaseIntensity;
	HWND					hWndBaseIntensityValue;
	HWND					hWndBaseIntensitySpin;
	HWND					hWndGlowIntensity;
	HWND					hWndGlowIntensityValue;
	HWND					hWndGlowIntensitySpin;
	HWND					hWndColorCorrection;
	HWND					hWndR;
	HWND					hWndG;
	HWND					hWndB;
	HWND					hWndColorShadows;
	HWND					hWndColorShadowsRValue;
	HWND					hWndColorShadowsRSpin;
	HWND					hWndColorShadowsGValue;
	HWND					hWndColorShadowsGSpin;
	HWND					hWndColorShadowsBValue;
	HWND					hWndColorShadowsBSpin;
	HWND					hWndColorHighlights;
	HWND					hWndColorHighlightsRValue;
	HWND					hWndColorHighlightsRSpin;
	HWND					hWndColorHighlightsGValue;
	HWND					hWndColorHighlightsGSpin;
	HWND					hWndColorHighlightsBValue;
	HWND					hWndColorHighlightsBSpin;
	HWND					hWndColorMidtones;
	HWND					hWndColorMidtonesRValue;
	HWND					hWndColorMidtonesRSpin;
	HWND					hWndColorMidtonesGValue;
	HWND					hWndColorMidtonesGSpin;
	HWND					hWndColorMidtonesBValue;
	HWND					hWndColorMidtonesBSpin;
	HWND					hWndColorMinOutput;
	HWND					hWndColorMinOutputRValue;
	HWND					hWndColorMinOutputRSpin;
	HWND					hWndColorMinOutputGValue;
	HWND					hWndColorMinOutputGSpin;
	HWND					hWndColorMinOutputBValue;
	HWND					hWndColorMinOutputBSpin;
	HWND					hWndColorMaxOutput;
	HWND					hWndColorMaxOutputRValue;
	HWND					hWndColorMaxOutputRSpin;
	HWND					hWndColorMaxOutputGValue;
	HWND					hWndColorMaxOutputGSpin;
	HWND					hWndColorMaxOutputBValue;
	HWND					hWndColorMaxOutputBSpin;
	HWND					hWndColorSaturation;
	HWND					hWndColorSaturationRValue;
	HWND					hWndColorSaturationRSpin;
	HWND					hWndColorSaturationGValue;
	HWND					hWndColorSaturationGSpin;
	HWND					hWndColorSaturationBValue;
	HWND					hWndColorSaturationBSpin;
	HWND					hWndColorTint;
	HWND					hWndColorTintRValue;
	HWND					hWndColorTintRSpin;
	HWND					hWndColorTintGValue;
	HWND					hWndColorTintGSpin;
	HWND					hWndColorTintBValue;
	HWND					hWndColorTintBSpin;
	HWND					hWndColorTable;
	HWND					hWndColorTableValue;
	HWND					hWndApply;
	HWND					hWndReset;
	HWND					hWndDefaults;
	HWND					hWndSave;

	HFONT					hFont;
} postProcessEditor_t;

static postProcessEditor_t	win_postProcessEditor;


/*
 ==================
 WIN_ApplyPostProcessParameters
 ==================
*/
static void WIN_ApplyPostProcessParameters (bool all){

	postProcessParms_t	parms;
	char				string[64];
	int					i;

	if (!win_postProcessEditor.enabled)
		return;

	// Read the controls
	if (GetWindowText(win_postProcessEditor.hWndBloomContrastValue, string, sizeof(string)))
		parms.bloomContrast = Str_ToFloat(string);
	else
		parms.bloomContrast = win_postProcessEditor.editParms.bloomContrast;

	if (GetWindowText(win_postProcessEditor.hWndBloomThresholdValue, string, sizeof(string)))
		parms.bloomThreshold = Str_ToFloat(string);
	else
		parms.bloomThreshold = win_postProcessEditor.editParms.bloomThreshold;

	if (GetWindowText(win_postProcessEditor.hWndBaseIntensityValue, string, sizeof(string)))
		parms.baseIntensity = Str_ToFloat(string);
	else
		parms.baseIntensity = win_postProcessEditor.editParms.baseIntensity;

	if (GetWindowText(win_postProcessEditor.hWndGlowIntensityValue, string, sizeof(string)))
		parms.glowIntensity = Str_ToFloat(string);
	else
		parms.glowIntensity = win_postProcessEditor.editParms.glowIntensity;

	if (GetWindowText(win_postProcessEditor.hWndColorShadowsRValue, string, sizeof(string)))
		parms.colorShadows[0] = Str_ToFloat(string);
	else
		parms.colorShadows[0] = win_postProcessEditor.editParms.colorShadows[0];

	if (GetWindowText(win_postProcessEditor.hWndColorShadowsGValue, string, sizeof(string)))
		parms.colorShadows[1] = Str_ToFloat(string);
	else
		parms.colorShadows[1] = win_postProcessEditor.editParms.colorShadows[1];

	if (GetWindowText(win_postProcessEditor.hWndColorShadowsBValue, string, sizeof(string)))
		parms.colorShadows[2] = Str_ToFloat(string);
	else
		parms.colorShadows[2] = win_postProcessEditor.editParms.colorShadows[2];

	if (GetWindowText(win_postProcessEditor.hWndColorHighlightsRValue, string, sizeof(string)))
		parms.colorHighlights[0] = Str_ToFloat(string);
	else
		parms.colorHighlights[0] = win_postProcessEditor.editParms.colorHighlights[0];

	if (GetWindowText(win_postProcessEditor.hWndColorHighlightsGValue, string, sizeof(string)))
		parms.colorHighlights[1] = Str_ToFloat(string);
	else
		parms.colorHighlights[1] = win_postProcessEditor.editParms.colorHighlights[1];

	if (GetWindowText(win_postProcessEditor.hWndColorHighlightsBValue, string, sizeof(string)))
		parms.colorHighlights[2] = Str_ToFloat(string);
	else
		parms.colorHighlights[2] = win_postProcessEditor.editParms.colorHighlights[2];

	if (GetWindowText(win_postProcessEditor.hWndColorMidtonesRValue, string, sizeof(string)))
		parms.colorMidtones[0] = Str_ToFloat(string);
	else
		parms.colorMidtones[0] = win_postProcessEditor.editParms.colorMidtones[0];

	if (GetWindowText(win_postProcessEditor.hWndColorMidtonesGValue, string, sizeof(string)))
		parms.colorMidtones[1] = Str_ToFloat(string);
	else
		parms.colorMidtones[1] = win_postProcessEditor.editParms.colorMidtones[1];

	if (GetWindowText(win_postProcessEditor.hWndColorMidtonesBValue, string, sizeof(string)))
		parms.colorMidtones[2] = Str_ToFloat(string);
	else
		parms.colorMidtones[2] = win_postProcessEditor.editParms.colorMidtones[2];

	if (GetWindowText(win_postProcessEditor.hWndColorMinOutputRValue, string, sizeof(string)))
		parms.colorMinOutput[0] = Str_ToFloat(string);
	else
		parms.colorMinOutput[0] = win_postProcessEditor.editParms.colorMinOutput[0];

	if (GetWindowText(win_postProcessEditor.hWndColorMinOutputGValue, string, sizeof(string)))
		parms.colorMinOutput[1] = Str_ToFloat(string);
	else
		parms.colorMinOutput[1] = win_postProcessEditor.editParms.colorMinOutput[1];

	if (GetWindowText(win_postProcessEditor.hWndColorMinOutputBValue, string, sizeof(string)))
		parms.colorMinOutput[2] = Str_ToFloat(string);
	else
		parms.colorMinOutput[2] = win_postProcessEditor.editParms.colorMinOutput[2];

	if (GetWindowText(win_postProcessEditor.hWndColorMaxOutputRValue, string, sizeof(string)))
		parms.colorMaxOutput[0] = Str_ToFloat(string);
	else
		parms.colorMaxOutput[0] = win_postProcessEditor.editParms.colorMaxOutput[0];

	if (GetWindowText(win_postProcessEditor.hWndColorMaxOutputGValue, string, sizeof(string)))
		parms.colorMaxOutput[1] = Str_ToFloat(string);
	else
		parms.colorMaxOutput[1] = win_postProcessEditor.editParms.colorMaxOutput[1];

	if (GetWindowText(win_postProcessEditor.hWndColorMaxOutputBValue, string, sizeof(string)))
		parms.colorMaxOutput[2] = Str_ToFloat(string);
	else
		parms.colorMaxOutput[2] = win_postProcessEditor.editParms.colorMaxOutput[2];

	if (GetWindowText(win_postProcessEditor.hWndColorSaturationRValue, string, sizeof(string)))
		parms.colorSaturation[0] = Str_ToFloat(string);
	else
		parms.colorSaturation[0] = win_postProcessEditor.editParms.colorSaturation[0];

	if (GetWindowText(win_postProcessEditor.hWndColorSaturationGValue, string, sizeof(string)))
		parms.colorSaturation[1] = Str_ToFloat(string);
	else
		parms.colorSaturation[1] = win_postProcessEditor.editParms.colorSaturation[1];

	if (GetWindowText(win_postProcessEditor.hWndColorSaturationBValue, string, sizeof(string)))
		parms.colorSaturation[2] = Str_ToFloat(string);
	else
		parms.colorSaturation[2] = win_postProcessEditor.editParms.colorSaturation[2];

	if (GetWindowText(win_postProcessEditor.hWndColorTintRValue, string, sizeof(string)))
		parms.colorTint[0] = Str_ToFloat(string);
	else
		parms.colorTint[0] = win_postProcessEditor.editParms.colorTint[0];

	if (GetWindowText(win_postProcessEditor.hWndColorTintGValue, string, sizeof(string)))
		parms.colorTint[1] = Str_ToFloat(string);
	else
		parms.colorTint[1] = win_postProcessEditor.editParms.colorTint[1];

	if (GetWindowText(win_postProcessEditor.hWndColorTintBValue, string, sizeof(string)))
		parms.colorTint[2] = Str_ToFloat(string);
	else
		parms.colorTint[2] = win_postProcessEditor.editParms.colorTint[2];

	if (!GetWindowText(win_postProcessEditor.hWndColorTableValue, parms.colorTableName, sizeof(parms.colorTableName)))
		Str_Copy(parms.colorTableName, "_default", sizeof(parms.colorTableName));

	// Update the parameters
	if (!all){
		R_PostProcessEditorUpdateCallback(win_postProcessEditor.editIndex, &parms);
		return;
	}

	for (i = 0; i < CM_NumAreas(); i++)
		R_PostProcessEditorUpdateCallback(i, &parms);
}

/*
 ==================
 WIN_ResetPostProcessParameters
 ==================
*/
static void WIN_ResetPostProcessParameters (postProcessParms_t *parms){

	if (!win_postProcessEditor.enabled)
		return;

	// Update the controls
	SetWindowText(win_postProcessEditor.hWndBloomContrastValue, Str_FromFloat(parms->bloomContrast, -1));
	SetWindowText(win_postProcessEditor.hWndBloomThresholdValue, Str_FromFloat(parms->bloomThreshold, -1));
	SetWindowText(win_postProcessEditor.hWndBaseIntensityValue, Str_FromFloat(parms->baseIntensity, -1));
	SetWindowText(win_postProcessEditor.hWndGlowIntensityValue, Str_FromFloat(parms->glowIntensity, -1));

	SetWindowText(win_postProcessEditor.hWndColorShadowsRValue, Str_FromFloat(parms->colorShadows[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorShadowsGValue, Str_FromFloat(parms->colorShadows[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorShadowsBValue, Str_FromFloat(parms->colorShadows[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorHighlightsRValue, Str_FromFloat(parms->colorHighlights[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorHighlightsGValue, Str_FromFloat(parms->colorHighlights[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorHighlightsBValue, Str_FromFloat(parms->colorHighlights[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorMidtonesRValue, Str_FromFloat(parms->colorMidtones[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorMidtonesGValue, Str_FromFloat(parms->colorMidtones[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorMidtonesBValue, Str_FromFloat(parms->colorMidtones[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorMinOutputRValue, Str_FromFloat(parms->colorMinOutput[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorMinOutputGValue, Str_FromFloat(parms->colorMinOutput[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorMinOutputBValue, Str_FromFloat(parms->colorMinOutput[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorMaxOutputRValue, Str_FromFloat(parms->colorMaxOutput[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorMaxOutputGValue, Str_FromFloat(parms->colorMaxOutput[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorMaxOutputBValue, Str_FromFloat(parms->colorMaxOutput[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorSaturationRValue, Str_FromFloat(parms->colorSaturation[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorSaturationGValue, Str_FromFloat(parms->colorSaturation[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorSaturationBValue, Str_FromFloat(parms->colorSaturation[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorTintRValue, Str_FromFloat(parms->colorTint[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorTintGValue, Str_FromFloat(parms->colorTint[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorTintBValue, Str_FromFloat(parms->colorTint[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorTableValue, parms->colorTableName);

	// Update the parameters
	R_PostProcessEditorUpdateCallback(win_postProcessEditor.editIndex, parms);
}

/*
 ==================
 WIN_PostProcessEditorWindowProc
 ==================
*/
static LRESULT CALLBACK WIN_PostProcessEditorWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	char	string[64];
	int		result;

	switch (uMsg){
	case WM_CLOSE:
		WIN_ApplyPostProcessParameters(false);

		R_PostProcessEditorCloseCallback();

		break;
	case WM_SYSCOMMAND:
		if (wParam == SC_KEYMENU)
			return 0;

		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED){
			if ((HWND)lParam == win_postProcessEditor.hWndApplyToAll){
				WIN_ApplyPostProcessParameters(true);
				break;
			}

			if ((HWND)lParam == win_postProcessEditor.hWndApply){
				WIN_ApplyPostProcessParameters(false);
				break;
			}

			if ((HWND)lParam == win_postProcessEditor.hWndReset){
				WIN_ResetPostProcessParameters(&win_postProcessEditor.editParms);
				break;
			}

			if ((HWND)lParam == win_postProcessEditor.hWndDefaults){
				WIN_ResetPostProcessParameters(&win_postProcessParms);
				break;
			}

			if ((HWND)lParam == win_postProcessEditor.hWndSave){
				WIN_ApplyPostProcessParameters(false);

				R_PostProcessEditorSaveCallback();

				break;
			}
		}

		if (HIWORD(wParam) == CBN_SELCHANGE){
			if ((HWND)lParam == win_postProcessEditor.hWndAreaValue){
				result = SendMessage(win_postProcessEditor.hWndAreaValue, CB_GETCURSEL, 0, 0) - 1;

				if (result < -1 || result >= CM_NumAreas())
					break;

				R_EditAreaPostProcess(result);

				break;
			}
		}

		break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == UDN_DELTAPOS){
			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndBloomContrastSpin){
				GetWindowText(win_postProcessEditor.hWndBloomContrastValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndBloomContrastValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 4.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndBloomContrastValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 4.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndBloomThresholdSpin){
				GetWindowText(win_postProcessEditor.hWndBloomThresholdValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndBloomThresholdValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndBloomThresholdValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndBaseIntensitySpin){
				GetWindowText(win_postProcessEditor.hWndBaseIntensityValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndBaseIntensityValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 4.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndBaseIntensityValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 4.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndGlowIntensitySpin){
				GetWindowText(win_postProcessEditor.hWndGlowIntensityValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndGlowIntensityValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 4.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndGlowIntensityValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 4.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorShadowsRSpin){
				GetWindowText(win_postProcessEditor.hWndColorShadowsRValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorShadowsRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorShadowsRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorShadowsGSpin){
				GetWindowText(win_postProcessEditor.hWndColorShadowsGValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorShadowsGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorShadowsGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorShadowsBSpin){
				GetWindowText(win_postProcessEditor.hWndColorShadowsBValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorShadowsBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorShadowsBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorHighlightsRSpin){
				GetWindowText(win_postProcessEditor.hWndColorHighlightsRValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorHighlightsRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorHighlightsRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorHighlightsGSpin){
				GetWindowText(win_postProcessEditor.hWndColorHighlightsGValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorHighlightsGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorHighlightsGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorHighlightsBSpin){
				GetWindowText(win_postProcessEditor.hWndColorHighlightsBValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorHighlightsBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorHighlightsBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorMidtonesRSpin){
				GetWindowText(win_postProcessEditor.hWndColorMidtonesRValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorMidtonesRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.1f, 9.99f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorMidtonesRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.1f, 9.99f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorMidtonesGSpin){
				GetWindowText(win_postProcessEditor.hWndColorMidtonesGValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorMidtonesGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.1f, 9.99f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorMidtonesGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.1f, 9.99f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorMidtonesBSpin){
				GetWindowText(win_postProcessEditor.hWndColorMidtonesBValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorMidtonesBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.1f, 9.99f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorMidtonesBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.1f, 9.99f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorMinOutputRSpin){
				GetWindowText(win_postProcessEditor.hWndColorMinOutputRValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorMinOutputRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorMinOutputRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorMinOutputGSpin){
				GetWindowText(win_postProcessEditor.hWndColorMinOutputGValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorMinOutputGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorMinOutputGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorMinOutputBSpin){
				GetWindowText(win_postProcessEditor.hWndColorMinOutputBValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorMinOutputBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorMinOutputBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorMaxOutputRSpin){
				GetWindowText(win_postProcessEditor.hWndColorMaxOutputRValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorMaxOutputRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorMaxOutputRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorMaxOutputGSpin){
				GetWindowText(win_postProcessEditor.hWndColorMaxOutputGValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorMaxOutputGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorMaxOutputGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorMaxOutputBSpin){
				GetWindowText(win_postProcessEditor.hWndColorMaxOutputBValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorMaxOutputBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorMaxOutputBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorSaturationRSpin){
				GetWindowText(win_postProcessEditor.hWndColorSaturationRValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorSaturationRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 4.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorSaturationRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 4.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorSaturationGSpin){
				GetWindowText(win_postProcessEditor.hWndColorSaturationGValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorSaturationGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 4.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorSaturationGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 4.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorSaturationBSpin){
				GetWindowText(win_postProcessEditor.hWndColorSaturationBValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorSaturationBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 4.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorSaturationBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 4.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorTintRSpin){
				GetWindowText(win_postProcessEditor.hWndColorTintRValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorTintRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorTintRValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorTintGSpin){
				GetWindowText(win_postProcessEditor.hWndColorTintGValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorTintGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorTintGValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_postProcessEditor.hWndColorTintBSpin){
				GetWindowText(win_postProcessEditor.hWndColorTintBValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_postProcessEditor.hWndColorTintBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_postProcessEditor.hWndColorTintBValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyPostProcessParameters(false);

				break;
			}
		}

		break;
	}

	// Pass all unhandled messages to DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 ==================
 WIN_CreatePostProcessEditorWindow
 ==================
*/
void *WIN_CreatePostProcessEditorWindow (){

	INITCOMMONCONTROLSEX	initCommonControls;
	WNDCLASSEX				wndClass;
	RECT					rect;
	HDC						hDC;
	char					name[MAX_PATH_LENGTH];
	const char				**fileList;
	int						numFiles;
	int						screenWidth, screenHeight;
	int						x, y, w, h;
	int						size;
	int						i;

	// Get the instance handle
	win_postProcessEditor.hInstance = (HINSTANCE)Sys_GetInstanceHandle();

	// Initialize up-down control class
	initCommonControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
	initCommonControls.dwICC = ICC_UPDOWN_CLASS;

	InitCommonControlsEx(&initCommonControls);

	// Calculate window position and dimensions
	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);

	rect.left = (screenWidth - 576) / 2;
	rect.top = (screenHeight - 322) / 2;
	rect.right = rect.left + 576;
	rect.bottom = rect.top + 322;

	AdjustWindowRectEx(&rect, POST_PROCESS_EDITOR_WINDOW_STYLE, FALSE, 0);

	x = rect.left;
	y = rect.top;
	w = rect.right - rect.left;
	h = rect.bottom - rect.top;

	// Register the window class
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = WIN_PostProcessEditorWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = (HINSTANCE)Sys_GetInstanceHandle();
	wndClass.hIcon = (HICON)Sys_GetIconHandle();
	wndClass.hIconSm = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = POST_PROCESS_EDITOR_WINDOW_CLASS;

	if (!RegisterClassEx(&wndClass)){
		Com_Printf(S_COLOR_RED "Could not register post-process editor window class\n");

		return NULL;
	}

	// Create the window
	win_postProcessEditor.hWnd = CreateWindowEx(0, POST_PROCESS_EDITOR_WINDOW_CLASS, POST_PROCESS_EDITOR_WINDOW_NAME, POST_PROCESS_EDITOR_WINDOW_STYLE, x, y, w, h, NULL, NULL, win_postProcessEditor.hInstance, NULL);
	if (!win_postProcessEditor.hWnd){
		UnregisterClass(POST_PROCESS_EDITOR_WINDOW_CLASS, win_postProcessEditor.hInstance);

		Com_Printf(S_COLOR_RED "Could not create post-process editor window\n");

		return NULL;
	}

	// Create the controls
	win_postProcessEditor.hWndArea = CreateWindowEx(0, "STATIC", "Map area", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 12, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndAreaValue = CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST, 116, 8, 80, 200, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndApplyToAll = CreateWindowEx(0, "BUTTON", "Apply to all areas", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 328, 7, 120, 23, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndBloom = CreateWindowEx(0, "BUTTON", "Bloom", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 8, 48, 192, 220, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndBloomContrast = CreateWindowEx(0, "STATIC", "Bloom contrast", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 79, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndBloomContrastValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 116, 76, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndBloomContrastSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 176, 76, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndBloomThreshold = CreateWindowEx(0, "STATIC", "Bloom threshold", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 103, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndBloomThresholdValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 116, 100, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndBloomThresholdSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 176, 100, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndBaseIntensity = CreateWindowEx(0, "STATIC", "Base intensity", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 127, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndBaseIntensityValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 116, 124, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndBaseIntensitySpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 176, 124, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndGlowIntensity = CreateWindowEx(0, "STATIC", "Glow intensity", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 151, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndGlowIntensityValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 116, 148, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndGlowIntensitySpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 176, 148, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorCorrection = CreateWindowEx(0, "BUTTON", "Color Correction", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 208, 48, 360, 220, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndR = CreateWindowEx(0, "STATIC", "R", WS_CHILD | WS_VISIBLE | SS_CENTER, 316, 62, 80, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndG = CreateWindowEx(0, "STATIC", "G", WS_CHILD | WS_VISIBLE | SS_CENTER, 400, 62, 80, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndB = CreateWindowEx(0, "STATIC", "B", WS_CHILD | WS_VISIBLE | SS_CENTER, 484, 62, 80, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorShadows = CreateWindowEx(0, "STATIC", "Shadows", WS_CHILD | WS_VISIBLE | SS_RIGHT, 212, 79, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorShadowsRValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 316, 76, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorShadowsRSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 376, 76, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorShadowsGValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 400, 76, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorShadowsGSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 460, 76, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorShadowsBValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 484, 76, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorShadowsBSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 524, 76, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorHighlights = CreateWindowEx(0, "STATIC", "Highlights", WS_CHILD | WS_VISIBLE | SS_RIGHT, 212, 103, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorHighlightsRValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 316, 100, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorHighlightsRSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 376, 100, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorHighlightsGValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 400, 100, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorHighlightsGSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 460, 100, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorHighlightsBValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 484, 100, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorHighlightsBSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 524, 100, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMidtones = CreateWindowEx(0, "STATIC", "Midtones", WS_CHILD | WS_VISIBLE | SS_RIGHT, 212, 127, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMidtonesRValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 316, 124, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMidtonesRSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 376, 124, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMidtonesGValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 400, 124, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMidtonesGSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 460, 124, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMidtonesBValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 484, 124, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMidtonesBSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 524, 124, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMinOutput = CreateWindowEx(0, "STATIC", "Min output", WS_CHILD | WS_VISIBLE | SS_RIGHT, 212, 151, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMinOutputRValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 316, 148, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMinOutputRSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 376, 148, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMinOutputGValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 400, 148, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMinOutputGSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 460, 148, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMinOutputBValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 484, 148, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMinOutputBSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 524, 148, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMaxOutput = CreateWindowEx(0, "STATIC", "Max output", WS_CHILD | WS_VISIBLE | SS_RIGHT, 212, 175, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMaxOutputRValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 316, 172, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMaxOutputRSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 376, 172, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMaxOutputGValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 400, 172, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMaxOutputGSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 460, 172, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMaxOutputBValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 484, 172, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorMaxOutputBSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 524, 172, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorSaturation = CreateWindowEx(0, "STATIC", "Saturation", WS_CHILD | WS_VISIBLE | SS_RIGHT, 212, 199, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorSaturationRValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 316, 196, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorSaturationRSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 376, 196, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorSaturationGValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 400, 196, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorSaturationGSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 460, 196, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorSaturationBValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 484, 196, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorSaturationBSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 524, 196, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorTint = CreateWindowEx(0, "STATIC", "Tint", WS_CHILD | WS_VISIBLE | SS_RIGHT, 212, 223, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorTintRValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 316, 220, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorTintRSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 376, 220, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorTintGValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 400, 220, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorTintGSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 460, 220, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorTintBValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 484, 220, 80, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorTintBSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 524, 220, 20, 20, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorTable = CreateWindowEx(0, "STATIC", "Table", WS_CHILD | WS_VISIBLE | SS_RIGHT, 212, 247, 100, 14, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndColorTableValue = CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWN | CBS_SORT | CBS_AUTOHSCROLL, 316, 244, 248, 200, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndApply = CreateWindowEx(0, "BUTTON", "Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 8, 291, 75, 23, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndReset = CreateWindowEx(0, "BUTTON", "Reset", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 87, 291, 75, 23, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndDefaults = CreateWindowEx(0, "BUTTON", "Defaults", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 166, 291, 75, 23, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);
	win_postProcessEditor.hWndSave = CreateWindowEx(0, "BUTTON", "Save", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 493, 291, 75, 23, win_postProcessEditor.hWnd, NULL, win_postProcessEditor.hInstance, NULL);

	// Create and set the font
	hDC = GetDC(win_postProcessEditor.hWnd);
	size = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ReleaseDC(win_postProcessEditor.hWnd, hDC);

	win_postProcessEditor.hFont = CreateFont(size, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Microsoft Sans Serif");

	SendMessage(win_postProcessEditor.hWndArea, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndAreaValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndApplyToAll, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndBloom, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndBloomContrast, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndBloomContrastValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndBloomThreshold, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndBloomThresholdValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndBaseIntensity, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndBaseIntensityValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndGlowIntensity, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndGlowIntensityValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorCorrection, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndR, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndG, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndB, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorShadows, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorShadowsRValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorShadowsGValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorShadowsBValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorHighlights, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorHighlightsRValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorHighlightsGValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorHighlightsBValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMidtones, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMidtonesRValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMidtonesGValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMidtonesBValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMinOutput, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMinOutputRValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMinOutputGValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMinOutputBValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMaxOutput, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMaxOutputRValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMaxOutputGValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorMaxOutputBValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorSaturation, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorSaturationRValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorSaturationGValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorSaturationBValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorTint, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorTintRValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorTintGValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorTintBValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorTable, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndColorTableValue, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndApply, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndReset, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndDefaults, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);
	SendMessage(win_postProcessEditor.hWndSave, WM_SETFONT, (WPARAM)win_postProcessEditor.hFont, FALSE);

	// Fill area combo box
	SendMessage(win_postProcessEditor.hWndAreaValue, CB_ADDSTRING, 0, (LPARAM)"None");

	for (i = 0; i < CM_NumAreas(); i++)
		SendMessage(win_postProcessEditor.hWndAreaValue, CB_ADDSTRING, 0, (LPARAM)Str_FromInteger(i));

	// Fill color table combo box
	SendMessage(win_postProcessEditor.hWndColorTableValue, CB_ADDSTRING, 0, (LPARAM)"_default");

	fileList = FS_ListFiles("colors", ".tga", false, &numFiles);

	for (i = 0; i < numFiles; i++){
		Str_SPrintf(name, sizeof(name), "colors/%s", fileList[i]);

		if (SendMessage(win_postProcessEditor.hWndColorTableValue, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)name) != CB_ERR)
			continue;

		SendMessage(win_postProcessEditor.hWndColorTableValue, CB_ADDSTRING, 0, (LPARAM)name);
	}

	FS_FreeFileList(fileList);

	// Set text limit for color table combo box
	SendMessage(win_postProcessEditor.hWndColorTableValue, CB_LIMITTEXT, (WPARAM)(MAX_PATH_LENGTH - 1), 0);

	// Show the window
	ShowWindow(win_postProcessEditor.hWnd, SW_SHOW);
	UpdateWindow(win_postProcessEditor.hWnd);
	SetForegroundWindow(win_postProcessEditor.hWnd);
	SetFocus(win_postProcessEditor.hWnd);

	win_postProcessEditor.initialized = true;

	return win_postProcessEditor.hWnd;
}

/*
 ==================
 WIN_DestroyPostProcessEditorWindow
 ==================
*/
void WIN_DestroyPostProcessEditorWindow (){

	if (!win_postProcessEditor.initialized)
		return;

	if (win_postProcessEditor.hFont)
		DeleteObject(win_postProcessEditor.hFont);

	ShowWindow(win_postProcessEditor.hWnd, SW_HIDE);
	DestroyWindow(win_postProcessEditor.hWnd);

	UnregisterClass(POST_PROCESS_EDITOR_WINDOW_CLASS, win_postProcessEditor.hInstance);

	Mem_Fill(&win_postProcessEditor, 0, sizeof(postProcessEditor_t));
}

/*
 ==================
 WIN_EditPostProcessParameters
 ==================
*/
void WIN_EditPostProcessParameters (int index, postProcessParms_t *parms){

	if (!win_postProcessEditor.initialized)
		return;

	// Apply current post-process parameters
	WIN_ApplyPostProcessParameters(false);

	// Set the current post-process parameters
	if (parms){
		win_postProcessEditor.enabled = true;

		win_postProcessEditor.editIndex = index;
		win_postProcessEditor.editParms = *parms;
	}
	else {
		win_postProcessEditor.enabled = false;

		parms = &win_postProcessParms;
	}

	// Update the controls
	SendMessage(win_postProcessEditor.hWndAreaValue, CB_SETCURSEL, (WPARAM)(index + 1), 0);

	SetWindowText(win_postProcessEditor.hWndBloomContrastValue, Str_FromFloat(parms->bloomContrast, -1));
	SetWindowText(win_postProcessEditor.hWndBloomThresholdValue, Str_FromFloat(parms->bloomThreshold, -1));
	SetWindowText(win_postProcessEditor.hWndBaseIntensityValue, Str_FromFloat(parms->baseIntensity, -1));
	SetWindowText(win_postProcessEditor.hWndGlowIntensityValue, Str_FromFloat(parms->glowIntensity, -1));

	SetWindowText(win_postProcessEditor.hWndColorShadowsRValue, Str_FromFloat(parms->colorShadows[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorShadowsGValue, Str_FromFloat(parms->colorShadows[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorShadowsBValue, Str_FromFloat(parms->colorShadows[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorHighlightsRValue, Str_FromFloat(parms->colorHighlights[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorHighlightsGValue, Str_FromFloat(parms->colorHighlights[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorHighlightsBValue, Str_FromFloat(parms->colorHighlights[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorMidtonesRValue, Str_FromFloat(parms->colorMidtones[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorMidtonesGValue, Str_FromFloat(parms->colorMidtones[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorMidtonesBValue, Str_FromFloat(parms->colorMidtones[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorMinOutputRValue, Str_FromFloat(parms->colorMinOutput[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorMinOutputGValue, Str_FromFloat(parms->colorMinOutput[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorMinOutputBValue, Str_FromFloat(parms->colorMinOutput[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorMaxOutputRValue, Str_FromFloat(parms->colorMaxOutput[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorMaxOutputGValue, Str_FromFloat(parms->colorMaxOutput[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorMaxOutputBValue, Str_FromFloat(parms->colorMaxOutput[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorSaturationRValue, Str_FromFloat(parms->colorSaturation[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorSaturationGValue, Str_FromFloat(parms->colorSaturation[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorSaturationBValue, Str_FromFloat(parms->colorSaturation[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorTintRValue, Str_FromFloat(parms->colorTint[0], -1));
	SetWindowText(win_postProcessEditor.hWndColorTintGValue, Str_FromFloat(parms->colorTint[1], -1));
	SetWindowText(win_postProcessEditor.hWndColorTintBValue, Str_FromFloat(parms->colorTint[2], -1));

	SetWindowText(win_postProcessEditor.hWndColorTableValue, parms->colorTableName);

	// Enable or disable the controls
	EnableWindow(win_postProcessEditor.hWndApplyToAll, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndBloom, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndBloomContrast, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndBloomContrastValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndBloomContrastSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndBloomThreshold, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndBloomThresholdValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndBloomThresholdSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndBaseIntensity, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndBaseIntensityValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndBaseIntensitySpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndGlowIntensity, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndGlowIntensityValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndGlowIntensitySpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorCorrection, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndR, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndG, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndB, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorShadows, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorShadowsRValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorShadowsRSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorShadowsGValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorShadowsGSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorShadowsBValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorShadowsBSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorHighlights, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorHighlightsRValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorHighlightsRSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorHighlightsGValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorHighlightsGSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorHighlightsBValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorHighlightsBSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMidtones, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMidtonesRValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMidtonesRSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMidtonesGValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMidtonesGSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMidtonesBValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMidtonesBSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMinOutput, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMinOutputRValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMinOutputRSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMinOutputGValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMinOutputGSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMinOutputBValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMinOutputBSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMaxOutput, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMaxOutputRValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMaxOutputRSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMaxOutputGValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMaxOutputGSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMaxOutputBValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorMaxOutputBSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorSaturation, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorSaturationRValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorSaturationRSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorSaturationGValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorSaturationGSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorSaturationBValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorSaturationBSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorTint, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorTintRValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorTintRSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorTintGValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorTintGSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorTintBValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorTintBSpin, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorTable, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndColorTableValue, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndApply, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndReset, win_postProcessEditor.enabled);
	EnableWindow(win_postProcessEditor.hWndDefaults, win_postProcessEditor.enabled);
}

#else

/*
 ==================
 WIN_CreatePostProcessEditorWindow
 ==================
*/
static void *WIN_CreatePostProcessEditorWindow (){

	Com_Printf(S_COLOR_RED "The post-process editor is not currently supported on this platform");

	return NULL;
}

/*
 ==================
 WIN_DestroyPostProcessEditorWindow
 ==================
*/
static void WIN_DestroyPostProcessEditorWindow (){

}

/*
 ==================
 WIN_EditPostProcessParameters
 ==================
*/
static void WIN_EditPostProcessParameters (int index, postProcessParms_t *parms){

}

#endif


/*
 ==============================================================================

 INTEGRATED LIGHT EDITOR

 ==============================================================================
*/

#ifdef _WIN32

#define LIGHT_EDITOR_WINDOW_NAME	ENGINE_NAME " Light Editor"
#define LIGHT_EDITOR_WINDOW_CLASS	ENGINE_NAME " Light Editor"
#define LIGHT_EDITOR_WINDOW_STYLE	(WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

#define LIGHT_COLOR_BITMAP_SIZE		(80 * 20)

static lightParms_t			win_lightParms = {"light", -1, RL_POINT, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {100.0f, 100.0f, 100.0f}, -0.5f, 0.5f, -0.5f, 0.5f, 1.0f, 200.0f, false, 500.0f, 500.0f, 0, 0, "_defaultLight", 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f};

typedef struct {
	bool					initialized;
	bool					enabled;

	int						editIndex;
	lightParms_t			editParms;

	dword					bitmapBits[LIGHT_COLOR_BITMAP_SIZE];

	// Window stuff
	HINSTANCE				hInstance;

	HWND					hWnd;
	HWND					hWndName;
	HWND					hWndNameValue;
	HWND					hWndFrame1;
	HWND					hWndFrame2;
	HWND					hWndFrame3;
	HWND					hWndFrame4;
	HWND					hWndFrame5;
	HWND					hWndX;
	HWND					hWndY;
	HWND					hWndZ;
	HWND					hWndOrigin;
	HWND					hWndOriginXValue;
	HWND					hWndOriginXSpin;
	HWND					hWndOriginYValue;
	HWND					hWndOriginYSpin;
	HWND					hWndOriginZValue;
	HWND					hWndOriginZSpin;
	HWND					hWndCenter;
	HWND					hWndCenterXValue;
	HWND					hWndCenterXSpin;
	HWND					hWndCenterYValue;
	HWND					hWndCenterYSpin;
	HWND					hWndCenterZValue;
	HWND					hWndCenterZSpin;
	HWND					hWndAngles;
	HWND					hWndAnglesXValue;
	HWND					hWndAnglesXSpin;
	HWND					hWndAnglesYValue;
	HWND					hWndAnglesYSpin;
	HWND					hWndAnglesZValue;
	HWND					hWndAnglesZSpin;
	HWND					hWndType;
	HWND					hWndTypeValue;
	HWND					hWndRadiusX;
	HWND					hWndRadiusY;
	HWND					hWndRadiusZ;
	HWND					hWndRadius;
	HWND					hWndRadiusXValue;
	HWND					hWndRadiusXSpin;
	HWND					hWndRadiusYValue;
	HWND					hWndRadiusYSpin;
	HWND					hWndRadiusZValue;
	HWND					hWndRadiusZSpin;
	HWND					hWndFrustumX;
	HWND					hWndFrustumY;
	HWND					hWndFrustumZ;
	HWND					hWndFrustumMin;
	HWND					hWndFrustumMax;
	HWND					hWndXMinValue;
	HWND					hWndXMinSpin;
	HWND					hWndXMaxValue;
	HWND					hWndXMaxSpin;
	HWND					hWndYMinValue;
	HWND					hWndYMinSpin;
	HWND					hWndYMaxValue;
	HWND					hWndYMaxSpin;
	HWND					hWndZNearValue;
	HWND					hWndZNearSpin;
	HWND					hWndZFarValue;
	HWND					hWndZFarSpin;
	HWND					hWndCastShadows;
	HWND					hWndFogDistance;
	HWND					hWndFogDistanceValue;
	HWND					hWndFogDistanceSpin;
	HWND					hWndFogHeight;
	HWND					hWndFogHeightValue;
	HWND					hWndFogHeightSpin;
	HWND					hWndDetailLevel;
	HWND					hWndDetailLevelValue;
	HWND					hWndColor;
	HWND					hWndColorValue;
	HWND					hWndAlpha;
	HWND					hWndAlphaValue;
	HWND					hWndAlphaSpin;
	HWND					hWndMaterial;
	HWND					hWndMaterialValue;
	HWND					hWndApply;
	HWND					hWndReset;
	HWND					hWndDefaults;
	HWND					hWndRemove;
	HWND					hWndSave;

	HFONT					hFont;

	HBITMAP					hBitmap;

	COLORREF				crCurrent;
	COLORREF				crCustom[16];
} lightParmsEditor_t;

static lightParmsEditor_t	win_lightParmsEditor;

/*
 ==================
 WIN_AddLightMaterial
 ==================
*/
static void WIN_AddLightMaterial (const char *name){

	if (Str_ICompareChars("lights/", name, 7))
		return;

	if (SendMessage(win_lightParmsEditor.hWndMaterialValue, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)name) != CB_ERR)
		return;

	SendMessage(win_lightParmsEditor.hWndMaterialValue, CB_ADDSTRING, 0, (LPARAM)name);
}

/*
 ==================
 WIN_ApplyLightParameters

 NOTE: not sure if name of the light is valid
 ==================
*/
void WIN_ApplyLightParameters (){

	lightParms_t	parms;
	char			string[64];

	if (!win_lightParmsEditor.enabled)
		return;

	// Read the controls
	parms.index = win_lightParmsEditor.editIndex;

	if (GetWindowText(win_lightParmsEditor.hWndNameValue, string, sizeof(string)))
		Str_Copy(parms.name, string, sizeof(parms.name));
	else
		Str_Copy(parms.name, "light", sizeof(parms.name));

	if (GetWindowText(win_lightParmsEditor.hWndOriginXValue, string, sizeof(string)))
		parms.origin[0] = Str_ToFloat(string);
	else
		parms.origin[0] = win_lightParmsEditor.editParms.origin[0];

	if (GetWindowText(win_lightParmsEditor.hWndOriginYValue, string, sizeof(string)))
		parms.origin[1] = Str_ToFloat(string);
	else
		parms.origin[1] = win_lightParmsEditor.editParms.origin[1];

	if (GetWindowText(win_lightParmsEditor.hWndOriginZValue, string, sizeof(string)))
		parms.origin[2] = Str_ToFloat(string);
	else
		parms.origin[2] = win_lightParmsEditor.editParms.origin[2];

	if (GetWindowText(win_lightParmsEditor.hWndCenterXValue, string, sizeof(string)))
		parms.center[0] = Str_ToFloat(string);
	else
		parms.center[0] = win_lightParmsEditor.editParms.center[0];

	if (GetWindowText(win_lightParmsEditor.hWndCenterYValue, string, sizeof(string)))
		parms.center[1] = Str_ToFloat(string);
	else
		parms.center[1] = win_lightParmsEditor.editParms.center[1];

	if (GetWindowText(win_lightParmsEditor.hWndCenterZValue, string, sizeof(string)))
		parms.center[2] = Str_ToFloat(string);
	else
		parms.center[2] = win_lightParmsEditor.editParms.center[2];

	if (GetWindowText(win_lightParmsEditor.hWndAnglesXValue, string, sizeof(string)))
		parms.angles[0] = Str_ToFloat(string);
	else
		parms.angles[0] = win_lightParmsEditor.editParms.angles[0];

	if (GetWindowText(win_lightParmsEditor.hWndAnglesYValue, string, sizeof(string)))
		parms.angles[1] = Str_ToFloat(string);
	else
		parms.angles[1] = win_lightParmsEditor.editParms.angles[1];

	if (GetWindowText(win_lightParmsEditor.hWndAnglesZValue, string, sizeof(string)))
		parms.angles[2] = Str_ToFloat(string);
	else
		parms.angles[2] = win_lightParmsEditor.editParms.angles[2];

	parms.type = SendMessage(win_lightParmsEditor.hWndTypeValue, CB_GETCURSEL, 0, 0);

	if (parms.type != RL_PROJECTED){
		if (GetWindowText(win_lightParmsEditor.hWndRadiusXValue, string, sizeof(string)))
			parms.radius[0] = Str_ToFloat(string);
		else
			parms.radius[0] = win_lightParmsEditor.editParms.radius[0];

		if (GetWindowText(win_lightParmsEditor.hWndRadiusYValue, string, sizeof(string)))
			parms.radius[1] = Str_ToFloat(string);
		else
			parms.radius[1] = win_lightParmsEditor.editParms.radius[1];

		if (GetWindowText(win_lightParmsEditor.hWndRadiusZValue, string, sizeof(string)))
			parms.radius[2] = Str_ToFloat(string);
		else
			parms.radius[2] = win_lightParmsEditor.editParms.radius[2];

		parms.xMin = -0.5f;
		parms.xMax = 0.5f;

		parms.yMin = -0.5f;
		parms.yMax = 0.5f;

		parms.zNear = 1.0f;
		parms.zFar = 200.0f;
	}
	else {
		if (GetWindowText(win_lightParmsEditor.hWndXMinValue, string, sizeof(string)))
			parms.xMin = Str_ToFloat(string);
		else
			parms.xMin = win_lightParmsEditor.editParms.xMin;

		if (GetWindowText(win_lightParmsEditor.hWndXMaxValue, string, sizeof(string)))
			parms.xMax = Str_ToFloat(string);
		else
			parms.xMax = win_lightParmsEditor.editParms.xMax;

		if (GetWindowText(win_lightParmsEditor.hWndYMinValue, string, sizeof(string)))
			parms.yMin = Str_ToFloat(string);
		else
			parms.yMin = win_lightParmsEditor.editParms.yMin;

		if (GetWindowText(win_lightParmsEditor.hWndYMaxValue, string, sizeof(string)))
			parms.yMax = Str_ToFloat(string);
		else
			parms.yMax = win_lightParmsEditor.editParms.yMax;

		if (GetWindowText(win_lightParmsEditor.hWndZNearValue, string, sizeof(string)))
			parms.zNear = Str_ToFloat(string);
		else
			parms.zNear = win_lightParmsEditor.editParms.zNear;

		if (GetWindowText(win_lightParmsEditor.hWndZFarValue, string, sizeof(string)))
			parms.zFar = Str_ToFloat(string);
		else
			parms.zFar = win_lightParmsEditor.editParms.zFar;

		VectorSet(parms.radius, 100.0f, 100.0f, 100.0f);
	}

	if (SendMessage(win_lightParmsEditor.hWndCastShadows, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
		parms.noShadows = true;
	else
		parms.noShadows = false;

	if (GetWindowText(win_lightParmsEditor.hWndFogDistanceValue, string, sizeof(string)))
		parms.fogDistance = Str_ToFloat(string);
	else
		parms.fogDistance = win_lightParmsEditor.editParms.fogDistance;

	if (GetWindowText(win_lightParmsEditor.hWndFogHeightValue, string, sizeof(string)))
		parms.fogHeight = Str_ToFloat(string);
	else
		parms.fogHeight = win_lightParmsEditor.editParms.fogHeight;

	parms.detailLevel = SendMessage(win_lightParmsEditor.hWndDetailLevelValue, CB_GETCURSEL, 0, 0);
	if (parms.detailLevel < 0 || parms.detailLevel > 2)
		parms.detailLevel = win_lightParmsEditor.editParms.detailLevel;

	parms.style = 0;	// FIXME!!!

	parms.materialParms[0] = GetRValue(win_lightParmsEditor.crCurrent) * (1.0f / 255.0f);
	parms.materialParms[1] = GetGValue(win_lightParmsEditor.crCurrent) * (1.0f / 255.0f);
	parms.materialParms[2] = GetBValue(win_lightParmsEditor.crCurrent) * (1.0f / 255.0f);

	if (GetWindowText(win_lightParmsEditor.hWndAlphaValue, string, sizeof(string)))
		parms.materialParms[3] = Str_ToFloat(string);
	else
		parms.materialParms[3] = win_lightParmsEditor.editParms.materialParms[3];

	parms.materialParms[4] = win_lightParmsEditor.editParms.materialParms[4];
	parms.materialParms[5] = win_lightParmsEditor.editParms.materialParms[5];
	parms.materialParms[6] = win_lightParmsEditor.editParms.materialParms[6];
	parms.materialParms[7] = win_lightParmsEditor.editParms.materialParms[7];

	if (!GetWindowText(win_lightParmsEditor.hWndMaterialValue, parms.material, sizeof(parms.material)))
		Str_Copy(parms.material, "", sizeof(parms.material));

	// Update the parameters
	R_LightEditorUpdateCallback(win_lightParmsEditor.editIndex, &parms);
}

/*
 ==================
 WIN_ResetLightParameters
 ==================
*/
void WIN_ResetLightParameters (lightParms_t *parms){

	byte	r, g, b;
	int		i;

	if (!win_lightParmsEditor.enabled)
		return;

	// Update the controls
	SetWindowText(win_lightParmsEditor.hWndNameValue, parms->name);

	SetWindowText(win_lightParmsEditor.hWndOriginXValue, Str_FromFloat(parms->origin[0], -1));
	SetWindowText(win_lightParmsEditor.hWndOriginYValue, Str_FromFloat(parms->origin[1], -1));
	SetWindowText(win_lightParmsEditor.hWndOriginZValue, Str_FromFloat(parms->origin[2], -1));

	SetWindowText(win_lightParmsEditor.hWndCenterXValue, Str_FromFloat(parms->center[0], -1));
	SetWindowText(win_lightParmsEditor.hWndCenterYValue, Str_FromFloat(parms->center[1], -1));
	SetWindowText(win_lightParmsEditor.hWndCenterZValue, Str_FromFloat(parms->center[2], -1));

	SetWindowText(win_lightParmsEditor.hWndAnglesXValue, Str_FromFloat(parms->angles[0], -1));
	SetWindowText(win_lightParmsEditor.hWndAnglesYValue, Str_FromFloat(parms->angles[1], -1));
	SetWindowText(win_lightParmsEditor.hWndAnglesZValue, Str_FromFloat(parms->angles[2], -1));

	SendMessage(win_lightParmsEditor.hWndTypeValue, CB_SETCURSEL, (WPARAM)ClampInt(parms->type, 0, 3), 0);

	SetWindowText(win_lightParmsEditor.hWndRadiusXValue, Str_FromFloat(parms->radius[0], -1));
	SetWindowText(win_lightParmsEditor.hWndRadiusYValue, Str_FromFloat(parms->radius[1], -1));
	SetWindowText(win_lightParmsEditor.hWndRadiusZValue, Str_FromFloat(parms->radius[2], -1));

	SetWindowText(win_lightParmsEditor.hWndXMinValue, Str_FromFloat(parms->xMin, -1));
	SetWindowText(win_lightParmsEditor.hWndXMaxValue, Str_FromFloat(parms->xMax, -1));

	SetWindowText(win_lightParmsEditor.hWndYMinValue, Str_FromFloat(parms->yMin, -1));
	SetWindowText(win_lightParmsEditor.hWndYMaxValue, Str_FromFloat(parms->yMax, -1));

	SetWindowText(win_lightParmsEditor.hWndZNearValue, Str_FromFloat(parms->zNear, -1));
	SetWindowText(win_lightParmsEditor.hWndZFarValue, Str_FromFloat(parms->zFar, -1));

	if (parms->noShadows)
		SendMessage(win_lightParmsEditor.hWndCastShadows, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	else
		SendMessage(win_lightParmsEditor.hWndCastShadows, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

	SetWindowText(win_lightParmsEditor.hWndFogDistanceValue, Str_FromFloat(parms->fogDistance, -1));
	SetWindowText(win_lightParmsEditor.hWndFogHeightValue, Str_FromFloat(parms->fogHeight, -1));

	SendMessage(win_lightParmsEditor.hWndDetailLevelValue, CB_SETCURSEL, (WPARAM)ClampInt(parms->detailLevel, 0, 2), 0);

	r = FloatToByte(parms->materialParms[0] * 255.0f);
	g = FloatToByte(parms->materialParms[1] * 255.0f);
	b = FloatToByte(parms->materialParms[2] * 255.0f);

	win_lightParmsEditor.crCurrent = RGB(r, g, b);

	for (i = 0; i < LIGHT_COLOR_BITMAP_SIZE; i++)
		win_lightParmsEditor.bitmapBits[i] = (b << 0) | (g << 8) | (r << 16) | (255 << 24);

	SetBitmapBits(win_lightParmsEditor.hBitmap, LIGHT_COLOR_BITMAP_SIZE * 4, win_lightParmsEditor.bitmapBits);
	SendMessage(win_lightParmsEditor.hWndColorValue, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)win_lightParmsEditor.hBitmap);

	SetWindowText(win_lightParmsEditor.hWndAlphaValue, Str_FromFloat(ClampFloat(parms->materialParms[3], 0.0f, 1.0f), -1));

	SetWindowText(win_lightParmsEditor.hWndMaterialValue, parms->material);

	// Enable or disable the controls
	if (parms->type != RL_PROJECTED){
		EnableWindow(win_lightParmsEditor.hWndRadiusX, TRUE);
		EnableWindow(win_lightParmsEditor.hWndRadiusY, TRUE);
		EnableWindow(win_lightParmsEditor.hWndRadiusZ, TRUE);
		EnableWindow(win_lightParmsEditor.hWndRadius, TRUE);
		EnableWindow(win_lightParmsEditor.hWndRadiusXValue, TRUE);
		EnableWindow(win_lightParmsEditor.hWndRadiusXSpin, TRUE);
		EnableWindow(win_lightParmsEditor.hWndRadiusYValue, TRUE);
		EnableWindow(win_lightParmsEditor.hWndRadiusYSpin, TRUE);
		EnableWindow(win_lightParmsEditor.hWndRadiusZValue, TRUE);
		EnableWindow(win_lightParmsEditor.hWndRadiusZSpin, TRUE);
		EnableWindow(win_lightParmsEditor.hWndFrustumX, FALSE);
		EnableWindow(win_lightParmsEditor.hWndFrustumY, FALSE);
		EnableWindow(win_lightParmsEditor.hWndFrustumZ, FALSE);
		EnableWindow(win_lightParmsEditor.hWndFrustumMin, FALSE);
		EnableWindow(win_lightParmsEditor.hWndFrustumMax, FALSE);
		EnableWindow(win_lightParmsEditor.hWndXMinValue, FALSE);
		EnableWindow(win_lightParmsEditor.hWndXMinSpin, FALSE);
		EnableWindow(win_lightParmsEditor.hWndXMaxValue, FALSE);
		EnableWindow(win_lightParmsEditor.hWndXMaxSpin, FALSE);
		EnableWindow(win_lightParmsEditor.hWndYMinValue, FALSE);
		EnableWindow(win_lightParmsEditor.hWndYMinSpin, FALSE);
		EnableWindow(win_lightParmsEditor.hWndYMaxValue, FALSE);
		EnableWindow(win_lightParmsEditor.hWndYMaxSpin, FALSE);
		EnableWindow(win_lightParmsEditor.hWndZNearValue, FALSE);
		EnableWindow(win_lightParmsEditor.hWndZNearSpin, FALSE);
		EnableWindow(win_lightParmsEditor.hWndZFarValue, FALSE);
		EnableWindow(win_lightParmsEditor.hWndZFarSpin, FALSE);
	}
	else {
		EnableWindow(win_lightParmsEditor.hWndRadiusX, FALSE);
		EnableWindow(win_lightParmsEditor.hWndRadiusY, FALSE);
		EnableWindow(win_lightParmsEditor.hWndRadiusZ, FALSE);
		EnableWindow(win_lightParmsEditor.hWndRadius, FALSE);
		EnableWindow(win_lightParmsEditor.hWndRadiusXValue, FALSE);
		EnableWindow(win_lightParmsEditor.hWndRadiusXSpin, FALSE);
		EnableWindow(win_lightParmsEditor.hWndRadiusYValue, FALSE);
		EnableWindow(win_lightParmsEditor.hWndRadiusYSpin, FALSE);
		EnableWindow(win_lightParmsEditor.hWndRadiusZValue, FALSE);
		EnableWindow(win_lightParmsEditor.hWndRadiusZSpin, FALSE);
		EnableWindow(win_lightParmsEditor.hWndFrustumX, TRUE);
		EnableWindow(win_lightParmsEditor.hWndFrustumY, TRUE);
		EnableWindow(win_lightParmsEditor.hWndFrustumZ, TRUE);
		EnableWindow(win_lightParmsEditor.hWndFrustumMin, TRUE);
		EnableWindow(win_lightParmsEditor.hWndFrustumMax, TRUE);
		EnableWindow(win_lightParmsEditor.hWndXMinValue, TRUE);
		EnableWindow(win_lightParmsEditor.hWndXMinSpin, TRUE);
		EnableWindow(win_lightParmsEditor.hWndXMaxValue, TRUE);
		EnableWindow(win_lightParmsEditor.hWndXMaxSpin, TRUE);
		EnableWindow(win_lightParmsEditor.hWndYMinValue, TRUE);
		EnableWindow(win_lightParmsEditor.hWndYMinSpin, TRUE);
		EnableWindow(win_lightParmsEditor.hWndYMaxValue, TRUE);
		EnableWindow(win_lightParmsEditor.hWndYMaxSpin, TRUE);
		EnableWindow(win_lightParmsEditor.hWndZNearValue, TRUE);
		EnableWindow(win_lightParmsEditor.hWndZNearSpin, TRUE);
		EnableWindow(win_lightParmsEditor.hWndZFarValue, TRUE);
		EnableWindow(win_lightParmsEditor.hWndZFarSpin, TRUE);
	}

	// Update the parameters
	R_LightEditorUpdateCallback(win_lightParmsEditor.editIndex, parms);
}

/*
 ==================
 WIN_RemoveLightParameters
 ==================
*/
void WIN_RemoveLightParameters (){

	byte	r, g, b;
	int		i;

	if (!win_lightParmsEditor.enabled)
		return;

	win_lightParmsEditor.enabled = false;

	// Update the controls
	SetWindowText(win_lightParmsEditor.hWndNameValue, win_lightParms.name);

	SetWindowText(win_lightParmsEditor.hWndOriginXValue, Str_FromFloat(win_lightParms.origin[0], -1));
	SetWindowText(win_lightParmsEditor.hWndOriginYValue, Str_FromFloat(win_lightParms.origin[1], -1));
	SetWindowText(win_lightParmsEditor.hWndOriginZValue, Str_FromFloat(win_lightParms.origin[2], -1));

	SetWindowText(win_lightParmsEditor.hWndCenterXValue, Str_FromFloat(win_lightParms.center[0], -1));
	SetWindowText(win_lightParmsEditor.hWndCenterYValue, Str_FromFloat(win_lightParms.center[1], -1));
	SetWindowText(win_lightParmsEditor.hWndCenterZValue, Str_FromFloat(win_lightParms.center[2], -1));

	SetWindowText(win_lightParmsEditor.hWndAnglesXValue, Str_FromFloat(win_lightParms.angles[0], -1));
	SetWindowText(win_lightParmsEditor.hWndAnglesYValue, Str_FromFloat(win_lightParms.angles[1], -1));
	SetWindowText(win_lightParmsEditor.hWndAnglesZValue, Str_FromFloat(win_lightParms.angles[2], -1));

	SendMessage(win_lightParmsEditor.hWndTypeValue, CB_SETCURSEL, (WPARAM)ClampInt(win_lightParms.type, 0, 3), 0);

	SetWindowText(win_lightParmsEditor.hWndRadiusXValue, Str_FromFloat(win_lightParms.radius[0], -1));
	SetWindowText(win_lightParmsEditor.hWndRadiusYValue, Str_FromFloat(win_lightParms.radius[1], -1));
	SetWindowText(win_lightParmsEditor.hWndRadiusZValue, Str_FromFloat(win_lightParms.radius[2], -1));

	SetWindowText(win_lightParmsEditor.hWndXMinValue, Str_FromFloat(win_lightParms.xMin, -1));
	SetWindowText(win_lightParmsEditor.hWndXMaxValue, Str_FromFloat(win_lightParms.xMax, -1));

	SetWindowText(win_lightParmsEditor.hWndYMinValue, Str_FromFloat(win_lightParms.yMin, -1));
	SetWindowText(win_lightParmsEditor.hWndYMaxValue, Str_FromFloat(win_lightParms.yMax, -1));

	SetWindowText(win_lightParmsEditor.hWndZNearValue, Str_FromFloat(win_lightParms.zNear, -1));
	SetWindowText(win_lightParmsEditor.hWndZFarValue, Str_FromFloat(win_lightParms.zFar, -1));

	if (win_lightParms.noShadows)
		SendMessage(win_lightParmsEditor.hWndCastShadows, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	else
		SendMessage(win_lightParmsEditor.hWndCastShadows, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

	SetWindowText(win_lightParmsEditor.hWndFogDistanceValue, Str_FromFloat(win_lightParms.fogDistance, -1));
	SetWindowText(win_lightParmsEditor.hWndFogHeightValue, Str_FromFloat(win_lightParms.fogHeight, -1));

	SendMessage(win_lightParmsEditor.hWndDetailLevelValue, CB_SETCURSEL, (WPARAM)ClampInt(win_lightParms.detailLevel, 0, 2), 0);

	r = FloatToByte(win_lightParms.materialParms[0] * 255.0f);
	g = FloatToByte(win_lightParms.materialParms[1] * 255.0f);
	b = FloatToByte(win_lightParms.materialParms[2] * 255.0f);

	win_lightParmsEditor.crCurrent = RGB(r, g, b);

	for (i = 0; i < LIGHT_COLOR_BITMAP_SIZE; i++)
		win_lightParmsEditor.bitmapBits[i] = (b << 0) | (g << 8) | (r << 16) | (255 << 24);

	SetBitmapBits(win_lightParmsEditor.hBitmap, LIGHT_COLOR_BITMAP_SIZE * 4, win_lightParmsEditor.bitmapBits);
	SendMessage(win_lightParmsEditor.hWndColorValue, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)win_lightParmsEditor.hBitmap);

	SetWindowText(win_lightParmsEditor.hWndAlphaValue, Str_FromFloat(ClampFloat(win_lightParms.materialParms[3], 0.0f, 1.0f), -1));

	SetWindowText(win_lightParmsEditor.hWndMaterialValue, win_lightParms.material);

	// Disable the controls
	EnableWindow(win_lightParmsEditor.hWndName, FALSE);
	EnableWindow(win_lightParmsEditor.hWndNameValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFrame1, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFrame2, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFrame3, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFrame4, FALSE);
	EnableWindow(win_lightParmsEditor.hWndX, FALSE);
	EnableWindow(win_lightParmsEditor.hWndY, FALSE);
	EnableWindow(win_lightParmsEditor.hWndZ, FALSE);
	EnableWindow(win_lightParmsEditor.hWndOrigin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndOriginXValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndOriginXSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndOriginYValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndOriginYSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndOriginZValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndOriginZSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndCenter, FALSE);
	EnableWindow(win_lightParmsEditor.hWndCenterXValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndCenterXSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndCenterYValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndCenterYSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndCenterZValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndCenterZSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndAngles, FALSE);
	EnableWindow(win_lightParmsEditor.hWndAnglesXValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndAnglesXSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndAnglesYValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndAnglesYSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndAnglesZValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndAnglesZSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndType, FALSE);
	EnableWindow(win_lightParmsEditor.hWndTypeValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRadiusX, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRadiusY, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRadiusZ, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRadius, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRadiusXValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRadiusXSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRadiusYValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRadiusYSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRadiusZValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRadiusZSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFrustumX, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFrustumY, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFrustumZ, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFrustumMin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFrustumMax, FALSE);
	EnableWindow(win_lightParmsEditor.hWndXMinValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndXMinSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndXMaxValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndXMaxSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndYMinValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndYMinSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndYMaxValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndYMaxSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndZNearValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndZNearSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndZFarValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndZFarSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndCastShadows, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFogDistance, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFogDistanceValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFogDistanceSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFogHeight, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFogHeightValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndFogHeightSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndDetailLevel, FALSE);
	EnableWindow(win_lightParmsEditor.hWndDetailLevelValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndColor, FALSE);
	EnableWindow(win_lightParmsEditor.hWndColorValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndAlpha, FALSE);
	EnableWindow(win_lightParmsEditor.hWndAlphaValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndAlphaSpin, FALSE);
	EnableWindow(win_lightParmsEditor.hWndMaterial, FALSE);
	EnableWindow(win_lightParmsEditor.hWndMaterialValue, FALSE);
	EnableWindow(win_lightParmsEditor.hWndApply, FALSE);
	EnableWindow(win_lightParmsEditor.hWndReset, FALSE);
	EnableWindow(win_lightParmsEditor.hWndDefaults, FALSE);
	EnableWindow(win_lightParmsEditor.hWndRemove, FALSE);

	// Remove the parameters
	R_LightEditorRemoveCallback(win_lightParmsEditor.editIndex);
}

/*
 ==================
 
 TODO: noShadows
 TODO: make sure name is valid
 ==================
*/
static LRESULT CALLBACK WIN_LightEditorWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	CHOOSECOLOR	chooseColor;
	char		string[64];
	int			result;
	int			i;

	switch (uMsg){
	case WM_CLOSE:
		WIN_ApplyLightParameters();

		R_LightEditorCloseCallback();

		break;
	case WM_SYSCOMMAND:
		if (wParam == SC_KEYMENU)
			return 0;

		break;
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_KILLFOCUS){
			if ((HWND)lParam == win_lightParmsEditor.hWndNameValue){
				if (!win_lightParmsEditor.enabled)
					break;

				GetWindowText(win_lightParmsEditor.hWndNameValue, string, sizeof(string));
				SetWindowText(win_lightParmsEditor.hWndNameValue, string);

				break;
			}
		}

		if (HIWORD(wParam) == BN_CLICKED){
			if ((HWND)lParam == win_lightParmsEditor.hWndCastShadows){
				if (SendMessage(win_lightParmsEditor.hWndCastShadows, BM_GETCHECK, 0, 0) == BST_UNCHECKED){

				}
				else {

				}

				WIN_ApplyLightParameters();

				break;
			}

			if ((HWND)lParam == win_lightParmsEditor.hWndColorValue){
				chooseColor.lStructSize = sizeof(CHOOSECOLOR);
				chooseColor.hwndOwner = win_lightParmsEditor.hWnd;
				chooseColor.hInstance = NULL;
				chooseColor.rgbResult = win_lightParmsEditor.crCurrent;
				chooseColor.lpCustColors = win_lightParmsEditor.crCustom;
				chooseColor.Flags = CC_FULLOPEN | CC_RGBINIT;
				chooseColor.lCustData = 0;
				chooseColor.lpfnHook = NULL;
				chooseColor.lpTemplateName = NULL;

				if (ChooseColor(&chooseColor)){
					win_lightParmsEditor.crCurrent = chooseColor.rgbResult;

					for (i = 0; i < LIGHT_COLOR_BITMAP_SIZE; i++)
						win_lightParmsEditor.bitmapBits[i] = (GetBValue(chooseColor.rgbResult) << 0) | (GetGValue(chooseColor.rgbResult) << 8) | (GetRValue(chooseColor.rgbResult) << 16) | (255 << 24);

					SetBitmapBits(win_lightParmsEditor.hBitmap, LIGHT_COLOR_BITMAP_SIZE * 4, win_lightParmsEditor.bitmapBits);
					SendMessage(win_lightParmsEditor.hWndColorValue, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)win_lightParmsEditor.hBitmap);

					WIN_ApplyLightParameters();
				}

				break;
			}

			if ((HWND)lParam == win_lightParmsEditor.hWndApply){
				WIN_ApplyLightParameters();
				break;
			}

			if ((HWND)lParam == win_lightParmsEditor.hWndReset){
				WIN_ResetLightParameters(&win_lightParmsEditor.editParms);
				break;
			}

			if ((HWND)lParam == win_lightParmsEditor.hWndDefaults){
				WIN_ResetLightParameters(&win_lightParms);
				break;
			}

			if ((HWND)lParam == win_lightParmsEditor.hWndRemove){
				WIN_RemoveLightParameters();
				break;
			}

			if ((HWND)lParam == win_lightParmsEditor.hWndSave){
				WIN_ApplyLightParameters();

				R_LightEditorSaveCallback();

				break;
			}
		}

		if (HIWORD(wParam) == CBN_SELCHANGE){
			if ((HWND)lParam == win_lightParmsEditor.hWndTypeValue){
				result = SendMessage(win_lightParmsEditor.hWndTypeValue, CB_GETCURSEL, 0, 0);

				if (result < 0 || result > 3)
					break;

				if (result != RL_PROJECTED){
					EnableWindow(win_lightParmsEditor.hWndRadiusX, TRUE);
					EnableWindow(win_lightParmsEditor.hWndRadiusY, TRUE);
					EnableWindow(win_lightParmsEditor.hWndRadiusZ, TRUE);
					EnableWindow(win_lightParmsEditor.hWndRadius, TRUE);
					EnableWindow(win_lightParmsEditor.hWndRadiusXValue, TRUE);
					EnableWindow(win_lightParmsEditor.hWndRadiusXSpin, TRUE);
					EnableWindow(win_lightParmsEditor.hWndRadiusYValue, TRUE);
					EnableWindow(win_lightParmsEditor.hWndRadiusYSpin, TRUE);
					EnableWindow(win_lightParmsEditor.hWndRadiusZValue, TRUE);
					EnableWindow(win_lightParmsEditor.hWndRadiusZSpin, TRUE);
					EnableWindow(win_lightParmsEditor.hWndFrustumX, FALSE);
					EnableWindow(win_lightParmsEditor.hWndFrustumY, FALSE);
					EnableWindow(win_lightParmsEditor.hWndFrustumZ, FALSE);
					EnableWindow(win_lightParmsEditor.hWndFrustumMin, FALSE);
					EnableWindow(win_lightParmsEditor.hWndFrustumMax, FALSE);
					EnableWindow(win_lightParmsEditor.hWndXMinValue, FALSE);
					EnableWindow(win_lightParmsEditor.hWndXMinSpin, FALSE);
					EnableWindow(win_lightParmsEditor.hWndXMaxValue, FALSE);
					EnableWindow(win_lightParmsEditor.hWndXMaxSpin, FALSE);
					EnableWindow(win_lightParmsEditor.hWndYMinValue, FALSE);
					EnableWindow(win_lightParmsEditor.hWndYMinSpin, FALSE);
					EnableWindow(win_lightParmsEditor.hWndYMaxValue, FALSE);
					EnableWindow(win_lightParmsEditor.hWndYMaxSpin, FALSE);
					EnableWindow(win_lightParmsEditor.hWndZNearValue, FALSE);
					EnableWindow(win_lightParmsEditor.hWndZNearSpin, FALSE);
					EnableWindow(win_lightParmsEditor.hWndZFarValue, FALSE);
					EnableWindow(win_lightParmsEditor.hWndZFarSpin, FALSE);
				}
				else {
					EnableWindow(win_lightParmsEditor.hWndRadiusX, FALSE);
					EnableWindow(win_lightParmsEditor.hWndRadiusY, FALSE);
					EnableWindow(win_lightParmsEditor.hWndRadiusZ, FALSE);
					EnableWindow(win_lightParmsEditor.hWndRadius, FALSE);
					EnableWindow(win_lightParmsEditor.hWndRadiusXValue, FALSE);
					EnableWindow(win_lightParmsEditor.hWndRadiusXSpin, FALSE);
					EnableWindow(win_lightParmsEditor.hWndRadiusYValue, FALSE);
					EnableWindow(win_lightParmsEditor.hWndRadiusYSpin, FALSE);
					EnableWindow(win_lightParmsEditor.hWndRadiusZValue, FALSE);
					EnableWindow(win_lightParmsEditor.hWndRadiusZSpin, FALSE);
					EnableWindow(win_lightParmsEditor.hWndFrustumX, TRUE);
					EnableWindow(win_lightParmsEditor.hWndFrustumY, TRUE);
					EnableWindow(win_lightParmsEditor.hWndFrustumZ, TRUE);
					EnableWindow(win_lightParmsEditor.hWndFrustumMin, TRUE);
					EnableWindow(win_lightParmsEditor.hWndFrustumMax, TRUE);
					EnableWindow(win_lightParmsEditor.hWndXMinValue, TRUE);
					EnableWindow(win_lightParmsEditor.hWndXMinSpin, TRUE);
					EnableWindow(win_lightParmsEditor.hWndXMaxValue, TRUE);
					EnableWindow(win_lightParmsEditor.hWndXMaxSpin, TRUE);
					EnableWindow(win_lightParmsEditor.hWndYMinValue, TRUE);
					EnableWindow(win_lightParmsEditor.hWndYMinSpin, TRUE);
					EnableWindow(win_lightParmsEditor.hWndYMaxValue, TRUE);
					EnableWindow(win_lightParmsEditor.hWndYMaxSpin, TRUE);
					EnableWindow(win_lightParmsEditor.hWndZNearValue, TRUE);
					EnableWindow(win_lightParmsEditor.hWndZNearSpin, TRUE);
					EnableWindow(win_lightParmsEditor.hWndZFarValue, TRUE);
					EnableWindow(win_lightParmsEditor.hWndZFarSpin, TRUE);
				}

				WIN_ApplyLightParameters();

				break;
			}

			if ((HWND)lParam == win_lightParmsEditor.hWndDetailLevelValue){
				WIN_ApplyLightParameters();
				break;
			}
		}

		break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == UDN_DELTAPOS){
			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndOriginXSpin){
				GetWindowText(win_lightParmsEditor.hWndOriginXValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndOriginXValue, Str_FromFloat(Str_ToFloat(string) + 1.0f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndOriginXValue, Str_FromFloat(Str_ToFloat(string) - 1.0f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndOriginYSpin){
				GetWindowText(win_lightParmsEditor.hWndOriginYValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndOriginYValue, Str_FromFloat(Str_ToFloat(string) + 1.0f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndOriginYValue, Str_FromFloat(Str_ToFloat(string) - 1.0f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndOriginZSpin){
				GetWindowText(win_lightParmsEditor.hWndOriginZValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndOriginZValue, Str_FromFloat(Str_ToFloat(string) + 1.0f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndOriginZValue, Str_FromFloat(Str_ToFloat(string) - 1.0f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndCenterXSpin){
				GetWindowText(win_lightParmsEditor.hWndCenterXValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndCenterXValue, Str_FromFloat(Str_ToFloat(string) + 1.0f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndCenterXValue, Str_FromFloat(Str_ToFloat(string) - 1.0f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndCenterYSpin){
				GetWindowText(win_lightParmsEditor.hWndCenterYValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndCenterYValue, Str_FromFloat(Str_ToFloat(string) + 1.0f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndCenterYValue, Str_FromFloat(Str_ToFloat(string) - 1.0f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndCenterZSpin){
				GetWindowText(win_lightParmsEditor.hWndCenterZValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndCenterZValue, Str_FromFloat(Str_ToFloat(string) + 1.0f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndCenterZValue, Str_FromFloat(Str_ToFloat(string) - 1.0f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndAnglesXSpin){
				GetWindowText(win_lightParmsEditor.hWndAnglesXValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndAnglesXValue, Str_FromFloat(Str_ToFloat(string) + 1.0f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndAnglesXValue, Str_FromFloat(Str_ToFloat(string) - 1.0f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndAnglesYSpin){
				GetWindowText(win_lightParmsEditor.hWndAnglesYValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndAnglesYValue, Str_FromFloat(Str_ToFloat(string) + 1.0f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndAnglesYValue, Str_FromFloat(Str_ToFloat(string) - 1.0f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndAnglesZSpin){
				GetWindowText(win_lightParmsEditor.hWndAnglesZValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndAnglesZValue, Str_FromFloat(Str_ToFloat(string) + 1.0f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndAnglesZValue, Str_FromFloat(Str_ToFloat(string) - 1.0f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndRadiusXSpin){
				GetWindowText(win_lightParmsEditor.hWndRadiusXValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndRadiusXValue, Str_FromFloat(Max(Str_ToFloat(string) + 1.0f, 1.0f), -1));
				else
					SetWindowText(win_lightParmsEditor.hWndRadiusXValue, Str_FromFloat(Max(Str_ToFloat(string) - 1.0f, 1.0f), -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndRadiusYSpin){
				GetWindowText(win_lightParmsEditor.hWndRadiusYValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndRadiusYValue, Str_FromFloat(Max(Str_ToFloat(string) + 1.0f, 1.0f), -1));
				else
					SetWindowText(win_lightParmsEditor.hWndRadiusYValue, Str_FromFloat(Max(Str_ToFloat(string) - 1.0f, 1.0f), -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndRadiusZSpin){
				GetWindowText(win_lightParmsEditor.hWndRadiusZValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndRadiusZValue, Str_FromFloat(Max(Str_ToFloat(string) + 1.0f, 1.0f), -1));
				else
					SetWindowText(win_lightParmsEditor.hWndRadiusZValue, Str_FromFloat(Max(Str_ToFloat(string) - 1.0f, 1.0f), -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndXMinSpin){
				GetWindowText(win_lightParmsEditor.hWndXMinValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndXMinValue, Str_FromFloat(Str_ToFloat(string) + 0.1f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndXMinValue, Str_FromFloat(Str_ToFloat(string) - 0.1f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndXMaxSpin){
				GetWindowText(win_lightParmsEditor.hWndXMaxValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndXMaxValue, Str_FromFloat(Str_ToFloat(string) + 0.1f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndXMaxValue, Str_FromFloat(Str_ToFloat(string) - 0.1f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndYMinSpin){
				GetWindowText(win_lightParmsEditor.hWndYMinValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndYMinValue, Str_FromFloat(Str_ToFloat(string) + 0.1f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndYMinValue, Str_FromFloat(Str_ToFloat(string) - 0.1f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndYMaxSpin){
				GetWindowText(win_lightParmsEditor.hWndYMaxValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndYMaxValue, Str_FromFloat(Str_ToFloat(string) + 0.1f, -1));
				else
					SetWindowText(win_lightParmsEditor.hWndYMaxValue, Str_FromFloat(Str_ToFloat(string) - 0.1f, -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndZNearSpin){
				GetWindowText(win_lightParmsEditor.hWndZNearValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndZNearValue, Str_FromFloat(Max(Str_ToFloat(string) + 1.0f, 1.0f), -1));
				else
					SetWindowText(win_lightParmsEditor.hWndZNearValue, Str_FromFloat(Max(Str_ToFloat(string) - 1.0f, 1.0f), -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndZFarSpin){
				GetWindowText(win_lightParmsEditor.hWndZFarValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndZFarValue, Str_FromFloat(Max(Str_ToFloat(string) + 1.0f, 2.0f), -1));
				else
					SetWindowText(win_lightParmsEditor.hWndZFarValue, Str_FromFloat(Max(Str_ToFloat(string) - 1.0f, 2.0f), -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndFogDistanceSpin){
				GetWindowText(win_lightParmsEditor.hWndFogDistanceValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndFogDistanceValue, Str_FromFloat(Max(Str_ToFloat(string) + 1.0f, 1.0f), -1));
				else
					SetWindowText(win_lightParmsEditor.hWndFogDistanceValue, Str_FromFloat(Max(Str_ToFloat(string) - 1.0f, 1.0f), -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndFogHeightSpin){
				GetWindowText(win_lightParmsEditor.hWndFogHeightValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndFogHeightValue, Str_FromFloat(Max(Str_ToFloat(string) + 1.0f, 1.0f), -1));
				else
					SetWindowText(win_lightParmsEditor.hWndFogHeightValue, Str_FromFloat(Max(Str_ToFloat(string) - 1.0f, 1.0f), -1));

				WIN_ApplyLightParameters();

				break;
			}

			if (((LPNMHDR)lParam)->hwndFrom == win_lightParmsEditor.hWndAlphaSpin){
				GetWindowText(win_lightParmsEditor.hWndAlphaValue, string, sizeof(string));

				if (((LPNMUPDOWN)lParam)->iDelta < 0)
					SetWindowText(win_lightParmsEditor.hWndAlphaValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) + 0.01f, 0.0f, 1.0f), -1));
				else
					SetWindowText(win_lightParmsEditor.hWndAlphaValue, Str_FromFloat(ClampFloat(Str_ToFloat(string) - 0.01f, 0.0f, 1.0f), -1));

				WIN_ApplyLightParameters();

				break;
			}
		}

		break;
	}

	// Pass all unhandled messages to DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 ==================
 WIN_CreateLightEditorWindow
 ==================
*/
void *WIN_CreateLightEditorWindow (){

	INITCOMMONCONTROLSEX	initCommonControls;
	WNDCLASSEX				wndClass;
	RECT					rect;
	HDC						hDC;
	int						screenWidth, screenHeight;
	int						x, y, w, h;
	int						size;
	int						i;

	// Get the instance handle
	win_lightParmsEditor.hInstance = (HINSTANCE)Sys_GetInstanceHandle();

	// Initialize up-down control class
	initCommonControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
	initCommonControls.dwICC = ICC_UPDOWN_CLASS;

	InitCommonControlsEx(&initCommonControls);

	// Calculate window position and dimensions
	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);
	
	rect.left = (screenWidth - 566) / 2;
	rect.top = (screenHeight - 644) / 2;
	rect.right = rect.left + 566;
	rect.bottom = rect.top + 644;

	AdjustWindowRectEx(&rect, LIGHT_EDITOR_WINDOW_STYLE, FALSE, 0);
	
	x = rect.left;
	y = rect.top;
	w = rect.right - rect.left;
	h = rect.bottom - rect.top;

	// Register the window class
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = WIN_LightEditorWindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = (HINSTANCE)Sys_GetInstanceHandle();
	wndClass.hIcon = (HICON)Sys_GetIconHandle();
	wndClass.hIconSm = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = LIGHT_EDITOR_WINDOW_CLASS;

	if (!RegisterClassEx(&wndClass)){
		Com_Printf(S_COLOR_RED "Could not register light editor window class\n");

		return NULL;
	}

	// Create the window
	win_lightParmsEditor.hWnd = CreateWindowEx(0, LIGHT_EDITOR_WINDOW_CLASS, LIGHT_EDITOR_WINDOW_NAME, LIGHT_EDITOR_WINDOW_STYLE, x, y, w, h, NULL, NULL, win_lightParmsEditor.hInstance, NULL);
	if (!win_lightParmsEditor.hWnd){
		UnregisterClass(LIGHT_EDITOR_WINDOW_CLASS, win_lightParmsEditor.hInstance);

		Com_Printf(S_COLOR_RED "Could not create light editor window\n");

		return NULL;
	}

	// Create the controls
	win_lightParmsEditor.hWndName = CreateWindowEx(0, "STATIC", "Name", WS_CHILD | WS_VISIBLE | SS_LEFT, 8, 12, 180, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndNameValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 8, 26, 180, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFrame1 = CreateWindowEx(0, "BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 8, 84, 310, 94, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFrame2 = CreateWindowEx(0, "BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 8, 184, 310, 272, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFrame3 = CreateWindowEx(0, "BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 326, 84, 232, 372, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFrame4 = CreateWindowEx(0, "BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 8, 464, 550, 56, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFrame5 = CreateWindowEx(0, "BUTTON", NULL, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 8, 528, 550, 62, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndX = CreateWindowEx(0, "STATIC", "X", WS_CHILD | WS_VISIBLE | SS_CENTER, 66, 92, 80, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndY = CreateWindowEx(0, "STATIC", "Y", WS_CHILD | WS_VISIBLE | SS_CENTER, 150, 92, 80, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndZ = CreateWindowEx(0, "STATIC", "Z", WS_CHILD | WS_VISIBLE | SS_CENTER, 234, 92, 80, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndOrigin = CreateWindowEx(0, "STATIC", "Origin", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 109, 50, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndOriginXValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 66, 106, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndOriginXSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 126, 106, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndOriginYValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 150, 106, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndOriginYSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 210, 106, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndOriginZValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 234, 106, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndOriginZSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 294, 106, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndCenter = CreateWindowEx(0, "STATIC", "Center", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 133, 50, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndCenterXValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 66, 130, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndCenterXSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 126, 130, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndCenterYValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 150, 130, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndCenterYSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 210, 130, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndCenterZValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 234, 130, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndCenterZSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 294, 130, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndAngles = CreateWindowEx(0, "STATIC", "Angles", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 157, 50, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndAnglesXValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 66, 154, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndAnglesXSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 126, 154, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndAnglesYValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 150, 154, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndAnglesYSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 210, 154, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndAnglesZValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 234, 154, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndAnglesZSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 294, 154, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndType = CreateWindowEx(0, "STATIC", "Type", WS_CHILD | WS_VISIBLE | SS_RIGHT, 150, 198, 80, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndTypeValue = CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 234, 194, 80, 200, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRadiusX = CreateWindowEx(0, "STATIC", "X", WS_CHILD | WS_VISIBLE | SS_CENTER, 66, 250, 80, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRadiusY = CreateWindowEx(0, "STATIC", "Y", WS_CHILD | WS_VISIBLE | SS_CENTER, 150, 250, 80, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRadiusZ = CreateWindowEx(0, "STATIC", "Z", WS_CHILD | WS_VISIBLE | SS_CENTER, 234, 250, 80, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRadius = CreateWindowEx(0, "STATIC", "Radius", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 267, 50, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRadiusXValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 66, 264, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRadiusXSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 126, 264, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRadiusYValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 150, 264, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRadiusYSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 210, 264, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRadiusZValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 234, 264, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRadiusZSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 294, 264, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFrustumX = CreateWindowEx(0, "STATIC", "X", WS_CHILD | WS_VISIBLE | SS_CENTER, 66, 302, 80, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFrustumY = CreateWindowEx(0, "STATIC", "Y", WS_CHILD | WS_VISIBLE | SS_CENTER, 150, 302, 80, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFrustumZ = CreateWindowEx(0, "STATIC", "Z", WS_CHILD | WS_VISIBLE | SS_CENTER, 234, 302, 80, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFrustumMin = CreateWindowEx(0, "STATIC", "Minimum", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 319, 50, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFrustumMax = CreateWindowEx(0, "STATIC", "Maximum", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 343, 50, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndXMinValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 66, 316, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndXMinSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 126, 316, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndXMaxValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 66, 340, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndXMaxSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 126, 340, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndYMinValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 150, 316, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndYMinSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 210, 316, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndYMaxValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 150, 340, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndYMaxSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 210, 340, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndZNearValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 234, 316, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndZNearSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 294, 316, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndZFarValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 234, 340, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndZFarSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 294, 340, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndCastShadows = CreateWindowEx(0, "BUTTON", "Cast shadows", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 330, 94, 140, 16, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFogDistance = CreateWindowEx(0, "STATIC", "Fog distance", WS_CHILD | WS_VISIBLE | SS_RIGHT, 330, 210, 140, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFogDistanceValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 474, 207, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFogDistanceSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 534, 207, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFogHeight = CreateWindowEx(0, "STATIC", "Fog height", WS_CHILD | WS_VISIBLE | SS_RIGHT, 330, 234, 140, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFogHeightValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 474, 231, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndFogHeightSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 534, 231, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndDetailLevel = CreateWindowEx(0, "STATIC", "Detail level", WS_CHILD | WS_VISIBLE | SS_RIGHT, 330, 370, 140, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndDetailLevelValue = CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 474, 366, 80, 200, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndColor = CreateWindowEx(0, "STATIC", "Color", WS_CHILD | WS_VISIBLE | SS_RIGHT, 330, 411, 140, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndColorValue = CreateWindowEx(0, "BUTTON", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_BITMAP, 474, 408, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndAlpha = CreateWindowEx(0, "STATIC", "Alpha", WS_CHILD | WS_VISIBLE | SS_RIGHT, 330, 435, 140, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndAlphaValue = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_RIGHT | ES_AUTOHSCROLL, 474, 432, 80, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndAlphaSpin = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_AUTOBUDDY, 534, 432, 20, 20, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndMaterial = CreateWindowEx(0, "STATIC", "Material", WS_CHILD | WS_VISIBLE | SS_RIGHT, 12, 544, 134, 14, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndMaterialValue = CreateWindowEx(WS_EX_CLIENTEDGE, "COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWN | CBS_SORT | CBS_AUTOHSCROLL, 150, 540, 404, 200, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndApply = CreateWindowEx(0, "BUTTON", "Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 8, 613, 75, 23, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndReset = CreateWindowEx(0, "BUTTON", "Reset", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 87, 613, 75, 23, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndDefaults = CreateWindowEx(0, "BUTTON", "Defaults", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 166, 613, 75, 23, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndRemove = CreateWindowEx(0, "BUTTON", "Remove", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 245, 613, 75, 23, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);
	win_lightParmsEditor.hWndSave = CreateWindowEx(0, "BUTTON", "Save", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 483, 613, 75, 23, win_lightParmsEditor.hWnd, NULL, win_lightParmsEditor.hInstance, NULL);

	// Create and set the font
	hDC = GetDC(win_lightParmsEditor.hWnd);
	size = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ReleaseDC(win_lightParmsEditor.hWnd, hDC);

	win_lightParmsEditor.hFont = CreateFont(size, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Microsoft Sans Serif");

	SendMessage(win_lightParmsEditor.hWndName, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndNameValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFrame1, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFrame2, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFrame3, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFrame4, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndX, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndY, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndZ, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndOrigin, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndOriginXValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndOriginYValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndOriginZValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndCenter, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndCenterXValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndCenterYValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndCenterZValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndAngles, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndAnglesXValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndAnglesYValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndAnglesZValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndType, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndTypeValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndRadiusX, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndRadiusY, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndRadiusZ, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndRadius, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndRadiusXValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndRadiusYValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndRadiusZValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFrustumX, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFrustumY, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFrustumZ, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFrustumMin, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFrustumMax, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndXMinValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndXMaxValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndYMinValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndYMaxValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndZNearValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndZFarValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndCastShadows, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFogDistance, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFogDistanceValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFogHeight, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndFogHeightValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndDetailLevel, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndDetailLevelValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndColor, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndAlpha, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndAlphaValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndMaterial, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndMaterialValue, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndApply, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndReset, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndDefaults, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndRemove, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);
	SendMessage(win_lightParmsEditor.hWndSave, WM_SETFONT, (WPARAM)win_lightParmsEditor.hFont, FALSE);

	// Create and set the bitmap
	win_lightParmsEditor.hBitmap = CreateBitmap(80, 20, 1, 32, NULL);

	for (i = 0; i < LIGHT_COLOR_BITMAP_SIZE; i++)
		win_lightParmsEditor.bitmapBits[i] = 0xFFFFFFFF;

	SetBitmapBits(win_lightParmsEditor.hBitmap, LIGHT_COLOR_BITMAP_SIZE * 4, win_lightParmsEditor.bitmapBits);
	SendMessage(win_lightParmsEditor.hWndColorValue, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)win_lightParmsEditor.hBitmap);

	// Fill type combo box
	SendMessage(win_lightParmsEditor.hWndTypeValue, CB_ADDSTRING, 0, (LPARAM)"Point");
	SendMessage(win_lightParmsEditor.hWndTypeValue, CB_ADDSTRING, 0, (LPARAM)"Cubic");
	SendMessage(win_lightParmsEditor.hWndTypeValue, CB_ADDSTRING, 0, (LPARAM)"Projected");
	SendMessage(win_lightParmsEditor.hWndTypeValue, CB_ADDSTRING, 0, (LPARAM)"Directional");

	// Fill detail level combo box
	SendMessage(win_lightParmsEditor.hWndDetailLevelValue, CB_ADDSTRING, 0, (LPARAM)"Low");
	SendMessage(win_lightParmsEditor.hWndDetailLevelValue, CB_ADDSTRING, 0, (LPARAM)"Medium");
	SendMessage(win_lightParmsEditor.hWndDetailLevelValue, CB_ADDSTRING, 0, (LPARAM)"High");

	// Fill material combo box
	SendMessage(win_lightParmsEditor.hWndMaterialValue, CB_ADDSTRING, 0, (LPARAM)"_defaultLight");
	SendMessage(win_lightParmsEditor.hWndMaterialValue, CB_ADDSTRING, 0, (LPARAM)"_defaultProjectedLight");

	R_EnumMaterialDefs(WIN_AddLightMaterial);

	// Set text limit for name edit box
	SendMessage(win_lightParmsEditor.hWndNameValue, CB_LIMITTEXT, (WPARAM)(MAX_LIGHT_NAME_LENGTH - 1), 0);

	// Set text limit for material combo box
	SendMessage(win_lightParmsEditor.hWndMaterialValue, CB_LIMITTEXT, (WPARAM)(MAX_PATH_LENGTH - 1), 0);

	// Set the current color
	win_lightParmsEditor.crCurrent = RGB(255, 255, 255);

	// Set the custom colors
	for (i = 0; i < 16; i++)
		win_lightParmsEditor.crCustom[i] = RGB(255, 255, 255);

	// Show the window
	ShowWindow(win_lightParmsEditor.hWnd, SW_SHOW);
	UpdateWindow(win_lightParmsEditor.hWnd);
	SetForegroundWindow(win_lightParmsEditor.hWnd);
	SetFocus(win_lightParmsEditor.hWnd);

	win_lightParmsEditor.initialized = true;

	return win_lightParmsEditor.hWnd;
}

/*
 ==================
 WIN_DestroyLightEditorWindow
 ==================
*/
void WIN_DestroyLightEditorWindow (){

	if (!win_lightParmsEditor.initialized)
		return;

	if (win_lightParmsEditor.hBitmap)
		DeleteObject(win_lightParmsEditor.hBitmap);

	if (win_lightParmsEditor.hFont)
		DeleteObject(win_lightParmsEditor.hFont);

	ShowWindow(win_lightParmsEditor.hWnd, SW_HIDE);
	DestroyWindow(win_lightParmsEditor.hWnd);

	UnregisterClass(LIGHT_EDITOR_WINDOW_CLASS, win_lightParmsEditor.hInstance);

	Mem_Fill(&win_lightParmsEditor, 0, sizeof(lightParmsEditor_t));
}

/*
 ==================
 WIN_EditLightParameters
 ==================
*/
void WIN_EditLightParameters (int index, lightParms_t *parms){

	byte	r, g, b;
	int		i;

	if (!win_lightParmsEditor.initialized)
		return;

	// Apply current light parameters
	WIN_ApplyLightParameters();

	// Set the current light parameters
	if (parms){
		win_lightParmsEditor.enabled = true;

		win_lightParmsEditor.editIndex = index;
		win_lightParmsEditor.editParms = *parms;
	}
	else {
		win_lightParmsEditor.enabled = false;

		parms = &win_lightParms;
	}

	// Update the controls
	SetWindowText(win_lightParmsEditor.hWndNameValue, parms->name);

	SetWindowText(win_lightParmsEditor.hWndOriginXValue, Str_FromFloat(parms->origin[0], -1));
	SetWindowText(win_lightParmsEditor.hWndOriginYValue, Str_FromFloat(parms->origin[1], -1));
	SetWindowText(win_lightParmsEditor.hWndOriginZValue, Str_FromFloat(parms->origin[2], -1));

	SetWindowText(win_lightParmsEditor.hWndCenterXValue, Str_FromFloat(parms->center[0], -1));
	SetWindowText(win_lightParmsEditor.hWndCenterYValue, Str_FromFloat(parms->center[1], -1));
	SetWindowText(win_lightParmsEditor.hWndCenterZValue, Str_FromFloat(parms->center[2], -1));

	SetWindowText(win_lightParmsEditor.hWndAnglesXValue, Str_FromFloat(parms->angles[0], -1));
	SetWindowText(win_lightParmsEditor.hWndAnglesYValue, Str_FromFloat(parms->angles[1], -1));
	SetWindowText(win_lightParmsEditor.hWndAnglesZValue, Str_FromFloat(parms->angles[2], -1));

	SendMessage(win_lightParmsEditor.hWndTypeValue, CB_SETCURSEL, (WPARAM)ClampInt(parms->type, 0, 3), 0);

	SetWindowText(win_lightParmsEditor.hWndRadiusXValue, Str_FromFloat(parms->radius[0], -1));
	SetWindowText(win_lightParmsEditor.hWndRadiusYValue, Str_FromFloat(parms->radius[1], -1));
	SetWindowText(win_lightParmsEditor.hWndRadiusZValue, Str_FromFloat(parms->radius[2], -1));

	SetWindowText(win_lightParmsEditor.hWndXMinValue, Str_FromFloat(parms->xMin, -1));
	SetWindowText(win_lightParmsEditor.hWndXMaxValue, Str_FromFloat(parms->xMax, -1));

	SetWindowText(win_lightParmsEditor.hWndYMinValue, Str_FromFloat(parms->yMin, -1));
	SetWindowText(win_lightParmsEditor.hWndYMaxValue, Str_FromFloat(parms->yMax, -1));

	SetWindowText(win_lightParmsEditor.hWndZNearValue, Str_FromFloat(parms->zNear, -1));
	SetWindowText(win_lightParmsEditor.hWndZFarValue, Str_FromFloat(parms->zFar, -1));

	if (parms->noShadows)
		SendMessage(win_lightParmsEditor.hWndCastShadows, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
	else
		SendMessage(win_lightParmsEditor.hWndCastShadows, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

	SetWindowText(win_lightParmsEditor.hWndFogDistanceValue, Str_FromFloat(parms->fogDistance, -1));
	SetWindowText(win_lightParmsEditor.hWndFogHeightValue, Str_FromFloat(parms->fogHeight, -1));

	SendMessage(win_lightParmsEditor.hWndDetailLevelValue, CB_SETCURSEL, (WPARAM)ClampInt(parms->detailLevel, 0, 2), 0);

	r = FloatToByte(parms->materialParms[0] * 255.0f);
	g = FloatToByte(parms->materialParms[1] * 255.0f);
	b = FloatToByte(parms->materialParms[2] * 255.0f);

	win_lightParmsEditor.crCurrent = RGB(r, g, b);

	for (i = 0; i < LIGHT_COLOR_BITMAP_SIZE; i++)
		win_lightParmsEditor.bitmapBits[i] = (b << 0) | (g << 8) | (r << 16) | (255 << 24);

	SetBitmapBits(win_lightParmsEditor.hBitmap, LIGHT_COLOR_BITMAP_SIZE * 4, win_lightParmsEditor.bitmapBits);
	SendMessage(win_lightParmsEditor.hWndColorValue, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)win_lightParmsEditor.hBitmap);

	SetWindowText(win_lightParmsEditor.hWndAlphaValue, Str_FromFloat(ClampFloat(parms->materialParms[3], 0.0f, 1.0f), -1));

	SetWindowText(win_lightParmsEditor.hWndMaterialValue, parms->material);

	// Enable or disable the controls
	EnableWindow(win_lightParmsEditor.hWndName, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndNameValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndFrame1, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndFrame2, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndFrame3, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndFrame4, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndX, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndY, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndZ, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndOrigin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndOriginXValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndOriginXSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndOriginYValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndOriginYSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndOriginZValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndOriginZSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndCenter, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndCenterXValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndCenterXSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndCenterYValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndCenterYSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndCenterZValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndCenterZSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndAngles, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndAnglesXValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndAnglesXSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndAnglesYValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndAnglesYSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndAnglesZValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndAnglesZSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndType, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndTypeValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndRadiusX, win_lightParmsEditor.enabled && parms->type != RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndRadiusY, win_lightParmsEditor.enabled && parms->type != RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndRadiusZ, win_lightParmsEditor.enabled && parms->type != RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndRadius, win_lightParmsEditor.enabled && parms->type != RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndRadiusXValue, win_lightParmsEditor.enabled && parms->type != RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndRadiusXSpin, win_lightParmsEditor.enabled && parms->type != RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndRadiusYValue, win_lightParmsEditor.enabled && parms->type != RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndRadiusYSpin, win_lightParmsEditor.enabled && parms->type != RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndRadiusZValue, win_lightParmsEditor.enabled && parms->type != RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndRadiusZSpin, win_lightParmsEditor.enabled && parms->type != RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndFrustumX, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndFrustumY, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndFrustumZ, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndFrustumMin, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndFrustumMax, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndXMinValue, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndXMinSpin, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndXMaxValue, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndXMaxSpin, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndYMinValue, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndYMinSpin, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndYMaxValue, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndYMaxSpin, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndZNearValue, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndZNearSpin, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndZFarValue, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndZFarSpin, win_lightParmsEditor.enabled && parms->type == RL_PROJECTED);
	EnableWindow(win_lightParmsEditor.hWndCastShadows, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndFogDistance, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndFogDistanceValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndFogDistanceSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndFogHeight, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndFogHeightValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndFogHeightSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndDetailLevel, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndDetailLevelValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndColor, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndColorValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndAlpha, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndAlphaValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndAlphaSpin, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndMaterial, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndMaterialValue, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndApply, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndReset, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndDefaults, win_lightParmsEditor.enabled);
	EnableWindow(win_lightParmsEditor.hWndRemove, win_lightParmsEditor.enabled);

	// Show the window
	ShowWindow(win_lightParmsEditor.hWnd, SW_RESTORE);
	UpdateWindow(win_lightParmsEditor.hWnd);
	SetForegroundWindow(win_lightParmsEditor.hWnd);
	SetFocus(win_lightParmsEditor.hWnd);
}

#else

/*
 ==================
 WIN_CreateLightEditorWindow
 ==================
*/
void *WIN_CreateLightEditorWindow (){

	Com_Printf(S_COLOR_RED "The light editor is not currently supported on this platform");

	return NULL;
}

/*
 ==================
 WIN_DestroyLightEditorWindow
 ==================
*/
void WIN_DestroyLightEditorWindow (){

}

/*
 ==================
 WIN_EditLightParameters
 ==================
*/
void WIN_EditLightParameters (int index, lightParms_t *parms){

}

#endif