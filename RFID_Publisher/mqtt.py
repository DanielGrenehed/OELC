#!/usr/bin/env python
import paho.mqtt.client as mqttClient

broker = "localhost"
port = 1883
mqtt_c = mqttClient.Client("rfid_publisher")
mqtt_connected = False

def clear_level():
    mqtt_c.publish("LED/R", "0")
    mqtt_c.publish("LED/G", "0")
    mqtt_c.publish("LED/B", "0")
    print("clr")

def on_broker_connected(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        global mqtt_connected
        mqtt_connected = True
    else:
        print("Failed to connect to broker")

def on_level_0():
    clear_level()
    mqtt_c.publish("LED/R", "255")
    print("l0")

def on_level_1():
    clear_level()
    mqtt_c.publish("LED/B", "255")
    print("l1")

def on_level_2():  
    clear_level()
    mqtt_c.publish("LED/G", "255")
    print("l2")

def publish_id(id):
    mqtt_c.publish("local/rfid/id", id)

def start():
    mqtt_c.on_connected = on_broker_connected
    mqtt_c.connect(broker)
    mqtt_c.loop_start()

def stop():
    mqtt_c.disconnect()
    mqtt_c.loop_stop()

    global mqtt_connected
    mqtt_connected = False

def is_connected():
    return mqtt_connected

