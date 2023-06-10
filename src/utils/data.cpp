#include <format>
#include <map>
#include <string.h>

#include "./utils.hpp"

inline const std::string Selino::Utils::Data::ByteToHex(const std::uint8_t byte) {
    std::string hex_str("");
    std::function<char(std::uint8_t)> convert_nibble = [](std::uint8_t nibble) {
        return (char)(nibble + (nibble < 0xA ? 48 : 55));
    };
    hex_str += convert_nibble(byte >> 4); // Leftmost (most significant) nibble
    hex_str += convert_nibble(byte & 0x0F); // Rightmost (least significant) nibble
    return hex_str;
}

const std::string Selino::Utils::Data::Dump(const std::uint8_t* const buffer, const std::size_t length) {
    std::string hex_str("");
    for (std::size_t i = 0; i < length; i++) {
        hex_str += ByteToHex(buffer[i]) + (i == length - 1 ? "" : " ");
    }
    return hex_str;
}

const std::chrono::milliseconds Selino::Utils::Data::StringToDuration(const std::string& duration_string) {
    std::string duration("");
    std::string timescale("");
    bool passed_duration = false;
    for (const char& iterative_character : duration_string) {
        if (iterative_character == ' ' || iterative_character == ',') {
            continue;
        }
        if (!passed_duration && !std::isdigit(iterative_character)) {
            passed_duration = true;
        }
        if (passed_duration) {
            timescale += iterative_character;
        }
        else {
            duration += iterative_character;
        }
    }
    std::unordered_map<std::string, unsigned short> multiplication_factors = {
        { "ms", 1 },
        { "milliseconds", 1 },
        { "msecs", 1 },
        { "s", 1000 },
        { "seconds", 1000 },
        { "secs", 1000 },
        { "m", 60000 },
        { "minutes", 60000 },
        { "mins", 60000 }
    };
    const std::unordered_map<std::string, unsigned short>::const_iterator current_factor = multiplication_factors.find(timescale.length() == 0 ? "ms" : timescale);
    if (current_factor == multiplication_factors.cend()) {
        throw std::runtime_error("Invalid timescale");
    }
    return std::chrono::milliseconds(std::stoull(duration) * current_factor->second);
}
