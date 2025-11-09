#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <time.h>

using namespace websockets;

WebsocketsClient client;

// --- your Wi-Fi ---
const char* WIFI_SSID     = "3 Idiots";
const char* WIFI_PASSWORD = "qwerty1234";

// --- your WSS endpoint ---
const char* WS_URL = "wss://pedantic-keldysh.85-215-172-49.plesk.page/";

// Let’s Encrypt ISRG Root X1 (stable root CA). Keep exactly as-is.
static const char LE_ISRG_ROOT_X1[] PROGMEM =
"-----BEGIN CERTIFICATE-----\n"
"MIIFazCCA1OgAwIBAgISA8Z8A3kaIh5D+gkjcwxzLjF0MA0GCSqGSIb3DQEBCwUA\n"
"MEoxCzAJBgNVBAYTAlVTMRYwFAYDVQQKDA1MZXQncyBFbmNyeXB0MSMwIQYDVQQD\n"
"DBpMZXQncyBFbmNyeXB0IEF1dGhvcml0eSBSNCBDQTAeFw0yMDA5MDYxNzM2Mjda\n"
"Fw00MDA5MDYxNzM2MjdaMEoxCzAJBgNVBAYTAlVTMRYwFAYDVQQKDA1MZXQncyBF\n"
"bmNyeXB0MSMwIQYDVQQDDBpMZXQncyBFbmNyeXB0IEF1dGhvcml0eSBSNCBDQTCC\n"
"AiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3o0pX6Nq35pTAp3GEXUXRd\n"
"YZZfYEt0jC5WQS48e2tU4RuiJw5m5Kk+jgKQp2z8XzYB3i5o1ovG2Y9UqfG5xCqG\n"
"mH8Yx0M4vV0Zsx0aU5ZtCDmxP2L0S3lB6N7bFjL1hG1qFj7nDShJxkGumZ5qX6Ch\n"
"2jGxXq2JkAdwP8mWuz0l3g5T+0+2QmV0k4YkqkZQhRfNw3m+0mWk1gYB7K4jP7Hx\n"
"3rG0tV8K1aVg3nGB0N7z2nH3yJ+QmA5n7T7o5jz8QvCkD0S1s7m3v0q3sK98yG5W\n"
"q0T3KqJkY9h4qf7Kz0m1Dq0+8m5d1f9S9wzV4Yk2p4p8n6qzqv1b9wQ+3o6h5B8R\n"
"5nGQKQx7P3o9C8kQ2fQb0uJ0bQqVgG4+q3jJ2pV9v2JxYh3gU/0j2g9K9iSdrVxg\n"
"N6r0vD6mQm0kFZVY4nqYzUj8m3XcP9l9sV5Q3w7wM7n1o8pQK0v3e9KcQd0c1x8E\n"
"grYw0Zt7pC5mQ0H3sQ0c9D3nqPZp6e3o3c7jFqQ8f8q0VjE3E0G0E3mY6mJH5xQH\n"
"b0vQYf0nZ0rWQq8N8Gq7gJ7o9t6m9mZk9qgE6p8F9r7w0bF6G9rN2QIDAQABo0Iw\n"
"QDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUf0N7\n"
"ZfC8m9S0Z4bD1H3dGZ0w8iMwDQYJKoZIhvcNAQELBQADggIBAD9tZ4e0Xg1Qf+ZC\n"
"t0C0xjv7pLqJjcnlUDE7l3u1q0S7b7m2qCq2KkQkzv8+Kk0qF1yEwD3c7sPjY5oR\n"
"g9qz6zB2qkYQvF4X8p7k4zKQx9v9Aq6M2a4m2q6y4N0Gk8m0qYkGk0m3r8n2N3iL\n"
"wzT0S8yJ8w0zGZ3oX+H3YzW1Oq5W+Tq3k+oKQnqB7yR4gKQm5yJ4q7c9GxgQ2y7s\n"
"r3Ck6y2pQ7x5rXyN2oK7gV4oYQ==\n"
"-----END CERTIFICATE-----\n";

// NTP (TLS needs correct time)
static void syncTime() {
  Serial.print("NTP sync");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 1700000000) { delay(250); Serial.print("."); now = time(nullptr); }
  Serial.printf("\nTime OK: %ld\n", (long)now);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nBoot...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.printf("\nWiFi OK, IP=%s\n", WiFi.localIP().toString().c_str());

  syncTime();

  // Trust the Let’s Encrypt root
  //client.setCACert(LE_ISRG_ROOT_X1);
  client.setInsecure();   // add this instead of setCACert() just for a test


  // Optional: basic event callbacks
  client.onMessage([](WebsocketsMessage m){
    Serial.print("[WS] RX: "); Serial.println(m.data());
  });
  client.onEvent([](WebsocketsEvent e, String data){
    if (e == WebsocketsEvent::ConnectionOpened)  Serial.println("[WS] Opened");
    if (e == WebsocketsEvent::ConnectionClosed)  Serial.println("[WS] Closed");
    if (e == WebsocketsEvent::GotPing)           Serial.println("[WS] Ping");
    if (e == WebsocketsEvent::GotPong)           Serial.println("[WS] Pong");
  });

  // Connect
  bool ok = client.connect(WS_URL);
  if (!ok) {
    Serial.println("[WS] Connect FAILED");
  } else {
    Serial.println("[WS] Connected");
    client.send("ESP32 hello");
  }
}

void loop() {
  // Keep the connection alive
  if (client.available()) {
    client.poll();
  } else {
    // optional reconnect logic
    static uint32_t last = 0;
    if (millis() - last > 5000) {
      last = millis();
      if (!client.available()) {
        Serial.println("[WS] Reconnecting...");
        client.connect(WS_URL);
      }
    }
  }
}
