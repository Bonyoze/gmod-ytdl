#include "GarrysMod/Lua/Interface.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <regex>

#ifdef _WIN32
#define YTDL_PATH ".\\garrysmod\\lua\\bin\\yt-dlp.exe"
#define ARG_ESCAPE "^$1"
#define popen _popen
#define pclose _pclose
#else
#define YTDL_PATH "./garrysmod/lua/bin/yt-dlp_linux"
#define ARG_ESCAPE "\\$1"
#endif

struct Result
{
    bool success;
    std::string output;
    int callback_ref;
};

std::vector<Result> results;
std::mutex mtx;

const std::regex url_regex(R"(^https?://[0-9a-z\.-]+(:[1-9][0-9]*)?(/[^\s]*)*$)");
const std::regex arg_regex(R"(([\\^"]))");

int json_to_tbl_ref;
int err_no_halt_ref;

using namespace GarrysMod::Lua;

bool isValidURL(std::string str) {
    return std::regex_match(str, url_regex);
}

void runCmd(std::vector<std::string> args, int callback_ref)
{
    // build command
    std::string cmd = YTDL_PATH;
    for (auto& arg : args) {
        cmd += " \"";
        cmd += std::regex_replace(arg, arg_regex, ARG_ESCAPE); // escape certain characters
        cmd += "\"";
    }
    cmd += " 2>&1";

    FILE* pipe = popen(cmd.c_str(), "r");

    if (!pipe)
    {
        const std::lock_guard<std::mutex> lg(mtx);
        results.push_back({ false, "failed to open ytdl", callback_ref });
        return;
    }

    char buffer[128];
    std::string output;

    while (fgets(buffer, 128, pipe) != NULL) {
        output += buffer;
    }

    bool success = pclose(pipe) == EXIT_SUCCESS;

    const std::lock_guard<std::mutex> lg(mtx);
    results.push_back({ success, output, callback_ref });
}

// --playlist-items {amt} overrides total results from playlist urls and ytsearch{amt}:{search}

LUA_FUNCTION(GetInfo)
{
    LUA->CheckType(1, Type::String);
    LUA->CheckType(2, Type::Function);

    std::string input = LUA->GetString(1);
    if (!isValidURL(input)) // assume the input is a search if it's not a url
    {
        input = "ytsearch:" + input;
    }

    std::vector<std::string> args = { "--playlist-items", "1", "--dump-json", input };
    LUA->Push(2);
    int callback_ref = LUA->ReferenceCreate();

    std::thread t(runCmd, args, callback_ref);
    t.detach();

    return 0;
}

// ran in the Think hook
LUA_FUNCTION(ytdlThink)
{
    const std::lock_guard<std::mutex> lg(mtx);
    if (results.empty()) return 0;

    Result res = results.front();
    results.erase(results.begin());

    bool success = res.success;
    std::string output = res.output;
    int callback_ref = res.callback_ref;

    LUA->ReferencePush(err_no_halt_ref);
    if (success)
    {
        // parse json output to table
        LUA->ReferencePush(json_to_tbl_ref);
        LUA->PushString(output.c_str());
        if (LUA->PCall(1, 1, -3) == 0 && LUA->IsType(-1, Type::Table))
        {
            // run callback function (got request info)
            LUA->ReferencePush(callback_ref);
            LUA->ReferenceFree(callback_ref);
            // info table
            LUA->Push(-2);
            if (LUA->PCall(1, 0, -4) != 0) LUA->Pop();
        }
        else
        {
            // run callback function (json parse failed/ytsearch failed)
            LUA->ReferencePush(callback_ref);
            LUA->ReferenceFree(callback_ref);
            // nil and error string
            LUA->PushNil();
            LUA->PushString("failed to get info");
            if (LUA->PCall(2, 0, -4) != 0) LUA->Pop();
        }
        LUA->Pop();
    }
    else
    {
        // run callback function (ytdl request failed)
        LUA->ReferencePush(callback_ref);
        LUA->ReferenceFree(callback_ref);
        // nil and error string
        LUA->PushNil();
        LUA->PushString(output.c_str());
        if (LUA->PCall(2, 0, -4) != 0) LUA->Pop();
    }
    LUA->Pop();

    return 0;
}

GMOD_MODULE_OPEN()
{
    LUA->PushSpecial(SPECIAL_GLOB);

    // define YTDL
    LUA->CreateTable();
    LUA->PushCFunction(GetInfo);
    LUA->SetField(-2, "GetInfo");
    LUA->SetField(-2, "YTDL");

    // add Think hook for running callback functions
    LUA->GetField(-1, "hook");
    LUA->GetField(-1, "Add");
    LUA->PushString("Think");
    LUA->PushString("__ytdlThink");
    LUA->PushCFunction(ytdlThink);
    LUA->Call(3, 0);
    LUA->Pop();

    // create a reference to util.JSONToTable
    LUA->GetField(-1, "util");
    LUA->GetField(-1, "JSONToTable");
    json_to_tbl_ref = LUA->ReferenceCreate();
    LUA->Pop();

    // create a reference to ErrorNoHalt
    LUA->GetField(-1, "ErrorNoHalt");
    err_no_halt_ref = LUA->ReferenceCreate();
    
    LUA->Pop();

    return 0;
}

GMOD_MODULE_CLOSE()
{
    LUA->PushSpecial(SPECIAL_GLOB);

    // cleanup YTDL
    LUA->PushNil();
    LUA->SetField(-2, "YTDL");

    // remove Think hook
    LUA->GetField(-1, "hook");
    LUA->GetField(-1, "Remove");
    LUA->PushString("Think");
    LUA->PushString("__ytdlThink");
    LUA->Call(2, 0);

    LUA->Pop(2);

    // free reference to util.JSONToTable
    LUA->ReferenceFree(json_to_tbl_ref);

    // free reference to ErrorNoHalt
    LUA->ReferenceFree(err_no_halt_ref);

    return 0;
}