#include <SoftwareSerial.h>

#define BAUD_RATE 9600
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
byte PotValue = 0;

byte ledselected = 0;


byte param_1 = 0;
byte param_2 = 255;
bool positive_count = true;

int RGBB_Data[]= {255, 0, 0, 255}; // RED, GREEN, BLUE, BRIGHTNESS
long last_update_time = 0;

void setBrightness(byte brightness) {
  RGBB_Data[3] = brightness;
}
void clearRGB() {
  RGBB_Data[0] = 0;
  RGBB_Data[1] = 0;
  RGBB_Data[2] = 0;
}

SoftwareSerial CtrlSerial(5, 6);

void nextLED() {
  ledselected++;
  if (ledselected > 2) ledselected = 0;
 
 /*  if (mode == 0) {
    clearRGB();
    RGBB_Data[ledselected] = 255;
  } */
}

class State {
public:
  virtual void onKeyPressed() = 0;
  virtual void onKeyReleased() = 0;
  virtual void onStart() = 0;
  virtual void update() = 0;
};

class RGB_State: public State {
public:
  RGB_State() {};
  ~RGB_State() {};
  virtual void onKeyPressed();
  virtual void onKeyReleased();
  virtual void onStart();
  virtual void update();
};

void RGB_State::onKeyPressed() {
  nextLED();
  clearRGB();
  RGBB_Data[ledselected] = 255;
}
void RGB_State::onKeyReleased() {}
void RGB_State::onStart() {
  clearRGB();
  RGBB_Data[ledselected] = 255;
}
void RGB_State::update() {
  setBrightness(param_1);
}

class Rainbow_State: public State {
private:
  bool positive_count = true;
public:
  Rainbow_State() {};
  ~Rainbow_State() {};
  virtual void onKeyPressed();
  virtual void onKeyReleased();
  virtual void onStart();
  virtual void update();
};

void Rainbow_State::onKeyPressed() {}
void Rainbow_State::onKeyReleased() {}
void Rainbow_State::onStart() {}
void Rainbow_State::update() {
  setBrightness(param_2);
  float delta_change = ((float)param_1 / 255) * (millis() - last_update_time);
  if (delta_change < 1.0) {
    return;
  } else {
    last_update_time = millis();  
  }
  
  if (this->positive_count) {
    RGBB_Data[ledselected] += delta_change;
    if (RGBB_Data[ledselected] >= 255) { 
      RGBB_Data[ledselected] = 255;
      nextLED();
      this->positive_count = false;
    }
  } else {
    RGBB_Data[ledselected] -= delta_change;
    if (RGBB_Data[ledselected] <= 0) {
      RGBB_Data[ledselected] = 0;
      nextLED();
      this->positive_count = true;
    }   
  }
}

class ValueControl_State: public State {
public:
  ValueControl_State() {};
  ~ValueControl_State() {};
  virtual void onKeyPressed();
  virtual void onKeyReleased();
  virtual void onStart();
  virtual void update();
};

void ValueControl_State::onKeyPressed() {
  nextLED();
}
void ValueControl_State::onKeyReleased() {}
void ValueControl_State::onStart() {
  setBrightness(255);
}
void ValueControl_State::update() {
  RGBB_Data[ledselected] = param_1;
}

class UART_State: public State {
public:
  UART_State() {};
  ~UART_State() {};
  virtual void onKeyPressed();
  virtual void onKeyReleased();
  virtual void onStart();
  virtual void update();
};

void UART_State::onKeyPressed() {}
void UART_State::onKeyReleased() {}
void UART_State::onStart() {
  clearRGB();
}
void UART_State::update() {
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
  setBrightness(param_1);
}

byte mode = 0;
State* states[] = {new RGB_State(), new Rainbow_State(), new ValueControl_State(), new UART_State()}; 




byte calculateBrightness(int value) {
  return (value*RGBB_Data[3]) / 255;
}



void writeRGBtoLED() {
  analogWrite(RedDiodePin, 255-calculateBrightness(RGBB_Data[0]));
  analogWrite(GreenDiodePin, 255-calculateBrightness(RGBB_Data[1])); 
  analogWrite(BlueDiodePin, 255-calculateBrightness(RGBB_Data[2]));
}



//
// Input handling
//

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

void onKey1Press() {
  //modes[mode].buttonPressed();
  states[mode]->onKeyPressed();
  /* if (mode == 0 || mode == 2) {
    nextLED();
  } else if (mode == 3) {
    byte out[] = {'1'};
    CtrlSerial.write(out, 1);
  } */
}

void onKey1Release() {
  //modes[mode].buttonReleased();
  states[mode]->onKeyReleased();
 /* if (mode == 3) {
    byte out[] = {'0'};
    CtrlSerial.write(out, 1);
  } */
}

void onKey2Press() {
  nextMode();
}

void onPotValueChanged(byte new_value) {
  if (Key1State == HIGH) param_2 = new_value;
  param_1 = new_value;
}

//
// RGB mode
//

void onModeRGBSet() {
    clearRGB();
    RGBB_Data[ledselected] = 255;
}

void modeRGB() {
  setBrightness(param_1);
}

//
// Rainbow mode
//

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

//
// Value control mode
//

void onModeValueControlSet() {
  setBrightness(255);
}

void modeValueControl() {
  RGBB_Data[ledselected] = param_1;
}


//
// UART control mode
//

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
  setBrightness(param_1);
}  

//
// Handle state(modes)
//

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
  states[mode]->onStart();
  /* switch (mode) {
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
  }  */
}

//
// USB Serial
//

void handleSerial() {
  if (Serial.available()) {
    
  }
}




//
// Arduino setup and loop
//

void setup() {
  // Set key pins as inputs
  pinMode(Key1Pin, INPUT);
  pinMode(Key2Pin, INPUT);

  // Add key interrupts
  pinMode(KEY_1_INTERRUPT_PIN, INPUT); 
  pinMode(KEY_2_INTERRUPT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(KEY_1_INTERRUPT_PIN), Key1Handle, CHANGE);
  attachInterrupt(digitalPinToInterrupt(KEY_2_INTERRUPT_PIN), Key2Handle, CHANGE);

  // set LED pins to output
  pinMode(RedDiodePin, OUTPUT);
  pinMode(GreenDiodePin, OUTPUT);
  pinMode(BlueDiodePin, OUTPUT);
  
  // start comms
  Serial.begin(BAUD_RATE);
  CtrlSerial.begin(BAUD_RATE);
}

void loop() {
  
  states[mode]->update();
  //operateMode();
  writeRGBtoLED();
  handleSerial();
  
  byte new_pot_value = map(analogRead(PotPin), 0, 1024, 0, 255);
  if (new_pot_value != PotValue) {
    onPotValueChanged(new_pot_value);
    PotValue = new_pot_value; 
  }
}
