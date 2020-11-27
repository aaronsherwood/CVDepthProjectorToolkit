#version 150 core

uniform vec3  uMousePos;
uniform vec3  uPreMousePos;
uniform sampler2D tex0;
uniform vec2 wh;
uniform bool useTexture;
uniform float time;
uniform float scale;

in vec2 ciTexCoord0;

in vec3   iLoc;
in vec3   iVel;

out vec3  loc;
out vec3  vel;

void main()
{
    loc = iLoc;
    vel = iVel;
    
    vec2 tc = ciTexCoord0;
    //not using for now, but here if you need it
    time;
    
    float dist = 20;
    if( (distance(uMousePos,loc)<dist && uMousePos!=uPreMousePos) || useTexture ){
        vec3 dir = vec3(0.);
        
        //dir of movement
        dir = uMousePos-uPreMousePos;
        dir=normalize(dir);
        
        if (useTexture){
            vec2 subtracted;
            dir = vec3(0.);
            //sample repos texture
            vec4 look = texture(tex0,vec2(loc.x, wh.y-loc.y)/wh);
            subtracted =look.yw-look.xz;
           
            vec2 offs = subtracted*scale;
            offs.x*=-1;
            dir = vec3(offs.xy,0.);
        }

        vec3 acc= vec3(0);
        acc+=dir;
        vel+=acc;
    }
    
    //dampen
    vel*=.98;
    loc+=vel;
    
    // bounds
    if (loc.x>wh.x || loc.x<0.)
        vel.x*=-1.;
    if (loc.y>wh.y || loc.y<0.)
        vel.y*=-1.;
}
