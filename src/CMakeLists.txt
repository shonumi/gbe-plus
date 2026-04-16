include_directories(.)

add_subdirectory(common)
add_subdirectory(gba)
add_subdirectory(dmg)
add_subdirectory(sgb)
add_subdirectory(nds)
add_subdirectory(min)

if (QT_GUI)
	add_subdirectory(qt)
endif()

set(SRCS main.cpp)

SET(USER_INSTALL_DIR $ENV{HOME} CACHE STRING "Target Installation Directory")
SET(USER $ENV{USER} CACHE STRING "Target User")

if ((USE_HIDDEN_INSTALL_FOLDER) AND (UNIX AND NOT APPLE))
	cmake_path(SET FINAL_INSTALL_DIR NORMALIZE ${USER_INSTALL_DIR}/.gbe_plus)
else()
	cmake_path(SET FINAL_INSTALL_DIR NORMALIZE ${USER_INSTALL_DIR}/gbe_plus)
endif()

#Build info.h on a per-system basis. Execute specific scripts for more control than CMake allows
add_custom_target(gbe_info ALL
	COMMENT "Building GBE+ info.h"
)

if (UNIX AND NOT APPLE)
	add_custom_command(TARGET gbe_info
		PRE_BUILD
		COMMAND /bin/sh ${CMAKE_CURRENT_SOURCE_DIR}/common/info/info.sh ${CMAKE_CURRENT_SOURCE_DIR}/common/info/ ${FINAL_INSTALL_DIR}
	)
elseif (WIN32)
	add_custom_command(TARGET gbe_info
		PRE_BUILD
		COMMAND cmd /c ${CMAKE_CURRENT_SOURCE_DIR}/common/info/info.bat ${CMAKE_CURRENT_SOURCE_DIR}/common/info/
	)
endif()

add_dependencies(gbe_info)

add_executable(gbe_plus ${SRCS})
target_link_libraries(gbe_plus common gba dmg sgb nds min)
target_link_libraries(gbe_plus SDL2::SDL2 SDL2::SDL2main)

if (USE_NETPLAY)
	target_link_libraries(gbe_plus SDL2_net::SDL2_net)
endif()

if (IMAGE_FORMATS)
	target_link_libraries(gbe_plus SDL2_image::SDL2_image)
endif()

if (USE_OGL)
	target_link_libraries(gbe_plus OpenGL::GL)
endif()

if (WIN32)
	target_link_libraries(gbe_plus GLEW::GLEW)
endif()

if (UNIX AND NOT APPLE)
	install(TARGETS gbe_plus DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(FILES gbe.ini DESTINATION ${FINAL_INSTALL_DIR})
	install(DIRECTORY data DESTINATION ${FINAL_INSTALL_DIR})
	install(CODE "execute_process(COMMAND chown -R ${USER}:${USER} ${FINAL_INSTALL_DIR})")
endif()
