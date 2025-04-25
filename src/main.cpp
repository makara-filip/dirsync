#include <string>
#include <vector>

#include "arguments.hpp"
#include "constants.hpp"
#include "help.hpp"
#include "synchronize.hpp"
#include "tests.hpp"

int main(const int argc, char **argv) {
	const std::vector<std::string> args(argv, argv + argc);
	ProgramArguments arguments;
	if (!arguments.try_parse(args))
		return EXIT_CODE_INCORRECT_USAGE;

	if (arguments.mode == ProgramMode::help) {
		print_help();
	} else if (arguments.mode == ProgramMode::test) {
		return run_tests();
	} else if (arguments.mode == ProgramMode::synchronize) {
		return synchronize_directories(arguments);
	}

	return 0;
}
