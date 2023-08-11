#include <sol/sol.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <utility>

#include "./lua.hpp"

namespace Selino::Scripting::Lua {
    std::vector<sol::state> States = {}; // https://stackoverflow.com/a/20911242, declared in 'lua.hpp'
    namespace Callbacks {
        std::unordered_map<std::string, std::size_t> CallbacksTally = {}; // See above (^)
    };
};

void AddCallbackTally(const std::string& function_name) {
    std::unique_lock tally_lock(Selino::Scripting::Lua::Callbacks::TallyAccessMutex);
    std::unordered_map<std::string, std::size_t>::iterator specific_count = Selino::Scripting::Lua::Callbacks::CallbacksTally.find(function_name);
    if (specific_count != Selino::Scripting::Lua::Callbacks::CallbacksTally.end()) {
        ++specific_count->second;
    }
    else {
        Selino::Scripting::Lua::Callbacks::CallbacksTally[function_name] = 1;
    }
}

void Selino::Scripting::Lua::Load::Script(const std::string& script_string) {
    std::unique_lock lua_state_mutex(Selino::Scripting::Lua::StateAccessMutex);

    sol::state new_lua_state;
    new_lua_state.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string,
        sol::lib::os, sol::lib::math, sol::lib::table, sol::lib::io, sol::lib::debug, sol::lib::bit32,
        sol::lib::io, sol::lib::ffi, sol::lib::jit);

    bool failed = false;
    try {
        new_lua_state.script(script_string);
        if (!new_lua_state[CallbacksTableName]["init"].valid() || !new_lua_state[CallbacksTableName]["init"]()) {
            failed = true;
        }
    }
    catch (...) {
        failed = true;
    }
    if (failed) {
        throw std::runtime_error("Unable to initialize LUA script from string");
    }
    for (std::pair<sol::object, sol::object>& function_pair : new_lua_state[CallbacksTableName].get<sol::table>()) {
        if (function_pair.second.get_type() == sol::type::function) {
            const std::string function_name = function_pair.first.as<std::string>();
            AddCallbackTally(function_name);
        }
    }
    Selino::Scripting::Lua::States.push_back(std::move(new_lua_state));
}

void Selino::Scripting::Lua::Load::Script(const std::filesystem::path& script_path) {
    std::unique_lock lua_state_mutex(Selino::Scripting::Lua::StateAccessMutex);

    sol::state new_lua_state;
    new_lua_state.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string, sol::lib::os, sol::lib::math, sol::lib::table, sol::lib::io, sol::lib::debug);

    bool failed = false;
    try {
        new_lua_state.script_file(script_path.string());
        if (!new_lua_state[CallbacksTableName]["init"].valid() || !new_lua_state[CallbacksTableName]["init"]()) {
            failed = true;
        }
    }
    catch (...) {
        failed = true;
    }
    if (failed) {
        throw std::runtime_error("Unable to initialize LUA script from file");
    }
    for (std::pair<sol::object, sol::object>& function_pair : new_lua_state[CallbacksTableName].get<sol::table>()) {
        if (function_pair.second.get_type() == sol::type::function) {
            const std::string function_name = function_pair.first.as<std::string>();
            AddCallbackTally(function_name);
        }
    }
    Selino::Scripting::Lua::States.push_back(std::move(new_lua_state));
}
