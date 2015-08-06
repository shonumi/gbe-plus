set(SRCS
	config.cpp
	hash.cpp
	util.cpp
	)

set(HEADERS
	common.h
	core_emu.h
	config.h
	hash.h
	util.h
	)


add_library(common STATIC ${SRCS} ${HEADERS})

target_link_libraries(common ${SDL_LIBRARY})
