#include "configuration.hpp"

#include <fstream>

#include "configuration-json.hpp"
#include "../arguments.hpp"

namespace fs = std::filesystem;
using Reader = DirectoryConfigurationReader;
using Result = DirectoryConfigurationReadResult;

const Reader *supported_readers[1] = {
	dynamic_cast<Reader *>(new JsonDirConfigReader())
};

std::optional<DirectoryConfiguration> get_directory_configuration(
	const fs::directory_entry &directory,
	const ProgramArguments &arguments
) {
	for (const Reader *reader : supported_readers) {
		Result result = reader->read_from_directory(directory, arguments);

		const DirectoryConfiguration *config = std::get_if<DirectoryConfiguration>(&result);
		if (config == nullptr) continue;

		return *config;
	}
	return {};
}
