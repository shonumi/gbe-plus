set(SRCS
	config.cpp
	util.cpp
	gx_util.cpp
	osd.cpp
	debug_util.cpp
	net_util.cpp
	)

set(HEADERS
	common.h
	core_emu.h
	config.h
	util.h
	gx_util.h
	dmg_core_pad.h
	debug_util.h
	net_util.h
	info.h
	)


add_library(common STATIC ${SRCS} ${HEADERS})

target_link_libraries(common SDL2::SDL2)

add_dependencies(common gbe_info)

if (IMAGE_FORMATS)
	target_link_libraries(common SDL2_image::SDL2_image)
endif()

if (USE_OGL)
	target_link_libraries(common OpenGL::GL)
endif()

if (USE_NETPLAY)
	target_link_libraries(common SDL2_net::SDL2_net)
endif()

if (WIN32)
	target_link_libraries(common GLEW::GLEW)
endif()
