set(CMAKE_AUTOMOC ON)

set(SRCS
	main.cpp
	main_menu.cpp
	render.cpp
	general_settings.cpp
	qt_common.cpp
	debug_dmg.cpp
	screens.cpp
	data_dialog.cpp
	cheat_menu.cpp
	rtc_menu.cpp
	pp2_menu.cpp
	zzh_menu.cpp
	ogl_manager.cpp
	con_ir_menu.cpp
	ps_menu.cpp
	mpos_menu.cpp
	tbf_menu.cpp
	utp_menu.cpp
	magic_reader_menu.cpp
	mw_menu.cpp
	wantame_menu.cpp
	wave_scanner_menu.cpp
	)

set(HEADERS
	main_menu.h
	render.h
	general_settings.h
	qt_common.h
	debug_dmg.h
	screens.h
	data_dialog.h
	cheat_menu.h
	rtc_menu.h
	pp2_menu.h
	zzh_menu.h
	ogl_manager.h
	con_ir_menu.h
	ps_menu.h
	mpos_menu.h
	tbf_menu.h
	utp_menu.h
	magic_reader_menu.h
	mw_menu.h
	wantame_menu.h
	wave_scanner_menu.h
	)

add_executable(gbe_plus_qt ${SRCS} ${HEADERS})
target_link_libraries(gbe_plus_qt common gba dmg sgb nds min)
target_link_libraries(gbe_plus_qt SDL2::SDL2 SDL2::SDL2main OpenGL::GL ${GBE_QT_LIBS})

if (LINK_CABLE)
	target_link_libraries(gbe_plus_qt SDL2_net::SDL2_net)
endif()

if (WIN32)
	target_link_libraries(gbe_plus_qt GLEW::GLEW)
endif()

install(TARGETS gbe_plus_qt DESTINATION ${CMAKE_INSTALL_BINDIR})
