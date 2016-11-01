#include "app.h"
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "utilities.h"
#include <sys/resource.h>
#include <sys/time.h>

using namespace std;

const char *strpolicy(int i) {
  if (i == SCHED_FIFO)
    return "SCHED_FIFO";
  else if (i == SCHED_RR)
    return "SCHED_RR";
  else if (i == SCHED_OTHER)
    return "SCHED_OTHER";
  else
    return "UNKNOWN";
}

void writehere(const char *msg) { write(STDOUT_FILENO, msg, strlen(msg)); }
// async-safe implementation
void positive_integer_to_string(int number, char *buffer, int length) {
  // count number of digits
  int numdigits, num;
  if (number < 0) {
    number = -number; // just in case
  }
  numdigits = 0;
  num = number;
  if (num) {
    do {
      numdigits++;
      num /= 10;
    } while (num);
  } else {
    numdigits = 1;
  }
  if (length < (numdigits + 1))
    return;
  int remainder, i;

  for (i = 0; i < numdigits; i++) {
    remainder = number % 10;
    number = number / 10;
    buffer[numdigits - (i + 1)] = remainder + '0';
  }
  buffer[numdigits] = '\0';
}

void signal_handler(int signum) {

  if (signum == SIGCHLD) {
    int status = 0;
    // Wait for any child process.
    int child_pid = 1;
    while (child_pid > 0) {
      child_pid = waitpid(-1, &status, WNOHANG);
      if (child_pid < 0)

      {
        char buffer[256];
        strerror_r(errno, buffer, 256);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);
        return;
      }

      else

      {

        if (WIFEXITED(status)) {
          writehere("exited status ");
          int exitcode = WEXITSTATUS(status);
          char exitstring[20];
          exitstring[0] = '\0';
          positive_integer_to_string(exitcode, exitstring, 20);
          writehere(exitstring);
          writehere("\n");
        } else if (WIFSIGNALED(status)) {
          writehere("terminated by signal ");
          int termsig = WTERMSIG(status);
          char termsigstring[20];
          termsigstring[0] = '\0';
          positive_integer_to_string(termsig, termsigstring, 20);
          writehere(termsigstring);
          writehere("\n");
        }
      }
    }
  }
}

// Constructor

app::app()
    : virtual_memory_limit(-1), scheduling_policy(SCHED_OTHER),
      scheduling_priority(0) {
  struct sigaction new_action;
  new_action.sa_handler = signal_handler;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &new_action, 0) == -1) {
    printf("process %d: error while installing handler for SIGINT\n", getpid());
  }

  cerr << "===============================================" << endl;
  cerr << "Welcome to the CS3281 Shell Assignment " << endl;
  cerr << "===============================================" << endl;
}

// Destructor
app::~app() {

  cerr << "===============================================" << endl;
  cerr << "Closing CS3281 Shell Assignment " << endl;
  cerr << "===============================================" << endl;
}

int app::parallel_execution(std::string command_string) {
  // YOU Lost 10 marks here. I will make a piazza post describing this section.
  std::vector<string> parallel_commands;
  std::map<int, string> children;
  tokenize_string(command_string, parallel_commands);
  for (auto command_str : parallel_commands) {
    std::vector<string> command;
    tokenize_string(command_str, command, " ");
    if (checkforeground(command)) {
      //  LOG << command[0] << " is foreground" << endl;
      if (checkbuiltin(command)) {
        executebuiltin(command);
      } else {
        // we will wait for all the processes together.
        int pid = execute(command, false);
        children[pid] = command[0];
      }
    } else {
      // LOG << command[0] << " is background" << endl;
      if (checkbuiltin(command)) {
        // Remove the & from the command.
        // The last element is & and we should pop it back.
        command.pop_back();
        executebuiltin(command);
      } else {
        // we will wait for all the processes together.
        int pid = execute(command, false);
        // we do not add the pid to the process we are going to be
        // waiting for.
      }
    }
    // @Task 1: Launch the parallel commands - note that if command1 ||
    // command2
    // is given, you must launch both of them and then wait for both of them.
    // make sure to check if they are built in and if they should not be
    // waited upon i.e. if they are not foreground tasks. Use checkforeground
    // function for that.
    // child is a pair of <int,string>
    for (auto child : children) {
      int status = 0;

      int retvalue = waitpid(child.first, &status, 0);
      if (retvalue < 0)

      {

        LOG << "error occurred " << strerror(errno)
            << "Did we already wait for it in the signal handler.\n";
        continue;
      }

      else

      {
        if (WIFEXITED(status)) {
          int exitcode = WEXITSTATUS(status);
          if (exitcode == 0)
            LOG << child.second << ":Child Exited Successfully\n";
          else
            LOG << child.second << ": Child Self Terminated With Exit Code "
                << exitcode << "\n";
        } else if (WIFSIGNALED(status)) {
          int signalsent = WTERMSIG(status);
          LOG << child.second << ": Child Terminated Due to Signal "
              << signalsent << "\n";
        }
      }
    }
  }

  return 0;
}

bool checkbuiltin(std::vector<std::string> &command) {
  //@ Task2: complete this check to include other built in commands - see
  // readme.
  if (command[0] == "set_memlimit" || command[0] == "cd" ||
      command[0] == "set_policy" || command[0] == "set_priority")
    return true;
  else
    return false;
}

int app::executebuiltin(std::vector<string> &command) {
  // all built in command have 1 command and 1 argument. The last entry is
  // return.

  if (command.size() != 2) {
    LOG << "Built in commands require two arguments\n";
    return -1;
  }

  if (command[0] == "cd" && command.size() >= 2) {
    // @Task 3: Implement the command to change directory. Search for chdir
    LOG << "Got a command to change directory to " << command[1] << std::endl;
    int retvalue = chdir(command[1].c_str());
    if (retvalue < 0) {
      LOG << "Error occured while changing directory to " << command[1]
          << " error was " << strerror(errno) << "std::endl";
    }
  }

  else if (command[0] == "set_memlimit") {
    std::cout << "setting memlimit to " << command[1] << " bytes\n";
    try {
      int bytes = std::stoi(command[1]);

      this->virtual_memory_limit = bytes;
    } catch (...) {
      LOG << "Exception occured while converting " << command[1] << " to int\n";
    }
  } else if (command[0] == "set_policy") {
    LOG << "setting policy to " << command[1] << "\n";
    if (command[1] == "fifo") {
      this->scheduling_policy = SCHED_FIFO;
    } else if (command[1] == "rr") {
      this->scheduling_policy = SCHED_RR;
    } else if (command[1] == "other") {
      this->scheduling_policy = SCHED_OTHER;
    } else {
      LOG << "wrong policy " << command[1] << std::endl;
    }
  } else if (command[0] == "set_priority") {
    std::cout << "setting priority to " << command[1] << "\n";
    try {
      int priority = std::stoi(command[1]);
      if ((this->scheduling_policy == SCHED_FIFO ||
           this->scheduling_policy == SCHED_RR)) {
        if (priority >= 1 && priority <= 99) {
          this->scheduling_priority = priority;
        } else {
          LOG << "real time priority should be between 1 and 99\n";
        }
      } else if (priority >= -20 && priority <= 19) {
        this->scheduling_priority = priority;
      } else {
        LOG << "niceness should be between -20 and 19\n";
      }
    } catch (...) {
      LOG << "Exception occured while converting " << command[1] << " to int\n";
    }
  }
}

// @Task 4:  Implement the SIGCHLD signal handler.

// This function is going to execute the shell command and going to execute
// wait, if the second parameter is true;
int app::execute(std::vector<string> &command, bool executingforeground) {
  pid_t w;
  int status;

  // Command string can contain the main command and a number of command line
  // arguments. We should allocate one extra element to have space for null.

  int commandLen = command.size();

  // If executing in background, remove "&" from command list passed to execvp
  if (!executingforeground && command[commandLen - 1] == "&") {
    commandLen--;
  }

  char **args = (char **)malloc((commandLen + 1) * sizeof(char *));
  for (int i = 0; i < commandLen; i++) {
    args[i] = strdup(command[i].c_str());
  }
  args[commandLen] = 0;

  // create a new process
  w = fork();
  if (w < 0) {
    LOG << "\nFork Failed " << errno << "\n";
    return 0;
  } else if (w == 0) {
    // @Task 5: Use the API to implement the memory limits, scheduling policy
    // and scheduling priority.

    if (this->virtual_memory_limit > 0) {
      struct rlimit rl;
      rl.rlim_cur = this->virtual_memory_limit;
      rl.rlim_max = this->virtual_memory_limit;
      int returncode = setrlimit(RLIMIT_AS, &rl);
      if (returncode == -1) {
        LOG << "Setting rlimit failed" << strerror(errno) << endl;
      }
    }
    if (this->scheduling_policy == SCHED_OTHER) {
      if (this->scheduling_priority >= -20 && this->scheduling_priority <= 19) {
        setpriority(PRIO_PROCESS, 0, this->scheduling_priority);
      } else {
        LOG << "Scheduling priority setting " << this->scheduling_priority
            << " does not match the limits for policy "
            << strpolicy(this->scheduling_policy) << endl;
      }
    } else if (this->scheduling_policy == SCHED_FIFO ||
               this->scheduling_policy == SCHED_RR) {
      if (this->scheduling_priority > 99 || this->scheduling_priority < 1) {
        LOG << "Scheduling priority setting " << this->scheduling_priority
            << " does not match the limits for policy "
            << strpolicy(this->scheduling_policy) << endl;
      } else {
        struct sched_param sp;
        sp.sched_priority = this->scheduling_priority;
        int retvalue = sched_setscheduler(0, this->scheduling_policy, &sp);
        if (retvalue < 0) {
          LOG << "error occured " << strerror(errno)
              << " . Try running the shell with sudo\n";
        }
      }
    } else {
      LOG << "Scheduling policy setting is unknown \n";
    }

    LOG << "Going to exec " << args[0] << "\n";
    execvp(args[0], args);
    LOG << "\nExec Failed " << errno << "\n";
    exit(2);
  } else {

    // return the child pid if we are not waiting in foreground
    if (!executingforeground)
      return w;

    int status;
    int retvalue = 0;
    while (retvalue != w)

    {
      status = 0;
      retvalue = waitpid(w, &status, 0);
      if (retvalue < 0)

      {
        char buffer[256];
        strerror_r(errno, buffer, 256);
        printf("error occured %s\n", buffer);
        break;
      }

      else

      {
        if (WIFEXITED(status)) {
          int exitcode = WEXITSTATUS(status);
          if (exitcode == 0)
            LOG << args[0] << ":Child Exited Successfully\n";
          else
            LOG << args[0] << ": Child Self Terminated With Exit Code "
                << exitcode << "\n";
        } else if (WIFSIGNALED(status)) {
          int signalsent = WTERMSIG(status);
          LOG << args[0] << ": Child Terminated Due to Signal " << signalsent
              << "\n";
        }
      }
    }
  }
  return 0;
}
