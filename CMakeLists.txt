cmake_minimum_required(VERSION 3.5)

project(gbe_plus)

if (NOT MSVC)
	add_definitions(-O3 -funroll-loops -Wno-error -Wno-narrowing -std=c++17)
endif()

if (MSVC AND WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
endif()

find_package(SDL2 REQUIRED)

option(LINK_CABLE "Enable link cable emulation (requires SDL2_net)" ON)

if (LINK_CABLE)
	find_package(SDL2_net REQUIRED)
	add_definitions(-DGBE_NETPLAY)
endif()

option(IMAGE_FORMATS "Enable image formats for screenshots and movies (requires SDL2_image)" OFF)

if (IMAGE_FORMATS)
	find_package(SDL2_image REQUIRED)
	add_definitions(-DGBE_IMAGE_FORMATS)
endif()

option(ADVANCED_DEBUG "Enable advanced debugging features (may affect performance)" OFF)

if (ADVANCED_DEBUG)
	add_definitions(-DGBE_DEBUG)
endif()

option(USE_OGL "Enable OpenGL for drawing operations (requires OpenGL)" ON)

option(FAST_FETCH "Enables fast instruction fetching on the GBA without memory checks. Offers a small speedup, on by default. Required to be off for Campho Advance emulation." ON)

if (FAST_FETCH)
	add_definitions(-DGBE_FAST_FETCH)
endif()

if (USE_OGL)
	set(OpenGL_GL_PREFERENCE GLVND)
	find_package(OpenGL REQUIRED)
	add_definitions(-DGBE_OGL)
endif()

if (WIN32 AND USE_OGL)
	find_package(GLEW REQUIRED)
	add_definitions(-DGBE_GLEW)
endif()

option(QT_GUI "Enable the Qt GUI" ON)

if(QT_GUI)
	find_package(Qt5OpenGL REQUIRED)
	find_package(Qt5Widgets REQUIRED)
	find_package(Qt5Gui REQUIRED)
	set(GBE_QT_LIBS Qt5::Gui Qt5::Widgets Qt5::OpenGL)
endif()

add_subdirectory(src)

include(GNUInstallDirs)
