require('./exit')
const express = require('express')
const app = express()
const path = require('path')
const expressWs = require('express-ws')(app)
const bodyParser = require('body-parser');

const renderer = require('./renderer')()

let frame = 0

app.use(bodyParser.json()); 
app.use(bodyParser.urlencoded({ extended: true }));
app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  next();
});

app.get('/init', (req, res) => res.send('init'))
app.post('/init', (req, res) => {
  renderer.init(
    req.body.NUM_LEDS,
    res.send(req.body)
  )
})

app.get('/stop', (req, res) => {
  renderer.stop(res.send('stopped rendered'))
})

app.ws('/render', (ws, req) => {
  ws.on('message', msg => renderer.render(msg.split(',')) )
})

app.use(express.static('dist'))

app.listen(3000, () => console.log('App listening on port 3000'))


