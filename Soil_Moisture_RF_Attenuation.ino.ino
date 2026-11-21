
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define CHANNEL 1
#define TEMP_PIN 4

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(128, 64, &Wire, -1);

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// ESP32 #2 MAC
uint8_t MAC_REF[] = {
  0x8C, 0x94, 0xDF, 0x96, 0x2C, 0xEC
};

// ESP32 #3 MAC
uint8_t MAC_SOIL[] = {
  0x68, 0x09, 0x47, 0x9C, 0x7C, 0x70
};

// Broadcast address
uint8_t BROADCAST[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

const uint32_t MAGIC = 0xAABBCCDD;

struct ProbePacket {
  uint32_t magic;
  uint8_t type;
  uint32_t sequence;
} __attribute__((packed));

struct TelemetryPacket {
  uint32_t magic;
  uint8_t type;
  int16_t rssi;
  uint16_t count;
} __attribute__((packed));

int refRSSI = -127;
int soilRSSI = -127;

bool refReceived = false;
bool soilReceived = false;

float temperature = -127;

uint32_t sequenceNumber = 0;

unsigned long lastTransmit = 0;
unsigned long lastDisplay = 0;
unsigned long lastTemperature = 0;

// Register ESP-NOW peer
void addPeer(uint8_t *mac) {

  esp_now_peer_info_t peer = {};

  memcpy(peer.peer_addr, mac, 6);
  peer.channel = CHANNEL;
  peer.encrypt = false;

  if (!esp_now_is_peer_exist(mac)) {

    esp_err_t result = esp_now_add_peer(&peer);

    if (result == ESP_OK)
      Serial.println("PEER ADDED");
    else
      Serial.println("PEER ADD ERROR");
  }
}

// Receive RSSI reports
void onReceive(const esp_now_recv_info_t *info,
               const uint8_t *data, int len) {

  if (len != sizeof(TelemetryPacket))
    return;

  TelemetryPacket packet;
  memcpy(&packet, data, sizeof(packet));

  if (packet.magic != MAGIC)
    return;

  if (packet.type == 2) {
    refRSSI = packet.rssi;
    refReceived = true;
  }

  if (packet.type == 3) {
    soilRSSI = packet.rssi;
    soilReceived = true;
  }
}

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 #1 STARTING");

  // Wi-Fi setup
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  esp_wifi_set_channel(
    CHANNEL, WIFI_SECOND_CHAN_NONE
  );

  Serial.print("ESP32 #1 MAC: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW ERROR");
    while (true) delay(1000);
  }

  // Add broadcast peer
  addPeer(BROADCAST);

  // Add receiver peers
  addPeer(MAC_REF);
  addPeer(MAC_SOIL);

  esp_now_register_recv_cb(onReceive);

  Serial.println("ESP32 #1 READY");

  // OLED setup
  Wire.begin(21, 22);

  if (!display.begin(
      SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("OLED ERROR");
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println("SYSTEM ON");
  display.display();

  // Temperature sensor
  sensors.begin();

  Serial.println("OLED READY");
  Serial.println("TEMPERATURE SENSOR INITIALIZED");
}

void loop() {

  unsigned long now = millis();

  // TRANSMIT RF PACKETS
  if (now - lastTransmit >= 100) {

    ProbePacket packet;

    packet.magic = MAGIC;
    packet.type = 1;
    packet.sequence = sequenceNumber++;

    esp_err_t result = esp_now_send(
      BROADCAST,
      (uint8_t *)&packet,
      sizeof(packet)
    );

    if (result == ESP_OK) {
      Serial.println("TX PACKET SENT");
    }
    else {
      Serial.print("TX SEND ERROR: ");
      Serial.println(esp_err_to_name(result));
    }

    lastTransmit = now;
  }

  // READ TEMPERATURE
  if (now - lastTemperature >= 1500) {

    sensors.requestTemperatures();

    temperature = sensors.getTempCByIndex(0);

    lastTemperature = now;

    Serial.print("TEMPERATURE: ");
    Serial.println(temperature);
  }

  // OLED DISPLAY
  if (now - lastDisplay >= 500) {

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("SOIL MOISTURE SYSTEM");

    display.setCursor(0, 15);
    display.print("REF RSSI: ");

    if (refReceived)
      display.print(refRSSI);
    else
      display.print("--");

    display.println(" dBm");

    display.setCursor(0, 27);
    display.print("SOIL RSSI: ");

    if (soilReceived)
      display.print(soilRSSI);
    else
      display.print("--");

    display.println(" dBm");

    display.setCursor(0, 39);
    display.print("LOSS: ");

    if (refReceived && soilReceived) {
      display.print(refRSSI - soilRSSI);
      display.println(" dB");
    }
    else {
      display.println("--");
    }

    display.setCursor(0, 51);
    display.print("TEMP: ");

    if (temperature > -100)
      display.print(temperature, 1);
    else
      display.print("--");

    display.println(" C");

    display.display();

    lastDisplay = now;
  }
}
