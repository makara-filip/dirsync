#include "tests.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

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

constexpr std::string old_version_content = "old version";
constexpr std::string new_version_content = "new version";

class Test {
	public:
	const fs::path source = "test/source";
	const fs::path target = "test/target";
	int result = 0;

	Test() = default;

	virtual void prepare();
	virtual void perform();
	virtual void assert_validity();
	virtual void cleanup();

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
		create_file(source / "conflicts" / "different.txt", new_version_content);

		create_file(source / "conflicts" / "skip-older.txt", old_version_content);
		create_file(target / "conflicts" / "skip-older.txt", new_version_content);
	}

	void perform() override {
		ProgramArguments args;
		args.source_directory = source.string();
		args.target_directory = target.string();
		args.is_one_way_synchronization = true;
		args.verbose = true;

		result = synchronize_directories(args);
	};

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

int run_tests() {
	SimpleOneWayTest test;
	test.prepare();
	test.perform();
	test.assert_validity();
	test.cleanup();

	return 0;
}
