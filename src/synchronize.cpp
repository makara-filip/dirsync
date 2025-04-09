#include "synchronize.hpp"
#include "constants.hpp"

#include <filesystem>
#include <iostream>

#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

int verify_source_directory(const fs::path &path, fs::directory_entry &directory, fs::file_status &status) {
	try {
		if (!fs::exists(path)) {
			std::cerr << "Source directory does not exist." << std::endl;
			return EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY;
		}
		directory = fs::directory_entry(path);
		status = directory.status();
	} catch (fs::filesystem_error &error) {
		std::cerr << "Failed to check the source directory details." << std::endl
			<< "System error: " << error.what() << std::endl;
		return EXIT_CODE_FILESYSTEM_ERROR;
	}

	if (!fs::is_directory(status)) {
		std::cerr << "Error: Source path is not a directory." << std::endl;
		return EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY;
	}

	return 0;
}

int ensure_target_directory(const fs::path &path, fs::directory_entry &directory, fs::file_status &status) {
	std::error_code error;
	fs::create_directories(path, error);
	if (error) {
		std::cerr << "Error: Failed to ensure the target directory existence." << std::endl;
		std::cerr << "System error: " << error.message() << std::endl;
		return EXIT_CODE_FILESYSTEM_ERROR;
	}
	directory = fs::directory_entry(path, error);
	if (error) return EXIT_CODE_FILESYSTEM_ERROR;
	status = directory.status();
	return 0;
}

struct RecursiveSynchronizationContext {
	std::vector<std::optional<DirectoryConfiguration>>
		source_configuration_stack, target_configuration_stack;
	const std::filesystem::directory_entry *source_directory, *target_directory;

	const auto time = std::chrono::system_clock::now();

	RecursiveSynchronizationContext(fs::directory_entry *source, fs::directory_entry *target) {
		this->source_directory = source;
		this->target_directory = target;
	}

	bool source_allows_to_copy(const fs::directory_entry &entry) {
		// TODO: finish this
		return true;
	}

	bool target_accepts(const fs::directory_entry &entry) {
		// TODO: finish this
		return true;
	}
};

void synchronize_file(
	const ProgramArguments &arguments,
	RecursiveSynchronizationContext &context,
	const fs::directory_entry &source_file,
	const fs::directory_entry &target_file,
	std::error_code &err
) {
	// TODO: what about .dirsync files themselves? To copy or not to copy? That is the question.
	bool is_config_file = source_file.path().filename().string().starts_with(CONFIG_FILE_NAME_PREFIX);
	if (is_config_file) {
		return;
		// TODO: finish this

		if (!arguments.copy_configurations) return;
		return;
	}

	bool copy = context.source_allows_to_copy(source_file) && context.target_accepts(source_file);
	if (!copy) return;

	const fs::file_status target_status = fs::status(target_file, err);
	fs::path target_path = target_file.path();
	fs::copy_options options = fs::copy_options::skip_existing;

	if (fs::exists(target_status)) {
		if (arguments.conflict_resolution == ConflictResolutionMode::skip) return;

		const fs::file_time_type source_written_at = fs::last_write_time(source_file);
		const fs::file_time_type target_written_at = fs::last_write_time(target_file);

		if (source_written_at == target_written_at) return;
		if (source_written_at < target_written_at) {
			// do not copy older versions, but inform the user
			// TODO: bi-directional synchronization handles this differently
			std::cout << "Skipped copying older version of " << source_file << "\n";
			return;
		}

		options = fs::copy_options::overwrite_existing;

		if (arguments.conflict_resolution == ConflictResolutionMode::overwrite_with_newer) {
			// no special treatment
		} else if (arguments.conflict_resolution == ConflictResolutionMode::rename) {
			const auto ticks = context.time.time_since_epoch().count();
			target_path.replace_filename(target_path.filename() / std::string(ticks));
		}
	}

	if (arguments.verbose) std::cout << "Copying " << source_file << "\n";
	if (arguments.dry_run) return;
	fs::copy_file(source_file, target_path, options, err);
}

int synchronize_directories_recursively(RecursiveSynchronizationContext context, const ProgramArguments &arguments) {
	// read this directory configurations, both source and target
	// add them to the configuration stack
	// make a composite configuration? - compressing all what to copy and what not
	// iterate over the source children
	// - if a file, copy if the config allows
	// - if a directory, call itself recursively with deeper configuration stack

	std::optional<DirectoryConfiguration> source_configuration, target_configuration;
	int error = get_directory_configuration(*context.source_directory, arguments, source_configuration);
	error |= get_directory_configuration(*context.target_directory, arguments, target_configuration);

	context.source_configuration_stack.push_back(source_configuration);
	context.target_configuration_stack.push_back(target_configuration);

	if (error) return error;

	// TODO: make a composite configuration?

	for (const fs::directory_entry &source_entry : fs::directory_iterator(*context.source_directory)) {
		std::error_code err;
		const fs::file_status status = fs::status(source_entry, err);
		if (err) {
			std::cerr << "Failed to check file status of " << source_entry << std::endl;
			continue;
		}

		const fs::directory_entry matching_target_entry(
			context.target_directory->path() / source_entry.path().filename(),
			err
		);

		if (fs::is_directory(status)) {
			// TODO: check if the config allows synchronization
			RecursiveSynchronizationContext recursive_context = context;
			recursive_context.source_directory = &source_entry;
			recursive_context.target_directory = &matching_target_entry;
			synchronize_directories_recursively(recursive_context, arguments);
		} else if (fs::is_regular_file(status)) {
			synchronize_file(arguments, context, source_entry, matching_target_entry, err);
		}
	}

	context.source_configuration_stack.pop_back();
	context.target_configuration_stack.pop_back();

	return error;
}

int synchronize_directories(const ProgramArguments &arguments) {
	const fs::path source_path(arguments.source_directory);
	const fs::path target_path(arguments.target_directory);

	fs::directory_entry source_directory, target_directory;
	fs::file_status source_status, target_status;

	int error;
	error = verify_source_directory(source_path, source_directory, source_status);
	if (error) return error;
	error = ensure_target_directory(target_path, target_directory, target_status);
	if (error) return error;

	RecursiveSynchronizationContext context(&source_directory, &target_directory);
	error = synchronize_directories_recursively(context, arguments);

	return error;
}
