#pragma once

#include <sol/sol.hpp>

#include <map>
#include <mutex>
#include <string>
#include <filesystem>

// Some notes about the LUA scripting section of this project:

// Q: Why not make LUA calls asynchronous or queue them onto another thread for improved performance?
// A: That would mean that any buffers being passed in would need to be deep-copied *and* maintained until
//    the call was completed. Also, this would mean that if the script needed to interact with any data
//    or act *before* a sequential action took place, it would be down to chance if the callback was run
//    from another thread.

// Q: Why not extract the (sol::function)s and store them in separate vectors to avoid having to traverse
//    a full sol::state every time a callback is run?
// A: That could(?) mean that scripts are unable to share data between callback calls or use global variables
//    to track application-wide metrics.

// The LUA side of this application takes a negligible amount of time to process
// (according to benchmarks) but optimizations are welcome.

namespace Selino::Scripting::Lua {
    const std::string CallbacksTableName = "callbacks";

    static std::recursive_mutex StateAccessMutex;
    extern std::vector<sol::state> States; // https://stackoverflow.com/a/20911242, assigned in 'load.cpp'

    namespace Load {
        void Script(const std::string& script_string);
        void Script(const std::filesystem::path& script_path);
    };

    namespace Callbacks {
        template <typename Ret, typename ... Args>
        std::vector<Ret> Call(const std::string& callback_name, Args... callback_arguments);
        template <typename ... Args>
        void Call(const std::string& callback_name, Args... callback_arguments);

        static std::recursive_mutex TallyAccessMutex;
        extern std::unordered_map<std::string, std::size_t> CallbacksTally;
        bool IsLoaded(const std::string& callback_name);
        bool IsSingleLoaded(const std::string& callback_name);
    };

    namespace General { };
};
