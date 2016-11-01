#ifndef APP_H
#define APP_H

#include <iostream>
#include <sched.h>
#include <vector>
#include <string>
#include <map>
class app {
  int execute(std::vector<std::string> &str, bool executingforeground);
  int parallel_execution(std::string command_string);
  int executebuiltin(std::vector<std::string> &command);
  int virtual_memory_limit;
  int scheduling_policy;
  int scheduling_priority;

public:
  app();
  ~app();
  void start();
};

#endif // APP_H
