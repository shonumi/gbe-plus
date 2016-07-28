set(SRCS
	core.cpp
	mmu.cpp
	arm9.cpp
	arm9_arm_instr.cpp
	arm9_thumb_instr.cpp
	lcd.cpp
	swi.cpp
	cp15.cpp
	dma.cpp
	)

set(HEADERS
	common.h
	core.h
	mmu.h
	arm9.h
	lcd.h
	cp15.h
	)

add_library(nds STATIC ${SRCS} ${HEADERS})

target_link_libraries(nds ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY}) 
