#version 150

uniform mat4    ciModelViewProjection;
in vec4         ciPosition;
in vec3        thresholds;
out float      depths;
out float depth;

void main()
{
	vec4 pos = ciModelViewProjection * ciPosition;
    depth = pos.z;
    vec4 transformed = ciModelViewProjection * vec4(thresholds,1.f) ;
    depths = transformed.z;
    gl_Position = pos;
}
