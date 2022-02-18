#include <SoftwareSerial.h>

int RedDiodePin = 11;
int GreenDiodePin = 10;
int BlueDiodePin = 9;
int Key1Pin = 8;
int Key2Pin = 12;
int PotPin = A0;
SoftwareSerial CtrlSerial(5, 6);

int Key1State = LOW;
int Key2State = LOW;

int ledselected = 0;
int mode = 0;
bool positive_count = true;
int RGBB_Data[]= {255, 0, 0, 255}; 

void writeRGBtoLED() {
  analogWrite(RedDiodePin, 255-((RGBB_Data[0]*RGBB_Data[3])/255));
  analogWrite(GreenDiodePin, 255-((RGBB_Data[1]*RGBB_Data[3])/255));
  analogWrite(BlueDiodePin, 255-((RGBB_Data[2]*RGBB_Data[3])/255));
}

int getPotValue() {
  return map(analogRead(PotPin), 0, 1024, 0, 255);
} 

void nextLED() {
  ledselected++;
  if (ledselected > 2) {
    ledselected = 0;
  }
}

void onKey1Press() {
  if (mode == 0 || mode == 2) {
    nextLED();
  } else if (mode == 3) {
    byte out[] = {'1'};
    CtrlSerial.write(out, 1);
  }
}

void onKey1Release() {
  if (mode == 3) {
    byte out[] = {'0'};
    CtrlSerial.write(out, 1);
  }
}

void onKey1High() {
  if (mode == 1) {
    RGBB_Data[3] = getPotValue(); // set Brightness
  }
}

void onKey2Press() {
  nextMode();
}

void modeRGB() {
  RGBB_Data[0] = 0;
  RGBB_Data[1] = 0;
  RGBB_Data[2] = 0;
  RGBB_Data[ledselected] = getPotValue();
}

void modeRainbow() {
  int counter = getPotValue();
  
  if (positive_count) {
    RGBB_Data[ledselected] += counter;
    if (RGBB_Data[ledselected] >= 255) { 
      RGBB_Data[ledselected] = 255;
      nextLED();
      positive_count = false;
    }
  } else {
    RGBB_Data[ledselected] -= counter;
    if (RGBB_Data[ledselected] <= 0) {
      RGBB_Data[ledselected] = 0;
      nextLED();
      positive_count = true;
    }   
  } 
}

void modeValueControl() {
  RGBB_Data[3] = 255;
  RGBB_Data[ledselected] = getPotValue();
}

void modeUART() {
 
  if (CtrlSerial.available()) {
    byte buffer[2];
    CtrlSerial.readBytes(buffer, 2);
    switch (buffer[0]) {
      case 'R':
      RGBB_Data[0] = (int)buffer[1];
      break;
      case 'G':
      RGBB_Data[1] = (int)buffer[1];
      break;
      case 'B':
      RGBB_Data[2] = (int)buffer[1];
    } 
    Serial.write(buffer, 2);
  }
  RGBB_Data[3] = getPotValue();
}  

void operateMode() {
  switch (mode) {
    case 0:
    modeRGB();
    break;
    case 1:
    modeRainbow();
    break;
    case 2:
    modeValueControl();
    break;  
    case 3:
    modeUART();
    break;
  }
}

void nextMode() {
  mode++;
  if (mode > 3) {
    mode = 0;
  }
}

void setup() {
  pinMode(Key1Pin, INPUT);
  pinMode(Key2Pin, INPUT);

  pinMode(RedDiodePin, OUTPUT);
  pinMode(GreenDiodePin, OUTPUT);
  pinMode(BlueDiodePin, OUTPUT);
  Serial.begin(9600);
  CtrlSerial.begin(9600);
}

void loop() {
  operateMode();
  writeRGBtoLED();

  delay(50);
  int tmp = digitalRead(Key1Pin);
  if (tmp != Key1State) {
    if (tmp == HIGH) {
      onKey1Press();
    } else {
      onKey1Release(); 
    }
    Key1State = tmp;
  }
  if (Key1State == HIGH) {
    onKey1High();
  }  
  
  tmp = digitalRead(Key2Pin);
  if (tmp != Key2State) {
    if (tmp == HIGH) {
      onKey2Press();
    }
    Key2State = tmp;
  }
}
