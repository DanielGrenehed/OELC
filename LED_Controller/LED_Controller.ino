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

char command_buffer[256];
int buffer_pos = 0;

void setOverallIntensity(byte brightness) {
  RGBB_Data[3] = brightness;
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
  Serial.println("State " + String(state_machine->stateNumber()) + ":");
  state_machine->currentState()->printInfo();
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
  state_machine->nextState();
}

void enableState(char *input, int len) {
  int tmp = getNumericArgument(input, len);
  state_machine->enableState(tmp);
}

void disableState(char *input, int len) {
  int tmp = getNumericArgument(input, len);
  state_machine->disableState(tmp);
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
    Serial.readBytes(command_buffer+buffer_pos, 1);
    if (command_buffer[buffer_pos] == '\n') {
      command_buffer[buffer_pos] = 0;
      Serial.write(command_buffer, buffer_pos);
      processCommands(command_buffer, buffer_pos);
      buffer_pos = 0;
    } else {
      buffer_pos++;
      if (buffer_pos >= 256) {
        Serial.println("Command too long");
        buffer_pos = 0;
      }
    }
  }
}

void setState(byte state) {
  state_machine->setState(state);
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
  if (pressed) state_machine->currentState()->onKeyPressed();
  else state_machine->currentState()->onKeyReleased();
}

void onKey2Event(bool pressed) {
  if (pressed) {
    if (!scheduler->isRunning()) state_machine->nextState();
  }
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
}

void loop() {
  // handle scheduler
  scheduler->run();
  state_machine->currentState()->update();
  writeLEDColor();
  handleSerial();
  processInput();
}
