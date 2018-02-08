const express = require('express')
const app = express()
const path = require('path')
var expressWs = require('express-ws')(app)

app.use(express.static('dist'))

app.get('/init', (req, res) => res.send('Init'))
app.get('/stop', (req, res) => res.send('stop'))

app.ws('/render', function(ws, req) {
  ws.on('message', function(msg) {
    ws.send('render');
  });
});

app.listen(3000, () => console.log('App listening on port 3000'))


