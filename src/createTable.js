import { setInterval } from "timers";

const createTable = (props) => {
  const {uRes, vRes} = props
  let NUM_LEDS, pixelData, ws281x, ws;

  const rgb2Int = (r, g, b) => (
    ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff)
  );

  const init = () => {
    const NUM_LEDS = 36 * 98
    ws = new WebSocket(`ws://${config.renderer}/render`);
    
    ws.onmessage = e => console.log(e)
    
    ws.onopen = e => {
      console.log('Connected to renderer')
    }

    ws.onclose = e =>  {
      console.warn('Connection to renderer closed, Attempting to restablish connection')
      //reconnect()
    }

    ws.onerror = e => {
      console.error('Error : ' + e)
      return undefined
    }
  }
  init();

  const reconnect = e => {
    ws = null
    //store.dispatch({msg: 'Attempting to connect to renderer'})
    setTimeout(e => init(), 1000)
  }

  const render = data => { 
    if(ws && ws.readyState === 1){
      let holdingData = []
      let i
      for(i = 0; i < data.length/4; i++) {
        let offset = i*4
        holdingData[i] = rgb2Int(
          data[offset],
          data[offset + 1],
          data[offset + 2],
        )
      }
     
      ws.send(holdingData)
      console.log('frame sent') 
    }
  }

  return {
    render
  }
}

export default createTable