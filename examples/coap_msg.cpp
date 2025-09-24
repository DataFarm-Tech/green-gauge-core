#include <WiFi.h>
#include "Thing.CoAP.h"
#include <Arduino.h>
#include <cbor.h>
#include <esp_sleep.h>

// CoAP client and UDP provider
Thing::CoAP::Client coapClient;
Thing::CoAP::ESP::UDPPacketProvider udpProvider;

// WiFi credentials
const char* ssid = "NETGEAR77";
const char* password = "aquaticcarrot628";

// Target CoAP server IP and port
IPAddress serverIp(45, 79, 239, 100);  // Replace with your actual server IP
const int serverPort = 5683;

// Time to sleep (10 minutes = 600,000,000 microseconds)
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 600  // seconds

void goToSleep() {
  Serial.println("Going to sleep for 10 minutes...");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void sendMessage() {
  uint8_t buffer[64];
  CborEncoder encoder, mapEncoder;

  cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);
  cbor_encoder_create_map(&encoder, &mapEncoder, 3);

  cbor_encode_text_stringz(&mapEncoder, "level");
  cbor_encode_int(&mapEncoder, 60);

  cbor_encode_text_stringz(&mapEncoder, "health");
  cbor_encode_int(&mapEncoder, 60);

  cbor_encode_text_stringz(&mapEncoder, "nodeId");
  cbor_encode_text_stringz(&mapEncoder, "he3345");

  cbor_encoder_close_container(&encoder, &mapEncoder);

  size_t cborPayloadSize = cbor_encoder_get_buffer_size(&encoder, buffer);
  std::vector<uint8_t> cborPayload(buffer, buffer + cborPayloadSize);

  coapClient.Post("battery", cborPayload, [](Thing::CoAP::Response response) {
    std::vector<uint8_t> payload = response.GetPayload();
    std::string received(payload.begin(), payload.end());


    delay(500);  // short delay to ensure logs go out
    goToSleep();
  });
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Booting up...");
  
  // Set static IP
  IPAddress local_IP(192, 168, 1, 147);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.setSleep(false); // Must be called before WiFi.begin()

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed. Going to sleep.");
    goToSleep();
    return;
  }

  Serial.println("\nConnected to WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Start CoAP client
  coapClient.SetPacketProvider(udpProvider);
  coapClient.Start(serverIp, serverPort);

  // Send the message
  sendMessage();
}

void loop() {
  // Only needed for CoAP client to process responses
  coapClient.Process();
}