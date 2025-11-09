// app.js
const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');

const app = express();
const server = http.createServer(app);

// Serve static site
app.use(express.static(path.join(__dirname, 'public')));

// Basic health check for Render
app.get('/health', (_req, res) => res.status(200).send('ok'));

// Create WS server on a stable path
const wss = new WebSocket.Server({ server, path: '/ws' });

// Heartbeat helpers
function noop() {}
function heartbeat() { this.isAlive = true; }

wss.on('connection', (ws) => {
  ws.isAlive = true;
  ws.on('pong', heartbeat);

  console.log('Client connected');

  ws.on('message', (msg) => {
    const text = msg.toString();
    console.log('RX:', text);

    // Broadcast to all connected clients
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(text);
      }
    });
  });

  ws.on('close', () => console.log('Client disconnected'));
});

// Ping clients periodically to detect dead sockets
const interval = setInterval(() => {
  wss.clients.forEach((ws) => {
    if (!ws.isAlive) return ws.terminate();
    ws.isAlive = false;
    ws.ping(noop);
  });
}, 30000);

wss.on('close', () => clearInterval(interval));

// Required for Render: listen on PORT and 0.0.0.0
const PORT = process.env.PORT || 8080;
server.listen(PORT, '0.0.0.0', () => console.log(`Server running on ${PORT}`));
