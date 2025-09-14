cmake_minimum_required(VERSION 3.16)
project(MemoryProfiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Ruta a Qt
set(CMAKE_PREFIX_PATH "C:/Users/raymo/Desktop/QT/6.9.2/mingw_64")

# --- Buscar módulos de Qt necesarios ---
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Charts)

# ========================
# --- Biblioteca de Instrumentalización ---
# ========================
add_library(memory_profiler_lib STATIC
        library.cpp
        library.h
        memtracker.cpp
        memtracker.h
)

target_link_libraries(memory_profiler_lib
        PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Charts
)

target_include_directories(memory_profiler_lib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}  # para que los includes .h sean visibles
)

# ========================
# --- Interfaz Gráfica (GUI) ---
# ========================
add_executable(memory_profiler_gui
        gui/main.cpp
        gui/mainwindow.cpp
        gui/mainwindow.h
)

target_link_libraries(memory_profiler_gui
        PRIVATE
        memory_profiler_lib
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Charts
)

target_include_directories(memory_profiler_gui
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Post-build: copiar DLLs de Qt automáticamente (Windows)
add_custom_command(TARGET memory_profiler_gui POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe" "$<TARGET_FILE:memory_profiler_gui>"
)

# ========================
# --- Ejecutable de prueba ---
# ========================
# Asegúrate de crear tests/test_main.cpp
add_executable(test_memory_profiler
        Test/test_main.cpp
)

target_link_libraries(test_memory_profiler
        PRIVATE
        memory_profiler_lib
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Charts
)

target_include_directories(test_memory_profiler
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
