#include "synchronize.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int verify_source_directory(const fs::path &path, fs::directory_entry &directory, fs::file_status &status) {
	try {
		if (!fs::exists(path)) {
			std::cerr << "Error: Source directory does not exist." << std::endl;
			return EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY;
		}
		directory = fs::directory_entry(path);
		status = directory.status();
	} catch (fs::filesystem_error &error) {
		std::cerr << "Error: Failed to check the source directory details." << std::endl
				  << "System error: " << error.what() << std::endl;
		return EXIT_CODE_FILESYSTEM_ERROR;
	}

	if (!fs::is_directory(status)) {
		std::cerr << "Error: Source path is not a directory." << std::endl;
		return EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY;
	}

	return 0;
}

int synchronize_directories(const ProgramArguments &arguments) {
	const fs::path source_path(arguments.source_directory);
	const fs::path target_directory(arguments.target_directory);

	fs::directory_entry source_directory;
	fs::file_status source_status;
	int code = verify_source_directory(source_path, source_directory, source_status);
	if (code)
		return code;

	// iterate over all directory entries, but the order is not specified
	for (const fs::directory_entry &dir_entry : fs::recursive_directory_iterator(source_directory))
		std::cout << dir_entry << '\n';

	return 0;
}
