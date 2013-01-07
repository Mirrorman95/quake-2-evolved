
in vec2						v_TexCoord;
in vec4						v_TexCoords[2];

uniform sampler2D				u_ColorMap;


void main (){

	vec4	color;

	// Blur using a 5x5 Gaussian filter
	color = texture(u_ColorMap, v_TexCoord) * 0.222549;

	color += texture(u_ColorMap, v_TexCoords[0].st) * 0.210522;
	color += texture(u_ColorMap, v_TexCoords[0].pq) * 0.210522;

	color += texture(u_ColorMap, v_TexCoords[1].st) * 0.178203;
	color += texture(u_ColorMap, v_TexCoords[1].pq) * 0.178203;

	// Write the final color
	gl_FragColor = color;
}