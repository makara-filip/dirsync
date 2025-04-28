#include "synchronize.hpp"
#include "constants.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "synchronize_one_way.hpp"
#include "synchronize_two_way.hpp"
#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

std::string get_formatted_time(const fs::file_time_type &time) {
	using namespace std::chrono;
	const time_point<file_clock, seconds> time_in_seconds = time_point_cast<seconds>(time);
	return std::format("{:%F-%H-%M-%S}", time_in_seconds);
}

std::chrono::file_time<std::chrono::seconds> reduce_precision_to_seconds(const fs::file_time_type &file_time) {
	using namespace std::chrono;
	return time_point_cast<seconds>(file_time);
}

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

std::string insert_timestamp_to_filename(const fs::directory_entry &entry) {
	return entry.path().stem().string()
		+ "-"
		+ get_formatted_time(entry.last_write_time())
		+ entry.path().extension().string();
}

int synchronize_directories(const ProgramArguments &arguments) {
	const fs::path source_path(arguments.source_directory);
	const fs::path target_path(arguments.target_directory);

	fs::directory_entry source_directory, target_directory;
	fs::file_status source_status, target_status;

	int error = 0;
	if (arguments.is_one_way_synchronization) {
		error = verify_source_directory(source_path, source_directory, source_status);
		if (error) return error;
		error = ensure_target_directory(target_path, target_directory, target_status);
		if (error) return error;

		MonodirectionalContext context(&source_directory, &target_directory);
		error = synchronize_directories_recursively(arguments, context, source_directory, target_directory);
	} else {
		// we do not have source and target directories, we have two source ones

		error = verify_source_directory(source_path, source_directory, source_status);
		if (error) return error;
		error = verify_source_directory(target_path, target_directory, target_status);
		if (error) return error;

		fs::directory_entry source_left = source_directory;
		fs::directory_entry source_right = target_directory;

		BidirectionalContext context(&source_left, &source_right);
		error = synchronize_directories_bidirectionally(arguments, context, source_left, source_right);
	}

	return error;
}

