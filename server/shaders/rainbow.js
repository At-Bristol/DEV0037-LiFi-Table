const utils = require('../lib/utils')
const { rgb2Int } = utils

const createRainbow = (numLeds, speed = 1) => {
  let offset = 0
  let pixelData = new Uint32Array(numLeds);
  let init = 0


  const colorWheel = (pos) => {
    pos = 255 - pos;
    if (pos < 85) { return rgb2Int(255 - pos * 3, 0, pos * 3); }
    else if (pos < 170) { pos -= 85; return rgb2Int(0, pos * 3, 255 - pos * 3); }
    else { pos -= 170; return rgb2Int(pos * 3, 255 - pos * 3, 0); }
  }
  

  const update = () => {
    if(init < 100){
      offset = (offset + (1*speed)) % 256;
      init += 1
      return pixelData.map((e, i) => 
        colorWheel((offset + i) % 256)
      )
    }

  }

  return {
    update
  }
}

module.exports = createRainbow