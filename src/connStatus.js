const createConnStatus = () => {
  let state = {
    isConnected: false
  }
  
  const getState = () => state
 
  const dispatch = x => {
    state = Object.assign(state, {isConnected: x} )
  }

  return {
    getState,
    dispatch
  }
}

export default createConnStatus