#include "synchronize_one_way.hpp"

#include <filesystem>
#include <iostream>

#include "synchronize.hpp"
#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

bool MonodirectionalContext::source_allows_to_copy(
	const fs::directory_entry &entry,
	const fs::file_status &status
) const {
	for (auto it = source_configuration_stack.rbegin(); it != source_configuration_stack.rend(); ++it) {
		if (!it->has_value()) continue;
		const DirectoryConfiguration &configuration = it->value();
		if (!configuration.allows(entry, status)) return false;
	}
	return true;
}

bool MonodirectionalContext::target_accepts(
	const fs::directory_entry &entry,
	const fs::file_status &status
) const {
	for (auto it = target_configuration_stack.rbegin(); it != target_configuration_stack.rend(); ++it) {
		if (!it->has_value()) continue;
		const DirectoryConfiguration &configuration = it->value();
		if (!configuration.accepts(entry, status)) return false;
	}
	return true;
}

/** Delete files and directories in the target directory that do not exist
 * in the source directory. No recursion here. Directory tree traversal
 * is managed by higher-level functions. */
void delete_extra_target_entries(
	const ProgramArguments &arguments,
	const fs::directory_entry &source_directory,
	const fs::directory_entry &target_directory
) {
	for (const fs::directory_entry &target_entry : fs::directory_iterator(target_directory)) {
		std::error_code err;
		const fs::file_status status = fs::status(target_entry, err);

		const fs::directory_entry matching_source_entry(
			source_directory.path() / target_entry.path().filename(),
			err
		);
		const fs::file_status source_status = fs::status(matching_source_entry, err);

		if (fs::exists(source_status)) {
			if (arguments.verbose) std::cout << "Deleting extra " << target_entry << "\n";
			if (arguments.dry_run) continue;
			fs::remove(target_entry, err);
		}
	}
}

/** Copies the file if the program arguments and local directory configuration allow it.
 * Uses the specified filename conflict strategy. Determines which file content is newer
 * by file's last written time. \n\n
 * Example: source/example.txt gets copied to target/example.txt (did not exist). \n\n
 * Example 2: source/common.txt gets copied to target/common-2025-01-02-12-34-56.txt,
 * because we use the renaming conflict strategy and target/common.txt already exists. */
void synchronize_file(
	const ProgramArguments &arguments,
	const MonodirectionalContext &context,
	const fs::directory_entry &source_file,
	const fs::file_status &source_status,
	const fs::path &target_path,
	std::error_code &err
) {
	fs::path result_target_path = target_path;
	fs::copy_options options = fs::copy_options::skip_existing;

	// TODO: handle OS filesystem case (in)sensitivity

	if (is_config_file(source_file)) {
		const bool target_directory_has_config = context.target_configuration_stack.back().has_value();
		if (target_directory_has_config || !arguments.copy_configurations) return;
		goto final;
	}

	if (!context.should_copy(source_file, source_status)) return;

	if (fs::exists(target_path)) {
		const fs::directory_entry target_file(target_path);
		if (arguments.conflict_resolution == ConflictResolutionMode::skip) return;

		const fs::file_time_type source_written_at = source_file.last_write_time(err);
		const fs::file_time_type target_written_at = target_file.last_write_time(err);

		if (source_written_at == target_written_at) return;
		if (source_written_at < target_written_at) {
			// do not copy older versions, but inform the user
			if (arguments.verbose)
				std::cout << "Skipped copying older version of " << source_file << "\n";
			return;
		}

		options = fs::copy_options::overwrite_existing;

		if (arguments.conflict_resolution == ConflictResolutionMode::overwrite_with_newer) {
			// no special treatment
		} else if (arguments.conflict_resolution == ConflictResolutionMode::rename) {
			result_target_path = result_target_path.parent_path() / insert_timestamp_to_filename(target_file);
		}
	}

final:
	if (arguments.verbose) std::cout << "Copying " << source_file << "\n";
	if (arguments.dry_run) return;
	fs::create_directories(result_target_path.parent_path(), err);
	fs::copy_file(source_file, result_target_path, options, err);
}

/** A recursive function for one-way directory synchronization.
 * For every directory, tries to read and parse the local configuration,
 * uses filesystem's directory iterators to traverse the files.
 * Delegates more granular actions for other functions.
 * If a file is encountered, calls the `synchronize_file` function.
 * If a directory is encountered, calls itself recursively.
 * It removes excess files in the target directory tree if the program arguments dictate.
 * The local configurations are stored in the `MonodirectionalContext` stack. */
int synchronize_directories_recursively(
	const ProgramArguments &arguments,
	MonodirectionalContext &context,
	const fs::directory_entry &source_directory,
	const fs::directory_entry &target_directory
) {
	// read this directory configurations, both source and target
	// add them to the configuration stack
	// iterate over the source children
	// - if a file, copy if the config allows
	// - if a directory, call itself recursively with deeper configuration stack

	std::optional<DirectoryConfiguration> source_configuration, target_configuration;

	int error = get_directory_configuration(source_directory, arguments, source_configuration);
	error |= get_directory_configuration(target_directory, arguments, target_configuration);

	context.source_configuration_stack.push_back(source_configuration);
	context.target_configuration_stack.push_back(target_configuration);

	if (error) return error;

	for (const fs::directory_entry &source_entry : fs::directory_iterator(source_directory)) {
		std::error_code err;
		const fs::file_status status = fs::status(source_entry, err);
		if (err) {
			std::cerr << "Failed to check file status of " << source_entry << std::endl;
			continue;
		}

		const fs::path matching_target_path = target_directory.path() / source_entry.path().filename();

		if (fs::is_directory(status)) {
			if (!context.should_copy(source_entry, status)) continue;
			error = synchronize_directories_recursively(
				arguments,
				context,
				source_entry,
				fs::directory_entry(matching_target_path)
			);
		} else if (fs::is_regular_file(status)) {
			synchronize_file(arguments, context, source_entry, status, matching_target_path, err);
		}
	}

	if (arguments.delete_extra_target_files)
		delete_extra_target_entries(arguments, source_directory, target_directory);

	context.source_configuration_stack.pop_back();
	context.target_configuration_stack.pop_back();

	return error;
}
