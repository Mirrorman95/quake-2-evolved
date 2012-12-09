
out vec4					v_LightTexCoord;

uniform mat4					u_LightMatrix;


void main (){

	// Position invariant
	gl_Position = ftransform();

#ifndef GLSL_ATI

	// Support clipping planes
	gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;

#endif

	// Compute the light projection/falloff map texture coord
	v_LightTexCoord.s = dot(gl_Vertex, u_LightMatrix[0]);
	v_LightTexCoord.t = dot(gl_Vertex, u_LightMatrix[1]);
	v_LightTexCoord.p = dot(gl_Vertex, u_LightMatrix[2]);
	v_LightTexCoord.q = dot(gl_Vertex, u_LightMatrix[3]);
}