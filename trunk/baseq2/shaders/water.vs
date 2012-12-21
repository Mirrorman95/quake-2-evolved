
in vec3						va_Normal;
in vec3						va_Tangent1;
in vec3						va_Tangent2;
in vec4						va_TexCoord;

out vec3					v_ViewVector;
out vec3					v_LightVector;
out vec4					v_BumpTexCoord;
out mat3					v_LocalToGlobal;
out float					v_Depth;

uniform vec3					u_ViewOrigin;		// Passed in automatically
uniform mat3					u_EntityAxis;		// Passed in automatically
uniform vec3					u_SunDirection;		// Passed in automatically
uniform vec4					u_BumpScale;
uniform vec4					u_BumpScroll;


void main (){

	vec3	toView, toLight;

	// Position invariant
	gl_Position = ftransform();

#ifndef GLSL_ATI

	// Support clipping planes
	gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;

#endif

	// Compute the view vector in tangent space
	toView = u_ViewOrigin - gl_Vertex.xyz;

	v_ViewVector.x = dot(toView, va_Tangent1);
	v_ViewVector.y = dot(toView, va_Tangent2);
	v_ViewVector.z = dot(toView, va_Normal);

	// Compute the light vector in tangent space
	toLight = -u_SunDirection;

	v_LightVector.x = dot(toLight, va_Tangent1);
	v_LightVector.y = dot(toLight, va_Tangent2);
	v_LightVector.z = dot(toLight, va_Normal);

	// Scale and scroll the bump map texture coords
	v_BumpTexCoord.st = va_TexCoord.st * u_BumpScale.xy + u_BumpScroll.xy;
	v_BumpTexCoord.pq = va_TexCoord.st * u_BumpScale.zw + u_BumpScroll.zw;

	// Compute the local-to-global transformation matrix
	v_LocalToGlobal[0].x = dot(u_EntityAxis[0], va_Tangent1);
	v_LocalToGlobal[0].y = dot(u_EntityAxis[0], va_Tangent2);
	v_LocalToGlobal[0].z = dot(u_EntityAxis[0], va_Normal);

	v_LocalToGlobal[1].x = dot(u_EntityAxis[1], va_Tangent1);
	v_LocalToGlobal[1].y = dot(u_EntityAxis[1], va_Tangent2);
	v_LocalToGlobal[1].z = dot(u_EntityAxis[1], va_Normal);

	v_LocalToGlobal[2].x = dot(u_EntityAxis[2], va_Tangent1);
	v_LocalToGlobal[2].y = dot(u_EntityAxis[2], va_Tangent2);
	v_LocalToGlobal[2].z = dot(u_EntityAxis[2], va_Normal);

	// Compute the eye space depth
	v_Depth = dot(gl_Vertex, gl_ModelViewMatrixTranspose[2]);
}