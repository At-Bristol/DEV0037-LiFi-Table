const init = () => {
  fetch(`http://${config.renderer}/init`,
  {
    method: 'POST',
    body:JSON.stringify({
      length:8,
      width:5
    }),
    headers: {
      'user-agent': 'Mozilla/4.0 MDN Example',
      'content-type': 'application/json'
    },
  })
  .then(e => store.dispatch({msg:'Successfully Initilaized'}))
  .catch(e => store.dispatch({msg:'Failed To Initialize'})) 
}

export default init