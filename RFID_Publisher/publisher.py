#!/usr/bin/env python

import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522

reader = SimpleMFRC522()

def readCard():
    try:
        id, text = reader.read()
    except:
        print("Failed to read card")



if __name__ == '__main__':
    readCard()