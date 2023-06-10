#pragma once

#include <map>

namespace Selino::Config {
    static constexpr unsigned int ValidArgsCount = 3;
    static const std::pair<std::string, std::string> ValidArgsArr[ValidArgsCount] = {
        { "s", "script" },
        { "p", "port" },
        { "t", "timeout"}
    };
    extern std::unordered_map<std::string, std::vector<std::string>> ArgumentsProvided;
};
