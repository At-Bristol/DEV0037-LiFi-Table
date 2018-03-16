const express = require('express')
const app = express()
const path = require('path')
const http = require('http').Server(app);
const io = require('socket.io')(http)

const bodyParser = require('body-parser')
const renderer = require('./renderer')()
const ws281x = require('rpi-ws281x-native')

process.on('SIGINT', () => {
  console.log('Closing')
  ws281x.reset();
  process.nextTick(() => process.exit())
})

process.on('uncaughtException', (err) => {
  console.log('Closing due to uncaught exception', err)
  ws281x.reset();
  process.nextTick(() => process.exit())
})

process.on('exit', (code) => {
  ws281x.reset();
  console.log(`About to exit with code: ${code}`);
});

const parentDir = __dirname.substring(0, __dirname.lastIndexOf('/'))

app.use(bodyParser.json()); 
app.use(bodyParser.urlencoded({ extended: true }));
app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  next();
});

app.use(express.static(path.join(parentDir, '/dist')))

app.get('/', (req, res) => res.sendFile(path.join(parentDir, '/dist/index.html')))

app.get('/init', (req, res) => res.send('init'))
app.post('/init', (req, res) => {
  renderer.init(req.body, res.send('init renderer'))
})

app.get('/stop', (req, res) => {
  renderer.stop(res.send('stopped rendered'))
})

app.post('/save', (req, res) => {
  console.log(req.body)
  storage.write(req.body, res.send(`saved ${req.body}`))
})

app.get('/load', (req, res) => {
  res.send(storage.read())
})

const socket = io.on('connection', socket => {
  console.log('client connected')

  socket.on('render', frame => renderer.render(frame))

  socket.on('error', err => console.log('error:', err))

  socket.on('disconnect', reason => console.log('client disconnected for reason: ', reason));

})

http.listen(3000, () => console.log('Lifi table listening on port 3000'))


