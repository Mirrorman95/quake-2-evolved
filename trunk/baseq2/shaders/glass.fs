
in vec3						v_ViewVector;
in vec2						v_TexCoord;
in mat3						v_LocalToGlobal;

uniform vec4					u_CoordScaleAndBias;	// Passed in automatically
uniform vec2					u_DistortionScale;
uniform vec3					u_ColorTint;
uniform vec3					u_FresnelParms;

uniform sampler2D				u_BumpMap;
uniform sampler2D				u_ColorMap;
uniform sampler2D				u_RefractionMap;
uniform samplerCube				u_ReflectionMap;


void main (){

	vec3	toView;
	vec3	normal, localNormal;
	vec3	color, refraction, reflection;
	vec2	coordOffset;
	float	d, fresnel;

	// Normalize the view vector
	toView = normalize(v_ViewVector);

	// Load the normal vector from the bump map
	localNormal = texture(u_BumpMap, v_TexCoord).rgb * 2.0 - 1.0;
	localNormal = normalize(localNormal);

	// Load the color from the color map and modulate by the color tint
	color = texture(u_ColorMap, v_TexCoord).rgb * u_ColorTint;

	// Load the refraction map with a modified texture coord
	coordOffset = localNormal.xy * u_DistortionScale.x;

	refraction = texture(u_RefractionMap, (gl_FragCoord.xy * u_CoordScaleAndBias.xy + u_CoordScaleAndBias.zw) + coordOffset).rgb;

	// Load the reflection map with a modified normal vector
	normal = vec3(localNormal.xy * u_DistortionScale.y, localNormal.z);
	normal = normalize(normal);

	reflection = texture(u_ReflectionMap, reflect(-toView, normal) * v_LocalToGlobal).rgb;

	// Compute the fresnel term
	d = dot(toView, localNormal);

	fresnel = pow(max(d, 0.0), u_FresnelParms.x) * u_FresnelParms.y + u_FresnelParms.z;
	fresnel = clamp(fresnel, 0.0, 1.0);

	// Blend reflection and refraction using the fresnel term and modulate by the color
	gl_FragColor.rgb = mix(reflection, refraction, fresnel) * color;
}