cmake_minimum_required(VERSION 3.16)
project(SectionViewer LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Use Qt 5.x
find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick Network)

add_executable(${PROJECT_NAME}
    main.cpp
    SectionController.cpp
    resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE .)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::Network
)

# For Qt5 compatibility uncomment below and comment Qt6 lines
# find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick)
# target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick) 
