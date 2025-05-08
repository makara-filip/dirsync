#include "synchronize_one_way.hpp"

#include <filesystem>
#include <iostream>
#include <ranges>

#include "synchronize.hpp"
#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

bool MonodirectionalContext::source_allows_to_copy(const fs::directory_entry &entry) const {
	for (const auto &[source, _] : std::ranges::reverse_view(configuration_stack)) {
		if (!source.has_value()) continue;
		const DirectoryConfiguration &configuration = source.value();

		if (!configuration.allows(entry)) return false;
	}
	return true;
}

bool MonodirectionalContext::target_accepts(const fs::directory_entry &entry) const {
	for (const auto &[_, target] : std::ranges::reverse_view(configuration_stack)) {
		if (!target.has_value()) continue;
		const DirectoryConfiguration &configuration = target.value();

		if (!configuration.accepts(entry)) return false;
	}
	return true;
}

int MonodirectionalSynchronizer::delete_extra_target_entries(
	const fs::path &source_directory,
	const fs::path &target_directory
) const {
	for (const fs::directory_entry &target_entry : fs::directory_iterator(target_directory)) {
		std::error_code err;
		const fs::file_status status = target_entry.status(err);

		const fs::file_status source_status = fs::status(
			source_directory / target_entry.path().filename(),
			err
		);

		if (err) return EXIT_CODE_FILESYSTEM_ERROR;

		// continue deleting only when the file does not exist in the source directory
		if (fs::exists(source_status)) continue;

		if (context.arguments.is_verbose())
			std::cout << "Deleting extra " << target_entry << "\n";
		if (context.arguments.is_dry_run()) continue;
		fs::remove(target_entry, err);
		if (err) return EXIT_CODE_FILESYSTEM_ERROR;
	}

	return 0;
}

int MonodirectionalSynchronizer::synchronize_regular_file(
	const fs::directory_entry &source_file,
	const fs::path &target_path
) {
	std::error_code err;
	fs::path result_target_path = target_path;

	if (fs::exists(target_path)) {
		const fs::directory_entry target_file(target_path);
		if (context.arguments.skips_conflicts()) return 0;

		const fs::file_time_type source_written_at = source_file.last_write_time(err);
		const fs::file_time_type target_written_at = target_file.last_write_time(err);
		if (err) return EXIT_CODE_FILESYSTEM_ERROR;

		if (source_written_at == target_written_at) return 0;
		if (source_written_at < target_written_at) {
			// do not copy older versions, but inform the user
			if (context.arguments.is_verbose())
				std::cout << "Skipped copying older version of " << source_file << "\n";
			return 0;
		}

		if (context.arguments.overwrites_conflicts()) {
			// no special treatment
		} else if (context.arguments.renames_conflicts()) {
			result_target_path = result_target_path.parent_path() / insert_timestamp_to_filename(target_file);
		}
	}

final:
	if (context.arguments.is_verbose())
		std::cout << "Copying " << source_file << "\n";
	if (context.arguments.is_dry_run()) return 0;

	fs::create_directories(result_target_path.parent_path(), err);
	fs::copy_file(source_file, result_target_path, fs::copy_options::overwrite_existing, err);
	if (err) return EXIT_CODE_FILESYSTEM_ERROR;
	return 0;
}

int MonodirectionalSynchronizer::synchronize_config_file(
	const fs::directory_entry &source_entry,
	const fs::path &target_path
) {
	std::error_code err;

	const bool target_directory_has_config = context.get_target_leaf_configuration().has_value();
	if (target_directory_has_config || !context.arguments.should_copy_configurations())
		return 0;

	if (context.arguments.is_verbose())
		std::cout << "Copying " << source_entry << "\n";
	if (context.arguments.is_verbose()) return 0;

	fs::create_directories(target_path.parent_path(), err);
	fs::copy_file(source_entry, target_path, err);
	if (err) return EXIT_CODE_FILESYSTEM_ERROR;

	return 0;
}

int MonodirectionalSynchronizer::synchronize_directory_entry(
	const fs::directory_entry &source_entry,
	const fs::path &target_directory
) {
	std::error_code err;
	const fs::file_status status = source_entry.status(err);
	if (err) {
		std::cerr << "Failed to check file status of " << source_entry << std::endl;
		return EXIT_CODE_FILESYSTEM_ERROR;
	}

	if (!context.should_synchronize(source_entry))
		return 0;

	const fs::path matching_target_path = target_directory / source_entry.path().filename();

	if (fs::is_directory(status))
		return synchronize_directories_recursively(
			source_entry,
			matching_target_path
		);
	if (fs::is_regular_file(status)) {
		if (is_config_file(source_entry))
			return synchronize_config_file(source_entry, matching_target_path);
		return synchronize_regular_file(source_entry, matching_target_path);
	}

	std::cerr << "Warning: unsupported file type of " << source_entry << std::endl;
	return 0;
}

int MonodirectionalSynchronizer::synchronize_directories_recursively(
	const fs::path &source_directory,
	const fs::path &target_directory
) {
	int error = context.load_configuration_pair(source_directory, target_directory);
	if (error) return error;

	for (const fs::directory_entry &source_entry : fs::directory_iterator(source_directory)) {
		error = synchronize_directory_entry(source_entry, target_directory);
		if (error) return error;
	}

	if (context.arguments.should_delete_extra_target_files())
		error = delete_extra_target_entries(source_directory, target_directory);

	context.pop_configuration_pair();
	return error;
}
