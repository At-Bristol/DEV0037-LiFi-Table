import * as THREE from 'three';

const array2Color = (array) => {

  const normailse = (arr) => arr.map(e => e/255)

  if (array.length === 3){
    const nArr = normailse(array)
    return new THREE.Vector3(nArr[0], nArr[1], nArr[2])
  }

  if (array.length === 4){
    
    return new THREE.Vector4(nArr[0], nArr[1], nArr[2], nArr[3])
  }

  throw new Error('Input array must be of length 3 or 4')
}

export default array2Color