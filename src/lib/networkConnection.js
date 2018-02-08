import { setTimeout } from "timers";
import connStatus from '../index'
import config from '../config'

const startCheckingConnection = () => {

  const remoteServer = config.remoteServer || 'http://localhost:3001/';

  const checkConnection = () => {
    setTimeout(() => {
      fetch(remoteServer)
       .then(res => connStatus.dispatch( true ))
       .then(() => checkConnection())
       .catch(err => { 
         connStatus.dispatch(false)
         checkConnection()
        })
    },1000)
  }

  checkConnection()
}

export default startCheckingConnection
