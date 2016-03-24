include_directories(.)

add_subdirectory(common)
add_subdirectory(gba)
add_subdirectory(dmg)

if(QT_GUI)
    add_subdirectory(qt)
endif()

set(SRCS main.cpp)

SET(USER_HOME $ENV{HOME} CACHE STRING "Target User Home")
SET(USER $ENV{USER} CACHE STRING "Target User")

add_executable(gbe_plus ${SRCS})
target_link_libraries(gbe_plus common gba dmg)
target_link_libraries(gbe_plus ${SDL_LIBRARY} ${OPENGL_gl_LIBRARY})

install(TARGETS gbe_plus DESTINATION /usr/local/bin)
install(FILES gbe.ini DESTINATION ~/.gbe_plus/)
install(FILES data/gbe_plus.png DESTINATION ~/.gbe_plus/data/)
install(CODE "execute_process(COMMAND chown -R ${USER}:${USER} ${USER_HOME}/.gbe_plus)")
