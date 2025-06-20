# CMake minimum and project for standalone build
cmake_minimum_required(VERSION 3.16)
project(ConfigApp LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Configurator sub-project

find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick Network)

add_executable(ConfigApp
    main.cpp
    ConfiguratorController.cpp
    config_resources.qrc
)

target_include_directories(ConfigApp PRIVATE .)

target_link_libraries(ConfigApp
    Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick Qt5::Network
) 
