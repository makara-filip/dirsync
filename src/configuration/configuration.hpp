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

struct DirectoryConfiguration {
	// TODO: verify the version at runtime
	Version config_version;
	//	DirectoryRole role = DirectoryRole::unspecified;
	//	 DateTime last_synchronized_date; // TODO
	//	ExclusionFlags exclusionFlags;

	// TODO: make a distinction between "not accepting" and "not allowing" patterns?
	// is there a real-world scenario for this logic?
	std::vector<std::string> exclusion_patterns;
	std::optional<std::uintmax_t> max_file_size;

	bool allows(const std::filesystem::directory_entry &entry) const {
		// const std::string filename = entry.path().filename().string();
		// TODO: implement "not accepting" and "not allowing" patterns?
		return true;
	}

	bool accepts(const std::filesystem::directory_entry &entry, const std::filesystem::file_status &status) const {
		const std::string filename = entry.path().filename().string();

		if (std::filesystem::is_regular_file(status) && max_file_size.has_value()) {
			const std::uintmax_t size = std::filesystem::file_size(filename);
			if (size > *max_file_size) return false;
		}

		for (const auto &pattern : exclusion_patterns) {
			if (wildcard_matches(pattern, filename))
				return false;
		}

		return allows(entry);
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

class DirectoryConfigurationReader {
	public:
	virtual DirectoryConfigurationReadResult read_from_directory(
		const std::filesystem::directory_entry &directory,
		const ProgramArguments &arguments
	) const = 0;
	virtual const char *config_file_name() const = 0;

	protected:
	~DirectoryConfigurationReader() = default;
};

int get_directory_configuration(
	const std::filesystem::directory_entry &directory,
	const ProgramArguments &arguments,
	std::optional<DirectoryConfiguration> &configuration
);

#endif //DIRSYNC_DIRECTORY_CONFIG_HPP
