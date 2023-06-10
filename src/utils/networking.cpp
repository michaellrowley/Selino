#include "./utils.hpp"
#include "../scripting/lua/lua.hpp"

#include <span>
#include <string>
#include <chrono>
#include <memory>
#include <future>
#include <iostream>
#include <stdint.h>
#include <functional>

#include <boost/bind.hpp>
#include <boost/asio.hpp>

const std::string Selino::Utils::Networking::IPv4IntToString(const std::uint32_t address) {
    std::string address_string;
    for (std::size_t i = 0; i < sizeof(std::uint32_t); i++) {
        // Get the i-th byte in 'address'
        const std::uint8_t iterative_byte = (address >> (8 * ((sizeof(std::uint32_t) - 1) - i))) & 0xFF;
        // Converts the aforementioned byte to string-representation and appends it to the address string.
        address_string += std::to_string(iterative_byte) + (i == sizeof(std::uint32_t) - 1 ? "" : ".");
    }
    return address_string;
}

void Selino::Utils::Networking::TwoWayForward(boost::asio::ip::tcp::socket& client_one,
    boost::asio::ip::tcp::socket& client_two) {

    std::atomic<std::size_t> communication_count = 0;

    const boost::asio::ip::tcp::endpoint endpoint_one = client_one.remote_endpoint();
    const boost::asio::ip::tcp::endpoint endpoint_two = client_two.remote_endpoint();
    Selino::Scripting::Lua::Callbacks::Call("tunnel_opened",
                    endpoint_one.address().to_string(), endpoint_one.port(), endpoint_two.address().to_string(), endpoint_two.port());

    const std::function<void(boost::asio::ip::tcp::socket& srcsock, boost::asio::ip::tcp::socket& destsock)> oneway_forward = [&communication_count]
        (boost::asio::ip::tcp::socket& srcsock, boost::asio::ip::tcp::socket& destsock) {

        std::shared_ptr<std::uint8_t[]> recv_buffer = std::make_shared<std::uint8_t[]>(Selino::Utils::Networking::ForwardChunkSize);

        const boost::asio::ip::tcp::endpoint source_endpoint = srcsock.remote_endpoint();
        const boost::asio::ip::tcp::endpoint dest_endpoint = destsock.remote_endpoint();

        // Initially used recursive function calls for this but that would make stack overflows (via infinite recursion) possible.
        std::size_t receive_length = 0;
        boost::system::error_code receive_err;
        while (true) {
            receive_length = srcsock.receive(boost::asio::mutable_buffer(recv_buffer.get(), Selino::Utils::Networking::ForwardChunkSize),
                0x00000000 /* No message flags */, receive_err);
            if (receive_length == 0 || receive_err) {
                break;
            }

            // To avoid issues with scripts accessing old data:
            std::memset(reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uintptr_t>(recv_buffer.get()) + receive_length), 0x00, Selino::Utils::Networking::ForwardChunkSize - receive_length);
            Selino::Scripting::Lua::Callbacks::Call("tunneling_data",
                source_endpoint.address().to_string(), source_endpoint.port(), dest_endpoint.address().to_string(), dest_endpoint.port(),
                reinterpret_cast<std::uint8_t(*)[Selino::Utils::Networking::ForwardChunkSize]>(recv_buffer.get()), receive_length);

            destsock.send(boost::asio::const_buffer(recv_buffer.get(), receive_length));
            ++communication_count;
        }
        srcsock.cancel();
        destsock.cancel();

    };

    std::size_t last_communication_count = 0; // Bit-wrapping these variables (hitting SIZE_MAX and then zero) is allowed, hence no checks for it.

    std::future<void> first_relay = std::async(std::launch::async, oneway_forward, std::ref(client_one), std::ref(client_two));
    std::future<void> second_relay = std::async(std::launch::async, oneway_forward, std::ref(client_two), std::ref(client_one));

    // Implementing a timeout, after which the sockets are cancelled and both relay lambdas (should) return:
    do {
        last_communication_count = communication_count;
        std::this_thread::sleep_for(TwoWayTimeout);
    } while (last_communication_count != communication_count);

    client_one.cancel();
    client_two.cancel();

    second_relay.get();
    first_relay.get();

    // Alert the script(s) that the tunnel has closed.
    Selino::Scripting::Lua::Callbacks::Call("tunnel_closed",
                    endpoint_one.address().to_string(), endpoint_one.port(), endpoint_two.address().to_string(), endpoint_two.port());
}

boost::asio::ip::address Selino::Utils::Networking::ResolveDomain(const std::string& domain_name, boost::asio::io_context& io_context, const std::uint16_t port) {
    boost::asio::ip::tcp::resolver domain_resolution(io_context);
    boost::asio::ip::tcp::resolver::results_type resolve_results;
    try {
        resolve_results = domain_resolution.resolve(boost::asio::ip::tcp::resolver::query(domain_name, std::to_string(port),
            boost::asio::ip::tcp::resolver::query::numeric_service));
    } catch ( ... ) {
        throw std::runtime_error("Unable to query domain");
    }

    if (resolve_results.cbegin() == resolve_results.cend()) {
        throw std::runtime_error("Unable to resolve domain to address");
    }

    // Just using the first result - IPv4 or IPv6 is irrelevant as we can parse either.
    // TODO: Consider pinging each result to ensure the one that we return is active.
    return (*(resolve_results.cbegin())).endpoint().address();
}
