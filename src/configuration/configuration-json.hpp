#ifndef DIRSYNC_CONFIGURATION_JSON_HPP
#define DIRSYNC_CONFIGURATION_JSON_HPP

#include "configuration.hpp"
#include "../json.hpp"

#define AUTOGENERATE_JSON_CONVERSION NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE

constexpr const char *DIRECTORY_CONFIG_FILE_JSON = ".dirsync.json";

class JsonDirConfigReader : public DirectoryConfigurationReader {
	public:
	DirectoryConfigurationReadResult read_from_directory(
		const std::filesystem::directory_entry &directory,
		const ProgramArguments &arguments
	) const override;

	virtual ~JsonDirConfigReader() = default;
};

#endif //DIRSYNC_CONFIGURATION_JSON_HPP
