// server.js
const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

app.use(express.static(path.join(__dirname, 'public'))); // serve index.html

wss.on('connection', (ws) => {
  console.log('Client connected');

  ws.on('message', (msg) => {
    const text = msg.toString();
    console.log('RX:', text);

    // Broadcast to all clients (including HTML browsers)
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(text);
      }
    });
  });

  ws.on('close', () => console.log('Client disconnected'));
});

const PORT = process.env.PORT || 8080;
server.listen(PORT, '0.0.0.0', () => console.log(`Server running on ${PORT}`));

