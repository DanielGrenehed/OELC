# OELC
 Over engeneered LED control

## Setup
An Arduino UNO with two switches, one pot and an rgb led, is connected via pin 5, 6 and GND to
pin 9, 8 and GND on a Arduino Leonardo with an ethernet shield. 
The ethernet port is connected via a cable to a local network and it is subscribed to an MQTT server.
The MQTT server is running on a Raspberry Pi(2b) with an RFID-RC522 card reader.

The UNO uses a 4 state state machine and the state is switched by pressint Key2 on it's shield. 
In state 4 it is listeng to UART and setting the LED color based on the data it is receiving.

The Leonardo is subscribed to the R, G and B subchannels of the LED channel on the MQTT server and sends the corresponding color over UART when received.

The Raspberry Pi is continually trying to read rifd-cards, when a card is read a color is published to the MQTT broker based on if the card is whitelisted(Green), Blacklisted(Red) or unlisted(Blue).

## Raspberry pi
[mqtt broker setup](https://pimylifeup.com/raspberry-pi-mosquitto-mqtt-server/)   
For the RFID_Publisher to work spi needs to bee enabled, and the RPi.GPIO, mfrc522 and paho-mqtt packages need to be installed for all users.   
To setup the publisher:   
Place the RFID_Publisher folder in /home/pi/, then copy rfidpub.service to /etc/systemd/system/    
and run 
```sudo systemctl daemon-reload```   
```sudo systemctl enable rfidpub.service```    
```sudo systemctl start rfidpub.service```    
Then check the status with:   
```sudo systemctl status rfidpub.service```

## Arduino Leonardo (with Ethernet shield)
The code for the Leonardo is in the ETOU_Gateway (Ethernet TO Uart) folder. It is depending on the PubSubClient library for the mqtt connection.
The only Leonardo specific code is the SoftwareSerial pins(pin 8(RX) and pin 9(TX)), if you are running a different µController then check what pins are recomended for your specific board.   
Make sure to change the MQTT_SERVER address and PORT to point to the ip and port of your mqtt broker.

## Arduino Uno (with custom shield)
The code for the Uno is in the LED_Controller folder. It has SoftwareSerial running on pin 5(RX) and 6(TX). There is an RGB-LED connected to pin 9(Blue), 10(Green) and 11(Red), a voltage divider connected to analog pin 0 and two switches connected to pin 8(Key1) and 12(Key2).  
   
The Uno has 4 different states that are switchable with Key2. In the first mode Key1 switches between the colors red, green and blue, and the pot is controlling the led brightness.   
In the second mode, the led is fading between all the colors of the rainbow, the pot is controlling the speed and if Key1 is held the pot is also controlling the brightness.   
In the third mode the pot sets the brightness of the currently selected led and the selection is switched by pressing Key1.   
And in the fourth mode, the brightness of the led is controlled by the pot and the color is set via uart.