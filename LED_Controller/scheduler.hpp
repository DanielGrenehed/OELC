void printlnBool(bool);

struct Task {
  long duration;
  uint8_t state;
  uint8_t param_1;
  uint8_t param_2;
  uint8_t selection;
  bool set_params;
};

#define MAX_TASKS 30
Task tasks[MAX_TASKS]; // Scheduled tasks
class Scheduler {
private:
  
  int task_count = 0;    // Number of tasks in schedule
  int current_task = 0;
  long task_start_time = 0; // Time measured when task started
  bool running = false;
  bool loop = true;
  void (*changeToState)(byte state);
  void (*setParameters)(byte p1, byte p2, byte s);
  void startTask();
  void nextTask();
public:
  Scheduler(void (*cts)(byte state), void (*stp)(byte,byte,byte)) : changeToState(cts), setParameters(stp){};
  ~Scheduler(){};
  void printSchedule();
  void printStatus();
  void printIndexError(int index);
  void printTask(int index);
  void addTask(Task task);
  void removeTask(int index);
  void moveTask(int index, int to);
  void updateTask(int index, Task task);
  void start();
  void stop();
  void enableLoop();
  void disableLoop();
  bool isRunning();
  bool isLooping();
  void run();
};

/*
 print order of tasks in schedule
*/
void Scheduler::printSchedule() {
  Serial.println(F("Schedule:"));
  if (task_count <= 0) Serial.println(F("\tNo tasks"));
  for (int i = 0; i < this->task_count; i++) printTask(i);
} 

/* 
 print status of scheduler and current task
*/
void Scheduler::printStatus() {
    Serial.println(F("Scheduler:"));
    Serial.print(F("\tStatus: "));
    printlnBool(this->isRunning());
    Serial.print(F("\tLoop: "));
    printlnBool(this->isLooping());
    Serial.print(F("\ttc: "));
    Serial.println(task_count);
    if (task_count > 0) {
        printTask(current_task);
        Serial.print(F("\tFor: "));
        Serial.print((millis() - task_start_time));
        Serial.println(F("ms"));
    }
}

/*
  Print task indexing errors
*/
void Scheduler::printIndexError(int index) {
    if (index >= task_count) {
        Serial.print(F("IdxErr: "));
        Serial.print(index);
        Serial.print(F("/"));
        Serial.println(task_count);
    }
    if (index < 0) {
        Serial.print(F("IdxErr: "));
        Serial.println(index);
    }
}

void Scheduler::printTask(int index) {
    Serial.print(F("\tTask: "));
    Serial.println(index);

    Serial.print(F("\tState_"));
    Serial.print(tasks[index].state);
    Serial.print(F(" for "));
    Serial.print(tasks[index].duration);
    Serial.print(F("ms, p1: "));
    Serial.print(tasks[index].param_1);
    Serial.print(F(", p2: "));
    Serial.print(tasks[index].param_2);
    Serial.print(F(", s: "));
    Serial.println(tasks[index].selection);
}

/*
  Appends task to schedule
*/
void Scheduler::addTask(Task task) {
  if (task_count + 1 < MAX_TASKS) {
    tasks[task_count++] = task;
  } else Serial.println(F("TskErr"));
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
  else Serial.println(F("No tasks!"));
}
/*
  Stops the schedule from running
*/
void Scheduler::stop() {
  this->running = false;
}

void Scheduler::enableLoop() {
    this->loop = true;
}

void Scheduler::disableLoop() {
    this->loop = false;
}

/*
  Returns wether or not the schedule is running
*/
bool Scheduler::isRunning() {
  return this->running;
}

bool Scheduler::isLooping() {
    return this->loop;
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
  changeToState(tasks[current_task].state);
  if (tasks[current_task].set_params) setParameters(tasks[current_task].param_1, tasks[current_task].param_2, tasks[current_task].selection);
  task_start_time = millis();
}

/*
  Change to next task and start it 
*/
void Scheduler::nextTask() {
  current_task++;
  if (current_task >= task_count) {
      current_task = 0;
      if (!isLooping()) stop();
  }
  startTask();
  task_start_time = millis();
}
