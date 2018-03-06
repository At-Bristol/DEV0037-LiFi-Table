import { setTimeout } from "timers";

const startCheckingConnection = () => {

  const lifiServer = `http://${config.lifiServer}/` || 'http://localhost:3001/';

  const checkConnection = () => {
    setTimeout(() => {
      fetch(lifiServer)
       .then(res => store.dispatch({isConnected: true}))
       .then(() => checkConnection())
       .catch(err => { 
         store.dispatch({isConnected: false})
         checkConnection()
        })
    },1000)
  }

  checkConnection()
}

export default startCheckingConnection
