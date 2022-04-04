cmake_minimum_required(VERSION 3.23.0)
project(bkbuddy VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

option(BKBMASTER "Build the server" ON)
option(BKBSLAVE "Build the client" OFF)

if(BKBMASTER)
    add_subdirectory(bkbmaster)
endif()

if(BKBSLAVE)
    add_subdirectory(bkbslave)
endif()
