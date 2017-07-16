cmake_minimum_required(VERSION 2.8.11)

project(gbe_plus)

add_definitions(-O3 -funroll-loops)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMakeTests)
include(FindSDL2 REQUIRED)

find_package(SDL2 REQUIRED)
if (SDL2_FOUND)
    include_directories(${SDL2_INCLUDE_DIRS})
else()
    message(STATUS "SDL2 not found.")
endif()

option(LINK_CABLE "Enable link cable emulation (requires SDL2_net)" ON)

if (LINK_CABLE)
    find_package(SDL2NET REQUIRED)
    if (SDL2NET_FOUND)
        include_directories(${SDL2NET_INCLUDE_DIR})
        add_definitions(-DGBE_NETPLAY)
    else()
        message(STATUS "SDL2_net not found.")
    endif()
endif()

option(ADVANCED_DEBUG "Enable advanced debugging features (may affect performance)" OFF)

if (ADVANCED_DEBUG)
    add_definitions(-DGBE_DEBUG)
endif()

find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIRS})
else()
    message(STATUS "OpenGL not found.")
endif()

option(QT_GUI "Enable the Qt GUI" ON)
option(QT_VERSION_4 "Use Qt 4" ON)
option(QT_VERSION_5 "Use Qt 5" OFF)

if(QT_VERSION_4 OR QT_VERSION_5)
    set(QT_GUI ON)
else()
    set(QT_GUI OFF)
endif()

if(QT_GUI)
    if(QT_VERSION_5)
    	find_package(Qt5OpenGL REQUIRED)
   	 find_package(Qt5Widgets REQUIRED)
   	 find_package(Qt5Gui REQUIRED)
  	  set(GBE_QT_LIBS Qt5::Gui Qt5::Widgets Qt5::OpenGL)
    elseif(QT_VERSION_4)
        find_package(Qt4 REQUIRED COMPONENTS QtGui QtOpenGL)
        set(GBE_QT_LIBS Qt4::QtGui Qt4::QtOpenGL)
    endif()
endif()

add_subdirectory(src)

SET(USER_HOME $ENV{HOME} CACHE STRING "Target User Home")
SET(USER $ENV{USER} CACHE STRING "Target User")

MESSAGE("-- Target User Home -> ${USER_HOME}")
MESSAGE("-- Target User -> ${USER}")
