#ifndef DIRSYNC_ARGUMENTS_HPP
#define DIRSYNC_ARGUMENTS_HPP

#include <string>
#include <vector>

enum class ProgramMode {
	help,
	synchronize,
	test
	// TODO: other options such as:
	// - ignore <file-or-directory>: adds the file/directrory to the .dirsync config file
};

enum class ConflictResolutionMode {
	overwrite_with_newer,
	skip,
	rename,
};

struct ProgramArguments {
	std::string executable;
	ProgramMode mode = ProgramMode::synchronize;
	bool verbose = false;
	bool dry_run = false;

	bool copy_configurations = false;
	bool delete_extra_target_files = false;

	bool is_one_way_synchronization = true;

	ConflictResolutionMode conflict_resolution = ConflictResolutionMode::overwrite_with_newer;

	std::string source_directory;
	std::string target_directory;

	bool try_parse(const std::vector<std::string> &arguments);
	void print(std::ostream &stream) const;
};

#endif // DIRSYNC_ARGUMENTS_HPP
