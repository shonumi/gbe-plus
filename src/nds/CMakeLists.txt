set(SRCS
	core.cpp
	mmu.cpp
	arm9.cpp
	arm9_arm_instr.cpp
	arm9_thumb_instr.cpp
	swi.cpp
	)

set(HEADERS
	common.h
	core.h
	mmu.h
	arm9.h
	)

add_library(nds STATIC ${SRCS} ${HEADERS})

target_link_libraries(nds ${SDL_LIBRARY} ${OPENGL_gl_LIBRARY}) 
