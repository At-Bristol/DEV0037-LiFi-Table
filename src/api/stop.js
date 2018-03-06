
const stop = () => {
  fetch(`http://${config.renderer}/stop`)
  .then(e => store.dispatch({msg:'Stopped'}))
  .catch(e => store.dispatch({msg:'Failed To Stop'})) 
}

export default stop