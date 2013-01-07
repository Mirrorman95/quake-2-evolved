
in vec4						va_TexCoord;

out vec2					v_TexCoord;


void main (){

	// Position invariant
	gl_Position = ftransform();

	// Copy the texture coord
	v_TexCoord = va_TexCoord.st;
}