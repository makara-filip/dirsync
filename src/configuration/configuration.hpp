#ifndef DIRSYNC_DIRECTORY_CONFIG_HPP
#define DIRSYNC_DIRECTORY_CONFIG_HPP

#include <filesystem>
#include <optional>
#include <variant>
#include <vector>

#include "../arguments.hpp"
#include "../constants.hpp"
#include "../wildcards.hpp"

//enum class DirectoryRole {
//	unspecified,
//	backup,
//	personal_computer,
//};
//
//struct ExclusionFlags {
//	bool media = false;
//	//	bool
//};

/** A local directory configuration, saved as a file inside the directory it configures.
 * Contains information about the wildcard-supported excluded patterns and other
 * filtering information. */
class DirectoryConfiguration {
	Version config_version;
	std::vector<std::string> exclusion_patterns;
	std::optional<std::uintmax_t> max_file_size;

	//	DirectoryRole role = DirectoryRole::unspecified;
	//	DateTime last_synchronized_date;
	//	ExclusionFlags exclusionFlags;

	// TODO: make a distinction between "not accepting" and "not allowing" patterns?
	// is there a real-world scenario for this logic?

	// expose private members to JSON serializer/parser functions
	using Json = nlohmann::json;
	friend void from_json(const Json &j, DirectoryConfiguration &p);
	friend void to_json(Json &j, const DirectoryConfiguration &p);

	public:
	// /** Getter for the semver (sematic versioning) configuration version
	//  * which should be compatible with the program version to work correctly. */
	// const Version &get_version() const { return config_version; }
	//
	// /** Getter for a list of excluded filenames that are not copied from nor to the configured directory.
	//  * Supports wildcards with asterisk '*' character, e.g. `example-*.txt` or `*.log`. */
	// const std::vector<std::string> &get_exclusion_patterns() const {
	// 	return exclusion_patterns;
	// }
	//
	// /** Getter for an optional maximum file size in bytes that is allowed
	//  * to be copied to the configured directory. */
	// const std::optional<std::uintmax_t> &get_max_file_size() const {
	// 	return max_file_size;
	// }

	/** Returns true if the filesystem entry is allowed to be copied
	 * from the directory configured by this instance. */
	bool allows(const std::filesystem::directory_entry &entry) const {
		// TODO: implement "not accepting" and "not allowing" patterns?
		return accepts(entry);
	}

	/** Returns true if the filesystem entry is accepted
	 * in the directory configured by this instance. */
	bool accepts(const std::filesystem::directory_entry &entry) const {
		const std::string filename = entry.path().filename().string();

		if (max_file_size.has_value() && entry.is_regular_file()) {
			const std::uintmax_t size = entry.file_size();
			if (size > *max_file_size) return false;
		}

		for (const auto &pattern : exclusion_patterns) {
			if (wildcard_matches(pattern, filename))
				return false;
		}

		return true;
	}
};

struct DirectoryConfigurationFileNonexistent {};
struct DirectoryConfigurationParseError {};
struct DirectoryConfigurationIncompatible {};

using DirectoryConfigurationReadResult = std::variant<
	DirectoryConfiguration,
	DirectoryConfigurationParseError,
	DirectoryConfigurationIncompatible,
	DirectoryConfigurationFileNonexistent,
	std::filesystem::filesystem_error
>;

constexpr char CONFIG_FILE_NAME_PREFIX[] = ".dirsync";

bool is_config_file(const std::filesystem::path &path);

/** An abstract reader and parser of the local directory configurations.
 * If you want to support a new file format, create a descendant class and implement
 * its virtual functions. Add an instance to the `supported_readers` list. */
class DirectoryConfigurationReader {
	public:
	/** Tries to read the configuration. Returns a composite instance
	 * - config or file error or "not found". */
	virtual DirectoryConfigurationReadResult read_from_directory(
		const std::filesystem::path &directory,
		const ProgramArguments &arguments
	) const = 0;

	/** Gets the config filename specific to this format.
	 * Example: .dirsync.json for JSON format. */
	virtual const char *config_file_name() const = 0;

	virtual ~DirectoryConfigurationReader() = default;
};

int get_directory_configuration(
	const std::filesystem::path &directory,
	const ProgramArguments &arguments,
	std::optional<DirectoryConfiguration> &configuration
);

#endif //DIRSYNC_DIRECTORY_CONFIG_HPP

