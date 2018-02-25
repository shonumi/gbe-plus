set(CMAKE_AUTOMOC ON)

set(SRCS
	main.cpp
	main_menu.cpp
	render.cpp
	general_settings.cpp
	qt_common.cpp
	cgfx.cpp
	debug_dmg.cpp
	screens.cpp
	data_dialog.cpp
	cheat_menu.cpp
	rtc_menu.cpp
	pp2_menu.cpp
	zzh_menu.cpp
	)

set(HEADERS
	main_menu.h
	render.h
	general_settings.h
	qt_common.h
	cgfx.h
	debug_dmg.h
	screens.h
	data_dialog.h
	cheat_menu.h
	rtc_menu.h
	pp2_menu.h
	zzh_menu.h
	)

add_executable(gbe_plus_qt ${SRCS} ${HEADERS})
target_link_libraries(gbe_plus_qt common gba dmg sgb nds)
target_link_libraries(gbe_plus_qt ${SDL2_LIBRARY} ${OPENGL_gl_LIBRARY} ${GBE_QT_LIBS})

if (LINK_CABLE)
    target_link_libraries(gbe_plus_qt ${SDL2NET_LIBRARY})
endif()

install(TARGETS gbe_plus_qt DESTINATION bin) 
