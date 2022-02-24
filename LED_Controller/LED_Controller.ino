#include "scheduler.hpp"
#include "stringutil.hpp"
#include "input.hpp"
#include "states.hpp"

/* @author Daniel Amos Grenehed

  Built for Arduino UNO
  
  Connected Hardware:
    1 RGB LED (Common anode)
    2 Buttons
    1 Voltage divider (Potentiometer)

  The LED is indirectly controlled
  by a "State". The state is 
  responsible for the logic,
  responding to button presses
  and parameter changes.

  The state is controlled
  with a state-machine.

  The state-machine is accessible 
  from Serial interraction and
  is controllable with a scheduler.

  The Scheduler is off by default.
  It is accessable by Serial 
  where tasks can be added, removed,
  moved and updated. It can be 
  in either loop mode or set to
  do a one time pass through the 
  scheduled tasks.

  A Task includes a duration(the
  time in milliseconds to run the
  task), which state to run,
  and parameters to set at the start
  of the task.

*/

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

/*
  Serial input buffer
*/
#define BUFFER_SIZE 256
char command_buffer[BUFFER_SIZE];
int buffer_pos = 0;


/*
  Maps a name and description to a function
*/
struct FunctionLink {
  const char *name;
  const char *description;
  void (*function)(char *input, int len);
};

/*
  Set LED Brightness
*/
void setOverallIntensity(byte brightness) {
  RGBB_Data[3] = brightness;
}

/*
  Print on or off
*/
void printlnBool(bool tf) {
  Serial.println(tf ? F("On") : F("Off"));
}

/*
  Print error
*/
void printArgumentError() {
  Serial.print(F("Err"));
}

/*
  Print acceptable input range for byte
*/
void printlnByteRange() {
  Serial.println(F(" (0 <=> 255)"));
}

/*
  Print Task error
*/
void printlnInvalidTask() {
  Serial.println(F("InvalTask Err"));
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

/*
  Set color value of LED d
*/
void setDiodeIntesity(byte d, byte intensity) {
  RGBB_Data[d] = intensity;
}

/*
  Returns the color value of LED d
*/
byte getDiodeIntensity(byte d) {
  return (byte) RGBB_Data[d];
}

//
// Serial scheduler functions
//

/*
  Start scheduler
*/
void schedulerStart(char* input, int len) {
  scheduler->start();
}

/*
  Stop scheduler
*/
void schedulerStop(char* input, int len) {
  scheduler->stop();
}

/*
  Set scheduler to loop mode
*/
void schedulerLoop(char* input, int len) {
  scheduler->enableLoop();
}

/*
  Disables scheduler loop mode 
*/
void schedulerNoLoop(char* input, int len) {
  scheduler->disableLoop();
}

/*
  Print Scheduler status
*/
void schedulerStatus(char* input, int len) {
  scheduler->printStatus();
}

/*
  Print schedule
*/
void schedulerTasks(char* input, int len) {
  scheduler->printSchedule();
} 

/*
  Remove task at index from schedule (0 <=> task_count-1)
*/
void schedulerRemoveTask(char* input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  scheduler->removeTask(tmp);
} 

/*
  Move task in schedule from x to y (0 <=> task_count-1)
  Shifts tasks between x and y one step towards x
*/
void schedulerMoveTask(char* input, int len) {
  int start = 0;
  int from = getNumericArgument(input, len, &start); // get first argument
  int to = getNumericArgument(input+start, len-start, &start); // get second argument
  if (from < 0 || to < 0) return;
  scheduler->moveTask(from, to);
}

/*
  Appends new task to schedule
*/
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
    scheduler->addTask({duration, state, 0, 0, 0, false});
  } else {
    scheduler->addTask({duration, state, (byte)p1, (byte)p2, (byte)s, true});
  }
}

/*
  Sets schedule task at index to new task
*/
void schedulerUpdateTask(char* input, int len) {
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
    scheduler->updateTask(index, {duration, state, 0, 0, 0, false});
  } else {
    scheduler->updateTask(index, {duration, state, (byte)p1, (byte)p2, (byte)s, true});
  }
}

// To be able to add to SchedulerMap
void schedulerHelp(char* input, int len);

/*
  Mapping commands and descriptions to Scheduler functions
*/
#define SCHEDULER_FUNCTIONS 11
const static FunctionLink SchedulerMap[] = {
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

/*
  Print Scheduler functions
*/
void schedulerHelp(char* input, int len) {
   Serial.println(F("Scheduler: "));
  for (int i = 0; i < SCHEDULER_FUNCTIONS; i++) {
    Serial.print(F("\t"));
    Serial.print(SchedulerMap[i].name);
    Serial.print(F(" - "));
    Serial.println(SchedulerMap[i].description);
  }
}

/*
  Map scheduler command to function call
  Calls schedulerHelp if no function found
*/
void schedulerCommand(char* input, int len) {
   for (int i = 0; i < SCHEDULER_FUNCTIONS; i++) {
    int start = startsWith(input, len, SchedulerMap[i].name);
    if (start >= 0) {
      SchedulerMap[i].function(input + start, len - start);
      return;
    }
  }
  schedulerHelp(command_buffer, 0);
}

/*
  Emit Key1 event (0 | 1)
*/
void button1(char* input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp == 1) onKey1Event(true);      // Key 1 Press
  else if (tmp == 0) onKey2Event(false);// Key 1 Release
}

/*
  Emit Key2 event (0 | 1)
*/
void button2(char* input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp == 1) onKey2Event(true);      // Key 2 Press
  else if (tmp == 0) onKey2Event(false);// Key 2 Release
}

/*
  Emit PotChanged event (0 <=> 255)
*/
void pot(char* input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp > 255 || tmp < 0) {
    printArgumentError();
    Serial.print(tmp);
    printlnByteRange();
  } else onPotValueChanged((byte) tmp);
}

/*
  Sets param_1 (0 <=> 255)
*/
void setParam1(char *input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp >= 0 && tmp < 256) param_1 = tmp;
  else {
    printArgumentError();
    printlnByteRange();
  }
}

/*
  Sets param_2 (0 <=> 255)
*/
void setParam2(char *input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp >= 0 && tmp < 256) param_2 = tmp;
  else {
    printArgumentError();
    printlnByteRange();
  }
}

/*
  Sets LED selection (0 <=> 2)
*/
void setSelection(char *input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  if (tmp >= 0 && tmp < 3) selectedLED = tmp;
  else {
    printArgumentError();
    Serial.println(F("(0 <=> 2)"));
  } 
}

/*
  Print info on current state
*/
void currentState(char *input, int len) {
  Serial.print(F("State "));
  Serial.print(state_machine->stateNumber());
  Serial.println(F(":"));

  state_machine->currentState()->printInfo();
  Serial.print(F("\tP1: "));
  Serial.println(param_1);
  Serial.print(F("\tP2: "));
  Serial.println(param_2);
  Serial.print(F("\tS: "));
  Serial.println(selectedLED);
}

/*
  Prints the values of the color
*/
void currentColor(char *input, int len) {
  Serial.println(F("Color:"));
  Serial.print(F("\tR: "));
  Serial.println(RED);
  Serial.print(F("\tG: "));
  Serial.println(GREEN);
  Serial.print(F("\tB: "));
  Serial.println(BLUE);
  Serial.print(F("\tBs: "));
  Serial.println(RGBB_Data[3]);
}

/*
  Go to next state
*/
void toNextState(char *input, int len) {
  state_machine->nextState();
}

/*
  Enables state (0 <=> 3)
*/
void enableState(char *input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  state_machine->enableState(tmp);
}

/*
  Disables state (0 <=> 3)
*/
void disableState(char *input, int len) {
  int start = 0;
  int tmp = getNumericArgument(input, len, &start);
  state_machine->disableState(tmp);
}

/*
  Selects next LED
*/
void toNextLED(char *input, int len) {
  nextLED();
  Serial.print(F("Current LED: "));
  Serial.println(selectedLED);
}

// To be able to add to FunctionMap
void printHelp(char* input, int len);

#define MAPPED_FUNCTIONS 14

/*
    Maps Command and description to function
*/
const static FunctionLink FunctionMap[] = {
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

/*
  Print commands and descriptions
*/
void printHelp(char *input, int len) {
  Serial.println(F("Functions: "));
  for (int i = 0; i < MAPPED_FUNCTIONS; i++) {
    Serial.print(F("\t"));
    Serial.print(FunctionMap[i].name);
    Serial.print(F(" - "));
    Serial.println(FunctionMap[i].description);
  }
}

/*
  Maps command to function call
  Prints error if no function mapped to command
*/
void processCommands(char *input, int len) {
  for (int i = 0; i < MAPPED_FUNCTIONS; i++) {
    int start = startsWith(input, len, FunctionMap[i].name);
    if (start >= 0) {
      // if input matched with function
      // call function with the subcommands/arguments (input - command)
      FunctionMap[i].function(input + start, len - start);
      return;
    }
  }
  // Input matched no function
  // Print error
  Serial.print(F("No fun_"));
  Serial.println(input);
}


/*
  Read serial if available and process command when fully received
*/
void handleSerial() {
  if (Serial.available()) {
    // read one byte when available
    Serial.readBytes(command_buffer+buffer_pos, 1);
    if (command_buffer[buffer_pos] == '\n') {
      // on newline char, replace with string terminator
      command_buffer[buffer_pos] = 0;
      // echo input and evaluate command
      Serial.println(command_buffer);
      processCommands(command_buffer, buffer_pos);
      // *reset* buffer for new command 
      buffer_pos = 0; 
    } else {
      // When not newline
      // set position for next byte
      buffer_pos++; 
      if (buffer_pos >= BUFFER_SIZE) {
        // If command too long, print error and *reset* buffer  
        Serial.println(F("CmdErr"));
        buffer_pos = 0;
      }
    }
  }
}

/*
  Set statemachine state
*/
void setState(byte state) {
  state_machine->setState(state);
}

/*
  Go to next statemachine state
*/
void nextState() {
  state_machine->nextState();
}

/*
  Set state parameters
*/
void setParameters(byte p1, byte p2, byte s) {
  param_1 = p1;
  param_2 = p2;
  selectedLED = s; 
}


//
// Input handling
//

/*
  Key1 Event callback
  Map Key1 Events to state Key1Pressed and Released calls 
*/
void onKey1Event(bool pressed) {
  if (pressed) state_machine->currentState()->onKey1Pressed();
  else state_machine->currentState()->onKey1Released();
}

/*
  Key2 Event callback
  Map Key2 events to state Key2Pressed and Released calls
*/
void onKey2Event(bool pressed) {
  if (pressed) state_machine->currentState()->onKey2Pressed();
  else state_machine->currentState()->onKey2Released();
}

/*
  Pot Event callback
  Sets parameters
*/
void onPotValueChanged(byte new_value) {
  if (Key1State == HIGH) param_2 = new_value;
  param_1 = new_value;
}

//
// Arduino setup and loop
//

void setup() {
  // Bind functions 
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

  // Print info when serial monitor connected
  printHelp(command_buffer, 0);
  currentState(command_buffer, 0);
  currentColor(command_buffer, 0);
}

void loop() {
  // handle scheduler
  scheduler->run();
  // handle state
  state_machine->currentState()->update();
  // Write current color to LED
  writeLEDColor();
  // Read serial 
  handleSerial();
  // Handle buttons and pot and emit events on change
  processInput();
}
