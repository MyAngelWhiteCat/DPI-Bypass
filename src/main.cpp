#include "dpi_bypasser.h"
#include <filesystem>

int main() {
    if (!std::filesystem::exists(std::filesystem::current_path() / "tls_clienthello_www_google_com.bin")) {
        std::cout << "Can't find tls_clienthello_www_google_com.bin file\n"
            << "Take it from here https://github.com/MyAngelWhiteCat/DPI-Bypass/releases/latest\n"
            << "And put it here -> " << std::filesystem::current_path().string() << "\n";
    }
    else {
        std::cout << "Geting ready..." << std::endl;
        DPIBypasser bypasser("outbound");
        bypasser.AddBypassRequiredHostname("osu.direct", BypassMethod::SIMPLE_SNI_FAKE);
        bypasser.AddBypassRequiredHostname("catboy.best", BypassMethod::SIMPLE_SNI_FAKE);
        bypasser.AddBypassRequiredHostname("api.nerinyan.moe", BypassMethod::SIMPLE_SNI_FAKE);
        bypasser.AddBypassRequiredHostname("www.youtube.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("youtube.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("googlevideo.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("i.ytimg.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("yt3.ggpht.com", BypassMethod::SSF_FAKED_SPLIT);
        bypasser.AddBypassRequiredHostname("i9.ytimg.com", BypassMethod::SSF_FAKED_SPLIT);
        try {
            std::cout << "System started!" << std::endl;
            bypasser.Listen();
        }
        catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
}