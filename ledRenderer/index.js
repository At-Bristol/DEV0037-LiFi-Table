const express = require('express')
const app = express()
const path = require('path')
var expressWs = require('express-ws')(app)

app.get('/init', () => express.static('dist'))
app.get('/stop', () => express.static('dist'))

app.ws('/render', function(ws, req) {
  ws.on('message', function(msg) {
    ws.send(msg);
  });
});

app.listen(3001, () => console.log('Render app  listening on port 3001'))
