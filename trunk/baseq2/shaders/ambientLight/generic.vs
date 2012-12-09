
in vec4						va_TexCoord;
in vec4						va_Color;

out vec2					v_BumpTexCoord;
out vec2					v_DiffuseTexCoord;
out vec4					v_LightTexCoord;
out vec3					v_VertexColor;

uniform mat4					u_BumpMatrix;
uniform mat4					u_DiffuseMatrix;
uniform mat4					u_LightMatrix;
uniform vec2					u_ColorScaleAndBias;


void main (){

	// Position invariant
	gl_Position = ftransform();

#ifndef GLSL_ATI

	// Support clipping planes
	gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;

#endif

	// Transform the bump map texture coord
	v_BumpTexCoord.s = dot(va_TexCoord, u_BumpMatrix[0]);
	v_BumpTexCoord.t = dot(va_TexCoord, u_BumpMatrix[1]);

	// Transform the diffuse map texture coord
	v_DiffuseTexCoord.s = dot(va_TexCoord, u_DiffuseMatrix[0]);
	v_DiffuseTexCoord.t = dot(va_TexCoord, u_DiffuseMatrix[1]);

	// Compute the light projection/falloff map texture coord
	v_LightTexCoord.s = dot(gl_Vertex, u_LightMatrix[0]);
	v_LightTexCoord.t = dot(gl_Vertex, u_LightMatrix[1]);
	v_LightTexCoord.p = dot(gl_Vertex, u_LightMatrix[2]);
	v_LightTexCoord.q = dot(gl_Vertex, u_LightMatrix[3]);

	// Compute the vertex color
	v_VertexColor = va_Color.rgb * u_ColorScaleAndBias.x + u_ColorScaleAndBias.y;
}