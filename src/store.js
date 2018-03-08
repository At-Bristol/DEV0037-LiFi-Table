
const createStore = (middleware = []) => {
  let state = {
    inputTexture: '',
    tableWidth: 300,
    tableLength: 600,
    ledSize: 5.0,
    isLifiConnected: false,
    isRendererConnected: false,
    msg:'',
    speed: config.defaults.speed || 10,
    uRes: config.uRes,
    vRes: config.vRes,
    scaleU: 1,
    scaleV: 1,
    color1Start: config.defaults.color1Start || [0,0,255],
    color1Variation: config.defaults.color1Variation || 0.17,
    color1Speed: config.defaults.color1speed || 5,
    color1Direction: true,
    color2Start: config.defaults.color2Start || [0,200,255],
    color2Variation: config.defaults.color2Variation || 0.25,
    color2Speed: config.defaults.color2speed || 2,
    color2Direction:true,
    reverse: config.defaults.reverse|| false,
    lifiServer: 'localhost:3001',
    renderer: 'localhost:3000' 
  }

  let subscribers = []
  
  const getState = () => state
 
  const dispatch = x => {
    const prevState = state
    state = Object.assign({}, state, x )
    render()
    middleware.map(fn => fn(state, prevState))
  }

  const render = () => {
    subscribers.map(fn => fn())
  }

  const subscribe = (fn, shouldExecute) => {
    subscribers = [...subscribers, fn]
    if(shouldExecute) return fn()
  }

  return {
    getState,
    dispatch,
    subscribe
  }
}

const logger = (state, prevState) => {
  console.log("%cPrevious State: ","color:red; font-weight:bold")
  console.log(prevState)
  console.log("%cNext State: ","color:green; font-weight:bold")
  console.log(state)
} 

export const store = createStore()

export default store