#ifndef DIRSYNC_ARGUMENTS_HPP
#define DIRSYNC_ARGUMENTS_HPP

#include <string>
#include <vector>

enum class ProgramMode {
	help,
	synchronize
};

struct ProgramArguments {
	std::string executable;
	ProgramMode mode = ProgramMode::synchronize;
	bool verbose = true;

	std::string source_directory;
	std::string target_directory;

	bool try_parse(const std::vector<std::string> &arguments);
	void print(std::ostream &stream) const;
};

constexpr int EXIT_CODE_INCORRECT_USAGE = 1;
constexpr int EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY = 2;
constexpr int EXIT_CODE_FILESYSTEM_ERROR = 3;

#endif //DIRSYNC_ARGUMENTS_HPP
