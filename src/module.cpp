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

void runCmd(std::string input, int ref)
{
    input = std::regex_replace(input, std::regex("\""), "\"\"");

    std::string cmd = "\"";
    cmd += YTDL_PATH;
    cmd += " --dump-json --no-playlist \"";
    cmd += input;
    cmd += "\"\" 2>&1";

    FILE* pipe = popen(cmd.c_str(), "r");

    if (!pipe)
    {
        mtx.lock();
        results.push_back({ false, "failed to open ytdl", ref});
        mtx.unlock();
        return;
    }

    std::array<char, 128> buffer;
    std::string output;

    while (fgets(buffer.data(), sizeof(buffer), pipe) != NULL) {
        output += buffer.data();
    }

    bool success = pclose(pipe) == EXIT_SUCCESS;

    mtx.lock();
    results.push_back({ success, output, ref });
    mtx.unlock();
}

LUA_FUNCTION(ytdlThink)
{
    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->GetField(-1, "YTDL");
    if (!LUA->IsType(-1, Type::Nil)) {
        mtx.lock();
        if (!results.empty()) {
            Result res = results.front();
            results.erase(results.begin());
            mtx.unlock();

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
        else
        {
            mtx.unlock();
        }
    }
    LUA->Pop(2);

    return 0;
}

LUA_FUNCTION(Request)
{
    LUA->CheckType(1, Type::String);
    LUA->CheckType(2, Type::Function);

    LUA->Push(2);
    std::thread t(runCmd, LUA->GetString(1), LUA->ReferenceCreate());
    t.detach();

    return 0;
}

GMOD_MODULE_OPEN()
{
    LUA->PushSpecial(SPECIAL_GLOB);
        LUA->CreateTable();
            LUA->PushCFunction(Request);
            LUA->SetField(-2, "Request");
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