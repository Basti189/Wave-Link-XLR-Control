#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char* ssid = "NETWORK";
const char* password = "PASSWORD";

WebSocketsClient webSocket;

const char* micId = "DS37K1A01148\\PCM_IN_01_C_00_SD1"; // Deine Mikro-ID
bool isMuted = false;

unsigned long lastToggle = 0;
const unsigned long debounceDelay = 500;

const int buttonPin = 0; // Physischer Button an GPIO0 // Boot
const int buttonExt = 15; // Externer Button
const int ledPin = LED_BUILTIN;

void toggleMute() {
  isMuted = !isMuted;

  StaticJsonDocument<256> doc;
  doc["jsonrpc"] = "2.0";
  doc["id"] = 1;
  doc["method"] = "setMicrophoneConfig";
  JsonObject params = doc.createNestedObject("params");
  params["identifier"] = micId;
  params["property"] = "Microphone Mute";
  params["value"] = isMuted;

  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(output);

  Serial.printf("📤 Mikrofon auf %s gesetzt\n", isMuted ? "STUMM" : "AKTIV");
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {

    case WStype_DISCONNECTED: {
      Serial.println("[WebSocket] ❌ Verbindung getrennt");
      break;
    }

    case WStype_CONNECTED: {
      Serial.println("[WebSocket] ✅ Verbunden");
      break;
    }

    case WStype_TEXT: {
      StaticJsonDocument<2048> doc;
      DeserializationError err = deserializeJson(doc, payload);

      if (err) {
        Serial.print("[WebSocket] ⚠️ JSON-Fehler: ");
        Serial.println(err.c_str());
        return;
      }

      const char* method = doc["method"];

      // Echtzeitdaten (Pegeländerungen etc.) nicht loggen
      if (method && strcmp(method, "realTimeChanges") == 0) {
        // TODO: Hier verarbeiten, z.B. Pegelanzeige etc.
        return;
      }

      // Alles andere loggen
      Serial.println("[WebSocket] 📥 Nachricht erhalten:");
      serializeJsonPretty(doc, Serial);
      Serial.println();

      if (method && strcmp(method, "microphoneConfigChanged") == 0) {
        const char* identifier = doc["params"]["identifier"];
        const char* property = doc["params"]["property"];
        bool value = doc["params"]["value"];

        Serial.println("[WebSocket] 🎙️ Mikrofon-Änderung:");
        Serial.printf("  Identifier: %s\n", identifier);
        Serial.printf("  Property: %s\n", property);
        Serial.printf("  Value: %s\n", value ? "true" : "false");

        if (strcmp(property, "Microphone Mute") == 0) {
          if (value) {
            Serial.println("🔇 Mikrofon wurde stummgeschaltet.");
            isMuted = true;
          } else {
            Serial.println("🎤 Mikrofon ist aktiv.");
            isMuted = false;
          }
        }
      }

      break;
    }

    default: {
      Serial.printf("[WebSocket] ⚠️ Unbehandelter Typ: %d\n", type);
      break;
    }
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buttonExt, INPUT_PULLDOWN);
  pinMode(ledPin, OUTPUT);

  Serial.print("🌐 Verbinde mit WLAN...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WLAN verbunden");

  webSocket.begin("IP_ADDRESS", 1824, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(2000); // 2s reconnect

  Serial.println("🔌 Starte WebSocket-Verbindung...");
}

void loop() {
  webSocket.loop();

  if (digitalRead(buttonPin) == LOW && millis() - lastToggle > debounceDelay) {
    toggleMute();
    lastToggle = millis();
  }
  if (digitalRead(buttonExt) && millis() -lastToggle > debounceDelay) {
    toggleMute();
    lastToggle = millis();
  }
}
