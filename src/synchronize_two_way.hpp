#ifndef DIRSYNC_SYNCHRONIZE_TWO_WAY_HPP
#define DIRSYNC_SYNCHRONIZE_TWO_WAY_HPP

#include <filesystem>
#include <set>
#include <utility>

#include "synchronize.hpp"
#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

class BidirectionalContext final : public BinaryContext {
	public:
	explicit BidirectionalContext(const ProgramArguments &args) : BinaryContext(args) {}

	const fs::path &get_root_first() const { return root_paths.first; }
	const fs::path &get_root_second() const { return root_paths.second; }
};

/** Helper structure to store the directory entry with its file status and boolean existence.
 * It is constructed by an algorithm's state, the files are not guaranteed to exist. */
struct ChildEntryInfo {
	bool exists;
	fs::path path;
	fs::directory_entry entry;
	fs::file_status status;

	ChildEntryInfo(
		const fs::path &parent,
		const std::set<std::string> &collection,
		const std::string &name
	) {
		const auto iterator = collection.find(name);

		exists = iterator != collection.end();
		path = parent / name;
		if (!exists) return;

		entry = fs::directory_entry(parent / name);
		status = fs::status(path);
	}

	bool is_regular_file() const noexcept { return fs::is_regular_file(status); }
	bool is_directory() const noexcept { return fs::is_directory(status); }

	operator const fs::path &() const { return path; }
};

class BidirectionalSynchronizer final : public Synchronizer {
	BidirectionalContext &context;

	public:
	explicit BidirectionalSynchronizer(BidirectionalContext &context)
		: context(context) {}

	int synchronize() override {
		return synchronize_directories(
			context.get_root_first(),
			context.get_root_second()
		);
	}

	private:
	/** Performs a two-way synchronization recursively.
	* For each common directory in the input tree,
	* list all files and directories, store in a std::map<std::string>
	* and perform symmetric synchronization.
	* Uses BidirectionalContext instance to store configuration pairs in a stack. */
	int synchronize_directories(
		const fs::path &source_left,
		const fs::path &source_right
	) const;

	/** Synchronizes two directory entries, only one of which exists. */
	int synchronize_partial_entries(
		const ChildEntryInfo &left,
		const ChildEntryInfo &right
	) const;
	/** Synchronizes two directory entries, assuming both exist. */
	int synchronize_existing_entries(
		const ChildEntryInfo &left,
		const ChildEntryInfo &right
	) const;

	/** Syncs the files if the program arguments and local directory configuration allow it.
	 * Uses the specified filename conflict strategy. Determines which file content is newer
	 * by file's last written time. \n\n
	 * Example: a/example.txt gets copied to b/example.txt (did not exist). \n\n
	 * Example 2: b/common.txt gets copied to a/common.txt (already existed)
	 * because we use the default time-based conflict strategy and a/common.txt is newer. */
	int synchronize_files(
		const fs::directory_entry &left,
		const fs::directory_entry &right
	) const;

	/** Iterates all synchronizable entries and saves as strings to the output set. */
	static void get_directory_entry_names(
		const fs::path &directory,
		const OptionalConfiguration &config,
		std::set<std::string> &out_names
	);
};

#endif //DIRSYNC_SYNCHRONIZE_TWO_WAY_HPP
