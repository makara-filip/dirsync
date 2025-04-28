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
        synchronize.hpp
        configuration/configuration.cpp
        configuration/configuration.hpp
        configuration/configuration-json.cpp
        configuration/configuration-json.hpp
        wildcards.cpp
        wildcards.hpp
        tests.cpp
        tests.hpp
        synchronize_two_way.cpp
        synchronize_two_way.hpp
        synchronize_one_way.cpp
        synchronize_one_way.hpp
)