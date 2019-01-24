CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(simpleTestCRC)
SET(CMAKE_CXX_STANDARD 14)

# Without this threading does not work
SET(CMAKE_CXX_FLAGS -pthread)

# Just two files, this will do.
ADD_EXECUTABLE(simpleTestCRC main.cpp 3rd_party/CRC.h)