cmake_minimum_required(VERSION 3.11)

##############################################################
# Some common stuff
##############################################################
set(CMAKE_CXX_STANDARD 20 CACHE STRING "CXX Standard")
set(CMAKE_VERBOSE_MAKEFILE true CACHE BOOL "verbose make output")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -g -O3)

##############################################################
# Define the project
##############################################################
project(simulator LANGUAGES CXX)
add_library(miningsim
"src/simulation.h"
"src/simulation.cpp"
"src/stations.h"
"src/stations.cpp"
"src/timerservice.h"
"src/timerservice.cpp"
"src/truck.h"
"src/truck.cpp"
)

add_executable(${PROJECT_NAME} "src/miningsim.cpp")
target_link_libraries(${PROJECT_NAME} miningsim boost_program_options)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

##############################################################
# Add tests
##############################################################

message("Adding test dir")
add_subdirectory("test")
