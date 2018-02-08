const createStore = () => {
  let state = {
    isConnectedToLifi: false,
    isConnectedToRenderer: false
  }

  let subscribers = []
  
  const getState = () => state
 
  const dispatch = x => {
    state = Object.assign({}, state, x )
    render()
  }

  const render = () => subscribers.map(fn => fn())

  const subscribe = fn => {
    subscribers = [...subscribers, fn]
  }

  return {
    getState,
    dispatch,
    subscribe
  }
}

export default createStore