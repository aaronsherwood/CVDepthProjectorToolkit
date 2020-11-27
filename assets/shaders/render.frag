#version 150

in vec4 vColor;
out vec4 outColor;
uniform sampler2D ParticleTex;

void main()
{
    outColor = texture( ParticleTex, gl_PointCoord );  
}
