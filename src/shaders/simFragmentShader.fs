#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;
uniform int u_width;
uniform int u_height;
uniform float u_isConnected;

varying vec2 vUv;

float random (in float x) {
    return fract(sin(x)*1e4);
}

float random (in vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))* 43758.5453123);
}

float pattern(vec2 st, vec2 v, float t) {
    vec2 p = floor(st+v);
    return step(0.25, random(100.+p*.000001)+random(p.x)*0.5 );
}

void main() {
    vec2 u_resolution = vec2(u_height,u_width);
    vec2 st = vUv;
    st.x *= 0.2;

    vec2 grid = vec2(100.,float(u_width));
    st *= grid;

    vec2 ipos = floor(st);  // integer
    vec2 fpos = fract(st);  // fraction

    vec2 vel = vec2((u_time*(max(grid.x,grid.y)/10.0))+0.5); // time
    vel *= vec2(-1.,0.0) * (random(1.0+ipos.y) + vec2(0.2,0.0)); // direction
    vel *= 0.2;

    vec3 mask = vec3(0.);
    mask.r = pattern(st,vel,0.5+0.5/u_resolution.x);
    mask.g = pattern(st,vel,0.5+0.5/u_resolution.x);
    mask.b = pattern(st,vel,0.5+0.5/u_resolution.x);

    vec2 st2 = vUv;
    st2.y *= float(u_width);
    vec2 ipos2 = floor(st2);
    float rows =  random(ipos2);
    float irows = 1.0 - rows;

    float rowsMask = floor(rows * 2.0);
    float iRowsMask = floor(irows * 2.0);

    vec3 color = vec3(1.0);
    vec3 iColor = vec3(1.0);
    
    color.r = (sin(0.6 * u_time + rows)*0.5);
    color.b = 0.0;
    color.g = (sin(u_time * rows)*2.0)*0.5;

    iColor.r = (sin(0.6 * u_time + rows)*0.5); 
    iColor.b = (sin(u_time * rows)*2.0)*0.5;
    iColor.g = 0.0;

    vec3 maskedColor = vec3(color * rowsMask);
    vec3 iMaskedColor = vec3(iColor * iRowsMask);

    float fade = u_time;

    fade = u_time * u_isConnected;
 
    gl_FragColor = vec4((maskedColor + iMaskedColor) - mask,1.0);

}