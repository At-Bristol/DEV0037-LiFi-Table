import postProcessFragmentShader from '../shaders/postProcessFragmentShader.fs'
import renderToTexture from '../lib/renderToTexture'
import * as THREE from 'three'
import img from '../img/3.png' 

const createPostProcessTexture = inputTexture => ( 
  renderToTexture({
    uniforms: {
      u_blur: {type: "f", value: 0.0},
      u_inputTex: {type: "t", value: inputTexture.texture},
      u_direction: {type: "f", value: 1.0 } 
    },
    fragmentShader: postProcessFragmentShader,
    uRes: store.getState().uRes,
    vRes: store.getState().vRes,
  })
);

export default createPostProcessTexture