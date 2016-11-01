#include "app.h"
#include "utilities.h"
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

int main(int argc, char *argv[]) {
  std::auto_ptr<app> a(new app);
  a->start();
}

void app::start() {
  char cstr[32];
  int command_num = 0; // record the number of the commands

  cerr << "\nvbash>>";
  cin.getline(cstr, 32);
  string command_str(cstr);
  cout << command_str << endl;
  LOG << "Command Entered: " << command_str << std::endl;

  command_num++; // each incoming command, increase by 1 
  char *buffer = new char[command_str.length() + 1];
  strcpy(buffer, command_str.c_str());

  // open a file in the current directory for recording commands
  int fd = open("commandsRecords", O_RDWR | O_CREAT | O_TRUNC,
                S_IRUSR | S_IWUSR); // open a file
  if (fd == -1) {
    cerr << "Unable to open a file" << endl;
  }

  // write into the file 
  if (write(fd, "All the input commands: ", 24) == -1 ||
      write(fd, " \n", 2) == -1 || write(fd, buffer, strlen(buffer)) == -1 ||
      write(fd, " \n", 2) == -1) {
    cerr << "Unable to write the command into the file" << endl;
  }

  // strip the command line of white space
  // tokenize the commands into chunks of two
  // With the "exit" command it exits the program
  while (command_str != "exit") {
    command_str = trim(command_str);
    if (command_str.size() != 0) {
    
      // implement the command to erase all history of task execution
      if (command_str.compare("erase history") == 0) {
        fd = open("commandsRecords", O_RDWR | O_CREAT | O_TRUNC,
                  S_IRUSR | S_IWUSR); // open a file
        command_num = 0;
        if (fd == -1) {
          cerr << "Unable to open a file" << endl;
        }
        if (write(fd, "You have already erased some commands!", 38) == -1 ||
            write(fd, " \n", 2) == -1) {
          cerr << "Unable to write the command into the file" << endl;
        }
      } else {
        // find content in string
        // Return the position of the first character of the first match.
        // If no matches were found, the function returns string::npos.
        std::size_t found = command_str.find("||");
        if (found == std::string::npos) {
          // tokenize by spaces.
          std::vector<string> command;
          tokenize_string(command_str, command, " "); // delimiter " "
          if (checkbuiltin(command)) {
            executebuiltin(command);
          } else {
            this->execute(command, checkforeground(command));
          }
        } else {
          this->parallel_execution(command_str);
        }
      }
      // Continuous input commands in while loop
      cerr << "\nvbash>>";
      getline(cin, command_str);
      LOG << "Command Entered: " << command_str << std::endl;

      command_num++;
      char *buffer = new char[command_str.length() + 1];
      strcpy(buffer, command_str.c_str());
      if (write(fd, buffer, strlen(buffer)) == -1 ||
          write(fd, " \n", 2) == -1) {
        cerr << "Unable to write the command into the file" << endl;
      }
      delete[] buffer;
    }
  }
  // finally, wirte the sum of the commands into the file
  char str[100];
  snprintf(str, command_num, "%d", command_num);
  if (write(fd, "The total number of commands launched: ", 39) == -1 ||
      write(fd, " \n", 2) == -1 || write(fd, str, strlen(str)) == -1) {
    cerr << "Unable to write the command into the file" << endl;
  }
  delete[] buffer;
  if (close(fd) == -1) {
    cerr << "fails to close the file" << endl;
  }
}
