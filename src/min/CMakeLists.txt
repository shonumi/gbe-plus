set(SRCS
	core.cpp
	mmu.cpp
	s1c88.cpp
	debug.cpp
	lcd.cpp
	opengl.cpp
	gamepad.cpp
	infrared.cpp
	apu.cpp
	)

set(HEADERS
	common.h
	core.h
	mmu.h
	s1c88.h
	timer.h
	lcd.h
	lcd_data.h
	gamepad.h
	apu_data.h
	apu.h
	)

add_library(min STATIC ${SRCS} ${HEADERS})

target_link_libraries(min common SDL2::SDL2)

if (LINK_CABLE)
	target_link_libraries(min SDL2_net::SDL2_net)
endif()

if (USE_OGL)
	target_link_libraries(min OpenGL::GL)
endif()

if (WIN32)
	target_link_libraries(min GLEW::GLEW)
endif() 
