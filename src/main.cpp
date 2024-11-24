#include <iostream>
#include <fstream>

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


//	std::cout << "Opening and parsing a JSON file..." << std::endl;
//	std::ifstream f("test-parsing.json");
//	json data = json::parse(f);

	test_edit_json();




	return 0;
}
