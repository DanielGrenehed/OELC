#include "scheduler.hpp"
#include "stringutil.hpp"
#include "input.hpp"
#include "states.hpp"

#define RED RGBB_Data[0]
#define GREEN RGBB_Data[1]
#define BLUE RGBB_Data[2]
#define SEL_COLOR RGBB_Data[selectedLED]

#define RED_DIODE_PIN 11
#define GREEN_DIODE_PIN 10
#define BLUE_DIODE_PIN 9

Scheduler *scheduler;
StateMachine *state_machine;

// State parameters

int RGBB_Data[] = {255, 0, 0, 255}; // RED, GREEN, BLUE, BRIGHTNESS
#define BUFFER_SIZE 256
char command_buffer[BUFFER_SIZE];
int buffer_pos = 0;

struct FunctionLink {
  char *name;
  char *description;
  void (*function)(char *input, int len);
};

void setOverallIntensity(byte brightness) {
  RGBB_Data[3] = brightness;
}

void printlnBool(bool tf) {
  Serial.println(tf ? "On" : "Off");
}
void printArgumentError() {
  Serial.print("Err");
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
  Returns the final brightnes of led calculated by general brightness and the value of the LED color.
*/
byte calculateBrightness(int value) {
  return (value * RGBB_Data[3]) / 255;
}

/*
  Set the color of the LED.
*/
void writeLEDColor() {
  analogWrite(RED_DIODE_PIN, 255 - calculateBrightness(RED));
  analogWrite(GREEN_DIODE_PIN, 255 - calculateBrightness(GREEN));
  analogWrite(BLUE_DIODE_PIN, 255 - calculateBrightness(BLUE));
}

void setDiodeIntesity(byte d, byte intensity) {
  RGBB_Data[d] = intensity;
}

byte getDiodeIntensity(byte d) {
  return (byte) RGBB_Data[d];
}

//
// Serial functions
//

void schedulerStart(char* input, int len) {
  scheduler->start();
}

void schedulerStop(char* input, int len) {
  scheduler->stop();
}

void schedulerLoop(char* input, int len) {
  scheduler->enableLoop();
}

void schedulerNoLoop(char* input, int len) {
  scheduler->disableLoop();
}

void schedulerStatus(char* input, int len) {
  scheduler->printStatus();
}

void schedulerTasks(char* input, int len) {
  scheduler->printSchedule();
} 

void schedulerRemoveTask(char* input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  scheduler->removeTask(tmp);
} 

void schedulerMoveTask(char* input, int len) {
  int start = 0;
  int from = getNumericArgument(input, len, &start); // get first argument
  int to = getNumericArgument(input+start, len-start, &start); // get second argument
  if (from < 0 || to < 0) return;
  scheduler->moveTask(from, to);
}

void printlnInvalidTask() {
  Serial.println("ITErr");
}


void schedulerAddTask(char* input, int len) {
  int start = 0;
  long duration = getNumericArgument(input, len, &start);
  byte state = getNumericArgument(input+start, len-start, &start);
  if (duration < 0 || state < 0) {
    printlnInvalidTask();
    return;
  }
  int p1 = getNumericArgument(input+start, len-start, &start);
  int p2 = getNumericArgument(input+start, len-start, &start);
  int s = getNumericArgument(input+start, len-start, &start);

  if (p1 < 0 || p2 < 0 || s < 0) {
    // set task (state duration)
    scheduler->addTask({duration, state, 0, 0, 0, false});
  } else {
    // set task (state duration p1 p2 s)
    scheduler->addTask({duration, state, (byte)p1, (byte)p2, (byte)s, true});
  }
}

void schedulerUpdateTask(char* input, int len) {
  // get index, get duration, get state, get p1, get p2, get s
  int start = 0;
  int index = getNumericArgument(input, len, &start);
  long duration = getNumericArgument(input+start, len-start, &start);
  byte state = getNumericArgument(input+start, len-start, &start);
  if (duration < 0 || state < 0) {
    printlnInvalidTask();
    return;
  }
  int p1 = getNumericArgument(input+start, len-start, &start);
  int p2 = getNumericArgument(input+start, len-start, &start);
  int s = getNumericArgument(input+start, len-start, &start);
  if (p1 < 0 || p2 < 0 || s < 0) {
    // set task (state duration)
    scheduler->updateTask(index, {duration, state, 0, 0, 0, false});
  } else {
    // set task (state duration p1 p2 s)
    scheduler->updateTask(index, {duration, state, (byte)p1, (byte)p2, (byte)s, true});
  }
}

void schedulerHelp(char* input, int len);
#define SCHEDULER_FUNCTIONS 11
FunctionLink SchedulerMap[] = {
  {"run", "Run", schedulerStart},
  {"sp", "Stop", schedulerStop}, 
  {"lp", "Loop", schedulerLoop},
  {"np", "Noloop", schedulerNoLoop},
  {"st", "Status", schedulerStatus},
  {"ts", "Tasks", schedulerTasks},
  {"rm", "Remove tsk", schedulerRemoveTask},
  {"ad", "Append tsk", schedulerAddTask},
  {"ud", "Update tsk", schedulerUpdateTask}, 
  {"mv", "Move tsk", schedulerMoveTask},
  {"hlp", "Help msg", schedulerHelp}
};
void schedulerHelp(char* input, int len) {
   Serial.println("Scheduler: ");
  for (int i = 0; i < SCHEDULER_FUNCTIONS; i++) {
    Serial.print("\t");
    Serial.print(SchedulerMap[i].name);
    Serial.print(" - ");
    Serial.println(SchedulerMap[i].description);
  }
}

void schedulerCommand(char* input, int len) {
  // Get number of arguments
   for (int i = 0; i < SCHEDULER_FUNCTIONS; i++) {
    int start = startsWith(input, len, SchedulerMap[i].name);
    if (start >= 0) {
      Serial.println(input+start);
      SchedulerMap[i].function(input + start, len - start);
      return;
    }
  }
  // if no subcommand
  schedulerHelp(command_buffer, 0);
}

void button1(char* input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp == 1) onKey1Event(true);
  else if (tmp == 0) onKey2Event(false);
}

void button2(char* input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp == 1) onKey2Event(true); // button 2 press
  else if (tmp == 0) onKey2Event(false); // button 2 relesase
}

void pot(char* input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp > 255) {
    printArgumentError();
    Serial.print(tmp);
    Serial.println(" (0 <=> 255)");
    return;
  }
  if (tmp < 0) return; 
  onPotValueChanged((byte) tmp);
}

void setParam1(char *input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp >= 0) param_1 = tmp;
}

void setParam2(char *input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp >= 0) param_2 = tmp;
}

void setSelection(char *input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp >= 0 && tmp < 3) selectedLED = tmp;
  else {
    printArgumentError();
    Serial.println("(0 <=> 2)");
  } 
}

void currentState(char *input, int len) {
  Serial.print("State ");
  Serial.print(state_machine->stateNumber());
  Serial.println(":");

  state_machine->currentState()->printInfo();
  Serial.print("\tP1: ");
  Serial.println(param_1);
  Serial.print("\tP2: ");
  Serial.println(param_2);
  Serial.print("\tS: ");
  Serial.println(selectedLED);
}

void currentColor(char *input, int len) {
  Serial.println("Color:");
  Serial.print("\tR: ");
  Serial.println(RED);
  Serial.print("\tG: ");
  Serial.println(GREEN);
  Serial.print("\tB: ");
  Serial.println(BLUE);
  Serial.print("\tBs: ");
  Serial.println(RGBB_Data[3]);
}

void toNextState(char *input, int len) {
  state_machine->nextState();
}

void enableState(char *input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  state_machine->enableState(tmp);
}

void disableState(char *input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  state_machine->disableState(tmp);
}

void toNextLED(char *input, int len) {
  nextLED();
  Serial.print("Current led: ");
  Serial.println(selectedLED);
}
void printHelp(char* input, int len);

FunctionLink FunctionMap[] = {
    {"cs", "State info", currentState},
    {"cc", "CurrColor", currentColor},
    {"ns", "NextState", toNextState},
    {"nl", "Next LED", toNextLED},
    {"sl", "Select LED", setSelection},
    {"sp1", "Set p1", setParam1},
    {"sp2", "Set p2", setParam2},
    {"pot", "Pot event", pot},
    {"btn1", "Key1 event", button1},
    {"btn2", "Key2 event", button2}, 
    {"schd", "Scheduler", schedulerCommand},
    {"help", "Help msg", printHelp},
    {"enbl", "Enable state", enableState},
    {"dsbl", "Disable state", disableState}
};

#define MAPPED_FUNCTIONS 14
void printHelp(char *input, int len) {
  Serial.println("Functions: ");
  for (int i = 0; i < MAPPED_FUNCTIONS; i++) {
    Serial.print("\t");
    Serial.print(FunctionMap[i].name);
    Serial.print(" - ");
    Serial.println(FunctionMap[i].description);
  }
}

void processCommands(char *input, int len) {
  for (int i = 0; i < MAPPED_FUNCTIONS; i++) {
    int start = startsWith(input, len, FunctionMap[i].name);
    if (start >= 0) {
      FunctionMap[i].function(input + start, len - start);
      return;
    }
  }
  Serial.print("No fun_");
  Serial.println(input);
}

void handleSerial() {
  if (Serial.available()) {
    Serial.readBytes(command_buffer+buffer_pos, 1);
    if (command_buffer[buffer_pos] == '\n') {
      command_buffer[buffer_pos] = 0;
      Serial.println(command_buffer);
      processCommands(command_buffer, buffer_pos);
      buffer_pos = 0;
    } else {
      buffer_pos++;
      if (buffer_pos >= BUFFER_SIZE) {
        Serial.println("CmdErr");
        buffer_pos = 0;
      }
    }
  }
}

void setState(byte state) {
  state_machine->setState(state);
}

void nextState() {
  state_machine->nextState();
}

void setParameters(byte p1, byte p2, byte s) {
  param_1 = p1;
  param_2 = p2;
  selectedLED = s; 
}


//
// Input handling
//


void onKey1Event(bool pressed) {
  if (pressed) state_machine->currentState()->onKey1Pressed();
  else state_machine->currentState()->onKey1Released();
}

void onKey2Event(bool pressed) {
  if (pressed) state_machine->currentState()->onKey2Pressed();
  else state_machine->currentState()->onKey2Released();
}

void onPotValueChanged(byte new_value) {
  if (Key1State == HIGH) param_2 = new_value;
  param_1 = new_value;
}

//
// Arduino setup and loop
//

void setup() {
  setupInput(onKey1Event, onKey2Event, onPotValueChanged);
  setStatesFunctions(setOverallIntensity, clearRGB, setDiodeIntesity, getDiodeIntensity);
  // set LED pins to output
  pinMode(RED_DIODE_PIN, OUTPUT);
  pinMode(GREEN_DIODE_PIN, OUTPUT);
  pinMode(BLUE_DIODE_PIN, OUTPUT);

  // start comms
  Serial.begin(BAUD_RATE);

  scheduler = new Scheduler(setState, setParameters);
  state_machine = new StateMachine();

  printHelp(command_buffer, 0);
  currentState(command_buffer, 0);
  currentColor(command_buffer, 0);
}

void loop() {
  // handle scheduler
  scheduler->run();
  state_machine->currentState()->update();
  writeLEDColor();
  handleSerial();
  processInput();
}
