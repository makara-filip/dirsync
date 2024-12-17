#ifndef DIRSYNC_DIRECTORY_CONFIG_HPP
#define DIRSYNC_DIRECTORY_CONFIG_HPP

#include <filesystem>
#include <optional>
#include <variant>
#include <vector>

#include "../arguments.hpp"
#include "../constants.hpp"

// TODO: introduce ".dirsyncignore" file?
// It would exclude paths from the same directory from being synchronized to other
// target directories (file will have any effect only when in source directory).

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
	Version config_version;
	//	DirectoryRole role = DirectoryRole::unspecified;
	//	 DateTime last_synchronized_date; // TODO
	//	ExclusionFlags exclusionFlags;
	//	std::vector<std::string> exclusionPatterns;
};

struct DirectoryConfigurationFileNonexistent {};
struct DirectoryConfigurationParseError {};

using DirectoryConfigurationReadResult = std::variant<
	DirectoryConfiguration,
	DirectoryConfigurationParseError,
	DirectoryConfigurationFileNonexistent,
	std::filesystem::filesystem_error
>;

class DirectoryConfigurationReader {
	public:
	virtual DirectoryConfigurationReadResult read_from_directory(
		const std::filesystem::directory_entry &directory,
		const ProgramArguments &arguments
	) const = 0;

	protected:
	~DirectoryConfigurationReader() = default;
};

std::optional<DirectoryConfiguration> get_directory_configuration(
	const std::filesystem::directory_entry &directory,
	const ProgramArguments &arguments
);

#endif //DIRSYNC_DIRECTORY_CONFIG_HPP
