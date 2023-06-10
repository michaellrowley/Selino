#include "./utils.hpp"

#include <bit>

// If you want to avoid using the std::endian data, cast the first byte of 0x0100 to an 'is_big_endian' boolean and work from there. Since we're using C++20 this isn't a concern.

template <typename T>
T Selino::Utils::Endianness::ToNetworkOrder(const T& original_value) {
    if (std::endian::native == std::endian::big) {
        return original_value;
    }

    T new_value;
    for (std::size_t byte_index = 0; byte_index < sizeof(T); byte_index++) {
        reinterpret_cast<std::uint8_t*>(&new_value)[(sizeof(T) - 1) - byte_index] = reinterpret_cast<const std::uint8_t*>(&original_value)[byte_index];
    }
    return new_value;
}

template <typename T>
T Selino::Utils::Endianness::FromNetworkOrder(const T& original_value) {
    return Selino::Utils::Endianness::ToNetworkOrder(original_value);
}

#define TEMPLATE_CONVERSION(prefix, type) template type Selino::Utils::Endianness::prefix##NetworkOrder(const type& original_value);
TEMPLATE_CONVERSION(From, std::uint16_t)
TEMPLATE_CONVERSION(From, std::uint32_t)
TEMPLATE_CONVERSION(To, std::uint16_t)
