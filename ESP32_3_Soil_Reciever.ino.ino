#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define CHANNEL 1

uint8_t ESP1_MAC[] = {
  0x68, 0x09, 0x47, 0x47, 0x5D, 0x5C
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

volatile int rssiSum = 0;
volatile int packetCount = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void onReceive(const esp_now_recv_info_t *info,
               const uint8_t *data, int len) {

  if (len != sizeof(ProbePacket)) return;

  ProbePacket packet;
  memcpy(&packet, data, sizeof(packet));

  if (packet.magic != MAGIC || packet.type != 1)
    return;

  int rssi = info->rx_ctrl->rssi;

  portENTER_CRITICAL(&mux);

  rssiSum += rssi;
  packetCount++;

  portEXIT_CRITICAL(&mux);
}

void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW ERROR");
    while (true) delay(1000);
  }

  esp_now_peer_info_t peer = {};

  memcpy(peer.peer_addr, ESP1_MAC, 6);
  peer.channel = CHANNEL;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("PEER ERROR");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onReceive);

  Serial.println("ESP32 #3 SOIL RECEIVER READY");
}

void loop() {

  static unsigned long previous = 0;

  if (millis() - previous >= 2000) {

    int sum;
    int count;

    portENTER_CRITICAL(&mux);

    sum = rssiSum;
    count = packetCount;

    rssiSum = 0;
    packetCount = 0;

    portEXIT_CRITICAL(&mux);

    if (count > 0) {

      TelemetryPacket report;

      report.magic = MAGIC;
      report.type = 3;
      report.rssi = sum / count;
      report.count = count;

      esp_now_send(ESP1_MAC,
                   (uint8_t *)&report,
                   sizeof(report));

      Serial.print("SOIL RSSI: ");
      Serial.print(report.rssi);
      Serial.println(" dBm");
    }
    else {
      Serial.println("NO RF PACKETS RECEIVED");
    }

    previous = millis();
  }
}