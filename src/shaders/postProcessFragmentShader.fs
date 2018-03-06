varying vec2 vUv;
uniform vec2 iResolution;
uniform sampler2D u_inputTex;
uniform vec2 u_direction;

vec4 blur9(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
  vec4 color = vec4(0.0);
  vec2 off1 = vec2(1.3846153846) * direction;
  vec2 off2 = vec2(3.2307692308) * direction;
  color += texture2D(image, uv) * 0.2270270270;
  color += texture2D(image, uv + (off1 / resolution)) * 0.3162162162;
  color += texture2D(image, uv - (off1 / resolution)) * 0.3162162162;
  color += texture2D(image, uv + (off2 / resolution)) * 0.0702702703;
  color += texture2D(image, uv - (off2 / resolution)) * 0.0702702703;
  return color;
}


void main() {

  //vec3 image = texture2D(u_inputTex, vec2(vUv.x, vUv.y)).xyz;
  //gl_FragColor =  vec4(image, 1.0);
  //vec4 image = texture2D(u_tex, vUv);
  
  float vingette = -abs(vUv.x * 2.0 - 1.0) + 1.0;
  vingette = clamp(vingette *2.0,0.0,1.0); 
  vec4 blur = blur9(u_inputTex, vUv, vec2(60.0, 60.0), vec2(1.0, 0.0));
  gl_FragColor = blur * vingette;

}