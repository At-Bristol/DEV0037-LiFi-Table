import * as THREE from 'three';
import vertexShader from '../shaders/basicVertexShader.vs';

const renderToTexture = ({
  uniforms={},  
  fragmentShader = {},
  width = 300,  
  height = 300,
  params = {
    minFilter:THREE.LinearFilter,
    stencilBuffer:false,
    depthBuffer:false
  }  
}) => {
  if(!fragmentShader) throw new Error('fragShader property must be defined');

  const texture = new THREE.WebGLRenderTarget(width, height, params)
  const scene = new THREE.Scene()
  const camera = new THREE.OrthographicCamera( 
    width / - 2,
    width / 2,
    height / 2,
    height / - 2, 
    -10000, 
    10000
  )
  const material =  new THREE.ShaderMaterial({
    uniforms,
    vertexShader,
    fragmentShader,
    transparent: false,
  });

  const geo  = new THREE.PlaneBufferGeometry(width, height);
  const mesh = new THREE.Mesh(geo, material);
  scene.add(mesh)
  
  return {
    camera,
    scene,
    texture,
    uniforms
  }
}

export default renderToTexture
