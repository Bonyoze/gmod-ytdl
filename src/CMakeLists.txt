set(SOURCES
	module.cpp)

add_library(ytdl SHARED ${SOURCES})
target_link_libraries(ytdl gmod-ytdl)

set_gmod_suffix_prefix(ytdl)