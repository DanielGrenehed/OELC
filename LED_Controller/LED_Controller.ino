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

int RGBB_Data[] = {255, 0, 0, 255}; // RED, GREEN, BLUE, BRIGHTNESS
long last_update_time = 0;

void setBrightness(byte brightness) {
  RGBB_Data[3] = brightness;
}

/*
  Returns first non-whitespace character of string
*/
int getStringStart(const char *string, int str_len) {
  for (int i = 0; i < str_len; i++) if (string[i] != ' ' && string[i] != '\t') return i;
  return -1;
}

/*
  returns the position of the last character of substring in string if the substring is in string
  else -1 is returned 
*/
int startsWith(const char *string, int str_len, const char *substring) {
  int string_start = getStringStart(string, str_len);
  int sub_len = strlen(substring);
  int i = 0;
  for (; i < sub_len; i++) {
    if (string_start + i >= str_len) return -1;
    if (substring[i] != string[string_start + i]) return -1;
  }
  return i;
}

/*
  Returns the position of the first non-numeric character in string, -1 if no numeric character was found
*/
int endOfNumber(const char *string, int str_len) {
  int start = getStringStart(string, str_len);
  for (int i = start; i < str_len; i++) if (string[i] < 48 || string[i] > 57) return i;
  return -1;
}

/*
  Clear color values
*/
void clearRGB() {
  RED = 0;
  GREEN = 0;
  BLUE = 0;
}

/*
  Change color selection
*/
void nextLED() {
  selectedLED++;
  if (selectedLED > 2) selectedLED = 0;
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
  int task_count = 0;    // Number of tasks in schedule
  int current_task = 0;
  long task_start_time = 0; // Time measured when task started
  bool running = false;
  void startTask();
  void nextTask();
public:
  Scheduler(){};
  ~Scheduler(){};
  void printSchedule();
  void printStatus();
  void printIndexError(int index);
  void addTask(Task task);
  void removeTask(int index);
  void moveTask(int index, int to);
  void updateTask(int index, Task task);
  void start();
  void stop();
  bool isRunning();
  void run();
};

/*
 print order of tasks in schedule
*/
void Scheduler::printSchedule() {
 
  Serial.println("Schedule:");
  if (task_count <= 0) Serial.println("\tNo tasks");
  for (int i = 0; i < this->task_count; i++) Serial.println("\t" + String(i) + ": State_" + String(this->tasks[i].state) + " for " + String(this->tasks[i].duration) + "ms, p1: " + String(this->tasks[i].param_1) + ", p2: " + String(this->tasks[i].param_2) + ", s: " + String(this->tasks[i].selection));
} 

/* 
 print status of scheduler and current task
*/
void Scheduler::printStatus() {
  Serial.println("Scheduler:");
  Serial.print("\tRunning: ");
  Serial.println(this->running ? "true" : "false");
  Serial.println("\t" + String(task_count) + " tasks");
  if (task_count > 0) {
    Serial.println("\tTask: " + String(current_task));
    Serial.println("\tFor: " + String(millis() - task_start_time) + "ms");
    Serial.println("\tState_" + String(this->tasks[current_task].state) + " for " + String(this->tasks[current_task].duration) + "ms, p1: " + String(this->tasks[current_task].param_1) + ", p2: " + String(this->tasks[current_task].param_2) + ", s: " + String(this->tasks[current_task].selection));
  }
}

/*
  Print task indexing errors
*/
void Scheduler::printIndexError(int index) {
  if (index >= task_count) Serial.println("Index out of range: " + String(index) + ", Task-count: " + String(task_count));
  if (index < 0) Serial.println("Illegal use of negative index: " + String(index));
}

/*
  Appends task to schedule
*/
void Scheduler::addTask(Task task) {
  if (this->task_count + 1 < MAX_TASKS) {
    this->tasks[this->task_count++] = task;
  } else Serial.println("Failed to add task, task limit reached!");
}

/*
  Remove task at index from schedule
*/
void Scheduler::removeTask(int index) {
  if (index < task_count && index >= 0) {
    for (int i = index; i < task_count; i++) {
      tasks[i] = tasks[i + 1];
    }
    task_count--;
  }
  else printIndexError(index);
}

/*
  Move task in schedule // change to move and not excange x with y like it is now
*/
void Scheduler::moveTask(int from, int to) {
  if (from < task_count && from >= 0 && to < task_count && to >= 0) {
    Task temp = tasks[to];
    tasks[to] = tasks[from];
    tasks[from] = temp;
  }
  else {
    printIndexError(from);
    printIndexError(to);
  }
}

/*
  Sets task at index in schedule to a new task
*/
void Scheduler::updateTask(int index, Task task) {
  if (index < task_count && index >= 0)
    tasks[index] = task;
  else
    printIndexError(index);
}

/*
  Tries to start schedule
*/
void Scheduler::start() {
  if (task_count > 0) this->running = true;
  else Serial.println("Scheduler cannot start without tasks!");
}
/*
  Stops the schedule from running
*/
void Scheduler::stop() {
  this->running = false;
}

/*
  Returns wether or not the schedule is running
*/
bool Scheduler::isRunning() {
  return this->running;
}

/*
  Do all scheduler handling
*/
void Scheduler::run() {
  if (!isRunning()) return;
  if (task_start_time == 0) startTask();
  if (millis() - task_start_time >= tasks[current_task].duration) nextTask();
}
void setState(byte new_state);

/*
  Starts the current task
*/
void Scheduler::startTask() {
  setState(tasks[current_task].state);
  param_1 = tasks[current_task].param_1;
  param_2 = tasks[current_task].param_2;
  selectedLED = tasks[current_task].selection;
  task_start_time = millis();
}

/*
  Change to next task and start it 
*/
void Scheduler::nextTask() {
  current_task++;
  if (current_task >= task_count) current_task = 0;
  startTask();
}

Scheduler *scheduler;

//
// Abstract state base class
//

class State {
private:
  bool enabled = true;

public:
  virtual void onKeyPressed() = 0; // Key1 pressed event
  virtual void onKeyReleased() = 0; // Key1 released event
  virtual void onStart() = 0; // called when state is set
  virtual void update() = 0; // called every loop iteration for the current state 
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
  virtual void onKeyReleased();
  virtual void onStart();
  virtual void update();
  virtual void printInfo();
};

void RGB_State::onKeyPressed() {
  nextLED();
}
void RGB_State::onKeyReleased() {}
void RGB_State::onStart() {}
void RGB_State::update() {
  setBrightness(param_1);
  clearRGB();
  SEL_COLOR = 255;
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
  if (delta_change < 1.0) return; // no change will happen
  else last_update_time = millis(); // reset last change

  // 3 against 2 polyrhythm
  if (this->positive_count) {
    SEL_COLOR += delta_change;
    if (SEL_COLOR >= 255) { // if max is reached: count down on next color
      SEL_COLOR = 255;
      nextLED();
      this->positive_count = false;
    }
  } else {
    SEL_COLOR -= delta_change;
    if (SEL_COLOR <= 0) { // if min is reach: count up on next color
      SEL_COLOR = 0;
      nextLED();
      this->positive_count = true;
    }
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
  clearRGB();
}

void UART_State::update() {
  if (this->UART->available()) { // read and process uart input when avaliable
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
  printMode();
}

//
// State machine variables
//

const byte Num_States = 4;
State *states[] = {new RGB_State(), new Rainbow_State(), new ValueControl_State(), new UART_State()};

/*
  Set, start and print state
*/
void setState(byte new_state) {
  current_state = new_state;
  STATE->onStart();
  Serial.println("State changed, state: " + String(current_state));
}

/*
  Sets state to next enabled state
*/
void nextState() { // Making sure to go to next enabled state, and to not get stuck in a loop
  for (int i = 0; i < Num_States; i++) {
    int potential = (current_state + i + 1) % Num_States;
    if (states[potential]->isEnabled()) {
      setState(potential);
      return;
    } else Serial.println("Potential failed (" + String(potential) + ")");
  }
}

/*
  Returns the final brightnes of led calculated by general brightness and the value of the LED color.
*/
byte calculateBrightness(int value) {
  return (value * RGBB_Data[3]) / 255;
}

/*
  Set the color of the LED.
*/
void setLEDColor() {
  analogWrite(RED_DIODE_PIN, 255 - calculateBrightness(RED));
  analogWrite(GREEN_DIODE_PIN, 255 - calculateBrightness(GREEN));
  analogWrite(BLUE_DIODE_PIN, 255 - calculateBrightness(BLUE));
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
  if (!scheduler->isRunning()) nextState();
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

long getNumericArgument(char * input, int len) {
  int ns = endOfNumber(input, len);
  int start = getStringStart(input, len);
  if (ns-start <= 0) {
    Serial.println("No value set");
    return -1;
  } 
  input[ns] = 0;
  return atol(input+start);
}

void schedulerCommand(char* input, int len) {
  // if no subcommand
  scheduler->printSchedule();
}





void setParam1(char *input, int len) {
  int tmp = getNumericArgument(input, len);
  if (tmp >= 0) param_1 = tmp;
}

void setParam2(char *input, int len) {
  int tmp = getNumericArgument(input, len);
  if (tmp >= 0) param_2 = tmp;
}

void setSelection(char *input, int len) {
  int tmp = getNumericArgument(input, len);
  if (tmp >= 0 && tmp < 3) selectedLED = tmp;
  else Serial.println("Invalid selection (0 <=> 2)"); 
}

void currentState(char *input, int len) {
  Serial.println("State " + String(current_state) + ":");
  STATE->printInfo();
  Serial.println("\tParam_1: " + String(param_1));
  Serial.println("\tParam_2: " + String(param_2));
  Serial.println("\tSelected_LED: " + String(selectedLED));
}

void currentColor(char *input, int len) {
  Serial.println("Color:");
  Serial.println("\tRed: " + String(RED));
  Serial.println("\tGreen: " + String(GREEN));
  Serial.println("\tBlue: " + String(BLUE));
  Serial.println("\tBrightness: " + String(RGBB_Data[3]));
}

void toNextState(char *input, int len) {
  nextState();
}

void enableState(char *input, int len) {
  int tmp = getNumericArgument(input, len);
  if (tmp >= 0 && tmp < Num_States) states[tmp]->enable();
  else Serial.println("Invalid state (0 <=> "+ String(Num_States) +")"); 
}

void disableState(char *input, int len) {
  int tmp = getNumericArgument(input, len);
  if (tmp >= 0 && tmp < Num_States) states[tmp]->disable();
  else Serial.println("Invalid state (0 <=> "+ String(Num_States) +")"); 
}

void toNextLED(char *input, int len) {
  nextLED();
  Serial.println("Current led: " + String(selectedLED));
}

void printHelp(char *input, int len) {
  Serial.println("Functions: ");
  Serial.println("\tcs - Print current state info");
  Serial.println("\tcc - Print current color values");
  Serial.println("\tns - Switch to next state");
  Serial.println("\tnl - Switch LED selection");
  Serial.println("\tsl - Set LED selection");
  Serial.println("\tsp1 - Set parameter 1");
  Serial.println("\tsp2 - Set parameter 2");
  Serial.println("\tschd - Handle scheduler");
  Serial.println("\tenbl - Enables a state");
  Serial.println("\tdsbl - Disables a state");
  Serial.println("\thelp - Print this help message");
  
}

struct FunctionLink {
  char *name;
  void (*function)(char *input, int len);
};

#define MAPPED_FUNCTIONS 11
FunctionLink FunctionMap[] = {
    {"cs", currentState},
    {"cc", currentColor},
    {"ns", toNextState},
    {"nl", toNextLED},
    {"sl", setSelection},
    {"sp1", setParam1},
    {"sp2", setParam2},
    {"schd", schedulerCommand},
    {"help", printHelp},
    {"enbl", enableState},
    {"dsbl", disableState}
};

void processCommands(char *input, int len) {
  for (int i = 0; i < MAPPED_FUNCTIONS; i++) {
    int start = startsWith(input, len, FunctionMap[i].name);
    if (start >= 0) {
      FunctionMap[i].function(input + start, len - start);
      return;
    }
  }
  Serial.print("No function with the name ");
  Serial.println(input);
}

void handleSerial() {
  if (Serial.available()) {
    char buffer[256];
    int input_length = Serial.readBytes(buffer, 256);
    buffer[input_length] = 0;
    Serial.write(buffer, input_length);
    processCommands(buffer, input_length);
  }
}

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

  scheduler = new Scheduler();
}

void loop() {
  // handle scheduler
  scheduler->run();

  PotHandle();
  STATE->update();
  setLEDColor();
  handleSerial();
}
