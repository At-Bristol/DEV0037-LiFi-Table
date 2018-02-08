import * as THREE from 'three'

import nx from './nx.png'
import ny from './ny.png'
import nz from './nz.png'
import px from './px.png'
import py from './py.png'
import pz from './pz.png'

const envMap = new THREE.CubeTextureLoader()
  .load([
    px,nx,
    py,ny,
    pz,nz
  ])

export default envMap