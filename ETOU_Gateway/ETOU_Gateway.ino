// https://pubsubclient.knolleary.net/api
// UART https://www.circuitbasics.com/how-to-set-up-uart-communication-for-arduino/
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

byte MAC_ADDRESS[] = {  0x90, 0xA2, 0xDA, 0x0E, 0x94, 0x93 };
byte MQTT_SERVER[] = { 192, 168, 1, 105};
unsigned int PORT = 1883;

EthernetClient ethClient;
PubSubClient mqtt_client(ethClient); 
SoftwareSerial SerialOut(8, 9);

byte charNumberToByte(char c) {
  switch (c) {
    case '1': return 0b1;
    case '2': return 0b10;
    case '3': return 0b11;
    case '4': return 0b100;
    case '5': return 0b101;
    case '6': return 0b110;
    case '7': return 0b111;
    case '8': return 0b1000;
    case '9': return 0b1001;
  }
  return 0;
}  

byte charArrayToByte(uint8_t* arr , unsigned int length) {
  byte result = 0;
  for (int i = 0; i < length; i++) {
    result *= 10;
    result += charNumberToByte(arr[i]);
  }
  return result;
}

void onMQTTMessage(char* topic, uint8_t* payload, unsigned int length) {  
    if (strlen(topic) > 4) {
      byte value = charArrayToByte(payload, length); 
      Serial.println(topic);
      Serial.println(value);
      Serial.write(payload, length);
      byte data[] = {(byte)topic[4], value};
      SerialOut.write(data, 2);
    }
}
void reconnect() {
    while (!mqtt_client.connected()) {
        if (mqtt_client.connect(F("ALeonardo"))) {
            mqtt_client.subscribe(F("LED/R"));
            mqtt_client.subscribe(F("LED/G"));
            mqtt_client.subscribe(F("LED/B"));
        } else {
          Serial.println(F("Failed to connect to server!"));
        }
        delay(5000);
    }
} 

void readSerial() {
  if (SerialOut.available()) {
    byte in[1];
    SerialOut.readBytes(in, 1);
    mqtt_client.publish(F("arduino/uno/key1"), in);
  }
}

void setup() {   
  Serial.begin(9600);
  if (Ethernet.begin(MAC_ADDRESS) == 0) Serial.println(F("Ethernet failed!")); 
  else Serial.println(F("Ethernet connected!"));
  
  mqtt_client.setServer(MQTT_SERVER, PORT);
  mqtt_client.setCallback(onMQTTMessage);
  SerialOut.begin(115200);
  delay(1000);
}

void loop() {
  if (!mqtt_client.connected()) reconnect();
  mqtt_client.loop();
  readSerial();
}
