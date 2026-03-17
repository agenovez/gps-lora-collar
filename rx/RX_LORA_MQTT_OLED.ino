#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

/* ===== OLED ===== */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* ===== WIFI ===== */
const char* ssid = "SU_WIFI";
const char* password = "SU_PASS";

/* ===== MQTT ===== */
const char* mqtt_server = "HOST_BACKEND";   // Cambie por la IP real de su servidor Docker
WiFiClient espClient;
PubSubClient client(espClient);

/* ===== LORA ===== */
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_CS     5
#define LORA_RST   14
#define LORA_DIO0  26

void reconnectMQTT() {
  while (!client.connected()) {
    client.connect("LoRaGateway");
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);

  /* OLED */
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("RX LoRa Gateway");
  display.display();

  /* WIFI */
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  /* MQTT */
  client.setServer(mqtt_server, 1883);

  /* LORA */
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa RX fallo");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();

  Serial.println("RX listo");
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String msg;
    while (LoRa.available()) msg += (char)LoRa.read();

    Serial.println("RX: " + msg);
    client.publish("gps/lora", msg.c_str());

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, msg) == DeserializationError::Ok) {
      display.clearDisplay();
      display.setCursor(0, 0);

      display.print("Nodo: ");
      display.println(doc["id"].as<const char*>());

      if (doc.containsKey("lat")) {
        display.print("Lat: ");
        display.println(doc["lat"].as<float>(), 5);
        display.print("Lng: ");
        display.println(doc["lng"].as<float>(), 5);
        display.print("Sat: ");
        display.println(doc["sat"].as<int>());
      } else {
        display.println("GPS SIN FIX");
      }

      display.display();
    }
  }
}