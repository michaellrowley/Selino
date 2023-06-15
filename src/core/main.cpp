#include <iostream>
#include <cstdint>
#include <map>

#include "./config/config.hpp"
#include "../scripting/lua/lua.hpp"
#include "./protocols/socks/socks.hpp"
#include "../utils/utils.hpp"

namespace Selino {
    namespace Config {
        std::unordered_map<std::string, std::vector<std::string>> ArgumentsProvided = {};
    };
    namespace Utils::Networking {
        std::chrono::milliseconds TwoWayTimeout(3000);
    };
};

int main(const int argc, const char* const* const argv) {
    try {
        Selino::Config::ParseArguments(argc, argv);
    } catch (std::runtime_error& err) {
        std::cerr << err.what() << "\n\n" << Selino::Config::GenerateHelp() << std::endl;
        return 0;
    }

    std::unordered_map<std::string, std::vector<std::string>>::const_iterator temp_pair =
        Selino::Config::ArgumentsProvided.find("timeout");
    if (temp_pair != Selino::Config::ArgumentsProvided.cend()) {
        Selino::Utils::Networking::TwoWayTimeout = Selino::Utils::Data::StringToDuration(temp_pair->second[0]);
    }

    for (const std::string& script_path : Selino::Config::ArgumentsProvided["script"]) {
        try {
            Selino::Scripting::Lua::Load::Script(std::filesystem::path(script_path));
        } catch (std::runtime_error&) {
            std::cerr << "Failed to load script from '" << script_path << "'" << std::endl;
            return 0;
        }
    }

    const std::function<void(const std::uint16_t)> socks_listen = [](const std::uint16_t port){
        Selino::Protocols::SOCKS proxy_instance;
        proxy_instance.Listen(port, true);
    };
    for (const std::string& listen_port_string : Selino::Config::ArgumentsProvided["port"]) {
        const int listen_port_wide = std::stoi(listen_port_string);
        if (listen_port_wide > UINT16_MAX || listen_port_wide <= 0) {
            std::cerr << "Invalid listening port specified ('" << listen_port_string << "')" << std::endl;
            return 0;
        }
        std::thread(socks_listen, *reinterpret_cast<const std::uint16_t*>(&listen_port_wide)).detach();
    }
    while (true) {
        std::this_thread::sleep_for(std::chrono::minutes(60));
    }
    return 1;
}
