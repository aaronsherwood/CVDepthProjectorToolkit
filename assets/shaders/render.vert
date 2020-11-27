#version 150

in vec4 ciPosition;
in vec4 ciColor;

uniform mat4 ciModelViewProjection;
uniform float time;
uniform vec2 resolution;
uniform bool useMouse;
uniform vec2 mouse;

out vec4 vColor;

void main()
{
    vec4 pos = ciPosition;
    
    if (useMouse){
        vec2 d = vec2(pos.x, pos.y) - mouse;
        float len =  sqrt(d.x*d.x + d.y*d.y);
        
        float s = sin(time);
        float spreadSpeed = .5;
        
        s*=50*spreadSpeed;
        s+=200*spreadSpeed;
        
        if( len < resolution.x+s && len > 0  ){
            //lets get the distance into 0-1 ranges
            float pct = len / (resolution.x+s);

            //flip it so the closer we are the greater the repulsion
            pct = 1.0 - pct;
            
            //normalize our repulsion vector
            d /= len;
            
            //apply the repulsion to our position
            vec2 p = pos.xy;
            float displace = pow(pct,5) * (20 + s);
            d*=displace;
            pos.xy+=d;
        }
    }
    
    gl_Position = ciModelViewProjection * pos;
    gl_PointSize = 20;
	vColor = ciColor;
}
