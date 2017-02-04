set(SRCS
	apu.cpp
	arm7.cpp
	arm_instr.cpp
	core.cpp
	dma.cpp
	gamepad.cpp
	lcd.cpp
	mmu.cpp
	opengl.cpp
	swi.cpp
	thumb_instr.cpp
	gpio.cpp
	)

set(HEADERS
	apu.h
	apu_data.h
	arm7.h
	common.h
	core.h
	gamepad.h
	lcd.h
	lcd_data.h
	mmu.h
	timer.h
	)

add_library(gba STATIC ${SRCS} ${HEADERS})

target_link_libraries(gba ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY}) 
