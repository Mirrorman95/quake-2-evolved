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
// r_render.c - Function routines used by the common renderer passes
//


#include "r_local.h"


/*
 ==============================================================================

 MATERIAL EXPRESSION EVALUATION

 ==============================================================================
*/


/*
 ==================
 RB_EvaluateRegisters
 ==================
*/
void RB_EvaluateRegisters (material_t *material, float time, const float *parms){

	float	*registers = material->expressionRegisters;
	expOp_t	*op;
	int		i;

	if (r_skipExpressions->integerValue)
		return;

	// Update the predefined registers
	registers[EXP_REGISTER_CONSTANT_ONE] = 1.0f;
	registers[EXP_REGISTER_CONSTANT_ZERO] = 0.0f;
	registers[EXP_REGISTER_TIME] = time;
	registers[EXP_REGISTER_PARM0] = parms[0];
	registers[EXP_REGISTER_PARM1] = parms[1];
	registers[EXP_REGISTER_PARM2] = parms[2];
	registers[EXP_REGISTER_PARM3] = parms[3];
	registers[EXP_REGISTER_PARM4] = parms[4];
	registers[EXP_REGISTER_PARM5] = parms[5];
	registers[EXP_REGISTER_PARM6] = parms[6];
	registers[EXP_REGISTER_PARM7] = parms[7];

	// Evaluate all the registers if needed
	if (!r_skipConstantExpressions->integerValue && material->constantExpressions)
		return;

	for (i = 0, op = material->expressionOps; i < material->numOps; i++, op++){
		switch (op->type){
		case OP_TYPE_MULTIPLY:
			registers[op->c] = registers[op->a] * registers[op->b];
			break;
		case OP_TYPE_DIVIDE:
			if (registers[op->b] == 0.0f){
				Com_DPrintf(S_COLOR_YELLOW "Material '%s' has a division by zero\n", material->name);

				registers[op->c] = 0.0f;
				break;
			}

			registers[op->c] = registers[op->a] / registers[op->b];
			break;
		case OP_TYPE_MOD:
			if (registers[op->b] == 0.0f){
				Com_DPrintf(S_COLOR_YELLOW "Material '%s' has a division by zero\n", material->name);

				registers[op->c] = 0.0f;
				break;
			}

			registers[op->c] = FloatToInt(FMod(registers[op->a], registers[op->b]));
			break;
		case OP_TYPE_ADD:
			registers[op->c] = registers[op->a] + registers[op->b];
			break;
		case OP_TYPE_SUBTRACT:
			registers[op->c] = registers[op->a] - registers[op->b];
			break;
		case OP_TYPE_GREATER:
			registers[op->c] = registers[op->a] > registers[op->b];
			break;
		case OP_TYPE_LESS:
			registers[op->c] = registers[op->a] < registers[op->b];
			break;
		case OP_TYPE_GEQUAL:
			registers[op->c] = registers[op->a] >= registers[op->b];
			break;
		case OP_TYPE_LEQUAL:
			registers[op->c] = registers[op->a] <= registers[op->b];
			break;
		case OP_TYPE_EQUAL:
			registers[op->c] = registers[op->a] == registers[op->b];
			break;
		case OP_TYPE_NOTEQUAL:
			registers[op->c] = registers[op->a] != registers[op->b];
			break;
		case OP_TYPE_AND:
			registers[op->c] = registers[op->a] && registers[op->b];
			break;
		case OP_TYPE_OR:
			registers[op->c] = registers[op->a] || registers[op->b];
			break;
		case OP_TYPE_TABLE:
			registers[op->c] = LUT_LookupTable(op->a, registers[op->b]);
			break;
		default:
			Com_Error(ERR_DROP, "RB_EvaluateRegisters: bad op type (%i)", op->type);
		}
	}
}

/*
 ==============================================================================

 MATERIAL RENDERING SETUP

 ==============================================================================
*/


/*
 ==================
 RB_Cull
 ==================
*/
void RB_Cull (material_t *material){

	if (material->cullType == CT_TWO_SIDED)
		GL_Disable(GL_CULL_FACE);
	else {
		GL_Enable(GL_CULL_FACE);

		if (material->cullType == CT_FRONT_SIDED)
			GL_CullFace(GL_FRONT);
		else
			GL_CullFace(GL_BACK);
	}
}

/*
 ==================
 RB_PolygonOffset
 ==================
*/
void RB_PolygonOffset (material_t *material){

	if (material->flags & MF_POLYGONOFFSET){
		GL_Enable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue * material->polygonOffset);
	}
	else
		GL_Disable(GL_POLYGON_OFFSET_FILL);
}

/*
 ==================
 RB_DrawState
 ==================
*/
void RB_DrawState (material_t *material, stage_t *stage){

	if (stage->drawState & DS_POLYGONOFFSET){
		GL_Enable(GL_POLYGON_OFFSET_FILL);
		GL_PolygonOffset(r_offsetFactor->floatValue, r_offsetUnits->floatValue * stage->polygonOffset);
	}
	else
		GL_Disable(GL_POLYGON_OFFSET_FILL);

	if (stage->drawState & DS_BLEND){
		GL_Enable(GL_BLEND);
		GL_BlendFunc(stage->blendSrc, stage->blendDst);
		GL_BlendEquation(stage->blendMode);
	}
	else
		GL_Disable(GL_BLEND);

	if (stage->drawState & DS_ALPHATEST){
		GL_Enable(GL_ALPHA_TEST);
		GL_AlphaFunc(GL_GREATER, material->expressionRegisters[stage->alphaTestRegister]);
	}
	else
		GL_Disable(GL_ALPHA_TEST);

	if (stage->drawState & DS_IGNOREALPHATEST)
		GL_DepthFunc(GL_LEQUAL);
	else
		GL_DepthFunc(GL_EQUAL);

	GL_ColorMask(!(stage->drawState & DS_MASKRED), !(stage->drawState & DS_MASKGREEN), !(stage->drawState & DS_MASKBLUE), !(stage->drawState & DS_MASKALPHA));
}

/*
 ==================
 
 ==================
*/
void RB_BindTexture (material_t *material, texture_t *texture, int cinematicHandle){

	// If a cinematic
	if (cinematicHandle){

		return;
	}

	// Bind the texture
	GL_BindTexture(texture);

	// Capture the current color if needed
	if (texture == rg.currentColorTexture && (material->flags & MF_UPDATECURRENTCOLOR)){
		backEnd.currentColorCaptured = material->sort;

		R_CopyFramebufferToTexture(texture, 0, backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);
	}

	// Capture the current depth if needed
	if (texture == rg.currentDepthTexture && (material->flags & MF_UPDATECURRENTDEPTH)){
		backEnd.currentDepthCaptured = true;

		R_CopyFramebufferToTexture(texture, 0, backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);
	}
}

/*
 ==================
 
 ==================
*/
void RB_BindMultitexture (material_t *material, texture_t *texture, int cinematicHandle, int unit){

	// If a cinematic
	if (cinematicHandle){

		return;
	}

	// Bind the texture
	GL_BindMultitexture(texture, unit);

	// Capture the current color if needed
	if (texture == rg.currentColorTexture && (material->flags & MF_UPDATECURRENTCOLOR)){
		backEnd.currentColorCaptured = material->sort;

		R_CopyFramebufferToTexture(texture, unit, backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);
	}

	// Capture the current depth if needed
	if (texture == rg.currentDepthTexture && (material->flags & MF_UPDATECURRENTDEPTH)){
		backEnd.currentDepthCaptured = true;

		R_CopyFramebufferToTexture(texture, unit, backEnd.viewport.x, backEnd.viewport.y, backEnd.viewport.width, backEnd.viewport.height);
	}
}

/*
 ==================
 
 ==================
*/
void RB_ComputeTexturePlanes (material_t *material, textureStage_t *textureStage, vec4_t planes[4]){

	mat4_t	matrix;

	if (textureStage->texGen == TG_EXPLICIT || textureStage->texGen == TG_NORMAL || textureStage->texGen == TG_REFLECT)
		return;

	if (textureStage->texGen == TG_VECTOR){
		VectorCopy(textureStage->texGenVectors[0], planes[0]);
		VectorCopy(textureStage->texGenVectors[1], planes[1]);

		return;
	}

	if (textureStage->texGen == TG_SKYBOX){
		Matrix4_Identity(matrix);
		Matrix4_Translate(matrix, -backEnd.localParms.viewOrigin[0], -backEnd.localParms.viewOrigin[1], -backEnd.localParms.viewOrigin[2]);
	}

	if (textureStage->texGen == TG_SCREEN){

		if (material->subviewType == ST_MIRROR){

		}
	}
}

/*
 ==================
 RB_ComputeTextureMatrix
 ==================
*/
void RB_ComputeTextureMatrix (material_t *material, textureStage_t *textureStage, mat4_t matrix){

	float	s, t, angle;
	int		i;

	Matrix4_Identity(matrix);

	for (i = 0; i < textureStage->numTexMods; i++){
		switch (textureStage->texMods[i]){
		case TM_TRANSLATE:
			s = material->expressionRegisters[textureStage->texModsRegisters[i][0]];
			t = material->expressionRegisters[textureStage->texModsRegisters[i][1]];

			Matrix4_Translate(matrix, s, t, 0.0f);

			break;
		case TM_SCALE:
			s = material->expressionRegisters[textureStage->texModsRegisters[i][0]];
			t = material->expressionRegisters[textureStage->texModsRegisters[i][1]];

			Matrix4_Scale(matrix, s, t, 1.0f);

			break;
		case TM_CENTERSCALE:
			s = material->expressionRegisters[textureStage->texModsRegisters[i][0]];
			t = material->expressionRegisters[textureStage->texModsRegisters[i][1]];

			Matrix4_Translate(matrix, 0.5f, 0.5f, 0.0f);
			Matrix4_Scale(matrix, s, t, 1.0f);
			Matrix4_Translate(matrix, -0.5f, -0.5f, 0.0f);

			break;
		case TM_SHEAR:
			s = material->expressionRegisters[textureStage->texModsRegisters[i][0]];
			t = material->expressionRegisters[textureStage->texModsRegisters[i][1]];

			Matrix4_Translate(matrix, 0.5f, 0.5f, 0.0f);
			Matrix4_Shear(matrix, s, t, 1.0f);
			Matrix4_Translate(matrix, -0.5f, -0.5f, 0.0f);

			break;
		case TM_ROTATE:
			angle = material->expressionRegisters[textureStage->texModsRegisters[i][0]];

			Matrix4_Translate(matrix, 0.5f, 0.5f, 0.0f);
			Matrix4_Rotate(matrix, angle, 0.0f, 0.0f, 1.0f);
			Matrix4_Translate(matrix, -0.5f, -0.5f, 0.0f);

			break;
		default:
			Com_Error(ERR_DROP, "RB_ComputeTextureMatrix: unknown texMod in material '%s'", material->name);
		}
	}
}

/*
 ==================
 RB_SetupTextureStage
 ==================
*/
void RB_SetupTextureStage (material_t *material, textureStage_t *textureStage){

	mat4_t matrix;
	vec4_t planes[4];

	// Enable texturing
	GL_EnableTexture(textureStage->texture->target);

	// Bind the texture
	RB_BindTexture(material, textureStage->texture, textureStage->cinematicHandle);

	// Set up the texture coords and matrix
	RB_ComputeTexturePlanes(material, textureStage, planes);

	switch (textureStage->texGen){
	case TG_EXPLICIT:
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, sizeof(glVertex_t), GL_VERTEX_TEXCOORD(backEnd.vertexPointer));

		if (!textureStage->numTexMods)
			GL_LoadIdentity(GL_TEXTURE);
		else {
			RB_ComputeTextureMatrix(material, textureStage, matrix);

			GL_LoadMatrix(GL_TEXTURE, matrix);
		}

		break;
	case TG_VECTOR:
		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);
		GL_TexGen(GL_T, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, planes[0]);
		qglTexGenfv(GL_T, GL_OBJECT_PLANE, planes[1]);

		if (!textureStage->numTexMods)
			GL_LoadIdentity(GL_TEXTURE);
		else {
			RB_ComputeTextureMatrix(material, textureStage, matrix);

			GL_LoadMatrix(GL_TEXTURE, matrix);
		}

		break;
	case TG_NORMAL:
		qglEnableClientState(GL_NORMAL_ARRAY);
		qglNormalPointer(GL_FLOAT, sizeof(glVertex_t), GL_VERTEX_NORMAL(backEnd.vertexPointer));

		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_R);

		GL_TexGen(GL_S, GL_NORMAL_MAP);
		GL_TexGen(GL_T, GL_NORMAL_MAP);
		GL_TexGen(GL_R, GL_NORMAL_MAP);

		GL_LoadTransposeMatrix(GL_TEXTURE, backEnd.viewParms.modelviewMatrix);

		break;
	case TG_REFLECT:
		qglEnableClientState(GL_NORMAL_ARRAY);
		qglNormalPointer(GL_FLOAT, sizeof(glVertex_t), GL_VERTEX_NORMAL(backEnd.vertexPointer));

		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_R);

		GL_TexGen(GL_S, GL_REFLECTION_MAP);
		GL_TexGen(GL_T, GL_REFLECTION_MAP);
		GL_TexGen(GL_R, GL_REFLECTION_MAP);

		GL_LoadTransposeMatrix(GL_TEXTURE, backEnd.viewParms.modelviewMatrix);

		break;
	case TG_SKYBOX:
		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_R);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);
		GL_TexGen(GL_T, GL_OBJECT_LINEAR);
		GL_TexGen(GL_R, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, planes[0]);
		qglTexGenfv(GL_T, GL_OBJECT_PLANE, planes[1]);
		qglTexGenfv(GL_R, GL_OBJECT_PLANE, planes[2]);

		GL_LoadIdentity(GL_TEXTURE);

		break;
	case TG_SCREEN:
		GL_Enable(GL_TEXTURE_GEN_S);
		GL_Enable(GL_TEXTURE_GEN_T);
		GL_Enable(GL_TEXTURE_GEN_Q);

		GL_TexGen(GL_S, GL_OBJECT_LINEAR);
		GL_TexGen(GL_T, GL_OBJECT_LINEAR);
		GL_TexGen(GL_Q, GL_OBJECT_LINEAR);

		qglTexGenfv(GL_S, GL_OBJECT_PLANE, planes[0]);
		qglTexGenfv(GL_T, GL_OBJECT_PLANE, planes[1]);
		qglTexGenfv(GL_Q, GL_OBJECT_PLANE, planes[3]);

		if (!textureStage->numTexMods)
			GL_LoadIdentity(GL_TEXTURE);
		else {
			RB_ComputeTextureMatrix(material, textureStage, matrix);

			GL_LoadMatrix(GL_TEXTURE, matrix);
		}

		break;
	default:
		Com_Error(ERR_DROP, "RB_SetupTextureStage: unknown texGen in material '%s'", material->name);
	}
}

/*
 ==================
 RB_CleanupTextureStage
 ==================
*/
void RB_CleanupTextureStage (material_t *material, textureStage_t *textureStage){

	if (textureStage->texGen == TG_EXPLICIT){
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		return;
	}

	if (textureStage->texGen == TG_NORMAL || textureStage->texGen == TG_REFLECT)
		qglDisableClientState(GL_NORMAL_ARRAY);

	GL_Disable(GL_TEXTURE_GEN_Q);
	GL_Disable(GL_TEXTURE_GEN_R);
	GL_Disable(GL_TEXTURE_GEN_T);
	GL_Disable(GL_TEXTURE_GEN_S);
}

/*
 ==================
 RB_SetupColorStage
 ==================
*/
void RB_SetupColorStage (material_t *material, colorStage_t *colorStage){

	vec4_t color;

	// Set up the color
	if (backEnd.depthFilling){
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = material->expressionRegisters[colorStage->registers[3]];
	}
	else {
		color[0] = material->expressionRegisters[colorStage->registers[0]];
		color[1] = material->expressionRegisters[colorStage->registers[1]];
		color[2] = material->expressionRegisters[colorStage->registers[2]];
		color[3] = material->expressionRegisters[colorStage->registers[3]];
	}

	// Set up the vertex color and texture environment
	switch (colorStage->vertexColor){
	case VC_IGNORE:
		if (!backEnd.depthFilling && colorStage->identity){
			GL_TexEnv(GL_REPLACE);
			break;
		}

		qglColor4fv(color);

		GL_TexEnv(GL_MODULATE);

		break;
	case VC_MODULATE:
		qglEnableClientState(GL_COLOR_ARRAY);
		qglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glVertex_t), GL_VERTEX_COLOR(backEnd.vertexPointer));

		GL_TexEnv(GL_MODULATE);

		if (!backEnd.depthFilling && colorStage->identity)
			break;

		GL_SelectTexture(1);
		GL_EnableTexture(rg.whiteTexture->target);
		GL_BindTexture(rg.whiteTexture);

		GL_TexEnv(GL_COMBINE);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, 1);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PREVIOUS);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_CONSTANT);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

		qglTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

		GL_SelectTexture(0);

		break;
	case VC_INVERSE_MODULATE:
		qglEnableClientState(GL_COLOR_ARRAY);
		qglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glVertex_t), GL_VERTEX_COLOR(backEnd.vertexPointer));

		GL_TexEnv(GL_COMBINE);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_ONE_MINUS_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, 1);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

		if (!backEnd.depthFilling && colorStage->identity)
			break;

		GL_SelectTexture(1);
		GL_EnableTexture(rg.whiteTexture->target);
		GL_BindTexture(rg.whiteTexture);

		GL_TexEnv(GL_COMBINE);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		qglTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, 1);

		qglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PREVIOUS);
		qglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_CONSTANT);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
		qglTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

		qglTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

		GL_SelectTexture(0);

		break;
	default:
		Com_Error(ERR_DROP, "RB_SetupColorStage: unknown vertexColor in material '%s'", material->name);
	}
}

/*
 ==================
 RB_CleanupColorStage
 ==================
*/
void RB_CleanupColorStage (material_t *material, colorStage_t *colorStage){

	if (colorStage->vertexColor == VC_IGNORE)
		return;

	qglDisableClientState(GL_COLOR_ARRAY);

	if (!backEnd.depthFilling && colorStage->identity)
		return;

	GL_SelectTexture(1);
	GL_DisableTexture();
	GL_SelectTexture(0);
}

/*
 ==================
 
 ==================
*/
void RB_SetupShaderStage (material_t *material, shaderStage_t *shaderStage){

	uniform_t		*uniform;
	shaderParm_t	*shaderParm;
	shaderMap_t		*shaderMap;
	int				i;

	// Bind the program
	GL_BindProgram(shaderStage->program);

	// Set up the arrays
	if (shaderStage->program->vertexAttribs & VA_NORMAL){
		qglEnableVertexAttribArray(GL_ATTRIB_NORMAL);
		qglVertexAttribPointer(GL_ATTRIB_NORMAL, 3, GL_FLOAT, false, sizeof(glVertex_t), GL_VERTEX_NORMAL(backEnd.vertexPointer));
	}

	if (shaderStage->program->vertexAttribs & VA_TANGENT1){
		qglEnableVertexAttribArray(GL_ATTRIB_TANGENT1);
		qglVertexAttribPointer(GL_ATTRIB_TANGENT1, 3, GL_FLOAT, false, sizeof(glVertex_t), GL_VERTEX_TANGENT1(backEnd.vertexPointer));
	}

	if (shaderStage->program->vertexAttribs & VA_TANGENT2){
		qglEnableVertexAttribArray(GL_ATTRIB_TANGENT2);
		qglVertexAttribPointer(GL_ATTRIB_TANGENT2, 3, GL_FLOAT, false, sizeof(glVertex_t), GL_VERTEX_TANGENT2(backEnd.vertexPointer));
	}

	if (shaderStage->program->vertexAttribs & VA_TEXCOORD){
		qglEnableVertexAttribArray(GL_ATTRIB_TEXCOORD);
		qglVertexAttribPointer(GL_ATTRIB_TEXCOORD, 2, GL_FLOAT, false, sizeof(glVertex_t), GL_VERTEX_TEXCOORD(backEnd.vertexPointer));
	}

	if (shaderStage->program->vertexAttribs & VA_COLOR){
		qglEnableVertexAttribArray(GL_ATTRIB_COLOR);
		qglVertexAttribPointer(GL_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, true, sizeof(glVertex_t), GL_VERTEX_COLOR(backEnd.vertexPointer));
	}

	// Set up the predefined uniforms
	for (i = 0, uniform = shaderStage->program->uniforms; i < shaderStage->program->numUniforms; i++, uniform++){
		if (uniform->type == UT_CUSTOM)
			continue;

		switch (uniform->type){
		case UT_VIEW_ORIGIN:
			R_UniformVector3(uniform, backEnd.localParms.viewOrigin);
			break;
		case UT_VIEW_AXIS:

			break;
		case UT_ENTITY_ORIGIN:
			R_UniformVector3(uniform, backEnd.entity->origin);
			break;
		case UT_ENTITY_AXIS:

			break;
		case UT_SUN_ORIGIN:

			break;
		case UT_SUN_DIRECTION:

			break;
		case UT_SUN_COLOR:

			break;
		case UT_SCREEN_MATRIX:

			break;
		case UT_COORD_SCALE_AND_BIAS:
			R_UniformFloat4(uniform, backEnd.coordScale[0], backEnd.coordScale[1], backEnd.coordBias[0], backEnd.coordBias[1]);
			break;
		case UT_COLOR_SCALE_AND_BIAS:
			R_UniformFloat2(uniform, shaderStage->colorScale, shaderStage->colorBias);
			break;
		default:
			Com_Error(ERR_DROP, "RB_SetupShaderStage: unknown shader uniform type in material '%s'", material->name);
		}
	}

	// Set up the shader parms
	for (i = 0, shaderParm = shaderStage->shaderParms; i < shaderStage->numShaderParms; i++, shaderParm++){
		switch (shaderParm->uniform->format){
		case GL_FLOAT:
			R_UniformFloat(shaderParm->uniform, material->expressionRegisters[shaderParm->registers[0]]);
			break;
		case GL_FLOAT_VEC2:
			R_UniformFloat2(shaderParm->uniform, material->expressionRegisters[shaderParm->registers[0]], material->expressionRegisters[shaderParm->registers[1]]);
			break;
		case GL_FLOAT_VEC3:
			R_UniformFloat3(shaderParm->uniform, material->expressionRegisters[shaderParm->registers[0]], material->expressionRegisters[shaderParm->registers[1]], material->expressionRegisters[shaderParm->registers[2]]);
			break;
		case GL_FLOAT_VEC4:
			R_UniformFloat4(shaderParm->uniform, material->expressionRegisters[shaderParm->registers[0]], material->expressionRegisters[shaderParm->registers[1]], material->expressionRegisters[shaderParm->registers[2]], material->expressionRegisters[shaderParm->registers[3]]);
			break;
		default:
			Com_Error(ERR_DROP, "RB_SetupShaderStage: unknown shader uniform format in material '%s'", material->name);
		}
	}

	// Set up the shader maps
	for (i = 0, shaderMap = shaderStage->shaderMaps; i < shaderStage->numShaderMaps; i++, shaderMap++)
		RB_BindMultitexture(material, shaderMap->texture, shaderMap->cinematicHandle, shaderMap->uniform->unit);
}

/*
 ==================
 RB_CleanupShaderStage
 ==================
*/
void RB_CleanupShaderStage (material_t *material, shaderStage_t *shaderStage){

	GL_SelectTexture(0);

	if (shaderStage->program->vertexAttribs & VA_COLOR)
		qglDisableVertexAttribArray(GL_ATTRIB_COLOR);

	if (shaderStage->program->vertexAttribs & VA_TEXCOORD)
		qglDisableVertexAttribArray(GL_ATTRIB_TEXCOORD);

	if (shaderStage->program->vertexAttribs & VA_TANGENT2)
		qglDisableVertexAttribArray(GL_ATTRIB_TANGENT2);

	if (shaderStage->program->vertexAttribs & VA_TANGENT1)
		qglDisableVertexAttribArray(GL_ATTRIB_TANGENT1);

	if (shaderStage->program->vertexAttribs & VA_NORMAL)
		qglDisableVertexAttribArray(GL_ATTRIB_NORMAL);

	GL_BindProgram(NULL);
}


/*
 ==============================================================================

 RENDERING SETUP & UTILITIES

 ==============================================================================
*/


/*
 ==================
 RB_EntityState
 ==================
*/
void RB_EntityState (renderEntity_t *entity){

	mat4_t	transformMatrix;

	// Transform view origin and view matrix into local space
	if (entity == rg.worldEntity){
		VectorCopy(backEnd.viewParms.origin, backEnd.localParms.viewOrigin);
		Matrix3_Copy(backEnd.viewParms.axis, backEnd.localParms.viewAxis);
	}
	else {
		R_WorldPointToLocal(backEnd.viewParms.origin, backEnd.localParms.viewOrigin, entity->origin, entity->axis);
		R_WorldAxisToLocal(backEnd.viewParms.axis, backEnd.localParms.viewAxis, entity->axis);
	}

	// Compute the view matrix
	if (entity == rg.worldEntity)
		Matrix4_Copy(backEnd.viewParms.modelviewMatrix, backEnd.localParms.viewMatrix);
	else {
		if (entity->type == RE_MODEL){
			Matrix4_Set(transformMatrix, entity->axis, entity->origin);
			Matrix4_MultiplyFast(backEnd.viewParms.modelviewMatrix, transformMatrix, backEnd.localParms.viewMatrix);
		}
		else
			Matrix4_Copy(backEnd.viewParms.modelviewMatrix, backEnd.localParms.viewMatrix);
	}

	// Set the modelview matrix
	GL_LoadMatrix(GL_MODELVIEW, backEnd.localParms.viewMatrix);

	// Set the depth range
	if (entity->depthHack)
		GL_DepthRange(0.0f, 0.3f);
	else
		GL_DepthRange(0.0f, 1.0f);
}

/*
 ==================
 RB_TransformLightForEntity
 ==================
*/
void RB_TransformLightForEntity (light_t *light, renderEntity_t *entity){

	// Transform light origin, light direction, and light axis into local space
	if (entity == rg.worldEntity){
		VectorCopy(light->origin, backEnd.localParms.lightOrigin);
		VectorCopy(light->direction, backEnd.localParms.lightDirection);
		Matrix3_Copy(light->axis, backEnd.localParms.lightAxis);
	}
	else {
		R_WorldPointToLocal(light->origin, backEnd.localParms.lightOrigin, entity->origin, entity->axis);
		R_WorldVectorToLocal(light->direction, backEnd.localParms.lightDirection, entity->axis);
		R_WorldAxisToLocal(light->axis, backEnd.localParms.lightAxis, entity->axis);
	}

	// Compute the light plane
	if (entity == rg.worldEntity){
		backEnd.localParms.lightPlane[0] = light->axis[0][0];
		backEnd.localParms.lightPlane[1] = light->axis[0][1];
		backEnd.localParms.lightPlane[2] = light->axis[0][2];
		backEnd.localParms.lightPlane[3] = -DotProduct(light->origin, light->axis[0]);
	}
	else {
		backEnd.localParms.lightPlane[0] = DotProduct(entity->axis[0], light->axis[0]);
		backEnd.localParms.lightPlane[1] = DotProduct(entity->axis[1], light->axis[0]);
		backEnd.localParms.lightPlane[2] = DotProduct(entity->axis[2], light->axis[0]);
		backEnd.localParms.lightPlane[3] = DotProduct(entity->origin, light->axis[0]) - DotProduct(light->origin, light->axis[0]);
	}
}

/*
 ==================

 ==================
*/
void RB_ComputeLightMatrix (light_t *light, renderEntity_t *entity, material_t *material, textureStage_t *textureStage){

	mat4_t	transformMatrix, entityMatrix, textureMatrix;
	mat4_t	tmpMatrix;
	float	distanceScale, heightScale;

	// Compute a generic, ambient, or blend light
	if (material->lightType != LT_FOG){
		if (entity == rg.worldEntity){
			if (!textureStage->numTexMods)
				Matrix4_Transpose(light->modelviewProjectionMatrix, backEnd.localParms.lightMatrix);
			else {
				RB_ComputeTextureMatrix(material, textureStage, textureMatrix);

				Matrix4_MultiplyFast(textureMatrix, light->modelviewProjectionMatrix, tmpMatrix);
				Matrix4_Transpose(tmpMatrix, backEnd.localParms.lightMatrix);
			}
		}
		else {
			Matrix4_Set(transformMatrix, entity->axis, entity->origin);
			Matrix4_MultiplyFast(light->modelviewProjectionMatrix, transformMatrix, entityMatrix);

			if (!textureStage->numTexMods)
				Matrix4_Transpose(entityMatrix, backEnd.localParms.lightMatrix);
			else {
				RB_ComputeTextureMatrix(material, textureStage, textureMatrix);

				Matrix4_MultiplyFast(textureMatrix, entityMatrix, tmpMatrix);
				Matrix4_Transpose(tmpMatrix, backEnd.localParms.lightMatrix);
			}
		}

		return;
	}

	// Compute a fog light
	if (light->fogDistance < 1.0f)
		distanceScale = 0.5f / 500.0f;
	else
		distanceScale = 0.5f / light->fogDistance;

	if (light->fogHeight < 1.0f)
		heightScale = 0.5f / 500.0f;
	else
		heightScale = 0.5f / light->fogHeight;

	// TODO!!!
}

/*
 ==================
 RB_DrawElements

 TODO: we might need to get indicies for shadows
 ==================
*/
void RB_DrawElements (){

	if (r_skipDrawElements->integerValue)
		return;

	rg.pc.draws++;
	rg.pc.totalIndices += backEnd.numIndices;
	rg.pc.totalVertices += backEnd.numVertices;

	qglDrawElements(GL_TRIANGLES, backEnd.numIndices, GL_INDEX_TYPE, backEnd.indexPointer);
}

/*
 ==================
 RB_DrawElementsWithCounters

 TODO: we might need to get indicies for shadows
 ==================
*/
void RB_DrawElementsWithCounters (int *totalIndices, int *totalVertices){

	if (r_skipDrawElements->integerValue)
		return;

	rg.pc.draws++;
	rg.pc.totalIndices += backEnd.numIndices;
	rg.pc.totalVertices += backEnd.numVertices;

	if (totalIndices)
		*totalIndices += backEnd.numIndices;
	if (totalVertices)
		*totalVertices += backEnd.numVertices;

	qglDrawElements(GL_TRIANGLES, backEnd.numIndices, GL_INDEX_TYPE, backEnd.indexPointer);
}

/*
 ==================
 RB_DrawElementsStaticIndices

 TODO: we might need to get indicies for shadows
 ==================
*/
void RB_DrawElementsStaticIndices (int numVertices, int numIndices, const void *indices){

	if (r_skipDrawElements->integerValue)
		return;

	rg.pc.draws++;
	rg.pc.totalIndices += numIndices;
	rg.pc.totalVertices += numVertices;

	qglDrawElements(GL_TRIANGLES, numIndices, GL_INDEX_TYPE, indices);
}