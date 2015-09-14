set(CMAKE_AUTOMOC ON)

set(SRCS
	main.cpp
	main_menu.cpp
	render.cpp
	general_settings.cpp
	qt_common.cpp
	cgfx.cpp
	)

set(HEADERS
	main_menu.h
	render.h
	general_settings.h
	qt_common.h
	cgfx.h
	)

add_executable(gbe_plus_qt ${SRCS} ${HEADERS})
target_link_libraries(gbe_plus_qt common gba dmg nds)
target_link_libraries(gbe_plus_qt ${SDL_LIBRARY} ${OPENGL_gl_LIBRARY} ${GBE_QT_LIBS}) 
