#include "configuration-json.hpp"
#include <filesystem>
#include <fstream>
#include <system_error>

#include "configuration.hpp"

namespace fs = std::filesystem;
using Json = nlohmann::json;

AUTOGENERATE_JSON_CONVERSION(Version, major, minor, patch)

const char *CONFIGURATION_VERSION_KEY = "configVersion";

void from_json(const Json &j, DirectoryConfiguration &p) {
	j.at(CONFIGURATION_VERSION_KEY).get_to(p.config_version);
	j.at("exclusionPatterns").get_to(p.exclusion_patterns);
	if (j.contains("maxFileSize")) {
		const std::int64_t *max_file_size_ptr = j.at("maxFileSize").get_ptr<const std::int64_t *>();
		if (max_file_size_ptr != nullptr)
			p.max_file_size = *max_file_size_ptr;
	}
}
void to_json(Json &j, const DirectoryConfiguration &p) {
	j = Json{
		{CONFIGURATION_VERSION_KEY, p.config_version},
		{"exclusionPatterns", p.exclusion_patterns},
		{"maxFileSize", nullptr}
	};

	if (p.max_file_size.has_value())
		j["maxFileSize"] = *p.max_file_size;
}

DirectoryConfigurationReadResult JsonDirConfigReader::read_from_directory(
	const std::filesystem::directory_entry &directory,
	const ProgramArguments &arguments
) const {
	const fs::path file_path = directory.path() / config_file_name();
	std::error_code error;
	const fs::file_status file_status = fs::status(file_path, error);
	if (!fs::exists(file_status))
		return DirectoryConfigurationFileNonexistent{};

	if (error) {
		if (arguments.verbose)
			std::cerr << "Error: Failed to check the directory configuration details in: "
				<< file_path << ": " << error.message() << std::endl;
		return fs::filesystem_error("Failed to check the directory configuration details", error);
	}

	std::ifstream file_stream(file_path);
	if (!file_stream.good())
		return DirectoryConfigurationFileNonexistent{};

	try {
		const Json json = Json::parse(file_stream);

		Version config_version = json.at(CONFIGURATION_VERSION_KEY);
		if (!config_version.is_compatible_with(PROGRAM_VERSION))
			return DirectoryConfigurationIncompatible{};

		DirectoryConfiguration result = json.get<DirectoryConfiguration>();
		return result;
	} catch (const Json::exception &) {
		return DirectoryConfigurationParseError{};
	}
}
