
in vec3						v_ViewVector;
in vec3						v_LightVector;
in vec4						v_BumpTexCoord;
in mat3						v_LocalToGlobal;
in float					v_Depth;

uniform vec3					u_SunColor;		// Passed in automatically
uniform vec4					u_CoordScaleAndBias;	// Passed in automatically
uniform float					u_FadeDepth;
uniform vec2					u_DistortionScale;
uniform vec3					u_ColorTint;
uniform vec3					u_FresnelParms;
uniform vec3					u_SpecularColor;
uniform vec2					u_SpecularParms;

uniform sampler2D				u_BumpMap;
uniform sampler2D				u_DepthMap;
uniform sampler2D				u_RefractionMap;
uniform samplerCube				u_ReflectionMap;


void main (){

	vec3	toView, toLight;
	vec3	localNormal;
	vec3	color, specular, refraction, reflection;
	vec3	normal, waveNormal[2];
	vec2	coordOffset;
	float	depth, fade;
	float	d, fresnel;

	// Normalize the view vector
	toView = normalize(v_ViewVector);

	// Normalize the light vector
	toLight = normalize(v_LightVector);

	// Load the normal vectors from the bump map
	waveNormal[0] = texture(u_BumpMap, v_BumpTexCoord.st).rgb * 2.0 - 1.0;
	waveNormal[1] = texture(u_BumpMap, v_BumpTexCoord.pq).rgb * 2.0 - 1.0;

	// Add the normal vectors
	localNormal = vec3((waveNormal[0].xy / waveNormal[0].z) + (waveNormal[1].xy / waveNormal[1].z), 1.0);
	localNormal = normalize(localNormal);

	// Load the depth from the depth map
	depth = texture(u_DepthMap, gl_FragCoord.xy * u_CoordScaleAndBias.xy + u_CoordScaleAndBias.zw).r;

	// Transform the depth into eye space
	depth = gl_ProjectionMatrix[3][2] / (-(depth * 2.0 - 1.0) - gl_ProjectionMatrix[2][2]);

	// Compute depth fade
	fade = clamp(v_Depth - depth, 0.0, u_FadeDepth) / u_FadeDepth;

	// Load the refraction map with a modified texture coord
	coordOffset = localNormal.xy * u_DistortionScale.x * fade;

	refraction = texture(u_RefractionMap, (gl_FragCoord.xy * u_CoordScaleAndBias.xy + u_CoordScaleAndBias.zw) + coordOffset).rgb;

	// Load the reflection map with a modified normal vector
	normal = vec3(localNormal.xy * u_DistortionScale.y * fade, localNormal.z);
	normal = normalize(normal);

	reflection = texture(u_ReflectionMap, reflect(-toView, normal) * v_LocalToGlobal).rgb;

	// Fade the reflection out
	reflection = mix(refraction, reflection, fade);

	// Compute the fresnel term
	d = dot(toView, localNormal);

	fresnel = pow(max(d, 0.0), u_FresnelParms.x) * u_FresnelParms.y + u_FresnelParms.z;
	fresnel = clamp(fresnel, 0.0, 1.0);

	// Blend reflection and refraction using the fresnel term and modulate by the color tint
	color = mix(reflection, refraction, fresnel) * mix(vec3(1.0), u_ColorTint, fade);

	// Compute specular light contribution
	d = dot(toView, reflect(-toLight, localNormal));

	specular = u_SunColor * u_SpecularColor * pow(max(d, 0.0), u_SpecularParms.x) * u_SpecularParms.y;

	// Add color and specular
	gl_FragColor.rgb = color + specular * fade;
}