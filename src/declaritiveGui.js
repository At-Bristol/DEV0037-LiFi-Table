
import dat from 'dat.gui';

const makeDeclaritiveGui = (params) => {

  const gui = new dat.GUI();

  const folders = Object.keys(params)

  const makeFolders = () => folders.map(e => gui.addFolder(e))

  const makeControl = (control, folder) => {
    const {name, type = 'float', value = 0, rangeIn = 0, rangeOut = 100 } = control
    //const entity = folder ? folder : gui

    const thing = {[name]: value}

    if(!name) throw new Error('control must have "name" prroperty')
    if(type === 'color') folder.addColor(thing, name )
    if(type === 'bool') folder.add(thing, name )
    else { 
      folder.add(thing, name, rangeIn, rangeOut )
    }
  }

  const makeControls = (controls, folder) => controls.map(c => makeControl(c, folder))

  makeFolders()

  Object.keys(gui.__folders).map(n =>
    makeControls(params[n], gui.__folders[n]) 
  )
  
    //.map(e => generateControl(e, f)))
    
   // params[e].map(c => generateControls(params[e][c])))
}


export default makeDeclaritiveGui

/*
store.subscribe(updateParams)


let params = Object.assign(
  store.getState(),
  {
    stop: () => stop(),
    init: () => init()
  }
)*/


  /*const updateParams = () => {
    Object.keys(params).map(e => {
      try {
        params[e] = state[e]
      }catch(e){}
    })
  }*/