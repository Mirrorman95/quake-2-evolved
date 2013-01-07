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
// s_soundShader.c - Sound shader definition parsing and loading
//


#include "s_local.h"


#define SOUND_SHADERS_HASH_SIZE		(MAX_SOUND_SHADERS >> 2)

typedef struct soundShaderDef_s {
	char					name[MAX_PATH_LENGTH];
	char *					text;
	int						length;

	char					source[MAX_PATH_LENGTH];
	int						line;

	struct soundShaderDef_s *	nextHash;
} soundShaderDef_t;

static soundShader_t		s_parseSoundShader;

static soundShaderDef_t *	s_soundShaderDefsHashTable[SOUND_SHADERS_HASH_SIZE];

static soundShader_t *		s_soundShadersHashTable[SOUND_SHADERS_HASH_SIZE];
static soundShader_t *		s_soundShaders[MAX_SOUND_SHADERS];
static int					s_numSoundShaders;


/*
 ==============================================================================

 SOUND SHADER PARSING

 ==============================================================================
*/


/*
 ==================
 S_ParsePrivate
 ==================
*/
static bool S_ParsePrivate (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_PRIVATE;

	return true;
}

/*
 ==================
 S_ParseAntiPrivate
 ==================
*/
static bool S_ParseAntiPrivate (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_ANTIPRIVATE;

	return true;
}

/*
 ==================
 S_ParsePlayOnce
 ==================
*/
static bool S_ParsePlayOnce (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_PLAYONCE;

	return true;
}

/*
 ==================
 S_ParseGlobal
 ==================
*/
static bool S_ParseGlobal (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_GLOBAL;

	return true;
}

/*
 ==================
 S_ParseOmnidirectional
 ==================
*/
static bool S_ParseOmnidirectional (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_OMNIDIRECTIONAL;

	return true;
}

/*
 ==================
 S_ParseVolumetric
 ==================
*/
static bool S_ParseVolumetric (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_VOLUMETRIC;

	return true;
}

/*
 ==================
 S_ParseLooping
 ==================
*/
static bool S_ParseLooping (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_LOOPING;

	return true;
}

/*
 ==================
 S_ParseRandomize
 ==================
*/
static bool S_ParseRandomize (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_RANDOMIZE;

	return true;
}

/*
 ==================
 S_ParseNoRestart
 ==================
*/
static bool S_ParseNoRestart (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NORESTART;

	return true;
}

/*
 ==================
 S_ParseNoOverride
 ==================
*/
static bool S_ParseNoOverride (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NOOVERRIDE;

	return true;
}

/*
 ==================
 S_ParseNoDups
 ==================
*/
static bool S_ParseNoDups (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NODUPS;

	return true;
}

/*
 ==================
 S_ParseNoOffset
 ==================
*/
static bool S_ParseNoOffset (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NOOFFSET;

	return true;
}

/*
 ==================
 S_ParseNoTimeOffset
 ==================
*/
static bool S_ParseNoTimeOffset (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NOTIMEOFFSET;

	return true;
}

/*
 ==================
 S_ParseNoFlicker
 ==================
*/
static bool S_ParseNoFlicker (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NOFLICKER;

	return true;
}

/*
 ==================
 S_ParseNoPortalFlow
 ==================
*/
static bool S_ParseNoPortalFlow (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NOPORTALFLOW;

	return true;
}

/*
 ==================
 S_ParseNoDynamicParms
 ==================
*/
static bool S_ParseNoDynamicParms (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NODYNAMICPARMS;

	return true;
}

/*
 ==================
 S_ParseNoReverb
 ==================
*/
static bool S_ParseNoReverb (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NOREVERB;

	return true;
}

/*
 ==================
 S_ParseNoObstruction
 ==================
*/
static bool S_ParseNoObstruction (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NOOBSTRUCTION;

	return true;
}

/*
 ==================
 S_ParseNoExclusion
 ==================
*/
static bool S_ParseNoExclusion (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NOEXCLUSION;

	return true;
}

/*
 ==================
 S_ParseNoOcclusion
 ==================
*/
static bool S_ParseNoOcclusion (script_t *script, soundShader_t *soundShader){

	soundShader->flags |= SSF_NOOCCLUSION;

	return true;
}

/*
 ==================
 S_ParsePriority
 ==================
*/
static bool S_ParsePriority (script_t *script, soundShader_t *soundShader){

	token_t	token;

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'priority' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (!Str_ICompare(token.string, "lowest"))
		soundShader->priority = PRIORITY_LOWEST;
	else if (!Str_ICompare(token.string, "low"))
		soundShader->priority = PRIORITY_LOW;
	else if (!Str_ICompare(token.string, "normal"))
		soundShader->priority = PRIORITY_NORMAL;
	else if (!Str_ICompare(token.string, "high"))
		soundShader->priority = PRIORITY_HIGH;
	else if (!Str_ICompare(token.string, "highest"))
		soundShader->priority = PRIORITY_HIGHEST;
	else {
		Com_Printf(S_COLOR_YELLOW "WARNING: unknown 'priority' parameter '%s' in sound shader '%s'\n", token.string, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseShakes
 ==================
*/
static bool S_ParseShakes (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->shakes)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'shakes' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->shakes < 0.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'shakes' in sound shader '%s'\n", soundShader->shakes, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseVolume
 ==================
*/
static bool S_ParseVolume (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->volume)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'volume' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->volume < 0.0f || soundShader->volume > 1.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'volume' in sound shader '%s'\n", soundShader->volume, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParsePitch
 ==================
*/
static bool S_ParsePitch (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->pitch)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'pitch' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->pitch < 0.1f || soundShader->pitch > 10.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'pitch' in sound shader '%s'\n", soundShader->pitch, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseDryFilter
 ==================
*/
static bool S_ParseDryFilter (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->dryFilter)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'dryFilter' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->dryFilter < 0.0f || soundShader->dryFilter > 1.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'dryFilter' in sound shader '%s'\n", soundShader->dryFilter, soundShader->name);
		return false;
	}

	if (!PS_ReadFloat(script, &soundShader->dryFilterHF)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'dryFilter' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->dryFilterHF < 0.0f || soundShader->dryFilterHF > 1.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'dryFilter' in sound shader '%s'\n", soundShader->dryFilterHF, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseWetFilter
 ==================
*/
static bool S_ParseWetFilter (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->wetFilter)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'wetFilter' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->wetFilter < 0.0f || soundShader->wetFilter > 1.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'wetFilter' in sound shader '%s'\n", soundShader->wetFilter, soundShader->name);
		return false;
	}

	if (!PS_ReadFloat(script, &soundShader->wetFilterHF)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'wetFilter' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->wetFilterHF < 0.0f || soundShader->wetFilterHF > 1.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'wetFilter' in sound shader '%s'\n", soundShader->wetFilterHF, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseMinDistance
 ==================
*/
static bool S_ParseMinDistance (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->minDistance)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'minDistance' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->minDistance < 0.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'minDistance' in sound shader '%s'\n", soundShader->minDistance, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseMaxDistance
 ==================
*/
static bool S_ParseMaxDistance (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->maxDistance)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'maxDistance' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->maxDistance < 0.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'maxDistance' in sound shader '%s'\n", soundShader->maxDistance, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseConeInnerAngle
 ==================
*/
static bool S_ParseConeInnerAngle (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->coneInnerAngle)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'coneInnerAngle' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->coneInnerAngle < 0.0f || soundShader->coneInnerAngle > 360.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'coneInnerAngle' in sound shader '%s'\n", soundShader->coneInnerAngle, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseConeOuterAngle
 ==================
*/
static bool S_ParseConeOuterAngle (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->coneOuterAngle)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'coneOuterAngle' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->coneOuterAngle < 0.0f || soundShader->coneOuterAngle > 360.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'coneOuterAngle' in sound shader '%s'\n", soundShader->coneOuterAngle, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseConeOuterVolume
 ==================
*/
static bool S_ParseConeOuterVolume (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->coneOuterVolume)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'coneOuterVolume' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->coneOuterVolume < 0.0f || soundShader->coneOuterVolume > 1.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'coneOuterVolume' in sound shader '%s'\n", soundShader->coneOuterVolume, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseRolloffFactor
 ==================
*/
static bool S_ParseRolloffFactor (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->rolloffFactor)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'rolloffFactor' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->rolloffFactor < 0.0f || soundShader->rolloffFactor > 10.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'rolloffFactor' in sound shader '%s'\n", soundShader->rolloffFactor, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseRoomRolloffFactor
 ==================
*/
static bool S_ParseRoomRolloffFactor (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->roomRolloffFactor)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'roomRolloffFactor' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->roomRolloffFactor < 0.0f || soundShader->roomRolloffFactor > 10.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'roomRolloffFactor' in sound shader '%s'\n", soundShader->roomRolloffFactor, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseAirAbsorptionFactor
 ==================
*/
static bool S_ParseAirAbsorptionFactor (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->airAbsorptionFactor)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameters for 'airAbsorptionFactor' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->airAbsorptionFactor < 0.0f || soundShader->airAbsorptionFactor > 10.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'airAbsorptionFactor' in sound shader '%s'\n", soundShader->airAbsorptionFactor, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseDopplerFactor
 ==================
*/
static bool S_ParseDopplerFactor (script_t *script, soundShader_t *soundShader){

	if (!PS_ReadFloat(script, &soundShader->dopplerFactor)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'dopplerFactor' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->dopplerFactor < 0.0f || soundShader->dopplerFactor > 10.0f){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %g for 'dopplerFactor' in sound shader '%s'\n", soundShader->dopplerFactor, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseMinSamples
 ==================
*/
static bool S_ParseMinSamples (script_t *script, soundShader_t *soundShader){

	if (soundShader->numLeadIns || soundShader->numEntries){
		Com_Printf(S_COLOR_YELLOW "WARNING: 'minSamples' used after sound specification in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (!PS_ReadInteger(script, &soundShader->minSamples)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'minSamples' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->minSamples < 1 || soundShader->minSamples > MAX_SOUNDS_PER_SHADER){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %i for 'minSamples' in sound shader '%s'\n", soundShader->minSamples, soundShader->name);
		return false;
	}

	return true;
}

/*
 ==================
 S_ParseLeadIn
 ==================
*/
static bool S_ParseLeadIn (script_t *script, soundShader_t *soundShader){

	token_t	token;

	if (soundShader->numLeadIns == MAX_SOUNDS_PER_SHADER){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_SOUNDS_PER_SHADER hit in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (!PS_ReadToken(script, &token)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'leadIn' in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->minSamples){
		if (soundShader->numLeadIns >= s_maxSoundsPerShader->integerValue && soundShader->numLeadIns >= soundShader->minSamples)
			return true;
	}
	else {
		if (soundShader->numLeadIns == s_maxSoundsPerShader->integerValue)
			return true;
	}

	soundShader->leadIns[soundShader->numLeadIns] = S_FindSound(token.string, 0);
	if (!soundShader->leadIns[soundShader->numLeadIns]){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find sound '%s' for sound shader '%s'\n", token.string, soundShader->name);
		return false;
	}

	soundShader->numLeadIns++;

	return true;
}


// ============================================================================

typedef struct {
	const char *				name;
	bool						(*parse)(script_t *script, soundShader_t *soundShader);
} soundShaderKeyword_t;

static soundShaderKeyword_t		s_soundShaderKeywords[] = {
	{"private",						S_ParsePrivate},
	{"antiPrivate",					S_ParseAntiPrivate},
	{"playOnce",					S_ParsePlayOnce},
	{"global",						S_ParseGlobal},
	{"omnidirectional",				S_ParseOmnidirectional},
	{"volumetric",					S_ParseVolumetric},
	{"looping",						S_ParseLooping},
	{"randomize",					S_ParseRandomize},
	{"noRestart",					S_ParseNoRestart},
	{"noOverride",					S_ParseNoOverride},
	{"noDups",						S_ParseNoDups},
	{"noOffset",					S_ParseNoOffset},
	{"noTimeOffset",				S_ParseNoTimeOffset},
	{"noFlicker",					S_ParseNoFlicker},
	{"noPortalFlow",				S_ParseNoPortalFlow},
	{"noDynamicParms",				S_ParseNoDynamicParms},
	{"noReverb",					S_ParseNoReverb},
	{"noObstruction",				S_ParseNoObstruction},
	{"noExclusion",					S_ParseNoExclusion},
	{"noOcclusion",					S_ParseNoOcclusion},
	{"priority",					S_ParsePriority},
	{"shakes",						S_ParseShakes},
	{"volume",						S_ParseVolume},
	{"pitch",						S_ParsePitch},
	{"dryFilter",					S_ParseDryFilter},
	{"wetFilter",					S_ParseWetFilter},
	{"minDistance",					S_ParseMinDistance},
	{"maxDistance",					S_ParseMaxDistance},
	{"coneInnerAngle",				S_ParseConeInnerAngle},
	{"coneOuterAngle",				S_ParseConeOuterAngle},
	{"coneOuterVolume",				S_ParseConeOuterVolume},
	{"rolloffFactor",				S_ParseRolloffFactor},
	{"roomRolloffFactor",			S_ParseRoomRolloffFactor},
	{"airAbsorptionFactor",			S_ParseAirAbsorptionFactor},
	{"dopplerFactor",				S_ParseDopplerFactor},
	{"minSamples",					S_ParseMinSamples},
	{"leadIn",						S_ParseLeadIn},
	{NULL,							NULL}
};


/*
 ==================
 S_ParseSoundEntry
 ==================
*/
static bool S_ParseSoundEntry (script_t *script, soundShader_t *soundShader, const char *name){

	if (soundShader->numEntries == MAX_SOUNDS_PER_SHADER){
		Com_Printf(S_COLOR_YELLOW "WARNING: MAX_SOUNDS_PER_SHADER hit in sound shader '%s'\n", soundShader->name);
		return false;
	}

	if (soundShader->minSamples){
		if (soundShader->numEntries >= s_maxSoundsPerShader->integerValue && soundShader->numEntries >= soundShader->minSamples)
			return true;
	}
	else {
		if (soundShader->numEntries == s_maxSoundsPerShader->integerValue)
			return true;
	}

	soundShader->entries[soundShader->numEntries] = S_FindSound(name, 0);
	if (!soundShader->entries[soundShader->numEntries]){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find sound '%s' for sound shader '%s'\n", name, soundShader->name);
		return false;
	}

	soundShader->numEntries++;

	return true;
}

/*
 ==================
 S_ParseKeyword
 ==================
*/
static bool S_ParseKeyword (script_t *script, const char *keyword, soundShader_t *soundShader){

	soundShaderKeyword_t	*k;

	for (k = s_soundShaderKeywords; k->name; k++){
		if (!Str_ICompare(k->name, keyword))
			return k->parse(script, soundShader);
	}

	Com_Printf(S_COLOR_YELLOW "WARNING: unknown keyword '%s' in sound shader '%s'\n", keyword, soundShader->name);

	return false;
}

/*
 ==================
 S_ParseSoundShader
 ==================
*/
static bool S_ParseSoundShader (script_t *script, soundShader_t *soundShader){

	token_t	token;

	// Parse the sound shader
	if (!PS_ExpectTokenString(script, &token, "{", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in sound shader '%s'\n", token.string, soundShader->name);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in sound shader '%s'\n", soundShader->name);
			return false;	// End of script
		}

		if (!Str_Compare(token.string, "}"))
			break;			// End of sound shader

		// Parse a sound entry
		if (token.string[0] == '_' || !Str_ICompareChars(token.string, "sounds/", 7) || !Str_ICompareChars(token.string, "sounds\\", 7)){
			if (!S_ParseSoundEntry(script, soundShader, token.string))
				return false;

			continue;
		}

		// Parse a keyword
		if (!S_ParseKeyword(script, token.string, soundShader))
			return false;
	}

	return true;
}

/*
 ==================
 S_ParseSoundShaderFile
 ==================
*/
static void S_ParseSoundShaderFile (script_t *script){

	token_t				token;
	soundShaderDef_t	*soundShaderDef;
	const char			*text;
	int					length;
	uint				hashKey;

	while (1){
		// Parse the name
		if (!PS_ReadToken(script, &token))
			break;		// End of script

		// Parse the script
		text = script->text;

		PS_SkipBracedSection(script, 0);

		length = script->text - text;

		// See if there's a definition for this sound shader already
		hashKey = Str_HashKey(token.string, SOUND_SHADERS_HASH_SIZE, false);

		for (soundShaderDef = s_soundShaderDefsHashTable[hashKey]; soundShaderDef; soundShaderDef = soundShaderDef->nextHash){
			if (!Str_ICompare(soundShaderDef->name, token.string))
				break;
		}

		// If it already exists, replace it
		if (soundShaderDef){
			Mem_Free(soundShaderDef->text);

			soundShaderDef->text = (char *)Mem_Alloc(length + 1, TAG_SOUND);
			soundShaderDef->length = length;
			Str_Copy(soundShaderDef->source, script->name, sizeof(soundShaderDef->source));
			soundShaderDef->line = token.line;

			Str_Copy(soundShaderDef->text, text, length + 1);

			continue;
		}

		// Store the sound shader definition
		soundShaderDef = (soundShaderDef_t *)Mem_Alloc(sizeof(soundShaderDef_t), TAG_SOUND);

		Str_Copy(soundShaderDef->name, token.string, sizeof(soundShaderDef->name));
		soundShaderDef->text = (char *)Mem_Alloc(length + 1, TAG_SOUND);
		soundShaderDef->length = length;
		Str_Copy(soundShaderDef->source, script->name, sizeof(soundShaderDef->source));
		soundShaderDef->line = token.line;

		Str_Copy(soundShaderDef->text, text, length + 1);

		// Add to hash table
		soundShaderDef->nextHash = s_soundShaderDefsHashTable[hashKey];
		s_soundShaderDefsHashTable[hashKey] = soundShaderDef;
	}
}


/*
 ==============================================================================

 SOUND SHADER REGISTRATION

 ==============================================================================
*/


/*
 ==================
 S_NewSoundShader
 ==================
*/
static soundShader_t *S_NewSoundShader (void){

	soundShader_t	*soundShader;

	// Clear the sound shader
	soundShader = &s_parseSoundShader;
	Mem_Fill(soundShader, 0, sizeof(soundShader_t));

	return soundShader;
}

/*
 ==================
 S_CreateDefaultSoundShader
 ==================
*/
static soundShader_t *S_CreateDefaultSoundShader (const char *name){

	soundShader_t	*soundShader;

	// Create a new sound shader
	soundShader = S_NewSoundShader();

	// Fill it in
	Str_Copy(soundShader->name, name, sizeof(soundShader->name));
	soundShader->flags = SSF_DEFAULTED;
	soundShader->volume = 1.0f;
	soundShader->pitch = 1.0f;
	soundShader->dryFilter = 1.0f;
	soundShader->dryFilterHF = 1.0f;
	soundShader->wetFilter = 1.0f;
	soundShader->wetFilterHF = 1.0f;
	soundShader->minDistance = 40.0f;
	soundShader->maxDistance = 2000.0f;
	soundShader->coneInnerAngle = 360.0f;
	soundShader->coneOuterAngle = 360.0f;
	soundShader->coneOuterVolume = 1.0f;
	soundShader->rolloffFactor = 1.0f;
	soundShader->roomRolloffFactor = 0.0f;
	soundShader->airAbsorptionFactor = 1.0f;
	soundShader->dopplerFactor = 0.0f;
	soundShader->entries[soundShader->numEntries++] = snd.defaultSound;

	return soundShader;
}

/*
 ==================
 S_CreateSoundShader
 ==================
*/
static soundShader_t *S_CreateSoundShader (const char *name, soundShaderDef_t *soundShaderDef){

	soundShader_t	*soundShader;
	script_t		*script;

	// Create a new sound shader
	soundShader = S_NewSoundShader();

	// Fill it in
	Str_Copy(soundShader->name, name, sizeof(soundShader->name));
	soundShader->flags = 0;
	soundShader->volume = 1.0f;
	soundShader->pitch = 1.0f;
	soundShader->dryFilter = 1.0f;
	soundShader->dryFilterHF = 1.0f;
	soundShader->wetFilter = 1.0f;
	soundShader->wetFilterHF = 1.0f;
	soundShader->minDistance = 40.0f;
	soundShader->maxDistance = 2000.0f;
	soundShader->coneInnerAngle = 360.0f;
	soundShader->coneOuterAngle = 360.0f;
	soundShader->coneOuterVolume = 1.0f;
	soundShader->rolloffFactor = 1.0f;
	soundShader->roomRolloffFactor = 0.0f;
	soundShader->airAbsorptionFactor = 1.0f;
	soundShader->dopplerFactor = 0.0f;

	// If we have a definition, create an explicit sound shader
	if (soundShaderDef){
		soundShader->flags |= SSF_EXPLICIT;

		// Load the script text
		script = PS_LoadScriptMemory(soundShaderDef->source, soundShaderDef->text, soundShaderDef->length, soundShaderDef->line);
		if (!script)
			return S_CreateDefaultSoundShader(name);

		PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

		// Parse it
		if (!S_ParseSoundShader(script, soundShader)){
			PS_FreeScript(script);

			return S_CreateDefaultSoundShader(name);
		}

		// Free the script text
		PS_FreeScript(script);

		return soundShader;
	}

	// Otherwise create an implicit sound shader
	soundShader->entries[soundShader->numEntries] = S_FindSound(Str_VarArgs("%s.wav", soundShader->name), 0);
	if (!soundShader->entries[soundShader->numEntries]){
		Com_Printf(S_COLOR_YELLOW "WARNING: couldn't find sound for sound shader '%s', using default\n", soundShader->name);

		soundShader->entries[soundShader->numEntries] = snd.defaultSound;
	}

	soundShader->numEntries++;

	return soundShader;
}

/*
 ==================
 S_FinishSoundShader
 ==================
*/
static void S_FinishSoundShader (soundShader_t *soundShader){

	// 'global' overrides 'omnidirectional' and 'volumetric', implies
	// 'noPortalFlow', 'noObstruction', 'noExclusion', and 'noOcclusion', and
	// disables distance attenuation, directional cones, and doppler shifts
	if (soundShader->flags & SSF_GLOBAL){
		soundShader->flags &= ~(SSF_OMNIDIRECTIONAL | SSF_VOLUMETRIC);
		soundShader->flags |= (SSF_NOPORTALFLOW | SSF_NOOBSTRUCTION | SSF_NOEXCLUSION | SSF_NOOCCLUSION);

		soundShader->minDistance = 0.0f;
		soundShader->maxDistance = 0.0f;

		soundShader->coneInnerAngle = 360.0f;
		soundShader->coneOuterAngle = 360.0f;
		soundShader->coneOuterVolume = 1.0f;

		soundShader->rolloffFactor = 0.0f;
		soundShader->roomRolloffFactor = 0.0f;
		soundShader->airAbsorptionFactor = 0.0f;

		soundShader->dopplerFactor = 0.0f;
	}

	// 'omnidirectional' overrides 'volumetric' and disables directional cones
	if (soundShader->flags & SSF_OMNIDIRECTIONAL){
		soundShader->flags &= ~SSF_VOLUMETRIC;

		soundShader->coneInnerAngle = 360.0f;
		soundShader->coneOuterAngle = 360.0f;
		soundShader->coneOuterVolume = 1.0f;
	}

	// 'volumetric' disables directional cones
	if (soundShader->flags & SSF_VOLUMETRIC){
		soundShader->coneInnerAngle = 360.0f;
		soundShader->coneOuterAngle = 360.0f;
		soundShader->coneOuterVolume = 1.0f;
	}

	// Set 'priority' if unset
	if (soundShader->priority == PRIORITY_BAD)
		soundShader->priority = PRIORITY_NORMAL;

	// If 'minDistance' is equal to or greater than 'maxDistance' or
	// 'rolloffFactor' is set to 0.0, disable distance attenuation
	if (soundShader->minDistance >= soundShader->maxDistance || soundShader->rolloffFactor == 0.0f){
		soundShader->minDistance = 0.0f;
		soundShader->maxDistance = 0.0f;

		soundShader->rolloffFactor = 0.0f;
		soundShader->roomRolloffFactor = 0.0f;
		soundShader->airAbsorptionFactor = 0.0f;
	}

	// If 'coneInnerAngle' is equal to or greater than 'coneOuterAngle' or
	// 'coneOuterVolume' is set to 1.0, disable directional cones
	if (soundShader->coneInnerAngle >= soundShader->coneOuterAngle || soundShader->coneOuterVolume == 1.0f){
		soundShader->coneInnerAngle = 360.0f;
		soundShader->coneOuterAngle = 360.0f;
		soundShader->coneOuterVolume = 1.0f;
	}

	// Make sure it has a sound
	if (!soundShader->numEntries){
		Com_Printf(S_COLOR_YELLOW "WARNING: sound shader '%s' has no sounds!\n", soundShader->name);

		soundShader->entries[soundShader->numEntries++] = snd.defaultSound;
	}
}

/*
 ==================
 S_LoadSoundShader
 ==================
*/
static soundShader_t *S_LoadSoundShader (soundShader_t *newSoundShader){

	soundShader_t	*soundShader;
	uint			hashKey;

	if (s_numSoundShaders == MAX_SOUND_SHADERS)
		Com_Error(ERR_DROP, "S_LoadSoundShader: MAX_SOUND_SHADERS hit");

	s_soundShaders[s_numSoundShaders++] = soundShader = (soundShader_t *)Mem_Alloc(sizeof(soundShader_t), TAG_SOUND);

	// Copy the sound shader
	Mem_Copy(soundShader, newSoundShader, sizeof(soundShader_t));

	// Make sure all the parameters are valid
	S_FinishSoundShader(soundShader);

	// Add to hash table
	hashKey = Str_HashKey(soundShader->name, SOUND_SHADERS_HASH_SIZE, false);

	soundShader->nextHash = s_soundShadersHashTable[hashKey];
	s_soundShadersHashTable[hashKey] = soundShader;

	return soundShader;
}

/*
 ==================
 S_FindSoundShader
 ==================
*/
soundShader_t *S_FindSoundShader (const char *name){

	soundShader_t		*soundShader;
	soundShaderDef_t	*soundShaderDef;
	uint				hashKey;

	// Performance evaluation option
	if (s_singleSoundShader->integerValue)
		return snd.defaultSoundShader;

	// See if already loaded
	hashKey = Str_HashKey(name, SOUND_SHADERS_HASH_SIZE, false);

	for (soundShader = s_soundShadersHashTable[hashKey]; soundShader; soundShader = soundShader->nextHash){
		if (!Str_ICompare(soundShader->name, name))
			return soundShader;
	}

	// See if there's a definition for this sound shader
	for (soundShaderDef = s_soundShaderDefsHashTable[hashKey]; soundShaderDef; soundShaderDef = soundShaderDef->nextHash){
		if (!Str_ICompare(soundShaderDef->name, name))
			break;
	}

	// Create the sound shader
	soundShader = S_CreateSoundShader(name, soundShaderDef);

	// Load it in
	return S_LoadSoundShader(soundShader);
}

/*
 ==================
 S_RegisterSoundShader
 ==================
*/
soundShader_t *S_RegisterSoundShader (const char *name){

	return S_FindSoundShader(name);
}


// ============================================================================


/*
 ==================
 S_EnumSoundShaderDefs
 ==================
*/
void S_EnumSoundShaderDefs (void (*callback)(const char *name)){

	soundShaderDef_t	*soundShaderDef;
	int					i;

	for (i = 0; i < SOUND_SHADERS_HASH_SIZE; i++){
		soundShaderDef = s_soundShaderDefsHashTable[i];

		while (soundShaderDef){
			callback(soundShaderDef->name);

			soundShaderDef = soundShaderDef->nextHash;
		}
	}
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 S_ListSoundShaders_f
 ==================
*/
static void S_ListSoundShaders_f (){

	soundShader_t	*soundShader;
	int				i;

	Com_Printf("\n");
	Com_Printf("      ldn ent pri src -name-----------\n");

	for (i = 0; i < s_numSoundShaders; i++){
		soundShader = s_soundShaders[i];

		Com_Printf("%4i: ", i);

		Com_Printf("%3i %3i ", soundShader->numLeadIns, soundShader->numEntries);

		Com_Printf("%3i ", soundShader->priority);

		if (soundShader->flags & SSF_EXPLICIT)
			Com_Printf(" E  ");
		else
			Com_Printf(" I  ");

		Com_Printf("%s%s\n", soundShader->name, (soundShader->flags & SSF_DEFAULTED) ? " (DEFAULTED)" : "");
	}

	Com_Printf("--------------------------------------\n");
	Com_Printf("%i total sound shaders\n", s_numSoundShaders);
	Com_Printf("\n");
}

/*
 ==================
 S_ListSoundShaderDefs_f
 ==================
*/
static void S_ListSoundShaderDefs_f (){

	soundShaderDef_t	*soundShaderDef;
	int					total = 0, bytes = 0;
	int					i;

	Com_Printf("\n");
	Com_Printf("       -name-----------\n");

	for (i = 0; i < SOUND_SHADERS_HASH_SIZE; i++){
		soundShaderDef = s_soundShaderDefsHashTable[i];

		while (soundShaderDef){
			total++;
			bytes += soundShaderDef->length;

			Com_Printf("%5i: %s\n", total - 1, soundShaderDef->name);

			soundShaderDef = soundShaderDef->nextHash;
		}
	}

	Com_Printf("-----------------------\n");
	Com_Printf("%i total sound shader definitions\n", total);
	Com_Printf("%.2f MB of sound shader definition text\n", bytes * (1.0f / 1048576.0f));
	Com_Printf("\n");
}

/*
 ==================
 S_PrintSoundShaderDef_f
 ==================
*/
static void S_PrintSoundShaderDef_f (){

	soundShaderDef_t	*soundShaderDef;
	const char			*name;
	uint				hashKey;

	if (Cmd_Argc() != 2){
		Com_Printf("Usage: printSoundShaderDef <name>\n");
		return;
	}

	name = Cmd_Argv(1);

	// Find the sound shader definition
	hashKey = Str_HashKey(name, SOUND_SHADERS_HASH_SIZE, false);

	for (soundShaderDef = s_soundShaderDefsHashTable[hashKey]; soundShaderDef; soundShaderDef = soundShaderDef->nextHash){
		if (!Str_ICompare(soundShaderDef->name, name))
			break;
	}

	if (!soundShaderDef){
		Com_Printf("Sound shader definition '%s' not found\n", name);
		return;
	}

	// Print it
	Com_Printf("\n");
	Com_Printf("Source: %s (line: %i)\n", soundShaderDef->source, soundShaderDef->line);
	Com_Printf("\n");
	Com_Printf("%s%s\n", soundShaderDef->name, soundShaderDef->text);
	Com_Printf("\n");
}


/*
 ==============================================================================

 DEFAULT SOUND SHADERS

 ==============================================================================
*/


/*
 ==================
 S_CreateDefaultSoundShaders
 ==================
*/
static void S_CreateDefaultSoundShaders (){

	soundShader_t	*soundShader;

	// Default sound shader
	soundShader = S_NewSoundShader();

	Str_Copy(soundShader->name, "_default", sizeof(soundShader->name));
	soundShader->flags = SSF_NOPORTALFLOW | SSF_NODYNAMICPARMS | SSF_NOREVERB | SSF_NOOBSTRUCTION | SSF_NOEXCLUSION | SSF_NOOCCLUSION;
	soundShader->volume = 1.0f;
	soundShader->pitch = 1.0f;
	soundShader->dryFilter = 1.0f;
	soundShader->dryFilterHF = 1.0f;
	soundShader->wetFilter = 1.0f;
	soundShader->wetFilterHF = 1.0f;
	soundShader->minDistance = 40.0f;
	soundShader->maxDistance = 2000.0f;
	soundShader->coneInnerAngle = 360.0f;
	soundShader->coneOuterAngle = 360.0f;
	soundShader->coneOuterVolume = 1.0f;
	soundShader->rolloffFactor = 1.0f;
	soundShader->roomRolloffFactor = 0.0f;
	soundShader->airAbsorptionFactor = 0.0f;
	soundShader->dopplerFactor = 0.0f;
	soundShader->entries[soundShader->numEntries++] = snd.defaultSound;

	snd.defaultSoundShader = S_LoadSoundShader(soundShader);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 S_InitSoundShaders
 ==================
*/
void S_InitSoundShaders (){

	script_t	*script;
	char		name[MAX_PATH_LENGTH];
	const char	**fileList;
	int			numFiles;
	int			i;

	Com_Printf("Initializing Sound Shaders\n");

	// Add commands
	Cmd_AddCommand("listSoundShaders", S_ListSoundShaders_f, "Lists loaded sound shaders", NULL);
	Cmd_AddCommand("listSoundShaderDefs", S_ListSoundShaderDefs_f, "Lists sound shader definitions", NULL);
	Cmd_AddCommand("printSoundShaderDef", S_PrintSoundShaderDef_f, "Prints a sound shader definition", Cmd_ArgCompletion_SoundShaderName);

	// Load and parse .sndshd files
	fileList = FS_ListFiles("sounds", ".sndshd", true, &numFiles);

	for (i = 0; i < numFiles; i++){
		// Load the script file
		Str_SPrintf(name, sizeof(name), "sounds/%s", fileList[i]);
		Com_Printf("...loading '%s'\n", name);

		script = PS_LoadScriptFile(name);
		if (!script){
			Com_Printf(S_COLOR_YELLOW "WARNING: couldn't load '%s'\n", name);
			continue;
		}

		PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

		// Parse it
		S_ParseSoundShaderFile(script);

		// Free the script file
		PS_FreeScript(script);
	}

	FS_FreeFileList(fileList);

	// Create the default sound shaders
	S_CreateDefaultSoundShaders();
}

/*
 ==================
 S_ShutdownSoundShaders
 ==================
*/
void S_ShutdownSoundShaders (){

	// Remove commands
	Cmd_RemoveCommand("listSoundShaders");
	Cmd_RemoveCommand("listSoundShaderDefs");
	Cmd_RemoveCommand("printSoundShaderDef");

	// Clear sound shader definition and sound shader lists
	Mem_Fill(s_soundShaderDefsHashTable, 0, sizeof(s_soundShaderDefsHashTable));

	Mem_Fill(s_soundShadersHashTable, 0, sizeof(s_soundShadersHashTable));
	Mem_Fill(s_soundShaders, 0, sizeof(s_soundShaders));

	s_numSoundShaders = 0;
}