#include "dpi_bypasser.h"
#include <filesystem>
#include <Windows.h>
#include <thread>
#include <chrono>

int main() {
    if (!std::filesystem::exists(std::filesystem::current_path() / "tls_clienthello_www_google_com.bin")) {
        std::cout << "Can't find tls_clienthello_www_google_com.bin file\n"
            << "Take it from here https://github.com/MyAngelWhiteCat/DPI-Bypass/releases/latest\n"
            << "And put it here -> " << std::filesystem::current_path().string() << "\n";
    }
    else {
        std::cout << "Getting ready...\n";
        DPIBypasser bypasser("outbound");
        std::cout << "Setup bypass rules...\n";
        bypasser.AddBypassRequiredHostname("osu.direct", BypassMethod::SIMPLE_SNI_FAKE);
        bypasser.AddBypassRequiredHostname("catboy.best", BypassMethod::SIMPLE_SNI_SPLIT);
        bypasser.AddBypassRequiredHostname("api.nerinyan.moe", BypassMethod::SIMPLE_SNI_FAKE);

        bypasser.AddBypassRequiredHostname("www.youtube.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("youtube.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("googlevideo.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("i.ytimg.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("yt3.ggpht.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("i9.ytimg.com", BypassMethod::SSF_FAKED_SPLIT);

        bypasser.AddBypassRequiredHostname("discord.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("discord", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("discord.gg", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("discord.media", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("mtalk.google.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("cloudflare-ech.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("login.live.com", BypassMethod::SSF_FAKED_SPLIT);

        std::cout << "Setted up!\n";
        try {
            std::cout << "5 seconds before start\nWindow will be hidden!\nRun KillProcess.bat for close DPIbypass" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5)); // For the user to read the information
            ShowWindow(GetConsoleWindow(), SW_HIDE); // Hide window
            bypasser.Start();
        }
        catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
}