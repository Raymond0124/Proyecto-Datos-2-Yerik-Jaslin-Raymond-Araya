cmake_minimum_required(VERSION 3.16)
project(MemoryProfiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Ruta a Qt (ajusta si es necesario)
set(CMAKE_PREFIX_PATH "C:/Users/raymo/Desktop/QT/6.9.2/mingw_64")

# Buscar módulos de Qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Charts)

# --- Biblioteca de instrumentalización ---
add_library(memory_profiler_lib SHARED
        library.cpp
        library.h
)

target_link_libraries(memory_profiler_lib
        PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Charts
)

# --- Interfaz gráfica ---
add_executable(memory_profiler_gui
        gui/main.cpp
        gui/mainwindow.cpp
        gui/mainwindow.h
)

# El ejecutable enlaza la librería propia
target_link_libraries(memory_profiler_gui PRIVATE memory_profiler_lib)

# El ejecutable enlaza Qt
target_link_libraries(memory_profiler_gui
        PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Charts
)

# Post-build: ejecutar windeployqt para copiar las DLLs necesarias
add_custom_command(TARGET memory_profiler_gui POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe" "$<TARGET_FILE:memory_profiler_gui>"
)
