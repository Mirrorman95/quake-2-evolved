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
// s_reverbEditor.c - Environmental reverberation effects editor
//


#include "s_local.h"


static reverbParms_t		s_reverbParmsGeneric = {1.0f, 1.0f, 0.32f, 0.89f, 1.0f, 1.49f, 0.83f, 1.0f, AL_TRUE, 0.05f, 0.007f, {0.0f, 0.0f, 0.0f}, 1.26f, 0.011f, {0.0f, 0.0f, 0.0f}, 0.25f, 0.0f, 0.25f, 0.0f, 5000.0f, 250.0f, 0.994f, 0.0f};
static reverbParms_t		s_reverbParmsUnderwater = {0.3645f, 1.0f, 0.32f, 0.01f, 1.0f, 1.49f, 0.1f, 1.0f, AL_TRUE, 0.596348f, 0.007f, {0.0f, 0.0f, 0.0f}, 7.07946f, 0.011f, {0.0f, 0.0f, 0.0f}, 0.25f, 0.0f, 1.18f, 0.348f, 5000.0f, 250.0f, 0.994f, 0.0f};


/*
 ==============================================================================

 CALLBACK FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 S_EditAreaReverb
 ==================
*/
void S_EditAreaReverb (int area){

	if (!snd.reverb.editing)
		return;		// Not active

	if (snd.reverb.editingArea == area)
		return;		// Area hasn't changed
	snd.reverb.editingArea = area;

	// Edit the specified area reverb parameters
	if (area == -1){
		WIN_EditReverbParameters(-1, NULL);
		return;
	}

	WIN_EditReverbParameters(area, &snd.reverb.reverbList[area]);
}

/*
 ==================
 S_ReverbEditorUpdateCallback
 ==================
*/
void S_ReverbEditorUpdateCallback (int index, reverbParms_t *parms){

	reverbParms_t	*reverbParms;

	if (!snd.reverb.editing)
		return;		// Not active

	if (index < 0 || index >= CM_NumAreas())
		return;		// Out of range

	reverbParms = &snd.reverb.reverbList[index];

	// Copy the parameters
	Mem_Copy(reverbParms, parms, sizeof(reverbParms_t));

	// Clamp and renormalize the parameters
	reverbParms->density = ClampFloat(reverbParms->density, AL_EAXREVERB_MIN_DENSITY, AL_EAXREVERB_MAX_DENSITY);
	reverbParms->diffusion = ClampFloat(reverbParms->diffusion, AL_EAXREVERB_MIN_DIFFUSION, AL_EAXREVERB_MAX_DIFFUSION);
	reverbParms->gain = ClampFloat(reverbParms->gain, AL_EAXREVERB_MIN_GAIN, AL_EAXREVERB_MAX_GAIN);
	reverbParms->gainHF = ClampFloat(reverbParms->gainHF, AL_EAXREVERB_MIN_GAINHF, AL_EAXREVERB_MAX_GAINHF);
	reverbParms->gainLF = ClampFloat(reverbParms->gainLF, AL_EAXREVERB_MIN_GAINLF, AL_EAXREVERB_MAX_GAINLF);
	reverbParms->decayTime = ClampFloat(reverbParms->decayTime, AL_EAXREVERB_MIN_DECAY_TIME, AL_EAXREVERB_MAX_DECAY_TIME);
	reverbParms->decayHFRatio = ClampFloat(reverbParms->decayHFRatio, AL_EAXREVERB_MIN_DECAY_HFRATIO, AL_EAXREVERB_MAX_DECAY_HFRATIO);
	reverbParms->decayLFRatio = ClampFloat(reverbParms->decayLFRatio, AL_EAXREVERB_MIN_DECAY_LFRATIO, AL_EAXREVERB_MAX_DECAY_LFRATIO);
	reverbParms->decayHFLimit = ClampInt(reverbParms->decayHFLimit, AL_EAXREVERB_MIN_DECAY_HFLIMIT, AL_EAXREVERB_MAX_DECAY_HFLIMIT);
	reverbParms->reflectionsGain = ClampFloat(reverbParms->reflectionsGain, AL_EAXREVERB_MIN_REFLECTIONS_GAIN, AL_EAXREVERB_MAX_REFLECTIONS_GAIN);
	reverbParms->reflectionsDelay = ClampFloat(reverbParms->reflectionsDelay, AL_EAXREVERB_MIN_REFLECTIONS_DELAY, AL_EAXREVERB_MAX_REFLECTIONS_DELAY);
	VectorNormalize(reverbParms->reflectionsPan);
	reverbParms->lateReverbGain = ClampFloat(reverbParms->lateReverbGain, AL_EAXREVERB_MIN_LATE_REVERB_GAIN, AL_EAXREVERB_MAX_LATE_REVERB_GAIN);
	reverbParms->lateReverbDelay = ClampFloat(reverbParms->lateReverbDelay, AL_EAXREVERB_MIN_LATE_REVERB_DELAY, AL_EAXREVERB_MAX_LATE_REVERB_DELAY);
	VectorNormalize(reverbParms->lateReverbPan);
	reverbParms->echoTime = ClampFloat(reverbParms->echoTime, AL_EAXREVERB_MIN_ECHO_TIME, AL_EAXREVERB_MAX_ECHO_TIME);
	reverbParms->echoDepth = ClampFloat(reverbParms->echoDepth, AL_EAXREVERB_MIN_ECHO_DEPTH, AL_EAXREVERB_MAX_ECHO_DEPTH);
	reverbParms->modulationTime = ClampFloat(reverbParms->modulationTime, AL_EAXREVERB_MIN_MODULATION_TIME, AL_EAXREVERB_MAX_MODULATION_TIME);
	reverbParms->modulationDepth = ClampFloat(reverbParms->modulationDepth, AL_EAXREVERB_MIN_MODULATION_DEPTH, AL_EAXREVERB_MAX_MODULATION_DEPTH);
	reverbParms->hfReference = ClampFloat(reverbParms->hfReference, AL_EAXREVERB_MIN_HFREFERENCE, AL_EAXREVERB_MAX_HFREFERENCE);
	reverbParms->lfReference = ClampFloat(reverbParms->lfReference, AL_EAXREVERB_MIN_LFREFERENCE, AL_EAXREVERB_MAX_LFREFERENCE);
	reverbParms->airAbsorptionGainHF = ClampFloat(reverbParms->airAbsorptionGainHF, AL_EAXREVERB_MIN_AIR_ABSORPTION_GAINHF, AL_EAXREVERB_MAX_AIR_ABSORPTION_GAINHF);
	reverbParms->roomRolloffFactor = ClampFloat(reverbParms->roomRolloffFactor, AL_EAXREVERB_MIN_ROOM_ROLLOFF_FACTOR, AL_EAXREVERB_MAX_ROOM_ROLLOFF_FACTOR);

	// Force an update
	snd.reverb.time = 0;

	Mem_Copy(&snd.reverb.previous, &s_reverbParmsGeneric, sizeof(reverbParms_t));
	snd.reverb.current = NULL;
}

/*
 ==================
 S_ReverbEditorSaveCallback
 ==================
*/
void S_ReverbEditorSaveCallback (){

	fileHandle_t	f;
	reverbParms_t	*reverbParms;
	int				i;

	if (!snd.reverb.editing)
		return;		// Not active

	if (!snd.reverb.reverbName[0]){
		Com_Printf("No reverb file name available\n");
		return;
	}

	// Write the reverb file
	FS_OpenFile(snd.reverb.reverbName, FS_WRITE, &f);
	if (!f){
		Com_Printf("Couldn't write reverb file %s\n", snd.reverb.reverbName);
		return;
	}

	for (i = 0, reverbParms = snd.reverb.reverbList; i < CM_NumAreas(); i++, reverbParms++){
		FS_Printf(f, "reverb %i" NEWLINE, i);
		FS_Printf(f, "{" NEWLINE);
		FS_Printf(f, "density             %g" NEWLINE, reverbParms->density);
		FS_Printf(f, "diffusion           %g" NEWLINE, reverbParms->diffusion);
		FS_Printf(f, "gain                %g" NEWLINE, reverbParms->gain);
		FS_Printf(f, "gainHF              %g" NEWLINE, reverbParms->gainHF);
		FS_Printf(f, "gainLF              %g" NEWLINE, reverbParms->gainLF);
		FS_Printf(f, "decayTime           %g" NEWLINE, reverbParms->decayTime);
		FS_Printf(f, "decayHFRatio        %g" NEWLINE, reverbParms->decayHFRatio);
		FS_Printf(f, "decayLFRatio        %g" NEWLINE, reverbParms->decayLFRatio);
		FS_Printf(f, "decayHFLimit        %i" NEWLINE, reverbParms->decayHFLimit);
		FS_Printf(f, "reflectionsGain     %g" NEWLINE, reverbParms->reflectionsGain);
		FS_Printf(f, "reflectionsDelay    %g" NEWLINE, reverbParms->reflectionsDelay);
		FS_Printf(f, "reflectionsPan      ( %g %g %g )" NEWLINE, reverbParms->reflectionsPan[0], reverbParms->reflectionsPan[1], reverbParms->reflectionsPan[2]);
		FS_Printf(f, "lateReverbGain      %g" NEWLINE, reverbParms->lateReverbGain);
		FS_Printf(f, "lateReverbDelay     %g" NEWLINE, reverbParms->lateReverbDelay);
		FS_Printf(f, "lateReverbPan       ( %g %g %g )" NEWLINE, reverbParms->lateReverbPan[0], reverbParms->lateReverbPan[1], reverbParms->lateReverbPan[2]);
		FS_Printf(f, "echoTime            %g" NEWLINE, reverbParms->echoTime);
		FS_Printf(f, "echoDepth           %g" NEWLINE, reverbParms->echoDepth);
		FS_Printf(f, "modulationTime      %g" NEWLINE, reverbParms->modulationTime);
		FS_Printf(f, "modulationDepth     %g" NEWLINE, reverbParms->modulationDepth);
		FS_Printf(f, "hfReference         %g" NEWLINE, reverbParms->hfReference);
		FS_Printf(f, "lfReference         %g" NEWLINE, reverbParms->lfReference);
		FS_Printf(f, "airAbsorptionGainHF %g" NEWLINE, reverbParms->airAbsorptionGainHF);
		FS_Printf(f, "roomRolloffFactor   %g" NEWLINE, reverbParms->roomRolloffFactor);
		FS_Printf(f, "}" NEWLINE);

		if (i < CM_NumAreas() - 1)
			FS_Printf(f, NEWLINE);
	}

	FS_CloseFile(f);

	Com_Printf("Wrote reverb file %s with %i areas\n", snd.reverb.reverbName, CM_NumAreas());
}

/*
 ==================
 S_ReverbEditorCloseCallback
 ==================
*/
void S_ReverbEditorCloseCallback (){

	if (!snd.reverb.editing)
		return;		// Not active

	snd.reverb.editing = false;
	snd.reverb.editingArea = -1;

	// Close the reverb editor
	Com_CloseEditor();
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 S_EditReverb_f
 ==================
*/
static void S_EditReverb_f (){

	editorCallbacks_t	callbacks;

	if (!snd.reverb.enabled){
		Com_Printf("You must enable reverb effects to launch the reverb editor\n");
		return;
	}

	// Launch the reverb editor
	callbacks.createWindow = WIN_CreateReverbEditorWindow;
	callbacks.destroyWindow = WIN_DestroyReverbEditorWindow;
	callbacks.mouseEvent = NULL;

	if (!Com_LaunchEditor("reverb", &callbacks))
		return;

	snd.reverb.editing = true;
	snd.reverb.editingArea = snd.listener.area;

	// Edit the current area reverb parameters
	if (snd.listener.area == -1){
		WIN_EditReverbParameters(-1, NULL);
		return;
	}

	WIN_EditReverbParameters(snd.listener.area, &snd.reverb.reverbList[snd.listener.area]);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 S_InitReverbs
 ==================
*/
void S_InitReverbs (){

	int		i;

	// Add commands
	Cmd_AddCommand("editReverb", S_EditReverb_f, "Launches the integrated reverb editor", NULL);

	// Fill in defaults
	Mem_Copy(&snd.reverb.reverbGeneric, &s_reverbParmsGeneric, sizeof(reverbParms_t));
	Mem_Copy(&snd.reverb.reverbUnderwater, &s_reverbParmsUnderwater, sizeof(reverbParms_t));

	for (i = 0; i < MAX_REVERB_AREAS; i++)
		Mem_Copy(&snd.reverb.reverbList[i], &s_reverbParmsGeneric, sizeof(reverbParms_t));

	Mem_Copy(&snd.reverb.reverbParms, &s_reverbParmsGeneric, sizeof(reverbParms_t));

	// Clear interpolation parameters
	snd.reverb.time = 0;

	Mem_Copy(&snd.reverb.previous, &s_reverbParmsGeneric, sizeof(reverbParms_t));
	snd.reverb.current = NULL;

	// Enable reverb effects if possible
	if (!s_reverbEffects->integerValue || !alConfig.efxAvailable)
		return;

	snd.reverb.enabled = true;

	// Create and set up the effect
	qalGenEffects(1, &snd.reverb.effectId);
	qalEffecti(snd.reverb.effectId, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_DENSITY, s_reverbParmsGeneric.density);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_DIFFUSION, s_reverbParmsGeneric.diffusion);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_GAIN, s_reverbParmsGeneric.gain);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_GAINHF, s_reverbParmsGeneric.gainHF);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_GAINLF, s_reverbParmsGeneric.gainLF);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_DECAY_TIME, s_reverbParmsGeneric.decayTime);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_DECAY_HFRATIO, s_reverbParmsGeneric.decayHFRatio);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_DECAY_LFRATIO, s_reverbParmsGeneric.decayLFRatio);
	qalEffecti(snd.reverb.effectId, AL_EAXREVERB_DECAY_HFLIMIT, s_reverbParmsGeneric.decayHFLimit);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_REFLECTIONS_GAIN, s_reverbParmsGeneric.reflectionsGain);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_REFLECTIONS_DELAY, s_reverbParmsGeneric.reflectionsDelay);
	qalEffectfv(snd.reverb.effectId, AL_EAXREVERB_REFLECTIONS_PAN, s_reverbParmsGeneric.reflectionsPan);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_LATE_REVERB_GAIN, s_reverbParmsGeneric.lateReverbGain);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_LATE_REVERB_DELAY, s_reverbParmsGeneric.lateReverbDelay);
	qalEffectfv(snd.reverb.effectId, AL_EAXREVERB_LATE_REVERB_PAN, s_reverbParmsGeneric.lateReverbPan);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_ECHO_TIME, s_reverbParmsGeneric.echoTime);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_ECHO_DEPTH, s_reverbParmsGeneric.echoDepth);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_MODULATION_TIME, s_reverbParmsGeneric.modulationTime);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_MODULATION_DEPTH, s_reverbParmsGeneric.modulationDepth);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_HFREFERENCE, s_reverbParmsGeneric.hfReference);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_LFREFERENCE, s_reverbParmsGeneric.lfReference);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, s_reverbParmsGeneric.airAbsorptionGainHF);
	qalEffectf(snd.reverb.effectId, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, s_reverbParmsGeneric.roomRolloffFactor);

	// Create and set up the auxiliary effect slot
	qalGenAuxiliaryEffectSlots(1, &snd.reverb.effectSlotId);
	qalAuxiliaryEffectSloti(snd.reverb.effectSlotId, AL_EFFECTSLOT_EFFECT, snd.reverb.effectId);
	qalAuxiliaryEffectSlotf(snd.reverb.effectSlotId, AL_EFFECTSLOT_GAIN, s_reverbVolume->floatValue);

	// Disable reverb effects if an error occurred
	if (qalGetError() != AL_NO_ERROR){
		qalAuxiliaryEffectSloti(snd.reverb.effectSlotId, AL_EFFECTSLOT_EFFECT, 0);
		qalDeleteAuxiliaryEffectSlots(1, &snd.reverb.effectSlotId);

		qalEffecti(snd.reverb.effectId, AL_EFFECT_TYPE, 0);
		qalDeleteEffects(1, &snd.reverb.effectId);

		qalGetError();

		// Clear the variables
		snd.reverb.enabled = false;

		snd.reverb.effectId = 0;
		snd.reverb.effectSlotId = 0;
	}
}

/*
 ==================
 S_ShutdownReverbs
 ==================
*/
void S_ShutdownReverbs (){

	// Remove commands
	Cmd_RemoveCommand("editReverb");

	// Close the reverb editor if active
	if (snd.reverb.editing)
		Com_CloseEditor();

	if (!snd.reverb.enabled)
		return;

	// Delete the auxiliary effect slot
	qalAuxiliaryEffectSloti(snd.reverb.effectSlotId, AL_EFFECTSLOT_EFFECT, 0);
	qalDeleteAuxiliaryEffectSlots(1, &snd.reverb.effectSlotId);

	// Delete the effect
	qalEffecti(snd.reverb.effectId, AL_EFFECT_TYPE, 0);
	qalDeleteEffects(1, &snd.reverb.effectId);
}