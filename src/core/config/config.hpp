#pragma once

#include <map>
#include <string>
#include <tuple>

namespace Selino::Config {
    static constexpr unsigned int ValidArgsCount = 4;

    enum ArgumentType : std::uint8_t {
        PRESENCE_TOGGLE = 1, // The argument does not need to be accompanied by any other information, its presence is a flag.
        STRING_VALUE = 2, // The argument should be passed as a string (with data after it).
        SINGLE_VALUE = 4 // Only one argument of this name can be passed.
    };

    static const std::tuple<ArgumentType, std::pair<std::string, std::string>> ValidArgsArr[ValidArgsCount] = {
        { ArgumentType::STRING_VALUE, {"s", "script"} },
        { ArgumentType::STRING_VALUE, {"p", "port"} },
        { static_cast<ArgumentType>(ArgumentType::STRING_VALUE | ArgumentType::SINGLE_VALUE), {"t", "timeout"} },
        { ArgumentType::PRESENCE_TOGGLE, {"elc", "enable-local-connections"} }
    };
    extern std::unordered_map<std::string, std::vector<std::string>> ArgumentsProvided;

    void ParseArguments(const std::size_t argc, const char* const* const argv);
};
