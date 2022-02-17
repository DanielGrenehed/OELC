#!/usr/bin/env python
import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522

rfidreader = SimpleMFRC522()

def readCard(callback):
    try:
        print("Waiting to read")
        id, text = rfidreader.read()
        callback(id, text)
    finally:
         GPIO.cleanup()

def printDetails(id, text):
    print(id)
    print(text)

if __name__ == "__main__":
    readCard(printDetails)