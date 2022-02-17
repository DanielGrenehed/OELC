#!/usr/bin/env python
import pathlib
import time
import rfid
import mqtt

relative_path = str(pathlib.Path(__file__).parent.resolve()) + "/"

def arrayFromFile(filename):
    array = []
    with open(filename) as file:
        for line in file:
            line = line.rstrip().lstrip()
            if line != "":
                array.append(line)
    return array

whitelist = []
blacklist = []

def on_card_read(id, text):
    print(id)
    if str(id) in blacklist:
        on_blacklisted_card(id, text)
    elif str(id) in whitelist:
        on_whitelisted_card(id, text)
    else:
        on_unlisted_card(id, text)
    time.sleep(3)
    mqtt.clear_level()

def on_whitelisted_card(id, text):
    mqtt.on_level_2()

def on_blacklisted_card(id, text):
    mqtt.on_level_0()

def on_unlisted_card(id, text):
    mqtt.publish_id(id)
    mqtt.on_level_1()

if __name__ == '__main__': 
    mqtt.start()
    try:
        whitelist = arrayFromFile(relative_path + "whitelist")
        blacklist = arrayFromFile(relative_path + "blacklist")

        print("Starting reader")
        while True:
            rfid.readCard(on_card_read)
            time.sleep(2)
    except Exception as e:
        print(e)
        mqtt.stop()
        exit()