import style from './css/main.css'

import * as THREE from 'three'
import OrbitControls from './lib/OrbitControls'
import renderToTexture from './lib/renderToTexture'
import simFragmentShader from './shaders/simFragmentShader.fs'
import particleShaderMaterial from './materials/particleShaderMaterial'
import createDefaultScene from './lib/createDefaultScene'
import startCheckingConnection from './lib/networkConnection'
import createStore from './createStore'
import createTable from './createTable'
import createGui from './gui'

import tableModel from './models/table.json'
import foyerEnv from './env/foyer/foyer.js'
import config from './config'

if(process.env.NODE_ENV !== 'production'){
  console.log('Not running in production mode')
}

let store = createStore()

const main = ({
  tableWidth = 300,
  tableLength = 600,
  hRes = 50,
  vRes = 50,
  background = false,
  ledSize = 2.0
}) => {

  const table = createTable(config.hRes, config.vRes)

  if(process.env.NETWORK === true) startCheckingConnection()

  const clock = new THREE.Clock()

  const renderer = new THREE.WebGLRenderer()
  renderer.setSize(window.innerWidth, window.innerHeight)
  document.body.appendChild(renderer.domElement)

  const particleTexture = renderToTexture({
    uniforms: {
      u_time: {type: "f", value: 0.0},
      u_width: {type: "i", value: hRes},
      u_height: {type: "i", value: vRes},
      u_isConnected: {type: "f", value: 1.0}
    },
    fragmentShader: simFragmentShader
  });

  const defaultScene = createDefaultScene({domElement: renderer.domElement})

  //preview plane
  const previewPlaneMaterial = new THREE.MeshBasicMaterial({map: particleTexture.texture.texture});
  //const previewPlaneGeo = new THREE.PlaneBufferGeometry(tableLength, tableWidth, 1, 1);
  const previewPlaneGeo = new THREE.PlaneBufferGeometry(tableLength, tableWidth, 1, 1);
  const previewPlane = new THREE.Mesh(previewPlaneGeo, previewPlaneMaterial);
  previewPlane.rotation.x = -90 * (3.14/180);
  // point particles geometry

  const particlePlaneGeo = new THREE.PlaneBufferGeometry(tableLength, tableWidth, hRes, vRes);
  const particles = new THREE.Points(
    particlePlaneGeo, 
    particleShaderMaterial({
      uniforms:{
        uColor1: { type: "v3", value: new THREE.Vector3(1.0, 1.0, 1.0) },
        colorTex: { type: "t", value: particleTexture },
        u_pointSize: {type: "f", value: config.ledSize},
      }
    })
  );
  
  particles.rotation.x = 3.14 / 2.0
  particles.rotation.y = 3.14
  particles.rotation.z = 3.14

  if(config.background) defaultScene.scene.background = foyerEnv
  defaultScene.scene.add(previewPlane)
  defaultScene.scene.add(particles)

  const outputTex = new Uint8Array( hRes * vRes * 4 )
  
  const render = () => {
    //fftTex.needsUpdate = true
    //fftTex.image.data = generateData(1, 64)
    //update
    particleTexture.uniforms.u_isConnected.value = store.getState().isConnected ? 1.0 : 1.0; 
    particleTexture.uniforms.u_time.value += clock.getDelta(); 
    //render
    renderer.render(
      particleTexture.scene, 
      particleTexture.camera, 
      particleTexture.texture
    );
    //render output to array
   renderer.readRenderTargetPixels(
      particleTexture.texture, 
      0, 0, hRes, vRes, 
      outputTex )
    table.render(outputTex)
    //renderToScreen
    renderer.render(
      defaultScene.scene, 
      defaultScene.camera
    );
  };
  
  const loop = () => {
    requestAnimationFrame(loop);
    render();
  };

  loop();

  //createGui(store)
};

main(config)

export default store

