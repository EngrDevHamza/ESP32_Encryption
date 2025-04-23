#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "crypto_utils.h"
#include "lockdown.h"
#include "ota_handler.h"
#include "key_rotation.h"
#include "DHT.h"
#include "secret_keys.h"

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

String aesKey = "01234567890123456789012345678901"; 
Preferences preferences; 

// === Credentials and Configuration ===
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

const char* mqtt_broker = SECRET_BROKER;
const int mqtt_port = SECRET_PORT;
const char* mqtt_username = SECRET_USERNAME;
const char* mqtt_password = SECRET_PASSWORD;
const char* topic_publish = SECRET_PUBLISH;
const char* topic_command = SECRET_COMMAND;
const char* topic_keys = SECRET_KEY;


WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// === States ===
bool lockdownEnabled = false;
unsigned long lastKeyRotation = 0;

void setup() {
  Serial.begin(115200);
  connectWiFi();
  wifiClient.setInsecure();
  setupMQTT();
  dht.begin();
  setupOTA();
  generateInitialKey(); // Sets AES key
}

void loop() {
  mqttClient.loop();
  if (!mqttClient.connected()) reconnectMQTT();
  if (lockdownEnabled) return;
  
  ArduinoOTA.handle();
 

  // Rotate encryption key every 24 hours 86400000
  if (millis() - lastKeyRotation > 20000) {
    rotateEncryptionKey();
    lastKeyRotation = millis();
    // Serial.println("Key rotated");
  }

  // Simulate secure log send
  // String logData = "Temperature: 25C";
  // String encrypted = encryptData(logData);
  // mqttClient.publish("esp32/dht", encrypted.c_str());

  // delay(5000); // 5 seconds
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 10000) {
    lastTime = millis();
    float h = dht.readHumidity();
    if (isnan(h)) return;

    String logData = "{\"humidity\":" + String(h) + "}";
    Serial.println(logData);
    String encrypted = encryptData(logData);
    Serial.println(encrypted);
    mqttClient.publish(topic_publish, encrypted.c_str());
  }
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi connected");
}

void setupMQTT() {
  mqttClient.setServer(mqtt_broker, mqtt_port);
  mqttClient.setCallback(mqttCallback);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  if (String(topic) == "esp32/command") {
    if (message == "LOCKDOWN") {
      lockdownEnabled = true;
      Serial.println("Device in lockdown.");
    }
    else if (message == "getkeys") {
      sendStoredKeysToServer();
      
    }
}
}
void sendStoredKeysToServer() {
  preferences.begin("keys", true);
  uint32_t index = preferences.getUInt("keyIndex", 1);

  for (uint32_t i = 0; i < index; i++) {
    String keyName = "key" + String(i);
    String keyValue = preferences.getString(keyName.c_str());
    String payload = "Key" + String(i) + ": " + keyValue;
    mqttClient.publish(topic_keys, payload.c_str());  // Send each key to broker
    Serial.println("Keys sent to broker");
    delay(100); // Slight delay to avoid flooding
  }

  preferences.end();
  mqttClient.publish("esp32/keys", "All keys sent.");
  Serial.println("Stored keys sent to server.");
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("MQTT connected");
      mqttClient.subscribe(topic_command); // Listen for commands
    } else {
      Serial.print("MQTT failed: ");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}
