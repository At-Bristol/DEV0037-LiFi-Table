var ws281x = require('rpi-ws281x-native');

const createRenderer = () => {

  var NUM_LEDS = parseInt(process.argv[2], 10) || 5*8,
    pixelData = new Uint32Array(NUM_LEDS);

    for (var i = 0; i < NUM_LEDS; i++) {
      pixelData[i] = rgb2Int(255,0,0);
    }

  ws281x.init(NUM_LEDS);


  // ---- animation-loop
  var offset = 0;


  console.log('Press <ctrl>+C to exit.');

  // rainbow-colors, taken from http://goo.gl/Cs3H0v
  function colorwheel(pos) {
    pos = 255 - pos;
    if (pos < 85) { return rgb2Int(255 - pos * 3, 0, pos * 3); }
    else if (pos < 170) { pos -= 85; return rgb2Int(0, pos * 3, 255 - pos * 3); }
    else { pos -= 170; return rgb2Int(pos * 3, 255 - pos * 3, 0); }
  }

  function rgb2Int(r, g, b) {
    return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
  }


  /*const init = (
    NUM_LEDS = 5*8,
    cb
  ) => {
    pixelData = new Uint32Array(NUM_LEDS)
    ws281x.init(NUM_LEDS)
    console.log('Renderer Initialised')
    cb
  }*
  init()*/

  const stop = cb => {
    ws281x.reset()
    console.log('renderer stopped')
    cb
  }
  
  const render = data => {
      for (var i = 0; i < NUM_LEDS; i++) {
        pixelData[i] = data[i] || 0;
      }
  
      offset = (offset + 1) % 256;
      ws281x.render(pixelData);
  }

  setInterval(function () {
    ws281x.render(pixelData);
  }, 1000 / 30);

  return {
    //init,
    render,
    stop
  }
}

module.exports = createRenderer