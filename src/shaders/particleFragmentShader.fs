#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D tPos;
uniform sampler2D dataTex;

uniform vec3 color;
varying float vHres;

//varying float vAlpha;
varying vec3 vColor;
varying vec2 vUv;
varying vec2 vUvm;

void main() {

  //colour for surface
  vec4 posSample = texture2D(tPos, vUvm);

  vec4 finalColour = vec4(0.0, 0.0, 0.0, 0.0);

  finalColour.r = vUvm.t * (posSample.r*5.0);
  finalColour.g = 0.5;//(1.0/sin(vUvm.s))-0.0;
  finalColour.b = posSample.r;
  //finalColour.a = (0.2*posSample.r)+0.01;

  float finalLuma = (max((1.0 - vUv.t) * - 1.0 + (20.0/vHres),0.0))*3.0;
    	 							
  gl_FragColor = vec4(1.0,1.0,1.0,1.0);
}
