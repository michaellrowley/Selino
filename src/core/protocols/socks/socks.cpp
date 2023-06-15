#include "./socks.hpp"
#include "../../config/config.hpp"
#include "../../../scripting/lua/lua.hpp"

#include <memory>
#include <span>
#include <mutex>

void Selino::Protocols::SOCKS::ConnectionHandler(boost::asio::ip::tcp::socket incoming_socket) {

    const boost::asio::ip::tcp::endpoint local_endpoint = incoming_socket.local_endpoint();
    const boost::asio::ip::tcp::endpoint remote_endpoint = incoming_socket.remote_endpoint();

    std::unique_ptr<std::uint8_t[]> receive_buffer = std::make_unique<std::uint8_t[]>(Selino::Protocols::SOCKS::InitialReceiveLength);
    const std::size_t amount_received = incoming_socket.receive(boost::asio::mutable_buffer(receive_buffer.get(), Selino::Protocols::SOCKS::InitialReceiveLength));

    if (amount_received == 0) {
        return;
    }
    // After the above check, we're good to check (*only*) the first byte

    const std::uint8_t* const receive_buffer_raw = receive_buffer.get();
    enum VersionOptions : std::uint8_t {
        SOCKS4 = 4,
        SOCKS5 = 5
        // , SOCKS6 = 6
    } version = static_cast<VersionOptions>(receive_buffer_raw[0]);
    if (!((version == VersionOptions::SOCKS4 && amount_received >= 9) || (version == VersionOptions::SOCKS5 && amount_received >= 3))) {
        return; // Invalid version/packet.
    }

    const char* const socks_verstr = version == VersionOptions::SOCKS4 ? "SOCKS4" : "SOCKS5";
    Selino::Scripting::Lua::Callbacks::Call("protocol_packet",
        socks_verstr, 0, reinterpret_cast<const std::uint8_t(*)[Selino::Protocols::SOCKS::InitialReceiveLength]>(receive_buffer_raw), amount_received, local_endpoint.address().to_string(), local_endpoint.port(), remote_endpoint.address().to_string(), remote_endpoint.port());

    // By catching inside 'ConnectionHandler' instead of allowing the caller to catch, we're able to call 'protocol_fail' in
    // more specific cases (the script will know that 'ConnectionHandler' reached this point, instead of being given bad data).
    #define CALL_SOCKSVER_CONN_PROCESSOR(vernum) if (version == VersionOptions::SOCKS##vernum) {                                 \
        try {                                                                                                                    \
            Selino::Protocols::SOCKS::V##vernum handler(incoming_socket);                                                        \
            handler.ProcessConnection(receive_buffer_raw, amount_received);                                                      \
        } catch (const std::runtime_error& err) {                                                                                \
            Selino::Scripting::Lua::Callbacks::Call("protocol_fail", socks_verstr, err.what());                                  \
        }                                                                                                                        \
    }
    // Selino::Protocols::SOCKS::V4 handler(incoming_socket, Selino::Config::ArgumentsProvided.find("enable-local-connections") != Selino::Config::ArgumentsProvided.cend());
    CALL_SOCKSVER_CONN_PROCESSOR(4)
    else CALL_SOCKSVER_CONN_PROCESSOR(5)
}

Selino::Protocols::SOCKS::Implementation::Implementation(
    boost::asio::ip::tcp::socket& connection
) : ConnectionSocket(connection), AllowLocalConnections(Selino::Config::ArgumentsProvided.find("enable-local-connections") != Selino::Config::ArgumentsProvided.cend()) { }
