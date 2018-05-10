include_directories(.)

add_subdirectory(common)
add_subdirectory(gba)
add_subdirectory(dmg)
add_subdirectory(sgb)
add_subdirectory(nds)

if(QT_GUI)
    add_subdirectory(qt)
endif()

set(SRCS main.cpp)

SET(USER_HOME $ENV{HOME} CACHE STRING "Target User Home")
SET(USER $ENV{USER} CACHE STRING "Target User")

add_executable(gbe_plus ${SRCS})
target_link_libraries(gbe_plus common gba dmg sgb nds)
target_link_libraries(gbe_plus ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY})

if (LINK_CABLE)
    target_link_libraries(gbe_plus ${SDL2NET_LIBRARY})
endif()

if (WIN32)
    target_link_libraries(gbe_plus ${GLEW_LIBRARY})
endif()

if(UNIX AND NOT APPLE)
	install(TARGETS gbe_plus DESTINATION /usr/local/bin)
	install(FILES gbe.ini DESTINATION ${USER_HOME}/.gbe_plus/)
	install(DIRECTORY data DESTINATION ${USER_HOME}/.gbe_plus/)
	install(CODE "execute_process(COMMAND chown -R ${USER}:${USER} ${USER_HOME}/.gbe_plus)")
endif()
