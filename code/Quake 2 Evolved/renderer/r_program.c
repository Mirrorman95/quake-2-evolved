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
// r_program.c - Program parsing and uploading
//


#include "r_local.h"


#define PROGRAMS_HASH_SIZE			(MAX_PROGRAMS >> 2)

typedef struct {
	const char *			name;

	uniformType_t			type;
	int						size;
	uint					format;
} uniformTable_t;

static uniformTable_t	r_uniformTable[] = {
	{"u_ViewOrigin",		UT_VIEW_ORIGIN,				1,	GL_FLOAT_VEC3},
	{"u_ViewAxis",			UT_VIEW_AXIS,				1,	GL_FLOAT_MAT3},
	{"u_EntityOrigin",		UT_ENTITY_ORIGIN,			1,	GL_FLOAT_VEC3},
	{"u_EntityAxis",		UT_ENTITY_AXIS,				1,	GL_FLOAT_MAT3},
	{"u_SunOrigin",			UT_SUN_ORIGIN,				1,	GL_FLOAT_VEC3},
	{"u_SunDirection",		UT_SUN_DIRECTION,			1,	GL_FLOAT_VEC3},
	{"u_SunColor",			UT_SUN_COLOR,				1,	GL_FLOAT_VEC3},
	{"u_ScreenMatrix",		UT_SCREEN_MATRIX,			1,	GL_FLOAT_MAT4},
	{"u_CoordScaleAndBias",	UT_COORD_SCALE_AND_BIAS,	1,	GL_FLOAT_VEC4},
	{"u_ColorScaleAndBias",	UT_COLOR_SCALE_AND_BIAS,	1,	GL_FLOAT_VEC2},
	{NULL,					UT_CUSTOM,					0,	0}
};

static program_t *			r_programsHashTable[PROGRAMS_HASH_SIZE];
static program_t *			r_programs[MAX_PROGRAMS];
static int					r_numPrograms;


/*
 ==============================================================================

 PROGRAM LOADING

 ==============================================================================
*/


/*
 ==================
 R_PrintProgramInfoLog
 ==================
*/
static void R_PrintProgramInfoLog (program_t *program){

	char	infoLog[MAX_PRINT_MESSAGE];
	int		infoLogLen;

	qglGetProgramiv(program->programId, GL_INFO_LOG_LENGTH, &infoLogLen);
	if (infoLogLen <= 1)
		return;

	qglGetProgramInfoLog(program->programId, sizeof(infoLog), NULL, infoLog);

	Com_DPrintf("---------- Program Info Log ----------\n");
	Com_DPrintf("%s", infoLog);

	if (infoLogLen >= sizeof(infoLog))
		Com_DPrintf("...\n");

	Com_DPrintf("--------------------------------------\n");
}

/*
 ==================
 R_ParseProgramVertexAttribs
 ==================
*/
static void R_ParseProgramVertexAttribs (program_t *program){

	if (qglGetAttribLocation(program->programId, "va_Normal") == GL_ATTRIB_NORMAL)
		program->vertexAttribs |= VA_NORMAL;

	if (qglGetAttribLocation(program->programId, "va_Tangent1") == GL_ATTRIB_TANGENT1)
		program->vertexAttribs |= VA_TANGENT1;

	if (qglGetAttribLocation(program->programId, "va_Tangent2") == GL_ATTRIB_TANGENT2)
		program->vertexAttribs |= VA_TANGENT2;

	if (qglGetAttribLocation(program->programId, "va_TexCoord") == GL_ATTRIB_TEXCOORD)
		program->vertexAttribs |= VA_TEXCOORD;

	if (qglGetAttribLocation(program->programId, "va_Color") == GL_ATTRIB_COLOR)
		program->vertexAttribs |= VA_COLOR;
}

/*
 ==================
 R_ParseProgramUniforms
 ==================
*/
static void R_ParseProgramUniforms (program_t *program){

	uniformTable_t	*uniformTable;
	uniform_t		*uniform, uniforms[MAX_PROGRAM_UNIFORMS];
	int				numUniforms;
	char			name[MAX_UNIFORM_NAME_LENGTH];
	uint			format;
	int				length, size;
	int				location;
	int				i;

	// Get active uniforms
	qglGetProgramiv(program->programId, GL_ACTIVE_UNIFORMS, &numUniforms);
	if (!numUniforms)
		return;

	for (i = 0; i < numUniforms; i++){
		qglGetActiveUniform(program->programId, i, sizeof(name), &length, &size, &format, name);

		location = qglGetUniformLocation(program->programId, name);

		if (location == -1)
			continue;		// Ignore built-in uniforms

		// Fix up uniform array names
		if (name[length-3] == '[' && name[length-2] == '0' && name[length-1] == ']')
			name[length-3] = 0;

		// Check for a predefined uniform name
		for (uniformTable = r_uniformTable; uniformTable->name; uniformTable++){
			if (!Str_Compare(uniformTable->name, name))
				break;
		}

		if (uniformTable->type != UT_CUSTOM){
			if (uniformTable->size != size)
				Com_Error(ERR_DROP, "R_ParseProgramUniforms: uniform '%s' in program '%s' has wrong size", name, program->name);

			if (uniformTable->format != format)
				Com_Error(ERR_DROP, "R_ParseProgramUniforms: uniform '%s' in program '%s' has wrong format", name, program->name);
		}

		// Add a new uniform
		if (program->numUniforms == MAX_PROGRAM_UNIFORMS)
			Com_Error(ERR_DROP, "R_ParseProgramUniforms: MAX_PROGRAM_UNIFORMS hit");

		uniform = &uniforms[program->numUniforms++];

		Str_Copy(uniform->name, name, sizeof(uniform->name));
		uniform->type = uniformTable->type;
		uniform->size = size;
		uniform->format = format;
		uniform->location = location;
		uniform->unit = -1;
		uniform->values[0] = 0.0f;
		uniform->values[1] = 0.0f;
		uniform->values[2] = 0.0f;
		uniform->values[3] = 0.0f;
	}

	// Copy the uniforms
	program->uniforms = (uniform_t *)Mem_DupData(uniforms, program->numUniforms * sizeof(uniform_t), TAG_RENDERER);
}

/*
 ==================
 R_LinkProgram
 ==================
*/
static void R_LinkProgram (program_t *program){

	Com_DPrintf("Linking GLSL program '%s'...\n", program->name);

	// Create the program
	program->programId = qglCreateProgram();

	// Attach the shaders
	program->vertexShader->references++;
	program->fragmentShader->references++;

	qglAttachShader(program->programId, program->vertexShader->shaderId);
	qglAttachShader(program->programId, program->fragmentShader->shaderId);

	// Bind vertex attribs
	qglBindAttribLocation(program->programId, GL_ATTRIB_NORMAL, "va_Normal");
	qglBindAttribLocation(program->programId, GL_ATTRIB_TANGENT1, "va_Tangent1");
	qglBindAttribLocation(program->programId, GL_ATTRIB_TANGENT2, "va_Tangent2");
	qglBindAttribLocation(program->programId, GL_ATTRIB_TEXCOORD, "va_TexCoord");
	qglBindAttribLocation(program->programId, GL_ATTRIB_COLOR, "va_Color");

	// Link the program
	qglLinkProgram(program->programId);

	// Check if the link was successful
	qglGetProgramiv(program->programId, GL_LINK_STATUS, &program->linkStatus);

	if (!program->linkStatus){
		Com_Printf(S_COLOR_RED "Failed to link program '%s'\n", program->name);

		// If an info log is available, print it to the console
		R_PrintProgramInfoLog(program);

		return;
	}

	// If an info log is available, print it to the console
	R_PrintProgramInfoLog(program);

	// Parse the active vertex attribs
	R_ParseProgramVertexAttribs(program);

	// Parse the active uniforms
	R_ParseProgramUniforms(program);
}

/*
 ==================
 R_LoadProgram
 ==================
*/
static program_t *R_LoadProgram (const char *name, shader_t *vertexShader, shader_t *fragmentShader){

	program_t	*program;
	uint		hashKey;

	if (r_numPrograms == MAX_PROGRAMS)
		Com_Error(ERR_DROP, "R_LoadProgram: MAX_PROGRAMS hit");

	r_programs[r_numPrograms++] = program = (program_t *)Mem_Alloc(sizeof(program_t), TAG_RENDERER);

	// Fill it in
	Str_Copy(program->name, name, sizeof(program->name));
	program->vertexShader = vertexShader;
	program->fragmentShader = fragmentShader;
	program->vertexAttribs = 0;
	program->numUniforms = 0;
	program->uniforms = NULL;

	R_LinkProgram(program);

	// Add to hash table
	hashKey = Str_HashKey(program->name, PROGRAMS_HASH_SIZE, false);

	program->nextHash = r_programsHashTable[hashKey];
	r_programsHashTable[hashKey] = program;

	return program;
}

/*
 ==================
 R_FindProgram
 ==================
*/
program_t *R_FindProgram (const char *name, shader_t *vertexShader, shader_t *fragmentShader){

	program_t	*program;
	uint		hashKey;

	if (!vertexShader || !fragmentShader)
		Com_Error(ERR_DROP, "R_FindProgram: NULL shader");

	if (!name || !name[0])
		name = Str_VarArgs("%s & %s", vertexShader->name, fragmentShader->name);

	// Check if already loaded
	hashKey = Str_HashKey(name, PROGRAMS_HASH_SIZE, false);

	for (program = r_programsHashTable[hashKey]; program; program = program->nextHash){
		if (program->vertexShader != vertexShader || program->fragmentShader != fragmentShader)
			continue;

		if (!Str_ICompare(program->name, name)){
			if (!program->linkStatus)
				return NULL;

			return program;
		}
	}

	// Load the program
	program = R_LoadProgram(name, vertexShader, fragmentShader);

	if (!program->linkStatus)
		return NULL;

	return program;
}


/*
 ==============================================================================

 PROGRAM UNIFORMS & SAMPLERS

 ==============================================================================
*/


/*
 ==================
 R_GetProgramUniform
 ==================
*/
uniform_t *R_GetProgramUniform (program_t *program, const char *name){

	uniform_t	*uniform;
	int			i;

	for (i = 0, uniform = program->uniforms; i < program->numUniforms; i++, uniform++){
		if (!Str_Compare(uniform->name, name))
			return uniform;
	}

	return NULL;
}

/*
 ==================
 R_GetProgramUniformExplicit
 ==================
*/
uniform_t *R_GetProgramUniformExplicit (program_t *program, const char *name, int size, uint format){

	uniform_t	*uniform;
	int			i;

	for (i = 0, uniform = program->uniforms; i < program->numUniforms; i++, uniform++){
		if (uniform->size != size || uniform->format != format)
			continue;

		if (!Str_Compare(uniform->name, name))
			return uniform;
	}

	Com_Error(ERR_DROP, "R_GetProgramUniformExplicit: couldn't find uniform '%s' in program '%s'", name, program->name);

	return NULL;
}

/*
 ==================
 R_SetProgramSampler
 ==================
*/
void R_SetProgramSampler (program_t *program, uniform_t *uniform, int unit){

	if (uniform->unit == unit)
		return;
	uniform->unit = unit;

	GL_BindProgram(program);
	qglUniform1i(uniform->location, uniform->unit);
	GL_BindProgram(NULL);
}

/*
 ==================
 R_SetProgramSamplerExplicit
 ==================
*/
void R_SetProgramSamplerExplicit (program_t *program, const char *name, int size, uint format, int unit){

	uniform_t	*uniform;
	int			i;

	for (i = 0, uniform = program->uniforms; i < program->numUniforms; i++, uniform++){
		if (uniform->size != size || uniform->format != format)
			continue;

		if (!Str_Compare(uniform->name, name))
			break;
	}

	if (i == program->numUniforms)
		Com_Error(ERR_DROP, "R_SetProgramSamplerExplicit: couldn't find sampler '%s' in program '%s'", name, program->name);

	if (uniform->unit == unit)
		return;
	uniform->unit = unit;

	GL_BindProgram(program);
	qglUniform1i(uniform->location, uniform->unit);
	GL_BindProgram(NULL);
}


/*
 ==============================================================================

 UNIFORM UPDATES

 ==============================================================================
*/


/*
 ==================
 R_UniformFloat
 ==================
*/
void R_UniformFloat (uniform_t *uniform, float v0){

	if (uniform->values[0] == v0)
		return;

	uniform->values[0] = v0;

	qglUniform1f(uniform->location, v0);
}

/*
 ==================
 R_UniformFloat2
 ==================
*/
void R_UniformFloat2 (uniform_t *uniform, float v0, float v1){

	if (uniform->values[0] == v0 && uniform->values[1] == v1)
		return;

	uniform->values[0] = v0;
	uniform->values[1] = v1;

	qglUniform2f(uniform->location, v0, v1);
}

/*
 ==================
 R_UniformFloat3
 ==================
*/
void R_UniformFloat3 (uniform_t *uniform, float v0, float v1, float v2){

	if (uniform->values[0] == v0 && uniform->values[1] == v1 && uniform->values[2] == v2)
		return;

	uniform->values[0] = v0;
	uniform->values[1] = v1;
	uniform->values[2] = v2;

	qglUniform3f(uniform->location, v0, v1, v2);
}

/*
 ==================
 R_UniformFloat4
 ==================
*/
void R_UniformFloat4 (uniform_t *uniform, float v0, float v1, float v2, float v3){

	if (uniform->values[0] == v0 && uniform->values[1] == v1 && uniform->values[2] == v2 && uniform->values[3] == v3)
		return;

	uniform->values[0] = v0;
	uniform->values[1] = v1;
	uniform->values[2] = v2;
	uniform->values[3] = v3;

	qglUniform4f(uniform->location, v0, v1, v2, v3);
}

/*
 ==================
 R_UniformFloatArray
 ==================
*/
void R_UniformFloatArray (uniform_t *uniform, int count, const float *v){

	uniform->values[0] = 0.0f;

	qglUniform1fv(uniform->location, count, v);
}

/*
 ==================
 R_UniformVector2
 ==================
*/
void R_UniformVector2 (uniform_t *uniform, const vec2_t v){

	if (uniform->values[0] == v[0] && uniform->values[1] == v[1])
		return;

	uniform->values[0] = v[0];
	uniform->values[1] = v[1];

	qglUniform2fv(uniform->location, 1, v);
}

/*
 ==================
 R_UniformVector2Array
 ==================
*/
void R_UniformVector2Array (uniform_t *uniform, int count, const vec2_t v){

	uniform->values[0] = 0.0f;
	uniform->values[1] = 0.0f;

	qglUniform2fv(uniform->location, count, v);
}

/*
 ==================
 R_UniformVector3
 ==================
*/
void R_UniformVector3 (uniform_t *uniform, const vec3_t v){

	if (uniform->values[0] == v[0] && uniform->values[1] == v[1] && uniform->values[2] == v[2])
		return;

	uniform->values[0] = v[0];
	uniform->values[1] = v[1];
	uniform->values[2] = v[2];

	qglUniform3fv(uniform->location, 1, v);
}

/*
 ==================
 R_UniformVector3Array
 ==================
*/
void R_UniformVector3Array (uniform_t *uniform, int count, const vec3_t v){

	uniform->values[0] = 0.0f;
	uniform->values[1] = 0.0f;
	uniform->values[2] = 0.0f;

	qglUniform3fv(uniform->location, count, v);
}

/*
 ==================
 R_UniformVector4
 ==================
*/
void R_UniformVector4 (uniform_t *uniform, const vec4_t v){

	if (uniform->values[0] == v[0] && uniform->values[1] == v[1] && uniform->values[2] == v[2] && uniform->values[3] == v[3])
		return;

	uniform->values[0] = v[0];
	uniform->values[1] = v[1];
	uniform->values[2] = v[2];
	uniform->values[3] = v[3];

	qglUniform4fv(uniform->location, 1, v);
}

/*
 ==================
 R_UniformVector4Array
 ==================
*/
void R_UniformVector4Array (uniform_t *uniform, int count, const vec4_t v){

	uniform->values[0] = 0.0f;
	uniform->values[1] = 0.0f;
	uniform->values[2] = 0.0f;
	uniform->values[3] = 0.0f;

	qglUniform4fv(uniform->location, count, v);
}

/*
 ==================
 R_UniformMatrix3
 ==================
*/
void R_UniformMatrix3 (uniform_t *uniform, bool transpose, const mat3_t m){

	qglUniformMatrix3fv(uniform->location, 1, transpose, m);
}

/*
 ==================
 R_UniformMatrix3Array
 ==================
*/
void R_UniformMatrix3Array (uniform_t *uniform, int count, bool transpose, const mat3_t m){

	qglUniformMatrix3fv(uniform->location, count, transpose, m);
}

/*
 ==================
 R_UniformMatrix4
 ==================
*/
void R_UniformMatrix4 (uniform_t *uniform, bool transpose, const mat4_t m){

	qglUniformMatrix4fv(uniform->location, 1, transpose, m);
}

/*
 ==================
 R_UniformMatrix4Array
 ==================
*/
void R_UniformMatrix4Array (uniform_t *uniform, int count, bool transpose, const mat4_t m){

	qglUniformMatrix4fv(uniform->location, count, transpose, m);
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_ListPrograms_f
 ==================
*/
static void R_ListPrograms_f (){

	program_t	*program;
	int			i;

	Com_Printf("\n");
	Com_Printf("      ufrms -name-----------\n");

	for (i = 0; i < r_numPrograms; i++){
		program = r_programs[i];

		Com_Printf("%4i: ", i);

		Com_Printf("%5i ", program->numUniforms);

		Com_Printf("%s%s\n", program->name, (!program->linkStatus) ? " (INVALID)" : "");
	}

	Com_Printf("----------------------------\n");
	Com_Printf("%i total programs\n", r_numPrograms);
	Com_Printf("\n");
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_InitPrograms
 ==================
*/
void R_InitPrograms (){

	// Add commands
	Cmd_AddCommand("listPrograms", R_ListPrograms_f, "Lists loaded programs", NULL);
}

/*
 ==================
 R_ShutdownPrograms
 ==================
*/
void R_ShutdownPrograms (){

	program_t	*program;
	int			i;

	// Remove commands
	Cmd_RemoveCommand("listPrograms");

	// Delete all the programs
	qglUseProgram(0);

	for (i = 0; i < r_numPrograms; i++){
		program = r_programs[i];

		qglDetachShader(program->programId, program->vertexShader->shaderId);
		qglDetachShader(program->programId, program->fragmentShader->shaderId);

		qglDeleteProgram(program->programId);
	}

	// Clear program list
	Mem_Fill(r_programsHashTable, 0, sizeof(r_programsHashTable));
	Mem_Fill(r_programs, 0, sizeof(r_programs));

	r_numPrograms = 0;
}