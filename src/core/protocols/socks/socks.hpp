#pragma once

#include <mutex>
#include <functional>

#include <boost/asio.hpp>

#include "../base.hpp"

namespace Selino::Protocols {
    class SOCKS : public Selino::Protocols::Base {
        private:
            void ConnectionHandler(boost::asio::ip::tcp::socket incoming_socket);
            constexpr static std::size_t InitialReceiveLength = 1024;

            class Implementation {
            protected:
                std::reference_wrapper<boost::asio::ip::tcp::socket> ConnectionSocket;
            public:
                // virtual void ProcessConnection(const std::uint8_t* const first_packet, const std::size_t first_packet_size);
                Implementation(boost::asio::ip::tcp::socket& connection);
            };

            class V4 : public Implementation {
            private:
                static constexpr bool AllowLocalConnections = false;
            public:
                void ProcessConnection(const std::uint8_t* const first_packet_buffer, const std::size_t first_packet_buffer_size);
            };

            class V5 : public Implementation {
            private:
                static constexpr std::size_t MaxAuthReadSize = 1024; // ENSURE THAT THIS IS ALWAYS GREATER THAN InitialReceiveLength.
                static constexpr const char* UsernamePasswordCombos[1][2 /* FIXED */] = {
                    {"username", "password"}
                };
                static constexpr bool RequireUsernamePassword = false;
                static constexpr bool AllowLocalConnections = false;

                void RunAuthentication(const std::uint8_t* const first_packet_buffer, const std::size_t first_packet_size);
                void RunIntroduction(std::uint8_t& fail_reason);

                // Throws if authentication fails.
                void HandleUsernamePasswordSubAuthentication();
                // Also throws in the event of failed auth.
                void HandleScriptedSubAuthentication(const std::uint8_t chosen_auth_mode,
                    const std::uint8_t* const first_packet_buffer, const std::size_t first_packet_size);

            public:
                void ProcessConnection(const std::uint8_t* const first_packet_buffer, const std::size_t first_packet_buffer_size);
            };

        public:
            using Selino::Protocols::Base::Base;
            const std::string Name = "SOCKS";
    };
};
