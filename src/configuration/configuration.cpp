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

/** A list of supported configuration reader instances,
 * one instance per file type (JSON, XML, etc.). */
const Reader *supported_readers[1] = {
	dynamic_cast<Reader *>(new JsonDirConfigReader())
};

/** Performs a high-level multi-format local configuration reading and parsing.
 * This function tries every supported file format given by `supported_readers`.
 * Checks the config version upon parsing.
 * If invalid version or a parse error, an error is written to standard error stream.
 * @param directory the directory in which to load a local configuration
 * @param arguments the processed CLI program arguments
 * @param configuration Output parameter of the configuration. Has no value
 * if no supported configuration file was present or an error occurred.
 * @return A program-wide error code. If none occurs, defaults to zero. */
int get_directory_configuration(
	const fs::path &directory,
	const ProgramArguments &arguments,
	std::optional<DirectoryConfiguration> &configuration
) {
	for (const Reader *reader : supported_readers) {
		Result result = reader->read_from_directory(directory, arguments);

		if (std::holds_alternative<DirectoryConfigurationParseError>(result)) {
			const fs::path config_file_path = directory / reader->config_file_name();
			std::cerr << "Parse error in " << config_file_path << std::endl;
			return EXIT_CODE_CONFIG_FILE_PARSE_ERROR;
		} else if (std::holds_alternative<DirectoryConfigurationIncompatible>(result)) {
			const fs::path config_file_path = directory / reader->config_file_name();
			std::cerr << "Incompatible configuration in " << config_file_path << std::endl;
			return EXIT_CODE_CONFIG_VERSION_INCOMPATIBLE;
		}

		const DirectoryConfiguration *config = std::get_if<DirectoryConfiguration>(&result);
		if (config == nullptr) continue;
		configuration = *config;
		return 0;
	}
	return 0;
}
