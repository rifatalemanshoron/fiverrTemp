#include <WiFi.h>
#include <WebSocketsClient.h>
#include <WiFiClientSecure.h>

// ===== WIFI =====
const char* WIFI_SSID = "3 Idiots";
const char* WIFI_PASS = "qwerty1234";

// ===== RENDER WS =====
const char* WS_HOST = "fiverrtemp-nyr6.onrender.com";
const uint16_t WS_PORT = 443;
const char* WS_PATH = "/ws";

// ===== BUTTON =====
const int BUTTON_PIN = 4;          // to GND, uses internal pull-up
const uint32_t DEBOUNCE_MS = 30;

// ===== GLOBALS =====
WebSocketsClient webSocket;
bool lastStablePressed = false;
bool currentPressed = false;
uint32_t lastChangeMs = 0;

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    if (millis() - t0 > 20000) { WiFi.disconnect(true); delay(500); WiFi.begin(WIFI_SSID, WIFI_PASS); t0 = millis(); }
  }
}

void sendState(bool pressed) {
  if (!webSocket.isConnected()) return;
  webSocket.sendTXT(pressed ? "ONLINE" : "OFFLINE");
  Serial.println(pressed ? "[WS] -> ONLINE" : "[WS] -> OFFLINE");
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WS] Connected to: %s\n", payload ? (const char*)payload : "(no url)");
      sendState(lastStablePressed); // publish current state immediately
      break;
    case WStype_TEXT:
      Serial.printf("[WS] <- %.*s\n", (int)length, (const char*)payload);
      break;
    default: break;
  }
}

void setupWebSocket() {
  // NOTE: Links2004 library will set insecure when no fingerprint/CA is provided on ESP32.
  webSocket.beginSSL(WS_HOST, WS_PORT, WS_PATH);  // wss://host:443/ws
  // No extra headers — simpler is safer through proxies
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(3000);
  webSocket.enableHeartbeat(30000, 5000, 2);      // ping 30s, expect pong <5s (2 misses -> reconnect)
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  ensureWiFi();
  setupWebSocket();

  lastStablePressed = (digitalRead(BUTTON_PIN) == LOW);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) ensureWiFi();
  webSocket.loop();

  // Debounce
  bool rawPressed = (digitalRead(BUTTON_PIN) == LOW);
  uint32_t now = millis();
  if (rawPressed != currentPressed) { currentPressed = rawPressed; lastChangeMs = now; }
  if ((now - lastChangeMs) > DEBOUNCE_MS) {
    if (lastStablePressed != currentPressed) {
      lastStablePressed = currentPressed;
      sendState(lastStablePressed);
    }
  }

  // Optional: periodic refresh
  static uint32_t lastRefresh = 0;
  if (now - lastRefresh > 30000) {
    lastRefresh = now;
    sendState(lastStablePressed);
  }
}
