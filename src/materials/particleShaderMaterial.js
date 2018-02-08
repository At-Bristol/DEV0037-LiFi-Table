import * as THREE from 'three'
import vertexShader from '../shaders/basicParticleVertexShader.vs'
import fragmentShader from '../shaders/basicParticleFragmentShader.fs'


const particleShaderMaterial = ({
  uniforms = {},
}) => (
  new THREE.ShaderMaterial({
    uniforms,
    vertexShader,
    fragmentShader,
    transparent: false,
  })
)

export default particleShaderMaterial