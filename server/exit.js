var ws281x = require('rpi-ws281x-native');

process.on('SIGINT', () => {
  console.log('closing')
  ws281x.reset();
  process.nextTick(() => process.exit())
})