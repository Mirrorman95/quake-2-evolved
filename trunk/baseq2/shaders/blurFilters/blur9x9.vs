
in vec4						va_TexCoord;

out vec2					v_TexCoord;
out vec4					v_TexCoords[4];

uniform vec2					u_CoordScale;


void main (){

	// Position invariant
	gl_Position = ftransform();

	// Copy the texture coord
	v_TexCoord = va_TexCoord.st;

	// Compute the offset texture coords
	v_TexCoords[0].st = va_TexCoord.st - u_CoordScale * 1.5;
	v_TexCoords[0].pq = va_TexCoord.st + u_CoordScale * 1.5;

	v_TexCoords[1].st = va_TexCoord.st - u_CoordScale * 3.5;
	v_TexCoords[1].pq = va_TexCoord.st + u_CoordScale * 3.5;

	v_TexCoords[2].st = va_TexCoord.st - u_CoordScale * 5.5;
	v_TexCoords[2].pq = va_TexCoord.st + u_CoordScale * 5.5;

	v_TexCoords[3].st = va_TexCoord.st - u_CoordScale * 7.5;
	v_TexCoords[3].pq = va_TexCoord.st + u_CoordScale * 7.5;
}