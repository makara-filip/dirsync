#include "help.hpp"

#include <iostream>

const char *HELP =
	"Usage: dirsync [--help] [source-directory] [target-directory]\n"
	"\n"
	"The dirsync utility recursively synchronizes or copies files from the source\n"
	"directory to the target directory.\n";

void print_help() {
	std::cout << HELP << std::endl;
}
