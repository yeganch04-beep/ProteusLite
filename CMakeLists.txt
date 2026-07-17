cmake_minimum_required(VERSION 3.19)
project(ProteusLite LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(ProteusLite
    WIN32 MACOSX_BUNDLE
    circuitcanvas.cpp
    circuitcanvas.h
    component.cpp
    component.h
    componentlistwidget.cpp
    componentlistwidget.h
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    newprojectdialog.cpp
    newprojectdialog.h
    node.cpp
    node.h
    pin.cpp
    pin.h
    startpage.cpp
    startpage.h
    wire.cpp
    wire.h
)

target_link_libraries(ProteusLite
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS ProteusLite
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ProteusLite
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
