cmake_minimum_required(VERSION 3.16)
project(GestorDomotica VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DOMOTICA_BUILD_GUI "Compilar la aplicación Qt (requiere Qt Widgets y Qt Sql)" ON)
option(DOMOTICA_BUILD_TESTS "Compilar las pruebas unitarias del núcleo" ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---- Núcleo de dominio (C++ estándar, sin Qt) -------------------------------------------
add_library(domotica_core STATIC
    src/core/Types.cpp
    src/core/Time.cpp
    src/core/Sha256.cpp
    src/core/Device.cpp
    src/core/Energy.cpp
    src/core/Routine.cpp
    src/core/AuthService.cpp
    src/core/DeviceService.cpp
    src/core/Report.cpp)
target_include_directories(domotica_core PUBLIC src)

# ---- Aplicación Qt ----------------------------------------------------------------------
if(DOMOTICA_BUILD_GUI)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)
    set(CMAKE_AUTOMOC ON)

    add_executable(GestorDomotica
        src/main.cpp
        src/db/SqliteRepository.cpp
        src/gui/ChartWidgets.cpp
        src/gui/LoginDialog.cpp
        src/gui/DeviceDialog.cpp
        src/gui/RoutineDialog.cpp
        src/gui/MainWindow.cpp)
    target_link_libraries(GestorDomotica PRIVATE
        domotica_core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql)
    if(WIN32)
        set_target_properties(GestorDomotica PROPERTIES WIN32_EXECUTABLE ON)
    endif()
    # Ejecutable liviano: optimiza tamaño en Release
    if(NOT MSVC)
        target_compile_options(GestorDomotica PRIVATE $<$<CONFIG:Release>:-Os>)
        target_link_options(GestorDomotica PRIVATE $<$<CONFIG:Release>:-s>)
    endif()
endif()

# ---- Pruebas ----------------------------------------------------------------------------
if(DOMOTICA_BUILD_TESTS)
    enable_testing()
    add_executable(test_core tests/test_core.cpp)
    target_link_libraries(test_core PRIVATE domotica_core)
    add_test(NAME core COMMAND test_core)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(NAME sql COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_sql.py)
    endif()
endif()
