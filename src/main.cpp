#include <iostream>
#include <string>
#include <vector>

//#include "json.hpp"
#include "arguments.hpp"
#include "help.hpp"
#include "synchronize.hpp"

int main(int argc, char **argv) {
	std::vector<std::string> args(argv, argv + argc);
	ProgramArguments arguments;
	if (!arguments.try_parse(args))
		return EXIT_CODE_INCORRECT_USAGE;

	if (arguments.mode == ProgramMode::help) {
		print_help();
	} else if (arguments.mode == ProgramMode::synchronize) {
		int code = synchronize_directories(arguments);
		return code;
	}

	return 0;
}