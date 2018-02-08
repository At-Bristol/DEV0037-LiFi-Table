import * as THREE from 'three';
import OrbitControls from './OrbitControls';

const createDefaultScene = ({
  domElement,
  cameraDistance = 500,
  cameraElevation = 45,
}) => {
  if(!domElement) throw Error('domElement is required')

  const scene = new THREE.Scene()
  const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 1, 10000)
  camera.position.z = cameraDistance
  camera.position.y = cameraDistance * Math.cos( cameraElevation )
  const controls = new THREE.OrbitControls(camera, domElement)
  controls.enableDamping = true
  controls.dampingFactor = 0.7
  controls.enableZoom = true

  return {
    scene,
    camera
  }
}

export default createDefaultScene