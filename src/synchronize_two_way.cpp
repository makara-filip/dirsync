#include "synchronize_two_way.hpp"

#include <filesystem>
#include <optional>
#include <set>
#include <utility>

#include "synchronize.hpp"
#include "synchronize_one_way.hpp"
#include "configuration/configuration.hpp"

int BidirectionalSynchronizer::synchronize_files(
	const fs::directory_entry &left,
	const fs::directory_entry &right
) const {
	if (context.arguments.skips_conflicts()) return 0;

	const std::chrono::time_point<std::chrono::file_clock, std::chrono::seconds>
		left_write_time = reduce_precision_to_seconds(left.last_write_time()),
		right_write_time = reduce_precision_to_seconds(right.last_write_time());

	if (left_write_time == right_write_time)
		// considered equal
		return 0;

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
	constexpr fs::copy_options options = fs::copy_options::overwrite_existing;
	std::error_code err;

	// if config file, keep newer, do not rename
	if (is_config_file(left) && is_config_file(right)) {
		if (!context.arguments.should_copy_configurations()) return 0;
		goto final;
	}

	if (context.arguments.overwrites_conflicts()) {
		// no special action
		target_path = target->path();
	} else if (context.arguments.renames_conflicts()) {
		target_path = target->path().parent_path() / insert_timestamp_to_filename(*target);
	}

final:
	if (context.arguments.is_verbose()) std::cout << "Copying " << *newer << "\n";
	if (context.arguments.is_dry_run()) return 0;

	fs::create_directories(target_path.parent_path(), err);
	fs::copy_file(*newer, target_path, options, err);

	if (err) return EXIT_CODE_FILESYSTEM_ERROR;
	return 0;
}

void BidirectionalSynchronizer::get_directory_entry_names(
	const fs::path &directory,
	const OptionalConfiguration &config,
	std::set<std::string> &out_names
) {
	for (const auto &entry : fs::directory_iterator(directory)) {
		const std::string filename = entry.path().filename().string();
		if (config.has_value() && !config->allows(entry)) continue;
		out_names.insert(filename);
	}
}

int BidirectionalSynchronizer::synchronize_partial_entries(
	const ChildEntryInfo &left,
	const ChildEntryInfo &right
) const {
	// if exactly one of the two exist, switch to normal one-directional synchronization

	const ChildEntryInfo *source, *target;
	if (left.exists) {
		source = &left;
		target = &right;
	} else {
		source = &right;
		target = &left;
	}

	if (source->is_directory()) {
		// TODO: is there loss of configuration data?
		// Possible solution?: convert from recursive one-directional context to bidirectional one
		// while keeping the loaded directory configurations

		ProgramArgumentsBuilder builder(context.arguments);
		builder.set_source_directory(source->path);
		builder.set_target_directory(target->path);

		MonodirectionalContext one_way_context(builder.build());
		MonodirectionalSynchronizer one_way_synchronizer(one_way_context);
		return one_way_synchronizer.synchronize();
	}

	if (fs::is_regular_file(source->status)) {
		if (context.arguments.is_verbose())
			std::cout << "Copying " << source->path << "\n";
		if (context.arguments.is_dry_run()) return 0;

		constexpr fs::copy_options options = fs::copy_options::skip_existing;
		std::error_code err;
		fs::copy_file(*source, *target, options, err);
		if (err) return EXIT_CODE_FILESYSTEM_ERROR;
		return 0;
	}

	// report unsupported file type
	std::cerr << "Incompatible file types at " << source->path << std::endl;
	return EXIT_CODE_INCOMPATIBLE_ENTRIES;
}

int BidirectionalSynchronizer::synchronize_existing_entries(
	const ChildEntryInfo &left,
	const ChildEntryInfo &right
) const {
	// both right and left exist
	if (left.is_regular_file() && right.is_regular_file())
		return synchronize_files(left.entry, right.entry);
	if (left.is_directory() && right.is_directory())
		return synchronize_directories(left.path, right.path);

	// incompatible types; symbolic links are not supported
	std::cerr << "Incompatible directory entry types at " << left.path << " and " << right.path << "\n";
	return EXIT_CODE_INCOMPATIBLE_ENTRIES;
}

int BidirectionalSynchronizer::synchronize_directories(
	const fs::path &source_left,
	const fs::path &source_right
) const {
	context.load_configuration_pair(source_left, source_right);
	BinaryContext::ConfigurationPair pair = context.get_leaf_configuration_pair();

	std::set<std::string> left_names, right_names;
	std::set<std::string> all_entry_names;

	get_directory_entry_names(source_left, pair.first, left_names);
	get_directory_entry_names(source_right, pair.second, right_names);

	all_entry_names.insert_range(left_names);
	all_entry_names.insert_range(right_names);

	for (const std::string &name : all_entry_names) {
		ChildEntryInfo left(source_left, left_names, name);
		ChildEntryInfo right(source_right, right_names, name);
		int error = 0;

		if (left.exists ^ right.exists) {
			error = synchronize_partial_entries(left, right);
		} else {
			error = synchronize_existing_entries(left, right);
		}

		if (error) return error;
	}

	return 0;
}
