import simFragmentShader from '../shaders/simFragmentShader.fs'
import renderToTexture from '../lib/renderToTexture'
import * as THREE from 'three'
import img from '../img/3.png' 

const {uRes, vRes } = store.getState()

const particleTexture = renderToTexture({
  uniforms: {
    u_time: {type: "f", value: 0.0},
    u_ledsU: {type: "i", value: uRes},
    u_ledsV: {type: "i", value: vRes},
    u_scaleU: {type: "f", value: 1.0},
    u_scaleV: {type: "f", value: 1.0},
    u_isConnected: {type: "i", value: 1},
    u_color1Start: {type: "v3", value: new THREE.Vector3(0,0,0), map: 'color1Start'},
    u_color1Variation: {type: "f", value: 0.5, map: 'color1Variation'},
    u_color1Speed: {type: "f", value: 1.0, map: 'color1Speed'},
    u_color2Start: {type: "v3", value: new THREE.Vector3(0,0,0), map: 'color2Speed'},
    u_color2Variation: {type: "f", value: 0.5, map: 'color2Variation'},
    u_color2Speed: {type: "f", value: 1.0, map: 'color2Speed'},
    u_speed: {type: "f", value: 1.0, map: 'speed'},
    u_reverse: {type: "i", value: 1, map: 'reverse'},
    u_tex: {type: "t", value: new THREE.TextureLoader().load(img) }
  },
  fragmentShader: simFragmentShader,
  updateFn: store.getState,
  uRes,
  vRes
});

export default particleTexture