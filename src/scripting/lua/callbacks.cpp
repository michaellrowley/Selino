#include <span>
#include <vector>
#include <iostream>

#include <sol/sol.hpp>
#include <boost/asio.hpp>

#include "./lua.hpp"
#include "../../core/protocols/socks/socks.hpp"

bool Selino::Scripting::Lua::Callbacks::IsLoaded(const std::string& callback_name) {
    std::unique_lock tally_lock(Selino::Scripting::Lua::Callbacks::TallyAccessMutex);
    std::unordered_map<std::string, std::size_t>::const_iterator specific_count = Selino::Scripting::Lua::Callbacks::CallbacksTally.find(callback_name);
    const bool loaded = specific_count != Selino::Scripting::Lua::Callbacks::CallbacksTally.cend() && specific_count->second > 0;
    return loaded;
}

bool Selino::Scripting::Lua::Callbacks::IsSingleLoaded(const std::string& callback_name) {
    std::unique_lock tally_lock(Selino::Scripting::Lua::Callbacks::TallyAccessMutex);
    std::unordered_map<std::string, std::size_t>::const_iterator specific_count = Selino::Scripting::Lua::Callbacks::CallbacksTally.find(callback_name);
    const bool loaded = (specific_count != Selino::Scripting::Lua::Callbacks::CallbacksTally.cend()) && (specific_count->second == 1);
    return loaded;
}

template <typename Ret, typename ... Args>
std::vector<Ret> Selino::Scripting::Lua::Callbacks::Call(const std::string& callback_name, Args... callback_arguments) {
    std::unique_lock lua_state_mutex(Selino::Scripting::Lua::StateAccessMutex);

    std::vector<Ret> return_values;

    if (!Selino::Scripting::Lua::Callbacks::IsLoaded(callback_name)) {
        return return_values;
    }

    for (sol::state& lua_state : Selino::Scripting::Lua::States) {
        try {
            return_values.push_back(lua_state[CallbacksTableName][callback_name](std::forward<Args>(callback_arguments)...));
        } catch (std::exception& err) {
            if (callback_name != "callback_error") {
                Selino::Scripting::Lua::Callbacks::Call("callback_error", callback_name.c_str(), err.what());
            }
        }
    }

    return return_values;
}

template <typename ... Args>
void Selino::Scripting::Lua::Callbacks::Call(const std::string& callback_name, Args... callback_arguments) {
    std::unique_lock lua_state_mutex(Selino::Scripting::Lua::StateAccessMutex);

    if (!Selino::Scripting::Lua::Callbacks::IsLoaded(callback_name)) {
        return;
    }

    for (sol::state& lua_state : Selino::Scripting::Lua::States) {
        try {
            lua_state[CallbacksTableName][callback_name](std::forward<Args>(callback_arguments)...);
        } catch (std::exception& err) {
            if (callback_name != "callback_error") {
                Selino::Scripting::Lua::Callbacks::Call("callback_error", callback_name.c_str(), err.what());
            }
        }
    }
}

#define SINGLE_ARGUMENT(...) __VA_ARGS__
#define TEMPLATE_CALLBACK(args) template void Selino::Scripting::Lua::Callbacks::Call<args>(const std::string&, args);
#define TEMPLATE_CALLBACK_RETURN(ret, args) template std::vector<ret> Selino::Scripting::Lua::Callbacks::Call<args>(const std::string&, args);

TEMPLATE_CALLBACK(SINGLE_ARGUMENT(std::string, std::uint16_t, std::string, std::uint16_t, unsigned char (*) [2048], unsigned long))
TEMPLATE_CALLBACK(SINGLE_ARGUMENT(char const*, int, unsigned char const (*) [Selino::Protocols::SOCKS::InitialReceiveLength], unsigned long, std::string, std::uint16_t, std::string, std::uint16_t))
TEMPLATE_CALLBACK(SINGLE_ARGUMENT(char const*, char const*))
TEMPLATE_CALLBACK(SINGLE_ARGUMENT(std::uint16_t, std::string, std::uint16_t, std::string, char const*, std::string, std::uint16_t, std::string))
TEMPLATE_CALLBACK(SINGLE_ARGUMENT(std::uint16_t, std::string, std::uint16_t, std::string, std::string, std::uint16_t, std::string))
TEMPLATE_CALLBACK(SINGLE_ARGUMENT(std::string, std::uint16_t, std::string, std::uint16_t))
TEMPLATE_CALLBACK(SINGLE_ARGUMENT(std::uint16_t, std::string, std::uint16_t, std::string, char const*, std::uint16_t, std::string, std::string))
TEMPLATE_CALLBACK(SINGLE_ARGUMENT(unsigned char (*) [256], unsigned long))
TEMPLATE_CALLBACK(SINGLE_ARGUMENT(std::__1::reference_wrapper<std::unordered_map<std::string, std::string>>, std::uint8_t(*)[Selino::Protocols::SOCKS::V5::MaxAuthReadSize]))
TEMPLATE_CALLBACK(SINGLE_ARGUMENT(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>, unsigned short, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>, unsigned short, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>, unsigned short, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>))
