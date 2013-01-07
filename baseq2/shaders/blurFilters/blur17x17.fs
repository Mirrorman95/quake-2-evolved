
in vec2						v_TexCoord;
in vec4						v_TexCoords[8];

uniform sampler2D				u_ColorMap;


void main (){

	vec4		color;

	// Blur using a 17x17 Gaussian filter
	color = texture(u_ColorMap, v_TexCoord) * 0.133571;

	color += texture(u_ColorMap, v_TexCoords[0].st) * 0.126353;
	color += texture(u_ColorMap, v_TexCoords[0].pq) * 0.126353;

	color += texture(u_ColorMap, v_TexCoords[1].st) * 0.106955;
	color += texture(u_ColorMap, v_TexCoords[1].pq) * 0.106955;

	color += texture(u_ColorMap, v_TexCoords[2].st) * 0.081015;
	color += texture(u_ColorMap, v_TexCoords[2].pq) * 0.081015;

	color += texture(u_ColorMap, v_TexCoords[3].st) * 0.054913;
	color += texture(u_ColorMap, v_TexCoords[3].pq) * 0.054913;

	color += texture(u_ColorMap, v_TexCoords[4].st) * 0.033306;
	color += texture(u_ColorMap, v_TexCoords[4].pq) * 0.033306;

	color += texture(u_ColorMap, v_TexCoords[5].st) * 0.018077;
	color += texture(u_ColorMap, v_TexCoords[5].pq) * 0.018077;

	color += texture(u_ColorMap, v_TexCoords[6].st) * 0.008779;
	color += texture(u_ColorMap, v_TexCoords[6].pq) * 0.008779;

	color += texture(u_ColorMap, v_TexCoords[7].st) * 0.003816;
	color += texture(u_ColorMap, v_TexCoords[7].pq) * 0.003816;

	// Write the final color
	gl_FragColor = color;
}