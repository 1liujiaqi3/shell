#include <string.h>
#include <memory>
#include <vector>
#include <string>
#include "PracticalSocket.h" // For Socket and SocketException
#include <iostream>          // For cerr and cout
#include <cstdlib>           // For atoi()
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
const int RCVBUFSIZE = 32;
using namespace std;

// If the command string contains "sleep",
// reset the wait time for recv() function
int waitTime(string command);

int main(int argc, char *argv[]) {
  string servAddress = argv[1]; // First arg: server address
  unsigned short servPort = (argc == 3) ? atoi(argv[2]) : 20000;
  try {
    // Establish connection with the server
    TCPSocket sock(servAddress, servPort);
    // get the socket file descriptor.
    int fd = sock.getSocketDesc();
    // This struct is used to set the timeout span for
    // recv function so that the recv does not wait forever
    struct timeval tv;
    // timeout span is initialized to 1 second
    tv.tv_sec = 1;
    tv.tv_usec = 0; // Not init'ing this can cause strange errors
    // set the timeout span
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
               sizeof(struct timeval));

    char buffer[RCVBUFSIZE + 1];
    int bytesReceived = 0;
    while (1) {
      while (1) {
        memset(buffer, 0, RCVBUFSIZE + 1);
        try {
          // wait here for output from server
          bytesReceived = sock.recv(buffer, RCVBUFSIZE);
        } catch (SocketException &e) {
          // when time is out, print the message in current buffer
          cerr << buffer;
          break;
        }
        // print the output message;
        cerr << buffer;
      }
      string command_str;
      // ask the user to type in a command
      getline(cin, command_str);
      // update the timeout span according to the command entered
      tv.tv_sec = waitTime(command_str) + 1;
      setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
                 sizeof(struct timeval));
      // need to append "\n" to the end of commend
      // so that the server knows where command ends
      command_str = command_str + "\n";
      int commandLen = command_str.size();
      // send command to the server
      sock.send(command_str.c_str(), commandLen);
      // if command typed in is "exit", the client quits
      if (command_str == "exit") {
        break;
      }
    }
  } catch (SocketException &e) {
    cerr << e.what() << endl;
    exit(1);
  }
  return 0;
}

// If the command string contains "sleep",
// reset the wait time for recv() function
// else the timeout is set to 1 second
int waitTime(string command) {
  int time = 0;
  int index = command.find("sleep", 0);
  while (index >= 0) {
    index += 5;
    while (!isdigit(command[index])) {
      index++;
    }
    time += command[index] - '0';
    index = command.find("sleep", index);
  }
  return time;
}
