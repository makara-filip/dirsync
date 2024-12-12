cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(DirSync)

add_executable(dirsync main.cpp
        arguments.cpp
        arguments.hpp
        help.hpp
        help.cpp
        synchronize.cpp
        synchronize.hpp)
