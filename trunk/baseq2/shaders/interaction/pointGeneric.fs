
in vec3						v_ViewVector;
in vec3						v_LightVector;
in vec2						v_BumpTexCoord;
in vec2						v_DiffuseTexCoord;
in vec2						v_SpecularTexCoord;
in vec4						v_LightTexCoord;
in vec3						v_VertexColor;

uniform vec3					u_DiffuseColor;
uniform vec3					u_SpecularColor;
uniform vec2					u_SpecularParms;
uniform vec3					u_LightColor;

uniform sampler2D				u_BumpMap;
uniform sampler2D				u_DiffuseMap;
uniform sampler2D				u_SpecularMap;
uniform sampler2D				u_LightProjectionMap;
uniform sampler2D				u_LightFalloffMap;


void main (){

	vec3	toView, toLight;
	vec3	localNormal;
	vec3	diffuse, specular, light;
	float	exponent;
	float	d;

	// Normalize the view vector
	toView = normalize(v_ViewVector);

	// Normalize the light vector
	toLight = normalize(v_LightVector);

	// Load the normal vector from the bump map
	localNormal = texture(u_BumpMap, v_BumpTexCoord).rgb * 2.0 - 1.0;
	localNormal = normalize(localNormal);

	// Modulate the diffuse map by the diffuse color
	diffuse = texture(u_DiffuseMap, v_DiffuseTexCoord).rgb * u_DiffuseColor;

	// Modulate the specular map by the specular color
	specular = texture(u_SpecularMap, v_SpecularTexCoord).rgb * u_SpecularColor;

	// Load the specular exponent from the specular map
	exponent = texture(u_SpecularMap, v_SpecularTexCoord).a;

	// Compute light attenuation
	light = textureProj(u_LightProjectionMap, v_LightTexCoord.stq).rgb * texture(u_LightFalloffMap, vec2(v_LightTexCoord.p, 0.5)).rgb * u_LightColor;

	// Compute diffuse light contribution
	d = dot(toLight, localNormal);

	light *= max(d, 0.0);

	// Compute specular light contribution
	if (exponent == 0.0)
		specular *= u_SpecularParms.y;
	else {
		d = dot(toView, reflect(-toLight, localNormal));

		specular *= pow(max(d, 0.0), exponent * u_SpecularParms.x) * u_SpecularParms.y;
	}

	// Compute lighting and modulate by the vertex color
	gl_FragColor.rgb = (diffuse + specular) * light * v_VertexColor;
}