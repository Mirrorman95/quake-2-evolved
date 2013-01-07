
in vec2						v_TexCoord;
in vec4						v_TexCoords[6];

uniform sampler2D				u_ColorMap;


void main (){

	vec4	color;

	// Blur using a 13x13 Gaussian filter
	color = texture(u_ColorMap, v_TexCoord) * 0.137023;

	color += texture(u_ColorMap, v_TexCoords[0].st) * 0.129618;
	color += texture(u_ColorMap, v_TexCoords[0].pq) * 0.129618;

	color += texture(u_ColorMap, v_TexCoords[1].st) * 0.109719;
	color += texture(u_ColorMap, v_TexCoords[1].pq) * 0.109719;

	color += texture(u_ColorMap, v_TexCoords[2].st) * 0.083109;
	color += texture(u_ColorMap, v_TexCoords[2].pq) * 0.083109;

	color += texture(u_ColorMap, v_TexCoords[3].st) * 0.056332;
	color += texture(u_ColorMap, v_TexCoords[3].pq) * 0.056332;

	color += texture(u_ColorMap, v_TexCoords[4].st) * 0.034167;
	color += texture(u_ColorMap, v_TexCoords[4].pq) * 0.034167;

	color += texture(u_ColorMap, v_TexCoords[5].st) * 0.018544;
	color += texture(u_ColorMap, v_TexCoords[5].pq) * 0.018544;

	// Write the final color
	gl_FragColor = color;
}