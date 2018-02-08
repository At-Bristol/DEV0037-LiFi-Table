import dat from 'dat.gui';
import config from './config'

const createGui = (store) => {
  const gui = new dat.GUI();
  const state = store.getState()

  const params = Object.assign({},
    state,
    {
      speed: 0,
      clearLeds: () => fetch(`${config.renderer}stop` || 'http://localhost:3000/stop'),
      initLeds: () => console.log('init')
    }
  )

  gui.add(params, 'isConnectedToLifi').listen()
  gui.add(params, 'isConnectedToRenderer').listen()
  gui.add(params, 'speed', -5, 5);
  gui.add(params, 'clearLeds');
  gui.add(params, 'initLeds');

  return gui
}

export default createGui