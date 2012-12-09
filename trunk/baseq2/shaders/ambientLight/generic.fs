
in vec2						v_BumpTexCoord;
in vec2						v_DiffuseTexCoord;
in vec4						v_LightTexCoord;
in vec3						v_VertexColor;

uniform vec3					u_DiffuseColor;
uniform vec3					u_LightColor;

uniform sampler2D				u_BumpMap;
uniform sampler2D				u_DiffuseMap;
uniform sampler2D				u_LightProjectionMap;
uniform sampler2D				u_LightFalloffMap;


void main (){

	vec3	localNormal;
	vec3	diffuse, light;

	// Load the normal vector from the bump map
	localNormal = texture(u_BumpMap, v_BumpTexCoord).rgb * 2.0 - 1.0;
	localNormal = normalize(localNormal);

	// Modulate the diffuse map by the diffuse color
	diffuse = texture(u_DiffuseMap, v_DiffuseTexCoord).rgb * u_DiffuseColor;

	// Compute light attenuation
	light = textureProj(u_LightProjectionMap, v_LightTexCoord.stq).rgb * texture(u_LightFalloffMap, vec2(v_LightTexCoord.p, 0.5)).rgb * u_LightColor;

	// Compute ambient light contribution
	light *= (localNormal.z * localNormal.z);

	// Compute lighting and modulate by the vertex color
	gl_FragColor.rgb = diffuse * light * v_VertexColor;
}