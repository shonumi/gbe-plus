set(SRCS
	./../dmg/apu.cpp
	core.cpp
	gamepad.cpp
	lcd.cpp
	./../dmg/mbc1.cpp
	./../dmg/mbc2.cpp
	./../dmg/mbc3.cpp
	./../dmg/mbc5.cpp
	./../dmg/mbc7.cpp
	./../dmg/huc1.cpp
	./../dmg/mmm01.cpp
	./../dmg/camera.cpp
	./../dmg/mmu.cpp
	opengl.cpp
	z80.cpp
	./../dmg/sio.cpp
	./../dmg/dmg07.cpp
	./../dmg/gbma.cpp
	debug.cpp
	)

set(HEADERS
	./../dmg/apu.h
	./../dmg/apu_data.h
	./../dmg/common.h
	core.h
	gamepad.h
	lcd.h
	./../dmg/lcd_data.h
	./../dmg/mmu.h
	z80.h
	./../dmg/sio.h
	./../dmg/sio_data.h
	)

add_library(sgb STATIC ${SRCS} ${HEADERS})

target_link_libraries(sgb common SDL2::SDL2)

if (LINK_CABLE)
	target_link_libraries(sgb SDL2_net::SDL2_net)
endif()

if (USE_OGL)
	target_link_libraries(sgb OpenGL::GL)
endif()

if (WIN32)
	target_link_libraries(sgb GLEW::GLEW)
endif()
