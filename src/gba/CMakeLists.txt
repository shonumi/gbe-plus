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
	debug.cpp
	cheats.cpp
	sio.cpp
	gbma.cpp
	jukebox.cpp
	am3.cpp
	play_yan.cpp
	campho.cpp
	glucoboy.cpp
	nmp.cpp
	tv_tuner.cpp
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
	sio_data.h
	sio.h
	)

add_library(gba STATIC ${SRCS} ${HEADERS})

target_link_libraries(gba SDL2::SDL2)

if (LINK_CABLE)
	target_link_libraries(gba SDL2_net::SDL2_net)
endif()

if (USE_OGL)
	target_link_libraries(gba OpenGL::GL)
endif()

if (WIN32)
	target_link_libraries(gba GLEW::GLEW)
endif()
