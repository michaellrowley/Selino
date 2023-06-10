#include <iostream>
#include <cstdint>
#include <map>

#include "./config.hpp"
#include "../scripting/lua/lua.hpp"
#include "./protocols/socks/socks.hpp"
#include "../utils/utils.hpp"

namespace Selino::Config {
    std::unordered_map<std::string, std::vector<std::string>> ArgumentsProvided = {};
};

namespace Selino::Utils::Networking {
    std::chrono::milliseconds TwoWayTimeout(3000);
};

int main(const int argc, const char* const* const argv) {
    for (unsigned int i = 1; i < argc; i++) {
        const std::string& argument_key = argv[i];
        for (unsigned int j = 0; j < Selino::Config::ValidArgsCount; j++) {

            const std::string clean_key = Selino::Config::ValidArgsArr[j].second;

            if (argument_key == "-" + Selino::Config::ValidArgsArr[j].first || argument_key == "--" + clean_key) {

                if (i == argc - 1) {
                    std::cerr << "Invalid final argument; expected script path/name." << std::endl;
                    return 0;
                }

                if (Selino::Config::ArgumentsProvided.find(clean_key) == Selino::Config::ArgumentsProvided.end()) {
                    Selino::Config::ArgumentsProvided.insert(std::pair<std::string, std::vector<std::string>>(clean_key, std::vector<std::string>(0)));
                }

                // This forces a second search but it doesn't really slow anything down.
                Selino::Config::ArgumentsProvided[clean_key].push_back(argv[++i]);

                break;
            }
            else if (j == Selino::Config::ValidArgsCount - 1) {
                std::cerr << "Unsupported argument provided: '" << argument_key << "'" << std::endl;
            }
        }
    }

    std::unordered_map<std::string, std::vector<std::string>>::const_iterator temp_pair =
        Selino::Config::ArgumentsProvided.find("timeout");
    if (temp_pair != Selino::Config::ArgumentsProvided.cend()) {
        if (temp_pair->second.size() != 1) {
            std::cerr << "Too many timeout parameters provided, (max: 1)" << std::endl;
            return 0;
        }
        else {
            Selino::Utils::Networking::TwoWayTimeout = Selino::Utils::Data::StringToDuration(temp_pair->second[0]);
        }
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
