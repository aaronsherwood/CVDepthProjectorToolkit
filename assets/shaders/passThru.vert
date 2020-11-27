uniform mat4	ciModelViewProjection;
in vec4			ciPosition;
in vec4         ciColor;
in vec2         ciTexCoord0;
out vec2        tc;
out vec4        vColor;
out vec4        vVertex;


void main()
{
    vVertex			= ciPosition;
    tc              = ciTexCoord0;
    vColor          = ciColor;
    gl_Position		= ciModelViewProjection * ciPosition;
}
