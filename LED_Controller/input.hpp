#ifndef INPUT_HPP
#define INPUT_HPP

/*  @author Daniel Amos Grenehed

    Arduino Input Handling

    Sets up Key1 and Key2 interrupts.
    Emits key and pot events if states changed 
    running processInput().

    Handles debounce for keys.

    Run setupInput() in setup()(!), 
    to setup input and callbacks
    
*/

#define KEY_1_INTERRUPT_PIN 2
#define KEY_2_INTERRUPT_PIN 3
#define KEY_1_PIN 8
#define KEY_2_PIN 12

#define POT_PIN A0

// Debounce time in millis
#define DEBOUNCE_TIME 6

// Key 1 variables
int Key1State = LOW;
bool Key1Debounce = false;
long Key1DebounceTime = 0;
void (*Key1Event)(bool pressed);

// Key 2 variables
int Key2State = LOW;
bool Key2Debounce = false;
long Key2DebounceTime = 0;
void (*Key2Event)(bool pressed); 

// Pot variables
byte PotValue = 0;
void (*PotChangeEvent)(byte value);

/*
    Handle debounde for key1 and emit events when pin state changend
*/
void Key1Handle() {
    if (!Key1Debounce) return;
    if (Key1DebounceTime + DEBOUNCE_TIME <= millis()) {
        // Debounce time elapsed
        // Read state of pin and emit event
        int tmp = digitalRead(KEY_1_PIN);
        if (tmp != Key1State) {
            if (tmp == HIGH) Key1Event(true);
            else Key1Event(false);
            Key1State = tmp;
        }
        // reset debounce
        Key1Debounce = false;
    }
}

/*
    Handle debounde for key2 and emit events when pin state changend
*/
void Key2Handle() {
    if (!Key2Debounce) return;
    if (Key2DebounceTime + DEBOUNCE_TIME <= millis()) {
        // Debounce time elapsed
        // Read state of pin and emit event
        int tmp = digitalRead(KEY_2_PIN);
        if (tmp != Key2State) {
            if (tmp == HIGH) Key2Event(true);
            else Key2Event(false);
            Key2State = tmp;
        }
        // reset debounce
        Key2Debounce = false;
    }
}

/*
    Reads pot value and calls PotChangeEvent callback if value changed
*/
void PotHandle() {
  byte new_pot_value = map(analogRead(POT_PIN), 0, 1024, 0, 255);
  if (new_pot_value != PotValue) {
    PotChangeEvent(new_pot_value);
    PotValue = new_pot_value;
  }
}

/*
    Handle key1 changed interrupt, start debounce  
*/
void onKey1Interrupt() {
    Key1Debounce = true;
    Key1DebounceTime = millis();
}

/*
    Handle key2 changed interrupt, start debounce
*/
void onKey2Interrupt() {
    Key2Debounce = true;
    Key2DebounceTime = millis();
}

/*
    Set event callbacks, setup buttons as input and attach interrupts
*/
void setupInput(void (*k1e)(bool), void (*k2e)(bool), void (*pce)(byte))  {
    Key1Event = k1e;        // Key1 callback
    Key2Event = k2e;        // Key2 callback
    PotChangeEvent = pce;   // Pot  callback
    
    // Set key pins as inputs
    pinMode(KEY_1_PIN, INPUT);
    pinMode(KEY_2_PIN, INPUT);

    // Add key interrupts
    pinMode(KEY_1_INTERRUPT_PIN, INPUT);
    pinMode(KEY_2_INTERRUPT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(KEY_1_INTERRUPT_PIN), onKey1Interrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(KEY_2_INTERRUPT_PIN), onKey2Interrupt, CHANGE);
}

/*
    Handle key and pot change, emit event on change
*/
void processInput() {
    // Emit event on change
    PotHandle();

    // Emit events when debounced 
    Key1Handle();
    Key2Handle();
}

#endif /* ifndef INPUT_HPP */