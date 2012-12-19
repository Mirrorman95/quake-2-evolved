
in vec3						va_Normal;
in vec3						va_Tangent1;
in vec3						va_Tangent2;
in vec4						va_TexCoord;

out vec3					v_ViewVector;
out vec2					v_TexCoord;
out mat3					v_LocalToGlobal;

uniform vec3					u_ViewOrigin;		// Passed in automatically
uniform mat3					u_EntityAxis;		// Passed in automatically


void main (){

	vec3	toView;

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

	// Copy the texture coord
	v_TexCoord = va_TexCoord.st;

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
}