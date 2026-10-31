#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "secrets.h"

// Common ESP32-S3 Super Mini onboard WS2812 pin. Change this if the board
// variant uses GPIO 38 or an external RGB LED.
constexpr uint8_t LED_PIN = 48;
constexpr uint8_t LED_COUNT = 1;
constexpr uint32_t POLL_INTERVAL_MS = 30000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000;
constexpr uint16_t HTTP_TIMEOUT_MS = 5000;

Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint32_t next_poll = 0;
uint32_t next_wifi_retry = 0;

enum class State { Boot, Wifi, Api, DevicePresent, DeviceAway, Error };

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void show(State state) {
  switch (state) {
    case State::Boot:        setColor(0, 0, 0); break;        // off
    case State::Wifi:        setColor(40, 24, 0); break;      // amber
    case State::Api:         setColor(40, 0, 30); break;     // purple
    case State::DevicePresent:setColor(0, 45, 0); break;     // green
    case State::DeviceAway:   setColor(45, 0, 0); break;     // red
    case State::Error:       setColor(45, 12, 0); break;     // orange
  }
}

String apiUrl() {
  return String("https://") + UNIFI_HOST +
         "/proxy/network/api/s/default/stat/sta/" + String(DEVICE_MAC);
}

bool devicePresent() {
  WiFiClientSecure client;
  client.setInsecure(); // Local controller certificates are commonly self-signed.
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(client, apiUrl())) return false;
  http.addHeader("X-API-Key", UNIFI_API_KEY);
  http.addHeader("Accept", "application/json");
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    Serial.printf("UniFi HTTP status: %d\n", status);
    http.end();
    return false;
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, http.getStream());
  http.end();
  if (error) {
    Serial.println("UniFi response was not valid JSON");
    return false;
  }

  // UniFi returns an empty data array when the client is not currently known
  // to the controller. A non-empty array means the client is associated.
  JsonArray data = doc["data"].as<JsonArray>();
  return !data.isNull() && data.size() > 0;
}

void connectWifi() {
  show(State::Wifi);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to Wi-Fi SSID %s\n", WIFI_SSID);
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 15000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Wi-Fi connected; IP=%s\n", WiFi.localIP().toString().c_str());
    next_poll = 0;
  } else {
    Serial.println("Wi-Fi connection failed");
    next_wifi_retry = millis() + WIFI_RETRY_INTERVAL_MS;
  }
}

void setup() {
  Serial.begin(115200);
  pixel.begin();
  pixel.setBrightness(32);
  show(State::Boot);
  delay(250);
  connectWifi();
}

void loop() {
  const uint32_t now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    show(State::Wifi);
    if (now >= next_wifi_retry) connectWifi();
    delay(50);
    return;
  }

  if (now < next_poll) return;
  next_poll = now + POLL_INTERVAL_MS;
  show(State::Api);
  const bool present = devicePresent();
  Serial.printf("Device %s\n", present ? "present" : "away");
  show(present ? State::DevicePresent : State::DeviceAway);
}
