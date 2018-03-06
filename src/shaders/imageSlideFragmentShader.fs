precision highp float;	
varying vec2 vUv;

uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;
uniform float u_time;

void main() {
	 vec4 image1 = texture2D(iChannel1, vUv);
   vec4 image2 = texture2D(iChannel2, vUv);
   vec4 image3 = texture2D(iChannel2, vUv);
	 gl_FragColor = vec4(image1);
}