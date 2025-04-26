#ifndef DIRSYNC_SYNCHRONIZE_HPP
#define DIRSYNC_SYNCHRONIZE_HPP

#include <filesystem>

#include "arguments.hpp"

std::string get_formatted_time(const std::filesystem::file_time_type &time);

int synchronize_directories(const ProgramArguments &arguments);

#endif //DIRSYNC_SYNCHRONIZE_HPP
