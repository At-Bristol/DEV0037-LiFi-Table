import style from './css/main.css'

import * as THREE from 'three'
import OrbitControls from './lib/OrbitControls'

import particleTexture from './textures/particleTexture'
import wolfensteinTexture from './textures/wolfensteinTexture'

import createPostProcessTexture from './textures/postProcessTexture'
import particleShaderMaterial from './materials/particleShaderMaterial'
import createDefaultScene from './lib/createDefaultScene'
import array2Color from './lib/array2Color'
import createTable from './createTable'
import createGui from './gui'
import tableModel from './models/table.json'
import foyerEnv from './env/foyer/foyer.js'
import config from './config'


if(process.env.NODE_ENV !== 'production'){
  console.log('Not running in production mode')
}

const main = (props) => {

  const {tableWidth,tableLength, uRes, vRes, background,ledSize } = props

  const inputTexture = particleTexture

  const table = createTable(uRes, vRes)
  const clock = new THREE.Clock()
  const renderer = new THREE.WebGLRenderer()

  renderer.setSize(window.innerWidth, window.innerHeight)
  document.body.appendChild(renderer.domElement)

  const defaultScene = createDefaultScene({domElement: renderer.domElement})

  const postProcessTexture = createPostProcessTexture(inputTexture.texture)
  
  //preview plane
  const previewPlaneMaterial = new THREE.MeshBasicMaterial({map:postProcessTexture.texture.texture});
  const previewPlaneGeo = new THREE.PlaneBufferGeometry(tableLength, tableWidth, 1, 1);
  const previewPlane = new THREE.Mesh(previewPlaneGeo, previewPlaneMaterial);
  previewPlane.rotation.x = -90 * (3.14/180);
  
  // point particles geometry
  const particlePlaneGeo = new THREE.PlaneBufferGeometry(tableLength, tableWidth, uRes, vRes);
  const particles = new THREE.Points(
    particlePlaneGeo, 
    particleShaderMaterial({
      uniforms:{
        uColor1: { type: "v3", value: new THREE.Vector3(1.0, 1.0, 1.0) },
        colorTex: { type: "t", value: 1, texture: postProcessTexture.texture.texture },
        u_pointSize: {type: "f", value: ledSize},
      }
    })
  );
  
  particles.rotation.x = 3.14 / 2.0
  particles.rotation.y = 3.14
  particles.rotation.z = 3.14

  if(background) defaultScene.scene.background = foyerEnv
  defaultScene.scene.add(previewPlane)
  defaultScene.scene.add(particles)

  const outputTex = new Uint8Array( uRes * vRes * 4 )
  
  const render = () => {

    const state = store.getState()

    inputTexture.uniforms.u_isConnected.value = store.isConnected ? 1.0 : 1.0
    inputTexture.uniforms.u_time.value += clock.getDelta()
    inputTexture.uniforms.u_scaleU.value = state.scaleU
    inputTexture.uniforms.u_scaleV.value = state.scaleV
    inputTexture.uniforms.u_speed.value = state.speed
    inputTexture.uniforms.u_reverse.value = state.reverse ? 1 : -1
    inputTexture.uniforms.u_color1Start.value = array2Color(state.color1Start)
    inputTexture.uniforms.u_color1End.value = array2Color(state.color1End)
    inputTexture.uniforms.u_color1Speed.value = state.color1Speed;
    inputTexture.uniforms.u_color2Start.value = array2Color(state.color2Start)
    inputTexture.uniforms.u_color2End.value = array2Color(state.color2End)
    inputTexture.uniforms.u_color2Speed.value = state.color2Speed
    //render
    renderer.render(
      inputTexture.scene, 
      inputTexture.camera, 
      inputTexture.texture
    );

    renderer.render(
      postProcessTexture.scene, 
      postProcessTexture.camera, 
      postProcessTexture.texture
    );
    //render output to array
   renderer.readRenderTargetPixels(
      postProcessTexture.texture, 
      0, 0, uRes, vRes, 
      outputTex )
    table.render(outputTex)
    //renderToScreen
    renderer.render(
      defaultScene.scene, 
      defaultScene.camera,
    );
  };
  
  const loop = () => {
    requestAnimationFrame(loop);
    render();
  };

  loop();
};

main(store.getState())


