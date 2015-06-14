set(SRCS config.cpp)

set(HEADERS
	common.h
	core_emu.h
	config.h
	)


add_library(common STATIC ${SRCS} ${HEADERS})

target_link_libraries(common ${SDL_LIBRARY})
