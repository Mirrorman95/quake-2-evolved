
in vec2						v_TexCoord;
in vec4						v_TexCoords[4];

uniform sampler2D				u_ColorMap;


void main (){

	vec4	color;

	// Blur using a 9x9 Gaussian filter
	color = texture(u_ColorMap, v_TexCoord) * 0.153170;

	color += texture(u_ColorMap, v_TexCoords[0].st) * 0.144893;
	color += texture(u_ColorMap, v_TexCoords[0].pq) * 0.144893;

	color += texture(u_ColorMap, v_TexCoords[1].st) * 0.122649;
	color += texture(u_ColorMap, v_TexCoords[1].pq) * 0.122649;

	color += texture(u_ColorMap, v_TexCoords[2].st) * 0.092903;
	color += texture(u_ColorMap, v_TexCoords[2].pq) * 0.092903;

	color += texture(u_ColorMap, v_TexCoords[3].st) * 0.062970;
	color += texture(u_ColorMap, v_TexCoords[3].pq) * 0.062970;

	// Write the final color
	gl_FragColor = color;
}