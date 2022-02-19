#include <SoftwareSerial.h>

#define BAUD_RATE 9600
#define SOFTWARE_SERIAL_RX 5
#define SOFTWARE_SERIAL_TX 6

#define RED RGBB_Data[0]
#define GREEN RGBB_Data[1]
#define BLUE RGBB_Data[2]
#define SEL_COLOR RGBB_Data[selectedLED]
#define STATE states[current_state]
 
#define KEY_1_INTERRUPT_PIN 2
#define KEY_2_INTERRUPT_PIN 3
#define KEY_1_PIN 8
#define KEY_2_PIN 12

#define POT_PIN A0

#define RED_DIODE_PIN 11
#define GREEN_DIODE_PIN 10
#define BLUE_DIODE_PIN 9

// 
int Key1State = LOW;
int Key2State = LOW;
byte PotValue = 0;

// State parameters
byte selectedLED = 0;
byte param_1 = 0;
byte param_2 = 255;

byte current_state = 0;

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
  selectedLED++;
  if (selectedLED > 2) selectedLED = 0;
}

void printEnabledOrDisabled(bool enabled) {
  Serial.print("\tMode: ");
  Serial.println(enabled ? "Enabled" : "Disabled");
}

struct Task {
  long duration;
  uint8_t state;
  uint8_t param_1;
  uint8_t param_2;
  uint8_t selection;
};


#define MAX_TASKS 30
class Scheduler {
private:
  Task tasks[MAX_TASKS]; // Scheduled tasks
  int task_count = 0; // Number of tasks in schedule
  int current_task = 0; 
  long task_start_time = 0; // Time measured when task started
  bool running = false;
public:
  Scheduler();
  void printSchedule();
  void printStatus();
  void addTask(Task task);
  void removeTask(int at);
  void moveTask(int at, int to);
  void updateTask(int at, Task task);
  void start();
  void stop();
  bool isRunning();
};

Scheduler::Scheduler() {}

void Scheduler::printSchedule() {
  // print order of tasks in schedule
  Serial.println("Schedule:");
  for (int i = 0; i < this->task_count; i++) {
    Serial.println("\t" + String(i) + ": State_" + String(this->tasks[i].state) + " for " + String(this->tasks[i].duration) + "ms, p1: " + String(this->tasks[i].param_1) + ", p2: " + String(this->tasks[i].param_2) + ", s: " + String(this->tasks[i].selection));
  }
}
void Scheduler::printStatus() {
  // print status of scheduler and current task
  Serial.print("\tRunning: ");
  Serial.println(this->running ? "true" : "false");
}

void Scheduler::addTask(Task task) {}
void Scheduler::removeTask(int at) {}
void Scheduler::moveTask(int from, int to) {}
void Scheduler::updateTask(int at, Task task) {}
void Scheduler::start() {
  this->running = true;
}
void Scheduler::stop() {
  this->running = false;
}
bool Scheduler::isRunning() {
  return this->running;
}

//
// Abstract state base class
//

class State {
private:
  bool enabled = true;
public:
  virtual void onKeyPressed() = 0;
  virtual void onKeyReleased() = 0;
  virtual void onStart() = 0;
  virtual void update() = 0;
  virtual void printInfo() = 0;
    
  bool isEnabled() { return this->enabled; };
  void enable() { this->enabled = true; };
  void disable() { this->enabled = false; };
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
  virtual void printInfo();

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
void RGB_State::printInfo() {
  Serial.println("\tRGB ");
  printEnabledOrDisabled(this->isEnabled());

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
  virtual void printInfo();
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
void Rainbow_State::printInfo() {
  Serial.println("\tRainbow ");
  printEnabledOrDisabled(this->isEnabled());
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
  virtual void printInfo();
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
void ValueControl_State::printInfo() {
  Serial.println("\tValue Control ");
  printEnabledOrDisabled(this->isEnabled());
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
  virtual void printInfo();
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

void UART_State::printInfo() {
  Serial.println("\tUART ");
  printEnabledOrDisabled(this->isEnabled());
  
}

// State machine variables

const byte Num_States = 4; 
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
void setLEDColor() {
  analogWrite(RED_DIODE_PIN, 255-calculateBrightness(RED));
  analogWrite(GREEN_DIODE_PIN, 255-calculateBrightness(GREEN)); 
  analogWrite(BLUE_DIODE_PIN, 255-calculateBrightness(BLUE));
}

// 
// Input handling
//

void Key1Handle() {
  int tmp = digitalRead(KEY_1_PIN);
  if (tmp != Key1State) {
    if (tmp == HIGH) STATE->onKeyPressed();
    else STATE->onKeyReleased(); 
    Key1State = tmp;
  }
}

void Key2Handle() {
  int tmp = digitalRead(KEY_2_PIN);
  if (tmp != Key2State) {
    if (tmp == HIGH) onKey2Press();
    Key2State = tmp;
  } 
}

void onKey2Press() {
  nextState(); 
}

void PotHandle() {
  byte new_pot_value = map(analogRead(POT_PIN), 0, 1024, 0, 255);
  if (new_pot_value != PotValue) {
    onPotValueChanged(new_pot_value);
    PotValue = new_pot_value; 
  }
}

void onPotValueChanged(byte new_value) {
  if (Key1State == HIGH) param_2 = new_value;
  param_1 = new_value;
}

bool setState(byte new_state) {
  if (new_state < Num_States) {
    if (states[new_state]->isEnabled()) {
      current_state = new_state;
      STATE->onStart();  
      return true;
    }
  }
  return false;
}

void nextState() { // Making sure to go to next enabled state, and to not get stuck in a loop
  for (int i = 0; i < Num_States; i++) { 
    int potential = (current_state + i+1) % Num_States;
    if (setState(potential)) {
      Serial.println("State changed, state: " + String(current_state));
      return;
    }
    Serial.println("Potential failed (" + String(potential) + ")");
  }
}

//
// USB Serial
//

void handleSerial() {
  if (Serial.available()) {
    char buffer[256];
    int input_length = Serial.readBytes(buffer, 256);
    buffer[input_length] = 0;
    Serial.write(buffer, input_length);
    processCommands(buffer, input_length);
  }
}

void processCommands(char* input, int len) {
  if (input[0] == 'c' && input[1] == 's') {
    Serial.println("State " + String(current_state) + ":");
    STATE->printInfo();
    Serial.println("\tParam_1: " + String(param_1));
    Serial.println("\tParam_2: " + String(param_2));
    Serial.println("\tSelected_LED: " + String(selectedLED));
  }
  if (input[0] == 'c' && input[1] == 'c') {
    Serial.println("Color:");
    Serial.println("\tRed: " + String(RED));
    Serial.println("\tGreen: " + String(GREEN));
    Serial.println("\tBlue: " + String(BLUE));
    Serial.println("\tBrightness: " + String(RGBB_Data[3]));
  }
  if (input[0] == 'n' && input[1] == 's') {
    nextState();
  }
}

Scheduler scheduler();
//
// Arduino setup and loop
//

void setup() {
  // Set key pins as inputs
  pinMode(KEY_1_PIN, INPUT);
  pinMode(KEY_2_PIN, INPUT);
  // Add key interrupts
  pinMode(KEY_1_INTERRUPT_PIN, INPUT); 
  pinMode(KEY_2_INTERRUPT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(KEY_1_INTERRUPT_PIN), Key1Handle, CHANGE);
  attachInterrupt(digitalPinToInterrupt(KEY_2_INTERRUPT_PIN), Key2Handle, CHANGE);

  // set LED pins to output
  pinMode(RED_DIODE_PIN, OUTPUT);
  pinMode(GREEN_DIODE_PIN, OUTPUT);
  pinMode(BLUE_DIODE_PIN, OUTPUT);
  
  // start comms
  Serial.begin(BAUD_RATE);
}

void loop() {
  // handle scheduler

  PotHandle();
  STATE->update();
  setLEDColor();
  handleSerial();
}
