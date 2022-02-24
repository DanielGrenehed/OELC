#ifndef STATES_HPP
#define STATES_HPP

/*  @author Daniel Amos Grenehed

    Arduino StateMachine and States

    StateMachine handles states.
    Not a facade, current state 
    exposed with currentState().

    State holds the logic for
    the state, responding to 
    input, and parameters.
    Usually defines the 
    behavior for the LED
    when the state is active.

*/

/*
    Global state parameters
*/
byte selectedLED = 0;
byte param_1 = 0;
byte param_2 = 255;
long last_update_time = 0;

/*
    External used functions
*/
void printlnBool(bool);
void printArgumentError();
void setState(byte);
void nextState();

/*
    Callback functions for color and led interactions
*/
void (*setBrightness)(byte brightness);
void (*clearColor)();
void (*setLEDColor)(byte led, byte intensity);
byte (*getCColor)(byte led);

/*
    Set callback functions
    
    Call in setup()
*/
void setStatesFunctions(void(*sb)(byte), void(*cc)(), void(*slc)(byte, byte), byte(*glc)(byte)) {
    setBrightness = sb;
    clearColor = cc;
    setLEDColor  = slc;
    getCColor = glc;
}

/*
    Return value of selected color
*/
byte getLEDColor() {
    return getCColor(selectedLED);
}

/*
    Set value of selected color
*/
void setSelectedColor(byte intensity) {
    setLEDColor(selectedLED, intensity);
}

/*
    Set LED selection
*/
void setSelected(byte led) {
  if (led >= 0 && led < 3) selectedLED = led;
  else Serial.println(F("LEDErr!"));
}

/*
    Selects next LED
*/
void nextLED() {
  selectedLED++;
  if (selectedLED > 2) selectedLED = 0;
}


/*  Abstract state base class
*/

class State {
private:
  bool enabled = true;
public:
  virtual void onKey1Pressed() {};  // Key1 pressed event
  virtual void onKey1Released() {}; // Key1 released event
  virtual void onKey2Pressed() {};  // Key2 pressed event
  virtual void onKey2Released() {}; // Key2 released event
  virtual void onStart() {};        // called when state is set
  virtual void update() {};         // called every loop iteration for the current state 
  virtual void printInfo() = 0;     // prints state info

  bool isEnabled() { return this->enabled; };
  void enable() { this->enabled = true; };
  void disable() { this->enabled = false; };
  void printMode();
};

/*
  Prints the mode of the current state, enabled or disabled.
*/
void State::printMode() {
  Serial.print(F("\tMode: "));
  printlnBool(enabled);
}


/*  RGB state
    Toggles between colors (R, G, B)
    Brightness set with param_1
*/

class RGB_State : public State {
public:
  RGB_State(){};
  ~RGB_State(){};
  virtual void onKey1Pressed();
  virtual void onKey2Pressed();
  virtual void update();
  virtual void printInfo();
};

void RGB_State::onKey1Pressed() {
    nextLED();
}
void RGB_State::onKey2Pressed() {
    nextState();
}
void RGB_State::update() {
  setBrightness(param_1);
  clearColor();
  setSelectedColor(255);
}
void RGB_State::printInfo() {
  Serial.println(F("\tRGB"));
  printMode();
}

/*  Rainbow state
    Fades between rainbow colors
    Param_1 sets speed
    Param_2 sets brightness
*/

class Rainbow_State : public State {
private:
  bool positive_count = true;

public:
  Rainbow_State(){};
  ~Rainbow_State(){};
  virtual void onKey2Pressed();
  virtual void update();
  virtual void printInfo();
};

void Rainbow_State::onKey2Pressed() {
    nextState();
}

/*
    Fade between colors

    Looking att RGB values when sliding a hue bar
    reveals that there is only two colors active 
    at once and the counter is alternating between 
    adding to a color until it's max value is reached
    and then subtracting from the next color value until
    the min value is reached.
    There are 3 colors and 6 blend combinations, but using
    a boolean(positive_count) i made a simple if-else
    statement to handle the change of color and achieve 
    a fading rainbow.
*/
void Rainbow_State::update() {
  setBrightness(param_2);
  float delta_change = ((float)param_1 / 255) * (millis() - last_update_time);
  if (delta_change < 1.0) return;   // no change will happen
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
  Serial.println(F("\tRainbow"));
  printMode();
}

/*  ValueControl state
    Toggles between colors 
    Param_1 sets the current colors value
*/

class ValueControl_State : public State {
public:
  ValueControl_State(){};
  ~ValueControl_State(){};
  virtual void onKey1Pressed();
  virtual void onKey2Pressed();
  virtual void onStart();
  virtual void update();
  virtual void printInfo();
};

void ValueControl_State::onKey1Pressed() {
    nextLED();
}

void ValueControl_State::onKey2Pressed() {
    nextState();
}

void ValueControl_State::onStart() {
    setBrightness(255);
}
void ValueControl_State::update() {
    setSelectedColor(param_1);
}
void ValueControl_State::printInfo() {
  Serial.println(F("\tValue Control"));
  printMode();
}

#include <SoftwareSerial.h> // For UART state, Arduino to Arduino

#define BAUD_RATE 115200
#define SOFTWARE_SERIAL_RX 5
#define SOFTWARE_SERIAL_TX 6

/*  UART state
    Listens to software serial(UART)
    Sets color value when a (ColorByte)(ValueByte)
    message is sent.
    Key1 sends events to UART.
*/

class UART_State : public State {
private:
  SoftwareSerial *UART; // Arduino to Arduino serial
public:
  UART_State();
  ~UART_State(){};
  virtual void onKey1Pressed();
  virtual void onKey1Released();
  virtual void onKey2Pressed();
  virtual void onStart();
  virtual void update();
  virtual void printInfo();
};

/*
    Sets up the softwareSerial UART
*/
UART_State::UART_State() {
  this->UART = new SoftwareSerial(SOFTWARE_SERIAL_RX, SOFTWARE_SERIAL_TX);
  this->UART->begin(BAUD_RATE);
}

void UART_State::onKey1Pressed() {
  byte out[] = {'1'};
  this->UART->write(out, 1); // send button state to uart
}

void UART_State::onKey1Released() {
  byte out[] = {'0'};
  this->UART->write(out, 1); // send button state to uart
}

void UART_State::onKey2Pressed() {
    nextState();
}

void UART_State::onStart() {
  clearColor();
}

/*
    Responds to serial commands
*/
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
  Serial.println(F("\tUART"));
  printMode();
}

/*  State machine variables
    Handles states
*/

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
    Set, start and print state.

    Does not account for a state
    being disabled!
*/
void StateMachine::setState(byte new_state) {
    if (new_state >= 0 && new_state < num_states) {
        current_state = new_state;
        states[current_state]->onStart();
        Serial.print(F("State: "));
        Serial.println(current_state);
    } else {
        printArgumentError();
        Serial.print(new_state);
        Serial.print(F(" (0 <> "));
        Serial.print(num_states);
        Serial.println(F(")"));
    }
}

/*
  Sets state to next enabled state
*/
void StateMachine::nextState() { 
    for (int i = 0; i < num_states; i++) {
        // Making sure to go to next enabled state,
        // and to not get stuck in a loop
        int potential = (current_state + i + 1) % num_states;
        if (states[potential]->isEnabled()) {
        setState(potential);
        return;
        } else {
            Serial.print(F("State disabled "));
            Serial.println(potential);}
    }
}

/*
    Returns pointer to current state
*/
State* StateMachine::currentState() {
    return states[current_state];
}

/*
    Returns the array position of current state
*/
byte StateMachine::stateNumber() {
    return current_state;
}

/*
    Enables state to be set
*/
void StateMachine::enableState(byte state) {
    if (state >= 0 && state < num_states) states[state]->enable();
    else printArgumentError();
}

/*
    Disables state from being set
*/
void StateMachine::disableState(byte state) {
    if (state >= 0 && state < num_states) states[state]->disable();
    else printArgumentError();
}

#endif /* ifndef STATES_HPP */