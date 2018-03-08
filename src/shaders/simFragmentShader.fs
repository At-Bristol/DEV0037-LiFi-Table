#define M_PI 3.1415926535897932384626433832795

#ifdef GL_ES
precision mediump float;
#endif

uniform int u_ledsV;
uniform int u_ledsU;
uniform int u_reverse;
uniform float u_scaleU;
uniform float u_scaleV;
uniform float u_time;
uniform float u_isConnected;
uniform float u_speed;
uniform vec3 u_color1Start;
uniform float u_color1Variation;
uniform float u_color1Speed;
uniform vec3 u_color2Start;
uniform float u_color2Variation;
uniform float u_color2Speed;
uniform sampler2D u_tex;

varying vec2 vUv;

float random (in float x) {
    return fract(sin(x)*1e4);
}

float random (in vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))* 43758.5453123);
}

float pattern(vec2 st, vec2 v) {
    vec2 p = floor(st+v);
    return step(0.25, random(p.x)*0.5 );
}

vec3 rgb2hsv(vec3 c){
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}


float oscillate(float c1, float variation, float time){
    float offset = sin(time)*variation;
    return offset + c1;
}


vec3 cycleBetweenColors(vec3 c1, float variation, float time){
    vec3 c1Hsv = rgb2hsv(c1);
    float hueCycle = oscillate(c1Hsv.r, variation, time);
    vec3 outputColor = hsv2rgb(vec3(hueCycle, 1.0 , 1.0));
    return outputColor;
}

float invert(float i){
    return (i * -1.0) + 1.0;
}

void main() {
    vec2 u_resolution = vec2(10000.0,10000.0);
    vec2 st = vUv;
    //st.x *= ((u_scaleU/10.0) * float(-u_reverse));
    //st.y *= u_scaleV;  

    vec2 grid = vec2((1.0/u_scaleV) * float(u_ledsV), (1.0/u_scaleU) * float(u_ledsU));
    st *= grid;

    vec2 ipos = floor(st);  // integer
    vec2 fpos = fract(st);  // fraction

    vec2 vel = vec2((u_time*(max(grid.x,grid.y)/10.0))+0.5); // time
    vel *= vec2(-1.,0.0) * (random(1.0+ipos.y) + vec2(0.2,0.0)); // direction
    vel *= (u_speed)*0.05 * float(-u_reverse);

    vec3 mask = vec3(0.);
    mask.r = pattern(st,vel);
    mask.g = pattern(st,vel);
    mask.b = pattern(st,vel);

    vec2 st2 = st;
    st2.x /= 1000000.0;
    vec2 ipos2 = floor(st2);
    float rows =  random(ipos2);
    float irows = 1.0 - rows;

    float rowsMask = rows;
    float iRowsMask = invert(rowsMask);
    
    //color.r = (sin(0.6 * u_time + rows)*0.5);
    //color.b = 0.0;
    //color.g = (sin(u_time * rows)*2.0)*0.5;

    vec3 color = cycleBetweenColors(u_color1Start, u_color1Variation, (u_time * u_color1Speed/50.0 + 0.5));
    vec3 iColor = cycleBetweenColors(u_color2Start, u_color2Variation, (u_time * u_color2Speed/50.0));

    //iColor.r = (sin(0.6 * u_time + rows)*0.5); 
    //iColor.b = (sin(u_time * rows)*2.0)*0.5;
    //iColor.g = 0.0;

    color = color * rowsMask;
    iColor = iColor * iRowsMask;

    vec3 maskedColor = vec3(color * rowsMask);
    vec3 iMaskedColor = vec3(iColor * iRowsMask);

    //gl_FragColor = vec4(image - mask, 1.0); 
    gl_FragColor = vec4((color + iColor) - mask, 1.0); 
      
}