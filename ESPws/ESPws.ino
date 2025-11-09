#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>

// ===== WIFI =====
const char* WIFI_SSID = "3 Idiots";
const char* WIFI_PASS = "qwerty1234";

// ===== RENDER SERVER =====
const char* WS_HOST = "fiverrtemp-nyr6.onrender.com";
const uint16_t WS_PORT = 443;
const char* WS_PATH = "/ws";

// ===== HARDWARE =====
const int BUTTON_PIN = 4;        // Button to GND (use internal pull-up)
const uint32_t DEBOUNCE_MS = 50; // debounce delay

// ===== GLOBALS =====
WebSocketsClient webSocket;
bool deviceOnline = false;       // current ON/OFF state
bool lastButtonState = HIGH;     // last stable state of button
uint32_t lastDebounceTime = 0;

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Connecting to WiFi ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void sendState() {
  if (!webSocket.isConnected()) return;
  const char* msg = deviceOnline ? "ONLINE" : "OFFLINE";
  webSocket.sendTXT(msg);
  Serial.printf("[WS] -> %s\n", msg);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("[WS] Connected!");
      sendState(); // send current state immediately
      break;
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected");
      break;
    case WStype_TEXT:
      Serial.printf("[WS] <- %.*s\n", (int)length, (const char*)payload);
      break;
    default: break;
  }
}

void setupWebSocket() {
  // Connect securely to Render WSS
  webSocket.beginSSL(WS_HOST, WS_PORT, WS_PATH);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(3000);
  webSocket.enableHeartbeat(30000, 5000, 2); // ping every 30s
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  ensureWiFi();
  setupWebSocket();
  sendState();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) ensureWiFi();
  webSocket.loop();

  // === BUTTON TOGGLE LOGIC ===
  bool reading = digitalRead(BUTTON_PIN);
  uint32_t now = millis();

  // detect button press (transition from HIGH to LOW)
  if (reading != lastButtonState) {
    lastDebounceTime = now;
    lastButtonState = reading;
  }

  // after debounce time, confirm button press
  if ((now - lastDebounceTime) > DEBOUNCE_MS) {
    // if pressed (LOW)
    if (reading == LOW) {
      // toggle the device state
      deviceOnline = !deviceOnline;
      sendState();
      delay(300); // simple press delay to avoid double toggling
    }
  }

  // periodic keep-alive resend
  static uint32_t lastHeartbeat = 0;
  if (now - lastHeartbeat > 30000) {
    lastHeartbeat = now;
    sendState();
  }
}
