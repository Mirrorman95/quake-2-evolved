
in vec4						v_LightTexCoord;

uniform vec4					u_LightColor;

uniform sampler2D				u_LightProjectionMap;
uniform sampler2D				u_LightFalloffMap;


void main (){

	// Compute blend light attenuation
	gl_FragColor = textureProj(u_LightProjectionMap, v_LightTexCoord.stq) * texture(u_LightFalloffMap, vec2(v_LightTexCoord.p, 0.5)) * u_LightColor;
}