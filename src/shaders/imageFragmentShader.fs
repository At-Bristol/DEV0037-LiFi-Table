precision highp float;	
varying vec2 vUv;

uniform sampler2D u_tex;

void main() {
	 vec4 image = texture2D(u_tex, vUv);
   float vingette = abs(vUv.x * 2.0 - 1.0) 
	 gl_FragColor = vec4(1.0,1.0,1.0,1.0);
}