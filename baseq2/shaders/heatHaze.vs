
in vec4						va_TexCoord;
in vec4						va_Color;

out vec2					v_AlphaTexCoord;
out vec2					v_BumpTexCoord;
out float					v_VertexAlpha;

uniform vec2					u_ColorScaleAndBias;	// Passed in automatically
uniform vec2					u_BumpScale;
uniform vec2					u_BumpScroll;


void main (){

	// Position invariant
	gl_Position = ftransform();

#ifndef GLSL_ATI

	// Support clipping planes
	gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;

#endif

	// Copy the alpha map texture coord
	v_AlphaTexCoord = va_TexCoord.st;

	// Scale and scroll the bump map texture coord
	v_BumpTexCoord = va_TexCoord.st * u_BumpScale + u_BumpScroll;

	// Compute the vertex alpha
	v_VertexAlpha = va_Color.a * u_ColorScaleAndBias.x + u_ColorScaleAndBias.y;
}