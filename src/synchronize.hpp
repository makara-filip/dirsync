#ifndef DIRSYNC_SYNCHRONIZE_HPP
#define DIRSYNC_SYNCHRONIZE_HPP

#include <filesystem>

#include "arguments.hpp"
#include "configuration/configuration.hpp"

namespace fs = std::filesystem;
using OptionalConfiguration = std::optional<DirectoryConfiguration>;

std::string get_formatted_time(const fs::file_time_type &time);
std::chrono::file_time<std::chrono::seconds> reduce_precision_to_seconds(const fs::file_time_type &file_time);
std::string insert_timestamp_to_filename(const fs::directory_entry &entry);

int synchronize_directories(const ProgramArguments &arguments);

/** An abstract base class for synchronization contexts.
 * Descendants may include synchronizer-specific information. */
class Context {
	public:
	const ProgramArguments &arguments;

	protected:
	explicit Context(const ProgramArguments &args): arguments(args) {}

	public:
	virtual ~Context() = 0;
};

inline Context::~Context() {}

/** An abstract base class for synchronization contexts using two directories.
 * Descendants can be one- or two-way sync contexts or other custom contexts.
 * Stores the recursive directory configurations in pairs of corresponding objects. */
class BinaryContext : public Context {
	public:
	using ConfigurationPair = std::pair<
		std::optional<DirectoryConfiguration>,
		std::optional<DirectoryConfiguration>
	>;

	protected:
	std::vector<ConfigurationPair> configuration_stack;
	std::pair<fs::path, fs::path> root_paths;

	explicit BinaryContext(const ProgramArguments &args)
		: Context(args), root_paths(args.get_source_path(), args.get_target_path()) {}

	public:
	/** For both directory paths, tries to read the local configurations from supported files. */
	int load_configuration_pair(const fs::path &path_first, const fs::path &path_second) {
		ConfigurationPair pair;

		int error = get_directory_configuration(path_first, arguments, pair.first);
		if (error) return error;
		error = get_directory_configuration(path_second, arguments, pair.second);
		if (error) return error;

		configuration_stack.push_back(std::move(pair));
		return 0;
	}

	/** Getter for the most specific, most local configuration pair
	 * (deepest in the file tree - leaf). */
	const ConfigurationPair &get_leaf_configuration_pair() const {
		return configuration_stack.back();
	}

	/** Discards the leaf configuration in the current configuration stack. */
	void pop_configuration_pair() {
		configuration_stack.pop_back();
	}
};

// TODO: declare a BinarySynchronizer? accepting only BinaryContexts

/** An abstract base class for a synchronizer. Descendants encapsulate
 * usage-specific helper functions. */
class Synchronizer {
	public:
	/** Starts the synchronization process, where the implementation is given
	 * by Synchronizer class descendants. Uses the provided context and program arguments. */
	virtual int synchronize() = 0;

	virtual ~Synchronizer() = default;

	// static bool supports_file_type(const fs::file_type type) {
	// 	if (type == fs::file_type::regular) return true;
	// 	if (type == fs::file_type::directory) return true;
	//
	// 	// other file types are unsupported - symlink, fifo...
	// 	return false;
	// }
};

#endif //DIRSYNC_SYNCHRONIZE_HPP
