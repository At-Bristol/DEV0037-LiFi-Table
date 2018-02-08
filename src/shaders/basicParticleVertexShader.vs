precision highp float;	
varying vec2 vUv;
uniform float u_pointSize;

void main() {
	vUv = uv;
	gl_PointSize = u_pointSize;
	gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0 );
}