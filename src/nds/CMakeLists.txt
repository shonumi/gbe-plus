set(SRCS
	core.cpp
	mmu.cpp
	)

set(HEADERS
	common.h
	core.h
	mmu.h
	)

add_library(nds STATIC ${SRCS} ${HEADERS})

target_link_libraries(nds ${SDL_LIBRARY} ${OPENGL_gl_LIBRARY}) 
