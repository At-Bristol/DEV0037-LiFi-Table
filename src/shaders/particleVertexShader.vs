
#ifdef GL_ES
precision highp float;
#endif

attribute float alpha;
varying float vHres;

uniform sampler2D tPos;
uniform sampler2D dataTex;

uniform float frameNo;
uniform float hRes;
uniform float ampl;

varying vec2 vUvm;
varying vec2 vUv;

void main() {

float uvOffset = (frameNo+2.0)/hRes;

vUv = uv;
vHres = hRes;
    	
vUvm = vec2(uv.s, uv.t + uvOffset);

vec4 posSample = texture2D(tPos, vec2(uv.s, vUvm.g));

  //vUv = uv;

  //vAlpha = alpha;
  float newPosition = position.z + (alpha * 0.3);

  vec4 mvPosition = modelViewMatrix * vec4( position.x, position.y, posSample.g * 100.0 * ampl, 1.0 );
  gl_PointSize = 50.0;
  gl_Position = projectionMatrix * mvPosition;

}