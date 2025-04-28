#ifndef DIRSYNC_SYNCHRONIZE_HPP
#define DIRSYNC_SYNCHRONIZE_HPP

#include <filesystem>

#include "arguments.hpp"

namespace fs = std::filesystem;

std::string get_formatted_time(const fs::file_time_type &time);
std::chrono::file_time<std::chrono::seconds> reduce_precision_to_seconds(const fs::file_time_type &file_time);
std::string insert_timestamp_to_filename(const fs::directory_entry &entry);

int synchronize_directories(const ProgramArguments &arguments);

#endif //DIRSYNC_SYNCHRONIZE_HPP
