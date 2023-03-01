#include "module.h"

using namespace GarrysMod::Lua;

struct Result
{
    bool success;
    std::string output;
    int ref;
};

std::vector<Result> results;
std::mutex mtx;
const std::regex url_regex("(^https?://[0-9a-z\.-]+(:[1-9][0-9]*)?(/[^\s]*)*$)");

bool isValidURL(std::string str) {
    return std::regex_match(str, url_regex);
}

void runCmd(std::vector<std::string> args, int ref)
{
    // build command
    std::string cmd = "\"";
    cmd += YTDL_PATH;
    for (auto& arg : args) {
        cmd += " \"";
        cmd += std::regex_replace(arg, std::regex("\""), "\"\""); // escape double quotes
        cmd += "\"";
    }
    cmd += "\" 2>&1";

    FILE* pipe = popen(cmd.c_str(), "r");

    if (!pipe)
    {
        const std::lock_guard<std::mutex> lg(mtx);
        results.push_back({ false, "failed to open ytdl", ref});
        return;
    }

    char buffer[128];
    std::string output;

    while (fgets(buffer, 128, pipe) != NULL) {
        output += buffer;
    }

    bool success = pclose(pipe) == EXIT_SUCCESS;

    const std::lock_guard<std::mutex> lg(mtx);
    results.push_back({ success, output, ref });
}

LUA_FUNCTION(ytdlThink)
{
    const std::lock_guard<std::mutex> lg(mtx);
    if (!results.empty()) {
        Result res = results.front();
        results.erase(results.begin());

        bool success = res.success;
        std::string output = res.output;
        int ref = res.ref;

        LUA->ReferencePush(ref);
        LUA->ReferenceFree(ref);
        if (success)
        {
            LUA->PushString(output.c_str());
            LUA->Call(1, 0);
        }
        else
        {
            LUA->PushNil();
            LUA->PushString(output.c_str());
            LUA->Call(2, 0);
        }
    }

    return 0;
}

// --playlist-items {amt} overrides total results from playlist urls and ytsearch{amt}:{search}

// get all json data from input
LUA_FUNCTION(GetJSON)
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
    int ref = LUA->ReferenceCreate();

    std::thread t(runCmd, args, ref);
    t.detach();

    return 0;
}

// get only the url from input
// note: this may return multiple urls for both video and audio (seperated by new line)
LUA_FUNCTION(GetURL)
{
    LUA->CheckType(1, Type::String);
    LUA->CheckType(2, Type::Function);

    std::string input = LUA->GetString(1);
    if (!isValidURL(input)) // assume the input is a search if it's not a url
    {
        input = "ytsearch:" + input;
    }

    std::vector<std::string> args = { "--playlist-items", "1", "--get-url", input };
    LUA->Push(2);
    int ref = LUA->ReferenceCreate();

    std::thread t(runCmd, args, ref);
    t.detach();

    return 0;
}

GMOD_MODULE_OPEN()
{
    LUA->PushSpecial(SPECIAL_GLOB);
        LUA->CreateTable();
            LUA->PushCFunction(GetJSON);
            LUA->SetField(-2, "GetJSON");
            LUA->PushCFunction(GetURL);
            LUA->SetField(-2, "GetURL");
        LUA->SetField(-2, "YTDL");

        LUA->GetField(-1, "hook");
        LUA->GetField(-1, "Add");
            LUA->PushString("Think");
            LUA->PushString("__ytdlThink");
            LUA->PushCFunction(ytdlThink);
        LUA->Call(3, 0);
        LUA->Pop();
    LUA->Pop();

    return 0;
}

GMOD_MODULE_CLOSE()
{
    return 0;
}