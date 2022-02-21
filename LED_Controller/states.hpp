#include <SoftwareSerial.h>

#define BAUD_RATE 115200
#define SOFTWARE_SERIAL_RX 5
#define SOFTWARE_SERIAL_TX 6

byte selectedLED = 0;
byte param_1 = 0;
byte param_2 = 255;
long last_update_time = 0;

/*
  Change color selection
*/


void (*setBrightness)(byte brightness);
void (*clearColor)();
void (*setLEDColor)(byte led, byte intensity);
byte (*getCColor)(byte led);

void setStatesFunctions(void(*sb)(byte), void(*cc)(), void(*slc)(byte, byte), byte(*glc)(byte)) {
    setBrightness = sb;
    clearColor = cc;
    setLEDColor  = slc;
    getCColor = glc;
}

byte getLEDColor() {
    return getCColor(selectedLED);
}

void setSelectedColor(byte intensity) {
    setLEDColor(selectedLED, intensity);
}

void setSelected(byte led) {
  if (led >= 0 && led < 3) selectedLED = led;
  else Serial.println("Invalid LED selection!");
}

void nextLED() {
  selectedLED++;
  if (selectedLED > 2) selectedLED = 0;
}

//
// Abstract state base class
//

class State {
private:
  bool enabled = true;

public:
  virtual void onKeyPressed() {}; // Key1 pressed event
  virtual void onKeyReleased() {}; // Key1 released event
  virtual void onStart() {}; // called when state is set
  virtual void update() {}; // called every loop iteration for the current state 
  virtual void printInfo() = 0; // prints state info

  bool isEnabled() { return this->enabled; };
  void enable() { this->enabled = true; };
  void disable() { this->enabled = false; };
  void printMode();
};

/*
  Prints the mode of the current state, enabled or disabled.
*/
void State::printMode() {
  Serial.print("\tMode: ");
  Serial.println(enabled ? "Enabled" : "Disabled");
}

//
// RGB state
//

class RGB_State : public State {
public:
  RGB_State(){};
  ~RGB_State(){};
  virtual void onKeyPressed();
  virtual void update();
  virtual void printInfo();
};

void RGB_State::onKeyPressed() {
  nextLED();
}
void RGB_State::update() {
  setBrightness(param_1);
  clearColor();
  setSelectedColor(255);
}
void RGB_State::printInfo() {
  Serial.println("\tRGB ");
  printMode();
}

//
// Rainbow state
//

class Rainbow_State : public State {
private:
  bool positive_count = true;

public:
  Rainbow_State(){};
  ~Rainbow_State(){};
  virtual void update();
  virtual void printInfo();
};

void Rainbow_State::update() {
  setBrightness(param_2);
  float delta_change = ((float)param_1 / 255) * (millis() - last_update_time);
  if (delta_change < 1.0) return; // no change will happen
  else last_update_time = millis(); // reset last change

  // 3 against 2 polyrhythm
  int color = getLEDColor();
  if (this->positive_count) {
    color += delta_change;
    if (color >= 255) { // if max is reached: count down on next color
      setSelectedColor(255);
      nextLED();
      this->positive_count = false;
    } else setSelectedColor(color);
  } else {
    color -= delta_change;
    if (color <= 0) { // if min is reach: count up on next color
      setSelectedColor(0);
      nextLED();
      this->positive_count = true;
    } else setSelectedColor(color);
  }
}
void Rainbow_State::printInfo() {
  Serial.println("\tRainbow ");
  printMode();
}

//
// ValueControl state
//

class ValueControl_State : public State {
public:
  ValueControl_State(){};
  ~ValueControl_State(){};
  virtual void onKeyPressed();
  virtual void onStart();
  virtual void update();
  virtual void printInfo();
};

void ValueControl_State::onKeyPressed() {
    nextLED();
}
void ValueControl_State::onStart() {
    setBrightness(255);
}
void ValueControl_State::update() {
    setSelectedColor(param_1);
}
void ValueControl_State::printInfo() {
  Serial.println("\tValue Control ");
  printMode();
}

//
// UART state
//

class UART_State : public State {
private:
  SoftwareSerial *UART; // Arduino to Arduino serial
public:
  UART_State();
  ~UART_State(){};
  virtual void onKeyPressed();
  virtual void onKeyReleased();
  virtual void onStart();
  virtual void update();
  virtual void printInfo();
};

UART_State::UART_State() {
  this->UART = new SoftwareSerial(SOFTWARE_SERIAL_RX, SOFTWARE_SERIAL_TX);
  this->UART->begin(BAUD_RATE);
}

void UART_State::onKeyPressed() {
  byte out[] = {'1'};
  this->UART->write(out, 1); // send button state to uart
}

void UART_State::onKeyReleased() {
  byte out[] = {'0'};
  this->UART->write(out, 1); // send button state to uart
}

void UART_State::onStart() {
  clearColor();
}

void UART_State::update() {
  if (this->UART->available()) { // read and process uart input when avaliable
    byte buffer[2];
    this->UART->readBytes(buffer, 2);
    switch (buffer[0]) {
    case 'R':
        setSelected(0);
        break;
    case 'G':
        setSelected(1);
        break;
    case 'B':
        setSelected(2);
        break;
    default:
        Serial.write(buffer, 2);
        return;
    } 
    setSelectedColor(buffer[1]);
  }
  setBrightness(param_1);
}

void UART_State::printInfo() {
  Serial.println("\tUART ");
  printMode();
}

//
// State machine variables
//

class StateMachine {
private:
    byte current_state = 0;
    State *states[4];
    byte num_states = 4;
public:
    StateMachine();
    void setState(byte state);
    void nextState();
    State* currentState();
    byte stateNumber();
    void enableState(byte state);
    void disableState(byte state);
};

StateMachine::StateMachine() {
    states[0] = new RGB_State();
    states[1] = new Rainbow_State();
    states[2] = new ValueControl_State();
    states[3] = new UART_State();
}

/*
  Set, start and print state
*/
void StateMachine::setState(byte new_state) {
    if (new_state >= 0 && new_state < num_states) {
        current_state = new_state;
        states[current_state]->onStart();
        Serial.println("State changed, state: " + String(current_state));
    } else {
        Serial.println("Cannot set state to " + String(new_state) + " (0 <> "+String(num_states) + ")");
    }
}

/*
  Sets state to next enabled state
*/
void StateMachine::nextState() { // Making sure to go to next enabled state, and to not get stuck in a loop
  for (int i = 0; i < num_states; i++) {
    int potential = (current_state + i + 1) % num_states;
    if (states[potential]->isEnabled()) {
      setState(potential);
      return;
    } else Serial.println("Potential failed (" + String(potential) + ")");
  }
}

State* StateMachine::currentState() {
    return states[current_state];
}

byte StateMachine::stateNumber() {
    return current_state;
}

void StateMachine::enableState(byte state) {
    if (state >= 0 && state < num_states) states[state]->enable();
    else Serial.println("Invalid state to enable");
}

void StateMachine::disableState(byte state) {
    if (state >= 0 && state < num_states) states[state]->disable();
    else Serial.println("Invalid state to disable");
}