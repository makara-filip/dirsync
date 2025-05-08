#include <string>
#include <vector>

#include "arguments.hpp"
#include "constants.hpp"
#include "help.hpp"
#include "synchronize.hpp"
#include "tests.hpp"

int main(const int argc, char **argv) {
	const std::vector<std::string> args(argv, argv + argc);
	const std::optional<ProgramArguments> arguments = ProgramArguments::try_parse(args);
	if (!arguments.has_value())
		return EXIT_CODE_INCORRECT_USAGE;

	const ProgramMode mode = arguments->get_mode();
	if (mode == ProgramMode::help) {
		print_help();
	} else if (mode == ProgramMode::test) {
		return run_tests();
	} else if (mode == ProgramMode::synchronize) {
		return synchronize_directories(*arguments);
	}

	return 0;
}
