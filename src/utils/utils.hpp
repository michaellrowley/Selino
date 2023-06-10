#include <string>
#include <chrono>

#include <boost/asio.hpp>

namespace Selino::Utils {

    #define SINGLE_ARGUMENT(...) __VA_ARGS__

    namespace Endianness {
        template <typename T>
        T ToNetworkOrder(const T& original_value);

        template <typename T>
        T FromNetworkOrder(const T& original_value);
    };
    namespace Networking {
        const std::string IPv4IntToString(const std::uint32_t address);

        extern std::chrono::milliseconds TwoWayTimeout;

        void TwoWayForward(boost::asio::ip::tcp::socket& client_one, boost::asio::ip::tcp::socket& client_two);
        static constexpr std::size_t ForwardChunkSize = 2048;

        boost::asio::ip::address ResolveDomain(const std::string& domain_name, boost::asio::io_context& io_context, const std::uint16_t port = 443);
    }
    namespace Data {
        const std::chrono::milliseconds StringToDuration(const std::string& duration_string);
        const std::string Dump(const std::uint8_t* const buffer, const std::size_t length);
        inline const std::string ByteToHex(const std::uint8_t byte);
    }
};
