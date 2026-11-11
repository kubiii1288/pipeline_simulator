cmake_minimum_required(VERSION 4.3)
project(pipeline_simulator)

set(CMAKE_CXX_STANDARD 20)

add_executable(pipeline_simulator main.cpp
        RegisterFile.h
        RegisterFile.cpp
        Memory.cpp
        Memory.h
        Instruction.h
        Decoder.cpp
        Decoder.h
        SequentialCpu.cpp
        SequentialCpu.h)
