cmake_minimum_required(VERSION 2.8)
project (Jiaqi)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g")
add_executable (server serverSocket.cpp PracticalSocket.cpp)
add_executable (shell main.cpp app.cpp utilities.cpp)


