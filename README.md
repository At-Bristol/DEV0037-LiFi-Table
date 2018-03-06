# DEV0037-LiFi-Table

This is the information for installing, starting and maintaining the lifi table software

## Design

The table is an http server listening on port 3000. It is designed to recieve an array of image data from the client an update the table.

Avaliable endpoints are

* / - access retrives the client
* /stop - stop the animation and clears the LED buffer  
* /init - restarts the LEDS

The image data is send via a websocket the connection to which can be established at /render

## Server

The server is a node.js server that listens on port 3000.

The server process is managed by pm2 

The source for the server is in /server 

## Client

The client is single page web application that renders the vsualisation using webgl.

It provides some basic parameters to control the table animation

source can be found in /src

It is designed to run on chrome

The client is compiled by webpack and put into /dist where the server actually serves the page from

## Hardware setup

The table server is set up to run node.js 9 on a Raspberry pi 3 Model B running Raspbian Stretch Lite

To set up on a new sd card run the following

```bash
  sudo apt-get update
  sudo apt-get upgrade
  curl -sL https://deb.nodesource.com/setup_9.x | sudo -E bash -
  sudo apt-get install -y nodejs
  npm install -g pm2
  sudo mkdir /var/www
  sudo chown pi /var/www
  sudo chmod 711 /var/www
  pm2 startup
```
follow the instructions from pm2 startup

Next on your development machine build the app by running

`npm run build`

Then copy /server, package.json, package-lock.json and /dist to /var/www on the server.

On the server install 

```
  cd /var/www
  npm install
``` 

Its now ready to run 

```
 sudo pm2 start /var/www/server/index.js
 sudo pm2 save 
``` 

Now the server will be avaliable and will start with server reboot

## Raspberry Pi as access point

It may be usful to run the table server as a wireless access point so you can connect to it

We followed [this guide to set it up]https://www.raspberrypi.org/documentation/configuration/wireless/access-point.md


