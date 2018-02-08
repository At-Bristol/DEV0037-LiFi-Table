precision highp float;	
varying vec2 vUv;

uniform sampler2D particleTexture;

void main() {
  vec4 color = texture2D(particleTexture, vUv);
	gl_FragColor = vec4(color);
}