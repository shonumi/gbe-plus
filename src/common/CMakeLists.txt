set(SRCS
	config.cpp
	hash.cpp
	util.cpp
	gx_util.cpp
	cgfx_common.cpp
	osd.cpp
	)

set(HEADERS
	common.h
	core_emu.h
	config.h
	hash.h
	util.h
	gx_util.h
	cgfx_common.h
	dmg_core_pad.h
	)


add_library(common STATIC ${SRCS} ${HEADERS})

target_link_libraries(common ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY})

if (WIN32)
    target_link_libraries(common ${GLEW_LIBRARY})
endif()
