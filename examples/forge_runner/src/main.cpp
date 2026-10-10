#include "Game.h"
#include <cstdio>

int main(int argc,char** argv) {
    bool smoke=false; std::filesystem::path data;
    for(int i=1;i<argc;++i) {
        std::string arg=argv[i];
        if(arg=="--smoke") smoke=true;
        else if(arg=="--data" && i+1<argc) data=argv[++i];
        else return 2;
    }
    if(smoke && data.empty()) return 2; // Automation always uses an isolated save directory.
    int result=0;
    try {
        if(data.empty()) data=RayGame::FileSystem::UserDataDirectory("raygame.forge");
        std::filesystem::create_directories(data);
        RayGame::Diagnostics::Initialize(data/"logs/game.log"); RayGame::Diagnostics::InstallRaylibCallback();
#if defined(__ANDROID__)
        std::string assets="forge"; // Relative to the APK's assets root.
#else
        std::string assets=std::string(GetApplicationDirectory())+"assets";
#endif
        if(smoke) SetConfigFlags(FLAG_WINDOW_HIDDEN);
        Forge::Game game(assets,data,smoke); game.Run();
        if(smoke) {
            if(game.art.body || game.art.glow || IsWindowReady() || IsAudioDeviceReady()) ++game.failures;
            std::printf("FORGE smoke: %d failures\n",game.failures);
            result=game.failures==0?0:1;
            RayGame::FileSystem::WriteAtomic(data/"smoke-result.txt",std::to_string(game.failures),false);
        }
    } catch(const std::exception& error) {
        RayGame::Diagnostics::Write(RayGame::LogLevel::Error,error.what()); result=1;
    }
    RayGame::Diagnostics::Shutdown(); return result;
}
