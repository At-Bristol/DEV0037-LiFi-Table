const express = require('express')
const app = express()
const path = require('path')


app.all('/', function(req, res, next) {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "X-Requested-With");
  next();
 });

app.get('/', (req, res) => {
  res.send('')
})

app.listen(3001, () => console.log('Listening for table on port 3001'))
