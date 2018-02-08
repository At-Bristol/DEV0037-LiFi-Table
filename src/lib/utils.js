export const generateData = (x,y) => {
  let val = 0
  return new Uint8Array( x * y * 3).map((e,i) => {
    if(i%3 === 0) val = Math.floor(Math.random()*2)*255
    return val
  });
}

export const generateEmptyDataTexture = (sizeX,sizeY) => {
  const size = sizeX * sizeY;
  let data = generateData(sizeX, sizeY);
  let dataTexture = new THREE.DataTexture( data, sizeX, sizeY, THREE.RGBFormat, THREE.UnsignedByteType);
  dataTexture.wrapT = THREE.RepeatWrapping;

  return dataTexture;		
};

