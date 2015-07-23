set(SRCS
	apu.cpp
	core.cpp
	gamepad.cpp
	lcd.cpp
	mbc1.cpp
	mbc2.cpp
	mbc3.cpp
	mbc5.cpp
	mmu.cpp
	opengl.cpp
	z80.cpp
	custom_graphics.cpp
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
	z80.h
	custom_graphics.h
	custom_graphics_data.h
	)

add_library(dmg STATIC ${SRCS} ${HEADERS})

target_link_libraries(dmg ${SDL_LIBRARY} ${OPENGL_gl_LIBRARY}) 
