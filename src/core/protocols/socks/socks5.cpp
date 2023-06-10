#include "./socks.hpp"
#include "../../../utils/utils.hpp"
#include "../../../scripting/lua/lua.hpp"

#include <string>
#include <memory>

void Selino::Protocols::SOCKS::V5::ProcessConnection(const std::uint8_t* const first_packet_buffer, const std::size_t first_packet_size) {

    this->RunAuthentication(first_packet_buffer, first_packet_size);
    std::uint8_t intro_fail_cause = 0x01 /* General SOCKS failure */;
    try {
        this->RunIntroduction(intro_fail_cause);
    } catch (std::runtime_error& err) {
        std::uint8_t reply_buffer[1 /* VER */ + 1 /* REP */ + 1 /* RSV (0) */ + 1 /* ATYP */ + 16 /* BND.ADDR */ + 2 /* BND.PORT */] =
            { 0x05, intro_fail_cause, 0x00, 0x04 /* IPv6 */,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* Address */
                0 /* Port */
            };
        this->ConnectionSocket.get().send(boost::asio::const_buffer(reply_buffer, sizeof(reply_buffer)));
        throw err;
    }
}

void Selino::Protocols::SOCKS::V5::RunIntroduction(std::uint8_t& fail_reason) {

    boost::asio::ip::tcp::socket& intro_socket = this->ConnectionSocket.get();

    boost::asio::io_context& socket_io_context = static_cast<boost::asio::io_context&>(
        intro_socket.get_executor().context()
    );

    // Request:
    // +----+-----+-------+------+----------+----------+
    // |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
    // +----+-----+-------+------+----------+----------+
    // | 1  |  1  | X'00' |  1   | Variable |    2     |
    // +----+-----+-------+------+----------+----------+

    static constexpr std::size_t max_request_size = 1 /* VER */ + 1 /* CMD */ + 1 /* RSV */ + 1 /* ATYP */ + 256 /* DST.ADDR */ + 2 /* DST.PORT */;
    static constexpr std::size_t min_request_size = 1 + 1 + 1 + 1 + 1 /* 1-byte domain name */ + 2;

    // Only use this structure when needed (uses a pretty chunky allocation of 256-bytes in *all* cases),
    // otherwise revert to an array or other primitive means.
    struct IntroPacket {
        std::uint8_t Version = 5;
        std::uint8_t Type;
        std::uint8_t Reserved = 0x00;
        std::uint8_t AddressType;
        union {
            struct {
                boost::asio::ip::address_v4::bytes_type AddressBytes;
                std::uint16_t Port;
            } IPv4;
            struct {
                boost::asio::ip::address_v6::bytes_type AddressBytes;
                std::uint16_t Port;
            } IPv6;
            struct {
                std::uint8_t DomainLength;
                char DomainName[256] = {0};
            } FQDN;
        } RemoteEndpoint;
    };

    IntroPacket request = {0};
    const std::size_t request_size = intro_socket.receive(boost::asio::mutable_buffer(&request, sizeof(request)));

    if (request_size < min_request_size || request.Version != 5 || request.Reserved != 0x00) {
        throw std::runtime_error("Invalid request packet");
    }

    const boost::asio::ip::tcp::endpoint local_endpoint = intro_socket.local_endpoint();
        const std::uint16_t local_port = local_endpoint.port();
    const boost::asio::ip::tcp::endpoint remote_endpoint = intro_socket.remote_endpoint();

    Selino::Scripting::Lua::Callbacks::Call("protocol_packet",
        "SOCKS5", 1, reinterpret_cast<const std::uint8_t(*)[Selino::Protocols::SOCKS::InitialReceiveLength]>(&request), sizeof(request), local_endpoint.address().to_string(), local_port,
        remote_endpoint.address().to_string(), remote_endpoint.port());


    boost::asio::ip::tcp::endpoint request_endpoint;
    std::string remote_endpoint_string;

    switch (request.AddressType) {
        case 0x01: // IPv4
            request_endpoint = boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address_v4(request.RemoteEndpoint.IPv4.AddressBytes),
                Selino::Utils::Endianness::FromNetworkOrder(request.RemoteEndpoint.IPv4.Port)
            );
            remote_endpoint_string = request_endpoint.address().to_string();
            break;
        case 0x03: { // Domain Name
                if (request.RemoteEndpoint.FQDN.DomainLength > 255) {
                    throw std::runtime_error("Domain name too long");
                }

                // Casting narrow to wide size here so it's fine.
                const std::size_t domain_length = request.RemoteEndpoint.FQDN.DomainLength;
                const std::string domain_name(request.RemoteEndpoint.FQDN.DomainName, domain_length);
                const std::uint16_t port = Selino::Utils::Endianness::FromNetworkOrder(
                    *reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::uintptr_t>(request.RemoteEndpoint.FQDN.DomainName) + domain_length)
                );

                // Resolve the domain name to a viable address
                request_endpoint = boost::asio::ip::tcp::endpoint(
                    Selino::Utils::Networking::ResolveDomain(
                        domain_name,
                        socket_io_context,
                        port
                    ),
                    port
                );
                remote_endpoint_string = domain_name;
            }
            break;
        case 0x04: // IPv6
            request_endpoint = boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address_v6(request.RemoteEndpoint.IPv6.AddressBytes),
                Selino::Utils::Endianness::FromNetworkOrder(request.RemoteEndpoint.IPv6.Port)
            );
            remote_endpoint_string = request_endpoint.address().to_string();
            break;
        default:
            fail_reason = 0x08; // "Address type not supported"
            throw std::runtime_error("Invalid addressing format");
            break;
    }

    const boost::asio::ip::address request_address = request_endpoint.address();

    if (!Selino::Protocols::SOCKS::V5::AllowLocalConnections && (request_address.is_loopback() || request_address.is_unspecified())) {
        throw std::runtime_error("Attempt to interact with local endpoint");
    }

    if (Selino::Scripting::Lua::Callbacks::IsLoaded("socks5_command")) {
        Selino::Scripting::Lua::Callbacks::Call("socks5_command", local_endpoint.address().to_string(), local_port,
            remote_endpoint.address().to_string(), remote_endpoint.port(),
            std::map<std::uint8_t, std::string>({ {0x01, "CONNECT"}, {0x02, "BIND"}, {0x03, "UDP ASSOCIATE"} })[request.Type], request_address.to_string(), request_endpoint.port(), remote_endpoint_string);
    }

    #define SEND_REPLY(address_size)                                                                   \
    {                                                                                                  \
        constexpr std::size_t reply_buffer_size = 1 + 1 + 1 + 1 + address_size + 2;                    \
        std::uint8_t reply_buffer[reply_buffer_size] = {                                               \
            0x05 /* Version = 5 */, 0x00 /* Reply = Success */, 0x00,                                  \
        };                                                                                             \
        apply_endpoint(reply_buffer, reply_buffer_size);                                               \
        intro_socket.send(boost::asio::const_buffer(reply_buffer, reply_buffer_size));                 \
    }

    // Reply:
    // +----+-----+-------+------+----------+----------+
    // |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
    // +----+-----+-------+------+----------+----------+
    // | 1  |  1  | X'00' |  1   | Variable |    2     |
    // +----+-----+-------+------+----------+----------+

    const boost::asio::ip::address local_addr = local_endpoint.address();
    const bool is_local_v6 = local_addr.is_v6();
    const std::function<void(std::uint8_t* const, const std::size_t)> apply_endpoint = [&local_addr, &local_port, &is_local_v6]
            (std::uint8_t* const buffer, const std::size_t buffer_size) {
        // Checks for buffer are done before calling.
        *reinterpret_cast<std::uint16_t*>(&(buffer[buffer_size - 2])) = Selino::Utils::Endianness::ToNetworkOrder(local_port);
        buffer[3] = is_local_v6 ? 0x04 : 0x01;
        if (is_local_v6) {
            *reinterpret_cast<boost::asio::ip::address_v6::bytes_type*>(&(buffer[1 + 1 + 1 + 1])) = local_addr.to_v6().to_bytes();
        }
        else {
            *reinterpret_cast<boost::asio::ip::address_v4::bytes_type*>(&(buffer[1 + 1 + 1 + 1])) = local_addr.to_v4().to_bytes();
        }
    };

    fail_reason = 0x04; // "Host unreachable"
    boost::asio::ip::tcp::socket destination_socket(socket_io_context);
    switch (request.Type) {
        case 0x01: { // 'CONNECT'
            boost::system::error_code err;
            destination_socket.connect(request_endpoint, err);
            if (err) {
                throw std::runtime_error("Unable to connect to destination");
            }
            if (is_local_v6) SEND_REPLY(16)
            else SEND_REPLY(4)

            Selino::Utils::Networking::TwoWayForward(std::ref(intro_socket), std::ref(destination_socket));

            break;
        }

        case 0x02: // No 'BIND' support:
        case 0x03: // No 'UDP-ASSOCIATE' support:
        default:
            fail_reason = 0x07; // "Command not supported"
            throw std::runtime_error("Unsupported command");
            break;
    }


    return; // To be clear, throwing will trigger an error-reply.
}

void Selino::Protocols::SOCKS::V5::HandleUsernamePasswordSubAuthentication() {
    // HOTSPOT: Despite best efforts, this function runs the extreme risk of an out-of-bound read via auth_buffer.

    std::uint8_t reply_buffer[2] = { 0x01 /* VER */, 0x01 /* STATUS (Nonzero --> Failure) */ };

    // REQUEST:
    // +----+------+----------+------+----------+
    // |VER | ULEN |  UNAME   | PLEN |  PASSWD  |
    // +----+------+----------+------+----------+
    // | 1  |  1   | 1 to 255 |  1   | 1 to 255 |
    // +----+------+----------+------+----------+

    // Having this as a fixed-size, static array is great as we won't run over the allocation boundary even if ULEN or
    // PLEN are 0xFF and the allocation is way less and the sanity checks didn't check for that (they do).
    std::uint8_t auth_buffer[1 + 1 + 255 + 1 + 255] = {0};

    std::size_t received_size = 0;
    try {
        received_size = this->ConnectionSocket.get().receive(boost::asio::mutable_buffer(auth_buffer, sizeof(auth_buffer)));
    } catch (...) {
        this->ConnectionSocket.get().send(boost::asio::const_buffer(reply_buffer, 2));
        throw std::runtime_error("Unable to receive username/password packet");
    }

    std::size_t username_length = 0, password_length = 0;
    if (received_size < 5 /* Minimum possible */ || auth_buffer[0] != 0x01 || auth_buffer[1] == 0 ||
        received_size < 2 /* VER, ULEN */ + (username_length = auth_buffer[1]) + 1 /* PLEN */ + 1 /* 1-byte PASSWD */ ||
        received_size < 2 /* VER, ULEN */ + username_length + 1 /* PLEN */ + (password_length = auth_buffer[2 + username_length]) /* Full PASSWD */ ||
        password_length == 0 || username_length == 0) {

        this->ConnectionSocket.get().send(boost::asio::const_buffer(reply_buffer, 2));
        throw std::runtime_error("Invalid username/password packet");
    }

    const std::string username(reinterpret_cast<char*>(reinterpret_cast<std::uintptr_t>(auth_buffer) + 2 /* VER, ULEN */ ), username_length);
    const std::string password(
        reinterpret_cast<char*>(
            reinterpret_cast<std::uintptr_t>(auth_buffer) +
            2 /* VER, ULEN */ + username_length + 1 /* PLEN */
        ),
        password_length
    );
    for (const char* const* iterative_combination : Selino::Protocols::SOCKS::V5::UsernamePasswordCombos) {
        if (iterative_combination[0] == username && iterative_combination[1] == password) {
            reply_buffer[1] = 0x00; // 0 -> Success/Valid
            this->ConnectionSocket.get().send(boost::asio::const_buffer(reply_buffer, 2));
            return;
        }
    }
    throw std::runtime_error("Invalid username/password combination");
    return;
}


void Selino::Protocols::SOCKS::V5::HandleScriptedSubAuthentication(const std::uint8_t chosen_auth_mode,
    const std::uint8_t* const first_packet_buffer, const std::size_t first_packet_size) {

    const std::string callback_name = "socks5_auth_loop_" + std::to_string(chosen_auth_mode);
    if (!Selino::Scripting::Lua::Callbacks::IsSingleLoaded(callback_name)) {
        throw std::runtime_error("Invalid amount of handlers (!= 1) for scripted authentication ('" + callback_name + "')");
    }

    assert(Selino::Protocols::SOCKS::V5::MaxAuthReadSize >= Selino::Protocols::SOCKS::InitialReceiveLength);

    std::uint8_t passthrough_buffer[Selino::Protocols::SOCKS::V5::MaxAuthReadSize] = {0};
    std::memcpy(passthrough_buffer, first_packet_buffer,
        std::min(
            first_packet_size,
            Selino::Protocols::SOCKS::V5::MaxAuthReadSize
        ) /* ::min()'s presence is just in case other checks are removed */
    );

    std::unordered_map<std::string, std::string> callback_data = {{"received_amount", std::to_string(first_packet_size)}};
    std::unordered_map<std::string, std::string>::iterator map_iterator_tmp;
    bool status_missing = false;
    while ((status_missing = ((map_iterator_tmp = callback_data.find("status")) == callback_data.end())) ||
        map_iterator_tmp->second != "end") {

        if (!status_missing) {
            if (map_iterator_tmp->second == "receive") {

                if ((map_iterator_tmp = callback_data.find("receive_amount")) == callback_data.end()) {
                    throw std::runtime_error("Invalid 'receive' request on behalf of scripted authentication");
                }

                const std::size_t receive_amount = std::stoi(map_iterator_tmp->second);
                // Could be interesting to dynamically allocate the receive buffer but it's not too important right now.
                callback_data["received_amount"] = std::to_string(
                    this->ConnectionSocket.get().receive(boost::asio::mutable_buffer(passthrough_buffer, receive_amount))
                );
            }
            else if (map_iterator_tmp->second == "send") {

                if ((map_iterator_tmp = callback_data.find("send_amount")) == callback_data.end()) {
                    throw std::runtime_error("Invalid 'send' request on behalf of scripted authentication");
                }

                const std::size_t send_amount = std::stoi(map_iterator_tmp->second);

                if (send_amount > Selino::Protocols::SOCKS::V5::MaxAuthReadSize) {
                    throw std::runtime_error("Send amount in scripted authentication exceeds passthrough buffer size");
                }

                this->ConnectionSocket.get().send(boost::asio::const_buffer(passthrough_buffer, send_amount));
            }
        }

        Selino::Scripting::Lua::Callbacks::Call(
            callback_name,
            std::ref(callback_data),
            reinterpret_cast<std::uint8_t(*)[Selino::Protocols::SOCKS::V5::MaxAuthReadSize]>(passthrough_buffer)
        );
    }
    if ((map_iterator_tmp = callback_data.find("end_reason")) == callback_data.end() || map_iterator_tmp->second != "auth_success") {
        throw std::runtime_error("Scripted SOCKS5 authentication failed.");
    }
}

void Selino::Protocols::SOCKS::V5::RunAuthentication(const std::uint8_t* const first_packet_buffer, const std::size_t first_packet_size) {
    // Guaranteed that first_packet is a valid buffer of at least 3 bytes. Safe to throw from this function.
    std::uint8_t auth_reply[2] = { 0x05, 0xFF /* No acceptable methods */ };

    const std::uint8_t methods_count = first_packet_buffer[1];
    if (methods_count == 0 || first_packet_size <= 2 /* SIZEOF(VER) + SIZEOF(NMETHODS) = 2 */) {
        this->ConnectionSocket.get().send(boost::asio::const_buffer(auth_reply, 2));
        throw std::runtime_error("Invalid NMETHODS segment");
    }

    enum AuthMethods {
        NO_AUTH = 0x00,
        GSSAPI = 0x01,
        USERNAME_PASSWORD = 0x02,
        IANA_METHODS_START = 0x03,
        // IANA-reserved authentication methods
        PRIVATE_METHODS_START = 0x80,
        // private authentication methods
        NO_ACCEPTABLE = 0xFF
    };

    const bool script_chooses_auth = Selino::Scripting::Lua::Callbacks::IsLoaded("socks5_choose_auth");
    if (script_chooses_auth) {

        std::uint8_t methods_copy[256] = {0};
        const std::size_t methods_count = first_packet_size - 2;
        std::memcpy(methods_copy, reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(first_packet_buffer) + 0x02), methods_count);

        Selino::Scripting::Lua::Callbacks::Call("socks5_choose_auth", reinterpret_cast<std::uint8_t(*)[256]>(methods_copy), methods_count);

        // 'socks5_choose_auth' will have wiped (by setting to 0xFF) all of the unacceptable methods.
        for (std::size_t i = 0; i < methods_count; i++) {
            const std::uint8_t iterative_method = methods_copy[i];
            if (iterative_method != AuthMethods::NO_ACCEPTABLE) {
                // Just choose the first non-blacklisted value (also allows scripts
                // to shift indexes based on priorities).
                auth_reply[1] = iterative_method;
                break;
            }
        }
    }
    else {
        for (std::size_t byte_index = 0x02 /* Start of 'METHODS' */; byte_index < first_packet_size && auth_reply[1] == 0xFF /* Loop until we send a good reply */; byte_index++) {
            const std::uint8_t iterative_auth_mode = first_packet_buffer[byte_index];
            if (iterative_auth_mode == AuthMethods::NO_AUTH && !Selino::Protocols::SOCKS::V5::RequireUsernamePassword) {
                auth_reply[1] = iterative_auth_mode;
            }
            else if (iterative_auth_mode == AuthMethods::USERNAME_PASSWORD && Selino::Protocols::SOCKS::V5::RequireUsernamePassword) {
                auth_reply[1] = iterative_auth_mode;
            }
        }
    }

    const std::uint8_t chosen_auth_mode = auth_reply[1];
    this->ConnectionSocket.get().send(boost::asio::const_buffer(auth_reply, 2)); // Could be good or bad.
    if (chosen_auth_mode == 0xFF) {
        throw std::runtime_error("No suitable authentication method identified");
    }

    if (chosen_auth_mode == AuthMethods::USERNAME_PASSWORD && !script_chooses_auth) {
        this->HandleUsernamePasswordSubAuthentication();
    }
    else if (chosen_auth_mode != AuthMethods::NO_AUTH && script_chooses_auth) {
        this->HandleScriptedSubAuthentication(chosen_auth_mode, first_packet_buffer, first_packet_size);
    }
}
