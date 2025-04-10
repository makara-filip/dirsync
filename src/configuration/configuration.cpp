#include "configuration.hpp"

#include <fstream>

#include "configuration-json.hpp"
#include "../arguments.hpp"

namespace fs = std::filesystem;
using Reader = DirectoryConfigurationReader;
using Result = DirectoryConfigurationReadResult;

bool is_config_file(const fs::path &path) {
	return path.filename().string().starts_with(CONFIG_FILE_NAME_PREFIX);
}

const Reader *supported_readers[1] = {
	dynamic_cast<Reader *>(new JsonDirConfigReader())
};

int get_directory_configuration(
	const fs::directory_entry &directory,
	const ProgramArguments &arguments,
	std::optional<DirectoryConfiguration> &configuration
) {
	for (const Reader *reader : supported_readers) {
		Result result = reader->read_from_directory(directory, arguments);

		if (std::holds_alternative<DirectoryConfigurationParseError>(result)) {
			const fs::path config_file_path = directory.path() / reader->config_file_name();
			std::cerr << "Parse error in " << config_file_path << std::endl;
			if (arguments.dry_run) break;
			return EXIT_CODE_CONFIG_FILE_PARSE_ERROR;
		}

		const DirectoryConfiguration *config = std::get_if<DirectoryConfiguration>(&result);
		if (config == nullptr) continue;
		configuration = *config;
		return 0;
	}
	return 0;
}
