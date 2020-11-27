precision mediump float;
in vec2 tc;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform vec2 scale;
uniform vec2 offset;
out vec4 oColor;

void main()
{
    vec4 a = texture(tex0, tc);
    vec4 b = texture(tex1, tc);
    vec2 x1 = vec2(offset.x,0.);
    vec2 y1 = vec2(0.,offset.y);
    vec4 curdif = b-a;
    vec4 gradx = texture(tex1, tc+x1)-texture(tex1, tc-x1);
    gradx += texture(tex0, tc+x1)-texture(tex0, tc-x1);
    vec4 grady = texture(tex1, tc+y1)-texture(tex1, tc-y1);
    grady += texture(tex0, tc+y1)-texture(tex0, tc-y1);
    float gradmag1 = distance(gradx.r,grady.r);
    float gradmag = mix(gradmag1,0.0000000000001,float(gradmag1==0.));
    float vxd = curdif.x*(gradx.x);
    vec2 xout = vec2(max(vxd,0.),abs(min(vxd,0.)))*scale.x;
    float vyd = curdif.x*(grady.x);
    vec2 yout = vec2(max(vyd,0.),abs(min(vyd,0.)))*scale.y;
    vec4 pout = vec4(xout.xy,yout.xy);
    vec4 final = mix(pout,vec4(0.),float(gradmag1==0.));
    oColor = final;
}
