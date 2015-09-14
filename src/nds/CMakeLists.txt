set(SRCS
	core.cpp
	)

set(HEADERS
	common.h
	core.h
	)

add_library(nds STATIC ${SRCS} ${HEADERS})

target_link_libraries(nds ${SDL_LIBRARY} ${OPENGL_gl_LIBRARY}) 
