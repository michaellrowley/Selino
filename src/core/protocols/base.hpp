#pragma once

#include <boost/asio.hpp>

#include <exception>
#include <stdint.h>
#include <thread>
#include <memory>
#include <string>

namespace Selino::Protocols {
    class Base {
        private:
            boost::asio::io_context IOContext;
            std::unique_ptr<boost::asio::ip::tcp::acceptor> SrvAcceptor;
            std::atomic<bool> IOTerminateFlag, IOTerminateCompleted;

            void AsyncAcceptLauncher();
            void ConnectionLoop(const boost::system::error_code& error, boost::asio::ip::tcp::socket incoming_socket);

        protected:
            virtual void ConnectionHandler(boost::asio::ip::tcp::socket incoming_socket) = 0;

        public:
            Base();
            ~Base();

            const std::string Protocol;

            void Listen(const std::uint16_t port, const bool ipv4_only = false);
    };
};
