#include "./config.hpp"

#include <iostream>
#include <vector>
#include <map>

void Selino::Config::ParseArguments(const std::size_t argc, const char* const* const argv) {
    for (unsigned int i = 1; i < argc; i++) {
        const std::string& argument_key = argv[i];
        for (unsigned int j = 0; j < Selino::Config::ValidArgsCount; j++) {

            const std::tuple<ArgumentType, std::pair<std::string, std::string>, std::string, std::string>& iterative_arg = Selino::Config::ValidArgsArr[j];
            const std::pair<std::string, std::string>& argument_names = std::get<1>(iterative_arg);
            const std::string clean_key = argument_names.second;

            if (argument_key == "-" + argument_names.first || argument_key == "--" + clean_key) {

                const Selino::Config::ArgumentType arg_type = std::get<0>(iterative_arg);
                const bool is_toggle = arg_type & Selino::Config::ArgumentType::PRESENCE_TOGGLE;

                if (i == argc - 1 && !is_toggle) {
                    throw std::runtime_error("Invalid final argument; expected follow-up value for argument '" + clean_key + "'");
                }

                if (Selino::Config::ArgumentsProvided.find(clean_key) == Selino::Config::ArgumentsProvided.end()) {
                    Selino::Config::ArgumentsProvided.insert(std::pair<std::string, std::vector<std::string>>(clean_key, std::vector<std::string>(0)));
                }
                else if (arg_type & Selino::Config::ArgumentType::SINGLE_VALUE) {
                    throw std::runtime_error("Invalid argument; flag specified more than once ('" + clean_key + "')");
                }

                // This forces a second search but it doesn't really slow anything down.
                Selino::Config::ArgumentsProvided[clean_key].push_back(is_toggle ? "ON" : argv[++i]);

                break;
            }
            else if (j == Selino::Config::ValidArgsCount - 1) {
                throw std::runtime_error("Unsupported argument provided: '" + argument_key + "'");
            }
        }
    }
}

const std::string Selino::Config::GenerateHelp() {
    std::string help_str("Selino CLI (v" + std::string(Selino::Config::VersionStr) + ") Help:");
    for (std::size_t i = 0; i < Selino::Config::ValidArgsCount; i++) {
        const std::tuple<ArgumentType, std::pair<std::string, std::string>, std::string, std::string>& arg_ref = Selino::Config::ValidArgsArr[i];
        const std::pair<std::string, std::string>& arg_name = std::get<1>(arg_ref);
        help_str += "\n\t'--" + arg_name.second + "' ('-" + arg_name.first + "'):\n\t\tDescription:\n\t\t\t" + std::get<3>(arg_ref) + "\n\t\tExample:\n\t\t\t$ selino-cli --" + arg_name.second + " " + std::get<2>(arg_ref) + "\n";
    }
    return help_str;
}
