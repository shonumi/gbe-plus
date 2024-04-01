set(SRCS
	config.cpp
	util.cpp
	gx_util.cpp
	osd.cpp
	)

set(HEADERS
	common.h
	core_emu.h
	config.h
	util.h
	gx_util.h
	dmg_core_pad.h
	)


add_library(common STATIC ${SRCS} ${HEADERS})

target_link_libraries(common SDL2::SDL2)

if (USE_OGL)
	target_link_libraries(common OpenGL::GL)
endif()

if (WIN32)
	target_link_libraries(common GLEW::GLEW)
endif()
