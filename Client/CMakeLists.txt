cmake_minimum_required(VERSION 2.8)
project (FinalProjectClient)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g")
add_executable (client clientSocket.cpp PracticalSocket.cpp)


