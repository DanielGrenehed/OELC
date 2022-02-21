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
  changeToState(tasks[current_task].state);
  setParameters(tasks[current_task].param_1, tasks[current_task].param_2, tasks[current_task].selection);
  task_start_time = millis();
}

/*
  Change to next task and start it 
*/
void Scheduler::nextTask() {
  current_task++;
  if (current_task >= task_count) current_task = 0;
  setParameters(tasks[current_task].param_1, tasks[current_task].param_2, tasks[current_task].selection);
  startTask();
  task_start_time = millis();
}
