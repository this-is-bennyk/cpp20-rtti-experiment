cmake_minimum_required(VERSION 3.31)
project(VariantV4)

set(CMAKE_CXX_STANDARD 20)

add_executable(VariantV4 main.cpp Meta.cpp
        MetaConfig.h
        Program.cpp
        Program.h
        ProgramConfig.h)
