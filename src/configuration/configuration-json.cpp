#include "configuration-json.hpp"
#include <filesystem>
#include <fstream>
#include <system_error>

#include "configuration.hpp"

namespace fs = std::filesystem;
using Json = nlohmann::json;

AUTOGENERATE_JSON_CONVERSION(Version, major, minor, patch)

void from_json(const Json &j, DirectoryConfiguration &p) {
	j.at("configVersion").get_to(p.config_version);
}
void to_json(Json &j, const DirectoryConfiguration &p) {
	j = Json{{"configVersion", p.config_version}};
}

DirectoryConfigurationReadResult JsonDirConfigReader::read_from_directory(
	const std::filesystem::directory_entry &directory,
	const ProgramArguments &arguments
) const {
	const fs::path file_path = directory.path() / DIRECTORY_CONFIG_FILE_JSON;
	std::error_code error;
	const fs::file_status file_status = fs::status(file_path, error);
	if (error) {
		if (arguments.verbose)
			std::cerr << "Error: Failed to check the directory configuration details in: "
				<< file_path << std::endl
				<< "-> system error: " << error.message() << std::endl;
		return fs::filesystem_error("Failed to check the directory configuration details", error);
	}
	if (!fs::exists(file_status))
		return DirectoryConfigurationFileNonexistent{};

	std::ifstream file_stream(file_path);
	if (!file_stream.good())
		return DirectoryConfigurationFileNonexistent{};

	const Json json = Json::parse(file_stream, nullptr, false);
	if (json.is_discarded())
		return DirectoryConfigurationParseError{};

	try {
		DirectoryConfiguration result = json.get<DirectoryConfiguration>();
		return result;
	} catch (Json::parse_error &) {
		return DirectoryConfigurationParseError{};
	}

	return DirectoryConfigurationParseError{};
}
