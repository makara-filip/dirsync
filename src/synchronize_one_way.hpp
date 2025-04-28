#ifndef DIRSYNC_SYNCHRONIZE_ONE_WAY_HPP
#define DIRSYNC_SYNCHRONIZE_ONE_WAY_HPP

#include <optional>
#include <vector>

#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

struct MonodirectionalContext {
	std::vector<std::optional<DirectoryConfiguration>>
		source_configuration_stack, target_configuration_stack;
	const std::filesystem::directory_entry *root_source, *root_target;

	MonodirectionalContext(const fs::directory_entry *source, const fs::directory_entry *target) {
		this->root_source = source;
		this->root_target = target;
	}

	bool should_copy(const fs::directory_entry &entry, const fs::file_status &status) const {
		return source_allows_to_copy(entry, status) && target_accepts(entry, status);
	}

	private:
	bool source_allows_to_copy(const fs::directory_entry &entry, const fs::file_status &status) const;

	bool target_accepts(const fs::directory_entry &entry, const fs::file_status &status) const;
};

int synchronize_directories_recursively(
	const ProgramArguments &arguments,
	MonodirectionalContext &context,
	const fs::directory_entry &source_directory,
	const fs::directory_entry &target_directory
);

#endif //DIRSYNC_SYNCHRONIZE_ONE_WAY_HPP
