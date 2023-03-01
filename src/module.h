#include "GarrysMod/Lua/Interface.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <regex>

#ifdef _WIN32
#define YTDL_NAME "yt-dlp.exe"
#define popen _popen
#define pclose _pclose
#else
#define YTDL_NAME "yt-dlp_linux"
#endif

#define YTDL_PATH ".\\garrysmod\\lua\\bin\\" YTDL_NAME