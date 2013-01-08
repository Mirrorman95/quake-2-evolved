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
// s_reverb.c - Environmental reverberation effects
//


#include "s_local.h"


/*
 ==============================================================================

 ENVIRONMENTAL REVERBERATION EFFECTS PARSING

 ==============================================================================
*/


/*
 ==================
 S_ParseReverb
 ==================
*/
static bool S_ParseReverb (script_t *script){

	token_t			token;
	reverbParms_t	*reverbParms;
	int				area;

	// Parse the area number
	if (!PS_ReadInteger(script, &area)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing area number in reverb file\n");
		return false;
	}

	if (area < 0 || area >= CM_NumAreas()){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %i for area number in reverb file\n", area);
		return false;
	}

	// Parse the reverb parameters
	reverbParms = &snd.reverb.reverbList[area];

	if (!PS_ExpectTokenString(script, &token, "{", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in reverb file (area %i)\n", token.string, area);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in reverb file (area %i)\n", area);
			return false;	// End of script
		}

		if (!Str_ICompare(token.string, "}"))
			break;			// End of reverb

		// Parse the parameter
		if (!Str_ICompare(token.string, "density")){
			if (!PS_ReadFloat(script, &reverbParms->density)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'density' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "diffusion")){
			if (!PS_ReadFloat(script, &reverbParms->diffusion)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'diffusion' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "gain")){
			if (!PS_ReadFloat(script, &reverbParms->gain)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'gain' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "gainHF")){
			if (!PS_ReadFloat(script, &reverbParms->gainHF)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'gainHF' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "gainLF")){
			if (!PS_ReadFloat(script, &reverbParms->gainLF)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'gainLF' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "decayTime")){
			if (!PS_ReadFloat(script, &reverbParms->decayTime)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'decayTime' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "decayHFRatio")){
			if (!PS_ReadFloat(script, &reverbParms->decayHFRatio)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'decayHFRatio' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "decayLFRatio")){
			if (!PS_ReadFloat(script, &reverbParms->decayLFRatio)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'decayLFRatio' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "decayHFLimit")){
			if (!PS_ReadInteger(script, &reverbParms->decayHFLimit)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'decayHFLimit' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "reflectionsGain")){
			if (!PS_ReadFloat(script, &reverbParms->reflectionsGain)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'reflectionsGain' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "reflectionsDelay")){
			if (!PS_ReadFloat(script, &reverbParms->reflectionsDelay)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'reflectionsDelay' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "reflectionsPan")){
			if (!PS_ReadMatrix1D(script, 3, reverbParms->reflectionsPan)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'reflectionsPan' parameters in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "lateReverbGain")){
			if (!PS_ReadFloat(script, &reverbParms->lateReverbGain)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'lateReverbGain' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "lateReverbDelay")){
			if (!PS_ReadFloat(script, &reverbParms->lateReverbDelay)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'lateReverbDelay' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "lateReverbPan")){
			if (!PS_ReadMatrix1D(script, 3, reverbParms->lateReverbPan)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'lateReverbPan' parameters in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "echoTime")){
			if (!PS_ReadFloat(script, &reverbParms->echoTime)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'echoTime' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "echoDepth")){
			if (!PS_ReadFloat(script, &reverbParms->echoDepth)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'echoDepth' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "modulationTime")){
			if (!PS_ReadFloat(script, &reverbParms->modulationTime)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'modulationTime' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "modulationDepth")){
			if (!PS_ReadFloat(script, &reverbParms->modulationDepth)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'modulationDepth' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "hfReference")){
			if (!PS_ReadFloat(script, &reverbParms->hfReference)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'hfReference' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "lfReference")){
			if (!PS_ReadFloat(script, &reverbParms->lfReference)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'lfReference' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "airAbsorptionGainHF")){
			if (!PS_ReadFloat(script, &reverbParms->airAbsorptionGainHF)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'airAbsorptionGainHF' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "roomRolloffFactor")){
			if (!PS_ReadFloat(script, &reverbParms->roomRolloffFactor)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing 'roomRolloffFactor' parameter in reverb file (area %i)\n", area);
				return false;
			}
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' in reverb file (area %i)\n", token.string, area);
			return false;
		}
	}

	// Clamp and renormalize the reverb parameters
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

	return true;
}

/*
 ==================
 S_LoadReverb
 ==================
*/
void S_LoadReverb (const char *name){

	script_t	*script;
	token_t		token;

	if (!snd.reverb.enabled)
		return;

	// Copy the name
	Str_Copy(snd.reverb.reverbName, name, sizeof(snd.reverb.reverbName));

	// Load the script file
	script = PS_LoadScriptFile(name);
	if (!script){
		Com_Printf(S_COLOR_RED "Reverb file %s not found\n", name);
		return;
	}

	PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS);

	// Parse it
	while (1){
		if (!PS_ReadToken(script, &token))
			break;		// End of script

		if (!Str_ICompare(token.string, "reverb")){
			if (!S_ParseReverb(script))
				break;
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: expected 'reverb', found '%s' instead in reverb file\n", token.string);
			break;
		}
	}

	// Free the script file
	PS_FreeScript(script);
}