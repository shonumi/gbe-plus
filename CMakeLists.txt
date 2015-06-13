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

add_subdirectory(src)
