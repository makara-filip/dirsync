#include "synchronize_two_way.hpp"

#include <filesystem>
#include <optional>
#include <set>
#include <utility>

#include "synchronize.hpp"
#include "synchronize_one_way.hpp"
#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

/** Syncs the files if the program arguments and local directory configuration allow it.
 * Uses the specified filename conflict strategy. Determines which file content is newer
 * by file's last written time. \n\n
 * Example: a/example.txt gets copied to b/example.txt (did not exist). \n\n
 * Example 2: b/common.txt gets copied to a/common.txt (already existed)
 * because we use the default time-based conflict strategy and a/common.txt is newer. */
void synchronize_files_bidirectionally(
	const ProgramArguments &arguments,
	const fs::directory_entry &left,
	const fs::directory_entry &right
) {
	if (arguments.conflict_resolution == ConflictResolutionMode::skip) return;

	const std::chrono::time_point<std::chrono::file_clock, std::chrono::seconds>
		left_write_time = reduce_precision_to_seconds(left.last_write_time()),
		right_write_time = reduce_precision_to_seconds(right.last_write_time());

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

/** Helper structure to store the directory entry with its file status and boolean existence.
 * It is constructed by an algorithm's state, the files are not guaranteed to exist. */
struct ChildEntryInfo {
	bool exists;
	fs::directory_entry entry;
	fs::file_status status;

	ChildEntryInfo(
		const fs::directory_entry &parent,
		const std::set<std::string> &collection,
		const std::string &name
	) {
		const auto iterator = collection.find(name);

		exists = iterator != collection.end();
		entry = fs::directory_entry(parent.path() / name);
		status = fs::status(entry);
	}

	bool is_regular_file() const noexcept { return fs::is_regular_file(status); }
	bool is_directory() const noexcept { return fs::is_directory(status); }
};

/** Performs a two-way synchronization recursively.
* For each common directory in the input tree,
* list all files and directories, store in a std::map<std::string>
* and perform symmetric synchronization.
* If a directory exists in just one directory, this function uses one-way synchronization.
* If a common directory, calls itself recursively.
* If a file, calls `synchronize_files_bidirectionally`.
* Uses BidirectionalContext instance to store configuration pairs in a stack. */
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
		ChildEntryInfo left(source_left, left_names, name);
		ChildEntryInfo right(source_right, right_names, name);

		if (left.exists ^ right.exists) {
			// if exactly one of the two exist, switch to normal one-directional synchronization

			ChildEntryInfo *source, *target;
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
				MonodirectionalContext single_direction_context(&source->entry, &target->entry);
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
