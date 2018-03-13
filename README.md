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

To set up on a new system you must first make sure you have ssh access to the Rasberry pi, then from project root run. NB this requires you have bash shell available

```
  npm run setup
```

This will attempt to install and configure the server and the LiFi drivers 

note: the raspberrypi must have internet access for this to work

## Known bugs

If the lifi AP is plugged in on boot then the WiFI ap will not be created. This is because the LiFI AP bumps the WiFi to wlan1 
