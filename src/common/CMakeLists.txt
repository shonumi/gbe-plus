set(SRCS
	config.cpp
	hash.cpp
	util.cpp
	gx_util.cpp
	osd.cpp
	)

set(HEADERS
	common.h
	core_emu.h
	config.h
	hash.h
	util.h
	gx_util.h
	dmg_core_pad.h
	)


add_library(common STATIC ${SRCS} ${HEADERS})

target_link_libraries(common ${SDL2_LIBRARY})

if (USE_OGL)
    target_link_libraries(common ${OPENGL_gl_LIBRARY})
endif()

if (WIN32)
    target_link_libraries(common ${GLEW_LIBRARY})
endif()
