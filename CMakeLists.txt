cmake_minimum_required(VERSION 3.19)
project(part5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Gui)

qt_standard_project_setup()

qt_add_executable(part5
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    canvasview.cpp
    canvasview.h
    componentlistwidget.cpp
    componentlistwidget.h
 circuitcomponent.cpp
    circuitcomponent.h
)

target_link_libraries(part5 PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)
