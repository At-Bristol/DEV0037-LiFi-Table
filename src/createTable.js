//import ws281x from 'rpi-ws281x-native';

const createTable = ({
  hRes = 8,
  vRes = 5
}) => {

  let NUM_LEDS, pixelData, ws281x, isRpi = false 

  const rgb2Int = (r, g, b) => (
    ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff)
  );

  const init = () => {
    const NUM_LEDS = hRes * vRes
    pixelData = new Uint32Array(NUM_LEDS)
    //if(ws281x) isRpi = true
    //if(isRpi) ws281x.reset()
    console.log('isRpi', isRpi)
    //if(isRpi) ws281x.init(NUM_LEDS)
    process.on('SIGINT', function () {
      console.log('reseting')
      //if(isRpi) ws281x.reset()
      process.nextTick(() => process.exit(0))
    });
  }

  init()

  const render = data => {
    //if(isRpi) ws281x.render(pixelData);
    console.log(pixelData.map(e => rgb2Int(200,200,200)))
  }

  return {
    render
  }
}

export default createTable