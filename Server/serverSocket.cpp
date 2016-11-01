#include "PracticalSocket.h" // For Socket, ServerSocket, and SocketException
#include <iostream>          // For cerr and cout
#include <cstdlib>           // For atoi()
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <unistd.h> // For file write(), read()
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h> // For getrusage()
#include <sys/stat.h>
#include <fcntl.h> // For file open()
using namespace std;

const unsigned int RCVBUFSIZE = 32; // Size of receive buffer

int main(int argc, char *argv[]) {
  if (argc != 2) { // Test for correct number of arguments
    cerr << "Usage: " << argv[0] << " <Server Port>" << endl;
    exit(1);
  }
  unsigned short servPort = atoi(argv[1]); // First arg: local port
  struct rusage r_usage;                   // struct updated by getrusage()
  struct timeval user_time;                // user CPU time used
  struct timeval sys_time;                 // system CPU time used
  // the largest amount of physical memory occupied by
  // the process's data
  long mem_usage;

  try {
    TCPServerSocket servSock(servPort); // Server Socket object
    cout << "Hi! I am a server!" << endl;
    for (;;) { // Run forever
      // Wait for a client to connect
      TCPSocket *csock;
      csock = servSock.accept();
      // get the socket file descriptor
      int sockDesc = csock->getSocketDesc();
      cout << "Handling client ";
      try {
        cout << csock->getForeignAddress() << ":";
      } catch (SocketException e) {
        cerr << "Unable to get foreign address" << endl;
      }
      try {
        cout << csock->getForeignPort();
      } catch (SocketException e) {
        cerr << "Unable to get foreign port" << endl;
      }
      cout << endl;

      pid_t pid = fork();
      if (pid < 0) {
        std::cout << "\nFork Failed " << errno << "\n";
        exit(1);
      } else if (pid) { // in parent process
        // no longer need this copy in parent
        close(sockDesc);

        // wait until all the children processes terminated
        for (;;) {
          int childPid = waitpid(-1, NULL, 0);
          if (childPid == -1)
            break;
        }

        // The CPU time and memory utilization of all children that terminated
        if (getrusage(RUSAGE_CHILDREN, &r_usage) == -1) {
          cerr << "getrusage() failed" << endl;
        }
        mem_usage = r_usage.ru_maxrss; // cumulative update mem_usage
        user_time.tv_sec = r_usage.ru_utime.tv_sec;   // update user CPU seconds
        user_time.tv_usec = r_usage.ru_utime.tv_usec; // update microseconds
        sys_time.tv_sec = r_usage.ru_stime.tv_sec;
        sys_time.tv_usec = r_usage.ru_stime.tv_usec;

        // open a file to record utilization
        int fd = open("utilizationRecords", O_RDWR | O_CREAT | O_TRUNC,
                      S_IRUSR | S_IWUSR); // open a file
        if (fd == -1) {
          cerr << "Unable to open a file" << endl;
        }

        // write(fd, (char *)&user_time, sizeof(timeval)) == -1 can only write
        // binary data
        // using snprintf() to convert int to char array
        char str1[150], str2[150], str3[150];
        snprintf(str1, 149, "The total memory usage is: %lu kilobytes",
                 mem_usage);
        snprintf(
            str2, 149,
            "The total user CPU time used is: %lu seconds + %lu microseconds",
            user_time.tv_sec, user_time.tv_usec);
        snprintf(
            str3, 149,
            "The total system CPU time used is: %lu seconds + %lu microseconds",
            sys_time.tv_sec, sys_time.tv_usec);
        // write info into the file
        if (write(fd, "The utilization are: ", 21) == -1 ||
            write(fd, " \n", 2) == -1 || write(fd, str1, strlen(str1)) == -1 ||
            write(fd, " \n", 2) == -1 || write(fd, str2, strlen(str2)) == -1 ||
            write(fd, " \n", 2) == -1 || write(fd, str3, strlen(str3)) == -1 ||
            write(fd, " \n", 2) == -1) {
          cerr << "Unable to write the command into the file" << endl;
        }

        if (close(fd) == -1) {
          cerr << "fails to close the file" << endl;
        }

        // print out on server's side
        cout << "The total memory usage is: " << mem_usage << endl;
        cout << "The total user CPU time used is: " << user_time.tv_sec << "+"
             << user_time.tv_usec << endl;
        cout << "The total system CPU time used is: " << sys_time.tv_sec << "+"
             << sys_time.tv_usec << endl;
      } else { // in children process
        // redirect the stdin, stdout and stderr to socket
        // file descriptor
        dup2(sockDesc, 0);
        dup2(sockDesc, 1);
        dup2(sockDesc, 2);
        execlp("./shell", "shell", (char *)NULL);
        std::cout << "error occured while running ls\n";
      }
    }
  } catch (SocketException &e) {
    cerr << e.what() << endl;
    exit(1);
  }
  // NOT REACHED
  return 0;
}
