cmake_minimum_required(VERSION 3.6)
project(mafia)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES ua.cpp)
add_executable(mafia ${SOURCE_FILES})