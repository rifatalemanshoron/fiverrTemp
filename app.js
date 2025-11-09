// app.js
const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');

const app = express();
const server = http.createServer(app);

// 👇 add a stable path and turn off perMessageDeflate (avoids ESP quirks)
const wss = new WebSocket.Server({
  server,
  path: '/ws',
  perMessageDeflate: false
});

app.use(express.static(path.join(__dirname, 'public'))); // serve index.html

// heartbeat
function noop() {}
function heartbeat() { this.isAlive = true; }

wss.on('connection', (ws, req) => {
  ws.isAlive = true;
  ws.on('pong', heartbeat);
  console.log('Client connected', req.url);

  ws.on('message', (msg) => {
    const text = msg.toString();
    console.log('RX:', text);

    // broadcast to all clients
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) client.send(text);
    });
  });

  ws.on('close', () => console.log('Client disconnected'));
});

// ping every 30s to keep socket alive through proxies
const interval = setInterval(() => {
  wss.clients.forEach((ws) => {
    if (!ws.isAlive) return ws.terminate();
    ws.isAlive = false;
    ws.ping(noop);
  });
}, 30000);

wss.on('close', () => clearInterval(interval));

const PORT = process.env.PORT || 8080;
server.listen(PORT, '0.0.0.0', () => console.log(`Server running on ${PORT}`));
