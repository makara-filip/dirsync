#ifndef DIRSYNC_ARGUMENTS_HPP
#define DIRSYNC_ARGUMENTS_HPP

#include <optional>
#include <string>
#include <vector>

/** The program sub-command. */
enum class ProgramMode {
	help,
	synchronize,
	test
	// TODO: other options such as:
	// - ignore <file-or-directory>: adds the file/directrory to the .dirsync config file
};

/** A conflict-resolving strategy when synchronizing different versions
 * of the same filename in the same source/target location. */
enum class ConflictResolutionMode {
	overwrite_with_newer,
	skip,
	rename,
};

class ProgramArguments {
	std::string executable;
	ProgramMode mode = ProgramMode::synchronize;
	bool verbose = false;
	bool dry_run = false;

	bool copy_configurations = false;
	bool delete_extra_target_files = false;

	bool is_one_way_synchronization = true;

	ConflictResolutionMode conflict_resolution = ConflictResolutionMode::overwrite_with_newer;

	std::string source_directory;
	std::string target_directory;

	public:
	// PUBLIC GETTERS:

	const std::string &get_executable() const { return executable; }
	ProgramMode get_mode() const { return mode; }
	bool is_verbose() const { return verbose; }
	bool is_dry_run() const { return dry_run; }

	bool should_copy_configurations() const { return copy_configurations; }
	bool should_delete_extra_target_files() const { return delete_extra_target_files; }

	bool is_one_way() const { return is_one_way_synchronization; }
	ConflictResolutionMode get_conflict_resolution_mode() const {
		return conflict_resolution;
	}
	bool skips_conflicts() const {
		return conflict_resolution == ConflictResolutionMode::skip;
	}
	bool overwrites_conflicts() const {
		return conflict_resolution == ConflictResolutionMode::overwrite_with_newer;
	}
	bool renames_conflicts() const {
		return conflict_resolution == ConflictResolutionMode::rename;
	}

	const std::string &get_source_path() const { return source_directory; }
	const std::string &get_target_path() const { return target_directory; }

	std::ostream &operator<<(std::ostream &stream) const;

	/** Tries to parse the input arguments.
	* Augments the `ProgramArguments` instance. */
	static std::optional<ProgramArguments> try_parse(
		const std::vector<std::string> &arguments
	);

	private:
	bool try_parse_impl(const std::vector<std::string> &arguments);

	friend class ProgramArgumentsBuilder;

	// private constructor disallows creating default instances
	// outside this class
	ProgramArguments() = default;
};

class ProgramArgumentsBuilder {
	ProgramArguments arguments;
	using Self = ProgramArgumentsBuilder;

	public:
	ProgramArguments build() const { return arguments; }

	Self &set_source_directory(const std::string &path) {
		arguments.source_directory = path;
		return *this;
	}
	Self &set_target_directory(const std::string &path) {
		arguments.target_directory = path;
		return *this;
	}
	Self &set_two_way() {
		arguments.is_one_way_synchronization = false;
		return *this;
	}
	Self &set_verbosity(bool v) {
		arguments.verbose = v;
		return *this;
	}
	Self &set_conflict_resolution(ConflictResolutionMode mode) {
		arguments.conflict_resolution = mode;
		return *this;
	}
};

#endif // DIRSYNC_ARGUMENTS_HPP
