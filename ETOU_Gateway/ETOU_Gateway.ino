// https://pubsubclient.knolleary.net/api
// UART https://www.circuitbasics.com/how-to-set-up-uart-communication-for-arduino/
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>


byte MAC_ADDRESS[] = {  0x90, 0xA2, 0xDA, 0x0E, 0x94, 0x93 };
byte MQTT_SERVER[] = { 192, 168, 0, 25 };
byte IP_ADDRESS[] = {192, 168, 0, 125};

EthernetClient ethClient;
PubSubClient mqtt_client(ethClient); 

void onMQTTMessage(const char[] topic, byte* payload, unsigned int length) {  
    if (strlen(topic) > 4) serial.write((byte*){topic[4], payload[0]}, 2);
}

void reconnect() {
    while (!mqtt_client.connected()) {
        if (mqtt_client.connect("ALeonardo")) {
            mqtt_client.subscribe("LED/R");
            mqtt_client.subscribe("LED/G");
            mqtt_client.subscribe("LED/B");
        }
        delay(5000);
    }
} 

void setup() {   
  Ethernet.begin(MAC_ADDRESS,IP_ADDRESS);
  mqtt_client.setServer(MQTT_SERVER, 1883);
  mqtt_client.setCallback(onMQTTMessage);
  delay(1000);
  Serial.begin(9600);
}

void loop() {
  //Retry until connected is true
  if (!mqtt_client.connected()) reconnect();
  // MQTT client loop processing
  mqtt_client.loop();
}
