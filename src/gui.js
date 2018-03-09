import dat from 'dat.gui';
import stop from './api/stop'
import init from './api/init'

let params = Object.assign(
  store.getState(),
  {
    stop: () => stop(),
    init: () => init()
  }
)

const updateParams = () => {
  const state = store.getState()
  Object.keys(state).map(e => {
    try {
      params[e] = state[e]
    }catch(e){}
  })
}

store.subscribe(updateParams)

const gui = new dat.GUI();
const s = gui.addFolder('Render')
s.add(params, 'scaleU',0,10)
s.add(params, 'scaleV',0,10)
s.add(params, 'speed',0,100)
s.add(params, 'reverse')
const c1 = gui.addFolder('Color One')
c1.addColor(params, 'color1Start')
c1.add(params, 'color1Variation',0,1)
c1.add(params, 'color1Speed',0,100)
const c2 = gui.addFolder('Color Two')
c2.addColor(params, 'color2Start')
c2.add(params, 'color2Variation',0,1)
c2.add(params, 'color2Speed',0, 100)
const ctl = gui.addFolder('Control') 
ctl.add(params, 'stop')
ctl.add(params, 'init')
gui.add(params, 'msg').listen()

s.open()
c1.open()
c2.open()
ctl.open()