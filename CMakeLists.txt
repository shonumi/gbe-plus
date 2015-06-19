cmake_minimum_required(VERSION 2.8.11)

project(gbe_plus)

add_definitions(-O3 -funroll-loops)

find_package(SDL REQUIRED)
if (SDL_FOUND)
    include_directories(${SDL_INCLUDE_DIRS})
else()
    message(STATUS "SDL not found.")
endif()

find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIRS})
else()
    message(STATUS "OpenGL not found.")
endif()

option(QT_GUI "Enable the Qt GUI" ON)
if(QT_GUI)
    find_package(Qt4 REQUIRED COMPONENTS QtGui QtOpenGL)
    set(GBE_QT_LIBS Qt4::QtGui Qt4::QtOpenGL)
endif()

add_subdirectory(src)
