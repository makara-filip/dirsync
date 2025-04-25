#include "synchronize.hpp"
#include "constants.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>

#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

std::string get_formatted_time(const fs::file_time_type &time) {
	return std::format("{:%F-%H-%M-%S}", time);
}

std::string get_formatted_time(const std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> &time) {
	return std::format("{:%F-%H-%M-%S}", time);
}

std::chrono::sys_time<std::chrono::seconds> convert_to_sys_time(const fs::file_time_type &file_time) {
	using namespace std::chrono;

	const time_point<system_clock, nanoseconds> system_time_nano = file_clock::to_sys(file_time);
	const time_point<system_clock, seconds> system_time_in_seconds = time_point_cast<seconds>(system_time_nano);

	return system_time_in_seconds;
}

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

std::string insert_timestamp_to_filename(const fs::directory_entry &entry) {
	return entry.path().stem().string()
		+ "-"
		+ get_formatted_time(convert_to_sys_time(entry.last_write_time()))
		+ entry.path().extension().string();
}

// TODO: rename to MonodirectionalContext ?
struct RecursiveSynchronizationContext {
	std::vector<std::optional<DirectoryConfiguration>>
		source_configuration_stack, target_configuration_stack;
	const std::filesystem::directory_entry *root_source, *root_target;

	RecursiveSynchronizationContext(const fs::directory_entry *source, const fs::directory_entry *target) {
		this->root_source = source;
		this->root_target = target;
	}

	bool should_copy(const fs::directory_entry &entry, const fs::file_status &status) const {
		return source_allows_to_copy(entry, status) && target_accepts(entry, status);
	}

	private:
	bool source_allows_to_copy(const fs::directory_entry &entry, const fs::file_status &status) const {
		for (auto it = source_configuration_stack.rbegin(); it != source_configuration_stack.rend(); ++it) {
			if (!it->has_value()) continue;
			const DirectoryConfiguration &configuration = it->value();
			if (!configuration.allows(entry, status)) return false;
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

void synchronize_files_bidirectionally(
	const ProgramArguments &arguments,
	const fs::directory_entry &left,
	const fs::directory_entry &right
) {
	if (arguments.conflict_resolution == ConflictResolutionMode::skip) return;

	const std::chrono::sys_time<std::chrono::seconds>
		left_write_time = convert_to_sys_time(left.last_write_time()),
		right_write_time = convert_to_sys_time(right.last_write_time());

	if (left_write_time == right_write_time)
		// considered equal
		return;

	const fs::directory_entry *older, *newer;
	if (left.last_write_time() < right.last_write_time()) {
		older = &left;
		newer = &right;
	} else {
		older = &right;
		newer = &left;
	}
	const fs::directory_entry *target = older;

	fs::path target_path;
	fs::copy_options options = fs::copy_options::overwrite_existing;
	std::error_code err;

	// if config file, keep newer, do not rename
	if (is_config_file(left) && is_config_file(right)) {
		if (!arguments.copy_configurations) return;
		goto final;
	}

	if (arguments.conflict_resolution == ConflictResolutionMode::overwrite_with_newer) {
		// no special action
		target_path = target->path();
	} else if (arguments.conflict_resolution == ConflictResolutionMode::rename) {
		target_path = target->path().parent_path() / insert_timestamp_to_filename(*target);
	}

final:
	if (arguments.verbose) std::cout << "Copying " << *newer << "\n";
	if (arguments.dry_run) return;
	fs::create_directories(target_path.parent_path(), err);
	fs::copy_file(*newer, target_path, options, err);
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
			std::cout << "Skipped copying older version of " << source_file << "\n";
			return;
		}

		options = fs::copy_options::overwrite_existing;

		if (arguments.conflict_resolution == ConflictResolutionMode::overwrite_with_newer) {
			// no special treatment
		} else if (arguments.conflict_resolution == ConflictResolutionMode::rename) {
			target_path.replace_filename(insert_timestamp_to_filename(target_file));
		}
	}

final:
	if (arguments.verbose) std::cout << "Copying " << source_file << "\n";
	if (arguments.dry_run) return;
	fs::create_directories(target_path.parent_path(), err);
	fs::copy_file(source_file, target_path, options, err);
}

int synchronize_directories_recursively(
	const ProgramArguments &arguments,
	RecursiveSynchronizationContext &context,
	const fs::directory_entry &source_directory,
	const fs::directory_entry &target_directory
) {
	// read this directory configurations, both source and target
	// add them to the configuration stack
	// make a composite configuration? - compressing all what to copy and what not
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

		const fs::directory_entry matching_target_entry(
			target_directory.path() / source_entry.path().filename(),
			err
		);

		if (fs::is_directory(status)) {
			if (!context.should_copy(source_entry, status)) continue;
			error = synchronize_directories_recursively(
				arguments,
				context,
				source_entry,
				matching_target_entry
			);
		} else if (fs::is_regular_file(status)) {
			synchronize_file(arguments, context, source_entry, status, matching_target_entry, err);
		}
	}

	if (arguments.delete_extra_target_files)
		delete_extra_target_entries(arguments, source_directory, target_directory);

	context.source_configuration_stack.pop_back();
	context.target_configuration_stack.pop_back();

	return error;
}

struct child_entry_info {
	bool exists;
	fs::directory_entry entry;
	fs::file_status status;

	child_entry_info(
		const fs::directory_entry &parent,
		const std::set<std::string> &collection,
		const std::string &name
	) {
		const auto iterator = collection.find(name);

		exists = iterator != collection.end();
		entry = fs::directory_entry(parent.path() / name);
		status = fs::status(entry);
	}

	constexpr bool is_regular_file() const noexcept { return fs::is_regular_file(status); }
	constexpr bool is_directory() const noexcept { return fs::is_directory(status); }
};

int synchronize_directories_bidirectionally(
	const ProgramArguments &arguments,
	BidirectionalContext &context,
	const fs::directory_entry &source_left,
	const fs::directory_entry &source_right
) {
	// add to recursive stack
	BidirectionalContext::ConfigurationPair pair;
	get_directory_configuration(source_left, arguments, pair.first);
	get_directory_configuration(source_right, arguments, pair.second);
	context.configuration_stack.emplace_back(pair);

	std::set<std::string> left_names, right_names;
	std::set<std::string> all_entry_names;

	auto get_names = [&all_entry_names](
		const fs::directory_entry &directory,
		const std::optional<DirectoryConfiguration> &config,
		std::set<std::string> &out_names) -> void {
		for (const auto &entry : fs::directory_iterator(directory)) {
			const std::string filename = entry.path().filename().string();
			if (config.has_value() && !config->allows(entry, fs::status(entry))) continue;
			out_names.insert(filename);
			all_entry_names.insert(filename);
		}
	};

	get_names(source_left, pair.first, left_names);
	get_names(source_right, pair.second, right_names);

	for (const std::string &name : all_entry_names) {
		child_entry_info left(source_left, left_names, name);
		child_entry_info right(source_right, right_names, name);

		if (left.exists ^ right.exists) {
			// if exactly one of the two exist, switch to normal one-directional synchronization

			child_entry_info *source, *target;
			if (left.exists) {
				source = &left;
				target = &right;
			} else {
				source = &right;
				target = &left;
			}

			// TODO: is there loss of configuration data?
			// Possible solution?: convert from recursive one-directional context to bidirectional one
			// while keeping the loaded directory configurations

			if (source->is_directory()) {
				RecursiveSynchronizationContext single_direction_context(&source->entry, &target->entry);
				synchronize_directories_recursively(
					arguments,
					single_direction_context,
					source->entry,
					target->entry
				);
			} else if (fs::is_regular_file(source->status)) {
				if (arguments.verbose) std::cout << "Copying " << source->entry << "\n";
				if (arguments.dry_run) continue;
				fs::copy_options options = fs::copy_options::skip_existing;
				std::error_code err;
				fs::copy_file(source->entry, target->entry, options, err);
			}
		} else {
			// both right and left exist
			if (left.is_regular_file() && right.is_regular_file()) {
				synchronize_files_bidirectionally(arguments, left.entry, right.entry);
			} else if (left.is_directory() && right.is_directory()) {
				synchronize_directories_bidirectionally(arguments, context, left.entry, right.entry);
			} else {
				// incompatible types; symbolic links are not supported
				std::cerr << "Incompatible directory entry types at " << left.entry << " and " << right.entry << "\n";
			}
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
		error = synchronize_directories_recursively(arguments, context, source_directory, target_directory);
	} else {
		// we do not have source and target directories, we have two source ones

		error = verify_source_directory(source_path, source_directory, source_status);
		if (error) return error;
		error = verify_source_directory(target_path, target_directory, target_status);
		if (error) return error;

		fs::directory_entry source_left = source_directory;
		fs::directory_entry source_right = target_directory;

		BidirectionalContext context(&source_left, &source_right);
		error = synchronize_directories_bidirectionally(arguments, context, source_left, source_right);
	}

	return error;
}

