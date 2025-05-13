#ifndef DIRSYNC_SYNCHRONIZE_ONE_WAY_HPP
#define DIRSYNC_SYNCHRONIZE_ONE_WAY_HPP

#include <vector>

#include "synchronize.hpp"
#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

/** The context object containing the local directory configurations in a stack.
 * Provides information for and stores state of `MonodirectionalSynchronizer`.
 * Contains both source and target configurations, see public getters. */
class MonodirectionalContext final : public BinaryContext {
	public:
	explicit MonodirectionalContext(const ProgramArguments &args) : BinaryContext(args) {}

	const fs::path &get_source_root() const { return root_paths.first; }
	const fs::path &get_target_root() const { return root_paths.second; }

	const OptionalConfiguration &get_target_leaf_configuration() const {
		return get_leaf_configuration_pair().second;
	}

	bool should_synchronize(const fs::directory_entry &entry) const {
		return source_allows_to_copy(entry) && target_accepts(entry);
	}

	private:
	bool source_allows_to_copy(const fs::directory_entry &entry) const;
	bool target_accepts(const fs::directory_entry &entry) const;
};

/** A final `Synchronizer` descendant. Provides implementation for one-way synchronization
 * and related helper functions. Uses `MonodirectionalContext` for specific behavior. */
class MonodirectionalSynchronizer final : public Synchronizer {
	MonodirectionalContext &context;

	public:
	explicit MonodirectionalSynchronizer(MonodirectionalContext &context)
		: context(context) {}

	int synchronize() override {
		return synchronize_directories_recursively(
			context.get_source_root(),
			context.get_target_root()
		);
	}

	private:
	int synchronize_directories_recursively(
		const fs::path &source_directory,
		const fs::path &target_directory
	);
	int synchronize_directory_entry(
		const fs::directory_entry &source_entry,
		const fs::path &target_directory
	);
	int synchronize_config_file(
		const fs::directory_entry &source_entry,
		const fs::path &target_path
	);
	int synchronize_regular_file(
		const fs::directory_entry &source_file,
		const fs::path &target_path
	);

	int delete_extra_target_entries(
		const fs::path &source_directory,
		const fs::path &target_directory
	) const;
};

#endif //DIRSYNC_SYNCHRONIZE_ONE_WAY_HPP
