set(SRCS
	config.cpp
	hash.cpp
	)

set(HEADERS
	common.h
	core_emu.h
	config.h
	hash.h
	)


add_library(common STATIC ${SRCS} ${HEADERS})

target_link_libraries(common ${SDL_LIBRARY})
