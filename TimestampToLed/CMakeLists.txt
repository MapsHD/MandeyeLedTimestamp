cmake_minimum_required(VERSION 3.5)
project(TimestampToLed)
find_path(LIBSERIAL_INCLUDE_DIR SerialPort.h
        PATH_SUFFIXES include
        PATHS /usr /usr/local /usr/include/libserial)
message(${LIBSERIAL_INCLUDE_DIR})
find_library(LIBSERIAL_LIBRARY
        NAMES libserial.so
        PATH_SUFFIXES lib
        PATHS /usr/lib)
        
include_directories(3rd/json)

add_executable(TimestampToLed TimestampToLed.cpp )
target_link_libraries(TimestampToLed ${LIBSERIAL_LIBRARY} zmq)


