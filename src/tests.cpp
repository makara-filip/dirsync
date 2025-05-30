#include "tests.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include "arguments.hpp"
#include "json.hpp"
#include "synchronize.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

void create_file(const fs::path &path, const std::string &content = "") {
	fs::create_directories(path.parent_path());
	std::ofstream file(path);
	file << content;
}

void create_large_file(const fs::path &path, const unsigned int size) {
	fs::create_directories(path.parent_path());
	std::ofstream file(path);
	for (size_t i = 0; i < size; i++)
		file << 'x';
}

bool file_equals(const fs::path &file1, const fs::path &file2) {
	std::ifstream f1(file1), f2(file2);
	std::string line1, line2;
	while (std::getline(f1, line1) && std::getline(f2, line2)) {
		if (line1 != line2) return false;
	}
	return f1.eof() && f2.eof();
}

bool file_content_equals(const fs::path &file, const std::string &content) {
	std::ifstream file_stream(file);
	std::stringstream str_stream(content);
	std::string line, line2;
	while (std::getline(file_stream, line) && std::getline(str_stream, line2)) {
		if (line != line2) return false;
	}
	return file_stream.eof() && str_stream.eof();
}

void remove_recursively(const fs::path &path) {
	std::error_code ec;
	fs::remove_all(path, ec);
}

const static std::string old_version_content = "old version";
const std::string new_version_content = "new version";

class Test {
	protected:
	const fs::path common_parent = "test";
	const fs::path source = common_parent / "source";
	const fs::path target = common_parent / "target";
	int result = 0;

	Test() = default;

	public:
	virtual void prepare() = 0;
	virtual void perform() = 0;
	virtual void assert_validity() = 0;
	virtual void cleanup() = 0;

	virtual ~Test() noexcept = default;
};

class SimpleOneWayTest final : public Test {
	public:
	void prepare() override {
		remove_recursively(source);
		remove_recursively(target);

		create_file(source / "root.txt", "root");
		create_file(source / "hello.ignored.txt", "hello");
		create_file(source / "first" / "recursive.ignored.txt");
		create_file(source / "ignored-directory" / "ignored-by-parent.txt");
		const json source_config = {
			{
				"configVersion", {
					{"major", 0},
					{"minor", 0},
					{"patch", 0},
				}
			},
			{"maxFileSize", 100},
			{"exclusionPatterns", {"*.ignored.txt", "ignored-directory"}},
		};
		std::ofstream file(source / ".dirsync.json");
		file << source_config;
		file.close();

		create_file(target / "conflicts" / "different.txt", old_version_content);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		create_file(source / "conflicts" / "different.txt", new_version_content);

		create_file(source / "conflicts" / "skip-older.txt", old_version_content);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		create_file(target / "conflicts" / "skip-older.txt", new_version_content);
	}

	void perform() override {
		ProgramArgumentsBuilder builder;
		builder.set_source_directory(source);
		builder.set_target_directory(target);
		builder.set_verbosity(true);

		const ProgramArguments args = builder.build();

		result = synchronize_directories(args);
	}

	void assert_validity() override {
		assert(result == 0);

		assert(fs::exists(target / "root.txt"));
		assert(file_equals(source / "root.txt", target / "root.txt"));

		assert(!fs::exists(target / "hello.ignored.txt"));
		assert(!fs::exists(target/ "first" / "recursive.ignored.txt"));
		assert(!fs::exists(target/ "ignored-directory" / "ignored-by-parent.txt"));

		assert(file_content_equals(target / "conflicts" / "different.txt", new_version_content));
		assert(file_equals(
			source / "conflicts" / "different.txt",
			target / "conflicts" / "different.txt"
		));

		assert(file_content_equals(target / "conflicts" / "skip-older.txt", new_version_content));
	}

	void cleanup() override {
		remove_recursively(source);
		remove_recursively(target);
	}
};

class SimpleTwoWayTest final : public Test {
	const fs::path file_in_source_only = source / "source-only.txt";
	const fs::path file_in_target_only = target / "target-only.txt";

	const std::string common_filename = "common.txt";
	const fs::path file_in_both_older = source / common_filename;
	const fs::path file_in_both_newer = target / common_filename;

	public:
	void prepare() override {
		remove_recursively(source);
		remove_recursively(target);

		create_file(file_in_both_older, old_version_content);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		create_file(file_in_both_newer, new_version_content);

		create_file(file_in_source_only);
		create_file(file_in_target_only);
	}

	void perform() override {
		ProgramArgumentsBuilder builder;
		builder.set_source_directory(source)
			.set_target_directory(target)
			.set_two_way()
			.set_verbosity(true);

		const ProgramArguments args = builder.build();
		result = synchronize_directories(args);
	}

	void assert_validity() override {
		assert(result == 0);

		// assert the existence of asymmetrical files
		assert(fs::exists(source / file_in_target_only.filename()));
		assert(fs::exists(target / file_in_source_only.filename()));

		// newer version should overwrite the older one
		assert(file_equals(file_in_both_older, file_in_both_newer));
		assert(file_content_equals(file_in_both_older, new_version_content));
	}

	void cleanup() override {
		remove_recursively(source);
		remove_recursively(target);
	}
};

class ConflictRenamingTest final : public Test {
	const std::string common_filename = "common.txt";
	const fs::path common_file_source = source / common_filename;
	const fs::path common_file_target = target / common_filename;

	fs::file_time_type older_file_time;

	public:
	void prepare() override {
		remove_recursively(source);
		remove_recursively(target);

		create_file(common_file_target, old_version_content);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		create_file(common_file_source, new_version_content);

		older_file_time = fs::last_write_time(common_file_target);
	}

	void perform() override {
		ProgramArgumentsBuilder builder;
		builder.set_source_directory(source);
		builder.set_target_directory(target);
		builder.set_conflict_resolution(ConflictResolutionMode::rename);
		builder.set_verbosity(true);

		const ProgramArguments args = builder.build();

		result = synchronize_directories(args);
	}

	void assert_validity() override {
		assert(result == 0);

		// build the expected renamed path for the older file (left one)
		const std::string timestamp = get_formatted_time(older_file_time);
		const fs::path file_with_timestamp = target / ("common-" + timestamp + ".txt");

		assert(fs::exists(file_with_timestamp));
		assert(file_content_equals(file_with_timestamp, new_version_content));

		// keeps the original source file:
		assert(file_content_equals(common_file_source, new_version_content));
		// keeps the old version:
		assert(file_content_equals(common_file_target, old_version_content));
		assert(fs::exists(file_with_timestamp));
		assert(file_content_equals(file_with_timestamp, new_version_content));
	}
	void cleanup() override {
		remove_recursively(source);
		remove_recursively(target);
	}
};

class MaxFileSizeTest final : public Test {
	const fs::path large_file_source = source / "large.txt";
	const fs::path large_file_target_should_not_exist = target / "large.txt";

	public:
	void prepare() override {
		remove_recursively(source);
		remove_recursively(target);

		create_large_file(large_file_source, 50); // 50 byte long file

		const json target_config = {
			{
				"configVersion", {
					{"major", 0},
					{"minor", 0},
					{"patch", 0},
				}
			},
			{"exclusionPatterns", json::array()},
			{"maxFileSize", 20},
		};
		create_directory(large_file_target_should_not_exist.parent_path());
		std::ofstream file(target / ".dirsync.json");
		if (!file.good()) {
			std::cerr << "Dirsync configuration could not be saved." << std::endl;
			return;
		}
		file << target_config;
		file.close();
	}

	void perform() override {
		ProgramArgumentsBuilder builder;
		builder.set_source_directory(source);
		builder.set_target_directory(target);
		builder.set_verbosity(true);

		const ProgramArguments args = builder.build();

		result = synchronize_directories(args);
	}

	void assert_validity() override {
		assert(result == 0);

		// the large file should not be copied, because the target does not accept such large files
		assert(!fs::exists(large_file_target_should_not_exist));
	}

	void cleanup() override {
		remove_recursively(source);
		remove_recursively(target);
	}
};

void perform_single_test(Test &test) {
	test.prepare();
	test.perform();
	test.assert_validity();
	test.cleanup();
}

int run_tests() {
	std::cout << "Test 1: simple one-way synchronization with default settings" << std::endl;
	SimpleOneWayTest test;
	perform_single_test(test);

	std::cout << "Test 2: simple two-way synchronization with default settings" << std::endl;
	SimpleTwoWayTest test2;
	perform_single_test(test2);

	std::cout << "Test 3: one-way synchronization with conflict renaming" << std::endl;
	ConflictRenamingTest test3;
	perform_single_test(test3);

	std::cout << "Test 4: too large files are not accepted in target" << std::endl;
	MaxFileSizeTest test4;
	perform_single_test(test4);

	return 0;
}
