include_directories(.)

add_subdirectory(common)
add_subdirectory(gba)
add_subdirectory(dmg)
add_subdirectory(sgb)
add_subdirectory(nds)
add_subdirectory(min)

if(QT_GUI)
	add_subdirectory(qt)
endif()

set(SRCS main.cpp)

SET(USER_HOME $ENV{HOME} CACHE STRING "Target User Home")
SET(USER $ENV{USER} CACHE STRING "Target User")

add_executable(gbe_plus ${SRCS})
target_link_libraries(gbe_plus common gba dmg sgb nds min)
target_link_libraries(gbe_plus SDL2::SDL2 SDL2::SDL2main)

if (LINK_CABLE)
	target_link_libraries(gbe_plus SDL2_net::SDL2_net)
endif()

if (USE_OGL)
	target_link_libraries(gbe_plus OpenGL::GL)
endif()

if (WIN32)
	target_link_libraries(gbe_plus GLEW::GLEW)
endif()

if(UNIX AND NOT APPLE)
	install(TARGETS gbe_plus DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(FILES gbe.ini DESTINATION ${USER_HOME}/.gbe_plus/)
	install(DIRECTORY data DESTINATION ${USER_HOME}/.gbe_plus)
	install(CODE "execute_process(COMMAND chown -R ${USER}:${USER} ${USER_HOME}/.gbe_plus)")
endif()
