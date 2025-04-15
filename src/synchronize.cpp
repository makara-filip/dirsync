#include "synchronize.hpp"
#include "constants.hpp"

#include <filesystem>
#include <iostream>
// #include <ranges>
#include <set>
#include <string>

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

	const std::chrono::system_clock::time_point time = std::chrono::system_clock::now();

	RecursiveSynchronizationContext(fs::directory_entry *source, fs::directory_entry *target) {
		this->source_directory = source;
		this->target_directory = target;
	}

	bool should_copy(const fs::directory_entry &entry, const fs::file_status &status) const {
		return source_allows_to_copy(entry) && target_accepts(entry, status);
	}

	private:
	bool source_allows_to_copy(const fs::directory_entry &entry) const {
		for (auto it = source_configuration_stack.rbegin(); it != source_configuration_stack.rend(); ++it) {
			if (!it->has_value()) continue;
			const DirectoryConfiguration &configuration = it->value();
			if (!configuration.allows(entry)) return false;
		}
		return true;
	}

	bool target_accepts(const fs::directory_entry &entry, const fs::file_status &status) const {
		for (auto it = target_configuration_stack.rbegin(); it != target_configuration_stack.rend(); ++it) {
			if (!it->has_value()) continue;
			const DirectoryConfiguration &configuration = it->value();
			if (!configuration.accepts(entry, status)) return false;
		}
		return true;
	}
};

void delete_extra_target_entries(const ProgramArguments &arguments, const RecursiveSynchronizationContext &context) {
	for (const fs::directory_entry &target_entry : fs::directory_iterator(*context.target_directory)) {
		std::error_code err;
		const fs::file_status status = fs::status(target_entry, err);

		const fs::directory_entry matching_source_entry(
			context.source_directory->path() / target_entry.path().filename(),
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

void synchronize_file(
	const ProgramArguments &arguments,
	const RecursiveSynchronizationContext &context,
	const fs::directory_entry &source_file,
	const fs::file_status &source_status,
	const fs::directory_entry &target_file,
	std::error_code &err
) {
	const fs::file_status target_status = fs::status(target_file, err);
	fs::path target_path = target_file.path();
	fs::copy_options options = fs::copy_options::skip_existing;

	// TODO: handle OS filesystem case (in)sensitivity

	if (is_config_file(source_file)) {
		bool target_directory_has_config = context.target_configuration_stack.back().has_value();
		if (target_directory_has_config || !arguments.copy_configurations) return;
		goto final;
	}

	if (!context.should_copy(source_file, source_status)) return;

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
			const long long ticks = context.time.time_since_epoch().count();
			target_path.replace_filename(target_path.filename() / std::to_string(ticks));
		}
	}

final:
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
			if (!context.should_copy(source_entry, status)) continue;
			RecursiveSynchronizationContext recursive_context = context;
			recursive_context.source_directory = &source_entry;
			recursive_context.target_directory = &matching_target_entry;
			error = synchronize_directories_recursively(recursive_context, arguments);
		} else if (fs::is_regular_file(status)) {
			synchronize_file(arguments, context, source_entry, status, matching_target_entry, err);
		}
	}

	if (arguments.delete_extra_target_files)
		delete_extra_target_entries(arguments, context);

	context.source_configuration_stack.pop_back();
	context.target_configuration_stack.pop_back();

	return error;
}

int synchronize_directories_bidirectionally(
	const fs::directory_entry &source_left,
	const fs::directory_entry &source_right,
	RecursiveSynchronizationContext &context
) {
	// TODO: load configuration somehow

	std::set<std::string> left_names, right_names;
	std::set<std::string> all_entry_names;

	for (const auto &entry : fs::directory_iterator(*context.source_directory)) {
		const std::string filename = entry.path().filename().string();
		// left_entries[filename] = entry;
		left_names.insert(filename);
		all_entry_names.insert(filename);
	}
	for (const auto &entry : fs::directory_iterator(*context.target_directory)) {
		const std::string filename = entry.path().filename().string();
		right_names.insert(filename);
		// right_entries[filename] = entry;
		all_entry_names.insert(filename);
	}

	// std::set<std::string> all_entry_names;
	// for (const std::string &name : std::views::keys(left_entries)) all_entry_names.insert(name);
	// for (const std::string &name : std::views::keys(right_entries)) all_entry_names.insert(name);

	for (const std::string &name : all_entry_names) {
		const auto left_it = left_names.find(name);
		const auto right_it = right_names.find(name);
		bool left_exists = left_it != left_names.end();
		bool right_exists = right_it != right_names.end();

		fs::directory_entry left_entry(source_left.path() / name);
		fs::directory_entry right_entry(source_right.path() / name);

		fs::file_status left_status = fs::status(left_entry);
		fs::file_status right_status = fs::status(right_entry);

		// if exactly one of the two exist, switch to normal one-directional synchronization
		if (left_exists ^ right_exists) {
			// TODO: add to context?
			// TODO: correct order
			// synchronize_directories_recursively(context, arguments);
		} else {
			// both right and left exist, make a decision how to declare conflicts
			// and if so, how to handle the conflict
		}
	}

	return 0;
}

int synchronize_directories(const ProgramArguments &arguments) {
	const fs::path source_path(arguments.source_directory);
	const fs::path target_path(arguments.target_directory);

	fs::directory_entry source_directory, target_directory;
	fs::file_status source_status, target_status;

	int error = 0;
	if (arguments.is_one_way_synchronization) {
		error = verify_source_directory(source_path, source_directory, source_status);
		if (error) return error;
		error = ensure_target_directory(target_path, target_directory, target_status);
		if (error) return error;

		RecursiveSynchronizationContext context(&source_directory, &target_directory);
		error = synchronize_directories_recursively(context, arguments);
	} else {
		// we do not have source and target directories, we have two source ones

		error = verify_source_directory(source_path, source_directory, source_status);
		if (error) return error;
		error = verify_source_directory(target_path, target_directory, target_status);

		fs::directory_entry source_left = source_directory;
		fs::directory_entry source_right = target_directory;

		// TODO: recursive context?
		error = synchronize_directories_bidirectionally(source_left, source_right);
	}

	return error;
}
