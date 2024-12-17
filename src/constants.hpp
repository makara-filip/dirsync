#ifndef DIRSYNC_CONSTANTS_HPP
#define DIRSYNC_CONSTANTS_HPP

#include <iostream>

constexpr int EXIT_CODE_INCORRECT_USAGE = 1;
constexpr int EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY = 2;
constexpr int EXIT_CODE_FILESYSTEM_ERROR = 3;

struct Version {
	size_t major = 0;
	size_t minor = 0;
	size_t patch = 0;

	std::ostream &operator<<(std::ostream &stream) const {
		stream << major << '.' << minor << '.' << patch;
		return stream;
	}
};

constexpr Version PROGRAM_VERSION = {0, 0, 0};

#endif // DIRSYNC_CONSTANTS_HPP
