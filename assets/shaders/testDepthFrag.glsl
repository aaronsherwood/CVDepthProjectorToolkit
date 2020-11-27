#version 150

in float depth;
uniform float threshold;
uniform bool calibrated;
in float depths;
out vec4 outColor;
void main()
{
	if( depth > depths-threshold && calibrated) discard;
	outColor.rgb	= vec3( 1. );
	outColor.a		= 1.0;
}





