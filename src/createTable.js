import { setInterval } from "timers";
import store from './index';

const createTable = ({
  hRes = 8,
  vRes = 5
}) => {
  let NUM_LEDS, pixelData, ws281x, ws;

  const rgb2Int = (r, g, b) => (
    ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff)
  );

  const init = () => {
    const NUM_LEDS = hRes * vRes
    pixelData = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
    ws = new WebSocket("ws://10.72.0.250:3000/render");
    //ws.onmessage = e => console.log(e.data)
    ws.onopen = e => {
      store.dispatch({isConnectedToRenderer: true})
      console.log('Connected to renderer')
    }

    ws.onclose = e =>  {
      console.warn('Connection to renderer closed, Attempting to restablish connection')
      store.dispatch({isConnectedToRenderer: false})
      reconnect()
    }

    ws.onerror = e => console.log(e)
  }
  init();

  const reconnect = e => {
    ws = null
    setTimeout(e => init(), 1000)
  }

  const render = data => {
    if(ws && ws.readyState === 1){
      let holdingData = []
      let i
      for(i = 0; i < pixelData.length; i++) {
        let offset = i*4
        holdingData[i] = rgb2Int(
          data[offset],
          data[offset + 1],
          data[offset + 2],
        )
      }

      ws.send(pixelData.map(e => holdingData))
    }
  }

  return {
    render
  }
}

export default createTable