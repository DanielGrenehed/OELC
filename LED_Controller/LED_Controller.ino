#include <SoftwareSerial.h>

#define BAUD_RATE 9600
#define SOFTWARE_SERIAL_RX 5
#define SOFTWARE_SERIAL_TX 6

#define RED RGBB_Data[0]
#define GREEN RGBB_Data[1]
#define BLUE RGBB_Data[2]
#define STATE states[current_state]
#define SEL_COLOR RGBB_Data[ledselected]
 
#define KEY_1_INTERRUPT_PIN 2
#define KEY_2_INTERRUPT_PIN 3
const byte Key1Pin = 8;
const byte Key2Pin = 12;

const byte PotPin = A0;

const byte RedDiodePin = 11;
const byte GreenDiodePin = 10;
const byte BlueDiodePin = 9;

// 
int Key1State = LOW;
int Key2State = LOW;
byte PotValue = 0;

// State parameters
byte ledselected = 0;
byte param_1 = 0;
byte param_2 = 255;


int RGBB_Data[]= {255, 0, 0, 255}; // RED, GREEN, BLUE, BRIGHTNESS
long last_update_time = 0;

void setBrightness(byte brightness) {
  RGBB_Data[3] = brightness;
}

/*
  Clear color values
*/
void clearRGB() {
  RED = 0; // clear red
  GREEN = 0; // clear green
  BLUE = 0; // clear blue
}

/*
  Change color selection
*/
void nextLED() { 
  ledselected++;
  if (ledselected > 2) ledselected = 0;
}

//
// Abstract state base class
//

class State {
public:
  virtual void onKeyPressed() = 0;
  virtual void onKeyReleased() = 0;
  virtual void onStart() = 0;
  virtual void update() = 0;
};

//
// RGB state
//

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
  SEL_COLOR = 255;
}
void RGB_State::onKeyReleased() {}
void RGB_State::onStart() {
  clearRGB();
  SEL_COLOR = 255;
}
void RGB_State::update() {
  setBrightness(param_1);
}

//
// Rainbow state
//

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
   SEL_COLOR += delta_change;
    if (SEL_COLOR >= 255) { 
      SEL_COLOR = 255;
      nextLED();
      this->positive_count = false;
    }
  } else {
    SEL_COLOR -= delta_change;
    if (SEL_COLOR <= 0) {
      SEL_COLOR = 0;
      nextLED();
      this->positive_count = true;
    }   
  }
}

//
// ValueControl state
//

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
  SEL_COLOR = param_1;
}

//
// UART state
//

class UART_State: public State {
private:
  SoftwareSerial* UART;
public:
  UART_State();
  ~UART_State() {};
  virtual void onKeyPressed();
  virtual void onKeyReleased();
  virtual void onStart();
  virtual void update();
};

UART_State::UART_State() {
  this->UART = new SoftwareSerial(SOFTWARE_SERIAL_RX, SOFTWARE_SERIAL_TX);
  this->UART->begin(BAUD_RATE);
}

void UART_State::onKeyPressed() {
  byte out[] = {'1'};
  this->UART->write(out, 1);
}

void UART_State::onKeyReleased() {
  byte out[] = {'0'};
  this->UART->write(out, 1);
}

void UART_State::onStart() {
  clearRGB();
}

void UART_State::update() {
  if (this->UART->available()) {
    byte buffer[2];
    this->UART->readBytes(buffer, 2);
    switch (buffer[0]) {
      case 'R':
      RED = (int)buffer[1];
      break;
      case 'G':
      GREEN = (int)buffer[1];
      break;
      case 'B':
      BLUE = (int)buffer[1];
      break;
    } 
    Serial.write(buffer, 2);
  }
  setBrightness(param_1);
}

// State machine variables
byte current_state = 0;
State* states[] = {new RGB_State(), new Rainbow_State(), new ValueControl_State(), new UART_State()}; 

/*
  Returns the final brightnes of led calculated by general brightness and the value of the LED color.
*/
byte calculateBrightness(int value) {
  return (value*RGBB_Data[3]) / 255;
}

/*
  Set the color of the LED.
*/
void writeRGBtoLED() {
  analogWrite(RedDiodePin, 255-calculateBrightness(RED));
  analogWrite(GreenDiodePin, 255-calculateBrightness(GREEN)); 
  analogWrite(BlueDiodePin, 255-calculateBrightness(BLUE));
}

// 
// Input handling
//

void Key1Handle() {
  int tmp = digitalRead(Key1Pin);
  if (tmp != Key1State) {
    if (tmp == HIGH) {
      STATE->onKeyPressed();
    } else {
      STATE->onKeyReleased(); 
    }
    Key1State = tmp;
  }
}

void Key2Handle() {
  int tmp = digitalRead(Key2Pin);
  if (tmp != Key2State) {
    if (tmp == HIGH) {
      onKey2Press();
    }
    Key2State = tmp;
  } 
}

void onKey2Press() {
  nextState();
  Serial.println("State changed, state: " + String(current_state)); 
}

void onPotValueChanged(byte new_value) {
  if (Key1State == HIGH) param_2 = new_value;
  param_1 = new_value;
}

void nextState() {
  current_state++;
  if (current_state > 3) {
    current_state = 0;
  }
  STATE->onStart();
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
}

void loop() {
  
  STATE->update();
  writeRGBtoLED();
  handleSerial();
  
  byte new_pot_value = map(analogRead(PotPin), 0, 1024, 0, 255);
  if (new_pot_value != PotValue) {
    onPotValueChanged(new_pot_value);
    PotValue = new_pot_value; 
  }
}
