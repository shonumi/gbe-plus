set(SRCS
	main_menu.cpp
	main.cpp
	)

set(HEADERS
	main_menu.h
	)

add_executable(gbe_plus_qt ${SRCS} ${HEADERS})
target_link_libraries(gbe_plus_qt ${GBE_QT_LIBS}) 
