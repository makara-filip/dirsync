#include "synchronize.hpp"
#include "constants.hpp"

#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

#include "synchronize_one_way.hpp"
#include "synchronize_two_way.hpp"
#include "configuration/configuration.hpp"

namespace fs = std::filesystem;

/** Formats the filesystem-related time to human-readable civil format.
 * @return the formatted string represented the input time */
std::string get_formatted_time(const fs::file_time_type &time) {
	using namespace std::chrono;
	const time_point<file_clock, seconds> time_in_seconds = time_point_cast<seconds>(time);
	return std::format("{:%F-%H-%M-%S}", time_in_seconds);
}

/** Converts the filesystem-related time to another instance of the same file clock,
 * but with fixed duration of `std::chrono::seconds`. */
std::chrono::file_time<std::chrono::seconds> reduce_precision_to_seconds(const fs::file_time_type &file_time) {
	using namespace std::chrono;
	return time_point_cast<seconds>(file_time);
}

/** Verifies the given path is an existing directory.
 * @param path the source directory path
 * @param directory output parameter of the directory entry representing information about the source
 * @param status output parameter of the file status
 * @return A program-wide error code. If none occurs, default to zero. */
int verify_source_directory(const fs::path &path, fs::directory_entry &directory, fs::file_status &status) {
	try {
		if (!fs::exists(path)) {
			std::cerr << "Source directory does not exist." << std::endl;
			return EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY;
		}
		directory = fs::directory_entry(path);
		status = directory.status();
	} catch (fs::filesystem_error &error) {
		std::cerr << "Failed to check the source directory details." << error.what() << std::endl;
		return EXIT_CODE_FILESYSTEM_ERROR;
	}

	if (!fs::is_directory(status)) {
		std::cerr << "Error: Source path is not a directory." << std::endl;
		return EXIT_CODE_NONEXISTENT_SOURCE_DIRECTORY;
	}

	return 0;
}

/** Ensures the target directory exists. If it does not, the function creates it.
 * @param path the target directory path
 * @param directory output parameter of the directory entry representing information about the target
 * @param status output parameter of the file status
 * @return A program-wide error code. If none occurs, default to zero. */
int ensure_target_directory(const fs::path &path, fs::directory_entry &directory, fs::file_status &status) {
	std::error_code error;
	fs::create_directories(path, error);
	if (error) {
		std::cerr << "Error: Failed to ensure the target directory existence." << error.message() << std::endl;
		return EXIT_CODE_FILESYSTEM_ERROR;
	}
	directory = fs::directory_entry(path, error);
	if (error) return EXIT_CODE_FILESYSTEM_ERROR;
	status = directory.status();
	return 0;
}

/** Creates a new string by appending a dash and a formatted last write time, keeping the file extension (if any).
 * @param entry file or directory whose name and last write time is used to compose the new filename */
std::string insert_timestamp_to_filename(const fs::directory_entry &entry) {
	return entry.path().stem().string()
		+ "-"
		+ get_formatted_time(entry.last_write_time())
		+ entry.path().extension().string();
}

/** Given the program CLI arguments, delegates the work to one-way-specific or two-way-specific
 * synchronization functions. Makes sure the source and target directories are valid.
 * Prepares the recursive synchronization Context, either MonodirectionalContext or BidirectionalContext.
 * @param arguments the processed CLI arguments, dictating synchronization details
 * @return An error code. If none occurs, defaults to zero. */
int synchronize_directories(const ProgramArguments &arguments) {
	const fs::path source_path = arguments.get_source_path();
	const fs::path target_path = arguments.get_target_path();

	fs::directory_entry source_directory, target_directory;
	fs::file_status source_status, target_status;

	int error = 0;
	if (arguments.is_one_way()) {
		error = verify_source_directory(source_path, source_directory, source_status);
		if (error) return error;
		error = ensure_target_directory(target_path, target_directory, target_status);
		if (error) return error;

		MonodirectionalContext context(arguments);
		MonodirectionalSynchronizer synchronizer(context);
		error = synchronizer.synchronize();
	} else {
		// we do not have the source and target directories, we have two source ones

		error = verify_source_directory(source_path, source_directory, source_status);
		if (error) return error;
		error = verify_source_directory(target_path, target_directory, target_status);
		if (error) return error;

		BidirectionalContext context(arguments);
		BidirectionalSynchronizer synchronizer(context);
		error = synchronizer.synchronize();
	}

	return error;
}

