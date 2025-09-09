cmake_minimum_required(VERSION 3.16)
project(MemoryProfiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Ruta a Qt
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.2/mingw_64")

# --- Buscar módulos de Qt necesarios ---
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Charts)

# --- Biblioteca de instrumentalización ---
add_library(memory_profiler_lib SHARED
        library.cpp
        library.h
        memtracker.h
        memtracker.cpp
)

target_link_libraries(memory_profiler_lib
        PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Charts
)

# Forzar includes para CLion/Clangd
target_include_directories(memory_profiler_lib PRIVATE ${Qt6Charts_INCLUDE_DIRS})

# --- Interfaz gráfica ---
add_executable(memory_profiler_gui
        gui/main.cpp
        gui/mainwindow.cpp
        gui/mainwindow.h
        memtracker.h
)

target_link_libraries(memory_profiler_gui
        PRIVATE
        memory_profiler_lib
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Charts
)

target_include_directories(memory_profiler_gui PRIVATE ${Qt6Charts_INCLUDE_DIRS})

# Post-build: ejecutar windeployqt para copiar las DLLs necesarias
add_custom_command(TARGET memory_profiler_gui POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe" "$<TARGET_FILE:memory_profiler_gui>"
)

# --- Segundo ejecutable ---
add_executable(Proyecto-Datos-2-Yerik-Jaslin-Raymond-Araya
        library.cpp
        memtracker.cpp
        gui/main.cpp
        gui/mainwindow.cpp
        memtracker.cpp
        memtracker.h
)

target_link_libraries(Proyecto-Datos-2-Yerik-Jaslin-Raymond-Araya
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Charts
        Qt6::Network
)
