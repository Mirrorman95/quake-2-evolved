
in vec2						v_TexCoord;

uniform float					u_BaseIntensity;
uniform float					u_GlowIntensity;
uniform vec3					u_ColorShadows;
uniform vec3					u_ColorHighlights;
uniform vec3					u_ColorMidtones;
uniform vec3					u_ColorMinOutput;
uniform vec3					u_ColorMaxOutput;
uniform vec3					u_ColorSaturation;
uniform vec3					u_ColorTint;

uniform sampler2D				u_BaseMap;
uniform sampler2D				u_GlowMap;
uniform sampler2D				u_ColorTable;


void main (){

	vec3	base, glow;
	vec3	color, luminance;

	// Modulate the base map by the base intensity
	base = texture(u_BaseMap, v_TexCoord).rgb * u_BaseIntensity;

	// Modulate the glow map by the glow intensity
	glow = texture(u_GlowMap, v_TexCoord).rgb * u_GlowIntensity;

	// Add base and glow
	color = base + glow;

	// Adjust contrast
	color = max(color - u_ColorShadows, 0.0) / (u_ColorHighlights - u_ColorShadows);

	// Adjust gamma
	color = pow(color, 1.0 / u_ColorMidtones);

	// Adjust output range
	color = max(color - u_ColorMinOutput, 0.0) / (u_ColorMaxOutput - u_ColorMinOutput);

	// Adjust saturation
	luminance = vec3((color.r * 0.2125) + (color.g * 0.7154) + (color.b * 0.0721));

	color = mix(luminance, color, u_ColorSaturation);

	// Modulate by the color tint
	color = color * u_ColorTint;

	// Lookup the final color from the color table
	gl_FragColor.r = texture(u_ColorTable, vec2(color.r, 0.5)).r;
	gl_FragColor.g = texture(u_ColorTable, vec2(color.g, 0.5)).g;
	gl_FragColor.b = texture(u_ColorTable, vec2(color.b, 0.5)).b;
}