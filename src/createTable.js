import io from 'socket.io-client'

const createTable = (props) => {
  const {uRes, vRes} = props
  let NUM_LEDS, pixelData, ws281x
  const socket = io()

  const rgb2Int = (r, g, b) => (
    ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff)
  );

  const init = () => {
    const NUM_LEDS = 36 * 98

    socket.on('connect', connection => console.log('Server connected'))

    socket.on('error', err => console.log('Socket Error:', err))

    socket.on('connect_timeout', timeout => console.log('A connection timed out:' + timeout))

    socket.on('reconnecting', attemptNumber => console.log('Attempting to reconnect for the ' + attemptNumber + ' time'));

    socket.on('reconnect', attemptNumber => console.log('Successfully reconnected'))

    socket.on('reconnect_error', err => console.log('Error reonnecting: ' + err))

    socket.on('disconnect', reason => console.log('Disconnected because ' + reason))
  }
  init();

  const render = data => {

    let renderData = []
    let i
    for(i = 0; i < data.length/4; i++) {
      let offset = i*4
      renderData[i] = rgb2Int(
        data[offset],
        data[offset + 1],
        data[offset + 2],
      )
    }
    socket.emit('render', renderData)
  }

  return {
    render
  }
}

export default createTable