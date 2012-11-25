
in vec4						v_LightTexCoord;

uniform vec4					u_LightColor;

uniform sampler2D				u_LightProjectionMap;
uniform sampler2D				u_LightFalloffMap;


void main (){

	// Compute fog light attenuation
	gl_FragColor = texture(u_LightProjectionMap, v_LightTexCoord.st) * texture(u_LightFalloffMap, v_LightTexCoord.pq) * u_LightColor;
}