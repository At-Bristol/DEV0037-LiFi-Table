const ws281x = require('rpi-ws281x-native');
const utils = require('./lib/utils');
const createSurfaceMap = require('./lib/mapping')
const createRainbow = require('./shaders/rainbow')
const config = {
  clamp:100
}

let pixelData
let isStopped = false
let map

const createRenderer = () => {

  var NUM_LEDS = parseInt(process.argv[2], 10) || 36*98,
    pixelData = new Uint32Array(NUM_LEDS);

  let rainbow;

  //var offset = 0;

  const init = (body,cb) => {
    ws281x.init(NUM_LEDS)
    ws281x.setBrightness(120)
    rainbow = createRainbow(NUM_LEDS, 10)
    map = createSurfaceMap()
    isStopped = false
    console.log('renderer initalized')

    cb
  }

  const stop = cb => {
    isStopped = true;
    ws281x.reset()
    console.log('renderer isStopped')
    cb
  }
  
  const render = data => {
      pixelData = data
  }

  var i = 0, prevData = ''

  setInterval(() => {
    if(pixelData[0] === prevData){
      process.stdout.write('render frame '+ i +'\r')
    }else{
      process.stdout.write(`render frame ${i} updated\r`)
    }

    prevData = pixelData[0]

    if(!isStopped){
      ws281x.render(map.apply(pixelData));
    }
    i++
  }, 1000 / 15);

  init()

  return {
    init,
    render,
    stop
  }
}

module.exports = createRenderer