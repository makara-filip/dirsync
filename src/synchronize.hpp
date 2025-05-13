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

class Context {
	public:
	const ProgramArguments &arguments;

	protected:
	explicit Context(const ProgramArguments &args): arguments(args) {}

	public:
	virtual ~Context() = 0;
};

inline Context::~Context() {}

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
	int load_configuration_pair(const fs::path &path_first, const fs::path &path_second) {
		ConfigurationPair pair;

		int error = get_directory_configuration(path_first, arguments, pair.first);
		if (error) return error;
		error = get_directory_configuration(path_second, arguments, pair.second);
		if (error) return error;

		configuration_stack.push_back(std::move(pair));
		return 0;
	}

	const ConfigurationPair &get_leaf_configuration_pair() const {
		return configuration_stack.back();
	}

	void pop_configuration_pair() {
		configuration_stack.pop_back();
	}
};

// TODO: declare a BinarySynchronizer? accepting only BinaryContexts

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
