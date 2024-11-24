#include <iostream>
#include <fstream>
#include <filesystem>

#include "json.hpp"

using json = nlohmann::json;

struct DirectoryConfiguration {
	std::string version;
	unsigned long testNumber;
};

void from_json(const json& j, DirectoryConfiguration& p) {
	j.at("version").get_to(p.version);
	j.at("testNumber").get_to(p.testNumber);
	//	j.at("age").get_to(p.age);
}
void to_json(json& j, const DirectoryConfiguration& p) {
	j = json{{"version", p.version}, {"testNumber", p.testNumber}};
}

void test_edit_json() {

	std::string filename = "test-parsing.json";
	std::ifstream input_file(filename);
	if (!input_file.good()) {
		std::cerr << "File error" << std::endl;
		return;
	}

	json data = json::parse(input_file);

	DirectoryConfiguration content = data.template get<DirectoryConfiguration>();
	input_file.close();

	content.testNumber++;

	json serialized = content;
	std::ofstream output_file(filename);
	output_file << serialized;
}

int main () {
	std::cout << "Hello, World!" << std::endl;

	std::filesystem::path cwd = std::filesystem::current_path();
	std::cout << "CWD:" << cwd << std::endl;

	test_edit_json();

	return 0;
}
