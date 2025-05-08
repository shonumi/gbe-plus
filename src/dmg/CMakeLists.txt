set(SRCS
	apu.cpp
	core.cpp
	gamepad.cpp
	lcd.cpp
	mbc1.cpp
	mbc2.cpp
	mbc3.cpp
	mbc5.cpp
	mbc6.cpp
	mbc7.cpp
	huc1.cpp
	huc3.cpp
	mmm01.cpp
	camera.cpp
	tama5.cpp
	mmu.cpp
	opengl.cpp
	sm83.cpp
	sio.cpp
	infrared.cpp
	dmg07.cpp
	gbma.cpp
	debug.cpp
	gb_kiss_link.cpp
	)

set(HEADERS
	apu.h
	apu_data.h
	common.h
	core.h
	gamepad.h
	lcd.h
	lcd_data.h
	mmu.h
	sm83.h
	sio.h
	sio_data.h
	)

add_library(dmg STATIC ${SRCS} ${HEADERS})

target_link_libraries(dmg common SDL2::SDL2)

if (LINK_CABLE)
	target_link_libraries(dmg SDL2_net::SDL2_net)
endif()

if (USE_OGL)
	target_link_libraries(dmg OpenGL::GL)
endif()

if (WIN32)
	target_link_libraries(dmg GLEW::GLEW)
endif()
