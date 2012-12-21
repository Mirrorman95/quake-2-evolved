
in vec2						v_AlphaTexCoord;
in vec2						v_BumpTexCoord;
in float					v_VertexAlpha;

uniform vec4					u_CoordScaleAndBias;	// Passed in automatically
uniform float					u_AlphaModulate;
uniform float					u_DistortionScale;

uniform sampler2D				u_AlphaMap;
uniform sampler2D				u_BumpMap;
uniform sampler2D				u_ColorMap;


void main (){

	vec3	localNormal;
	vec2	coordOffset;
	float	mask;

	// Load the mask from the alpha map and modulate by the alpha modulate and vertex alpha
	mask = texture(u_AlphaMap, v_AlphaTexCoord).a * u_AlphaModulate * v_VertexAlpha;

	// Discard the fragment if possible
	if (mask <= 0.0)
		discard;

	// Load the normal vector from the bump map
	localNormal = texture(u_BumpMap, v_BumpTexCoord).rgb * 2.0 - 1.0;
	localNormal = normalize(localNormal);

	// Load the color map with a modified texture coord
	coordOffset = localNormal.xy * u_DistortionScale * mask;

	gl_FragColor.rgb = texture(u_ColorMap, (gl_FragCoord.xy * u_CoordScaleAndBias.xy + u_CoordScaleAndBias.zw) + coordOffset).rgb;
}