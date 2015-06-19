include_directories(.)

add_subdirectory(common)
add_subdirectory(gba)
add_subdirectory(dmg)

if(QT_GUI)
    add_subdirectory(qt)
endif()

set(SRCS main.cpp)

add_executable(gbe_plus ${SRCS})
target_link_libraries(gbe_plus common gba dmg)
target_link_libraries(gbe_plus ${SDL_LIBRARY} ${OPENGL_gl_LIBRARY})
