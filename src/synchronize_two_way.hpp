#ifndef DIRSYNC_SYNCHRONIZE_TWO_WAY_HPP
#define DIRSYNC_SYNCHRONIZE_TWO_WAY_HPP

#include <filesystem>
#include <optional>
#include <utility>

#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

struct BidirectionalContext {
	using ConfigurationPair = std::pair<
		std::optional<DirectoryConfiguration>,
		std::optional<DirectoryConfiguration>
	>;

	std::vector<ConfigurationPair> configuration_stack;

	const fs::directory_entry *root_left, *root_right;

	BidirectionalContext(const fs::directory_entry *left, const fs::directory_entry *right)
		: root_left(left), root_right(right) {}
};

int synchronize_directories_bidirectionally(
	const ProgramArguments &arguments,
	BidirectionalContext &context,
	const fs::directory_entry &source_left,
	const fs::directory_entry &source_right
);

#endif //DIRSYNC_SYNCHRONIZE_TWO_WAY_HPP
