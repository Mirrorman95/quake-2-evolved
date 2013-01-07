
in vec2						v_TexCoord;

uniform vec2					u_STOffset1;
uniform vec2					u_STOffset2;
uniform vec2					u_STOffset3;
uniform float					u_BloomContrast;
uniform float					u_BloomThreshold;

uniform sampler2D				u_ColorMap;


void main (void){

	vec3	color;
	float	luminance;

	// Downsample using a box filter
	color = texture(u_ColorMap, v_TexCoord).rgb;
	color += texture(u_ColorMap, v_TexCoord + u_STOffset1).rgb;
	color += texture(u_ColorMap, v_TexCoord + u_STOffset2).rgb;
	color += texture(u_ColorMap, v_TexCoord + u_STOffset3).rgb;

	color *= 0.25;

	// Compute luminance
	luminance = (color.r * 0.2125) + (color.g * 0.7154) + (color.b * 0.0721);

	// Adjust contrast
	if (u_BloomContrast == 0.0)
		luminance = 1.0;
	else
		luminance = pow(luminance, u_BloomContrast);

	// Filter out dark pixels
	luminance = max(luminance - u_BloomThreshold, 0.0);

	// Write the final color modulated by the calculated luminance
	gl_FragColor.rgb = color * luminance;
}