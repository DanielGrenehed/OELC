# OELC
 Over engeneered LED control

## Setup
An Arduino UNO with two switches, one pot and an rgb led is connected via pin 0, 1 and GND to
pin 1, 0 and GND on a Arduino Leonardo with an ethernet shield. 
The ethernet port is connected via a cable to a local network and it is subscribed to an MQTT server.
The MQTT server is running on a Raspberry Pi(2b) with an RFID-RC522 card reader.

The UNO uses a 4 state state machine and the state is switched by pressint Key2 on it's shield. 
In state 4 it is listeng to UART and setting the LED color based on the data it is receiving.

The Leonardo is subscribed to the LED R G and B channel on the MQTT server and sends the corresponding color over UART when received.

The Raspberry Pi is continually trying to read rifd-cards, if a card is red a color is published to the MQTT based on if the card is whitelisted(Green), Blacklisted(Red) or unlisted(Blue).

## Raspberry pi
[mqtt setup](https://pimylifeup.com/raspberry-pi-mosquitto-mqtt-server/)