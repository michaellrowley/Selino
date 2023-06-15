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

    // Argument settings, {shortname, longname}, examplevalue, explanation
    static const std::tuple<ArgumentType, std::pair<std::string, std::string>, std::string, std::string> ValidArgsArr[ValidArgsCount] = {
        { ArgumentType::STRING_VALUE, {"s", "script"}, "testing/logging.lua", "Selects a plugin to load from a file." },
        { ArgumentType::STRING_VALUE, {"p", "port"}, "1080", "Specifies the listening port of all proxies." },
        { static_cast<ArgumentType>(ArgumentType::STRING_VALUE | ArgumentType::SINGLE_VALUE), {"t", "timeout"}, "30ms", "The amount of time that should be allowed during a blind-relay before disconnecting both sides." },
        { ArgumentType::PRESENCE_TOGGLE, {"elc", "enable-local-connections"}, "", "If set, allows external clients to request local/private endpoints." }
    };
    extern std::unordered_map<std::string, std::vector<std::string>> ArgumentsProvided;

    void ParseArguments(const std::size_t argc, const char* const* const argv);
    const std::string GenerateHelp();
};
