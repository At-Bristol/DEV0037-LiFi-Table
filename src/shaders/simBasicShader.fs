precision highp float;
varying vec2 vUv;
uniform sampler2D fftArray;
uniform sampler2D dataTex;

void main(void) {

  //float posSample = fftArray;

  //vec4 posSample  = texture2D(fftArray, vec2(vUv.s - 0.5, vUv.t + 0.5));
  vec4 posSample2 = texture2D(fftArray, vUv);

  //posSample = posSample + posSample2;
  //posSample2 = fftArray;

  gl_FragColor = vec4(0.0,0.0,0.0,0.0);

  // gl_FragColor = posSample2; // enable for texture preview
}
