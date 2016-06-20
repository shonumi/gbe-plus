set(SRCS
	config.cpp
	hash.cpp
	util.cpp
	cgfx_common.cpp
	)

set(HEADERS
	common.h
	core_emu.h
	config.h
	hash.h
	util.h
	cgfx_common.h
	)


add_library(common STATIC ${SRCS} ${HEADERS})

target_link_libraries(common ${SDL2_LIBRARY})
