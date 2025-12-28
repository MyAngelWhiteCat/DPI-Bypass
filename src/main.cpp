#include "dpi_bypasser.h"

int main() {
    DPIBypasser bypasser("outbound");
    bypasser.AddBypassRequiredHostname("osu.direct", BypassMethod::SIMPLE_SNI_FAKE);
    bypasser.AddBypassRequiredHostname("www.youtube.com", BypassMethod::SSF_FAKED_SPLIT);
    bypasser.AddBypassRequiredHostname("googlevideo.com", BypassMethod::SSF_FAKED_SPLIT);
    bypasser.AddBypassRequiredHostname("i.ytimg.com", BypassMethod::SSF_FAKED_SPLIT);
    bypasser.AddBypassRequiredHostname("yt3.ggpht.com", BypassMethod::SSF_FAKED_SPLIT);
    bypasser.AddBypassRequiredHostname("i9.ytimg.com", BypassMethod::SSF_FAKED_SPLIT);
    try {
        bypasser.Listen();
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
}