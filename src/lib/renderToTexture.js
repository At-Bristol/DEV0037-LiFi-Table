import * as THREE from 'three';
import vertexShader from '../shaders/basicVertexShader.vs';

const renderToTexture = ({
  uniforms={},  
  fragmentShader={},
  uRes = 300,  
  vRes = 300,
  params = {
    magFilter:THREE.NearestFilter,
    minFilter:THREE.NearestFilter,
    stencilBuffer:false,
    depthBuffer:false
    
  }  
}) => {
  if(!fragmentShader) throw new Error('fragmentShader property must be defined');

  const texture = new THREE.WebGLRenderTarget(uRes, vRes, params)
  const scene = new THREE.Scene()
  const camera = new THREE.OrthographicCamera( 
    uRes / - 2,
    uRes / 2,
    vRes / 2,
    vRes / - 2, 
    -10000, 
    10000
  )
  const material =  new THREE.ShaderMaterial({
    uniforms,
    vertexShader,
    fragmentShader,
    transparent: false,
  });

  const geo  = new THREE.PlaneBufferGeometry(uRes, vRes);
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
