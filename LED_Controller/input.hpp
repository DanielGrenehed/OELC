#define KEY_1_INTERRUPT_PIN 2
#define KEY_2_INTERRUPT_PIN 3
#define KEY_1_PIN 8
#define KEY_2_PIN 12

#define POT_PIN A0

// Debounce time in millis
#define DEBOUNCE_TIME 6

int Key1State = LOW;
bool Key1Debounce = false;
long Key1DebounceTime = 0;
void (*Key1Event)(bool pressed);

int Key2State = LOW;
bool Key2Debounce = false;
long Key2DebounceTime = 0;
void (*Key2Event)(bool pressed); 

byte PotValue = 0;
void (*PotChangeEvent)(byte value);

void Key1Handle() {
  if (!Key1Debounce) return;
  if (Key1DebounceTime + DEBOUNCE_TIME <= millis()) {
    int tmp = digitalRead(KEY_1_PIN);
    if (tmp != Key1State) {
      if (tmp == HIGH) Key1Event(true);
      else Key1Event(false);
      Key1State = tmp;
    }
    Key1Debounce = false;
  }
}

void Key2Handle() {
  if (!Key2Debounce) return;
  if (Key2DebounceTime + DEBOUNCE_TIME <= millis()) {
    int tmp = digitalRead(KEY_2_PIN);
    if (tmp != Key2State) {
        if (tmp == HIGH) Key2Event(true);
        else Key2Event(false);
        Key2State = tmp;
    }
    Key2Debounce = false;
  }
}

void PotHandle() {
  byte new_pot_value = map(analogRead(POT_PIN), 0, 1024, 0, 255);
  if (new_pot_value != PotValue) {
    PotChangeEvent(new_pot_value);
    PotValue = new_pot_value;
  }
}

void onKey1Interrupt() {
    Key1Debounce = true;
    Key1DebounceTime = millis();
}

void onKey2Interrupt() {
    Key2Debounce = true;
    Key2DebounceTime = millis();
}

void setupInput(void (*k1e)(bool), void (*k2e)(bool), void (*pce)(byte))  {
    Key1Event = k1e;
    Key2Event = k2e;
    PotChangeEvent = pce;
    // Set key pins as inputs
    pinMode(KEY_1_PIN, INPUT);
    pinMode(KEY_2_PIN, INPUT);
    // Add key interrupts
    pinMode(KEY_1_INTERRUPT_PIN, INPUT);
    pinMode(KEY_2_INTERRUPT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(KEY_1_INTERRUPT_PIN), onKey1Interrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(KEY_2_INTERRUPT_PIN), onKey2Interrupt, CHANGE);
}

void processInput() {
    PotHandle();
    Key1Handle();
    Key2Handle();
}