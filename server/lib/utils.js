// rainbow-colors, taken from http://goo.gl/Cs3H0v

const rgb2Int = (r, g, b) => {
  return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

const int2Rgb = int => {
    return [
      int >> 16,
      int >> 8,
      int 
    ]
}

const rgb2PixelArray = rgbArray => {
  let holdingData = []
  let i
  for(i = 0; i < rgbArray.length/4; i++) {
    let offset = i*4
    holdingData.push(rgb2Int(
      rgbArray[offset],
      rgbArray[offset + 1],
      rgbArray[offset + 2],
    ))
  }

  return holdingData
}

module.exports = {
  rgb2Int,
  int2Rgb,
  rgb2PixelArray
}