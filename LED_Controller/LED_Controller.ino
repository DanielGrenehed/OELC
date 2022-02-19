#include <SoftwareSerial.h>

#define KEY_1_INTERRUPT_PIN 2
#define KEY_2_INTERRUPT_PIN 3
byte Key1Pin = 8;
byte Key2Pin = 12;

byte PotPin = A0;

byte RedDiodePin = 11;
byte GreenDiodePin = 10;
byte BlueDiodePin = 9;

int Key1State = LOW;
int Key2State = LOW;
int PotValue = 0;

byte ledselected = 0;
byte mode = 0;
byte param_1 = 0;
byte param_2 = 255;
bool positive_count = true;

int RGBB_Data[]= {255, 0, 0, 255}; // RED, GREEN, BLUE, BRIGHTNESS
long last_update_time = 0;
SoftwareSerial CtrlSerial(5, 6);

void setBrightness(byte brightness) {
  RGBB_Data[3] = brightness;
}

byte calculateBrightness(int value) {
  return (value*RGBB_Data[3]) / 255;
}

void clearRGB() {
  RGBB_Data[0] = 0;
  RGBB_Data[1] = 0;
  RGBB_Data[2] = 0;
}

void writeRGBtoLED() {
  analogWrite(RedDiodePin, 255-calculateBrightness(RGBB_Data[0]));
  analogWrite(GreenDiodePin, 255-calculateBrightness(RGBB_Data[1])); 
  analogWrite(BlueDiodePin, 255-calculateBrightness(RGBB_Data[2]));
}

int getPotValue() {
  return map(analogRead(PotPin), 0, 1024, 0, 255);
} 

void nextLED() {
  ledselected++;
  if (ledselected > 2) ledselected = 0;
 
  if (mode == 0) {
    clearRGB();
    RGBB_Data[ledselected] = 255;
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

void onKey2Press() {
  nextMode();
}

void onPotValueChanged(byte new_value) {
  if (Key1State == HIGH) param_2 = new_value;
  else param_1 = new_value;
}

void onModeRGBSet() {
    clearRGB();
    RGBB_Data[ledselected] = 255;
}

void modeRGB() {
  setBrightness((byte)PotValue);
}

void onModeRainbowSet() {}

void modeRainbow() {
  setBrightness(param_2);
  float delta_change = ((float)param_1 / 255) * (millis() - last_update_time);
  if (delta_change < 1.0) {
    return;
  } else {
    last_update_time = millis();  
  }
  
  if (positive_count) {
    RGBB_Data[ledselected] += delta_change;
    if (RGBB_Data[ledselected] >= 255) { 
      RGBB_Data[ledselected] = 255;
      nextLED();
      positive_count = false;
    }
  } else {
    RGBB_Data[ledselected] -= delta_change;
    if (RGBB_Data[ledselected] <= 0) {
      RGBB_Data[ledselected] = 0;
      nextLED();
      positive_count = true;
    }   
  } 
}

void onModeValueControlSet() {
  setBrightness(255);
}

void modeValueControl() {
  // if pot changed
  RGBB_Data[ledselected] = PotValue;
}

void onModeUARTSet() {
  clearRGB();
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
      break;
    } 
    Serial.write(buffer, 2);
  }
  setBrightness((byte)PotValue);
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
  switch (mode) {
    case 0:
    onModeRGBSet();
    break;
    case 1:
    onModeRainbowSet();
    break;
    case 2:
    onModeValueControlSet();
    break;
    case 3:
    onModeUARTSet();
    break;
  }
}

void Key1Handle() {
  int tmp = digitalRead(Key1Pin);
  if (tmp != Key1State) {
    if (tmp == HIGH) {
      onKey1Press();
      Serial.println("Key1 pressed");
    } else {
      onKey1Release(); 
    }
    Key1State = tmp;
  }
}

void Key2Handle() {
  int tmp = digitalRead(Key2Pin);
  if (tmp != Key2State) {
    if (tmp == HIGH) {
      onKey2Press();
      Serial.println("Key2 pressed");
    }
    Key2State = tmp;
  } 
}

void handleSerial() {}

void setup() {
  pinMode(Key1Pin, INPUT);
  pinMode(Key2Pin, INPUT);
  
  pinMode(KEY_1_INTERRUPT_PIN, INPUT); 
  pinMode(KEY_2_INTERRUPT_PIN, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(KEY_1_INTERRUPT_PIN), Key1Handle, CHANGE);
  attachInterrupt(digitalPinToInterrupt(KEY_2_INTERRUPT_PIN), Key2Handle, CHANGE);

  pinMode(RedDiodePin, OUTPUT);
  pinMode(GreenDiodePin, OUTPUT);
  pinMode(BlueDiodePin, OUTPUT);
  
  Serial.begin(9600);
  CtrlSerial.begin(9600);
}

void loop() {
  operateMode();
  writeRGBtoLED();
  handleSerial();
  int tmp = getPotValue();
  if (tmp != PotValue) {
    onPotValueChanged((byte)tmp);
    PotValue = tmp;
  }
}
