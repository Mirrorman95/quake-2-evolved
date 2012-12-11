
in vec3						va_Normal;
in vec3						va_Tangent1;
in vec3						va_Tangent2;
in vec4						va_TexCoord;
in vec4						va_Color;

out vec3					v_ViewVector;
out vec3					v_LightVector;
out vec2					v_BumpTexCoord;
out vec2					v_DiffuseTexCoord;
out vec2					v_SpecularTexCoord;
out vec4					v_LightTexCoord;
out vec3					v_VertexColor;

uniform vec3					u_ViewOrigin;
uniform vec3					u_LightOrigin;
uniform mat4					u_BumpMatrix;
uniform mat4					u_DiffuseMatrix;
uniform mat4					u_SpecularMatrix;
uniform mat4					u_LightMatrix;
uniform vec2					u_ColorScaleAndBias;


void main (void){

	vec3		toView, toLight;

	// Position invariant
	gl_Position = ftransform();

#ifndef GLSL_ATI

	// Support clipping planes
	gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;

#endif

	// Compute the view vector in tangent space
	toView = u_ViewOrigin - gl_Vertex.xyz;

	v_ViewVector.x = dot(toView, va_Tangent1);
	v_ViewVector.y = dot(toView, va_Tangent2);
	v_ViewVector.z = dot(toView, va_Normal);

	// Compute the light vector in tangent space
	toLight = u_LightOrigin - gl_Vertex.xyz;

	v_LightVector.x = dot(toLight, va_Tangent1);
	v_LightVector.y = dot(toLight, va_Tangent2);
	v_LightVector.z = dot(toLight, va_Normal);

	// Transform the bump map texture coord
	v_BumpTexCoord.s = dot(va_TexCoord, u_BumpMatrix[0]);
	v_BumpTexCoord.t = dot(va_TexCoord, u_BumpMatrix[1]);

	// Transform the diffuse map texture coord
	v_DiffuseTexCoord.s = dot(va_TexCoord, u_DiffuseMatrix[0]);
	v_DiffuseTexCoord.t = dot(va_TexCoord, u_DiffuseMatrix[1]);

	// Transform the specular map texture coord
	v_SpecularTexCoord.s = dot(va_TexCoord, u_SpecularMatrix[0]);
	v_SpecularTexCoord.t = dot(va_TexCoord, u_SpecularMatrix[1]);

	// Compute the light projection/falloff map texture coord
	v_LightTexCoord.s = dot(gl_Vertex, u_LightMatrix[0]);
	v_LightTexCoord.t = dot(gl_Vertex, u_LightMatrix[1]);
	v_LightTexCoord.p = dot(gl_Vertex, u_LightMatrix[2]);
	v_LightTexCoord.q = dot(gl_Vertex, u_LightMatrix[3]);

	// Compute the vertex color
	v_VertexColor = va_Color.rgb * u_ColorScaleAndBias.x + u_ColorScaleAndBias.y;
}