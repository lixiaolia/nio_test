PROJECT(NIO_DEMO LANGUAGES CXX C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

add_executable(nio_server nio_server.cpp)
add_executable(nio_client nio_client.cpp)
