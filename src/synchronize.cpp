#include "synchronize.hpp"
#include "constants.hpp"

#include <filesystem>
#include <iostream>

#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

int verify_source_directory(const fs::path &path, fs::directory_entry &directory, fs::file_status &status) {
	try {
		if (!fs::exists(path)) {
			std::cerr << "Source directory does not exist." << std::endl;
			return EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY;
		}
		directory = fs::directory_entry(path);
		status = directory.status();
	} catch (fs::filesystem_error &error) {
		std::cerr << "Failed to check the source directory details." << std::endl
			<< "System error: " << error.what() << std::endl;
		return EXIT_CODE_FILESYSTEM_ERROR;
	}

	if (!fs::is_directory(status)) {
		std::cerr << "Error: Source path is not a directory." << std::endl;
		return EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY;
	}

	return 0;
}

int ensure_target_directory(const fs::path &path, fs::directory_entry &directory, fs::file_status &status) {
	std::error_code error;
	fs::create_directories(path, error);
	if (error) {
		std::cerr << "Error: Failed to ensure the target directory existence." << std::endl;
		std::cerr << "System error: " << error.message() << std::endl;
		return EXIT_CODE_FILESYSTEM_ERROR;
	}
	directory = fs::directory_entry(path, error);
	if (error) return EXIT_CODE_FILESYSTEM_ERROR;
	status = directory.status();
	return 0;
}

int synchronize_directories(const ProgramArguments &arguments) {
	const fs::path source_path(arguments.source_directory);
	const fs::path target_path(arguments.target_directory);

	fs::directory_entry source_directory, target_directory;
	fs::file_status source_status, target_status;

	int error;
	error = verify_source_directory(source_path, source_directory, source_status);
	if (error) return error;
	error = ensure_target_directory(target_path, target_directory, target_status);
	if (error) return error;

	// iterate over all directory entries, but the order is not specified
	for (const fs::directory_entry &dir_entry : fs::recursive_directory_iterator(source_directory)) {
		std::cout << dir_entry << '\n';

		std::error_code err;
		const fs::file_status status = fs::status(dir_entry, err);
		if (err) {
			std::cerr << "Failed to check file status of " << dir_entry << std::endl;
			continue;
		}

		// Now, distinguis between directories and files,
		// whether to copy them or not.

		// if (fs::is_directory(status)) {
		// 	std::optional<DirectoryConfiguration> configuration;
		// 	int config_error = get_directory_configuration(dir_entry, arguments, configuration);
		// 	if (config_error) continue;
		//
		// 	// if (!configuration.has_value()) {
		// 	// 	// proceed with default copy instructions
		// 	// 	// fs::copy(dir_entry.path(), destination...)
		// 	// } else {
		// 	// 	// treat special copy instructions
		// 	// }
		// }
	}

	return 0;
}
