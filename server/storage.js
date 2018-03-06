/*const low = require('lowdb')
const FileSync = require('lowdb/adapters/FileSync')

const adapter = new FileSync('db.json')
const db = low(adapter)


db.defaults({
  presets:[]
}).write()

const write = (data, cb) => {
  console.log(data)
  db.get('presets')
    .push({...data, presetId: Date.now()})
}

const read = (params = {}, cb) => {
  const { id } = params

  if (id) {
    return db.get('presets')
      .find({presetId: id})
      .value()
  }
  
  return db.get('presets')
    .takeRight(1)
    .value()
    
}

module.exports = {
  write,
  read
}*/