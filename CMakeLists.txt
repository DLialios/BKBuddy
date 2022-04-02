cmake_minimum_required(VERSION 3.23.0)

project(bkbuddy VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

add_executable(bkbmaster 
    src/bkbmaster.cxx
    )

target_link_libraries(bkbmaster PUBLIC
    X11
    )

target_include_directories(bkbmaster PUBLIC
    "${PROJECT_SOURCE_DIR}/includes"
    )
