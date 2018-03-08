import simFragmentShader from '../shaders/simFragmentShader.fs'
import renderToTexture from '../lib/renderToTexture'
import * as THREE from 'three'
import img from '../img/3.png' 

const {tableWidth,tableLength, uRes, vRes, background,ledSize } = store.getState()

const particleTexture = renderToTexture({
  uniforms: {
    u_time: {type: "f", value: 0.0},
    u_ledsU: {type: "i", value: uRes},
    u_ledsV: {type: "i", value: vRes},
    u_scaleU: {type: "f", value: 1.0},
    u_scaleV: {type: "f", value: 1.0},
    u_isConnected: {type: "i", value: 1},
    u_color1Start: {type: "v3", value: new THREE.Vector3(0,0,0)},
    u_color1Variation: {type: "f", value: 0.5},
    u_color1Speed: {type: "f", value: 1.0},
    u_color2Start: {type: "v3", value: new THREE.Vector3(0,0,0)},
    u_color2Variation: {type: "f", value: 0.5},
    u_color2Speed: {type: "f", value: 1.0},
    u_speed: {type: "f", value: 1.0},
    u_reverse: {type: "i", value: 1},
    u_tex: {type: "t", value: new THREE.TextureLoader().load(img) }
  },
  fragmentShader: simFragmentShader,
  uRes: store.getState().uRes,
  vRes: store.getState().vRes,
});

export default particleTexture