cmake_minimum_required(VERSION 4.0)
project(Proyecto_Datos_2)

set(CMAKE_CXX_STANDARD 20)

add_library(Proyecto_Datos_2 SHARED library.cpp)
add_executable(MemoryProfilerGUI gui/main.cpp)

