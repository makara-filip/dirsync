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
	bool dry_run = false;

	std::string source_directory;
	std::string target_directory;

	bool try_parse(const std::vector<std::string> &arguments);
	void print(std::ostream &stream) const;
};

#endif // DIRSYNC_ARGUMENTS_HPP
