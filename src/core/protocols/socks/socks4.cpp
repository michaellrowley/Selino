#include "./socks.hpp"
#include "../../../scripting/lua/lua.hpp"
#include "../../../utils/utils.hpp"

#include <span>
#include <memory>
#include <chrono>
#include <stdint.h>
#include <string.h>
#include <iostream>

void Selino::Protocols::SOCKS::V4::ProcessConnection(const std::uint8_t* const first_packet_buffer, const std::size_t first_packet_size) {
    // Guaranteed that first_packet is a valid buffer of at least 9 bytes. Safe to throw from this function.

    std::uint8_t reply_buffer[8] = {0, 91 /* Failed/Rejected */, 0, 0, 0, 0, 0, 0};

    enum ConnIDs : std::uint8_t {
        CONNECT = 1,
        BIND = 2
    } command_id = static_cast<ConnIDs>(first_packet_buffer[1]);

    if (first_packet_buffer[first_packet_size - 1] != 0x00) {
        this->ConnectionSocket.get().send(boost::asio::const_buffer(reinterpret_cast<const std::uint8_t*>(reply_buffer), 8));
        throw std::runtime_error("Invalid packet: missing NULL at end of buffer");
    }

    // +----+----+----+----+----+----+----+----+----+----+....+----+
    // | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
    // +----+----+----+----+----+----+----+----+----+----+....+----+
    // [  1 ][ 1 ][    2    ][        4        ][   ???  ]    [ 01 ]

    const std::uint16_t destination_port = Selino::Utils::Endianness::FromNetworkOrder(reinterpret_cast<const std::uint16_t* const>(first_packet_buffer)[1]);
    const std::uint32_t destination_addr_num = Selino::Utils::Endianness::FromNetworkOrder(reinterpret_cast<const std::uint32_t* const>(first_packet_buffer)[1]);

    boost::asio::io_context& io_context_handler = static_cast<boost::asio::io_context&>(this->ConnectionSocket.get().get_executor().context());

    std::string destination_addr_str;
    boost::asio::ip::address destination_addr;
    if (destination_addr_num <= 0x000000FF && destination_addr_num != 0x00000000) {
        // This means the request is SOCKS4a.
        for (std::size_t char_index = first_packet_size - 2 /* one for array boundary, one to skip NULL terminator */; char_index > 0 && first_packet_buffer[char_index] != 0x00; char_index--) {
            destination_addr_str = static_cast<const char>(first_packet_buffer[char_index]) + destination_addr_str;

            static constexpr std::size_t max_domain_length = 256;
            if (first_packet_size - 2 > max_domain_length + 8 /* VN, CD, DSTPORT, and DSTIP sizes */ &&
                (first_packet_size - 1) - char_index > max_domain_length) {
                    this->ConnectionSocket.get().send(boost::asio::const_buffer(reinterpret_cast<const std::uint8_t*>(reply_buffer), 8));
                    throw std::runtime_error("Invalid domain name; length exceeds max_domain_length");
            }
        }

        // Need to lookup the IP address of the domain.
        try {
            destination_addr = Selino::Utils::Networking::ResolveDomain(destination_addr_str, io_context_handler, destination_port);
        } catch (const std::runtime_error& err) {
            this->ConnectionSocket.get().send(boost::asio::const_buffer(reinterpret_cast<const std::uint8_t*>(reply_buffer), 8));
            throw err;
        }
    }
    else {
        destination_addr_str = Selino::Utils::Networking::IPv4IntToString(destination_addr_num);
        destination_addr = boost::asio::ip::address::from_string(destination_addr_str);
    }

    if (!Selino::Protocols::SOCKS::V4::AllowLocalConnections && (destination_addr.is_loopback() || destination_addr.is_unspecified())) {
        this->ConnectionSocket.get().send(boost::asio::const_buffer(reinterpret_cast<const std::uint8_t*>(reply_buffer), 8));
        throw std::runtime_error("Invalid request: attempt to interact with local endpoint");
    }

    if (Selino::Scripting::Lua::Callbacks::IsLoaded("socks4_command")) {
        // Hand-checked this code for an off-by-one and it seems to be fine but to be super safe,
        // this section is only run if the callback is in-use. This makes mitigation easier in the
        // event of a vulnerability being discovered.
        const char* const user_id_start = reinterpret_cast<const char* const>(reinterpret_cast<const std::uintptr_t>(first_packet_buffer) + 8);
        const std::size_t user_id_size = strnlen(user_id_start, first_packet_size - 8);
        std::string user_id(user_id_start, user_id_size);

        const std::uint16_t local_port = this->ConnectionSocket.get().local_endpoint().port();
        const boost::asio::ip::tcp::endpoint remote_endpoint = this->ConnectionSocket.get().remote_endpoint();

        Selino::Scripting::Lua::Callbacks::Call("socks4_command", local_port, remote_endpoint.address().to_string(), remote_endpoint.port(),
            user_id, (command_id == ConnIDs::CONNECT ? "CONNECT" : "BIND"), destination_addr.to_string(), destination_port, destination_addr_str);
    }

    boost::asio::ip::tcp::socket destination_connection(io_context_handler);
    switch (command_id) {
        case ConnIDs::CONNECT:
            try {
                destination_connection.connect(boost::asio::ip::tcp::endpoint(destination_addr, destination_port));
            } catch (const boost::system::system_error&) {
                this->ConnectionSocket.get().send(boost::asio::const_buffer(reinterpret_cast<const std::uint8_t*>(reply_buffer), 8));
                throw std::runtime_error("Unable to connect to destination endpoint");
            }
            break;
        case ConnIDs::BIND:
            this->ConnectionSocket.get().send(boost::asio::const_buffer(reinterpret_cast<const std::uint8_t*>(reply_buffer), 8));
            throw std::runtime_error("unsupported operation requested (BIND)");
            break;
    }

    reply_buffer[1] = 90 /* Request granted */;
    this->ConnectionSocket.get().send(boost::asio::const_buffer(reinterpret_cast<const std::uint8_t*>(reply_buffer), 8));

    Selino::Utils::Networking::TwoWayForward(this->ConnectionSocket.get(), destination_connection);
}
