uniform mat4	ciModelViewProjection;
in vec4			ciPosition;
in vec2         ciTexCoord0;
in vec4         ciColor;
out vec4        vColor;
out vec2        tc;

void main(){
    gl_Position = ciModelViewProjection * ciPosition;
    tc = ciTexCoord0;
    vColor = ciColor;
}
