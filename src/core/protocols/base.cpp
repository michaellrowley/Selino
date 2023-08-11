#include "./base.hpp"
#include "../../utils/utils.hpp"
#include "../../scripting/lua/lua.hpp"

#include <boost/asio.hpp>

#include <memory>
#include <mutex>
#include <iostream>

Selino::Protocols::Base::Base() : IOContext() { }

Selino::Protocols::Base::~Base() {
    this->IOTerminateFlag = true;
}

void Selino::Protocols::Base::Listen(const std::uint16_t port, const bool ipv4_only) {

    // Create an acceptor for the server, this is dynamically allocated and is thus freed later.
    this->SrvAcceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(
        this->IOContext,
        boost::asio::ip::tcp::endpoint(ipv4_only ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6(), port)
    );

    boost::system::error_code error_code;
    this->SrvAcceptor->set_option(boost::asio::ip::v6_only(false), error_code);
    // "V6_ONLY->FALSE" Doesn't work on MacOS and some Linux distributions.

    this->AsyncAcceptLauncher();
    this->SrvAcceptor->listen();

    this->IOContext.run();
}

void Selino::Protocols::Base::AsyncAcceptLauncher() {
    if (!this->IOTerminateFlag) {
        this->SrvAcceptor->async_accept(std::bind(&Selino::Protocols::Base::ConnectionLoop, this,
            std::placeholders::_1, std::placeholders::_2));
    }
    else {
        this->IOTerminateCompleted = true;
    }
}

void Selino::Protocols::Base::ConnectionLoop(const boost::system::error_code& error,
    boost::asio::ip::tcp::socket incoming_socket) {

    this->AsyncAcceptLauncher();

    if (!error) {
        const boost::asio::ip::tcp::endpoint remote_endpoint = incoming_socket.remote_endpoint();
        const boost::asio::ip::tcp::endpoint local_endpoint = incoming_socket.local_endpoint();

        Selino::Scripting::Lua::Callbacks::Call("connection_received",
            local_endpoint.address().to_string(), local_endpoint.port(), remote_endpoint.address().to_string(), remote_endpoint.port());

        std::thread([this, remote_endpoint, local_endpoint](boost::asio::ip::tcp::socket incoming_socket){
            try {
                this->ConnectionHandler(std::move(incoming_socket));
            } catch (...) {
                // More often than not, exceptions are called from protocol handlers when they're attempting to read
                // from a socket that has been closed, hence boot.asio throws an EOF or 'bad file/pipe/stream' error.
                // Catching and continuing from here is good because it avoids having to wrap all asio code in error checking
                // and since most buffers are ownership-tracked, there's no risk of a memory leak.
            }
            Selino::Scripting::Lua::Callbacks::Call("connection_closed",
                local_endpoint.address().to_string(), local_endpoint.port(), remote_endpoint.address().to_string(), remote_endpoint.port());
        }, std::move(incoming_socket)).detach();
    }
}
