#include <SPI.h>
#include <LoRa.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <PubSubClient.h>

/* ===== ID DEL NODO ===== */
const char* NODE_ID = "VACA_01";

/* ===== GPS ===== */
TinyGPSPlus gps;
HardwareSerial GPSSerial(2);

/* ===== LORA ===== */
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_CS     5
#define LORA_RST   14
#define LORA_DIO0  26

/* ===== WIFI ===== */
const char* WIFI_SSID = "SU_WIFI";
const char* WIFI_PASS = "SU_PASS";

/* ===== MQTT (respaldo) ===== */
const char* MQTT_HOST = "HOST_BACKEND";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC_BASE = "gps/lora";

// Si su broker requiere usuario/clave, descomente y complete:
// const char* MQTT_USER = "usuario";
// const char* MQTT_PASS = "clave";

WiFiClient espClient;
PubSubClient mqtt(espClient);

/* ===== Timers ===== */
unsigned long lastSendMs = 0;
const unsigned long SEND_INTERVAL_MS = 5000;

unsigned long lastWifiRetryMs = 0;
const unsigned long WIFI_RETRY_MS = 10000;

unsigned long lastMqttRetryMs = 0;
const unsigned long MQTT_RETRY_MS = 8000;

void setupLoRa() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa TX fallo");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();

  Serial.println("LoRa OK");
}

void wifiEnsureConnected() {
  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - lastWifiRetryMs < WIFI_RETRY_MS) return;
  lastWifiRetryMs = now;

  Serial.print("Conectando WiFi a ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void mqttEnsureConnected() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connected()) return;

  unsigned long now = millis();
  if (now - lastMqttRetryMs < MQTT_RETRY_MS) return;
  lastMqttRetryMs = now;

  String clientId = String("TX-") + NODE_ID + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);

  Serial.print("Conectando MQTT a ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(MQTT_PORT);

  bool ok = mqtt.connect(clientId.c_str());

  // Si usa auth:
  // bool ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);

  if (ok) {
    Serial.println("MQTT conectado");
  } else {
    Serial.print("MQTT fallo, rc=");
    Serial.println(mqtt.state());
  }
}

void publishBackupMQTT(const String& payload) {
  if (!mqtt.connected()) return;

  String topic = String(MQTT_TOPIC_BASE) + "/" + NODE_ID;
  bool retained = true;
  bool ok = mqtt.publish(topic.c_str(), payload.c_str(), retained);

  if (ok) {
    Serial.println("MQTT backup OK -> " + topic);
  } else {
    Serial.println("MQTT backup FAIL");
  }
}

String buildPayload() {
  String payload;

  if (gps.location.isValid()) {
    payload = "{";
    payload += "\"id\":\"" + String(NODE_ID) + "\",";
    payload += "\"lat\":" + String(gps.location.lat(), 6) + ",";
    payload += "\"lng\":" + String(gps.location.lng(), 6) + ",";
    payload += "\"sat\":" + String(gps.satellites.value());
    payload += "}";
  } else {
    payload = "{";
    payload += "\"id\":\"" + String(NODE_ID) + "\",";
    payload += "\"status\":\"NO_FIX\"";
    payload += "}";
  }

  return payload;
}

void sendLoRa(const String& payload) {
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  GPSSerial.begin(9600, SERIAL_8N1, 16, 17);

  setupLoRa();

  WiFi.setSleep(true);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(15);

  Serial.println("TX GPS + LoRa + MQTT backup listo");
}

void loop() {
  while (GPSSerial.available()) {
    gps.encode(GPSSerial.read());
  }

  wifiEnsureConnected();
  mqttEnsureConnected();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastSendMs >= SEND_INTERVAL_MS) {
    lastSendMs = now;

    String payload = buildPayload();

    sendLoRa(payload);
    Serial.println("TX LoRa: " + payload);

    if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
      publishBackupMQTT(payload);
    } else {
      Serial.println("MQTT backup no disponible (sin WiFi/MQTT)");
    }
  }
}