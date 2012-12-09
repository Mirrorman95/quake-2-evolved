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
// r_shader.c - Shader script compiling
//


#include "r_local.h"


#define SHADERS_HASH_SIZE			(MAX_SHADERS >> 2)

static char					r_shaderDefines[MAX_STRING_LENGTH];

static shader_t *			r_shadersHashTable[SHADERS_HASH_SIZE];
static shader_t *			r_shaders[MAX_SHADERS];
static int					r_numShaders;


/*
 ==============================================================================

 SHADER LOADING

 ==============================================================================
*/


/*
 ==================
 R_PrintShaderInfoLog
 ==================
*/
static void R_PrintShaderInfoLog (shader_t *shader){

	char	infoLog[MAX_PRINT_MESSAGE];
	int		infoLogLen;

	qglGetShaderiv(shader->shaderId, GL_INFO_LOG_LENGTH, &infoLogLen);
	if (infoLogLen <= 1)
		return;

	qglGetShaderInfoLog(shader->shaderId, sizeof(infoLog), NULL, infoLog);

	Com_DPrintf("---------- Shader Info Log ----------\n");
	Com_DPrintf("%s", infoLog);

	if (infoLogLen >= sizeof(infoLog))
		Com_DPrintf("...\n");

	Com_DPrintf("-------------------------------------\n");
}

/*
 ==================
 R_CompileShader
 ==================
*/
static void R_CompileShader (shader_t *shader, const char *defines, const char *code){

	const char	*source[2];

	switch (shader->type){
	case GL_VERTEX_SHADER:
		Com_DPrintf("Compiling GLSL vertex shader '%s'...\n", shader->name);
		break;
	case GL_FRAGMENT_SHADER:
		Com_DPrintf("Compiling GLSL fragment shader '%s'...\n", shader->name);
		break;
	}

	// Create the shader
	shader->shaderId = qglCreateShader(shader->type);

	// Upload the shader source
	source[0] = defines;
	source[1] = code;

	qglShaderSource(shader->shaderId, 2, source, NULL);

	// Compile the shader
	qglCompileShader(shader->shaderId);

	// Check if the compile was successful
	qglGetShaderiv(shader->shaderId, GL_COMPILE_STATUS, &shader->compileStatus);

	if (!shader->compileStatus){
		switch (shader->type){
		case GL_VERTEX_SHADER:
			Com_Printf(S_COLOR_RED "Failed to compile vertex shader '%s'\n", shader->name);
			break;
		case GL_FRAGMENT_SHADER:
			Com_Printf(S_COLOR_RED "Failed to compile fragment shader '%s'\n", shader->name);
			break;
		}

		// If an info log is available, print it to the console
		R_PrintShaderInfoLog(shader);

		return;
	}

	// If an info log is available, print it to the console
	R_PrintShaderInfoLog(shader);
}

/*
 ==================
 R_LoadShader
 ==================
*/
static shader_t *R_LoadShader (const char *name, uint type, const char *code){

	shader_t	*shader;
	uint		hashKey;

	if (r_numShaders == MAX_SHADERS)
		Com_Error(ERR_DROP, "R_LoadShader: MAX_SHADER hit");

	r_shaders[r_numShaders++] = shader = (shader_t *)Mem_Alloc(sizeof(shader_t), TAG_RENDERER);

	// Fill it in
	Str_Copy(shader->name, name, sizeof(shader->name));
	shader->type = type;
	shader->references = 0;

	R_CompileShader(shader, r_shaderDefines, code);

	// Add to hash table
	hashKey = Str_HashKey(shader->name, SHADERS_HASH_SIZE, false);

	shader->nextHash = r_shadersHashTable[hashKey];
	r_shadersHashTable[hashKey] = shader;

	return shader;
}

/*
 ==================
 R_FindShader
 ==================
*/
shader_t *R_FindShader (const char *name, uint type){

	shader_t	*shader;
	char		realName[MAX_PATH_LENGTH];
	char		*code;
	uint		hashKey;

	// Check if already loaded
	hashKey = Str_HashKey(name, SHADERS_HASH_SIZE, false);

	for (shader = r_shadersHashTable[hashKey]; shader; shader = shader->nextHash){
		if (shader->type != type)
			continue;

		if (!Str_ICompare(shader->name, name)){
			if (!shader->compileStatus)
				return NULL;

			return shader;
		}
	}

	// Load it from disk
	switch (type){
	case GL_VERTEX_SHADER:
		Str_SPrintf(realName, sizeof(realName), "shaders/%s.vs", name);
		break;
	case GL_FRAGMENT_SHADER:
		Str_SPrintf(realName, sizeof(realName), "shaders/%s.fs", name);
		break;
	default:
		Com_Error(ERR_DROP, "R_FindShader: bad shader type (%u)", type);
	}

	FS_ReadFile(realName, (void **)&code);
	if (!code)
		return NULL;

	// Load the shader
	shader = R_LoadShader(name, type, code);

	FS_FreeFile(code);

	if (!shader->compileStatus)
		return NULL;

	return shader;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_ListShaders_f
 ==================
*/
static void R_ListShaders_f (){

	shader_t	*shader;
	int			i;

	Com_Printf("\n");
	Com_Printf("      type refs -name-----------\n");

	for (i = 0; i < r_numShaders; i++){
		shader = r_shaders[i];

		Com_Printf("%4i: ", i);

		switch (shader->type){
		case GL_VERTEX_SHADER:
			Com_Printf(" VS  ");
			break;
		case GL_FRAGMENT_SHADER:
			Com_Printf(" FS  ");
			break;
		default:
			Com_Printf("???? ");
			break;
		}

		Com_Printf("%4i ", shader->references);

		Com_Printf("%s%s\n", shader->name, (!shader->compileStatus) ? " (INVALID)" : "");
	}

	Com_Printf("---------------------------\n");
	Com_Printf("%i total shaders\n", r_numShaders);
	Com_Printf("\n");
}


/*
 ==============================================================================

 INTERNAL TEXTURES

 ==============================================================================
*/


/*
 ==================
 R_InitShaders
 ==================
*/
void R_InitShaders (){

	// Add commands
	Cmd_AddCommand("listShaders", R_ListShaders_f, "Lists loaded shaders", NULL);

	// Build defines string
	switch (glConfig.hardwareType){
	case GLHW_GENERIC:
		Str_Copy(r_shaderDefines, "#version 330 compatibility" NEWLINE "#line 0", sizeof(r_shaderDefines));
		break;
	case GLHW_ATI:
		Str_Copy(r_shaderDefines, "#version 330 compatibility" NEWLINE "#define GLSL_ATI" NEWLINE "#line 0", sizeof(r_shaderDefines));
		break;
	case GLHW_NVIDIA:
		Str_Copy(r_shaderDefines, "#version 330 compatibility" NEWLINE "#define GLSL_NVIDIA" NEWLINE "#line 0", sizeof(r_shaderDefines));
		break;
	}
}

/*
 ==================
 R_ShutdownShaders
 ==================
*/
void R_ShutdownShaders (){

	shader_t	*shader;
	int			i;

	// Remove commands
	Cmd_RemoveCommand("listShaders");

	// Delete all the shaders
	for (i = 0; i < r_numShaders; i++){
		shader = r_shaders[i];

		qglDeleteShader(shader->shaderId);
	}

	// Clear shader list
	Mem_Fill(r_shadersHashTable, 0, sizeof(r_shadersHashTable));
	Mem_Fill(r_shaders, 0, sizeof(r_shaders));

	r_numShaders = 0;
}